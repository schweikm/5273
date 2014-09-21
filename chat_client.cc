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

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

using std::string;

/* constants */
const int BUFFER_SIZE = 4096;

int create_command_socket();
int send_udp_command(const int, const char* const, const int, const string&);

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

	// begin command line processing
	send_udp_command(command_socket, coordinator_host, coordinator_port, "Start first");
	sleep(5);
	send_udp_command(command_socket, coordinator_host, coordinator_port, "Find first");
	sleep(60);
	send_udp_command(command_socket, coordinator_host, coordinator_port, "Find first");

	close(command_socket);
	return 0;
}

int create_command_socket() {
	// create new UDP socket
	const int command_socket = socket(AF_INET, SOCK_DGRAM, 0); 

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

int send_udp_command(const int in_command_socket, const char* const in_coord_host, const int in_coord_port, const string& in_command) {
	// create the info for the coordinator
	struct sockaddr_in si_coord;
	socklen_t si_coord_len = sizeof(si_coord);
	memset(&si_coord, 0, si_coord_len);
	si_coord.sin_family = AF_INET;
	si_coord.sin_port = htons(in_coord_port);
	si_coord.sin_addr.s_addr = inet_addr(in_coord_host);

	// create the sendable string
	#ifdef DEBUG
	assert(in_command.length() < BUFFER_SIZE - 1);
	#endif

	char command_str[BUFFER_SIZE];
	memset(command_str, 0, BUFFER_SIZE);
	strncpy(command_str, in_command.c_str(), in_command.length());
	const size_t command_len = strlen(command_str);

	#ifdef DEBUG
	printf("DEBUG:  client sending |%s| to coordinator\n", command_str);
	#endif

	// send the command
	if (-1 == sendto(in_command_socket, command_str, command_len, 0, (struct sockaddr*)&si_coord, si_coord_len)) {
		fprintf(stderr, "sendto called failed!  Error is %s\n", strerror(errno));
		return -1;
	}

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
	#ifdef DEBUG
	assert(recv_len < BUFFER_SIZE);
	#endif
	receive_buffer[recv_len] = 0;
	receive_buffer[BUFFER_SIZE - 1] = 0;

	#ifdef DEBUG
	printf("DEBUG:  client received raw response |%s|\n", receive_buffer);
	#endif

	return 0;
}

