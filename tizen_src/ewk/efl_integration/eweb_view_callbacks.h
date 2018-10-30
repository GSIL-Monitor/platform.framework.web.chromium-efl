/*
 * Copyright (C) 2014-2016 Samsung Electronics. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 *
 */

#ifndef EWEB_VIEW_CALLBACKS
#define EWEB_VIEW_CALLBACKS

#include <Evas.h>
#include <Eina.h>

#include "eweb_view.h"
#include "private/ewk_certificate_info_private.h"
#include "private/ewk_certificate_private.h"
#include "private/ewk_console_message_private.h"
#include "private/ewk_context_menu_private.h"
#include "private/ewk_error_private.h"
#include "private/ewk_form_repost_decision_private.h"
#include "private/ewk_geolocation_private.h"
#include "private/ewk_policy_decision_private.h"
#include "private/ewk_user_media_private.h"

#if defined(OS_TIZEN_TV_PRODUCT)
#include "private/ewk_autofill_profile_private.h"
#include "public/ewk_view_product.h"
#endif

typedef struct EwkObject Ewk_Auth_Request;
typedef struct EwkObject Ewk_Download_Job;
typedef struct EwkObject Ewk_File_Chooser_Request;
typedef struct EwkObject Ewk_Form_Submission_Request;
typedef struct EwkObject Ewk_Navigation_Policy_Decision;
typedef struct _Ewk_User_Media_Permission_Request
  Ewk_User_Media_Permission_Request;

class Ewk_Wrt_Message_Data;

namespace EWebViewCallbacks {

enum CallbackType {
  AuthenticationRequest,
  BackForwardListChange,
  CancelVibration,
  CreateNewWindow,
  CreateNewWindowUrl,
  ContentsSizeChanged,
  DownloadJobCancelled,
  DownloadJobFailed,
  DownloadJobFinished,
  DownloadJobRequested,
  FileChooserRequest,
  NewFormSubmissionRequest,
  FaviconChanged,
  LoadError,
  LoadStarted,
  LoadFinished,
  LoadProgress,
  MenuBarVisible,
  PopupBlocked,
  ProvisionalLoadFailed,
  ProvisionalLoadRedirect,
  ProvisionalLoadStarted,
  LoadCommitted,
  StatusBarVisible,
  NavigationPolicyDecision,
  NewWindowPolicyDecision,
  TextFound,
  TitleChange,
  ToolbarVisible,
  TooltipTextUnset,
  TooltipTextSet,
  URLChanged,
  Vibrate,
  WebProcessCrashed,
  WindowClosed,
  WindowResizable,
  EnterFullscreen,
  ExitFullscreen,
  UserMediaPermission,
  IMEInputPanelShow,
  IMEInputPanelHide,
  IMECandidatePanelShow,
  IMECandidatePanelHide,
  IMEInputMethodChanged,
  GeoLocationPermissionRequest,
  GeoLocationValid,
  RequestCertificateConfirm,
  SSLCertificateChanged,
  PolicyResponseDecide,
  ContextMenuCustomize,
  ContextMenuItemSelected,
  LoadNonEmptyLayoutFinished,
  VideoPlayingURL,
  PopupReplyWaitStart,
  PopupReplyWaitFinish,
  FrameRendered,
#if defined(TIZEN_TBM_SUPPORT)
  OffscreenFrameRendered,
#endif
  RotatePrepared,
  TopControlMoved,
  CreateNewBackgroundWindow,
  EdgeLeft,
  EdgeRight,
  EdgeTop,
  EdgeBottom,
#if defined(OS_TIZEN_TV_PRODUCT)
  EdgeScrollLeft,
  EdgeScrollRight,
  EdgeScrollTop,
  EdgeScrollBottom,
  DidChagneScrollbarsThumbFocus,
  RunArrowScroll,
  HoverOverLink,
  HoverOutLink,
  VideoResized,
  ContextMenuShow,
  ContextMenuHide,
  PopupMenuShow,
  PopupMenuHide,
  DidBlockInsecureContent,
  PlaybackLoad,
  PlaybackReady,
  PlaybackStart,
  PlaybackFinish,
  PlaybackStop,
  SubtitlePlay,
  SubtitlePause,
  SubtitleStop,
  SubtitleResume,
  SubtitleSeekStart,
  SubtitleSeekComplete,
  SubtitleNotifyData,
  ParentalRatingInfo,
  LoginFormSubmitted,
  LoginFields,
#endif
#if defined(TIZEN_ATK_FEATURE_VD)
  AtkKeyEventNotHandled,
#endif
  TextSelectionMode,
  SaveSessionData,
  UndoSize,
  RedoSize,
  MagnifierShow,
  MagnifierHide,
  ClipboardOpened,
  ConsoleMessage,
  WrtPluginsMessage,
  IconReceived,
  FormSubmit,
  OverflowScrollOff,
  OverflowScrollOn,
  TouchmoveHandled,
  WebloginCheckboxClicked,
  WebloginCheckboxResume,
  WebloginReady,
  ZoomStarted,
  ZoomFinished,
#if defined(OS_TIZEN)
  NewWindowNavigationPolicyDecision,
#endif // OS_TIZEN
  URIChanged,
  DidNotAllowScript,
  FocusIn,
  FocusOut,
  ReaderMode,
  NotificationPermissionReply,
  FormRepostWarningShow,
  BeforeFormRepostWarningShow
};

template <CallbackType>
struct CallBackInfo;

class EvasObjectHolder {
protected:
  explicit EvasObjectHolder(Evas_Object* object)
      : m_object(object)
  {
  }

