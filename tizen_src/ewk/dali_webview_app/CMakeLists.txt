# Copyright (c) 2013 Samsung Electronics Co., Ltd All Rights Reserved
#
#    Licensed under the Apache License, Version 2.0 (the "License");
#    you may not use this file except in compliance with the License.
#    You may obtain a copy of the License at
#
#        http://www.apache.org/licenses/LICENSE-2.0
#
#    Unless required by applicable law or agreed to in writing, software
#    distributed under the License is distributed on an "AS IS" BASIS,
#    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#    See the License for the specific language governing permissions and
#    limitations under the License.
#

CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT("dali_webview" CXX)


IF(NOT DEFINED PACKAGE_NAME)
        SET(PACKAGE_NAME "org.tizen.dali_webview")
ENDIF(NOT DEFINED PACKAGE_NAME)

SET(PREFIX ${CMAKE_INSTALL_PREFIX})
IF(NOT DEFINED BINDIR)
        SET(BINDIR"${PREFIX}/bin")
ENDIF(NOT DEFINED BINDIR)


SET(SRCS src/main.cpp)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/include)
ADD_DEFINITIONS("-DPACKAGE=\"${PACKAGE_NAME}\"")

INCLUDE(FindPkgConfig)
pkg_check_modules(PKGS REQUIRED
                elementary
                capi-appfw-application
                libtbm
                dali-core
                dali-adaptor
                dali-toolkit
                chromium-efl
                )

FOREACH(flag ${PKGS_CFLAGS})
        SET(EXTRA_CXXFLGAS "${EXTRA_CXXFLGAS} ${flag}")
ENDFOREACH(flag)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${EXTRA_CXXFLGAS} -fPIE")
SET(CMAKE_EXE_LINKER_FLAGS "-pie -Wl,--as-needed")
ADD_EXECUTABLE(${PROJECT_NAME} ${SRCS})
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${PKGS_LDFLAGS})

INSTALL(TARGETS ${PROJECT_NAME} DESTINATION ${BINDIR})
