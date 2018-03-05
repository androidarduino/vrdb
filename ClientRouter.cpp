#include "ClientRouter.h"

Router::Router() {
	// get routing tables from central
}

int Router::nodeDied(Node node) {
	// find the died node, mark it died
	// delete node from all routing partitions
	// notify central about the finding
}

int Router::nodeAlive(Node node) {
	// find the node, mark it alive
	// if not found, create a new node and add it to _nodes
}

int Router::pmapChanged(string pmap) {
	// merge the pmap to current pmap
}

int sync() {
	// sync pmap from central
	// send my time stamps
	// get back all new changes
	// merge them into pmaps
}

long exceptionRoute(string key, string scope) {
	// use map to find hot keys, if not exist, return 0
	// hot key maps to pmap number -1 to - 1000000
	if (_topKeys.find(key) == _topKeys.end()) {
		return 0;
	} else {
		return 0 - _topKeys[key];
	}
}

long moreHashRoute(string key, string scope) {
	// use bloom filter to find whether key is in hot key list or scope list
	if (!_hotKeys.has(key) && !_hotScope.has(scope)) {
		return 0;
	} else {
		return 0 - 100000 - hash2(key);
	}
}

long hashRoute(string key) {
	// normal hash routing
	return hash(key);
secondHash}
