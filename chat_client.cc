/**
 * @file chat_client.cc
 * @author Marc Schweikert
 * @date 26 September 2014
 * @brief Chat client implementation
 */

#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <unistd.h>

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "strings.h"
#include "socket_utils.h"

using std::cin;
using std::cout;
using std::endl;
using std::string;


/** Maximum length of a message that can be submitted */
const int MAX_MESSAGE_LENGTH = 80;
/** Maximum length of a chat session name */
const int MAX_SESSION_NAME = 8;

int do_start(const int, const char* const, const struct sockaddr_in&, const string&);
int do_join(const int, const char* const, const struct sockaddr_in&, const string&);
int do_submit(const int);
int do_get_next(const int);
int do_get_all(const int);
int print_session_message(const int);


/**
  * Main - entry point of program
  *
  * @param argc Number of command line arguments
  * @param argv Command line arguments
  * @return 0 if success; any other value if error
  */
int main(const int argc, const char** const argv) {
	if (3 != argc) {
		fprintf(stderr, "Usage: chat_client.exe host/IP port\n");
		exit(1);
	}

	// how to communicate with the chat coordinator
	const char* const coordinator_host = argv[1];
	const int coordinator_port = atoi(argv[2]);

	// this is the socket that we will send commands to the chat coordinator
	const int command_socket = util_create_server_socket(SOCK_DGRAM, IPPROTO_UDP, coordinator_host, 0);
	if (-1 == command_socket) {
		fprintf(stderr, "Failed to create UDP socket to communicate with Chat Coordinator!\n");
		return -1;
	}

    // create the info for the coordinator
    struct sockaddr_in si_coord;
	if( -1 == util_create_sockaddr(coordinator_host, coordinator_port, &si_coord)) {
		fprintf(stderr, "Failed to create sockaddr for coordinator.  Error is %s\n", strerror(errno));
		return -1;
	}

	// this will hold our chat session sockets
	string active_session_name = "";
	int active_session_socket = -1;

	// begin command line processing
	string user_command;
	string user_arguments;
	for(;;) {
		// I don't like using the C++ stream libraries, but this will make this processing much easier
		user_command = "";
		user_arguments = "";

		// prompt user for command
		cout << "Chat Client>>  ";
		getline(cin, user_command);

		// these commands require a session name
		string session_name;
		if (CMD_CLIENT_START == user_command || CMD_CLIENT_JOIN == user_command) {
			// we need a session name
			cout << "Session Name:  ";
			getline(cin, user_arguments);

			// validate session name
			session_name = user_arguments;
			if (session_name.length() > MAX_SESSION_NAME) {
				session_name = session_name.substr(0, MAX_SESSION_NAME);
				fprintf(stderr, "Session name too long - truncating to ->%s<-\n", session_name.c_str());
			}
		}
	
		// execute command
		if (CMD_CLIENT_START == user_command) {
			const int val = do_start(command_socket, coordinator_host, si_coord, session_name);
			if (-1 != val) {
				printf("A new chat session \"%s\" has been created and you have joined this session\n", session_name.c_str());
				active_session_name = session_name;
				active_session_socket = val;
			}
		}
		else if (CMD_CLIENT_JOIN == user_command) {
			const int val = do_join(command_socket, coordinator_host, si_coord, session_name);
			if (-1 != val) {
				printf("You have joined the chat session \"%s\"\n", session_name.c_str());
				active_session_name = session_name;
				active_session_socket = val;
			}
		}
		else if (CMD_CLIENT_SUBMIT == user_command) {
			do_submit(active_session_socket);
		}
		else if (CMD_CLIENT_GET_NEXT == user_command) {
			do_get_next(active_session_socket);
		}
		else if (CMD_CLIENT_GET_ALL == user_command) {
			do_get_all(active_session_socket);
		}
		else if (CMD_CLIENT_LEAVE == user_command) {
			if (0 == util_send_tcp(active_session_socket, CMD_SERVER_LEAVE.c_str(), CMD_SERVER_LEAVE.length())) {
				printf("You have left the chat session \"%s\"\n", active_session_name.c_str());
				close(active_session_socket);
				active_session_name = "";
				active_session_socket = -1;
			}
		}
		else if (CMD_CLIENT_EXIT == user_command) {
			if (-1 != active_session_socket) {
				close(active_session_socket);
			}
			break;
		}
		else {
			fprintf(stderr, "Chat client - unrecognized command |%s|\n", user_command.c_str());
		}
	}

	close(command_socket);
	return 0;
}

