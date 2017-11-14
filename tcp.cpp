#include <string>
#include <iostream>
#include <cstring>
#include <stdexcept>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include "tcp.hpp"

using namespace std;

TCPSock::TCPSock(void) {
  sock_d = 0;
  memset((char *) &addr, 0, sizeof(addr));
}

void TCPSock::close() {
  if (sock_d >= 0) {
    ::close(sock_d);
  }
}

void TCPSock::shutdown() {
  ::shutdown(sock_d, SHUT_RD);
}

TCPServer::TCPServer(void) {
  // create TCP socket
  sock_d = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
  if (sock_d <= 0) {
    throw std::runtime_error("Could not create socket.");
  }
  // fcntl(sock_d, F_SETFL, fcntl(sock_d, F_GETFL, 0) | O_NONBLOCK);
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
  if (con->sock_d <= 0) {
    delete con;
    if (errno == EINTR) return NULL;
    if (errno == EINVAL) return NULL;
    if (errno == EAGAIN) return NULL;
    if (errno == EWOULDBLOCK) return NULL;
    throw std::runtime_error("Could not accept connection");
  }
  return con;
}

TCPClient::TCPClient(void) {
  // create TCP socket
  sock_d = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
  if (sock_d <= 0) {
    throw std::runtime_error("Could not create socket.");
  }
}

void TCPClient::connect(std::string address, int port) {
  int err;
  socklen_t sock_size;
  sock_size = sizeof(addr);
  addr.sin_addr.s_addr = inet_addr(address.c_str());
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);

  err = ::connect(sock_d, (struct sockaddr *) &addr, sock_size);
  if (err) {
    switch (errno) {
      case EADDRNOTAVAIL:
        throw std::runtime_error("EADDRNOTAVAIL: Could not connect.");
        break;
      case EAFNOSUPPORT:
        throw std::runtime_error("EAFNOSUPPORT: Could not connect.");
        break;
      case EALREADY:
        throw std::runtime_error("EALREADY: Could not connect.");
        break;
      case EBADF:
        throw std::runtime_error("EBADF: Could not connect.");
        break;
      case ECONNREFUSED:
        throw std::runtime_error("ECONNREFUSED: Could not connect.");
        break;
      case EFAULT:
        throw std::runtime_error("EFAULT: Could not connect.");
        break;
      case EINPROGRESS:
        throw std::runtime_error("EINPROGRESS: Could not connect.");
        break;
      case EINTR:
        throw std::runtime_error("EINTR: Could not connect.");
        break;
      case EISCONN:
        throw std::runtime_error("EISCONN: Could not connect.");
        break;
      case ENETUNREACH:
        throw std::runtime_error("ENETUNREACH: Could not connect.");
        break;
      case ENOTSOCK:
        throw std::runtime_error("ENOTSOCK: Could not connect.");
        break;
      case EPROTOTYPE:
        throw std::runtime_error("EPROTOTYPE: Could not connect.");
        break;
      case ETIMEDOUT:
        throw std::runtime_error("ETIMEDOUT: Could not connect.");
        break;
      case EACCES:
        throw std::runtime_error("EACCES: Could not connect.");
        break;
      case EADDRINUSE:
        throw std::runtime_error("EADDRINUSE: Could not connect.");
        break;
      case ECONNRESET:
        throw std::runtime_error("ECONNRESET: Could not connect.");
        break;
      case EHOSTUNREACH:
        throw std::runtime_error("EHOSTUNREACH: Could not connect.");
        break;
      case EINVAL:
        throw std::runtime_error("EINVAL: Could not connect.");
        break;
      case ENAMETOOLONG:
        throw std::runtime_error("ENAMETOOLONG: Could not connect.");
        break;
      case ENETDOWN:
        throw std::runtime_error("ENETDOWN: Could not connect.");
        break;
      case ENOBUFS:
        throw std::runtime_error("ENOBUFS: Could not connect.");
        break;
      case ENOSR:
        throw std::runtime_error("ENOSR: Could not connect.");
        break;
      case EOPNOTSUPP:
        throw std::runtime_error("EOPNOTSUPP: Could not connect.");
        break;
      default:
        throw std::runtime_error("Could not connect");
    }
  }
}

TCPConnection::TCPConnection() {
  socklen_t sock_size = sizeof(addr);
  memset((char *) &addr, 0, sock_size);
  sock_d = 0;
}

