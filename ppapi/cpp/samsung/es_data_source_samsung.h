// Copyright (c) 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_CPP_SAMSUNG_ES_DATA_SOURCE_SAMSUNG_H_
#define PPAPI_CPP_SAMSUNG_ES_DATA_SOURCE_SAMSUNG_H_

#include <string>

#include "ppapi/c/samsung/ppb_media_data_source_samsung.h"
#include "ppapi/cpp/completion_callback.h"
#include "ppapi/cpp/resource.h"
#include "ppapi/cpp/samsung/media_data_source_samsung.h"

/// @file
/// This file defines the types allowing user to feed the player with
/// Elementary Stream data.
///
/// Part of Pepper Media Player interfaces (Samsung's extension).
namespace pp {

class ElementaryStreamListener_Samsung;
class InstanceHandle;

/// Interface representing common functionalities of elementary streams.
///
/// Basic usage:
/// 1. Crate stream by calling <code>ESDataSource_Samsung.AddStream</code>
/// 2. Initialize buffer specific information (audio/video config)
/// 3. Call <code>InitializeDone</code>
/// 5. Attach Data Source to the player
/// 6. Appends Elementary Stream packets by calling <code>AppendPacket</code>
/// 7. Signalize end of stream (clip) by calling
///    <code>ESDataSource_Samsung.SetEndOfStream</code>
class ElementaryStream_Samsung : public Resource {
 public:
  ElementaryStream_Samsung(const ElementaryStream_Samsung& other);
  ElementaryStream_Samsung& operator=(const ElementaryStream_Samsung& other);

  virtual ~ElementaryStream_Samsung();

  /// Retrieves stream type represented by this resource.
  virtual PP_ElementaryStream_Type_Samsung GetStreamType() const = 0;

  /// Call this method to confirm new/updated buffer config. This method will
  /// return PP_OK if set buffer config is valid or one of the error codes from
  /// <code>pp_errors.h</code> otherwise.
  ///
  /// This method assumes that all plugin provided all necessary Elementary
  /// Stream initialization metadata
  /// (see <code>PP_STREAMINITIALIZATIONMODE_FULL</code>).
  ///
  /// @param[in] callback A <code>CompletionCallback</code> to be called upon
  /// completion.
  ///
  /// @return PP_OK on success, otherwise an error code from
  /// <code>pp_errors.h</code>.
  /// Method returns <code>PP_ERROR_BADARGUMENT</code> when stream configuration
  /// is invalid.
  int32_t InitializeDone(
      const CompletionCallback& callback);

  /// Call this method to confirm new/updated buffer config. This method will
  /// return PP_OK if set buffer config is valid or one of the error codes from
  /// <code>pp_errors.h</code> otherwise.
  ///
  /// @param[in] mode A parameter indicating how much information regarding
  /// elementary stream configuration was extracted by the plugin (demuxer).
  /// @param[in] callback A <code>CompletionCallback</code> to be called upon
  /// completion.
  ///
  /// @return PP_OK on success, otherwise an error code from
  /// <code>pp_errors.h</code>.
  /// Returns <code>PP_ERROR_BADARGUMENT</code> when stream configuration
  /// is invalid.
  /// Returns <code>PP_ERROR_NOTSUPPORTED</code> when given initialization
  /// mode is not supported by the browser.
  int32_t InitializeDone(PP_StreamInitializationMode mode,
                         const CompletionCallback& callback);

  /// Appends Elementary Stream packet.
  ///
  /// Before appending any packet to the buffer, it must be properly configured
  /// (see <code>InitializeDone</code>).
  ///
  /// @param[in] packet A <code>PP_ESPacket</code> containing Elementary Stream
  /// packet data and metadata.
  /// @param[in] callback A <code>CompletionCallback</code> to be called upon
  /// completion.
  ///
  /// @return PP_OK on success, otherwise an error code from
  /// <code>pp_errors.h</code>.
  /// @return <code>PP_ERROR_NOQUOTA</code> if internal buffer is full.
  /// @return <code>PP_ERROR_FAILED</code> if <code>InitializeDone()</code> has
  /// not successfully completed.
  int32_t AppendPacket(
      const PP_ESPacket& packet,
      const CompletionCallback& callback);

