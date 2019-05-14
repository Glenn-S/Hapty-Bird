/**
 * Filename: Body.cpp
 * 
 * Authors: Brandon Seiu, Glenn Skelton
 */

#include "Wing.h"
#include "Body.h"
#include "chai3d.h"

using namespace chai3d;
using namespace std;

/////////////////////////////////////////// CONSTRUCTORS ///////////////////////////////////////////////////
Body::Body(cVector3d a_velocity,
		   cVector3d a_wingNaturalPos,
		   double a_mass, 
		   double a_area, 
		   double a_drag, 
		   double a_maxBound,
		   double a_minBound) {
	m_mass = a_mass > 0.0 ? a_mass : 0.0; // if 0, not initialized
	m_velocity = a_velocity;
	m_leftWing = new Wing(-a_wingNaturalPos, a_area, a_drag, a_maxBound, a_minBound);
	m_rightWing = new Wing(a_wingNaturalPos, a_area, a_drag, a_maxBound, a_minBound);
}


Body::~Body() {
	delete m_leftWing;
	delete m_rightWing;
}

////////////////////////////////////////////// FUNCTIONS ///////////////////////////////////////////////////

/**
 * To calculate the bounds of the device and add to the device force accumulator
 */
void Body::calculateBounds(Wing* a_wing, cVector3d a_devicePos) {
	double k = 4000;
	double wingDistance = 0.0;
	double wingNaturalDistance;
	double wingMaxLength = 0.0;
	double wingMinLength = 0.0;
	cVector3d globalNaturalWingPos = this->body->getLocalPos() + a_wing->m_initialPos;
	cVector3d force(0, 0, 0);

	// get max and min length magnitudes for bounding
	wingNaturalDistance = cDistance(this->body->getLocalPos(), globalNaturalWingPos);
	wingMaxLength = wingNaturalDistance + a_wing->m_maxBound;
	wingMinLength = wingNaturalDistance + a_wing->m_minBound;

	// calculate vector of wing pos according to device
	cVector3d globalWingPos = globalNaturalWingPos + a_devicePos;
	
	wingDistance = cDistance(this->body->getLocalPos(), globalWingPos); // magnitude of device global position
	globalWingPos = globalWingPos - this->body->getLocalPos();

	a_wing->m_currentLength = globalWingPos.length();
	globalWingPos.normalize(); // update vector normalized vector

	if (wingDistance < wingMinLength) {
		//update force
		force += -k * (wingDistance - wingMinLength) * globalWingPos;
	} else if (wingDistance > wingMaxLength) {
		//update force
		force += -k * (wingDistance - wingMaxLength) * globalWingPos;
	}

	double forwardBound = -0.005;
	double backBound = 0.005;
	double topBound = 0.037;
	double bottomBound = -0.037;

	//check bounds of avatar relative to device in x direction and z direction
	if (a_devicePos.x() < forwardBound) {
		force += -k * (a_devicePos.x() - forwardBound) * cVector3d(1,0,0);
	}
	else if (a_devicePos.x() > backBound) {
		force += -k * (a_devicePos.x() - backBound) * cVector3d(1,0,0);
	}
	
	if (a_devicePos.z() > topBound) {
		force += -(k / 2) * (a_devicePos.z() - topBound) * cVector3d(0,0,1);
	}
	else if (a_devicePos.z() < bottomBound) {
		force += -(k / 2) * (a_devicePos.z() - bottomBound) * cVector3d(0,0,1);
	}

	a_wing->m_F_net = a_wing->m_F_net + force; // update net force of wing


}


/**
 * To calculate basic force feed back for joystick effect.
 */
void Body::controllerStartup(cVector3d a_devicePosRight,
							 cVector3d a_devicePosLeft,
							 double a_t,
							 double a_totalTime) {
	this->m_rightWing->m_F_net = cVector3d(0, 0, 0);
	this->m_leftWing->m_F_net = cVector3d(0, 0, 0);

	// calculate device boundaries
	//cout << a_devicePosRight << endl;
	this->calculateBounds(this->m_rightWing, a_devicePosRight); // must happen first
	//cout << this->m_rightWing->m_F_net << endl;
	this->m_rightWing->m_F_net = this->m_rightWing->m_F_net * (a_t / a_totalTime);
	this->calculateBounds(this->m_leftWing, a_devicePosLeft); // must happen first
	this->m_leftWing->m_F_net = this->m_leftWing->m_F_net * (a_t / a_totalTime);

}


/**
 * To calculate the forces created by the flapping wings
 */
void Body::updateForces(cVector3d a_devicePosRight, 
						cVector3d a_devicePosLeft, 
						cVector3d a_deviceVelRight, 
						cVector3d a_deviceVelLeft,
						double a_airDensity) {
	// zero forces out before calculations
	this->m_rightWing->m_F_up = cVector3d(0, 0, 0);
	this->m_rightWing->m_F_net = cVector3d(0, 0, 0);
	this->m_leftWing->m_F_up = cVector3d(0, 0, 0);
	this->m_leftWing->m_F_net = cVector3d(0, 0, 0);

	// calculate device boundaries
	this->calculateBounds(this->m_rightWing, a_devicePosRight); // must happen first
	this->calculateBounds(this->m_leftWing, a_devicePosLeft); // must happen first

	// calculate wing forces
	this->m_rightWing->calculateLift(a_airDensity, a_deviceVelRight);
	this->m_leftWing->calculateLift(a_airDensity, a_deviceVelLeft);


}

/**
 * To calculate the turublance force and add it to the force accumulator.
 */
void Body::applyTurbulence(double a_currentTime, unsigned int a_periodRange, unsigned int a_amplitudeRange) {
	cVector3d turbulence;
	
	double period = (rand() % a_periodRange + 1);
	double amplitude = (rand() % a_amplitudeRange + 1);
	turbulence = (amplitude * sin(period * M_PI * a_currentTime)) * cVector3d(0.0, 0.0, 1.0);

	// update wing net forces
	this->m_rightWing->m_F_net += turbulence;
	this->m_leftWing->m_F_net += turbulence;
}


/**
 * To detect a collision with the bird and a pipe.
 */
bool Body::collisionDetector(cShapeCylinder* a_pipe) {
	double xLength;
	double zLength;
	double minxDist;
	double minzDist;
	double cornerDist;
	double avatarRadius = 0.025;

	cVector3d adjustedPipeCenter = a_pipe->getLocalPos() + cVector3d(0, 0, a_pipe->getHeight() / 2);

	xLength = abs(this->body->getLocalPos().x() - adjustedPipeCenter.x());
	zLength = abs(this->body->getLocalPos().z() - adjustedPipeCenter.z());

	minxDist = a_pipe->getBaseRadius() + avatarRadius;
	minzDist = a_pipe->getHeight() / 2 + avatarRadius;

	if ((xLength > minxDist) || (zLength > minzDist)) return false;
	else if ((xLength <= a_pipe->getBaseRadius() / 2) || ((zLength <= a_pipe->getHeight() / 2))) return true;
	else {
		cornerDist = pow(xLength - a_pipe->getBaseRadius(), 2) + pow(zLength - a_pipe->getHeight() / 2, 2);
		if (cornerDist <= avatarRadius * avatarRadius) return true;
	}
	return false;
}