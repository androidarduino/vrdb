#include "client.h"

Value _getKey(Hash& hash, Mode& mode) {
	// In Passive mode, return value from cache, passively wait for the cache to be updated from host, can not gurantee up to date
	// In Moderate mode, check with one or more hosts to determine data version, update cache if needed, high possibility up to date
	// In Critical mode, check with the master node of the key for version, up to date guranteed
}

void _publish(Hash& hash, Value& value, Mode& mode) {
	// In Passive mode, publish value to any or some of the hosts, leave it to the hosts to spread around, leave consistency problem to the hosts
	// In Moderate mode, ask the master node for publish version and publish values to host
	// In Critical mode, ask the master node to publish data on behalf, wait for the master node's acknowledgement as receipt
}
