// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/shell/browser/shell.h"

#include <Evas.h>
#include <Ecore_Evas.h>
#include <Elementary.h>

#include "base/command_line.h"
#include "base/strings/utf_string_conversions.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/content_switches.h"
#include "content/public/browser/render_view_host.h"
#include "content/shell/common/shell_switches.h"
#include "content/shell/browser/layout_test/layout_test_javascript_dialog_manager.h"
#include "components/js_dialogs_efl/javascript_dialog_manager_efl.h"
#include "efl/window_factory.h"
#include "ui/gl/gl_switches.h"
#include "cc/base/switches.h"

namespace content {

class Shell::Impl {
 public:
  Impl(Shell* shell, Evas_Object* window)
      : url_bar_(NULL),
        refresh_btn_(NULL),
        stop_btn_(NULL),
        back_btn_(NULL),
        forward_btn_(NULL),
        shell_(shell) {
    Evas_Object* box = elm_box_add(window);
    evas_object_size_hint_align_set(box, EVAS_HINT_FILL, EVAS_HINT_FILL);
    evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    elm_win_resize_object_add(window, box);
    evas_object_show(box);

    if (!switches::IsRunLayoutTestSwitchPresent())
      CreateURLBar(box);

    Evas_Object* view =
        static_cast<Evas_Object*>(shell->web_contents()->GetNativeView());
    evas_object_size_hint_align_set(view, EVAS_HINT_FILL, EVAS_HINT_FILL);
    evas_object_size_hint_weight_set(view, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    elm_box_pack_end(box, view);
  }

  void CreateURLBar(Evas_Object* box) {
    Evas_Object* wrapper = elm_box_add(box);
    elm_box_horizontal_set(wrapper, EINA_TRUE);
    evas_object_size_hint_align_set(wrapper, EVAS_HINT_FILL, EVAS_HINT_FILL);
    evas_object_size_hint_weight_set(wrapper, EVAS_HINT_EXPAND, 0.0f);
    evas_object_show(wrapper);

    Evas_Object* button_box = elm_box_add(wrapper);
    elm_box_horizontal_set(button_box, EINA_TRUE);

    Evas_Object* new_window_button = AddButton(button_box, "file", OnNewWindow);
    back_btn_ = AddButton(button_box, "arrow_left", OnBack);
    forward_btn_ = AddButton(button_box, "arrow_right", OnForward);
    refresh_btn_ = AddButton(button_box, "refresh", OnRefresh);
    stop_btn_ = AddButton(button_box, "close", OnStop);

#if !defined(OS_TIZEN)
    elm_object_text_set(new_window_button, "New Window");
    elm_object_text_set(back_btn_, "Back");
    elm_object_text_set(forward_btn_, "Forward");
    elm_object_text_set(refresh_btn_, "Refresh");
    elm_object_text_set(stop_btn_, "Stop");
#else
    (void)new_window_button;
#endif

    evas_object_show(button_box);
    elm_box_pack_start(wrapper, button_box);

    url_bar_ = elm_entry_add(wrapper);
    elm_entry_single_line_set(url_bar_, EINA_TRUE);
    elm_entry_scrollable_set(url_bar_, EINA_TRUE);
    evas_object_size_hint_align_set(url_bar_, EVAS_HINT_FILL, EVAS_HINT_FILL);
    evas_object_size_hint_weight_set(url_bar_, EVAS_HINT_EXPAND,
                                     EVAS_HINT_EXPAND);
    elm_entry_input_panel_layout_set(url_bar_, ELM_INPUT_PANEL_LAYOUT_URL);
    evas_object_size_hint_weight_set(url_bar_, EVAS_HINT_EXPAND, 0.0f);
    evas_object_size_hint_align_set(url_bar_, EVAS_HINT_FILL, EVAS_HINT_FILL);
    evas_object_smart_callback_add(url_bar_, "activated", OnURLEntered, this);
    elm_box_pack_end(wrapper, url_bar_);
    evas_object_show(url_bar_);

    elm_box_pack_end(box, wrapper);
  }

  Evas_Object* AddButton(Evas_Object* parent_box,
                         const char* icon_name,
                         Evas_Smart_Cb cb) {
    Evas_Object* btn = elm_button_add(parent_box);
    Evas_Object* icon = elm_icon_add(parent_box);
    elm_icon_standard_set(icon, icon_name);
    elm_object_part_content_set(btn, "icon", icon);
    evas_object_smart_callback_add(btn, "clicked", cb, this);
    elm_box_pack_end(parent_box, btn);
    evas_object_show(btn);
    return btn;
  }

  void SetAddressBarURL(const char* url) {
    if (url_bar_)
      elm_object_text_set(url_bar_, url);
  }

  void ToggleControl(UIControl control, bool is_enabled) {
    if (control == BACK_BUTTON && back_btn_) {
      elm_object_disabled_set(back_btn_, !is_enabled);
    } else if (control == FORWARD_BUTTON && forward_btn_) {
      elm_object_disabled_set(forward_btn_, !is_enabled);
    } else if (control == STOP_BUTTON && stop_btn_) {
      elm_object_disabled_set(stop_btn_, !is_enabled);
    }
  }

 private:
  static void OnBack(void* data, Evas_Object*, void*) {
    static_cast<Shell::Impl*>(data)->shell_->GoBackOrForward(-1);
  }

  static void OnNewWindow(void* data, Evas_Object*, void*) {
    Shell* shell = static_cast<Shell::Impl*>(data)->shell_;
    gfx::Size initial_size = Shell::GetShellDefaultSize();
    Shell::CreateNewWindow(shell->web_contents()->GetBrowserContext(),
                           GURL("https://www.google.com"), NULL, initial_size);
  }

  static void OnForward(void* data, Evas_Object*, void*) {
    static_cast<Shell::Impl*>(data)->shell_->GoBackOrForward(1);
  }

  static void OnRefresh(void* data, Evas_Object*, void*) {
    static_cast<Shell::Impl*>(data)->shell_->Reload();
  }

  static void OnStop(void* data, Evas_Object*, void*) {
    static_cast<Shell::Impl*>(data)->shell_->Stop();
  }

  static void OnURLEntered(void* data, Evas_Object* obj, void*) {
    GURL url(elm_object_text_get(obj));
    if (!url.has_scheme()) {
      url = GURL(std::string("http://") + elm_object_text_get(obj));
      elm_object_text_set(obj, url.spec().c_str());
    }
    static_cast<Shell::Impl*>(data)->shell_->LoadURL(url);
  }

  Evas_Object* url_bar_;
  Evas_Object* refresh_btn_;
  Evas_Object* stop_btn_;
  Evas_Object* back_btn_;
  Evas_Object* forward_btn_;

  Shell* shell_;
};

namespace {
void OnShellWindowDelRequest(void* data, Evas_Object*, void*) {
  Shell* shell = static_cast<Shell*>(data);
  shell->Close();
}
}

// static
void Shell::PlatformInitialize(const gfx::Size& default_window_size) {
  elm_policy_set(ELM_POLICY_QUIT, ELM_POLICY_QUIT_LAST_WINDOW_CLOSED);
  LOG(INFO) << "EFL Shell platform initialized";
}

void Shell::PlatformExit() {
  LOG(INFO) << "EFL Shell platform exit";
}

void Shell::PlatformCleanUp() {}

void Shell::PlatformEnableUIControl(UIControl control, bool is_enabled) {
  impl_->ToggleControl(control, is_enabled);
}

void Shell::PlatformSetAddressBarURL(const GURL& url) {
  impl_->SetAddressBarURL(url.spec().c_str());
}

void Shell::PlatformSetIsLoading(bool loading) {}

void Shell::PlatformCreateWindow(int width, int height) {}

void Shell::PlatformSetContents() {
  Evas_Object* win = efl::WindowFactory::GetHostWindow(web_contents_.get());
  evas_object_smart_callback_add(win, "delete,request", OnShellWindowDelRequest,
                                 this);
  gfx::Size initial_size = Shell::GetShellDefaultSize();
  evas_object_resize(win, initial_size.width(), initial_size.height());
  evas_object_show(win);
  window_ = win;
  impl_ = new Impl(this, static_cast<Evas_Object*>(window_));
}

void Shell::PlatformResizeSubViews() {}

void Shell::Close() {
  LOG(INFO) << "Closing Content Shell EFL";
  delete impl_;
  delete this;
}

void Shell::PlatformSetTitle(const base::string16& title) {
  elm_win_title_set(static_cast<Evas_Object*>(window_),
                    base::UTF16ToUTF8(title).c_str());
}

JavaScriptDialogManager* Shell::GetJavaScriptDialogManager(
    WebContents* source) {
  if (!dialog_manager_) {
    if (switches::IsRunLayoutTestSwitchPresent()) {
      dialog_manager_.reset(new LayoutTestJavaScriptDialogManager);
    } else {
      dialog_manager_.reset(new JavaScriptDialogManagerEfl);
    }
  }
  return dialog_manager_.get();
}

}  // namespace content
