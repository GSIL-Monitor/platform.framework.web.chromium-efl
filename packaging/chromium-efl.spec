Name: chromium-efl
Summary: Chromium EFL
# Set by by scripts/update-chromium-version.sh
%define ChromiumVersion 63.3239.0
%define Week 1
Version: %{ChromiumVersion}.%{Week}
Release: 1
# The 'Group' should be specified as one of the following valid group list.
# https://wiki.tizen.org/wiki/Packaging/Guidelines#Group_Tag
Group: Web Framework/Web Engine
# The 'License' should be specified as some of the following known license list.
# http://spdx.org/licenses/
License: LGPL-2.1 or BSD-2-Clause

Source0: %{name}-%{version}.tar.bz2
Source1: content_shell.in

%ifarch i586 i686
%define _nodebug 1
%endif

%if "%{?_nodebug}" == "1" && "%{?asan}" != "1"
%global __debug_install_post %{nil}
%global debug_package %{nil}
%endif

%if 0%{?_enable_unittests}
%define _debug_mode 1
%endif

# Do not use the hardcoded CPU numbers such as "jobs 80" or "_smp_mflags -j80"
# . The below statement is compatible with RPM 4.4.2.1+.
# . /usr/bin/getconf depends on the libc-common (or libc-bin) base package.
%if 0%{?_use_customized_job}
%define _smp_mflags -j%(echo "`/usr/bin/getconf _NPROCESSORS_ONLN`")
%endif

%if 0%{?_use_customized_job_for_gbs}
%define _smp_mflags -j%(echo $((`/usr/bin/getconf _NPROCESSORS_ONLN` / 2)))
%endif

%define tizen_version %{tizen_version_major}%{tizen_version_minor}

%if "%{?_with_da_profile}" != "1" && "%{?tizen_profile_name}" != "tv"
%define _tizen_public_target 1
%endif

# Enable DLP
%define TIZEN_DLP 1

Requires: /usr/bin/systemctl
Requires(post): /sbin/ldconfig
Requires(post): xkeyboard-config
Requires(postun): /sbin/ldconfig

BuildRequires: binutils-gold
BuildRequires: at-spi2-atk-devel, bison, edje-tools, expat-devel, flex, gettext, gperf, libatk-bridge-2_0-0, libcap-devel
BuildRequires: libjpeg-turbo-devel, ninja, pbzip2, perl, python, python-xml, which
BuildRequires: pkgconfig(atk)
BuildRequires: pkgconfig(capi-appfw-application)
BuildRequires: pkgconfig(capi-appfw-app-manager)
BuildRequires: pkgconfig(capi-location-manager)
BuildRequires: pkgconfig(capi-media-audio-io)
BuildRequires: pkgconfig(capi-media-camera)
BuildRequires: pkgconfig(capi-media-player)
BuildRequires: pkgconfig(capi-media-tool)
BuildRequires: pkgconfig(capi-system-device)
BuildRequires: pkgconfig(capi-system-info)
BuildRequires: pkgconfig(capi-system-sensor)
BuildRequires: pkgconfig(capi-system-system-settings)
BuildRequires: pkgconfig(dlog)
BuildRequires: pkgconfig(ecore)
BuildRequires: pkgconfig(ecore-evas)
BuildRequires: pkgconfig(ecore-imf)
BuildRequires: pkgconfig(ecore-imf-evas)
BuildRequires: pkgconfig(ecore-input)
BuildRequires: pkgconfig(eldbus)
BuildRequires: pkgconfig(efl-extension)
BuildRequires: pkgconfig(elementary)
BuildRequires: pkgconfig(evas)
BuildRequires: pkgconfig(feedback)
BuildRequires: pkgconfig(fontconfig)
BuildRequires: pkgconfig(freetype2)
BuildRequires: pkgconfig(gles20)
BuildRequires: pkgconfig(glib-2.0)
BuildRequires: pkgconfig(gstreamer-1.0)
BuildRequires: pkgconfig(gstreamer-app-1.0)
BuildRequires: pkgconfig(gstreamer-plugins-base-1.0)
BuildRequires: pkgconfig(libdrm)
BuildRequires: pkgconfig(libexif)
BuildRequires: pkgconfig(libffi)
BuildRequires: pkgconfig(libpng)
BuildRequires: pkgconfig(libpulse)
BuildRequires: pkgconfig(libtbm)
BuildRequires: pkgconfig(libtzplatform-config)
BuildRequires: pkgconfig(libudev)
BuildRequires: pkgconfig(libusb-1.0)
BuildRequires: pkgconfig(libxml-2.0)
BuildRequires: pkgconfig(libxslt)
BuildRequires: pkgconfig(minizip)
BuildRequires: pkgconfig(mm-player)
BuildRequires: pkgconfig(notification)
BuildRequires: pkgconfig(nspr)
BuildRequires: pkgconfig(nss)
BuildRequires: pkgconfig(openssl)
BuildRequires: pkgconfig(pkgmgr-info)
BuildRequires: pkgconfig(scim)
BuildRequires: pkgconfig(security-manager)
BuildRequires: pkgconfig(sqlite3)
BuildRequires: pkgconfig(ttrace)
BuildRequires: pkgconfig(tts)
BuildRequires: pkgconfig(stt)
BuildRequires: pkgconfig(ui-gadget-1)
BuildRequires: pkgconfig(vconf)
BuildRequires: pkgconfig(zlib)
%if %{tizen_version} >= 50 && "%{_tizen_public_target}" == "1"
BuildRequires: pkgconfig(icu-i18n)
%endif