/**
  * Starts a new chat session server.  Returns the TCP port number of chat session.
  *
  * @pre in_socket is a valid socket file descriptor
  * @post Chat session has been started
  * @param in_socket Socket file descriptor to send UDP message to chat coordinator
  * @param in_coord_host Hostname / IP address of the chat coordinator
  * @param in_coord Address information for the chat coordinator
  * @param in_session_name Chat session name to start
  * @return TCP port number of new chat session if successful; -1 if error
  */
int do_start(const int in_socket,
             const char* const in_coord_host,
             const struct sockaddr_in& in_coord,
             const string& in_session_name) {
	// send the command to the chat coordinator
	if (-1 == util_send_udp(in_socket, CMD_COORDINATOR_START.c_str(), CMD_COORDINATOR_START.length(), (struct sockaddr*)&in_coord)) {
		fprintf(stderr, "Failed to send command coordinator.  Error is %s\n", strerror(errno));
		return -1;
	}

	if (-1 == util_send_udp(in_socket, in_session_name.c_str(), in_session_name.length(), (struct sockaddr*)&in_coord)) {
		fprintf(stderr, "Failed to send command coordinator.  Error is %s\n", strerror(errno));
		return -1;
	}

	// now recv the new port number
	int new_port;
	if (-1 == util_recv_udp(in_socket, new_port, (struct sockaddr*)&in_coord, sizeof(in_coord))) {
		fprintf(stderr, "Failed to send command coordinator.  Error is %s\n", strerror(errno));
		return -1;
	}

	// join the chat
	int new_socket = -1;
	if (-1 != new_port) {
		new_socket = util_create_client_socket(SOCK_STREAM, IPPROTO_TCP, in_coord_host, new_port);
		if (-1 != new_socket) {
		}
		else {
			fprintf(stderr, "Failed to start chat session \"%s\"\n", in_session_name.c_str());
			return -1;
		}
	}
	else {
		fprintf(stderr, "Chat session \"%s\" has already been started\n", in_session_name.c_str());
		return -1;
	}

	return new_socket;
}

/**
  * Joins an existing chat session server.  Returns the TCP port number of chat session.
  *
  * @pre in_socket is a valid socket file descriptor
  * @post Chat session has been joined
  * @param in_socket Socket file descriptor to send UDP message to chat coordinator
  * @param in_coord_host Hostname / IP address of the chat coordinator
  * @param in_coord Address information for the chat coordinator
  * @param in_session_name Chat session name to join
  * @return TCP port number of new chat session if successful; -1 if error
  */
int do_join(const int in_socket,
            const char* const in_coord_host,
            const struct sockaddr_in& in_coord,
            const string& in_session_name) {
	// send the command to the chat coordinator
	if (-1 == util_send_udp(in_socket, CMD_COORDINATOR_FIND.c_str(), CMD_COORDINATOR_FIND.length(), (struct sockaddr*)&in_coord)) {
		fprintf(stderr, "Failed to send command coordinator.  Error is %s\n", strerror(errno));
		return -1;
	}

	if (-1 == util_send_udp(in_socket, in_session_name.c_str(), in_session_name.length(), (struct sockaddr*)&in_coord)) {
		fprintf(stderr, "Failed to send command coordinator.  Error is %s\n", strerror(errno));
		return -1;
	}

	int new_port;
	if (-1 == util_recv_udp(in_socket, new_port, (struct sockaddr*)&in_coord, sizeof(in_coord))) {
		fprintf(stderr, "Failed to receive session port number.  Error is %s\n", strerror(errno));
		return -1;
	}

	// join the new session
	const int new_socket = util_create_client_socket(SOCK_STREAM, IPPROTO_TCP, in_coord_host, new_port);
	if (-1 != new_socket) {
	}
	else {
		fprintf(stderr, "Failed to join chat session \"%s\"\n", in_session_name.c_str());
	}

	return new_socket;
}

