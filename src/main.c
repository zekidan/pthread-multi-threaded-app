/*
 * main.c
 *
 *  Created on: May 26, 2019
 *      Author: zekidan
 */
#include <allegro.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "planet.h"
//#include "field.h"
#include "multimedia.h"
#include "rt_thread.h"


int main(int argc, char **argv) {

    printf("Loading...\n");

    /* Initialize data structures to support the creation of real-time threads */
    init_rt_thread_manager();
    printf("Thread manager successfully initialized.\n");

    /* Initialize data structures needed to spawn/kill planets */
    init_planets_manager();
    printf("Planet manager successfully initialized.\n");

    /* Spawn a few planets [default 9 planets]*/
    unsigned int spawned = spawn_planets(MAX_PLANETS);
    printf("Successfully spawned %d planets.\n", spawned);

    /* Initialize graphics, mouse and keyboard */
    if (init_multimedia())
        return 1;

    //Threads execution statistics Report Labels
    printf("\nThread Response Time(in μ sec.) and Deadline Statistics per every 10K iterations\n");
    printf("-----------------------------------------------------------------------\n");
    printf("Thread\t\tMin.\t\tMax.\t\tAvg.\t\tDl_misses\n");

    /* Wait for the end of simulation (ESC key pressed by user) */
    wait_for_termination();

    /* Remove all the planets from the space*/
    kill_planets();
    printf("planets removed.\n");

    /* Halt graphics, mouse and keyboard */
    stop_multimedia();

    return 0;
}
END_OF_MAIN()