%if "%{?tizen_profile_name}" != "tv"
BuildRequires: pkgconfig(capi-media-codec)
%endif

# SW video decoder support
%if "%{?tizen_profile_name}" == "tv"
BuildRequires: pkgconfig(libavcodec)
BuildRequires: pkgconfig(libavformat)
BuildRequires: pkgconfig(libavutil)
%define __enable_tizen_libav true
%else
%define __enable_tizen_libav false
%endif

# Applied python acceleration for generating gyp files.
%ifarch armv7l
BuildRequires: python-accel-armv7l-cross-arm
%endif
%ifarch aarch64
BuildRequires: python-accel-aarch64-cross-aarch64
%endif

%if "%{?_with_wayland}" == "1"
%if %{tizen_version} >= 50
BuildRequires: pkgconfig(ecore-wl2)
%else
BuildRequires: pkgconfig(ecore-wayland)
%endif
BuildRequires: pkgconfig(wayland-egl)
%else
BuildRequires: pkgconfig(ecore-x)
BuildRequires: pkgconfig(libdri2)
BuildRequires: pkgconfig(x11)
BuildRequires: pkgconfig(xcomposite)
BuildRequires: pkgconfig(xcursor)
BuildRequires: pkgconfig(xext)
BuildRequires: pkgconfig(xi)
BuildRequires: pkgconfig(xrandr)
BuildRequires: pkgconfig(xt)
BuildRequires: pkgconfig(xfixes)
BuildRequires: pkgconfig(xtst)
%endif

%if "%{?tizen_profile_name}" == "tv"
BuildRequires: pkgconfig(vd-win-util)
BuildRequires: pkgconfig(psa-common)
BuildRequires: pkgconfig(drmdecrypt)
BuildRequires: pkgconfig(marlin)
BuildRequires: pkgconfig(WebProduct)
BuildRequires: pkgconfig(capi-appfw-app-manager)
BuildRequires: pkgconfig(capi-media-sound-manager)
BuildRequires: pkgconfig(cynara-client)
BuildRequires: pkgconfig(security-privilege-manager)
BuildRequires: pkgconfig(WebProduct)
BuildRequires: pkgconfig(capi-stt-wrapper-tv)
%if "%{_vd_cfg_product_type}" != "AUDIO"
BuildRequires: pkgconfig(libvr360)
BuildRequires: pkgconfig(soc-pq-interface)
%endif
%else
BuildRequires: pkgconfig(capi-privacy-privilege-manager)
BuildRequires: pkgconfig(libevent)
BuildRequires: pkgconfig(push)
%endif

%if 0%{?_enable_nacl:0}
%define __enable_nacl %{_enable_nacl}
%else
%if "%{?tizen_profile_name}" == "tv"
%define __enable_nacl 1
%else
%define __enable_nacl 0
%endif
%endif

%if %{__enable_nacl} == 1
BuildRequires: nacl-toolchain-glibc
BuildRequires: nacl-toolchain-pnacl-newlib
%ifarch i586 i686
BuildRequires:  glibc-64bit
BuildRequires:  libgcc-64bit
BuildRequires:  zlib-64bit
BuildRequires:  nacl-toolchain-third_party
BuildRequires:  libstdc++-64bit
%endif
%ifarch armv7l
BuildRequires: nacl-toolchain-symlink
%endif
%endif

# Texture based MFC video decoder (FHD)
%if "%{?tizen_profile_name}" == "tv" && "%{?_with_emulator}" != "1" && "%{_vd_cfg_product_type}" != "AUDIO" &&  "%{_vd_cfg_product_type}" !="AV"
%define __enable_mfc_texture_video_dec true
%{!?__enable_omx_video_dec: %define __enable_omx_video_dec true}
%else
%define __enable_mfc_texture_video_dec false
%endif

