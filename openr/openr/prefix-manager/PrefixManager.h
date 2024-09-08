/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <folly/IPAddress.h>
#include <folly/futures/Future.h>
#include <folly/gen/Base.h>

#include <openr/common/AsyncThrottle.h>
#include <openr/common/OpenrEventBase.h>
#include <openr/common/Types.h>
#include <openr/common/Util.h>
#include <openr/config/Config.h>
#include <openr/decision/RouteUpdate.h>
#include <openr/if/gen-cpp2/Network_types.h>
#include <openr/if/gen-cpp2/Types_types.h>
#include <openr/messaging/ReplicateQueue.h>
#include <openr/policy/PolicyManager.h>

namespace openr {

namespace detail {

class PrefixManagerPendingUpdates {
 public:
  explicit PrefixManagerPendingUpdates() : changedPrefixes_{} {}

  void
  clear() {
    changedPrefixes_.clear();
  }

  size_t
  size() {
    return changedPrefixes_.size();
  }

  const std::unordered_set<folly::CIDRNetwork>&
  getChangedPrefixes() {
    return changedPrefixes_;
  }

  bool
  hasPrefix(const folly::CIDRNetwork& prefix) {
    return changedPrefixes_.count(prefix) > 0;
  }

  void
  addPrefixChange(const folly::CIDRNetwork& prefix) {
    changedPrefixes_.insert(prefix);
  }

 private:
  // Track prefixes that have changed within this batch
  // ATTN: this collection contains:
  //  - newly added prefixes
  //  - updated prefixes
  //  - prefixes being withdrawn
  std::unordered_set<folly::CIDRNetwork> changedPrefixes_{};
};

} // namespace detail

class PrefixManager final : public OpenrEventBase {
 public:
  PrefixManager(
      // producer queue
      messaging::ReplicateQueue<DecisionRouteUpdate>& staticRouteUpdatesQueue,
      messaging::ReplicateQueue<KeyValueRequest>& kvRequestQueue,
      messaging::ReplicateQueue<thrift::InitializationEvent>&
          initializationEventQueue,
      // consumer queue
      messaging::RQueue<KvStorePublication> kvStoreUpdatesQueue,
      messaging::RQueue<PrefixEvent> prefixUpdatesQueue,
      messaging::RQueue<DecisionRouteUpdate> fibRouteUpdatesQueue,
      // config
      std::shared_ptr<const Config> config);

  ~PrefixManager();

  // disable copying
  PrefixManager(PrefixManager const&) = delete;
  PrefixManager& operator=(PrefixManager const&) = delete;

  /*
   * Public API for PrefixManager operations:
   *
   * Write APIs - will schedule syncKvStoreThrottled_ to update kvstore,
   * @return true if there are changes else false
   *  - add prefixes
   *  - withdraw prefixes
   *  - withdraw prefixes by type
   *  - sync prefixes by type: replace all prefixes of @type w/ @prefixes
   *
   *
   * Read APIs - dump internal prefixDb
   *  - dump all prefixes
   *  - dump all prefixes by type
   */
  folly::SemiFuture<bool> advertisePrefixes(
      std::vector<thrift::PrefixEntry> prefixes);

  folly::SemiFuture<bool> withdrawPrefixes(
      std::vector<thrift::PrefixEntry> prefixes);

  folly::SemiFuture<bool> withdrawPrefixesByType(thrift::PrefixType prefixType);

  folly::SemiFuture<bool> syncPrefixesByType(
      thrift::PrefixType prefixType, std::vector<thrift::PrefixEntry> prefixes);

  folly::SemiFuture<std::unique_ptr<std::vector<thrift::PrefixEntry>>>
  getPrefixes();

  folly::SemiFuture<std::unique_ptr<std::vector<thrift::PrefixEntry>>>
  getPrefixesByType(thrift::PrefixType prefixType);

  folly::SemiFuture<std::unique_ptr<std::vector<thrift::AdvertisedRouteDetail>>>
  getAdvertisedRoutesFiltered(thrift::AdvertisedRouteFilter filter);

  folly::SemiFuture<std::unique_ptr<std::vector<thrift::OriginatedPrefixEntry>>>
  getOriginatedPrefixes();