  /// Appends Elementary Stream encrypted packet.
  ///
  /// Before appending any packet to the buffer, it must be properly configured
  /// (see <code>InitializeDone</code>).
  ///
  /// @param[in] packet A <code>PP_ESPacket</code> containing Elementary Stream
  /// packet data and metadata.
  /// @param[in] encryption_info A <code>PP_ESPacketEncryptionInfo</code>
  /// containing packet encryption description.
  /// @param[in] callback A <code>CompletionCallback</code> to be called upon
  /// completion.
  ///
  /// @return PP_OK on success, otherwise an error code from
  /// <code>pp_errors.h</code>.
  /// @return <code>PP_ERROR_NOQUOTA</code> if internal buffer is full.
  /// @return <code>PP_ERROR_FAILED</code> if <code>InitializeDone()</code> has
  /// not successfully completed.
  int32_t AppendEncryptedPacket(
      const PP_ESPacket& packet,
      const PP_ESPacketEncryptionInfo& encryption_info,
      const CompletionCallback& callback);

  /// Appends Elementary Stream packet.
  ///
  /// Before appending any packet to the buffer, it must be properly configured
  /// (see <code>InitializeDone</code>).
  ///
  /// Note:
  /// After successful append ownership of the TrustZone memory is transfered
  /// from the module to the browser and handle must not be released.
  /// But in case of any error (i.e. return code/callback call with status
  /// other than PP_OK) the module is responsible for releasing memory
  /// reference.
  ///
  /// @param[in] packet A <code>PP_ESPacket</code> containing Elementary Stream
  /// metadata. Note: when using this method of appending packets
  /// <code>size</code> and <code>buffer</code> fields of
  /// <code>PP_ESPacket</code> struct must be set to 0 and nullptr. If they are
  /// not set to zero (nullptr), then PP_ERROR_BADARGUMENT will be returned.
  /// @param[in] handle A handle to TrustZone memory containing packet data
  /// which will be appended.
  /// @param[in] callback A <code>PCompletionCallback</code> to be called upon
  /// completion.
  ///
  /// @return PP_OK on success, otherwise an error code from
  /// <code>pp_errors.h</code>.
  /// Returns PP_ERROR_FAILED if InitializeDone() has not successfully
  /// completed.
  /// Returns PP_ERROR_NOQUOTA if internal buffer is full.
  /// Returns PP_ERROR_BADARGUMENT if either <code>size</code> or
  /// <code>buffer</code> are not set to 0/nullptr.
  int32_t AppendTrustZonePacket(const PP_ESPacket& packet,
                                const PP_TrustZoneReference& handle,
                                const CompletionCallback& callback);

  /// Flushes all appended, but not decoded or rendered packets to this buffer.
  /// This method is usually called during seek operations.
  ///
  /// @param[in] callback A <code>CompletionCallback</code> to be called upon
  /// completion.
  ///
  /// @return PP_OK on success, otherwise an error code from
  /// <code>pp_errors.h</code>.
  /// Returns PP_ERROR_FAILED if InitializeDone() has not successfully
  /// completed.
  int32_t Flush(
      const CompletionCallback& callback);

  /// Found DRM system initialization metadata. |type| describes type
  /// of the initialization data |init_data| associated with the stream.
  ///
  /// @param[in] type_size A size of DRM specific |type| buffer
  /// @param[in] type A buffer containing DRM system specific description of
  /// type of an |init_data|.
  /// @param[in] init_data_size A size of DRM specific |init_data| buffer
  /// @param[in] init_data A buffer containing DRM system initialization data.
  /// @param[in] callback A <code>CompletionCallback</code> to be called upon
  /// completion.
  ///
  /// @return PP_OK on success, otherwise an error code from
  /// <code>pp_errors.h</code>.
  int32_t SetDRMInitData(
      uint32_t type_size,
      const void* type,
      uint32_t init_data_size,
      const void* init_data,
      const CompletionCallback& callback);


  /// Found DRM system initialization metadata. |type| describes type
  /// of the initialization data |init_data| associated with the stream.
  ///
  /// @param[in] type A string describing type of an |init_data|.
  ///                 Examples:
  ///                 - "cenc:pssh" - |init_data| will contain PSSH box as
  ///                   described by Common Encryption specifiacation
  ///                 - "mspr:pro" - |init_data| will contain Microsoft
  ///                   PlayReady Object Header (PRO).
  /// @param[in] init_data_size A size of DRM specific |init_data| buffer
  /// @param[in] init_data A buffer containing DRM system initialization data.
  /// @param[in] callback A <code>CompletionCallback</code> to be called upon
  /// completion.
  ///
  ///   @return PP_OK on success, otherwise an error code from
  /// <code>pp_errors.h</code>.
  int32_t SetDRMInitData(
      const std::string& type,
      uint32_t init_data_size,
      const void* init_data,
      const CompletionCallback& callback);