int TCPConnection::getbuffersizes() {
    socklen_t optlen = sizeof(sendbuffer);
    getsockopt(sock_d, SOL_SOCKET, SO_SNDBUF, &sendbuffer, &optlen);
    return sendbuffer;
}

ssize_t TCPConnection::send(char* buffer, size_t length) {
  size_t total_sent = 0;
  while (total_sent < length) {
    ssize_t bytes;
    bytes = ::send(sock_d, &buffer[total_sent], length - total_sent, 0);
    if (bytes > 0) {
      total_sent += bytes;
    } else {
      if (bytes < 0) {
        switch (errno) {
          case EBADF:
            throw std::runtime_error("(send) EBADF: The socket argument is not a valid file descriptor.");
          case ECONNRESET:
            throw std::runtime_error("(send) ECONNRESET: A connection was forcibly closed by a peer.");
          case EDESTADDRREQ:
            throw std::runtime_error("(send) EDESTADDRREQ: The socket is not connection-mode and no peer address is set.");
          case EFAULT:
            throw std::runtime_error("(send) EFAULT: The buffer parameter can not be accessed.");
          case EINTR:
            throw std::runtime_error("(send) EINTR: A signal interrupted send() before any data was transmitted.");
          case EMSGSIZE:
            throw std::runtime_error("(send) EMSGSIZE: The message is too large be sent all at once, as the socket requires.");
          case ENOTCONN:
            throw std::runtime_error("(send) ENOTCONN: The socket is not connected or otherwise has not had the peer prespecified.");
          case ENOTSOCK:
            throw std::runtime_error("(send) ENOTSOCK: The socket argument does not refer to a socket.");
          case EOPNOTSUPP:
            throw std::runtime_error("(send) EOPNOTSUPP: The socket argument is associated with a socket that does not support one or more of the values set in flags.");
          case EPIPE:
            throw std::runtime_error("(send) EPIPE: The socket is shut down for writing, or the socket is connection-mode and is no longer connected. In the latter case, and if the socket is of type SOCK_STREAM, the SIGPIPE signal is generated to the calling process.");
          case EACCES:
            throw std::runtime_error("(send) EACCES: The calling process does not have the appropriate privileges.");
          case EIO:
            throw std::runtime_error("(send) EIO: An I/O error occurred while reading from or writing to the file system.");
          case ENETDOWN:
            throw std::runtime_error("(send) ENETDOWN: The local interface used to reach the destination is down.");
          case ENETUNREACH:
            throw std::runtime_error("(send) ENETUNREACH: No route to the network is present.");
          case ENOBUFS:
            throw std::runtime_error("(send) ENOBUFS: Insufficient resources were available in the system to perform the operation.");
          case ENOSR:
            throw std::runtime_error("(send) ENOSR: There were insufficient STREAMS resources available for the operation to complete.");
          default:
            throw std::runtime_error("(send) send: Undefined error");
        }
      } else {
        return 0;
      }
    }
  }
  return total_sent;
}

ssize_t TCPConnection::recv(char *buffer, size_t length) {
  size_t total_received = 0;
  while (total_received < length) {
    ssize_t bytes;
    bytes = ::recv(sock_d, &buffer[total_received], length - total_received, 0);
    if (bytes > 0) {
      total_received += bytes;
    } else {
      if (bytes < 0) switch (errno) {
        case EBADF:
          throw std::runtime_error("(recv) EBADF: The argument sockfd is an invalid file descriptor.");

        case ECONNREFUSED:
          throw std::runtime_error("(recv) ECONNREFUSED: A remote host refused to allow the network connection (typically because it is not running the requested service).");

        case EFAULT:
          throw std::runtime_error("(recv) EFAULT: The receive buffer pointer(s) point outside the process's address space.");

        case EINTR:
          throw std::runtime_error("(recv) EINTR: The receive was interrupted by delivery of a signal before any data were available; see signal(7).");

        case EINVAL:
          throw std::runtime_error("(recv) EINVAL: Invalid argument passed.");

        case ENOMEM:
          throw std::runtime_error("(recv) ENOMEM: Could not allocate memory for recvmsg().");

        case ENOTCONN:
          throw std::runtime_error("(recv) ENOTCONN: The socket is associated with a connection-oriented protocol and has not been connected (see connect(2) and accept(2)).");

        case ENOTSOCK:
          throw std::runtime_error("(recv) ENOTSOCK: The file descriptor sockfd does not refer to a socket.");
        default:
          throw std::runtime_error("(recv) Undefined error");
      } else {
        return 0;
      }
    }
  }
  return total_received;
}