  /**
   * Helper functinon used in getAreaAdvertisedRoutes()
   * Filter routes with 1. <type> attribute
   */
  void filterAndAddAreaRoute(
      std::vector<thrift::AdvertisedRoute>& routes,
      const std::string& area,
      const thrift::RouteFilterType& routeFilterType,
      std::unordered_map<thrift::PrefixType, PrefixEntry> const& prefixEntries,
      apache::thrift::optional_field_ref<thrift::PrefixType&> const&
          typeFilter);
  /**
   * Util function to convert thrift::OriginatedPrefix to thrift::PrefixEntry
   * @param prefix: the OriginatedPrefix to be converted
   * @param tType: type of this prefix, can be CONFIG or VIP
   */
  thrift::PrefixEntry toPrefixEntryThrift(
      const thrift::OriginatedPrefix& prefix, const thrift::PrefixType& tType);

  /*
   * Dump post filter (policy) routes for given area.
   *
   * @param rejected
   * - true: return a list of accepted post policy routes
   * - false: return a list of rejected routes
   */
  folly::SemiFuture<std::unique_ptr<std::vector<thrift::AdvertisedRoute>>>
  getAreaAdvertisedRoutes(
      std::string areaName,
      thrift::RouteFilterType routeFilterType,
      thrift::AdvertisedRouteFilter filter);

  /**
   * Helper functinon used in getAdvertisedRoutesFiltered()
   * Filter routes with 1. <type> attribute
   */
  static void filterAndAddAdvertisedRoute(
      std::vector<thrift::AdvertisedRouteDetail>& routes,
      apache::thrift::optional_field_ref<thrift::PrefixType&> const& typeFilter,
      folly::CIDRNetwork const& prefix,
      std::unordered_map<thrift::PrefixType, PrefixEntry> const& prefixEntries);

  /*
   * Dump routes from prefixEvent that are subject to origination policy.
   *
   * @param routeFilterType
   * - pre-policy: return all received prefixes before applying origination
   * policy
   * - post-policy: return all accepted prefixes after applying policy
   * - rejected: return all prefixes rejected by policy
   * @param filter
   *    filter prefixes by prefixes and/or prefix-type
   *
   */
  folly::SemiFuture<std::unique_ptr<std::vector<thrift::AdvertisedRoute>>>
  getAdvertisedRoutesWithOriginationPolicy(
      thrift::RouteFilterType routeFilterType,
      thrift::AdvertisedRouteFilter filter);

  /**
   * Helper functinon used in getAdvertisedRoutesFiltered()
   * Filter routes with 1. <type> attribute
   */
  void filterAndAddOriginatedRoute(
      std::vector<thrift::AdvertisedRoute>& routes,
      const thrift::RouteFilterType& routeFilterType,
      std::unordered_map<
          thrift::PrefixType,
          std::pair<PrefixEntry, std::string>> const& prefixEntries,
      apache::thrift::optional_field_ref<thrift::PrefixType&> const&
          typeFilter);

 private:
  // initialize counters
  void initCounters() noexcept;

  // Process thrift publication from KvStore.
  void processPublication(thrift::Publication&& thriftPub);

  /*
   * Private helpers to update `prefixMap_`
   *
   * Called upon:
   * - public write APIs
   * - request from replicate queue
   *
   * modify prefix db and schedule syncKvStoreThrottled_ to update kvstore
   * @return true if the db is modified
   */
  bool advertisePrefixesImpl(
      std::vector<thrift::PrefixEntry>&& tPrefixEntries,
      const std::unordered_set<std::string>& dstAreas,
      const std::optional<std::string>& policyName = std::nullopt);
  bool advertisePrefixesImpl(
      std::vector<PrefixEntry>&& tPrefixEntries,
      const std::unordered_set<std::string>& dstAreas,
      const std::optional<std::string>& policyName = std::nullopt);
  bool advertisePrefixesImpl(
      const std::vector<PrefixEntry>& prefixEntries,
      const std::optional<std::string>& policyName = std::nullopt);
  bool withdrawPrefixesImpl(
      const std::vector<thrift::PrefixEntry>& tPrefixEntries);
  bool withdrawPrefixEntriesImpl(const std::vector<PrefixEntry>& prefixEntries);
  bool withdrawPrefixesByTypeImpl(thrift::PrefixType type);
  bool syncPrefixesByTypeImpl(
      thrift::PrefixType type,
      const std::vector<thrift::PrefixEntry>& tPrefixEntries,
      const std::unordered_set<std::string>& dstAreas,
      const std::optional<std::string>& policyName = std::nullopt);
  std::vector<PrefixEntry> applyOriginationPolicy(
      const std::vector<PrefixEntry>& prefixEntries,
      const std::string& policyName);

