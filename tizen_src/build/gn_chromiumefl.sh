#!/bin/bash

source $(dirname $0)/common.sh
trap 'error_report $0 $LINENO' ERR SIGINT SIGTERM SIGQUIT
host_arch=$(getHostArch)
export HOST_ARCH=$host_arch

if [ -z "$GN_GENERATOR_OUTPUT" ]; then
  GN_GENERATOR_OUTPUT="out.${host_arch}"
fi

echo ${host_arch}
echo $GN_GENERATOR_OUTPUT

if [ -z "$GN_GENERATOR_FLAG" ]; then
  export GN_GENERATOR_FLAG=$GN_GENERATOR_OUTPUT
fi

EXTRA_GN_ARGS="$@"

target=$GN_CHROMIUMEFL_TARGET

if [ "$GN_CHROMIUMEFL_TARGET" == "tizen" ]; then
  buildType="gbs"
elif [ "$GN_CHROMIUMEFL_TARGET" == "crosscompile" ]; then
  buildType="crosscompile"
  EXTRA_GN_ARGS+=" edje_compiler=${TOPDIR}/out.${host_arch}/Dependencies/Root/bin/edje_cc"
fi

while [[ $# > 0 ]]; do
  case "$1" in
    tizen_version=*)
      tizen_version=$(echo $1 | sed -e 's#tizen_version\=\([0-9.]*\)#\1#')
      ;;
    tizen_emulator_support=1)
      tizen_emulator_support=1
      ;;
    tizen_product_da=true)
      tizen_product_da=true
      ;;
    tizen_product_tv=true)
      tizen_product_tv=true
      ;;
    enable_nacl=true)
      enable_nacl=true
      ;;
    #use_wayland=true)
    #  use_wayland=true
    #  ;;
  esac
  shift;
done

findElementInArray "$target" "${supported_targets[@]}"
if [[ $? == 1 ]]; then
  echo "Unsupported target: $target"
  exit 1
fi

if [ "$DCHECK_ALWAYS_ON" == "1" ]; then
  EXTRA_GN_ARGS+=" dcheck_always_on=true"
fi

ORIGINAL_GN_DEFINES="$GN_DEFINES"
export GN_DEFINES=$(echo "$GN_DEFINES" | sed -e 's/component\s*=\s*shared_library//g')
if [ "$ORIGINAL_GN_DEFINES" != "$GN_DEFINES" ]; then
    echo "WARNING: component build is not supported."
    echo "Removing component=shared_library from GN_DEFINES."
fi

USE_UDEV=false
if [ "$tizen_product_tv" == "true" ]; then
  USE_UDEV=true
fi

COMMON_GN_PARAMETERS="
                      ewk_bringup=true
                      use_libjpeg_turbo=true
                      proprietary_codecs=true
                      rtc_build_openmax_dl=false
                      rtc_use_openmax_dl=false
                      use_alsa=false
                      use_aura=false
                      use_gconf=false
                      use_kerberos=false
                      use_openmax_dl_fft=false
                      use_ozone=true
                      use_system_harfbuzz=true
                      ozone_auto_platforms=false
                      ozone_platform_wayland=false
                      is_desktop_linux=false
                      use_pango=false
                      use_cairo=false
                      enable_pdf=false
                      enable_basic_printing=false
                      enable_print_preview=false
                      use_cups=false
                      use_allocator=\"none\"
                      depth=\"${TOPDIR}\"
                      use_libpci=false
                     "
                      # Moved it in //tizen_src/build/config/BUILDCONFIG.gn
                      # use_efl=true
                      # declared also from build config
                      # is_desktop_linux=false
                      # declared also from //build/config/features.gni
                      # proprietary_codecs=true
                      # use_gconf=false
                      # enable_plugins=true
if [ "$tizen_product_tv" == "true" ]; then
  COMMON_GN_PARAMETERS+="
                         use_udev=${USE_UDEV}
                        "
fi

SYSTEM_DEPS="--system-libraries
             expat
             harfbuzz-ng
             libpng
             libxml
             libxslt
             zlib
            "

add_desktop_flags() {
  is_clang="false"
  if [ "$USECLANG" == "1" ]; then
    is_clang="true"
  fi
  SYSTEM_DEPS+="libevent
               "
  ADDITIONAL_GN_PARAMETERS+="enable_plugins=true
                             is_clang=${is_clang}
                             is_cfi=${is_clang}
                             werror=false
                             use_sysroot=false
                             use_system_libevent=true
                             use_wayland=false
                             use_plugin_placeholder_hole=true
                             target_os="\"linux\""
                             target_cpu=\"${host_arch}\"
                            "
}

