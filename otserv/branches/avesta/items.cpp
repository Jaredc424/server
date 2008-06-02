//////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
//////////////////////////////////////////////////////////////////////
// The database of items.
//////////////////////////////////////////////////////////////////////
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//////////////////////////////////////////////////////////////////////
#include "otpch.h"

#include "items.h"
#include "spells.h"
#include "condition.h"
#include "weapons.h"

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include <iostream>
#include <string>

uint32_t Items::dwMajorVersion = 0;
uint32_t Items::dwMinorVersion = 0;
uint32_t Items::dwBuildNumber = 0;

extern Spells* g_spells;

ItemType::ItemType()
{
	article          = "";
	group            = ITEM_GROUP_NONE;
	type             = ITEM_TYPE_NONE;
	stackable        = false;
	useable	         = false;
	moveable         = true;
	alwaysOnTop      = false;
	alwaysOnTopOrder = 0;
	pickupable       = false;
	rotable          = false;
	rotateTo		     = 0;
	hasHeight        = false;

	floorChangeDown = true;
	floorChangeNorth = false;
	floorChangeSouth = false;
	floorChangeEast = false;
	floorChangeWest = false;

	blockSolid = false;
	blockProjectile = false;
	blockPathFind = false;

	std::string runeSpellName;
	runeMagLevel    = 0;

	speed		      = 0;
	id            = 0;
	clientId      = 100;
	maxItems      = 8;  // maximum size if this is a container
	weight        = 0;  // weight of the item, e.g. throwing distance depends on it
	showCount     = true;
	weaponType    = WEAPON_NONE;
	slot_position = SLOTP_RIGHT | SLOTP_LEFT | SLOTP_AMMO;
	amuType       = AMMO_NONE;
	ammoAction    = AMMOACTION_NONE;
	shootType     = (ShootType_t)0;
	magicEffect   = NM_ME_NONE;
	attack        = 0;
	defence       = 0;
	extraDef      = 0;
	armor         = 0;
	decayTo       = -1;
	decayTime     = 0;
	stopTime      = false;
	isCorpse      = false;

	allowDistRead = false;

	isVertical		= false;
	isHorizontal	= false;
	isHangable		= false;

	lightLevel    = 0;
	lightColor    = 0;

	maxTextLen = 0;
	canReadText = false;
	canWriteText = false;
	writeOnceItemId  = 0;

	transformEquipTo   = 0;
	transformDeEquipTo = 0;
	showDuration  = false;
	showCharges   = false;
	charges       = 0;
	hitChance     = -1;
	maxHitChance  = -1;
	breakChance   = -1;
	shootRange    = 1;

	condition = NULL;
	combatType = COMBAT_NONE;

	replaceable = true;
	//[ added for beds system
	bedPartnerDir = NORTH;
	maleSleeperID = 0;
	femaleSleeperID = 0;
	noSleeperID = 0;
	//]
}

ItemType::~ItemType()
{
	delete condition;
}

Items::Items() :
items(8000)
{
	//
}

Items::~Items()
{
	clear();
}

void Items::clear()
{
	//TODO. clear items?
}

bool Items::reload()
{
	//TODO?
	/*
	for (ItemMap::iterator it = items.begin(); it != items.end(); it++){
		delete it->second->condition;
	}
	return loadFromXml(m_datadir);
	*/
	return false;
}

