// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <EWebKit_internal.h>
#include <Elementary.h>
#include <getopt.h>
#include <unistd.h>
#include <vector>
#if defined(OS_TIZEN)
#include <appfw/app.h>
#endif // OS_TIZEN

#include "browser.h"
#include "logger.h"

// Default command line flags for all profiles and platforms
const char* kDefaultCommandLineFlags[] = {
    "allow-file-access-from-files",
    "process-per-tab",
};

// Default command line flags added by default when
// ubrowser is started with --mobile|-m flag.
const char* kMobileFlags[] = {
    "use-mobile-user-agent",
    "enable-overlay-scrollbar",
    "enable-viewport",
    "enable-prefer-compositing-to-lcd-text",
    "ewk-enable-mobile-features-for-desktop",
};

extern int logger_show_trace;
extern int logger_use_color;
std::string user_agent;
std::string tizen_version;
Browser::GUILevel gui_level = Browser::UBROWSER_GUI_LEVEL_URL_ONLY;
double zoom_factor = 0;
#if defined(OS_TIZEN) && !defined(OS_TIZEN_TV_PRODUCT)
int desktop_mode = 0;
#else
int desktop_mode = 1;
#endif

#if defined(TIZEN_VIDEO_HOLE)
int video_hw_overlay = 0;
#endif

struct AppData {
  AppData() : browser(nullptr), ewk_initialized(false) { }

  std::vector<std::string> urls;
  Browser* browser;
  bool ewk_initialized;
};

void show_help_and_exit(const char* app_name) {
  printf("Usage: %s [OPTION]... [URL]...\n\n", app_name);
  printf("Available options:\n");
  printf("  -v, --verbose    Print verbose application logs\n");
  printf("  -n, --no-color   Don't use colors in application logs\n");
  printf("  -d, --desktop    Run application UI in desktop mode\n");
  printf("  -m, --mobile     Run application UI in mobile mode\n");
  printf("  -u, --user-agent Set user agent string\n");
#if defined(TIZEN_VIDEO_HOLE)
  printf("  -o, --overlay    Enable the H/W overlay of video\n");
#endif
  printf("  -l, --gui-level  Run application different GUI elements\n");
  printf("        0 - no GUI\n");
  printf("        1 - show URL bar only, default option\n");
  printf("        2 - minimal GUI\n");
  printf("        3 - full GUI\n");
  printf("  -z, --zoom       Set zoom factor using double number\n");
  printf("  -h, --help       Show this help message\n");
  printf("  -t, --tizen      Run WebView with compliance to Tizen version,\n");
  printf("                   for example, -t 2.4 or -t 2.3.1 etc.\n");
  exit(0);
}

static bool verify_gui_level(const char* value) {
  int level = atoi(value);

  if ((level == 0 && value[0] != '0') ||
      level < Browser::UBROWSER_GUI_LEVEL_NONE ||
      level > Browser::UBROWSER_GUI_LEVEL_ALL) {
    log_error("Unsupported GUI level!");
    return false;
  }

  gui_level = static_cast<Browser::GUILevel>(level);
  return true;
}

static bool verify_zoom_factor(const char* value) {
  double zoom = atof(value);

  if (zoom <= 0 || zoom > 5) {
    log_error("Unsupported Zoom factor!");
    return false;
  }

  zoom_factor = zoom;
  return true;
}

