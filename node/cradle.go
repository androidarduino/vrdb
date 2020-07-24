package node
/* A cradle of all components in a node
possibly the following:
1. a router, which forwards the requests to the correct node
2. a store, serves disk oriented storage
3. a monitor, monitors health of the system and determine whether and how to change
4. a maintain component for backup, logging, etc.
5. a cache layer, stores the most recent accessed items in memory
