/*
 * planet.h
 *
 *  Created on: May 26, 2019
 *      Author: zekidan
 */

#ifndef PLANET_H_
#define PLANET_H_

#include <math.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#include "conf/planets.h"
#include "conf/space.h"
#include "rt_thread.h"

extern const float TWO_PI;

typedef struct position {
	double 			x;		// [pixel] x coordinate
	double 			y;		// [pixel] y coordinate
	float 			angle;		// [radians] angle in [0, 2*pi)
} position;


typedef struct planet {
	bool 			alive;		// is a planet allocated? is the data meaningful?
	unsigned int 	id;		// thread identifier
	unsigned int 	tid;		// thread identifier
	position 		pos;		// planet position

	double speed;
	unsigned short int size;
	unsigned short int orbit;
	double curr_time;//Current time of the planet in milliseconds

	pthread_mutex_t 	mtx;		// mutex to protect the struct
} planet;

typedef struct orbit {
	double diametrx;
	double diametry;
} orbit;

extern planet planets[MAX_PLANETS];
extern orbit orbits[MAX_PLANETS];
extern uint8_t n_planets;
extern pthread_mutex_t planets_mtx;


void init_planets_manager();
int add_planet(void);
unsigned int add_planets(unsigned int n_planets);
int remove_planet(unsigned int i);
void remove_planets(void);
int get_planet_id_by_pos(int x, int y);
int get_planet_id_by_orbit_pos(int x , int y);


#endif/* PLANET_H_ */
