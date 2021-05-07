/**
 * The Forgotten Server - a free and open-source MMORPG server emulator
 * Copyright (C) 2019  Mark Samman <mark.samman@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "otpch.h"

#include "item.h"
#include "container.h"
#include "teleport.h"
#include "trashholder.h"
#include "mailbox.h"
#include "house.h"
#include "game.h"
#include "bed.h"

#include "actions.h"
#include "spells.h"

extern Game g_game;
extern Spells* g_spells;
extern Vocations g_vocations;

Items Item::items;

Item* Item::CreateItem(const uint16_t type, uint16_t count /*= 0*/)
{
	Item* newItem = nullptr;

	const ItemType& it = Item::items[type];
	if (it.group == ITEM_GROUP_DEPRECATED) {
		return nullptr;
	}

	if (it.stackable && count == 0) {
		count = 1;
	}

	if (it.id != 0) {
		if (it.isDepot()) {
			newItem = new DepotLocker(type);
		} else if (it.isContainer()) {
			newItem = new Container(type);
		} else if (it.isTeleport()) {
			newItem = new Teleport(type);
		} else if (it.isMagicField()) {
			newItem = new MagicField(type);
		} else if (it.isDoor()) {
			newItem = new Door(type);
		} else if (it.isTrashHolder()) {
			newItem = new TrashHolder(type);
		} else if (it.isMailbox()) {
			newItem = new Mailbox(type);
		} else if (it.isBed()) {
			newItem = new BedItem(type);
		} else if (it.id >= 2210 && it.id <= 2212) { // magic rings
			newItem = new Item(type - 3, count);
		} else if (it.id == 2215 || it.id == 2216) { // magic rings
			newItem = new Item(type - 2, count);
		} else if (it.id >= 2202 && it.id <= 2206) { // magic rings
			newItem = new Item(type - 37, count);
		} else if (it.id == 2640) { // soft boots
			newItem = new Item(6132, count);
		} else if (it.id == 6301) { // death ring
			newItem = new Item(6300, count);
		} else if (it.id == 18528) { // prismatic ring
			newItem = new Item(18408, count);
		} else {
			newItem = new Item(type, count);
		}

		newItem->incrementReferenceCounter();
	}

	return newItem;
}

Container* Item::CreateItemAsContainer(const uint16_t type, uint16_t size)
{
	const ItemType& it = Item::items[type];
	if (it.id == 0 || it.group == ITEM_GROUP_DEPRECATED || it.stackable || it.useable || it.moveable || it.pickupable || it.isDepot() || it.isSplash() || it.isDoor()) {
		return nullptr;
	}

	Container* newItem = new Container(type, size);
	newItem->incrementReferenceCounter();
	return newItem;
}

Item* Item::CreateItem(PropStream& propStream)
{
	uint16_t id;
	if (!propStream.read<uint16_t>(id)) {
		return nullptr;
	}

	switch (id) {
		case ITEM_FIREFIELD_PVP_FULL:
			id = ITEM_FIREFIELD_PERSISTENT_FULL;
			break;

		case ITEM_FIREFIELD_PVP_MEDIUM:
			id = ITEM_FIREFIELD_PERSISTENT_MEDIUM;
			break;

		case ITEM_FIREFIELD_PVP_SMALL:
			id = ITEM_FIREFIELD_PERSISTENT_SMALL;
			break;

		case ITEM_ENERGYFIELD_PVP:
			id = ITEM_ENERGYFIELD_PERSISTENT;
			break;

		case ITEM_POISONFIELD_PVP:
			id = ITEM_POISONFIELD_PERSISTENT;
			break;

		case ITEM_MAGICWALL:
			id = ITEM_MAGICWALL_PERSISTENT;
			break;

		case ITEM_WILDGROWTH:
			id = ITEM_WILDGROWTH_PERSISTENT;
			break;

		default:
			break;
	}

	return Item::CreateItem(id, 0);
}

Item::Item(const uint16_t type, uint16_t count /*= 0*/) :
	id(type)
{
	const ItemType& it = items[id];

	if (it.isFluidContainer() || it.isSplash()) {
		setFluidType(count);
	} else if (it.stackable) {
		if (count != 0) {
			setItemCount(count);
		} else if (it.charges != 0) {
			setItemCount(it.charges);
		}
	} else if (it.charges != 0) {
		if (count != 0) {
			setCharges(count);
		} else {
			setCharges(it.charges);
		}
	}

	setDefaultDuration();
}

Item::Item(const Item& i) :
	Thing(), id(i.id), count(i.count), loadedFromMap(i.loadedFromMap)
{
	if (i.attributes) {
		attributes.reset(new ItemAttributes(*i.attributes));
	}
}

Item* Item::clone() const
{
	Item* item = Item::CreateItem(id, count);
	if (attributes) {
		item->attributes.reset(new ItemAttributes(*attributes));
		if (item->getDuration() > 0) {
			item->incrementReferenceCounter();
			item->setDecaying(DECAYING_TRUE);
			g_game.toDecayItems.push_front(item);
		}
	}
	return item;
}

bool Item::equals(const Item* otherItem) const
{
	if (!otherItem || id != otherItem->id) {
		return false;
	}

	const auto& otherAttributes = otherItem->attributes;
	if (!attributes) {
		return !otherAttributes || (otherAttributes->attributeBits == 0);
	} else if (!otherAttributes) {
		return (attributes->attributeBits == 0);
	}

	if (attributes->attributeBits != otherAttributes->attributeBits) {
		return false;
	}

	const auto& attributeList = attributes->attributes;
	const auto& otherAttributeList = otherAttributes->attributes;
	for (const auto& attribute : attributeList) {
		if (ItemAttributes::isStrAttrType(attribute.type)) {
			for (const auto& otherAttribute : otherAttributeList) {
				if (attribute.type == otherAttribute.type && *attribute.value.string != *otherAttribute.value.string) {
					return false;
				}
			}
		} else {
			for (const auto& otherAttribute : otherAttributeList) {
				if (attribute.type == otherAttribute.type && attribute.value.integer != otherAttribute.value.integer) {
					return false;
				}
			}
		}
	}
	return true;
}

void Item::setDefaultSubtype()
{
	const ItemType& it = items[id];

	setItemCount(1);

	if (it.charges != 0) {
		if (it.stackable) {
			setItemCount(it.charges);
		} else {
			setCharges(it.charges);
		}
	}
}

void Item::onRemoved()
{
	ScriptEnvironment::removeTempItem(this);

	if (hasAttribute(ITEM_ATTRIBUTE_UNIQUEID)) {
		g_game.removeUniqueItem(getUniqueId());
	}
}

void Item::setID(uint16_t newid)
{
	const ItemType& prevIt = Item::items[id];
	id = newid;

	const ItemType& it = Item::items[newid];
	uint32_t newDuration = it.decayTime * 1000;

	if (newDuration == 0 && !it.stopTime && it.decayTo < 0) {
		removeAttribute(ITEM_ATTRIBUTE_DECAYSTATE);
		removeAttribute(ITEM_ATTRIBUTE_DURATION);
	}

	removeAttribute(ITEM_ATTRIBUTE_CORPSEOWNER);

	if (newDuration > 0 && (!prevIt.stopTime || !hasAttribute(ITEM_ATTRIBUTE_DURATION))) {
		setDecaying(DECAYING_FALSE);
		setDuration(newDuration);
	}
}

Cylinder* Item::getTopParent()
{
	Cylinder* aux = getParent();
	Cylinder* prevaux = dynamic_cast<Cylinder*>(this);
	if (!aux) {
		return prevaux;
	}

	while (aux->getParent() != nullptr) {
		prevaux = aux;
		aux = aux->getParent();
	}

	if (prevaux) {
		return prevaux;
	}
	return aux;
}

const Cylinder* Item::getTopParent() const
{
	const Cylinder* aux = getParent();
	const Cylinder* prevaux = dynamic_cast<const Cylinder*>(this);
	if (!aux) {
		return prevaux;
	}

	while (aux->getParent() != nullptr) {
		prevaux = aux;
		aux = aux->getParent();
	}

	if (prevaux) {
		return prevaux;
	}
	return aux;
}

Tile* Item::getTile()
{
	Cylinder* cylinder = getTopParent();
	//get root cylinder
	if (cylinder && cylinder->getParent()) {
		cylinder = cylinder->getParent();
	}
	return dynamic_cast<Tile*>(cylinder);
}

const Tile* Item::getTile() const
{
	const Cylinder* cylinder = getTopParent();
	//get root cylinder
	if (cylinder && cylinder->getParent()) {
		cylinder = cylinder->getParent();
	}
	return dynamic_cast<const Tile*>(cylinder);
}

uint16_t Item::getSubType() const
{
	const ItemType& it = items[id];
	if (it.isFluidContainer() || it.isSplash()) {
		return getFluidType();
	} else if (it.stackable) {
		return count;
	} else if (it.charges != 0) {
		return getCharges();
	}
	return count;
}

Player* Item::getHoldingPlayer() const
{
	Cylinder* p = getParent();
	while (p) {
		if (p->getCreature()) {
			return p->getCreature()->getPlayer();
		}

		p = p->getParent();
	}
	return nullptr;
}

void Item::setSubType(uint16_t n)
{
	const ItemType& it = items[id];
	if (it.isFluidContainer() || it.isSplash()) {
		setFluidType(n);
	} else if (it.stackable) {
		setItemCount(n);
	} else if (it.charges != 0) {
		setCharges(n);
	} else {
		setItemCount(n);
	}
}

