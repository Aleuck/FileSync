#include <string>
#include <iostream>
#include <stdexcept>

#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <pthread.h>

#include "tcp.hpp"

#define DEFAULT_PORT 22022
#define NUM_PKGS 300000
#define PKG_SIZE 23020

pthread_t thread_server;
pthread_t thread_client;

int port = DEFAULT_PORT;
int num_pkgs = NUM_PKGS;
size_t pkg_size = PKG_SIZE;

void* run_client(void* arg);

int main(int argc, char* argv[]) {
  TCPServer server;
  TCPConnection* conn;
  if (argc >= 2) {
    pkg_size = atoi(argv[1]);
  }
  if (argc >= 3) {
    num_pkgs = atoi(argv[2]);
  }
  if (argc >= 4) {
    port = atoi(argv[3]);
  }
  std::cerr << "binding and listening..." << '\n';
  try {
    server.bind(port);
    server.listen(3);
  }
  catch (std::runtime_error e) {
    std::cerr << "exception" << '\n';
    std::cerr << e.what() << std::endl;
    exit(1);
  }

  std::cerr << "creating client thread..." << '\n';
  int errCode = pthread_create(&thread_client, NULL, run_client, NULL);
  std::cerr << "accepting..." << '\n';
  try {
    conn = server.accept();
  }
  catch (std::runtime_error e) {
    std::cerr << "accept exception:" << '\n';
    std::cerr << e.what() << std::endl;
    exit(1);
  }
  char* buffer = (char*) malloc(pkg_size);
  for(int i = 0; i < num_pkgs; ++i) {
    unsigned char ch = i % 256;
    memset(buffer, ch, pkg_size);
    try {
      conn->send((char*) buffer, pkg_size);
    }
    catch (std::runtime_error e) {
      fprintf(stderr, "Problem on package %d : ", i);
      std::cerr << e.what() << '\n';
    }
  }
  pthread_join(thread_client, NULL);
  exit(0);
}

void* run_client(void* arg) {
  TCPClient client;
  std::cerr << "connecting..." << '\n';
  try {
    client.connect("127.0.0.1", port);
  }
  catch (std::runtime_error e) {
    std::cerr << "connect exception:" << '\n';
    std::cerr << e.what() << std::endl;
    exit(1);
  }
  unsigned char* buffer = (unsigned char*) malloc(pkg_size);
  for (int i = 0; i < num_pkgs; ++i) {
    unsigned char ch = i % 256;
    try {
      client.recv((char*) buffer, pkg_size);
    }
    catch (std::runtime_error e) {
      std::cerr << e.what() << '\n';
    }
    if (buffer[0] != ch || buffer[pkg_size-1] != ch) {
      fprintf(stderr, "Problem on package %d ", i);
      fprintf(stderr, "ch(%d), b[first](%d), b[last](%d)\n", ch, buffer[0], buffer[pkg_size-1]);
    }
  }
  return NULL;
}