int Items::loadFromOtb(std::string file)
{
	FileLoader f;
	if(!f.openFile(file.c_str(), false, true)){
		return f.getError();
	}

	unsigned long type;
	NODE node = f.getChildNode(NO_NODE, type);

	PropStream props;
	if(f.getProps(node,props)){
		//4 byte flags
		//attributes
		//0x01 = version data
		uint32_t flags;
		if(!props.GET_ULONG(flags)){
			return ERROR_INVALID_FORMAT;
		}
		attribute_t attr;
		if(!props.GET_VALUE(attr)){
			return ERROR_INVALID_FORMAT;
		}
		if(attr == ROOT_ATTR_VERSION){
			datasize_t datalen = 0;
			if(!props.GET_VALUE(datalen)){
				return ERROR_INVALID_FORMAT;
			}
			if(datalen != sizeof(VERSIONINFO)){
				return ERROR_INVALID_FORMAT;
			}
			VERSIONINFO *vi;
			if(!props.GET_STRUCT(vi)){
				return ERROR_INVALID_FORMAT;
			}
			Items::dwMajorVersion = vi->dwMajorVersion;	//items otb format file version
			Items::dwMinorVersion = vi->dwMinorVersion;	//client version
			Items::dwBuildNumber = vi->dwBuildNumber;	//revision
		}
	}

	if(Items::dwMajorVersion != 2){
		std::cout << "Not supported items.otb version." << std::endl;
		return ERROR_INVALID_FORMAT;
	}

	if(Items::dwMajorVersion == 0xFFFFFFFF){
		std::cout << "[Warning] Items::loadFromOtb items.otb using generic client version." << std::endl;
	}
	else if(Items::dwMinorVersion != CLIENT_VERSION_811){
		std::cout << "Not supported items.otb client version." << std::endl;
		return ERROR_INVALID_FORMAT;
	}

	node = f.getChildNode(node, type);

	while(node != NO_NODE){
		PropStream props;
		if(!f.getProps(node,props)){
			return f.getError();
		}

		flags_t flags;
		ItemType* iType = new ItemType();
		iType->group = (itemgroup_t)type;

		switch(type){
			case ITEM_GROUP_CONTAINER:
				iType->type = ITEM_TYPE_CONTAINER;
				break;
			case ITEM_GROUP_DOOR:
				iType->type = ITEM_TYPE_DOOR;
				break;
			case ITEM_GROUP_MAGICFIELD:
				iType->type = ITEM_TYPE_MAGICFIELD;
				break;
			case ITEM_GROUP_TELEPORT:
				iType->type = ITEM_TYPE_TELEPORT;
				break;
			case ITEM_GROUP_NONE:
			case ITEM_GROUP_GROUND:
			case ITEM_GROUP_RUNE:
			case ITEM_GROUP_SPLASH:
			case ITEM_GROUP_FLUID:
			case ITEM_GROUP_DEPRECATED:
				break;
			default:
				return ERROR_INVALID_FORMAT;
				break;
		}

		//read 4 byte flags
		if(!props.GET_VALUE(flags)){
			return ERROR_INVALID_FORMAT;
		}

		iType->blockSolid = hasBitSet(FLAG_BLOCK_SOLID, flags);
		iType->blockProjectile = hasBitSet(FLAG_BLOCK_PROJECTILE, flags);
		iType->blockPathFind = hasBitSet(FLAG_BLOCK_PATHFIND, flags);
		iType->hasHeight = hasBitSet(FLAG_HAS_HEIGHT, flags);
		iType->useable = hasBitSet(FLAG_USEABLE, flags);
		iType->pickupable = hasBitSet(FLAG_PICKUPABLE, flags);
		iType->moveable = hasBitSet(FLAG_MOVEABLE, flags);
		iType->stackable = hasBitSet(FLAG_STACKABLE, flags);
		iType->floorChangeDown = hasBitSet(FLAG_FLOORCHANGEDOWN, flags);
		iType->floorChangeNorth = hasBitSet(FLAG_FLOORCHANGENORTH, flags);
		iType->floorChangeEast = hasBitSet(FLAG_FLOORCHANGEEAST, flags);
		iType->floorChangeSouth = hasBitSet(FLAG_FLOORCHANGESOUTH, flags);
		iType->floorChangeWest = hasBitSet(FLAG_FLOORCHANGEWEST, flags);
		iType->alwaysOnTop = hasBitSet(FLAG_ALWAYSONTOP, flags);
		iType->isVertical = hasBitSet(FLAG_VERTICAL, flags);
		iType->isHorizontal = hasBitSet(FLAG_HORIZONTAL, flags);
		iType->isHangable = hasBitSet(FLAG_HANGABLE, flags);
		iType->allowDistRead = hasBitSet(FLAG_ALLOWDISTREAD, flags);
		iType->rotable = hasBitSet(FLAG_ROTABLE, flags);
		iType->canReadText = hasBitSet(FLAG_READABLE, flags);
		iType->isCorpse = hasBitSet(FLAG_CORPSE, flags);

		attribute_t attrib;
		datasize_t datalen = 0;
		while(props.GET_VALUE(attrib)){
			//size of data
			if(!props.GET_VALUE(datalen)){
				delete iType;
				return ERROR_INVALID_FORMAT;
			}
			switch(attrib){
			case ITEM_ATTR_SERVERID:
			{
				if(datalen != sizeof(uint16_t))
					return ERROR_INVALID_FORMAT;

				uint16_t serverid;
				if(!props.GET_USHORT(serverid))
					return ERROR_INVALID_FORMAT;

				if(serverid > 20000)
					return ERROR_INVALID_FORMAT;

				iType->id = serverid;
				break;
			}
			case ITEM_ATTR_CLIENTID:
			{
				if(datalen != sizeof(uint16_t))
					return ERROR_INVALID_FORMAT;

				uint16_t clientid;
				if(!props.GET_USHORT(clientid))
					return ERROR_INVALID_FORMAT;

				iType->clientId = clientid;
				break;
			}
			case ITEM_ATTR_SPEED:
			{
				if(datalen != sizeof(uint16_t))
					return ERROR_INVALID_FORMAT;

				uint16_t speed;
				if(!props.GET_USHORT(speed))
					return ERROR_INVALID_FORMAT;

				iType->speed = speed;

				break;
			}
			case ITEM_ATTR_LIGHT2:
			{
				if(datalen != sizeof(lightBlock2))
					return ERROR_INVALID_FORMAT;

				lightBlock2* lb2;
				if(!props.GET_STRUCT(lb2))
					return ERROR_INVALID_FORMAT;

				iType->lightLevel = lb2->lightLevel;
				iType->lightColor = lb2->lightColor;
				break;
			}
			case ITEM_ATTR_TOPORDER:
			{
				if(datalen != sizeof(uint8_t))
					return ERROR_INVALID_FORMAT;

				uint8_t v;
				if(!props.GET_UCHAR(v))
					return ERROR_INVALID_FORMAT;

				iType->alwaysOnTopOrder = v;
				break;
			}
			default:
				//skip unknown attributes
				if(!props.SKIP_N(datalen))
					return ERROR_INVALID_FORMAT;
				break;
			}
		}

		reverseItemMap[iType->clientId] = iType->id;

		// store the found item
		items.addElement(iType, iType->id);
		node = f.getNextNode(node, type);
	}

	return ERROR_NONE;
}

