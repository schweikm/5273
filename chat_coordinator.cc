// chat_coordinator.cc
// Marc Schweikert
// CSCI 5273 Fall 2014

#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <sstream>
#include <string>
#include <unistd.h>

#include <netinet/in.h>
#include <sys/socket.h>

using std::map;
using std::string;
using std::stringstream;


/* constants */
const int BUFFER_SIZE = 256;
const int MAX_SESSION_NAME_SIZE = 8;

const string CMD_START = "Start";
const string CMD_FIND = "Find";
const string CMD_TERMINATE = "Terminate";

const string SERVER_EXE = "chat_server.exe";


/* function declarations */
int create_socket(const int, const int);
int get_port_number(const int);
void do_start(const string&, map<string, int>&);
void do_find(const string&, const map<string, int>&);
void do_terminate(const string&, map<string, int>&);


/*
 * main
 */
int main() {
	// this will be the mapping between chat session names and TCP port numbers
	map<string, int> chat_session_map;

	// create the server socket
	const int coordinator_socket = create_socket(SOCK_DGRAM, 0);

	// print the UDP port number
	const int server_port = get_port_number(coordinator_socket);
	printf("%d\n", server_port);

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
		const ssize_t recv_len = recvfrom(coordinator_socket, receive_buffer, BUFFER_SIZE, 0, (struct sockaddr *)&remote_addr, &remote_addr_len);
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

	close(coordinator_socket);
	return 0;
}

/*
 * create_socket
 */
int create_socket(const int in_socket_type, const int in_protocol)
{
	// allocate a socket
	const int new_socket = socket(PF_INET, in_socket_type, in_protocol);
	if (new_socket < 0) {
		fprintf(stderr, "Unable to create socket.  Error is %s\n", strerror(errno));
		exit (1);
	}

	// an Internet endpoint address
	struct sockaddr_in sin;
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;			// Internet Protocol
	sin.sin_addr.s_addr = INADDR_ANY;	// use any available address
	sin.sin_port=htons(0);				// request a port number to be allocated by bind

	// bind the socket
	if (bind(new_socket, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
		fprintf(stderr, "Unable to bind to port.  Error is %s\n", strerror(errno));
		exit(2);
	}

	return new_socket;
}

/*
 * get_port_number
 */
int get_port_number(const int in_socket) {
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
void do_start(const string& in_session_name, map<string, int>& in_chat_session_map) {
	// see if an existing chat session is available
	if ( in_chat_session_map.end() == in_chat_session_map.find(in_session_name)) {
		const int session_socket = create_socket(SOCK_STREAM, 0);
		const int session_port = get_port_number(session_socket);

		stringstream socket_fd_stream;
		socket_fd_stream << session_socket;
		const string socket_fd = socket_fd_stream.str();

		// start session server using fork and execl
		if (0 == fork()) {
			//
			// CHILD PROCESS
			//

			// let's replace ourself with the chat_server program
			// we need to inform the child process of the file descriptor for it's TCP socket
			execl(SERVER_EXE.c_str(), socket_fd.c_str(), NULL);

			// this call never returns.  we are now in the other program - goodbye!
	}
		else {
			//
			// PARENT PROCESS
			//

			// tell the client how to connect to the session server
			printf("%d\n", session_port);
			in_chat_session_map.insert(std::make_pair<string, int>(in_session_name, session_port));
		}
	}
	else {
		// found existing session
		printf("-1\n");
	}
}


/*
 * do_find
 */
void do_find(const string& in_session_name, const map<string, int>& in_chat_session_map) {
	const map<string, int>::const_iterator find_iterator = in_chat_session_map.find(in_session_name);
	if ( in_chat_session_map.end() == find_iterator) {
		printf("-1\n");
	}
	else {
		printf("%d\n", find_iterator->second);
	}
}

/*
 * do_terminate
 */
void do_terminate(const string& in_session_name, map<string, int>& in_chat_session_map) {
	in_chat_session_map.erase(in_session_name);
}

