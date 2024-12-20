cmake_minimum_required(VERSION 3.30)
project(gps-client C)

set(CMAKE_C_STANDARD 23)

add_executable(
        gps-client
        main.c
        net.h
        net.c
        queue.c
        queue.h
        constants.h
        producer.c
        producer.h
        consumer.c
        consumer.h
)