bool Items::loadFromXml(const std::string& datadir)
{
	m_datadir = datadir;
	std::string filename = m_datadir + "/items/items.xml";

	xmlDocPtr doc = xmlParseFile(filename.c_str());
	int intValue;
	std::string strValue;
	uint32_t id = 0;

	if(doc){
		xmlNodePtr root = xmlDocGetRootElement(doc);

		if(xmlStrcmp(root->name,(const xmlChar*)"items") != 0){
			xmlFreeDoc(doc);
			return false;
		}

		xmlNodePtr itemNode = root->children;
		while(itemNode){
			if(xmlStrcmp(itemNode->name,(const xmlChar*)"item") == 0){
				if(readXMLInteger(itemNode, "id", intValue)){
					id = intValue;

					if(id > 20000 && id < 20100){
						id = id - 20000;

						ItemType* iType = new ItemType();
						iType->id = id;
						items.addElement(iType, iType->id);
					}

					ItemType& it = Item::items.getItemType(id);

					if(readXMLString(itemNode, "name", strValue)){
						it.name = strValue;
					}

					if(readXMLString(itemNode, "article", strValue)){
						it.article = strValue;
					}

					if(readXMLString(itemNode, "plural", strValue)){
						it.pluralName = strValue;
					}

					xmlNodePtr itemAttributesNode = itemNode->children;

					while(itemAttributesNode){
						if(readXMLString(itemAttributesNode, "key", strValue)){
							if(strValue == "type"){
								if(readXMLString(itemAttributesNode, "value", strValue)){
									if(strValue == "key"){
										it.group = ITEM_GROUP_KEY;
									}
									else if(strValue == "magicfield"){
										it.group = ITEM_GROUP_MAGICFIELD;
										it.type = ITEM_TYPE_MAGICFIELD;
									}
									else if(strValue == "depot"){
										it.type = ITEM_TYPE_DEPOT;
									}
									else if(strValue == "mailbox"){
										it.type = ITEM_TYPE_MAILBOX;
									}
									else if(strValue == "trashholder"){
										it.type = ITEM_TYPE_TRASHHOLDER;
									}
									//[ added for beds system
									else if(strValue == "bed"){
										it.type = ITEM_TYPE_BED;
									}
									//]
									else{
										std::cout << "Warning: [Items::loadFromXml] " << "Unknown type " << strValue  << std::endl;
									}
								}
							}
							else if(strValue == "name"){
								if(readXMLString(itemAttributesNode, "value", strValue)){
									it.name = strValue;
								}
							}
							else if(strValue == "article"){
								if(readXMLString(itemAttributesNode, "value", strValue)){
									it.article = strValue;
								}
							}
							else if(strValue == "plural"){
								if(readXMLString(itemAttributesNode, "value", strValue)){
									it.pluralName = strValue;
								}
							}
							else if(strValue == "description"){
								if(readXMLString(itemAttributesNode, "value", strValue)){
									it.description = strValue;
								}
							}
							else if(strValue == "runeSpellName"){
								if(readXMLString(itemAttributesNode, "value", strValue)){
									it.runeSpellName = strValue;
								}
							}
							else if(strValue == "weight"){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.weight = intValue / 100.f;
								}
							}
							else if(strValue == "showcount"){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.showCount = (intValue != 0);
								}
							}
							else if(strValue == "armor"){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.armor = intValue;
								}
							}
							else if(strValue == "defense"){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.defence = intValue;
								}
							}
							else if(strValue == "extradef"){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.extraDef = intValue;
								}
							}
							else if(strValue == "attack"){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.attack = intValue;
								}
							}
							else if(strValue == "rotateTo"){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.rotateTo = intValue;
								}
							}
							else if(strValue == "containerSize"){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.maxItems = intValue;
								}
							}
							/*
							else if(strValue == "readable"){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.canReadText = true;
								}
							}
							*/
							else if(strValue == "writeable"){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.canWriteText = (intValue != 0);
									it.canReadText = (intValue != 0);
								}
							}
							else if(strValue == "maxTextLen"){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.maxTextLen = intValue;
								}
							}
							else if(strValue == "writeOnceItemId"){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.writeOnceItemId = intValue;
								}
							}
							else if(strValue == "weaponType"){
								if(readXMLString(itemAttributesNode, "value", strValue)){
									if(strValue == "sword"){
										it.weaponType = WEAPON_SWORD;
									}
									else if(strValue == "club"){
										it.weaponType = WEAPON_CLUB;
									}
									else if(strValue == "axe"){
										it.weaponType = WEAPON_AXE;
									}
									else if(strValue == "shield"){
										it.weaponType = WEAPON_SHIELD;
									}
									else if(strValue == "distance"){
										it.weaponType = WEAPON_DIST;
									}
									else if(strValue == "wand"){
										it.weaponType = WEAPON_WAND;
									}
									else if(strValue == "ammunition"){
										it.weaponType = WEAPON_AMMO;
									}
									else{
										std::cout << "Warning: [Items::loadFromXml] " << "Unknown weaponType " << strValue  << std::endl;
									}
								}
							}
							else if(strValue == "slotType"){
								if(readXMLString(itemAttributesNode, "value", strValue)){
									if(strValue == "head"){
										it.slot_position |= SLOTP_HEAD;
									}
									else if(strValue == "body"){
										it.slot_position |= SLOTP_ARMOR;
									}
									else if(strValue == "legs"){
										it.slot_position |= SLOTP_LEGS;
									}
									else if(strValue == "feet"){
										it.slot_position |= SLOTP_FEET;
									}
									else if(strValue == "backpack"){
										it.slot_position |= SLOTP_BACKPACK;
									}
									else if(strcasecmp(strValue.c_str(), "two-handed") == 0){
										it.slot_position |= SLOTP_TWO_HAND;
									}
									else if(strValue == "necklace"){
										it.slot_position |= SLOTP_NECKLACE;
									}
									else if(strValue == "ring"){
										it.slot_position |= SLOTP_RING;
									}
									else{
										std::cout << "Warning: [Items::loadFromXml] " << "Unknown slotType " << strValue  << std::endl;
									}
								}
							}
							else if(strValue == "ammoType"){
								if(readXMLString(itemAttributesNode, "value", strValue)){
									it.amuType = getAmmoType(strValue);
									if(it.amuType == AMMO_NONE){
										std::cout << "Warning: [Items::loadFromXml] " << "Unknown ammoType " << strValue  << std::endl;
									}
								}
							}
							else if(strValue == "shootType"){
								if(readXMLString(itemAttributesNode, "value", strValue)){
									ShootType_t shoot = getShootType(strValue);
									if(shoot != NM_SHOOT_UNK){
										it.shootType = shoot;
									}
									else{
										std::cout << "Warning: [Items::loadFromXml] " << "Unknown shootType " << strValue  << std::endl;
									}
								}
							}
							else if(strValue == "effect"){
								if(readXMLString(itemAttributesNode, "value", strValue)){
									MagicEffectClasses effect = getMagicEffect(strValue);
									if(effect != NM_ME_UNK){
										it.magicEffect = effect;
									}
									else{
										std::cout << "Warning: [Items::loadFromXml] " << "Unknown effect " << strValue  << std::endl;
									}
								}
							}
							else if(strValue == "range"){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.shootRange = intValue;
								}
							}
							else if(strValue == "stopduration"){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.stopTime = (intValue != 0);
								}
							}
							else if(strValue == "decayTo"){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.decayTo = intValue;
								}
							}
							else if(strValue == "transformEquipTo"){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.transformEquipTo = intValue;
								}
							}
							else if(strValue == "transformDeEquipTo"){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.transformDeEquipTo = intValue;
								}
							}
							else if(strValue == "duration"){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.decayTime = intValue;
								}
							}
							else if(strValue == "showduration"){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.showDuration = (intValue != 0);
								}
							}
							else if(strValue == "charges"){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.charges = intValue;
								}
							}
							else if(strValue == "showcharges"){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.showCharges = (intValue != 0);
								}
							}
							else if(strValue == "breakChance"){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									if(intValue < 0){
										intValue = 0;
									}
									else if(intValue > 100){
										intValue = 100;
									}

									it.breakChance = intValue;
								}
							}
							else if(strValue == "ammoAction"){
								if(readXMLString(itemAttributesNode, "value", strValue)){
									it.ammoAction = getAmmoAction(strValue);

									if(it.ammoAction == AMMOACTION_NONE){
										std::cout << "Warning: [Items::loadFromXml] " << "Unknown ammoAction " << strValue  << std::endl;
									}
								}
							}
							else if(strValue == "hitChance"){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									if(intValue < 0){
										intValue = 0;
									}
									else if(intValue > 100){
										intValue = 100;
									}

									it.hitChance = intValue;
								}
							}
							else if(strValue == "maxHitChance"){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									if(intValue < 0){
										intValue = 0;
									}
									else if(intValue > 100){
										intValue = 100;
									}

									it.maxHitChance = intValue;
								}
							}
							else if(strValue == "invisible"){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.abilities.invisible = (intValue != 0);
								}
							}
							else if(strValue == "speed"){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.abilities.speed = intValue;
								}
							}
							else if(strValue == "healthGain"){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.abilities.regeneration = true;
									it.abilities.healthGain = intValue;
								}
							}
							else if(strValue == "healthTicks"){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.abilities.regeneration = true;
									it.abilities.healthTicks = intValue;
								}
							}
							else if(strValue == "manaGain"){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.abilities.regeneration = true;
									it.abilities.manaGain = intValue;
								}
							}
							else if(strValue == "manaTicks"){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.abilities.regeneration = true;
									it.abilities.manaTicks = intValue;
								}
							}
							else if(strValue == "manaShield"){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.abilities.manaShield = (intValue != 0);
								}
							}
							else if(strValue == "skillSword"){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.abilities.skills[SKILL_SWORD] = intValue;
								}
							}
							else if(strValue == "skillAxe"){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.abilities.skills[SKILL_AXE] = intValue;
								}
							}
							else if(strValue == "skillClub"){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.abilities.skills[SKILL_CLUB] = intValue;
								}
							}
							else if(strValue == "skillDist"){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.abilities.skills[SKILL_DIST] = intValue;
								}
							}
							else if(strValue == "skillFish"){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.abilities.skills[SKILL_FISH] = intValue;
								}
							}
							else if(strValue == "skillShield"){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.abilities.skills[SKILL_SHIELD] = intValue;
								}
							}
							else if(strValue == "skillFist"){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.abilities.skills[SKILL_FIST] = intValue;
								}
							}
							else if(strValue == "maxHitPoints"){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.abilities.stats[STAT_MAXHITPOINTS] = intValue;
								}
							}
							else if(strValue == "maxHitPointsPercent"){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.abilities.statsPercent[STAT_MAXHITPOINTS] = intValue;
								}
							}
							else if(strValue == "maxManaPoints"){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.abilities.stats[STAT_MAXMANAPOINTS] = intValue;
								}
							}
							else if(strValue == "maxManaPointsPercent"){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.abilities.statsPercent[STAT_MAXMANAPOINTS] = intValue;
								}
							}
							else if(strValue == "soulPoints"){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.abilities.stats[STAT_SOULPOINTS] = intValue;
								}
							}
							else if(strValue == "soulPointsPercent"){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.abilities.statsPercent[STAT_SOULPOINTS] = intValue;
								}
							}
							else if(strValue == "magicPoints"){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.abilities.stats[STAT_MAGICPOINTS] = intValue;
								}
							}
							else if(strValue == "magicPointsPercent"){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.abilities.statsPercent[STAT_MAGICPOINTS] = intValue;
								}
							}
							else if(strValue == "absorbPercentAll"){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.abilities.absorbPercentAll = intValue;
								}
							}
							else if(strValue == "absorbPercentEnergy"){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.abilities.absorbPercentEnergy = intValue;
								}
							}
							else if(strValue == "absorbPercentFire"){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.abilities.absorbPercentFire = intValue;
								}
							}
							else if(strValue == "absorbPercentPoison" ||
									strValue == "absorbPercentEarth"){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.abilities.absorbPercentEarth = intValue;
								}
							}
							else if(strValue == "absorbPercentIce"){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.abilities.absorbPercentIce = intValue;
								}
							}
							else if(strValue == "absorbPercentHoly"){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.abilities.absorbPercentHoly = intValue;
								}
							}
							else if(strValue == "absorbPercentDeath"){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.abilities.absorbPercentDeath = intValue;
								}
							}
							else if(strValue == "absorbPercentLifeDrain"){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.abilities.absorbPercentLifeDrain = intValue;
								}
							}
							else if(strValue == "absorbPercentManaDrain"){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.abilities.absorbPercentManaDrain = intValue;
								}
							}
							else if(strValue == "absorbPercentDrown"){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.abilities.absorbPercentDrown = intValue;
								}
							}
							else if(strValue == "absorbPercentPhysical"){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.abilities.absorbPercentPhysical = intValue;
								}
							}
							else if(strValue == "suppressDrunk"){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.abilities.conditionSuppressions |= CONDITION_DRUNK;
								}
							}
							else if(strValue == "suppressEnergy"){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.abilities.conditionSuppressions |= CONDITION_ENERGY;
								}
							}
							else if(strValue == "suppressFire"){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.abilities.conditionSuppressions |= CONDITION_FIRE;
								}
							}
							else if(strValue == "suppressPoison"){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.abilities.conditionSuppressions |= CONDITION_POISON;
								}
							}
							else if(strValue == "suppressLifeDrain"){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.abilities.conditionSuppressions |= CONDITION_LIFEDRAIN;
								}
							}
							else if(strValue == "suppressDrown"){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.abilities.conditionSuppressions |= CONDITION_DROWN;
								}
							}
							else if(strValue == "suppressFreeze"){
								if(readXMLInteger(itemAttributesNode, "value", intValue) && intValue != 0){
									it.abilities.conditionSuppressions |= CONDITION_FREEZING;
								}
							}

							else if(strValue == "suppressDazzle"){
								if(readXMLInteger(itemAttributesNode, "value", intValue) && intValue != 0){
									it.abilities.conditionSuppressions |= CONDITION_DAZZLED;
								}
							}

							else if(strValue == "suppressCurse"){
								if(readXMLInteger(itemAttributesNode, "value", intValue) && intValue != 0){
									it.abilities.conditionSuppressions |= CONDITION_CURSED;
								}
							}
							/*else if(strValue == "suppressManaDrain"){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.abilities.conditionSuppressions |= CONDITION_MANADRAIN;
								}
							}
							else if(strValue == "suppressPhysical"){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.abilities.conditionSuppressions |= CONDITION_PHYSICAL;
								}
							}*/
							else if(strValue == "field"){
								it.group = ITEM_GROUP_MAGICFIELD;
								it.type = ITEM_TYPE_MAGICFIELD;
								CombatType_t combatType = COMBAT_NONE;
								ConditionDamage* conditionDamage = NULL;

								if(readXMLString(itemAttributesNode, "value", strValue)){
									if(strValue == "fire"){
										conditionDamage = new ConditionDamage(CONDITIONID_COMBAT, CONDITION_FIRE);
										combatType = COMBAT_FIREDAMAGE;
									}
									else if(strValue == "energy"){
										conditionDamage = new ConditionDamage(CONDITIONID_COMBAT, CONDITION_ENERGY);
										combatType = COMBAT_ENERGYDAMAGE;
									}
									else if(strValue == "poison"){
										conditionDamage = new ConditionDamage(CONDITIONID_COMBAT, CONDITION_POISON);
										combatType = COMBAT_EARTHDAMAGE;
									}
									else if(strValue == "drown"){
										conditionDamage = new ConditionDamage(CONDITIONID_COMBAT, CONDITION_DROWN);
										combatType = COMBAT_DROWNDAMAGE;
									}
									//else if(strValue == "physical"){
									//	damageCondition = new ConditionDamage(CONDITIONID_COMBAT, CONDITION_PHYSICAL);
									//	combatType = COMBAT_PHYSICALDAMAGE;
									//}
									else{
										std::cout << "Warning: [Items::loadFromXml] " << "Unknown field value " << strValue  << std::endl;
									}

									if(combatType != COMBAT_NONE){
										it.combatType = combatType;
										it.condition = conditionDamage;
										it.condition->setParam(CONDITIONPARAM_FORCEUPDATE, true);
										uint32_t ticks = 0;
										int32_t damage = 0;
										int32_t start = 0;
										int32_t count = 1;

										xmlNodePtr fieldAttributesNode = itemAttributesNode->children;
										while(fieldAttributesNode){
											if(readXMLString(fieldAttributesNode, "key", strValue)){
												if(strValue == "ticks"){
													if(readXMLInteger(fieldAttributesNode, "value", intValue)){
														ticks = std::max(0, intValue);
													}
												}

												if(strValue == "count"){
													if(readXMLInteger(fieldAttributesNode, "value", intValue)){
														if(intValue > 0){
															count = intValue;
														}
														else{
															count = 1;
														}
													}
												}

												if(strValue == "start"){
													if(readXMLInteger(fieldAttributesNode, "value", intValue)){
														if(intValue > 0){
															start = intValue;
														}
														else{
															start = 0;
														}
													}
												}

												if(strValue == "damage"){
													if(readXMLInteger(fieldAttributesNode, "value", intValue)){

														damage = -intValue;

														if(start > 0){
															std::list<int32_t> damageList;
															ConditionDamage::generateDamageList(damage, start, damageList);

															for(std::list<int32_t>::iterator it = damageList.begin(); it != damageList.end(); ++it){
																conditionDamage->addDamage(1, ticks, -*it);
															}

															start = 0;
														}
														else{
															conditionDamage->addDamage(count, ticks, damage);
														}
													}
												}
											}

											fieldAttributesNode = fieldAttributesNode->next;
										}
									}
								}
							}
							else if(strValue == "replaceable"){
								if(readXMLInteger(itemAttributesNode, "value", intValue)){
									it.replaceable = (intValue != 0);
								}
							}
							else if(strValue == "partnerDirection")
							{
								if(readXMLString(itemAttributesNode, "value", strValue)) {
									if(strValue == "0" || strValue == "north" || strValue == "n") {
										it.bedPartnerDir = NORTH;
									} else if(strValue == "1" || strValue == "east" || strValue == "e") {
										it.bedPartnerDir = EAST;
									} else if(strValue == "2" || strValue == "south" || strValue == "s") {
										it.bedPartnerDir = SOUTH;
									} else if(strValue == "3" || strValue == "west" || strValue == "w") {
										it.bedPartnerDir = WEST;
									}
								}
							}
							else if(strValue == "maleSleeper")
							{
								if(readXMLInteger(itemAttributesNode, "value", intValue)) {
									it.maleSleeperID = intValue;
									ItemType& other = getItemType(intValue);
									if(other.id != 0 && other.noSleeperID == 0) {
										other.noSleeperID = it.id;
									}
									if(it.femaleSleeperID == 0)
										it.femaleSleeperID = intValue;
								}
							}
							else if(strValue == "femaleSleeper")
							{
								if(readXMLInteger(itemAttributesNode, "value", intValue)) {
									it.femaleSleeperID = intValue;
									ItemType& other = getItemType(intValue);
									if(other.id != 0 && other.noSleeperID == 0) {
										other.noSleeperID = it.id;
									}
									if(it.maleSleeperID == 0)
										it.maleSleeperID = intValue;
								}
							}
							/*
							else if(strValue == "noSleeper")
							{
								if(readXMLInteger(itemAttributesNode, "value", intValue)) {
									it.noSleeperID = intValue;
								}
							}
							*/
							else{
								std::cout << "Warning: [Items::loadFromXml] Unknown key value " << strValue  << std::endl;
							}
						}

						itemAttributesNode = itemAttributesNode->next;
					}
					// if no plural is specified we will build the default
					// plural adding "s" at the end
					if(it.pluralName.size() == 0 && it.name.size() != 0){
						it.pluralName = it.name + "s";
					}
				}
				else{
					std::cout << "Warning: [Spells::loadFromXml] - No itemid found" << std::endl;
				}
			}

			itemNode = itemNode->next;
		}

		xmlFreeDoc(doc);
	}

	//Lets do some checks..
	for(uint32_t i = 0; i < Item::items.size(); ++i){
		const ItemType* it = Item::items.getElement(i);

		if(!it){
			continue;
		}

		//check bed items
		if((it->noSleeperID != 0 || it->maleSleeperID != 0 || it->femaleSleeperID != 0) && it->type != ITEM_TYPE_BED){
			std::cout << "Warning: [Items::loadFromXml] Item " << it->id <<  "is not set as a bed-type." << std::endl;
		}

		//check looping decaying items
		if(it->decayTo <= 0 || !it->moveable){
			continue;
		}
	
		std::vector<int32_t> decayList;
		decayList.push_back(it->id);
		int32_t decayTo = it->decayTo;
		while(decayTo > 0){
			if(decayList.size() >= 10){
				std::cout << "Warning: [Items::loadFromXml] Item  " << *decayList.begin() << " an unsual long decay-chain" << std::endl;
			}

			if(std::find(decayList.begin(), decayList.end(), decayTo) == decayList.end()){
				decayList.push_back(decayTo);

				const ItemType& it = Item::items.getItemType(decayTo);
				if(it.id == 0){
					break;
				}

				decayTo = it.decayTo;
			}
			else{
				std::cout << "Warning: [Items::loadFromXml] Item  " << it->id << " has an infinite decay-chain" << std::endl;
				break;
			}
		}
	}

	return true;
}

