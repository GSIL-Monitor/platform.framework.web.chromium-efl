// Copyright (c) 2015 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <cstdlib>
#include <string.h>
#include "system_info.h"

#if defined(OS_TIZEN)
#include <system_info.h>
#endif


volatile Profile_Inform g_profile__ = PROFILE_UNKNOWN;
volatile Arch_Inform g_arch__ = ARCH_UNKNOWN;

void GetProfile(void) {
  if (g_profile__ != PROFILE_UNKNOWN)
    return;

#if defined(OS_TIZEN)
  char *profileName;
  system_info_get_platform_string("http://tizen.org/feature/profile", &profileName);
  switch (*profileName) {
  case 'm':
  case 'M':
    g_profile__ = PROFILE_MOBILE;
    break;
  case 'w':
  case 'W':
    g_profile__ = PROFILE_WEARABLE;
    break;
  case 't':
  case 'T':
    g_profile__ = PROFILE_TV;
    break;
  case 'i':
  case 'I':
    g_profile__ = PROFILE_IVI;
    break;
  default: // common or unknown ==> ALL ARE COMMON.
    g_profile__ = PROFILE_COMMON;
  }
  free(profileName);
#else // for desktop profile
  g_profile__ = PROFILE_DESKTOP;
#endif

}

int IsDesktopProfile(void) {
  return GET_PROFILE() == PROFILE_DESKTOP ? 1 : 0;
}

int IsMobileProfile(void) {
  return GET_PROFILE() == PROFILE_MOBILE ? 1 : 0;
}

int IsWearableProfile(void) {
  return GET_PROFILE() == PROFILE_WEARABLE ? 1 : 0;
}

int IsTvProfile(void) {
  return GET_PROFILE() == PROFILE_TV ? 1 : 0;
}

int IsIviProfile(void) {
  return GET_PROFILE() == PROFILE_IVI ? 1 : 0;
}

int IsCommonProfile(void) {
  return GET_PROFILE() == PROFILE_COMMON ? 1 : 0;
}

void GetArch(void) {
#if defined(OS_TIZEN)
  if (g_arch__ != ARCH_UNKNOWN)
    return;

  char *archName;
  system_info_get_platform_string("http://tizen.org/feature/platform.core.cpu.arch", &archName);

  int archNamelen = strlen(archName);
  if (strncmp(archName, "armv7", archNamelen) == 0) {
    g_arch__ = ARCH_ARMV7;
  }
  else if (strncmp(archName, "aarch64", archNamelen) == 0) {
    g_arch__ = ARCH_AARCH64;
  }
  else if (strncmp(archName, "x86", archNamelen) == 0) {
    g_arch__ = ARCH_X86;
  }
  else if (strncmp(archName, "x86_64", archNamelen) == 0) {
    g_arch__ = ARCH_X86_64;
  }
  else {
    g_arch__ = ARCH_UNKNOWN;
  }
  free(archName);
#endif
}

int IsEmulatorArch(void) {
  return GET_ARCH() == ARCH_X86 ? 1 : GET_ARCH() == ARCH_X86_64 ? 1 : 0;
}

