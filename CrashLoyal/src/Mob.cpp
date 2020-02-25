#include "Mob.h"

#include <memory>
#include <limits>
#include <stdlib.h>
#include <stdio.h>
#include "Building.h"
#include "Waypoint.h"
#include "GameState.h"
#include "Point.h"

int Mob::previousUUID;

Mob::Mob() 
	: pos(-10000.f,-10000.f)
	, nextWaypoint(NULL)
	, targetPosition(new Point)
	, state(MobState::Moving)
	, uuid(Mob::previousUUID + 1)
	, attackingNorth(true)
	, health(-1)
	, targetLocked(false)
	, target(NULL)
	, lastAttackTime(0)
{
	Mob::previousUUID += 1;
}

void Mob::Init(const Point& pos, bool attackingNorth)
{
	health = GetMaxHealth();
	this->pos = pos;
	this->attackingNorth = attackingNorth;
	findClosestWaypoint();
}

std::shared_ptr<Point> Mob::getPosition() {
	return std::make_shared<Point>(this->pos);
}

bool Mob::findClosestWaypoint() {
	std::shared_ptr<Waypoint> closestWP = GameState::waypoints[0];
	float smallestDist = std::numeric_limits<float>::max();

	for (std::shared_ptr<Waypoint> wp : GameState::waypoints) {
		//std::shared_ptr<Waypoint> wp = GameState::waypoints[i];
		// Filter out any waypoints that are "behind" us (behind is relative to attack dir
		// Remember y=0 is in the top left
		if (attackingNorth && wp->pos.y > this->pos.y) {
			continue;
		}
		else if ((!attackingNorth) && wp->pos.y < this->pos.y) {
			continue;
		}

		float dist = this->pos.dist(wp->pos);
		if (dist < smallestDist) {
			smallestDist = dist;
			closestWP = wp;
		}
	}
	std::shared_ptr<Point> newTarget = std::shared_ptr<Point>(new Point);
	this->targetPosition->x = closestWP->pos.x;
	this->targetPosition->y = closestWP->pos.y;
	this->nextWaypoint = closestWP;
	
	return true;
}

void Mob::moveTowards(std::shared_ptr<Point> moveTarget, double elapsedTime) {
	Point movementVector;
	movementVector.x = moveTarget->x - this->pos.x;
	movementVector.y = moveTarget->y - this->pos.y;
	movementVector.normalize();
	movementVector *= (float)this->GetSpeed();
	movementVector *= (float)elapsedTime;
	pos += movementVector;
}


void Mob::findNewTarget() {
	// Find a new valid target to move towards and update this mob
	// to start pathing towards it

	if (!findAndSetAttackableMob()) { findClosestWaypoint(); }
}

// Have this mob start aiming towards the provided target
// TODO: impliment true pathfinding here
void Mob::updateMoveTarget(std::shared_ptr<Point> target) {
	this->targetPosition->x = target->x;
	this->targetPosition->y = target->y;
}

void Mob::updateMoveTarget(Point target) {
	this->targetPosition->x = target.x;
	this->targetPosition->y = target.y;
}


// Movement related
//////////////////////////////////
// Combat related

int Mob::attack(int dmg) {
	this->health -= dmg;
	return health;
}

bool Mob::findAndSetAttackableMob() {
	// Find an attackable target that's in the same quardrant as this Mob
	// If a target is found, this function returns true
	// If a target is found then this Mob is updated to start attacking it
	for (std::shared_ptr<Mob> otherMob : GameState::mobs) {
		if (otherMob->attackingNorth == this->attackingNorth) { continue; }

		bool imLeft    = this->pos.x     < (SCREEN_WIDTH / 2);
		bool otherLeft = otherMob->pos.x < (SCREEN_WIDTH / 2);

		bool imTop    = this->pos.y     < (SCREEN_HEIGHT / 2);
		bool otherTop = otherMob->pos.y < (SCREEN_HEIGHT / 2);
		if ((imLeft == otherLeft) && (imTop == otherTop)) {
			// If we're in the same quardrant as the otherMob
			// Mark it as the new target
			this->setAttackTarget(otherMob);
			return true;
		}
	}
	return false;
}

// TODO Move this somewhere better like a utility class
int randomNumber(int minValue, int maxValue) {
	// Returns a random number between [min, max). Min is inclusive, max is not.
	return (rand() % maxValue) + minValue;
}

void Mob::setAttackTarget(std::shared_ptr<Attackable> newTarget) {
	this->state = MobState::Attacking;
	target = newTarget;
}

