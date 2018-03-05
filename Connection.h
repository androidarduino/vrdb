#pragma once

#include <string>
#include <priority_queue>
#include <map>
#include <sys/socket.h>

using namespace std;

/*
	Connection is a pipeline like network transmitter.
	It queues up all the messages and send them using priority algorithms.
	A database client could have multiple connections as needed.
	A connection has a tunable depth, to help limit the access speed.
*/

class Connection {
/*
	UDP protocol used in VRDB:
	1. Sender send a udp request with id
	2. Receiver reply with an ack with id
	3. Receiver work and send results back
	4. Receiver send replies in packets, with packet numbers and request id
	5. Sender ask for missing packets if later packets are received or timeout
	6. Receiver send missing packets
	7. Protocol finishes
*/
	public:
		Connection ();
		int tps = 100;
		int bps = 10000000;
		int send(string request);
	protected:
		priority_queue<Request> _queue;
		void loop();
};

/*
	Request stores request in a compressed format.
	It also get priorities, to enable the connection sending high priority request first.
*/
class Request {
	public:
		// constructor, example request string:
		// verb:getkey ip:192.168.0.3 port:12345 key:test scope:tsp urgency:top
		Request(string req == "") {
			_compressed = Compressor::compress(req);
			// parse the string and set meta data
		}
		// compare priority
		bool operator < (Request& req) { return req._priority > this.priority; }
		// for debugging only
		string tostring() { return Compressor::decompress(_compressed); }
		string& compressedstring() { return _compressed; }
		string operator [] (string metaName) {
			string meta = _meta[metaName];
			return (meta == NULL) ? "" : meta;
		}
	protected:
		string _compressed;
		void calculatePriority();
};

/*
	Helper class for Connection to compress the communication.
	It could use different compression strategies.
*/

class Compressor {
	public:
		//? return string or something else ?
		static string compress(string request) { return request; }
		static string decompress(string request) { return request; }
	private:
		static map<string, string> dictionary;
};

class Node {
	public:
		string ip, port;
};