# Samsung OMX decoder base
%{!?__enable_omx_video_dec: %define __enable_omx_video_dec false}
%if "%{?__enable_omx_video_dec}" == "true"
BuildRequires: vd_kernel-interfaces
BuildRequires: pkgconfig(omxil-e4x12-v4l2)
%{!?__enable_tizen_resource_manager: %define __enable_tizen_resource_manager true}
%endif

# Tizen Resource Manager
%{!?__enable_tizen_resource_manager: %define __enable_tizen_resource_manager false}
%if "%{?__enable_tizen_resource_manager}" == "true"
BuildRequires: pkgconfig(tv-resource-manager)
BuildRequires: pkgconfig(tv-resource-information)
%endif

%if "%{?tizen_profile_name}" == "tv" && "%{?_with_emulator}" != "1" && "%{_vd_cfg_product_type}" != "AUDIO" &&  "%{_vd_cfg_product_type}" !="AV" && "%{_vd_cfg_product_type}" != "LFD" && "%{_vd_cfg_product_type}" != "HTV"
BuildRequires: pkgconfig(sm-player)
%define __enable_tizen_pepper_player_d2tv 1
%else
%define __enable_tizen_pepper_player_d2tv 0
%endif

# KANTSU doesn't support H.264 UHD 60fps playback
%if "%{?tizen_profile_name}" == "tv" && "%{?_vd_cfg_panel_resolution}" == "UHD" && "%{?_vd_cfg_chip}" == "CHIP_KANTSU"
%define __enable_tizen_pepper_player_h264_uhd_only_30fps 1
%else
%define __enable_tizen_pepper_player_h264_uhd_only_30fps 0
%endif

%if "%{?tizen_profile_name}" == "tv" && "%{_vd_cfg_product_type}" != "AUDIO" &&  "%{_vd_cfg_product_type}" != "AV"
BuildRequires: pkgconfig(accessory)
%endif

%if "%{?tizen_profile_name}" == "tv" && "%{?_with_emulator}" != "1"
BuildRequires: pkgconfig(trustzone-nwd)
%define __enable_tizen_pepper_teec 1
%else
%define __enable_tizen_pepper_teec 0
%endif

%description
Browser Engine based on Chromium EFL (Shared Library)

%package devel
Summary: Chromium EFL
Group: Development/Libraries
Requires: %{name} = %{version}-%{release}
%description devel
Browser Engine dev library based on Chromium EFL (development files)

%if 0%{?_enable_content_shell}
%package shell
Summary: Chromium EFL port of content_shell application
Group: Development/Libraries
Requires: %{name} = %{version}-%{release}
%description shell
Chromium EFL version of content_shell application
%endif

%if 0%{?_enable_unittests}
%package unittests
Summary: Chromium unittests
Group: Development/Libraries
Requires: %{name} = %{version}-%{release}
%description unittests
Chromium unite tests
%endif

%if 0%{?build_ewk_unittests}
%package ewktest
Summary: Chromium EWK unittests
Group: Development/UnitTests
Requires: %{name} = %{version}-%{release}
%description ewktest
Chromium EFL unit test utilities
%endif

%if 0%{?build_tizen_ppapi_extension_unittests}
%package tizen_ppapi_extension_unittests
Summary: Chromium tizen ppapi extension unittests
Group: Development/UnitTests
Requires: %{name} = %{version}-%{release}
%description tizen_ppapi_extension_unittests
Chromium tizen ppapi extension unit test utilities
%endif
# The macros '%TZ_' are valid from tizen v3.0
%define _pkgid org.tizen.%{name}
%define _icondir %TZ_SYS_RO_APP/%{_pkgid}/shared/res
%define _xmldir %TZ_SYS_RO_PACKAGES

# Directory for internal chromium executable components
%global CHROMIUM_EXE_DIR %{_libdir}/%{name}
# Constant read only data used by chromium-efl port
%global CHROMIUM_DATA_DIR %{_datadir}/%{name}
# Chromium unit tests install directory
%global CHROMIUM_UNITTESTS_DIR /opt/usr/chromium-unittests/
# Directory containing localization files
%global CHROMIUM_LOCALE_DIR %{_datadir}/%{name}/locale