bool Mob::targetInRange() {
	float range = this->GetSize(); // TODO: change this for ranged units
	float totalSize = range + target->GetSize();
	return this->pos.insideOf(*(target->getPosition()), totalSize);
}
// Combat related
////////////////////////////////////////////////////////////
// Collisions

// PROJECT 3: 
//  1) return a vector of mobs that we're colliding with
//  2) handle collision with towers & river 

void Mob::processBuildingCollision(double elapsedTime) {
	for (std::shared_ptr<Building> building : GameState::buildings) {
		float xDist = this->getPosition()->x - building->getPosition()->x;
		float yDist = this->getPosition()->y - building->getPosition()->y;
		float touchDist = (this->GetSize() + building->GetSize()) / 2.0;
		if (abs(xDist) >= touchDist || abs(yDist) >= touchDist) {
			//no collision, no need to move
			continue;
		}
		Point movementVector;
		movementVector.x = 0;
		movementVector.y = 0;
		if (touchDist > xDist&& xDist >= 0) {
			movementVector.x = touchDist - xDist;
		}
		else if (touchDist > -xDist && xDist < 0) {
			movementVector.x = -touchDist - xDist;
		}
		if (touchDist > yDist&& yDist > 0) {
			movementVector.y = touchDist - yDist;
		}
		else if (touchDist > -yDist && yDist < 0) {
			movementVector.y = -touchDist - yDist;
		}

		movementVector.normalize();
		movementVector *= elapsedTime;

		this->pos += movementVector; // we always move
	}
}

void Mob::processRiverCollision(double elapsedTime) {
	if ((RIVER_TOP_Y >= this->getPosition()->y + (this->GetSize() / 2.0))
		|| (RIVER_BOT_Y <= this->getPosition()->y - (this->GetSize() / 2.0))) {
		//we're outside the vertical river strip. Do nothing.
		return;
	}
	Point movementVector;
	movementVector.x = 0;
	movementVector.y = 0;
	if (this->getPosition()->x - (this->GetSize() / 2.0) < LEFT_BRIDGE_CENTER_X - (BRIDGE_WIDTH / 2.0)) {
		//we are in the left river segment
		if (this->getPosition()->x - (this->GetSize() / 2.0) < ((LEFT_BRIDGE_CENTER_X - (BRIDGE_WIDTH / 2.0)) / 2.0)) {
			//we are really far left in the river. just move right.
			movementVector.x = 1.0;
		}
		else {
			movementVector.x = this->getPosition()->x - (this->GetSize() / 2.0) - ((LEFT_BRIDGE_CENTER_X - (BRIDGE_WIDTH / 2.0)) / 2.0);
		}
	} else if (this->getPosition()->x + (this->GetSize() / 2.0) > LEFT_BRIDGE_CENTER_X + (BRIDGE_WIDTH / 2.0)
		&& this->getPosition()->x - (this->GetSize() / 2.0) <= RIGHT_BRIDGE_CENTER_X - (BRIDGE_WIDTH / 2.0)) {
		//we are in the center river
		//we need to always have horizontal movement, so no widths here, just distance from center
		movementVector.x = this->getPosition()->x - GAME_GRID_WIDTH / 2.0;
	} else if (this->getPosition()->x + (this->GetSize() / 2.0) > RIGHT_BRIDGE_CENTER_X + (BRIDGE_WIDTH / 2.0)) {
		//we are in the right river segment
		if (this->getPosition()->x + (this->GetSize() / 2.0) >= ((RIGHT_BRIDGE_CENTER_X + (BRIDGE_WIDTH / 2.0) + GAME_GRID_WIDTH) / 2.0)) {
			//we are really far right in the river. just move left.
			movementVector.x = -1.0;
		}
		else {
			movementVector.x = this->getPosition()->x + (this->GetSize() / 2.0) - ((RIGHT_BRIDGE_CENTER_X + (BRIDGE_WIDTH / 2.0) + GAME_GRID_WIDTH) / 2.0);
		}
	}
	else {
		//we're on a bridge. don't need to do anything...
		return;
	}

	//now that we have x, let's get y
	movementVector.y = this->getPosition()->y - (GAME_GRID_HEIGHT / 2.0); // once again, don't use sides, just center points

	movementVector.normalize();
	movementVector *= elapsedTime;
	this->pos += movementVector;

}

