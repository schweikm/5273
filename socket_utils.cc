#include "socket_utils.h"

#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstring>

#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>

int util_create_server_socket(const int in_socket_type, const int in_protocol, const char* const in_host, const int in_port)
{
	// allocate a socket
	const int new_socket = socket(PF_INET, in_socket_type, in_protocol);
	if (new_socket < 0) {
		fprintf(stderr, "Unable to create socket.  Error is %s\n", strerror(errno));
		return -1;
	}

	// an Internet endpoint address
	struct sockaddr_in sin;
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;           // Internet Protocol
	sin.sin_port=htons(in_port);

	if (NULL == in_host) {
		sin.sin_addr.s_addr = INADDR_ANY;   // use any available address
	}
	else {
		// we were passed a hostname rather than IP address
		struct hostent *hostp = gethostbyname(in_host);
		if (NULL == hostp) {
			fprintf(stderr, "Unable to resolve hostname!");
			return -1;
		}
		memcpy(&sin.sin_addr, hostp->h_addr, sizeof(sin.sin_addr));
	}

	// bind the socket
	if (bind(new_socket, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
		fprintf(stderr, "Unable to bind to port.  Error is %s\n", strerror(errno));
		return -1;
	}

	return new_socket;
}

int util_create_client_socket(const int in_socket_type, const int in_protocol, const char* const in_host, const int in_port)
{
	// allocate a socket
	const int new_socket = socket(PF_INET, in_socket_type, in_protocol);
	if (new_socket < 0) {
		fprintf(stderr, "Unable to create socket.  Error is %s\n", strerror(errno));
		return -1;
	}

	// an Internet endpoint address
	struct sockaddr_in sin;
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;           // Internet Protocol
	sin.sin_port=htons(in_port);

	if (NULL == in_host) {
		sin.sin_addr.s_addr = INADDR_ANY;   // use any available address
	}
	else {
		// we were passed a hostname rather than IP address
		struct hostent *hostp = gethostbyname(in_host);
		if (NULL == hostp) {
			fprintf(stderr, "Unable to resolve hostname!");
			return -1;
		}
		memcpy(&sin.sin_addr, hostp->h_addr, sizeof(sin.sin_addr));
	}

	// connect to our endpoint
	if (connect(new_socket, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
		fprintf(stderr, "failed to connect socket %s\n", strerror(errno));
		return -1; 
	}  

	return new_socket;
}

int util_listen(const int in_socket) {
	// start the socket listening for connections
	if (listen(in_socket, LISTEN_QUEUE_LENGTH) < 0) {
		fprintf(stderr, "Failed to listen on socket.  Error is %s\n", strerror(errno));
		return -1;
	}

	return 0;
}


//
// TCP METHODS - SEND
//


int util_send_tcp(const int in_socket, const int in_int) {
    #ifdef DEBUG
    printf("DEBUG:  util (tcp) int - sending |%d|\n", in_int);
    #endif

	const int net_int = htonl(in_int);

    // send the number
    if (-1 == send(in_socket, &net_int, sizeof(net_int), 0)) {
        fprintf(stderr, "util (tcp) int send called failed!  Error is %s\n", strerror(errno));
		return -1;
    }

	return 0;
}

int util_send_tcp(const int in_socket, const char* const in_buf, const int in_buf_len) {
    #ifdef DEBUG
    printf("DEBUG:  util (tcp) str - sending |%s|\n", in_buf);
    #endif

    // send the string
    if (-1 == send(in_socket, in_buf, in_buf_len, 0)) {
        fprintf(stderr, "util (tcp) str send called failed!  Error is %s\n", strerror(errno));
		return -1;
    }

	return 0;
}


//
// TCP METHODS - RECEIVE
//


int util_recv_tcp(const int in_socket, int& ret_int) {
	int recv_int;

	const int num_bytes = recv(in_socket, &recv_int, sizeof(recv_int), 0);
	if (num_bytes <= 0) {
		fprintf(stderr, "util (tcp) int - error or client disconnect: %s\n", strerror(errno));
		return -1;
	}

	ret_int = ntohl(recv_int);

	#ifdef DEBUG
	printf("DEBUG:  util (tcp) int - received |%d|\n", ret_int);
	#endif

	return 0;
}

int util_recv_tcp(const int in_socket, char* ret_buf, const int ret_buf_len) {
	// make sure our buffer is clean
	memset(ret_buf, 0, ret_buf_len);

	const int num_bytes = recv(in_socket, ret_buf, ret_buf_len, 0);
	if (num_bytes <= 0) {
		fprintf(stderr, "util (tcp) str - error or client disconnect: %s\n", strerror(errno));
		return -1;
	}

	// prevent buffer overflow
	assert(num_bytes <= ret_buf_len);

	// THIS IS DANGEROUS
	// there must be a pre condition that the argument buffer has space for the received
	// string PLUS one more character for the NULL byte!
    ret_buf[num_bytes] = 0;

	#ifdef DEBUG
	printf("DEBUG:  util (tcp) str - received |%s|\n", ret_buf);
	#endif

    return num_bytes;
}


//
// UDP METHODS - SEND
//


//
// UDP METHODS - RECV
//


/*
int util_recv_udp(const int in_socket, long& ret_val) {
	// create the sender info
	struct sockaddr_in si_to;
	socklen_t si_to_len = sizeof(si_to);
	memset(&si_to, 0, si_to_len);
	si_to.sin_family = AF_INET;
	si_coord.sin_port = htons(in_coord_port);

    // we may be passed a hostname rather than IP address
    struct hostent *hostp = gethostbyname(in_coord_host);
    if (NULL == hostp) {
        fprintf(stderr, "Unable to resolve hostname!");
        return -1;
    }   
    memcpy(&si_coord.sin_addr, hostp->h_addr, sizeof(si_coord.sin_addr));

}

int util_recv_udp(const int in_socket, char* ret_buf, const int ret_buf_len) {
	return 0;
}

int util_recv_udp_impl(const int in_socket, char* in_recv_buf, const int in_recv_buf_len) {

	memset(in_recv_buf, 0, in_recv_buf_len);
	const ssize_t recv_len = recvfrom(in_socket, receive_buffer, BUFFER_SIZE, 0, (struct sockaddr *)&si_coord, &si_coord_len);

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
*/