Attr_ReadValue Item::readAttr(AttrTypes_t attr, PropStream& propStream)
{
	switch (attr) {
		case ATTR_COUNT:
		case ATTR_RUNE_CHARGES: {
			uint8_t count;
			if (!propStream.read<uint8_t>(count)) {
				return ATTR_READ_ERROR;
			}

			setSubType(count);
			break;
		}

		case ATTR_ACTION_ID: {
			uint16_t actionId;
			if (!propStream.read<uint16_t>(actionId)) {
				return ATTR_READ_ERROR;
			}

			setActionId(actionId);
			break;
		}

		case ATTR_UNIQUE_ID: {
			uint16_t uniqueId;
			if (!propStream.read<uint16_t>(uniqueId)) {
				return ATTR_READ_ERROR;
			}

			setUniqueId(uniqueId);
			break;
		}

		case ATTR_TEXT: {
			std::string text;
			if (!propStream.readString(text)) {
				return ATTR_READ_ERROR;
			}

			setText(text);
			break;
		}

		case ATTR_WRITTENDATE: {
			uint32_t writtenDate;
			if (!propStream.read<uint32_t>(writtenDate)) {
				return ATTR_READ_ERROR;
			}

			setDate(writtenDate);
			break;
		}

		case ATTR_WRITTENBY: {
			std::string writer;
			if (!propStream.readString(writer)) {
				return ATTR_READ_ERROR;
			}

			setWriter(writer);
			break;
		}

		case ATTR_DESC: {
			std::string text;
			if (!propStream.readString(text)) {
				return ATTR_READ_ERROR;
			}

			setSpecialDescription(text);
			break;
		}

		case ATTR_CHARGES: {
			uint16_t charges;
			if (!propStream.read<uint16_t>(charges)) {
				return ATTR_READ_ERROR;
			}

			setSubType(charges);
			break;
		}

		case ATTR_DURATION: {
			uint32_t duration;
			if (!propStream.read<uint32_t>(duration)) {
				return ATTR_READ_ERROR;
			}

			setDuration(std::max<uint32_t>(0, duration));
			break;
		}

		case ATTR_DECAYING_STATE: {
			uint8_t state;
			if (!propStream.read<uint8_t>(state)) {
				return ATTR_READ_ERROR;
			}

			if (state != DECAYING_FALSE) {
				setDecaying(DECAYING_PENDING);
			}
			break;
		}

		case ATTR_NAME: {
			std::string name;
			if (!propStream.readString(name)) {
				return ATTR_READ_ERROR;
			}

			setStrAttr(ITEM_ATTRIBUTE_NAME, name);
			break;
		}

		case ATTR_ARTICLE: {
			std::string article;
			if (!propStream.readString(article)) {
				return ATTR_READ_ERROR;
			}

			setStrAttr(ITEM_ATTRIBUTE_ARTICLE, article);
			break;
		}

		case ATTR_PLURALNAME: {
			std::string pluralName;
			if (!propStream.readString(pluralName)) {
				return ATTR_READ_ERROR;
			}

			setStrAttr(ITEM_ATTRIBUTE_PLURALNAME, pluralName);
			break;
		}

		case ATTR_ATTACK:
		{
			uint32_t attack;
			if (!propStream.read<uint32_t>(attack))
			{
				return ATTR_READ_ERROR;
			}
			setIntAttr(ITEM_ATTRIBUTE_ATTACK, attack);
			break;
		}
		case ATTR_WEIGHT:
		{
			uint32_t weight;
			if (!propStream.read<uint32_t>(weight))
			{
				return ATTR_READ_ERROR;
			}

			setIntAttr(ITEM_ATTRIBUTE_WEIGHT, weight);
			break;
		}
			/*uint64_t and beyond */
		case ATTR_DEFENSE:
		{
			uint64_t defense;
			if (!propStream.read<uint64_t>(defense))
			{
				return ATTR_READ_ERROR;
			}

			setIntAttr(ITEM_ATTRIBUTE_DEFENSE, defense);
			break;
		}

		case ATTR_EXTRADEFENSE:
		{
			uint64_t extraDefense;
			if (!propStream.read<uint64_t>(extraDefense))
			{
				return ATTR_READ_ERROR;
			}

			setIntAttr(ITEM_ATTRIBUTE_EXTRADEFENSE, extraDefense);
			break;
		}

		case ATTR_ARMOR:
		{
			uint64_t armor;
			if (!propStream.read<uint64_t>(armor))
			{
				return ATTR_READ_ERROR;
			}

			setIntAttr(ITEM_ATTRIBUTE_ARMOR, armor);
			break;
		}

		case ATTR_HITCHANCE:
		{
			uint64_t hitChance;
			if (!propStream.read<uint64_t>(hitChance))
			{
				return ATTR_READ_ERROR;
			}

			setIntAttr(ITEM_ATTRIBUTE_HITCHANCE, hitChance);
			break;
		}

		case ATTR_SHOOTRANGE:
		{
			uint64_t shootRange;
			if (!propStream.read<uint64_t>(shootRange))
			{
				return ATTR_READ_ERROR;
			}

			setIntAttr(ITEM_ATTRIBUTE_SHOOTRANGE, shootRange);
			break;
		}

		case ATTR_DECAYTO:
		{
			uint64_t decayTo;
			if (!propStream.read<uint64_t>(decayTo))
			{
				return ATTR_READ_ERROR;
			}

			setIntAttr(ITEM_ATTRIBUTE_DECAYTO, decayTo);
			break;
		}

		case ATTR_WRAPID:
		{
			uint64_t wrapId;
			if (!propStream.read<uint64_t>(wrapId))
			{
				return ATTR_READ_ERROR;
			}

			setIntAttr(ITEM_ATTRIBUTE_WRAPID, wrapId);
			break;
		}

		case ATTR_STOREITEM:
		{
			uint64_t storeItem;
			if (!propStream.read<uint64_t>(storeItem))
			{
				return ATTR_READ_ERROR;
			}

			setIntAttr(ITEM_ATTRIBUTE_STOREITEM, storeItem);
			break;
		}

		case ATTR_ACCURACY:
		{
			uint64_t accuracy;
			if (!propStream.read<uint64_t>(accuracy))
			{
				return ATTR_READ_ERROR;
			}

			setIntAttr(ITEM_ATTRIBUTE_ACCURACY, accuracy);
			break;
		}

		case ATTR_EVASION:
		{
			uint64_t evasion;
			if (!propStream.read<uint64_t>(evasion))
			{
				return ATTR_READ_ERROR;
			}

			setIntAttr(ITEM_ATTRIBUTE_EVASION, evasion);
			break;
		}
		case ATTR_RESOLVE:
		{
			uint64_t resolve;
			if (!propStream.read<uint64_t>(resolve))
			{
				return ATTR_READ_ERROR;
			}

			setIntAttr(ITEM_ATTRIBUTE_RESOLVE, resolve);
			break;
		}
		case ATTR_AGILITY:
		{
			uint64_t agility;
			if (!propStream.read<uint64_t>(agility))
			{
				return ATTR_READ_ERROR;
			}

			setIntAttr(ITEM_ATTRIBUTE_AGILITY, agility);
			break;
		}
		case ATTR_ALACRITY:
		{
			uint64_t alacrity;
			if (!propStream.read<uint64_t>(alacrity))
			{
				return ATTR_READ_ERROR;
			}

			setIntAttr(ITEM_ATTRIBUTE_ALACRITY, alacrity);
			break;
		}
		case ATTR_MAGIC:
		{
			uint64_t magic;
			if (!propStream.read<uint64_t>(magic))
			{
				return ATTR_READ_ERROR;
			}

			setIntAttr(ITEM_ATTRIBUTE_MAGIC, magic);
			break;
		}

		case ATTR_FINESSE:
		{
			uint64_t finesse;
			if (!propStream.read<uint64_t>(finesse))
			{
				return ATTR_READ_ERROR;
			}
			setIntAttr(ITEM_ATTRIBUTE_FINESSE, finesse);
			break;
		}
		case ATTR_CONCENTRATION:
		{
			uint64_t concentration;
			if (!propStream.read<uint64_t>(concentration))
			{
				return ATTR_READ_ERROR;
			}

			setIntAttr(ITEM_ATTRIBUTE_CONCENTRATION, concentration);
			break;
		}
		case ATTR_FOCUS:
		{
			uint64_t focus;
			if (!propStream.read<uint64_t>(focus))
			{
				return ATTR_READ_ERROR;
			}

			setIntAttr(ITEM_ATTRIBUTE_FOCUS, focus);
			break;
		}
		case ATTR_DISTANCE:
		{
			uint64_t distance;
			if (!propStream.read<uint64_t>(distance))
			{
				return ATTR_READ_ERROR;
			}

			setIntAttr(ITEM_ATTRIBUTE_DISTANCE, distance);
			break;
		}
		case ATTR_MELEE:
		{
			uint64_t melee;
			if (!propStream.read<uint64_t>(melee))
			{
				return ATTR_READ_ERROR;
			}

			setIntAttr(ITEM_ATTRIBUTE_MELEE, melee);
			break;
		}
		case ATTR_SHIELD:
		{
			uint64_t shield;
			if (!propStream.read<uint64_t>(shield))
			{
				return ATTR_READ_ERROR;
			}

			setIntAttr(ITEM_ATTRIBUTE_SHIELD, shield);
			break;
		}
		case ATTR_CONCOCTING:
		{
			uint64_t concocting;
			if (!propStream.read<uint64_t>(concocting))
			{
				return ATTR_READ_ERROR;
			}

			setIntAttr(ITEM_ATTRIBUTE_CONCOCTING, concocting);
			break;
		}
		case ATTR_ENCHANTING:
		{
			uint64_t enchanting;
			if (!propStream.read<uint64_t>(enchanting))
			{
				return ATTR_READ_ERROR;
			}

			setIntAttr(ITEM_ATTRIBUTE_ENCHANTING, enchanting);
			break;
		}
		case ATTR_EXPLORING:
		{
			uint64_t exploring;
			if (!propStream.read<uint64_t>(exploring))
			{
				return ATTR_READ_ERROR;
			}

			setIntAttr(ITEM_ATTRIBUTE_EXPLORING, exploring);
			break;
		}
		case ATTR_SMITHING:
		{
			uint64_t smithing;
			if (!propStream.read<uint64_t>(smithing))
			{
				return ATTR_READ_ERROR;
			}

			setIntAttr(ITEM_ATTRIBUTE_SMITHING, smithing);
			break;
		}
		case ATTR_COOKING:
		{
			uint64_t cooking;
			if (!propStream.read<uint64_t>(cooking))
			{
				return ATTR_READ_ERROR;
			}

			setIntAttr(ITEM_ATTRIBUTE_COOKING, cooking);
			break;
		}
		case ATTR_MINING:
		{
			uint64_t mining;
			if (!propStream.read<uint64_t>(mining))
			{
				return ATTR_READ_ERROR;
			}

			setIntAttr(ITEM_ATTRIBUTE_MINING, mining);
			break;
		}
		case ATTR_GATHERING:
		{
			uint64_t gathering;
			if (!propStream.read<uint64_t>(gathering))
			{
				return ATTR_READ_ERROR;
			}

			setIntAttr(ITEM_ATTRIBUTE_GATHERING, gathering);
			break;
		}
		case ATTR_SLAYING:
		{
			uint64_t slaying;
			if (!propStream.read<uint64_t>(slaying))
			{
				return ATTR_READ_ERROR;
			}

			setIntAttr(ITEM_ATTRIBUTE_SLAYING, slaying);
			break;
		}

		case ATTR_UPGRADE:
		{
			uint64_t upgrade;
			if (!propStream.read<uint64_t>(upgrade))
			{
				return ATTR_READ_ERROR;
			}

			setIntAttr(ITEM_ATTRIBUTE_UPGRADE, upgrade);
			break;
		}

		case ATTR_SLOT1:
		{
			uint64_t slot1;
			if (!propStream.read<uint64_t>(slot1))
			{
				return ATTR_READ_ERROR;
			}

			setIntAttr(ITEM_ATTRIBUTE_SLOT1, slot1);
			break;
		}
		case ATTR_SLOT1VALUE:
		{
			uint64_t slot1value;
			if (!propStream.read<uint64_t>(slot1value))
			{
				return ATTR_READ_ERROR;
			}

			setIntAttr(ITEM_ATTRIBUTE_SLOT1VALUE, slot1value);
			break;
		}

		case ATTR_SLOT2:
		{
			uint64_t slot2;
			if (!propStream.read<uint64_t>(slot2))
			{
				return ATTR_READ_ERROR;
			}

			setIntAttr(ITEM_ATTRIBUTE_SLOT2, slot2);
			break;
		}
		case ATTR_SLOT2VALUE:
		{
			uint64_t slot2value;
			if (!propStream.read<uint64_t>(slot2value))
			{
				return ATTR_READ_ERROR;
			}

			setIntAttr(ITEM_ATTRIBUTE_SLOT2VALUE, slot2value);
			break;
		}

		case ATTR_SLOT3:
		{
			uint64_t slot3;
			if (!propStream.read<uint64_t>(slot3))
			{
				return ATTR_READ_ERROR;
			}

			setIntAttr(ITEM_ATTRIBUTE_SLOT3, slot3);
			break;
		}
		case ATTR_SLOT3VALUE:
		{
			uint64_t slot3value;
			if (!propStream.read<uint64_t>(slot3value))
			{
				return ATTR_READ_ERROR;
			}

			setIntAttr(ITEM_ATTRIBUTE_SLOT3VALUE, slot3value);
			break;
		}

		case ATTR_SLOT4:
		{
			uint64_t slot4;
			if (!propStream.read<uint64_t>(slot4))
			{
				return ATTR_READ_ERROR;
			}

			setIntAttr(ITEM_ATTRIBUTE_SLOT4, slot4);
			break;
		}
		case ATTR_SLOT4VALUE:
		{
			uint64_t slot4value;
			if (!propStream.read<uint64_t>(slot4value))
			{
				return ATTR_READ_ERROR;
			}

			setIntAttr(ITEM_ATTRIBUTE_SLOT4VALUE, slot4value);
			break;
		}

		case ATTR_SLOT5:
		{
			uint64_t slot5;
			if (!propStream.read<uint64_t>(slot5))
			{
				return ATTR_READ_ERROR;
			}

			setIntAttr(ITEM_ATTRIBUTE_SLOT5, slot5);
			break;
		}
		case ATTR_SLOT5VALUE:
		{
			uint64_t slot5value;
			if (!propStream.read<uint64_t>(slot5value))
			{
				return ATTR_READ_ERROR;
			}

			setIntAttr(ITEM_ATTRIBUTE_SLOT5VALUE, slot5value);
			break;
		}

		case ATTR_MP:
		{
			uint64_t mp;
			if (!propStream.read<uint64_t>(mp))
			{
				return ATTR_READ_ERROR;
			}

			setIntAttr(ITEM_ATTRIBUTE_MP, mp);
			break;
		}
		case ATTR_CRITICALHITCHANCE:
		{
			uint64_t criticalhitchance;
			if (!propStream.read<uint64_t>(criticalhitchance))
			{
				return ATTR_READ_ERROR;
			}

			setIntAttr(ITEM_ATTRIBUTE_CRITICALHITCHANCE, criticalhitchance);
			break;
		}
		case ATTR_CRITICALHITAMOUNT:
		{
			uint64_t criticalhitamount;
			if (!propStream.read<uint64_t>(criticalhitamount))
			{
				return ATTR_READ_ERROR;
			}

			setIntAttr(ITEM_ATTRIBUTE_CRITICALHITAMOUNT, criticalhitamount);
			break;
		}
		case ATTR_MPREGEN:
		{
			uint64_t mpregen;
			if (!propStream.read<uint64_t>(mpregen))
			{
				return ATTR_READ_ERROR;
			}

			setIntAttr(ITEM_ATTRIBUTE_MPREGEN, mpregen);
			break;
		}
		case ATTR_HPREGEN:
		{
			uint64_t hpregen;
			if (!propStream.read<uint64_t>(hpregen))
			{
				return ATTR_READ_ERROR;
			}

			setIntAttr(ITEM_ATTRIBUTE_HPREGEN, hpregen);
			break;
		}
		case ATTR_HP:
		{
			uint64_t hp;
			if (!propStream.read<uint64_t>(hp))
			{
				return ATTR_READ_ERROR;
			}

			setIntAttr(ITEM_ATTRIBUTE_HP, hp);
			break;
		}

		//these should be handled through derived classes
		//If these are called then something has changed in the items.xml since the map was saved
		//just read the values

		//Depot class
		case ATTR_DEPOT_ID: {
			if (!propStream.skip(2)) {
				return ATTR_READ_ERROR;
			}
			break;
		}

		//Door class
		case ATTR_HOUSEDOORID: {
			if (!propStream.skip(1)) {
				return ATTR_READ_ERROR;
			}
			break;
		}

		//Bed class
		case ATTR_SLEEPERGUID: {
			if (!propStream.skip(4)) {
				return ATTR_READ_ERROR;
			}
			break;
		}

		case ATTR_SLEEPSTART: {
			if (!propStream.skip(4)) {
				return ATTR_READ_ERROR;
			}
			break;
		}

		//Teleport class
		case ATTR_TELE_DEST: {
			if (!propStream.skip(5)) {
				return ATTR_READ_ERROR;
			}
			break;
		}

		//Container class
		case ATTR_CONTAINER_ITEMS: {
			return ATTR_READ_ERROR;
		}

		case ATTR_CUSTOM_ATTRIBUTES: {
			uint64_t size;
			if (!propStream.read<uint64_t>(size)) {
				return ATTR_READ_ERROR;
			}

			for (uint64_t i = 0; i < size; i++) {
				// Unserialize key type and value
				std::string key;
				if (!propStream.readString(key)) {
					return ATTR_READ_ERROR;
				};

				// Unserialize value type and value
				ItemAttributes::CustomAttribute val;
				if (!val.unserialize(propStream)) {
					return ATTR_READ_ERROR;
				}

				setCustomAttribute(key, val);
			}
			break;
		}

		default:
			return ATTR_READ_ERROR;
	}

	return ATTR_READ_CONTINUE;
}

