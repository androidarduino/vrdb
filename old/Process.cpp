#include "Process.h"

using namespace std;

void doJob() {
	// read from socket to buffer.
	buffer.readSocket();

	// if there is a whole message is received, process the message, until there is no whole message in the buffer.
	// process cache reply for a time slice
	for (int i = time.now(); i < time.now() + _config["TimeSliceForCache"]; ) {
		auto msg = _inBuffer.getMessage();
		if (_cache.handle(msg)) continue;
		_cacheMissQueue.push(msg);
	}

	// process read write to database for a time slice
	for (int i = time.now(); i < time.now() + _config["TimeSliceForStore"]; ) {
		auto msg = _cacheMissQueue.pop();
		if (_store.handle(msg)) continue;
		_errorQueue.push(msg);
	}

	// process socket sending for a time slice
	for (int i = time.now(); i < time.now() + _config["TimeSliceForSending"]; ) {
		auto msg = _sendingQueue.pop();
		if (_outBuffer.send(msg)) continue;
		_errorQueue.push(msg);
	}
	// process errored messages
	for (int i = time.now(); i < time.now() + _config["TimeSliceForError"]; ) {
		auto msg = errorQueue.pop();
		//TODO: implement retry
		logOrReportError(msg);
	}

	// process administration work for a time slice
	for (int i = time.now(); i < time.now() + _config["TimeSliceForSending"]; ) {
		auto healthy = checkHealthStatus();
		_outBuffer.send(healthy);
	}
}

Message checkHealthStatus() {
	return Message(_config["HealthServer", "good"]);
}
