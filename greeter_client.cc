/* Clients of greeter_server, i.e. clients using the coordinator service will use this local RPC calls to communicate with the Coordinator service */


#include <iostream>
#include <memory>
#include <string>
#include <time.h>
#include <grpc++/grpc++.h>
#include <sys/stat.h>
#include "greeter_client.h"
#include "helloworld.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::Status;
using helloworld::HelloRequest;
using helloworld::HelloReply;
using helloworld::FetchRequest;
using helloworld::FetchReply;
using helloworld::StoreRequest;
using helloworld::StoreReply;
using helloworld::RegisterRequest;
using helloworld::RegisterReply;
using helloworld::CommitVoteRequest;
using helloworld::Empty;
using helloworld::Greeter;

#define printf

GreeterClient::GreeterClient(std::shared_ptr<Channel> channel)
	: stub_(Greeter::NewStub(channel)) {}

	// Assambles the client's payload, sends it and presents the response back
	// from the server.

std::string GreeterClient::SayHello(const std::string& user) {
		// Data we are sending to the server.
		HelloRequest request;
		request.set_name(user);

		// Container for the data we expect from the server.
		HelloReply reply;

		// Context for the client. It could be used to convey extra information to
		// the server and/or tweak certain RPC behaviors.
		ClientContext context;

		// The actual RPC.
		Status status = stub_->SayHello(&context, request, &reply);

		// Act upon its status.
		if (status.ok()) {
			return reply.message();
		} else {
			return "RPC failed";
		}
	}


int GreeterClient::Fetch (const std::string& path, char **buf, int *size) {
	FetchRequest request;
	request.set_path(path);


	FetchReply *reply = new FetchReply();

	ClientContext context;

	Status status = stub_->Fetch(&context, request, reply);

	if (status.ok() && reply->error()==0) {
//                std::cout << reply->buf() <<std::endl;
		*buf = (char *)(reply->buf()).data();
//                printf("%s\n", *buf);
		*size = reply->size();
		return 0;
	} else {
		return -1;
	}
}


int GreeterClient::Store (const std::string& path, const char *buf, int size) {
	StoreRequest request;
	request.set_path(path);
	request.set_size(size);
	request.set_buf(std::string(buf, size));
        request.set_op("store");
	StoreReply reply;

	ClientContext context;

	Status status = stub_->Store(&context, request, &reply);

	if (status.ok()) {
		return reply.error();
	} else {
		return -1;
	}
}

int GreeterClient::Delete (const std::string& path) {
        StoreRequest request;
        request.set_path(path);
        request.set_op("delete");
        StoreReply reply;

        ClientContext context;

        Status status = stub_->Store(&context, request, &reply);

        if (status.ok()) {
            return reply.error();
        } else {
            return -1;
        }       
}

int GreeterClient::Register (const std::string& conn) {
        RegisterRequest request;
        request.set_conn(conn);

        RegisterReply reply;

        ClientContext context;

        Status status = stub_->Register(&context, request, &reply);

        if (status.ok()) {
            return reply.error();
        } else {
            return -1;
        }
}

void GreeterClient::CommitVote(uint64_t txid, const std::string& conn) {
        CommitVoteRequest request;
        Empty             reply;
        request.set_txid(txid);
        request.set_conn(conn);

        ClientContext context;

        Status status = stub_->CommitVote(&context, request, &reply);

        if(status.ok()) {
        } else {
            printf("CommitVote send failed.\n");
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