%prep
# Enabling parallelism to improve (de)compression time on multi-core systems.
# Caution: Keep the below contents' location in front of 'prep' section.
# Note that there three steps as following:
#
# 1st step: To compress git source in parallel whenever we run "gbs submit" to OBS.
# Please, contact an administrator of OBS/GBS server to append the below content.
# sudo apt-get (or zypper) install pbzip2
# sudo vi /etc/profile
# alias bzip2='/usr/bin/pbzip2'
# alias bunzip2='/usr/bin/pbunzip2'
# sudo mv /bin/bzip2 /bin/bzip2.disable
# sudo cp /usr/bin/pbzip2 /bin/bzip2
#
# 2nd step: To decompress a source file (e.g., tar.gz, tar.bz2) in parallel
# with rpmbuild command. Enable pbzip2 to support parallel decompression.
alias tar="tar --use-compress-program=pbzip2"
alias bzip2='pbzip2'
alias bunzip2='pbunzip2'

%if 0%{?use_pbzip2}
%define __bzip2 pbzip2
%endif
%setup -q

%build
%{?asan:/usr/bin/gcc-unforce-options}

# architecture related configuration + neon temporary workaround
%if %{?_skip_ninja:0}%{!?_skip_ninja:1}

%ifarch %{arm} aarch64
  export ADDITION_OPTION=" -finline-limit=64 -foptimize-sibling-calls -fno-unwind-tables -fno-exceptions -Os "

  export CFLAGS="$CFLAGS $ADDITION_OPTION"
  export CXXFLAGS="$CXXFLAGS $ADDITION_OPTION"
  export FFLAGS="$FFLAGS $ADDITION_OPTION"

  export CFLAGS="$(echo $CFLAGS | sed 's/-mfpu=[a-zA-Z0-9-]*/-mfpu=neon/g')"
  export CXXFLAGS="$(echo $CXXFLAGS | sed 's/-mfpu=[a-zA-Z0-9-]*/-mfpu=neon/g')"
  export FFLAGS="$(echo $FFLAGS | sed 's/-mfpu=[a-zA-Z0-9-]*/-mfpu=neon/g')"
%else
  export CFLAGS="$(echo $CFLAGS | sed 's/-Wl,--as-needed//g')"
  export CXXFLAGS="$(echo $CXXFLAGS | sed 's/-Wl,--as-needed//g')"
%endif

%if "%{?_nodebug}" == "1" || "%{?asan}" == "1"
  CFLAGS="$(echo $CFLAGS | sed -E 's/-g[0-9] /-g0 /g')"
  CXXFLAGS="$(echo $CXXFLAGS | sed -E 's/-g[0-9] /-g0 /g')"
%endif

%ifarch armv7l
%define ARCHITECTURE armv7l
%endif
%ifarch aarch64
%define ARCHITECTURE aarch64
%endif
%ifarch i586 i686
%define ARCHITECTURE ix86
%endif
%ifarch x86_64
%define ARCHITECTURE x86_64
%endif

# The "_repository" flag was changed to "_vd_cfg_target_repository" in tizen 4.0 product tv.
%if "%{?tizen_profile_name}" == "tv"
%define repo_name %{_vd_cfg_target_repository}
%else
%define repo_name %{_repository}
%endif

export GN_CHROMIUMEFL_TARGET=tizen

%define OUTPUT_BASE_FOLDER out.tz_v%{tizen_version_major}.%{tizen_version_minor}.%{repo_name}.%{ARCHITECTURE}

export GN_GENERATOR_OUTPUT=$PWD/%{OUTPUT_BASE_FOLDER}

#set build mode
%global OUTPUT_FOLDER %{OUTPUT_BASE_FOLDER}

if type ccache &> /dev/null; then
  source tizen_src/build/ccache_env.sh tizen
fi

%if %{__enable_nacl} == 1
  rm -rf %{_builddir}/%{name}-%{version}/native_client/toolchain/linux_x86
  mkdir -p %{_builddir}/%{name}-%{version}/native_client/toolchain/
  ln -s /opt/nacl_toolchain %{_builddir}/%{name}-%{version}/native_client/toolchain/linux_x86
  export LD_LIBRARY_PATH=/emul/usr/lib/:/emul/lib/:/emul/lib64/
%endif

%if 0%{!?_skip_gn:1}
#gn generate
#run standard gn_chromiumefl wrapper
./tizen_src/build/gn_chromiumefl.sh \
  "data_dir=\"%{CHROMIUM_DATA_DIR}\"" \
  "edje_dir=\"%{CHROMIUM_DATA_DIR}/themes\"" \
  "image_dir=\"%{CHROMIUM_DATA_DIR}/images\"" \
  "exe_dir=\"%{CHROMIUM_EXE_DIR}\"" \
  "tizen_version=%{tizen_version}" \
  "tizen_version_major=%{tizen_version_major}" \
  "tizen_version_minor=%{tizen_version_minor}" \
  "tizen_version_patch=%{tizen_version_patch}" \
