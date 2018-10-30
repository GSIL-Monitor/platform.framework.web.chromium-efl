#!/bin/sh

SCRIPTDIR=$(readlink -e $(dirname $0))
${SCRIPTDIR}/launch_exec.sh ubrowser "$@"
