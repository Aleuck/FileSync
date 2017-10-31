#include "client.hpp"
#include "clientUI.hpp"

using namespace std;

int main(int argc, char* argv[]) {
  cout << "Starting client..." << endl;
  FileSyncClient client;
  if (argc < 4) {
    cerr << "Usage: $ ";
    cerr << argv[0] << " <username> <address> <port>" << endl;
    return 1;
  }
  try {
    client.connect(argv[1], atoi(argv[2]))
  }
  catch (runtime_error e) {
    cerr << e.what() << endl;
    return 1;
  }
  return 0;
}

FileSyncClient::FileSyncClient(void) {
}
FileSyncClient::~FileSyncClient(void) {
}
void FileSyncClient::connect(std::string address, int port) {
  tcp.connect(address, port);
  std::cerr << e.what() << '\n';
}
bool FileSyncClient::login(std::string userid) {}
void FileSyncClient::upload_file(std::string filepath) {}
void FileSyncClient::download_file(std::string filename) {}
void FileSyncClient::delete_file(std::string filename) {}
void FileSyncClient::list_files(std::string filename) {}
