#define _POSIX_C_SOURCE 199309L

#include <arpa/inet.h>
#include <math.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <time.h>

#include "bmi270.h"
#include "bmi270_config_file.h"

#define UPDATE_RATE 200.0               // Hz       (Current Max: ~1000.0 Hz)
#define UPDATE_TIME (1.0 / UPDATE_RATE) // Seconds
#define NUM_DATA 14                     // Number of int values to send

/* ----------------------------------------------------
                    HELPER FUNCTIONS
-----------------------------------------------------*/

int32_t get_microseconds_delta(struct timespec *old_time, struct timespec *timer)
{
    double seconds = timer->tv_sec - old_time->tv_sec;
    double nanoseconds = timer->tv_nsec - old_time->tv_nsec;

    int32_t elapsed_us = (int32_t)(seconds * 1000000.0 + nanoseconds / 1000.0);

    return elapsed_us;
}

/* ----------------------------------------------------
                        MAIN
-----------------------------------------------------*/

int main()
{
    // -------------------------------------------------
    // INITIALIZATION
    // -------------------------------------------------

    struct bmi270 sensor_upper = {.i2c_addr = I2C_PRIM_ADDR};

    struct bmi270 sensor_lower = {.i2c_addr = I2C_SEC_ADDR};

    if (bmi270_init(&sensor_upper) == -1)
        printf("Failed to initialize sensor_upper. You might want to do a power cycle.\n");

    if (bmi270_init(&sensor_lower) == -1)
        printf("Failed to initialize sensor_lower. You might want to do a power cycle.\n");

    // -------------------------------------------------
    // HARDWARE CONFIGURATION
    // -------------------------------------------------

    set_mode(&sensor_upper, PERFORMANCE_MODE);
    set_acc_range(&sensor_upper, ACC_RANGE_2G);
    set_gyr_range(&sensor_upper, GYR_RANGE_1000);
    set_acc_odr(&sensor_upper, ACC_ODR_200);
    set_gyr_odr(&sensor_upper, GYR_ODR_200);
    set_acc_bwp(&sensor_upper, ACC_BWP_OSR4);
    set_gyr_bwp(&sensor_upper, GYR_BWP_OSR4);
    disable_fifo_header(&sensor_upper);
    enable_data_streaming(&sensor_upper);
    enable_acc_filter_perf(&sensor_upper);
    enable_gyr_noise_perf(&sensor_upper);
    enable_gyr_filter_perf(&sensor_upper);

    set_mode(&sensor_lower, PERFORMANCE_MODE);
    set_acc_range(&sensor_lower, ACC_RANGE_2G);
    set_gyr_range(&sensor_lower, GYR_RANGE_1000);
    set_acc_odr(&sensor_lower, ACC_ODR_200);
    set_gyr_odr(&sensor_lower, GYR_ODR_200);
    set_acc_bwp(&sensor_lower, ACC_BWP_OSR4);
    set_gyr_bwp(&sensor_lower, GYR_BWP_OSR4);
    disable_fifo_header(&sensor_lower);
    enable_data_streaming(&sensor_lower);
    enable_acc_filter_perf(&sensor_lower);
    enable_gyr_noise_perf(&sensor_lower);
    enable_gyr_filter_perf(&sensor_lower);

    // -------------------------------------------------
    // NETWORK CONFIGURATION
    // -------------------------------------------------

    int sock = socket(AF_INET, SOCK_DGRAM, 0);

    if (sock < 0)
    {
        printf("ERROR: Socket creation failed!\n");
        return -1;
    }

    struct sockaddr_in receiver_address;
    receiver_address.sin_family = AF_INET;
    receiver_address.sin_addr.s_addr = inet_addr("192.168.1.2");
    receiver_address.sin_port = htons(8000);


    // -------------------------------------------------
    // VARIABLES
    // -------------------------------------------------

    int data_streaming = 0;

    // for package data
    int16_t temp_acc[3], temp_gyr[3];
    int32_t data_array[NUM_DATA];
    struct timespec begin, data_timer, old_time1, old_time2;
    clock_gettime(CLOCK_MONOTONIC_RAW, &begin);
    old_time1 = begin;
    old_time2 = begin;

    // for timing
    struct timespec sleep_time = {0, (long)(1.0 / UPDATE_RATE * 1000000000.0)};
    struct timespec tic, toc;
    double sleep_time_avg = 0.0;
    double sleep_time_var = 0.0;
    int i = 0;

    while (1)
    {
        clock_gettime(CLOCK_MONOTONIC_RAW, &tic);

        // -------------------------------------------------
        // PACKAGE DATA
        // -------------------------------------------------

        clock_gettime(CLOCK_MONOTONIC_RAW, &data_timer);
        data_array[0] = get_microseconds_delta(&old_time1, &data_timer);    // Sensor 1 - Time
        old_time1 = data_timer;
        
        get_acc_raw(&sensor_upper, &temp_acc[0], &temp_acc[1], &temp_acc[2]);
        get_gyr_raw(&sensor_upper, &temp_gyr[0], &temp_gyr[1], &temp_gyr[2]);

        data_array[1]  = (int32_t)temp_acc[0];          // Sensor 1 - Acc X
        data_array[2]  = (int32_t)temp_acc[1];          // Sensor 1 - Acc Y
        data_array[3]  = (int32_t)temp_acc[2];          // Sensor 1 - Acc Z
        data_array[4]  = (int32_t)temp_gyr[0];          // Sensor 1 - Gyr X
        data_array[5]  = (int32_t)temp_gyr[1];          // Sensor 1 - Gyr Y
        data_array[6]  = (int32_t)temp_gyr[2];          // Sensor 1 - Gyr Z

        clock_gettime(CLOCK_MONOTONIC_RAW, &data_timer);
        data_array[7] = get_microseconds_delta(&old_time2, &data_timer);    // Sensor 2 - Time
        old_time2 = data_timer;

        get_acc_raw(&sensor_lower, &temp_acc[0], &temp_acc[1], &temp_acc[2]);
        get_gyr_raw(&sensor_lower, &temp_gyr[0], &temp_gyr[1], &temp_gyr[2]);

        data_array[8]  = (int32_t)temp_acc[0];          // Sensor 2 - Acc X
        data_array[9]  = (int32_t)temp_acc[1];          // Sensor 2 - Acc Y
        data_array[10] = (int32_t)temp_acc[2];          // Sensor 2 - Acc Z
        data_array[11] = (int32_t)temp_gyr[0];          // Sensor 2 - Gyr X
        data_array[12] = (int32_t)temp_gyr[1];          // Sensor 2 - Gyr Y
        data_array[13] = (int32_t)temp_gyr[2];          // Sensor 2 - Gyr Z

        // -------------------------------------------------
        // SENDING DATA
        // -------------------------------------------------

        int result = sendto(sock, data_array, NUM_DATA * sizeof(int), 0, (struct sockaddr *)&receiver_address, sizeof(receiver_address));

        if (result < 0)
        {
            printf("ERROR: Sending data failed!\n");
            data_streaming = 0;
            return -1;
        }

        if (!data_streaming)
        {
            printf("\nSending data to %s:%d at %i Hz.\n", inet_ntoa(receiver_address.sin_addr), ntohs(receiver_address.sin_port), (int)(UPDATE_RATE));
            data_streaming = 1;
        }

        // -------------------------------------------------
        // SLEEPING TO KEEP UPDATE RATE
        // -------------------------------------------------

        clock_gettime(CLOCK_MONOTONIC_RAW, &toc);
        double elapsed_time = (toc.tv_sec - tic.tv_sec) + (double)(toc.tv_nsec - tic.tv_nsec) / 1000000000.0;

        sleep_time_avg += (elapsed_time - sleep_time_avg) / (i + 1);
        sleep_time_var += (elapsed_time - sleep_time_avg) * (elapsed_time - sleep_time_avg) / (i + 1);
        i++;

        double adjustment = sqrt(sleep_time_var);

        sleep_time.tv_nsec = (long)((UPDATE_TIME - elapsed_time - adjustment) * 1000000000.0);

        if (i >= (UPDATE_RATE * 1.25))
        {
            sleep_time_avg = 0.0;
            sleep_time_var = 0.0;
            i = 0;
        }

        if (sleep_time.tv_nsec > 0)
            nanosleep(&sleep_time, NULL);

        // -------------------------------------------------
        // DEBUGGING PRINTS
        // -------------------------------------------------

        // // PRINT SENT DATA
        // for (int i = 0; i < NUM_DATA; i++)
        // {
        //     printf("%i | ", data_array[i]);
        // }
        // printf("\n");

        // // PRINT TOTAL ELAPSED TIME
        // clock_gettime(CLOCK_MONOTONIC_RAW, &data_timer);
        // double total_elapsed = (data_timer.tv_sec - tic.tv_sec) + (double)(data_timer.tv_nsec - tic.tv_nsec) / 1000000000.0;

        // printf("Total elapsed time: %f\n", total_elapsed);
    }

    // -------------------------------------------------
    // CLOSE I2C DEVICE
    // -------------------------------------------------

    close(sensor_upper.i2c_fd);
    close(sensor_lower.i2c_fd);

    printf("\n-------- SCRIPT ENDED SUCCESSFULLY --------\n");

    return 0;
}
