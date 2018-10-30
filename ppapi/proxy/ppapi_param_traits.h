// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_PROXY_PPAPI_PARAM_TRAITS_H_
#define PPAPI_PROXY_PPAPI_PARAM_TRAITS_H_

#include <string>
#include <vector>

#include "build/build_config.h"
#include "ipc/ipc_message_utils.h"
#include "ipc/ipc_platform_file.h"
#include "ppapi/c/pp_completion_callback.h"
#include "ppapi/c/pp_rect.h"
#include "ppapi/c/pp_var.h"
#include "ppapi/proxy/ppapi_proxy_export.h"
#include "ppapi/shared_impl/compositor_layer_data.h"
#include "ppapi/shared_impl/file_path.h"
#include "ppapi/shared_impl/file_ref_create_info.h"
#include "ppapi/shared_impl/media_stream_video_track_shared.h"
#include "ppapi/shared_impl/ppapi_permissions.h"
#include "ppapi/shared_impl/ppb_tcp_socket_shared.h"
#include "ppapi/shared_impl/socket_option_data.h"

#if defined(TIZEN_PEPPER_PLAYER)
#include "ppapi/c/samsung/pp_media_player_samsung.h"
#endif  // defined(TIZEN_PEPPER_PLAYER)

struct PP_KeyInformation;
struct PP_NetAddress_Private;

#if defined(TIZEN_PEPPER_TEEC)
struct PP_TEEC_Operation;
#endif  // defined(TIZEN_PEPPER_TEEC)

namespace ppapi {

class HostResource;
class PPB_X509Certificate_Fields;

namespace proxy {

struct PPBFlash_DrawGlyphs_Params;
struct PPBURLLoader_UpdateProgress_Params;
struct SerializedDirEntry;
struct SerializedFontDescription;
struct SerializedTrueTypeFontDesc;
class SerializedFlashMenu;
class SerializedHandle;
class SerializedVar;

}  // namespace proxy
}  // namespace ppapi

