/*
 * planets.h
 *
 *  Created on: May 26, 2019
 *      Author: zekidan
 */

#ifndef CONF_PLANETS_H_
#define CONF_PLANETS_H_

// Information from http://www.enchantedlearning.com/subjects/astronomy/planets/

//speed. 1/year
#define mercurySpeed	365.26/87.96
#define venusSpeed		365.26/224.68
#define earthSpeed 		1
#define marsSpeed 		365.26/686.98
#define jupiterSpeed 	1/11.862
#define saturnSpeed 	1/29.456
#define uranusSpeed 	1/84.07
#define neptuneSpeed 	1/164.81
#define plutoSpeed 		1/247.7

//size
#define  mercurySize	8
#define  venusSize		15
#define  earthSize		15
#define  marsSize 		10
#define  jupiterSize	30
#define  saturnSize 	22
#define  uranusSize 	15
#define  neptuneSize 	15
#define	 plutoSize 		8

// Orbit
#define  mercuryOrbit 	1
#define  venusOrbit		2
#define  earthOrbit		3
#define  marsOrbit		4
#define  jupiterOrbit	5
#define  saturnOrbit 	6
#define	 uranusOrbit    7
#define  neptuneOrbit	8
#define	 plutoOrbit		9

// These control the shape of the orbits
#define  baseSize		50// Size of The Sun
#define  incrX 			60
#define  incrY 			40


#define MAX_PLANETS 	9

#define PRD_PLANETS		20	// Task period in milliseconds
#define DL_PLANETS		20 // Task deadline
#define PRIO_PLANETS	99	// Task priority

//#define STEP_TIME		(double)(PRD_PLANETS/1000)



#endif /* CONF_PLANETS_H_ */
