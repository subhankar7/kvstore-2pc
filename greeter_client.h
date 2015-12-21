
#include <grpc++/grpc++.h>

#include "helloworld.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using helloworld::HelloRequest;
using helloworld::HelloReply;
using helloworld::Greeter;

class GreeterClient {
 public:
  GreeterClient(std::shared_ptr<Channel> channel);

  // Assambles the client's payload, sends it and presents the response back
  // from the server.
  std::string SayHello(const std::string& user);

  int Fetch(const std::string& path, char **buf, int *size);
  int Store(const std::string& path, const char *buf, int size);
  int Delete (const std::string& path);
  int Register (const std::string& conn);
  void CommitVote (uint64_t txid, const std::string& conn);
 private:
  std::unique_ptr<Greeter::Stub> stub_;
};

