#include "util.hpp"
#include "server.hpp"
#include "serverUI.hpp"

using namespace std;

void* run_server(void* arg);
void close_server(int sig);

pthread_t thread_ui;
pthread_t thread_server;

TCPServer::TCPServer(void) {
  throw runtime_error("Port not specified.");
}

TCPServer::TCPServer(int port) {
  socklen_t sock_size;

  sock_size = sizeof(addr);
  memset((char *) &addr, 0, sock_size);

  // create TCP socket
  sock_l = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
  if (sock_l < 0) {
    throw runtime_error("Could not create socket.");
  }

  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(port);
  addr.sin_family = AF_INET;

  if (bind(sock_l, (struct sockaddr *)&addr, sock_size) < 0) {
    throw runtime_error("Could not bind socket to address.");
  }
  if(listen(sock_l, QUEUE_SIZE) < 0) {
    throw runtime_error("Could not listen");
  }
}

TCPConnection::TCPConnection() {
  sock_d = 0;
}

TCPConnection* TCPServer::accept_con() {
  TCPConnection *con = new TCPConnection;
  socklen_t sock_size;
  sock_size = sizeof(con->addr);
  memset((char *) &con->addr, 0, sock_size);

  con->sock_d = accept(sock_l, (struct sockaddr *) &(con->addr), &sock_size);
  if (con->sock_d < 0) {
    delete con;
    throw runtime_error("could not accept connection");
  }
  return con;
}

ssize_t TCPConnection::send_bytes(char* buffer, size_t length) {
  size_t total_sent = 0;
  while (total_sent < length) {
    ssize_t bytes;
    bytes = send(sock_d, buffer+total_sent, length - total_sent, 0);
    if (bytes > 0) {
      total_sent += bytes;
    } else if (bytes == ENOTCONN) {
      throw runtime_error("send: Socket not connected.");
    } else {

    }
  }
  return total_sent;
}

ssize_t TCPConnection::recv_bytes(char *buffer, size_t length) {
  size_t total_received = 0;
  while (total_received < length) {
    ssize_t bytes;
    bytes = recv(sock_d, buffer+total_received, length - total_received, 0);
    if (bytes > 0) {
      total_received += bytes;
    } else {
      if (bytes == ENOTCONN) {
        throw runtime_error("recv: Socket not connected.");
      }
    }
  }
  return total_received;
}

int main(int argc, char* argv[]) {
  TCPServer* server;
  int errCode, port;
  signal(SIGINT, close_server);
  if (argc < 2) {
    cerr << "Usage: " << argv[0] << " <port>" << endl;
    exit(0);
  }
  port = atoi(argv[1]);
  cout << "starting service..." << endl;
  try {
    server = new TCPServer(port);
  }
  catch (runtime_error e) {
    cerr << e.what() << endl;
    exit(1);
  }
  errCode = pthread_create(&thread_server, NULL, run_server, (void*) server);
  cout << "starting interface..." << endl;
  errCode = pthread_create(&thread_ui, NULL, serverUI, (void*) server);
  if (errCode != 0) {
    cout << "Error starting interface." << endl;
    exit(1);
  }
  pthread_join(thread_ui, NULL);
  pthread_join(thread_server, NULL);
  delete server;
  exit(0);
}

void* run_server(void* arg) {
  TCPServer* serv = (TCPServer*) arg;

  return NULL;
}

void close_server(int sig) {
  pthread_kill(thread_ui, sig);
  pthread_join(thread_ui, NULL);
  cout << "Stopping service..." << sig << endl;
  pthread_kill(thread_server, sig);
  pthread_join(thread_server, NULL);
  exit(0);
}
