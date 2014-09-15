// chat_coordinator.cc
// Marc Schweikert
// CSCI 5273 Fall 2014

#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
#include <unistd.h>

#include <netinet/in.h>
#include <sys/socket.h>

using std::map;
using std::string;


/* constants */
const int BUFFER_SIZE = 256;
const int MAX_SESSION_NAME_SIZE = 8;

const string CMD_START = "Start";
const string CMD_FIND = "Find";
const string CMD_TERMINATE = "Terminate";


/* function declarations */
int create_socket(const int, const int);
unsigned int get_port_number(const int);
void do_start(const string&, map<string, unsigned int>&);
void do_find(const string&, const map<string, unsigned int>&);
void do_terminate(const string&, map<string, unsigned int>&);


/*
 * main
 */
int main() {
	// this will be the mapping between chat session names and TCP port numbers
	map<string, unsigned int> chat_session_map;

	// create the server socket
	const int server_socket = create_socket(SOCK_DGRAM, 0);

	// print the UDP port numbe
	const int server_port = get_port_number(server_socket);
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
				do_start(session_name, chat_session_map);
			}
			else if (CMD_FIND == command) {
				do_find(session_name, chat_session_map);
			}
			else if (CMD_TERMINATE == command) {
				do_terminate(session_name, chat_session_map);
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
int create_socket(const int in_socket_type, const int in_protocol)
{
	// allocate a socket
	const int server_socket = socket(PF_INET, in_socket_type, in_protocol);
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

	return server_socket;
}

/*
 * get_port_number
 */
unsigned int get_port_number(const int in_socket) {
	struct sockaddr_in sin;
	socklen_t socklen = sizeof(sin);
	memset(&sin, 0, sizeof(sin));

	if (getsockname(in_socket, (struct sockaddr *)&sin, &socklen) < 0) {
		fprintf(stderr, "getsockname called failed!  Error is %s\n", strerror(errno));
		exit(3);
	}

	return ntohs(sin.sin_port);
}


/*
 * do_start
 */
void do_start(const string& in_session_name, map<string, unsigned int>& in_chat_session_map) {
	// see if an existing chat session is available
	if ( in_chat_session_map.end() == in_chat_session_map.find(in_session_name)) {
		// new session
		const int session_socket = create_socket(SOCK_STREAM, 0);
		const int session_port = get_port_number(session_socket);

		printf("Starting session |%s| on port |%d|\n", in_session_name.c_str(), session_port);
		in_chat_session_map.insert(std::make_pair<string, unsigned int>(in_session_name, session_port));
	}
	else {
		// found existing session
	}
}


/*
 * do_find
 */
void do_find(const string& in_session_name, const map<string, unsigned int>& in_chat_session_map) {
	const string debug(CMD_FIND + " called with arguments:  |" + in_session_name.c_str() + "|");
	printf("%s\n", debug.c_str());
}

/*
 * do_terminate
 */
void do_terminate(const string& in_session_name, map<string, unsigned int>& in_chat_session_map) {
	const string debug(CMD_TERMINATE + " called with arguments:  |" + in_session_name.c_str() + "|");
	printf("%s\n", debug.c_str());
}

