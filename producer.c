//
// Created by martin on 18/12/24.
//

#include "producer.h"
#include "queue.h"

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <termios.h>
#include <threads.h>
#include <sys/ioctl.h>
#include <gpiod.h>
#include <signal.h>

#define SLEEP_US 900000
#define MAX_FAILS 5

int fail_check(int *fail_count, int max_fails, volatile int *alive) {
    if (*fail_count < max_fails) {
        ++(*fail_count);
        sleep(1);
        return 1;
    }
    *alive = 0;
    return 0;
}

void *producer_thread(void *arg) {
    enum gpiod_line_value gps_leds_off[] = {
        GPIOD_LINE_VALUE_INACTIVE,
        GPIOD_LINE_VALUE_INACTIVE,
        GPIOD_LINE_VALUE_INACTIVE
    };

    enum gpiod_line_value gps_leds_green[] = {
        GPIOD_LINE_VALUE_ACTIVE,
        GPIOD_LINE_VALUE_INACTIVE,
        GPIOD_LINE_VALUE_INACTIVE
    };

    enum gpiod_line_value gps_leds_yellow[] = {
        GPIOD_LINE_VALUE_INACTIVE,
        GPIOD_LINE_VALUE_ACTIVE,
        GPIOD_LINE_VALUE_INACTIVE
    };

    enum gpiod_line_value gps_leds_red[] = {
        GPIOD_LINE_VALUE_INACTIVE,
        GPIOD_LINE_VALUE_INACTIVE,
        GPIOD_LINE_VALUE_ACTIVE
    };

    const ProducerArgs *producer_args = (ProducerArgs *) arg;
    Queue *queue = producer_args->queue;
    struct gpiod_line_request *led_request = producer_args->led_request;
    volatile int *alive = producer_args->alive;

    int serial_port = open("/dev/ttyUSB2", O_RDWR);

    if (serial_port < 0) {
        printf("Error %i from open: %s\n", errno, strerror(errno));
        gpiod_line_request_set_values(led_request, gps_leds_red);
        return NULL;
    }

    struct termios tty;

    if (tcgetattr(serial_port, &tty) != 0) {
        printf("Error %i from tcgetattr: %s\n", errno, strerror(errno));
        gpiod_line_request_set_values(led_request, gps_leds_red);
        return NULL;
    }

    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;
    tty.c_cflag &= ~CRTSCTS;
    tty.c_cflag |= CREAD | CLOCAL;

    tty.c_lflag &= ~ICANON;
    tty.c_lflag &= ~ECHO;
    tty.c_lflag &= ~ECHOE;
    tty.c_lflag &= ~ECHONL;
    tty.c_lflag &= ~ISIG;
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL);

    tty.c_oflag &= ~OPOST;
    tty.c_oflag &= ~ONLCR;

    tty.c_cc[VTIME] = 1; // wait for up to x deciseconds, returning as soon as any data is received.
    tty.c_cc[VMIN] = 0;

    cfsetispeed(&tty, B115200);
    cfsetospeed(&tty, B115200);

    tcflush(serial_port, TCIFLUSH);

    if (tcsetattr(serial_port, TCSANOW, &tty) != 0) {
        printf("Error %i from tcsetattr: %s\n", errno, strerror(errno));
        gpiod_line_request_set_values(led_request, gps_leds_red);
        *alive = 0;
        pthread_exit(NULL);
    }

    unsigned char msg[] = "AT+CGNSSINFO\r\n";
    char response_prefix[] = "AT+CGNSSINFO\r\r\n+CGNSSINFO: ";
    int response_prefix_len = strlen(response_prefix);
    char response_suffix[] = "OK";
    int response_suffix_len = strlen(response_suffix);
    char read_buf[128];
    int fail_count = 0;

    while (*alive) {
        int status;
        if (ioctl(serial_port, TIOCMGET, &status) < 0) {
            printf("Serial port check failed: %s\n", strerror(errno));
            gpiod_line_request_set_values(led_request, gps_leds_red);
            if (fail_check(&fail_count, MAX_FAILS, alive)) {
                continue;
            } else {
                break;
            }
        }

        if (write(serial_port, msg, sizeof(msg)) < (ssize_t) sizeof(msg)) {
            printf("Error %i from write: %s\n", errno, strerror(errno));
            gpiod_line_request_set_values(led_request, gps_leds_red);
            if (fail_check(&fail_count, MAX_FAILS, alive)) {
                continue;
            } else {
                break;
            }
        };

        int num_bytes = 0;
        int total_bytes = 0;

        while ((num_bytes = read(serial_port, &read_buf[total_bytes], sizeof(read_buf) - total_bytes)) > 0) {
            total_bytes += num_bytes;
        };

        if (num_bytes < 0) {
            printf("Error reading: %s\n", strerror(errno));
            gpiod_line_request_set_values(led_request, gps_leds_red);
            if (fail_check(&fail_count, MAX_FAILS, alive)) {
                continue;
            } else {
                break;
            }
        }

        if (total_bytes < 50) {
            printf("Not enough data read from serial port.\n");
            gpiod_line_request_set_values(led_request, gps_leds_red);
            if (fail_check(&fail_count, MAX_FAILS, alive)) {
                continue;
            } else {
                break;
            }
        }

        if (strncmp(response_prefix, read_buf, response_prefix_len) != 0) {
            printf("Wrong prefix: %s\n", read_buf);
            gpiod_line_request_set_values(led_request, gps_leds_red);
            continue;
        }

        if (strncmp(&read_buf[total_bytes - 2 - response_suffix_len], response_suffix, response_suffix_len) != 0) {
            printf("Wrong suffix: %s\n", read_buf);
            gpiod_line_request_set_values(led_request, gps_leds_red);
            continue;
        }

        if (strncmp(",,,,,,,,,,,,,,,", &read_buf[27], 15) == 0) {
            printf("No GPS data...\n");
            gpiod_line_request_set_values(led_request, gps_leds_yellow);
        } else {
            read_buf[total_bytes - 8] = '\0';
            enqueue(queue, &read_buf[27]);
            gpiod_line_request_set_values(led_request, gps_leds_green);
        }
        // enqueue(queue, &read_buf[27]);
        fail_count = 0;
        usleep(SLEEP_US);
    }

    pthread_cond_signal(&queue->cond);
    gpiod_line_request_set_values(led_request, gps_leds_off);
    close(serial_port);
    printf("Producer thread exiting.\n");
    pthread_exit(NULL);
}
