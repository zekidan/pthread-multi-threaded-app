/*
 * planet.c
 *
 *  Created on: May 26, 2019
 *      Author: zekidan
 */

#include <stdio.h>
#include<stdbool.h>
#include <allegro/keyboard.h>
#include "planet.h"
#include<time.h>

planet planets[MAX_PLANETS];		// container for planet descriptors
orbit orbits[MAX_PLANETS];
uint8_t n_planets = 0; 		// current number of planets in the space
pthread_mutex_t planets_mtx = PTHREAD_MUTEX_INITIALIZER; // mutex to protect planets

const float TWO_PI = 2*M_PI;
const double STEP_TIME = (double)PRD_PLANETS/1000;

/* Initialize the descriptor of an planet right after add */
void init_planet_param( planet *const p, int tid) {

	p->alive = true;
	p->tid = (unsigned int)tid;

	switch(p->id){
	case 0:
		p->speed=mercurySpeed;
		p->orbit=mercuryOrbit;
		p->size=mercurySize;
		break;

	case 1:
		p->speed=venusSpeed;
		p->orbit=venusOrbit;
		p->size=venusSize;
		break;

	case 2:
		p->speed=earthSpeed;
		p->orbit=earthOrbit;
		p->size=earthSize;
		break;

	case 3:
		p->speed=marsSpeed;
		p->orbit=marsOrbit;
		p->size=marsSize;
		break;

	case 4:
		p->speed=jupiterSpeed;
		p->orbit=jupiterOrbit;
		p->size=jupiterSize;
		break;

	case 5:
		p->speed=saturnSpeed;
		p->orbit=saturnOrbit;
		p->size=saturnSize;
		break;

	case 6:

		p->speed=uranusSpeed;
		p->orbit=uranusOrbit;
		p->size=uranusSize;
		break;
	case 7:
		p->speed=neptuneSpeed;
		p->orbit=neptuneOrbit;
		p->size=neptuneSize;
		break;

	case 8:
		p->speed=plutoSpeed;
		p->orbit=plutoOrbit;
		p->size=plutoSize;
		break;
	}
	p->curr_time=0;
	p->pos.x=0;	p->pos.y=0;	p->pos.angle=0;
	}

void init_planet(planet *const p,int tid){
	init_planet_param(p,tid);
}
void set_orbit_param(orbit *const o, int i){
	o->diametrx= (double)(baseSize + incrX * pow((double)(i+1), 1.1));
	o->diametry =(double)(baseSize + incrY * pow((double)(i+1), 1.1));
	        //ellipse(0, 0, diameterX, diameterY);
	}
void init_orbits(){
	for(int i=0;i<9;i++){
		set_orbit_param(&orbits[i],i);
	}
}


//move a planet forward in an elliptical path
void *move_planet(void *arg ){
	planet *const p=(planet *)arg;
	double t = p->curr_time  * p->speed;
	double degree = fmod(t,360.0);
	double radiusX = (double)(baseSize + incrX * pow((double)(p->orbit), 1.1)) / 2;
	double radiusY = (double)(baseSize + incrY * pow((double)(p->orbit),1.1)) / 2;
	p->pos.x = radiusX * cos(degree);
	p->pos.y = radiusY * sin(degree);
	p->curr_time += STEP_TIME;
	p->pos.angle =degree;

}


/* Allocates a previously unused planet structure and returns its index.
*  If none is free, return MAX_PLANETS. */
unsigned int allocate_planet_id(void) {

	unsigned int i = 0;

	pthread_mutex_lock(&planets_mtx);

	if (n_planets == MAX_PLANETS) {
		pthread_mutex_unlock(&planets_mtx);
		return MAX_PLANETS;
	}

	while (planets[i].alive)
		++i;

	planets[i].alive = true;
	++n_planets;

	pthread_mutex_unlock(&planets_mtx);
	return i;
}



/* Deallocates the planet struct of a terminated planet. */
void deallocate_planet_id(unsigned int id) {

	pthread_mutex_lock(&planets_mtx);
	--n_planets;
	planets[id].alive = false;
	pthread_mutex_unlock(&planets_mtx);
}

/* Add a new planet to orbit in the solar space.
*  Returns 0 if success, -1 if the solar space is full, -2 if thread pool is full. */
int add_planet(void) {

	unsigned int p_id;		// id of the new planet (index in planets array)
	int t_id;				// if the new thread

	p_id = allocate_planet_id();
	if (p_id == MAX_PLANETS)
		return -1;

	pthread_mutex_lock(&planets[p_id].mtx);	// prevents the planet from running before initialization

	t_id = start_thread(move_planet, &planets[p_id], SCHED_FIFO, PRD_PLANETS, DL_PLANETS, PRIO_PLANETS);
	if (t_id < 0) {
		pthread_mutex_unlock(&planets[p_id].mtx);
		deallocate_planet_id(p_id);
		return -2;
	} else {
		init_planet(&planets[p_id], t_id);
		pthread_mutex_unlock(&planets[p_id].mtx);
		printf("Created planet with id %d and tid #%d\n", p_id, t_id);
	}
	pthread_mutex_unlock(&planets[p_id].mtx);
	return 0;
}


