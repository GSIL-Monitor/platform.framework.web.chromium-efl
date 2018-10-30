#!/bin/bash

SCRIPTDIR=$( cd $(dirname $0) ; pwd -P )

source ${SCRIPTDIR}/common.sh
trap 'error_report $0 $LINENO' ERR SIGINT SIGTERM SIGQUIT

host_arch=$(getHostArch)

parseHostBuildScriptParams desktop $@

JHBUILD_STAMPFILE=""
if [ -z "$GN_GENERATOR_OUTPUT" ]; then
  export GN_GENERATOR_OUTPUT=${TOPDIR}/"out.${host_arch}"
fi
JHBUILD_STAMPFILE="${GN_GENERATOR_OUTPUT}/Dependencies/Root/jhbuild.stamp"

forceJHBuildIfNeeded() {
  if [[ $FORCE_JHBUILD == 1 ]]; then
    rm -f $JHBUILD_STAMPFILE
    return
  fi

  # Check if anything in jhbuild is more recent than stamp file.
  if [ $(find $SCRIPTDIR/jhbuild -type f -newer $JHBUILD_STAMPFILE -print | wc -l) != "0" ]; then
    rm -f $JHBUILD_STAMPFILE
  fi
}

forceJHBuildIfNeeded

JHBUILD_DEPS=""
JHBUILD_DEPS="${GN_GENERATOR_OUTPUT}/Dependencies/Root"
if [ "${host_arch}" == "x64" ]; then
  _LIBDIR=lib64
elif [ "${host_arch}" == "ia32" ]; then
  _LIBDIR=lib
fi
export PKG_CONFIG_PATH="${JHBUILD_DEPS}/${_LIBDIR}/pkgconfig"

if [ ! -f "$JHBUILD_STAMPFILE" ]; then
  jhbuild --no-interact -f ${SCRIPTDIR}/jhbuild/jhbuildrc

  if [[ $? == 0 ]]; then
    echo "Yay! jhbuild done!" > $JHBUILD_STAMPFILE
  fi
fi

export GN_CHROMIUMEFL_TARGET=desktop

export C_INCLUDE_PATH="${SCRIPTDIR}/desktop/tizen_include:$C_INCLUDE_PATH"
export CPLUS_INCLUDE_PATH="${SCRIPTDIR}/desktop/tizen_include:$CPLUS_INCLUDE_PATH"

hostGnChromiumEfl "deps_include_path=\"${JHBUILD_DEPS}/include\" deps_lib_path=\"${JHBUILD_DEPS}/${_LIBDIR}\""

export LD_LIBRARY_PATH="${JHBUILD_DEPS}/${_LIBDIR}:$LD_LIBRARY_PATH"
export PATH="${JHBUILD_DEPS}/bin:$PATH"

hostNinja desktop