bool Item::unserializeAttr(PropStream& propStream)
{
	uint8_t attr_type;
	while (propStream.read<uint8_t>(attr_type) && attr_type != 0) {
		Attr_ReadValue ret = readAttr(static_cast<AttrTypes_t>(attr_type), propStream);
		if (ret == ATTR_READ_ERROR) {
			return false;
		}
		if (ret == ATTR_READ_END) {
			return true;
		}
	}
	return true;
}

bool Item::unserializeItemNode(OTB::Loader&, const OTB::Node&, PropStream& propStream)
{
	return unserializeAttr(propStream);
}

void Item::serializeAttr(PropWriteStream& propWriteStream) const
{
	const ItemType& it = items[id];
	if (it.stackable || it.isFluidContainer() || it.isSplash()) {
		propWriteStream.write<uint8_t>(ATTR_COUNT);
		propWriteStream.write<uint8_t>(getSubType());
	}

	uint16_t charges = getCharges();
	if (charges != 0) {
		propWriteStream.write<uint8_t>(ATTR_CHARGES);
		propWriteStream.write<uint16_t>(charges);
	}

	if (it.moveable) {
		uint16_t actionId = getActionId();
		if (actionId != 0) {
			propWriteStream.write<uint8_t>(ATTR_ACTION_ID);
			propWriteStream.write<uint16_t>(actionId);
		}
	}

	const std::string& text = getText();
	if (!text.empty()) {
		propWriteStream.write<uint8_t>(ATTR_TEXT);
		propWriteStream.writeString(text);
	}

	const time_t writtenDate = getDate();
	if (writtenDate != 0) {
		propWriteStream.write<uint8_t>(ATTR_WRITTENDATE);
		propWriteStream.write<uint32_t>(writtenDate);
	}

	const std::string& writer = getWriter();
	if (!writer.empty()) {
		propWriteStream.write<uint8_t>(ATTR_WRITTENBY);
		propWriteStream.writeString(writer);
	}

	const std::string& specialDesc = getSpecialDescription();
	if (!specialDesc.empty()) {
		propWriteStream.write<uint8_t>(ATTR_DESC);
		propWriteStream.writeString(specialDesc);
	}

	if (hasAttribute(ITEM_ATTRIBUTE_DURATION))
	{
		propWriteStream.write<uint8_t>(ATTR_DURATION);
		propWriteStream.write<uint32_t>(getIntAttr(ITEM_ATTRIBUTE_DURATION));
	}

	ItemDecayState_t decayState = getDecaying();
	if (decayState == DECAYING_TRUE || decayState == DECAYING_PENDING)
	{
		propWriteStream.write<uint8_t>(ATTR_DECAYING_STATE);
		propWriteStream.write<uint8_t>(decayState);
	}

	if (hasAttribute(ITEM_ATTRIBUTE_NAME))
	{
		propWriteStream.write<uint8_t>(ATTR_NAME);
		propWriteStream.writeString(getStrAttr(ITEM_ATTRIBUTE_NAME));
	}

	if (hasAttribute(ITEM_ATTRIBUTE_ARTICLE))
	{
		propWriteStream.write<uint8_t>(ATTR_ARTICLE);
		propWriteStream.writeString(getStrAttr(ITEM_ATTRIBUTE_ARTICLE));
	}

	if (hasAttribute(ITEM_ATTRIBUTE_PLURALNAME))
	{
		propWriteStream.write<uint8_t>(ATTR_PLURALNAME);
		propWriteStream.writeString(getStrAttr(ITEM_ATTRIBUTE_PLURALNAME));
	}

	if (hasAttribute(ITEM_ATTRIBUTE_ATTACK))
	{
		propWriteStream.write<uint8_t>(ATTR_ATTACK);
		propWriteStream.write<uint32_t>(getIntAttr(ITEM_ATTRIBUTE_ATTACK));
	}
	if (hasAttribute(ITEM_ATTRIBUTE_WEIGHT))
	{
		propWriteStream.write<uint8_t>(ATTR_WEIGHT);
		propWriteStream.write<uint32_t>(getIntAttr(ITEM_ATTRIBUTE_WEIGHT));
	}
/*uint64_t and beyond */
	if (hasAttribute(ITEM_ATTRIBUTE_DEFENSE))
	{
		propWriteStream.write<uint8_t>(ATTR_DEFENSE);
		propWriteStream.write<uint64_t>(getIntAttr(ITEM_ATTRIBUTE_DEFENSE));
	}

	if (hasAttribute(ITEM_ATTRIBUTE_EXTRADEFENSE))
	{
		propWriteStream.write<uint8_t>(ATTR_EXTRADEFENSE);
		propWriteStream.write<uint64_t>(getIntAttr(ITEM_ATTRIBUTE_EXTRADEFENSE));
	}

	if (hasAttribute(ITEM_ATTRIBUTE_ARMOR))
	{
		propWriteStream.write<uint8_t>(ATTR_ARMOR);
		propWriteStream.write<uint64_t>(getIntAttr(ITEM_ATTRIBUTE_ARMOR));
	}

	if (hasAttribute(ITEM_ATTRIBUTE_HITCHANCE))
	{
		propWriteStream.write<uint8_t>(ATTR_HITCHANCE);
		propWriteStream.write<uint64_t>(getIntAttr(ITEM_ATTRIBUTE_HITCHANCE));
	}

	if (hasAttribute(ITEM_ATTRIBUTE_SHOOTRANGE))
	{
		propWriteStream.write<uint8_t>(ATTR_SHOOTRANGE);
		propWriteStream.write<uint64_t>(getIntAttr(ITEM_ATTRIBUTE_SHOOTRANGE));
	}

	if (hasAttribute(ITEM_ATTRIBUTE_DECAYTO))
	{
		propWriteStream.write<uint8_t>(ATTR_DECAYTO);
		propWriteStream.write<uint64_t>(getIntAttr(ITEM_ATTRIBUTE_DECAYTO));
	}

	if (hasAttribute(ITEM_ATTRIBUTE_WRAPID))
	{
		propWriteStream.write<uint8_t>(ATTR_WRAPID);
		propWriteStream.write<uint64_t>(getIntAttr(ITEM_ATTRIBUTE_WRAPID));
	}

	if (hasAttribute(ITEM_ATTRIBUTE_STOREITEM))
	{
		propWriteStream.write<uint8_t>(ATTR_STOREITEM);
		propWriteStream.write<uint64_t>(getIntAttr(ITEM_ATTRIBUTE_STOREITEM));
	}

	if (hasAttribute(ITEM_ATTRIBUTE_ACCURACY))
	{
		propWriteStream.write<uint8_t>(ATTR_ACCURACY);
		propWriteStream.write<uint64_t>(getIntAttr(ITEM_ATTRIBUTE_ACCURACY));
	}

	if (hasAttribute(ITEM_ATTRIBUTE_EVASION))
	{
		propWriteStream.write<uint8_t>(ATTR_EVASION);
		propWriteStream.write<uint64_t>(getIntAttr(ITEM_ATTRIBUTE_EVASION));
	}
	if (hasAttribute(ITEM_ATTRIBUTE_RESOLVE))
	{
		propWriteStream.write<uint8_t>(ATTR_RESOLVE);
		propWriteStream.write<uint64_t>(getIntAttr(ITEM_ATTRIBUTE_RESOLVE));
	}
	if (hasAttribute(ITEM_ATTRIBUTE_AGILITY))
	{
		propWriteStream.write<uint8_t>(ATTR_AGILITY);
		propWriteStream.write<uint64_t>(getIntAttr(ITEM_ATTRIBUTE_AGILITY));
	}
	if (hasAttribute(ITEM_ATTRIBUTE_ALACRITY))
	{
		propWriteStream.write<uint8_t>(ATTR_ALACRITY);
		propWriteStream.write<uint64_t>(getIntAttr(ITEM_ATTRIBUTE_ALACRITY));
	}
	if (hasAttribute(ITEM_ATTRIBUTE_MAGIC))
	{
		propWriteStream.write<uint8_t>(ATTR_MAGIC);
		propWriteStream.write<uint64_t>(getIntAttr(ITEM_ATTRIBUTE_MAGIC));
	}
	
	if (hasAttribute(ITEM_ATTRIBUTE_FINESSE))
	{
		propWriteStream.write<uint8_t>(ATTR_FINESSE);
		propWriteStream.write<uint64_t>(getIntAttr(ITEM_ATTRIBUTE_FINESSE));
	}
	if (hasAttribute(ITEM_ATTRIBUTE_CONCENTRATION))
	{
		propWriteStream.write<uint8_t>(ATTR_CONCENTRATION);
		propWriteStream.write<uint64_t>(getIntAttr(ITEM_ATTRIBUTE_CONCENTRATION));
	}
	if (hasAttribute(ITEM_ATTRIBUTE_FOCUS))
	{
		propWriteStream.write<uint8_t>(ATTR_FOCUS);
		propWriteStream.write<uint64_t>(getIntAttr(ITEM_ATTRIBUTE_FOCUS));
	}
	if (hasAttribute(ITEM_ATTRIBUTE_DISTANCE))
	{
		propWriteStream.write<uint8_t>(ATTR_DISTANCE);
		propWriteStream.write<uint64_t>(getIntAttr(ITEM_ATTRIBUTE_DISTANCE));
	}
	if (hasAttribute(ITEM_ATTRIBUTE_MELEE))
	{
		propWriteStream.write<uint8_t>(ATTR_MELEE);
		propWriteStream.write<uint64_t>(getIntAttr(ITEM_ATTRIBUTE_MELEE));
	}
	if (hasAttribute(ITEM_ATTRIBUTE_SHIELD))
	{
		propWriteStream.write<uint8_t>(ATTR_SHIELD);
		propWriteStream.write<uint64_t>(getIntAttr(ITEM_ATTRIBUTE_SHIELD));
	}
	if (hasAttribute(ITEM_ATTRIBUTE_CONCOCTING))
	{
		propWriteStream.write<uint8_t>(ATTR_CONCOCTING);
		propWriteStream.write<uint64_t>(getIntAttr(ITEM_ATTRIBUTE_CONCOCTING));
	}
	if (hasAttribute(ITEM_ATTRIBUTE_ENCHANTING))
	{
		propWriteStream.write<uint8_t>(ATTR_ENCHANTING);
		propWriteStream.write<uint64_t>(getIntAttr(ITEM_ATTRIBUTE_ENCHANTING));
	}
	if (hasAttribute(ITEM_ATTRIBUTE_EXPLORING))
	{
		propWriteStream.write<uint8_t>(ATTR_EXPLORING);
		propWriteStream.write<uint64_t>(getIntAttr(ITEM_ATTRIBUTE_EXPLORING));
	}
	if (hasAttribute(ITEM_ATTRIBUTE_SMITHING))
	{
		propWriteStream.write<uint8_t>(ATTR_SMITHING);
		propWriteStream.write<uint64_t>(getIntAttr(ITEM_ATTRIBUTE_SMITHING));
	}
	if (hasAttribute(ITEM_ATTRIBUTE_COOKING))
	{
		propWriteStream.write<uint8_t>(ATTR_COOKING);
		propWriteStream.write<uint64_t>(getIntAttr(ITEM_ATTRIBUTE_COOKING));
	}
	if (hasAttribute(ITEM_ATTRIBUTE_MINING))
	{
		propWriteStream.write<uint8_t>(ATTR_MINING);
		propWriteStream.write<uint64_t>(getIntAttr(ITEM_ATTRIBUTE_MINING));
	}
	if (hasAttribute(ITEM_ATTRIBUTE_GATHERING))
	{
		propWriteStream.write<uint8_t>(ATTR_GATHERING);
		propWriteStream.write<uint64_t>(getIntAttr(ITEM_ATTRIBUTE_GATHERING));
	}

	if (hasAttribute(ITEM_ATTRIBUTE_SLAYING))
	{
		propWriteStream.write<uint8_t>(ATTR_SLAYING);
		propWriteStream.write<uint64_t>(getIntAttr(ITEM_ATTRIBUTE_SLAYING));
	}

	if (hasAttribute(ITEM_ATTRIBUTE_SLOT1))
	{
		propWriteStream.write<uint8_t>(ATTR_SLOT1);
		propWriteStream.write<uint64_t>(getIntAttr(ITEM_ATTRIBUTE_SLOT1));
	}
	if (hasAttribute(ITEM_ATTRIBUTE_SLOT1VALUE))
	{
		propWriteStream.write<uint8_t>(ATTR_SLOT1VALUE);
		propWriteStream.write<uint64_t>(getIntAttr(ITEM_ATTRIBUTE_SLOT1VALUE));
	}

	if (hasAttribute(ITEM_ATTRIBUTE_SLOT2))
	{
		propWriteStream.write<uint8_t>(ATTR_SLOT1);
		propWriteStream.write<uint64_t>(getIntAttr(ITEM_ATTRIBUTE_SLOT2));
	}
	if (hasAttribute(ITEM_ATTRIBUTE_SLOT2VALUE))
	{
		propWriteStream.write<uint8_t>(ATTR_SLOT2VALUE);
		propWriteStream.write<uint64_t>(getIntAttr(ITEM_ATTRIBUTE_SLOT2VALUE));
	}

	if (hasAttribute(ITEM_ATTRIBUTE_SLOT3))
	{
		propWriteStream.write<uint8_t>(ATTR_SLOT3);
		propWriteStream.write<uint64_t>(getIntAttr(ITEM_ATTRIBUTE_SLOT3));
	}
	if (hasAttribute(ITEM_ATTRIBUTE_SLOT3VALUE))
	{
		propWriteStream.write<uint8_t>(ATTR_SLOT3VALUE);
		propWriteStream.write<uint64_t>(getIntAttr(ITEM_ATTRIBUTE_SLOT3VALUE));
	}

	if (hasAttribute(ITEM_ATTRIBUTE_SLOT4))
	{
		propWriteStream.write<uint8_t>(ATTR_SLOT4);
		propWriteStream.write<uint64_t>(getIntAttr(ITEM_ATTRIBUTE_SLOT4));
	}
	if (hasAttribute(ITEM_ATTRIBUTE_SLOT4VALUE))
	{
		propWriteStream.write<uint8_t>(ATTR_SLOT4VALUE);
		propWriteStream.write<uint64_t>(getIntAttr(ITEM_ATTRIBUTE_SLOT4VALUE));
	}

	if (hasAttribute(ITEM_ATTRIBUTE_SLOT5))
	{
		propWriteStream.write<uint8_t>(ATTR_SLOT5);
		propWriteStream.write<uint64_t>(getIntAttr(ITEM_ATTRIBUTE_SLOT5));
	}
	if (hasAttribute(ITEM_ATTRIBUTE_SLOT5VALUE))
	{
		propWriteStream.write<uint8_t>(ATTR_SLOT5VALUE);
		propWriteStream.write<uint64_t>(getIntAttr(ITEM_ATTRIBUTE_SLOT5VALUE));
	}

	if (hasAttribute(ITEM_ATTRIBUTE_CRITICALHITCHANCE))
	{
		propWriteStream.write<uint8_t>(ATTR_CRITICALHITCHANCE);
		propWriteStream.write<uint64_t>(getIntAttr(ITEM_ATTRIBUTE_CRITICALHITCHANCE));
	}
	if (hasAttribute(ITEM_ATTRIBUTE_CRITICALHITAMOUNT))
	{
		propWriteStream.write<uint8_t>(ATTR_CRITICALHITAMOUNT);
		propWriteStream.write<uint64_t>(getIntAttr(ITEM_ATTRIBUTE_CRITICALHITAMOUNT));
	}
	if (hasAttribute(ITEM_ATTRIBUTE_MPREGEN))
	{
		propWriteStream.write<uint8_t>(ATTR_MPREGEN);
		propWriteStream.write<uint64_t>(getIntAttr(ITEM_ATTRIBUTE_MPREGEN));
	}
	if (hasAttribute(ITEM_ATTRIBUTE_HPREGEN))
	{
		propWriteStream.write<uint8_t>(ATTR_HPREGEN);
		propWriteStream.write<uint64_t>(getIntAttr(ITEM_ATTRIBUTE_HPREGEN));
	}
	if (hasAttribute(ITEM_ATTRIBUTE_HP))
	{
		propWriteStream.write<uint8_t>(ATTR_HP);
		propWriteStream.write<uint64_t>(getIntAttr(ITEM_ATTRIBUTE_HP));
	}
	if (hasAttribute(ITEM_ATTRIBUTE_MP))
	{
		propWriteStream.write<uint8_t>(ATTR_MP);
		propWriteStream.write<uint64_t>(getIntAttr(ITEM_ATTRIBUTE_MP));
	}

	if (hasAttribute(ITEM_ATTRIBUTE_CUSTOM)) {
		const ItemAttributes::CustomAttributeMap* customAttrMap = attributes->getCustomAttributeMap();
		propWriteStream.write<uint8_t>(ATTR_CUSTOM_ATTRIBUTES);
		propWriteStream.write<uint64_t>(static_cast<uint64_t>(customAttrMap->size()));
		for (const auto &entry : *customAttrMap) {
			// Serializing key type and value
			propWriteStream.writeString(entry.first);

			// Serializing value type and value
			entry.second.serialize(propWriteStream);
		}
	}
}

