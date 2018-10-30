// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @file   utc_blink_ewk_base.cpp
 * @author Kamil Klimek <k.klimek@partner.samsung.com>
 * @date   2014-03-03
 * @brief  A Source file to be used by all unit test cases for Chromium EFL API
 *
 * This header file contains a common content for unit test cases, i.e.:
 * it includes useful header files, defines shared object pointers,
 * useful macros, functions and enumeration types.
 */

#include "utc_blink_ewk_base.h"

// Do not use them except ctor. You can access argv table from
// utc_blink_ewk_base::argv if needed.
extern int argc_;
extern char** argv_;

utc_blink_ewk_base::utc_blink_ewk_base()
    : ::testing::Test()
    , timeout(NULL)
    , main_loop_running(false)
    , main_loop_result(utc_blink_ewk_base::NoOp)
    , log_javascript(true)
    , ewk_window(NULL)
    , ewk_evas(NULL)
    , ewk_background(NULL)
    , ewk_webview(NULL)
    , ewk_context(NULL)
    , resource_dir(NULL)
{
    char *env_dir = getenv("UTC_RESOURCE_PATH");

    if (env_dir) {
        resource_dir = strdup(env_dir);
    } else {
        resource_dir = strdup("/opt/usr/resources");
    }
    for (int i = 0; i < argc_; i++)
      argv.push_back(argv_[i]);
}

utc_blink_ewk_base::~utc_blink_ewk_base()
{
    if (timeout) {
        ecore_timer_del(timeout);
        timeout = NULL;
    }

    free(resource_dir);
}

void utc_blink_ewk_base::AllowFileAccessFromFiles() {
  static const char* file_access_literal = "--allow-file-access-from-files";
  argv.push_back(const_cast<char*>(file_access_literal));
}

std::string utc_blink_ewk_base::GetResourcePath(const char* resource_path) const
{
    std::string retval = resource_dir;
    retval.append("/");
    retval.append(resource_path);

    return retval;
}

bool utc_blink_ewk_base::CompareEvasImageWithResource(Evas_Object* image, const char* resource, int pixel_fuzziness) const
{
  if (!image || !resource) {
    utc_warning("[CompareEvasImageWithResource] :: both image and resource can't be NULL");
    return false;
  }
  std::string resource_absolute_path = GetResourcePath(resource);

  Evas_Object* resource_image = evas_object_image_filled_add(GetEwkEvas());
  evas_object_image_file_set(resource_image, resource_absolute_path.c_str(), nullptr);

  int resource_w = 0, resource_h = 0;
  evas_object_image_size_get(resource_image, &resource_w, &resource_h);

  int image_w = 0, image_h = 0;
  evas_object_image_size_get(image, &image_w, &image_h);

  if (resource_w != image_w || resource_h != image_h) {
    evas_object_del(resource_image);
    utc_warning("[CompareEvasImageWithResource] :: image size differs: %dx%d (image) vs %dx%d (resource)", image_w, image_h, resource_w, resource_h);
    return false;
  }

  if (evas_object_image_colorspace_get(image) != evas_object_image_colorspace_get(resource_image)) {
    evas_object_del(resource_image);
    utc_warning("[CompareEvasImageWithResource] :: image colorspace differs");
    return false;
  }

  int bytes_per_pixel = 0;

  switch (evas_object_image_colorspace_get(image)) {
    case EVAS_COLORSPACE_ARGB8888:
      bytes_per_pixel = 4;
      break;

    case EVAS_COLORSPACE_RGB565_A5P:
      bytes_per_pixel = 2;
      break;

    case EVAS_COLORSPACE_GRY8:
      bytes_per_pixel = 1;
      break;

    default:
      evas_object_del(resource_image);
      utc_warning("[CompareEvasImageWithResource] :: unsupported colorspace");
      return false;
  }

  unsigned char* image_data = static_cast<unsigned char*>(evas_object_image_data_get(image, EINA_FALSE));
  unsigned char* resource_data = static_cast<unsigned char*>(evas_object_image_data_get(resource_image, EINA_FALSE));

  bool retval = true;
  for (int i = 0; i < image_w * image_h * bytes_per_pixel; i += bytes_per_pixel) {
    // we allow little differences in pixels, Evas_Image and SkBitmap has different decoders and there
    // may be little diference in individual bytes

    int total_diff = 0;

    for (int j = 0; j < bytes_per_pixel; ++j) {
      total_diff += abs(resource_data[i + j] - image_data[i + j]);
    }

    if (total_diff > pixel_fuzziness) {
      utc_warning("[CompareEvasImageWithResource] :: maximum fuzziness (%d) exceeded at %d pixel: %d", pixel_fuzziness, i, total_diff);
      retval = false;

      for (int j = 0; j < bytes_per_pixel; ++j) {
        utc_info("image: 0x%2X resource: 0x%2X", image_data[i + j], resource_data[i + j]);
      }

      break;
    }
  }

  evas_object_image_data_set(image, image_data);
  evas_object_image_data_set(resource_image, resource_data);
  evas_object_del(resource_image);

  return retval;
}

