/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

namespace cpp openr.thrift
namespace cpp2 openr.thrift
namespace go openr.OpenrCtrl
namespace py openr.OpenrCtrl
namespace py3 openr.thrift
namespace php Openr
namespace lua openr.OpenrCtrl
namespace rust openr_thrift
namespace wiki Open_Routing.Thrift_APIs.OpenrCtrl

include "openr/if/Network.thrift"
include "openr/if/KvStore.thrift"
include "openr/if/OpenrConfig.thrift"
include "openr/if/Types.thrift"
include "thrift/annotation/thrift.thrift"

exception OpenrError {
  @thrift.ExceptionMessage
  1: string message;
}

struct NodeAndArea {
  1: string node;
  2: string area;
}

enum StreamSubscriberType {
  KVSTORE = 0,
  FIB = 1,
}

//
// Prefix Manager data structures
//

enum RouteFilterType {
  PREFILTER_ADVERTISED = 0,
  POSTFILTER_ADVERTISED = 1,
  REJECTED_ON_ADVERTISE = 2,
}

struct AdvertisedRoute {
  1: Network.PrefixType key;
  2: Types.PrefixEntry route;
  // Store the policy name that accepted the prefix if an area policy is configered.
  // nullopt otherwise.
  // Get populated in getAreaAdvertisedRoutes()
  3: optional string hitPolicy;
  // Min igp of the route (for the immediate area)
  4: i32 igpCost;
}

struct AdvertisedRouteDetail {
  1: Network.IpPrefix prefix;
  2: Network.PrefixType bestKey;
  3: list<Network.PrefixType> bestKeys;
  4: list<AdvertisedRoute> routes;
}

struct AdvertisedRouteFilter {
  1: optional list<Network.IpPrefix> prefixes;
  2: optional Network.PrefixType prefixType;
}

/**
* Subscriber Info attributes
*/
struct StreamSubscriberInfo {
  // Unique id for subscriber
  1: i64 subscriber_id;
  // Subscription time in msecs
  2: i64 uptime;
  // Time that the last message was sent
  3: i64 last_msg_sent_time;
  // Total number of messages streamed
  4: i64 total_streamed_msgs;
}

//
// Decision data structures
//

struct ReceivedRoute {
  1: NodeAndArea key;
  2: Types.PrefixEntry route;
}

struct ReceivedRouteDetail {
  1: Network.IpPrefix prefix;
  2: NodeAndArea bestKey;
  3: list<NodeAndArea> bestKeys;
  4: list<ReceivedRoute> routes;
}

struct ReceivedRouteFilter {
  1: optional list<Network.IpPrefix> prefixes;
  2: optional string nodeName;
  3: optional string areaName;
}

struct AdjacenciesFilter {
  1: set<string> selectAreas;
}

//
// RIB Policy related data structures
//

/**
 * Matcher selects the routes. As of now supports selecting specified routes,
 * but can be expanded to select route by tags as well.
 *
 * - Atleast one criteria must be specified for selection
 * - `AND` operator is assumed for selecting on multiple attributes.
 * - setting criteria to none will skip the matching
 */
struct RibRouteMatcher {
  1: optional list<Network.IpPrefix> prefixes;

  // Select route based on the tag. Specifying multiple tag match on any
  2: optional list<string> tags;
}

/**
 * `set weight <>` action for RibRoute
 */
struct RibRouteActionWeight {
  // Default weight for the next-hops with no a neighbor not in the map.
  // Usually next-hops from static routes (external next-hops)
  2: i32 default_weight;

  // Area name to weight mapping (internal next-hops)
  3: map<string, i32> area_to_weight;

  // Neighbor device name to weight mapping (internal next-hops)
  4: map<string, i32> neighbor_to_weight;
}

/**
 * Captures information for route transform. So far supports only weight
 * manipulation, but in future we can add more `set_<>` action
 * e.g. `set_admin_distance`
 *
 * - Atleast one `set_` statement must be specified
 * - All provided `set_` statements are applied at once
 */
struct RibRouteAction {
  1: optional RibRouteActionWeight set_weight;
}

/**
 * RibPolicyStatement express a single `match` and `action`
 * based on forseable need for route manipulation. However more sophisticated
 * matching and transformation can be specified to modify multiple route
 * attributes in granular way.
 */
struct RibPolicyStatement {
  // Just for reference and understanding. Code doesn't use it in any way.
  1: string name;

