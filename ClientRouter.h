#pragma once

#include <string>

using namespace std;

/*
	This class manages a local routing table.
	It will try to route the message to the best target
	When there is any change in targets, it will get notified and updated
	It will also sync with main routing table on the target side
*/

class Router {
	public:
		Router();
		// add routing parameters to the request
		void route(string& request);
		void nodeDied(Node node);
		void pmapChanged(string pmap);
		void syncFrom(Node node);
		void syncTo(Node node);
	protected:
		// directly map a key to a partition using map, for extremly high usage key
		long exceptionRoute(string key);
		// secondary hashing method using bloom filter, routing for hot partitions
		long moreHashRoute(string key);
		// normal hash routing, for general keys
		long hashRoute(string key);

		// three types of partition maps
		PMaps _top, _hot, _normal;

		map<string, long> _topKeys,_topScopes;
		BloomFilter _hotKeys, _hotScopes;
		long hash(string key);
		long hash2(string key);
};

/*
	Provides a mapping from partition number to a chain of nodes.
	It is like (partition number 0 is reserved, indicating no partition found):
		Partition		Nodes
		1				a, b, c
		2				d, e, f
		-1				a, b, c, d, e, f, g, h, i, j, k, l, m, n
		-1000001		a, b, c, d, e, f
	In a typical large cluster, we will have around 20k nodes, 10k partitions,
	so the partition maps will have no more than 10k entries, with average size of < 10 bytes,
	which will take around a few hundreds of kb. The node list will be around the size as well.
	After compression, it could be less than 100k in total, which is syncable around each second.
*/

class PMap {
	public:
		PartitionMap(string pmap);
		Node& partitionToNode(long partition);
		int nodeDied(Node& node);
		int nodeAlive(Node& node);
		long timeStamp;
		int type;
	protected:
		map<long, Node> _nodes;
		map<long, vector<long>> _p2n;
};
