// chat_coordinator.cc
// Marc Schweikert
// CSCI 5273 Fall 2014

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <unistd.h>
//#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
//#include <netdb.h>


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

	// begin main loop
	unsigned char receive_buffer[BUFFER_SIZE];	// receive buffer
	struct sockaddr_in remote_addr;
	socklen_t addrlen = sizeof(remote_addr);

	for (;;) {
		// zero out data
		memset(receive_buffer, 0, BUFFER_SIZE);
		memset(&remote_addr, 0, sizeof(remote_addr));

		// receive data on the socket
		const int recv_len = recvfrom(server_socket, receive_buffer, BUFFER_SIZE, 0, (struct sockaddr *)&remote_addr, &addrlen);
		printf("received %d bytes\n", recv_len);
		if (recv_len > 0) {
			receive_buffer[recv_len] = 0;
			printf("received message: \"%s\"\n", receive_buffer);
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

