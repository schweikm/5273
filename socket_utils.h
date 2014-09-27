#ifndef __CSCI_5273_SOCKET_UTILS_H
#define __CSCI_5273_SOCKET_UTILS_H

/**
 * @file socket_utils.h
 * @author Marc Schweikert
 * @date 26 September 2014
 * @brief Generic socket library
 */

#include <sys/socket.h>


/** Maximum number of queued connections for listen() */
const int LISTEN_QUEUE_LENGTH = 32;
/** Maximum amount of data that can be sent or recv'd */
const int BUFFER_SIZE = 4096;


/**
  * Creates and binds a server socket.
  *
  * @pre none
  * @post A new socket is created
  * @param in_socket_type The type of socket to create e.g. SOCK_DGRAM or SOCK_STREAM
  * @param in_protocol The socket protocol e.g. IPPROTO_UDP or IPPROTO_TCP
  * @param in_host The hostname or IP address to bind to.  NULL if any address is valid
  * @param in_port The port number to bind to.  0 for OS to choose for you
  * @return Socket file descriptor if successful; -1 if error.
  */
int util_create_server_socket(const int in_socket_type,
                              const int in_protocol,
                              const char* const in_host,
                              const int in_port);

/**
  * Creates a client socket and connects to the specified host / port.
  *
  * @pre none
  * @post A new socket is created
  * @param in_socket_type The type of socket to create e.g. SOCK_DGRAM or SOCK_STREAM
  * @param in_protocol The socket protocol e.g. IPPROTO_UDP or IPPROTO_TCP
  * @param in_host The hostname or IP address to bind to.  NULL if any address is valid
  * @param in_port The port number to bind to.  0 for the OS to choose one for you
  * @return Socket file descriptor if successful; -1 if error.
  */
int util_create_client_socket(const int in_socket_type,
                              const int in_protocol,
                              const char* const in_host,
                              const int in_port);

/**
  * Starts listen()'ing on the provided socket.
  *
  * @pre in_socket is a valid socket file descriptor
  * @post in_socket is listening for incoming connections
  * @param in_socket A socket file descriptor to call listen() on
  * @return 0 if successful; -1 if error.
  */
int util_listen(const int in_socket);

/**
  * Initializes a sockaddr_in instance
  *
  * @pre in_sin has been created and is not NULL
  * @post in_sin has been initialized
  * @param in_host The hostname / IP address to use.  NULL if any address is valid
  * @param in_port The port number to use.  0 for the OS to choose one for you
  * @param in_sin The structure to initialize
  * @return 0 if successful; -1 if error.
  */
int util_create_sockaddr(const char* const in_host,
                         const int in_port,
                         struct sockaddr_in* in_sin);

/**
  * Retrieves the port number assigned to the socket.
  *
  * @pre in_socket is a valid socket file descriptor
  * @post none
  * @param in_socket Socket file descriptor to query
  * @return Port number if successful; -1 if error.
  */
int util_get_port_number(const int in_socket);

/**
  * Sends an integer value using TCP.
  *
  * @pre in_socket is a valid socket file descriptor
  * @post in_int has been sent
  * @param in_socket Socket file descriptor to use
  * @param in_int Integer value to send
  * @return 0 if successful; -1 if error.
  */
int util_send_tcp(const int in_socket,
                  const int in_int);

/**
  * Sends a generic byte array using TCP.
  *
  * @pre in_socket is a valid socket file descriptor
  * @post in_buf has been sent
  * @param in_socket Socket file descriptor to use
  * @param in_buf Byte array to send
  * @param in_buf_len Number of bytes to send
  * @return 0 if successful; -1 if error.
  */
int util_send_tcp(const int in_socket,
                  const char* const in_buf,
                  const int in_buf_len);

/**
  * Receive an integer value using TCP.
  *
  * @pre in_socket is a valid socket file descriptor
  * @post in_ret_int contains the recv'd integer
  * @param in_socket Socket file descriptor to use
  * @param in_ret_int Variable to store the recv'd integer
  * @param in_flags Flags to pass to recv()
  * @return Number of bytes received if successful; -1 if error.
  */
int util_recv_tcp(const int in_socket,
                  int& in_ret_int,
                  const int in_flags = 0);

/**
  * Receive a generic byte array using TCP.
  *
  * @pre in_socket is a valid socket file descriptor.  in_ret_buf is an allocated buffer.  in_ret_buf_len is 1 less than the buffer size.
  * @post in_ret_buf contains the recv'd value and the last position in the buffer is 0.
  * @param in_socket Socket file descriptor to use
  * @param in_ret_buf Buffer to store the recv'd value
  * @param in_ret_buf_len The number of bytes to recv()
  * @param in_flags Flags to pass to recv()
  * @return Number of bytes received if successful; -1 if error.
  */
int util_recv_tcp(const int in_socket,
                  char* in_ret_buf,
                  const int in_ret_buf_len,
                  const int in_flags = 0);

/**
  * Sends an integer value using UDP.
  *
  * @pre in_socket is a valid socket file descriptor
  * @post in_int has been sent to the specified address
  * @param in_socket Socket file descriptor to use
  * @param in_int Integer value to send
  * @param in_to Holds the address of the UDP receiver
  * @return 0 if successful; -1 if error.
  */
int util_send_udp(const int in_socket,
                  const int in_int,
                  const struct sockaddr* in_to);

/**
  * Sends a generic byte array using UDP.
  *
  * @pre in_socket is a valid socket file descriptor.  in_ret_buf is an allocated buffer.  in_ret_buf_len is 1 less than the buffer size.
  * @post in_ret_buf contains the recv'd value and the last position in the buffer is 0.
  * @param in_socket Socket file descriptor to use
  * @param in_buf Byte array to send
  * @param in_buf_len Number of bytes to send
  * @param in_to Holds the address of the UDP receiver
  * @return Number of bytes received if successful; -1 if error.
  */
int util_send_udp(const int in_socket,
                  const char* const in_buf,
                  const int in_buf_len,
                  const struct sockaddr* in_to);

/**
  * Receive an integer value using UDP.
  *
  * @pre in_socket is a valid socket file descriptor
  * @post in_ret_int contains the recv'd integer
  * @param in_socket Socket file descriptor to use
  * @param in_ret_int Variable to store the recv'd integer
  * @param in_from The UDP sender information
  * @param in_from_len Length of the sender's UDP information
  * @param in_flags Flags to pass to recv()
  * @return Number of bytes received if successful; -1 if error.
  */
int util_recv_udp(const int in_socket,
                  int& in_ret_int,
                  struct sockaddr *in_from,
                  socklen_t in_from_len,
                  const int in_flags = 0);

/**
  * Receive a generic byte array using UDP.
  *
  * @pre in_socket is a valid socket file descriptor.  in_ret_buf is an allocated buffer.  in_ret_buf_len is 1 less than the buffer size.
  * @post in_ret_buf contains the recv'd value and the last position in the buffer is 0.
  * @param in_socket Socket file descriptor to use
  * @param in_ret_buf Buffer to store the recv'd value
  * @param in_ret_buf_len The number of bytes to recv()
  * @param in_from The UDP sender information
  * @param in_from_len Length of the sender's UDP information
  * @param in_flags Flags to pass to recv()
  * @return Number of bytes received if successful; -1 if error.
  */
int util_recv_udp(const int in_socket,
                  char* in_ret_buf,
                  const int in_ret_buf_len,
                  struct sockaddr *in_from,
                  socklen_t in_from_len,
                  const int in_flags = 0);

#endif /* __CSCI_5273_SOCKET_UTILS_H */