/**
  * Submits a message to the chat session.
  *
  * @pre in_socket is a valid socket file descriptor
  * @post Message has been submitted to chat session
  * @param in_socket Socket file descriptor for chat session server
  * @return 0 if successful; -1 if error
  */
int do_submit(const int in_socket) {
	// we need a message to submit
	string user_arguments;
	cout << "Message:  ";
	getline(cin, user_arguments);

	// validate session name
	string user_message = user_arguments;
	if (user_message.length() > MAX_MESSAGE_LENGTH) {
		user_message = user_message.substr(0, MAX_MESSAGE_LENGTH);
		fprintf(stderr, "Message too long - truncating to ->%s<-\n", user_message.c_str());
	}

	// send the message over TCP
	if (-1 == util_send_tcp(in_socket, CMD_SERVER_SUBMIT.c_str(), CMD_SERVER_SUBMIT.length())) {
		fprintf(stderr, "Failed to send submit command.  Error is %s\n", strerror(errno));
		return -1;
	}

	if (-1 == util_send_tcp(in_socket, user_message.length())) {
		fprintf(stderr, "Failed to send message length.  Error is %s\n", strerror(errno));
		return -1;
	}

	if (-1 == util_send_tcp(in_socket, user_message.c_str(), user_message.length())) {
		fprintf(stderr, "Failed to send message.  Error is %s\n", strerror(errno));
		return -1;
	}

	return 0;
}

/**
  * Gets the next unread message from the chat session server.
  *
  * @pre in_socket is a valid socket file descriptor
  * @post One unread message has been retrieved from chat session if it exists
  * @param in_socket Socket file descriptor for chat session server
  * @return 0 if successful; -1 if error
  */
int do_get_next(const int in_socket) {
	// send the coomand
	if (0 != util_send_tcp(in_socket, CMD_SERVER_GET_NEXT.c_str(), CMD_SERVER_GET_NEXT.length())) {
		fprintf(stderr, "Failure during get_next\n");
		return -1;
	}

	return print_session_message(in_socket);
}

/**
  * Gets all unread messages from the chat session server.
  *
  * @pre in_socket is a valid socket file descriptor
  * @post All unread messages have been retrieved from chat session if they exist
  * @param in_socket Socket file descriptor for chat session server
  * @return 0 if successful; -1 if error
  */
int do_get_all(const int in_socket) {
	// send the coomand
	if (0 != util_send_tcp(in_socket, CMD_SERVER_GET_ALL.c_str(), CMD_SERVER_GET_ALL.length())) {
		fprintf(stderr, "Failure during get_all\n");
		return -1;
	}

	int num_msgs;
	if (-1 == util_recv_tcp(in_socket, num_msgs)) {
		fprintf(stderr, "Failed to receive number of messages\n");
		return -1;
	}

	if (-1 == num_msgs) {
		printf("No new messages in the chat session\n");
	}
	else {
		for (int i = 0; i < num_msgs; i++) {
			print_session_message(in_socket);
		}
	}

	return 0;
}

/**
  * Implementation of GetNext and GetAll methods.
  *
  * @pre in_socket is a valid socket file descriptor
  * @post Requested number of unread messages have been retrieved from chat session if they exist
  * @param in_socket Socket file descriptor for chat session server
  * @return 0 if successful; -1 if error
  */
int print_session_message(const int in_socket) {
	// get the message length first
	int msg_len;
	if(-1 == util_recv_tcp(in_socket, msg_len)) {
		fprintf(stderr, "Failed to get message length.  Error is %s\n", strerror(errno));
		return -1;
	}
	
	// if we literally recv'd -1 from the socket then there is no message
	if (-1 == msg_len) {
		printf("No new message in the chat session\n");
		return 0;
	}

	// then the response
	char recv_buffer[BUFFER_SIZE];

	if (-1 == util_recv_tcp(in_socket, recv_buffer, msg_len)) {
		fprintf(stderr, "Failed to get message.  Error is %s\n", strerror(errno));
		return -1;
	}

	printf("%s\n", recv_buffer);
	return 0;
}

