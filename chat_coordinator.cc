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
const int MAX_SESSION_NAME_SIZE = 8;

const string CMD_START = "Start";
const string CMD_FIND = "Find";
const string CMD_TERMINATE = "Terminate";


/* function declarations */
int create_server_socket(int& in_server_port);
void do_start(const string& in_session_name);
void do_find(const string& in_session_name);
void do_terminate(const string& in_session_name);


/*
 * main
 */
int main() {
	// create the server socket
	int server_port = 0;
	const int server_socket = create_server_socket(server_port);

	// print the UDP port numbe
	printf("Chat Coordinator is running on port:  %d\n", server_port);

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
			string message(receive_buffer);

			// removing tailing newline
			const size_t newline_index = message.find_last_of("\r\n");
			message.erase(newline_index);

			// begin parsing string
			const size_t first_space = message.find_first_of(" ");
			if (string::npos == first_space) {
				fprintf(stderr, "Failed to find space in message.  Ignoring command.\n");
				continue;
			}

			const string command = message.substr(0, first_space);
			const string arguments = message.substr(first_space + 1);

			// ensure we have valid strings
			if (0 == command.size()) {
				fprintf(stderr, "Parsed command has zero length.  Ignoring command.\n");
				continue;
			}

			if (0 == arguments.size()) {
				fprintf(stderr, "Parsed command arguments have zero length.  Ignoring command.\n");
				continue;
			}

			// we only support 8 character names
			string session_name = arguments;
			if (arguments.length() > MAX_SESSION_NAME_SIZE) {
				fprintf(stderr, "Chat session name is greater than %d characters - truncating.\n", MAX_SESSION_NAME_SIZE);
				session_name = arguments.substr(0, MAX_SESSION_NAME_SIZE);
			}

			// perform the requested operation
			if (CMD_START == command) {
				do_start(session_name);
			}
			else if (CMD_FIND == command) {
				do_find(session_name);
			}
			else if (CMD_TERMINATE == command) {
				do_terminate(session_name);
			}
			else {
				fprintf(stderr, "Unrecognized command:  %s\n", command.c_str());
			}
		}
	}

	close(server_socket);
	return 0;
}

/*
 * create_server_socket
 */
int create_server_socket(int& in_server_port)
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

	in_server_port = ntohs(sin.sin_port);

	return server_socket;
}


/*
 * do_start
 */
void do_start(const string& in_session_name) {
	const string debug(CMD_START + ": begin room |" + in_session_name.c_str() + "|");
	printf("%s\n", debug.c_str());
}


/*
 * do_find
 */
void do_find(const string& in_session_name) {
	const string debug(CMD_FIND + " called with arguments:  |" + in_session_name.c_str() + "|");
	printf("%s\n", debug.c_str());
}

/*
 * do_terminate
 */
void do_terminate(const string& in_session_name) {
	const string debug(CMD_TERMINATE + " called with arguments:  |" + in_session_name.c_str() + "|");
	printf("%s\n", debug.c_str());
}

