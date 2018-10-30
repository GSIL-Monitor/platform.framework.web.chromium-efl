#!/bin/bash

export SCRIPTDIR=$(readlink -e $(dirname $0))
export TOPDIR=$(readlink -f "${SCRIPTDIR}/../..")
export CHROME_SRC="${TOPDIR}"
# Please set DEFAULT_TIZEN_VERSION to empty value for the latest tizen version
# or set DEFAULT_TIZEN_VERSION to current tizen version for the others.
export DEFAULT_TIZEN_VERSION=

if [ "$(echo "$@" | grep -e "--tizen")" != "" ]; then
  tizen_version=$(echo $@ | sed -e 's#.*--tizen_\([0-9.]*\).*#\1#')
  if $(echo $tizen_version | grep -qe "^[-\?[0-9]\+\.\?[0-9]*$" && echo true || echo false); then
    DEFAULT_TIZEN_VERSION=$tizen_version
  else
    DEFAULT_TIZEN_VERSION=
  fi
fi

supported_targets=("tizen" "desktop")

function getHostOs() {
  echo $(uname -s | sed -e 's/Linux/linux/;s/Darwin/mac/')
}

function getHostArch() {
  echo $(uname -m | sed -e \
        's/i.86/x86/;s/x86_64/x64/;s/amd64/x64/;s/arm.*/arm/;s/i86pc/x86/;s/aarch64/arm64/')
}

function getPythonVersion() {
  echo $(python --version  2>&1 | sed -e 's/Python \([0-9]\+\.[0-9]\+\)\.[0-9]\+/\1/')
}

function setIfUndef() {
    eval original=\$$1
    new=$2
    if [ -n "$original" ]; then
        echo $original
    else
        echo $new
    fi
}


function hostBuldScriptUsage() {
cat << EOF
usage: $1 [OPTIONS]

Build non gbs version of chromium-efl

OPTIONS:
   -h, --help                    Show this message
   --build-ewk-unittests         Build ewk unittests
   --enable-pepper-extensions    Enable Tizen Pepper Extensions
   --ccache                      Configure ccache installed in your system
   --clang                       Use chromium's clang compiler to build the sources
   --no-content-shell            Don't build content_shell application
   --debug                       Build debug version of chromium-efl (out.${host_arch}/Debug instead of out.${host_arch}/Release)
   --dcheck                      Enable DCHECK macro function
   --ttrace                      Enable TTRACE
   -jN                           Set number of jobs, just like with make or ninja
   --skip-gn                     Skip generating ninja step
   --skip-ninja                  Skip ninja step

examples:
$0 --ccache
$0 --skip-ninja
EOF
  exit
}

function parseHostBuildScriptParams() {

  export USE_CCACHE=0
  export USE_CLANG=0
  export FORCE_JHBUILD=0
  export SKIP_NINJA=0
  export BUILD_EWK_UNITTESTS=0
  export BUILD_CONTENT_SHELL=1
  export BUILD_SUBDIRECTORY=Release
  export COMPONENT_BUILD=0
  export DCHECK_ALWAYS_ON=0
  export ENABLE_PEPPER_EXTENSIONS=0

  local platform="$1"
  shift

  while [[ $# > 0 ]]; do
    case "$1" in
      -h|--help)
        hostBuldScriptUsage ${0}
        ;;
      --skip-gn)
        export SKIP_GN=1
        ;;
      --ccache)
        echo using ccache
        export USE_CCACHE=1
        source $TOPDIR/tizen_src/build/ccache_env.sh ${platform}
        ;;
      --clang)
        export USE_CLANG=1
        ;;
      --no-content-shell)
        export BUILD_CONTENT_SHELL=0
        ;;
      --force-jhbuild)
        export FORCE_JHBUILD=1
        ;;
      --skip-ninja)
        export SKIP_NINJA=1
        ;;
      --build-ewk-unittests)
        export BUILD_EWK_UNITTESTS=1
        ;;
      --debug)
        export BUILD_SUBDIRECTORY="Debug"
        ;;
      --dcheck)
        export DCHECK_ALWAYS_ON=1
        ;;
      --component-build)
        export COMPONENT_BUILD=1
        ;;
      --enable-pepper-extensions)
        export ENABLE_PEPPER_EXTENSIONS=1
        ;;
      -j*)
        export JOBS="$1"
        ;;
      *)
        echo "Unknown argument: $1"
        exit 1
        ;;
    esac
    shift;
  done
}

