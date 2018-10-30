/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "modules/mediasource/MediaSource.h"

#include <memory>
#include "bindings/core/v8/ExceptionMessages.h"
#include "bindings/core/v8/ExceptionState.h"
#include "core/dom/ExceptionCode.h"
#include "core/dom/events/Event.h"
#include "core/dom/events/MediaElementEventQueue.h"
#include "core/frame/Deprecation.h"
#include "core/frame/UseCounter.h"
#include "core/html/HTMLMediaElement.h"
#include "core/html/track/AudioTrackList.h"
#include "core/html/track/VideoTrackList.h"
#include "modules/mediasource/MediaSourceRegistry.h"
#include "modules/mediasource/SourceBufferTrackBaseSupplement.h"
#include "platform/instrumentation/tracing/TraceEvent.h"
#include "platform/network/mime/ContentType.h"
#include "platform/network/mime/MIMETypeRegistry.h"
#include "platform/runtime_enabled_features.h"
#include "platform/wtf/PtrUtil.h"
#include "platform/wtf/text/CString.h"
#include "public/platform/WebMediaSource.h"
#include "public/platform/WebSourceBuffer.h"

#if defined(OS_TIZEN_TV_PRODUCT)
#include <capi-system-info/system_info.h>
#endif

#ifndef BLINK_MSLOG
#define BLINK_MSLOG DVLOG(3)
#endif

using blink::WebMediaSource;
using blink::WebSourceBuffer;

