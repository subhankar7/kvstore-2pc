/* Coordinator Library: Runs as a server accepting local RPCs from the client code on each client node */

#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <string>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <grpc++/grpc++.h>
#include "backend_client.h"

#include "helloworld.grpc.pb.h"

#define NSEC 1000000000LLU

using namespace std;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerWriter;
using grpc::Status;
using helloworld::HelloRequest;
using helloworld::FetchRequest;
using helloworld::FetchReply;
using helloworld::StoreRequest;
using helloworld::StoreReply;
using helloworld::HelloReply;
using helloworld::RegisterRequest;
using helloworld::RegisterReply;
using helloworld::CommitVoteRequest;
using helloworld::Empty;
using helloworld::Greeter;

unsigned long long int fetchcount;
unsigned long long int storecount;
unsigned long long int statcount;
unsigned long long int mkdircount;
unsigned long long int listdircount;

//#define printf
#define DBG printf

char afs_path[PATH_MAX];

int log_fd;
int conf_fd;

enum state {
    PREPARE,
    ABORT,
    COMMIT,
    END
};

typedef struct Tx_log_entry {
    uint64_t txid;
    state    state_t;
} Tx_log_entry;

typedef struct Tx_entry {
    uint64_t txid;
    state    state_t;
    uint32_t vote_count;
    uint32_t ack_count;
} Tx_entry;

typedef struct thd_info {
    string conn;
    Tx_entry *tx;
    void*  data;
} thd_info;


// Logic and data behind the server's behavior.
class GreeterServiceImpl final : public Greeter::Service {

  public : 
  static void Initialize(void) {
      if (pthread_rwlock_init(&reg_db_lock,NULL) != 0) {
          printf("Can't create reg_db_lock\n");
      }

      if (pthread_rwlock_init(&tx_db_lock,NULL) != 0) {
          printf("Can't create tx_db_lock\n");
      }

      if (pthread_rwlock_init(&log_lock, NULL) != 0) {
          printf("Can't create log_lock\n");
      }
  }

  static void Recover(void) {

      Tx_log_entry entry;
      char buf[sizeof(Tx_log_entry)];
      char connection[PATH_MAX];

      pthread_rwlock_wrlock(&reg_db_lock);
      while(read(conf_fd, connection, PATH_MAX) > 0) {
          reg_db.insert(std::string(connection));
      }
      R_FACTOR = reg_db.size();
      pthread_rwlock_unlock(&reg_db_lock);

      pthread_rwlock_wrlock(&log_lock);
      for(int i=1; lseek(log_fd, -i*sizeof(Tx_log_entry), SEEK_END) >= 0; i++) {
          read(log_fd, (char *)&entry, sizeof(entry));
          if(entry.state_t == COMMIT && !tx_db.count(entry.txid)) {
              Tx_entry* tx_entry = new Tx_entry();
	      tx_entry->txid = entry.txid;
	      tx_entry->state_t = COMMIT;
	      tx_entry->vote_count = R_FACTOR;
	      tx_entry->ack_count = 0;
	      pthread_rwlock_wrlock(&tx_db_lock);
	      tx_db.insert (std::make_pair((uint64_t)tx_entry->txid, (Tx_entry*)tx_entry));
	      pthread_rwlock_unlock(&tx_db_lock);
	      CommitRequest *commitReq = new CommitRequest();
	      commitReq->set_txid(tx_entry->txid);

	      for (auto it=reg_db.begin(); it != reg_db.end(); ++it) {
		  pthread_t tid;
		  thd_info *arg = new thd_info();
		  arg->conn = *it;
		  arg->tx = tx_entry;
		  arg->data = (void *)commitReq;
		  pthread_create(&tid, NULL, CommitRunnable, arg);
	      }
/*          } else if(entry.state_t == PREPARE && !tx_db.count(entry.txid)) {
              printf("Recovering txn: %d, Aborting ...\n", entry.txid);
              Tx_entry* tx_entry = new Tx_entry();
              tx_entry->txid = entry.txid;
              tx_entry->state_t = ABORT;
              tx_entry->vote_count = 0;
              tx_entry->ack_count = 0;
              pthread_rwlock_wrlock(&tx_db_lock);
              tx_db.insert (std::make_pair((uint64_t)tx_entry->txid, (Tx_entry*)tx_entry));
              pthread_rwlock_unlock(&tx_db_lock);
              AbortRequest *abortReq = new AbortRequest();
              abortReq->set_txid(tx_entry->txid);
	      for (auto it=reg_db.begin(); it != reg_db.end(); ++it) {
	          pthread_t tid;
		  thd_info *arg = new thd_info();
		  arg->conn = *it;
		  arg->tx = tx_entry;
		  arg->data = (void *)abortReq;
                  printf("Creating thread ...\n");
		  pthread_create(&tid, NULL, SendAbortRunnable, arg);
	      } */
          } else {
          }
      }
      pthread_rwlock_unlock(&log_lock);

  }

