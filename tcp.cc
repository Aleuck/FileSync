#include <string>
#include <cstring>
#include <stdexcept>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "tcp.hh"

using namespace std;

TCPSock::TCPSock(void) {
  sock_d = 0;
  memset((char *) &addr, 0, sizeof(addr));
}

TCPServer::TCPServer(void) {
  // create TCP socket
  sock_d = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
  if (sock_d < 0) {
    throw std::runtime_error("Could not create socket.");
  }
}

void TCPServer::bind(int port) {
  socklen_t sock_size = sizeof(addr);
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(port);
  addr.sin_family = AF_INET;

  if (::bind(sock_d, (struct sockaddr *)&addr, sock_size) < 0) {
    throw std::runtime_error("Could not bind socket to address.");
  }
}

void TCPServer::listen(int queue_size) {
  if(::listen(sock_d, queue_size) < 0) {
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

TCPClient::TCPClient(void) {
  // create TCP socket
  sock_d = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
  if (sock_d < 0) {
    throw std::runtime_error("Could not create socket.");
  }
}

TCPConnection* TCPClient::connect(std::string addr, int port) {
  TCPConnection *con = new TCPConnection;
  socklen_t sock_size;
  sock_size = sizeof(con->addr);

  con->addr.sin_addr.s_addr = inet_addr(addr.c_str());
  con->addr.sin_family = AF_INET;
  con->addr.sin_port = htons(port);

  con->sock_d = ::connect(sock_d, (struct sockaddr *) &(con->addr), sock_size);
  if (con->sock_d < 0) {
    delete con;
    switch (errno) {
      case EADDRNOTAVAIL:
        throw std::runtime_error("Could not connect (EADDRNOTAVAIL)");
        break;
      case EAFNOSUPPORT:
        throw std::runtime_error("Could not connect (EAFNOSUPPORT)");
        break;
      case EALREADY:
        throw std::runtime_error("Could not connect (EALREADY)");
        break;
      case EBADF:
        throw std::runtime_error("Could not connect (EBADF)");
        break;
      case ECONNREFUSED:
        throw std::runtime_error("Could not connect (ECONNREFUSED)");
        break;
      case EFAULT:
        throw std::runtime_error("Could not connect (EFAULT)");
        break;
      case EINPROGRESS:
        throw std::runtime_error("Could not connect (EINPROGRESS)");
        break;
      case EINTR:
        throw std::runtime_error("Could not connect (EINTR)");
        break;
      case EISCONN:
        throw std::runtime_error("Could not connect (EISCONN)");
        break;
      case ENETUNREACH:
        throw std::runtime_error("Could not connect (ENETUNREACH)");
        break;
      case ENOTSOCK:
        throw std::runtime_error("Could not connect (ENOTSOCK)");
        break;
      case EPROTOTYPE:
        throw std::runtime_error("Could not connect (EPROTOTYPE)");
        break;
      case ETIMEDOUT:
        throw std::runtime_error("Could not connect (ETIMEDOUT)");
        break;
      case EACCES:
        throw std::runtime_error("Could not connect (EACCES)");
        break;
      case EADDRINUSE:
        throw std::runtime_error("Could not connect (EADDRINUSE)");
        break;
      case ECONNRESET:
        throw std::runtime_error("Could not connect (ECONNRESET)");
        break;
      case EHOSTUNREACH:
        throw std::runtime_error("Could not connect (EHOSTUNREACH)");
        break;
      case EINVAL:
        throw std::runtime_error("Could not connect (EINVAL)");
        break;
      case ENAMETOOLONG:
        throw std::runtime_error("Could not connect (ENAMETOOLONG)");
        break;
      case ENETDOWN:
        throw std::runtime_error("Could not connect (ENETDOWN)");
        break;
      case ENOBUFS:
        throw std::runtime_error("Could not connect (ENOBUFS)");
        break;
      case ENOSR:
        throw std::runtime_error("Could not connect (ENOSR)");
        break;
      case EOPNOTSUPP:
        throw std::runtime_error("Could not connect (EOPNOTSUPP)");
        break;
      default:
        throw std::runtime_error("Could not connect");
    }
  }
  return con;
}

TCPConnection::TCPConnection() {
  socklen_t sock_size = sizeof(addr);
  memset((char *) &addr, 0, sock_size);
  sock_d = 0;
}

int TCPConnection::getbuffersizes() {
    int res = 0;
    socklen_t optlen = sizeof(sendbuffer);
    res = getsockopt(sock_d, SOL_SOCKET, SO_SNDBUF, &sendbuffer, &optlen);
    res = getsockopt(sock_d, SOL_SOCKET, SO_SNDBUF, &sendbuffer, &optlen);
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