function hostGnChromiumEfl() {
  if [[ $SKIP_GN != 1 ]]; then
    local XWALK_ARG=""
    local COMPONENT_ARG=""
    local PEPPER_EXTENSIONS_ARG=""
    if [[ $COMPONENT_BUILD == 1 ]]; then
      COMPONENT_ARG="component=shared_library"
    fi
    if [[ $ENABLE_PEPPER_EXTENSIONS == 1 ]]; then
      PEPPER_EXTENSIONS_ARG="tizen_pepper_extensions=true"
    fi
    ${TOPDIR}/tizen_src/build/gn_chromiumefl.sh \
      $XWALK_ARG \
      $COMPONENT_ARG \
      $PEPPER_EXTENSIONS_ARG \
      $@
  fi
}

function hostNinja() {
  if [[ $SKIP_NINJA == 0 ]]; then
    TARGETS="chromium-ewk efl_webprocess efl_webview_app ubrowser"
    if [[ $BUILD_EWK_UNITTESTS == 1 ]]; then
      TARGETS="$TARGETS ewk_unittests"
    fi
    if [[ $BUILD_CONTENT_SHELL == 1 ]]; then
      TARGETS="$TARGETS content_shell dump_syms minidump_stackwalk"
    fi
    if [[ $BUILD_CHROMEDRIVER == 1 ]]; then
      TARGETS="$TARGETS chromedriver"
    fi
    export BUILD_SUBDIRECTORY=""
    BUILDDIR=${GN_GENERATOR_OUTPUT}/${BUILD_SUBDIRECTORY}
    ninja -C $BUILDDIR ${JOBS} ${TARGETS}
  fi
}

function error_report() {
  echo "Error: File:$1 Line:$2"
  exit 1
}

function findElementInArray() {
  local elm
  for elm in "${@:2}"; do
    [[ "$elm" = "$1" ]] && return 0;
  done
  return 1;
}