  private:

  static pthread_rwlock_t reg_db_lock;
  static pthread_rwlock_t tx_db_lock;
  static pthread_rwlock_t log_lock; 
  static unordered_set<string> reg_db;
  static unordered_map<uint64_t, Tx_entry *> tx_db;
  static uint32_t R_FACTOR;

  Status SayHello(ServerContext* context, const HelloRequest* request,
                  HelloReply* reply) override {
    std::string prefix("Hello ");
    reply->set_message(prefix + request->name());
    return Status::OK;
  }

  Status Fetch(ServerContext* context, const FetchRequest* request,
               FetchReply* reply) override {


    srand (time(NULL));

    int iBackend = rand()%R_FACTOR;

    auto it=reg_db.begin();

    for(int i=0; i<iBackend; it++, i++);

    
    BackendClient backend_store(
        grpc::CreateChannel(*it, grpc::InsecureCredentials()));

    backend_store.Fetch((BackendFetchRequest *)request, (BackendFetchReply *)reply);
    return Status::OK;
    
  }

  uint64_t getTid() {
      struct timespec time;
      clock_gettime(CLOCK_MONOTONIC, &time);
      return time.tv_sec*NSEC + time.tv_nsec;
    //  return 12345678;
  }

struct timespec diff(struct timespec start, struct timespec end)
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
}

  int tpcLog(uint64_t txid, state state_t) {
      struct timespec before, after, delta;
      Tx_log_entry log_entry;
      log_entry.txid = txid;
      log_entry.state_t = state_t;
      char buf[sizeof(Tx_log_entry)]; 
      ::memcpy(buf, &log_entry, sizeof(Tx_log_entry));
      int rc = 0;
      pthread_rwlock_wrlock(&log_lock);
      clock_gettime(CLOCK_MONOTONIC, &before);
      lseek(log_fd, 0, SEEK_END);
      rc = write(log_fd, buf, sizeof(Tx_log_entry));
      if(rc<0) {
          return errno;
      }
      fsync(log_fd);
      clock_gettime(CLOCK_MONOTONIC, &after);
      pthread_rwlock_unlock(&log_lock);
      delta = diff(before, after);
      cout << delta.tv_sec*1000000000LLU + delta.tv_nsec << endl; 
      return 0;
  }

  static void *GetVoteRunnable(void *arg) {

      thd_info *input = (thd_info *)arg;

      BackendClient backend_store(
         grpc::CreateChannel(input->conn, grpc::InsecureCredentials()));

      // cout << "Calling Prepare for tx: " << input->tx->txid << endl;
      bool vote = backend_store.Prepare((PrepareRequest *)input->data); 
      // cout << "Prepare finished for tx: " << input->tx->txid << endl;
      

      Tx_entry *tx_entry = input->tx;

      if(vote==true) {
          pthread_rwlock_wrlock(&tx_db_lock);
          tx_entry->vote_count++;
          pthread_rwlock_unlock(&tx_db_lock); 
      }
  }

  static void *SendAbortRunnable(void *arg) {
      thd_info *input = (thd_info *)arg;
      printf("SendAbortRunnable...\n");
      fflush(stdout);
      BackendClient backend_store(
         grpc::CreateChannel(input->conn, grpc::InsecureCredentials()));

      bool ack = backend_store.Abort((AbortRequest *)input->data);

      Tx_entry *tx_entry = input->tx;

      if(ack==true && tx_entry != NULL) {    // tx_entry will be NULL in Presumed Abort.
          pthread_rwlock_wrlock(&tx_db_lock);
          tx_entry->ack_count++;
          pthread_rwlock_unlock(&tx_db_lock);
      }
  }

  static void *CommitRunnable(void *arg) {
      thd_info *input = (thd_info *)arg;

      BackendClient backend_store(
         grpc::CreateChannel(input->conn, grpc::InsecureCredentials()));

      bool ack = backend_store.Commit((CommitRequest *)input->data);

      Tx_entry *tx_entry = input->tx;

      if(ack==true) {
          pthread_rwlock_wrlock(&tx_db_lock);
          tx_entry->ack_count++;
          pthread_rwlock_unlock(&tx_db_lock);
      }
  }

  Status CommitVote(ServerContext* context, const CommitVoteRequest* request,
                    Empty* reply) override {

      Tx_entry* tx_entry = NULL;

      pthread_rwlock_wrlock(&tx_db_lock);
      printf("Sending commit for CommitVote Txn: %llu.\n", request->txid());
      if(tx_db.count(request->txid())) {
          tx_entry = tx_db[request->txid()];
          if(tx_entry->state_t == COMMIT) {
              tx_entry->ack_count = 0;
              // Send Commit to all clients.
              CommitRequest *commitReq = new CommitRequest();
              commitReq->set_txid(tx_entry->txid);

              pthread_t tid;
              thd_info *arg = new thd_info();
              arg->conn = request->conn();
              arg->tx = tx_entry;
              arg->data = (void *)commitReq;
              pthread_create(&tid, NULL, CommitRunnable, arg);
          } else if(tx_entry->state_t == ABORT) {
              tx_entry->ack_count = 0;
              // Send Abort to all clients.
              AbortRequest *abortReq = new AbortRequest();
              abortReq->set_txid(tx_entry->txid);

              pthread_t tid;
              thd_info *arg = new thd_info();
              arg->conn = request->conn();
              arg->tx = tx_entry;
              arg->data = (void *)abortReq;
              pthread_create(&tid, NULL, SendAbortRunnable, arg);
          }
      } else {
          AbortRequest *abortReq = new AbortRequest();
          abortReq->set_txid(request->txid());

          pthread_t tid;
          thd_info *arg = new thd_info();
          arg->conn = request->conn();
          arg->tx = NULL;
          arg->data = (void *)abortReq;
          pthread_create(&tid, NULL, SendAbortRunnable, arg);
      }
      
      pthread_rwlock_unlock(&tx_db_lock);
      return Status::OK;
  }

  Status Store(ServerContext* context, const StoreRequest* request,
               StoreReply* reply) override {

      storecount++;

      vector<pthread_t> tid_v;

      int rc = 0;
      int i = 0;
      uint64_t txid;

      PrepareRequest *prepReq = new PrepareRequest();

      txid = getTid();

      // cout << "Store start for file:" << request->path() << "Tx: " << txid << endl;

      prepReq->set_txid(txid);

      if(request->op()=="store") {
          prepReq->set_op("store");
          prepReq->set_size(request->size());
          prepReq->set_buf(request->buf()); 
      } else if(request->op()=="delete") {
          prepReq->set_op("delete");
      } 
      prepReq->set_path(request->path());

      //rc = tpcLog(txid, PREPARE);

      if(rc!=0) {
          reply->set_error(rc);
          return Status::OK;
      }

      Tx_entry* tx_entry = new Tx_entry();
      tx_entry->txid = txid;
      tx_entry->state_t = PREPARE;
      tx_entry->vote_count = 0;
      tx_entry->ack_count = 0;
      pthread_rwlock_wrlock(&tx_db_lock);
      tx_db.insert (std::make_pair((uint64_t)txid, (Tx_entry*)tx_entry));
      pthread_rwlock_unlock(&tx_db_lock);

      for (auto it=reg_db.begin(); it != reg_db.end(); ++it) {
          pthread_t tid;
          thd_info *arg = new thd_info();
          arg->conn = *it;
          arg->tx   = tx_entry;
          arg->data = (void *)prepReq;
          pthread_create(&tid, NULL, GetVoteRunnable, arg);
          tid_v.push_back(tid);
      }

      struct timespec timer;
      clock_gettime(CLOCK_REALTIME, &timer);
      timer.tv_sec += 5;

      for (auto it=reg_db.begin(); it != reg_db.end(); ++it) {
          pthread_timedjoin_np(tid_v[i], NULL, &timer);
          i++;
      }

      if(tx_entry->vote_count!=R_FACTOR) {
          //rc = tpcLog(txid, ABORT);     /* TODO: We can figure out if there is only PREPARE in log */
          tx_entry->state_t = ABORT;      

          AbortRequest *abortReq = new AbortRequest();
          abortReq->set_txid(txid); 
          for (auto it=reg_db.begin(); it != reg_db.end(); ++it) {
              pthread_t tid;
              thd_info *arg = new thd_info();
              arg->conn = *it;
              arg->tx = tx_entry;
              arg->data = (void *)abortReq;
              pthread_create(&tid, NULL, SendAbortRunnable, arg);
          }
          reply->set_error(-1);
          // cout << "Store end aborted for file:" << request->path() << endl;
          return Status::OK;
      }

      //printf("Sleeping server now...\n");
      //sleep(30);

      rc = tpcLog(txid, COMMIT);   /* TODO: Need to abort when rc is error */
      tx_entry->state_t = COMMIT;

      CommitRequest *commitReq = new CommitRequest();
      commitReq->set_txid(txid);

      struct timespec before, after, delta;
      clock_gettime(CLOCK_MONOTONIC, &before);
      for (auto it=reg_db.begin(); it != reg_db.end(); ++it) {
          pthread_t tid;
          thd_info *arg = new thd_info();
          arg->conn = *it;
          arg->tx = tx_entry;
          arg->data = (void *)commitReq;
          pthread_create(&tid, NULL, CommitRunnable, arg);
      }
      clock_gettime(CLOCK_MONOTONIC, &after);

      delta = diff(before, after);
      printf("Thread creation time: %llu\n", delta.tv_sec*1000000000LLU + delta.tv_nsec);


      reply->set_error(0);

      // cout << "Store end for file:" << request->path() << endl;

      return Status::OK;
  }


  Status Register(ServerContext* context, const RegisterRequest* request,
                 RegisterReply* reply) override {

      char connection[PATH_MAX];

      pthread_rwlock_wrlock(&reg_db_lock);
      if(!reg_db.count(request->conn())) {
          strncpy(connection, request->conn().c_str(), PATH_MAX);
          write(conf_fd, connection, PATH_MAX);
          fsync(conf_fd);          
          reg_db.insert(request->conn());
      }
      R_FACTOR = reg_db.size();
      pthread_rwlock_unlock(&reg_db_lock);
      for (auto it=reg_db.begin(); it != reg_db.end(); ++it)
          // // cout << ' ' << *it;

      fflush(stdout);
      reply->set_error(0);
      return Status::OK;
  }

};