  // Select routes to be transformed
  2: RibRouteMatcher matcher;

  // Transform operation for a single route object
  3: RibRouteAction action;

  // Counter Id assigned to the routes that fit the RibPolicyStatement matcher
  4: optional Network.RouteCounterID counterID;
}

/**
 * Represents a RibPolicy. The intent of RibPolicy is to perform the route
 * modifications on computed routes before programming. The technique provides
 * ability to influence the forwarding decision of distributed computation with
 * central knowledge (e.g. load-aware weight balancing among the areas) for
 * non-oblivious routing scheme.
 *
 * `RibPolicy` is intends to be set via RPC API and is not supported via Config
 * primarily because of its dynamic nature (ttl_secs). Read more about it below.
 */
struct RibPolicy {
  // List of policy statements. Each statement can select routes and can
  // transform it. The statements are applied in order they're specified. The
  // first successful match/action will terminate the further policy processing.
  // Different `action` can be specified for different set of routes (matchers).
  1: list<RibPolicyStatement> statements;

  // Number of seconds this policy is valid for. The policy must be refreshed or
  // updated within `ttl_secs`. If not then policy will become in-effective.
  // Policy is not preserved across restarts. Hence then external agent setting
  // this policy should detect Open/R restart within `Initial Hold Time` (30s)
  // and update the policy within the same window. No routes will be updated in
  // HW within initial hold time window, which ensures the route programmed
  // according to old policy will remain in effect until initial hold time
  // expires.
  // It is recommended to refresh policy every 20 seconds (< hold time).
  2: i32 ttl_secs;
}

/*
 * UnicastRouteDetail includes additional information from UnicastRoute which
 * is not downloaded to FibService
 */
struct UnicastRouteDetail {
  @thrift.Mixin
  1: Network.UnicastRoute unicastRoute;
  2: optional Types.PrefixEntry bestRoute;
}

/*
 * MplsRouteDetail includes additional information from MplsRoute which
 * is not downloaded to FibService
 */
struct MplsRouteDetail {
  @thrift.Mixin
  1: Network.MplsRoute mplsRoute;
}

/*
 * RouteDatabaseDetail includes additional information from RouteDatabase which
 * is not downloaded to FibService
 */
struct RouteDatabaseDetail {
  1: string thisNodeName;
  2: list<UnicastRouteDetail> unicastRoutes;
  3: list<MplsRouteDetail> mplsRoutes;
}

/*
 * RouteDatabaseDeltaDetail includes additional information from
 * RouteDatabaseDelta which is not downloaded to FibService
 */
struct RouteDatabaseDeltaDetail {
  1: list<UnicastRouteDetail> unicastRoutesToUpdate;
  2: list<Network.IpPrefix> unicastRoutesToDelete;
  3: list<MplsRouteDetail> mplsRoutesToUpdate;
  4: list<i32> mplsRoutesToDelete;
}

/**
 * Thrift service - exposes RPC APIs for interaction with all of Open/R's
 * modules.
 */
