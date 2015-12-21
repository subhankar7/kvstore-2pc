/* Clients of backend will use this RPCs to call on the Backend_store */

#include <iostream>
#include <memory>
#include <string>
#include <time.h>
#include <grpc++/grpc++.h>
#include <sys/stat.h>
#include "backend_client.h"
#include "backend.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::Status;
using backend::PrepareRequest;
using backend::PrepareReply;
using backend::CommitRequest;
using backend::CommitReply;
using backend::AbortRequest;
using backend::AbortReply;
using backend::BackendFetchRequest;
using backend::BackendFetchReply;

BackendClient::BackendClient(std::shared_ptr<Channel> channel)
	: stub_(Backend::NewStub(channel)) {}

	// Assambles the client's payload, sends it and presents the response back
	// from the server.

bool BackendClient::Prepare(PrepareRequest* request) {
		// Container for the data we expect from the server.
		PrepareReply reply;

		// Context for the client. It could be used to convey extra information to
		// the server and/or tweak certain RPC behaviors.
		ClientContext context;

		// The actual RPC.
		Status status = stub_->Prepare(&context, *request, &reply);

		// Act upon its status.
		if (status.ok()) {
			if(reply.result()=="ok") {
                            return true;
                        } else {
                            return false;
                        }
		} else {
			return false;
		}
	}

bool BackendClient::Commit(CommitRequest* request) {
                // Container for the data we expect from the server.
                CommitReply reply;

                // Context for the client. It could be used to convey extra information to
                // the server and/or tweak certain RPC behaviors.
                ClientContext context;

                // The actual RPC.
                Status status = stub_->Commit(&context, *request, &reply);

                // Act upon its status.
                if (status.ok()) {
                        if(reply.result()=="ack") {
                            return true;
                        } else {
                            return false;
                        }
                } else {
                        return false;
                }
        }

bool BackendClient::Abort(AbortRequest* request) {
                // Container for the data we expect from the server.
                AbortReply reply;

                // Context for the client. It could be used to convey extra information to
                // the server and/or tweak certain RPC behaviors.
                ClientContext context;

                // The actual RPC.
                Status status = stub_->Abort(&context, *request, &reply);

                // Act upon its status.
                if (status.ok()) {
                        if(reply.result()=="ack") {
                            return true;
                        } else {
                            return false;
                        }
                } else {
                        return false;
                }
        }


int BackendClient::Fetch (BackendFetchRequest* request, BackendFetchReply *reply) {

	ClientContext context;

	Status status = stub_->Fetch(&context, *request, reply);

	if (status.ok() && reply->error()==0) {
		return 0;
	} else {
		return -1;
	}
}


/*struct timespec diff(struct timespec start, struct timespec end)
  {
  struct timespec temp;
  if ((end.tv_nsec-start.tv_nsec)<0) {
  temp.tv_sec = end.tv_sec-start.tv_sec-1;
  temp.tv_nsec = 1000000000+end.tv_nsec-start.tv_nsec;
  } else {
  temp.tv_sec = end.tv_sec-start.tv_sec;
                temp.tv_nsec = end.tv_nsec-start.tv_nsec;
        }
        return temp;
} */

/*
int main(int argc, char** argv) {
  // Instantiate the client. It requires a channel, out of which the actual RPCs
  // are created. This channel models a connection to an endpoint (in this case,
  // localhost at port 50051). We indicate that the channel isn't authenticated
  // (use of InsecureCredentials()).
  GreeterClient greeter(
      grpc::CreateChannel("king-01:12348", grpc::InsecureCredentials()));
  std::string user("world");

  struct timespec start;
  struct timespec end;
  for( int i=0; i<1000; i++) {
      clock_gettime(CLOCK_MONOTONIC, &start); 
      std::string reply = greeter.SayHello(user);
      clock_gettime(CLOCK_MONOTONIC, &end);
      printf("%llu\t%llu\n", (long long unsigned int)diff(start,end).tv_sec, (long long unsigned int)diff(start,end).tv_nsec);
  }
  //std::cout << "Greeter received: " << reply << std::endl;

  return 0;
}
*/
