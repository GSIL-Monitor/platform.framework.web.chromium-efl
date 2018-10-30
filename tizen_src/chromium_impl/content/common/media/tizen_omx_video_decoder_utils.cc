// Copyright 2018 Samsung Electronics Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/media/tizen_omx_video_decoder_utils.h"

namespace content {

OMX_VIDEO_CODINGTYPE VideoCodecToOmxCodingType(const media::VideoCodec codec) {
  switch (codec) {
    case media::kCodecH264:
      return OMX_VIDEO_CodingAVC;
    case media::kCodecVC1:
      return OMX_VIDEO_CodingWMV;
    case media::kCodecMPEG2:
      return OMX_VIDEO_CodingMPEG2;
    case media::kCodecMPEG4:
      return OMX_VIDEO_CodingMPEG4;
    case media::kCodecVP8:
      return OMX_VIDEO_CodingVP8;
    case media::kCodecVP9:
      return OMX_VIDEO_CodingVP9;
    case media::kCodecHEVC:
      return OMX_VIDEO_CodingHEVC;
    case media::kCodecDolbyVision:
    case media::kCodecTheora:
    default:
      return OMX_VIDEO_CodingMax;
  }
}

#define ENUM_CASE(x) \
  case x:            \
    return #x;

const char* OmxCmdStr(OMX_COMMANDTYPE cmd) {
  switch (cmd) {
    ENUM_CASE(OMX_CommandStateSet);
    ENUM_CASE(OMX_CommandFlush);
    ENUM_CASE(OMX_CommandPortDisable);
    ENUM_CASE(OMX_CommandPortEnable);
    ENUM_CASE(OMX_CommandMarkBuffer);
    ENUM_CASE(OMX_CommandKhronosExtensions);
    ENUM_CASE(OMX_CommandVendorStartUnused);
    ENUM_CASE(OMX_CommandComponentDeInit);
    ENUM_CASE(OMX_CommandEmptyBuffer);
    ENUM_CASE(OMX_CommandFillBuffer);
    ENUM_CASE(OMX_CommandFakeBuffer);
    default:
      break;
  }
  return "<unknown command>";
}

const char* OmxEventStr(OMX_EVENTTYPE ev) {
  switch (ev) {
    ENUM_CASE(OMX_EventCmdComplete);
    ENUM_CASE(OMX_EventError);
    ENUM_CASE(OMX_EventMark);
    ENUM_CASE(OMX_EventPortSettingsChanged);
    ENUM_CASE(OMX_EventBufferFlag);
    ENUM_CASE(OMX_EventResourcesAcquired);
    ENUM_CASE(OMX_EventComponentResumed);
    ENUM_CASE(OMX_EventDynamicResourcesAvailable);
    ENUM_CASE(OMX_EventPortFormatDetected);
    ENUM_CASE(OMX_EventKhronosExtensions);
    ENUM_CASE(OMX_EventVendorStartUnused);
    ENUM_CASE(OMX_EventSdpVideoInfo);
    ENUM_CASE(OMX_EventSdpVbiData);
    ENUM_CASE(OMX_EventSdpUserdataEtc);
    ENUM_CASE(OMX_EventSdpColourDescription);
    ENUM_CASE(OMX_EventSdpMasteringDisplayColourVolume);
    ENUM_CASE(OMX_EventSdphdrcompatibilityinfo);
    default:
      break;
  }
  return "<unknown event type>";
}

const char* OmxStateStr(OMX_STATETYPE state) {
  switch (state) {
    ENUM_CASE(OMX_StateInvalid);
    ENUM_CASE(OMX_StateExecuting);
    ENUM_CASE(OMX_StateIdle);
    ENUM_CASE(OMX_StateKhronosExtensions);
    ENUM_CASE(OMX_StateLoaded);
    ENUM_CASE(OMX_StatePause);
    ENUM_CASE(OMX_StateWaitForResources);
    default:
      break;
  }
  return "Unknown_state";
}

const char* OmxErrStr(OMX_ERRORTYPE err) {
  switch (err) {
    ENUM_CASE(OMX_ErrorNone);
    ENUM_CASE(OMX_ErrorInsufficientResources);
    ENUM_CASE(OMX_ErrorUndefined);
    ENUM_CASE(OMX_ErrorInvalidComponentName);
    ENUM_CASE(OMX_ErrorComponentNotFound);
    ENUM_CASE(OMX_ErrorInvalidComponent);
    ENUM_CASE(OMX_ErrorBadParameter);
    ENUM_CASE(OMX_ErrorNotImplemented);
    ENUM_CASE(OMX_ErrorUnderflow);
    ENUM_CASE(OMX_ErrorOverflow);
    ENUM_CASE(OMX_ErrorHardware);
    ENUM_CASE(OMX_ErrorInvalidState);
    ENUM_CASE(OMX_ErrorStreamCorrupt);
    ENUM_CASE(OMX_ErrorPortsNotCompatible);
    ENUM_CASE(OMX_ErrorResourcesLost);
    ENUM_CASE(OMX_ErrorNoMore);
    ENUM_CASE(OMX_ErrorVersionMismatch);
    ENUM_CASE(OMX_ErrorNotReady);
    ENUM_CASE(OMX_ErrorTimeout);
    ENUM_CASE(OMX_ErrorSameState);
    ENUM_CASE(OMX_ErrorResourcesPreempted);
    ENUM_CASE(OMX_ErrorPortUnresponsiveDuringAllocation);
    ENUM_CASE(OMX_ErrorPortUnresponsiveDuringDeallocation);
    ENUM_CASE(OMX_ErrorPortUnresponsiveDuringStop);
    ENUM_CASE(OMX_ErrorIncorrectStateTransition);
    ENUM_CASE(OMX_ErrorIncorrectStateOperation);
    ENUM_CASE(OMX_ErrorUnsupportedSetting);
    ENUM_CASE(OMX_ErrorUnsupportedIndex);
    ENUM_CASE(OMX_ErrorBadPortIndex);
    ENUM_CASE(OMX_ErrorPortUnpopulated);
    ENUM_CASE(OMX_ErrorComponentSuspended);
    ENUM_CASE(OMX_ErrorDynamicResourcesUnavailable);
    ENUM_CASE(OMX_ErrorMbErrorsInFrame);
    ENUM_CASE(OMX_ErrorFormatNotDetected);
    ENUM_CASE(OMX_ErrorContentPipeOpenFailed);
    ENUM_CASE(OMX_ErrorContentPipeCreationFailed);
    ENUM_CASE(OMX_ErrorSeperateTablesUsed);
    ENUM_CASE(OMX_ErrorTunnelingUnsupported);
    ENUM_CASE(OMX_ErrorKhronosExtensions);
    ENUM_CASE(OMX_ErrorVendorStartUnused);
    ENUM_CASE(OMX_ErrorNoEOF);
    ENUM_CASE(OMX_ErrorInputDataDecodeYet);
    ENUM_CASE(OMX_ErrorInputDataEncodeYet);
    ENUM_CASE(OMX_ErrorCodecInit);
    ENUM_CASE(OMX_ErrorCodecDecode);
    ENUM_CASE(OMX_ErrorCodecEncode);
    ENUM_CASE(OMX_ErrorCodecFlush);
    ENUM_CASE(OMX_ErrorOutputBufferUseYet);
    ENUM_CASE(OMX_ErrorCorruptedFrame);
    ENUM_CASE(OMX_ErrorPollFail);
    default:
      break;
  }
  return "Unknown error";
}

}  // namespace content
