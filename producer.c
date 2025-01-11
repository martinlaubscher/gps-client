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

void set_leds(const GpsLedLines *gps_led_lines, int red, int yellow, int green);

void *producer_thread(void *arg) {

    const ProducerArgs *producer_args = (ProducerArgs *) arg;
    Queue *queue = producer_args->queue;
    GpsLedLines *gps_led_lines = producer_args->gps_led_lines;
    volatile int *alive = producer_args->alive;

    int serial_port = open("/dev/ttyUSB2", O_RDWR);

    if (serial_port < 0) {
        printf("Error %i from open: %s\n", errno, strerror(errno));
        set_leds(gps_led_lines, 1, 0, 0);
        return NULL;
    }

    struct termios tty;

    if (tcgetattr(serial_port, &tty) != 0) {
        printf("Error %i from tcgetattr: %s\n", errno, strerror(errno));
        set_leds(gps_led_lines, 1, 0, 0);
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
        set_leds(gps_led_lines, 1, 0, 0);
        exit(1);
    }

    unsigned char msg[] = "AT+CGNSSINFO\r\n";
    char read_buf[128];

    while (*alive) {

        int status;
        if (ioctl(serial_port, TIOCMGET, &status) < 0) {
            printf("Serial port check failed: %s\n", strerror(errno));
            set_leds(gps_led_lines, 1, 0, 0);
            sleep(1);
            continue;
        }

        if (write(serial_port, msg, sizeof(msg)) < (ssize_t) sizeof(msg)) {
            printf("Error %i from write: %s\n", errno, strerror(errno));
            set_leds(gps_led_lines, 1, 0, 0);
            sleep(1);
            continue;
        };

        int num_bytes = 0;
        int total_bytes = 0;

        while ((num_bytes = read(serial_port, &read_buf[total_bytes], sizeof(read_buf) - total_bytes)) > 0) {
            total_bytes += num_bytes;
        };

        if (num_bytes < 0) {
            printf("Error reading: %s\n", strerror(errno));
            set_leds(gps_led_lines, 1, 0, 0);
            usleep(SLEEP_US);
            continue;
        }

        if (total_bytes < 50) {
            printf("Not enough data read from serial port.\n");
            set_leds(gps_led_lines, 1, 0, 0);
            usleep(SLEEP_US);
            continue;
        }

        if (strncmp("AT+CGNSSINFO\r\r\n+CGNSSINFO: ", read_buf, 27) != 0) {
            printf("Wrong prefix: %s\n", read_buf);
            set_leds(gps_led_lines, 1, 0, 0);
            continue;
        }


        if (strncmp(&read_buf[total_bytes - 4], "OK", 2) != 0) {
            printf("Wrong suffix: %s\n", read_buf);
            set_leds(gps_led_lines, 1, 0, 0);
            continue;
        }

        if (strncmp(",,,,,,,,,,,,,,,", &read_buf[27], 15) == 0) {
            printf("No GPS data...\n");
            set_leds(gps_led_lines, 0, 1, 0);
        } else {
            read_buf[total_bytes - 8] = '\0';
            enqueue(queue, &read_buf[27]);
            set_leds(gps_led_lines, 0, 0, 1);
        }
        // enqueue(queue, &read_buf[27]);

        usleep(SLEEP_US);
    }

    close(serial_port);

    pthread_cond_signal(&queue->cond);

    printf("Producer thread exiting.\n");

    pthread_exit(NULL);
}

void set_leds(const GpsLedLines *gps_led_lines, int red, int yellow, int green) {
    gpiod_line_set_value(gps_led_lines->red, red);
    gpiod_line_set_value(gps_led_lines->yellow, yellow);
    gpiod_line_set_value(gps_led_lines->green, green);
}