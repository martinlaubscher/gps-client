//
// Created by martin on 18/12/24.
//

#include "consumer.h"
#include "net.h"
#include "queue.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <gpiod.h>
#include <signal.h>

#define GREEN_LED_LINE 22
#define RED_LED_LINE 27
#define min(a,b) (((a) < (b)) ? (a) : (b))

void *consumer_thread(void *arg) {

    // open gpio chip
    struct gpiod_chip *chip = gpiod_chip_open("/dev/gpiochip0");
    if (!chip) {
        perror("gpiochip_open");
        return NULL;
    }

    // get gpio line
    struct gpiod_line *green_led_line = gpiod_chip_get_line(chip, GREEN_LED_LINE);
    // reserve single gpio line
    if (gpiod_line_request_output(green_led_line, "green_led", 0) < 0) {
        perror("gpiod_line_request_output");
        gpiod_chip_close(chip);
        return NULL;
    }

    // get gpio line
    struct gpiod_line *red_led_line = gpiod_chip_get_line(chip, RED_LED_LINE);
    // reserve single gpio line
    if (gpiod_line_request_output(red_led_line, "red_led", 1) < 0) {
        perror("gpiod_line_request_output");
        gpiod_chip_close(chip);
        return NULL;
    }

    const ConsumerArgs *consumer_args = (ConsumerArgs *) arg;
    Queue *queue = consumer_args->queue;
    const char *hostname = consumer_args->hostname;
    const char *port = consumer_args->port;

    int sockfd = connect_to_server(hostname, port);

    printf("sockfd: %d\n", sockfd);

    if (sockfd == -1) {
        perror("Failed to connect to server");
        return NULL;
    }

    while (1) {
        char *msg = peek_queue(queue);
        if (strcmp(msg, "quit") == 0) {
            break;
        }

        const uint32_t msg_len = min(strlen(msg), MAX_MSG_SIZE - 1);
        const uint32_t network_msg_len = htonl(msg_len);
        size_t total_len = sizeof(network_msg_len) + msg_len;
        char send_buf[total_len];

        memcpy(send_buf, &network_msg_len, sizeof(network_msg_len));
        memcpy(send_buf + sizeof(network_msg_len), msg, msg_len);

        if (send_all(sockfd, send_buf, &total_len) == -1) {
            gpiod_line_set_value(green_led_line, 0);
            gpiod_line_set_value(red_led_line, 1);
            perror("send failed: message\n");
            close(sockfd);
            sockfd = connect_to_server(hostname, port); // try reconnecting on error
            continue;
        }
        gpiod_line_set_value(red_led_line, 0);
        gpiod_line_set_value(green_led_line, 1);

        printf("sent: '%s'\n", msg);
        free(dequeue(queue));

    }

    gpiod_line_set_value(green_led_line, 0);
    gpiod_line_set_value(red_led_line, 0);

    gpiod_line_release(green_led_line);
    gpiod_line_release(red_led_line);
    gpiod_chip_close(chip);

    close(sockfd);
    return NULL;
}