add_arm_flags() {
  ADDITIONAL_GN_PARAMETERS+="arm_thumb=true
                             arm_use_neon=true
                             deps_include_path=\"/usr/include\"
                            "

  if [ "$host_arch" == "arm64" ]; then
    ADDITIONAL_GN_PARAMETERS+="system_libdir=\"lib64\"
                               deps_lib_path=\"/usr/lib64\"
                              "
  elif [ "$host_arch" == "arm" ]; then
    ADDITIONAL_GN_PARAMETERS+="system_libdir=\"lib\"
                               deps_lib_path=\"/usr/lib\"
                              "
  fi
}

add_emulator_flags() {
  if [ "$host_arch" == "x64" ]; then
    ADDITIONAL_GN_PARAMETERS+="system_libdir=\"lib64\"
                               deps_lib_path=\"/usr/lib64\"
                              "
  fi
}

add_tizen_flags() {
  ADDITIONAL_GN_PARAMETERS+="python_ver=\"$(getPythonVersion)\"
                             icu_use_data_file=true
                             is_clang=false
                             is_official_build=true
                             linux_use_bundled_binutils=false
                             enable_nacl=false
                             enable_basic_printing=true
                             enable_print_preview=true
                             target_sysroot=\"/\"
                             tizen_multimedia_eme_support=false
                             target_os="\"tizen\""
                             current_cpu=\"${host_arch}\"
                             host_cpu=\"${host_arch}\"
                             target_cpu=\"${host_arch}\"
                             host_toolchain=\"//tizen_src/build/toolchain/tizen:tizen_$host_arch\"
                             v8_snapshot_toolchain=\"//tizen_src/build/toolchain/tizen:tizen_$host_arch\"
                            "

                              # We do not need it
                              # tizen=true
                              # not used any place
                              # linux_use_bundled_gold=false
                              # declared also from //build/config/features.gni
                              #enable_basic_printing=true
                              #enable_print_preview=true
                              #It is defined from //build/toolchain/gcc_toolchain.gni
                              #is_clang=false

  # FIXME : Note that the v8_snapshot_toolchain has been set to wrong
  # toolchain clang in x64 build even is_clang is false.
  # This sets it to toolchain gcc forcibly as workaround.
  # It needs to be set to the toolchain gcc under is_clang is false.
  if [ "$host_arch" == "x64" ]; then
    ADDITIONAL_GN_PARAMETERS+="v8_snapshot_toolchain=\"//tizen_src/build/toolchain/tizen:tizen_x64\"
                              "
  fi

  ADDITIONAL_GN_PARAMETERS+="linux_use_gold_flags=true
                            "
  ADDITIONAL_GN_PARAMETERS+="direct_canvas=true
                            "

  # [M49_2623] Temporary disabling the flag.
  #            FIXME: http://165.213.149.170/jira/browse/TWF-610
  ADDITIONAL_GN_PARAMETERS+="tizen_multimedia_support=true
                             tizen_video_hole=true
                             tizen_tbm_support=true
                            "
  if (( $tizen_version >= 50 )); then
    SYSTEM_DEPS+="libjpeg
                 "
  fi
  if [ "$tizen_product_tv" == "true" ]; then
    ADDITIONAL_GN_PARAMETERS+="tizen_pepper_extensions=true
                               tizen_pepper_player=true
                               use_plugin_placeholder_hole=true
                               tizen_vd_native_scrollbars=true
                              "
    if [ "$tizen_vd_lfd" == "true" ]; then
      ADDITIONAL_GN_PARAMETERS+="tizen_vd_lfd=true
                                "
    fi
  else
    SYSTEM_DEPS+="libevent
                 "
    if (( $tizen_version >= 50 )); then
      SYSTEM_DEPS+="icu
                   "
    fi
    ADDITIONAL_GN_PARAMETERS+="use_system_libevent=true
                              "
  fi

  if [ "$enable_nacl" == "true" ]; then
    ADDITIONAL_GN_PARAMETERS+="ffmpeg_branding=\"Chrome\""
  fi

  # use_wayland come from spec file and based on that wayland_bringup sets in tizen_features.gni
  #add_wayland_flags
}

add_wayland_flags() {
  if [ "$use_wayland" == "true" ]; then
    ADDITIONAL_GN_PARAMETERS+="use_wayland=true
                               wayland_bringup=true
                              "
  else
    ADDITIONAL_GN_PARAMETERS+="use_wayland=false
                               wayland_bringup=false
                              "
  fi
}

