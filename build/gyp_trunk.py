# Copyright 2014 Samsung Electronics. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Wrapper script upon gyp_chromium.
# Enforces our configuration to take precedence by providing it to the gyp
# build system as a list of command line variable assignments.

import optparse
import os
import re
import sys

import gyp_chromium
import gyp_environment

defaults = {
  # AddressSanitizer: http://www.chromium.org/developers/testing/addresssanitizer
  'asan': 0,
  'profiling': 0,
  'component': 'static_library',

  # Minimal build configuration.
  # Used to sanity check that configurable options can be actually turned off.
  'minimal': 0,

  # Disable support for building desktop chrome by default.
  'enable_desktop_chrome': 0,

  # Turns on for low-end specific optimizations and vice-versa.
  'low_end_target': 0,

  # Disable components only used by Samsung's Android browser (and breaks building ChromeShell).
  'enable_s_android_browser': 0,

  # Disable components only used by Samsung's Cowboy browser.
  'enable_s_cowboy_browser': 0,

  # Disable components only used by Samsung's Android browser
  # (and breaks building TestShell)
  # Turns on Kerberos Feature
  'enable_kerberos_feature': 0,

  # Disable components only used by Samsung's Daydream VR browser.
  'enable_s_dreamvr_browser': 0,

  # Disable trace events by default. Enable them if you need to use about:tracing.
  'enable_trace_events': 0,

  # Re-enable using mmap() to speed up the disk cache for ARM Android - see net/net.gyp
  'posix_avoid_mmap': 0,

  # Enable neon in compile time instead of runtime by default.
  # FIXME: http://107.108.218.239/bugzilla/show_bug.cgi?id=8555
  #        At the moment the build is broken with this. Should be fixed and uncommented.
  #'arm_neon': 1,
  #'arm_neon_optional': 0,

  # Disable debug logs and some other developer-only features
  'buildtype': 'Official',

  # Always disable tcmalloc and friends as it is causing build problems on Linux.
  # (On Android it is never used anyway.)
  'use_allocator': 'none',
}

conditions = [
  ['profiling==1 or asan==1', {
    # Build time neon does not play nicely with instrumentation.
    'arm_neon': 0,
    'arm_neon_optional': 1,
  }],
  ['minimal==1', {
    # Minimal build configuration
    # Used to sanity check that configurable options can be actually turned off.
    'arm_neon': 0,
    'arm_neon_optional': 1,
    'disable_file_support': 1,
    'disable_ftp_support': 1,
    'enable_basic_printing': 0,
    'enable_print_preview': 0,
    'enable_setting_search_from_js': 0,
    'enable_trace_events': 1,
    'low_end_target': 1,
    'notifications': 0,
    'proprietary_codecs': 0,
  }],
  ['enable_desktop_chrome!=1', {
    # Configuration for all Samsung products
    # Disable features that are not desired in products (e.g. Google specific)
    'disable_nacl': 1,
    'enable_app_list': 0,
    'enable_background': 0,
    'enable_extensions': 0,
    'enable_google_now': 0,
    'enable_pepper_cdms': 0,
    'enable_plugins': 0,
    'enable_prod_wallet_service': 0,
    'enable_task_manager': 0,
    'ffmpeg_branding':'Chrome',
    'proprietary_codecs': 1,
    'remoting': 0,
    'use_alsa': 0,
    'use_cups': 0,
    'use_gconf': 0,
    'use_gio': 0,
    'use_gnome_keyring': 0,
  }],
  ['low_end_target==1', {
    # Static DATA reduction.
    'v8_use_snapshot' : 'false',
  }],
  ['OS!="android"', {
    'disable_fatal_linker_warnings': 1,
  }],
  ['enable_s_android_browser==1', {
    'enable_extensions': 0,
    'enable_managed_users': 0,
    'enable_ucproxy_feature': 1,
    'enable_supervised_users': 0,
  }],
  ['enable_s_cowboy_browser==1', {
    # Enable the Android Chromium linker to share ELF .data.rel.ro segments
    # between renderers and save RAM.
    'use_chromium_linker': 1,
    'enable_kerberos_feature': 1,
    'enable_multidex': 1,
  }],
  ['enable_s_vr_browser==1', {
    # Enable the Android Chromium linker to share ELF .data.rel.ro segments
    # between renderers and save RAM.
    'use_chromium_linker': 1,
    'enable_kerberos_feature': 1,
    'enable_multidex': 1,
    'enable_sharedlib_v8': 1,
    'v8_product': 'v8',
    'enable_webvr': 1,
  }],
  ['is_xenial==1', {
    # Disable use sysroot for ubuntu 16.04.
    # https://codereview.chromium.org/1699713002/
    'use_sysroot': 0,
    # fix unsupported reloc 42 against global symbol __gmon_start__
    'linux_use_bundled_gold': 0,
  }],
]


