/**
 * @file socket_utils.cc
 * @author Marc Schweikert
 * @date 26 September 2014
 * @brief Generic socket library implementation
 */

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

	struct sockaddr_in sin;
	if(-1 == util_create_sockaddr(in_host, in_port, &sin)) {
		fprintf(stderr, "Failed to create sockaddr_in.  Error is %s\n", strerror(errno));
		return -1;
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

	struct sockaddr_in sin;
	if(-1 == util_create_sockaddr(in_host, in_port, &sin)) {
		fprintf(stderr, "Failed to create sockaddr_in.  Error is %s\n", strerror(errno));
		return -1;
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

int util_create_sockaddr(const char* const in_host, const int in_port, struct sockaddr_in* in_sin) {
	// an Internet endpoint address
	memset(in_sin, 0, sizeof(*in_sin));
	in_sin->sin_family = AF_INET;           // Internet Protocol
	in_sin->sin_port=htons(in_port);

	if (NULL == in_host) {
		in_sin->sin_addr.s_addr = INADDR_ANY;   // use any available address
	}
	else {
		// we were passed a hostname rather than IP address
		struct hostent *hostp = gethostbyname(in_host);
		if (NULL == hostp) {
			fprintf(stderr, "Unable to resolve hostname!");
			return -1;
		}
		memcpy(&in_sin->sin_addr, hostp->h_addr, sizeof(in_sin->sin_addr));
	}

	return 0;
}

int util_get_port_number(const int in_socket) {
    struct sockaddr_in sin;
    socklen_t socklen = sizeof(sin);
    memset(&sin, 0, sizeof(sin));

    if (getsockname(in_socket, (struct sockaddr *)&sin, &socklen) < 0) {
        fprintf(stderr, "getsockname called failed!  Error is %s\n", strerror(errno));
        return -1;
    }   

    return ntohs(sin.sin_port);
}



//
// TCP METHODS - SEND
//


int util_send_tcp(const int in_socket, const int in_int) {
	// same as UDP but with no sender!
	return util_send_udp(in_socket, in_int, NULL);
}

int util_send_tcp(const int in_socket, const char* const in_buf, const int in_buf_len) {
	// same as UDP but with no sender!
	return util_send_udp(in_socket, in_buf, in_buf_len, NULL);
}


//
// TCP METHODS - RECEIVE
//


int util_recv_tcp(const int in_socket, int& in_ret_int, const int in_flags) {
	// same as UDP but with no sender!
	return util_recv_udp(in_socket, in_ret_int, NULL, 0, in_flags);
}

int util_recv_tcp(const int in_socket, char* in_ret_buf, const int in_ret_buf_len, const int in_flags) {
	// same as UDP but with no sender!
	return util_recv_udp(in_socket, in_ret_buf, in_ret_buf_len, NULL, 0, in_flags);
}


//
// UDP METHODS - SEND
//


int util_send_udp(const int in_socket, const int in_int, const struct sockaddr* in_to) {
    #ifdef DEBUG
    printf("DEBUG:  util (int) - sending |%d|\n", in_int);
    #endif

	const int net_int = htonl(in_int);
	const socklen_t in_to_len = sizeof(*in_to);

    // send the number
    if (-1 == sendto(in_socket, &net_int, sizeof(net_int), 0, in_to, in_to_len)) {
        fprintf(stderr, "util (int) send called failed!  Error is %s\n", strerror(errno));
		return -1;
    }

	return 0;
}

int util_send_udp(const int in_socket, const char* const in_buf, const int in_buf_len, const struct sockaddr* in_to) {
    #ifdef DEBUG
    printf("DEBUG:  util (str) - sending |%s|\n", in_buf);
    #endif

	const socklen_t in_to_len = sizeof(*in_to);

    // send the string
    if (-1 == sendto(in_socket, in_buf, in_buf_len, 0, in_to, in_to_len)) {
        fprintf(stderr, "util (str) send called failed!  Error is %s\n", strerror(errno));
		return -1;
    }

	return 0;
}


//
// UDP METHODS - RECV
//


int util_recv_udp(const int in_socket, int& in_ret_int, struct sockaddr *in_from, socklen_t in_from_len, const int in_flags) {
	memset(in_from, 0, in_from_len);
	int recv_int;

	const int num_bytes = recvfrom(in_socket, &recv_int, sizeof(recv_int), in_flags, in_from, &in_from_len);
	if (num_bytes <= 0) {
		fprintf(stderr, "util (int) - recvfrom error or client disconnect: %s\n", strerror(errno));
		return -1;
	}

	in_ret_int = ntohl(recv_int);

	#ifdef DEBUG
	printf("DEBUG:  util (int) - received |%d|\n", ret_int);
	#endif

	return num_bytes;
}

int util_recv_udp(const int in_socket, char* in_ret_buf, const int in_ret_buf_len, struct sockaddr *in_from, socklen_t in_from_len, const int in_flags) {
	// make sure our buffer is clean
	memset(in_ret_buf, 0, in_ret_buf_len);
	memset(in_from, 0, in_from_len);

	const int num_bytes = recvfrom(in_socket, in_ret_buf, in_ret_buf_len, in_flags, in_from, &in_from_len);
	if (num_bytes <= 0) {
		fprintf(stderr, "util (str) - recvfrom error or client disconnect: %s\n", strerror(errno));
		return -1;
	}

	// prevent buffer overflow
	assert(num_bytes <= in_ret_buf_len);

	// THIS IS DANGEROUS
	// there must be a pre condition that the argument buffer has space for the received
	// string PLUS one more character for the NULL byte!
    in_ret_buf[num_bytes] = 0;

	#ifdef DEBUG
	printf("DEBUG:  util (str) - received |%s|\n", in_ret_buf);
	#endif

    return num_bytes;
}

