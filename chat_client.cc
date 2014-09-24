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

#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

using std::cin;
using std::cout;
using std::endl;
using std::string;

/* constants */
const int BUFFER_SIZE = 4096;
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
int create_command_socket();
int send_udp_command(const int, const char* const, const int, const string&, const bool);
int create_tcp_socket(const char* const, const int);
int send_tcp_command(const int, const string&);
int receive_tcp(const int, char*, const int);
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
	const int command_socket = create_command_socket();
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
				const int new_socket = create_tcp_socket(coordinator_host, recv_val);
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
				const int new_socket = create_tcp_socket(coordinator_host, recv_val);
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
			if (0 != send_tcp_command(active_session_socket, CMD_SUBMIT + " " + user_message)) {
				fprintf(stderr, "Failed to submit message\n");
			}
		}
		else if (CMD_GET_NEXT == user_command) {
			// send the coomand
			if (-1 == send_tcp_command(active_session_socket, CMD_GET_NEXT)) {
				fprintf(stderr, "Failure during get_next\n");
			}
			else {
				print_session_message(active_session_socket);
			}
		}
		else if (CMD_GET_ALL == user_command) {
			// send the coomand
			if (-1 == send_tcp_command(active_session_socket, CMD_GET_ALL)) {
				fprintf(stderr, "Failure during get_all\n");
			}
			else {
				char num_buf[BUFFER_SIZE];
				if(-1 == receive_tcp(active_session_socket, num_buf, BUFFER_SIZE)) {
					fprintf(stderr, "Failed to receive number of messages\n");
				}
				else {
					const int num_messages = atoi(num_buf);
					for (int i = 0; i < num_messages; i++) {
						print_session_message(active_session_socket);
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

int create_command_socket() {
	// create new UDP socket
	const int command_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP); 

	// command address
	struct sockaddr_in si_command;
	memset(&si_command, 0, sizeof(si_command));
	si_command.sin_family = AF_INET;
	si_command.sin_port = htons(0);
	si_command.sin_addr.s_addr = htonl(INADDR_ANY);

	// bind socket to port
	if (-1 == bind(command_socket, (struct sockaddr*)&si_command, sizeof(si_command) )) {
		fprintf(stderr, "Failed to bind command socket.  Error is %s\n", strerror(errno));
		return -1;
	}

	return command_socket;
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

int create_tcp_socket(const char* const in_coord_host, const int in_tcp_port) {
	struct sockaddr_in sin;
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port=htons(in_tcp_port);

	// we may be passed a hostname rather than IP address
	struct hostent *hostp = gethostbyname(in_coord_host);
	if (NULL == hostp) {
		fprintf(stderr, "Unable to resolve hostname!");
		return -1;
	}
	memcpy(&sin.sin_addr, hostp->h_addr, sizeof(sin.sin_addr));

	const int tcp_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (tcp_socket < 0) {
		fprintf(stderr, "Failed to create socket\n");
		return -1;
	}

	if (connect(tcp_socket, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
		fprintf(stderr, "failed to connect socket %s\n", strerror(errno));
        return -1;
	}

	return tcp_socket;
}

int send_tcp_command(const int in_socket, const string& in_command) {
    #ifdef DEBUG
    printf("DEBUG:  chat_server - sending |%s|\n", in_command.c_str());
    #endif

	if (-1 == in_socket) {
		fprintf(stderr, "Invalid socket passed as argument!\n");
		return -1;
	}

    // send the code
    if (-1 == send(in_socket, in_command.c_str(), in_command.length(), 0)) {
        fprintf(stderr, "send called failed!  Error is %s\n", strerror(errno));
    } 

	return 0;
}

int receive_tcp(const int in_socket, char* in_recv_buffer, const int in_recv_buffer_len) {
	// receive the response
	memset(in_recv_buffer, 0, in_recv_buffer_len);

	const int num_bytes = recv(in_socket, in_recv_buffer, in_recv_buffer_len, 0); 
	if (num_bytes <= 0) {
		fprintf(stderr, "error or client disconnect: %s\n", strerror(errno));
		return -1;
	}

	// prevent buffer overflow
	assert(num_bytes <= in_recv_buffer_len);
	in_recv_buffer[num_bytes] = 0;
	in_recv_buffer[in_recv_buffer_len - 1] = 0;

    #ifdef DEBUG
    printf("DEBUG:  chat_server - received |%s|\n", in_recv_buffer);
    #endif

	return 0;
}

int print_session_message(const int in_socket) {
	// and get the response
	char recv_buffer[BUFFER_SIZE];

	if (-1 == receive_tcp(in_socket, recv_buffer, BUFFER_SIZE)) {
		fprintf(stderr, "Failure in TCP recv.  Error is %s\n", strerror(errno));
		return -1;
	}
	else {
		// parse the response
		string message(recv_buffer);
		if ("-1" == message) {
			printf("No new message in the chat session\n");
		}
		else {
			// since we are using C++ strings, we don't care about the first length parameter
			message = message.substr(message.find_first_of(" ") + 1);
			printf("%s\n", message.c_str());
		}
	}

	return 0;
}

