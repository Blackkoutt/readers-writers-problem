#ifndef VALIDATE_H
#define VALIDATE_H

#define MAX_SLEEP_SEC 3 //Maksymalny czas usypiania wątków (w sekundach)
//#define MAX_SLEEP_TIME_MSEC 1000 //1s = 1000 ms

extern volatile int readers_count;
extern volatile int writers_count;
extern volatile bool flag_info;

void initArgs(int argc, char* argv[]);
int random_sleep_time();

#endif