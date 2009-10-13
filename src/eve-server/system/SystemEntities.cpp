/*
	------------------------------------------------------------------------------------
	LICENSE:
	------------------------------------------------------------------------------------
	This file is part of EVEmu: EVE Online Server Emulator
	Copyright 2006 - 2008 The EVEmu Team
	For the latest information visit http://evemu.mmoforge.org
	------------------------------------------------------------------------------------
	This program is free software; you can redistribute it and/or modify it under
	the terms of the GNU Lesser General Public License as published by the Free Software
	Foundation; either version 2 of the License, or (at your option) any later
	version.

	This program is distributed in the hope that it will be useful, but WITHOUT
	ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
	FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public License along with
	this program; if not, write to the Free Software Foundation, Inc., 59 Temple
	Place - Suite 330, Boston, MA 02111-1307, USA, or go to
	http://www.gnu.org/copyleft/lesser.txt.
	------------------------------------------------------------------------------------
	Author:		Zhur
*/

#include "EVEServerPCH.h"

using namespace Destiny;

SimpleSystemEntity *SimpleSystemEntity::MakeEntity(SystemManager *system, const DBSystemEntity &entity) {
	switch(entity.groupID) {
	case EVEDB::invGroups::Sun:
	case EVEDB::invGroups::Planet:
	case EVEDB::invGroups::Moon:
		return(new SystemPlanetEntity(system, entity));
	case EVEDB::invGroups::Asteroid_Belt:
		return(new SystemAsteroidBeltEntity(system, entity));
	case EVEDB::invGroups::Stargate:	//Stargate
		return(new SystemStargateEntity(system, entity));
	//case EVEDB::invGroup::Cargo_Container:	//Cargo Container
	case EVEDB::invGroups::Station:		//Station
		return(new SystemStationEntity(system, entity));
	default:
        sLog.Error("Simple sys Entity", "Unrecognized entity type '%u' on '%s' (%u), falling back to simple space item.", entity.typeID, entity.itemName.c_str(), entity.itemID);
		break;
	}
	return new SystemSimpleEntity(system, entity);
}

SimpleSystemEntity::SimpleSystemEntity(SystemManager *system, const DBSystemEntity &entity)
: InanimateSystemEntity(system), data(entity) {}

bool SimpleSystemEntity::LoadExtras(SystemDB *db) {
	return true;
}

PyDict *SimpleSystemEntity::MakeSlimItem() const {
	PyDict *slim = new PyDict();
	slim->SetItemString("typeID", new PyInt(data.typeID));
	slim->SetItemString("ownerID", new PyInt(1));
	slim->SetItemString("itemID", new PyInt(data.itemID));
	return slim;
}

void SimpleSystemEntity::MakeDamageState(DoDestinyDamageState &into) const {
	into.shield = 0.75;		//just for testing so we know we are doing something.
	into.tau = 100000;	//no idea
	into.armor = 0.95;		//just for testing so we know we are doing something.
	into.structure = 1.00;
	into.timestamp = Win32TimeNow();
}

void InanimateSystemEntity::QueueDestinyUpdate(PyTuple **du) {
	//do nothing, the caller will handle it properly.
	//this gives them the opportunity to reduce a copy if they are smart.
}

void InanimateSystemEntity::QueueDestinyEvent(PyTuple **multiEvent) {
	//do nothing, the caller will handle it properly.
	//this gives them the opportunity to reduce a copy if they are smart.
}

SystemPlanetEntity::SystemPlanetEntity(SystemManager *system, const DBSystemEntity &entity) : SimpleSystemEntity(system, entity) {}

void SystemPlanetEntity::EncodeDestiny(std::vector<uint8> &into) const {
	#pragma pack(1)
	struct AddBall_Planet {
		BallHeader head;
		DSTBALL_RIGID_Struct main;
		NameStruct name;
	};
	#pragma pack()

	int start = into.size();
	into.resize(start + sizeof(AddBall_Planet) + data.itemName.length()*sizeof(uint16) );
	uint8 *ptr = &into[start];
	AddBall_Planet *item = (AddBall_Planet *) ptr;
	ptr += sizeof(AddBall_Planet);

	item->head.entityID = data.itemID;
	item->head.mode = Destiny::DSTBALL_RIGID;
	item->head.radius = data.radius;
	item->head.x = data.position.x;
	item->head.y = data.position.y;
	item->head.z = data.position.z;
	item->head.sub_type = AddBallSubType_planet;
	item->main.formationID = 0xFF;
	
	item->name.name_len = data.itemName.length();

	py_mbstowcs(item->name.name, data.itemName.c_str(), data.itemName.length());
}