%if 0%{?_remove_webcore_debug_symbols:1}
  "remove_webcore_debug_symbols=true" \
%endif
%if "%{?_with_wayland}" == "1"
  "use_wayland=true" \
%endif
%if "%{VD_PRODUCT_TYPE}" == "LFD"
  "tizen_vd_lfd=true" \
%endif
%if "%{?_repository}" == "emulator" || "%{?_repository}" == "emulator32-x11"
  "tizen_emulator_support=true"  \
%endif
%if "%{?_tizen_public_target}" == "1"
  "tizen_public_target=true" \
%endif
%if "%{?_with_da_profile}" == "1"
  "tizen_product_da=true" \
%endif
%if "%{?tizen_profile_name}" == "tv"
  "tizen_product_tv=true" \
%if "%{VD_PRODUCT_TYPE}" == "AUDIO"
  "tizen_vd_disable_gpu=true" \
%endif
%if "%{?tizen_profile_name}" == "tv" && "%{_vd_cfg_product_type}" != "AUDIO" &&  "%{_vd_cfg_product_type}" != "AV"
  "tizen_vd_accessory=true" \
%endif
%if "%{__enable_nacl}" == "1"
  "enable_nacl=true" \
%endif
%if "%{__enable_tizen_pepper_teec}" == "1"
  "tizen_pepper_teec=true" \
%endif
%if "%{__enable_tizen_pepper_player_d2tv}" == "1"
  "tizen_pepper_player_d2tv=true" \
%endif
%if "%{__enable_tizen_pepper_player_h264_uhd_only_30fps}" == "1"
  "tizen_pepper_player_h264_uhd_only_30fps=true" \
%endif
%endif
  "tizen_mfc_texture_video_decoder=%{__enable_mfc_texture_video_dec}"\
  "tizen_omx_video_decoder=%{__enable_omx_video_dec}"\
  "tizen_resource_manager=%{__enable_tizen_resource_manager}"\
%if "%{?_tizen_build_devel_mode}" == "1" && "%{?_tizen_public_target}" == "1"
  "use_symbolize=true" \
%endif
  "tizen_libav=%{__enable_tizen_libav}" \
%if "%{TIZEN_DLP}" == "1"
  "tizen_dlp=true" \
%endif
%if 0%{?component_build}
 "component=\"shared_library\"" \
%endif
%if "%{?dcheck_always_on}" == "1"
  "dcheck_always_on=true" \
%endif
%if "%{?enable_gcov}" == "1"
  "enable_gcov=true" \
%endif
%if "%{?_ttrace}" == "1" || "%{?tizen_profile_name}" == "tv"
  "use_ttrace=true" \
%endif
  %{?asan:"is_asan=true"}
%endif  # _skip_gn

ninja %{_smp_mflags} -C"%{OUTPUT_FOLDER}" \
%if 0%{?_enable_content_shell}
  content_shell \
%endif
%if 0%{?build_ewk_unittests}
  ewk_unittests \
%endif
%if 0%{?build_tizen_ppapi_extension_unittests}
  efl_integration_unittests \
  ppapi_unittests \
%endif
%if "%{__enable_nacl}" == "1"
  chrome_sandbox \
%endif
  efl_webprocess chromium-ewk efl_webview_app mini_browser ubrowser

%if 0%{?_enable_unittests}
ninja %{_smp_mflags} -C"%{OUTPUT_FOLDER}" angle_unittests env_chromium_unittests cacheinvalidation_unittests \
  angle_unittests env_chromium_unittests cacheinvalidation_unittests \
  url_unittests sandbox_linux_unittests crypto_unittests sql_unittests accessibility_unittests \
  gfx_unittests printing_unittests events_unittests ppapi_unittests jingle_unittests \
  flip_in_mem_edsm_server_unittests breakpad_unittests dbus_unittests libphonenumber_unittests \
  base_unittests ffmpeg_unittests gin_unittests net_unittests snapshot_unittests \
  google_apis_unittests
# TODO: Fix compilation of the following tests content_unittests cc_unittests shell_dialogs_unittests
# gpu_unittests compos9itor_unittests media_unittests
%endif

%endif  # _skip_ninja

#XXX icudtl.dat is not copied by gyp. Do that manually
cp third_party/icu/android/icudtl.dat "%{OUTPUT_FOLDER}"