bool Item::hasProperty(ITEMPROPERTY prop) const
{
	const ItemType& it = items[id];
	switch (prop) {
		case CONST_PROP_BLOCKSOLID: return it.blockSolid;
		case CONST_PROP_MOVEABLE: return it.moveable && !hasAttribute(ITEM_ATTRIBUTE_UNIQUEID);
		case CONST_PROP_HASHEIGHT: return it.hasHeight;
		case CONST_PROP_BLOCKPROJECTILE: return it.blockProjectile;
		case CONST_PROP_BLOCKPATH: return it.blockPathFind;
		case CONST_PROP_ISVERTICAL: return it.isVertical;
		case CONST_PROP_ISHORIZONTAL: return it.isHorizontal;
		case CONST_PROP_IMMOVABLEBLOCKSOLID: return it.blockSolid && (!it.moveable || hasAttribute(ITEM_ATTRIBUTE_UNIQUEID));
		case CONST_PROP_IMMOVABLEBLOCKPATH: return it.blockPathFind && (!it.moveable || hasAttribute(ITEM_ATTRIBUTE_UNIQUEID));
		case CONST_PROP_IMMOVABLENOFIELDBLOCKPATH: return !it.isMagicField() && it.blockPathFind && (!it.moveable || hasAttribute(ITEM_ATTRIBUTE_UNIQUEID));
		case CONST_PROP_NOFIELDBLOCKPATH: return !it.isMagicField() && it.blockPathFind;
		case CONST_PROP_SUPPORTHANGABLE: return it.isHorizontal || it.isVertical;
		default: return false;
	}
}