function setupAndExecuteTargetBuild() {
  local platform="$1"
  shift

  local PROFILE
  local ARCHITECTURE
  local CONF_FLAG
  local -a ARGS

  local count=0
  local exclusive_options=0
  local RPMLINT=0
  local NOINIT=0
  local MIRROR=0
  local RELEASE_MODE="_tizen_build_devel_mode 1"

  # "|| :" means "or always succeeding built-in command"
  PROFILE=$(echo "$@" | grep -Po "(?<=\-P\s)[^\s]*" | uniq || :)
  ARCHITECTURE=$(echo "$@" | grep -Po "(?<=\-A\s)[^\s]*" | uniq || :)

  while [[ $# > 0 ]]; do
    count=$(( $count + 1 ))
    case "$1" in
    --asan)
        ASAN=1
        ARGS[$count]=--define
        count=$(( $count + 1 ))
        ARGS[$count]="asan 1"
    ;;
    --debug)
        ARGS[$count]=--define
        count=$(( $count + 1 ))
        ARGS[$count]="_debug_mode 1"
    ;;
    --skip-ninja)
        ARGS[$count]=--define
        count=$(( $count + 1 ))
        ARGS[$count]="_skip_ninja 1"
    ;;
    --gcov)
        ARGS[$count]=--define
        count=$(( $count + 1 ))
        ARGS[$count]="enable_gcov 1"
    ;;
    --skip-gn)
        ARGS[$count]=--define
        count=$(( $count + 1 ))
        ARGS[$count]="_skip_gn 1"
    ;;
   --dcheck)
        ARGS[$count]=--define
        count=$(( $count + 1 ))
        ARGS[$count]="dcheck_always_on 1"
    ;;
    --ttrace)
        ARGS[$count]=--define
        count=$(( $count + 1 ))
        ARGS[$count]="_ttrace 1"
    ;;
    --component-build)
        ARGS[$count]=--define
        count=$(( $count + 1 ))
        ARGS[$count]="component_build 1"
    ;;
    --gbs-debug)
        ARGS[$count]=--debug
    ;;
    --rpmlint)
        RPMLINT=1
        count=$(( $count + 1 ))
    ;;
    --nodebug)
        ARGS[$count]=--define
        count=$(( $count + 1 ))
        ARGS[$count]="_nodebug 1"
    ;;
    --noinit)
        NOINIT=1
        ARGS[$count]="$1"
        count=$(( $count + 1 ))
    ;;
    --mirror)
        MIRROR=1
    ;;
    --release)
        RELEASE_MODE="_tizen_build_devel_mode 0"
    ;;
    --skip-gn)
        ARGS[$count]=--define
        count=$(( $count + 1 ))
        ARGS[$count]="_skip_gn 1"
    ;;
    --standard*)
        if [ "$DEFAULT_TIZEN_VERSION" == "" ]; then
          echo $1
          PROFILE=tz_$(echo $1 | sed 's/--//g')
        elif [ "$DEFAULT_TIZEN_VERSION" == "4.0" ]; then
          PROFILE=tz_${DEFAULT_TIZEN_VERSION}_$(echo $1 | sed 's/--//g')_$(echo $ARCHITECTURE | grep "i586" -q && echo "ia32" || echo $ARCHITECTURE)
        else
          PROFILE=tz_${DEFAULT_TIZEN_VERSION}_$(echo $1 | sed 's/--//g')
        fi
    ;;
    --emulator*)
        if [ "$DEFAULT_TIZEN_VERSION" == "" ]; then
          PROFILE=tz_$(echo $1 | sed 's/--//g')
        elif [ "$DEFAULT_TIZEN_VERSION" == "4.0" ]; then
          PROFILE=tz_${DEFAULT_TIZEN_VERSION}_$(echo $1 | sed 's/--//g')_$(echo $ARCHITECTURE | grep "i586" -q && echo "ia32" || echo $ARCHITECTURE)
        else
          PROFILE=tz_${DEFAULT_TIZEN_VERSION}_$(echo $1 | sed 's/--//g')
        fi
    ;;
    --tizen*)
    ;;
    *)
      ARGS[$count]="$1"
    ;;
    esac
    shift;
  done

  if [ "$exclusive_options" -gt 1 ]; then
    echo "ERROR: --libs and --crosswalk-bin are mutually exclusive parameters."
    exit 1
  fi

  if [ "$target" == "libs" ]; then
    SPEC_FILE="chromium-efl-libs.spec"
  elif [ "$target" == "crosswalk-bin" ]; then
    SPEC_FILE="crosswalk-bin.spec"
  fi

  if [ "$PROFILE" == "" ]; then
    if [[ $platform == "mobile" ]]; then
      PROFILE=tzmb_3.0_target-TM1
    elif [[ $platform == "tv" ]]; then
      if [ "$DEFAULT_TIZEN_VERSION" == "" ]; then
        PROFILE=tztv_arm-kantm2
      elif [ "$DEFAULT_TIZEN_VERSION" == "4.0" ]; then
        PROFILE=tztv_4.0_arm-kantm
      fi
    elif [[ $platform == "da" ]]; then
      if [ "$DEFAULT_TIZEN_VERSION" == "" ]; then
        PROFILE=tzda_arm-kantm
      else
        PROFILE=tzda_${DEFAULT_TIZEN_VERSION}_arm-kantm
      fi
    else
      echo "Cannot set default PROFILE for platform=${platform}"
      exit 1
    fi
  fi

  if [ "$MIRROR" == "1" ]; then
    PROFILE=${PROFILE}_mirror
  fi

  echo "Set the profile : $PROFILE"

  if [ "$ARCHITECTURE" == "" ]; then
    if [[ $platform == "mobile" ]]; then
      ARCHITECTURE=armv7l
    elif [[ $platform == "tv" ]]; then
      ARCHITECTURE=armv7l
    elif [[ $platform == "da" ]]; then
      ARCHITECTURE=armv7l
    else
      echo "Cannot set default ARCHITECTURE for platform=${platform}"
      exit 1
    fi
  fi
  echo "Set the architecture : $ARCHITECTURE"

  if [ "$USE_GLOBAL_GBS_CONF" == "" ]; then
    CONF_FLAG="--conf ${SCRIPTDIR}/gbs.conf"
  fi

  if [ "$(echo "${PROFILE}" | grep -P "kant|jazz|hawk|product")" == "" -a "$NOINIT" == 0 ]; then
    processRpmlintOption $PROFILE $RPMLINT
  fi

  local EXTRA_PACK_OPTS=""
  if [ "$ASAN" == "1" ]; then
    EXTRA_PACK_OPTS="--extra-packs asan-force-options,asan-build-env"
  fi

  gbs $CONF_FLAG build -P $PROFILE --include-all -A $ARCHITECTURE "${ARGS[@]}" $EXTRA_PACK_OPTS \
      $BUILD_CONF_OPTS --define "$RELEASE_MODE" --incremental --define "_use_customized_job_for_gbs 1"
}

