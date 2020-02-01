#pragma once

#include <memory>
#include "Attackable.h"
#include "Building.h"
#include "Waypoint.h"
#include "Point.h"

class Mob : public Attackable {

public:
	Mob(const Point& pos, bool attackingNorth);
	virtual ~Mob() {}

	bool IsAttackingNorth() const { return attackingNorth; }

	// Stats related
	virtual int GetMaxHealth() const = 0;
	virtual float GetSpeed() const = 0;
	virtual float GetSize() const = 0;
	virtual int GetDamage() const = 0;
	virtual float GetAttackTime() const = 0;

	int GetHealth() const { return health; }

	// Movement related
	Point pos; // The position of this mob
	std::shared_ptr<Waypoint> nextWaypoint; // The default movement target if there are no enemies nearby
	std::shared_ptr<Point> targetPosition; // The actual place this mob is moving towards

	// The main function that drives this mob. Should be called once every game tick.
	void update(double elapsedTime);

	bool isDead() { return health <= 0; }

	int attack(int dmg); // deal dmg to this mob

	bool sameMob(Mob* otherMob) { return this->uuid == otherMob->uuid; }
	bool sameMob(std::shared_ptr<Mob> otherMob) { return this->uuid == otherMob->uuid; }

	std::shared_ptr<Point> getPosition();

protected: 
	enum class MobState
	{
		Moving,
		Attacking
	};
	MobState state;

	static int previousUUID;
	int uuid;

	bool attackingNorth;

	int health;

	bool targetLocked;
	std::shared_ptr<Attackable> target;

	int lastAttackTime;           // How many frames ago was the last attack? 

	bool findClosestWaypoint();

	void findNewTarget();

	// Have this mob start moving towards the provided target
	void updateMoveTarget(std::shared_ptr<Point> target);

	void updateMoveTarget(Point target);

	void moveTowards(std::shared_ptr<Point> moveTarget, double elapsedTime);
	// Movement related
	//////////////////////////////////
	// Combat related

	bool findAndSetAttackableMob();

	// Push this unit away from the provided point
	void pushAway(Point awayFrom, double elapsedTime);

	void setNewWaypoint(std::shared_ptr<Waypoint> newWaypoint) {
		this->nextWaypoint = newWaypoint;
		this->updateMoveTarget(newWaypoint->pos);
	}

	void setAttackTarget(std::shared_ptr<Attackable> b);

	bool targetInRange();

	// Combat related
	////////////////////////////////////////////////////////////
	// Collisions

	std::shared_ptr<Building> checkBuildingCollision();

	void processBuildingCollision(std::shared_ptr<Building> b, double elapsedTime);

	std::shared_ptr<Mob> checkMobCollision();

	void processMobCollision(std::shared_ptr<Mob> otherMob, double elapsedTime);

	// Collisions
	///////////////////////////////////////////////
	// Procedures

	void attackProcedure(double elapsedTime);

	void moveProcedure(double elapsedTime);

};

