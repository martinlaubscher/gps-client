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


void *producer_thread(void *arg) {
    Queue *queue = (Queue *) arg;

    int serial_port = open("/dev/ttyUSB2", O_RDWR);

    if (serial_port < 0) {
        printf("Error %i from open: %s\n", errno, strerror(errno));
    }

    struct termios tty;

    if (tcgetattr(serial_port, &tty) != 0) {
        printf("Error %i from tcgetattr: %s\n", errno, strerror(errno));
        exit(1);
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

    tty.c_cc[VTIME] = 10; // wait for up to 1s (10 deciseconds), returning as soon as any data is received.
    tty.c_cc[VMIN] = 0;

    cfsetispeed(&tty, B115200);
    cfsetospeed(&tty, B115200);

    tcflush(serial_port, TCIFLUSH);

    if (tcsetattr(serial_port, TCSANOW, &tty) != 0) {
        printf("Error %i from tcsetattr: %s\n", errno, strerror(errno));
        exit(1);
    }

    unsigned char msg[] = "AT+CGNSSINFO\r\n";

    while (1) {
        char read_buf[128] = {0};

        if (write(serial_port, msg, sizeof(msg)) < sizeof(msg)) {
            printf("Error %i from write: %s\n", errno, strerror(errno));
        };

        int num_bytes = 0;
        int total_bytes = 0;

        while ((num_bytes = read(serial_port, &read_buf[total_bytes], sizeof(read_buf) - total_bytes)) > 0) {
            total_bytes += num_bytes;
        };

        if (num_bytes < 0) {
            printf("Error reading: %s", strerror(errno));
            break;
        }

        enqueue(queue, read_buf);

        sleep(1);
    }

    close(serial_port);


    // for (int i = 100; i > 0; --i) {
    //     char msg[MAX_MSG_SIZE] = {0};
    //     sprintf(msg, "+CGNSSINFO: %d,05,02,03,473%d.235748,N,00738.929515,E,161224,144959.0,317.2,0.0,,2.1,1.9,0.8", i, i);
    //     // printf("message: %s\n", msg);
    //
    //     enqueue(queue, msg);
    //
    //     const long ms = 100;
    //     usleep(ms * 1000);
    //
    // }

    enqueue(queue, "quit");

    return NULL;
}
