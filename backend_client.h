
#include <grpc++/grpc++.h>

#include "backend.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using backend::PrepareRequest;
using backend::CommitRequest;
using backend::AbortRequest;
using backend::BackendFetchRequest;
using backend::BackendFetchReply;
using backend::Backend;

class BackendClient {
 public:
  BackendClient(std::shared_ptr<Channel> channel);

  // Assambles the client's payload, sends it and presents the response back
  // from the server.
  bool Prepare(PrepareRequest* request);
  bool Commit(CommitRequest* request);
  bool Abort(AbortRequest* request);
  int Fetch(BackendFetchRequest* request, BackendFetchReply* reply);
 private:
  std::unique_ptr<Backend::Stub> stub_;
};

