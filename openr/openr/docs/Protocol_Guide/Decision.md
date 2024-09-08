# Decision - Route Computation

## Introduction

---

`Decision` is the module that computes the routing table (IPv4, IPv6, MPLS). It
uses Topology and Reachability information received from `KvStore` in the
computation. Outputs RIB, aka `Routing Information Base`, which will get
programmed and/or redistributed.

## Inter Module Communication

---

![Decision Intermodule Communication](https://user-images.githubusercontent.com/51382140/102831445-b70f6580-43a0-11eb-8a8e-190df6c13ec5.png)

- `[Consumer] ReplicateQueue<Publication>`: read publications (updates) from the
  `KvStore` to learn Topology/Reachability information and kvStoreSynced signal.

- `[Consumer] ReplicateQueue<thrift::RouteDatabaseDelta>`: static route updates
  written by `PrefixManager`. The route delta will be applied to the RIB output
  by `Decision` module

- `[Producer] ReplicateQueue<DecisionRouteUpdate>`: the RIB delta consumed by
  the `Fib` module for programming to the underlying platform and the
  `PrefixManager` module for route redistribution

## General Workflow

---

Below diagram describes two main things

- The Flow of information in the context of Decision module and
- Route computation flow-chart

![openr-route-computation-flow](https://user-images.githubusercontent.com/1482609/89572763-70004980-d7de-11ea-8c07-a8b3e446ef40.png)

## Deep Dive

---

### Storage

Decision models two major data-structures namely `PrefixState`, and `LinkState`.
This facilitates the route computation with less code complexity.

#### PrefixState aka Adjacency RIB

PrefixState stores all received route advertisements from all the nodes in the
network. This is also termed or known as `Adjacency RIB` (aka ADJ RIB) in
routing terminology. The route advertisement is keyed by a tuple containing
`Destination Prefix`, `Originating Node`, and `Originating Area`. This uniquely
identifies a route advertisement.

#### LinkState aka Topology

As the name refers, the LinkState stores all received link-state updates from
nodes in the network. This represents the network topology (nodes & links). The
topology is maintained for each area separately. The link-state update uniquely
identified by `Area ID`, and `Node Name`.

LinkState offers much more structured data and APIs for network topology on the
top of received data from KvStore. This hides away a good amount of complexity
from route computation code. e.g.

- Bi-directional check for the link. Both nodes should report each other as
  their neighbors
- Incremental update of topology
- Dijkstra Implementation (aka Shortest Path Computation)

### Computing Routes

Decision computes two types of routes, Unicast (aka IPv4 or IPv6), and MPLS.
Below we describe the steps involved for both route types.

#### Unicast Routes

Flow Diagram above captures the gist of the route computation steps for unicast
routes. The route computation steps described below (and in the diagram) are in
the context of a single route.

1. Create a list of all received route advertisements
2. Perform best route selection
3. Exit if the self node is the best route. Avoid programming self-advertised
   routes
4. Based on Algorithm type (SPF or K-SPF), compute paths
5. Based on Forwarding type (IP or MPLS), compute next-hops
6. Exit if the min next-hop criteria is not met
7. Apply RIB policy
8. Send out route notification

#### MPLS Routes

Open/R implement source routing capabilities on the lines of
[Segment Routing Architecture](https://tools.ietf.org/html/draft-ietf-spring-segment-routing-15)
and with MPLS data plane support.

The Decision module computes the MPLS routes, aka `Label Routes`, for two types
of labels `Node Label`, and `Adjacency Label`.

`Adjacency Label` is a label associated with an established Adjacency with the
neighbor. This label is unique in a given node and hence only has the local
significance on this node. If a packet is received with this label, it will be
popped and forward towards the neighbor of the adjacency.

`Node Label` is a label associated with a node. It is unique within a network
and has a global meaning. If a packet is received with this label, it is
forwarded towards the node owning this label over the shortest ECMP paths. The
label is preserved until it reaches the destination node.

Computed label routes leverage Penultimate Hop Popping (PHP) for pop & forward
instruction to avoid duplicate lookup of a packet when it reaches the
destination.

For more details refer to
[Source Routing in Open/R](../Features/SourceRouting.md)

### Route Notifications

After the route computation step, Decision computes the route delta for both
Unicast and MPLS routes. The route delta is then broadcasted to all listeners
via `ReplicateQueue`.

Note that only changes are published, hence all listeners must retain a snapshot
and update snapshot as changes arrive based on their requirements. Also, we must
create a `Reader` of the `ReplicateQueue` before decision module is started to
ensure there is no loss of route update.

### Miscellaneous Features

#### Event Dampening

We implemented simple event dampening - i.e. hold SPF runs on first received
link state update to see if we can collect more before doing the full
computation run. This timer could use exponential back-off to allow for catching
more events under heavy network churn. In practice, this helps save a lot of CPU
under heavy network churn.

> NOTE: we assume all links are point-to-point, no multi-access networks are
> being considered. This simplifies many things, e.g. there is no need to
> consider pseudo-nodes to develop special flooding schemes for shared segments.

### More Reading

- [Basic Specification for IP Fast Reroute: Loop-Free Alternates](https://tools.ietf.org/html/rfc5286)
- [Segment Routing Architecture](https://tools.ietf.org/html/draft-ietf-spring-segment-routing-15)
