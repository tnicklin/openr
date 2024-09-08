/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <fb303/BaseService.h>
#include <openr/common/Types.h>
#include <openr/config-store/PersistentStore.h>
#include <openr/config/Config.h>
#include <openr/decision/Decision.h>
#include <openr/dispatcher/Dispatcher.h>
#include <openr/fib/Fib.h>
#include <openr/if/gen-cpp2/OpenrCtrl.h>
#include <openr/if/gen-cpp2/OpenrCtrlCpp.h>
#include <openr/if/gen-cpp2/OpenrCtrl_types.h>
#include <openr/kvstore/KvStore.h>
#include <openr/kvstore/KvStorePublisher.h>
#include <openr/link-monitor/LinkMonitor.h>
#include <openr/monitor/Monitor.h>
#include <openr/prefix-manager/PrefixManager.h>
#include <openr/spark/Spark.h>

namespace openr {

struct FibStreamSubscriber {
  FibStreamSubscriber(
      std::chrono::steady_clock::time_point upSince,
      std::unique_ptr<apache::thrift::ServerStreamPublisher<
          thrift::RouteDatabaseDeltaDetail>> publisher)
      : upSince(upSince), publisher(std::move(publisher)), total_messages(0) {}

  std::chrono::steady_clock::time_point upSince;
  std::unique_ptr<
      apache::thrift::ServerStreamPublisher<thrift::RouteDatabaseDeltaDetail>>
      publisher;
  int64_t total_messages;
  std::chrono::system_clock::time_point last_message_time;
};

class OpenrCtrlHandler final : public thrift::OpenrCtrlCppSvIf,
                               public facebook::fb303::BaseService {
 public:
  /**
   * NOTE: If acceptablePeerCommonNames is empty then check for peerName is
   *       skipped
   */
  OpenrCtrlHandler(
      const std::string& nodeName,
      const std::unordered_set<std::string>& acceptablePeerCommonNames,
      OpenrEventBase* ctrlEvb,
      Decision* decision,
      Fib* fib,
      KvStore<thrift::OpenrCtrlCppAsyncClient>* kvStore,
      LinkMonitor* linkMonitor,
      Monitor* Monitor,
      PersistentStore* configStore,
      PrefixManager* prefixManager,
      Spark* spark,
      std::shared_ptr<const Config> config,
      Dispatcher* dispatcher = nullptr);

  ~OpenrCtrlHandler() override;

  //
  // fb303 service APIs
  //

  facebook::fb303::cpp2::fb303_status getStatus() override;

  void getCounters(std::map<std::string, int64_t>& _return) override;
  void getRegexCounters(
      std::map<std::string, int64_t>& _return,
      std::unique_ptr<std::string> regex) override;
  void getSelectedCounters(
      std::map<std::string, int64_t>& _return,
      std::unique_ptr<std::vector<std::string>> keys) override;
  int64_t getCounter(std::unique_ptr<std::string> key) override;

  // Openr Node Name
  void getMyNodeName(std::string& _return) override;

  //
  // config APIs
  //

  void getRunningConfig(std::string& _return) override;

  void getRunningConfigThrift(thrift::OpenrConfig& _config) override;

  void dryrunConfig(
      ::std::string& _return, std::unique_ptr<::std::string> file) override;

  folly::SemiFuture<std::unique_ptr<thrift::OpenrDrainState>>
  semifuture_getDrainState(std::unique_ptr<std::string> nodeName) override;

  //
  // OpenR initialization APIs
  //

  void getInitializationEvents(
      std::map<thrift::InitializationEvent, int64_t>& _return) override;

  bool initializationConverged() override;

  int64_t getInitializationDurationMs() override;

  //
  // Monitor APIs
  //

  void getEventLogs(std::vector<::std::string>& _return) override;

  //
  // PrefixManager APIs
  //

  folly::SemiFuture<folly::Unit> semifuture_advertisePrefixes(
      std::unique_ptr<std::vector<thrift::PrefixEntry>> prefixes) override;

  folly::SemiFuture<folly::Unit> semifuture_withdrawPrefixes(
      std::unique_ptr<std::vector<thrift::PrefixEntry>> prefixes) override;

  folly::SemiFuture<folly::Unit> semifuture_withdrawPrefixesByType(
      thrift::PrefixType prefixType) override;

  folly::SemiFuture<folly::Unit> semifuture_syncPrefixesByType(
      thrift::PrefixType prefixType,
      std::unique_ptr<std::vector<thrift::PrefixEntry>> prefixes) override;

  folly::SemiFuture<std::unique_ptr<std::vector<thrift::PrefixEntry>>>
  semifuture_getPrefixes() override;

  folly::SemiFuture<std::unique_ptr<std::vector<thrift::PrefixEntry>>>
  semifuture_getPrefixesByType(thrift::PrefixType prefixType) override;

  folly::SemiFuture<std::unique_ptr<std::vector<thrift::AdvertisedRouteDetail>>>
  semifuture_getAdvertisedRoutes() override;

  folly::SemiFuture<std::unique_ptr<std::vector<thrift::AdvertisedRouteDetail>>>
  semifuture_getAdvertisedRoutesFiltered(
      std::unique_ptr<thrift::AdvertisedRouteFilter> filter) override;

  folly::SemiFuture<std::unique_ptr<std::vector<thrift::AdvertisedRoute>>>
  semifuture_getAreaAdvertisedRoutes(
      std::unique_ptr<std::string> areaName,
      thrift::RouteFilterType routeFilterType) override;

  folly::SemiFuture<std::unique_ptr<std::vector<thrift::AdvertisedRoute>>>
  semifuture_getAreaAdvertisedRoutesFiltered(
      std::unique_ptr<std::string> areaName,
      thrift::RouteFilterType routeFilterType,
      std::unique_ptr<thrift::AdvertisedRouteFilter> filter) override;

  folly::SemiFuture<std::unique_ptr<std::vector<thrift::AdvertisedRoute>>>
  semifuture_getAdvertisedRoutesWithOriginationPolicy(
      thrift::RouteFilterType routeFilterType,
      std::unique_ptr<thrift::AdvertisedRouteFilter> filter) override;

  folly::SemiFuture<std::unique_ptr<std::vector<thrift::OriginatedPrefixEntry>>>
  semifuture_getOriginatedPrefixes() override;

  //
  // Fib APIs
  //

  folly::SemiFuture<std::unique_ptr<thrift::RouteDatabase>>
  semifuture_getRouteDb() override;

  folly::SemiFuture<std::unique_ptr<thrift::RouteDatabaseDetail>>
  semifuture_getRouteDetailDb() override;

  folly::SemiFuture<std::unique_ptr<std::vector<thrift::UnicastRoute>>>
  semifuture_getUnicastRoutesFiltered(
      std::unique_ptr<std::vector<::std::string>> prefixes) override;

  folly::SemiFuture<std::unique_ptr<std::vector<thrift::UnicastRoute>>>
  semifuture_getUnicastRoutes() override;

  folly::SemiFuture<std::unique_ptr<std::vector<thrift::MplsRoute>>>
  semifuture_getMplsRoutesFiltered(
      std::unique_ptr<std::vector<int32_t>> labels) override;

  folly::SemiFuture<std::unique_ptr<std::vector<openr::thrift::MplsRoute>>>
  semifuture_getMplsRoutes() override;

  //
  // Spark APIs
  //

  folly::SemiFuture<folly::Unit> semifuture_floodRestartingMsg() override;

  folly::SemiFuture<std::unique_ptr<std::vector<thrift::SparkNeighbor>>>
  semifuture_getNeighbors() override;

  //
  // Performance stats APIs
  //

  folly::SemiFuture<std::unique_ptr<thrift::PerfDatabase>>
  semifuture_getPerfDb() override;

  //
  // Decision APIs
  //

  folly::SemiFuture<std::unique_ptr<std::vector<thrift::ReceivedRouteDetail>>>
  semifuture_getReceivedRoutes() override;

  folly::SemiFuture<std::unique_ptr<std::vector<thrift::ReceivedRouteDetail>>>
  semifuture_getReceivedRoutesFiltered(
      std::unique_ptr<thrift::ReceivedRouteFilter> filter) override;

  folly::SemiFuture<std::unique_ptr<thrift::AdjDbs>>
  semifuture_getDecisionAdjacencyDbs() override;

  // DEPRECATED. Perfer getDecisionAreaAdjacenciesFiltered to return the areas
  // as well.
  folly::SemiFuture<std::unique_ptr<std::vector<thrift::AdjacencyDatabase>>>
  semifuture_getDecisionAdjacenciesFiltered(
      std::unique_ptr<thrift::AdjacenciesFilter> filter) override;

  folly::SemiFuture<std::unique_ptr<
      std::map<std::string, std::vector<thrift::AdjacencyDatabase>>>>
  semifuture_getDecisionAreaAdjacenciesFiltered(
      std::unique_ptr<thrift::AdjacenciesFilter> filter) override;

  folly::SemiFuture<std::unique_ptr<thrift::RouteDatabase>>
  semifuture_getRouteDbComputed(std::unique_ptr<std::string> nodeName) override;

  //
  // Dispatcher APIs
  //

  folly::SemiFuture<std::unique_ptr<std::vector<std::vector<std::string>>>>
  semifuture_getDispatcherFilters() override;

  // Subscriber Info API

  folly::SemiFuture<std::unique_ptr<std::vector<thrift::StreamSubscriberInfo>>>
  semifuture_getSubscriberInfo(int64_t type) override;

  /*
   * KvStore APIs
   */

  /*
   * API to return key-val pairs by given:
   *  - a set of string "keys"; (ATTN: this is NOT regex matching)
   *  - a specific area;
   */
  folly::SemiFuture<std::unique_ptr<thrift::Publication>>
  semifuture_getKvStoreKeyValsArea(
      std::unique_ptr<std::vector<std::string>> filterKeys,
      std::unique_ptr<std::string> area) override;

  /*
   * [Backward Compatibility] Same as above, but use local KvStoreDb's area
   */
  folly::SemiFuture<std::unique_ptr<thrift::Publication>>
  semifuture_getKvStoreKeyVals(
      std::unique_ptr<std::vector<std::string>> filterKeys) override;

  /*
   * API to return key-val pairs by given:
   *  - thrift::KeyDumpParams;
   *  - a specific area;
   *
   * ATTN: thrift::KeyDumpParams is an advanced version of matching with:
   *  - keyPrefixList; (ATTN: this is regex matching)
   *  - originatorIds; (ATTN: this is NOT regex matching)
   *  - operator to support AND/OR condition matching;
   */
  folly::SemiFuture<std::unique_ptr<thrift::Publication>>
  semifuture_getKvStoreKeyValsFilteredArea(
      std::unique_ptr<thrift::KeyDumpParams> filter,
      std::unique_ptr<std::string> area) override;

  /*
   * [Backward Compatibility] Same as above, but use local KvStoreDb's area
   */
  folly::SemiFuture<std::unique_ptr<thrift::Publication>>
  semifuture_getKvStoreKeyValsFiltered(
      std::unique_ptr<thrift::KeyDumpParams> filter) override;

  /*
   * API to return key-val HASHes(NO binary value included) only by given:
   *  - thrift::KeyDumpParams;
   *  - a specific area;
   *
   * ATTN: same as above usage of thrift::KeyDumpParams
   */
  folly::SemiFuture<std::unique_ptr<thrift::Publication>>
  semifuture_getKvStoreHashFilteredArea(
      std::unique_ptr<thrift::KeyDumpParams> filter,
      std::unique_ptr<std::string> area) override;

  /*
   * [Backward Compatibility] Same as above, but use local KvStoreDb's area
   */
  folly::SemiFuture<std::unique_ptr<thrift::Publication>>
  semifuture_getKvStoreHashFiltered(
      std::unique_ptr<thrift::KeyDumpParams> filter) override;

  /*
   * API to set key-val pairs by given:
   *  - thrift::KeySetParams;
   *  - a specifc area;
   *
   * ATTN: set key will automatically trigger (K, V) merge operation to:
   *  1. update local kvStoreDb;
   *  2. flood the delta update to peers if any;
   * new api with debug info
   */
  folly::SemiFuture<std::unique_ptr<thrift::SetKeyValsResult>>
  semifuture_setKvStoreKeyValues(
      std::unique_ptr<thrift::KeySetParams> setParams,
      std::unique_ptr<std::string> area) override;

  /*
   * API to set key-val pairs by given:
   *  - thrift::KeySetParams;
   *  - a specifc area;
   *
   * ATTN: set key will automatically trigger (K, V) merge operation to:
   *  1. update local kvStoreDb;
   *  2. flood the delta update to peers if any;
   */
  folly::SemiFuture<folly::Unit> semifuture_setKvStoreKeyVals(
      std::unique_ptr<thrift::KeySetParams> setParams,
      std::unique_ptr<std::string> area) override;

  /*
   * API to dump existing peers in a specified area
   */
  folly::SemiFuture<std::unique_ptr<thrift::PeersMap>>
  semifuture_getKvStorePeersArea(std::unique_ptr<std::string> area) override;

  /*
   * [Backward Compatibility] Same as above, but use local KvStoreDb's area
   */
  folly::SemiFuture<std::unique_ptr<thrift::PeersMap>>
  semifuture_getKvStorePeers() override;

  /*
   * API to return structured thrift::KvStoreAreaSummary to include:
   *  - selected area names;
   *  - number of key-val pairs;
   *  - total of key-val bytes;
   *  - peers in each area;
   *  - counters in each area;
   *  - etc;
   */
  folly::SemiFuture<std::unique_ptr<::std::vector<thrift::KvStoreAreaSummary>>>
  semifuture_getKvStoreAreaSummary(
      std::unique_ptr<std::set<std::string>> selectAreas) override;

  // Stream API's
  // Intentionally not use SemiFuture as stream is async by nature and we will
  // immediately create and return the stream handler
  apache::thrift::ServerStream<thrift::Publication> subscribeKvStoreFilter(
      std::unique_ptr<thrift::KeyDumpParams> filter,
      std::unique_ptr<std::set<std::string>> selectAreas);

  apache::thrift::ServerStream<thrift::RouteDatabaseDelta> subscribeFib();

  apache::thrift::ServerStream<thrift::RouteDatabaseDeltaDetail>
  subscribeFibDetail();

  folly::SemiFuture<apache::thrift::ResponseAndServerStream<
      thrift::Publication,
      thrift::Publication>>
  semifuture_subscribeAndGetKvStore() override;

  folly::SemiFuture<apache::thrift::ResponseAndServerStream<
      thrift::Publication,
      thrift::Publication>>
  semifuture_subscribeAndGetKvStoreFiltered(
      std::unique_ptr<thrift::KeyDumpParams> filter) override;

  folly::SemiFuture<apache::thrift::ResponseAndServerStream<
      std::vector<thrift::Publication>,
      thrift::Publication>>
  semifuture_subscribeAndGetAreaKvStores(
      std::unique_ptr<thrift::KeyDumpParams> filter,
      std::unique_ptr<std::set<std::string>> selectAreas) override;

  folly::SemiFuture<apache::thrift::ResponseAndServerStream<
      thrift::RouteDatabase,
      thrift::RouteDatabaseDelta>>
  semifuture_subscribeAndGetFib() override;

  folly::SemiFuture<apache::thrift::ResponseAndServerStream<
      thrift::RouteDatabaseDetail,
      thrift::RouteDatabaseDeltaDetail>>
  semifuture_subscribeAndGetFibDetail() override;

  // Long poll support
  folly::SemiFuture<bool> semifuture_longPollKvStoreAdj(
      std::unique_ptr<thrift::KeyVals> snapshot) override;

  folly::SemiFuture<bool> semifuture_longPollKvStoreAdjArea(
      std::unique_ptr<std::string> area,
      std::unique_ptr<thrift::KeyVals> snapshot) override;

  //
  // LinkMonitor APIs
  //

  folly::SemiFuture<folly::Unit> semifuture_setNodeOverload() override;
  folly::SemiFuture<folly::Unit> semifuture_unsetNodeOverload() override;

  folly::SemiFuture<folly::Unit> semifuture_setInterfaceOverload(
      std::unique_ptr<std::string> interfaceName) override;
  folly::SemiFuture<folly::Unit> semifuture_unsetInterfaceOverload(
      std::unique_ptr<std::string> interfaceName) override;

  folly::SemiFuture<folly::Unit> semifuture_setInterfaceMetric(
      std::unique_ptr<std::string> interfaceName,
      int32_t overrideMetric) override;
  folly::SemiFuture<folly::Unit> semifuture_unsetInterfaceMetric(
      std::unique_ptr<std::string> interfaceName) override;

  folly::SemiFuture<folly::Unit> semifuture_setNodeInterfaceMetricIncrement(
      int32_t metricIncrementVal) override;
  folly::SemiFuture<folly::Unit> semifuture_unsetNodeInterfaceMetricIncrement()
      override;

  folly::SemiFuture<folly::Unit> semifuture_setInterfaceMetricIncrement(
      std::unique_ptr<std::string> interfaceName,
      int32_t metricIncrementVal) override;
  folly::SemiFuture<folly::Unit> semifuture_unsetInterfaceMetricIncrement(
      std::unique_ptr<std::string> interfaceName) override;
  folly::SemiFuture<folly::Unit> semifuture_setInterfaceMetricIncrementMulti(
      std::unique_ptr<std::vector<std::string>> interfaces,
      int32_t metricIncrementVal) override;
  folly::SemiFuture<folly::Unit> semifuture_unsetInterfaceMetricIncrementMulti(
      std::unique_ptr<std::vector<std::string>> interfaces) override;

  folly::SemiFuture<folly::Unit> semifuture_setAdjacencyMetric(
      std::unique_ptr<std::string> interfaceName,
      std::unique_ptr<std::string> adjNodeName,
      int32_t overrideMetric) override;
  folly::SemiFuture<folly::Unit> semifuture_unsetAdjacencyMetric(
      std::unique_ptr<std::string> interfaceName,
      std::unique_ptr<std::string> adjNodeName) override;

  folly::SemiFuture<std::unique_ptr<thrift::DumpLinksReply>>
  semifuture_getInterfaces() override;

  folly::SemiFuture<std::unique_ptr<thrift::AdjacencyDatabase>>
  semifuture_getLinkMonitorAdjacencies() override;

  // DEPRECATED. Perfer getLinkMonitorAreaAdjacenciesFiltered to return the
  // areas as well.
  folly::SemiFuture<std::unique_ptr<std::vector<thrift::AdjacencyDatabase>>>
  semifuture_getLinkMonitorAdjacenciesFiltered(
      std::unique_ptr<thrift::AdjacenciesFilter> filter) override;

  folly::SemiFuture<std::unique_ptr<
      std::map<std::string, std::vector<thrift::AdjacencyDatabase>>>>
  semifuture_getLinkMonitorAreaAdjacenciesFiltered(
      std::unique_ptr<thrift::AdjacenciesFilter> filter) override;

  // Explicitly override blocking API call as no ASYNC needed
  void getOpenrVersion(thrift::OpenrVersions& openrVersion) override;
  void getBuildInfo(thrift::BuildInfo& buildInfo) override;

  //
  // PersistentStore APIs
  //

  folly::SemiFuture<folly::Unit> semifuture_setConfigKey(
      std::unique_ptr<std::string> key,
      std::unique_ptr<std::string> value) override;

  folly::SemiFuture<folly::Unit> semifuture_eraseConfigKey(
      std::unique_ptr<std::string> key) override;

  folly::SemiFuture<std::unique_ptr<std::string>> semifuture_getConfigKey(
      std::unique_ptr<std::string> key) override;

  //
  // RibPolicy APIs
  //

  folly::SemiFuture<folly::Unit> semifuture_setRibPolicy(
      std::unique_ptr<thrift::RibPolicy> policy) override;

  folly::SemiFuture<std::unique_ptr<thrift::RibPolicy>>
  semifuture_getRibPolicy() override;

  folly::SemiFuture<folly::Unit> semifuture_clearRibPolicy() override;

  //
  // APIs to expose state of private variables
  //

  inline size_t
  getNumKvStorePublishers() {
    return kvStorePublishers_.wlock()->size();
  }

  inline size_t
  getNumPendingLongPollReqs() {
    return longPollReqs_->size();
  }

  inline size_t
  getNumFibPublishers() {
    return fibPublishers_.wlock()->size();
  }

  inline size_t
  getNumFibDetailSubscribers() {
    return fibDetailSubscribers_.wlock()->size();
  }

  //
  // API to cleanup private variables
  //
  inline void
  cleanupPendingLongPollReqs() {
    longPollReqs_->clear();
  }

  /* Coroutine APIs */
 public:
#if FOLLY_HAS_COROUTINES
  folly::coro::Task<void> co_setKvStoreKeyVals(
      std::unique_ptr<thrift::KeySetParams> setParams,
      std::unique_ptr<std::string> area) override;

  folly::coro::Task<std::unique_ptr<thrift::SetKeyValsResult>>
  co_setKvStoreKeyValues(
      std::unique_ptr<thrift::KeySetParams> setParams,
      std::unique_ptr<std::string> area) override;

  folly::coro::Task<std::unique_ptr<std::string>> co_getMyNodeName() override;

  folly::coro::Task<std::unique_ptr<thrift::Publication>>
  co_getKvStoreKeyValsArea(
      std::unique_ptr<std::vector<std::string>> filterKeys,
      std::unique_ptr<std::string> area) override;

  folly::coro::Task<std::unique_ptr<thrift::Publication>> co_getKvStoreKeyVals(
      std::unique_ptr<std::vector<std::string>> filterKeys) override;

  folly::coro::Task<std::unique_ptr<thrift::Publication>>
  co_getKvStoreKeyValsFilteredArea(
      std::unique_ptr<thrift::KeyDumpParams> filter,
      std::unique_ptr<std::string> area) override;

  folly::coro::Task<std::unique_ptr<thrift::Publication>>
  co_getKvStoreHashFilteredArea(
      std::unique_ptr<thrift::KeyDumpParams> filter,
      std::unique_ptr<std::string> area) override;

  folly::coro::Task<std::unique_ptr<thrift::Publication>>
  co_getKvStoreHashFiltered(
      std::unique_ptr<thrift::KeyDumpParams> filter) override;

  folly::coro::Task<std::unique_ptr<::std::vector<thrift::KvStoreAreaSummary>>>
  co_getKvStoreAreaSummary(
      std::unique_ptr<std::set<std::string>> selectAreas) override;

  folly::coro::Task<std::unique_ptr<thrift::PeersMap>> co_getKvStorePeersArea(
      std::unique_ptr<std::string> area) override;

  folly::coro::Task<std::unique_ptr<thrift::Publication>>
  co_getKvStoreKeyValsFiltered(
      std::unique_ptr<thrift::KeyDumpParams> filter) override;

  folly::coro::Task<std::unique_ptr<thrift::PeersMap>> co_getKvStorePeers()
      override;
#endif

 private:
  // returns the single area name configured for this node or throws if not
  // eaxclty 1 area is configured
  std::unique_ptr<std::string> getSingleAreaOrThrow(std::string const& caller);

  void processPublication(thrift::Publication&& pub);
  void authorizeConnection();
  void closeKvStorePublishers();
  void closeFibPublishers();

  const std::string nodeName_;
  const std::unordered_set<std::string> acceptablePeerCommonNames_;

  // Pointers to Open/R modules
  OpenrEventBase* ctrlEvb_{nullptr};
  Decision* decision_{nullptr};
  Fib* fib_{nullptr};
  KvStore<thrift::OpenrCtrlCppAsyncClient>* kvStore_{nullptr};
  LinkMonitor* linkMonitor_{nullptr};
  Monitor* monitor_{nullptr};
  PersistentStore* configStore_{nullptr};
  PrefixManager* prefixManager_{nullptr};
  Spark* spark_{nullptr};
  std::shared_ptr<const Config> config_;
  Dispatcher* dispatcher_{nullptr};

  // Publisher token (monotonically increasing) for all publishers
  std::atomic<int64_t> publisherToken_{0};

  // Active kvstore snoop publishers
  folly::Synchronized<
      std::unordered_map<int64_t, std::unique_ptr<KvStorePublisher>>>
      kvStorePublishers_;

  // Active Fib streaming publishers
  folly::Synchronized<std::unordered_map<
      int64_t,
      apache::thrift::ServerStreamPublisher<thrift::RouteDatabaseDelta>>>
      fibPublishers_;

  // Active Fib Detail streaming publishers
  folly::Synchronized<std::unordered_map<int64_t, FibStreamSubscriber>>
      fibDetailSubscribers_;

  // pending longPoll requests from clients, which consists of
  // 1). promise; 2). timestamp when req received on server
  std::atomic<int64_t> pendingRequestId_{0};
  folly::Synchronized<std::unordered_map<
      std::string /* area */,
      std::unordered_map<int64_t, std::pair<folly::Promise<bool>, int64_t>>>>
      longPollReqs_;

  // fiber task future hold for kvStore update, fib update reader's
  std::vector<folly::Future<folly::Unit>> workers_;
}; // class OpenrCtrlHandler
} // namespace openr
