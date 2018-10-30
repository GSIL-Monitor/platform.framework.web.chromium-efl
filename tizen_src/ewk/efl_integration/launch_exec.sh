#!/bin/sh

EXECUTABLE=$1

if [ ! $EXECUTABLE ]; then
  echo launch_exec.sh takes the name of an executable argument >&2
  exit 1
fi

shift

HOST_ARCH=$(uname -m)

if [ $HOST_ARCH = "x86_64" -o $HOST_ARCH = "amd64" ]; then
  _LIBDIR=lib64
else
  _LIBDIR=lib
fi

SCRIPTDIR=$(readlink -e $(dirname $0))
OUT_DIR=$(echo $SCRIPTDIR | grep -Po "(?<=/)out\..*?(?=/)")

if echo $SCRIPTDIR | grep -qw "Debug"; then
  BUILD_MODE=Debug
else
  BUILD_MODE=Release
fi

# FIXME: CHROMIUM_EFL_LIBDIR should be fixed.
# CHROMIUM_EFL_LIBDIR=$(readlink -e $SCRIPTDIR/lib)
CHROMIUM_EFL_LIBDIR=$(readlink -e $SCRIPTDIR)
CHROMIUM_EFL_DEPENDENCIES_LIBDIR=$(readlink -e $SCRIPTDIR/Dependencies/Root/$_LIBDIR)

export LD_LIBRARY_PATH=$CHROMIUM_EFL_DEPENDENCIES_LIBDIR:$CHROMIUM_EFL_LIBDIR:${LD_LIBRARY_PATH}
echo "LD_LIBRARY_PATH=${LD_LIBRARY_PATH}"

# Allow chromium-efl to work with llvmpipe or softpipe mesa backends
export EVAS_GL_NO_BLACKLIST=1

debug=0
while [ $# -gt 0 ]; do
    case "$1" in
        -g | --debug )
            debug=1
            shift
            ;;
        * )
            break
            ;;
    esac
done

if [ $debug -eq 1 ] ; then
  exec gdb --args ${SCRIPTDIR}/$EXECUTABLE "$@"
else
  exec ${SCRIPTDIR}/$EXECUTABLE "$@"
fi