uint32_t Item::getWeight() const
{
	uint32_t weight = getBaseWeight();
	if (isStackable()) {
		return weight * std::max<uint32_t>(1, getItemCount());
	}
	return weight;
}

std::string Item::getDescription(const ItemType& it, int32_t lookDistance,
                                 const Item* item /*= nullptr*/, int32_t subType /*= -1*/, bool addArticle /*= true*/)
{
	const std::string* text = nullptr;

	std::ostringstream s;
	s << getNameDescription(it, item, subType, addArticle);

	if (item) {
		subType = item->getSubType();
	}

	if (it.isRune()) {
		if (it.runeLevel > 0 || it.runeMagLevel > 0) {
			if (RuneSpell* rune = g_spells->getRuneSpell(it.id)) {
				int32_t tmpSubType = subType;
				if (item) {
					tmpSubType = item->getSubType();
				}
				s << ". " << (it.stackable && tmpSubType > 1 ? "They" : "It") << " can only be used by ";

				const VocSpellMap& vocMap = rune->getVocMap();
				std::vector<Vocation*> showVocMap;

				// vocations are usually listed with the unpromoted and promoted version, the latter being
				// hidden from description, so `total / 2` is most likely the amount of vocations to be shown.
				showVocMap.reserve(vocMap.size() / 2);
				for (const auto& voc : vocMap) {
					if (voc.second) {
						showVocMap.push_back(g_vocations.getVocation(voc.first));
					}
				}

				if (!showVocMap.empty()) {
					auto vocIt = showVocMap.begin(), vocLast = (showVocMap.end() - 1);
					while (vocIt != vocLast) {
						s << asLowerCaseString((*vocIt)->getVocName()) << "s";
						if (++vocIt == vocLast) {
							s << " and ";
						} else {
							s << ", ";
						}
					}
					s << asLowerCaseString((*vocLast)->getVocName()) << "s";
				} else {
					s << "players";
				}

				s << " with";

				if (it.runeLevel > 0) {
					s << " level " << it.runeLevel;
				}

				if (it.runeMagLevel > 0) {
					if (it.runeLevel > 0) {
						s << " and";
					}

					s << " magic level " << it.runeMagLevel;
				}

				s << " or higher";
			}
		}
	} 
	//it is a weapon cause it has a weapon type given to it in items.xml
	else if (it.weaponType != WEAPON_NONE) 
	{
		bool begin = true;
		/*if (it.weaponType == WEAPON_DISTANCE && it.ammoType != AMMO_NONE) {
			s << " (Range:" << static_cast<uint16_t>(item ? item->getShootRange() : it.shootRange);

			int32_t attack;
			int8_t hitChance;
			if (item) {
				attack = item->getAttack();
				hitChance = item->getHitChance();
			} else {
				attack = it.attack;
				hitChance = it.hitChance;
			}

			if (attack != 0) {
				s << ", Atk" << std::showpos << attack << std::noshowpos;
			}

			if (hitChance != 0) {
				s << ", Hit%" << std::showpos << static_cast<int16_t>(hitChance) << std::noshowpos;
			}

			begin = false;
		} */
		if (it.weaponType != WEAPON_AMMO) 
		{

			int32_t attack, defense, extraDefense, accuracy, evasion, resolve, agility, alacrity, magic, finesse, concentration, focus, armour, shield, distance, melee, concocting, enchanting, exploring, smithing, cooking, mining, gathering, slaying, criticalhitchance, criticalhitamount, mpregen, hpregen, mp, hp, upgrade, slot1, slot1value, slot2, slot2value, slot3, slot3value, slot4, slot4value, slot5, slot5value, range, hitchance;
			if (item)
			{
				attack = item->getAttack();
				defense = item->getDefense();
				extraDefense = item->getExtraDefense();
				range = item->getShootRange();
				//hitchance = item->getHitChance();

				accuracy = item->getAccuracy();
				evasion = item->getEvasion();
				resolve = item->getResolve();
				agility = item->getAgility();
				alacrity = item->getAlacrity();
				magic = item->getMagic();
				finesse = item->getFinesse();
				concentration = item->getConcentration();
				focus = item->getFocus();
				armour = item->getArmor();
				shield = item->getShield();
				distance = item->getDistance();
				melee = item->getMelee();
				concocting = item->getConcocting();
				enchanting = item->getEnchanting();
				exploring = item->getExploring();
				smithing = item->getSmithing();
				cooking = item->getCooking();
				mining = item->getMining();
				gathering = item->getGathering();
				slaying = item->getSlaying();
				criticalhitchance = item->getCRITICALHITCHANCE();
				criticalhitamount = item->getCRITICALHITAMOUNT();
				mpregen = item->getMPREGEN();
				hpregen = item->getHPREGEN();
				mp = item->getMP();
				hp = item->getHP();
				upgrade = item->getUpgrade();
				slot1 = item->getSlot1();
				slot1value = item->getSlot1Value();
				slot2 = item->getSlot2();
				slot2value = item->getSlot2Value();
				slot3 = item->getSlot3();
				slot3value = item->getSlot3Value();
				slot4 = item->getSlot4();
				slot4value = item->getSlot4Value();
				slot5 = item->getSlot5();
				slot5value = item->getSlot5Value();
			}
		
			else
			{
				attack = it.attack;
				defense = it.defense;
				extraDefense = it.extraDefense;
				range = it.shootRange;
				//hitchance = it.hitChance;
				accuracy = it.accuracy;	
				evasion = it.evasion;
				resolve = it.resolve;
				agility = it.agility;
				alacrity = it.alacrity;
				magic = it.magic;
				finesse = it.finesse;
				concentration = it.concentration;
				focus = it.focus;
				armour = it.armor;
				shield = it.shield;
				distance = it.distance;
				melee = it.melee;
				concocting = it.concocting;
				enchanting = it.enchanting;
				exploring = it.exploring;
				smithing = it.smithing;
				cooking = it.cooking;
				mining = it.mining;
				gathering = it.gathering;
				slaying = it.slaying;
				criticalhitchance = it.criticalhitchance;
				criticalhitamount = it.criticalhitamount;
				mpregen = it.mpregen;
				hpregen = it.hpregen;
				mp = it.mp;
				hp = it.hp;
				upgrade = it.upgrade;
				slot1 = it.slot1;
				slot1value = it.slot1value;
				slot2 = it.slot2;
				slot2value = it.slot2value;
				slot3 = it.slot3;
				slot3value = it.slot3value;
				slot4 = it.slot4;
				slot4value = it.slot4value;
				slot5 = it.slot5;
				slot5value = it.slot5value;
			}

			if (attack != 0) {
				begin = false;
				s << " \nAtk: " << attack;

				if (it.abilities && it.abilities->elementType != COMBAT_NONE && it.abilities->elementDamage != 0) {
					s << " physical + " << it.abilities->elementDamage << ' ' << getCombatName(it.abilities->elementType);
				}
			}

			if (defense != 0 || extraDefense != 0) {
				if (begin) {
					begin = false;
					s << "\n";
				} else {
					s << "\n";
				}

				s << "Def: " << defense;
				if (extraDefense != 0) {
					s << ' ' << std::showpos << extraDefense << std::noshowpos;
				}
			}
			if (range != 0)
			{

				s << "\nRange: " << range;
			}
			if (accuracy != 0)
			{

				s << "\nAccuracy: " << accuracy;
			}
			if (evasion != 0)
			{

				s << "\nEvasion: " << evasion;
			}
			if (resolve != 0)
			{

				s << "\nResolve: " << resolve;
			}
			if (agility != 0)
			{

				s << "\nAgility: " << agility;
			}
			if (alacrity != 0)
			{

				s << "\nAlacrity: " << alacrity;
			}
			if (magic != 0)
			{

				s << "\nMagic: " << magic;
			}
			if (finesse != 0)
			{

				s << "\nFinesse: " << finesse;
			}
			if (concentration != 0)
			{

				s << "\nConcentration: " << concentration;
			}
			if (focus != 0)
			{

				s << "\nFocus: " << focus;
			}
			if (armour != 0)
			{

				s << "\nArmour: " << armour;
			}
			if (shield != 0)
			{

				s << "\nShield: " << shield;
			}
			if (distance != 0)
			{

				s << "\nDistance: " << distance;
			}
			if (melee != 0)
			{

				s << "\nMelee: " << melee;
			}
			if (concocting != 0)
			{

				s << "\nConcocting: " << concocting;
			}
			if (enchanting != 0)
			{

				s << "\nEnchanting: " << enchanting;
			}
			if (exploring != 0)
			{

				s << "\nExploring: " << exploring;
			}
			if (smithing != 0)
			{

				s << "\nSmithing: " << smithing;
			}
			if (cooking != 0)
			{

				s << "\nCooking: " << cooking;
			}
			if (mining != 0)
			{

				s << "\nMining: " << mining;
			}
			if (gathering != 0)
			{

				s << "\nGathering: " << gathering;
			}
			if (slaying != 0)
			{

				s << "\nSlaying: " << slaying;
			}
			if (criticalhitchance != 0)
			{

				s << "\nCritHit%: " << criticalhitchance;
			}
			if (criticalhitamount != 0)
			{

				s << "\nCritDmg%: " << criticalhitamount;
			}
			if (mpregen != 0)
			{

				s << "\nMP Regen: " << mpregen;
			}
			if (hpregen != 0)
			{

				s << "\nHP Regen: " << hpregen;
			}
			if (mp != 0)
			{

				s << "\nMP: " << mp;
			}
			if (hp != 0)
			{

				s << "\nHP: " << hp;
			}
			if (upgrade != 0)
			{

				s << "\nUpgrade: " << upgrade;
			}
			if (slot1 != 0)
			{

				s << "\nSlot 1: " << slot1;
			}
			if (slot1value != 0)
			{

				s << "\nSlot 1 Value: " << slot1value;
			}
			if (slot2 != 0)
			{

				s << "\nSlot 2: " << slot2;
			}
			if (slot2value != 0)
			{

				s << "\nSlot 2 Value: " << slot2value;
			}
			if (slot3 != 0)
			{

				s << "\nSlot 3: " << slot3;
			}
			if (slot3value != 0)
			{

				s << "\nSlot 3 Value: " << slot3value;
			}
			if (slot4 != 0)
			{

				s << "\nSlot 4: " << slot4;
			}
			if (slot4value != 0)
			{

				s << "\nSlot 4 Value: " << slot4value;
			}
			if (slot5 != 0)
			{

				s << "\nSlot 5: " << slot5;
			}
			if (slot5value != 0)
			{

				s << "\nSlot 5 Value: " << slot5value;
			}
		}//END OF WEAPON TYPES 

		if (it.abilities) 
		{
			/*for (uint8_t i = SKILL_FIRST; i <= SKILL_LAST; i++) {
				if (!it.abilities->skills[i]) {
					continue;
				}

				if (begin) {
					begin = false;
					s << " (";
				} else {
					s << ", ";
				}

				s << getSkillName(i) << ' ' << std::showpos << it.abilities->skills[i] << std::noshowpos;
			}*/

			/*for (uint8_t i = SPECIALSKILL_FIRST; i <= SPECIALSKILL_LAST; i++) {
				if (!it.abilities->specialSkills[i]) {
					continue;
				}

				if (begin) {
					begin = false;
					s << " (";
				} else {
					s << ", ";
				}

				s << getSpecialSkillName(i) << ' ' << std::showpos << it.abilities->specialSkills[i] << '%' << std::noshowpos;
			}*/

			/*if (it.abilities->stats[STAT_MAGICPOINTS]) {
				if (begin) {
					begin = false;
					s << " (";
				} else {
					s << ", ";
				}

				s << "magic level " << std::showpos << it.abilities->stats[STAT_MAGICPOINTS] << std::noshowpos;
			}*/

			int16_t show = it.abilities->absorbPercent[0];
			if (show != 0) {
				for (size_t i = 1; i < COMBAT_COUNT; ++i) {
					if (it.abilities->absorbPercent[i] != show) {
						show = 0;
						break;
					}
				}
			}

			if (show == 0) {
				bool tmp = true;

				for (size_t i = 0; i < COMBAT_COUNT; ++i) {
					if (it.abilities->absorbPercent[i] == 0) {
						continue;
					}

					if (tmp) {
						tmp = false;

						if (begin) {
							begin = false;
							s << " (";
						} else {
							s << ", ";
						}

						s << "protection ";
					} else {
						s << ", ";
					}

					s << getCombatName(indexToCombatType(i)) << ' ' << std::showpos << it.abilities->absorbPercent[i] << std::noshowpos << '%';
				}
			} else {
				if (begin) {
					begin = false;
					s << " (";
				} else {
					s << ", ";
				}

				s << "protection all " << std::showpos << show << std::noshowpos << '%';
			}

			show = it.abilities->fieldAbsorbPercent[0];
			if (show != 0) {
				for (size_t i = 1; i < COMBAT_COUNT; ++i) {
					if (it.abilities->absorbPercent[i] != show) {
						show = 0;
						break;
					}
				}
			}

			if (show == 0) {
				bool tmp = true;

				for (size_t i = 0; i < COMBAT_COUNT; ++i) {
					if (it.abilities->fieldAbsorbPercent[i] == 0) {
						continue;
					}

					if (tmp) {
						tmp = false;

						if (begin) {
							begin = false;
							s << " (";
						} else {
							s << ", ";
						}

						s << "protection ";
					} else {
						s << ", ";
					}

					s << getCombatName(indexToCombatType(i)) << " field " << std::showpos << it.abilities->fieldAbsorbPercent[i] << std::noshowpos << '%';
				}
			} else {
				if (begin) {
					begin = false;
					s << " (";
				} else {
					s << ", ";
				}

				s << "protection all fields " << std::showpos << show << std::noshowpos << '%';
			}

			if (it.abilities->speed) {
				if (begin) {
					begin = false;
					s << " (";
				} else {
					s << ", ";
				}

				s << "speed " << std::showpos << (it.abilities->speed >> 1) << std::noshowpos;
			}
		}

		/*if (!begin) {
			s << ')';
		}*/
	} 
	else if (it.armor != 0 || (item && item->getArmor() != 0) || it.showAttributes) 
	{
		bool begin = true;

		int32_t armor = (item ? item->getArmor() : it.armor);
		if (armor != 0) {
			s << " (Arm:" << armor;
			begin = false;
		}

		if (it.abilities) {
			for (uint8_t i = SKILL_FIRST; i <= SKILL_LAST; i++) {
				if (!it.abilities->skills[i]) {
					continue;
				}

				if (begin) {
					begin = false;
					s << " (";
				} else {
					s << ", ";
				}

				s << getSkillName(i) << ' ' << std::showpos << it.abilities->skills[i] << std::noshowpos;
			}

			if (it.abilities->stats[STAT_MAGICPOINTS]) {
				if (begin) {
					begin = false;
					s << " (";
				} else {
					s << ", ";
				}

				s << "magic level " << std::showpos << it.abilities->stats[STAT_MAGICPOINTS] << std::noshowpos;
			}

			int16_t show = it.abilities->absorbPercent[0];
			if (show != 0) {
				for (size_t i = 1; i < COMBAT_COUNT; ++i) {
					if (it.abilities->absorbPercent[i] != show) {
						show = 0;
						break;
					}
				}
			}

			if (!show) {
				bool protectionBegin = true;
				for (size_t i = 0; i < COMBAT_COUNT; ++i) {
					if (it.abilities->absorbPercent[i] == 0) {
						continue;
					}

					if (protectionBegin) {
						protectionBegin = false;

						if (begin) {
							begin = false;
							s << " (";
						} else {
							s << ", ";
						}

						s << "protection ";
					} else {
						s << ", ";
					}

					s << getCombatName(indexToCombatType(i)) << ' ' << std::showpos << it.abilities->absorbPercent[i] << std::noshowpos << '%';
				}
			} else {
				if (begin) {
					begin = false;
					s << " (";
				} else {
					s << ", ";
				}

				s << "protection all " << std::showpos << show << std::noshowpos << '%';
			}

			show = it.abilities->fieldAbsorbPercent[0];
			if (show != 0) {
				for (size_t i = 1; i < COMBAT_COUNT; ++i) {
					if (it.abilities->absorbPercent[i] != show) {
						show = 0;
						break;
					}
				}
			}

			if (!show) {
				bool tmp = true;

				for (size_t i = 0; i < COMBAT_COUNT; ++i) {
					if (it.abilities->fieldAbsorbPercent[i] == 0) {
						continue;
					}

					if (tmp) {
						tmp = false;

						if (begin) {
							begin = false;
							s << " (";
						} else {
							s << ", ";
						}

						s << "protection ";
					} else {
						s << ", ";
					}

					s << getCombatName(indexToCombatType(i)) << " field " << std::showpos << it.abilities->fieldAbsorbPercent[i] << std::noshowpos << '%';
				}
			} else {
				if (begin) {
					begin = false;
					s << " (";
				} else {
					s << ", ";
				}

				s << "protection all fields " << std::showpos << show << std::noshowpos << '%';
			}

			if (it.abilities->speed) {
				if (begin) {
					begin = false;
					s << " (";
				} else {
					s << ", ";
				}

				s << "speed " << std::showpos << (it.abilities->speed >> 1) << std::noshowpos;
			}
		}

		if (!begin) {
			s << ')';
		}
	} else if (it.isContainer() || (item && item->getContainer())) {
		uint32_t volume = 0;
		if (!item || !item->hasAttribute(ITEM_ATTRIBUTE_UNIQUEID)) {
			if (it.isContainer()) {
				volume = it.maxItems;
			} else {
				volume = item->getContainer()->capacity();
			}
		}

		if (volume != 0) {
			s << " (Vol:" << volume << ')';
		}
	} else {
		bool found = true;

		if (it.abilities) {
			if (it.abilities->speed > 0) {
				s << " (speed " << std::showpos << (it.abilities->speed / 2) << std::noshowpos << ')';
			} else if (hasBitSet(CONDITION_DRUNK, it.abilities->conditionSuppressions)) {
				s << " (hard drinking)";
			} else if (it.abilities->invisible) {
				s << " (invisibility)";
			} else if (it.abilities->regeneration) {
				s << " (faster regeneration)";
			} else if (it.abilities->manaShield) {
				s << " (mana shield)";
			} else {
				found = false;
			}
		} else {
			found = false;
		}

		if (!found) {
			if (it.isKey()) {
				int32_t keyNumber = (item ? item->getActionId() : 0);
				if (keyNumber != 0) {
					s << " (Key:" << std::setfill('0') << std::setw(4) << keyNumber << ')';
				}
			} else if (it.isFluidContainer()) {
				if (subType > 0) {
					const std::string& itemName = items[subType].name;
					s << " of " << (!itemName.empty() ? itemName : "unknown");
				} else {
					s << ". It is empty";
				}
			} else if (it.isSplash()) {
				s << " of ";

				if (subType > 0 && !items[subType].name.empty()) {
					s << items[subType].name;
				} else {
					s << "unknown";
				}
			} else if (it.allowDistRead && (it.id < 7369 || it.id > 7371)) {
				s << ".\n";

				if (lookDistance <= 4) {
					if (item) {
						text = &item->getText();
						if (!text->empty()) {
							const std::string& writer = item->getWriter();
							if (!writer.empty()) {
								s << writer << " wrote";
								time_t date = item->getDate();
								if (date != 0) {
									s << " on " << formatDateShort(date);
								}
								s << ": ";
							} else {
								s << "You read: ";
							}
							s << *text;
						} else {
							s << "Nothing is written on it";
						}
					} else {
						s << "Nothing is written on it";
					}
				} else {
					s << "You are too far away to read it";
				}
			} else if (it.levelDoor != 0 && item) {
				uint16_t actionId = item->getActionId();
				if (actionId >= it.levelDoor) {
					s << " for level " << (actionId - it.levelDoor);
				}
			}
		}
	}

	if (it.showCharges) {
		s << " that has " << subType << " charge" << (subType != 1 ? "s" : "") << " left";
	}

	if (it.showDuration) {
		if (item && item->hasAttribute(ITEM_ATTRIBUTE_DURATION)) {
			uint32_t duration = item->getDuration() / 1000;
			s << " that will expire in ";

			if (duration >= 86400) {
				uint16_t days = duration / 86400;
				uint16_t hours = (duration % 86400) / 3600;
				s << days << " day" << (days != 1 ? "s" : "");

				if (hours > 0) {
					s << " and " << hours << " hour" << (hours != 1 ? "s" : "");
				}
			} else if (duration >= 3600) {
				uint16_t hours = duration / 3600;
				uint16_t minutes = (duration % 3600) / 60;
				s << hours << " hour" << (hours != 1 ? "s" : "");

				if (minutes > 0) {
					s << " and " << minutes << " minute" << (minutes != 1 ? "s" : "");
				}
			} else if (duration >= 60) {
				uint16_t minutes = duration / 60;
				s << minutes << " minute" << (minutes != 1 ? "s" : "");
				uint16_t seconds = duration % 60;

				if (seconds > 0) {
					s << " and " << seconds << " second" << (seconds != 1 ? "s" : "");
				}
			} else {
				s << duration << " second" << (duration != 1 ? "s" : "");
			}
		} else {
			s << " that is brand-new";
		}
	}

	if (!it.allowDistRead || (it.id >= 7369 && it.id <= 7371)) {
		//s << '.';
	} else {
		if (!text && item) {
			text = &item->getText();
		}

		if (!text || text->empty()) {
			//s << '.';
		}
	}

	if (it.wieldInfo != 0) {
		s << "\nIt can only be wielded properly by ";

		if (it.wieldInfo & WIELDINFO_PREMIUM) {
			s << "premium ";
		}

		if (!it.vocationString.empty()) {
			s << it.vocationString;
		} else {
			s << "players";
		}

		if (it.wieldInfo & WIELDINFO_LEVEL) {
			s << " of level " << it.minReqLevel << " or higher";
		}

		if (it.wieldInfo & WIELDINFO_MAGLV) {
			if (it.wieldInfo & WIELDINFO_LEVEL) {
				s << " and";
			} else {
				s << " of";
			}

			s << " magic level " << it.minReqMagicLevel << " or higher";
		}

		s << '.';
	}

	if (lookDistance <= 1) {
		if (item) {
			const uint32_t weight = item->getWeight();
			if (weight != 0 && it.pickupable) {
				s << '\n' << getWeightDescription(it, weight, item->getItemCount());
			}
		} else if (it.weight != 0 && it.pickupable) {
			s << '\n' << getWeightDescription(it, it.weight);
		}
	}

	if (item) {
		const std::string& specialDescription = item->getSpecialDescription();
		if (!specialDescription.empty()) {
			s << '\n' << specialDescription;
		} else if (lookDistance <= 1 && !it.description.empty()) {
			s << '\n' << it.description;
		}
	} else if (lookDistance <= 1 && !it.description.empty()) {
		s << '\n' << it.description;
	}

	if (it.allowDistRead && it.id >= 7369 && it.id <= 7371) {
		if (!text && item) {
			text = &item->getText();
		}

		if (text && !text->empty()) {
			s << '\n' << *text;
		}
	}
	return s.str();
}

