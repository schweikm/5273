// chat_server.cc
// Marc Schweikert
// CSCI 5273

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>

#include <netinet/in.h>
#include <sys/socket.h>

/* constants */
const int QUEUE_LENGTH = 32;

int echo(const int);

/*
 * main
 */
int main(const int, const char* const argv[]) {
	const int server_socket = atoi(argv[1]);
	printf("Chat Server socket |%d|\n", server_socket);

	// our socket has already been created and bound
	// we just need to start listening
	if (listen(server_socket, QUEUE_LENGTH) < 0) {
		fprintf(stderr, "Failed to listen on socket.  Error is %s\n", strerror(errno));
	}

	//
	// BEGIN MAIN LOOP
	//

	fd_set rfds;			/* read file descriptor set */
	fd_set afds;			/* active file descriptor set */

	int nfds = getdtablesize();
	FD_ZERO(&afds);
	FD_SET(server_socket, &afds);

	for(;;) {
		memcpy(&rfds, &afds, sizeof(rfds));

		if (select(nfds, &rfds, (fd_set *)0, (fd_set *)0, (struct timeval *)0) < 0) {
			fprintf(stderr, "Error in select.  Error is %s\n", strerror(errno));
		}

		int client_socket = -1;
		if (FD_ISSET(server_socket, &rfds)) {
			struct sockaddr_in fsin;	/* the from address of a client */
			unsigned int alen = sizeof(fsin);
			client_socket = accept(server_socket, (struct sockaddr *)&fsin, &alen);

			if (client_socket < 0) {
				fprintf(stderr, "accept failed: %s\n", strerror(errno));
			}

			FD_SET(client_socket, &afds);
		}

		for (int fd = 0; fd < nfds; ++fd) {
			if (fd != client_socket && FD_ISSET(fd, &rfds)) {
				if (0 == echo(fd)) {
					close(fd);
					FD_CLR(fd, &afds);
				}
			}
		}
	}

	close(server_socket);
	return 0;
}

int echo(const int fd) {
	char buf[1024];
	int cc = read(fd, buf, sizeof buf);

	if (cc < 0) {
		fprintf(stderr, "echo read: %s\n", strerror(errno));
	}

	if (cc && write(fd, buf, cc) < 0) {
		fprintf(stderr, "echo write: %s\n", strerror(errno));
	}

	return cc;
}