  Evas_Object* m_object;
};

template <CallbackType callbackType, typename ArgType = typename CallBackInfo<callbackType>::Type>
struct CallBack: public EvasObjectHolder {
  explicit CallBack(Evas_Object* view) : EvasObjectHolder(view) { }

  void call(ArgType argument) {
    evas_object_smart_callback_call(m_object, CallBackInfo<callbackType>::name(), static_cast<void*>(argument));
  }
};

template <CallbackType callbackType>
struct CallBack <callbackType, void> : public EvasObjectHolder {
  explicit CallBack(Evas_Object* view) : EvasObjectHolder(view) { }

  void call() {
    evas_object_smart_callback_call(m_object, CallBackInfo<callbackType>::name(), 0);
  }
};

template <CallbackType callbackType>
struct CallBack <callbackType, const char*> : public EvasObjectHolder {
  explicit CallBack(Evas_Object* view) : EvasObjectHolder(view) { }

  void call(const char* arg) {
    evas_object_smart_callback_call(m_object, CallBackInfo<callbackType>::name(), const_cast<char*>(arg));
  }
};

/* LCOV_EXCL_START */
#define DECLARE_EWK_VIEW_CALLBACK(callbackType, string, type) \
template <>                                                   \
struct CallBackInfo<callbackType> {                           \
  typedef type Type;                                          \
  static inline const char* name() { return string; }         \
}

// Copied from WebKit as is. TODO: implement each and delete this section.
/*
DECLARE_EWK_VIEW_CALLBACK(AuthenticationRequest, "authentication,request", Ewk_Auth_Request*);
DECLARE_EWK_VIEW_CALLBACK(CancelVibration, "cancel,vibration", void);
DECLARE_EWK_VIEW_CALLBACK(DownloadJobCancelled, "download,cancelled", Ewk_Download_Job*);
DECLARE_EWK_VIEW_CALLBACK(DownloadJobFailed, "download,failed", Ewk_Download_Job_Error*);
DECLARE_EWK_VIEW_CALLBACK(DownloadJobFinished, "download,finished", Ewk_Download_Job*);
DECLARE_EWK_VIEW_CALLBACK(DownloadJobRequested, "download,request", Ewk_Download_Job*);
DECLARE_EWK_VIEW_CALLBACK(NewFormSubmissionRequest, "form,submission,request", Ewk_Form_Submission_Request*);
DECLARE_EWK_VIEW_CALLBACK(FaviconChanged, "favicon,changed", void);
DECLARE_EWK_VIEW_CALLBACK(ProvisionalLoadFailed, "load,provisional,failed", Ewk_Error*);
DECLARE_EWK_VIEW_CALLBACK(NavigationPolicyDecision, "policy,decision,navigation", Ewk_Navigation_Policy_Decision*);
DECLARE_EWK_VIEW_CALLBACK(Vibrate, "vibrate", uint32_t*);
*/

// Note: type 'void' means that no arguments are expected.

DECLARE_EWK_VIEW_CALLBACK(FileChooserRequest, "file,chooser,request", Ewk_File_Chooser_Request*);
DECLARE_EWK_VIEW_CALLBACK(FormSubmit, "form,submit", const char*);
DECLARE_EWK_VIEW_CALLBACK(LoadFinished, "load,finished", void);
DECLARE_EWK_VIEW_CALLBACK(LoadStarted, "load,started", void);
DECLARE_EWK_VIEW_CALLBACK(LoadError, "load,error", _Ewk_Error*);
DECLARE_EWK_VIEW_CALLBACK(TitleChange, "title,changed", const char*);
DECLARE_EWK_VIEW_CALLBACK(URLChanged, "url,changed", const char*);
DECLARE_EWK_VIEW_CALLBACK(URIChanged, "uri,changed", const char*);
DECLARE_EWK_VIEW_CALLBACK(LoadProgress, "load,progress", double*);
DECLARE_EWK_VIEW_CALLBACK(TooltipTextUnset, "tooltip,text,unset", void);
DECLARE_EWK_VIEW_CALLBACK(TooltipTextSet, "tooltip,text,set", const char*);
DECLARE_EWK_VIEW_CALLBACK(EnterFullscreen, "fullscreen,enterfullscreen", void);
DECLARE_EWK_VIEW_CALLBACK(ExitFullscreen, "fullscreen,exitfullscreen", void);
DECLARE_EWK_VIEW_CALLBACK(UserMediaPermission, "usermedia,permission,request", _Ewk_User_Media_Permission_Request*);
DECLARE_EWK_VIEW_CALLBACK(IMEInputPanelShow, "editorclient,ime,opened", void);
DECLARE_EWK_VIEW_CALLBACK(IMEInputPanelHide, "editorclient,ime,closed", void);
DECLARE_EWK_VIEW_CALLBACK(IMECandidatePanelShow, "editorclient,candidate,opened", void);
DECLARE_EWK_VIEW_CALLBACK(IMECandidatePanelHide, "editorclient,candidate,closed", void);
DECLARE_EWK_VIEW_CALLBACK(CreateNewWindow, "create,window", Evas_Object**);
DECLARE_EWK_VIEW_CALLBACK(CreateNewWindowUrl, "create,window,url", const char*);
DECLARE_EWK_VIEW_CALLBACK(WindowClosed, "close,window", void);
DECLARE_EWK_VIEW_CALLBACK(IMEInputMethodChanged, "inputmethod,changed", Eina_Rectangle*);
DECLARE_EWK_VIEW_CALLBACK(ProvisionalLoadStarted, "load,provisional,started", void);
DECLARE_EWK_VIEW_CALLBACK(ProvisionalLoadRedirect, "load,provisional,redirect", void);
DECLARE_EWK_VIEW_CALLBACK(LoadCommitted, "load,committed", void);
DECLARE_EWK_VIEW_CALLBACK(GeoLocationPermissionRequest, "geolocation,permission,request", _Ewk_Geolocation_Permission_Request*);
DECLARE_EWK_VIEW_CALLBACK(GeoLocationValid, "geolocation,valid", Eina_Bool*);
DECLARE_EWK_VIEW_CALLBACK(RequestCertificateConfirm, "request,certificate,confirm", _Ewk_Certificate_Policy_Decision*);
DECLARE_EWK_VIEW_CALLBACK(SSLCertificateChanged, "ssl,certificate,changed", _Ewk_Certificate_Info*);
DECLARE_EWK_VIEW_CALLBACK(PolicyResponseDecide, "policy,response,decide", _Ewk_Policy_Decision*);
DECLARE_EWK_VIEW_CALLBACK(ContextMenuCustomize, "contextmenu,customize", _Ewk_Context_Menu*);
DECLARE_EWK_VIEW_CALLBACK(ContextMenuItemSelected, "contextmenu,selected", _Ewk_Context_Menu_Item*);
DECLARE_EWK_VIEW_CALLBACK(NavigationPolicyDecision, "policy,navigation,decide", _Ewk_Policy_Decision*);
DECLARE_EWK_VIEW_CALLBACK(TextFound, "text,found", unsigned int*);
DECLARE_EWK_VIEW_CALLBACK(TextSelectionMode, "textselection,mode", bool*);
DECLARE_EWK_VIEW_CALLBACK(NewWindowPolicyDecision, "policy,newwindow,decide", _Ewk_Policy_Decision*);
DECLARE_EWK_VIEW_CALLBACK(LoadNonEmptyLayoutFinished, "load,nonemptylayout,finished", void);
DECLARE_EWK_VIEW_CALLBACK(PopupBlocked, "popup,blocked", Eina_Stringshare*);
DECLARE_EWK_VIEW_CALLBACK(VideoPlayingURL, "video,playing,url", const char*);
DECLARE_EWK_VIEW_CALLBACK(PopupReplyWaitStart, "popup,reply,wait,start", void*);
DECLARE_EWK_VIEW_CALLBACK(PopupReplyWaitFinish, "popup,reply,wait,finish", void*);
DECLARE_EWK_VIEW_CALLBACK(FrameRendered, "frame,rendered", void*);
#if defined(TIZEN_TBM_SUPPORT)
DECLARE_EWK_VIEW_CALLBACK(OffscreenFrameRendered, "offscreen,frame,rendered", void*);
#endif
DECLARE_EWK_VIEW_CALLBACK(RotatePrepared, "rotate,prepared", void*);
DECLARE_EWK_VIEW_CALLBACK(TopControlMoved, "topcontrol,moved", int*);
DECLARE_EWK_VIEW_CALLBACK(CreateNewBackgroundWindow, "create,window,background", Evas_Object**);
DECLARE_EWK_VIEW_CALLBACK(EdgeLeft, "edge,left", void);
DECLARE_EWK_VIEW_CALLBACK(EdgeTop, "edge,top", void);
DECLARE_EWK_VIEW_CALLBACK(EdgeBottom, "edge,bottom", void);
DECLARE_EWK_VIEW_CALLBACK(EdgeRight, "edge,right", void);
#if defined(OS_TIZEN_TV_PRODUCT)
DECLARE_EWK_VIEW_CALLBACK(EdgeScrollLeft, "edge,scroll,left", bool*);
DECLARE_EWK_VIEW_CALLBACK(EdgeScrollTop, "edge,scroll,top", bool*);
DECLARE_EWK_VIEW_CALLBACK(EdgeScrollBottom, "edge,scroll,bottom", bool*);
DECLARE_EWK_VIEW_CALLBACK(EdgeScrollRight, "edge,scroll,right", bool*);
DECLARE_EWK_VIEW_CALLBACK(DidChagneScrollbarsThumbFocus,
                          "scrollbar,thumb,focus,changed",
                          Ewk_Scrollbar_Data*);
DECLARE_EWK_VIEW_CALLBACK(RunArrowScroll, "run,arrow,scroll", void);
DECLARE_EWK_VIEW_CALLBACK(HoverOverLink, "hover,over,link", const char*);
DECLARE_EWK_VIEW_CALLBACK(HoverOutLink, "hover,out,link", const char*);
DECLARE_EWK_VIEW_CALLBACK(VideoResized, "notify,video,resized", void*);
DECLARE_EWK_VIEW_CALLBACK(ContextMenuShow,
                          "contextmenu,show",
                          _Ewk_Context_Menu*);
DECLARE_EWK_VIEW_CALLBACK(ContextMenuHide,
                          "contextmenu,hide",
                          _Ewk_Context_Menu*);
DECLARE_EWK_VIEW_CALLBACK(PopupMenuShow, "popup,menu,show", void);
DECLARE_EWK_VIEW_CALLBACK(PopupMenuHide, "popup,menu,hide", void);
DECLARE_EWK_VIEW_CALLBACK(DidBlockInsecureContent,
                          "did,block,insecure,content",
                          void);
DECLARE_EWK_VIEW_CALLBACK(PlaybackLoad, "notification,playback,load", void*);
DECLARE_EWK_VIEW_CALLBACK(PlaybackReady, "notification,playback,ready", void*);
DECLARE_EWK_VIEW_CALLBACK(PlaybackStart, "notification,playback,start", void);
DECLARE_EWK_VIEW_CALLBACK(PlaybackStop, "notification,playback,stop", void*);
DECLARE_EWK_VIEW_CALLBACK(PlaybackFinish, "notification,playback,finish", void);
DECLARE_EWK_VIEW_CALLBACK(SubtitlePlay, "notify,subtitle,play", void*);
DECLARE_EWK_VIEW_CALLBACK(SubtitlePause, "notify,subtitle,pause", void);
DECLARE_EWK_VIEW_CALLBACK(SubtitleStop, "notify,subtitle,stop", void);
DECLARE_EWK_VIEW_CALLBACK(SubtitleResume, "notify,subtitle,resume", void);
DECLARE_EWK_VIEW_CALLBACK(SubtitleSeekStart,
                          "notify,subtitle,seek,start",
                          double*);
DECLARE_EWK_VIEW_CALLBACK(SubtitleSeekComplete,
                          "notify,subtitle,seek,completed",
                          void);
DECLARE_EWK_VIEW_CALLBACK(SubtitleNotifyData, "on,subtitle,data", void*);
DECLARE_EWK_VIEW_CALLBACK(ParentalRatingInfo, "on,parentalrating,info", void*);
DECLARE_EWK_VIEW_CALLBACK(LoginFormSubmitted,
                          "login,form,submitted",
                          _Ewk_Form_Info*);
DECLARE_EWK_VIEW_CALLBACK(LoginFields,
                          "login,field,identified",
                          _Ewk_Form_Info*);
#endif
#if defined(TIZEN_ATK_FEATURE_VD)
DECLARE_EWK_VIEW_CALLBACK(AtkKeyEventNotHandled,
                          "atk,keyevent,nothandled",
                          int*);
#endif
DECLARE_EWK_VIEW_CALLBACK(SaveSessionData, "save,session,data", void);
DECLARE_EWK_VIEW_CALLBACK(UndoSize, "undo,size", size_t*);
DECLARE_EWK_VIEW_CALLBACK(RedoSize, "redo,size", size_t*);
DECLARE_EWK_VIEW_CALLBACK(MagnifierShow, "magnifier,show", void);
DECLARE_EWK_VIEW_CALLBACK(MagnifierHide, "magnifier,hide", void);
DECLARE_EWK_VIEW_CALLBACK(ClipboardOpened, "clipboard,opened", void*);
DECLARE_EWK_VIEW_CALLBACK(BackForwardListChange, "back,forward,list,changed", void);
DECLARE_EWK_VIEW_CALLBACK(WebProcessCrashed, "webprocess,crashed", bool*);
DECLARE_EWK_VIEW_CALLBACK(ConsoleMessage, "console,message", _Ewk_Console_Message*);
DECLARE_EWK_VIEW_CALLBACK(WrtPluginsMessage, "wrt,message", Ewk_Wrt_Message_Data*);
DECLARE_EWK_VIEW_CALLBACK(IconReceived, "icon,received", void);
DECLARE_EWK_VIEW_CALLBACK(OverflowScrollOff, "overflow,scroll,off", void);
DECLARE_EWK_VIEW_CALLBACK(OverflowScrollOn, "overflow,scroll,on", void);
DECLARE_EWK_VIEW_CALLBACK(TouchmoveHandled, "touchmove,handled", bool);
DECLARE_EWK_VIEW_CALLBACK(WebloginCheckboxClicked, "weblogin,checkbox,clicked", void);
DECLARE_EWK_VIEW_CALLBACK(WebloginCheckboxResume, "weblogin,checkbox,resume", void);
DECLARE_EWK_VIEW_CALLBACK(WebloginReady, "weblogin,ready" , void);
DECLARE_EWK_VIEW_CALLBACK(ZoomStarted, "zoom,started", void);
DECLARE_EWK_VIEW_CALLBACK(ZoomFinished, "zoom,finished", void);
#if defined(OS_TIZEN)
DECLARE_EWK_VIEW_CALLBACK(NewWindowNavigationPolicyDecision, "policy,decision,new,window", Ewk_Navigation_Policy_Decision*);
#endif // OS_TIZEN
DECLARE_EWK_VIEW_CALLBACK(ContentsSizeChanged, "contents,size,changed", void);
DECLARE_EWK_VIEW_CALLBACK(MenuBarVisible, "menubar,visible", bool*);
DECLARE_EWK_VIEW_CALLBACK(StatusBarVisible, "statusbar,visible", bool*);
DECLARE_EWK_VIEW_CALLBACK(ToolbarVisible, "toolbar,visible", bool*);
DECLARE_EWK_VIEW_CALLBACK(WindowResizable, "window,resizable", bool*);
DECLARE_EWK_VIEW_CALLBACK(DidNotAllowScript, "did,not,allow,script", void);
DECLARE_EWK_VIEW_CALLBACK(FocusOut, "webview,focus,out", void);
DECLARE_EWK_VIEW_CALLBACK(FocusIn, "webview,focus,in", void);
DECLARE_EWK_VIEW_CALLBACK(NotificationPermissionReply,
                          "notification,permission,reply",
                          Ewk_Notification_Permission*);
DECLARE_EWK_VIEW_CALLBACK(ReaderMode, "reader,mode", bool*);
DECLARE_EWK_VIEW_CALLBACK(FormRepostWarningShow,
                          "form,repost,warning,show",
                          _Ewk_Form_Repost_Decision_Request*);
DECLARE_EWK_VIEW_CALLBACK(BeforeFormRepostWarningShow,
                          "before,repost,warning,show",
                          _Ewk_Form_Repost_Decision_Request*);
/* LCOV_EXCL_STOP */
}

#endif