function processRpmlintOption() {
  local PROFILE=$1
  local RPMLINT=$2
  local URL_TARGET=
  export BUILD_CONF_OPTS=

  echo "** Process rpmlint option"

  local PREVIOUS_RPMLINT="!rpmlint"
  if [ ! -f "$PREVIOUS_BUILD_CONF_PATH" ]; then
    # Never built before
    PREVIOUS_RPMLINT=""
  elif [ "$(grep -Po "\!rpmlint" "$PREVIOUS_BUILD_CONF_PATH" | uniq -d)" != "!rpmlint" ]; then
    # Once built with rpmlint
    PREVIOUS_RPMLINT="rpmlint"
  fi

  # Disable rpmlint
  if [ "$RPMLINT" == 0 ]; then
    echo "** Disable rpmlint"
    BUILD_CONF_DIR=$SCRIPTDIR/build_conf
    rm -rf $BUILD_CONF_DIR
    mkdir $BUILD_CONF_DIR
    URL_TARGET=$(echo $PROFILE | sed -e 's#.*\.[0-9]*_\([-a-zA-Z0-9]*\).*#\1#;s/tz_//g')
    # The latest version doesn't have tizen version in snapshot repository url.
    if [ "$(echo $DEFAULT_TIZEN_VERSION)" == "" ]; then
      URL_TIZEN_VERSION=
    else
      URL_TIZEN_VERSION=$DEFAULT_TIZEN_VERSION-
    fi
    REPO=http://download.tizen.org/snapshots/tizen/$URL_TIZEN_VERSION$platform/latest/repos/$URL_TARGET/packages/repodata/
    echo "** Repo : $REPO"
    wget $REPO -O $BUILD_CONF_DIR/index.html > /dev/null 2>&1
    BUILD_CONF_GZ=$(grep "build.conf.gz" $BUILD_CONF_DIR/index.html | sed -e 's#.*\=\"\(.*\)\".*#\1#')
    BUILD_CONF=$(basename $BUILD_CONF_GZ .gz)
    wget $REPO$BUILD_CONF_GZ -P $BUILD_CONF_DIR > /dev/null 2>&1
    if [ ! -f "$BUILD_CONF_DIR/$BUILD_CONF_GZ" ]; then
      echo "It's failed to donwload build.conf. Please contact system administrator."
      exit 1
    fi
    gunzip $BUILD_CONF_DIR/$BUILD_CONF_GZ
    sed -i 's/rpmlint-mini\ rpmlint-tizen/!rpmlint-mini\ !rpmlint-tizen/g' $BUILD_CONF_DIR/$BUILD_CONF
    BUILD_CONF_OPTS="-D $BUILD_CONF_DIR/$BUILD_CONF"
    if [ "$PREVIOUS_RPMLINT" == "rpmlint" ]; then
      echo "** Once built with rpmlint"
      BUILD_CONF_OPTS+=" --clean"
    fi
  else # Enable rpmlint
    echo "** Enable rpmlint"
    BUILD_CONF_OPTS=""
    if [ "$PREVIOUS_RPMLINT" == "!rpmlint" ]; then
      BUILD_CONF_OPTS="--clean"
    fi
  fi
}
