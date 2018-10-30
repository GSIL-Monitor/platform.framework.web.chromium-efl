// Copyright (c) 2015 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "efl/init.h"

#include <Ecore.h>
#include <Elementary.h>
#include "ecore_x_wayland_wrapper.h"

#include "base/logging.h"
#include "base/message_loop/message_loop.h"
#include "base/message_loop/message_pump_ecore.h"
#include "cc/base/switches.h"
#include "chromium_impl/build/tizen_version.h"
#include "content/common/paths_efl.h"
#include "content/public/common/content_switches.h"
#include "gpu/config/gpu_switches.h"
#include "tizen/system_info.h"
#include "ui/display/screen_efl.h"
#include "ui/gfx/switches.h"
#include "ui/gl/gl_switches.h"
#include "ui/ozone/public/ozone_platform.h"

namespace efl {

namespace {
std::unique_ptr<base::MessagePump> MessagePumpFactory() {
  return std::unique_ptr<base::MessagePump>(new base::MessagePumpEcore);
}
}  // namespace

int Initialize(int argc, const char* argv[]) {
  base::CommandLine::Init(argc, argv);

  const base::CommandLine& cmd_line = *base::CommandLine::ForCurrentProcess();
  std::string process_type =
      cmd_line.GetSwitchValueASCII(switches::kProcessType);

  if (process_type != "zygote") {
    elm_init(argc, (char**)argv);
    elm_config_accel_preference_set("hw");

    ecore_init();
#if defined(USE_WAYLAND)
#if TIZEN_VERSION_AT_LEAST(5, 0, 0)
    Ecore_Wl2_Display *_ecore_wl2_display = NULL;
    if (!ecore_wl2_init())
      return EINA_FALSE;
    if (!_ecore_wl2_display)
      _ecore_wl2_display = ecore_wl2_display_connect(NULL);
#else
    ecore_wl_init(NULL);
#endif  // TIZEN_VERSION_AT_LEAST(5, 0, 0)
#else
    ecore_x_init(NULL);
#endif
    eina_init();
    evas_init();
  }

  if (!base::MessageLoop::InitMessagePumpForUIFactory(&MessagePumpFactory)) {
    LOG(ERROR) << "Failed to install EFL message pump!";
    return 1;
  }

  ui::InstallScreenInstance();

#if defined(USE_OZONE)
  ui::OzonePlatform::InitParams params;

  // TODO: If required fetch from command-line argument.
  params.single_process = false;
  // params.using_mojo = true;
  ui::OzonePlatform::InitializeForUI(params);
#endif

  PathsEfl::Register();

  return 0;
}

void AppendPortParams(base::CommandLine& cmdline) {
  cmdline.AppendSwitchASCII(switches::kUseGL, gl::kGLImplementationEGLName);
  cmdline.AppendSwitch(switches::kInProcessGPU);

#if !defined(EWK_BRINGUP)
// [M44_2403] Temporary disabling the codes for switching to new chromium
//            FIXME: http://web.sec.samsung.net/bugzilla/show_bug.cgi?id=14040
#if defined(OS_TIZEN)
  cmdline.AppendSwitch(cc::switches::kEnableTileCompression);
  cmdline.AppendSwitchASCII(cc::switches::kTileCompressionThreshold, "-1");
  cmdline.AppendSwitchASCII(cc::switches::kTileCompressionMethod, "3");

  // Select ETC1 compression method
  if (!cmdline.HasSwitch(cc::switches::kDisableTileCompression) &&
      cmdline.HasSwitch(cc::switches::kEnableTileCompression))
    cmdline.AppendSwitch(switches::kDisableGpuRasterization);
#endif
#endif  // EWK_BRINGUP

#if !defined(OS_TIZEN)
  cmdline.AppendSwitch(switches::kIgnoreGpuBlacklist);
#endif

  if (IsTvProfile()) {
    cmdline.AppendSwitchASCII(switches::kAcceleratedCanvas2dMSAASampleCount, "4");
  }
}

}  // namespace efl