  /*
   * Trigger inital prefix sync after all dependent OpenR initialization signals
   * are reveived.
   */
  void triggerInitialPrefixDbSync();

  /*
   * One prefixEntry is ready to be advertised iff
   * - The associated unicast route has been programmed locally.
   */
  bool prefixEntryReadyToBeAdvertised(const PrefixEntry& prefixEntry);

  /*
   * Util function to interact with KvStore to advertise/withdraw prefixes
   * ATTN: syncKvStore() has throttled version `syncKvStoreThrottled_` to
   *       batch processing updates
   */
  void syncKvStore();

  /*
   * [Util function]
   *
   * This is the util function to interact with KvStore. It will
   *  1) advertise into different KvStore specified with different areas.
   *  2) withdraw from the areas which are no longer advertised to.
   */
  void updatePrefixKeysInKvStore(
      const folly::CIDRNetwork& prefix, const PrefixEntry& prefixEntry);

  void deletePrefixKeysInKvStore(
      const folly::CIDRNetwork& prefix,
      DecisionRouteUpdate& routeUpdatesForDecision);
  /*
   * [Util function]
   *
   * Add prefix entry to KvStore(multi-areas)
   *
   * @param: prefix entry object containing prefix and destination areas
   * @return: set of the areas that this prefix will advertise to.
   */
  std::unordered_set<std::string> addKvStoreKeyHelper(const PrefixEntry& entry);

  /*
   * [Util function]
   *
   * Delete prefix entry from KvStore(multi-areas)
   *
   * @param: prefix to be withdrawn
   * @param: areas to be withdrawn from
   */
  void deleteKvStoreKeyHelper(
      const folly::CIDRNetwork& prefix,
      const std::unordered_set<std::string>& deletedArea);

  /*
   * Perform best entry selection among the given prefixTypeToEntry
   */
  std::pair<thrift::PrefixType, const PrefixEntry> getBestPrefixEntry(
      const std::unordered_map<thrift::PrefixType, PrefixEntry>&
          prefixTypeToEntry);

  /*
   * Send static unicast routes for prefix entries of certain type in OpenR
   * initialization process.
   */
  void sendStaticUnicastRoutes(thrift::PrefixType prefixType);

  /*
   * Get route updates of prefixEntry.
   * PrefixEntry with nexthops attr set/reset introduces route add/delete.
   */
  void populateRouteUpdates(
      const folly::CIDRNetwork& prefix,
      const PrefixEntry& prefixEntry,
      DecisionRouteUpdate& routeUpdatesForDecision);

  /*
   * [Route Origination/Aggregation]
   *
   * Methods implement utility function for route origination/aggregation.
   */

  /*
   * Build prefixes for routes read from OpenrConfig.
   * - Read routes from OpenrConfig and stored in `OriginatedPrefixDb_`
   *   for potential advertisement/withdrawn based on min_supporting_route
   *   ref-count.
   * - Publish initial static routes for prefixes with `install_to_fib=true`.
   *   This faciliates the programming of associated routes in initial Fib sync
   *   and the advertisement of these prefixes in initial PrefixDbSync. For
   *   originated prefixes with `minimum_supporting_routes>0`, the nonexistence
   *   of supporting routes will result in the delete of static routes in next
   *   route program iteration.
   */
  void buildOriginatedPrefixes(
      const std::vector<thrift::OriginatedPrefix>& prefixes);

