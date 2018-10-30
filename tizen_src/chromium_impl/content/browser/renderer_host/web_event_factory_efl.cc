// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/web_event_factory_efl.h"

#include <Ecore.h>
#include <unordered_map>

#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "tizen/system_info.h"
#include "ui/display/screen.h"
#include "ui/events/event_utils.h"
#include "ui/events/keycodes/keyboard_code_conversion_x.h"
#include "ui/events/keycodes/keyboard_codes.h"


#if defined(OS_TIZEN_TV_PRODUCT)
#include "tizen_src/ewk/efl_integration/common/application_type.h"
#include "ui/events/keycodes/dom/keycode_converter.h"
#include "ui/events/keycodes/keyboard_code_conversion.h"
#include "ui/events/keycodes/dom/dom_code.h"
#include "ui/events/keycodes/dom/dom_key.h"
#endif

using namespace blink;

namespace content {

namespace {

static const float kDefaultScrollStep = 20;

static void TranslateEvasCoordToWebKitCoord(Evas_Object* web_view,
                                            blink::WebPoint* point) {
  Evas_Coord tmpX, tmpY;
  evas_object_geometry_get(web_view, &tmpX, &tmpY, 0, 0);
  point->x -= tmpX;
  point->y -= tmpY;
}

float GetDeviceScaleFactor() {
  static float device_scale_factor = 0.0f;
  if (!device_scale_factor) {
    device_scale_factor =
        display::Screen::GetScreen()->GetPrimaryDisplay().device_scale_factor();
  }
  return device_scale_factor;
}

inline bool string_ends_with(std::string const& value,
                             std::string const& match) {
  if (match.size() > value.size()) {
    return false;
  }
  return std::equal(match.rbegin(), match.rend(), value.rbegin());
}

inline bool string_starts_with(std::string const& value,
                               std::string const& match) {
  if (match.size() > value.size()) {
    return false;
  }
  return std::equal(match.begin(), match.end(), value.begin());
}

// TV RC device names
const std::string kDeviceNameSmartView = "SMART_VIEW";
const std::string kDeviceNamePanelKey = "wt61p807 panel key device";
const std::string kDeviceNameRemote = "wt61p807 rc device";
const std::string kDeviceNameSmartRC = "Smart Control";
// To allow to receive Tomato send key events
const std::string kDeviceNameAutoModeDevice = "AUTO_MODE_DEVICE";
const std::string kDeviceNameIME = "ime";

#if defined(OS_TIZEN_TV_PRODUCT)
bool IsRCDevice(Evas_Device_Class device_id, const std::string device_name) {
  if (device_id == EVAS_DEVICE_CLASS_KEYBOARD) {
    if (!device_name.compare(kDeviceNameRemote) ||
        !device_name.compare(kDeviceNameSmartView) ||
        !device_name.compare(kDeviceNamePanelKey) ||
        !device_name.compare(kDeviceNameAutoModeDevice) ||
        device_name.find(kDeviceNameSmartRC) != std::string::npos)
      return true;
  }
  return false;
}
#endif

static ui::KeyboardCode windows_key_codeFromEflKey(const char* key) {
  static std::unordered_map<std::string, ui::KeyboardCode> code_from_key_map({
      {"Shift_L", ui::VKEY_SHIFT},
      {"Shift_R", ui::VKEY_SHIFT},
      {"Control_L", ui::VKEY_CONTROL},
      {"Control_R", ui::VKEY_CONTROL},
      {"Alt_L", ui::VKEY_MENU},
      {"Alt_R", ui::VKEY_MENU},
      {"Meta_L", ui::VKEY_MENU},
      {"Meta_R", ui::VKEY_MENU},

      // comes from KeyboardCodeFromXKeysym
      {"BackSpace", ui::VKEY_BACK},
      {"Delete", ui::VKEY_DELETE},
      {"Tab", ui::VKEY_TAB},
      {"Return", ui::VKEY_RETURN},
      {"KP_Enter", ui::VKEY_RETURN},
      {"Clear", ui::VKEY_CLEAR},
      {"space", ui::VKEY_SPACE},
      {"Home", ui::VKEY_HOME},
      {"KP_Home", ui::VKEY_HOME},
      {"End", ui::VKEY_END},
      {"KP_End", ui::VKEY_END},
      {"Prior", ui::VKEY_PRIOR},
      {"KP_Prior", ui::VKEY_PRIOR},
      {"Next", ui::VKEY_NEXT},
      {"KP_Next", ui::VKEY_NEXT},
      {"Left", ui::VKEY_LEFT},
      {"KP_Left", ui::VKEY_LEFT},
      {"Right", ui::VKEY_RIGHT},
      {"KP_Right", ui::VKEY_RIGHT},
      {"Down", ui::VKEY_DOWN},
      {"KP_Down", ui::VKEY_DOWN},
      {"Up", ui::VKEY_UP},
      {"KP_Up", ui::VKEY_UP},
      {"Escape", ui::VKEY_ESCAPE},
      {"Kana_Lock", ui::VKEY_KANA},
      {"Kana_Shift", ui::VKEY_KANA},
      {"Hangul", ui::VKEY_HANGUL},
      {"Hangul_Hanja", ui::VKEY_HANJA},
      {"Kanji", ui::VKEY_KANJI},
      {"Henkan", ui::VKEY_CONVERT},
      {"Muhenkan", ui::VKEY_NONCONVERT},
      {"Zenkaku_Hankaku", ui::VKEY_DBE_DBCSCHAR},
      {"KP_0", ui::VKEY_NUMPAD0},
      {"KP_1", ui::VKEY_NUMPAD1},
      {"KP_2", ui::VKEY_NUMPAD2},
      {"KP_3", ui::VKEY_NUMPAD3},
      {"KP_4", ui::VKEY_NUMPAD4},
      {"KP_5", ui::VKEY_NUMPAD5},
      {"KP_6", ui::VKEY_NUMPAD6},
      {"KP_7", ui::VKEY_NUMPAD7},
      {"KP_8", ui::VKEY_NUMPAD8},
      {"KP_9", ui::VKEY_NUMPAD9},
      {"KP_Multiply", ui::VKEY_MULTIPLY},
      {"KP_Add", ui::VKEY_ADD},
      {"KP_Separator", ui::VKEY_SEPARATOR},
      {"KP_Subtract", ui::VKEY_SUBTRACT},
      {"KP_Decimal", ui::VKEY_DECIMAL},
      {"KP_Divide", ui::VKEY_DIVIDE},

      {"ISO_Level5_Shift", ui::VKEY_OEM_8},
      {"ISO_Level3_Shift", ui::VKEY_ALTGR},
      {"Mode_switch", ui::VKEY_ALTGR},
      {"Multi_key", ui::VKEY_COMPOSE},
      {"Pause", ui::VKEY_PAUSE},
      {"Caps_Lock", ui::VKEY_CAPITAL},
      {"Num_Lock", ui::VKEY_NUMLOCK},
      {"Scroll_Lock", ui::VKEY_SCROLL},
      {"Print", ui::VKEY_PRINT},
      {"Execute", ui::VKEY_EXECUTE},
      {"Insert", ui::VKEY_INSERT},
      {"KP_Insert", ui::VKEY_INSERT},
      {"Help", ui::VKEY_HELP},
      {"Super_L", ui::VKEY_LWIN},
      {"Super_R", ui::VKEY_RWIN},
      {"Menu", ui::VKEY_APPS},
      {"F1", ui::VKEY_F1},
      {"KP_F1", ui::VKEY_F1},
      {"F2", ui::VKEY_F2},
      {"KP_F2", ui::VKEY_F2},
      {"F3", ui::VKEY_F3},
      {"KP_F3", ui::VKEY_F3},
      {"F4", ui::VKEY_F4},
      {"KP_F4", ui::VKEY_F4},
      {"F5", ui::VKEY_F5},
      {"F6", ui::VKEY_F6},
      {"F7", ui::VKEY_F7},
      {"F8", ui::VKEY_F8},
      {"F9", ui::VKEY_F9},
      {"F10", ui::VKEY_F10},
      {"F11", ui::VKEY_F11},
      {"F12", ui::VKEY_F12},
      {"F13", ui::VKEY_F13},
      {"F14", ui::VKEY_F14},
      {"F15", ui::VKEY_F15},
      {"F16", ui::VKEY_F16},
      {"F17", ui::VKEY_F17},
      {"F18", ui::VKEY_F18},
      {"F19", ui::VKEY_F19},
      {"F20", ui::VKEY_F20},
      {"F21", ui::VKEY_F21},
      {"F22", ui::VKEY_F22},
      {"F23", ui::VKEY_F23},
      {"F24", ui::VKEY_F24},
      {"guillemotleft", ui::VKEY_OEM_102},
      {"guillemotright", ui::VKEY_OEM_102},
      {"degree", ui::VKEY_OEM_102},
      {"ugrave", ui::VKEY_OEM_102},
      {"Ugrave", ui::VKEY_OEM_102},
      {"brokenbar", ui::VKEY_OEM_102},
      {"XF86Forward", ui::VKEY_BROWSER_FORWARD},
      {"XF86Reload", ui::VKEY_BROWSER_REFRESH},
      {"XF86Stop", ui::VKEY_BROWSER_STOP},
      {"XF86Favorites", ui::VKEY_BROWSER_FAVORITES},
      {"XF86HomePage", ui::VKEY_BROWSER_HOME},
      {"XF86AudioMute", ui::VKEY_VOLUME_MUTE},
      {"XF86AudioLowerVolume", ui::VKEY_VOLUME_DOWN},
      {"XF86AudioRaiseVolume", ui::VKEY_VOLUME_UP},

#if !defined(OS_TIZEN_TV_PRODUCT)
      {"XF86Search", ui::VKEY_BROWSER_SEARCH},
      {"XF86AudioNext", ui::VKEY_MEDIA_NEXT_TRACK},
      {"XF86AudioPrev", ui::VKEY_MEDIA_PREV_TRACK},
      {"XF86AudioStop", ui::VKEY_MEDIA_STOP},
      {"XF86AudioPlay", ui::VKEY_MEDIA_PLAY_PAUSE},
      {"Select", ui::VKEY_SELECT},
      {"XF86Back", ui::VKEY_BROWSER_BACK},
#endif
      {"XF86Mail", ui::VKEY_MEDIA_LAUNCH_MAIL},
      {"XF86LaunchA", ui::VKEY_MEDIA_LAUNCH_APP1},
      {"XF86LaunchB", ui::VKEY_MEDIA_LAUNCH_APP2},
      {"XF86Calculator", ui::VKEY_MEDIA_LAUNCH_APP2},
      {"XF86WLAN", ui::VKEY_WLAN},
      {"XF86PowerOff", ui::VKEY_POWER},
      {"XF86Sleep", ui::VKEY_SLEEP},
      {"XF86MonBrightnessDown", ui::VKEY_BRIGHTNESS_DOWN},
      {"XF86MonBrightnessUp", ui::VKEY_BRIGHTNESS_UP},
      {"XF86KbdBrightnessDown", ui::VKEY_KBD_BRIGHTNESS_DOWN},
      {"XF86KbdBrightnessUp", ui::VKEY_KBD_BRIGHTNESS_UP},
      {"0", ui::VKEY_0},
      {"1", ui::VKEY_1},
      {"2", ui::VKEY_2},
      {"3", ui::VKEY_3},
      {"4", ui::VKEY_4},
      {"5", ui::VKEY_5},
      {"6", ui::VKEY_6},
      {"7", ui::VKEY_7},
      {"8", ui::VKEY_8},
      {"9", ui::VKEY_9},
#if defined(OS_TIZEN_TV_PRODUCT)
      {"XF86Back", ui::VKEY_DTV_RETURN},
      {"XF86RaiseChannel", ui::VKEY_CHANNEL_UP},
      {"XF86LowerChannel", ui::VKEY_CHANNEL_DOWN},
      {"XF86AudioRewind", ui::VKEY_REWIND},
      {"XF86AudioPause", ui::VKEY_PAUSE},
      {"XF86AudioNext", ui::VKEY_FAST_FWD},
      {"XF86AudioRecord", ui::VKEY_RECORD},
      {"XF86AudioPlay", ui::VKEY_PLAY},
      {"XF86AudioStop", ui::VKEY_STOP},
      {"XF86Red", ui::VKEY_RED},
      {"XF86Green", ui::VKEY_GREEN},
      {"XF86Yellow", ui::VKEY_YELLOW},
      {"XF86Blue", ui::VKEY_BLUE},
      {"XF86Grey", ui::VKEY_GREY},
      {"XF86Brown", ui::VKEY_BROWN},
      {"XF86Info", ui::VKEY_INFO},
      {"XF86Home", ui::VKEY_DTV_HOME},
      {"XF86Display", ui::VKEY_SOURCE},
      {"XF86ChannelList", ui::VKEY_CH_LIST},
      {"XF86MBRRepeat", ui::VKEY_REPEAT},
      {"XF86PictureSize", ui::VKEY_ASPECT},
      {"XF86PictureMode", ui::VKEY_PMODE},
      {"XF86Hdmi", ui::VKEY_HDMI},
      {"XF86UsbHub", ui::VKEY_USBHUB_SWITCH},
      {"XF86EManual", ui::VKEY_EMANUAL},
      {"XF86SimpleMenu", ui::VKEY_TOOLS},
      {"XF86More", ui::VKEY_MORE},
      {"XF86FactoryMode", ui::VKEY_FACTORY},
      {"XF86Sleep", ui::VKEY_SLEEP},
      {"XF86TV", ui::VKEY_TV},
      {"XF86DTV", ui::VKEY_DTV},
      {"XF86STBPower", ui::VKEY_STB_POWER},
      {"XF86PanelDown", ui::VKEY_PANEL_DOWN},
      {"XF86WWW", ui::VKEY_CONVERGENCE},
      {"XF86BTColorMecha", ui::VKEY_BT_COLOR_MECHA},
      {"XF86StillPicture", ui::VKEY_STILL_PICTURE},
      {"XF86BTPairing", ui::VKEY_BT_TRIGGER},
      {"XF86BTHotkey", ui::VKEY_BT_HOTVK},
      {"XF86BTDevice", ui::VKEY_BT_DEVICE},
      {"XF86BTContentsBar", ui::VKEY_BT_CONTENTSBAR},
      {"XF86Game", ui::VKEY_GAME},
      {"XF86PIPChannelUp", ui::VKEY_PIP_CHUP},
      {"XF86PIPChannelDown", ui::VKEY_PIP_CHDOWN},
      {"XF86Antena", ui::VKEY_ANTENA},
      {"XF86PanelEnter", ui::VKEY_PANEL_ENTER},
      {"XF86MBRLink", ui::VKEY_LINK},
      {"XF86PanelUp", ui::VKEY_PANEL_UP},
      {"XF86Game3D", ui::VKEY_ANGLE},
      {"XF86WheelLeftKey", ui::VKEY_WHEEL_LEFT},
      {"XF86WheelRightKey", ui::VKEY_WHEEL_RIGHT},
      {"XF86PanelExit", ui::VKEY_PANEL_EXIT},
      {"XF86Exit", ui::VKEY_EXIT},
      {"XF86MBRTV", ui::VKEY_MBR_TV},
      {"XF86MBRSTBGuide", ui::VKEY_MBR_STB_GUIDE},
      {"XF86MBRBDPopup", ui::VKEY_MBR_BD_POPUP},
      {"XF86MBRBDDVDPower", ui::VKEY_MBR_BDDVD_POWER},
      {"XF86MBRSetupFailure", ui::VKEY_MBR_SETUP_FAILURE},
      {"XF86MBRSetup", ui::VKEY_MBR_SETUP},
      {"XF86MBRWatchTV", ui::VKEY_MBR_WATCH_TV},
      {"XF86PreviousChannel", ui::VKEY_PRECH},
      {"XF86Recommend", ui::VKEY_RECOMMEND_SEARCH_TOGGLE},
      {"XF86NumberPad", ui::VKEY_BT_NUMBER},
      {"XF86AspectRatio169", ui::VKEY_16_9},
      {"XF86MTS", ui::VKEY_MTS},
      {"XF86SoundMode", ui::VKEY_SMODE},
      {"XF863XSpeed", ui::VKEY_3SPEED},
      {"XF863D", ui::VKEY_3D},
      {"XF86TTXMIX", ui::VKEY_TTX_MIX},
      {"XF86SRSSXT", ui::VKEY_SRSTSXT},
      {"XF86WIFIPairing", ui::VKEY_WIFI_PAIRING},
      {"XF86BTApps", ui::VKEY_BT_SAMSUNG_APPS},
      {"XF86EnergySaving", ui::VKEY_ESAVING},
      {"XF86MBRClear", ui::VKEY_CLEAR},
      {"XF86TVSNS", ui::VKEY_TV_SNS},
      {"XF86DVR", ui::VKEY_DVR},
      {"XF86Apps", ui::VKEY_APP_LIST},
      {"XF86Camera", ui::VKEY_CAMERA},
      {"XF86Caption", ui::VKEY_CAPTION},
      {"XF86ZoomIn", ui::VKEY_ZOOM1},
      {"XF86PanelPlus", ui::VKEY_PANEL_PLUS},
      {"XF86BTVoice", ui::VKEY_BT_VOICE},
      {"XF86Search", ui::VKEY_SEARCH},
      {"XF86PanelMinus", ui::VKEY_PANEL_MINUS},
      {"XF86SoccerMode", ui::VKEY_SOCCER_MODE},
      {"XF86Amazon", ui::VKEY_FUNCTIONS_AMAZON},
      {"XF86AudioDescription", ui::VKEY_AD},
      {"XF86PreviousChapter", ui::VKEY_REWIND_},
      {"XF86NextChapter", ui::VKEY_FF_},
      {"XF86Netflix", ui::VKEY_FUNCTIONS_NETFLIX},
      {"XF86PIP", ui::VKEY_PIP_ONOFF},
      {"XF86MBRWatchMovie", ui::VKEY_MBR_WATCH_MOVIE},
      {"XF86MBRMenu", ui::VKEY_MBR_STBBD_MENU},
      {"XF86MBRConfirm", ui::VKEY_MBR_SETUP_CONFIRM},
      {"XF86FamilyHub", ui::VKEY_FAMILYHUB},
      {"XF86HDMICEC", ui::VKEY_ANYVIEW},
      {"XF86LeftPage", ui::VKEY_PAGE_LEFT},
      {"XF86RightPage", ui::VKEY_PAGE_RIGHT},
      {"XF86PowerOff", ui::VKEY_POWER},
      {"XF86SysMenu", ui::VKEY_DTV_MENU},
      {"XF86ChannelGuide", ui::VKEY_GUIDE},
      {"XF86ChannelAddDel", ui::VKEY_ADDDEL},
      {"XF86ChannelAutoTune", ui::VKEY_AUTO_PROGRAM},
      {"XF86FavoriteChannel", ui::VKEY_FAVCH},
      {"XF86DualView", ui::VKEY_BT_DUALVIEW},
      {"XF86Subtitle", ui::VKEY_SUB_TITLE},
      {"XF86SoftWakeup", ui::VKEY_SOFT_WAKE_UP},
      {"XF86PlayBack", ui::VKEY_PLAY_BACK},
      {"XF86ExtraApp", ui::VKEY_EXTRA},
      {"XF86Color", ui::VKEY_COLOR},

      {"XF86NoiseReduction", ui::VKEY_NOISE_REDUCTION},
      {"XF86Help", ui::VKEY_HELP},
      {"XF86HotelAppsGuest", ui::VKEY_HOTEL_APPS_GUEST},
      {"XF86HotelMovies", ui::VKEY_HOTEL_MOVIES},
      {"XF86HotelLanguage", ui::VKEY_HOTEL_LANGUAGE},
      {"XF86HotelTVGuide", ui::VKEY_HOTEL_TV_GUIDE},
      {"XF86NR", ui::VKEY_NR},
      {"XF86HotelRoomControl", ui::VKEY_HOTEL_ROOM_CONTROL},
      {"XF86Alarm", ui::VKEY_ALARM},

      // TV IME Keys
      {"Select", ui::VKEY_RETURN},
      {"Clear", ui::VKEY_DELETE},
#endif

      // XXX: it won't work on all keyboard layouts
      {"comma", ui::VKEY_OEM_COMMA},
      {"less", ui::VKEY_OEM_COMMA},
      {"minus", ui::VKEY_OEM_MINUS},
      {"underscore", ui::VKEY_OEM_MINUS},
      {"greater", ui::VKEY_OEM_PERIOD},
      {"period", ui::VKEY_OEM_PERIOD},
      {"semicolon", ui::VKEY_OEM_1},
      {"colon", ui::VKEY_OEM_1},
      {"question", ui::VKEY_OEM_2},
      {"slash", ui::VKEY_OEM_2},
      {"asciitilde", ui::VKEY_OEM_3},
      {"quoteleft", ui::VKEY_OEM_3},
      {"bracketleft", ui::VKEY_OEM_4},
      {"braceleft", ui::VKEY_OEM_4},
      {"backslash", ui::VKEY_OEM_5},
      {"bar", ui::VKEY_OEM_5},
      {"bracketright", ui::VKEY_OEM_6},
      {"braceright", ui::VKEY_OEM_6},
      {"quoteright", ui::VKEY_OEM_7},
      {"quotedbl", ui::VKEY_OEM_7},

      // XXX: handle accents and other characters
      {"a", ui::VKEY_A},
      {"A", ui::VKEY_A},
      {"b", ui::VKEY_B},
      {"B", ui::VKEY_B},
      {"c", ui::VKEY_C},
      {"C", ui::VKEY_C},
      {"d", ui::VKEY_D},
      {"D", ui::VKEY_D},
      {"e", ui::VKEY_E},
      {"E", ui::VKEY_E},
      {"f", ui::VKEY_F},
      {"F", ui::VKEY_F},
      {"g", ui::VKEY_G},
      {"G", ui::VKEY_G},
      {"h", ui::VKEY_H},
      {"H", ui::VKEY_H},
      {"i", ui::VKEY_I},
      {"I", ui::VKEY_I},
      {"j", ui::VKEY_J},
      {"J", ui::VKEY_J},
      {"k", ui::VKEY_K},
      {"K", ui::VKEY_K},
      {"l", ui::VKEY_L},
      {"L", ui::VKEY_L},
      {"m", ui::VKEY_M},
      {"M", ui::VKEY_M},
      {"n", ui::VKEY_N},
      {"N", ui::VKEY_N},
      {"o", ui::VKEY_O},
      {"O", ui::VKEY_O},
      {"p", ui::VKEY_P},
      {"P", ui::VKEY_P},
      {"q", ui::VKEY_Q},
      {"Q", ui::VKEY_Q},
      {"r", ui::VKEY_R},
      {"R", ui::VKEY_R},
      {"s", ui::VKEY_S},
      {"S", ui::VKEY_S},
      {"t", ui::VKEY_T},
      {"T", ui::VKEY_T},
      {"u", ui::VKEY_U},
      {"U", ui::VKEY_U},
      {"v", ui::VKEY_V},
      {"V", ui::VKEY_V},
      {"w", ui::VKEY_W},
      {"W", ui::VKEY_W},
      {"x", ui::VKEY_X},
      {"X", ui::VKEY_X},
      {"y", ui::VKEY_Y},
      {"Y", ui::VKEY_Y},
      {"z", ui::VKEY_Z},
      {"Z", ui::VKEY_Z},
  });

#if defined(OS_TIZEN_TV_PRODUCT)
  if (IsTvProfile() && IsTIZENWRT()) {
    // WRT use 2 different key codes
    if (strcmp(key, "Select") == 0)
      return ui::VKEY_IME_DONE;
    if (strcmp(key, "Cancel") == 0)
      return ui::VKEY_IME_CANCEL;
  }
#endif

  auto uicode = code_from_key_map.find(key);
  if (uicode == code_from_key_map.end()) {
    return ui::VKEY_UNKNOWN;
  }

  return uicode->second;
}

static int ModifiersFromEflKey(const char* key) {
  if (string_ends_with(key, "_L")) {
    return WebInputEvent::kIsLeft;
  } else if (string_ends_with(key, "_R")) {
    return WebInputEvent::kIsRight;
  } else if (string_starts_with(key, "KP_")) {
    return WebInputEvent::kIsKeyPad;
  }
  return 0;
}

static int CharacterFromEflString(const char* string) {
  if (string) {
    base::string16 result;
    base::UTF8ToUTF16(string, strlen(string), &result);
    return result.length() == 1 ? result[0] : 0;
  }

  return 0;
}

// From
// third_party/blink/Source/blink/chromium/src/gtk/WebInputEventFactory.cpp:
static blink::WebUChar GetControlCharacter(int windows_key_code, bool shift) {
  if (windows_key_code >= ui::VKEY_A && windows_key_code <= ui::VKEY_Z) {
    // ctrl-A ~ ctrl-Z map to \x01 ~ \x1A
    return windows_key_code - ui::VKEY_A + 1;
  }
  if (shift) {
    // following graphics chars require shift key to input.
    switch (windows_key_code) {
      // ctrl-@ maps to \x00 (Null byte)
      case ui::VKEY_2:
        return 0;
      // ctrl-^ maps to \x1E (Record separator, Information separator two)
      case ui::VKEY_6:
        return 0x1E;
      // ctrl-_ maps to \x1F (Unit separator, Information separator one)
      case ui::VKEY_OEM_MINUS:
        return 0x1F;
      // Returns 0 for all other keys to avoid inputting unexpected chars.
      default:
        break;
    }
  } else {
    switch (windows_key_code) {
      // ctrl-[ maps to \x1B (Escape)
      case ui::VKEY_OEM_4:
        return 0x1B;
      // ctrl-\ maps to \x1C (File separator, Information separator four)
      case ui::VKEY_OEM_5:
        return 0x1C;
      // ctrl-] maps to \x1D (Group separator, Information separator three)
      case ui::VKEY_OEM_6:
        return 0x1D;
      // ctrl-Enter maps to \x0A (Line feed)
      case ui::VKEY_RETURN:
        return 0x0A;
      // Returns 0 for all other keys to avoid inputting unexpected chars.
      default:
        break;
    }
  }
  return 0;
}

enum { LeftButton = 1, MiddleButton = 2, RightButton = 3 };

static WebMouseEvent::Button EvasToWebMouseButton(int button) {
  if (button == LeftButton)
    return WebMouseEvent::Button::kLeft;
  if (button == MiddleButton)
    return WebMouseEvent::Button::kMiddle;
  if (button == RightButton)
    return WebMouseEvent::Button::kRight;

  return WebMouseEvent::Button::kNoButton;
}

enum { LeftButtonPressed = 1, MiddleButtonPressed = 2, RightButtonPressed = 4 };

static WebInputEvent::Modifiers EvasToWebMouseButtonsPressed(int buttons) {
  unsigned result = 0;

  if (buttons & LeftButtonPressed)
    result |= WebInputEvent::kLeftButtonDown;
  if (buttons & MiddleButtonPressed)
    result |= WebInputEvent::kMiddleButtonDown;
  if (buttons & RightButtonPressed)
    result |= WebInputEvent::kRightButtonDown;

  return static_cast<WebInputEvent::Modifiers>(result);
}

static WebInputEvent::Modifiers EvasToWebModifiers(
    const Evas_Modifier* modifiers) {
  unsigned result = 0;

  if (evas_key_modifier_is_set(modifiers, "Shift"))
    result |= WebInputEvent::kShiftKey;
  if (evas_key_modifier_is_set(modifiers, "Control"))
    result |= WebInputEvent::kControlKey;
  if (evas_key_modifier_is_set(modifiers, "Alt"))
    result |= WebInputEvent::kAltKey;
  if (evas_key_modifier_is_set(modifiers, "Meta"))
    result |= WebInputEvent::kMetaKey;

  return static_cast<WebInputEvent::Modifiers>(result);
}

}  // namespace

template <class EVT>
blink::WebMouseEvent MakeWebMouseEvent(WebInputEvent::Type type,
                                       Evas_Object* view,
                                       const EVT* ev) {
  const float sf = GetDeviceScaleFactor();
  blink::WebPoint point(ev->canvas.x / sf, ev->canvas.y / sf);
  TranslateEvasCoordToWebKitCoord(view, &point);
  Evas* evas = evas_object_evas_get(view);

  int clickCount = 1;
#if defined(OS_TIZEN_TV_PRODUCT)
  if (ev->flags & EVAS_BUTTON_DOUBLE_CLICK) // dblclick
    clickCount = 2;
#endif

  WebMouseEvent event(
      type, WebFloatPoint(point.x, point.y),
      WebFloatPoint(evas_coord_world_x_to_screen(evas, ev->canvas.x) / sf,
                    evas_coord_world_y_to_screen(evas, ev->canvas.y) / sf),
      EvasToWebMouseButton(ev->button), clickCount, EvasToWebModifiers(ev->modifiers),
      (double)ev->timestamp / 1000);

  return event;
}

template blink::WebMouseEvent MakeWebMouseEvent<Evas_Event_Mouse_Down>(
    WebInputEvent::Type,
    Evas_Object*,
    const Evas_Event_Mouse_Down*);
template blink::WebMouseEvent MakeWebMouseEvent<Evas_Event_Mouse_Up>(
    WebInputEvent::Type,
    Evas_Object*,
    const Evas_Event_Mouse_Up*);

blink::WebMouseEvent MakeWebMouseEvent(Evas_Object* view,
                                       const Evas_Event_Mouse_Out* ev) {
  WebMouseEvent event(WebInputEvent::kMouseLeave,
                      EvasToWebModifiers(ev->modifiers),
                      (double)ev->timestamp / 1000);
  event.button = EvasToWebMouseButton(ev->buttons);
  event.SetModifiers(event.GetModifiers() |
                     EvasToWebMouseButtonsPressed(ev->buttons));

  const float sf = GetDeviceScaleFactor();
  blink::WebPoint point(ev->canvas.x / sf, ev->canvas.y / sf);
  TranslateEvasCoordToWebKitCoord(view, &point);
  event.SetPositionInWidget(point.x, point.y);

  Evas* evas = evas_object_evas_get(view);
  event.SetPositionInScreen(
      evas_coord_world_x_to_screen(evas, ev->canvas.x) / sf,
      evas_coord_world_y_to_screen(evas, ev->canvas.y) / sf);

  return event;
}

blink::WebMouseEvent MakeWebMouseEvent(Evas_Object* view,
                                       const Evas_Event_Mouse_Move* ev) {
  WebMouseEvent event(WebInputEvent::kMouseMove,
                      EvasToWebModifiers(ev->modifiers),
                      (double)ev->timestamp / 1000);

  event.button = EvasToWebMouseButton(ev->buttons);
  event.SetModifiers(event.GetModifiers() |
                     EvasToWebMouseButtonsPressed(ev->buttons));

  const float sf = GetDeviceScaleFactor();
  blink::WebPoint point(ev->cur.canvas.x / sf, ev->cur.canvas.y / sf);
  TranslateEvasCoordToWebKitCoord(view, &point);
  event.SetPositionInWidget(point.x, point.y);

  Evas* evas = evas_object_evas_get(view);
  event.SetPositionInScreen(
      evas_coord_world_x_to_screen(evas, ev->cur.canvas.x) / sf,
      evas_coord_world_y_to_screen(evas, ev->cur.canvas.y) / sf);

  return event;
}

blink::WebMouseWheelEvent MakeWebMouseEvent(Evas_Object* view,
                                            const Evas_Event_Mouse_Wheel* ev) {
  WebMouseWheelEvent event(WebInputEvent::kMouseWheel,
                           EvasToWebModifiers(ev->modifiers),
                           (double)ev->timestamp / 1000);

  const float sf = GetDeviceScaleFactor();
  blink::WebPoint point((ev->canvas.x) / sf, (ev->canvas.y) / sf);
  TranslateEvasCoordToWebKitCoord(view, &point);
  event.SetPositionInWidget(point.x, point.y);

  Evas* evas = evas_object_evas_get(view);
  event.SetPositionInScreen(
      evas_coord_world_x_to_screen(evas, ev->canvas.x) / sf,
      evas_coord_world_y_to_screen(evas, ev->canvas.y) / sf);

  if (ev->direction) {
    event.wheel_ticks_x = ev->z;
    event.delta_x = ev->z * kDefaultScrollStep;
  } else {
    event.wheel_ticks_y = -(ev->z);
    event.delta_y = -(ev->z * kDefaultScrollStep);
  }

  return event;
}

template <class EVT>
NativeWebKeyboardEvent MakeWebKeyboardEvent(bool pressed, const EVT* evt) {
  NativeWebKeyboardEvent event(
      pressed ? WebInputEvent::kKeyDown : WebInputEvent::kKeyUp,
      ModifiersFromEflKey(evt->key), (double)evt->timestamp / 1000);

  event.native_key_code = evt->keycode;
  event.windows_key_code = windows_key_codeFromEflKey(evt->key);

  if (evas_key_lock_is_set(evt->locks, "Caps_Lock")) {
    LOG(INFO) << "CapsLock On!!!";
    event.SetModifiers(event.GetModifiers() | WebInputEvent::kCapsLockOn);
  }

  event.SetModifiers(event.GetModifiers() | EvasToWebModifiers(evt->modifiers));

  if (event.windows_key_code == ui::VKEY_RETURN) {
    event.unmodified_text[0] = '\r';
#if defined(OS_TIZEN_TV_PRODUCT)
    const char* device_name = evas_device_name_get(evt->dev);
    if (device_name &&
        IsRCDevice(evas_device_class_get(evt->dev), device_name)) {
      // Member unmodifiedText is an array with size 4.
      // Utilize the space after teminator NULL to store 'O' and 'K'
      // to indicate this is an OK key from RC.
      event.unmodified_text[1] = WebUChar(0);
      event.unmodified_text[2] = 'O';
      event.unmodified_text[3] = 'K';
    } else {
      event.unmodified_text[1] = WebUChar(0);
    }
  } else if (IsTvProfile() && event.windows_key_code == ui::VKEY_IME_DONE) {
    event.unmodified_text[0] = '\r';
  } else if (event.windows_key_code == 0) {
    event.unmodified_text[0] = WebUChar(0);
#endif
  } else {
    event.unmodified_text[0] = CharacterFromEflString(evt->string);
  }

  if (event.GetModifiers() & blink::WebInputEvent::kControlKey) {
    event.text[0] = GetControlCharacter(
        event.windows_key_code,
        event.GetModifiers() & blink::WebInputEvent::kShiftKey);
  } else {
    event.text[0] = event.unmodified_text[0];
  }

#if defined(OS_TIZEN_TV_PRODUCT)
  ui::DomCode domCode = ui::KeycodeConverter::NativeKeycodeToDomCode(event.native_key_code);
  event.dom_code = static_cast<int>(domCode);

  ui::DomKey domKey = ui::KeycodeConverter::KeyStringToDomKey(evt->key);
  // If the efl key name is not identified, use its domcode to find it in domkey map
  if ((domKey == ui::DomKey::NONE) && (domCode != ui::DomCode::NONE)) {
    int flags = ui::WebEventModifiersToEventFlags(event.GetModifiers());
    ui::KeyboardCode windowsKeyCode = ui::VKEY_UNKNOWN;
    ignore_result(DomCodeToUsLayoutDomKey(domCode, flags, &domKey, &windowsKeyCode));
  }
  event.dom_key = static_cast<int>(domKey);
#endif

  return event;
}

template NativeWebKeyboardEvent MakeWebKeyboardEvent(
    bool,
    const Evas_Event_Key_Down*);
template NativeWebKeyboardEvent MakeWebKeyboardEvent(bool,
                                                     const Evas_Event_Key_Up*);

static ui::EventType EvasTouchEventTypeToUI(Evas_Touch_Point_State evas_touch) {
  switch (evas_touch) {
    case EVAS_TOUCH_POINT_DOWN:
      return ui::ET_TOUCH_PRESSED;
    case EVAS_TOUCH_POINT_MOVE:
      return ui::ET_TOUCH_MOVED;
    case EVAS_TOUCH_POINT_UP:
      return ui::ET_TOUCH_RELEASED;
    case EVAS_TOUCH_POINT_CANCEL:
      return ui::ET_TOUCH_CANCELLED;
    case EVAS_TOUCH_POINT_STILL:
    // Not handled by chromium, should not be passed here.
    default:
      NOTREACHED();
      return ui::ET_UNKNOWN;
  }
}

bool IsHardwareBackKey(const Evas_Event_Key_Down* event) {
  if (IsTvProfile()) {
    return (strcmp(event->key, "XF86Back") == 0 ||
            strcmp(event->key, "XF86Stop") == 0);
  }

  return (strcmp(event->key, "Escape") == 0);
}

ui::TouchEvent MakeTouchEvent(Evas_Coord_Point pt,
                              Evas_Touch_Point_State state,
                              int id,
                              Evas_Object* view,
                              unsigned int timestamp) {
  blink::WebPoint point(pt.x, pt.y);
  TranslateEvasCoordToWebKitCoord(view, &point);
  pt.x = point.x;
  pt.y = point.y;

  const float scale = GetDeviceScaleFactor();
  base::TimeTicks event_timestamp = base::TimeTicks::FromInternalValue(
      timestamp * base::Time::kMicrosecondsPerMillisecond);

  if (timestamp == 0)
    event_timestamp = ui::EventTimeForNow();

  return ui::TouchEvent(
      EvasTouchEventTypeToUI(state), gfx::Point(pt.x / scale, pt.y / scale),
      event_timestamp,
      ui::PointerDetails(ui::EventPointerType::POINTER_TYPE_TOUCH, id));
}

template WebGestureEvent MakeGestureEvent<Evas_Event_Mouse_Down>(
    WebInputEvent::Type type,
    Evas_Object*,
    const Evas_Event_Mouse_Down* ev);
template WebGestureEvent MakeGestureEvent<Evas_Event_Mouse_Up>(
    WebInputEvent::Type type,
    Evas_Object*,
    const Evas_Event_Mouse_Up* ev);

template <class EVT>
WebGestureEvent MakeGestureEvent(WebInputEvent::Type type,
                                 Evas_Object* view,
                                 const EVT* ev) {
  WebGestureEvent event(type, EvasToWebModifiers(ev->modifiers),
                        (static_cast<double>(ev->timestamp) / 1000));

  event.source_device = kWebGestureDeviceTouchscreen;

  const float sf = GetDeviceScaleFactor();
  blink::WebPoint point(ev->canvas.x / sf, ev->canvas.y / sf);
  TranslateEvasCoordToWebKitCoord(view, &point);

  event.x = point.x;
  event.y = point.y;

  return event;
}

}  // namespace content
