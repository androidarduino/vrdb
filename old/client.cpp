#include "client.h"

Value _getKey(Hash& hash, Mode& mode) {
	// In Passive mode, return value from cache, passively wait for the cache to be updated from host, can not gurantee up to date
	if (mode == Mode::Passive) return getKeyPassive(hash);
	// In Moderate mode, check with one or more hosts to determine data version, update cache if needed, high possibility up to date
	if (mode == Mode::Moderate) return getKeyModerate(hash);
	// In Critical mode, check with the master node of the key for version, up to date gurantee
	if (mode == Mode::Critical) return getKeyCritical(hash);
}

void _publish(Hash& hash, Value& value, Mode& mode) {
	auto item = _cache.getItem(hash);
	// In Passive mode, publish value to any or some of the hosts, leave it to the hosts to spread around, leave consistency problem to the hosts
	if (mode == Mode::Passive) publishPassive(item, value);
	// In Moderate mode, ask the master node for publish version and publish values to host
	if (mode == Mode::Moderate) publishModerate(item, value);
	// In Critical mode, ask the master node to publish data on behalf, wait for the master node's acknowledgement as receipt
	if (mode == Mode::Critical publishCritical(item, value);
}

Value getKeyPassive(Hash& hash) {
	return _cache.get(hash);
}

Value getKeyModerate(Hash& hash) {
	Host& host = findRandomHost();
	auto version = _cache.getVersion(hash);
	try {
		ACK result = host.getIfVersionHigher(hash);
	} catch (...) {
		//process error contacting host
	}
	if (result["empty"])
		return _cache.get(hash);
	else
		return result.getValue();
}

Value getKeyCritical(Hash& hash) {
	try {
		CacheItem& item = _cache.getItem(hash);
		ACK result = item.masterNode.passIfVersionHigher(hash, item.key._version);
	} catch (...) {
		// process error contacting master node
	}
	// if data is small enough and master node is not busy, it will serve the request
	if (!result["redirect"]) return result.getValue();
	// otherwise master node will redirect it to one of the other nodes,reply will arrive from another node, we don't worry about it here
}

void publishPassive(CacheItem& item, Value& value) {
	try {
		item.getBestHost().publish(item.key.hash, value);
		item.update(value);
	} catch (...) {
		//process publish errors
	}
}

void publishModerate(CacheItem& item, Value& value) {
	try {
		auto host = item.getBestHost();
		auto master = item.masterNode;
		auto version = item.key._version;
		auto tokenId = randomTokenId();
		ACK ack = master.getPublishTimestampToHost(version, tokenId, host);
		host.publish(item.key.hash, value, tokenId);
	} catch (...) {
		//process publish errors
	}
}

void publishCritical(CacheItem& item, Value& value) {
	try {
		auto master = item.masterNode;
		auto version = item.key._version;
		auto tokenId = randomTokenId();
		ACK ack = master.getPublishTimestampToHost(version, tokenId, host);
		host.publish(item.key.hash, value, tokenId);
		//TODO: wait for master node to ack all nodes updated
	} catch (...) {
		//process publish errors
	}
}
