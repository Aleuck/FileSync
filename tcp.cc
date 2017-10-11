#include <string>
#include <cstring>
#include <stdexcept>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "tcp.hh"

#define QUEUE_SIZE 5

using namespace std;

TCPServer::TCPServer(void) {
  //throw runtime_error("Port not specified.");
  socklen_t sock_size;
  sock_size = sizeof(addr);
  memset((char *) &addr, 0, sock_size);
}

TCPServer::TCPServer(int port) {
  TCPServer();
  socklen_t sock_size = sizeof(addr);

  // create TCP socket
  sock_d = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
  if (sock_d < 0) {
    throw std::runtime_error("Could not create socket.");
  }

  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(port);
  addr.sin_family = AF_INET;

  if (bind(sock_d, (struct sockaddr *)&addr, sock_size) < 0) {
    throw std::runtime_error("Could not bind socket to address.");
  }
  if(listen(sock_d, QUEUE_SIZE) < 0) {
    throw std::runtime_error("Could not start listenning.");
  }
}

TCPConnection* TCPServer::accept() {
  TCPConnection *con = new TCPConnection;
  socklen_t sock_size;
  sock_size = sizeof(con->addr);

  con->sock_d = ::accept(sock_d, (struct sockaddr *) &(con->addr), &sock_size);
  if (con->sock_d < 0) {
    delete con;
    throw std::runtime_error("Could not accept connection");
  }
  return con;
}

TCPConnection::TCPConnection() {
  socklen_t sock_size = sizeof(addr);
  memset((char *) &addr, 0, sock_size);
  sock_d = 0;
}

ssize_t TCPConnection::send(char* buffer, size_t length) {
  size_t total_sent = 0;
  while (total_sent < length) {
    ssize_t bytes;
    bytes = ::send(sock_d, buffer + total_sent, length - total_sent, 0);
    if (bytes > 0) {
      total_sent += bytes;
    } else {
      return bytes;
    }
  }
  return total_sent;
}

ssize_t TCPConnection::recv(char *buffer, size_t length) {
  size_t total_received = 0;
  while (total_received < length) {
    ssize_t bytes;
    bytes = ::recv(sock_d, buffer + total_received, length - total_received, 0);
    if (bytes > 0) {
      total_received += bytes;
    } else {
      return bytes;
    }
  }
  return total_received;
}
