#pragma once
#include <string>
#include <vector>
#include <unique_ptr>

namespace VRDBClient {
using namespace std;
#typedef Version unsigned long long

class Key {
public:
	Key(std::string& key);
	std::vector<std::string> _path;
	Hash _hash;
	Version _version;
};

class CacheItem {
public:
	Key key;
	Value value;
	std::vector<std::unique_ptr<Host>> hosts;
	Host masterNode;
};

class ClientCache {
public:
	Value get(Hash& hash);
	void set(Key& key, Value& value);
	void setHosts(Key& key, std::string& hosts);
};

class Address {
	std::string url;
	sock_addr ip;
	int dataCenter, room, rack, host, vhost;
};

class Client {

public:
	enum Mode {Passive, Moderate, Critical};
	Client();
	std::string getKey(std::string& key, Mode& mode);
	Value getKey(std::string& key, Mode& mode);
	void publish(std::string& key, Value& value, Mode& mode);
	std::unique_ptr<ClientCache> _cache;
	Address address;
private:
	Value _getKey(Hash& hash, Mode& mode);
	void _publish(Hash& hash, Value& value, Mode& mode);
};

};
