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

    // create the info for the coordinator
    struct sockaddr_in si_coord;
	if( -1 == util_create_sockaddr(coordinator_host, coordinator_port, &si_coord)) {
		fprintf(stderr, "Failed to create sockaddr for coordinator.  Error is %s\n", strerror(errno));
		return -1;
	}
	int si_coord_len = sizeof(si_coord);

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
			const string my_command = CMD_START + " " + session_name;
			if (-1 == util_send_udp(command_socket, my_command.c_str(), my_command.length(), (struct sockaddr*)&si_coord, si_coord_len)) {
				fprintf(stderr, "Failed to send command coordinator.  Error is %s\n", strerror(errno));
			}
			else {
				int new_port;
				if (-1 == util_recv_udp(command_socket, new_port, (struct sockaddr*)&si_coord, si_coord_len)) {
					fprintf(stderr, "Failed to send command coordinator.  Error is %s\n", strerror(errno));
				}
				else {
					if (-1 != new_port) {
						const int new_socket = util_create_client_socket(SOCK_STREAM, IPPROTO_TCP, coordinator_host, new_port);
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
			}
		}
		else if (CMD_JOIN == user_command) {
			// send the command to the chat coordinator
			const string my_command = CMD_COORD_FIND + " " + session_name;
			if (-1 == util_send_udp(command_socket, my_command.c_str(), my_command.length(), (struct sockaddr*)&si_coord, si_coord_len)) {
				fprintf(stderr, "Failed to send command coordinator.  Error is %s\n", strerror(errno));
			}
			else {
				int new_port;
				if (-1 == util_recv_udp(command_socket, new_port, (struct sockaddr*)&si_coord, si_coord_len)) {
					fprintf(stderr, "Failed to receive session port number.  Error is %s\n", strerror(errno));
				}
				else {
					// leave the current session if active
					if (-1 != active_session_socket) {
						const string leave_command = CMD_COORD_LEAVE + " " + active_session_name;
						if (0 == util_send_udp(command_socket, leave_command.c_str(), leave_command.length(), (struct sockaddr*)&si_coord, si_coord_len)) {
							active_session_name = "";
							active_session_socket = -1;
						}
						else {
							fprintf(stderr, "Failed to leave chat session \"%s\"\n", active_session_name.c_str());
						}
					}

					// join the new session
					const int new_socket = util_create_client_socket(SOCK_STREAM, IPPROTO_TCP, coordinator_host, new_port);
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
			const string leave_command = CMD_COORD_LEAVE + " " + active_session_name;
			if (0 == util_send_udp(command_socket, leave_command.c_str(), leave_command.length(), (struct sockaddr*)&si_coord, si_coord_len)) {
				active_session_name = "";
				active_session_socket = -1;
			}
			else {
				fprintf(stderr, "Failed to leave chat session \"%s\"\n", active_session_name.c_str());
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

