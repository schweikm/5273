#ifndef __CSCI_5273_SOCKET_UTILS_H
#define __CSCI_5273_SOCKET_UTILS_H

#include <sys/socket.h>

/* constants */
const int LISTEN_QUEUE_LENGTH = 32;
const int BUFFER_SIZE = 4096;

/* function declarations */
int util_create_server_socket(const int in_socket_type, const int in_protocol, const char* const in_host, const int in_port);
int util_create_client_socket(const int in_socket_type, const int in_protocol, const char* const in_host, const int in_port);
int util_listen(const int in_socket);
int util_create_sockaddr(const char* const in_host, const int in_port, struct sockaddr_in* in_sin);

int util_send_tcp(const int in_socket, const int in_int);
int util_send_tcp(const int in_socket, const char* const in_buf, const int in_buf_len);
int util_recv_tcp(const int in_socket, int& ret_int, const int in_flags = 0);
int util_recv_tcp(const int in_socket, char* ret_buf, const int ret_buf_len, const int in_flags = 0);

int util_send_udp(const int in_socket, const int in_int, const struct sockaddr* in_to, const socklen_t in_to_len);
int util_send_udp(const int in_socket, const char* const in_buf, const int in_buf_len, const struct sockaddr* in_to, const socklen_t in_to_len);
int util_recv_udp(const int in_socket, int& recv_int, struct sockaddr *src_addr, socklen_t addrlen, const int in_flags = 0);
int util_recv_udp(const int in_socket, char* ret_buf, const int ret_buf_len, struct sockaddr *src_addr, socklen_t addrlen, const int in_flags = 0);

#endif /* __CSCI_5273_SOCKET_UTILS_H */

