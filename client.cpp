#include "client.hpp"
#include "clientUI.hpp"

using namespace std;

int main(int argc, char* argv[]) {
  cout << "Starting client..." << endl;

  Session session;

  if (argc < 4) {
    cerr << "Usage: $ ";
    cerr << argv[0] << " <username> <address> <port>" << endl;
    return 1;
  }
  try {
    session.connect_to_server(argv[2], atoi(argv[3]));
  }
  catch (runtime_error e) {
    cerr << e.what() << endl;
    return 1;
  }
  return 0;
}


Session::Session() {
  memset(&server, 0, sizeof(server));
  connected = 0;
  pthread_mutex_init(&mutex, NULL);
}
Session::~Session() {
  //TODO
}
void Session::connect_to_server(string address, int port) {
  connection = socket(AF_INET, SOCK_STREAM, 0);
  if (connection < 0) {
    throw runtime_error("Could not create socket.");
  }
  server.sin_addr.s_addr = inet_addr(address.c_str());
  server.sin_family = AF_INET;
  server.sin_port = htons(port);
  if (connect(connection, (struct sockaddr *) &server, sizeof(server)) < 0) {
    throw runtime_error("Could not connect to server.");
  }
  connected = true;
}
bool Session::login(string userid) {
  // login code;
  return 0;
}
