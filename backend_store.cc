/* Backend Store: Server code for the backend replicated stores */


#include <sstream>
#include <iostream>
#include <memory>
#include <string>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <unordered_map>
#include <stdlib.h>
#include <grpc++/grpc++.h>
#include "greeter_client.h"
#include "backend.grpc.pb.h"

#define BUF_SIZE 4096
#define TIMEOUT 5
#define NSEC 1000000000LLU

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerWriter;
using grpc::Status;
using backend::BackendFetchRequest;
using backend::BackendFetchReply;
using backend::PrepareRequest;
using backend::PrepareReply;
using backend::CommitRequest;
using backend::CommitReply;
using backend::AbortRequest;
using backend::AbortReply;
using backend::Backend;

using namespace std;
Server *serv;
static GreeterClient *leader;
int log_fd = 0;
string myconn;
unsigned long long int fetchcount;
unsigned long long int storecount;
unsigned long long int statcount;
unsigned long long int mkdircount;
unsigned long long int listdircount;
unsigned long long int cmt;

vector<uint64_t> times;
//#define printf
#define DBG printf

char afs_path[PATH_MAX];

#define OP_LENGTH 8


enum state {
    PREPARE,
    ABORT,
    COMMIT,
    END
};

typedef struct Tx_entry {
    uint64_t txid;
    uint64_t timestamp;
    state    state_t;
    string   path;
    string   op;
} Tx_entry;

typedef struct Tx_log_entry {
    uint64_t txid;
    state    state_t;
    char     path[PATH_MAX];
    char     op[OP_LENGTH];
} Tx_log_entry;

// Logic and data behind the server's behavior.
class BackendServiceImpl final : public Backend::Service {

  public:

  static void Initialize(void) {
      pthread_t tid;

      if (pthread_rwlock_init(&tx_db_lock,NULL) != 0) {
          printf("Can't create tx_db_lock\n");
      }

      if (pthread_rwlock_init(&log_lock, NULL) != 0) {
          printf("Can't create log_lock\n");
      }

      pthread_create(&tid, NULL, AskCoordRetryRunnable, NULL);
  }

  static void *AskCoordRetryRunnable(void *arg) {
      struct timespec time;
      while(1) {
         pthread_rwlock_rdlock(&tx_db_lock);
         for ( auto it = tx_db.begin(); it != tx_db.end(); ++it ) {
             Tx_entry *tx_entry = it->second;
             clock_gettime(CLOCK_MONOTONIC, &time);
             //printf("Waited: %llu\n", (time.tv_sec*NSEC+time.tv_nsec - tx_entry->timestamp));
             if(tx_entry->state_t==PREPARE && (time.tv_sec*NSEC+time.tv_nsec - tx_entry->timestamp) > TIMEOUT*NSEC) {
                 leader->CommitVote(tx_entry->txid, myconn);
                 tx_entry->timestamp = time.tv_sec*NSEC + time.tv_nsec;
             }
         }
         pthread_rwlock_unlock(&tx_db_lock);
         sleep(TIMEOUT);
      }
  }

