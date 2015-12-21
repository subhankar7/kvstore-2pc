// Generated by the gRPC protobuf plugin.
// If you make any local change, they will be lost.
// source: helloworld.proto
#ifndef GRPC_helloworld_2eproto__INCLUDED
#define GRPC_helloworld_2eproto__INCLUDED

#include "helloworld.pb.h"

#include <grpc++/support/async_stream.h>
#include <grpc++/impl/rpc_method.h>
#include <grpc++/impl/proto_utils.h>
#include <grpc++/impl/service_type.h>
#include <grpc++/support/async_unary_call.h>
#include <grpc++/support/status.h>
#include <grpc++/support/stub_options.h>
#include <grpc++/support/sync_stream.h>

namespace grpc {
class CompletionQueue;
class Channel;
class RpcService;
class ServerCompletionQueue;
class ServerContext;
}  // namespace grpc

namespace helloworld {

class Greeter GRPC_FINAL {
 public:
  class StubInterface {
   public:
    virtual ~StubInterface() {}
    virtual ::grpc::Status SayHello(::grpc::ClientContext* context, const ::helloworld::HelloRequest& request, ::helloworld::HelloReply* response) = 0;
    std::unique_ptr< ::grpc::ClientAsyncResponseReaderInterface< ::helloworld::HelloReply>> AsyncSayHello(::grpc::ClientContext* context, const ::helloworld::HelloRequest& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncResponseReaderInterface< ::helloworld::HelloReply>>(AsyncSayHelloRaw(context, request, cq));
    }
    virtual ::grpc::Status Fetch(::grpc::ClientContext* context, const ::helloworld::FetchRequest& request, ::helloworld::FetchReply* response) = 0;
    std::unique_ptr< ::grpc::ClientAsyncResponseReaderInterface< ::helloworld::FetchReply>> AsyncFetch(::grpc::ClientContext* context, const ::helloworld::FetchRequest& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncResponseReaderInterface< ::helloworld::FetchReply>>(AsyncFetchRaw(context, request, cq));
    }
    virtual ::grpc::Status Store(::grpc::ClientContext* context, const ::helloworld::StoreRequest& request, ::helloworld::StoreReply* response) = 0;
    std::unique_ptr< ::grpc::ClientAsyncResponseReaderInterface< ::helloworld::StoreReply>> AsyncStore(::grpc::ClientContext* context, const ::helloworld::StoreRequest& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncResponseReaderInterface< ::helloworld::StoreReply>>(AsyncStoreRaw(context, request, cq));
    }
    virtual ::grpc::Status Register(::grpc::ClientContext* context, const ::helloworld::RegisterRequest& request, ::helloworld::RegisterReply* response) = 0;
    std::unique_ptr< ::grpc::ClientAsyncResponseReaderInterface< ::helloworld::RegisterReply>> AsyncRegister(::grpc::ClientContext* context, const ::helloworld::RegisterRequest& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncResponseReaderInterface< ::helloworld::RegisterReply>>(AsyncRegisterRaw(context, request, cq));
    }
    virtual ::grpc::Status CommitVote(::grpc::ClientContext* context, const ::helloworld::CommitVoteRequest& request, ::helloworld::Empty* response) = 0;
    std::unique_ptr< ::grpc::ClientAsyncResponseReaderInterface< ::helloworld::Empty>> AsyncCommitVote(::grpc::ClientContext* context, const ::helloworld::CommitVoteRequest& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncResponseReaderInterface< ::helloworld::Empty>>(AsyncCommitVoteRaw(context, request, cq));
    }
  private:
    virtual ::grpc::ClientAsyncResponseReaderInterface< ::helloworld::HelloReply>* AsyncSayHelloRaw(::grpc::ClientContext* context, const ::helloworld::HelloRequest& request, ::grpc::CompletionQueue* cq) = 0;
    virtual ::grpc::ClientAsyncResponseReaderInterface< ::helloworld::FetchReply>* AsyncFetchRaw(::grpc::ClientContext* context, const ::helloworld::FetchRequest& request, ::grpc::CompletionQueue* cq) = 0;
    virtual ::grpc::ClientAsyncResponseReaderInterface< ::helloworld::StoreReply>* AsyncStoreRaw(::grpc::ClientContext* context, const ::helloworld::StoreRequest& request, ::grpc::CompletionQueue* cq) = 0;
    virtual ::grpc::ClientAsyncResponseReaderInterface< ::helloworld::RegisterReply>* AsyncRegisterRaw(::grpc::ClientContext* context, const ::helloworld::RegisterRequest& request, ::grpc::CompletionQueue* cq) = 0;
    virtual ::grpc::ClientAsyncResponseReaderInterface< ::helloworld::Empty>* AsyncCommitVoteRaw(::grpc::ClientContext* context, const ::helloworld::CommitVoteRequest& request, ::grpc::CompletionQueue* cq) = 0;
  };
  class Stub GRPC_FINAL : public StubInterface {
   public:
    Stub(const std::shared_ptr< ::grpc::Channel>& channel);
    ::grpc::Status SayHello(::grpc::ClientContext* context, const ::helloworld::HelloRequest& request, ::helloworld::HelloReply* response) GRPC_OVERRIDE;
    std::unique_ptr< ::grpc::ClientAsyncResponseReader< ::helloworld::HelloReply>> AsyncSayHello(::grpc::ClientContext* context, const ::helloworld::HelloRequest& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncResponseReader< ::helloworld::HelloReply>>(AsyncSayHelloRaw(context, request, cq));
    }
    ::grpc::Status Fetch(::grpc::ClientContext* context, const ::helloworld::FetchRequest& request, ::helloworld::FetchReply* response) GRPC_OVERRIDE;
    std::unique_ptr< ::grpc::ClientAsyncResponseReader< ::helloworld::FetchReply>> AsyncFetch(::grpc::ClientContext* context, const ::helloworld::FetchRequest& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncResponseReader< ::helloworld::FetchReply>>(AsyncFetchRaw(context, request, cq));
    }
    ::grpc::Status Store(::grpc::ClientContext* context, const ::helloworld::StoreRequest& request, ::helloworld::StoreReply* response) GRPC_OVERRIDE;
    std::unique_ptr< ::grpc::ClientAsyncResponseReader< ::helloworld::StoreReply>> AsyncStore(::grpc::ClientContext* context, const ::helloworld::StoreRequest& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncResponseReader< ::helloworld::StoreReply>>(AsyncStoreRaw(context, request, cq));
    }
    ::grpc::Status Register(::grpc::ClientContext* context, const ::helloworld::RegisterRequest& request, ::helloworld::RegisterReply* response) GRPC_OVERRIDE;
    std::unique_ptr< ::grpc::ClientAsyncResponseReader< ::helloworld::RegisterReply>> AsyncRegister(::grpc::ClientContext* context, const ::helloworld::RegisterRequest& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncResponseReader< ::helloworld::RegisterReply>>(AsyncRegisterRaw(context, request, cq));
    }
    ::grpc::Status CommitVote(::grpc::ClientContext* context, const ::helloworld::CommitVoteRequest& request, ::helloworld::Empty* response) GRPC_OVERRIDE;
    std::unique_ptr< ::grpc::ClientAsyncResponseReader< ::helloworld::Empty>> AsyncCommitVote(::grpc::ClientContext* context, const ::helloworld::CommitVoteRequest& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncResponseReader< ::helloworld::Empty>>(AsyncCommitVoteRaw(context, request, cq));
    }