 protected:
  ElementaryStream_Samsung();
  explicit ElementaryStream_Samsung(PP_Resource resource);
  explicit ElementaryStream_Samsung(const Resource& resource);
  explicit ElementaryStream_Samsung(PassRef, PP_Resource resource);
};

/// Interface representing an audio elementary stream and containing methods
/// to set audio codec specific configuration.
///
/// All pending configuration changes/initialization must be confirmed
/// by call to <code>ElementaryStream_Samsung.InitializeDone</code>.
///
/// All getters return last set configuration, which might be not confirmed yet.
class AudioElementaryStream_Samsung : public ElementaryStream_Samsung {
 public:
  AudioElementaryStream_Samsung();
  explicit AudioElementaryStream_Samsung(PP_Resource resource);
  explicit AudioElementaryStream_Samsung(PassRef, PP_Resource resource);
  AudioElementaryStream_Samsung(const AudioElementaryStream_Samsung& other);
  AudioElementaryStream_Samsung& operator=(
      const AudioElementaryStream_Samsung& other);

  virtual ~AudioElementaryStream_Samsung();

  /// Retrieves stream type represented by this resource,
  /// in this case <code>PP_ELEMENTARYSTREAM_TYPE_SAMSUNG_AUDIO</code>.
  virtual PP_ElementaryStream_Type_Samsung GetStreamType() const;

  /// Retrieves a currently set audio codec type.
  PP_AudioCodec_Type_Samsung GetAudioCodecType() const;

  /// Sets a new audio codec type.
  ///
  /// @remarks
  /// This is a mandatory parameter which must be set for every stream.
  void SetAudioCodecType(PP_AudioCodec_Type_Samsung audio_codec);

  /// Sets a new audio codec profile.
  ///
  /// @remarks
  /// This parameter can be figured out by the platform most of the times and
  /// therefore isn't mandatory to set for all configurations.
  PP_AudioCodec_Profile_Samsung GetAudioCodecProfile() const;

  /// Retrieves a currently set audio sample format.
  void SetAudioCodecProfile(PP_AudioCodec_Profile_Samsung profile);

  /// Sets a new audio sample format.
  ///
  /// @remarks
  /// This parameter can be figured out by the platform most of the times. It
  /// must be set explictly only for PCM and MULAW audio streams.
  PP_SampleFormat_Samsung GetSampleFormat() const;

  /// Retrieves a currently set audio channel layout.
  void SetSampleFormat(PP_SampleFormat_Samsung sample_format);

  /// Retrieves a currently set audio channel layout.
  PP_ChannelLayout_Samsung GetChannelLayout() const;

  /// Sets a new audio channel layout.
  void SetChannelLayout(PP_ChannelLayout_Samsung channel_layout);

  /// Retrieves a currently set number of bits that are used to represent audio
  /// sample per each channel.
  int32_t GetBitsPerChannel() const;

  /// Sets how many bits are used to represent audio sample per each channel.
  ///
  /// @remarks
  /// This parameter can be figured out by the platform most of the times and
  /// therefore isn't mandatory to set for all configurations.
  void SetBitsPerChannel(int32_t bits_per_channel);

  /// Retrieves a currently set sample rate.
  int32_t GetSamplesPerSecond() const;

  /// Sets how many audio samples were recorded per second (sample rate).
  void SetSamplesPerSecond(int32_t samples_per_second);

  /// Sets audio codec specific extra data. Those data are needed by audio codec
  /// to initialize properly audio decoding.
  ///
  /// @param[in] extra_data_size Size in bytes of |extra_data| buffer.
  /// @param[in] extra_data A pointer to the buffer containing audio codec
  /// specific extra data.
  void SetCodecExtraData(
      uint32_t extra_data_size,
      const void* extra_data);
};

/// Interface representing an video elementary stream and containing methods
/// to set video codec specific configuration.
///
/// All pending configuration changes/initialization must be confirmed
/// by call to <code>ElementaryStream_Samsung.InitializeDone</code>.
///
/// All getters return last set configuration, which might be not confirmed yet.
class VideoElementaryStream_Samsung : public ElementaryStream_Samsung {
 public:
  VideoElementaryStream_Samsung();
  explicit VideoElementaryStream_Samsung(PP_Resource resource);
  explicit VideoElementaryStream_Samsung(PassRef, PP_Resource resource);
  VideoElementaryStream_Samsung(const VideoElementaryStream_Samsung& other);
  VideoElementaryStream_Samsung& operator=(
      const VideoElementaryStream_Samsung& other);

  virtual ~VideoElementaryStream_Samsung();

