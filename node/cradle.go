package node
/* A cradle of all components in a node
possibly the following:
1. a router, which forwards the requests to the correct node
2. a store, serves disk oriented storage
3. a monitor, monitors health of the system and determine whether and how to change
4. a maintain component for backup, logging, etc.
5. a cache layer, stores the most recent accessed items in memory
6. a connection manager, accepts incoming connections and create new router/stores on demand

A cradle can have multiple nodes, cradle can create, delete, split, merge nodes on the host, serving as a manager for all nodes

There would be only one connection manager, all nodes are connected to it, it will triarge message to the correct node

Cache could be shared among all nodes as well, same as monitor and stores, depends on requirement

*/