   private:
    std::shared_ptr< ::grpc::Channel> channel_;
    ::grpc::ClientAsyncResponseReader< ::helloworld::HelloReply>* AsyncSayHelloRaw(::grpc::ClientContext* context, const ::helloworld::HelloRequest& request, ::grpc::CompletionQueue* cq) GRPC_OVERRIDE;
    ::grpc::ClientAsyncResponseReader< ::helloworld::FetchReply>* AsyncFetchRaw(::grpc::ClientContext* context, const ::helloworld::FetchRequest& request, ::grpc::CompletionQueue* cq) GRPC_OVERRIDE;
    ::grpc::ClientAsyncResponseReader< ::helloworld::StoreReply>* AsyncStoreRaw(::grpc::ClientContext* context, const ::helloworld::StoreRequest& request, ::grpc::CompletionQueue* cq) GRPC_OVERRIDE;
    ::grpc::ClientAsyncResponseReader< ::helloworld::RegisterReply>* AsyncRegisterRaw(::grpc::ClientContext* context, const ::helloworld::RegisterRequest& request, ::grpc::CompletionQueue* cq) GRPC_OVERRIDE;
    ::grpc::ClientAsyncResponseReader< ::helloworld::Empty>* AsyncCommitVoteRaw(::grpc::ClientContext* context, const ::helloworld::CommitVoteRequest& request, ::grpc::CompletionQueue* cq) GRPC_OVERRIDE;
    const ::grpc::RpcMethod rpcmethod_SayHello_;
    const ::grpc::RpcMethod rpcmethod_Fetch_;
    const ::grpc::RpcMethod rpcmethod_Store_;
    const ::grpc::RpcMethod rpcmethod_Register_;
    const ::grpc::RpcMethod rpcmethod_CommitVote_;
  };
  static std::unique_ptr<Stub> NewStub(const std::shared_ptr< ::grpc::Channel>& channel, const ::grpc::StubOptions& options = ::grpc::StubOptions());