std::string Item::getDescription(int32_t lookDistance) const
{
	const ItemType& it = items[id];
	return getDescription(it, lookDistance, this);
}

std::string Item::getNameDescription(const ItemType& it, const Item* item /*= nullptr*/, int32_t subType /*= -1*/, bool addArticle /*= true*/)
{
	if (item) {
		subType = item->getSubType();
	}

	std::ostringstream s;

	const std::string& name = (item ? item->getName() : it.name);
	if (!name.empty()) {
		if (it.stackable && subType > 1) {
			if (it.showCount) {
				s << subType << ' ';
			}

			s << (item ? item->getPluralName() : it.getPluralName());
		} else {
			if (addArticle) {
				const std::string& article = (item ? item->getArticle() : it.article);
				if (!article.empty()) {
					s << article << ' ';
				}
			}

			s << name;
		}
	} else {
		if (addArticle) {
			s << "an ";
		}
		s << "item of type " << it.id;
	}
	return s.str();
}

std::string Item::getNameDescription() const
{
	const ItemType& it = items[id];
	return getNameDescription(it, this);
}

std::string Item::getWeightDescription(const ItemType& it, uint32_t weight, uint32_t count /*= 1*/)
{
	std::ostringstream ss;
	if (it.stackable && count > 1 && it.showCount != 0) {
		ss << "They weigh ";
	} else {
		ss << "It weighs ";
	}

	if (weight < 10) {
		ss << "0.0" << weight;
	} else if (weight < 100) {
		ss << "0." << weight;
	} else {
		std::string weightString = std::to_string(weight);
		weightString.insert(weightString.end() - 2, '.');
		ss << weightString;
	}

	ss << " oz.";
	return ss.str();
}