namespace blink {

static bool ThrowExceptionIfClosed(bool is_open,
                                   ExceptionState& exception_state) {
  if (!is_open) {
    MediaSource::LogAndThrowDOMException(
        exception_state, kInvalidStateError,
        "The MediaSource's readyState is not 'open'.");
    return true;
  }

  return false;
}

static bool ThrowExceptionIfClosedOrUpdating(bool is_open,
                                             bool is_updating,
                                             ExceptionState& exception_state) {
  if (ThrowExceptionIfClosed(is_open, exception_state))
    return true;

  if (is_updating) {
    MediaSource::LogAndThrowDOMException(exception_state, kInvalidStateError,
                                         "The 'updating' attribute is true on "
                                         "one or more of this MediaSource's "
                                         "SourceBuffers.");
    return true;
  }

  return false;
}

const AtomicString& MediaSource::OpenKeyword() {
  DEFINE_STATIC_LOCAL(const AtomicString, open, ("open"));
  return open;
}

const AtomicString& MediaSource::ClosedKeyword() {
  DEFINE_STATIC_LOCAL(const AtomicString, closed, ("closed"));
  return closed;
}

const AtomicString& MediaSource::EndedKeyword() {
  DEFINE_STATIC_LOCAL(const AtomicString, ended, ("ended"));
  return ended;
}

MediaSource* MediaSource::Create(ExecutionContext* context) {
  return new MediaSource(context);
}

MediaSource::MediaSource(ExecutionContext* context)
    : ContextLifecycleObserver(context),
      ready_state_(ClosedKeyword()),
      async_event_queue_(MediaElementEventQueue::Create(this, context)),
      attached_element_(nullptr),
      source_buffers_(SourceBufferList::Create(GetExecutionContext(),
                                               async_event_queue_.Get())),
      active_source_buffers_(
          SourceBufferList::Create(GetExecutionContext(),
                                   async_event_queue_.Get())),
      live_seekable_range_(TimeRanges::Create()),
      added_to_registry_counter_(0) {
  BLINK_MSLOG << __func__ << " this=" << this;
}

MediaSource::~MediaSource() {
  BLINK_MSLOG << __func__ << " this=" << this;
  DCHECK(IsClosed());
}

void MediaSource::LogAndThrowDOMException(ExceptionState& exception_state,
                                          ExceptionCode error,
                                          const String& message) {
  BLINK_MSLOG << __func__ << " (error=" << error << ", message=" << message
              << ")";
  exception_state.ThrowDOMException(error, message);
}

void MediaSource::LogAndThrowTypeError(ExceptionState& exception_state,
                                       const String& message) {
  BLINK_MSLOG << __func__ << " (message=" << message << ")";
  exception_state.ThrowTypeError(message);
}

SourceBuffer* MediaSource::addSourceBuffer(const String& type,
                                           ExceptionState& exception_state) {
  BLINK_MSLOG << __func__ << " this=" << this << " type=" << type;

  // 2.2
  // https://www.w3.org/TR/media-source/#widl-MediaSource-addSourceBuffer-SourceBuffer-DOMString-type/
  // 1. If type is an empty string then throw a TypeError exception
  //    and abort these steps.
  if (type.IsEmpty()) {
    LogAndThrowTypeError(exception_state, "The type provided is empty");
    return 0;
  }

  // 2. If type contains a MIME type that is not supported ..., then throw a
  // NotSupportedError exception and abort these steps.
  if (!isTypeSupported(type)) {
    LogAndThrowDOMException(
        exception_state, kNotSupportedError,
        "The type provided ('" + type + "') is unsupported.");
    return 0;
  }

  // 4. If the readyState attribute is not in the "open" state then throw an
  // InvalidStateError exception and abort these steps.
  if (!IsOpen()) {
    LogAndThrowDOMException(exception_state, kInvalidStateError,
                            "The MediaSource's readyState is not 'open'.");
    return 0;
  }

  // 5. Create a new SourceBuffer object and associated resources.
  ContentType content_type(type);
  String codecs = content_type.Parameter("codecs");
  std::unique_ptr<WebSourceBuffer> web_source_buffer =
      CreateWebSourceBuffer(content_type.GetType(), codecs, exception_state);

  if (!web_source_buffer) {
    DCHECK(exception_state.Code() == kNotSupportedError ||
           exception_state.Code() == kQuotaExceededError);
    // 2. If type contains a MIME type that is not supported ..., then throw a
    //    NotSupportedError exception and abort these steps.
    // 3. If the user agent can't handle any more SourceBuffer objects then
    //    throw a QuotaExceededError exception and abort these steps
    return 0;
  }

  SourceBuffer* buffer = SourceBuffer::Create(std::move(web_source_buffer),
                                              this, async_event_queue_.Get());
  // 6. Add the new object to sourceBuffers and fire a addsourcebuffer on that
  //    object.
  source_buffers_->Add(buffer);

  // 7. Return the new object to the caller.
  BLINK_MSLOG << __func__ << " this=" << this << " type=" << type << " -> "
              << buffer;
  return buffer;
}

void MediaSource::removeSourceBuffer(SourceBuffer* buffer,
                                     ExceptionState& exception_state) {
  BLINK_MSLOG << __func__ << " this=" << this << " buffer=" << buffer;

  // 2.2
  // https://dvcs.w3.org/hg/html-media/raw-file/default/media-source/media-source.html#widl-MediaSource-removeSourceBuffer-void-SourceBuffer-sourceBuffer

  // 1. If sourceBuffer specifies an object that is not in sourceBuffers then
  //    throw a NotFoundError exception and abort these steps.
  if (!source_buffers_->length() || !source_buffers_->Contains(buffer)) {
    LogAndThrowDOMException(
        exception_state, kNotFoundError,
        "The SourceBuffer provided is not contained in this MediaSource.");
    return;
  }

  // Steps 2-8 are implemented by SourceBuffer::removedFromMediaSource.
  buffer->RemovedFromMediaSource();

  // 9. If sourceBuffer is in activeSourceBuffers, then remove sourceBuffer from
  //    activeSourceBuffers ...
  active_source_buffers_->Remove(buffer);

  // 10. Remove sourceBuffer from sourceBuffers and fire a removesourcebuffer
  //     event on that object.
  source_buffers_->Remove(buffer);

  // 11. Destroy all resources for sourceBuffer.
  //     This should have been done already by
  //     SourceBuffer::removedFromMediaSource (steps 2-8) above.
}

void MediaSource::OnReadyStateChange(const AtomicString& old_state,
                                     const AtomicString& new_state) {
  if (IsOpen()) {
    ScheduleEvent(EventTypeNames::sourceopen);
    return;
  }

  if (old_state == OpenKeyword() && new_state == EndedKeyword()) {
    ScheduleEvent(EventTypeNames::sourceended);
    return;
  }

  DCHECK(IsClosed());

  active_source_buffers_->Clear();

  // Clear SourceBuffer references to this object.
  for (unsigned long i = 0; i < source_buffers_->length(); ++i)
    source_buffers_->item(i)->RemovedFromMediaSource();
  source_buffers_->Clear();

  attached_element_.Clear();

  ScheduleEvent(EventTypeNames::sourceclose);
}

bool MediaSource::IsUpdating() const {
  // Return true if any member of |m_sourceBuffers| is updating.
  for (unsigned long i = 0; i < source_buffers_->length(); ++i) {
    if (source_buffers_->item(i)->updating())
      return true;
  }

  return false;
}

bool MediaSource::isTypeSupported(const String& type) {
  // Section 2.2 isTypeSupported() method steps.
  // https://dvcs.w3.org/hg/html-media/raw-file/tip/media-source/media-source.html#widl-MediaSource-isTypeSupported-boolean-DOMString-type
  // 1. If type is an empty string, then return false.
  if (type.IsEmpty()) {
    BLINK_MSLOG << __func__ << "(" << type << ") -> false (empty input)";
    return false;
  }

  ContentType content_type(type);
  String codecs = content_type.Parameter("codecs");

  // 2. If type does not contain a valid MIME type string, then return false.
  if (content_type.GetType().IsEmpty()) {
    BLINK_MSLOG << __func__ << "(" << type << ") -> false (invalid mime type)";
    return false;
  }

  // Note: MediaSource.isTypeSupported() returning true implies that
  // HTMLMediaElement.canPlayType() will return "maybe" or "probably" since it
  // does not make sense for a MediaSource to support a type the
  // HTMLMediaElement knows it cannot play.
  if (HTMLMediaElement::GetSupportsType(content_type) ==
      MIMETypeRegistry::kIsNotSupported) {
    BLINK_MSLOG << __func__ << "(" << type
                << ") -> false (not supported by HTMLMediaElement)";
    return false;
  }

#if defined(OS_TIZEN_TV_PRODUCT)
  if (!MediaSource::IsCodecAndMediaTypeSupported(content_type)) {
    BLINK_MSLOG << __func__ << "(" << type
                << ") -> false (invalid addditional type)";
    return false;
  }
#endif

  // 3. If type contains a media type or media subtype that the MediaSource does
  //    not support, then return false.
  // 4. If type contains at a codec that the MediaSource does not support, then
  //    return false.
  // 5. If the MediaSource does not support the specified combination of media
  //    type, media subtype, and codecs then return false.
  // 6. Return true.
  bool result = MIMETypeRegistry::IsSupportedMediaSourceMIMEType(
      content_type.GetType(), codecs);
  BLINK_MSLOG << __func__ << "(" << type << ") -> "
              << (result ? "true" : "false");
  return result;
}

const AtomicString& MediaSource::InterfaceName() const {
  return EventTargetNames::MediaSource;
}

ExecutionContext* MediaSource::GetExecutionContext() const {
  return ContextLifecycleObserver::GetExecutionContext();
}

DEFINE_TRACE(MediaSource) {
  visitor->Trace(async_event_queue_);
  visitor->Trace(attached_element_);
  visitor->Trace(source_buffers_);
  visitor->Trace(active_source_buffers_);
  visitor->Trace(live_seekable_range_);
  EventTargetWithInlineData::Trace(visitor);
  ContextLifecycleObserver::Trace(visitor);
}

void MediaSource::SetWebMediaSourceAndOpen(
    std::unique_ptr<WebMediaSource> web_media_source) {
  TRACE_EVENT_ASYNC_END0("media", "MediaSource::attachToElement", this);
  DCHECK(web_media_source);
  DCHECK(!web_media_source_);
  DCHECK(attached_element_);
  web_media_source_ = std::move(web_media_source);
  SetReadyState(OpenKeyword());
}

void MediaSource::AddedToRegistry() {
  ++added_to_registry_counter_;
  // Ensure there's no counter overflow.
  CHECK_GT(added_to_registry_counter_, 0);
}

void MediaSource::RemovedFromRegistry() {
  DCHECK_GT(added_to_registry_counter_, 0);
  --added_to_registry_counter_;
}

double MediaSource::duration() const {
  return IsClosed() ? std::numeric_limits<float>::quiet_NaN()
                    : web_media_source_->Duration();
}

TimeRanges* MediaSource::Buffered() const {
  // Implements MediaSource algorithm for HTMLMediaElement.buffered.
  // https://dvcs.w3.org/hg/html-media/raw-file/default/media-source/media-source.html#htmlmediaelement-extensions
  HeapVector<Member<TimeRanges>> ranges(active_source_buffers_->length());
  for (size_t i = 0; i < active_source_buffers_->length(); ++i)
    ranges[i] = active_source_buffers_->item(i)->buffered(ASSERT_NO_EXCEPTION);

  // 1. If activeSourceBuffers.length equals 0 then return an empty TimeRanges
  //    object and abort these steps.
  if (ranges.IsEmpty())
    return TimeRanges::Create();

  // 2. Let active ranges be the ranges returned by buffered for each
  //    SourceBuffer object in activeSourceBuffers.
  // 3. Let highest end time be the largest range end time in the active ranges.
  double highest_end_time = -1;
  for (size_t i = 0; i < ranges.size(); ++i) {
    unsigned length = ranges[i]->length();
    if (length)
      highest_end_time = std::max(
          highest_end_time, ranges[i]->end(length - 1, ASSERT_NO_EXCEPTION));
  }

  // Return an empty range if all ranges are empty.
  if (highest_end_time < 0)
    return TimeRanges::Create();

  // 4. Let intersection ranges equal a TimeRange object containing a single
  //    range from 0 to highest end time.
  TimeRanges* intersection_ranges = TimeRanges::Create(0, highest_end_time);

  // 5. For each SourceBuffer object in activeSourceBuffers run the following
  //    steps:
  bool ended = readyState() == EndedKeyword();
  for (size_t i = 0; i < ranges.size(); ++i) {
    // 5.1 Let source ranges equal the ranges returned by the buffered attribute
    //     on the current SourceBuffer.
    TimeRanges* source_ranges = ranges[i].Get();

    // 5.2 If readyState is "ended", then set the end time on the last range in
    //     source ranges to highest end time.
    if (ended && source_ranges->length())
      source_ranges->Add(source_ranges->start(source_ranges->length() - 1,
                                              ASSERT_NO_EXCEPTION),
                         highest_end_time);

    // 5.3 Let new intersection ranges equal the the intersection between the
    //     intersection ranges and the source ranges.
    // 5.4 Replace the ranges in intersection ranges with the new intersection
    //     ranges.
    intersection_ranges->IntersectWith(source_ranges);
  }

  return intersection_ranges;
}

TimeRanges* MediaSource::Seekable() const {
  // Implements MediaSource algorithm for HTMLMediaElement.seekable.
  // http://w3c.github.io/media-source/#htmlmediaelement-extensions

  double source_duration = duration();
  // If duration equals NaN: Return an empty TimeRanges object.
  if (std::isnan(source_duration))
    return TimeRanges::Create();

  // If duration equals positive Infinity:
  if (source_duration == std::numeric_limits<double>::infinity()) {
    TimeRanges* buffered = attached_element_->buffered();

    // 1. If live seekable range is not empty:
    if (live_seekable_range_->length() != 0) {
      // 1.1. Let union ranges be the union of live seekable range and the
      //      HTMLMediaElement.buffered attribute.
      // 1.2. Return a single range with a start time equal to the
      //      earliest start time in union ranges and an end time equal to
      //      the highest end time in union ranges and abort these steps.
      if (buffered->length() == 0) {
        return TimeRanges::Create(
            live_seekable_range_->start(0, ASSERT_NO_EXCEPTION),
            live_seekable_range_->end(0, ASSERT_NO_EXCEPTION));
      }

      return TimeRanges::Create(
          std::min(live_seekable_range_->start(0, ASSERT_NO_EXCEPTION),
                   buffered->start(0, ASSERT_NO_EXCEPTION)),
          std::max(live_seekable_range_->end(0, ASSERT_NO_EXCEPTION),
                   buffered->end(buffered->length() - 1, ASSERT_NO_EXCEPTION)));
    }
    // 2. If the HTMLMediaElement.buffered attribute returns an empty TimeRanges
    //    object, then return an empty TimeRanges object and abort these steps.
    if (buffered->length() == 0)
      return TimeRanges::Create();

    // 3. Return a single range with a start time of 0 and an end time equal to
    //    the highest end time reported by the HTMLMediaElement.buffered
    //    attribute.
    return TimeRanges::Create(
        0, buffered->end(buffered->length() - 1, ASSERT_NO_EXCEPTION));
  }

  // 3. Otherwise: Return a single range with a start time of 0 and an end time
  //    equal to duration.
  return TimeRanges::Create(0, source_duration);
}

void MediaSource::OnTrackChanged(TrackBase* track) {
  DCHECK(HTMLMediaElement::MediaTracksEnabledInternally());
  SourceBuffer* source_buffer =
      SourceBufferTrackBaseSupplement::sourceBuffer(*track);
  if (!source_buffer)
    return;

  DCHECK(source_buffers_->Contains(source_buffer));
  if (track->GetType() == WebMediaPlayer::kAudioTrack) {
    source_buffer->audioTracks().ScheduleChangeEvent();
  } else if (track->GetType() == WebMediaPlayer::kVideoTrack) {
    if (static_cast<VideoTrack*>(track)->selected())
      source_buffer->videoTracks().TrackSelected(track->id());
    source_buffer->videoTracks().ScheduleChangeEvent();
  }

  bool is_active = (source_buffer->videoTracks().selectedIndex() != -1) ||
                   source_buffer->audioTracks().HasEnabledTrack();
  SetSourceBufferActive(source_buffer, is_active);
}

void MediaSource::setDuration(double duration,
                              ExceptionState& exception_state) {
  // 2.1 https://www.w3.org/TR/media-source/#widl-MediaSource-duration
  // 1. If the value being set is negative or NaN then throw a TypeError
  // exception and abort these steps.
  if (std::isnan(duration)) {
    LogAndThrowTypeError(exception_state, ExceptionMessages::NotAFiniteNumber(
                                              duration, "duration"));
    return;
  }
  if (duration < 0.0) {
    LogAndThrowTypeError(
        exception_state,
        ExceptionMessages::IndexExceedsMinimumBound("duration", duration, 0.0));
    return;
  }

  // 2. If the readyState attribute is not "open" then throw an
  //    InvalidStateError exception and abort these steps.
  // 3. If the updating attribute equals true on any SourceBuffer in
  //    sourceBuffers, then throw an InvalidStateError exception and abort these
  //    steps.
  if (ThrowExceptionIfClosedOrUpdating(IsOpen(), IsUpdating(), exception_state))
    return;

  // 4. Run the duration change algorithm with new duration set to the value
  //    being assigned to this attribute.
  DurationChangeAlgorithm(duration, exception_state);
}

void MediaSource::DurationChangeAlgorithm(double new_duration,
                                          ExceptionState& exception_state) {
  // http://w3c.github.io/media-source/#duration-change-algorithm
  // 1. If the current value of duration is equal to new duration, then return.
  if (new_duration == duration())
    return;

  // 2. If new duration is less than the highest starting presentation
  // timestamp of any buffered coded frames for all SourceBuffer objects in
  // sourceBuffers, then throw an InvalidStateError exception and abort these
  // steps. Note: duration reductions that would truncate currently buffered
  // media are disallowed. When truncation is necessary, use remove() to
  // reduce the buffered range before updating duration.
  double highest_buffered_presentation_timestamp = 0;
  for (size_t i = 0; i < source_buffers_->length(); ++i) {
    highest_buffered_presentation_timestamp =
        std::max(highest_buffered_presentation_timestamp,
                 source_buffers_->item(i)->HighestPresentationTimestamp());
  }

  if (new_duration < highest_buffered_presentation_timestamp) {
    if (RuntimeEnabledFeatures::MediaSourceNewAbortAndDurationEnabled()) {
      LogAndThrowDOMException(
          exception_state, kInvalidStateError,
          "Setting duration below highest presentation timestamp of any "
          "buffered coded frames is disallowed. Instead, first do asynchronous "
          "remove(newDuration, oldDuration) on all sourceBuffers, where "
          "newDuration < oldDuration.");
      return;
    }

    Deprecation::CountDeprecation(
        attached_element_->GetDocument(),
        WebFeature::kMediaSourceDurationTruncatingBuffered);
    // See also deprecated remove(new duration, old duration) behavior below.
  }

  // 3. Set old duration to the current value of duration.
  double old_duration = duration();
  DCHECK_LE(highest_buffered_presentation_timestamp,
            std::isnan(old_duration) ? 0 : old_duration);

  // 4. Update duration to new duration.
  bool request_seek = attached_element_->currentTime() > new_duration;
  web_media_source_->SetDuration(new_duration);

  if (!RuntimeEnabledFeatures::MediaSourceNewAbortAndDurationEnabled() &&
      new_duration < old_duration) {
    // Deprecated behavior: if the new duration is less than old duration,
    // then call remove(new duration, old duration) on all all objects in
    // sourceBuffers.
    for (size_t i = 0; i < source_buffers_->length(); ++i)
      source_buffers_->item(i)->remove(new_duration, old_duration,
                                       ASSERT_NO_EXCEPTION);
  }

  // 5. If a user agent is unable to partially render audio frames or text cues
  //    that start before and end after the duration, then run the following
  //    steps:
  //    NOTE: Currently we assume that the media engine is able to render
  //    partial frames/cues. If a media engine gets added that doesn't support
  //    this, then we'll need to add logic to handle the substeps.

  // 6. Update the media controller duration to new duration and run the
  //    HTMLMediaElement duration change algorithm.
  attached_element_->DurationChanged(new_duration, request_seek);
}

void MediaSource::SetReadyState(const AtomicString& state) {
  DCHECK(state == OpenKeyword() || state == ClosedKeyword() ||
         state == EndedKeyword());

  AtomicString old_state = readyState();
  BLINK_MSLOG << __func__ << " this=" << this << " : " << old_state << " -> "
              << state;

  if (state == ClosedKeyword()) {
    web_media_source_.reset();
  }

  if (old_state == state)
    return;

  ready_state_ = state;

  OnReadyStateChange(old_state, state);
}

void MediaSource::endOfStream(const AtomicString& error,
                              ExceptionState& exception_state) {
  DEFINE_STATIC_LOCAL(const AtomicString, network, ("network"));
  DEFINE_STATIC_LOCAL(const AtomicString, decode, ("decode"));

  // https://www.w3.org/TR/media-source/#dom-mediasource-endofstream
  // 1. If the readyState attribute is not in the "open" state then throw an
  //    InvalidStateError exception and abort these steps.
  // 2. If the updating attribute equals true on any SourceBuffer in
  //    sourceBuffers, then throw an InvalidStateError exception and abort these
  //    steps.
  if (ThrowExceptionIfClosedOrUpdating(IsOpen(), IsUpdating(), exception_state))
    return;

  // 3. Run the end of stream algorithm with the error parameter set to error.
  if (error == network)
    EndOfStreamAlgorithm(WebMediaSource::kEndOfStreamStatusNetworkError);
  else if (error == decode)
    EndOfStreamAlgorithm(WebMediaSource::kEndOfStreamStatusDecodeError);
  else  // "" is allowed internally but not by IDL bindings.
    EndOfStreamAlgorithm(WebMediaSource::kEndOfStreamStatusNoError);
}

void MediaSource::endOfStream(ExceptionState& exception_state) {
  endOfStream("", exception_state);
}

void MediaSource::setLiveSeekableRange(double start,
                                       double end,
                                       ExceptionState& exception_state) {
  // http://w3c.github.io/media-source/#widl-MediaSource-setLiveSeekableRange-void-double-start-double-end
  // 1. If the readyState attribute is not "open" then throw an
  //    InvalidStateError exception and abort these steps.
  // 2. If the updating attribute equals true on any SourceBuffer in
  //    SourceBuffers, then throw an InvalidStateError exception and abort
  //    these steps.
  //    Note: https://github.com/w3c/media-source/issues/118, once fixed, will
  //    remove the updating check (step 2). We skip that check here already.
  if (ThrowExceptionIfClosed(IsOpen(), exception_state))
    return;

  // 3. If start is negative or greater than end, then throw a TypeError
  //    exception and abort these steps.
  if (start < 0 || start > end) {
    LogAndThrowTypeError(
        exception_state,
        ExceptionMessages::IndexOutsideRange(
            "start value", start, 0.0, ExceptionMessages::kInclusiveBound, end,
            ExceptionMessages::kInclusiveBound));
    return;
  }

  // 4. Set live seekable range to be a new normalized TimeRanges object
  //    containing a single range whose start position is start and end
  //    position is end.
  live_seekable_range_ = TimeRanges::Create(start, end);
}

void MediaSource::clearLiveSeekableRange(ExceptionState& exception_state) {
  // http://w3c.github.io/media-source/#widl-MediaSource-clearLiveSeekableRange-void
  // 1. If the readyState attribute is not "open" then throw an
  //    InvalidStateError exception and abort these steps.
  // 2. If the updating attribute equals true on any SourceBuffer in
  //    SourceBuffers, then throw an InvalidStateError exception and abort
  //    these steps.
  //    Note: https://github.com/w3c/media-source/issues/118, once fixed, will
  //    remove the updating check (step 2). We skip that check here already.
  if (ThrowExceptionIfClosed(IsOpen(), exception_state))
    return;

  // 3. If live seekable range contains a range, then set live seekable range
  //    to be a new empty TimeRanges object.
  if (live_seekable_range_->length() != 0)
    live_seekable_range_ = TimeRanges::Create();
}

bool MediaSource::IsOpen() const {
  return readyState() == OpenKeyword();
}

void MediaSource::SetSourceBufferActive(SourceBuffer* source_buffer,
                                        bool is_active) {
  if (!is_active) {
    DCHECK(active_source_buffers_->Contains(source_buffer));
    active_source_buffers_->Remove(source_buffer);
    return;
  }

  if (active_source_buffers_->Contains(source_buffer))
    return;

  // https://dvcs.w3.org/hg/html-media/raw-file/tip/media-source/media-source.html#widl-MediaSource-activeSourceBuffers
  // SourceBuffer objects in SourceBuffer.activeSourceBuffers must appear in
  // the same order as they appear in SourceBuffer.sourceBuffers.
  // SourceBuffer transitions to active are not guaranteed to occur in the
  // same order as buffers in |m_sourceBuffers|, so this method needs to
  // insert |sourceBuffer| into |m_activeSourceBuffers|.
  size_t index_in_source_buffers = source_buffers_->Find(source_buffer);
  DCHECK(index_in_source_buffers != kNotFound);

  size_t insert_position = 0;
  while (insert_position < active_source_buffers_->length() &&
         source_buffers_->Find(active_source_buffers_->item(insert_position)) <
             index_in_source_buffers) {
    ++insert_position;
  }

  active_source_buffers_->insert(insert_position, source_buffer);
}

HTMLMediaElement* MediaSource::MediaElement() const {
  return attached_element_.Get();
}

void MediaSource::EndOfStreamAlgorithm(
    const WebMediaSource::EndOfStreamStatus eos_status) {
  // https://www.w3.org/TR/media-source/#end-of-stream-algorithm
  // 1. Change the readyState attribute value to "ended".
  // 2. Queue a task to fire a simple event named sourceended at the
  //    MediaSource.
  SetReadyState(EndedKeyword());

  // 3. Do various steps based on |eos_status|.
  web_media_source_->MarkEndOfStream(eos_status);

  if (eos_status == WebMediaSource::kEndOfStreamStatusNoError) {
    // The implementation may not have immediately informed the
    // |attached_element_| of the potentially reduced duration. Prevent
    // app-visible duration race by synchronously running the duration change
    // algorithm. The MSE spec supports this:
    // https://www.w3.org/TR/media-source/#end-of-stream-algorithm
    // 2.4.7.3 (If error is not set)
    // Run the duration change algorithm with new duration set to the largest
    // track buffer ranges end time across all the track buffers across all
    // SourceBuffer objects in sourceBuffers.
    //
    // Since MarkEndOfStream caused the demuxer to update its duration (similar
    // to the MediaSource portion of the duration change algorithm), all that
    // is left is to notify the element.
    // TODO(wolenetz): Consider refactoring the MarkEndOfStream implementation
    // to just mark end of stream, and move the duration reduction logic to here
    // so we can just run DurationChangeAlgorithm(...) here.
    double new_duration = duration();
    bool request_seek = attached_element_->currentTime() > new_duration;
    attached_element_->DurationChanged(new_duration, request_seek);
  }
}

bool MediaSource::IsClosed() const {
  return readyState() == ClosedKeyword();
}

void MediaSource::Close() {
  SetReadyState(ClosedKeyword());
}

bool MediaSource::AttachToElement(HTMLMediaElement* element) {
  if (attached_element_)
    return false;

  DCHECK(IsClosed());

  TRACE_EVENT_ASYNC_BEGIN0("media", "MediaSource::attachToElement", this);
  attached_element_ = element;
  return true;
}

void MediaSource::OpenIfInEndedState() {
  if (ready_state_ != EndedKeyword())
    return;

  SetReadyState(OpenKeyword());
  web_media_source_->UnmarkEndOfStream();
}

bool MediaSource::HasPendingActivity() const {
  return attached_element_ || web_media_source_ ||
         async_event_queue_->HasPendingEvents() ||
         added_to_registry_counter_ > 0;
}

void MediaSource::ContextDestroyed(ExecutionContext*) {
  async_event_queue_->Close();
  if (!IsClosed())
    SetReadyState(ClosedKeyword());
  web_media_source_.reset();
}

std::unique_ptr<WebSourceBuffer> MediaSource::CreateWebSourceBuffer(
    const String& type,
    const String& codecs,
    ExceptionState& exception_state) {
  WebSourceBuffer* web_source_buffer = 0;

  switch (
      web_media_source_->AddSourceBuffer(type, codecs, &web_source_buffer)) {
    case WebMediaSource::kAddStatusOk:
      return WTF::WrapUnique(web_source_buffer);
    case WebMediaSource::kAddStatusNotSupported:
      DCHECK(!web_source_buffer);
      // 2.2
      // https://dvcs.w3.org/hg/html-media/raw-file/default/media-source/media-source.html#widl-MediaSource-addSourceBuffer-SourceBuffer-DOMString-type
      // Step 2: If type contains a MIME type ... that is not supported with the
      // types specified for the other SourceBuffer objects in sourceBuffers,
      // then throw a NotSupportedError exception and abort these steps.
      LogAndThrowDOMException(
          exception_state, kNotSupportedError,
          "The type provided ('" + type + "') is not supported.");
      return nullptr;
    case WebMediaSource::kAddStatusReachedIdLimit:
      DCHECK(!web_source_buffer);
      // 2.2
      // https://dvcs.w3.org/hg/html-media/raw-file/default/media-source/media-source.html#widl-MediaSource-addSourceBuffer-SourceBuffer-DOMString-type
      // Step 3: If the user agent can't handle any more SourceBuffer objects
      // then throw a QuotaExceededError exception and abort these steps.
      LogAndThrowDOMException(exception_state, kQuotaExceededError,
                              "This MediaSource has reached the limit of "
                              "SourceBuffer objects it can handle. No "
                              "additional SourceBuffer objects may be added.");
      return nullptr;
  }

  NOTREACHED();
  return nullptr;
}

void MediaSource::ScheduleEvent(const AtomicString& event_name) {
  DCHECK(async_event_queue_);

  Event* event = Event::Create(event_name);
  event->SetTarget(this);

  async_event_queue_->EnqueueEvent(BLINK_FROM_HERE, event);
}

URLRegistry& MediaSource::Registry() const {
  return MediaSourceRegistry::Registry();
}

#if defined(OS_TIZEN_TV_PRODUCT)
bool MediaSource::IsCodecAndMediaTypeSupported(
    const ContentType& content_type) {
  BLINK_MSLOG << __func__ << "MediaSource Extention Type (W="
              << content_type.Parameter("width").ToInt()
              << ", xH=" << content_type.Parameter("height").ToInt()
              << ", dfps=" << content_type.Parameter("framerate").ToInt()
              << ", fMbps=" << content_type.Parameter("bitrate").ToInt()
              << ", dchannels=" << content_type.Parameter("channels").ToInt()
              << ")";
  const int k8K_panel_width = 7680;
  const int k8K_panel_height = 4320;
  const int UHD_panel_width = 3840;
  const int UHD_panel_height = 2160;
  const int FHD_panel_width = 1920;
  const int FHD_panel_height = 1080;
  int info_ret = 0;

  static int panel_width = 0;
  if (!panel_width) {
    info_ret = system_info_get_value_int(SYSTEM_INFO_KEY_PANEL_RESOLUTION_WIDTH,
                                         &panel_width);
    if (info_ret != SYSTEM_INFO_ERROR_NONE) {
      BLINK_MSLOG << __func__ << "(System info error:" << info_ret
                  << ") Fail to get panel width!!";
      panel_width = FHD_panel_width;
    }
  }

  if (panel_width == k8K_panel_width) {
    if (content_type.Parameter("width").ToInt() > k8K_panel_width ||
        content_type.Parameter("height").ToInt() > k8K_panel_height)
      return false;
  } else if (panel_width == UHD_panel_width) {
    if (content_type.Parameter("width").ToInt() > UHD_panel_width ||
        content_type.Parameter("height").ToInt() > UHD_panel_height)
      return false;
  } else {
    if (content_type.Parameter("width").ToInt() > FHD_panel_width ||
        content_type.Parameter("height").ToInt() > FHD_panel_height)
      return false;
  }

  const int max_framerate = 60;
  const int stable_framerate = 30;
  const int max_bitrate = 60;
  const int stable_bitrate = 30;
  const int max_audio_channels = 6;
  const int unit = 1000000;

  int support_max_bitrate = max_bitrate;
  int support_max_framerate = max_framerate;
  static int max_VP9_bitrate = 0;
  static int max_VP9_framerate = 0;
  const String crypto_block_format =
      content_type.Parameter("cryptoblockformat");

  if (content_type.Parameter("codec").FindIgnoringCase("vp9") !=
          WTF::kNotFound ||
      content_type.Parameter("codec").FindIgnoringCase("vp09") !=
          WTF::kNotFound) {
    const String eotf = content_type.Parameter("eotf");
    if (!eotf.IsEmpty() && (eotf != "bt709") && (eotf != "smpte2084") &&
        (eotf != "arib-std-b67"))
      return false;

    if (!crypto_block_format.IsEmpty() && crypto_block_format != "subsample")
      return false;

    if (!max_VP9_bitrate) {
      info_ret = system_info_get_value_int(SYSTEM_INFO_KEY_VP9_BITRATE_SIZE,
                                           &max_VP9_bitrate);
      if (info_ret != SYSTEM_INFO_ERROR_NONE) {
        BLINK_MSLOG << __func__ << "(System info error:" << info_ret
                    << ") Fail to get bitrate!!";
        max_VP9_bitrate = stable_bitrate;
      }
    }
    support_max_bitrate = max_VP9_bitrate;

    if (!max_VP9_framerate) {
      info_ret = system_info_get_value_int(SYSTEM_INFO_KEY_VP9_FRAMERATE_SIZE,
                                           &max_VP9_framerate);
      if (info_ret != SYSTEM_INFO_ERROR_NONE) {
        BLINK_MSLOG << __func__ << "(System info error:" << info_ret
                    << ") Fail to get framerate!!";
        max_VP9_framerate = stable_framerate;
      }
    }
    support_max_framerate = max_VP9_framerate;
  } else {
    if (!crypto_block_format.IsEmpty())
      return false;
  }
  BLINK_MSLOG << __func__
              << "(SYSTEM INFO: Bitrate fMbps=" << support_max_bitrate
              << ", dfps=" << support_max_framerate << ")";
  if (content_type.Parameter("bitrate").ToInt() > support_max_bitrate * unit ||
      content_type.Parameter("framerate").ToInt() > support_max_framerate ||
      content_type.Parameter("channels").ToInt() > max_audio_channels)
    return false;

  return true;
}
#endif

}  // namespace blink
