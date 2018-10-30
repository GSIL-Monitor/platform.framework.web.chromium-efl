// Copyright 2015-2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Multiply-included file, no traditional include guard.

#include "base/values.h"
#include "common/hit_test_params.h"
#include "common/navigation_policy_params.h"
#include "common/print_pages_params.h"
#include "common/web_preferences_efl.h"
#include "ipc/ipc_channel_handle.h"
#include "ipc/ipc_message_macros.h"
#include "ipc_message_start_ewk.h"
#include "private/ewk_hit_test_private.h"
#include "private/ewk_wrt_private.h"
#include "public/ewk_hit_test_internal.h"
#include "public/ewk_view_internal.h"
#include "third_party/WebKit/public/web/WebNavigationPolicy.h"
#include "third_party/WebKit/public/web/WebNavigationType.h"
#include "third_party/WebKit/public/web/WebViewModeEnums.h"
#include "ui/gfx/geometry/rect_f.h"

#if defined(OS_TIZEN_TV_PRODUCT)
#include "third_party/WebKit/public/platform/WebScrollbar.h"
#endif

typedef std::map<std::string, std::string> StringMap;
typedef std::map<std::string, bool> ExtensibleApiMap;

#define IPC_MESSAGE_START EwkMsgStart

IPC_STRUCT_TRAITS_BEGIN(Ewk_Wrt_Message_Data)
  IPC_STRUCT_TRAITS_MEMBER(type)
  IPC_STRUCT_TRAITS_MEMBER(value)
  IPC_STRUCT_TRAITS_MEMBER(id)
  IPC_STRUCT_TRAITS_MEMBER(reference_id)
IPC_STRUCT_TRAITS_END()

IPC_STRUCT_TRAITS_BEGIN(WebPreferencesEfl)
  IPC_STRUCT_TRAITS_MEMBER(shrinks_viewport_content_to_fit)
  IPC_STRUCT_TRAITS_MEMBER(javascript_can_open_windows_automatically_ewk)
  IPC_STRUCT_TRAITS_MEMBER(hw_keyboard_connected)
IPC_STRUCT_TRAITS_END()

IPC_STRUCT_TRAITS_BEGIN(Hit_Test_Params::Node_Data)
  IPC_STRUCT_TRAITS_MEMBER(tagName)
  IPC_STRUCT_TRAITS_MEMBER(nodeValue)
  IPC_STRUCT_TRAITS_MEMBER(attributes)
IPC_STRUCT_TRAITS_END()

IPC_STRUCT_TRAITS_BEGIN(Hit_Test_Params::Image_Data)
  IPC_STRUCT_TRAITS_MEMBER(fileNameExtension)
  IPC_STRUCT_TRAITS_MEMBER(imageBitmap)
IPC_STRUCT_TRAITS_END()

IPC_STRUCT_TRAITS_BEGIN(Hit_Test_Params)
  IPC_STRUCT_TRAITS_MEMBER(context)
  IPC_STRUCT_TRAITS_MEMBER(linkURI)
  IPC_STRUCT_TRAITS_MEMBER(linkTitle)
  IPC_STRUCT_TRAITS_MEMBER(linkLabel)
  IPC_STRUCT_TRAITS_MEMBER(imageURI)
  IPC_STRUCT_TRAITS_MEMBER(isEditable)
  IPC_STRUCT_TRAITS_MEMBER(mode)
  IPC_STRUCT_TRAITS_MEMBER(nodeData)
  IPC_STRUCT_TRAITS_MEMBER(imageData)
IPC_STRUCT_TRAITS_END()

IPC_ENUM_TRAITS(Ewk_CSP_Header_Type)

IPC_ENUM_TRAITS(Ewk_Hit_Test_Mode)

IPC_STRUCT_TRAITS_BEGIN(DidPrintPagesParams)
  IPC_STRUCT_TRAITS_MEMBER(metafile_data_handle)
  IPC_STRUCT_TRAITS_MEMBER(data_size)
  IPC_STRUCT_TRAITS_MEMBER(document_cookie)
  IPC_STRUCT_TRAITS_MEMBER(filename)
IPC_STRUCT_TRAITS_END()

IPC_ENUM_TRAITS(blink::WebNavigationPolicy)
IPC_ENUM_TRAITS(blink::WebNavigationType)

