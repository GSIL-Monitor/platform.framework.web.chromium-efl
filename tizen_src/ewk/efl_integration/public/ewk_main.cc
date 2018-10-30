/*
 * Copyright (C) 2009-2010 ProFUSION embedded systems
 * Copyright (C) 2009-2016 Samsung Electronics. All rights reserved.
 * Copyright (C) 2012 Intel Corporation
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "ewk_main_internal.h"

#include <Ecore.h>
#include <Ecore_Evas.h>
#include <Ecore_IMF.h>
#include <Edje.h>
#include <Eina.h>
#include <Evas.h>

#include "command_line_efl.h"
#include "eweb_context.h"
#include "ewk_global_data.h"
#include "ewk_log_internal.h"
#include "ewk_view.h"
#include "private/webview_delegate_ewk.h"
#include "private/ewk_context_private.h"
#include "private/ewk_private.h"
#include "private/ewk_main_private.h"

#include "ecore_x_wayland_wrapper.h"

#if defined(TIZEN_ATK_SUPPORT)
#include <Eldbus.h>
#endif

static int _ewkInitCount = 0;

/////////////////////////////////////////////////////////////////////////////
//private function declaration here
static void _ewk_init_web_engine(void);
static void _ewk_shutdown_web_engine(void);
static void _ewk_force_acceleration() __attribute__((constructor));

extern std::string g_homeDirectory;

/**
 * \var     _ewk_log_dom
 * @brief   the log domain identifier that is used with EINA's macros
 */
int _ewk_log_dom = -1;
#if defined(USE_WAYLAND) && TIZEN_VERSION_AT_LEAST(5, 0, 0)
Ecore_Wl2_Display *_ecore_wl2_display = NULL;
#endif

/* LCOV_EXCL_START */
int ewk_init(void)
{
  if (_ewkInitCount)
      return ++_ewkInitCount;

  if (!eina_init()) {
      ERR("could not init eina.");
      goto error_eina;
  }

  _ewk_log_dom = eina_log_domain_register("ewebview-blink", EINA_COLOR_ORANGE);
  if (_ewk_log_dom < 0) {
      EINA_LOG_ERR("could not register log domain 'ewebview-blink'");
      goto error_log_domain;
  }

  if (!evas_init()) {
      ERR("could not init evas.");
      goto error_evas;
  }

  if (!ecore_init()) {
      ERR("could not init ecore.");
      goto error_ecore;
  }

  if (!ecore_evas_init()) {
      ERR("could not init ecore_evas.");
      goto error_ecore_evas;
  }

  if (!ecore_imf_init()) {
      ERR("could not init ecore_imf.");
      goto error_ecore_imf;
  }

#if defined(USE_WAYLAND)
#if TIZEN_VERSION_AT_LEAST(5, 0, 0)
  if (!ecore_wl2_init()) {
#else
  if (!ecore_wl_init(0)) {
#endif  // TIZEN_VERSION_AT_LEAST(5, 0, 0)
      ERR("could not init ecore_wl.");
      goto error_ecore_wl;
  }
#if TIZEN_VERSION_AT_LEAST(5, 0, 0)
  if (!_ecore_wl2_display)
    _ecore_wl2_display = ecore_wl2_display_connect(NULL);
#endif  // TIZEN_VERSION_AT_LEAST(5, 0, 0)
#else
  if (!ecore_x_init(0)) {
      ERR("could not init ecore_x.");
      goto error_ecore_x;
  }
#endif

  if (!edje_init()) {
      ERR("Could not init edje.");
      goto error_edje;
  }
#if defined(TIZEN_ATK_SUPPORT)
  if (!eldbus_init()) {
      CRITICAL("Could not load eldbus");
      goto error_eldbus;
  }
#endif

  _ewk_init_web_engine();
  return ++_ewkInitCount;

#if defined(TIZEN_ATK_SUPPORT)
error_eldbus:
  edje_shutdown();
#endif
error_edje:
#if defined(USE_WAYLAND)
#if TIZEN_VERSION_AT_LEAST(5, 0, 0)
  if (_ecore_wl2_display) {
    ecore_wl2_display_disconnect(_ecore_wl2_display);
    _ecore_wl2_display = NULL;
    ecore_wl2_shutdown();
  }
#else
  ecore_wl_shutdown();
#endif  // TIZEN_VERSION_AT_LEAST(5, 0, 0)
error_ecore_wl:
#else
  ecore_x_shutdown();
error_ecore_x:
#endif
  ecore_imf_shutdown();
error_ecore_imf:
  ecore_evas_shutdown();
error_ecore_evas:
  ecore_shutdown();
error_ecore:
  evas_shutdown();
error_evas:
  eina_log_domain_unregister(_ewk_log_dom);
  _ewk_log_dom = -1;
error_log_domain:
  eina_shutdown();
error_eina:
  return 0;
}
/* LCOV_EXCL_STOP */

int ewk_shutdown(void)
{
  if (!_ewkInitCount || --_ewkInitCount)
      return _ewkInitCount;

  _ewk_shutdown_web_engine();

#if defined(TIZEN_ATK_SUPPORT)
  eldbus_shutdown();
#endif
  edje_shutdown();
#if defined(USE_WAYLAND)
#if TIZEN_VERSION_AT_LEAST(5, 0, 0)
  if (_ecore_wl2_display) {
    ecore_wl2_display_disconnect(_ecore_wl2_display);
    _ecore_wl2_display = NULL;
    ecore_wl2_shutdown();
  }
#else
  ecore_wl_shutdown();
#endif  // TIZEN_VERSION_AT_LEAST(5, 0, 0)
#else
  ecore_x_shutdown();
#endif
  ecore_imf_shutdown();
  ecore_evas_shutdown();
  ecore_shutdown();
  evas_shutdown();
  eina_log_domain_unregister(_ewk_log_dom);
  _ewk_log_dom = -1;
  eina_shutdown();

  return 0;
}

/* LCOV_EXCL_START */
void ewk_set_arguments(int argc, char** argv)
{
    CommandLineEfl::Init(argc, argv);
}

void ewk_home_directory_set(const char* path)
{
  if (!path)
    g_homeDirectory.clear();
  else
    g_homeDirectory = path;
}
/* LCOV_EXCL_STOP */
/////////////////////////////////////////////////////////////////////////////////////////////
//Private functions implementations for ewk_main module

//Initialize web engine
void _ewk_init_web_engine()
{
}

void _ewk_shutdown_web_engine(void)
{
  //TODO: any web engine destroy to be done here
  CommandLineEfl::Shutdown();
  Ewk_Context::DefaultContextRelease();
  EwkGlobalData::Delete();
}

void _ewk_force_acceleration()
{
  // Chromium-efl port does not support s/w mode. So we need to set h/w mode
  // before creating elm_window. To do this, make constructor function which is
  // called at library loading time and set "ELM_ACCEL=hw" here. If not, native
  // app which does not call elm_config_accel_preference_set() function will
  // fail to execute.
  setenv("ELM_ACCEL", "hw", 1);

  // Chromium-efl does not support evasgl render thread feature yet.
  setenv("EVAS_GL_DISABLE_RENDER_THREAD", "1", 1);
}