SystemStationEntity::SystemStationEntity(SystemManager *system, const DBSystemEntity &entity)
: SimpleSystemEntity(system, entity) {
}

void SystemStationEntity::EncodeDestiny(std::vector<uint8> &into) const {
	#pragma pack(1)
	struct AddBall_Station {
		BallHeader head;
		DSTBALL_RIGID_Struct main;
		MiniBallList miniballs;
		NameStruct name;
	};
	#pragma pack()
	
	int miniball_count = 1;
	
	int start = into.size();
	into.resize(start 
		+ sizeof(AddBall_Station) 
		+ (miniball_count-1)*sizeof(MiniBall)
		+ data.itemName.length()*sizeof(uint16) );
	uint8 *ptr = &into[start];
	AddBall_Station *item = (AddBall_Station *) ptr;

	item->head.entityID = data.itemID;
	item->head.mode = Destiny::DSTBALL_RIGID;
	item->head.radius = data.radius;
	item->head.x = data.position.x;
	item->head.y = data.position.y;
	item->head.z = data.position.z;
	item->head.sub_type = AddBallSubType_station;
	item->main.formationID = 0xFF;
	
	item->miniballs.count = miniball_count;
	//TOTAL CRAP: we do not have this in our DB right now... 
	item->miniballs.balls[0].x = -7701.181;
	item->miniballs.balls[0].y = 8060.06;
	item->miniballs.balls[0].z = 27878.900;
	item->miniballs.balls[0].radius = 1639.241;
	
	//slide pointer over points...
	item = (AddBall_Station *) (((uint8 *) item)+(miniball_count-1)*sizeof(MiniBall));
	
	item->name.name_len = data.itemName.length();
	py_mbstowcs(item->name.name, data.itemName.c_str(), data.itemName.length());
}

PyDict *SystemStationEntity::MakeSlimItem() const {
	PyDict *slim = new PyDict();
	slim->SetItemString("typeID", new PyInt(data.typeID));
	//HACKED:::
	slim->SetItemString("ownerID", new PyInt(1000044));
	slim->SetItemString("itemID", new PyInt(data.itemID));
	return(slim);
}




SystemStargateEntity::SystemStargateEntity(SystemManager *system, const DBSystemEntity &entity)
: SystemStationEntity(system, entity),
  m_jumps(NULL)
{
}
SystemStargateEntity::~SystemStargateEntity() {
	targets.DoDestruction();
	PySafeDecRef( m_jumps );
}
	
bool SystemStargateEntity::LoadExtras(SystemDB *db) {
	if(!SystemStationEntity::LoadExtras(db))
		return false;

	m_jumps = db->ListJumps(GetID());
	if(m_jumps == NULL)
		return false;
	return true;
}

PyDict *SystemStargateEntity::MakeSlimItem() const {
	PyDict *slim = SystemStationEntity::MakeSlimItem();
	if(m_jumps != NULL)
		slim->SetItemString("jumps", m_jumps->Clone());
	return(slim);
}






SystemAsteroidBeltEntity::SystemAsteroidBeltEntity(SystemManager *system, const DBSystemEntity &entity)
: SimpleSystemEntity(system, entity),
  m_manager(NULL)
{
}

SystemAsteroidBeltEntity::~SystemAsteroidBeltEntity() {
	targets.DoDestruction();
	delete m_manager;
}

void SystemAsteroidBeltEntity::EncodeDestiny(std::vector<uint8> &into) const {
	#pragma pack(1)
	struct AddBall_AsteroidBelt {
		BallHeader head;
		DSTBALL_RIGID_Struct main;
		NameStruct name;
	};
	#pragma pack()
	
	
	int start = into.size();
	into.resize(start 
		+ sizeof(AddBall_AsteroidBelt)
		+ data.itemName.length()*sizeof(uint16) );
	uint8 *ptr = &into[start];
	AddBall_AsteroidBelt *item = (AddBall_AsteroidBelt *) ptr;
	ptr += sizeof(AddBall_AsteroidBelt);

	item->head.entityID = data.itemID;
	item->head.mode = Destiny::DSTBALL_RIGID;
	item->head.radius = data.radius;
	item->head.x = data.position.x;
	item->head.y = data.position.y;
	item->head.z = data.position.z;
	item->head.sub_type = AddBallSubType_asteroidBelt;
	item->main.formationID = 0xFF;
	
	item->name.name_len = data.itemName.length();
	py_mbstowcs(item->name.name, data.itemName.c_str(), data.itemName.length());
}

