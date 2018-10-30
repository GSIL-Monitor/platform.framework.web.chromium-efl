// Copyright 2017 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/javascript_interface/gin_native_bridge_dispatcher_host.h"

#include "base/logging.h"
#include "base/task_runner_util.h"
#include "browser/javascript_interface/gin_native_bridge_message_filter.h"
#include "common/gin_native_bridge_messages.h"
#include "common/gin_native_bridge_value.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/web_contents.h"
#include "ipc/ipc_message_utils.h"

namespace content {

GinNativeBridgeDispatcherHost::GinNativeBridgeDispatcherHost(
    WebContents* web_contents)
    : WebContentsObserver(web_contents), next_object_id_(1) {}

GinNativeBridgeDispatcherHost::~GinNativeBridgeDispatcherHost() {}

void GinNativeBridgeDispatcherHost::InstallFilterAndRegisterAllRoutingIds() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (!web_contents()->GetMainFrame()->GetProcess()->GetChannel()) {
    return;  // LCOV_EXCL_LINE
  }

  auto filter = GinNativeBridgeMessageFilter::FromHost(this, true);
  // ForEachFrame is synchronous.
  web_contents()->ForEachFrame(
      base::Bind(&GinNativeBridgeMessageFilter::AddRoutingIdForHost, filter,
                 base::Unretained(this)));
}

void GinNativeBridgeDispatcherHost::RenderFrameCreated(
    RenderFrameHost* render_frame_host) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  if (auto filter = GinNativeBridgeMessageFilter::FromHost(this, false)) {
    filter->AddRoutingIdForHost(this, render_frame_host);  // LCOV_EXCL_LINE
  } else {
    InstallFilterAndRegisterAllRoutingIds();
  }
  for (NamedObjectMap::const_iterator iter = named_objects_.begin();
       iter != named_objects_.end(); ++iter) {
    /* LCOV_EXCL_START */
    render_frame_host->Send(new GinNativeBridgeMsg_AddNamedObject(
        render_frame_host->GetRoutingID(), iter->first, iter->second));
    /* LCOV_EXCL_STOP */
  }
}

void GinNativeBridgeDispatcherHost::WebContentsDestroyed() {
  scoped_refptr<GinNativeBridgeMessageFilter> filter =
      GinNativeBridgeMessageFilter::FromHost(this, false);
  if (filter)
    filter->RemoveHost(this);
}

bool GinNativeBridgeDispatcherHost::AddNamedObject(
    Evas_Object* view,
    Ewk_View_Script_Message_Cb callback,
    const std::string& name) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  GinNativeBoundObject::ObjectID object_id;
  NamedObjectMap::iterator iter = named_objects_.find(name);
  if (iter != named_objects_.end())
    return false;

  object_id = AddObject(view, callback, name, true, 0);
  named_objects_[name] = object_id;

  InstallFilterAndRegisterAllRoutingIds();

  web_contents()->SendToAllFrames(
      new GinNativeBridgeMsg_AddNamedObject(MSG_ROUTING_NONE, name, object_id));

  return true;
}

/* LCOV_EXCL_START */
void GinNativeBridgeDispatcherHost::RemoveNamedObject(const std::string& name) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  NamedObjectMap::iterator iter = named_objects_.find(name);
  if (iter == named_objects_.end())
    return;

  // |name| may come from |named_objects_|. Make a copy of name so that if
  // |name| is from |named_objects_| it'll be valid after the remove below.
  const std::string copied_name(name);
  {
    base::AutoLock locker(objects_lock_);
    objects_[iter->second]->RemoveName();
  }
  named_objects_.erase(iter);

  // As the object isn't going to be removed from the JavaScript side until the
  // next page reload, calls to it must still work, thus we should continue to
  // hold it. All the transient objects and removed named objects will be purged
  // during the cleansing caused by DocumentAvailableInMainFrame event.

  web_contents()->SendToAllFrames(
      new GinNativeBridgeMsg_RemoveNamedObject(MSG_ROUTING_NONE, copied_name));
}
/* LCOV_EXCL_STOP */

