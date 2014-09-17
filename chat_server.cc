// chat_server.cc
// Marc Schweikert
// CSCI 5273

#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <unistd.h>

#include <netinet/in.h>
#include <sys/socket.h>

using std::string;

/* constants */
const int QUEUE_LENGTH = 32;
const int BUFFER_SIZE = 4096;


/*
 * main
 */
int main(const int, const char* const argv[]) {
	const int server_socket = atoi(argv[0]);
	printf("Chat Server started\n");

	// our socket has already been created and bound
	// we just need to start listening
	if (listen(server_socket, QUEUE_LENGTH) < 0) {
		fprintf(stderr, "Failed to listen on socket.  Error is %s\n", strerror(errno));
	}

	//
	// BEGIN MAIN LOOP
	//

	struct sockaddr_in fsin;    /* the from address of a client */
	fd_set  rfds;           /* read file descriptor set */
	fd_set  afds;           /* active file descriptor set   */

	int nfds = getdtablesize();
	FD_ZERO(&afds);
	FD_SET(server_socket, &afds);

	for(;;) {
		memcpy(&rfds, &afds, sizeof(rfds));
		int client_socket = -1;

		if (select(nfds, &rfds, (fd_set *)0, (fd_set *)0,(struct timeval *)0) < 0) {
			fprintf(stderr, "select: %s\n", strerror(errno));
		}

		if (FD_ISSET(server_socket, &rfds)) {
			unsigned int alen = sizeof(fsin);
			client_socket = accept(server_socket, (struct sockaddr *)&fsin, &alen);

			if (client_socket < 0) {
				fprintf(stderr, "accept: %s\n", strerror(errno));
			}

			FD_SET(client_socket, &afds);
		}

		for (int fd = 0; fd < nfds; ++fd) {
			if (fd != server_socket && FD_ISSET(fd, &rfds)) {
				// read the client message
				char recv_buffer[BUFFER_SIZE];
				memset(recv_buffer, 0, BUFFER_SIZE);

				const int num_bytes = read(fd, recv_buffer, sizeof recv_buffer);
				if (num_bytes < 0) {
					close(fd);
					FD_CLR(fd, &afds);
					fprintf(stderr, "echo read: %s\n", strerror(errno));
				}

				// prevent buffer overflow
				assert(num_bytes < BUFFER_SIZE);
				recv_buffer[num_bytes] = 0;
				recv_buffer[BUFFER_SIZE - 1] = 0;

				// parse message
				string message(recv_buffer);

				// removing tailing newline
				const size_t newline_index = message.find_last_of("\r\n");
				message.erase(newline_index);

				// debug echo
				printf("GOT MESSAGE!  Client |%d| - parsed message:  |%s|\n", fd, message.c_str());
			}
		}
	}

	close(server_socket);
	return 0;
}

