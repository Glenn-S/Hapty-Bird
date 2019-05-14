/**
 * Filename: Wing.cpp
 *
 * Authors: Brandon Seiu, Glenn Skelton
 */

#include "Wing.h"
#include "chai3d.h"

using namespace chai3d;

/////////////////////////////////////////// CONSTRUCTORS ///////////////////////////////////////////////////
Wing::Wing(cVector3d a_initialPos, double a_area, double a_dragCo, double a_maxBound, double a_minBound) {
	m_initialPos = a_initialPos;
	m_currentLength = m_initialPos.length();
	m_area = a_area;
	m_dragCo = a_dragCo;
	m_maxBound = a_maxBound;
	m_minBound = a_minBound;
	m_F_net = cVector3d(0, 0, 0);
	m_F_up = cVector3d(0, 0, 0);
}

Wing::~Wing() {}





////////////////////////////////////////////// FUNCTIONS ///////////////////////////////////////////////////
/**
 * To calculate the lift produced by a wing given the wings velocity downwards
 */
void Wing::calculateLift(double p, cVector3d vel) {

	cVector3d Flift(0, 0, 0);

	double wingArea = (m_currentLength - m_initialPos.length() + -m_minBound) / (m_maxBound + -m_minBound); // shift to be in positive range
	
	wingArea = wingArea < 0.95 ? 0.95 : wingArea; // make sure it is not negative
	wingArea = wingArea > 1.0 ? 1.0 : wingArea; // make sure wing does not extend greater than max area
	m_currentAreaRatio = wingArea;

	wingArea = m_area * m_currentAreaRatio; // *wingArea
	
	if (vel.z() < -EPSILON) { //
		double mag = vel.z();
		Flift = ((p * m_dragCo * wingArea) / 2) * ((mag*mag) * cVector3d(0, 0, 1.0));
		
		m_F_up += Flift; // all forces not including the device bounding
		m_F_net += Flift; // all forces for device specifically
	}

}