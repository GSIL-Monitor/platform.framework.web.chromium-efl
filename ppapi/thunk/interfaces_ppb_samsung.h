// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Please see inteface_ppb_public_stable for the documentation on the format of
// this file.

#include "ppapi/thunk/interfaces_preamble.h"

#if defined(TIZEN_PEPPER_EXTENSIONS)
#if !defined(OS_NACL)
PROXIED_IFACE(PPB_EXTENSIONSYSTEM_SAMSUNG_INTERFACE_0_1,
              PPB_ExtensionSystem_Samsung_0_1)
PROXIED_IFACE(PPB_REMOTECONTROLLER_SAMSUNG_INTERFACE_0_1,
              PPB_RemoteController_Samsung_0_1)
#endif  // !defined(OS_NACL)
PROXIED_IFACE(PPB_AUDIO_CONFIG_SAMSUNG_INTERFACE_1_0,
              PPB_AudioConfig_Samsung_1_0)
PROXIED_IFACE(PPB_COMPOSITORLAYER_SAMSUNG_INTERFACE_0_1,
              PPB_CompositorLayer_Samsung_0_1)
PROXIED_IFACE(PPB_UDPSOCKETEXTENSION_SAMSUNG_INTERFACE_0_1,
              PPB_UDPSocketExtension_Samsung_0_1)
PROXIED_IFACE(PPB_VIDEODECODER_SAMSUNG_INTERFACE_1_0,
              PPB_VideoDecoder_Samsung_1_0)
#endif  // defined(TIZEN_PEPPER_EXTENSIONS)

#if defined(TIZEN_PEPPER_PLAYER)
PROXIED_IFACE(PPB_MEDIAPLAYER_SAMSUNG_INTERFACE_1_0,
              PPB_MediaPlayer_Samsung_1_0)
PROXIED_IFACE(PPB_MEDIAPLAYER_SAMSUNG_INTERFACE_1_1,
              PPB_MediaPlayer_Samsung_1_1)
PROXIED_IFACE(PPB_MEDIAPLAYER_SAMSUNG_INTERFACE_1_2,
              PPB_MediaPlayer_Samsung_1_2)
PROXIED_IFACE(PPB_MEDIAPLAYER_SAMSUNG_INTERFACE_1_3,
              PPB_MediaPlayer_Samsung_1_3)
PROXIED_IFACE(PPB_MEDIAPLAYER_SAMSUNG_INTERFACE_1_4,
              PPB_MediaPlayer_Samsung_1_4)
PROXIED_IFACE(PPB_MEDIADATASOURCE_SAMSUNG_INTERFACE_1_0,
              PPB_MediaDataSource_Samsung_1_0)
PROXIED_IFACE(PPB_URLDATASOURCE_SAMSUNG_INTERFACE_1_0,
              PPB_URLDataSource_Samsung_1_0)
PROXIED_IFACE(PPB_ESDATASOURCE_SAMSUNG_INTERFACE_1_0,
              PPB_ESDataSource_Samsung_1_0)
PROXIED_IFACE(PPB_ELEMENTARYSTREAM_SAMSUNG_INTERFACE_1_0,
              PPB_ElementaryStream_Samsung_1_0)
PROXIED_IFACE(PPB_ELEMENTARYSTREAM_SAMSUNG_INTERFACE_1_1,
              PPB_ElementaryStream_Samsung_1_1)
PROXIED_IFACE(PPB_AUDIOELEMENTARYSTREAM_SAMSUNG_INTERFACE_1_0,
              PPB_AudioElementaryStream_Samsung_1_0)
PROXIED_IFACE(PPB_VIDEOELEMENTARYSTREAM_SAMSUNG_INTERFACE_1_0,
              PPB_VideoElementaryStream_Samsung_1_0)
#endif

#if defined(TIZEN_PEPPER_TEEC)
PROXIED_IFACE(PPB_TEECCONTEXT_SAMSUNG_INTERFACE_1_0,
              PPB_TEECContext_Samsung_1_0)
PROXIED_IFACE(PPB_TEECSESSION_SAMSUNG_INTERFACE_1_0,
              PPB_TEECSession_Samsung_1_0)
PROXIED_IFACE(PPB_TEECSHAREDMEMORY_SAMSUNG_INTERFACE_1_0,
              PPB_TEECSharedMemory_Samsung_1_0)
#endif  // defined(TIZEN_PEPPER_TEEC)

#include "ppapi/thunk/interfaces_postamble.h"