std::string Item::getWeightDescription(uint32_t weight) const
{
	const ItemType& it = Item::items[id];
	return getWeightDescription(it, weight, getItemCount());
}

std::string Item::getWeightDescription() const
{
	uint32_t weight = getWeight();
	if (weight == 0) {
		return std::string();
	}
	return getWeightDescription(weight);
}

void Item::setUniqueId(uint16_t n)
{
	if (hasAttribute(ITEM_ATTRIBUTE_UNIQUEID)) {
		return;
	}

	if (g_game.addUniqueItem(n, this)) {
		getAttributes()->setUniqueId(n);
	}
}

bool Item::canDecay() const
{
	if (isRemoved()) {
		return false;
	}

	const ItemType& it = Item::items[id];
	if (getDecayTo() < 0 || it.decayTime == 0) {
		return false;
	}

	if (hasAttribute(ITEM_ATTRIBUTE_UNIQUEID)) {
		return false;
	}

	return true;
}

uint32_t Item::getWorth() const
{
	switch (id) {
		case ITEM_GOLD_COIN:
			return count;

		case ITEM_PLATINUM_COIN:
			return count * 100;

		case ITEM_CRYSTAL_COIN:
			return count * 10000;

		default:
			return 0;
	}
}

LightInfo Item::getLightInfo() const
{
	const ItemType& it = items[id];
	return {it.lightLevel, it.lightColor};
}