IPC_STRUCT_TRAITS_BEGIN(NavigationPolicyParams)
  IPC_STRUCT_TRAITS_MEMBER(render_view_id)
  IPC_STRUCT_TRAITS_MEMBER(cookie)
  IPC_STRUCT_TRAITS_MEMBER(url)
  IPC_STRUCT_TRAITS_MEMBER(httpMethod)
  IPC_STRUCT_TRAITS_MEMBER(referrer)
  IPC_STRUCT_TRAITS_MEMBER(policy)
  IPC_STRUCT_TRAITS_MEMBER(type)
  IPC_STRUCT_TRAITS_MEMBER(should_replace_current_entry)
  IPC_STRUCT_TRAITS_MEMBER(is_main_frame)
  IPC_STRUCT_TRAITS_MEMBER(is_redirect)
IPC_STRUCT_TRAITS_END()

IPC_ENUM_TRAITS(blink::WebViewMode)

IPC_MESSAGE_ROUTED1(EwkViewMsg_IsVideoPlaying, int /* calback id */)

IPC_MESSAGE_ROUTED2(EwkHostMsg_IsVideoPlaying,
                    bool, /* isplaying */
                    int /* calback id */)

#if defined(OS_TIZEN_TV_PRODUCT)
IPC_SYNC_MESSAGE_ROUTED1_1(EwkViewMsg_TranslatedUrlSet,
                           std::string, /* translated url */
                           bool /* translated result */)
#endif

IPC_MESSAGE_CONTROL2(WrtMsg_ParseUrl,
                     int,            // result: request_id
                     GURL)           // result: url

IPC_MESSAGE_CONTROL2(WrtMsg_ParseUrlResponse,
                     int,            // result: request_id
                     GURL)           // result: url

IPC_MESSAGE_CONTROL1(WrtMsg_SendWrtMessage,
                     Ewk_Wrt_Message_Data /* data */)

IPC_SYNC_MESSAGE_ROUTED1_1(EwkHostMsg_WrtSyncMessage,
                           Ewk_Wrt_Message_Data /* data */,
                           std::string /*result*/)

IPC_MESSAGE_ROUTED1(EwkHostMsg_WrtMessage,
                    Ewk_Wrt_Message_Data /* data */)

IPC_MESSAGE_CONTROL2(EwkViewHostMsg_HitTestReply,
                    int, /* render_view_id */
                    Hit_Test_Params);

IPC_MESSAGE_CONTROL3(EwkViewHostMsg_HitTestAsyncReply,
                    int, /* render_view_id */
                    Hit_Test_Params,
                    int64_t /* request id */)

IPC_MESSAGE_ROUTED2(EwkViewMsg_SetCSP,
                    std::string, /* policy */
                    Ewk_CSP_Header_Type /* header type */)

IPC_MESSAGE_ROUTED0(EwkViewMsg_UpdateFocusedNodeBounds)

IPC_MESSAGE_ROUTED1(EwkHostMsg_DidChangeFocusedNodeBounds,
                    gfx::RectF /* focused node bounds */)

IPC_SYNC_MESSAGE_ROUTED0_2(EwkHostMsg_GetContentSecurityPolicy,
                           std::string, /* policy */
                           Ewk_CSP_Header_Type /* header type */)

IPC_MESSAGE_ROUTED0(EwkViewMsg_ScrollFocusedNodeIntoView);

IPC_MESSAGE_ROUTED1(EwkHostMsg_DidPrintPagesToPdf,
                    DidPrintPagesParams /* pdf document parameters */)

IPC_MESSAGE_CONTROL0(EflViewMsg_ClearCache)
IPC_MESSAGE_CONTROL1(EflViewMsg_SetCache, int64_t /* cache_total_capacity */)

IPC_MESSAGE_ROUTED3(EwkViewMsg_PrintToPdf,
                    int, /* width */
                    int, /* height */
                    base::FilePath /* file name to save pdf*/)

IPC_MESSAGE_ROUTED1(EwkViewMsg_GetMHTMLData,
                    int /* callback id */)

IPC_MESSAGE_ROUTED3(EwkViewMsg_DoHitTest,
                           int, /* horizontal position */
                           int, /* vertical position */
                           Ewk_Hit_Test_Mode /* mode */)

IPC_MESSAGE_ROUTED4(EwkViewMsg_DoHitTestAsync,
                    int, /* horizontal position */
                    int, /* vertical position */
                    Ewk_Hit_Test_Mode, /* mode */
                    int64_t /* request id */)

// Tells the renderer to clear the cache.
IPC_MESSAGE_ROUTED0(EwkViewMsg_UseSettingsFont)
IPC_MESSAGE_ROUTED0(EwkViewMsg_SetBrowserFont)
IPC_MESSAGE_ROUTED0(EwkViewMsg_SuspendScheduledTask)
IPC_MESSAGE_ROUTED0(EwkViewMsg_ResumeScheduledTasks)

