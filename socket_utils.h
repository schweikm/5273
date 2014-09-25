#ifndef __SOCKET_UTILS_H
#define __SOCKET_UTILS_H

/* constants */
const int LISTEN_QUEUE_LENGTH = 32;
const int BUFFER_SIZE = 4096;

/* function declarations */
int util_create_server_socket(const int in_socket_type, const int in_protocol, const char* const in_host, const int in_port);
int util_create_client_socket(const int in_socket_type, const int in_protocol, const char* const in_host, const int in_port);
int util_listen(const int in_socket);

int util_send_tcp(const int in_socket, const int in_int);
int util_send_tcp(const int in_socket, const char* const in_buf, const int in_buf_len);
int util_recv_tcp(const int in_socket, int& ret_int);
int util_recv_tcp(const int in_socket, char* ret_buf, const int ret_buf_len);

int util_recv_udp(const int in_socket, int& ret_int);
int util_recv_udp(const int in_socket, char* ret_buf, const int ret_buf_len);

#endif /* __SOCKET_UTILS_H */
