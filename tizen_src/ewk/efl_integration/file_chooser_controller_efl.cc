// Copyright 2014-2017 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "file_chooser_controller_efl.h"

#include <Elementary.h>

#if defined(OS_TIZEN)
#include <app_control.h>
#endif

#include "base/files/file_path.h"
#include "base/strings/utf_string_conversions.h"
#include "content/common/view_messages.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/common/file_chooser_file_info.h"
#include "tizen/system_info.h"

namespace content {

namespace {

const char kDefaultTitleOpen[] = "Open";
const char kDefaultTitleSave[] = "Save";
const char kDefaultFileName[] = "/tmp";

const char kMimeTypeAudio[] = "audio/*";
const char kMimeTypeImage[] = "image/*";
const char kMimeTypeVideo[] = "video/*";

#if defined(OS_TIZEN)
static void destroy_app_handle(app_control_h* handle) {
  ignore_result(app_control_destroy(*handle));
}

static void filechooserResultCb(app_control_h request,
                                app_control_h reply,
                                app_control_result_e result,
                                void* data) {
  auto* fcc = static_cast<FileChooserControllerEfl*>(data);
  auto* rfh = fcc->GetRenderFrameHost();

  if (!rfh)
    return;

  std::vector<content::FileChooserFileInfo> files;

  if (result == APP_CONTROL_RESULT_SUCCEEDED) {
    char** selected_files = nullptr;
    int file_count = 0;
    int ret = app_control_get_extra_data_array(reply, APP_CONTROL_DATA_SELECTED,
                                               &selected_files, &file_count);

    if (ret == APP_CONTROL_ERROR_NONE && file_count > 0) {
      for (int i = 0; i < file_count; i++) {
        base::FilePath path(selected_files[i]);
        content::FileChooserFileInfo file_info;
        file_info.file_path = path;
        files.push_back(file_info);
        free(selected_files[i]);
      }
      free(selected_files);
    } else {
      LOG(ERROR) << __PRETTY_FUNCTION__ << " : "
                 << "Failed to get extra data array. Error code: " << ret;
    }
  } else {
    LOG(ERROR) << __PRETTY_FUNCTION__ << " : "
               << "Failed to launch file chooser application. Error code: "
               << result;
  }

  rfh->FilesSelectedInChooser(files, fcc->GetParams()->mode);
}

static void CameraResultCb(app_control_h request,
                           app_control_h reply,
                           app_control_result_e result,
                           void* data) {
  auto* fcc = static_cast<FileChooserControllerEfl*>(data);
  auto* rfh = fcc->GetRenderFrameHost();

  if (!rfh)
    return;

  std::vector<content::FileChooserFileInfo> files;

  if (result == APP_CONTROL_RESULT_SUCCEEDED) {
    char** return_paths = nullptr;
    int array_length = 0;
    int ret;

    ret = app_control_get_extra_data_array(reply, APP_CONTROL_DATA_SELECTED,
                                           &return_paths, &array_length);
    if (ret == APP_CONTROL_ERROR_NONE && array_length > 0) {
      for (int i = 0; i < array_length; ++i) {
        content::FileChooserFileInfo file_info;
        file_info.file_path = base::FilePath(return_paths[i]);
        files.push_back(file_info);
        free(return_paths[i]);
      }
      free(return_paths);
    } else {
      LOG(ERROR) << __PRETTY_FUNCTION__
                 << "Fail to get selected data. ERR: " << ret;
    }
  } else {
    LOG(ERROR) << __PRETTY_FUNCTION__
               << "Fail or cancel app control. ERR: " << result;
  }

  rfh->FilesSelectedInChooser(files, fcc->GetParams()->mode);
}
#endif  // defined(OS_TIZEN)

static void _fs_done(void* data, Evas_Object* obj, void* event_info) {
  FileChooserControllerEfl* inst = static_cast<FileChooserControllerEfl*>(data);
  RenderFrameHost* render_frame_host = inst->GetRenderFrameHost();

  if (!render_frame_host)
    return;

  std::vector<content::FileChooserFileInfo> files;
  const char* sel_path = static_cast<char*>(event_info);
  if (sel_path) {
    base::FilePath path(sel_path);
    content::FileChooserFileInfo file_info;
    file_info.file_path = path;
    files.push_back(file_info);
  }

  render_frame_host->FilesSelectedInChooser(files, inst->GetParams()->mode);
  evas_object_del(elm_object_top_widget_get(obj));
}

}  // namespace

FileChooserControllerEfl::FileChooserControllerEfl(
    RenderFrameHost* render_frame_host,
    const content::FileChooserParams* params)
    : render_frame_host_(render_frame_host), params_(params) {
  ParseParams();
}

void FileChooserControllerEfl::ParseParams() {
  title_ = kDefaultTitleOpen;
  file_name_ = kDefaultFileName;
  is_save_file_ = EINA_FALSE;
  folder_only_ = EINA_FALSE;

  if (!params_)
      return;

  switch (params_->mode) {
    case FileChooserParams::Open:
      break;
    case FileChooserParams::OpenMultiple:
      // only since elementary 1.8
      break;
    case FileChooserParams::UploadFolder:
      folder_only_ = EINA_TRUE;
      break;
    case FileChooserParams::Save:
      title_ = kDefaultTitleSave;
      is_save_file_ = EINA_TRUE;
      break;
    default:
      NOTREACHED();
  }

  if (!params_->title.empty())
    title_ = base::UTF16ToUTF8(params_->title);

  if (!params_->default_file_name.empty()) {
    if (params_->default_file_name.EndsWithSeparator())
      file_name_ = params_->default_file_name.value();
    else
      file_name_ = params_->default_file_name.DirName().value();
  }
}

void FileChooserControllerEfl::Open() {
#if defined(OS_TIZEN)
  if (IsMobileProfile() && params_->capture) {
    bool ret = false;
    for (const auto& type : params_->accept_types) {
      const std::string type_utf8 = base::UTF16ToUTF8(type);
      if (type_utf8 == kMimeTypeImage || type_utf8 == kMimeTypeVideo) {
        ret = LaunchCamera(type_utf8);
        break;
      }
    }
    if (!ret)
      GetRenderFrameHost()->FilesSelectedInChooser(
          std::vector<content::FileChooserFileInfo>(), params_->mode);
    return;
  }
#endif

  if (IsMobileProfile() || IsWearableProfile()) {
    if (!LaunchFileChooser())
      GetRenderFrameHost()->FilesSelectedInChooser(
          std::vector<content::FileChooserFileInfo>(), params_->mode);
  } else {
    CreateAndShowFileChooser();
  }
}

void FileChooserControllerEfl::CreateAndShowFileChooser() {
  Evas_Object* win = elm_win_util_standard_add("fileselector", title_.c_str());
  elm_win_modal_set(win, EINA_TRUE);

  Evas_Object* bx = elm_box_add(win);
  elm_win_resize_object_add(win, bx);
  elm_box_horizontal_set(bx, EINA_TRUE);
  evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
  evas_object_show(bx);

  Evas_Object* fs = elm_fileselector_add(win);
  elm_fileselector_is_save_set(fs, is_save_file_);
  elm_fileselector_folder_only_set(fs, folder_only_);
  elm_fileselector_expandable_set(fs, EINA_FALSE);

  /* start the fileselector in proper dir */
  elm_fileselector_path_set(fs, file_name_.c_str());

  evas_object_size_hint_weight_set(fs, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
  evas_object_size_hint_align_set(fs, EVAS_HINT_FILL, EVAS_HINT_FILL);
  elm_box_pack_end(bx, fs);
  evas_object_show(fs);

  /* the 'done' cb is called when the user presses ok/cancel */
  evas_object_smart_callback_add(fs, "done", _fs_done, this);

  evas_object_resize(win, 400, 400);
  evas_object_show(win);
}

bool FileChooserControllerEfl::LaunchFileChooser() {
#if defined(OS_TIZEN)
  if (!IsMobileProfile() && !IsWearableProfile())
    return false;

  app_control_h service_handle = 0;

  int ret = app_control_create(&service_handle);
  if (ret != APP_CONTROL_ERROR_NONE || !service_handle) {
    LOG(ERROR) << __PRETTY_FUNCTION__ << " : "
               << "Failed to create file chooser service. Error code: " << ret;
    return false;
  }

  std::unique_ptr<app_control_h, decltype(&destroy_app_handle)>
      service_handle_ptr(&service_handle, destroy_app_handle);

  ret = app_control_set_operation(service_handle, APP_CONTROL_OPERATION_PICK);
  if (ret != APP_CONTROL_ERROR_NONE) {
    LOG(ERROR) << __PRETTY_FUNCTION__ << " : "
               << "Failed to set operation. Error code: " << ret;
    return false;
  }

  // If there is more than one valid MIME type or file extension supported,
  // do not restrict selectable files to any specific type. The reason for this
  // is that app_control does not support multiple MIME types, that is,
  // each MIME type is associated with a particular external Tizen Application.
  if (params_->accept_types.size() == 1) {
    ret = app_control_set_mime(
        service_handle, base::UTF16ToUTF8(params_->accept_types[0]).c_str());
  } else {
    ret = app_control_set_mime(service_handle, "*/*");
  }
  if (ret != APP_CONTROL_ERROR_NONE) {
    LOG(ERROR) << __PRETTY_FUNCTION__ << " : "
               << "Failed to set specified mime type. Error code: " << ret;
    return false;
  }

  switch (params_->mode) {
    case FileChooserParams::Open:
      app_control_add_extra_data(service_handle,
                                 APP_CONTROL_DATA_SELECTION_MODE, "single");
      break;
    case FileChooserParams::OpenMultiple:
      app_control_add_extra_data(service_handle,
                                 APP_CONTROL_DATA_SELECTION_MODE, "multiple");
      break;
    default:
      // cancel file chooser request
      LOG(ERROR) << __PRETTY_FUNCTION__ << " : "
                 << "Unsupported file chooser mode. Mode: " << params_->mode;
      return false;
  }

  app_control_add_extra_data(service_handle, APP_CONTROL_DATA_TYPE, "vcf");

  ret = app_control_send_launch_request(service_handle, filechooserResultCb,
                                        this);
  if (ret != APP_CONTROL_ERROR_NONE) {
    LOG(ERROR) << __PRETTY_FUNCTION__ << " : "
               << "Failed to send launch request. Error code: " << ret;
    return false;
  }

  return true;
#else
  NOTIMPLEMENTED() << "This is only available on Tizen.";
  return false;
#endif
}

bool FileChooserControllerEfl::LaunchCamera(const std::string& mime_type) {
#if defined(OS_TIZEN)
  if (!IsMobileProfile())
    return false;

  app_control_h handle = nullptr;
  int ret;

  ret = app_control_create(&handle);
  if (ret != APP_CONTROL_ERROR_NONE || handle == nullptr) {
    LOG(ERROR) << __PRETTY_FUNCTION__
               << "Fail to create app control. ERR: " << ret;
    return false;
  }

  std::unique_ptr<app_control_h, decltype(&destroy_app_handle)> handle_ptr(
      &handle, destroy_app_handle);

  ret = app_control_set_operation(handle, APP_CONTROL_OPERATION_CREATE_CONTENT);
  if (ret != APP_CONTROL_ERROR_NONE) {
    LOG(ERROR) << __PRETTY_FUNCTION__ << "Fail to set operation. ERR: " << ret;
    return false;
  }

  ret = app_control_set_mime(handle, mime_type.c_str());
  if (ret != APP_CONTROL_ERROR_NONE) {
    LOG(ERROR) << __PRETTY_FUNCTION__ << "Fail to set mime. ERR: " << ret;
    return false;
  }

  ret = app_control_send_launch_request(handle, CameraResultCb, this);
  if (ret != APP_CONTROL_ERROR_NONE) {
    LOG(ERROR) << __PRETTY_FUNCTION__
               << "Fail to send launch request. ERR: " << ret;
    return false;
  }

  return true;
#else
  NOTIMPLEMENTED() << "This is only available on Tizen.";
  return false;
#endif
}

}  // namespace content
