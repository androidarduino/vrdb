package node

import (
	"fmt"
)

type Router struct {
	Name string // the registered name of a router to the dns
	Range map[string]string // serving range of the router
	UnivMapping map[string]string // a universal mapping of registered routers
	ConnManager *NodeConnectionManager //TODO: change this to a channel connection to another module
	Cache *NodeCache //TODO: change this to a channel connection to another module
}

/* 
	A router is responsible for the following:
	1. Accept connection requests from other parties (siblings, peers, client)
	2. Maintain connection in healthy state
	3. Handle requests from other parties
	4. 
*/

//Init a router, start the connection handler and start listening
func (r *Router) Init(config Config) bool {
	ConnManager.listen(CONN_PORT)
}

//Forward a request or a response to another party
func (r *Router) forward(data *string, destination *string) {

}

// Handle a client request: GET, PUT, LIST
func (r *Router) HandleRequest(request *string) (resp Response, ok bool) {
// parse the request, call put, get or list to complete the request
	switch(requestType) {
	case PUT:
		put(key, value)
		return r.putResponse(request)
	case GET:
		return r.getResponse(get(key), request)
	case LIST:
		return r.listResponse(list(filter, params), request)
	}
}

func (r *Router) put(key *string, value *string) bool {

}

func (r *Router) get(key *string) *string {
	
}

func (r *Router) list(key *string, value *string) []*Record {
	
}

func (r *Router) putResponse(r *Request) *Response {
	
}

func (r *Router) getResponse(r *string) *Response {
	
}

func (r *Router) listResponse(r *[]*Record) *Response {
	
}



