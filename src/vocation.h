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

#ifndef FS_VOCATION_H_ADCAA356C0DB44CEBA994A0D678EC92D
#define FS_VOCATION_H_ADCAA356C0DB44CEBA994A0D678EC92D

#include "enums.h"
#include "item.h"

class Vocation
{
	public:
		explicit Vocation(uint16_t id) : id(id) {}

		const std::string& getVocName() const {
			return name;
		}
		const std::string& getVocDescription() const {
			return description;
		}
		uint64_t getReqSkillTries(uint8_t skill, uint16_t level);
		uint64_t getReqMana(uint32_t magLevel);

		uint16_t getId() const {
			return id;
		}

		uint8_t getClientId() const {
			return clientId;
		}

		uint32_t getHPGain() const {
			return gainHP;
		}
		uint32_t getManaGain() const {
			return gainMana;
		}
		uint32_t getCapGain() const {
			return gainCap;
		}

		uint32_t getManaGainTicks() const {
			return gainManaTicks;
		}
		uint32_t getManaGainAmount() const {
			return gainManaAmount;
		}
		uint32_t getHealthGainTicks() const {
			return gainHealthTicks;
		}
		uint32_t getHealthGainAmount() const {
			return gainHealthAmount;
		}

		uint8_t getSoulMax() const {
			return soulMax;
		}
		uint16_t getSoulGainTicks() const {
			return gainSoulTicks;
		}

		uint32_t getAttackSpeed() const {
			return attackSpeed;
		}
		uint32_t getBaseSpeed() const {
			return baseSpeed;
		}

		uint32_t getFromVocation() const {
			return fromVocation;
		}
		uint32_t getArmour() const
		{
			return armor;
		}
		uint32_t getDefense() const
		{
			return defense;
		}
		uint32_t getDistance() const
		{
			return distance;
		}
		uint32_t getMelee() const
		{
			return melee;
		}
		uint32_t getFist() const
		{
			return fist;
		}
		uint32_t getShield() const
		{
			return shield;
		}
		uint32_t getMagic() const
		{
			return magic;
		}
		uint32_t getAccuracy() const
		{
			return accuracy;
		}
		uint32_t getEvasion() const
		{
			return evasion;
		}
		uint32_t getResolve() const
		{
			return resolve;
		}
		uint32_t getAgility() const
		{
			return agility;
		}
		uint32_t getAlacrity() const
		{
			return alacrity;
		}
		uint32_t getFinesse() const
		{
			return finesse;
		}
		uint32_t getConcentration() const
		{
			return concentration;
		}
		uint32_t getFocus() const
		{
			return focus;
		}
		uint32_t getConcocting() const
		{
			return concocting;
		}
		uint32_t getEnchanting() const
		{
			return enchanting;
		}
		uint32_t getExploring() const
		{
			return exploring;
		}
		uint32_t getSmithing() const
		{
			return smithing;
		}
		uint32_t getCooking() const
		{
			return cooking;
		}
		uint32_t getMining() const
		{
			return mining;
		}
		uint32_t getGathering() const
		{
			return gathering;
		}
		uint32_t getSlaying() const
		{
			return slaying;
		}
		
		uint32_t getCRITICALHITCHANCE() const
		{
			return criticalhitchance;
		}
		uint32_t getCRITICALHITAMOUNT() const
		{
			return criticalhitamount;
		}
		uint32_t getHP() const
		{
			return hp;
		}
		uint32_t getMP() const
		{
			return mp;
		}
		uint32_t getHPREGEN() const
		{
			return hpregen;
		}
		uint32_t getMPREGEN() const
		{
			return mpregen;
		}

		bool canDualWield() const
		{
			return dualWield;
		}

		float meleeDamageMultiplier = 1.0f;
		float distDamageMultiplier = 1.0f;
		float defenseMultiplier = 1.0f;
		float armorMultiplier = 1.0f;

	private:
		friend class Vocations;

		std::string name = "none";
		std::string description;

		double skillMultipliers[SKILL_LAST + 1] = {1.5, 2.0, 2.0, 2.0, 2.0, 1.5, 1.1, 2.0, 2.0, 2.0, 2.0, 2.0, 2.0, 2.0, 2.0, 2.0, 2.0, 1.1, 1.1, 1.1, 1.1, 1.1, 1.1, 1.1, 1.1};
		float manaMultiplier = 4.0f;

		uint32_t gainHealthTicks = 6;
		uint32_t gainHealthAmount = 1;
		uint32_t gainManaTicks = 6;
		uint32_t gainManaAmount = 1;
		uint32_t gainCap = 500;
		uint32_t gainMana = 5;
		uint32_t gainHP = 5;
		uint32_t fromVocation = VOCATION_NONE;
		bool dualWield = true; 
		uint32_t attackSpeed = 1500;
		uint32_t baseSpeed = 220;
		uint16_t id;

		uint16_t gainSoulTicks = 120;

		uint8_t soulMax = 100;
		uint8_t clientId = 0;
		uint32_t defense = 0;
		uint32_t armor = 0;
		uint32_t accuracy = 0;
		uint32_t evasion = 0;
		uint32_t resolve = 0;
		uint32_t agility = 0;
		uint32_t alacrity = 0;
		uint32_t finesse = 0;
		uint32_t concentration = 0;
		uint32_t focus = 0;
		uint32_t concocting = 0;
		uint32_t enchanting = 0;
		uint32_t exploring = 0;
		uint32_t smithing = 0;
		uint32_t cooking = 0;
		uint32_t mining = 0;
		uint32_t gathering = 0;
		uint32_t slaying = 0;
		uint32_t magic = 0;
		uint32_t distance = 0;
		uint32_t melee = 0;
		uint32_t shield = 0;
		uint32_t fist = 0;
		int32_t criticalhitchance = 0;
		int32_t criticalhitamount = 0;
		int32_t mpregen = 0;
		int32_t hpregen = 0;
		int32_t hp = 0;
		int32_t mp = 0;
};

class Vocations
{
	public:
		bool loadFromXml();

		Vocation* getVocation(uint16_t id);
		int32_t getVocationId(const std::string& name) const;
		uint16_t getPromotedVocation(uint16_t vocationId) const;

	private:
		std::map<uint16_t, Vocation> vocationsMap;
};

#endif