std::vector<std::shared_ptr<Mob>> Mob::checkCollision() {
	std::vector<std::shared_ptr<Mob>> collisions;
	for (std::shared_ptr<Mob> otherMob : GameState::mobs) {
		// don't collide with yourself
		if (this->sameMob(otherMob)) { continue; }

		// PROJECT 3: YOUR CODE CHECKING FOR A COLLISION GOES HERE
		float xDist = abs(this->getPosition()->x - otherMob->getPosition()->x);
		float yDist = abs(this->getPosition()->y - otherMob->getPosition()->y);
		float touchDist = (this->GetSize() + otherMob->GetSize()) / 2.0;
		if (xDist < touchDist && yDist < touchDist) {
			collisions.push_back(otherMob);
		}
	}
	return collisions;
}

void Mob::processCollision(std::shared_ptr<Mob> otherMob, double elapsedTime) {
	// PROJECT 3: YOUR COLLISION HANDLING CODE GOES HERE
	Point movementVector;
	float xDist = this->getPosition()->x - otherMob->getPosition()->x;
	float yDist = this->getPosition()->y - otherMob->getPosition()->y;
	float touchDist = ((this->GetSize() + otherMob->GetSize()) / 2.0);
	movementVector.x = 0;
	movementVector.y = 0;
	if (touchDist > xDist && xDist >= 0) {
		movementVector.x = touchDist - xDist;
	}
	else if (touchDist > -xDist && xDist < 0) {
		movementVector.x = -touchDist - xDist;
	}
	if (touchDist > yDist&& yDist > 0) {
		movementVector.y = touchDist - yDist;
	}
	else if (touchDist > -yDist && yDist < 0) {
		movementVector.y = -touchDist - yDist;
	}

	movementVector.normalize();
	movementVector *= elapsedTime;
	
	//movement at this point is calibrated for us moving back and them not moving
	if (otherMob->GetMass() > this->GetMass()) {
		//we should get pushed back
		this->pos += movementVector;
	}
	else if (otherMob->GetMass() == this->GetMass()) {
		//move both half
		movementVector *= 0.5;
		this->pos += movementVector;
		movementVector *= -1;
		otherMob->pos = otherMob->pos + movementVector;
	}
	else {
		//they should get pushed back
		movementVector *= -1;
		otherMob->pos = otherMob->pos + movementVector;
	}
	
}

// Collisions
///////////////////////////////////////////////
// Procedures

void Mob::attackProcedure(double elapsedTime) {
	if (this->target == nullptr || this->target->isDead()) {
		this->targetLocked = false;
		this->target = nullptr;
		this->state = MobState::Moving;
		return;
	}

	//check collisions
	std::vector<std::shared_ptr<Mob>> otherMobs = this->checkCollision();
	for (std::shared_ptr<Mob> otherMob : otherMobs) {
		if (otherMob) {
			this->processCollision(otherMob, elapsedTime);
		}
	}

	this->processBuildingCollision(elapsedTime);

	this->processRiverCollision(elapsedTime);

	if (targetInRange()) {
		if (this->lastAttackTime >= this->GetAttackTime()) {
			// If our last attack was longer ago than our cooldown
			this->target->attack(this->GetDamage());
			this->lastAttackTime = 0; // lastAttackTime is incremented in the main update function
			return;
		}
	}
	else {
		// If the target is not in range
		moveTowards(target->getPosition(), elapsedTime);
	}
}

void Mob::moveProcedure(double elapsedTime) {
	if (targetPosition) {
		moveTowards(targetPosition, elapsedTime);

		// Check for collisions
		if (this->nextWaypoint->pos.insideOf(this->pos, (this->GetSize() + WAYPOINT_SIZE))) {
			std::shared_ptr<Waypoint> trueNextWP = this->attackingNorth ?
												   this->nextWaypoint->upNeighbor :
												   this->nextWaypoint->downNeighbor;
			setNewWaypoint(trueNextWP);
		}

		// PROJECT 3: You should not change this code very much, but this is where your 
		// collision code will be called from
		std::vector<std::shared_ptr<Mob>> otherMobs = this->checkCollision();
		for (std::shared_ptr<Mob> otherMob : otherMobs) {
			if (otherMob) {
				this->processCollision(otherMob, elapsedTime);
			}
		}

		this->processBuildingCollision(elapsedTime);

		this->processRiverCollision(elapsedTime);

		// Fighting otherMob takes priority always
		findAndSetAttackableMob();

	} else {
		// if targetPosition is nullptr
		findNewTarget();
	}
}

void Mob::update(double elapsedTime) {

	switch (this->state) {
	case MobState::Attacking:
		this->attackProcedure(elapsedTime);
		break;
	case MobState::Moving:
	default:
		this->moveProcedure(elapsedTime);
		break;
	}

	this->lastAttackTime += (float)elapsedTime;
}
