#ifndef MULTIMEDIA_H
#define MULTIMEDIA_H

#include "planet.h"
#include "conf/field.h"
#include "conf/multimedia.h"
#include "rt_thread.h"

extern unsigned int graphics_tid, keyboard_tid, mouse_tid;     // threads IDs

unsigned int init_multimedia(void);

void stop_multimedia(void);

void wait_for_termination(void);

#endif
