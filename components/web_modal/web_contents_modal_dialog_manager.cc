// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/web_modal/web_contents_modal_dialog_manager.h"

#include <algorithm>
#include <utility>

#include "components/web_modal/web_contents_modal_dialog_manager_delegate.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/browser/web_contents.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"

using content::WebContents;

DEFINE_WEB_CONTENTS_USER_DATA_KEY(web_modal::WebContentsModalDialogManager);

namespace web_modal {

WebContentsModalDialogManager::~WebContentsModalDialogManager() {
  DCHECK(child_dialogs_.empty());
}

void WebContentsModalDialogManager::SetDelegate(
    WebContentsModalDialogManagerDelegate* d) {
  delegate_ = d;

  for (const auto& dialog : child_dialogs_) {
    // Delegate can be null on Views/Win32 during tab drag.
    dialog.manager->HostChanged(d ? d->GetWebContentsModalDialogHost()
                                  : nullptr);
  }
}

// TODO(gbillock): Maybe "ShowBubbleWithManager"?
void WebContentsModalDialogManager::ShowDialogWithManager(
    gfx::NativeWindow dialog,
    std::unique_ptr<SingleWebContentsDialogManager> manager) {
  if (delegate_)
    manager->HostChanged(delegate_->GetWebContentsModalDialogHost());
  child_dialogs_.emplace_back(dialog, std::move(manager));

  if (child_dialogs_.size() == 1) {
    BlockWebContentsInteraction(true);
    if (delegate_ && delegate_->IsWebContentsVisible(web_contents()))
      child_dialogs_.back().manager->Show();
  }
}

bool WebContentsModalDialogManager::IsDialogActive() const {
  return !child_dialogs_.empty();
}

void WebContentsModalDialogManager::FocusTopmostDialog() const {
  DCHECK(!child_dialogs_.empty());
  child_dialogs_.front().manager->Focus();
}

content::WebContents* WebContentsModalDialogManager::GetWebContents() const {
  return web_contents();
}

void WebContentsModalDialogManager::WillClose(gfx::NativeWindow dialog) {
  WebContentsModalDialogList::iterator dlg =
      std::find_if(child_dialogs_.begin(), child_dialogs_.end(),
                   [dialog](const DialogState& child_dialog) {
                     return child_dialog.dialog == dialog;
                   });

  // The Views tab contents modal dialog calls WillClose twice.  Ignore the
  // second invocation.
  if (dlg == child_dialogs_.end())
    return;

  bool removed_topmost_dialog = dlg == child_dialogs_.begin();
  child_dialogs_.erase(dlg);
  if (!closing_all_dialogs_ &&
      (!child_dialogs_.empty() && removed_topmost_dialog) &&
      (delegate_ && delegate_->IsWebContentsVisible(web_contents()))) {
    child_dialogs_.front().manager->Show();
  }

  BlockWebContentsInteraction(!child_dialogs_.empty());
}

WebContentsModalDialogManager::WebContentsModalDialogManager(
    content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents),
      delegate_(nullptr),
      closing_all_dialogs_(false) {}

WebContentsModalDialogManager::DialogState::DialogState(
    gfx::NativeWindow dialog,
    std::unique_ptr<SingleWebContentsDialogManager> mgr)
    : dialog(dialog), manager(std::move(mgr)) {}

WebContentsModalDialogManager::DialogState::DialogState(DialogState&& state) =
    default;

WebContentsModalDialogManager::DialogState::~DialogState() = default;

// TODO(gbillock): Move this to Views impl within Show()? It would
// call WebContents* contents = native_delegate_->GetWebContents(); and
// then set the block state. Advantage: could restrict some of the
// WCMDM delegate methods, then, and pass them behind the scenes.
void WebContentsModalDialogManager::BlockWebContentsInteraction(bool blocked) {
  WebContents* contents = web_contents();
  if (!contents) {
    // The WebContents has already disconnected.
    return;
  }

  // RenderViewHost may be null during shutdown.
  content::RenderViewHost* host = contents->GetRenderViewHost();
  if (host)
    host->GetWidget()->SetIgnoreInputEvents(blocked);
  if (delegate_)
    delegate_->SetWebContentsBlocked(contents, blocked);
}

void WebContentsModalDialogManager::CloseAllDialogs() {
  closing_all_dialogs_ = true;

  // Clear out any dialogs since we are leaving this page entirely.
  while (!child_dialogs_.empty()) {
    child_dialogs_.front().manager->Close();
  }

  closing_all_dialogs_ = false;
}

void WebContentsModalDialogManager::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  if (!navigation_handle->IsInMainFrame() || !navigation_handle->HasCommitted())
    return;

  // Close constrained windows if necessary.
  if (!net::registry_controlled_domains::SameDomainOrHost(
          navigation_handle->GetPreviousURL(), navigation_handle->GetURL(),
          net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES))
    CloseAllDialogs();
}

void WebContentsModalDialogManager::DidGetIgnoredUIEvent() {
  if (!child_dialogs_.empty()) {
    child_dialogs_.front().manager->Focus();
  }
}

void WebContentsModalDialogManager::WasShown() {
  if (!child_dialogs_.empty())
    child_dialogs_.front().manager->Show();
}

void WebContentsModalDialogManager::WasHidden() {
  if (!child_dialogs_.empty())
    child_dialogs_.front().manager->Hide();
}

void WebContentsModalDialogManager::WebContentsDestroyed() {
  // First cleanly close all child dialogs.
  // TODO(mpcomplete): handle case if MaybeCloseChildWindows() already asked
  // some of these to close.  CloseAllDialogs is async, so it might get called
  // twice before it runs.
  CloseAllDialogs();
}

void WebContentsModalDialogManager::DidAttachInterstitialPage() {
  CloseAllDialogs();
}

}  // namespace web_modal
