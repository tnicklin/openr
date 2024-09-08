/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <openr/common/Constants.h>
#include <openr/common/Types.h>
#include <openr/config/Config.h>
#include <openr/messaging/ReplicateQueue.h>
#include <openr/spark/Spark.h>

namespace openr {

/**
 * A utility class to wrap and interact with Spark. It exposes the APIs to
 * send commands to and receive publications from Spark.
 * Mainly used for testing.
 *
 * This should be managed from only one thread. Otherwise behaviour will be
 * undesirable.
 */
class SparkWrapper {
 public:
  SparkWrapper(
      std::string const& myNodeName,
      std::pair<uint32_t, uint32_t> version,
      std::shared_ptr<IoProvider> ioProvider,
      std::shared_ptr<const Config> config,
      bool isRateLimitEnabled = true);

  ~SparkWrapper();

  // start spark
  void run();

  // stop spark
  void stop();

  // get spark instance
  std::shared_ptr<Spark>
  get() {
    return spark_;
  }

  // add interfaceDb for Spark to tracking
  void updateInterfaceDb(const InterfaceDatabase& ifDb);

  // send ADJ_DB_SYNC signal to Spark
  void sendPrefixDbSyncedSignal();

  // send neighbor down event to Spark
  void sendNeighborDownEvent(
      const std::string& ifName, const thrift::BinaryAddress& v6Addr);

  // receive spark neighbor event
  std::optional<NeighborEvents> recvNeighborEvent(
      std::optional<std::chrono::milliseconds> timeout = std::nullopt);

  std::optional<NeighborEvents> waitForEvents(
      const NeighborEventType neighborEventType,
      std::optional<std::chrono::milliseconds> rcvdTimeout = std::nullopt,
      std::optional<std::chrono::milliseconds> procTimeout =
          Constants::kPlatformProcTimeout) noexcept;

  std::optional<thrift::InitializationEvent> recvInitializationEvent(
      std::optional<std::chrono::milliseconds> timeout);

  bool waitForInitializationEvent(
      bool expectEmptyNeighborEvent = false,
      std::optional<std::chrono::milliseconds> rcvdTimeout = std::nullopt,
      std::optional<std::chrono::milliseconds> procTimeout =
          Constants::kPlatformProcTimeout) noexcept;

  // utility call to check neighbor state
  std::optional<thrift::SparkNeighState> getSparkNeighState(
      std::string const& ifName, std::string const& neighborName);

  static std::pair<folly::IPAddress, folly::IPAddress> getTransportAddrs(
      const NeighborEvent& event);

  // utility function to construct thrift::AreaConfig.SparkConfigs
  const openr::thrift::SparkConfig
  getSparkConfig() {
    return config_->getSparkConfig();
  }

  /* Forwarded to the underlying Spark processPacket()
   * For test purposes, e.g. to manually invoke packet handling
   * inline on the same thread - bypassing the event-base - as is needed
   * for fuzzing.
   */
  void processPacket();

  // Get the count of all known neighbors to Spark.
  uint64_t getTotalNeighborCount();

  // Get the count of all active neighbors to Spark.
  uint64_t getActiveNeighborCount();

 private:
  std::string myNodeName_;
  std::shared_ptr<const Config> config_{nullptr};

  // Queue to send neighbor event to LinkMonitor
  messaging::ReplicateQueue<NeighborInitEvent> neighborUpdatesQueue_;
  messaging::RQueue<NeighborInitEvent> neighborUpdatesReader_{
      neighborUpdatesQueue_.getReader()};

  // Queue to receive interface update from LinkMonitor
  messaging::ReplicateQueue<InterfaceDatabase> interfaceUpdatesQueue_;

  // Queue to receive interface update from PrefixManager
  messaging::ReplicateQueue<thrift::InitializationEvent>
      initializationEventQueue_;

  // Queue to receive neighbor update from NeighborMonitor
  messaging::ReplicateQueue<AddressEvent> addrEventQueue_;

  // Spark owned by this wrapper.
  std::shared_ptr<Spark> spark_{nullptr};

  // Thread in which Spark will be running.
  std::unique_ptr<std::thread> thread_{nullptr};
};

} // namespace openr