/* removes the planet with the specified id.
*  Returns 0 if success, -1 if it was already removed. */
int remove_planet(unsigned int i) {

	pthread_mutex_lock(&planets[i].mtx);
	if (!planets[i].alive) {
		pthread_mutex_unlock(&planets[i].mtx);
		return -1;
	}
	stop_thread(planets[i].tid);
	pthread_mutex_unlock(&planets[i].mtx);

	deallocate_planet_id(i);

	return 0;
}


/* adds the specified number of planets. If there is not enough room for all,
*  create as many as possible.
*  Returns the number of successfully added planets. */
unsigned int add_planets(unsigned int n) {

	for (int i = 0; i < n; ++i) {
		if (add_planet() < 0)
			return i;
	}
	return n;
}


/* removes all the planets from the space. */
void remove_planets(void) {

	for (int i = 0; i < MAX_PLANETS; ++i)
		remove_planet(i);
}


/* Return the id of a planet given a position, within a certain degree of approximation.
** */
int get_planet_id_by_pos(int x, int y) {

	for (int i = 0; i < MAX_PLANETS; ++i) {
		if (planets[i].alive && hypot(x - (SPACE_WIDTH/2-planets[i].size/2 + planets[i].pos.x), y - (SPACE_HEIGHT/2-planets[i].size/2 + planets[i].pos.x)) < 20)
			return i;
	}
	return -1;
}

int get_planet_id_by_orbit_pos(int x , int y){
	double s;
	if(0.9<=(s=(pow((x-SPACE_WIDTH/2)/(0.5*orbits[0].diametrx),2)) +
			pow((y-SPACE_HEIGHT/2)/(0.5*orbits[0].diametry),2))&&(s<=1))
		return 0;
	else if(0.9<=(s=(pow((x-SPACE_WIDTH/2)/(0.5*orbits[1].diametrx),2)) +
			pow((y-SPACE_HEIGHT/2)/(0.5*orbits[1].diametry),2))&&(s<=1))
		return 1;
	else if(0.9<=(s=(pow((x-SPACE_WIDTH/2)/(0.5*orbits[2].diametrx),2)) +
				pow((y-SPACE_HEIGHT/2)/(0.5*orbits[2].diametry),2))&&(s<=1))
		return 2;
	else if(0.9<=(s=(pow((x-SPACE_WIDTH/2)/(0.5*orbits[3].diametrx),2)) +
				pow((y-SPACE_HEIGHT/2)/(0.5*orbits[3].diametry),2))&&(s<=1))
		return 3;
	else if(0.9<=(s=(pow((x-SPACE_WIDTH/2)/(0.5*orbits[4].diametrx),2)) +
				pow((y-SPACE_HEIGHT/2)/(0.5*orbits[4].diametry),2))&&(s<=1))
		return 4;
	else if(0.9<=(s=(pow((x-SPACE_WIDTH/2)/(0.5*orbits[5].diametrx),2)) +
				pow((y-SPACE_HEIGHT/2)/(0.5*orbits[5].diametry),2))&&(s<=1))
		return 5;
	else if(0.9<=(s=(pow((x-SPACE_WIDTH/2)/(0.5*orbits[6].diametrx),2)) +
				pow((y-SPACE_HEIGHT/2)/(0.5*orbits[6].diametry),2))&&(s<=1))
		return 6;
	else if(0.9<=(s=(pow((x-SPACE_WIDTH/2)/(0.5*orbits[7].diametrx),2)) +
				pow((y-SPACE_HEIGHT/2)/(0.5*orbits[7].diametry),2))&&(s<=1))
		return 7;
	else if(0.9<=(s=(pow((x-SPACE_WIDTH/2)/(0.5*orbits[8].diametrx),2)) +
				pow((y-SPACE_HEIGHT/2)/(0.5*orbits[8].diametry),2))&&(s<=1))
		return 8;
	else return -1;
}
/* Initialize data structures to handle planet adding/removeing */
void init_planets_manager(void) {

	pthread_mutex_lock(&planets_mtx);

	for (int i = 0; i < MAX_PLANETS; ++i) {
		pthread_mutex_init(&planets[i].mtx, NULL);
		planets[i].alive = false;
		planets[i].id = i;
	}

	init_orbits();

	pthread_mutex_unlock(&planets_mtx);
}