std::string ItemAttributes::emptyString;
int64_t ItemAttributes::emptyInt;
double ItemAttributes::emptyDouble;
bool ItemAttributes::emptyBool;

const std::string& ItemAttributes::getStrAttr(itemAttrTypes type) const
{
	if (!isStrAttrType(type)) {
		return emptyString;
	}

	const Attribute* attr = getExistingAttr(type);
	if (!attr) {
		return emptyString;
	}
	return *attr->value.string;
}

void ItemAttributes::setStrAttr(itemAttrTypes type, const std::string& value)
{
	if (!isStrAttrType(type)) {
		return;
	}

	if (value.empty()) {
		return;
	}

	Attribute& attr = getAttr(type);
	delete attr.value.string;
	attr.value.string = new std::string(value);
}

void ItemAttributes::removeAttribute(itemAttrTypes type)
{
	if (!hasAttribute(type)) {
		return;
	}

	auto prev_it = attributes.rbegin();
	if ((*prev_it).type == type) {
		attributes.pop_back();
	} else {
		auto it = prev_it, end = attributes.rend();
		while (++it != end) {
			if ((*it).type == type) {
				(*it) = attributes.back();
				attributes.pop_back();
				break;
			}
		}
	}
	attributeBits &= ~type;
}

uint32_t ItemAttributes::getIntAttr(itemAttrTypes type) const
{
	if (!isIntAttrType(type)) {
		return 0;
	}

	const Attribute* attr = getExistingAttr(type);
	if (!attr) {
		return 0;
	}
	return attr->value.integer;
}

void ItemAttributes::setIntAttr(itemAttrTypes type, uint64_t value)
{
	if (!isIntAttrType(type)) {
		return;
	}

	getAttr(type).value.integer = value;
}

void ItemAttributes::increaseIntAttr(itemAttrTypes type, int64_t value)
{
	if (!isIntAttrType(type)) {
		return;
	}

	getAttr(type).value.integer += value;
}

const ItemAttributes::Attribute* ItemAttributes::getExistingAttr(itemAttrTypes type) const
{
	if (hasAttribute(type)) {
		for (const Attribute& attribute : attributes) {
			if (attribute.type == type) {
				return &attribute;
			}
		}
	}
	return nullptr;
}

ItemAttributes::Attribute& ItemAttributes::getAttr(itemAttrTypes type)
{
	if (hasAttribute(type)) {
		for (Attribute& attribute : attributes) {
			if (attribute.type == type) {
				return attribute;
			}
		}
	}

	attributeBits |= type;
	attributes.emplace_back(type);
	return attributes.back();
}

void Item::startDecaying()
{
	g_game.startDecay(this);
}

bool Item::hasMarketAttributes() const
{
	if (attributes == nullptr) {
		return true;
	}

	for (const auto& attr : attributes->getList()) {
		if (attr.type == ITEM_ATTRIBUTE_CHARGES) {
			uint16_t charges = static_cast<uint16_t>(attr.value.integer);
			if (charges != items[id].charges) {
				return false;
			}
		} else if (attr.type == ITEM_ATTRIBUTE_DURATION) {
			uint32_t duration = static_cast<uint32_t>(attr.value.integer);
			if (duration != getDefaultDuration()) {
				return false;
			}
		} else {
			return false;
		}
	}
	return true;
}

template<>
const std::string& ItemAttributes::CustomAttribute::get<std::string>() {
	if (value.type() == typeid(std::string)) {
		return boost::get<std::string>(value);
	}

	return emptyString;
}

template<>
const int64_t& ItemAttributes::CustomAttribute::get<int64_t>() {
	if (value.type() == typeid(int64_t)) {
		return boost::get<int64_t>(value);
	}

	return emptyInt;
}

template<>
const double& ItemAttributes::CustomAttribute::get<double>() {
	if (value.type() == typeid(double)) {
		return boost::get<double>(value);
	}

	return emptyDouble;
}

template<>
const bool& ItemAttributes::CustomAttribute::get<bool>() {
	if (value.type() == typeid(bool)) {
		return boost::get<bool>(value);
	}

	return emptyBool;
}
