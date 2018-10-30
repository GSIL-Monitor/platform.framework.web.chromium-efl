// Copyright (c) 2015 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "efl/window_factory.h"

#include <Elementary.h>

#include "base/logging.h"

using content::WebContents;

namespace efl {

std::map<const WebContents*, Evas_Object*> WindowFactory::window_map_;
WindowFactory::GetHostWindowDelegate WindowFactory::delegate_;

// static
Evas_Object* WindowFactory::GetHostWindow(const WebContents* web_contents) {
  DCHECK(web_contents);

  if (delegate_)
    return (*delegate_)(web_contents);

  if (window_map_.find(web_contents) != window_map_.end())
    return window_map_[web_contents];

  Evas_Object* win = elm_win_util_standard_add("", "");
  elm_win_autodel_set(win, EINA_TRUE);

  evas_object_smart_callback_add(win, "delete,request",
      OnWindowRemoved, web_contents);

  window_map_[web_contents] = win;

  return win;
}

// static
void WindowFactory::OnWindowRemoved(void* data, Evas_Object*, void*) {
  const WebContents* wc = static_cast<const WebContents*>(data);

  LOG(INFO) << "Closed EFL window host for WebContens: " << wc;

  DCHECK(window_map_.find(wc) != window_map_.end());
  window_map_.erase(wc);
}

} // namespace efl