std::string utc_blink_ewk_base::GetResourceUrl(const char* resource_path) const
{
    std::string retval("file://");
    retval.append(GetResourcePath(resource_path));
    utc_debug("Resource:\t\"%s\"",retval.c_str());
    return retval;
}

utc_blink_ewk_base::MainLoopResult utc_blink_ewk_base::EventLoopStart(double max_time)
{
    utc_blink_ewk_base::MainLoopResult retval = utc_blink_ewk_base::NoOp;

    if (!main_loop_running) {
        main_loop_result = utc_blink_ewk_base::NoOp;

        utc_debug("[EventLoopStart] :: timeout: %f", max_time);
        main_loop_running = true;
        timeout = ecore_timer_add(max_time, timeout_cb, this);
        ecore_main_loop_begin();

        if (timeout) {
          ecore_timer_del(timeout);
          timeout = NULL;
        }

        main_loop_running = false;
        retval = main_loop_result;
    }

    return retval;
}

bool utc_blink_ewk_base::EventLoopWait(double time)
{
    if (!main_loop_running) {
        utc_debug("[EventLoopWait] :: time %f", time);
        main_loop_running = true;
        timeout = ecore_timer_add(time, timeout_cb_event_loop_wait, this);

        ecore_main_loop_begin();

        if (timeout) {
          ecore_timer_del(timeout);
          timeout = NULL;
        }

        main_loop_running = false;
        return true;
    }

    return false;
}

bool utc_blink_ewk_base::EventLoopStop(utc_blink_ewk_base::MainLoopResult result)
{
    if (main_loop_running && result != utc_blink_ewk_base::NoOp ) {
        utc_debug("[EventLoopStop] :: Setting result to: %s", (result == utc_blink_ewk_base::Success ? "Success" : "Failure"));
        main_loop_running = false;
        main_loop_result = result;
        ecore_main_loop_quit();
        return true;
    }

    return false;
}

bool utc_blink_ewk_base::TimeOut() {
  return false;
}

bool utc_blink_ewk_base::LoadError(Evas_Object* webview, Ewk_Error* error) {
  EventLoopStop(LoadFailure);
  return false;
}

void utc_blink_ewk_base::SetUp()
{
    PreSetUp();

    EwkInit();

    evas_object_smart_callback_add(ewk_webview, "load,started", load_started_cb, this);
    evas_object_smart_callback_add(ewk_webview, "load,finished", load_finished_cb, this);
    evas_object_smart_callback_add(ewk_webview, "load,error", load_error_cb, this);
    evas_object_smart_callback_add(ewk_webview, "load,progress", load_progress_cb, this);
    evas_object_smart_callback_add(ewk_webview, "console,message", ToSmartCallback(console_message_cb), this);

    PostSetUp();
}

void utc_blink_ewk_base::TearDown()
{
    PreTearDown();

    evas_object_smart_callback_del(ewk_webview, "load,started", load_started_cb);
    evas_object_smart_callback_del(ewk_webview, "load,finished", load_finished_cb);
    evas_object_smart_callback_del(ewk_webview, "load,error", load_error_cb);
    evas_object_smart_callback_del(ewk_webview, "load,progress", load_progress_cb);
    evas_object_smart_callback_del(ewk_webview, "console,message", ToSmartCallback(console_message_cb));

    EwkDeinit();

    PostTearDown();
}

void utc_blink_ewk_base::load_started_cb(void* data, Evas_Object* webview, void* event_info)
{
    utc_debug("[load,started] :: data: %p, webview: %p, event_info: %p", data, webview, event_info);
    utc_blink_ewk_base *ut = static_cast<utc_blink_ewk_base*>(data);
    ut->LoadStarted(webview);
}

void utc_blink_ewk_base::load_finished_cb(void* data, Evas_Object* webview, void* event_info)
{
    utc_debug("[load,finished] :: data: %p, webview: %p, event_info: %p", data, webview, event_info);
    utc_blink_ewk_base *ut = static_cast<utc_blink_ewk_base*>(data);
    ut->LoadFinished(webview);
}

void utc_blink_ewk_base::load_error_cb(void* data, Evas_Object* webview, void* event_info)
{
    utc_debug("[load,error] :: data: %p, webview: %p, event_info: %p", data, webview, event_info);
    utc_blink_ewk_base *ut = static_cast<utc_blink_ewk_base*>(data);
    Ewk_Error *err = static_cast<Ewk_Error *>(event_info);

    if(!ut->LoadError(webview, err)) {
        utc_warning("[load,error] :: not handled by test, stopping main loop with Failure");
        ut->EventLoopStop(utc_blink_ewk_base::Failure);
    }
}

