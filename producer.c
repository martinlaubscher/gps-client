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


void *producer_thread(void *arg) {
    Queue *queue = (Queue *) arg;

    int serial_port = open("/dev/ttyUSB2", O_RDWR);

    if (serial_port < 0) {
        printf("Error %i from open: %s\n", errno, strerror(errno));
        return NULL;
    }

    struct termios tty;

    if (tcgetattr(serial_port, &tty) != 0) {
        printf("Error %i from tcgetattr: %s\n", errno, strerror(errno));
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
        exit(1);
    }

    unsigned char msg[] = "AT+CGNSSINFO\r\n";
    char read_buf[128];

    while (1) {

        int status;
        if (ioctl(serial_port, TIOCMGET, &status) < 0) {
            printf("Serial port check failed: %s\n", strerror(errno));
            break;
        }

        if (write(serial_port, msg, sizeof(msg)) < sizeof(msg)) {
            printf("Error %i from write: %s\n", errno, strerror(errno));
            continue;
        };

        int num_bytes = 0;
        int total_bytes = 0;

        while ((num_bytes = read(serial_port, &read_buf[total_bytes], sizeof(read_buf) - total_bytes)) > 0) {
            total_bytes += num_bytes;
        };

        if (num_bytes < 0) {
            printf("Error reading: %s\n", strerror(errno));
            break;
        }

        if (total_bytes == 0) {
            printf("No data read from serial port.\n");
            continue;
        }

        if (total_bytes < 4) {
            printf("Not enough data read from serial port. n");
            continue;
        }

        if (strncmp("AT+CGNSSINFO\r\r\n+CGNSSINFO: ", read_buf, 27) != 0) {
            printf("Wrong prefix: %s\n", read_buf);
            continue;
        }


        if (strncmp(&read_buf[total_bytes - 4], "OK", 2) != 0) {
            printf("Wrong suffix: %s\n", read_buf);
            continue;
        }

        read_buf[total_bytes - 8] = '\0';

        if (strncmp(",,,,,,,,,,,,,,,", &read_buf[27], 15) == 0) {
            printf("No GPS data...\n");
        } else {
            enqueue(queue, &read_buf[27]);
        }

        usleep(900000);
    }

    close(serial_port);

    enqueue(queue, "quit");

    return NULL;
}