  class Service : public ::grpc::SynchronousService {
   public:
    Service() : service_(nullptr) {}
    virtual ~Service();
    virtual ::grpc::Status SayHello(::grpc::ServerContext* context, const ::helloworld::HelloRequest* request, ::helloworld::HelloReply* response);
    virtual ::grpc::Status Fetch(::grpc::ServerContext* context, const ::helloworld::FetchRequest* request, ::helloworld::FetchReply* response);
    virtual ::grpc::Status Store(::grpc::ServerContext* context, const ::helloworld::StoreRequest* request, ::helloworld::StoreReply* response);
    virtual ::grpc::Status Register(::grpc::ServerContext* context, const ::helloworld::RegisterRequest* request, ::helloworld::RegisterReply* response);
    virtual ::grpc::Status CommitVote(::grpc::ServerContext* context, const ::helloworld::CommitVoteRequest* request, ::helloworld::Empty* response);
    ::grpc::RpcService* service() GRPC_OVERRIDE GRPC_FINAL;
   private:
    ::grpc::RpcService* service_;
  };
  class AsyncService GRPC_FINAL : public ::grpc::AsynchronousService {
   public:
    explicit AsyncService();
    ~AsyncService() {};
    void RequestSayHello(::grpc::ServerContext* context, ::helloworld::HelloRequest* request, ::grpc::ServerAsyncResponseWriter< ::helloworld::HelloReply>* response, ::grpc::CompletionQueue* new_call_cq, ::grpc::ServerCompletionQueue* notification_cq, void *tag);
    void RequestFetch(::grpc::ServerContext* context, ::helloworld::FetchRequest* request, ::grpc::ServerAsyncResponseWriter< ::helloworld::FetchReply>* response, ::grpc::CompletionQueue* new_call_cq, ::grpc::ServerCompletionQueue* notification_cq, void *tag);
    void RequestStore(::grpc::ServerContext* context, ::helloworld::StoreRequest* request, ::grpc::ServerAsyncResponseWriter< ::helloworld::StoreReply>* response, ::grpc::CompletionQueue* new_call_cq, ::grpc::ServerCompletionQueue* notification_cq, void *tag);
    void RequestRegister(::grpc::ServerContext* context, ::helloworld::RegisterRequest* request, ::grpc::ServerAsyncResponseWriter< ::helloworld::RegisterReply>* response, ::grpc::CompletionQueue* new_call_cq, ::grpc::ServerCompletionQueue* notification_cq, void *tag);
    void RequestCommitVote(::grpc::ServerContext* context, ::helloworld::CommitVoteRequest* request, ::grpc::ServerAsyncResponseWriter< ::helloworld::Empty>* response, ::grpc::CompletionQueue* new_call_cq, ::grpc::ServerCompletionQueue* notification_cq, void *tag);
  };
};

}  // namespace helloworld


#endif  // GRPC_helloworld_2eproto__INCLUDED