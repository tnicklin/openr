#!/bin/bash
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# This source code is licensed under the MIT license found in the
# LICENSE file in the root directory of this source tree.

DEFAULT_CFG="/etc/openr.conf"
INSTALL_BASE="/opt"

# Allow config file to be passed as an argument
if [ "$1" ]; then
  OPENR_CFG=$1
  shift
else
  echo "[$(date)] Error: No config file specified."
  exit 1
fi

# Check if the config file exists and is not empty
if [ ! -s "$OPENR_CFG" ]; then
  echo "[$(date)] Error: Config file $OPENR_CFG not found or is empty."
  exit 1
fi

DEPS=(
  "${INSTALL_BASE}/folly/lib/"
  "${INSTALL_BASE}/fbthrift/lib/"
)

for dep in "${DEPS[@]}"; do
  evaled_path=$(eval echo "${dep}")
  export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$evaled_path"
done

# Add Open/R binary directory to PATH
export PATH="${PATH}:${INSTALL_BASE}/openr/sbin"

echo "[$(date)] Attempting to start Open/R using $OPENR_CFG $*"
openr_bin -v 2 --config "$OPENR_CFG" "$@"