pthread_rwlock_t GreeterServiceImpl::reg_db_lock;
pthread_rwlock_t GreeterServiceImpl::tx_db_lock;
pthread_rwlock_t GreeterServiceImpl::log_lock;
unordered_set<string> GreeterServiceImpl::reg_db;
unordered_map<uint64_t, Tx_entry *> GreeterServiceImpl::tx_db;
uint32_t GreeterServiceImpl::R_FACTOR;

void RunServer(int recover) {
  std::string server_address("0.0.0.0:12348");
  GreeterServiceImpl service;
  service.Initialize();
  if(recover!=0) {
      service.Recover();
  } else {
      ftruncate(log_fd, 0);
      ftruncate(conf_fd,0);
      fsync(log_fd);
      fsync(conf_fd);
  }
  ServerBuilder builder;
  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  // Register "service" as the instance through which we'll communicate with
  // clients. In this case it corresponds to an *synchronous* service.
  builder.RegisterService(&service);
  // Finally assemble the server.
  std::unique_ptr<Server> server(builder.BuildAndStart());
  // // cout << "Server listening on " << server_address << std::endl;

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  server->Wait();
}

int main(int argc, char** argv) {
  int recover = 0;
  if(argc==2) {
      recover = atoi(argv[1]);
  } else {
      printf("Usage: ./greeter_server <recovery_mode>\n"
             "recovery_mode: 0 or 1, 0: no recovery from log, 1: recovery will be done from log\n");
      return -1;
  }

  log_fd = open("log", O_CREAT | O_RDWR | O_APPEND, 0666);
  conf_fd = open("conf", O_CREAT | O_RDWR | O_APPEND, 0666);
  RunServer(recover);
  close(log_fd);
  close(conf_fd);
  unlink("log");
  unlink("conf");
  return 0;
}
