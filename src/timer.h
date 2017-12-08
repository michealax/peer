//
// Created by syang on 12/2/17.
//

#ifndef PEER_TIMER_H
#define PEER_TIMER_H

#include <sys/time.h>

#define ALPHA 0.125
#define BETA 0.25

void timer_start(struct timeval*);
int timer_now(struct timeval*, struct timeval*);

#endif //PEER_TIMER_H
