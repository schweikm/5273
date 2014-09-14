// chat_coordinator.cc
// Marc Schweikert
// CSCI 5273 Fall 2014

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
const int BUFFER_SIZE = 256;


/* function declarations */
int create_server_socket();


/*
 * main
 */
int main() {
	// create the server socket
	const int server_socket = create_server_socket();

	//
	// begin main loop
	//
	char receive_buffer[BUFFER_SIZE];
	struct sockaddr_in remote_addr;
	socklen_t remote_addr_len = sizeof(remote_addr);

	for (;;) {
		// zero out data
		memset(receive_buffer, 0, BUFFER_SIZE);
		memset(&remote_addr, 0, sizeof(remote_addr));

		// receive data on the socket
		const ssize_t recv_len = recvfrom(server_socket, receive_buffer, BUFFER_SIZE, 0, (struct sockaddr *)&remote_addr, &remote_addr_len);
		if (-1 == recv_len) {
			fprintf(stderr, "Error during call to recvfrom().  Error is %s\n", strerror(errno));
		}
		else if (0 == recv_len) {
			printf("recvfrom() encountered orderly shutdown");
			break;
		}
		else {
			// prevent buffer overflow
			assert(recv_len < BUFFER_SIZE);
			receive_buffer[recv_len] = 0;
			receive_buffer[BUFFER_SIZE - 1] = 0;

			// parse message
			const string message(receive_buffer);
			printf("received message:  |%s|\n", message.c_str());
		}
	}

	close(server_socket);
	return 0;
}

/*
 * create_server_socket
 */
int create_server_socket()
{
	// allocate a socket
	const int server_socket = socket(PF_INET, SOCK_DGRAM, 0);
	if (server_socket < 0) {
		fprintf(stderr, "Unable to create server socket.  Error is %s\n", strerror(errno));
		exit (1);
	}

	// an Internet endpoint address
	struct sockaddr_in sin;
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;			// Internet Protocol
	sin.sin_addr.s_addr = INADDR_ANY;	// use any available address
	sin.sin_port=htons(0);				// request a port number to be allocated by bind

	// bind the socket
	if (bind(server_socket, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
		fprintf(stderr, "Unable to bind to port.  Error is %s\n", strerror(errno));
		exit(2);
	}

	// find the port number that we were provided
	socklen_t socklen = sizeof(sin);
	if (getsockname(server_socket, (struct sockaddr *)&sin, &socklen) < 0) {
		fprintf(stderr, "getsockname called failed!  Error is %s\n", strerror(errno));
		exit(3);
	}

	// print the UDP port number
	printf("Server port number is %d\n", ntohs(sin.sin_port));

	return server_socket;
}

