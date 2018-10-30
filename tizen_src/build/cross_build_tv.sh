#!/bin/bash

SCRIPTDIR=$( cd $(dirname $0) ; pwd -P )

source ${SCRIPTDIR}/common.sh
trap 'postBuild ${SYSROOTDIR};\
      error_report $0 $LINENO' ERR SIGINT SIGTERM SIGQUIT

host_arch=$(getHostArch)


__SUFFIX=crosscompile.orig

function adaptGbsSysrootToCrossCompilation() {
  echo "cd /etc/; [ -e ld.so.conf.d ] && mv ld.so.conf.d ld.so.conf.d.$__SUFFIX;" | gbs chroot --root ${1}
}
function rollbackGbsSysrootChanges() {
  echo "cd /etc/; [ -e ld.so.conf.d.$__SUFFIX ] && mv ld.so.conf.d.$__SUFFIX ld.so.conf.d;" | gbs chroot --root ${1}
}

function preBuild() {
  adaptGbsSysrootToCrossCompilation $1
}

function postBuild() {
  rollbackGbsSysrootChanges $1
}

parseHostBuildScriptParams crosscompile $@

if [ -z "$GBS_ROOT_PATH" ]; then
  echo "Set GBS_ROOT_PATH for cross compilation"
  exit 1
fi

if [ -z "$CROSS_COMPILE" ]; then
  echo "Set CROSS_COMPILE for cross compilation like: /home/user/toolchains/bin/armv7l-tizen-linux-gnueabi-"
  exit 1
fi

# Only override if not set to enable customization for distributed compiling.
export CC_target=$(setIfUndef CC_target ${CROSS_COMPILE}gcc)
export CXX_target=$(setIfUndef CXX_target ${CROSS_COMPILE}g++)
export AR_target=${CROSS_COMPILE}ar
export AS_target=${CROSS_COMPILE}as
export RANLIB_target=${CROSS_COMPILE}ranlib

export SYSROOTDIR="${GBS_ROOT_PATH}"/local/BUILD-ROOTS/scratch.armv7l.0
export PKG_CONFIG_SYSROOT_DIR="${SYSROOTDIR}"
export PKG_CONFIG_PATH="${SYSROOTDIR}/usr/lib/pkgconfig:${SYSROOTDIR}/usr/share/pkgconfig"

export PATH="${TOPDIR}/build/cross-shim:$PATH"

preBuild ${SYSROOTDIR}

hostNinja crosscompile
RET=$?

postBuild ${SYSROOTDIR}

exit $RET