void utc_blink_ewk_base::load_progress_cb(void* data, Evas_Object* webview, void* event_info)
{
    utc_debug("[load,progress] :: data: %p, webview: %p, event_info: %p", data, webview, event_info);
    double progress = -1.0;
    if (event_info) {
        progress = *((double*)event_info);
    }

    utc_blink_ewk_base *ut = static_cast<utc_blink_ewk_base*>(data);
    ut->LoadProgress(webview, progress);
}

void utc_blink_ewk_base::ConsoleMessage(Evas_Object*webview, const Ewk_Console_Message* msg)
{
  EXPECT_EQ(ewk_webview, webview);
  if (log_javascript)
    fprintf(stdout, "JavaScript::console (%p):\t\"%s\"", webview, ewk_console_message_text_get(msg));
}

void utc_blink_ewk_base::console_message_cb(utc_blink_ewk_base* owner, Evas_Object* webview, Ewk_Console_Message* console)
{
  ASSERT_TRUE(owner);
  owner->ConsoleMessage(webview, console);
}

Eina_Bool utc_blink_ewk_base::timeout_cb(void *data)
{
    utc_debug("[timeout] :: data: %p", data);
    utc_blink_ewk_base *ut = static_cast<utc_blink_ewk_base*>(data);

    if (!ut->TimeOut()) {
        utc_warning("[timeout] :: not handled by test, stopping main loop with Failure");
        ut->EventLoopStop(utc_blink_ewk_base::Timeout);
    }

    ut->timeout = NULL;
    return ECORE_CALLBACK_CANCEL;
}

Eina_Bool utc_blink_ewk_base::timeout_cb_event_loop_wait(void *data)
{
    utc_blink_ewk_base *ut = static_cast<utc_blink_ewk_base*>(data);
    ecore_main_loop_quit();
    ut->timeout = NULL;
    return ECORE_CALLBACK_CANCEL;
}

void utc_blink_ewk_base::EwkInit()
{
    /* 1. Standard TETware test initialization message */
    utc_info("[[ TET_MSG ]]:: ============ Startup ============");

    ewk_set_arguments(argv.size(), argv.data());

    ewk_window = elm_win_add(NULL, "TC Launcher", ELM_WIN_BASIC);
    elm_win_title_set(ewk_window, "TC Launcher");
    ewk_evas = evas_object_evas_get(ewk_window);

    ewk_background = evas_object_rectangle_add(ewk_evas);
    evas_object_name_set(ewk_background, "view");
    evas_object_color_set(ewk_background, 255, 0, 255, 255);
    evas_object_move(ewk_background, 0, 0);
    evas_object_resize(ewk_background, DEFAULT_WIDTH_OF_WINDOW, DEFAULT_HEIGHT_OF_WINDOW);
    evas_object_layer_set(ewk_background, EVAS_LAYER_MIN);

    evas_object_show(ewk_background);

    /* 3. Initialization of webview */
    ewk_context = ewk_context_default_get();
    ewk_webview = ewk_view_add_with_context(ewk_evas, ewk_context);
    evas_object_move(ewk_webview, 10, 10);
    evas_object_resize(ewk_webview, DEFAULT_WIDTH_OF_WINDOW - 20, DEFAULT_HEIGHT_OF_WINDOW - 20);

    evas_object_show(ewk_webview);
    evas_object_show(ewk_window);
}

void utc_blink_ewk_base::EwkDeinit()
{
    /* 1. Standard TETware test end/cleanup messages */
    utc_info("[[ TET_MSG ]]:: ============ Cleanup ============");

    /* 2. Freeing resources */
    if (ewk_webview)
        evas_object_del(ewk_webview);

    if (ewk_window)
        evas_object_del(ewk_window);

    ewk_window = NULL;
    ewk_evas = NULL;
    ewk_background = NULL;
    ewk_webview = NULL;
}

void utc_log(UtcLogSeverity severity, const char* format, ...) {
  static UtcLogSeverity minLogSeverity = -1;
  if (minLogSeverity == -1) {
    char *env_severity = getenv("UTC_MIN_LOG");
    if (env_severity && !strcmp(env_severity, "DEBUG")) {
      minLogSeverity = UTC_LOG_DEBUG;
    } else if (env_severity && !strcmp(env_severity, "INFO")) {
      minLogSeverity = UTC_LOG_INFO;
    } else if (env_severity && !strcmp(env_severity, "WARNING")) {
      minLogSeverity = UTC_LOG_WARNING;
    } else if (env_severity && !strcmp(env_severity, "ERROR")) {
      minLogSeverity = UTC_LOG_ERROR;
    } else {
      minLogSeverity = UTC_LOG_INFO;
    }
  }
  if (minLogSeverity > severity)
    return;

  va_list arglist;
  va_start(arglist, format);
  vfprintf(stderr, format, arglist );
  va_end(arglist);
}