  static void Recover() {
      Tx_log_entry entry;
      char buf[sizeof(Tx_log_entry)];
      pthread_rwlock_t *file_lock = NULL;


      printf("Recovering...\n");
      fflush(stdout);
      pthread_rwlock_wrlock(&log_lock);
      for(int i=1; lseek(log_fd, -i*sizeof(Tx_log_entry), SEEK_END) >= 0; i++) {
          read(log_fd, (char *)&entry, sizeof(entry));
          Tx_entry* tx_entry = new Tx_entry();
          tx_entry->txid = entry.txid;
          printf("Recovering txn: %d\n", entry.txid);
          tx_entry->state_t = entry.state_t;
          tx_entry->path = std::string(entry.path);
          tx_entry->op   = std::string(entry.op);
      
          if(entry.state_t == COMMIT && !tx_db.count(tx_entry->txid)) {
              pthread_rwlock_wrlock(&tx_db_lock);
              tx_db.insert (std::make_pair((uint64_t)tx_entry->txid, (Tx_entry*)tx_entry));
              pthread_rwlock_unlock(&tx_db_lock);
              //TODO: Send Ack asynchronously. Probably not needed because these are only for bookkeeping and will be taken care of on restarts.
          } else if(entry.state_t == ABORT && !tx_db.count(tx_entry->txid)) {
              pthread_rwlock_wrlock(&tx_db_lock);
              tx_db.insert (std::make_pair((uint64_t)tx_entry->txid, (Tx_entry*)tx_entry));
              pthread_rwlock_unlock(&tx_db_lock);
              //TODO: Send Ack asynchronously. Probably not needed because these are only for bookkeeping and will be taken care of on restarts.
          } else if(entry.state_t == PREPARE && !tx_db.count(tx_entry->txid)) {
              pthread_rwlock_wrlock(&tx_db_lock);
              tx_db.insert (std::make_pair((uint64_t)tx_entry->txid, (Tx_entry*)tx_entry));
              pthread_rwlock_unlock(&tx_db_lock);
	      if(lock_db.count(tx_entry->path)) {

		  file_lock = lock_db[tx_entry->path];

	      } else {
		  pthread_rwlock_t *lock = new pthread_rwlock_t();
		  if (pthread_rwlock_init(lock,NULL) != 0) {
		      printf("Can't create lock\n");
		  }

		  lock_db.insert(std::make_pair((string)tx_entry->path, (pthread_rwlock_t *)lock));
		  file_lock = lock;
	      }
              pthread_rwlock_wrlock(file_lock);

              // Send CommitVote to coordinator which will trigger a commit.
              struct timespec time;
              clock_gettime(CLOCK_MONOTONIC, &time);
              leader->CommitVote(tx_entry->txid, myconn);
              tx_entry->timestamp = time.tv_sec*NSEC + time.tv_nsec;
          } else {
          }
      }
      pthread_rwlock_unlock(&log_lock);
  }

  private:

  static unordered_map<uint64_t, Tx_entry *> tx_db;
  static unordered_map<string, pthread_rwlock_t *> lock_db;
  static pthread_rwlock_t tx_db_lock;
  static pthread_rwlock_t log_lock;

