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
#include <string>
#include <unistd.h>
#include <vector>

#include <netinet/in.h>
#include <sys/socket.h>

#include "strings.h"
#include "socket_utils.h"

using std::map;
using std::string;
using std::vector;


/* constants */
const int MESSAGE_MAX_LENGTH = 80;
const int RECEIVE_TIMEOUT = 60;  // seconds

/* function declarations */
void handle_select_timeout(const int, const char* const, const int, const string&);
int do_submit(const int, vector<string>&);
int do_get_next(const int, map<int, int>&, const vector<string>&);
int do_get_all(const int, map<int, int>&, const vector<string>&);
int send_chat_messages(const int, const vector<string>&, const unsigned int, const unsigned int);


/*
 * main
 */
int main(const int, const char* const argv[]) {
	const int server_socket = atoi(argv[0]);
	const int coordinator_port = atoi(argv[1]);
	const string session_name = argv[2];

	// let's start up our data structure
	map<int, int> next_message_map;
	vector<string> all_messages;

	//
	// BEGIN MAIN LOOP
	//

	struct sockaddr_in fsin;    /* the from address of a client */
	fd_set  rfds;           /* read file descriptor set */
	fd_set  afds;           /* active file descriptor set   */

	int nfds = getdtablesize();
	FD_ZERO(&afds);
	FD_SET(server_socket, &afds);

	for(;;) {
		memcpy(&rfds, &afds, sizeof(rfds));
		int client_socket = -1;

		// select timeout
		struct timeval select_timeout;
		select_timeout.tv_sec = RECEIVE_TIMEOUT;
		select_timeout.tv_usec = 0;
		const int select_code = select(nfds, &rfds, (fd_set *)0, (fd_set *)0, &select_timeout);
		// error
		if (select_code < 0) {
			fprintf(stderr, "select: %s\n", strerror(errno));
			exit(1);
		}

		// timeout
		if (0 == select_code) {
			handle_select_timeout(server_socket, NULL, coordinator_port, session_name);
		}

		if (FD_ISSET(server_socket, &rfds)) {
			unsigned int alen = sizeof(fsin);
			client_socket = accept(server_socket, (struct sockaddr *)&fsin, &alen);

			if (client_socket < 0) {
				fprintf(stderr, "accept: %s\n", strerror(errno));
			}

			FD_SET(client_socket, &afds);

			// we have a new client, so initialize it's last read message
			next_message_map.insert(std::make_pair<int, int>(client_socket, 0));
		}

		for (int client_socket = 0; client_socket < nfds; ++client_socket) {
			if (client_socket != server_socket && FD_ISSET(client_socket, &rfds)) {
				// let's peek into the stream to see how we are supposed to read the command
				const int peek_len = 6;
				char peek_buf[peek_len];
				if (-1 == util_recv_tcp(client_socket, peek_buf, peek_len - 1, MSG_PEEK)) {
					fprintf(stderr, "failed to peek command stream.  Error is %s\n", strerror(errno));
					close(client_socket);
					FD_CLR(client_socket, &afds);
					continue;
				}

				const string peek_msg(peek_buf);
				string peek_command;
				if (string::npos != CMD_SERVER_SUBMIT.find(peek_msg)) {
					peek_command = CMD_SERVER_SUBMIT;
				}
				else if (string::npos != CMD_SERVER_GET_NEXT.find(peek_msg)) {
					peek_command = CMD_SERVER_GET_NEXT;
				}
				else if (string::npos != CMD_SERVER_GET_ALL.find(peek_msg)) {
					peek_command = CMD_SERVER_GET_ALL;
				}
				else if (string::npos != CMD_SERVER_LEAVE.find(peek_msg)) {
					peek_command = CMD_SERVER_LEAVE;
				}
				else {
					fprintf(stderr, "Invalid command |%s|.  Cannot continue.\n", peek_command.c_str());
					close(client_socket);
					FD_CLR(client_socket, &afds);
					continue;
				}

				// now read the message from the stream for real
				char recv_buffer[BUFFER_SIZE];
				if (util_recv_tcp(client_socket, recv_buffer, peek_command.length()) <= 0) {
					fprintf(stderr, "error or client disconnect: %s\n", strerror(errno));
					close(client_socket);
					FD_CLR(client_socket, &afds);
					continue;
				}

				// parse message
				string command(recv_buffer);

				// perform the requested operation
				if (CMD_SERVER_SUBMIT == command) {
					if (-1 == do_submit(client_socket, all_messages)) {
						fprintf(stderr, "do_submit failed!\n");
					}
				}
				else if (CMD_SERVER_GET_NEXT == command) {
					if (-1 == do_get_next(client_socket, next_message_map, all_messages)) {
						fprintf(stderr, "do_get_next failed!\n");
					}
				}
				else if (CMD_SERVER_GET_ALL == command) {
					if (-1 == do_get_all(client_socket, next_message_map, all_messages)) {
						fprintf(stderr, "do_get_all failed!\n");
					}
				}
				else if (CMD_SERVER_LEAVE == command) {
					close(client_socket);
					FD_CLR(client_socket, &afds);
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

void handle_select_timeout(const int in_server_socket, const char* const in_coord_host, const int in_coord_port, const string& in_session_name) {
	printf("Chat server closing after select timeout!\n");

	// tell the coordinator that we are exiting
	// create new UDP socket
	const int udp_socket = util_create_server_socket(SOCK_DGRAM, IPPROTO_UDP, NULL, 0); 

	// communicate over UDP
	struct sockaddr_in coord_addr;
	if (-1 == util_create_sockaddr(in_coord_host, in_coord_port, &coord_addr)) {
		fprintf(stderr, "Failed to create coordinator sockaddr.  Error is %s\n", strerror(errno));
		exit(-1);
	}

	if (-1 == util_send_udp(udp_socket, CMD_COORDINATOR_TERMINATE.c_str(), CMD_COORDINATOR_TERMINATE.length(), (struct sockaddr *)&coord_addr, sizeof(coord_addr))) {
		fprintf(stderr, "sendto called failed!  Error is %s\n", strerror(errno));
		exit(-1);
	}

	if (-1 == util_send_udp(udp_socket, in_session_name.c_str(), in_session_name.length(), (struct sockaddr *)&coord_addr, sizeof(coord_addr))) {
		fprintf(stderr, "sendto called failed!  Error is %s\n", strerror(errno));
		exit(-1);
	}

	// clean up and exit
	close(udp_socket);
	close(in_server_socket);
	exit(0);
}


/*
 * do_submit
 */
int do_submit(const int in_socket, vector<string>& in_all_messages) {
	int msg_len;
	if (-1 == util_recv_tcp(in_socket, msg_len)) {
		fprintf(stderr, "Failed to receive message length.  Error is %s\n", strerror(errno));
		return -1;
	}

	char msg_buf[BUFFER_SIZE + 1];
	if (-1 == util_recv_tcp(in_socket, msg_buf, BUFFER_SIZE)) {
		fprintf(stderr, "Failed to receive message.  Error is %s\n", strerror(errno));
		return -1;
	}

	// we only support 80 character messages
	string message_text(msg_buf);
	if (message_text.length() > MESSAGE_MAX_LENGTH) {
		fprintf(stderr, "Message is greater than %d characters - truncating.\n", MESSAGE_MAX_LENGTH);
		message_text = message_text.substr(0, MESSAGE_MAX_LENGTH);
	}

	// store the message in the chat history
	in_all_messages.push_back(message_text);

	return 0;
}

/*
 * do_get_next
 */
int do_get_next(const int in_client_socket, map<int, int>& in_next_message, const vector<string>& in_all_messages) {
	// let's make sure that the next message exists
	map<int, int>::const_iterator next_it = in_next_message.find(in_client_socket);
	if (in_next_message.end() == next_it) {
		fprintf(stderr, "failed to find last message index for client %d\n", in_client_socket);
		util_send_tcp(in_client_socket, -1);
		return -1;
	}

	const int start_index = next_it->second;
	const int stop_index = start_index + 1;

	// no new messages
	if (static_cast<size_t>(stop_index) > in_all_messages.size()) {
		if (-1 == util_send_tcp(in_client_socket, -1)) {
			fprintf(stderr, "Failed to send number of messages.  Error is %s\n", strerror(errno));
			return -1;
		}

		// nothing more to do
		return 0;
	}

	// send the message
	if (0 == send_chat_messages(in_client_socket, in_all_messages, start_index, stop_index)) {
		// update the index if successful
		in_next_message[in_client_socket] = stop_index;
	}
	else {
		util_send_tcp(in_client_socket, -1);
	}

	return 0;
}

/*
 * do_get_all
 */
int do_get_all(const int in_client_socket, map<int, int>& in_next_message, const vector<string>& in_all_messages) {
	// let's make sure that the next message exists
	map<int, int>::const_iterator next_it = in_next_message.find(in_client_socket);
	if (in_next_message.end() == next_it) {
		fprintf(stderr, "failed to find last message index for client %d\n", in_client_socket);
		util_send_tcp(in_client_socket, -1);
		return -1;
	}

	const int start_index = next_it->second;
	const int stop_index = in_all_messages.size();

	// we need to send the number of messages that will be sent first
	const int num_msgs = stop_index - start_index;

	// no new messages
	if (0 == num_msgs) {
		if (-1 == util_send_tcp(in_client_socket, -1)) {
			fprintf(stderr, "Failed to send number of messages.  Error is %s\n", strerror(errno));
			return -1;
		}

		// nothing more to do
		return 0;
	}

	// have n messages
	if (-1 == util_send_tcp(in_client_socket, num_msgs)) {
		fprintf(stderr, "Failed to send number of messages.  Error is %s\n", strerror(errno));
		return -1;
	}

	// then send the messages
	if (0 == send_chat_messages(in_client_socket, in_all_messages, start_index, stop_index)) {
		// update the index if successful
		in_next_message[in_client_socket] = stop_index;
	}
	else {
		util_send_tcp(in_client_socket, -1);
	}

	return 0;
}

/*
 * send_chat_messages
 */
int send_chat_messages(const int in_client_socket, const vector<string>& in_all_messages, const unsigned int start_index, const unsigned int stop_index) {
	if (start_index > stop_index) {
		fprintf(stderr, "start index > stop index for client %d\n", in_client_socket);
		return -1;
	}

	if (start_index >= in_all_messages.size() || stop_index > in_all_messages.size()) {
		fprintf(stderr, "message index out of range for client %d\n", in_client_socket);
		return -1;
	}

	// write the message to the socket
	for (unsigned int i = start_index; i < stop_index; i++) {
		const string message = in_all_messages.at(i);

		// send the length
		util_send_tcp(in_client_socket, message.length());

		// then the message itself
		util_send_tcp(in_client_socket, message.c_str(), message.length());
	}

	return 0;
}