add_gbs_flags() {
  # To reduce binary size, O2 is only applied to major components
  # and Os is applied to the others.
  local lto_level="s"

  if [[ $lto_level =~ ^[0-9s]$ ]]; then
    ADDITIONAL_GN_PARAMETERS+="lto_level=\"$lto_level\"
                              "
  else
    ADDITIONAL_GN_PARAMETERS+="lto_level=\"s\"
                              "
  fi
  # TODO(youngsoo):
  # Due to large file size issue of libchromium-ewk.so,
  # The symbol level is set to 1 by default.
  # Once the issue is fixed, set it to the level from platform.
  local symbol_level=$(echo " $CFLAGS" | sed -e 's#.* -g\([0-9]\).*#\1#')
  if [ "$symbol_level" != "0" ]; then
    symbol_level="1"
  fi

  if [[ $symbol_level =~ ^[0-9]$ ]]; then
    ADDITIONAL_GN_PARAMETERS+="symbol_level=$symbol_level
                              "
  else
    ADDITIONAL_GN_PARAMETERS+="symbol_level=1
                              "
  fi
  # target_arch changed to target_cpu but not changed because it seems that
  # it is used for gbs and added target_cpu also.
  ADDITIONAL_GN_PARAMETERS+="target_cpu=\"${host_arch}\"
                            "
                            #  sysroot=""
                            #"
                              #no-parallel=true
                             #"
  # TODO(b.kelemen): ideally crosscompile should also support system libs.
  # Unfortunately the gbs root doesn't contain everything chromium needs.
#  SYSTEM_DEPS="-Duse_system_expat=true
#               -Duse_system_libjpeg=false
#               -Duse_system_libpng=true
#               -Duse_system_libusb=true
#               -Duse_system_libxml=true
#               -Duse_system_libxslt=true
#               -Duse_system_re2=true
#               -Duse_system_zlib=true
#              "

  # [M50_2661] Temporary using the icu of internal chformium instead of system.
  #            The icu of system doesn't support utrie2.h
  #            FIXME: http://suprem.sec.samsung.net/jira/browse/TWF-967
#  SYSTEM_DEPS+="-Duse_system_icu=false
#               "

  if [ "$target" == "mobile" ]; then
#    SYSTEM_DEPS+="-Duse_system_bzip2=true
#                  -Duse_system_libexif=true
#                  -Duse_system_nspr=true
#                 "
    SYSTEM_DEPS+="
                 "
  fi
}

add_cross_flags() {
  # target_arch changed to target_cpu but not changed because it seems that
  # it is used for gbs and added target_cpu also.
  ADDITIONAL_GN_PARAMETERS+="target_cpu=\"arm\"
                             sysroot=\"$SYSROOTDIR\"
                             arm_tune=\"arm7\"
                             "

  # Compiling yasm with crosscompile + icecc leads to some strange errors (one file is built for target instead of host).
  # Yasm is an assembler used only by the build (not at runtime) and it is generally available in Linux distros so let's just
  # use it from the system.
  ADDITIONAL_GN_PARAMETERS+="use_system_yasm=true"
}

if [ "$target" == "desktop" ]; then
  add_desktop_flags
else
  add_tizen_flags
  if [ "$host_arch" == "arm" -o "$host_arch" == "arm64" ]; then
    add_arm_flags
  elif [ "$host_arch" == "x86" -o "$host_arch" == "x64" ]; then
    add_emulator_flags
  fi
  if [ "$buildType" == "gbs" ]; then
    add_gbs_flags
  elif [ "$buildType" == "crosscompile" ]; then
    add_cross_flags
  fi
fi

if [ "$SYSTEM_DEPS" != "" ]; then
  echo "** use system lib : replace **"
  #replacing original files with correct ones according to $SYSTEM_DEPS
  $TOPDIR/build/linux/unbundle/replace_gn_files.py $SYSTEM_DEPS
fi

_GN_ARGS="
    gen
    $GN_GENERATOR_FLAG
    --root=${TOPDIR}
    --dotfile=${TOPDIR}/tizen_src/.gn
    --args=
    $COMMON_GN_PARAMETERS
    $ADDITIONAL_GN_PARAMETERS
    $EXTRA_GN_ARGS
    "
printf "GN_ARGUMENTS:\n"
for arg in $_GN_ARGS; do
  printf "    * ${arg##-D}\n"
done
for arg in $SYSTEM_DEPS; do
  printf "    * ${arg##-D}\n"
done

if [ -f "${TOPDIR}/BUILD_.gn" ]; then
  rm "${TOPDIR}/BUILD.gn"
else
  mv "${TOPDIR}/BUILD.gn" "${TOPDIR}/BUILD_.gn"
fi
ln -s "${TOPDIR}/tizen_src/BUILD.gn" "${TOPDIR}/BUILD.gn"

${TOPDIR}/tizen_src/build/gn_chromiumefl \
    ${_GN_ARGS}

ret=$?

mv ${TOPDIR}/BUILD_.gn ${TOPDIR}/BUILD.gn

if [ "$SYSTEM_DEPS" != "" ]; then
  echo "** use system lib : undo **"
  # Restore gn files to their original states not to mess up the tree permanently.
  $TOPDIR/build/linux/unbundle/replace_gn_files.py --undo $SYSTEM_DEPS
fi

exit $ret
