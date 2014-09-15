// chat_server.cc
// Marc Schweikert
// CSCI 5273

#include <cstdio>
#include <cstdlib>

int main(const int, const char* const argv[]) {
	const int server_socket = atoi(argv[1]);
	printf("Chat Server socket |%d|\n", server_socket);

	return 0;
}