IPC_MESSAGE_ROUTED1(EwkViewMsg_SetMainFrameScrollbarVisible, bool /* visible */)

IPC_MESSAGE_ROUTED1(EwkViewMsg_GetMainFrameScrollbarVisible,
                    int /* callback id */)

IPC_MESSAGE_ROUTED2(EwkHostMsg_GetMainFrameScrollbarVisible,
                    bool /* visible */,
                    int /* callback id */)

IPC_MESSAGE_ROUTED2(EwkViewMsg_SetScroll,
                    int, /* horizontal position */
                    int /* vertical position */)

IPC_MESSAGE_ROUTED0(EwkViewMsg_SuspendNetworkLoading)
IPC_MESSAGE_ROUTED0(EwkViewMsg_ResumeNetworkLoading)

#if defined(TIZEN_ATK_FEATURE_VD)
IPC_MESSAGE_ROUTED1(EwkHostMsg_MoveFocusToBrowser, int /* direction */);
#endif

IPC_MESSAGE_ROUTED1(EwkViewMsg_PlainTextGet,
                    int /* callback id */)

IPC_MESSAGE_ROUTED1(EwkSettingsMsg_UpdateWebKitPreferencesEfl, WebPreferencesEfl)

IPC_MESSAGE_ROUTED1(EwkHostMsg_HandleTapGestureWithContext,
                    bool /* is_editable_content */)

IPC_MESSAGE_ROUTED2(EwkHostMsg_PlainTextGetContents,
                    std::string, /* contentText */
                    int /* callback id */)

IPC_MESSAGE_ROUTED2(EwkHostMsg_DidChangeContentsSize,
                    int, /* width */
                    int /* height */)

IPC_MESSAGE_ROUTED2(EwkHostMsg_DidChangeMaxScrollOffset,
                    int, /*max scrollX*/
                    int  /*max scrollY*/)

IPC_MESSAGE_ROUTED2(EwkHostMsg_DidChangeScrollOffset,
                    int, /*scrollX*/
                    int  /*scrollY*/)

IPC_MESSAGE_ROUTED2(EwkHostMsg_ReadMHTMLData,
                    std::string, /* Mhtml text */
                    int /* callback id */)

IPC_MESSAGE_ROUTED1(EwkViewMsg_GetBackgroundColor, int /* callback id */)

IPC_MESSAGE_ROUTED5(EwkHostMsg_GetBackgroundColor,
                    int, /* red */
                    int, /* green */
                    int, /* blue */
                    int, /* alpha */
                    int /* callback id */)

IPC_MESSAGE_ROUTED4(EwkViewMsg_SetBackgroundColor,
                    int, /* red */
                    int, /* green */
                    int, /* blue */
                    int /* alpha */)

// Notifies the browser to form submit
IPC_MESSAGE_ROUTED1(EwkHostMsg_FormSubmit, GURL)

IPC_MESSAGE_ROUTED1(EwkViewMsg_WebAppIconUrlGet,
                    int /* callback id */)

IPC_MESSAGE_ROUTED2(EwkHostMsg_WebAppIconUrlGet,
                    std::string, /* icon url */
                    int /* callback id */)

IPC_MESSAGE_ROUTED1(EwkViewMsg_WebAppIconUrlsGet,
                    int /* callback id */)

IPC_MESSAGE_ROUTED2(EwkHostMsg_WebAppIconUrlsGet,
                    StringMap, /* icon urls */
                    int /* callback id */)

IPC_MESSAGE_ROUTED1(EwkViewMsg_WebAppCapableGet,
                    int /* calback id */)

IPC_MESSAGE_ROUTED2(EwkHostMsg_WebAppCapableGet,
                    bool, /* capable */
                    int /* calback id */)

#if defined(TIZEN_VIDEO_HOLE)
IPC_MESSAGE_ROUTED1(EwkViewMsg_SetVideoHole, bool /* Enable */)
#endif
IPC_SYNC_MESSAGE_CONTROL1_1(EwkHostMsg_DecideNavigationPolicy,
                            NavigationPolicyParams,
                            bool /*handled*/)

// FIXME: error: ‘WebNavigationTypeOther’ is not a member of ‘blink’
IPC_MESSAGE_ROUTED1(ViewMsg_SetViewMode,
                    blink::WebViewMode /* view_mode */)

IPC_MESSAGE_ROUTED1(ViewMsg_SetTextZoomFactor,
                    float /*font zoom factor*/)

IPC_MESSAGE_ROUTED1(EwkFrameMsg_LoadNotFoundErrorPage,
                    std::string /* error url */)

IPC_MESSAGE_ROUTED1(EwkFrameMsg_MoveToNextOrPreviousSelectElement,
                    bool /* next */)