void parse_options(int argc, char** argv, AppData* data) {
  int show_help = 0;

  while (1) {
    int c = -1;
    static struct option long_options[] = {
      {"verbose", no_argument, &logger_show_trace, 1},
      {"no-color", no_argument, &logger_use_color, 0},
      {"desktop", no_argument, &desktop_mode, 1},
      {"mobile", no_argument, &desktop_mode, 0},
      {"user-agent", required_argument, 0, 'u'},
      {"gui-level", required_argument, 0, 'l'},
      {"zoom", required_argument, 0, 'z'},
      {"help", no_argument, &show_help, 1},
      {"tizen-version", required_argument, 0, 't'},
#if defined(TIZEN_VIDEO_HOLE)
      {"overlay", no_argument, &video_hw_overlay, 1},
#endif
      {0, 0, 0, 0}
    };

    int option_index = 0;
#if defined(TIZEN_VIDEO_HOLE)
    c = getopt_long(argc, argv, "vndmhou:l:z:t:", long_options, &option_index);
#else
    c = getopt_long(argc, argv, "vndmhu:l:z:t:", long_options, &option_index);
#endif

    if (c == -1)
      break;

    switch (c) {
    case 'v':
      logger_show_trace = 1;
      break;
    case 'n':
      logger_use_color = 0;
      break;
    case 'd':
      desktop_mode = 1;
      break;
    case 'm':
      desktop_mode = 0;
      break;
    case 'h':
      show_help = 1;
      break;
#if defined(TIZEN_VIDEO_HOLE)
    case 'o':
      video_hw_overlay = 1;
      break;
#endif
    case 'u':
      user_agent = optarg;
      break;
    case 'l':
      if (!verify_gui_level(optarg))
        show_help = 1;
      break;
    case 'z':
      if (!verify_zoom_factor(optarg))
        show_help = 1;
      break;
    case 't':
        tizen_version = optarg;
        break;
    default:
      // Ignore EFL or chromium specific options,
      // ewk_set_arguments should handle them
      continue;
    }
  }

  if (show_help)
    show_help_and_exit(argv[0]);

  for (int i = optind; i < argc; ++i)
    if (argv[i][0] != '-')
      data->urls.push_back(std::string(argv[i]));

  if (data->urls.empty())
    data->urls.push_back("http://www.google.com");
}

static bool app_create(void* data) {
  AppData* app_data = static_cast<AppData*>(data);

  if (!ewk_init())
    return false;

  elm_policy_set(ELM_POLICY_QUIT, ELM_POLICY_QUIT_LAST_WINDOW_CLOSED);

  app_data->browser = new Browser(desktop_mode, gui_level, user_agent, tizen_version);
#if defined(TIZEN_VIDEO_HOLE)
  if (video_hw_overlay)
    app_data->browser->SetEnableVideoHole(true);
#endif
  if (zoom_factor > 0)
    app_data->browser->SetDefaultZoomFactor(zoom_factor);
  std::vector<std::string>::iterator it = app_data->urls.begin();
  for (;it != app_data->urls.end(); ++it)
    app_data->browser->CreateWindow().LoadURL(*it);
  app_data->urls.clear();
  app_data->ewk_initialized = true;
  return true;
}

static void app_terminate(void* data) {
  AppData* app_data = static_cast<AppData*>(data);
  if (app_data->browser)
    delete app_data->browser;
  if (app_data->ewk_initialized)
    ewk_shutdown();
}

#if defined(OS_TIZEN) && defined(TIZEN_APP)
static void app_pause(void* data) {
  log_debug("Tizen uBrowser pause");
}

static void app_resume(void* data) {
  log_debug("Tizen uBrowser resume");
}
#endif // OS_TIZEN

// Passes user args and adds custom ubrowser flags.
static void set_arguments(int argc, char **argv) {
  static std::vector<char*> ubrowser_argv;
  // Copy original params first.
  for (int i = 0; i < argc; i++)
    ubrowser_argv.push_back(argv[i]);

  // Add a default command lines.
  for (auto arg : kDefaultCommandLineFlags)
    ubrowser_argv.push_back(const_cast<char*>(arg));

  // Add mobile flags for desktop ubrowser --mobile|-m.
#if !defined(OS_TIZEN)
  if (!desktop_mode) {
    for (auto arg : kMobileFlags)
      ubrowser_argv.push_back(const_cast<char*>(arg));
  }
#endif
  ewk_set_arguments(ubrowser_argv.size(), ubrowser_argv.data());
}

int main(int argc, char** argv) {
  elm_init(argc, argv);
  elm_config_accel_preference_set("opengl");

  AppData data;
  parse_options(argc, argv, &data);
  set_arguments(argc, argv);

  int ret = 0;
// Enable the section below once ubrowser is ready
// to replace mini_browser. For the code below to work
// proper manifest needs to be added to the rpm package
#if defined(OS_TIZEN) && defined(TIZEN_APP)
  ui_app_lifecycle_callback_s ops;

  memset(&ops, 0, sizeof(ops));
  ops.create = app_create;
  ops.terminate = app_terminate;
  ops.pause = app_pause;
  ops.resume = app_resume;

  ret = ui_app_main(argc, argv, &ops, &data)
#else
  if (!app_create(&data))
    return EXIT_FAILURE;

  elm_run();

  app_terminate(&data);
#endif

  return ret;
}