  /*
   * Util function to process programmed route update from Fib and populate
   * aggregates to advertise.
   */
  void aggregatesToAdvertise(const folly::CIDRNetwork& prefix);

  /*
   * Util function to process programmed route update from Fib and populate
   * aggregates to withdraw.
   */
  void aggregatesToWithdraw(const folly::CIDRNetwork& prefix);

  /*
   * Util function to iterate through originatedPrefixDb_ for route
   * advertisement/withdrawn
   */
  void processOriginatedPrefixes();

  // Process Fib route update.
  void processFibRouteUpdates(DecisionRouteUpdate&& fibRouteUpdate);

  // Store programmed routes update from FIB, which are later used to check
  // whether prefixes are ready to be injected into KvStore.
  void storeProgrammedRoutes(const DecisionRouteUpdate& fibRouteUpdates);

  // For one node locating in multiple areas, it should redistribute prefixes
  // received from one area into other areas, performing similar role as border
  // routers in BGP.
  void redistributePrefixesAcrossAreas(DecisionRouteUpdate&& fibRouteUpdate);

  // get all areaIds
  std::unordered_set<std::string> allAreaIds();

  // Record originated prefixes with origination policy name for to be exposed
  // by Cli for debugging purpose
  void storeOriginatedPrefixes(
      std::vector<PrefixEntry> prefixEntries, const std::string& policyName);

  void removeOriginatedPrefixes(
      const std::vector<thrift::PrefixEntry>& prefixEntries);

  void removeOriginatedPrefixes(const std::vector<PrefixEntry>& prefixEntries);

  /*
   * Private variables/Structures
   */

  // this node name
  const std::string nodeId_;

  // Openr config
  std::shared_ptr<const Config> config_;

  // map from area id to area policy
  std::unordered_map<std::string, std::optional<std::string>> areaToPolicy_;

  // queue to publish originated route updates to decision
  messaging::ReplicateQueue<DecisionRouteUpdate>& staticRouteUpdatesQueue_;

  // queue to send key-value update requests to KvStore
  messaging::ReplicateQueue<KeyValueRequest>& kvRequestQueue_;

  // Queue to publish thrift::InitializationEvent to downstream consumer
  messaging::ReplicateQueue<thrift::InitializationEvent>&
      initializationEventQueue_;

  // Throttled version of syncKvStore. It batches up multiple calls and
  // send them in one go!
  std::unique_ptr<AsyncThrottle> syncKvStoreThrottled_;

  // TODO: Merge this with advertiseStatus_.
  // The current prefix db this node is advertising. In-case if multiple entries
  // exists for a given prefix, best-route-selection process would select the
  // ones with the best metric. Lowest prefix-type is used as a tie-breaker for
  // advertising the best selected routes to KvStore.
  std::unordered_map<
      folly::CIDRNetwork,
      std::unordered_map<thrift::PrefixType, PrefixEntry>>
      prefixMap_;

  // For prefixes came from PrefixEvent with an origination policy,
  // store the pre-policy version in originatedPrefixMap_.
  // Used in thrift request getAdvertisedRoutesWithOriginationPolicy().
  std::unordered_map<
      folly::CIDRNetwork,
      std::unordered_map<
          thrift::PrefixType,
          std::pair<PrefixEntry, std::string>>>
      originatedPrefixMap_;

  // the serializer/deserializer helper we'll be using
  apache::thrift::CompactSerializer serializer_;

  /*
   * AdvertiseStatus records following information of one prefix,
   *  1) Areas where one prefix is advertised into. `PrefixManager` advertises
   *    one prefix based on best-route-selection process, and could advertise
   *    into multiple areas. Prefix withdraw process will remove the prefix
   *    from all advertised areas.
   *
   *  2) Published unicast route. `PrefixManager` sends to Decision the
   *    associated unicast routes for the prefixes with nexthops set. Prefix
   *    withdraw process will remove associated unicast routes from Decision.
   */
  struct AdvertiseStatus {
    // Set of areas the prefix was already advertised.
    std::unordered_set<std::string> areas;
    // Unicast route published to Decision for programming.
    std::optional<RibUnicastEntry> publishedRoute;
    // Best entries published to KvStore for distribution
    PrefixEntry advertisedBestEntry;
  };
  std::unordered_map<folly::CIDRNetwork, AdvertiseStatus> advertiseStatus_{};