  Status Fetch(ServerContext* context, const BackendFetchRequest* request,
               BackendFetchReply* reply) override {

    pthread_rwlock_t *file_lock;
    int fd;
    int bytes;
    char *buf;
    struct stat st; 
    if(lock_db.count(request->path())) {

        file_lock = lock_db[request->path()];

    } else {
        pthread_rwlock_t *lock = new pthread_rwlock_t();
        if (pthread_rwlock_init(lock,NULL) != 0) {
            printf("Can't create lock\n");
        }

        lock_db.insert(std::make_pair((string)request->path(), (pthread_rwlock_t *)lock));
        file_lock = lock;
    }

    pthread_rwlock_rdlock(file_lock); 
    fd = open(request->path().c_str(), O_RDONLY);
    if(fd<0) {
        pthread_rwlock_unlock(file_lock);
        reply->set_error(-errno);
        return Status::OK;
    }    
    fstat(fd, &st);
    buf = (char *)malloc(st.st_size);
    bytes = read(fd, buf, st.st_size);
    if(bytes!=st.st_size) {
        pthread_rwlock_unlock(file_lock);
        reply->set_error(-errno);
        return Status::OK;
    }
    close(fd);
    pthread_rwlock_unlock(file_lock); 
   
    reply->set_error(0);
    reply->set_buf(std::string(buf,st.st_size));
    reply->set_size(st.st_size);
    free(buf);
    return Status::OK;
    
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

  int tpcLog(Tx_entry* tx_entry) {
      Tx_log_entry log_entry;
      log_entry.txid = tx_entry->txid;
      log_entry.state_t = tx_entry->state_t;
      strncpy(log_entry.path, tx_entry->path.c_str(), PATH_MAX);
      strncpy(log_entry.op, tx_entry->op.c_str(), OP_LENGTH);
      char buf[sizeof(Tx_log_entry)];
      ::memcpy(buf, &log_entry, sizeof(Tx_log_entry));
      int rc = 0;
      pthread_rwlock_wrlock(&log_lock);
      struct timespec before, after, delta;
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
  //    printf("TIME: %llu\n", delta.tv_sec*1000000000LLU + delta.tv_nsec);
      times.push_back(delta.tv_sec*1000000000LLU + delta.tv_nsec);
      return 0;
  }

  Status Prepare(ServerContext* context, const PrepareRequest* request,
               PrepareReply* reply) override {

      cmt++;

      // cout << "Prepare start for tx:" << request->txid() <<"File: " << request->path() << " Commit Pending:" << cmt << endl;
      pthread_rwlock_t *file_lock = NULL;  /* Get the approprite lock var from lock_db */
      int fd, ffd, n, rc;
      char buf[BUF_SIZE];
      char path[PATH_MAX];
      path[0] = '\0';
     

      
//      strncat(path, (request->path()).c_str(), PATH_MAX);
//      strncat(path, ".", PATH_MAX);

      if(lock_db.count(request->path())) {

          file_lock = lock_db[request->path()];

      } else {
          pthread_rwlock_t *lock = new pthread_rwlock_t();
          if (pthread_rwlock_init(lock,NULL) != 0) {
              printf("Can't create lock\n");
          }    
       
          lock_db.insert(std::make_pair((string)request->path(), (pthread_rwlock_t *)lock));
          file_lock = lock;
      }


      printf("File lock: %p : %d", file_lock, *file_lock);
      fflush(stdout);
      Tx_entry* tx_entry = new Tx_entry();
      tx_entry->txid = request->txid();
      tx_entry->state_t = PREPARE;
      tx_entry->path = request->path();
      tx_entry->op   = request->op();

      struct timespec time;
      clock_gettime(CLOCK_MONOTONIC, &time);
      tx_entry->timestamp = time.tv_sec*NSEC + time.tv_nsec;
      
      pthread_rwlock_wrlock(&tx_db_lock);
      tx_db.insert (std::make_pair((uint64_t)request->txid(), (Tx_entry*)tx_entry));
      pthread_rwlock_unlock(&tx_db_lock);

      if(tx_entry->op == "store") {
          sprintf(path, "%llu", request->txid());

          fd = open(path, O_RDWR | O_CREAT | O_TRUNC, 0666);

	  if(fd == -1) {
	      // TODO: Abort
	  }

	  pthread_rwlock_rdlock(file_lock);
          struct timespec before, after, delta;
          clock_gettime(CLOCK_MONOTONIC, &before);
	  ffd = open((request->path()).c_str(), O_RDONLY);
	  while((n = read(ffd, buf, BUF_SIZE)) > 0) {
	      write(fd, buf, n);
	  }
	  close(ffd);
	  pthread_rwlock_unlock(file_lock);

	  
	  write(fd, (request->buf()).data(), request->size());
          fsync(fd);
	  close(fd);
          clock_gettime(CLOCK_MONOTONIC, &after);
          delta = diff(before, after);
//          printf("TIME: %llu\n", delta.tv_sec*1000000000LLU + delta.tv_nsec);
          times.push_back(delta.tv_sec*1000000000LLU + delta.tv_nsec);
      }

      pthread_rwlock_wrlock(file_lock);

      rc = tpcLog(tx_entry);  /* TODO: Need to store the key too in log */
      if(rc != 0) {
          // TODO: Abort
      }

      printf("Prepare logging done.\n");


      reply->set_txid(request->txid());
      reply->set_result("ok");

      //printf("Sleeping now...\n");
      //sleep(60);

      // cout << "Prepare end for tx:" << request->txid() << endl;
      //// cout << "Sleeping now ..." << endl;
      //sleep(60); 
      return Status::OK;
  }

  Status Abort(ServerContext* context, const AbortRequest* request,
               AbortReply* reply) override {

      pthread_rwlock_t *file_lock = NULL;
      char path[PATH_MAX];
      path[0] = '\0';
      int rc;

      printf("Started Abort...\n");
      fflush(stdout);

      if(tx_db.count(request->txid())) {
          sprintf(path, "%llu", request->txid());
	  Tx_entry *tx_entry = tx_db[request->txid()];
	  tx_entry->state_t = ABORT;
	  file_lock = lock_db[tx_entry->path];      
	  unlink(path);
	  rc = tpcLog(tx_entry);
	  // Handle rc ??? 
	  pthread_rwlock_unlock(file_lock);
      }

      printf("Starting Log...\n");
      fflush(stdout);
      
      reply->set_txid(request->txid());
      reply->set_result("ok");
      return Status::OK;
  }

  Status Commit(ServerContext* context, const CommitRequest* request,
               CommitReply* reply) override {

      // cout << "Commit start for tx:" << request->txid() << endl; 

      pthread_rwlock_t *file_lock = NULL;  /* Nedd to get the appropriate lock from lock_db */
      char path[PATH_MAX];
      path[0] = '\0';
      int rc;

      //printf("Sleeping in commit now...\n");
      //sleep(15);

     
      if(tx_db.count(request->txid())) { 

	  Tx_entry *tx_entry = tx_db[request->txid()];

          if(tx_entry->state_t != COMMIT) {
	      file_lock = lock_db[tx_entry->path];

	      if(tx_entry->op=="store") {
		  sprintf(path, "%llu", request->txid());
		  rename(path, tx_entry->path.c_str()); /* Need to be idempotent otherwise rollback enabled*/
                  printf("Renamed while committing\n");
	      } else if (tx_entry->op=="delete") {
		  unlink(tx_entry->path.c_str()); /* Need to be idempotent otherwise rollback enabled*/
                  printf("Deleted while committing\n");
	      } else {
		  // cout << "Error: unknown op\n" << endl;
	      }
	      tx_entry->state_t = COMMIT;
	      rc = tpcLog(tx_entry);
              printf("Commit logging done.\n");   
	      // Handle rc ???
	      pthread_rwlock_unlock(file_lock);
          }
      } else {
          // This should never happen.
          printf("Commit entry not found for txn: %llu\n", request->txid());
      }

      reply->set_txid(request->txid());
      reply->set_result("ok");

      // cout << "Commit end for tx:" << request->txid() << endl;
      cmt--;

      for(auto it=times.begin(); it!=times.end(); it++) {
          cout << *it << " ";
      }
      cout << endl;
      times.clear();

      //exit(0);
      return Status::OK;
  }

};

unordered_map<uint64_t, Tx_entry *> BackendServiceImpl::tx_db;
unordered_map<string, pthread_rwlock_t *> BackendServiceImpl::lock_db;
pthread_rwlock_t BackendServiceImpl::tx_db_lock;
pthread_rwlock_t BackendServiceImpl::log_lock;

void RunServer(int recover, int port) {
  std::stringstream out;
  out << port;
  std::string server_address("0.0.0.0:" + out.str());
  BackendServiceImpl service;

  service.Initialize();
  if(recover!=0) {
      service.Recover();
  } else {
      ftruncate(log_fd, 0);
      fsync(log_fd);
  }

  ServerBuilder builder;
  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  // Register "service" as the instance through which we'll communicate with
  // clients. In this case it corresponds to an *synchronous* service.
  builder.RegisterService(&service);
  // Finally assemble the server.
  std::unique_ptr<Server> server(builder.BuildAndStart());
  serv = server.get();
  // // cout << "Server listening on " << server_address << std::endl;

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  server->Wait();
}

int main(int argc, char** argv) {

/*  if(argc!=2) {
      // // cout << "Usage: server <afs_path>" << std::endl;
      return -1;
  }

  strncpy(afs_path, argv[1], PATH_MAX);

  // // cout << afs_path << std::endl; */

  int recover = 0;
  string servername = "localhost";
  int port = 12349;

  if(argc==4) {
      recover = atoi(argv[1]);
      servername = argv[2];
      port    = atoi(argv[3]);
  } else {
      printf("Usage: ./backend_store <recovery_mode> <client_hostname_or_ip> <backend_store_port_num>\n"
             "recovery_mode: 0 or 1, 0: no recovery from log, 1: recovery will be done from log\n");
      return -1;
  }


  GreeterClient greeter(
       grpc::CreateChannel(servername+":12348", grpc::InsecureCredentials()));

  leader = &greeter;

  char hostname[1024];
  gethostname(hostname, 1023); 
  std::string conn(hostname);
  std::stringstream out;
  out << port;
  myconn = conn + ":" + out.str();  
  leader->Register(myconn); 

  log_fd = open("store_log", O_CREAT | O_RDWR | O_APPEND, 0666);
  RunServer(recover, port);
  printf("Server shut\n");
  close(log_fd);
  unlink("store_log");
  return 0;
}