service OpenrCtrl extends KvStore.KvStoreService {
  //
  // Config APIs
  //

  /**
   * get string config
   */
  string getRunningConfig();

  /**
   * get config in thrift
   */
  OpenrConfig.OpenrConfig getRunningConfigThrift();

  /**
   * Load file config and do validation. Throws exception upon error.
   * Return - loaded config content.
   *
   * NOTE: json seriliazer ommit extra fields.
   * loaded content is returned so user could verify the difference between
   * file content and loaded content
   */
  string dryrunConfig(1: string file) throws (1: OpenrError error);

  /**
   * Retrieve drain state including:
   *  1) node drain state
   *  2) interface drain state
   */
  Types.OpenrDrainState getDrainState(1: string nodeName);

  //
  // OpenR initialization APIs
  //

  /**
   * Get the mapping of <initialization event, duration in millisecond since
   * OpenR starts>.
   */
  map<KvStore.InitializationEvent, i64> getInitializationEvents();

  /**
   * OpenR initialization is converged or not.
   */
  bool initializationConverged();

  /**
   * Duration of initialization in milliseconds; Throws std::invalid_argument()
   * if OpenR initialization is not completed yet.
   */
  i64 getInitializationDurationMs();

  //
  // PrefixManager APIs
  //

  /**
   * Advertise or Update prefixes
   */
  void advertisePrefixes(1: list<Types.PrefixEntry> prefixes) throws (
    1: OpenrError error,
  );

  /**
   * Withdraw previously advertised prefixes. Only relevant attributes for this
   * operation are `prefix` and `type`
   */
  void withdrawPrefixes(1: list<Types.PrefixEntry> prefixes) throws (
    1: OpenrError error,
  );

  /**
   * Withdraw prefixes in bulk by type (aka client-id)
   */
  void withdrawPrefixesByType(1: Network.PrefixType prefixType) throws (
    1: OpenrError error,
  );

  /**
   * Sync prefixes by type. This operation set the new state for given type.
   * PrefixType specified in API parameter must match with individual
   * PrefixEntry object
   */
  void syncPrefixesByType(
    1: Network.PrefixType prefixType,
    2: list<Types.PrefixEntry> prefixes,
  ) throws (1: OpenrError error);

  /**
   * Get all prefixes being advertised
   * @deprecated - use getAdvertisedRoutes() instead
   */
  list<Types.PrefixEntry> getPrefixes() throws (1: OpenrError error);

  /**
   * Get prefixes of specific types
   */
  list<Types.PrefixEntry> getPrefixesByType(
    1: Network.PrefixType prefixType,
  ) throws (1: OpenrError error);

  //
  // Route APIs
  //

  /**
   * Get routes that current node is advertising. Filter parameters if specified
   * will follow `AND` operator
   */
  list<AdvertisedRouteDetail> getAdvertisedRoutes();
  list<AdvertisedRouteDetail> getAdvertisedRoutesFiltered(
    1: AdvertisedRouteFilter filter,
  ) throws (1: OpenrError error);

  /**
   * For given area, show pre/post policy advertised routes.
   */
  list<AdvertisedRoute> getAreaAdvertisedRoutes(
    1: string area,
    2: RouteFilterType routeFilterType,
  ) throws (1: OpenrError error);
  list<AdvertisedRoute> getAreaAdvertisedRoutesFiltered(
    1: string area,
    2: RouteFilterType routeFilterType,
    3: AdvertisedRouteFilter filter,
  ) throws (1: OpenrError error);

  /**
   * For node, show pre/post origination policy routes.
   */
  list<AdvertisedRoute> getAdvertisedRoutesWithOriginationPolicy(
    1: RouteFilterType routeFilterType,
    2: AdvertisedRouteFilter filter,
  ) throws (1: OpenrError error);

  /**
   * Get received routes, aka Adjacency RIB. Filter parameters if specified will
   * follow `AND` operator
   */
  list<ReceivedRouteDetail> getReceivedRoutes();
  list<ReceivedRouteDetail> getReceivedRoutesFiltered(
    1: ReceivedRouteFilter filter,
  ) throws (1: OpenrError error);

  /**
   * Get route database of the current node. It is retrieved from FIB module.
   */
  Types.RouteDatabase getRouteDb() throws (1: OpenrError error);

  /**
   * Get route detailed database of the current node. It is retrieved from FIB module.
   */
  RouteDatabaseDetail getRouteDetailDb() throws (1: OpenrError error);

  /**
   * Get route database from decision module. Since Decision has global
   * topology information, any node can be retrieved.
   *
   * NOTE: Current node's routes are returned if `nodeName` is empty.
   */
  Types.RouteDatabase getRouteDbComputed(1: string nodeName) throws (
    1: OpenrError error,
  );

  /**
   * Get a list of active stream subscribers
   */
  list<StreamSubscriberInfo> getSubscriberInfo(1: i64 type);

  /**
   * Get unicast routes after applying a list of prefix filter.
   * Perform longest prefix match for each input filter among the prefixes
   * in from FIB module.
   * Return all unicast routes if the input list is empty.
   */
  list<Network.UnicastRoute> getUnicastRoutesFiltered(
    1: list<string> prefixes,
  ) throws (1: OpenrError error);

  /**
   * Get all unicast routes of the current node, retrieved from FIB module.
   */
  list<Network.UnicastRoute> getUnicastRoutes() throws (1: OpenrError error);

  /**
   * Get Mpls routes after applying a list of prefix filter.
   * Return all Mpls routes if the input list is empty.
   */
  list<Network.MplsRoute> getMplsRoutesFiltered(1: list<i32> labels) throws (
    1: OpenrError error,
  );

  /**
   * Get all Mpls routes of the current node, retrieved from FIB module.
   */
  list<Network.MplsRoute> getMplsRoutes() throws (1: OpenrError error);

  /**
   * Get all originated prefixes of the current node, retrieved from PrefixManager.
   */
  list<Types.OriginatedPrefixEntry> getOriginatedPrefixes() throws (
    1: OpenrError error,
  );

  //
  // Performance stats APIs
  //

  /**
   * Get latest performance events. Useful for debugging delays and performance
   * of Open/R.
   */

  Types.PerfDatabase getPerfDb() throws (1: OpenrError error);

  //
  // Decision APIs
  //

  /**
   * Get default area adjacency databases of all nodes, representing default
   * area topology from Decision module. This represents currently active nodes
   * (includes bi-directional check)
   * only selects default area adj DBs. Deprecated, perfer
   * getDecisionAdjacenciesFiltered()
   */
  Types.AdjDbs getDecisionAdjacencyDbs() throws (1: OpenrError error);

  /**
   * Get adjacency databases of all nodes. NOTE: for ABRs, there can be more
   * than one AdjDb for a node (one per area)
   * (includes bi-directional check)
   *
   * DEPRECATED. Perfer getDecisionAreaAdjacenciesFiltered to return the areas as well.
   */
  list<Types.AdjacencyDatabase> getDecisionAdjacenciesFiltered(
    1: AdjacenciesFilter filter,
  ) throws (1: OpenrError error);

  /**
   * Get map<area_name, list<adjacency databases>> of all nodes.
   * NOTE: for ABRs, there can be more than one AdjDb for a node (one per area).
   * (includes bi-directional check)
   */
  map<string, list<Types.AdjacencyDatabase>> getDecisionAreaAdjacenciesFiltered(
    1: AdjacenciesFilter filter,
  ) throws (1: OpenrError error);

  /**
   * Long poll API to get KvStore
   * Will return true/false with our own KeyVal snapshot provided
   */
  bool longPollKvStoreAdjArea(
    1: string area,
    2: KvStore.KeyVals snapshot,
  ) throws (1: OpenrError error);

  // Deprecated, prefer API specifying area
  // TODO, remove once EBB has transition away from this
  bool longPollKvStoreAdj(1: KvStore.KeyVals snapshot) throws (
    1: OpenrError error,
  );

  //
  // LinkMonitor APIs
  //

  /**
   * Commands to set/unset overload bit. If overload bit is set then the node
   * will not do any transit traffic. However node will still be reachable in
   * the network from other nodes.
   */
  void setNodeOverload() throws (1: OpenrError error);
  void unsetNodeOverload() throws (1: OpenrError error);

  /**
   * Command to set/unset overload bit for interface. If overload bit is set
   * then no transit traffic will pass through the interface which is equivalent
   * to hard drain on the interface.
   */
  void setInterfaceOverload(1: string interfaceName) throws (
    1: OpenrError error,
  );
  void unsetInterfaceOverload(1: string interfaceName) throws (
    1: OpenrError error,
  );

  /**
   * Command to override metric for adjacencies over specific interfaces. This
   * can be used to emulate soft-drain of interfaces by using higher metric
   * value for link.
   *
   * Request must have valid `interfaceName` and `overrideMetric` values.
   */
  void setInterfaceMetric(
    1: string interfaceName,
    2: i32 overrideMetric,
  ) throws (1: OpenrError error);
  void unsetInterfaceMetric(1: string interfaceName) throws (
    1: OpenrError error,
  );

  /**
   * Command to enable/disable static metric increment for all interfaces in the node.
   * It will add a static value `metricIncrementVal` to the existing metric on all interfaces.
   *
   *  Request must have a valid increment metric value.
   */
  void setNodeInterfaceMetricIncrement(1: i32 metricIncrementVal) throws (
    1: OpenrError error,
  );
  void unsetNodeInterfaceMetricIncrement() throws (1: OpenrError error);

  /**
   * (TO BE deprecated by the newer version that supports multiple interfaces)
   * Command to enable/disable static metric increment for an interface.
   * It will add a static value `metricIncrementVal` to the existing metric on that interface.
   *
   *  Request must have a valid increment metric value.
   */
  void setInterfaceMetricIncrement(
    1: string interfaceName,
    2: i32 metricIncrementVal,
  ) throws (1: OpenrError error);

  void unsetInterfaceMetricIncrement(1: string interfaceName) throws (
    1: OpenrError error,
  );

  /**
   * Command to enable/disable static metric increment for interface(s).
   * It will add a static value `metricIncrementVal` to the existing metric on that interface.
   *
   *  Request must have a valid increment metric value.
   */
  void setInterfaceMetricIncrementMulti(
    1: list<string> interfaceNames,
    2: i32 metricIncrementVal,
  ) throws (1: OpenrError error);

  void unsetInterfaceMetricIncrementMulti(
    1: list<string> interfaceNames,
  ) throws (1: OpenrError error);

  /**
   * Command to override metric for specific adjacencies. Request must have
   * valid interface name, adjacency-node name and override metric value.
   */
  void setAdjacencyMetric(
    1: string interfaceName,
    2: string adjNodeName,
    3: i32 overrideMetric,
  ) throws (1: OpenrError error);
  void unsetAdjacencyMetric(
    1: string interfaceName,
    2: string adjNodeName,
  ) throws (1: OpenrError error);

  /**
   * Get the current link status information
   */
  Types.DumpLinksReply getInterfaces() throws (1: OpenrError error);

  /**
   * Get the current adjacencies information, only works for nodes with one
   * configured area.
   *
   * DEPRECATED. Perfer getLinkMonitorAreaAdjacenciesFiltered to return the areas as well.
   */
  Types.AdjacencyDatabase getLinkMonitorAdjacencies() throws (
    1: OpenrError error,
  );

  /**
   * Get the current adjacencies information, provide set of areas to get
   * adjancecy databases for. Providing an empty set will return a DB for
   * all configured areas
   *
   * DEPRECATED. Perfer getLinkMonitorAreaAdjacenciesFiltered to return the areas as well.
   */
  list<Types.AdjacencyDatabase> getLinkMonitorAdjacenciesFiltered(
    1: AdjacenciesFilter filter,
  ) throws (1: OpenrError error);

  /**
   * Get the map<area, adjacencies> information, provide set of areas to get
   * adjancecy databases for. Providing an empty set will return a DB for
   * each of all configured areas.
   */
  map<
    string,
    list<Types.AdjacencyDatabase>
  > getLinkMonitorAreaAdjacenciesFiltered(1: AdjacenciesFilter filter) throws (
    1: OpenrError error,
  );

  /**
   * Command to request OpenR version
   */
  Types.OpenrVersions getOpenrVersion() throws (1: OpenrError error);

  /**
   * Command to request build information
   * @deprecated - instead use getRegexExportedValues("build.*") API
   */
  Types.BuildInfo getBuildInfo() throws (1: OpenrError error);

  //
  // PersistentStore APIs (query / alter dynamic configuration)
  //

  /**
   * Set new config key - you will never need to use it
   * NOTE: This API should only be accessible from local node
   */
  void setConfigKey(1: string key, 2: binary value) throws (
    1: OpenrError error,
  );

  /**
   * Erase key from config
   * NOTE: This API should only be accessible from local node
   */
  void eraseConfigKey(1: string key) throws (1: OpenrError error);

  /**
   * Get config key
   */
  binary getConfigKey(1: string key) throws (1: OpenrError error);

  //
  // Spark APIs
  //

  /*
   * Send out SparkHelloMsg with `restarting` flag
   * indicating graceful restart usage
   */
  void floodRestartingMsg() throws (1: OpenrError error);

  /*
   * Get info for Spark neighors
   */
  list<Types.SparkNeighbor> getNeighbors() throws (1: OpenrError error);

  //
  //  Monitor APIs (get log events)
  //

  // Get log events
  list<string> getEventLogs() throws (1: OpenrError error);

  // Get Openr Node Name
  string getMyNodeName();

  //
  // RibPolicy
  //

  /**
   * Set RibPolicy.
   *
   * @throws OpenrError if rib-policy is not valid or enabled via configuration
   */
  void setRibPolicy(1: RibPolicy ribPolicy) throws (1: OpenrError error);

  /**
   * Get RibPolicy.
   *
   * @throws OpenrError if rib-policy is not enabled via configuration or is
   *         not set previously
   */
  RibPolicy getRibPolicy() throws (1: OpenrError error);

  /**
  * Clear RibPolicy.
  * @throws OpenrError if rib-policy is not enabled via configuration or is
  *         not set previously.
  */
  void clearRibPolicy() throws (1: OpenrError error);

  //
  // Dispatcher APIs
  //

  /**
   * Get filters for each of the subscribers of Dispatcher
   */
  list<list<string>> getDispatcherFilters();
}
