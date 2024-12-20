//
// Created by martin on 18/12/24.
//

#ifndef NET_H
#define NET_H

#include <stddef.h>
#include <sys/types.h>
#include <stdint.h>

int connect_to_server(char *hostname, char *port);
// ssize_t send_message(int sockfd, char *msg, uint32_t length);
// ssize_t receive_message(int sockfd, char *buf, size_t bufsize);

#endif //NET_H