ItemType& Items::getItemType(int32_t id)
{
	ItemType* iType = items.getElement(id);
	if(iType){
		return *iType;
	}
	else{
		#ifdef __DEBUG__
		std::cout << "WARNING! unknown itemtypeid " << id << ". using defaults." << std::endl;
		#endif
		static ItemType dummyItemType; // use this for invalid ids
		return dummyItemType;
	}
}

const ItemType& Items::getItemType(int32_t id) const
{
	ItemType* iType = items.getElement(id);
	if(iType){
		return *iType;
	}
	else{
		#ifdef __DEBUG__
		std::cout << "WARNING! unknown itemtypeid " << id << ". using defaults." << std::endl;
		#endif
		static ItemType dummyItemType; // use this for invalid ids
		return dummyItemType;
	}
}

const ItemType& Items::getItemIdByClientId(int32_t spriteId) const
{
	uint32_t i = 100;
	ItemType* iType;
	do{
		iType = items.getElement(i);
		if(iType && iType->clientId == spriteId){
			return *iType;
		}
		i++;
	}while(iType);

	#ifdef __DEBUG__
	std::cout << "WARNING! unknown sprite id " << spriteId << ". using defaults." << std::endl;
	#endif
	static ItemType dummyItemType; // use this for invalid ids
	return dummyItemType;
}

