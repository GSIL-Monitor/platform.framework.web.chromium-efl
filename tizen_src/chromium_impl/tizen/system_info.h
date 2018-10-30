// Copyright (c) 2016 Samsung Electronics Co., Ltd All Rights Reserved
//
//    Licensed under the Apache License, Version 2.0 (the "License");
//    you may not use this file except in compliance with the License.
//    You may obtain a copy of the License at
//
//        http://www.apache.org/licenses/LICENSE-2.0
//
//    Unless required by applicable law or agreed to in writing, software
//    distributed under the License is distributed on an "AS IS" BASIS,
//    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//    See the License for the specific language governing permissions and
//    limitations under the License.

#ifndef _PROFILE_INFO_H_
#define _PROFILE_INFO_H_

typedef enum {
    PROFILE_UNKNOWN  = 1 << 1,
    PROFILE_DESKTOP  = 1 << 2,
    PROFILE_MOBILE   = 1 << 3,
    PROFILE_WEARABLE = 1 << 4,
    PROFILE_TV       = 1 << 5,
    PROFILE_IVI      = 1 << 6,
    PROFILE_COMMON   = 1 << 7
} Profile_Inform;

typedef enum {
    ARCH_UNKNOWN = 1 << 1,
    ARCH_ARMV7   = 1 << 2,
    ARCH_AARCH64 = 1 << 3,
    ARCH_X86     = 1 << 4,
    ARCH_X86_64  = 1 << 5
} Arch_Inform;

#ifdef __cplusplus
extern "C" {
#endif

extern volatile Profile_Inform g_profile__;
extern volatile Arch_Inform g_arch__;

void GetProfile();
int IsDesktopProfile();
int IsMobileProfile();
int IsWearableProfile();
int IsTvProfile();
int IsIviProfile();
int IsCommonProfile();

void GetArch();
int IsEmulatorArch();

#ifdef __cplusplus
}
#endif


#define GET_PROFILE() ({ \
    if(g_profile__ == PROFILE_UNKNOWN) \
        GetProfile(); \
    g_profile__; \
})

#define GET_ARCH() ({ \
    if(g_arch__ == ARCH_UNKNOWN) \
        GetArch(); \
    g_arch__; \
})

#define IS_PROFILE(PROFILE)     GET_PROFILE() == PROFILE_##PROFILE ? 1: 0

/*
CURRENT PROFILE CAN BE GET WITH ABOVE MACRO FUCTION LIKE AS BELOW

cout << "GET CURRENT PROFILE:" << GETPROFILE() <<endl;
if(IS_PROFILE(MOBILE)) cout << "MOBILE PROFILE" <<endl;
if(IS_PROFILE(DESKTOP)) cout << "DESKTOP PROFILE" <<endl;
if(IS_PROFILE(WEARABLE)) cout << "WEARABLE PROFILE" <<endl;
if(IS_PROFILE(TV)) cout << "TV PROFILE" <<endl;
if(IS_PROFILE(IVI)) cout << "TV PROFILE" <<endl;
if(IS_PROFILE(COMMON)) cout << "TV PROFILE" <<endl;
*/

#endif // _PLATFORM_INFO_H_