# XXX Workaround for using rpmlint with emulator build on Tizen_TV 3.0
#
# When using this repo http://download.tizen.org/snapshots/tizen/tv/latest/repos/emulator32-x11/packages/
# rpmlint-tizen-1.0-6.1 is installed in GBS-ROOT. So, after rpms are built
# gbs build script runs rpmlint to test created packages. There is some BUG in
# this script and directory /home/abuild/rpmbuild/OTHER/ isn't created. This
# directory is required for rpmlint's log file and its lack causes build error.
if [ ! -d %{buildroot}/../../OTHER/ -a -f /opt/testing/bin/rpmlint ]; then
   mkdir -p %{buildroot}/../../OTHER/
fi

# Running unittests
%if 0%{?run_unittests_in_gbs}
%if 0%{?build_tizen_ppapi_extension_unittests}
%{OUTPUT_FOLDER}/efl_integration_unittests
%{OUTPUT_FOLDER}/ppapi_unittests
%endif
%endif

%install
# Let's replace eu-strip (elfutils) with strip (binutils) to fix
# stripping issue of *.so happened since m53.
# Caution: Please keep "STRIP_DEFAULT_PACKAGE" variable in 'install' section.
export STRIP_DEFAULT_PACKAGE="binutils"

# Generate pkg-confg file
mkdir -p "%{OUTPUT_FOLDER}"/pkgconfig/
sed -e 's#@CHROMIUM_VERSION@#%{version}#' tizen_src/build/pkgconfig/chromium-efl.pc.in \
    > "%{OUTPUT_FOLDER}"/pkgconfig/chromium-efl.pc

install -d "%{buildroot}"%{_bindir}
install -d "%{buildroot}"%{_libdir}/pkgconfig
install -d "%{buildroot}"%{_includedir}/chromium-ewk
install -d "%{buildroot}"%{_includedir}/v8
install -d "%{buildroot}%{CHROMIUM_EXE_DIR}"
install -d "%{buildroot}%{CHROMIUM_EXE_DIR}/locales"
install -d "%{buildroot}%{CHROMIUM_DATA_DIR}"/themes
install -d "%{buildroot}%{CHROMIUM_DATA_DIR}"/images

# locale for ewk layer
cp -r "%{OUTPUT_FOLDER}/locale" "%{buildroot}/%{CHROMIUM_LOCALE_DIR}"

