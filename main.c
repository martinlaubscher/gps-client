/*
** client.c -- a stream socket client demo
*/

#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <gpiod.h>
#include <signal.h>
#include <string.h>

#include "consumer.h"
#include "net.h"
#include "producer.h"
#include "queue.h"

#define RED_LED_LINE 15
#define GREEN_LED_LINE 18

#define GPS_RED_LINE 17
#define GPS_YELLOW_LINE 27
#define GPS_GREEN_LINE 22

#define NUM_GPS_LEDS 3
#define NUM_CONSUMER_LEDS 2


static volatile int alive = 1;

void sigHandler(int) {
    alive = 0;
}

static struct gpiod_line_request *
request_lines(struct gpiod_chip *chip, const unsigned int *offsets, unsigned int num_lines, const char *consumer,
              enum gpiod_line_direction direction, enum gpiod_line_bias bias) {
    struct gpiod_request_config *rconfig = NULL;
    struct gpiod_line_request *request = NULL;
    struct gpiod_line_settings *settings;
    struct gpiod_line_config *lconfig;

    settings = gpiod_line_settings_new();
    if (!settings)
        return NULL;

    gpiod_line_settings_set_direction(settings, direction);
    gpiod_line_settings_set_bias(settings, bias);

    lconfig = gpiod_line_config_new();
    if (!lconfig)
        goto free_settings;

    if (gpiod_line_config_add_line_settings(lconfig, offsets, num_lines, settings) == -1) {
        goto free_settings;
    }


    if (consumer) {
        rconfig = gpiod_request_config_new();
        if (!rconfig)
            goto free_line_config;

        gpiod_request_config_set_consumer(rconfig, consumer);
    }

    request = gpiod_chip_request_lines(chip, rconfig, lconfig);
    gpiod_request_config_free(rconfig);

free_line_config:
    gpiod_line_config_free(lconfig);

free_settings:
    gpiod_line_settings_free(settings);

    return request;
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
        return EXIT_FAILURE;;
    }

    static const unsigned int gps_led_offsets[NUM_GPS_LEDS] = {GPS_GREEN_LINE, GPS_YELLOW_LINE, GPS_RED_LINE};
    static const unsigned int consumer_led_offsets[NUM_CONSUMER_LEDS] = {GREEN_LED_LINE, RED_LED_LINE};


    struct gpiod_line_request *gps_led_request = request_lines(chip, gps_led_offsets,NUM_GPS_LEDS, "gps_led",
                                                               GPIOD_LINE_DIRECTION_OUTPUT, GPIOD_LINE_BIAS_AS_IS);
    struct gpiod_line_request *consumer_led_request = request_lines(chip, consumer_led_offsets, NUM_CONSUMER_LEDS,
                                                                    "consumer_led",
                                                                    GPIOD_LINE_DIRECTION_OUTPUT, GPIOD_LINE_BIAS_AS_IS);
    gpiod_chip_close(chip);

    if (!gps_led_request || !consumer_led_request) {
        fprintf(stderr, "failed to request line: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    Queue *queue = init_queue();

    ConsumerArgs consumer_args;
    consumer_args.hostname = argv[1];
    consumer_args.port = argv[2];
    consumer_args.queue = queue;
    consumer_args.led_request = consumer_led_request;
    consumer_args.alive = &alive;

    ProducerArgs producer_args;
    producer_args.queue = queue;
    producer_args.led_request = gps_led_request;
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

    // release gpio lines
    gpiod_line_request_release(gps_led_request);
    gpiod_line_request_release(consumer_led_request);

    return 0;
}