GinNativeBoundObject::ObjectID GinNativeBridgeDispatcherHost::AddObject(
    Evas_Object* view,
    Ewk_View_Script_Message_Cb callback,
    const std::string& name,
    bool is_named,
    int32_t holder) {
  scoped_refptr<GinNativeBoundObject> new_object =
      is_named
          ? GinNativeBoundObject::CreateNamed(view, callback, name)
          : GinNativeBoundObject::CreateTransient(view, callback, name, holder);
  // Note that we are abusing the fact that StaticAtomicSequenceNumber
  // uses Atomic32 as a counter, so it is guaranteed that it will not
  // overflow our int32_t IDs. IDs start from 1.
  GinNativeBoundObject::ObjectID object_id = next_object_id_++;
  {
    base::AutoLock locker(objects_lock_);
    objects_[object_id] = new_object;
  }
  return object_id;
}

void GinNativeBridgeDispatcherHost::DocumentAvailableInMainFrame() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  // Called when the window object has been cleared in the main frame.
  // That means, all sub-frames have also been cleared, so only named
  // objects survived.

  // FIXME : need to implement.
}

/* LCOV_EXCL_START */
scoped_refptr<GinNativeBoundObject> GinNativeBridgeDispatcherHost::FindObject(
    GinNativeBoundObject::ObjectID object_id) {
  // Can be called on any thread.
  base::AutoLock locker(objects_lock_);
  auto iter = objects_.find(object_id);
  if (iter != objects_.end())
    return iter->second;
  LOG(ERROR) << "WebView: Unknown object: " << object_id;  // LCOV_EXCL_LINE
  return nullptr;
}

void GinNativeBridgeDispatcherHost::OnHasMethod(
    GinNativeBoundObject::ObjectID object_id,
    const std::string& method_name,
    bool* result) {
  scoped_refptr<GinNativeBoundObject> object = FindObject(object_id);

  *result = (object.get()) ? true : false;
}

void GinNativeBridgeDispatcherHost::OnInvokeMethod(
    int routing_id,
    GinNativeBoundObject::ObjectID object_id,
    const std::string& method_name,
    const base::ListValue& arguments,
    base::ListValue* wrapped_result,
    content::GinNativeBridgeError* error_code) {
  DCHECK(routing_id != MSG_ROUTING_NONE);
  if (method_name != "postMessage") {
    wrapped_result->Append(base::MakeUnique<base::Value>());
    *error_code = kGinNativeBridgeMethodNotFound;
    return;
  }

  scoped_refptr<GinNativeBoundObject> object = FindObject(object_id);
  if (!object.get()) {
    LOG(ERROR) << "WebView: Unknown object: " << object_id;
    wrapped_result->Append(base::MakeUnique<base::Value>());
    *error_code = kGinNativeBridgeUnknownObjectId;
    return;
  }

  const base::ListValue* list;
  arguments.GetAsList(&list);

  Ewk_Script_Message msg;
  msg.name = object->Name();

  if (named_objects_.find(msg.name) == named_objects_.end()) {
    wrapped_result->Append(base::MakeUnique<base::Value>());
    *error_code = kGinNativeBridgeMessageNameIsWrong;
    return;
  }

  JavaScript_Values values;
  values.bool_buf = EINA_FALSE;
  values.int_buf = -1;
  values.double_buf = -1;
  values.str_buf = "";

  bool bool_buf = false;
  bool should_free = false;
  base::ListValue::const_iterator iter = list->begin();

  switch (iter->GetType()) {
    case base::Value::Type::BOOLEAN:
      iter->GetAsBoolean(&bool_buf);
      (bool_buf) ? values.bool_buf = EINA_TRUE : values.bool_buf = EINA_FALSE;
      msg.body = &values.bool_buf;
      break;
    case base::Value::Type::INTEGER:
      iter->GetAsInteger(&values.int_buf);
      msg.body = &values.int_buf;
      break;
    case base::Value::Type::DOUBLE:
      iter->GetAsDouble(&values.double_buf);
      msg.body = &values.double_buf;
      break;
    case base::Value::Type::STRING:
      iter->GetAsString(&values.str_buf);
      msg.body = strdup(values.str_buf.c_str());
      should_free = true;
      break;
    case base::Value::Type::DICTIONARY:
      const base::DictionaryValue* dict;
      iter->GetAsDictionary(&dict);
      values.str_buf = ConvertDictionaryValueToString(dict);
      msg.body = strdup(values.str_buf.c_str());
      should_free = true;
      break;
    case base::Value::Type::LIST:
      const base::ListValue* array;
      iter->GetAsList(&array);
      values.str_buf = ConvertListValueToString(array);
      msg.body = strdup(values.str_buf.c_str());
      should_free = true;
      break;
    default:
      wrapped_result->Append(base::MakeUnique<base::Value>());
      *error_code = kGinNativeBridgeNotSupportedTypes;
      return;
  }

  object->CallBack()(object->GetView(), msg);
  wrapped_result->Append(GinNativeBridgeValue::CreateObjectIDValue(object_id));

  if (should_free)
    free(msg.body);

  return;
}