namespace IPC {

template<>
struct PPAPI_PROXY_EXPORT ParamTraits<PP_Bool> {
  typedef PP_Bool param_type;
  static void Write(base::Pickle* m, const param_type& p);
  static bool Read(const base::Pickle* m,
                   base::PickleIterator* iter,
                   param_type* r);
  static void Log(const param_type& p, std::string* l);
};

template <>
struct PPAPI_PROXY_EXPORT ParamTraits<PP_NetAddress_Private> {
  typedef PP_NetAddress_Private param_type;
  static void Write(base::Pickle* m, const param_type& p);
  static bool Read(const base::Pickle* m,
                   base::PickleIterator* iter,
                   param_type* p);
  static void Log(const param_type& p, std::string* l);
};

template <>
struct PPAPI_PROXY_EXPORT ParamTraits<PP_KeyInformation> {
  typedef PP_KeyInformation param_type;
  static void Write(base::Pickle* m, const param_type& p);
  static bool Read(const base::Pickle* m,
                   base::PickleIterator* iter,
                   param_type* p);
  static void Log(const param_type& p, std::string* l);
};

template<>
struct PPAPI_PROXY_EXPORT ParamTraits<
    ppapi::proxy::PPBFlash_DrawGlyphs_Params> {
  typedef ppapi::proxy::PPBFlash_DrawGlyphs_Params param_type;
  static void Write(base::Pickle* m, const param_type& p);
  static bool Read(const base::Pickle* m,
                   base::PickleIterator* iter,
                   param_type* r);
  static void Log(const param_type& p, std::string* l);
};

template<>
struct PPAPI_PROXY_EXPORT ParamTraits<
    ppapi::proxy::PPBURLLoader_UpdateProgress_Params> {
  typedef ppapi::proxy::PPBURLLoader_UpdateProgress_Params param_type;
  static void Write(base::Pickle* m, const param_type& p);
  static bool Read(const base::Pickle* m,
                   base::PickleIterator* iter,
                   param_type* r);
  static void Log(const param_type& p, std::string* l);
};

template<>
struct PPAPI_PROXY_EXPORT ParamTraits<ppapi::proxy::SerializedDirEntry> {
  typedef ppapi::proxy::SerializedDirEntry param_type;
  static void Write(base::Pickle* m, const param_type& p);
  static bool Read(const base::Pickle* m,
                   base::PickleIterator* iter,
                   param_type* r);
  static void Log(const param_type& p, std::string* l);
};

template<>
struct PPAPI_PROXY_EXPORT ParamTraits<ppapi::proxy::SerializedFontDescription> {
  typedef ppapi::proxy::SerializedFontDescription param_type;
  static void Write(base::Pickle* m, const param_type& p);
  static bool Read(const base::Pickle* m,
                   base::PickleIterator* iter,
                   param_type* r);
  static void Log(const param_type& p, std::string* l);
};

template<>
struct PPAPI_PROXY_EXPORT
    ParamTraits<ppapi::proxy::SerializedTrueTypeFontDesc> {
  typedef ppapi::proxy::SerializedTrueTypeFontDesc param_type;
  static void Write(base::Pickle* m, const param_type& p);
  static bool Read(const base::Pickle* m,
                   base::PickleIterator* iter,
                   param_type* r);
  static void Log(const param_type& p, std::string* l);
};

template<>
struct PPAPI_PROXY_EXPORT ParamTraits<ppapi::proxy::SerializedHandle> {
  typedef ppapi::proxy::SerializedHandle param_type;
  static void Write(base::Pickle* m, const param_type& p);
  static bool Read(const base::Pickle* m,
                   base::PickleIterator* iter,
                   param_type* r);
  static void Log(const param_type& p, std::string* l);
};

template<>
struct PPAPI_PROXY_EXPORT ParamTraits<ppapi::HostResource> {
  typedef ppapi::HostResource param_type;
  static void Write(base::Pickle* m, const param_type& p);
  static bool Read(const base::Pickle* m,
                   base::PickleIterator* iter,
                   param_type* r);
  static void Log(const param_type& p, std::string* l);
};

template<>
struct PPAPI_PROXY_EXPORT ParamTraits<ppapi::proxy::SerializedVar> {
  typedef ppapi::proxy::SerializedVar param_type;
  static void Write(base::Pickle* m, const param_type& p);
  static bool Read(const base::Pickle* m,
                   base::PickleIterator* iter,
                   param_type* r);
  static void Log(const param_type& p, std::string* l);
};

template<>
struct PPAPI_PROXY_EXPORT ParamTraits<
    std::vector<ppapi::proxy::SerializedVar> > {
  typedef std::vector<ppapi::proxy::SerializedVar> param_type;
  static void Write(base::Pickle* m, const param_type& p);
  static bool Read(const base::Pickle* m,
                   base::PickleIterator* iter,
                   param_type* r);
  static void Log(const param_type& p, std::string* l);
};

template<>
struct PPAPI_PROXY_EXPORT ParamTraits<ppapi::PpapiPermissions> {
  typedef ppapi::PpapiPermissions param_type;
  static void Write(base::Pickle* m, const param_type& p);
  static bool Read(const base::Pickle* m,
                   base::PickleIterator* iter,
                   param_type* r);
  static void Log(const param_type& p, std::string* l);
};

#if !defined(OS_NACL) && !defined(NACL_WIN64)
template <>
struct ParamTraits<ppapi::PepperFilePath> {
  typedef ppapi::PepperFilePath param_type;
  static void Write(base::Pickle* m, const param_type& p);
  static bool Read(const base::Pickle* m,
                   base::PickleIterator* iter,
                   param_type* p);
  static void Log(const param_type& p, std::string* l);
};

template<>
struct PPAPI_PROXY_EXPORT ParamTraits<ppapi::proxy::SerializedFlashMenu> {
  typedef ppapi::proxy::SerializedFlashMenu param_type;
  static void Write(base::Pickle* m, const param_type& p);
  static bool Read(const base::Pickle* m,
                   base::PickleIterator* iter,
                   param_type* r);
  static void Log(const param_type& p, std::string* l);
};
#endif  // !defined(OS_NACL) && !defined(NACL_WIN64)

template<>
struct PPAPI_PROXY_EXPORT ParamTraits<ppapi::PPB_X509Certificate_Fields> {
  typedef ppapi::PPB_X509Certificate_Fields param_type;
  static void Write(base::Pickle* m, const param_type& p);
  static bool Read(const base::Pickle* m,
                   base::PickleIterator* iter,
                   param_type* r);
  static void Log(const param_type& p, std::string* l);
};

template<>
struct PPAPI_PROXY_EXPORT ParamTraits<ppapi::SocketOptionData> {
  typedef ppapi::SocketOptionData param_type;
  static void Write(base::Pickle* m, const param_type& p);
  static bool Read(const base::Pickle* m,
                   base::PickleIterator* iter,
                   param_type* r);
  static void Log(const param_type& p, std::string* l);
};


template <>
struct PPAPI_PROXY_EXPORT ParamTraits<ppapi::DataView> {
  typedef ppapi::DataView param_type;
  static void Write(base::Pickle* m, const param_type& p);
  static bool Read(const base::Pickle* m,
                   base::PickleIterator* iter,
                   param_type* r);
  static void Log(const param_type& p, std::string* l);
};

#if defined(TIZEN_PEPPER_PLAYER)
namespace internal {
struct PPAPI_PROXY_EXPORT CharSequenceParamTraits {
  static void Write(base::Pickle* m, const char* p, size_t size);
  static bool Read(const base::Pickle* m,
                   base::PickleIterator* iter,
                   char* p,
                   size_t size);
  static void Log(const char* p, size_t size, std::string* l);
};
}  // namespace internal

template <size_t N>
struct PPAPI_PROXY_EXPORT ParamTraits<char[N]> {
  static void Write(base::Pickle* m, const char (&p)[N]) {
    internal::CharSequenceParamTraits::Write(m, p, N);
  }
  static bool Read(const base::Pickle* m,
                   base::PickleIterator* iter,
                   char (*p)[N]) {
    return internal::CharSequenceParamTraits::Read(m, iter, *p, N);
  }
  static void Log(const char (&p)[N], std::string* l) {
    internal::CharSequenceParamTraits::Log(p, N, l);
  }
};

template <>
struct PPAPI_PROXY_EXPORT ParamTraits<PP_ESPacket> {
  typedef PP_ESPacket param_type;
  static void Write(base::Pickle* m, const param_type& p);
  static bool Read(const base::Pickle* m,
                   base::PickleIterator* iter,
                   param_type* r);
  static void Log(const param_type& p, std::string* l);
};

template <>
struct PPAPI_PROXY_EXPORT ParamTraits<PP_ESPacketEncryptionInfo> {
  typedef PP_ESPacketEncryptionInfo param_type;
  static void Write(base::Pickle* m, const param_type& p);
  static bool Read(const base::Pickle* m,
                   base::PickleIterator* iter,
                   param_type* r);
  static void Log(const param_type& p, std::string* l);
};
#endif  // defined(TIZEN_PEPPER_PLAYER)

#if defined(TIZEN_PEPPER_TEEC)
namespace internal {
struct PPAPI_PROXY_EXPORT Uint8SequenceParamTraits {
  static void Write(base::Pickle* m, const uint8_t* p, size_t size);
  static bool Read(const base::Pickle* m,
                   base::PickleIterator* iter,
                   uint8_t* p,
                   size_t size);
  static void Log(const uint8_t* p, size_t size, std::string* l);
};
}  // namespace internal

template <size_t N>
struct PPAPI_PROXY_EXPORT ParamTraits<uint8_t[N]> {
  static void Write(base::Pickle* m, const uint8_t (&p)[N]) {
    internal::Uint8SequenceParamTraits::Write(m, p, N);
  }
  static bool Read(const base::Pickle* m,
                   base::PickleIterator* iter,
                   uint8_t (*p)[N]) {
    return internal::Uint8SequenceParamTraits::Read(m, iter, *p, N);
  }
  static void Log(const uint8_t (&p)[N], std::string* l) {
    internal::Uint8SequenceParamTraits::Log(p, N, l);
  }
};

template <>
struct PPAPI_PROXY_EXPORT ParamTraits<PP_TEEC_Operation> {
  typedef PP_TEEC_Operation param_type;
  static void Write(base::Pickle* m, const PP_TEEC_Operation& p);
  static bool Read(const base::Pickle* m,
                   base::PickleIterator* iter,
                   PP_TEEC_Operation* p);
  static void Log(const PP_TEEC_Operation& p, std::string* l);
};

#endif  // defined(TIZEN_PEPPER_TEEC)

}  // namespace IPC

#endif  // PPAPI_PROXY_PPAPI_PARAM_TRAITS_H_
