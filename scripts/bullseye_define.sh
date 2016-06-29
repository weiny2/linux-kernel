#!/bin/bash

export BULLSEYE_SRC_DIR=/mnt/fabric
export BULLSEYE_DIR=/opt/BullseyeCoverage
export HOME_DIR=/home/fxr

USER_BULLSEYE=scripts/bullseye_define_${USER}.sh
if [ -e ${USER_BULLSEYE} ]; then
    . ${USER_BULLSEYE}
fi
