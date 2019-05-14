/**
 * Filename: Body.h
 *
 * Authors: Brandon Seiu, Glenn Skelton
 */

#pragma once
#ifndef BODY_H
#define BODY_H

#include "Wing.h"
#include "chai3d.h"

using namespace chai3d;
using namespace std;

class Body {
// Public functions
public:
	Body(cVector3d a_velocity, // initial velocity
		 cVector3d a_wingNaturalPos, // resting position of the wing
		 double a_mass, // mass of the wing
		 double a_area, // total area of the wing in m^2
		 double a_drag, // drag coefficient 
		 double a_maxLength, // maximium length that the wing can be outstretched
		 double a_minLength); // minimum length that the wing can be outstretched
	~Body();

	void calculateBounds(Wing* a_wing, cVector3d a_devicePos);
	void Body::controllerStartup(cVector3d a_devicePosRight,
								 cVector3d a_devicePosLeft,
								 double a_t,
								 double a_totalTime);
	void updateForces(cVector3d a_devicePosLeft, 
					  cVector3d a_devicePosRight, 
					  cVector3d a_deviceVelLeft, 
					  cVector3d a_deviceVelRight,
					  double a_airDensity = 0.038);
	void applyTurbulence(double a_currentTime, unsigned int a_periodRange, unsigned int a_amplitudeRange);

	bool collisionDetector(cShapeCylinder* a_pipe);

// Public varibles
public:
	cMultiMesh *body; // pointer to the mesh of the body
	cMultiMesh *leftWing; // pointer to the mesh of the left wing for the body
	cMultiMesh *rightWing; // pointer to the mesh of the right wing for the body
	cVector3d m_velocity; // current velocity of bird

	double m_mass; // mass of the bird
	
	Wing *m_leftWing; // pointer to the left wing storage class
	Wing *m_rightWing; // pointer to the right wing storage class

};

#endif