/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <wangle/ssl/SSLContextConfig.h>

#include <openr/common/LsdbTypes.h>
#include <openr/config/Config.h>
#include <openr/decision/RouteUpdate.h>
#include <openr/if/gen-cpp2/Types_types.h>
#include <openr/messaging/Queue.h>
#include <openr/messaging/ReplicateQueue.h>

namespace openr {
struct PluginArgs {
  messaging::ReplicateQueue<PrefixEvent>& prefixUpdatesQueue;
  messaging::RQueue<DecisionRouteUpdate> routeUpdatesQueue;
  std::shared_ptr<const Config> config;
  std::shared_ptr<wangle::SSLContextConfig> sslContext;
};

struct VipPluginArgs {
  folly::EventBase* vipRouteEvb;
  messaging::ReplicateQueue<PrefixEvent>& prefixUpdatesQueue;
  std::shared_ptr<const Config> config;
  std::shared_ptr<wangle::SSLContextConfig> sslContext;
};

void pluginStart(const PluginArgs& /* pluginArgs */);
void pluginStop();
/* create a vip plugin object and run */
void vipPluginStart(const VipPluginArgs& /* PluginArgs */);
/* stop vip plugin */
void vipPluginStop();
/* destroy vip plugin, called after vipRouteEvb is stopped */
void vipPluginDestroy();
} // namespace openr
