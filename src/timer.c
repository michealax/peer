//
// Created by syang on 12/2/17.
//
#include "timer.h"
#include "stdio.h"

double estimated_rtt = 1000;
double dev_rtt = 0;

void timer_start(struct timeval *starter) {
    gettimeofday(starter, NULL);
}

int timer_now(struct timeval *starter, struct timeval *now) {
    long diff_sec = now->tv_sec-starter->tv_sec;
    long diff_usec = now->tv_usec-starter->tv_usec;
    int ret = diff_sec*1000+diff_usec/1000; // return the ms
    return ret;
}

void update(int sample_rtt) {
    double abs = sample_rtt>estimated_rtt?sample_rtt-estimated_rtt:estimated_rtt-sample_rtt;
    estimated_rtt = (1-ALPHA)*estimated_rtt+ALPHA*sample_rtt;
    dev_rtt = (1-BETA)*dev_rtt+BETA*abs;
}

int get_timeout_interval() {
    double timeout_interval = estimated_rtt+4*dev_rtt;
    return (int)timeout_interval;
}