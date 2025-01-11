/*
** client.c -- a stream socket client demo
*/

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <gpiod.h>
#include <signal.h>

#include "consumer.h"
#include "net.h"
#include "producer.h"
#include "queue.h"

#define GREEN_LED_LINE 22
#define RED_LED_LINE 27

#define GPS_RED_LINE 23
#define GPS_YELLOW_LINE 24
#define GPS_GREEN_LINE 25

static volatile int alive = 1;

void sigHandler(int)
{
    alive = 0;
}

int main(int argc, char *argv[]) {

    signal(SIGINT, sigHandler);
    signal(SIGTERM, sigHandler);

    if (argc != 3) {
        fprintf(stderr, "usage: client hostname port\n");
        exit(1);
    }

    // open gpio chip
    struct gpiod_chip *chip = gpiod_chip_open("/dev/gpiochip0");
    if (!chip) {
        perror("gpiochip_open");
        return 1;
    }

    // get gpio line
    struct gpiod_line *green_led_line = gpiod_chip_get_line(chip, GREEN_LED_LINE);
    // reserve single gpio line
    if (gpiod_line_request_output(green_led_line, "green_led", 0) < 0) {
        perror("gpiod_line_request_output");
        gpiod_chip_close(chip);
        return 1;
    }

    // get gpio line
    struct gpiod_line *red_led_line = gpiod_chip_get_line(chip, RED_LED_LINE);
    // reserve single gpio line
    if (gpiod_line_request_output(red_led_line, "red_led", 1) < 0) {
        perror("gpiod_line_request_output");
        gpiod_chip_close(chip);
        return 1;
    }

    // get gpio line
    struct gpiod_line *gps_red_line = gpiod_chip_get_line(chip, GPS_RED_LINE);
    // reserve single gpio line
    if (gpiod_line_request_output(gps_red_line, "gps_red_led", 0) < 0) {
        perror("gpiod_line_request_output");
        gpiod_chip_close(chip);
        return 1;
    }

    // get gpio line
    struct gpiod_line *gps_yellow_line = gpiod_chip_get_line(chip, GPS_YELLOW_LINE);
    // reserve single gpio line
    if (gpiod_line_request_output(gps_yellow_line, "gps_yellow_led", 0) < 0) {
        perror("gpiod_line_request_output");
        gpiod_chip_close(chip);
        return 1;
    }

    // get gpio line
    struct gpiod_line *gps_green_line = gpiod_chip_get_line(chip, GPS_GREEN_LINE);
    // reserve single gpio line
    if (gpiod_line_request_output(gps_green_line, "gps_green_led", 0) < 0) {
        perror("gpiod_line_request_output");
        gpiod_chip_close(chip);
        return 1;
    }

    Queue *queue = init_queue();

    ConsumerArgs consumer_args;
    consumer_args.hostname = argv[1];
    consumer_args.port = argv[2];
    consumer_args.queue = queue;
    consumer_args.green_led_line = green_led_line;
    consumer_args.red_led_line = red_led_line;
    consumer_args.alive = &alive;

    GpsLedLines gps_led_lines;
    gps_led_lines.green = gps_green_line;
    gps_led_lines.yellow = gps_yellow_line;
    gps_led_lines.red = gps_red_line;

    ProducerArgs producer_args;
    producer_args.queue = queue;
    producer_args.gps_led_lines = &gps_led_lines;
    producer_args.alive = &alive;

    pthread_t prod_tid, cons_tid;
    pthread_create(&prod_tid, NULL, producer_thread, &producer_args);
    pthread_create(&cons_tid, NULL, consumer_thread, &consumer_args);

    pthread_join(prod_tid, NULL);
    pthread_join(cons_tid, NULL);

    printf("Main thread exiting.\n");

    // close down thread stuff
    pthread_mutex_destroy(&queue->lock);
    pthread_cond_destroy(&queue->cond);
    free_queue(queue);
    queue = NULL;

    // turn off LEDs
    gpiod_line_set_value(green_led_line, 0);
    gpiod_line_set_value(red_led_line, 0);
    gpiod_line_set_value(gps_red_line, 0);
    gpiod_line_set_value(gps_yellow_line, 0);
    gpiod_line_set_value(gps_green_line, 0);

    // release gpio lines
    gpiod_line_release(green_led_line);
    gpiod_line_release(red_led_line);
    gpiod_line_release(gps_red_line);
    gpiod_line_release(gps_yellow_line);
    gpiod_line_release(gps_green_line);

    gpiod_chip_close(chip);

    return 0;
}