# locale for chromium layer
install -m 0644 "%{OUTPUT_FOLDER}"/locales/efl/*.pak "%{buildroot}%{CHROMIUM_EXE_DIR}"/locales

install -m 0755 "%{OUTPUT_FOLDER}"/libchromium-ewk.so    "%{buildroot}"%{_libdir}

install -m 0755 "%{OUTPUT_FOLDER}"/efl_webprocess    "%{buildroot}%{CHROMIUM_EXE_DIR}"
install -m 0755 "%{OUTPUT_FOLDER}"/icudtl.dat        "%{buildroot}%{CHROMIUM_EXE_DIR}"
install -m 0755 "%{OUTPUT_FOLDER}"/natives_blob.bin  "%{buildroot}%{CHROMIUM_EXE_DIR}"
install -m 0755 "%{OUTPUT_FOLDER}"/snapshot_blob.bin "%{buildroot}%{CHROMIUM_EXE_DIR}"
install -m 0644 "%{OUTPUT_FOLDER}"/content_shell.pak "%{buildroot}%{CHROMIUM_EXE_DIR}"
install -m 0644 "%{OUTPUT_FOLDER}"/resources/*.edj   "%{buildroot}%{CHROMIUM_DATA_DIR}"/themes
install -m 0644 "%{OUTPUT_FOLDER}"/images/*.png   "%{buildroot}%{CHROMIUM_DATA_DIR}"/images

mkdir -p "%{buildroot}"/usr/apps/org.tizen.%{name}/bin
install -m 0755 "%{OUTPUT_FOLDER}"/efl_webview_app   "%{buildroot}"/usr/apps/org.tizen.%{name}/bin/
install -m 0755 "%{OUTPUT_FOLDER}"/mini_browser      "%{buildroot}"/usr/apps/org.tizen.%{name}/bin/
install -m 0755 "%{OUTPUT_FOLDER}"/ubrowser          "%{buildroot}"/usr/apps/org.tizen.%{name}/bin/

install -d "%{OUTPUT_FOLDER}"/packages
install -d %{buildroot}%{_xmldir}
sed -e 's#@TIZEN_VERSION@#%{tizen_version_major}.%{tizen_version_minor}#g' \
    -e 's#@CHROMIUM_VERSION@#%{version}#g' \
    -e 's#@TZ_SYS_RO_APP@#%{TZ_SYS_RO_APP}#g' \
    -e 's#@ICON_DIR@#%{_icondir}#g' \
    tizen_src/ewk/efl_webview_app/%{_pkgid}.xml.in \
    > %{OUTPUT_FOLDER}/packages/%{_pkgid}.xml
install -m 0644 %{OUTPUT_FOLDER}/packages/%{_pkgid}.xml           %{buildroot}%{_xmldir}

install -d %{buildroot}%{_icondir}
install -m 0644 tizen_src/ewk/ubrowser/ubrowser.png               %{buildroot}%{_icondir}
install -m 0644 tizen_src/ewk/efl_webview_app/mini_browser.png    %{buildroot}%{_icondir}
install -m 0644 tizen_src/ewk/efl_webview_app/efl_webview_app.png %{buildroot}%{_icondir}

install -m 0644 "%{OUTPUT_FOLDER}"/pkgconfig/*.pc    "%{buildroot}"%{_libdir}/pkgconfig/
install -m 0644 tizen_src/ewk/efl_integration/public/*.h                  "%{buildroot}"%{_includedir}/chromium-ewk/
install -m 0644 v8/include/*.h "%{buildroot}"%{_includedir}/v8/

%if 0%{?_enable_content_shell}
install -m 0755 "%{OUTPUT_FOLDER}"/content_shell "%{buildroot}%{CHROMIUM_EXE_DIR}"/content_shell
sed 's#@binary@#%{CHROMIUM_EXE_DIR}/content_shell#' %{SOURCE1} > "%{buildroot}"%{_bindir}/content_shell
%endif

%if 0%{?_enable_unittests}
install -d "%{INSTALL_ROOT}%{CHROMIUM_UNITTESTS_DIR}"
for test in "%{OUTPUT_FOLDER}/*_unittests"; do
  install -m 0755 ${test} "%{INSTALL_ROOT}%{CHROMIUM_UNITTESTS_DIR}"
done
%endif

%if 0%{?build_ewk_unittests}
mkdir -p %{buildroot}/opt/usr/resources/
mkdir -p %{buildroot}/opt/usr/utc_exec/
cp -r tizen_src/ewk/unittest/resources/* %{buildroot}/opt/usr/resources/
install -m 0644 "%{OUTPUT_FOLDER}"/ewk_unittests %{buildroot}/opt/usr/utc_exec/
install -m 0755 tizen_src/ewk/utc_gtest_run.sh %{buildroot}/opt/usr/utc_exec/
%endif

# The native applications using ewk API from Tizen 2.x have a dependency about libewebkit2.so.
# The chromium-efl should support the backward compatibility of the native applications.
ln -s %{_libdir}/libchromium-ewk.so %{buildroot}%{_libdir}/libewebkit2.so.0
ln -s %{_libdir}/libewebkit2.so.0   %{buildroot}%{_libdir}/libewebkit2.so

%if 0%{?build_tizen_ppapi_extension_unittests}
mkdir -p %{buildroot}/opt/usr/utc_exec/
install -m 0755 -p -D %{OUTPUT_FOLDER}/efl_integration_unittests %{buildroot}/opt/usr/utc_exec/
install -m 0755 -p -D %{OUTPUT_FOLDER}/ppapi_unittests %{buildroot}/opt/usr/utc_exec/
%endif

# clientcertlist for Tizen 4.0 TV
%if "%{?tizen_profile_name}" == "tv"
install -d "%{buildroot}%{CHROMIUM_DATA_DIR}"/clientcert
install -m 0644 %{OUTPUT_FOLDER}/resources/clientcert/* %{buildroot}%{CHROMIUM_DATA_DIR}/clientcert
%endif

%if %{__enable_nacl} == 1
install -m 0755 "%{OUTPUT_FOLDER}"/nacl_helper                    "%{buildroot}%{CHROMIUM_EXE_DIR}"
install -m 0755 "%{OUTPUT_FOLDER}"/nacl_helper_bootstrap          "%{buildroot}%{CHROMIUM_EXE_DIR}"
%if "%{?tizen_profile_name}" == "tv" && "%{?_with_emulator}" != "1"
install -m 0755 "%{OUTPUT_FOLDER}"/nacl_helper_nonsfi             "%{buildroot}%{CHROMIUM_EXE_DIR}"
%endif
install -m 0644 "%{OUTPUT_FOLDER}"/nacl_irt_*.nexe                "%{buildroot}%{CHROMIUM_EXE_DIR}"
%endif

%if "%{__enable_nacl}" == "1"
# 4755 is required by chrome suid sandbox architecture:
# http://www.chromium.org/developers/design-documents/sandbox
#
# Compare with chrome-sandbox desktop installation ./build/update-linux-sandbox.sh
# sudo -- chmod 4755 "${CHROME_SANDBOX_INST_PATH}"
#
# 4755 for chrome sandbox is required for sandbox to move NaCl process to new pid and network namespace.
# After moving new process there, one of the next steps is to drop root privileges for it.
# Process that is being spawned under sandbox (currently only /usr/lib/chromium-efl/nacl_helper_bootstrap)
# does not have suid or root permissions and is being chrooted.
#
# It means that it is safe to grant 4755 to this sandbox binary - processes that run under it have
# restricted access to filesystem (chroot), network and doesn't have root permissions.
#
# If chrome-sandbox doesn't have 4755 permissions, processes won't be able to start under sandbox.
install -m 4755 -p -D %{OUTPUT_FOLDER}/chrome_sandbox %{buildroot}%{CHROMIUM_EXE_DIR}/chrome-sandbox
%endif

%post
%if %{__enable_nacl} == 1
chmod 4755 %{CHROMIUM_EXE_DIR}/chrome-sandbox
%endif

%if "%{?tizen_profile_name}" == "tv"
chsmack -a "*" opt/usr/home/owner/.pki/nssdb/cert9.db
%endif
%postun

%files
%manifest packaging/chromium-efl.manifest
/usr/apps/org.tizen.%{name}/bin/efl_webview_app
/usr/apps/org.tizen.%{name}/bin/mini_browser
/usr/apps/org.tizen.%{name}/bin/ubrowser
%{_icondir}/efl_webview_app.png
%{_icondir}/mini_browser.png
%{_icondir}/ubrowser.png
%{_xmldir}/%{_pkgid}.xml
%defattr(-,root,root,-)
%{_libdir}/libchromium-ewk.so
%{_libdir}/libewebkit2.so*
%if "%{chromium_efl_tizen_profile}" == "tv"
%{CHROMIUM_EXE_DIR}/efl_pluginprocess
%endif
%if "%{?tizen_profile_name}" == "tv"
%caps(cap_mac_admin,cap_mac_override,cap_setgid=ei) %{CHROMIUM_EXE_DIR}/efl_webprocess
%else
%{CHROMIUM_EXE_DIR}/efl_webprocess
%endif
%{CHROMIUM_EXE_DIR}/icudtl.dat
%{CHROMIUM_EXE_DIR}/natives_blob.bin
%{CHROMIUM_EXE_DIR}/snapshot_blob.bin
%{CHROMIUM_EXE_DIR}/content_shell.pak
%{CHROMIUM_EXE_DIR}/locales/*.pak
%{CHROMIUM_DATA_DIR}/themes/*.edj
%{CHROMIUM_DATA_DIR}/images/*.png
%{CHROMIUM_LOCALE_DIR}
%if "%{?tizen_profile_name}" == "tv"
%{CHROMIUM_DATA_DIR}/clientcert
%endif

%if %{__enable_nacl} == 1
%{CHROMIUM_EXE_DIR}/chrome-sandbox
%{CHROMIUM_EXE_DIR}/nacl_helper
%{CHROMIUM_EXE_DIR}/nacl_helper_bootstrap
%if "%{?tizen_profile_name}" == "tv" && "%{?_with_emulator}" != "1"
%{CHROMIUM_EXE_DIR}/nacl_helper_nonsfi
%endif
%{CHROMIUM_EXE_DIR}/nacl_irt_*.nexe
%endif

%files devel
%defattr(-,root,root,-)
%{_includedir}/chromium-ewk/*.h
%{_libdir}/pkgconfig/*.pc
%{_includedir}/v8/*

%if 0%{?_enable_content_shell}
%files shell
%defattr(0755,root,root,-)
%{CHROMIUM_EXE_DIR}/content_shell
%{_bindir}/content_shell
%endif

%if 0%{?_enable_unittests}
%files unittests
%defattr(-,root,root,-)
%{CHROMIUM_UNITTESTS_DIR}/*
%endif

%if 0%{?build_ewk_unittests}
%files ewktest
%defattr(-,root,root,-)
%manifest ./packaging/chromium-ewktest.manifest
/opt/usr/utc_exec/*
/opt/usr/resources/*
%endif

%if "%{chromium_efl_tizen_profile}" == "tv"
ln -s %{CHROMIUM_EXE_DIR}/efl_webprocess %{buildroot}%{CHROMIUM_EXE_DIR}/efl_pluginprocess
%endif

%if 0%{?build_tizen_ppapi_extension_unittests}
%files tizen_ppapi_extension_unittests
%defattr(-,root,root,-)
%manifest ./packaging/chromium-tizen_ppapi_extension_unittests.manifest
/opt/usr/utc_exec/*
%endif
