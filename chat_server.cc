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
const int BUFFER_SIZE = 4096;
const int MESSAGE_MAX_LENGTH = 80;
const int RECEIVE_TIMEOUT = 60;  // seconds

/* Chat Server Commands */
const string CMD_SUBMIT = "Submit";
const string CMD_GET_NEXT = "GetNext";
const string CMD_GET_ALL = "GetAll";
const string CMD_LEAVE = "Leave";

/* Chat Coordinator Commands */
const string CMD_COORD_TERM = "Terminate";

/* function declarations */
void do_submit(const string&, vector<string>&);
void do_get_next(const int, map<int, int>&, const vector<string>&);
void do_get_all(const int, map<int, int>&, const vector<string>&);
int send_chat_messages(const int, const vector<string>&, const unsigned int, const unsigned int);
void send_client_error(const int);
void send_coord_term(const int, const string&);


/*
 * main
 */
int main(const int, const char* const argv[]) {
	const int server_socket = atoi(argv[0]);
	const int coordinator_port = atoi(argv[1]);
	const string session_name = argv[2];
	printf("Chat Server session \"%s\" started.  Coordinator UDP port is %d\n", session_name.c_str(), coordinator_port);

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
			printf("Chat server closing after select timeout!\n");

			// tell the coordinator that we are exiting
			send_coord_term(coordinator_port, session_name);

			// clean up and exit
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

			// we have a new client, so initialize it's last read message
			next_message_map.insert(std::make_pair<int, int>(client_socket, 0));
		}

		for (int client_socket = 0; client_socket < nfds; ++client_socket) {
			if (client_socket != server_socket && FD_ISSET(client_socket, &rfds)) {
				// read the client message
				char recv_buffer[BUFFER_SIZE];
				memset(recv_buffer, 0, BUFFER_SIZE);

				const int num_bytes = recv(client_socket, recv_buffer, sizeof recv_buffer, 0);
				if (num_bytes <= 0) {
					close(client_socket);
					FD_CLR(client_socket, &afds);
					fprintf(stderr, "error or client disconnect: %s\n", strerror(errno));
					continue;
				}

				// prevent buffer overflow
				assert(num_bytes <= BUFFER_SIZE);
				recv_buffer[num_bytes] = 0;
				recv_buffer[BUFFER_SIZE - 1] = 0;

				#ifdef DEBUG
				printf("DEBUG:  chat_server - received |%s|\n", recv_buffer);
				#endif

				// parse message
				string message(recv_buffer);

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

					do_submit(arguments, all_messages);
				}
				else if (CMD_GET_NEXT == command) {
					do_get_next(client_socket, next_message_map, all_messages);
				}
				else if (CMD_GET_ALL == command) {
					do_get_all(client_socket, next_message_map, all_messages);
				}
				else if (CMD_LEAVE == command) {
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

/*
 * do_submit
 */
void do_submit(const string& in_message, vector<string>& in_all_messages) {
	// we only support 80 character messages
	string message_text = in_message;
	if (message_text.length() > MESSAGE_MAX_LENGTH) {
		fprintf(stderr, "Message is greater than %d characters - truncating.\n", MESSAGE_MAX_LENGTH);
		message_text = message_text.substr(0, MESSAGE_MAX_LENGTH);
	}

	// store the message in the chat history
	in_all_messages.push_back(in_message);
}

/*
 * do_get_next
 */
void do_get_next(const int in_client_socket, map<int, int>& in_next_message, const vector<string>& in_all_messages) {
	// let's make sure that the next message exists
	map<int, int>::const_iterator next_it = in_next_message.find(in_client_socket);
	if (in_next_message.end() == next_it) {
		fprintf(stderr, "failed to find last message index for client %d\n", in_client_socket);
		send_client_error(in_client_socket);
		return;
	}

	const int start_index = next_it->second;
	const int stop_index = start_index + 1;
	if (0 == send_chat_messages(in_client_socket, in_all_messages, start_index, stop_index)) {
		// update the index if successful
		in_next_message[in_client_socket] = stop_index;
	}
	else {
		send_client_error(in_client_socket);
	}
}

/*
 * do_get_all
 */
void do_get_all(const int in_client_socket, map<int, int>& in_next_message, const vector<string>& in_all_messages) {
	// let's make sure that the next message exists
	map<int, int>::const_iterator next_it = in_next_message.find(in_client_socket);
	if (in_next_message.end() == next_it) {
		fprintf(stderr, "failed to find last message index for client %d\n", in_client_socket);
		send_client_error(in_client_socket);
		return;
	}

	const int start_index = next_it->second;
	const int stop_index = in_all_messages.size();

	// we need to send the number of messages that will be sent first
	char msgs_buffer[BUFFER_SIZE];
	memset(msgs_buffer, 0, BUFFER_SIZE);
	sprintf(msgs_buffer, "%d", stop_index - start_index);

	#ifdef DEBUG
	printf("DEBUG:  chat_server - sending |%s|\n", msgs_buffer);
	#endif

	if (-1 == send(in_client_socket, msgs_buffer, strlen(msgs_buffer), 0)) {
		fprintf(stderr, "get send called failed!  Error is %s\n", strerror(errno));
	}

	// then send the messages
	if (0 == send_chat_messages(in_client_socket, in_all_messages, start_index, stop_index)) {
		// update the index if successful
		in_next_message[in_client_socket] = stop_index;
	}
	else {
		send_client_error(in_client_socket);
	}
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
		const size_t message_length = message.length();

		char message_buffer[BUFFER_SIZE];
	    memset(message_buffer, 0, BUFFER_SIZE);
	    sprintf(message_buffer, "%lu %s", message_length, message.c_str());

		#ifdef DEBUG
		printf("DEBUG:  chat_server - sending |%s|\n", message_buffer);
		#endif

    	if (-1 == send(in_client_socket, message_buffer, strlen(message_buffer), 0)) {
	        fprintf(stderr, "get send called failed!  Error is %s\n", strerror(errno));
			return -1;
	    }
	}

	return 0;
}

/*
 * send_client_error
 */
void send_client_error(const int in_socket) {
    char ret_str[BUFFER_SIZE];
    memset(ret_str, 0, BUFFER_SIZE);
    sprintf(ret_str, "%d", -1);
	const size_t ret_len = strlen(ret_str);

    #ifdef DEBUG
    printf("DEBUG:  chat_server - sending |%d|\n", in_socket);
    #endif

    // send the code
    if (-1 == send(in_socket, ret_str, ret_len, 0)) {
        fprintf(stderr, "send called failed!  Error is %s\n", strerror(errno));
    }   
}

void send_coord_term(const int in_coordinator_port, const string& in_session_name) {
	// create new UDP socket
	const int udp_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	// address
	struct sockaddr_in si_me;
	memset((char *) &si_me, 0, sizeof(si_me));     
	si_me.sin_family = AF_INET;
	si_me.sin_port = htons(0);
	si_me.sin_addr.s_addr = htonl(INADDR_ANY);

	bind(udp_socket, (struct sockaddr*)&si_me, sizeof(si_me) );

	char term_str[BUFFER_SIZE];
	memset(term_str, 0, BUFFER_SIZE);
	sprintf(term_str, "%s %s", CMD_COORD_TERM.c_str(), in_session_name.c_str());
	const size_t term_len = strlen(term_str);

	// communicate over UDP
	struct sockaddr_in coord_addr;
	memset((char *)&coord_addr, 0, sizeof(coord_addr));
	coord_addr.sin_family = AF_INET;
	coord_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	coord_addr.sin_port = htons(in_coordinator_port);
	socklen_t coord_addr_len = sizeof(coord_addr);

	#ifdef DEBUG
	printf("DEBUG:  chat_server - sending |%s|\n", term_str);
	#endif

	if (-1 == sendto(udp_socket, term_str, term_len, 0, (struct sockaddr *)&coord_addr, coord_addr_len)) {
		fprintf(stderr, "sendto called failed!  Error is %s\n", strerror(errno));
	}  

	// clean up and exit
	close(udp_socket);
}