def check_assignment(name, value):
  """Checks if |name=value| is a valid python statement (with some accuracy)."""
  try:
    context = {}
    context[name] = value
    eval(name, context)
  except:
    details = "'%s=%s' is not a valid python statement" % (name, value)
    sys.exit("ERROR[gyp_trunk.py]: " + details)


def determine_variables(gyp_env_vars):
  if not gyp_env_vars.has_key('OS'):
    gyp_env_vars['OS'] = 'linux'

  try:
    # dectect version of build machine.
    import lsb_release
    gyp_env_vars['is_xenial'] = lsb_release.get_lsb_information()['RELEASE'] == '16.04'
  except:
    gyp_env_vars['is_xenial'] = False
    print 'Can not import lsb_release.';

  config = defaults
  config.update(gyp_env_vars)

  # globals dictionary for evaluating conditions.
  eval_globals = config
  eval_globals['__builtins__'] = None

  for item in conditions:
    if len(item) != 2 and len(item) != 3:
      sys.exit('ERROR[gyp_trunk.py]: ill-format conditional block\n ' +
          str(item))

    cond = item[0]
    variables = None
    if eval(cond, eval_globals):
      variables = item[1]
    elif len(item) == 3:
      variables = item[2]

    if variables:
      # Update variables from conditional block not overriding definitions from
      # environment and command line. A la |'var%': value| in gyp.
      for var, value in variables.items():
        check_assignment(var, value)
        if not gyp_env_vars.has_key(var):
          config[var] = value

  return config


def overwrite_arguments():
  gyp_environment.SetEnvironment()

  # GetGypVars reads command line, GYP_DEFINES and ~/.gyp/include.gypi.
  gyp_env_variables = gyp_chromium.GetGypVars([])

  # GetGypVars stringifies the values. Fix it up so we can use the relaxed
  # syntax in conditions.
  tmp = {}
  for k,v in gyp_env_variables.items():
    try:
      v_tmp = int(v)
    except ValueError:
      v_tmp = v
    tmp[k] = v_tmp

  gyp_env_variables = tmp

  variables = determine_variables(gyp_env_variables)

  forwarded_args = [sys.argv[0]]
  for v in variables:
    forwarded_args.append('-D' + str(v) + '=' + str(variables[v]))

  # Filter out definitions from command line not to duplicate them.
  next_is_var = False
  for arg in sys.argv[1:]:
    # gyp accepts both -Dvar=value and -D var=value.
    if next_is_var:
      next_is_var = False
      continue
    if arg.startswith('-D'):
      if arg == '-D':
        next_is_var = True
      continue
    forwarded_args.append(arg)

  sys.argv = forwarded_args

  # Check for non-supported configurations
  if ('-Denable_desktop_chrome=1' in sys.argv[1:]):
   if ('-Denable_s_android_browser=1' in sys.argv[1:]):
    print 'The enable_desktop_chrome and enable_s_android_browser ' \
          'flags can not be set together!';
    sys.exit();

  print '---- gyp variables ----'
  gyp_file_specified = (os.path.splitext(sys.argv[-1])[1] == '.gyp')
  print ' '.join(sys.argv[1:-1] if gyp_file_specified else sys.argv[1:])
  print '----\n'


if __name__ == '__main__':
  overwrite_arguments()
  execfile(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'gyp_chromium.py'))
