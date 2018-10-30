// Copyright 2013 The Chromium Authors. All rights reserved.
// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/shell/browser/shell_web_contents_view_delegate.h"

#include <Evas.h>
#include <Elementary.h>

#include "base/command_line.h"
#include "content/public/browser/render_view_host.h"
#include "content/shell/browser/shell.h"
#include "content/shell/browser/shell_browser_context.h"
#include "content/shell/browser/shell_content_browser_client.h"
#include "content/shell/browser/shell_devtools_frontend.h"
#include "content/shell/common/shell_switches.h"
#include "third_party/WebKit/public/web/WebContextMenuData.h"

using blink::WebContextMenuData;

namespace content {

namespace {

static Evas_Object* g_context_menu = NULL;

void OpenInNewWindow(void* data, Evas_Object*, void*) {
  const ContextMenuParams* params = static_cast<const ContextMenuParams*>(data);
  ShellBrowserContext* browser_context =
      ShellContentBrowserClient::Get()->browser_context();
  Shell::CreateNewWindow(browser_context, params->link_url, NULL, gfx::Size());
}

void Cut(void* data, Evas_Object*, void*) {
  WebContents* wc = static_cast<WebContents*>(data);
  wc->Cut();
}

void Copy(void* data, Evas_Object*, void*) {
  WebContents* wc = static_cast<WebContents*>(data);
  wc->Copy();
}

void Paste(void* data, Evas_Object*, void*) {
  WebContents* wc = static_cast<WebContents*>(data);
  wc->Paste();
}

void Delete(void* data, Evas_Object*, void*) {
  WebContents* wc = static_cast<WebContents*>(data);
  wc->Delete();
}

void Forward(void* data, Evas_Object*, void*) {
  WebContents* wc = static_cast<WebContents*>(data);
  wc->GetController().GoToOffset(1);
  wc->Focus();
}

void Back(void* data, Evas_Object*, void*) {
  WebContents* wc = static_cast<WebContents*>(data);
  wc->GetController().GoToOffset(-1);
  wc->Focus();
}

void Reload(void* data, Evas_Object*, void*) {
  WebContents* wc = static_cast<WebContents*>(data);
  wc->GetController().Reload(content::ReloadType::NORMAL, false);
  wc->Focus();
}
}

WebContentsViewDelegate* CreateShellWebContentsViewDelegate(
    WebContents* web_contents) {
  return new ShellWebContentsViewDelegate(web_contents);
}

ShellWebContentsViewDelegate::ShellWebContentsViewDelegate(
    WebContents* web_contents)
    : web_contents_(web_contents) {}

ShellWebContentsViewDelegate::~ShellWebContentsViewDelegate() {}

void ShellWebContentsViewDelegate::ShowContextMenu(
    RenderFrameHost* render_frame_host,
    const ContextMenuParams& params) {
  if (switches::IsRunLayoutTestSwitchPresent())
    return;

  if (g_context_menu) {
    elm_menu_close(g_context_menu);
    evas_object_del(g_context_menu);
  }

  params_ = params;

  bool has_link = !params.unfiltered_link_url.is_empty();
  Evas_Object* parent =
      static_cast<Evas_Object*>(web_contents_->GetContentNativeView());

  Evas_Object* menu = elm_menu_add(parent);
  Elm_Object_Item* menu_it = NULL;

  if (has_link) {
    elm_menu_item_add(menu, NULL, NULL, "Open in new window", OpenInNewWindow,
                      &params_);
    elm_menu_item_separator_add(menu, NULL);
  }

  if (params_.is_editable) {
    menu_it = elm_menu_item_add(menu, NULL, NULL, "Cut", Cut, web_contents_);
    elm_object_item_disabled_set(
        menu_it, (params_.edit_flags & WebContextMenuData::kCanCut) == 0);
    menu_it = elm_menu_item_add(menu, NULL, NULL, "Copy", Copy, web_contents_);
    elm_object_item_disabled_set(
        menu_it, (params_.edit_flags & WebContextMenuData::kCanCopy) == 0);
    menu_it =
        elm_menu_item_add(menu, NULL, NULL, "Paste", Paste, web_contents_);
    elm_object_item_disabled_set(
        menu_it, (params_.edit_flags & WebContextMenuData::kCanPaste) == 0);
    menu_it = elm_menu_item_add(menu, NULL, "delete", "Delete", Delete,
                                web_contents_);
    elm_object_item_disabled_set(
        menu_it, (params_.edit_flags & WebContextMenuData::kCanDelete) == 0);
    elm_menu_item_separator_add(menu, NULL);
  }

  menu_it = elm_menu_item_add(menu, NULL, "back", "Back", Back, web_contents_);
  elm_object_item_disabled_set(menu_it,
                               !web_contents_->GetController().CanGoBack());
  menu_it = elm_menu_item_add(menu, NULL, "forward", "Forward", Forward,
                              web_contents_);
  elm_object_item_disabled_set(menu_it,
                               !web_contents_->GetController().CanGoForward());
  elm_menu_item_add(menu, NULL, "reload", "Reload", Reload, web_contents_);

  g_context_menu = menu;
  elm_menu_move(menu, params.x, params.y);
  evas_object_show(menu);
}

}  // namespace content