  // store pending updates from advertise/withdraw operation
  detail::PrefixManagerPendingUpdates pendingUpdates_;

  std::unique_ptr<PolicyManager> policyManager_{nullptr};

  /*
   * [Route Origination/Aggregation]
   *
   * Local-originated prefixes will be advertise/withdrawn from
   * `Prefix-Manager` by calculating ref-count of supporting-
   * route from `Decision`.
   *  --------               ---------
   *           ------------>
   *  Decision               PrefixMgr
   *           <------------
   *  --------               ---------
   */

  // struct to represent local-originiated route
  struct OriginatedRoute {
    // thrift structure read from config
    thrift::OriginatedPrefix originatedPrefix;

    // unicastEntry contains nexthops info
    RibUnicastEntry unicastEntry;

    // supporting routes for this originated prefix
    std::unordered_set<folly::CIDRNetwork> supportingRoutes{};

    // flag indicates is local route has been originated
    bool isAdvertised{false};

    OriginatedRoute(
        const thrift::OriginatedPrefix& originatedPrefix,
        const RibUnicastEntry& unicastEntry,
        const std::unordered_set<folly::CIDRNetwork>& supportingRoutes)
        : originatedPrefix(originatedPrefix),
          unicastEntry(unicastEntry),
          supportingRoutes(supportingRoutes) {}

    /*
     * Util function for route-agg check
     */
    bool
    shouldInstall() const {
      return originatedPrefix.install_to_fib().value_or(false);
    }

    bool
    shouldAdvertise() const {
      const auto& minSupportingRouteCnt =
          *originatedPrefix.minimum_supporting_routes();
      return (not isAdvertised) and
          (supportingRoutes.size() >= minSupportingRouteCnt);
    }

    bool
    shouldWithdraw() const {
      const auto& minSupportingRouteCnt =
          *originatedPrefix.minimum_supporting_routes();
      return isAdvertised and (supportingRoutes.size() < minSupportingRouteCnt);
    }

    bool
    supportingRoutesFulfilled() const {
      return supportingRoutes.size() >=
          *originatedPrefix.minimum_supporting_routes();
    }
  };

  /*
   * prefixes to be originated from prefix-manager
   * ATTN: to support quick information retrieval, cache the mapping:
   *
   *  OriginatedPrefix -> set of FIB prefixEntry(i.e. supporting routes)
   */
  std::unordered_map<folly::CIDRNetwork, OriginatedRoute> originatedPrefixDb_;

  /*
   * prefixes received from OpenR/Fib.
   * ATTN: to avoid loop through ALL entries inside `originatedPrefixes`,
   *       cache the reverse mapping:
   *
   *  FIB prefixEntry -> vector of OriginatedPrefix(i.e. supported routes)
   */
  std::unordered_map<folly::CIDRNetwork, std::vector<folly::CIDRNetwork>>
      ribPrefixDb_;

  /*
   * Prefixes with unicast route already programmed by FIB. For one prefix,
   * PrefixManager needs to make sure the associated unicast route is programmed
   * before advertisement.
   */
  std::unordered_set<folly::CIDRNetwork> programmedPrefixes_;

  /*
   * Set of prefix types for which PrefixManager awaits in OpenR initialization
   * procedure. This would be populated based on config
   * - `RIB` is always added to set. For standalone node or first node brought
   *   up in the network, there will be empty RIB route updates from OpenR/Fib
   *   in initialization procedure.
   * - `BGP` is added if BGP peering is enabled in config.
   * - `VIP` is added if VIP plugin is enabled in config.
   *
   * As we receive the first prefix update request from these types we remove
   * them from this set. Empty set indicates all expected prefix types are
   * initialized. First update from Fib will remove `RIB` from this set (Note
   * Fib sends routes rather than prefixes to PrefixManager, here we leverage
   * the concept of `RIB` for simplicity).
   */
  std::unordered_set<thrift::PrefixType> uninitializedPrefixTypes_{};
}; // PrefixManager

} // namespace openr