int32_t Items::getItemIdByName(const std::string& name)
{
	if(!name.empty()){
		uint32_t i = 100;
		ItemType* iType;
		do{
			iType = items.getElement(i);
			if(iType){
				if(strcasecmp(name.c_str(), iType->name.c_str()) == 0){
					return i;
				}
			}
			i++;
		}while(iType);
	}
	return -1;
}

template<typename A>
Array<A>::Array(uint32_t n)
{
	m_data = (A*)malloc(sizeof(A)*n);
	memset(m_data, 0, sizeof(A)*n);
	m_size = n;
}

template<typename A>
Array<A>::~Array()
{
	free(m_data);
}

template<typename A>
A Array<A>::getElement(uint32_t id)
{
	if(id < m_size){
		return m_data[id];
	}
	else{
		return 0;
	}
}

template<typename A>
const A Array<A>::getElement(uint32_t id) const
{
	if(id < m_size){
		return m_data[id];
	}
	else{
		return 0;
	}
}

template<typename A>
void Array<A>::addElement(A a, uint32_t pos)
{
	#define INCREMENT 5000
	if(pos >= m_size){
		m_data = (A*)realloc(m_data, sizeof(A)*(pos + INCREMENT));
		memset(m_data + m_size, 0, sizeof(A)*(pos + INCREMENT - m_size));
		m_size = pos + INCREMENT;
	}
	m_data[pos] = a;
}
