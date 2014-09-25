// chat_client.cc
// Marc Schweikert
// CSCI 5273

#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <string>
#include <unistd.h>

#include <iostream>

#include "socket_utils.h"

#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

using std::cin;
using std::cout;
using std::endl;
using std::string;

/* constants */
const int MAX_MESSAGE_LENGTH = 80;
const int MAX_SESSION_NAME = 8;

// client commands
const string CMD_START = "Start";
const string CMD_JOIN = "Join";
const string CMD_SUBMIT = "Submit";
const string CMD_GET_NEXT = "GetNext";
const string CMD_GET_ALL = "GetAll";
const string CMD_LEAVE = "Leave";
const string CMD_EXIT = "Exit";

// coordinator commands
const string CMD_COORD_FIND = "Find";
const string CMD_COORD_LEAVE = "Leave";

/* function declarations */
int send_udp_command(const int, const char* const, const int, const string&, const bool);
int print_session_message(const int);


/*
 * main
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
		if (CMD_START == user_command || CMD_JOIN == user_command) {
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
		if (CMD_START == user_command) {
			// send the command to the chat coordinator
			const int recv_val = send_udp_command(command_socket, coordinator_host, coordinator_port, CMD_START + " " + session_name, true);
			if (-1 != recv_val) {
				const int new_socket = util_create_client_socket(SOCK_STREAM, IPPROTO_TCP, coordinator_host, recv_val);
				if (-1 != new_socket) {
					printf("A new chat session \"%s\" has been created and you have joined this session\n", session_name.c_str());
					active_session_name = session_name;
					active_session_socket = new_socket;
				}
				else {
					fprintf(stderr, "Failed to start chat session \"%s\"\n", session_name.c_str());
				}
			}
		}
		else if (CMD_JOIN == user_command) {
			// send the command to the chat coordinator
			const int recv_val = send_udp_command(command_socket, coordinator_host, coordinator_port, CMD_COORD_FIND + " " + session_name, true);
			if (-1 != recv_val) {
				// leave the current session if active
				if (-1 != active_session_socket) {
					if (0 == send_udp_command(command_socket, coordinator_host, coordinator_port, CMD_COORD_LEAVE + " " + active_session_name, false)) {
						active_session_name = "";
						active_session_socket = -1;
					}
					else {
						fprintf(stderr, "Failed to leave chat session \"%s\"\n", active_session_name.c_str());
					}
				}

				// join the new session
				const int new_socket = util_create_client_socket(SOCK_STREAM, IPPROTO_TCP, coordinator_host, recv_val);
				if (-1 != new_socket) {
					printf("You have joined the chat session \"%s\"\n", session_name.c_str());
					active_session_name = session_name;
					active_session_socket = new_socket;
				}
				else {
					fprintf(stderr, "Failed to join chat session \"%s\"\n", session_name.c_str());
				}
			}
		}
		else if (CMD_SUBMIT == user_command) {
			// we need a message to submit
			cout << "Message:  ";
			getline(cin, user_arguments);

			// validate session name
			string user_message = user_arguments;
			if (user_message.length() > MAX_MESSAGE_LENGTH) {
				user_message = user_message.substr(0, MAX_MESSAGE_LENGTH);
				fprintf(stderr, "Message too long - truncating to ->%s<-\n", user_message.c_str());
			}

			// send the message over TCP
			const string command = CMD_SUBMIT + " " + user_message;
			if (0 != util_send_tcp(active_session_socket, command.c_str(), command.length())) {
				fprintf(stderr, "Failed to submit message\n");
			}
		}
		else if (CMD_GET_NEXT == user_command) {
			// send the coomand
			if (0 != util_send_tcp(active_session_socket, CMD_GET_NEXT.c_str(), CMD_GET_NEXT.length())) {
				fprintf(stderr, "Failure during get_next\n");
			}
			else {
				print_session_message(active_session_socket);
			}
		}
		else if (CMD_GET_ALL == user_command) {
			// send the coomand
			if (0 != util_send_tcp(active_session_socket, CMD_GET_ALL.c_str(), CMD_GET_ALL.length())) {
				fprintf(stderr, "Failure during get_all\n");
			}
			else {
				int num_msgs;
				if (-1 == util_recv_tcp(active_session_socket, num_msgs)) {
					fprintf(stderr, "Failed to receive number of messages\n");
				}
				else {
					if (-1 == num_msgs) {
						printf("No new message in the chat session\n");
					}
					else {
						for (int i = 0; i < num_msgs; i++) {
							print_session_message(active_session_socket);
						}
					}
				}
			}
		}
		else if (CMD_LEAVE == user_command) {
			if (0 == send_udp_command(command_socket, coordinator_host, coordinator_port, CMD_COORD_LEAVE + " " + active_session_name, false)) {
				active_session_name = "";
				active_session_socket = -1;
			}
		}
		else if (CMD_EXIT == user_command) {
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

int send_udp_command(const int in_command_socket, const char* const in_coord_host, const int in_coord_port, const string& in_command, const bool in_response) {
	// create the info for the coordinator
	struct sockaddr_in si_coord;
	socklen_t si_coord_len = sizeof(si_coord);
	memset(&si_coord, 0, si_coord_len);
	si_coord.sin_family = AF_INET;
	si_coord.sin_port = htons(in_coord_port);

	// we may be passed a hostname rather than IP address
	struct hostent *hostp = gethostbyname(in_coord_host);
	if (NULL == hostp) {
		fprintf(stderr, "Unable to resolve hostname!");
		return -1;
	}
	memcpy(&si_coord.sin_addr, hostp->h_addr, sizeof(si_coord.sin_addr));

	// create the sendable string
	assert(in_command.length() <= BUFFER_SIZE - 1);

	char command_str[BUFFER_SIZE];
	memset(command_str, 0, BUFFER_SIZE);
	strncpy(command_str, in_command.c_str(), in_command.length());
	const size_t command_len = strlen(command_str);

	#ifdef DEBUG
	printf("DEBUG:  chat_client - sending |%s| to UDP coordinator\n", command_str);
	#endif

	// send the command
	if (-1 == sendto(in_command_socket, command_str, command_len, 0, (struct sockaddr*)&si_coord, si_coord_len)) {
		fprintf(stderr, "sendto called failed!  Error is %s\n", strerror(errno));
		return -1;
	}

	if (true == in_response) {
		// receive the response
		char receive_buffer[BUFFER_SIZE];
		memset(receive_buffer, 0, BUFFER_SIZE);
		const ssize_t recv_len = recvfrom(in_command_socket, receive_buffer, BUFFER_SIZE, 0, (struct sockaddr *)&si_coord, &si_coord_len);

		if (-1 == recv_len) {
			fprintf(stderr, "Error during call to recvfrom().  Error is %s\n", strerror(errno));
			return -1;
		}
		if (0 == recv_len) {
			printf("recvfrom() encountered orderly shutdown");
			return 0;
		}

		// prevent buffer overflow
		assert(recv_len <= BUFFER_SIZE);
		receive_buffer[recv_len] = 0;
		receive_buffer[BUFFER_SIZE - 1] = 0;

		#ifdef DEBUG
		printf("DEBUG:  chat_client - received from UDP coordinator|%s|\n", receive_buffer);
		#endif

		// we are going to cheat here
		// the chat coordinator only returns integer values
		const int return_code = atoi(receive_buffer);

		return return_code;
	}
	
	return 0;
}

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