  /// Retrieves stream type represented by this resource,
  /// in this case <code>PP_ELEMENTARYSTREAM_TYPE_SAMSUNG_VIDEO</code>.
  virtual PP_ElementaryStream_Type_Samsung GetStreamType() const;

  /// Retrieves a currently set video codec type.
  PP_VideoCodec_Type_Samsung GetVideoCodecType() const;

  /// Sets a new video codec type.
  ///
  /// @remarks
  /// This is a mandatory parameter which must be set for every stream.
  void SetVideoCodecType(PP_VideoCodec_Type_Samsung video_codec);

  /// Retrieves a currently set video codec type.
  PP_VideoCodec_Profile_Samsung GetVideoCodecProfile() const;

  /// Sets a new video codec profile.
  ///
  /// @remarks
  /// This parameter can be figured out by the platform most of the times and
  /// therefore isn't mandatory to set for all configurations.
  void SetVideoCodecProfile(PP_VideoCodec_Profile_Samsung video_codec);

  /// Retrieves a currently set video codec profile.
  PP_VideoFrame_Format_Samsung GetVideoFrameFormat() const;

  /// Sets a new video frame format.
  ///
  /// @remarks
  /// This parameter can be figured out by the platform most of the times and
  /// therefore isn't mandatory to set for all configurations.
  void SetVideoFrameFormat(PP_VideoFrame_Format_Samsung frame_format);

  /// Retrieves current video frame size in pixels.
  PP_Size GetVideoFrameSize() const;

  /// Sets new video frame size in pixels.
  void SetVideoFrameSize(const PP_Size& size);

  /// Retrieves current video frame rate as rational number represented by
  /// fraction |numerator| / |denominator|.
  ///
  /// Both |numerator| and |denominator| must be non-null, otherwise no
  /// information is retrieved.
  void GetFrameRate(uint32_t* numerator, uint32_t* denominator) const;

  /// Sets new video frame rate as rational number represented by
  /// fraction |numerator| / |denominator|.
  ///
  /// |denominator| must be positive (!= 0) otherwise no information is set.
  void SetFrameRate(uint32_t numerator, uint32_t denominator);

  /// Sets video codec specific extra data. Those data are needed by video codec
  /// to initialize properly video decoding.
  ///
  /// @param[in] extra_data_size Size in bytes of |extra_data| buffer.
  /// @param[in] extra_data A pointer to the buffer containing video codec
  /// specific extra data.
  void SetCodecExtraData(
      uint32_t extra_data_size,
      const void* extra_data);
};

/// Data source handling appends of Elementary Streams.
///
/// It is a container for Elementary Streams (audio/video) and there can be at
/// most one stream of given type (see
/// <code>PP_ElementaryStream_Type_Samsung</code>).
/// Basic usage (playback of clip containing audio and video):
///  1. Create ESDataSource_Samsung object.
///  2. Add audio stream using <code>AddStream<T><code> with argument being
///     callback accepting <code>AudioElementryStream_Samsung</code>.
///  3. Configure audio stream, by setting codec, sampling rate, channels and
///     other necessary information.
///  4. Call <code>ElementaryStream_Samsung.InitializeDone</code> to confirm
///     the configuration
///  2. Add video stream using <code>AddStream<T><code> with argument being
///     callback accepting <code>VideoElementryStream_Samsung</code>.
///  6. Configure video stream, by setting codec, frame rate, resolution and
///     other necessary information.
///  7. Call <code>ElementaryStream_Samsung.InitializeDone</code> to confirm
///     the configuration.
///  8. Attach data source to the player by calling
///     <code>MediaPlayer_Samsung.AttachMediaSource</code>.
///  9. Download and append Elementary Stream audio and video packets
///     by calling <code>ElementaryStream._SamsungAppendPacket</code>
/// 10. Signalize end of stream (clip) by calling <code>SetEndOfStream</code>
/// 11. Detach data source from the player by calling
///     <code>MediaPlayer_Samsung.AttachMediaSource</code> with
///     <code>NULL</code> object/resource.
class ESDataSource_Samsung : public MediaDataSource_Samsung {
 public:
  explicit ESDataSource_Samsung(const InstanceHandle& instance);

  ESDataSource_Samsung(const ESDataSource_Samsung& other);

  explicit ESDataSource_Samsung(PP_Resource resource);
  ESDataSource_Samsung(PassRef, PP_Resource resource);

  ESDataSource_Samsung& operator=(const ESDataSource_Samsung& other);

  virtual ~ESDataSource_Samsung();

