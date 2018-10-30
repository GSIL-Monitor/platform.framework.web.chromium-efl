// Copyright 2016 Samsung Electronics. All rights reseoved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/tizen_extensible_host.h"

#include "common/render_messages_ewk.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_source.h"
#include "content/public/browser/notification_types.h"
#include "content/public/browser/render_process_host.h"

// static
TizenExtensibleHost* TizenExtensibleHost::GetInstance() {
  return base::Singleton<TizenExtensibleHost>::get();
}

TizenExtensibleHost::TizenExtensibleHost()
    : extensible_api_table_{
          {"background,music", true},
          {"background,vibration", false},
          {"block,multimedia,on,call", false},
          {"csp", false},
          {"encrypted,database", false},
          {"fullscreen", false},
          {"mediastream,record", false},
          {"media,volume,control", false},
          {"prerendering,for,rotation", false},
          {"rotate,camera,view", true},
          {"rotation,lock", false},
          {"sound,mode", false},
          {"support,fullscreen", true},
          {"visibility,suspend", false},
          {"xwindow,for,fullscreen,video", false},
          {"support,multimedia", true},
      } {}

TizenExtensibleHost::~TizenExtensibleHost() {}

void TizenExtensibleHost::Initialize() {
  if (registrar_.IsEmpty()) {
    registrar_.Add(
        this, content::NOTIFICATION_RENDERER_PROCESS_CREATED,
        content::NotificationService::AllBrowserContextsAndSources());
    registrar_.Add(
        this, content::NOTIFICATION_RENDERER_PROCESS_TERMINATED,
        content::NotificationService::AllBrowserContextsAndSources());
  }
}

void TizenExtensibleHost::Observe(int type,
                                  const content::NotificationSource& source,
                                  const content::NotificationDetails& details) {
  content::RenderProcessHost* process =
      content::Source<content::RenderProcessHost>(source).ptr();
  DCHECK(process);

  int renderer_id = process->GetID();
  switch (type) {
    case content::NOTIFICATION_RENDERER_PROCESS_CREATED: {
      renderers_.insert(renderer_id);
      UpdateTizenExtensible(renderer_id);
      break;
    }
    case content::NOTIFICATION_RENDERER_PROCESS_TERMINATED: {
      renderers_.erase(renderer_id);
      break;
    }
    default:
      NOTREACHED();
      break;
  }
}

void TizenExtensibleHost::UpdateTizenExtensible(int render_process_id) {
  DCHECK(render_process_id);
  DCHECK(renderers_.find(render_process_id) != renderers_.end());
  content::RenderProcessHost* host =
      content::RenderProcessHost::FromID(render_process_id);
  if (host)
    host->Send(new EwkProcessMsg_UpdateTizenExtensible(extensible_api_table_));
}

bool TizenExtensibleHost::SetExtensibleAPI(const std::string& api_name,
                                           bool enable) {
  auto it = extensible_api_table_.find(api_name);
  if (it == extensible_api_table_.end())
    return false;
  it->second = enable;

  for_each(renderers_.begin(), renderers_.end(), [api_name, enable](int e) {
    content::RenderProcessHost* host = content::RenderProcessHost::FromID(e);
    if (host)
      host->Send(new EwkProcessMsg_SetExtensibleAPI(api_name, enable));
  });

  return true;
}

bool TizenExtensibleHost::GetExtensibleAPI(const std::string& api_name) {
  auto it = extensible_api_table_.find(api_name);
  return ((it == extensible_api_table_.end()) ? false : it->second);
}
