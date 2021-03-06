/**
 * @file chat_coordinator.cc
 * @author Marc Schweikert
 * @date 26 September 2014
 * @brief Chat Coordinator implementation
 */

#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <signal.h>
#include <string>
#include <unistd.h>

#include <netinet/in.h>
#include <sys/socket.h>

#include "strings.h"
#include "socket_utils.h"

using std::map;
using std::string;

/** Name of the chat server executable we will spawn */
const string SERVER_EXE = "chat_server.exe";


/* function declarations */
int do_start(const string&, map<string, int>&, const int);
int do_find(const string&, const map<string, int>&);
void do_terminate(const string&, map<string, int>&);

/**
  * Main - entry point of program
  *
  * @return 0 if success; any other value if error
  */
int main() {
	// this will be the mapping between chat session names and TCP port numbers
	map<string, int> chat_session_map;

	// create the server socket
	const int coordinator_socket = util_create_server_socket(SOCK_DGRAM, IPPROTO_UDP, NULL, 0);

	// print the UDP port number
	const int server_port = util_get_port_number(coordinator_socket);
	printf("Chat Coordinator started on UDP port %d\n", server_port);

	//
	// begin main loop
	//
	char receive_buffer[BUFFER_SIZE];
	struct sockaddr_in remote_addr;
	socklen_t remote_addr_len = sizeof(remote_addr);

	for (;;) {
		// receive the message with our command
		if(-1 == util_recv_udp(coordinator_socket, receive_buffer, BUFFER_SIZE - 1, (struct sockaddr *)&remote_addr, remote_addr_len)) {
			fprintf(stderr, "Error reading socket.  Error is %s\n", strerror(errno));
		}
		const string command(receive_buffer);

		// receive the chat seesion name
		if(-1 == util_recv_udp(coordinator_socket, receive_buffer, BUFFER_SIZE - 1, (struct sockaddr *)&remote_addr, remote_addr_len)) {
			fprintf(stderr, "Error reading socket.  Error is %s\n", strerror(errno));
		}
		const string session_name(receive_buffer);

		// perform the requested operation
		if (CMD_COORDINATOR_START == command) {
			const int code = do_start(session_name, chat_session_map, server_port);
			util_send_udp(coordinator_socket, code, (struct sockaddr *)&remote_addr);
		}
		else if (CMD_COORDINATOR_FIND == command) {
			const int code = do_find(session_name, chat_session_map);
			util_send_udp(coordinator_socket, code, (struct sockaddr *)&remote_addr);
		}
		else if (CMD_COORDINATOR_TERMINATE == command) {
			do_terminate(session_name, chat_session_map);
		}
		else {
			fprintf(stderr, "Chat Coordinator - unrecognized command:  ->%s<-\n", command.c_str());
			util_send_udp(coordinator_socket, -1, (struct sockaddr *)&remote_addr);
		}
	}

	close(coordinator_socket);
	return 0;
}

/**
  * Starts a new chat session server with the requested name.
  * A new process is created for each session server.
  *
  * @pre in_session_name is a non-empty string
  * @post A new session server has been spawned
  * @param in_session_name Name of the chat session server
  * @param in_chat_session_map Contains a mapping of names to TCP port numbers
  * @param in_server_port UDP port number of the chat coordinator
  * @return TCP port of the session server if successul; -1 if error
  */
int do_start(const string& in_session_name,
             map<string, int>& in_chat_session_map,
             const int in_server_port) {
	int return_code = 0;
	// see if an existing chat session is available
	if ( in_chat_session_map.end() == in_chat_session_map.find(in_session_name)) {
		const int session_socket = util_create_server_socket(SOCK_STREAM, IPPROTO_TCP, NULL, 0);
		const int session_port = util_get_port_number(session_socket);

		// start the socket listening for connections
		if (-1 == util_listen(session_socket)) {
			fprintf(stderr, "Failed to listen on socket.  Error is %s\n", strerror(errno));
		}

		// socket file descriptor
		char fd_str[BUFFER_SIZE];
		memset(fd_str, 0, BUFFER_SIZE);
		sprintf(fd_str, "%d", session_socket);

		// coordinator port number
		char port_str[BUFFER_SIZE];
		memset(port_str, 0, BUFFER_SIZE);
		sprintf(port_str, "%d", in_server_port);

		// start session server using fork and execl
		signal(SIGCHLD, SIG_IGN);
		const pid_t fork_code = fork();
		if (-1 == fork_code) {
			fprintf(stderr, "fork called failed!  Error is %s\n", strerror(errno));
		}
		else if (0 == fork_code) {
			//
			// CHILD PROCESS
			//

			// let's replace ourself with the chat_server program
			// we need to inform the child process of the file descriptor for it's TCP socket
			execl(SERVER_EXE.c_str(), fd_str, port_str, in_session_name.c_str(), NULL);

			// this call never returns.  we are now in the other program - goodbye!
		}
		else {
			//
			// PARENT PROCESS
			//

			// tell the client how to connect to the session server
			printf("Session \"%s\" started on TCP port %d\n", in_session_name.c_str(), session_port);
			in_chat_session_map.insert(std::make_pair<string, int>(in_session_name, session_port));
			return_code = session_port;
		}
	}
	else {
		return_code = -1;
	}

	return return_code;
}

/**
  * Finds the TCP port number of an existing chat session server.
  *
  * @pre in_session_name is a non-empty string
  * @post none
  * @param in_session_name Name of the chat session server
  * @param in_chat_session_map Contains a mapping of names to TCP port numbers
  * @return TCP port of the session server if successul; -1 if error
  */
int do_find(const string& in_session_name,
            const map<string, int>& in_chat_session_map) {
	const map<string, int>::const_iterator find_iterator = in_chat_session_map.find(in_session_name);
	if ( in_chat_session_map.end() == find_iterator) {
		return -1;
	}

	return find_iterator->second;
}

/**
  * Removes references to the chat session server.
  *
  * @pre in_session_name is a non-empty string
  * @post none
  * @param in_session_name Name of the chat session server
  * @param in_chat_session_map Contains a mapping of names to TCP port numbers
  */
void do_terminate(const string& in_session_name,
                  map<string, int>& in_chat_session_map) {
	in_chat_session_map.erase(in_session_name);
}

