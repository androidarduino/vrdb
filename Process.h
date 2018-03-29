#pragma once

#include<map>
#include<string>
#include<vector>

/* Cradle process that contains a lot of endpoints

*/

class CradleProcess {

	// all endpoints in this cradle
	std::map<std::string, Endpoint> _endpoints;
	// socket to listen to messages to endpoints on this host
	SocketBuffer _inBuffer, _outBuffer;
	// singleton config object
	Config _config;

	// cache miss queue, waiting to read from disk
	// error queue, waiting to log error or report error, or retry
	MessageQueue _cacheMissQueue, _sendingQueue, _errorQueue;

	VRCache _cache;
	VRStore _store;

	// main loop
	void doJob();
	Message checkhealthStatus();
};
