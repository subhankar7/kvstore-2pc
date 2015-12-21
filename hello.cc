/* Example Usage. This is the client using the coordinator service through local RPC calls */


#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#include <grpc++/grpc++.h>
#include <sys/stat.h>
#include <time.h>
#include "greeter_client.h"

#include "helloworld.grpc.pb.h"


using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using helloworld::HelloRequest;
using helloworld::HelloReply; 
using helloworld::Greeter;

static GreeterClient *ctx;

unsigned long
hash(unsigned char *str)
{
    unsigned long hash = 5381;
    int c;

    while (c = *str++)
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
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


int main(int argc, char *argv[])
{
        GreeterClient greeter(
         grpc::CreateChannel("localhost:12348", grpc::InsecureCredentials()));

        ctx = &greeter;

        char path[PATH_MAX];

/*        for (int i=0; i<10; i++) {
            sprintf(path, "nhello%d", i);
            for(int j=0; j<10; j++) {
                ctx->Store(std::string(path), (const char *)"Hello World", strlen("Hello World"));
            }
        } */

        struct timespec before, after, delta;
        clock_gettime(CLOCK_MONOTONIC, &before);
        ctx->Store("nhello", (const char *)"Hello World", strlen("Hello World"));
        clock_gettime(CLOCK_MONOTONIC, &after);
        //ctx->Delete(std::string(path));

        delta = diff(before, after);

        printf("%llu\n", delta.tv_sec*1000000000LLU+delta.tv_nsec); 

        char *buf;
        int size;
        int rc = ctx->Fetch("nhello", &buf, &size);
        printf("%s\n", buf);
        return 0;
}