  /// Factory method which adds stream of given type to the data source.
  ///
  /// Type T must be one of concrete types inheriting form
  /// <code>ElementaryStream_Samsung</code>
  ///
  /// This data source can handle at most one buffer of given time, so calling
  /// multiple times this method with the same buffer type will return the
  /// same resource as all previous calls.
  ///
  /// @param[in] callback A <code>CompletionCallbackWithOutput</code>
  /// to be called upon completion with added elementary stream.
  ///
  /// @return PP_OK on success, otherwise an error code from
  /// <code>pp_errors.h</code>.
  /// FIXME: remove listener default value once all related components will be
  /// updated to use new ElementaryStreamListener_Samsung listener.
  template<typename T>
  int32_t AddStream(const CompletionCallbackWithOutput<T>& callback,
      ElementaryStreamListener_Samsung* listener = NULL);

  /// Sets duration of the whole stream/container/clip.
  ///
  /// @param[in] duration A duration of played media.
  /// @param[in] callback A <code>CompletionCallback</code> to be called
  /// upon completion.
  ///
  /// @return PP_OK on success, otherwise an error code from
  /// <code>pp_errors.h</code>.
  int32_t SetDuration(
      PP_TimeDelta duration,
      const CompletionCallback& callback);

  /// Signalizes end of the whole stream/container/clip.
  ///
  ///
  /// @param[in] callback A <code>CompletionCallback</code> to be called upon
  /// completion.
  ///
  /// @return PP_OK on success, otherwise an error code from
  /// <code>pp_errors.h</code>.
  int32_t SetEndOfStream(
      const CompletionCallback& callback);

 private:
  /// Factory method which adds stream of given type to the data source.
  ///
  /// This data source can handle at most one buffer of given time, so calling
  /// multiple times this method with the same buffer type will return the
  /// same resource as all previous calls.
  ///
  /// @param[in] stream_type A <code>PP_ElementaryStream_Type_Samsung</code>
  /// identifying the stream type which will be added.
  /// @param[out] stream An added stream
  /// @param[in] callback A <code>CompletionCallback</code>
  /// to be called upon completion.
  ///
  /// @return PP_OK on success, otherwise an error code from
  /// <code>pp_errors.h</code>.
  int32_t AddStream(
          PP_ElementaryStream_Type_Samsung stream_type,
          PP_Resource* stream,
          ElementaryStreamListener_Samsung* listener,
          const CompletionCallback& callback);
};

/// Type traits of the stream type T.
///
/// kType will be PP_ELEMENTARYSTREAM_TYPE_SAMSUNG_UNKNOWN if T is not a valid
/// stream type.
template <typename T>
struct ElementaryStreamTraits_Samsung {
    static const PP_ElementaryStream_Type_Samsung kType =
            PP_ELEMENTARYSTREAM_TYPE_SAMSUNG_UNKNOWN;
};

template <>
struct ElementaryStreamTraits_Samsung<VideoElementaryStream_Samsung> {
    static const PP_ElementaryStream_Type_Samsung kType =
            PP_ELEMENTARYSTREAM_TYPE_SAMSUNG_VIDEO;
};

template <>
struct ElementaryStreamTraits_Samsung<AudioElementaryStream_Samsung> {
    static const PP_ElementaryStream_Type_Samsung kType =
            PP_ELEMENTARYSTREAM_TYPE_SAMSUNG_AUDIO;
};

#if __cplusplus >= 201103L
#define PP_MEDIA_PLAYER_SAMSUNG_STATIC_ASSERT(expr, msg) \
    static_assert(expr, msg)
#else
#if __GNUC__ >= 4
#define UNUSED_TYPEDEF __attribute__((__unused__))
#else
#define UNUSED_TYPEDEF
#endif
#define PP_MEDIA_PLAYER_SAMSUNG_STATIC_ASSERT(expr, msg)             \
  do {                                                               \
    typedef int static_assert_check[(expr) ? 1 : -1] UNUSED_TYPEDEF; \
  } while (0)
#endif

template<typename T>
inline int32_t ESDataSource_Samsung::AddStream(
    const CompletionCallbackWithOutput<T>& callback,
    ElementaryStreamListener_Samsung* listener) {
  typedef ElementaryStreamTraits_Samsung<T> Traits;
  PP_MEDIA_PLAYER_SAMSUNG_STATIC_ASSERT(
      Traits::kType != PP_ELEMENTARYSTREAM_TYPE_SAMSUNG_UNKNOWN,
      "Bad stream type!");
  return AddStream(Traits::kType, callback.output(), listener, callback);
}

}  // namespace pp

#endif  // PPAPI_CPP_SAMSUNG_ES_DATA_SOURCE_SAMSUNG_H_
