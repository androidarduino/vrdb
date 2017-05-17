#pragma once

class Key {
public:
	Key(std::string& key);
	std::vector<std::string> _path;
	Hash _hash;
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
	Value get(Key& key);
	void set(Key& key, Value& value);
	void setHosts(Key& key, std::string& hosts);
};

class Client {

public:
	enum Mode {Passive, Moderate, Critical};
	Client();
	std::string getKey(std::string& key, Mode& mode);
	Value getKey(std::string& key, Mode& mode);
	void publish(std::string& key, Value& value, Mode& mode);
	std::unique_ptr<ClientCache> _cache;
private:
	Value _getKey(Hash& hash, Mode& mode);
	void _publish(Hash& hash, Value& value, Mode& mode);
};
