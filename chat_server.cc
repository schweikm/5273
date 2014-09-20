// chat_server.cc
// Marc Schweikert
// CSCI 5273

#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <map>
#include <vector>
#include <string>
#include <unistd.h>

#include <netinet/in.h>
#include <sys/socket.h>

using std::map;
using std::string;
using std::vector;

/* constants */
const int QUEUE_LENGTH = 32;
const int BUFFER_SIZE = 4096;
const int MESSAGE_MAX_LENGTH = 79;	// 80 characters minus newline
const int RECEIVE_TIMEOUT = 600;	// seconds

const string CMD_SUBMIT = "Submit";
const string CMD_GET_NEXT = "GetNext";
const string CMD_GET_ALL = "GetAll";
const string CMD_LEAVE = "Leave";

/* function declarations */
int do_submit(const int, const string&);
int do_get_next(const int);
int do_get_all(const int);
int do_leave(const int);


/*
 * main
 */
int main(const int, const char* const argv[]) {
	const int server_socket = atoi(argv[0]);
	printf("Chat Server started\n");

	// let's start up our data structure
	map<int, int> client_last_message();
	vector<string> all_messages();

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

	// select timeout
	struct timeval select_timeout;
	select_timeout.tv_sec = RECEIVE_TIMEOUT;
	select_timeout.tv_usec = 0;

	for(;;) {
		memcpy(&rfds, &afds, sizeof(rfds));
		int client_socket = -1;

		const int select_code = select(nfds, &rfds, (fd_set *)0, (fd_set *)0, &select_timeout);
		// error
		if (select_code < 0) {
			fprintf(stderr, "select: %s\n", strerror(errno));
			exit(1);
		}

		// timeout
		if (0 == select_code) {
			printf("Chat server closing after select timeout!\n");
			close(server_socket);
			exit(0);
		}

		if (FD_ISSET(server_socket, &rfds)) {
			unsigned int alen = sizeof(fsin);
			client_socket = accept(server_socket, (struct sockaddr *)&fsin, &alen);

			if (client_socket < 0) {
				fprintf(stderr, "accept: %s\n", strerror(errno));
			}

			FD_SET(client_socket, &afds);
		}

		for (int client_socket = 0; client_socket < nfds; ++client_socket) {
			if (client_socket != server_socket && FD_ISSET(client_socket, &rfds)) {
				// read the client message
				char recv_buffer[BUFFER_SIZE];
				memset(recv_buffer, 0, BUFFER_SIZE);

				const int num_bytes = read(client_socket, recv_buffer, sizeof recv_buffer);
				if (num_bytes <= 0) {
					close(client_socket);
					FD_CLR(client_socket, &afds);
					fprintf(stderr, "error or client disconnect: %s\n", strerror(errno));
					continue;
				}

				// prevent buffer overflow
				#ifdef DEBUG
				assert(num_bytes < BUFFER_SIZE);
				#endif
				recv_buffer[num_bytes] = 0;
				recv_buffer[BUFFER_SIZE - 1] = 0;

				// parse message
				string message(recv_buffer);

				// removing tailing newline
				const size_t newline_index = message.find_last_of("\r\n");
				message.erase(newline_index);

				// try to split the string
				const size_t first_space = message.find_first_of(" ");
				string command = message;
				string arguments;

				if (string::npos != first_space) {
					command = message.substr(0, first_space); 
					arguments = message.substr(first_space + 1); 
				}

				// perform the requested operation
				if (CMD_SUBMIT == command) {
					// ensure we have valid strings
					if (0 == arguments.size()) {
						fprintf(stderr, "Parsed message has zero length.  Ignoring command.\n");
						continue;
					}

					do_submit(client_socket, arguments);
				}
				else if (CMD_GET_NEXT == command) {
					do_get_next(client_socket);
				}
				else if (CMD_GET_ALL == command) {
					do_get_all(client_socket);
				}
				else if (CMD_LEAVE == command) {
					do_leave(client_socket);
				}
				else {
					fprintf(stderr, "Chat Server - unrecognized command:  ->%s<-\n", command.c_str());
				}
			}
		}
	}

	close(server_socket);
	return 0;
}

/*
 * do_submit
 */
int do_submit(const int in_client_socket, const string& in_message) {
	// we only support 80 character messages
	string message_text = in_message;
	if (message_text.length() > MESSAGE_MAX_LENGTH) {
		fprintf(stderr, "Message is greater than %d characters - truncating.\n", MESSAGE_MAX_LENGTH);
		message_text = message_text.substr(0, MESSAGE_MAX_LENGTH);
	}

	printf("submit:  message is |%s|\n", message_text.c_str());
	return 0;
}

/*
 * do_get_next
 */
int do_get_next(const int) {
	printf("get_next\n");
	return 0;
}

/*
 * do_get_all
 */
int do_get_all(const int) {
	printf("get_all\n");
	return 0;
}

/*
 * do_leave
 */
int do_leave(const int) {
	printf("leave\n");
	return 0;
}

