/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "Plugin.h"

namespace openr {
void
pluginStart(const PluginArgs& /* pluginArgs*/) {
  return;
}

void
pluginStop() {
  return;
}

void
vipPluginStart(const VipPluginArgs& /* pluginArgs*/) {
  return;
}

void
vipPluginStop() {
  return;
}

void
vipPluginDestroy() {
  return;
}
} // namespace openr