std::string GinNativeBridgeDispatcherHost::ConvertListValueToString(
    const base::ListValue* list) {
  bool init = false;
  bool bool_buf = false;
  int int_buf = -1;
  double double_buf = -1;
  std::string str_buf = "";
  std::string token = "";

  str_buf = "[";
  for (base::ListValue::const_iterator iter_list = list->begin();
       iter_list != list->end(); ++iter_list) {
    if (init)
      str_buf += ",";

    switch (iter_list->GetType()) {
      case base::Value::Type::BOOLEAN:
        bool_buf = false;
        iter_list->GetAsBoolean(&bool_buf);
        (bool_buf) ? str_buf += "true" : str_buf += "false";
        break;
      case base::Value::Type::INTEGER:
        int_buf = -1;
        iter_list->GetAsInteger(&int_buf);
        str_buf += std::to_string(int_buf);
        break;
      case base::Value::Type::DOUBLE:
        double_buf = -1;
        iter_list->GetAsDouble(&double_buf);
        str_buf += std::to_string(double_buf);
        break;
      case base::Value::Type::STRING:
        token = "";
        iter_list->GetAsString(&token);
        str_buf += "\"";
        str_buf += token;
        str_buf += "\"";
        break;
      case base::Value::Type::DICTIONARY:
        const base::DictionaryValue* dict;
        iter_list->GetAsDictionary(&dict);
        str_buf += ConvertDictionaryValueToString(dict);
        break;
      case base::Value::Type::LIST:
        const base::ListValue* array;
        iter_list->GetAsList(&array);
        str_buf += ConvertListValueToString(array);
        break;
      default:
        str_buf += "\"\"";
        break;
    }
    init = true;
  }
  str_buf += "]";

  return str_buf;
}

std::string GinNativeBridgeDispatcherHost::ConvertDictionaryValueToString(
    const base::DictionaryValue* dict) {
  bool init = false;
  bool bool_buf = false;
  int int_buf = -1;
  double double_buf = -1;
  std::string str_buf = "";
  std::string token = "";

  str_buf = "{";
  for (base::DictionaryValue::Iterator iter_dict(*dict); !iter_dict.IsAtEnd();
       iter_dict.Advance()) {
    if (init)
      str_buf += ",";
    str_buf += "   \"";
    str_buf += iter_dict.key();
    str_buf += "\": ";

    switch (iter_dict.value().GetType()) {
      case base::Value::Type::BOOLEAN:
        bool_buf = false;
        iter_dict.value().GetAsBoolean(&bool_buf);
        (bool_buf) ? str_buf += "true" : str_buf += "false";
        break;
      case base::Value::Type::INTEGER:
        int_buf = -1;
        iter_dict.value().GetAsInteger(&int_buf);
        str_buf += std::to_string(int_buf);
        break;
      case base::Value::Type::DOUBLE:
        double_buf = -1;
        iter_dict.value().GetAsDouble(&double_buf);
        str_buf += std::to_string(double_buf);
        break;
      case base::Value::Type::STRING:
        token = "";
        iter_dict.value().GetAsString(&token);
        str_buf += "\"";
        str_buf += token;
        str_buf += "\"";
        break;
      case base::Value::Type::DICTIONARY:
        const base::DictionaryValue* dict;
        iter_dict.value().GetAsDictionary(&dict);
        str_buf += ConvertDictionaryValueToString(dict);
        break;
      case base::Value::Type::LIST:
        const base::ListValue* array;
        iter_dict.value().GetAsList(&array);
        str_buf += ConvertListValueToString(array);
        break;
      default:
        str_buf += "\"\"";
        break;
    }
    init = true;
  }
  str_buf += "}";

  return str_buf;
}
/* LCOV_EXCL_STOP */

}  // namespace content