bool SystemAsteroidBeltEntity::LoadExtras(SystemDB *db) {
	if(!SimpleSystemEntity::LoadExtras(db))
		return false;
	
	//TODO: fire up the belt manager.
	//m_manager = new AsteroidBeltManager(new MiningDB(db), data.itemID);
	//TODO: decide if we are really going to try to use a MiningDB wrapper, or if we 
	// should just cram it into the system DB.. Obviously a mining DB is desirable, but
	//  we need to solve the issue of who owns the object and where to keep it.
	
	return true;
}

void SystemAsteroidBeltEntity::Process() {
	SimpleSystemEntity::Process();
	
}

//TODO: split this into its own file.
#include "inventory/InventoryItem.h"

SystemDungeonEntranceEntity::SystemDungeonEntranceEntity(SystemManager *system, InventoryItemRef self)
: ItemSystemEntity(self),
  m_system(system)
{
	//will use effects.WarpGateEffect
}

//this is a big hack just to document the kind of stuff a dungeon conveys.
PyDict *SystemDungeonEntranceEntity::MakeSlimItem() const {
	
	PyDict *slim = ItemSystemEntity::MakeSlimItem();
	if(slim == NULL)
		return NULL;
	
	//slim->SetItemString("itemID", new PyInt(2100000396));
	//slim->SetItemString("typeID", new PyInt(12273));
	//slim->SetItemString("ownerID", new PyInt(500021));
	
	slim->SetItemString("dunSkillLevel", new PyInt(0));
	slim->SetItemString("dunSkillTypeID", new PyNone);
	slim->SetItemString("dunObjectID", new PyInt(160449));
	slim->SetItemString("dunWipeNPC", new PyInt(1));
	slim->SetItemString("dunToGateID", new PyInt(160484));
	slim->SetItemString("dunCloaked", new PyInt(0));
	slim->SetItemString("dunScenarioID", new PyInt(23));
	slim->SetItemString("dunSpawnID", new PyInt(4));
	slim->SetItemString("dunAmount", new PyFloat(0.0));
	slim->SetItemString("dunShipClasses", new PyList(/*237, 31*/));
	slim->SetItemString("dunDirection", new PyList(/*235, 0, 1*/));
	slim->SetItemString("dunKeyLock", new PyInt(0));
	//slim->SetItemString("dunKeyQuantity", new PyInt(1));
	//slim->SetItemString("dunKeyTypeID", new PyInt(21839));
	//slim->SetItemString("dunOpenUntil", new PyInt(Win32TimeNow()+Win32Time_Hour));
	slim->SetItemString("dunMusicUrl", new PyString("res:/Sound/Music/Ambient031combat.ogg"));
	
	return(slim);
}

void SystemDungeonEntranceEntity::EncodeDestiny(std::vector<uint8> &into) const {
	#pragma pack(1)
	struct AddBall_Dungeon {
		BallHeader head;
		MassSector mass;
		DSTBALL_STOP_Struct main;
		MiniBallList miniballs;
		NameStruct name;
	};
	#pragma pack()
	
	int miniball_count = 1;

	const std::string itemName = m_self->itemName();
	
	int start = into.size();
	into.resize(start 
		+ sizeof(AddBall_Dungeon)
		+ (miniball_count-1)*sizeof(MiniBall)
		+ itemName.length()*sizeof(uint16) );
	uint8 *ptr = &into[start];
	AddBall_Dungeon *item = (AddBall_Dungeon *) ptr;
	ptr += sizeof(AddBall_Dungeon);

	item->head.entityID = m_self->itemID();
	item->head.mode = Destiny::DSTBALL_STOP;
	item->head.radius = GetRadius();
	const GPoint &pos = m_self->position();
	item->head.x = pos.x;
	item->head.y = pos.y;
	item->head.z = pos.z;
	item->head.sub_type = AddBallSubType_dungeonEntrance;
	item->mass.mass = 10000000000.00;
	item->mass.cloak = 0;
	item->mass.unknown52 = 0xFFFFFFFFFFFFFFFFLL;
	item->mass.corpID = m_self->ownerID();	//a little hacky...
	item->mass.allianceID = 0xFFFFFFFF;
	item->main.formationID = 0xFF;
	
	item->miniballs.count = miniball_count;
	//TOTAL CRAP: we do not have this in our DB right now... 
	item->miniballs.balls[0].x = -7701.181;
	item->miniballs.balls[0].y = 8060.06;
	item->miniballs.balls[0].z = 2778.900;
	item->miniballs.balls[0].radius = GetRadius();

	//slide pointer over points...
	item = (AddBall_Dungeon *) (((uint8 *) item)+(miniball_count-1)*sizeof(MiniBall));

	item->name.name_len = itemName.length();
	py_mbstowcs(item->name.name, itemName.c_str(), itemName.length());
}






