IPC_MESSAGE_ROUTED0(EwkFrameMsg_RequestSelectCollectionInformation)
IPC_MESSAGE_ROUTED4(EwkHostMsg_RequestSelectCollectionInformationUpdateACK,
                    int /* formElementCount */,
                    int /* currentNodeIndex */,
                    bool /* prevState */,
                    bool /* nextState */)

IPC_MESSAGE_ROUTED1(EwkViewMsg_SetLongPollingGlobalTimeout,
                    unsigned long /* timeout */)

IPC_MESSAGE_CONTROL1(EwkProcessMsg_UpdateTizenExtensible,
                     ExtensibleApiMap /* Extensible APIs */)
IPC_MESSAGE_CONTROL2(EwkProcessMsg_SetExtensibleAPI,
                     std::string /* api name */,
                     bool /* enable */)

IPC_MESSAGE_ROUTED0(EwkHostMsg_DidNotAllowScript)

IPC_MESSAGE_ROUTED0(EwkViewMsg_QueryNumberFieldAttributes)
IPC_MESSAGE_ROUTED2(EwkHostMsg_DidChangeNumberFieldAttributes,
                    bool /* is_minimum_negative*/,
                    bool /* is_step_integer */)

#if defined(OS_TIZEN_TV_PRODUCT)
IPC_ENUM_TRAITS(blink::WebScrollbar::Orientation)
IPC_MESSAGE_ROUTED2(EwkViewHostMsg_ScrollbarThumbPartFocusChanged,
                    blink::WebScrollbar::Orientation, /* orientation */
                    bool /* focused */)

IPC_MESSAGE_ROUTED2(EwkViewMsg_EdgeScrollBy,
                    gfx::Point, /* offset */
                    gfx::Point /* mouse position */)

IPC_MESSAGE_ROUTED2(EwkHostMsg_DidEdgeScrollBy,
                    gfx::Point, /* offset */
                    bool /* handled */)

IPC_MESSAGE_ROUTED0(EwkHostMsg_RunArrowScroll);

IPC_MESSAGE_ROUTED3(EwkViewMsg_RequestHitTestForMouseLongPress,
                    int, /* render_id */
                    int, /* view x */
                    int /* view y */)

IPC_MESSAGE_CONTROL5(EwkViewHostMsg_RequestHitTestForMouseLongPressACK,
                     int,  /* render view id */
                     GURL, /* hit url */
                     int,  /* render id */
                     int,  /* view x */
                     int /* view y */)

IPC_MESSAGE_ROUTED1(EwkViewMsg_SetFloatVideoWindowState, bool /* Enable */)

IPC_MESSAGE_CONTROL1(HbbtvMsg_RegisterURLSchemesAsCORSEnabled,
                     std::string /* scheme */)

IPC_MESSAGE_CONTROL1(HbbtvMsg_RegisterJSPluginMimeTypes,
                     std::string /* mime types */)

IPC_MESSAGE_CONTROL1(HbbtvMsg_SetTimeOffset, double /* time offset */)

// Sent when the renderer was prevented from displaying insecure content in
// a secure page by a security policy.  The page may appear incomplete.
IPC_MESSAGE_ROUTED0(EwkHostMsg_DidBlockInsecureContent)

// Sent in response to FrameHostMsg_DidBlockDisplayingInsecureContent.
IPC_MESSAGE_ROUTED1(EwkMsg_SetAllowInsecureContent, bool /* allowed */)
IPC_MESSAGE_ROUTED1(EwkViewMsg_SetPreferTextLang, std::string /*prefer lang*/)
IPC_MESSAGE_ROUTED2(EwkViewMsg_ParentalRatingResultSet,
                    std::string, /*media url*/
                    bool /*is_pass*/)

IPC_MESSAGE_ROUTED5(EwkHostMsg_LoginFormSubmitted,
                    std::string, /* user name */
                    std::string, /* password */
                    std::string, /* username element */
                    std::string, /* password element */
                    std::string /* action url */)

IPC_MESSAGE_ROUTED5(EwkHostMsg_LoginFieldsIdentified,
                    int,         /* login form type */
                    std::string, /* username */
                    std::string, /* username element */
                    std::string, /* password element */
                    std::string /* action url */)

IPC_MESSAGE_ROUTED2(EwkViewMsg_AutoLogin,
                    std::string, /* user name */
                    std::string /* password */)
#endif

IPC_MESSAGE_ROUTED0(EwkViewMsg_QueryInputType)
IPC_MESSAGE_ROUTED1(EwkHostMsg_DidChangeInputType, bool /* is_password_input */)
