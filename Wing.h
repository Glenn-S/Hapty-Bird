/**
 * Filename: Wing.h
 * 
 * Authors: Brandon Seiu, Glenn Skelton
 */

#pragma once
#ifndef WING_H
#define WING_H

#include "chai3d.h"
using namespace chai3d;
using namespace std;

constexpr double EPSILON = 0.05;

class Wing{
// public functions
public:
	Wing(cVector3d a_initialPos, 
		 double a_area, 
		 double a_dragCo, 
		 double a_maxBound, 
		 double a_minBound);
	~Wing();

	void calculateLift(double p, cVector3d vel);


// public member variables
public:
	cVector3d m_initialPos; // initial position that the wing is set to
	double m_currentLength; // the current length of the wing being outstretched

	double m_area; // the suraface area of the wing in m^2
	double m_currentAreaRatio; // ratio of the wing area given how out stretched the wing is
	double m_dragCo; // coeeficient of drag on wing

	double m_maxBound; // maximum distance the wing can be outstretched
	double m_minBound; // minimum distance the wing can be oustretched

	cVector3d m_F_net; // the total net force of one wing (used for updating haptic device)
	cVector3d m_F_up; // force accumulator for the graphics portion of calculations (no controller forces)

};

#endif