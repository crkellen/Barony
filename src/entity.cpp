/*-------------------------------------------------------------------------------

	BARONY
	File: entity.cpp
	Desc: implements entity code

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#pragma once

#include "main.hpp"
#include "game.hpp"
#include "stat.hpp"
#include "entity.hpp"
#include "items.hpp"
#include "monster.hpp"
#include "sound.hpp"
#include "magic/magic.hpp"
#include "interface/interface.hpp"
#include "net.hpp"
#include "collision.hpp"
#include "paths.hpp"
#include "book.hpp"
#ifdef STEAMWORKS
#include <steam/steam_api.h>
#endif
#include "player.hpp"
#ifdef __ARM_NEON__
#include <arm_neon.h>
#endif

/*-------------------------------------------------------------------------------

	Entity::Entity)

	Construct an Entity

-------------------------------------------------------------------------------*/

Entity::Entity(Sint32 in_sprite, Uint32 pos, list_t* entlist) :
	char_gonnavomit(skill[26]),
	char_heal(skill[22]),
	char_energize(skill[23]),
	char_torchtime(skill[25]),
	char_poison(skill[21]),
	circuit_status(skill[28]),
	switch_power(skill[0]),
	chestInit(skill[0]),
	chestStatus(skill[1]),
	chestHealth(skill[3]),
	chestLocked(skill[4]),
	chestOpener(skill[5]),
	chestLidClicked(skill[6]),
	chestAmbience(skill[7]),
	chestMaxHealth(skill[8]),
	chestType(skill[9]),
	chestPreventLockpickCapstoneExploit(skill[10]),
	monsterState(skill[0]),
	monsterTarget(skill[1]),
	monsterTargetX(fskill[2]),
	monsterTargetY(fskill[3]),
	crystalInitialised(skill[1]),
	crystalTurning(skill[3]),
	crystalTurnStartDir(skill[4]),
	crystalGeneratedElectricityNodes(skill[5]),
	crystalNumElectricityNodes(skill[6]),
	crystalHoverDirection(skill[7]),
	crystalHoverWaitTimer(skill[8]),
	crystalTurnReverse(skill[9]),
	crystalSpellToActivate(skill[10]),
	crystalStartZ(fskill[0]),
	crystalMaxZVelocity(fskill[1]),
	crystalMinZVelocity(fskill[2]),
	crystalTurnVelocity(fskill[3]),
	monsterAnimationLimbDirection(skill[20]),
	monsterAnimationLimbOvershoot(skill[30]),
	monsterSpecialTimer(skill[29]),
	monsterSpecialState(skill[33]),
	monsterSpellAnimation(skill[31]),
	monsterFootstepType(skill[32]),
	monsterLookTime(skill[4]),
	monsterMoveTime(skill[6]),
	monsterLookDir(fskill[4]),
	monsterAttack(skill[8]),
	monsterAttackTime(skill[9]),
	monsterArmbended(skill[10]),
	monsterWeaponYaw(fskill[5]),
	particleDuration(skill[0]),
	monsterHitTime(skill[7]),
	itemNotMoving(skill[18])

{
	int c;
	// add the entity to the entity list
	if (!pos)
	{
		mynode = list_AddNodeFirst(entlist);
	}
	else
	{
		mynode = list_AddNodeLast(entlist);
	}
	mynode->element = this;
	mynode->deconstructor = &entityDeconstructor;
	mynode->size = sizeof(Entity);

	// now reset all of my data elements
	lastupdate = 0;
	lastupdateserver = 0;
	ticks = 0;
	x = 0;
	y = 0;
	z = 0;
	new_x = 0;
	new_y = 0;
	new_z = 0;
	focalx = 0;
	focaly = 0;
	focalz = 0;
	scalex = 1;
	scaley = 1;
	scalez = 1;
	vel_x = 0;
	vel_y = 0;
	vel_z = 0;
	sizex = 0;
	sizey = 0;
	yaw = 0;
	pitch = 0;
	roll = 0;
	new_yaw = 0;
	new_pitch = 0;
	new_roll = 0;
	sprite = in_sprite;
	light = NULL;
	string = NULL;
	children.first = NULL;
	children.last = NULL;
	//this->magic_effects = (list_t *) malloc(sizeof(list_t));
	//this->magic_effects->first = NULL; this->magic_effects->last = NULL;
	for ( c = 0; c < NUMENTITYSKILLS; ++c )
	{
		skill[c] = 0;
	}
	for ( c = 0; c < NUMENTITYFSKILLS; ++c )
	{
		fskill[c] = 0;
	}
	skill[2] = -1;
	for ( c = 0; c < 16; c++ )
	{
		flags[c] = false;
	}
	if ( entlist == map.entities )
	{
		if ( multiplayer != CLIENT || loading )
		{
			uid = entity_uids;
			entity_uids++;
			map.entities_map.insert({uid, mynode});
		}
		else
		{
			uid = -2;
		}
	}
	else
	{
		uid = -2;
	}
	behavior = NULL;
	ranbehavior = false;
	parent = 0;
	path = NULL;

	if ( checkSpriteType(this->sprite) > 1 )
	{
		setSpriteAttributes(this, nullptr, nullptr);
	}

	clientStats = nullptr;
	clientsHaveItsStats = false;
}

void Entity::setUID(Uint32 new_uid) {
	if ( mynode->list == map.entities ) {
		map.entities_map.erase(uid);
		map.entities_map.insert({new_uid, mynode});
	}
	uid = new_uid;
}

/*-------------------------------------------------------------------------------

	Entity::~Entity)

	Deconstruct an Entity

-------------------------------------------------------------------------------*/

Entity::~Entity()
{
	node_t* node;
	//node_t *node2;
	int i;
	//deleteent_t *deleteent;

	// remove any remaining "parent" references
	/*if( entity->mynode != NULL ) {
		if( entity->mynode->list != NULL ) {
			for( node2=entity->mynode->list->first; node2!=NULL; node2=node2->next ) {
				Entity *entity2 = (Entity *)node2->element;
				if( entity2 != entity && entity2 != NULL )
					if( entity2->parent == entity )
						entity2->parent = NULL;
			}
		}
	}*/

	// alert clients of the entity's deletion
	if (multiplayer == SERVER && !loading)
	{
		if (mynode->list == map.entities && uid != 0 && flags[NOUPDATE] == false)
		{
			for (i = 1; i < MAXPLAYERS; i++)
			{
				if ( client_disconnected[i] == true )
				{
					continue;
				}

				// create a reminder for the server to continue informing the client of the deletion
				/*deleteent = (deleteent_t *) malloc(sizeof(deleteent_t)); //TODO: C++-PORT: Replace with new + class.
				deleteent->uid = uid;
				deleteent->tries = 0;
				node = list_AddNodeLast(&entitiesToDelete[i]);
				node->element = deleteent;
				node->deconstructor = &defaultDeconstructor;*/

				// send the delete entity command to the client
				strcpy((char*)net_packet->data, "ENTD");
				SDLNet_Write32((Uint32)uid, &net_packet->data[4]);
				net_packet->address.host = net_clients[i - 1].host;
				net_packet->address.port = net_clients[i - 1].port;
				net_packet->len = 8;
				/*if ( directConnect ) {
					SDLNet_UDP_Send(net_sock,-1,net_packet);
				} else {
					#ifdef STEAMWORKS
					SteamNetworking()->SendP2PPacket(*static_cast<CSteamID* >(steamIDRemote[i - 1]), net_packet->data, net_packet->len, k_EP2PSendUnreliable, 0);
					#endif
				}*/
				sendPacketSafe(net_sock, -1, net_packet, i - 1);
			}
		}
	}

	// set appropriate player pointer to NULL
	for (i = 0; i < MAXPLAYERS; i++)
		if (this == players[i]->entity)
		{
			players[i]->entity = nullptr;    //TODO: PLAYERSWAP VERIFY. Should this do anything to the player itself?
		}
	// destroy my children
	list_FreeAll(&this->children);

	node = list_AddNodeLast(&entitiesdeleted);
	node->element = this;
	node->deconstructor = &emptyDeconstructor;

	if ( clientStats )
	{
		delete clientStats;
	}
}

/*-------------------------------------------------------------------------------

	Entity::setObituary

	Sets the obituary text on an entity.

-------------------------------------------------------------------------------*/

void Entity::setObituary(char* obituary)
{
	Stat* tempStats = this->getStats();
	if ( !tempStats )
	{
		return;
	}
	strncpy(tempStats->obituary, obituary, 127);
}

/*-------------------------------------------------------------------------------

	Entity::killedByMonsterObituary

	Sets the obituary to that of a mon

-------------------------------------------------------------------------------*/

void Entity::killedByMonsterObituary(Entity* victim)
{
	if ( !victim )
	{
		return;
	}
	Stat* hitstats = victim->getStats();
	Stat* myStats = this->getStats();
	if ( !hitstats || !myStats )
	{
		return;
	}

	if ( myStats->type == hitstats->type )
	{
		if ( hitstats->sex == MALE )
		{
			if ( hitstats->type < KOBOLD ) //Original monster count
			{
				snprintf(tempstr, 256, language[1509], language[90 + hitstats->type]);
			}
			else if ( hitstats->type >= KOBOLD ) //New monsters
			{
				snprintf(tempstr, 256, language[1509], language[2000 + (hitstats->type - KOBOLD)]);
			}
		}
		else
		{
			if ( hitstats->type < KOBOLD ) //Original monster count
			{
				snprintf(tempstr, 256, language[1510], language[90 + hitstats->type]);
			}
			else if ( hitstats->type >= KOBOLD ) //New monsters
			{
				snprintf(tempstr, 256, language[1510], language[2000 + (hitstats->type - KOBOLD)]);
			}
		}
		victim->setObituary(tempstr);
	}
	else
	{
		switch ( myStats->type )
		{
			case HUMAN:
				victim->setObituary(language[1511]);
				break;
			case RAT:
				victim->setObituary(language[1512]);
				break;
			case GOBLIN:
				victim->setObituary(language[1513]);
				break;
			case SLIME:
				victim->setObituary(language[1514]);
				break;
			case TROLL:
				victim->setObituary(language[1515]);
				break;
			case SPIDER:
				victim->setObituary(language[1516]);
				break;
			case GHOUL:
				victim->setObituary(language[1517]);
				break;
			case SKELETON:
				victim->setObituary(language[1518]);
				break;
			case SCORPION:
				victim->setObituary(language[1519]);
				break;
			case CREATURE_IMP:
				victim->setObituary(language[1520]);
				break;
			case GNOME:
				victim->setObituary(language[1521]);
				break;
			case DEMON:
				victim->setObituary(language[1522]);
				break;
			case SUCCUBUS:
				victim->setObituary(language[1523]);
				break;
			case LICH:
				victim->setObituary(language[1524]);
				break;
			case MINOTAUR:
				victim->setObituary(language[1525]);
				break;
			case DEVIL:
				victim->setObituary(language[1526]);
				break;
			case SHOPKEEPER:
				victim->setObituary(language[1527]);
				break;
			case KOBOLD:
				victim->setObituary(language[2150]);
				break;
			case SCARAB:
				victim->setObituary(language[2151]);
				break;
			case CRYSTALGOLEM:
				victim->setObituary(language[2152]);
				break;
			case INCUBUS:
				victim->setObituary(language[2153]);
				break;
			case VAMPIRE:
				victim->setObituary(language[2154]);
				break;
			case SHADOW:
				victim->setObituary(language[2155]);
				break;
			case COCKATRICE:
				victim->setObituary(language[2156]);
				break;
			case INSECTOID:
				victim->setObituary(language[2157]);
				break;
			case GOATMAN:
				victim->setObituary(language[2158]);
				break;
			case AUTOMATON:
				victim->setObituary(language[2159]);
				break;
			case LICH_ICE:
				victim->setObituary(language[2160]);
				break;
			case LICH_FIRE:
				victim->setObituary(language[2161]);
				break;
			default:
				victim->setObituary(language[1500]);
				break;
		}
	}
}

/*-------------------------------------------------------------------------------

	Entity::light

	Returns the illumination of the given entity

-------------------------------------------------------------------------------*/

int Entity::entityLight()
{
	if ( this->flags[BRIGHT] )
	{
		return 255;
	}
	if ( this->x < 0 || this->y < 0 || this->x >= map.width << 4 || this->y >= map.height << 4 )
	{
		return 255;
	}
	int light_x = (int)this->x / 16;
	int light_y = (int)this->y / 16;
	return lightmap[light_y + light_x * map.height];
}

/*-------------------------------------------------------------------------------

	Entity::effectTimes

	Counts down effect timers and toggles effects whose timers reach zero

-------------------------------------------------------------------------------*/

void Entity::effectTimes()
{
	Stat* myStats = this->getStats();
	int player, c;
	spell_t* spell = NULL;
	node_t* node = NULL;
	int count = 0;

	if ( myStats == NULL )
	{
		return;
	}
	if ( this->behavior == &actPlayer )
	{
		player = this->skill[2];
	}
	else
	{
		player = -1;
	}


	spell_t* invisibility_hijacked = nullptr; //If NULL, function proceeds as normal. If points to something, it ignores the invisibility timer since a spell is doing things. //TODO: Incorporate the spell into isInvisible() instead?
	spell_t* levitation_hijacked = nullptr; //If NULL, function proceeds as normal. If points to something, it ignore the levitation timer since a spell is doing things.
	spell_t* reflectMagic_hijacked = nullptr;
	//Handle magic effects (like invisibility)
	for (node = myStats->magic_effects.first; node; node = node->next, ++count)
	{
		//printlog( "%s\n", "Potato.");
		//Handle magic effects.
		spell = (spell_t*) node->element;
		if (!spell->sustain)
		{
			node_t* temp = NULL;
			if (node->prev)
			{
				temp = node->prev;
			}
			else if (node->next)
			{
				temp = node->next;
			}
			spell->magic_effects_node = NULL; //To prevent recursive removal, which results in a crash.
			if (player > -1 && multiplayer == SERVER)
			{
				strcpy( (char*)net_packet->data, "UNCH");
				net_packet->data[4] = player;
				SDLNet_Write32(spell->ID, &net_packet->data[5]);
				net_packet->address.host = net_clients[player - 1].host;
				net_packet->address.port = net_clients[player - 1].port;
				net_packet->len = 9;
				sendPacketSafe(net_sock, -1, net_packet, player - 1);
			}
			list_RemoveNode(node); //Bugger the spell.
			node = temp;
			if (!node)
			{
				break; //Done with list. Stop.
			}
			continue; //Skip this spell.
		}

		switch (spell->ID)
		{
			case SPELL_INVISIBILITY:
				invisibility_hijacked = spell;
				if (!myStats->EFFECTS[EFF_INVISIBLE])
				{
					for (c = 0; c < numplayers; ++c)
					{
						if (players[c] && players[c]->entity == uidToEntity(spell->caster) && players[c]->entity != nullptr)
						{
							messagePlayer(c, language[591]);    //If cure ailments or somesuch bombs the status effects.
						}
					}
					node_t* temp = nullptr;
					if (node->prev)
					{
						temp = node->prev;
					}
					else if (node->next)
					{
						temp = node->next;
					}
					list_RemoveNode(node); //Remove this here node.
					node = temp;
				}
				break;
			case SPELL_LEVITATION:
				levitation_hijacked = spell;
				if (!myStats->EFFECTS[EFF_LEVITATING])
				{
					for (c = 0; c < numplayers; ++c)
					{
						if (players[c] && players[c]->entity == uidToEntity(spell->caster) && players[c]->entity != nullptr)
						{
							messagePlayer(c, language[592]);
						}
					}
					node_t* temp = nullptr;
					if (node->prev)
					{
						temp = node->prev;
					}
					else if (node->next)
					{
						temp = node->next;
					}
					list_RemoveNode(node); //Remove this here node.
					node = temp;
				}
				break;
			case SPELL_REFLECT_MAGIC:
				reflectMagic_hijacked = spell;
				if ( !myStats->EFFECTS[EFF_MAGICREFLECT] )
				{
					for ( c = 0; c < numplayers; ++c )
					{
						if ( players[c] && players[c]->entity == uidToEntity(spell->caster) && players[c]->entity != nullptr )
						{
							messagePlayer(c, language[591]);
						}
					}
					node_t* temp = nullptr;
					if ( node->prev )
					{
						temp = node->prev;
					}
					else if ( node->next )
					{
						temp = node->next;
					}
					list_RemoveNode(node); //Remove this here node.
					node = temp;
				}
				break;
			default:
				//Unknown spell, undefined effect. Like, say, a fireball spell wound up in here for some reason. That's a nono.
				printlog("[entityEffectTimes] Warning: magic_effects spell that's not relevant. Should not be in the magic_effects list!\n");
				list_RemoveNode(node);
		}

		if (!node)
		{
			break;    //BREAK OUT. YEAAAAAH. Because otherwise it crashes.
		}
	}
	if (count)
	{
		//printlog( "Number of magic effects spells: %d\n", count); //Debugging output.
	}

	bool dissipate = true;
	bool updateClient = false;

	for ( c = 0; c < NUMEFFECTS; c++ )
	{
		if ( myStats->EFFECTS_TIMERS[c] > 0 )
		{
			myStats->EFFECTS_TIMERS[c]--;
			if ( myStats->EFFECTS_TIMERS[c] == 0 )
			{
				myStats->EFFECTS[c] = false;
				switch ( c )
				{
					case EFF_ASLEEP:
						messagePlayer(player, language[593]);
						break;
					case EFF_POISONED:
						messagePlayer(player, language[594]);
						break;
					case EFF_STUNNED:
						messagePlayer(player, language[595]);
						break;
					case EFF_CONFUSED:
						messagePlayer(player, language[596]);
						break;
					case EFF_DRUNK:
						messagePlayer(player, language[597]);
						break;
					case EFF_INVISIBLE:
						; //To make the compiler shut up: "error: a label can only be part of a statement and a declaration is not a statement"
						dissipate = true; //Remove the effect by default.
						if (invisibility_hijacked)
						{
							bool sustained = false;
							Entity* caster = uidToEntity(invisibility_hijacked->caster);
							if (caster)
							{
								//Deduct mana from caster. Cancel spell if not enough mana (simply leave sustained at false).
								bool deducted = caster->safeConsumeMP(1); //Consume 1 mana ever duration / mana seconds
								if (deducted)
								{
									sustained = true;
									myStats->EFFECTS[c] = true;
									myStats->EFFECTS_TIMERS[c] = invisibility_hijacked->channel_duration;
								}
								else
								{
									int i = 0;
									for (i = 0; i < 4; ++i)
									{
										if (players[i]->entity == caster)
										{
											messagePlayer(i, language[598]);
										}
									}
									list_RemoveNode(invisibility_hijacked->magic_effects_node); //Remove it from the entity's magic effects. This has the side effect of removing it from the sustained spells list too.
									//list_RemoveNode(invisibility_hijacked->sustain_node); //Remove it from the channeled spells list.
								}
							}
							if (sustained)
							{
								dissipate = false;    //Sustained the spell, so do not stop being invisible.
							}
						}
						if (dissipate)
						{
							if ( !this->isBlind() )
							{
								messagePlayer(player, language[599]);
							}
						}
						break;
					case EFF_BLIND:
						if ( !this->isBlind())
						{
							messagePlayer(player, language[600]);
						}
						else
						{
							messagePlayer(player, language[601]);
						}
						break;
					case EFF_GREASY:
						messagePlayer(player, language[602]);
						break;
					case EFF_MESSY:
						messagePlayer(player, language[603]);
						break;
					case EFF_FAST:
						messagePlayer(player, language[604]);
						break;
					case EFF_PARALYZED:
						messagePlayer(player, language[605]);
						break;
					case EFF_LEVITATING:
						; //To make the compiler shut up: "error: a label can only be part of a statement and a declaration is not a statement"
						dissipate = true; //Remove the effect by default.
						if (levitation_hijacked)
						{
							bool sustained = false;
							Entity* caster = uidToEntity(levitation_hijacked->caster);
							if (caster)
							{
								//Deduct mana from caster. Cancel spell if not enough mana (simply leave sustained at false).
								bool deducted = caster->safeConsumeMP(1); //Consume 1 mana ever duration / mana seconds
								if (deducted)
								{
									sustained = true;
									myStats->EFFECTS[c] = true;
									myStats->EFFECTS_TIMERS[c] = levitation_hijacked->channel_duration;
								}
								else
								{
									int i = 0;
									for (i = 0; i < 4; ++i)
									{
										if (players[i]->entity == caster)
										{
											messagePlayer(i, language[606]);    //TODO: Unhardcode name?
										}
									}
									list_RemoveNode(levitation_hijacked->magic_effects_node); //Remove it from the entity's magic effects. This has the side effect of removing it from the sustained spells list too.
								}
							}
							if (sustained)
							{
								dissipate = false;    //Sustained the spell, so do not stop levitating.
							}
						}
						if (dissipate)
						{
							messagePlayer(player, language[607]);
						}
						break;
					case EFF_TELEPATH:
						messagePlayer(player, language[608]);
						break;
					case EFF_VOMITING:
						messagePlayer(player, language[609]);
						if ( myStats->HUNGER > 1500 )
						{
							messagePlayer(player, language[610]);
						}
						else if ( myStats->HUNGER > 150 && myStats->HUNGER <= 250 )
						{
							messagePlayer(player, language[611]);
							playSoundPlayer(player, 32, 128);
						}
						else if ( myStats->HUNGER > 50 )
						{
							messagePlayer(player, language[612]);
							playSoundPlayer(player, 32, 128);
						}
						else
						{
							myStats->HUNGER = 50;
							messagePlayer(player, language[613]);
							playSoundPlayer(player, 32, 128);
						}
						serverUpdateHunger(player);
						break;
					case EFF_BLEEDING:
						messagePlayer(player, language[614]);
						break;
					case EFF_MAGICRESIST:
						messagePlayer(player, language[2470]);
						break;
					case EFF_MAGICREFLECT:
						dissipate = true; //Remove the effect by default.
						if ( reflectMagic_hijacked )
						{
							bool sustained = false;
							Entity* caster = uidToEntity(reflectMagic_hijacked->caster);
							if ( caster )
							{
								//Deduct mana from caster. Cancel spell if not enough mana (simply leave sustained at false).
								bool deducted = caster->safeConsumeMP(1); //Consume 1 mana ever duration / mana seconds
								if ( deducted )
								{
									sustained = true;
									myStats->EFFECTS[c] = true;
									myStats->EFFECTS_TIMERS[c] = reflectMagic_hijacked->channel_duration;
								}
								else
								{
									int i = 0;
									for ( i = 0; i < 4; ++i )
									{
										if ( players[i]->entity == caster )
										{
											messagePlayer(i, language[2474]);
										}
									}
									list_RemoveNode(reflectMagic_hijacked->magic_effects_node); //Remove it from the entity's magic effects. This has the side effect of removing it from the sustained spells list too.
									//list_RemoveNode(reflectMagic_hijacked->sustain_node); //Remove it from the channeled spells list.
								}
							}
							if ( sustained )
							{
								dissipate = false; //Sustained the spell, so do not stop being invisible.
							}
						}
						if ( dissipate )
						{
							messagePlayer(player, language[2471]);
							updateClient = true;
						}
						break;
					default:
						break;
				}
				if ( player > 0 && multiplayer == SERVER )
				{
					serverUpdateEffects(player);
				}
			}
		}
	}

	if ( updateClient )
	{
		//Only a select few effects have something that needs to be handled on the client's end.
		//(such as spawning particles for the magic reflection effect)
		//Only update the entity's effects in that case.
		serverUpdateEffectsForEntity(true);
	}
}

/*-------------------------------------------------------------------------------

	Entity::increaseSkill

	Increases the given skill of the given entity by 1.

-------------------------------------------------------------------------------*/

void Entity::increaseSkill(int skill)
{
	Stat* myStats = this->getStats();
	int player = -1;

	if ( myStats == NULL )
	{
		return;
	}
	if ( this->behavior == &actPlayer )
	{
		player = this->skill[2];
	}

	Uint32 color = SDL_MapRGB(mainsurface->format, 255, 255, 0);
	if ( myStats->PROFICIENCIES[skill] < 100 )
	{
		myStats->PROFICIENCIES[skill]++;
		messagePlayerColor(player, color, language[615], language[236 + skill]);
		switch ( myStats->PROFICIENCIES[skill] )
		{
			case 20:
				messagePlayerColor(player, color, language[616], language[236 + skill]);
				break;
			case 40:
				messagePlayerColor(player, color, language[617], language[236 + skill]);
				break;
			case 60:
				messagePlayerColor(player, color, language[618], language[236 + skill]);
				break;
			case 80:
				messagePlayerColor(player, color, language[619], language[236 + skill]);
				break;
			case 100:
				messagePlayerColor(player, color, language[620], language[236 + skill]);
				break;
			default:
				break;
		}

		if ( skill == PRO_SPELLCASTING && skillCapstoneUnlockedEntity(PRO_SPELLCASTING) )
		{
			//Spellcasting capstone = free casting of magic missile.
			//Give the player the spell if they haven't learned it yet.
			if ( player > 0 && multiplayer == SERVER )
			{
				strcpy((char*)net_packet->data, "ASPL");
				net_packet->data[4] = clientnum;
				net_packet->data[5] = SPELL_MAGICMISSILE;
				net_packet->address.host = net_clients[player - 1].host;
				net_packet->address.port = net_clients[player - 1].port;
				net_packet->len = 6;
				sendPacketSafe(net_sock, -1, net_packet, player - 1);
			}
			else if ( player >= 0 )
			{
				addSpell(SPELL_MAGICMISSILE, player, true);
			}
		}

		if ( skill == PRO_MAGIC && skillCapstoneUnlockedEntity(PRO_MAGIC) )
		{
			//magic capstone = bonus spell: Dominate.
			if ( player > 0 && multiplayer == SERVER )
			{
				strcpy((char*)net_packet->data, "ASPL");
				net_packet->data[4] = clientnum;
				net_packet->data[5] = SPELL_DOMINATE;
				net_packet->address.host = net_clients[player - 1].host;
				net_packet->address.port = net_clients[player - 1].port;
				net_packet->len = 6;
				sendPacketSafe(net_sock, -1, net_packet, player - 1);
			}
			else if ( player >= 0 )
			{
				addSpell(SPELL_DOMINATE, player, true);
			}
		}
	}
	myStats->EXP += 2;

	int statBonusSkill = getStatForProficiency(skill);

	if ( statBonusSkill >= STAT_STR )
	{
		// stat has chance for bonus point if the relevant proficiency has been trained.
		// write the last proficiency that effected the skill.
		myStats->PLAYER_LVL_STAT_BONUS[statBonusSkill] = skill;
	}

	if ( player > 0 && multiplayer == SERVER )
	{
		// update SKILL
		strcpy((char*)net_packet->data, "SKIL");
		net_packet->data[4] = clientnum;
		net_packet->data[5] = skill;
		net_packet->data[6] = myStats->PROFICIENCIES[skill];
		net_packet->address.host = net_clients[player - 1].host;
		net_packet->address.port = net_clients[player - 1].port;
		net_packet->len = 7;
		sendPacketSafe(net_sock, -1, net_packet, player - 1);

		// update EXP
		strcpy((char*)net_packet->data, "ATTR");
		net_packet->data[4] = clientnum;
		net_packet->data[5] = (Sint8)myStats->STR;
		net_packet->data[6] = (Sint8)myStats->DEX;
		net_packet->data[7] = (Sint8)myStats->CON;
		net_packet->data[8] = (Sint8)myStats->INT;
		net_packet->data[9] = (Sint8)myStats->PER;
		net_packet->data[10] = (Sint8)myStats->CHR;
		net_packet->data[11] = (Sint8)myStats->EXP;
		net_packet->data[12] = (Sint8)myStats->LVL;
		SDLNet_Write16((Sint16)myStats->HP, &net_packet->data[13]);
		SDLNet_Write16((Sint16)myStats->MAXHP, &net_packet->data[15]);
		SDLNet_Write16((Sint16)myStats->MP, &net_packet->data[17]);
		SDLNet_Write16((Sint16)myStats->MAXMP, &net_packet->data[19]);
		net_packet->address.host = net_clients[player - 1].host;
		net_packet->address.port = net_clients[player - 1].port;
		net_packet->len = 21;
		sendPacketSafe(net_sock, -1, net_packet, player - 1);
	}
}

/*-------------------------------------------------------------------------------

	Entity::stats

	Returns a pointer to a Stat instance given a pointer to an entity

-------------------------------------------------------------------------------*/

Stat* Entity::getStats() const
{
	if (this->behavior == &actMonster) // monsters
	{
		if ( multiplayer == CLIENT && clientStats )
		{
			return clientStats;
		}
		if (this->children.first != nullptr)
		{
			if (this->children.first->next != nullptr)
			{
				return (Stat*)this->children.first->next->element;
			}
		}
	}
	else if (this->behavior == &actPlayer) // players
	{
		return stats[this->skill[2]];
	}
	else if (this->behavior == &actPlayerLimb) // player bodyparts
	{
		return stats[this->skill[2]];
	}

	return nullptr;
}

/*-------------------------------------------------------------------------------

	Entity::checkBetterEquipment

	Checks the tiles immediately surrounding the given entity for items and
	replaces the entity's equipment with those items if they are better

-------------------------------------------------------------------------------*/

void Entity::checkBetterEquipment(Stat* myStats)
{
	if (!myStats)
	{
		return; //Can't continue without these.
	}

	list_t* items = nullptr;
	//X and Y in terms of tiles.
	int tx = x / 16;
	int ty = y / 16;
	getItemsOnTile(tx, ty, &items); //Check the tile the goblin is on for items.
	getItemsOnTile(tx - 1, ty, &items); //Check tile to the left.
	getItemsOnTile(tx + 1, ty, &items); //Check tile to the right.
	getItemsOnTile(tx, ty - 1, &items); //Check tile up.
	getItemsOnTile(tx, ty + 1, &items); //Check tile down.
	getItemsOnTile(tx - 1, ty - 1, &items); //Check tile diagonal up left.
	getItemsOnTile(tx + 1, ty - 1, &items); //Check tile diagonal up right.
	getItemsOnTile(tx - 1, ty + 1, &items); //Check tile diagonal down left.
	getItemsOnTile(tx + 1, ty + 1, &items); //Check tile diagonal down right.
	int currentAC, newAC;
	Item* oldarmor = nullptr;

	node_t* node = nullptr;

	bool glovesandshoes = false;
	if ( myStats->type == HUMAN )
	{
		glovesandshoes = true;
	}

	if ( items )
	{
		/*
		 * Rundown of the function:
		 * Loop through all items.
		 * Check the monster's item. Compare and grab the best item.
		 */

		for (node = items->first; node != nullptr; node = node->next)
		{
			//Turn the entity into an item.
			if (node->element)
			{
				Entity* entity = (Entity*)node->element;
				Item* item = nullptr;
				if (entity != nullptr)
				{
					item = newItemFromEntity(entity);
				}
				if ( !item )
				{
					continue;
				}
				if ( !canWieldItem(*item) )
				{
					free(item);
					continue;
				}

				//If weapon.
				if (itemCategory(item) == WEAPON)
				{
					if (myStats->weapon == nullptr) //Not currently holding a weapon.
					{
						myStats->weapon = item; //Assign the monster's weapon.
						item = nullptr;
						list_RemoveNode(entity->mynode);
					}
					else
					{
						//Ok, the monster has a weapon already. First check if the monster's weapon is cursed. Can't drop it if it is.
						if (myStats->weapon->beatitude >= 0 && itemCategory(myStats->weapon) != MAGICSTAFF && itemCategory(myStats->weapon) != POTION && itemCategory(myStats->weapon) != THROWN && itemCategory(myStats->weapon) != GEM )
						{
							//Next compare the two weapons. If the item on the ground is better, drop the weapon it's carrying and equip that one.
							int weapon_tohit = myStats->weapon->weaponGetAttack();
							int new_weapon_tohit = item->weaponGetAttack();

							//If the new weapon does more damage than the current weapon.
							if (new_weapon_tohit > weapon_tohit)
							{
								dropItemMonster(myStats->weapon, this, myStats);
								myStats->weapon = item;
								item = nullptr;
								list_RemoveNode(entity->mynode);
							}
						}
					}
				}
				else if ( itemCategory(item) == ARMOR )
				{
					if ( checkEquipType(item) == TYPE_HAT ) // hats
					{
						if (myStats->helmet == nullptr) // nothing on head currently
						{
							// goblins love hats.
							myStats->helmet = item; // pick up the hat.
							item = nullptr;
							list_RemoveNode(entity->mynode);
						}
					}
					else if ( checkEquipType(item) == TYPE_HELM ) // helmets
					{
						if (myStats->helmet == nullptr) // nothing on head currently
						{
							myStats->helmet = item; // pick up the helmet.
							item = nullptr;
							list_RemoveNode(entity->mynode);
						}
						else
						{
							if (myStats->helmet->beatitude >= 0) // if the armor is not cursed, proceed. Won't do anything if the armor is cursed.
							{
								// to compare the armors, we use the AC function to check the Armor Class of the equipment the goblin
								// is currently wearing versus the Armor Class that the goblin would have if it had the new armor.
								currentAC = AC(myStats);
								oldarmor = myStats->helmet;
								myStats->helmet = item;
								newAC = AC(myStats);
								myStats->helmet = oldarmor;

								//If the new armor is better than the current armor.
								if (newAC > currentAC)
								{
									dropItemMonster(myStats->helmet, this, myStats);
									myStats->helmet = item;
									item = nullptr;
									list_RemoveNode(entity->mynode);
								}
							}
						}
					}
					else if ( checkEquipType(item) == TYPE_SHIELD )     // shields
					{
						if (myStats->shield == nullptr) // nothing in left hand currently
						{
							myStats->shield = item; // pick up the shield.
							item = nullptr;
							list_RemoveNode(entity->mynode);
						}
						else
						{
							if (myStats->shield->beatitude >= 0)   // if the armor is not cursed, proceed. Won't do anything if the armor is cursed.
							{
								// to compare the armors, we use the AC function to check the Armor Class of the equipment the goblin
								// is currently wearing versus the Armor Class that the goblin would have if it had the new armor.
								currentAC = AC(myStats);
								oldarmor = myStats->shield;
								myStats->shield = item;
								newAC = AC(myStats);
								myStats->shield = oldarmor;

								//If the new armor is better than the current armor (OR we're not carrying anything)
								if (newAC > currentAC || !myStats->shield )
								{
									dropItemMonster(myStats->shield, this, myStats);
									myStats->shield = item;
									item = nullptr;
									list_RemoveNode(entity->mynode);
								}
							}
						}
					}
					else if ( checkEquipType(item) == TYPE_BREASTPIECE ) // breastpieces
					{
						if (myStats->breastplate == nullptr) // nothing on torso currently
						{
							myStats->breastplate = item; // pick up the armor.
							item = nullptr;
							list_RemoveNode(entity->mynode);
						}
						else
						{
							if (myStats->breastplate->beatitude >= 0) // if the armor is not cursed, proceed. Won't do anything if the armor is cursed.
							{
								// to compare the armors, we use the AC function to check the Armor Class of the equipment the goblin
								// is currently wearing versus the Armor Class that the goblin would have if it had the new armor.
								currentAC = AC(myStats);
								oldarmor = myStats->breastplate;
								myStats->breastplate = item;
								newAC = AC(myStats);
								myStats->breastplate = oldarmor;

								//If the new armor is better than the current armor.
								if (newAC > currentAC)
								{
									dropItemMonster(myStats->breastplate, this, myStats);
									myStats->breastplate = item;
									item = nullptr;
									list_RemoveNode(entity->mynode);
								}
							}
						}
					}
					else if ( checkEquipType(item) == TYPE_CLOAK ) // cloaks
					{
						if (myStats->cloak == nullptr) // nothing on back currently
						{
							myStats->cloak = item; // pick up the armor.
							item = nullptr;
							list_RemoveNode(entity->mynode);
						}
						else
						{
							if (myStats->cloak->beatitude >= 0)   // if the armor is not cursed, proceed. Won't do anything if the armor is cursed.
							{
								// to compare the armors, we use the AC function to check the Armor Class of the equipment the goblin
								// is currently wearing versus the Armor Class that the goblin would have if it had the new armor.
								currentAC = AC(myStats);
								oldarmor = myStats->cloak;
								myStats->cloak = item;
								newAC = AC(myStats);
								myStats->cloak = oldarmor;

								//If the new armor is better than the current armor.
								if (newAC > currentAC)
								{
									dropItemMonster(myStats->cloak, this, myStats);
									myStats->cloak = item;
									item = nullptr;
									list_RemoveNode(entity->mynode);
								}
							}
						}
					}
					if ( glovesandshoes && item != nullptr )
					{
						if ( checkEquipType(item) == TYPE_BOOTS ) // boots
						{
							if (myStats->shoes == nullptr)
							{
								myStats->shoes = item; // pick up the armor
								item = nullptr;
								list_RemoveNode(entity->mynode);
							}
							else
							{
								if (myStats->shoes->beatitude >= 0) // if the armor is not cursed, proceed. Won't do anything if the armor is cursed.
								{
									// to compare the armors, we use the AC function to check the Armor Class of the equipment the goblin
									// is currently wearing versus the Armor Class that the goblin would have if it had the new armor.
									currentAC = AC(myStats);
									oldarmor = myStats->shoes;
									myStats->shoes = item;
									newAC = AC(myStats);
									myStats->shoes = oldarmor;

									//If the new armor is better than the current armor.
									if (newAC > currentAC)
									{
										dropItemMonster(myStats->shoes, this, myStats);
										myStats->shoes = item;
										item = nullptr;
										list_RemoveNode(entity->mynode);
									}
								}
							}
						}
						else if ( checkEquipType(item) == TYPE_GLOVES )
						{
							if (myStats->gloves == nullptr)
							{
								myStats->gloves = item; // pick up the armor
								item = nullptr;
								list_RemoveNode(entity->mynode);
							}
							else
							{
								if (myStats->gloves->beatitude >= 0) // if the armor is not cursed, proceed. Won't do anything if the armor is cursed.
								{
									// to compare the armors, we use the AC function to check the Armor Class of the equipment the goblin
									// is currently wearing versus the Armor Class that the goblin would have if it had the new armor.
									currentAC = AC(myStats);
									oldarmor = myStats->gloves;
									myStats->gloves = item;
									newAC = AC(myStats);
									myStats->gloves = oldarmor;

									//If the new armor is better than the current armor.
									if (newAC > currentAC)
									{
										dropItemMonster(myStats->gloves, this, myStats);
										myStats->gloves = item;
										item = nullptr;
										list_RemoveNode(entity->mynode);
									}
								}
							}
						}
					}
				}
				else if ( itemCategory(item) == POTION )
				{
					if ( myStats->weapon == nullptr ) //Not currently holding a weapon.
					{
						myStats->weapon = item; //Assign the monster's weapon.
						item = nullptr;
						list_RemoveNode(entity->mynode);
					}
					//Don't pick up if already wielding something.
				}
				else if ( itemCategory(item) == THROWN )
				{
					if ( myStats->weapon == nullptr ) //Not currently holding a weapon.
					{
						if ( !entity->itemNotMoving && entity->parent && entity->parent != uid )
						{
							//Don't pick up the item.
						}
						else
						{
							myStats->weapon = item; //Assign the monster's weapon.
							item = nullptr;
							list_RemoveNode(entity->mynode);
						}
					}
					//Don't pick up if already wielding something.
				}

				if (item != nullptr)
				{
					free(item);
				}
			}
		}

		list_FreeAll(items);
		free(items);
	}
}

/*-------------------------------------------------------------------------------

	uidToEntity

	Returns an entity pointer from the given entity UID, provided one exists.
	Otherwise returns NULL

-------------------------------------------------------------------------------*/

Entity* uidToEntity(Sint32 uidnum)
{
	node_t* node;
	Entity* entity;

	auto it = map.entities_map.find(uidnum);
	if(it != map.entities_map.end())
		return (Entity*)it->second->element;

	return NULL;
}

/*-------------------------------------------------------------------------------

	Entity::setHP

	sets the HP of the given entity

-------------------------------------------------------------------------------*/

void Entity::setHP(int amount)
{
	Stat* entitystats = this->getStats();

	if (this->behavior == &actPlayer && godmode)
	{
		amount = entitystats->MAXHP;
	}
	if (!entitystats || amount == entitystats->HP)
	{
		return;
	}
	entitystats->HP = std::min(std::max(0, amount), entitystats->MAXHP);
	strncpy(entitystats->obituary, language[1500], 127);

	int i = 0;
	if (multiplayer == SERVER)
	{
		for (i = 1; i < numplayers; i++)
		{
			if (this == players[i]->entity)
			{
				// tell the client its HP changed
				strcpy((char*)net_packet->data, "UPHP");
				SDLNet_Write32((Uint32)entitystats->HP, &net_packet->data[4]);
				SDLNet_Write32((Uint32)NOTHING, &net_packet->data[8]);
				net_packet->address.host = net_clients[i - 1].host;
				net_packet->address.port = net_clients[i - 1].port;
				net_packet->len = 12;
				sendPacketSafe(net_sock, -1, net_packet, i - 1);
			}
		}
	}
}

/*-------------------------------------------------------------------------------

	Entity::modHP

	modifies the HP of the given entity

-------------------------------------------------------------------------------*/

void Entity::modHP(int amount)
{
	Stat* entitystats = this->getStats();

	if ( this->behavior == &actPlayer && godmode && amount < 0 )
	{
		amount = 0;
	}
	if ( !entitystats || amount == 0 )
	{
		return;
	}

	this->setHP(entitystats->HP + amount);
}

/*-------------------------------------------------------------------------------

	Entity::setMP

	sets the MP of the given entity

-------------------------------------------------------------------------------*/

void Entity::setMP(int amount)
{
	Stat* entitystats = this->getStats();

	if (this->behavior == &actPlayer && godmode)
	{
		amount = entitystats->MAXMP;
	}
	if (!entitystats || amount == entitystats->MP)
	{
		return;
	}
	entitystats->MP = std::min(std::max(0, amount), entitystats->MAXMP);

	int i = 0;
	if (multiplayer == SERVER)
	{
		for (i = 1; i < numplayers; i++)
		{
			if (this == players[i]->entity)
			{
				// tell the client its MP just changed
				strcpy((char*)net_packet->data, "UPMP");
				SDLNet_Write32((Uint32)entitystats->MP, &net_packet->data[4]);
				net_packet->address.host = net_clients[i - 1].host;
				net_packet->address.port = net_clients[i - 1].port;
				net_packet->len = 8;
				sendPacketSafe(net_sock, -1, net_packet, i - 1);
			}
		}
	}
}

/*-------------------------------------------------------------------------------

	Entity::modMP

	modifies the MP of the given entity

-------------------------------------------------------------------------------*/

void Entity::modMP(int amount)
{
	Stat* entitystats = this->getStats();

	if ( !entitystats )
	{
		return;
	}

	if ( this->behavior == &actPlayer && godmode && amount < 0 )
	{
		amount = 0;
	}
	if ( !entitystats || amount == 0 )
	{
		return;
	}

	this->setMP(entitystats->MP + amount);
}

int Entity::getMP()
{
	Stat* myStats = getStats();

	if ( !myStats )
	{
		return 0;
	}

	return myStats->MP;
}

/*-------------------------------------------------------------------------------

	Entity::drainMP

	 Removes this much from MP. Anything over the entity's MP is subtracted from their health. Can be very dangerous.

-------------------------------------------------------------------------------*/

void Entity::drainMP(int amount)
{
	//A pointer to the entity's stats.
	Stat* entitystats = this->getStats();

	//Check if no stats found.
	if (entitystats == NULL || amount == 0)
	{
		return;
	}

	int overdrawn = 0;
	entitystats->MP -= amount;
	int player = -1;
	int i = 0;
	for (i = 0; i < numplayers; ++i)
	{
		if (this == players[i]->entity)
		{
			player = i; //Set the player.
		}
	}
	if (entitystats->MP < 0)
	{
		//Overdrew. Take that extra and flow it over into HP.
		overdrawn = entitystats->MP;
		entitystats->MP = 0;
	}
	if (multiplayer == SERVER)
	{
		//First check if the entity is the player.
		for (i = 1; i < numplayers; ++i)
		{
			if (this == players[i]->entity)
			{
				//It is. Tell the client its MP just changed.
				strcpy((char*)net_packet->data, "UPMP");
				SDLNet_Write32((Uint32)entitystats->MP, &net_packet->data[4]);
				SDLNet_Write32((Uint32)stats[i]->type, &net_packet->data[8]);
				net_packet->address.host = net_clients[i - 1].host;
				net_packet->address.port = net_clients[i - 1].port;
				net_packet->len = 12;
				sendPacketSafe(net_sock, -1, net_packet, i - 1);
			}
		}
	}
	else if (clientnum != 0 && multiplayer == CLIENT)
	{
		if (this == players[clientnum]->entity)
		{
			//It's the player entity. Tell the server its MP changed.
			strcpy((char*)net_packet->data, "UPMP");
			net_packet->data[4] = clientnum;
			SDLNet_Write32((Uint32)entitystats->MP, &net_packet->data[5]);
			SDLNet_Write32((Uint32)stats[clientnum]->type, &net_packet->data[9]);
			net_packet->address.host = net_server.host;
			net_packet->address.port = net_server.port;
			net_packet->len = 13;
			sendPacketSafe(net_sock, -1, net_packet, 0);
		}
	}

	if (overdrawn < 0)
	{
		if (player >= 0)
		{
			Uint32 color = SDL_MapRGB(mainsurface->format, 255, 255, 0);
			messagePlayerColor(player, color, language[621]);
		}
		this->modHP(overdrawn); //Drain the extra magic from health.
		Stat* tempStats = this->getStats();
		if ( tempStats )
		{
			if ( tempStats->sex == MALE )
			{
				this->setObituary(language[1528]);
			}
			else
			{
				this->setObituary(language[1529]);
			}
		}
	}
}

/*-------------------------------------------------------------------------------

	Entity::safeConsumeMP

	A function for the magic code. Attempts to remove mana without overdrawing the player. Returns true if success, returns false if didn't have enough mana.

-------------------------------------------------------------------------------*/

bool Entity::safeConsumeMP(int amount)
{
	Stat* stat = this->getStats();

	//Check if no stats found.
	if (!stat)
	{
		return false;
	}

	if (amount > stat->MP)
	{
		return false;    //Not enough mana.
	}
	else
	{
		this->modMP(-amount);
		return true;
	}

	return false;
}

/*-------------------------------------------------------------------------------

	Entity::handleEffects

	processes general character status updates for a given entity, such as
	hunger, level ups, poison, etc.

-------------------------------------------------------------------------------*/

void Entity::handleEffects(Stat* myStats)
{
	int increasestat[3];
	int i, c;
	int player = -1;

	if ( this->behavior == &actPlayer )
	{
		player = this->skill[2];

		// god mode and buddha mode
		if ( godmode )
		{
			myStats->HP = myStats->MAXHP;
			myStats->MP = myStats->MAXMP;
		}
		else if ( buddhamode )
		{
			if ( myStats->HP <= 0 )
			{
				myStats->HP = 1;
			}
		}
	}

	// sleep Zs
	if ( myStats->EFFECTS[EFF_ASLEEP] && ticks % 30 == 0 )
	{
		spawnSleepZ(this->x + cos(this->yaw) * 2, this->y + sin(this->yaw) * 2, this->z);
	}

	// level ups
	if ( myStats->EXP >= 100 )
	{
		myStats->EXP -= 100;
		myStats->LVL++;
		Uint32 color = SDL_MapRGB(mainsurface->format, 255, 255, 0);
		messagePlayerColor(player, color, language[622]);
		playSoundPlayer(player, 97, 128);

		// increase MAXHP/MAXMP
		myStats->HP += HP_MOD;
		myStats->MAXHP += HP_MOD;
		myStats->HP = std::min(myStats->HP, myStats->MAXHP);
		myStats->MP += MP_MOD;
		myStats->MAXMP += MP_MOD;
		myStats->MP = std::min(myStats->MP, myStats->MAXMP);

		// now pick three attributes to increase
		// changed rolls to be unique for each possibility.
		increasestat[0] = rand() % 6;
		int r = rand() % 6;
		while ( r == increasestat[0] ) {
			r = rand() % 6;
		}
		increasestat[1] = r;
		r = rand() % 6;
		while ( r == increasestat[0] || r == increasestat[1] ) {
			r = rand() % 6;
		}
		increasestat[2] = r;

		// debug
		// messagePlayer(0, "Stats rolled: %d %d %d", increasestat[0], increasestat[1], increasestat[2]);

		/*increasestat[0] = rand() % 6;
		increasestat[1] = rand() % 5;
		increasestat[2] = rand() % 4;
		if ( increasestat[1] >= increasestat[0] )
		{
			increasestat[1]++;
		}
		if ( increasestat[2] >= increasestat[0] )
		{
			increasestat[2]++;
		}
		if ( increasestat[2] >= increasestat[1] )
		{
			increasestat[2]++;
		}*/

		for ( i = 0; i < 6; i++ )
		{
			myStats->PLAYER_LVL_STAT_TIMER[i] = 0;
		}

		bool rolledBonusStat = false;
		int statIconTicks = 250;

		for ( i = 0; i < 3; i++ )
		{
			messagePlayerColor(player, color, language[623 + increasestat[i]]);
			switch ( increasestat[i] )
			{
				case STAT_STR: // STR
					myStats->STR++;
					myStats->PLAYER_LVL_STAT_TIMER[increasestat[i]] = statIconTicks;
					if ( myStats->PLAYER_LVL_STAT_BONUS[increasestat[i]] >= PRO_LOCKPICKING && !rolledBonusStat)
					{
						if ( rand() % 5 == 0 )
						{
							myStats->STR++;
							rolledBonusStat = true;
							myStats->PLAYER_LVL_STAT_TIMER[increasestat[i] + NUMSTATS] = statIconTicks;
							//messagePlayer(0, "Rolled bonus in %d", increasestat[i]);
						}
					}
					break;
				case STAT_DEX: // DEX
					myStats->DEX++;
					myStats->PLAYER_LVL_STAT_TIMER[increasestat[i]] = statIconTicks;
					if ( myStats->PLAYER_LVL_STAT_BONUS[increasestat[i]] >= PRO_LOCKPICKING && !rolledBonusStat )
					{
						if ( rand() % 5 == 0 )
						{
							myStats->DEX++;
							rolledBonusStat = true;
							myStats->PLAYER_LVL_STAT_TIMER[increasestat[i] + NUMSTATS] = statIconTicks;
							//messagePlayer(0, "Rolled bonus in %d", increasestat[i]);
						}
					}
					break;
				case STAT_CON: // CON
					myStats->CON++;
					myStats->PLAYER_LVL_STAT_TIMER[increasestat[i]] = statIconTicks;
					if ( myStats->PLAYER_LVL_STAT_BONUS[increasestat[i]] >= PRO_LOCKPICKING && !rolledBonusStat )
					{
						if ( rand() % 5 == 0 )
						{
							myStats->CON++;
							rolledBonusStat = true;
							myStats->PLAYER_LVL_STAT_TIMER[increasestat[i] + NUMSTATS] = statIconTicks;
							//messagePlayer(0, "Rolled bonus in %d", increasestat[i]);
						}
					}
					break;
				case STAT_INT: // INT
					myStats->INT++;
					myStats->PLAYER_LVL_STAT_TIMER[increasestat[i]] = statIconTicks;
					if ( myStats->PLAYER_LVL_STAT_BONUS[increasestat[i]] >= PRO_LOCKPICKING && !rolledBonusStat )
					{
						if ( rand() % 5 == 0 )
						{
							myStats->INT++;
							rolledBonusStat = true;
							myStats->PLAYER_LVL_STAT_TIMER[increasestat[i] + NUMSTATS] = statIconTicks;
							//messagePlayer(0, "Rolled bonus in %d", increasestat[i]);
						}
					}
					break;
				case STAT_PER: // PER
					myStats->PER++;
					myStats->PLAYER_LVL_STAT_TIMER[increasestat[i]] = statIconTicks;
					if ( myStats->PLAYER_LVL_STAT_BONUS[increasestat[i]] >= PRO_LOCKPICKING && !rolledBonusStat )
					{
						if ( rand() % 5 == 0 )
						{
							myStats->PER++;
							rolledBonusStat = true;
							myStats->PLAYER_LVL_STAT_TIMER[increasestat[i] + NUMSTATS] = statIconTicks;
							//messagePlayer(0, "Rolled bonus in %d", increasestat[i]);
						}
					}
					break;
				case STAT_CHR: // CHR
					myStats->CHR++;
					myStats->PLAYER_LVL_STAT_TIMER[increasestat[i]] = statIconTicks;
					if ( myStats->PLAYER_LVL_STAT_BONUS[increasestat[i]] >= PRO_LOCKPICKING && !rolledBonusStat )
					{
						if ( rand() % 5 == 0 )
						{
							myStats->CHR++;
							rolledBonusStat = true;
							myStats->PLAYER_LVL_STAT_TIMER[increasestat[i] + NUMSTATS] = statIconTicks;
							//messagePlayer(0, "Rolled bonus in %d", increasestat[i]);
						}
					}
					break;
			}
		}

		// inform clients of stat changes
		if ( multiplayer == SERVER && player > 0 )
		{
			strcpy((char*)net_packet->data, "ATTR");
			net_packet->data[4] = clientnum;
			net_packet->data[5] = (Sint8)myStats->STR;
			net_packet->data[6] = (Sint8)myStats->DEX;
			net_packet->data[7] = (Sint8)myStats->CON;
			net_packet->data[8] = (Sint8)myStats->INT;
			net_packet->data[9] = (Sint8)myStats->PER;
			net_packet->data[10] = (Sint8)myStats->CHR;
			net_packet->data[11] = (Sint8)myStats->EXP;
			net_packet->data[12] = (Sint8)myStats->LVL;
			SDLNet_Write16((Sint16)myStats->HP, &net_packet->data[13]);
			SDLNet_Write16((Sint16)myStats->MAXHP, &net_packet->data[15]);
			SDLNet_Write16((Sint16)myStats->MP, &net_packet->data[17]);
			SDLNet_Write16((Sint16)myStats->MAXMP, &net_packet->data[19]);
			net_packet->address.host = net_clients[player - 1].host;
			net_packet->address.port = net_clients[player - 1].port;
			net_packet->len = 21;
			sendPacketSafe(net_sock, -1, net_packet, player - 1);
		}

		for ( i = 0; i < NUMSTATS; i++ )
		{
			myStats->PLAYER_LVL_STAT_BONUS[i] = -1;
		}
	}

	// hunger
	int hungerring = 0;
	if ( myStats->ring != NULL )
	{
		if ( myStats->ring->type == RING_SLOWDIGESTION )
		{
			if ( myStats->ring->beatitude >= 0 )
			{
				hungerring = 1;
			}
			else
			{
				hungerring = -1;
			}
		}
	}
	if ( (ticks % 30 == 0 && !hungerring) || (ticks % 15 == 0 && hungerring < 0) || (ticks % 120 == 0 && hungerring > 0) )
	{
		if ( myStats->HUNGER > 0 )
		{
			if ( svFlags & SV_FLAG_HUNGER )
			{
				myStats->HUNGER--;
			}
			else if ( myStats->HUNGER != 800 )
			{
				myStats->HUNGER = 800;
				serverUpdateHunger(player);
			}
			if ( myStats->HUNGER == 1500 )
			{
				if ( !myStats->EFFECTS[EFF_VOMITING] )
				{
					messagePlayer(player, language[629]);
				}
				serverUpdateHunger(player);
			}
			else if ( myStats->HUNGER == 250 )
			{
				if ( !myStats->EFFECTS[EFF_VOMITING] )
				{
					messagePlayer(player, language[630]);
					playSoundPlayer(player, 32, 128);
				}
				serverUpdateHunger(player);
			}
			else if ( myStats->HUNGER == 150 )
			{
				if ( !myStats->EFFECTS[EFF_VOMITING] )
				{
					messagePlayer(player, language[631]);
					playSoundPlayer(player, 32, 128);
				}
				serverUpdateHunger(player);
			}
			else if ( myStats->HUNGER == 50 )
			{
				if ( !myStats->EFFECTS[EFF_VOMITING] )
				{
					messagePlayer(player, language[632]);
					playSoundPlayer(player, 32, 128);
				}
				serverUpdateHunger(player);
			}
		}
		else
		{
			myStats->HUNGER = 0;
			if ( !myStats->EFFECTS[EFF_VOMITING] && ticks % 120 == 0 )
			{
				serverUpdateHunger(player);
				if ( player >= 0 )   // bad guys don't starve. Sorry.
				{
					this->modHP(-4);
				}
				if ( myStats->HP > 0 )
				{
					messagePlayer(player, language[633]);
				}
				this->setObituary(language[1530]);
			}
		}
	}

	// "random" vomiting
	if ( !this->char_gonnavomit && !myStats->EFFECTS[EFF_VOMITING] )
	{
		if ( myStats->HUNGER > 1500 && rand() % 1000 == 0 )
		{
			// oversatiation
			messagePlayer(player, language[634]);
			this->char_gonnavomit = 140 + rand() % 60;
		}
		else if ( ticks % 60 == 0 && rand() % 200 == 0 && myStats->EFFECTS[EFF_DRUNK] )
		{
			// drunkenness
			messagePlayer(player, language[634]);
			this->char_gonnavomit = 140 + rand() % 60;
		}
	}
	if ( this->char_gonnavomit > 0 )
	{
		this->char_gonnavomit--;
		if ( this->char_gonnavomit == 0 )
		{
			messagePlayer(player, language[635]);
			myStats->EFFECTS[EFF_VOMITING] = true;
			myStats->EFFECTS_TIMERS[EFF_VOMITING] = 50 + rand() % 20;
			serverUpdateEffects(player);
			if ( player == clientnum )
			{
				camera_shakey += 9;
			}
			else if ( player > 0 && multiplayer == SERVER )
			{
				strcpy((char*)net_packet->data, "SHAK");
				net_packet->data[4] = 0; // turns into 0
				net_packet->data[5] = 9;
				net_packet->address.host = net_clients[player - 1].host;
				net_packet->address.port = net_clients[player - 1].port;
				net_packet->len = 6;
				sendPacketSafe(net_sock, -1, net_packet, player - 1);
			}
			playSoundEntity(this, 78, 96);
		}
	}

	// vomiting
	if ( myStats->EFFECTS[EFF_VOMITING] && ticks % 2 == 0 )
	{
		Entity* entity = spawnGib(this);
		entity->sprite = 29;
		entity->flags[SPRITE] = true;
		entity->flags[GENIUS] = true;
		entity->flags[INVISIBLE] = false;
		entity->yaw = this->yaw - 0.1 + (rand() % 20) * 0.01;
		entity->pitch = (rand() % 360) * PI / 180.0;
		entity->roll = (rand() % 360) * PI / 180.0;
		double vel = (rand() % 15) / 10.f;
		entity->vel_x = vel * cos(entity->yaw);
		entity->vel_y = vel * sin(entity->yaw);
		entity->vel_z = -.5;
		myStats->HUNGER -= 40;
		if ( myStats->HUNGER <= 50 )
		{
			myStats->HUNGER = 50;
			myStats->EFFECTS_TIMERS[EFF_VOMITING] = 1;
		}
		serverSpawnGibForClient(entity);
	}

	// healing over time
	int healring = 0;
	int healthRegenInterval = 0;
	if ( myStats->ring != NULL )
	{
		if ( myStats->ring->type == RING_REGENERATION )
		{
			if ( myStats->ring->beatitude >= 0 )
			{
				healring++;
			}
			else
			{
				healring--;
			}
		}
	}
	if ( myStats->breastplate != NULL )
	{
		if ( myStats->breastplate->type == ARTIFACT_BREASTPIECE )
		{
			if ( myStats->breastplate->beatitude >= 0 )
			{
				healring++;
			}
			else
			{
				healring--;
			}
		}
	}

	if ( healring > 0 )
	{
		healthRegenInterval = HEAL_TIME / (healring * 8);
	}
	else if ( healring < 0 )
	{
		healthRegenInterval = healring * HEAL_TIME * 4;
	}
	else if ( healring == 0 )
	{
		healthRegenInterval = HEAL_TIME;
	}
	if ( myStats->HP < myStats->MAXHP )
	{
		this->char_heal++;
		if ( healring > 0 || svFlags & SV_FLAG_HUNGER )
		{
			if ( this->char_heal >= healthRegenInterval )
			{
				this->char_heal = 0;
				this->modHP(1);
			}
		}
	}
	else
	{
		this->char_heal = 0;
	}

	// random teleportation
	if ( myStats->ring != NULL )
	{
		if ( myStats->ring->type == RING_TELEPORTATION )
		{
			if ( rand() % 1000 == 0 )   // .1% chance every frame
			{
				teleportRandom();
			}
		}
	}

	// regaining energy over time
	int manaring = 0;
	int manaRegenInterval = 0;
	if ( myStats->breastplate != NULL )
	{
		if ( myStats->breastplate->type == VAMPIRE_DOUBLET )
		{
			if ( myStats->breastplate->beatitude >= 0 )
			{
				manaring++;
			}
			else
			{
				manaring--;
			}
		}
	}
	if ( myStats->cloak != NULL )
	{
		if ( myStats->cloak->type == ARTIFACT_CLOAK )
		{
			if ( myStats->cloak->beatitude >= 0 )
			{
				manaring++;
			}
			else
			{
				manaring--;
			}
		}
	}

	if ( manaring > 0 )
	{
		manaRegenInterval = MAGIC_REGEN_TIME / (manaring * 8);
	}
	else if ( manaring < 0 )
	{
		manaRegenInterval = manaring * MAGIC_REGEN_TIME * 4;
	}
	else if ( manaring == 0 )
	{
		manaRegenInterval = MAGIC_REGEN_TIME;
	}

	if ( myStats->MP < myStats->MAXMP )
	{
		this->char_energize++;
		if ( this->char_energize >= manaRegenInterval )
		{
			this->char_energize = 0;
			this->modMP(1);
		}
	}
	else
	{
		this->char_energize = 0;
	}

	// effects of greasy fingers
	if ( myStats->EFFECTS[EFF_GREASY] == true )
	{
		if ( myStats->weapon != NULL )
		{
			messagePlayer(player, language[636]);
			if ( player >= 0 )
			{
				dropItem(myStats->weapon, player);
				if ( player > 0 && multiplayer == SERVER )
				{
					strcpy((char*)net_packet->data, "DROP");
					net_packet->data[4] = 5;
					net_packet->address.host = net_clients[player - 1].host;
					net_packet->address.port = net_clients[player - 1].port;
					net_packet->len = 5;
					sendPacketSafe(net_sock, -1, net_packet, player - 1);
				}
			}
			else
			{
				dropItemMonster(myStats->weapon, this, myStats);
			}
			myStats->weapon = NULL;
		}
	}

	// torches/lamps burn down
	if ( myStats->shield != NULL )
	{
		if ( myStats->shield->type == TOOL_TORCH || myStats->shield->type == TOOL_LANTERN )
		{
			this->char_torchtime++;
			if ( (this->char_torchtime >= 7200 && myStats->shield->type == TOOL_TORCH) || (this->char_torchtime >= 10260) )
			{
				this->char_torchtime = 0;
				if ( player == clientnum )
				{
					if ( myStats->shield->count > 1 )
					{
						newItem(myStats->shield->type, myStats->shield->status, myStats->shield->beatitude, myStats->shield->count - 1, myStats->shield->appearance, myStats->shield->identified, &myStats->inventory);
					}
				}
				myStats->shield->count = 1;
				myStats->shield->status = static_cast<Status>(myStats->shield->status - 1);
				if ( myStats->shield->status > BROKEN )
				{
					messagePlayer(player, language[637], myStats->shield->getName());
				}
				else
				{
					messagePlayer(player, language[638], myStats->shield->getName());
				}
				if ( multiplayer == SERVER && player > 0 )
				{
					strcpy((char*)net_packet->data, "ARMR");
					net_packet->data[4] = 4;
					net_packet->data[5] = myStats->shield->status;
					net_packet->address.host = net_clients[player - 1].host;
					net_packet->address.port = net_clients[player - 1].port;
					net_packet->len = 6;
					sendPacketSafe(net_sock, -1, net_packet, player - 1);
				}
			}
		}
	}

	// effects of being poisoned
	if ( myStats->EFFECTS[EFF_POISONED] )
	{
		if ( myStats->amulet != NULL )
		{
			if ( myStats->amulet->type == AMULET_POISONRESISTANCE )
			{
				messagePlayer(player, language[639]);
				messagePlayer(player, language[640]);
				myStats->EFFECTS_TIMERS[EFF_POISONED] = 0;
				myStats->EFFECTS[EFF_POISONED] = false;
				serverUpdateEffects(player);
				this->char_poison = 0;
			}
		}
		this->char_poison++;
		if ( this->char_poison > 180 )   // three seconds
		{
			this->char_poison = 0;
			int poisonhurt = std::max(1 + rand() % 4 - myStats->CON, 3);
			this->modHP(-poisonhurt);
			if ( myStats->HP <= 0 )
			{
				Entity* killer = uidToEntity( myStats->poisonKiller );
				if ( killer )
				{
					killer->awardXP( this, true, true );
				}
			}
			this->setObituary(language[1531]);
			playSoundEntity(this, 28, 64);
			if ( player == clientnum )
			{
				camera_shakex += .1;
				camera_shakey += 10;
			}
			else if ( player > 0 && multiplayer == SERVER )
			{
				strcpy((char*)net_packet->data, "SHAK");
				net_packet->data[4] = 10; // turns into .1
				net_packet->data[5] = 10;
				net_packet->address.host = net_clients[player - 1].host;
				net_packet->address.port = net_clients[player - 1].port;
				net_packet->len = 6;
				sendPacketSafe(net_sock, -1, net_packet, player - 1);
			}
			if ( rand() % 5 == 0 )
			{
				messagePlayer(player, language[641]);
				myStats->EFFECTS_TIMERS[EFF_POISONED] = 0;
				myStats->EFFECTS[EFF_POISONED] = false;
				serverUpdateEffects(player);
			}
		}
	}
	else
	{
		this->char_poison = 0;
	}

	// bleeding
	if ( myStats->EFFECTS[EFF_BLEEDING] )
	{
		if ( ticks % 120 == 0 )
		{
			if ( myStats->HP > 5 )
			{
				int bleedhurt = 1;
				this->modHP(-bleedhurt);
				this->setObituary(language[1532]);
				Entity* gib = spawnGib(this);
				serverSpawnGibForClient(gib);
				if ( player == clientnum )
				{
					camera_shakex -= .03;
					camera_shakey += 3;
				}
				else if ( player > 0 && multiplayer == SERVER )
				{
					strcpy((char*)net_packet->data, "SHAK");
					net_packet->data[4] = -3; // turns into -.03
					net_packet->data[5] = 3;
					net_packet->address.host = net_clients[player - 1].host;
					net_packet->address.port = net_clients[player - 1].port;
					net_packet->len = 6;
					sendPacketSafe(net_sock, -1, net_packet, player - 1);
				}
				messagePlayer(player, language[642]);
				if ( spawn_blood )
				{
					Entity* entity = NULL;
					if ( gibtype[myStats->type] == 1 )
					{
						entity = newEntity(203, 1, map.entities);
					}
					else if ( gibtype[myStats->type] == 2 )
					{
						entity = newEntity(213, 1, map.entities);
					}
					if ( entity != NULL )
					{
						entity->x = this->x;
						entity->y = this->y;
						entity->z = 7.4 + (rand() % 20) / 100.f;
						entity->parent = this->uid;
						entity->sizex = 2;
						entity->sizey = 2;
						entity->yaw = (rand() % 360) * PI / 180.0;
						entity->flags[UPDATENEEDED] = true;
						entity->flags[PASSABLE] = true;
					}
				}
			}
			else
			{
				messagePlayer(player, language[643]);
				myStats->EFFECTS[EFF_BLEEDING] = false;
				myStats->EFFECTS_TIMERS[EFF_BLEEDING] = 0;
				serverUpdateEffects(player);
			}
		}
	}

	if ( myStats->EFFECTS[EFF_MAGICREFLECT] )
	{
		spawnAmbientParticles(80, 579, 10 + rand() % 40);
	}

	// burning
	if ( this->flags[BURNING] )
	{
		if ( ticks % 30 == 0 )
		{
			this->modHP(-2 - rand() % 3);
			if ( myStats->HP <= 0 )
			{
				Entity* killer = uidToEntity( myStats->poisonKiller );
				if ( killer )
				{
					killer->awardXP( this, true, true );
				}
			}
			this->setObituary(language[1533]);
			messagePlayer(player, language[644]);
			playSoundEntity(this, 28, 64);
			if ( player == clientnum )
			{
				camera_shakey += 3;
			}
			else if ( player > 0 && multiplayer == SERVER )
			{
				strcpy((char*)net_packet->data, "SHAK");
				net_packet->data[4] = 0; // turns into 0
				net_packet->data[5] = 3;
				net_packet->address.host = net_clients[player - 1].host;
				net_packet->address.port = net_clients[player - 1].port;
				net_packet->len = 6;
				sendPacketSafe(net_sock, -1, net_packet, player - 1);
			}
			if ( rand() % 10 == 0 )
			{
				if ( myStats->cloak != NULL )
				{
					if ( player == clientnum )
					{
						if ( myStats->cloak->count > 1 )
						{
							newItem(myStats->cloak->type, myStats->cloak->status, myStats->cloak->beatitude, myStats->cloak->count - 1, myStats->cloak->appearance, myStats->cloak->identified, &myStats->inventory);
						}
					}
					myStats->cloak->count = 1;
					myStats->cloak->status = static_cast<Status>(myStats->cloak->status - 1);
					if ( myStats->cloak->status != BROKEN )
					{
						messagePlayer(player, language[645], myStats->cloak->getName());
					}
					else
					{
						messagePlayer(player, language[646], myStats->cloak->getName());
					}
					if ( player > 0 && multiplayer == SERVER )
					{
						strcpy((char*)net_packet->data, "ARMR");
						net_packet->data[4] = 6;
						net_packet->data[5] = myStats->cloak->status;
						net_packet->address.host = net_clients[player - 1].host;
						net_packet->address.port = net_clients[player - 1].port;
						net_packet->len = 6;
						sendPacketSafe(net_sock, -1, net_packet, player - 1);
					}
				}
			}
			if ( rand() % 10 == 0 )
			{
				this->flags[BURNING] = false;
				messagePlayer(player, language[647]);
				if ( player > 0 && multiplayer == SERVER )
				{
					serverUpdateEntityFlag(this, BURNING);
				}
			}
		}
	}

	// amulet effects
	if ( myStats->amulet != NULL )
	{
		// strangulation
		if ( myStats->amulet->type == AMULET_STRANGULATION )
		{
			if ( ticks % 60 == 0 )
			{
				if ( rand() % 25 )
				{
					messagePlayer(player, language[648]);
					this->modHP(-(2 + rand() % 3));
					this->setObituary(language[1534]);
					if ( myStats->HP <= 0 )
					{
						if ( player <= 0 )
						{
							Item* item = myStats->amulet;
							if ( item->count > 1 )
							{
								newItem(item->type, item->status, item->beatitude, item->count - 1, item->appearance, item->identified, &myStats->inventory);
							}
						}
						myStats->amulet->count = 1;
						myStats->amulet->status = BROKEN;
						playSoundEntity(this, 76, 64);
						if ( player > 0 && multiplayer == SERVER )
						{
							strcpy((char*)net_packet->data, "ARMR");
							net_packet->data[4] = 7;
							net_packet->data[5] = myStats->amulet->status;
							net_packet->address.host = net_clients[player - 1].host;
							net_packet->address.port = net_clients[player - 1].port;
							net_packet->len = 6;
							sendPacketSafe(net_sock, -1, net_packet, player - 1);
						}
					}
					if ( player == clientnum )
					{
						camera_shakey += 8;
					}
					else if ( player > 0 && multiplayer == SERVER )
					{
						strcpy((char*)net_packet->data, "SHAK");
						net_packet->data[4] = 0; // turns into 0
						net_packet->data[5] = 8;
						net_packet->address.host = net_clients[player - 1].host;
						net_packet->address.port = net_clients[player - 1].port;
						net_packet->len = 6;
						sendPacketSafe(net_sock, -1, net_packet, player - 1);
					}
				}
				else
				{
					messagePlayer(player, language[649]);
					messagePlayer(player, language[650]);
					if ( player <= 0 )
					{
						Item* item = myStats->amulet;
						if ( item->count > 1 )
						{
							newItem(item->type, item->status, item->beatitude, item->count - 1, item->appearance, item->identified, &myStats->inventory);
						}
					}
					myStats->amulet->count = 1;
					myStats->amulet->status = BROKEN;
					playSoundEntity(this, 76, 64);
					if ( player > 0 && multiplayer == SERVER )
					{
						strcpy((char*)net_packet->data, "ARMR");
						net_packet->data[4] = 7;
						net_packet->data[5] = myStats->amulet->status;
						net_packet->address.host = net_clients[player - 1].host;
						net_packet->address.port = net_clients[player - 1].port;
						net_packet->len = 6;
						sendPacketSafe(net_sock, -1, net_packet, player - 1);
					}
				}
			}
		}
		// life saving
		if ( myStats->amulet->type == AMULET_LIFESAVING )   //TODO: Doesn't save against boulder traps.
		{
			if ( myStats->HP <= 0 )
			{
				if ( myStats->HUNGER > 0 )
				{
					messagePlayer(player, language[651]);
				}
				if ( !this->isBlind())
				{
					messagePlayer(player, language[652]);
				}
				else
				{
					messagePlayer(player, language[653]);
				}
				if ( myStats->amulet->beatitude >= 0 )
				{
					messagePlayer(player, language[654]);
					messagePlayer(player, language[655]);
					steamAchievementClient(player, "BARONY_ACH_BORN_AGAIN");
					myStats->HUNGER = 800;
					if ( myStats->MAXHP < 10 )
					{
						myStats->MAXHP = 10;
						if ( player > 0 && multiplayer == SERVER )
						{
							strcpy((char*)net_packet->data, "ATTR");
							net_packet->data[4] = clientnum;
							net_packet->data[5] = (Sint8)myStats->STR;
							net_packet->data[6] = (Sint8)myStats->DEX;
							net_packet->data[7] = (Sint8)myStats->CON;
							net_packet->data[8] = (Sint8)myStats->INT;
							net_packet->data[9] = (Sint8)myStats->PER;
							net_packet->data[10] = (Sint8)myStats->CHR;
							net_packet->data[11] = (Sint8)myStats->EXP;
							net_packet->data[12] = (Sint8)myStats->LVL;
							SDLNet_Write16((Sint16)myStats->HP, &net_packet->data[13]);
							SDLNet_Write16((Sint16)myStats->MAXHP, &net_packet->data[15]);
							SDLNet_Write16((Sint16)myStats->MP, &net_packet->data[17]);
							SDLNet_Write16((Sint16)myStats->MAXMP, &net_packet->data[19]);
							net_packet->address.host = net_clients[player - 1].host;
							net_packet->address.port = net_clients[player - 1].port;
							net_packet->len = 21;
							sendPacketSafe(net_sock, -1, net_packet, player - 1);
						}
					}
					this->setHP(std::max(myStats->MAXHP, 10));
					for ( c = 0; c < NUMEFFECTS; c++ )
					{
						myStats->EFFECTS[c] = false;
						myStats->EFFECTS_TIMERS[c] = 0;
					}
					this->flags[BURNING] = false;
					serverUpdateEntityFlag(this, BURNING);
					serverUpdateEffects(player);
				}
				else
				{
					messagePlayer(player, language[656]);
					messagePlayer(player, language[657]);
				}
				myStats->amulet->status = BROKEN;
				playSoundEntity(this, 76, 64);
				if ( player > 0 && multiplayer == SERVER )
				{
					strcpy((char*)net_packet->data, "ARMR");
					net_packet->data[4] = 7;
					net_packet->data[5] = myStats->amulet->status;
					net_packet->address.host = net_clients[player - 1].host;
					net_packet->address.port = net_clients[player - 1].port;
					net_packet->len = 6;
					sendPacketSafe(net_sock, -1, net_packet, player - 1);
				}
				myStats->amulet = NULL;
			}
		}
	}

	// unparalyze certain boss characters
	if ( myStats->EFFECTS[EFF_PARALYZED] && ( (myStats->type >= LICH && myStats->type < KOBOLD)
		|| myStats->type == COCKATRICE || myStats->type == LICH_FIRE || myStats->type == LICH_ICE) )
	{
		myStats->EFFECTS[EFF_PARALYZED] = false;
		myStats->EFFECTS_TIMERS[EFF_PARALYZED] = 0;
	}

	// wake up
	if ( myStats->EFFECTS[EFF_ASLEEP] && (myStats->OLDHP != myStats->HP || (myStats->type >= LICH && myStats->type < KOBOLD)
		|| myStats->type == COCKATRICE || myStats->type == LICH_FIRE || myStats->type == LICH_ICE) )
	{
		messagePlayer(player, language[658]);
		myStats->EFFECTS[EFF_ASLEEP] = false;
		myStats->EFFECTS_TIMERS[EFF_ASLEEP] = 0;
		serverUpdateEffects(player);
	}
	myStats->OLDHP = myStats->HP;
}

/*-------------------------------------------------------------------------------

	Entity::getAttack

	returns the attack power of an entity based on strength, weapon, and a
	base number

-------------------------------------------------------------------------------*/

Sint32 Entity::getAttack()
{
	Stat* entitystats;
	Sint32 attack = 0;

	if ( (entitystats = this->getStats()) == NULL )
	{
		return 0;
	}

	attack = 8; // base attack strength
	if ( entitystats->weapon != NULL)
	{
		attack += entitystats->weapon->weaponGetAttack();
	}
	else if ( entitystats->weapon == NULL )
	{
		// bare handed.
		if ( entitystats->gloves )
		{
			if ( entitystats->gloves->type == BRASS_KNUCKLES )
			{
				attack += 1;
			}
			else if (entitystats->gloves->type == IRON_KNUCKLES)
			{
				attack += 2;
			}
			else if (entitystats->gloves->type == SPIKED_GAUNTLETS)
			{
				attack += 3;
			}
		}
	}
	attack += this->getSTR();

	return attack;
}

/*-------------------------------------------------------------------------------

	Entity::getSTR()

	returns the STR attribute of an entity, post modifiers

-------------------------------------------------------------------------------*/

Sint32 Entity::getSTR()
{
	Stat* entitystats;

	if ( (entitystats = this->getStats()) == NULL )
	{
		return 0;
	}
	return statGetSTR(entitystats);
}

Sint32 statGetSTR(Stat* entitystats)
{
	Sint32 STR;

	STR = entitystats->STR;
	if ( entitystats->HUNGER >= 1500 )
	{
		STR--;
	}
	if ( entitystats->HUNGER <= 150 )
	{
		STR--;
	}
	if ( entitystats->HUNGER <= 50 )
	{
		STR--;
	}
	if ( entitystats->gloves != NULL )
		if ( entitystats->gloves->type == GAUNTLETS_STRENGTH )
		{
			STR++;
		}
	if ( entitystats->ring != NULL )
	{
		if ( entitystats->ring->type == RING_STRENGTH )
		{
			if ( entitystats->ring->beatitude >= 0 )
			{
				STR++;
			}
			else
			{
				STR--;
			}
		}
	}
	if ( entitystats->EFFECTS[EFF_DRUNK] )
	{
		switch ( entitystats->type )
		{
			case GOATMAN:
				STR += 10; //Goatman love booze.
				break;
			default:
				++STR;
				break;
		}
	}
	return STR;
}

/*-------------------------------------------------------------------------------

	Entity::getDEX

	returns the DEX attribute of an entity, post modifiers

-------------------------------------------------------------------------------*/

Sint32 Entity::getDEX()
{
	Stat* entitystats;

	if ( (entitystats = this->getStats()) == NULL )
	{
		return 0;
	}
	return statGetDEX(entitystats);
}

Sint32 statGetDEX(Stat* entitystats)
{
	Sint32 DEX;

	// paralyzed
	if ( entitystats->EFFECTS[EFF_PARALYZED] )
	{
		return -10;
	}
	if ( entitystats->EFFECTS[EFF_ASLEEP] )
	{
		return -10;
	}

	DEX = entitystats->DEX;
	if ( entitystats->EFFECTS[EFF_FAST] && !entitystats->EFFECTS[EFF_SLOW] )
	{
		DEX += 10;
	}
	if ( entitystats->EFFECTS[EFF_STUNNED] )
	{
		//DEX -= 5;
	}
	if ( entitystats->HUNGER >= 1500 )
	{
		DEX--;
	}
	if ( entitystats->HUNGER <= 150 )
	{
		DEX--;
	}
	if ( entitystats->HUNGER <= 50 )
	{
		DEX--;
	}
	if ( !entitystats->EFFECTS[EFF_FAST] && entitystats->EFFECTS[EFF_SLOW] )
	{
		DEX = std::min(DEX - 3, -2);
	}
	if ( entitystats->shoes != nullptr )
	{
		if ( entitystats->shoes->type == LEATHER_BOOTS_SPEED )
		{
			DEX++;
		}
	}
	if ( entitystats->gloves != nullptr )
	{
		if ( entitystats->gloves->type == GLOVES_DEXTERITY )
		{
			DEX++;
		}
	}
	if ( entitystats->EFFECTS[EFF_DRUNK] )
	{
		switch ( entitystats->type )
		{
			default:
				--DEX;
				break;
		}
	}
	return DEX;
}

/*-------------------------------------------------------------------------------

	Entity::getCON

	returns the CON attribute of an entity, post modifiers

-------------------------------------------------------------------------------*/

Sint32 Entity::getCON()
{
	Stat* entitystats;

	if ( (entitystats = this->getStats()) == NULL )
	{
		return 0;
	}
	return statGetCON(entitystats);
}

Sint32 statGetCON(Stat* entitystats)
{
	Sint32 CON;

	CON = entitystats->CON;
	if ( entitystats->ring != NULL )
	{
		if ( entitystats->ring->type == RING_CONSTITUTION )
		{
			if ( entitystats->ring->beatitude >= 0 )
			{
				CON++;
			}
			else
			{
				CON--;
			}
		}
	}
	if ( entitystats->gloves != NULL )
	{
		if ( entitystats->gloves->type == BRACERS_CONSTITUTION )
		{
			if ( entitystats->gloves->beatitude >= 0 )
			{
				CON++;
			}
			else
			{
				CON--;
			}
		}
	}
	return CON;
}

/*-------------------------------------------------------------------------------

	Entity::getINT

	returns the INT attribute of an entity, post modifiers

-------------------------------------------------------------------------------*/

Sint32 Entity::getINT()
{
	Stat* entitystats;

	if ( (entitystats = this->getStats()) == NULL )
	{
		return 0;
	}
	return statGetINT(entitystats);
}

Sint32 statGetINT(Stat* entitystats)
{
	Sint32 INT;

	INT = entitystats->INT;
	if ( entitystats->HUNGER <= 50 )
	{
		INT--;
	}
	if ( entitystats->helmet != NULL )
	{
		if ( entitystats->helmet->type == HAT_WIZARD )
		{
			INT++;
		}
		else if ( entitystats->helmet->type == ARTIFACT_HELM )
		{
			INT += 5;
		}
	}
	return INT;
}

/*-------------------------------------------------------------------------------

	Entity::getPER

	returns the PER attribute of an entity, post modifiers

-------------------------------------------------------------------------------*/

Sint32 Entity::getPER()
{
	Stat* entitystats;

	if ( (entitystats = this->getStats()) == NULL )
	{
		return 0;
	}
	return statGetPER(entitystats);
}

Sint32 statGetPER(Stat* entitystats)
{
	Sint32 PER;

	PER = entitystats->PER;
	if ( entitystats->HUNGER <= 50 )
	{
		PER--;
	}
	if ( entitystats->mask )
		if ( entitystats->mask->type == TOOL_GLASSES )
		{
			PER++;
		}
	return PER;
}

/*-------------------------------------------------------------------------------

	Entity::getCHR

	returns the CHR attribute of an entity, post modifiers

-------------------------------------------------------------------------------*/

Sint32 Entity::getCHR()
{
	Stat* entitystats;

	if ( (entitystats = this->getStats()) == NULL )
	{
		return 0;
	}
	return statGetCHR(entitystats);
}

Sint32 statGetCHR(Stat* entitystats)
{
	Sint32 CHR;

	CHR = entitystats->CHR;
	if ( entitystats->helmet != NULL )
		if ( entitystats->helmet->type == HAT_JESTER )
		{
			CHR++;
		}
	if ( entitystats->ring != NULL )
		if ( entitystats->ring->type == RING_ADORNMENT )
		{
			if ( entitystats->ring->beatitude >= 0 )
			{
				CHR++;
			}
			else
			{
				CHR--;
			}
		}
	return CHR;
}

/*-------------------------------------------------------------------------------

	Entity::isBlind

	returns true if the given entity is blind, and false if it is not

-------------------------------------------------------------------------------*/

bool Entity::isBlind()
{
	Stat* entitystats;
	if ( (entitystats = this->getStats()) == NULL )
	{
		return false;
	}

	// being blind
	if ( entitystats->EFFECTS[EFF_BLIND] == true )
	{
		return true;
	}

	// asleep
	if ( entitystats->EFFECTS[EFF_ASLEEP] == true )
	{
		return true;
	}

	// messy face
	if ( entitystats->EFFECTS[EFF_MESSY] == true )
	{
		return true;
	}

	// wearing blindfolds
	if ( entitystats->mask != NULL )
		if ( entitystats->mask->type == TOOL_BLINDFOLD )
		{
			return true;
		}

	return false;
}

/*-------------------------------------------------------------------------------

	Entity::isInvisible

	returns true if the given entity is invisible or else wearing something
	that would make it invisible

-------------------------------------------------------------------------------*/

bool Entity::isInvisible() const
{
	Stat* entitystats;
	if ( (entitystats = getStats()) == NULL )
	{
		return false;
	}

	// being invisible
	if ( entitystats->EFFECTS[EFF_INVISIBLE] == true )
	{
		return true;
	}

	// wearing invisibility cloaks
	if ( entitystats->cloak != NULL )
	{
		if ( entitystats->cloak->type == CLOAK_INVISIBILITY )
		{
			return true;
		}
	}

	// wearing invisibility ring
	if ( entitystats->ring != NULL )
	{
		if ( entitystats->ring->type == RING_INVISIBILITY )
		{
			return true;
		}
	}

	if ( skillCapstoneUnlockedEntity(PRO_STEALTH) )
	{
		return true;
	}

	return false;
}

/*-------------------------------------------------------------------------------

	Entity::isMobile

	returns true if the given entity can move, or false if it cannot

-------------------------------------------------------------------------------*/

bool Entity::isMobile()
{
	Stat* entitystats;
	if ( (entitystats = getStats()) == NULL )
	{
		return true;
	}

	// paralyzed
	if ( entitystats->EFFECTS[EFF_PARALYZED] )
	{
		return false;
	}

	// asleep
	if ( entitystats->EFFECTS[EFF_ASLEEP] )
	{
		return false;
	}

	// stunned
	if ( entitystats->EFFECTS[EFF_STUNNED] )
	{
		return false;
	}

	return true;
}

/*-------------------------------------------------------------------------------

	checkTileForEntity

	returns a list of entities that are occupying the map tile specified at
	(x, y)

-------------------------------------------------------------------------------*/

list_t* checkTileForEntity(int x, int y)
{
	list_t* return_val = NULL;

	//Loop through the list.
	//If the entity's x and y match the tile's x and y (correcting for the difference in the two x/y systems, of course), then the entity is on the tile.
	//Traverse map.entities...
	node_t* node = NULL;
	node_t* node2 = NULL;
	#ifdef __ARM_NEON__
	const int32x2_t xy = {x, y};
	#endif

	for ( node = map.entities->first; node != NULL; node = node->next )
	{
		if (node->element)
		{
			Entity* entity = (Entity*)node->element;
			if (entity) {
			#ifdef __ARM_NEON__
			uint32x2_t eqxy =vceq_s32(vcvt_s32_f32(vmul_n_f32(vld1_f32(&entity->x), 1.0f/16.0f)), xy);
			if ( eqxy[0] && eqxy[1] )
			#else
			if ( (int)floor((entity->x / 16)) == x && (int)floor((entity->y / 16)) == y)   //Check if the current entity is on the tile.
			#endif
			{
				//Right. So. Create the list if it doesn't exist.
				if (!return_val)
				{
					return_val = (list_t*) malloc(sizeof(list_t));
					return_val->first = NULL;
					return_val->last = NULL;
				}

				//And add the current entity to it.
				node2 = list_AddNodeLast(return_val);
				node2->element = entity;
				node2->deconstructor = &emptyDeconstructor;
			}
			}
		}
	}

	return return_val;
}

/*-------------------------------------------------------------------------------

	getItemsOnTile

	Fills the given list with nodes for every item entity on the given
	map tile (x, y)

-------------------------------------------------------------------------------*/

void getItemsOnTile(int x, int y, list_t** list)
{

	//Take the return value of checkTileForEntity() and sort that list for items.
	//if( entity->behavior == &actItem )
	//And then free the list returned by checkTileForEntity.

	//Right. First, grab all the entities on the tile.
	list_t* entities = NULL;
	entities = checkTileForEntity(x, y);

	if (!entities)
	{
		return;    //No use continuing of got no entities.
	}

	node_t* node = NULL;
	node_t* node2 = NULL;
	//Loop through the list of entities.
	for (node = entities->first; node != NULL; node = node->next)
	{
		if (node->element)
		{
			Entity* entity = (Entity*) node->element;
			//Check if the entity is an item.
			if (entity && entity->behavior == &actItem)
			{
				//If this is the first item found, the list needs to be created.
				if (!(*list))
				{
					*list = (list_t*) malloc(sizeof(list_t));
					(*list)->first = NULL;
					(*list)->last = NULL;
				}

				//Add the current entity to it.
				node2 = list_AddNodeLast(*list);
				node2->element = entity;
				node2->deconstructor = &emptyDeconstructor;
			}
		}
	}

	if (entities)
	{
		list_FreeAll(entities);
		free(entities);
	}

	//return return_val;
}

/*-------------------------------------------------------------------------------

	Entity::attack

	Causes an entity to attack using whatever weapon it's holding

-------------------------------------------------------------------------------*/

void Entity::attack(int pose, int charge, Entity* target)
{
	Stat* hitstats = nullptr;
	Stat* myStats = nullptr;
	Entity* entity = nullptr;
	int player, playerhit = -1;
	double dist;
	int c, i;
	int weaponskill = -1;
	node_t* node = nullptr;
	double tangent;

	if ( (myStats = getStats()) == nullptr )
	{
		return;
	}

	// get the player number, if applicable
	if ( behavior == &actPlayer )
	{
		player = skill[2];
	}
	else
	{
		player = -1; // not a player
	}

	if (multiplayer != CLIENT)
	{
		// animation
		if (player >= 0)
		{
			if (stats[player]->weapon != nullptr)
			{
				players[player]->entity->skill[9] = pose; // PLAYER_ATTACK
			}
			else
			{
				players[player]->entity->skill[9] = 1; // special case for punch to eliminate spanking motion :p
			}
			players[player]->entity->skill[10] = 0; // PLAYER_ATTACKTIME
		}
		else
		{
			if ( pose >= MONSTER_POSE_MELEE_WINDUP1 && pose <= MONSTER_POSE_SPECIAL_WINDUP3 )
			{
				monsterAttack = pose;
				monsterAttackTime = 0;
				if ( multiplayer == SERVER )
				{
					// be sure to update the clients with the new wind-up pose.
					serverUpdateEntitySkill(this, 8);
					serverUpdateEntitySkill(this, 9);
				}
				return; // don't execute the attack, let the monster animation call the attack() function again.
			}
			else if ( myStats->weapon != nullptr || myStats->type == CRYSTALGOLEM || myStats->type == COCKATRICE )
			{
				monsterAttack = pose;
			}
			else
			{
				monsterAttack = 1;    // punching
			}
			monsterAttackTime = 0;
		}

		if (multiplayer == SERVER)
		{
			if (player >= 0 && player < MAXPLAYERS)
			{
				serverUpdateEntitySkill(players[player]->entity, 9);
				serverUpdateEntitySkill(players[player]->entity, 10);
			}
			else
			{
				serverUpdateEntitySkill(this, 8);
				serverUpdateEntitySkill(this, 9);
			}
		}

		if ( myStats->type == GOATMAN )
		{
			if ( monsterSpecialState > 0 )
			{
				monsterSpecialState = 0; //Resume the weapon choosing AI for a goatman, since he's now chucking his held item.
			}
		}

		if ( myStats->weapon != nullptr )
		{
			// magical weapons
			if ( itemCategory(myStats->weapon) == SPELLBOOK || itemCategory(myStats->weapon) == MAGICSTAFF )
			{
				if ( itemCategory(myStats->weapon) == MAGICSTAFF )
				{
					switch ( myStats->weapon->type )
					{
						case MAGICSTAFF_LIGHT:
							castSpell(uid, &spell_light, true, false);
							break;
						case MAGICSTAFF_DIGGING:
							castSpell(uid, &spell_dig, true, false);
							break;
						case MAGICSTAFF_LOCKING:
							castSpell(uid, &spell_locking, true, false);
							break;
						case MAGICSTAFF_MAGICMISSILE:
							castSpell(uid, &spell_magicmissile, true, false);
							break;
						case MAGICSTAFF_OPENING:
							castSpell(uid, &spell_opening, true, false);
							break;
						case MAGICSTAFF_SLOW:
							castSpell(uid, &spell_slow, true, false);
							break;
						case MAGICSTAFF_COLD:
							castSpell(uid, &spell_cold, true, false);
							break;
						case MAGICSTAFF_FIRE:
							castSpell(uid, &spell_fireball, true, false);
							break;
						case MAGICSTAFF_LIGHTNING:
							castSpell(uid, &spell_lightning, true, false);
							break;
						case MAGICSTAFF_SLEEP:
							castSpell(uid, &spell_sleep, true, false);
							break;
						case MAGICSTAFF_SUMMON:
							castSpell(uid, &spell_summon, true, false);
							break;
						case MAGICSTAFF_STONEBLOOD:
							castSpell(uid, &spell_stoneblood, true, false);
							break;
						case MAGICSTAFF_BLEED:
							castSpell(uid, &spell_bleed, true, false);
							break;
						default:
							messagePlayer(player, "This is my wish stick! Wishy wishy wish!");
							break;
					}

					// magicstaffs deplete themselves for each use
					if ( rand() % 3 == 0 )
					{
						if ( player == clientnum )
						{
							if ( myStats->weapon->count > 1 )
							{
								newItem(myStats->weapon->type, myStats->weapon->status, myStats->weapon->beatitude, myStats->weapon->count - 1, myStats->weapon->appearance, myStats->weapon->identified, &myStats->inventory);
							}
						}
						myStats->weapon->count = 1;
						myStats->weapon->status = static_cast<Status>(myStats->weapon->status - 1);
						if ( myStats->weapon->status != BROKEN )
						{
							messagePlayer(player, language[659]);
						}
						else
						{
							messagePlayer(player, language[660]);
						}
						if ( player > 0 && multiplayer == SERVER )
						{
							strcpy((char*)net_packet->data, "ARMR");
							net_packet->data[4] = 5;
							net_packet->data[5] = myStats->weapon->status;
							net_packet->address.host = net_clients[player - 1].host;
							net_packet->address.port = net_clients[player - 1].port;
							net_packet->len = 6;
							sendPacketSafe(net_sock, -1, net_packet, player - 1);
						}
					}
				}
				else
				{
					// this is mostly used for monsters that "cast" spells
					switch ( myStats->weapon->type )
					{
						case SPELLBOOK_FORCEBOLT:
							castSpell(uid, &spell_forcebolt, true, false);
							break;
						case SPELLBOOK_MAGICMISSILE:
							castSpell(uid, &spell_magicmissile, true, false);
							break;
						case SPELLBOOK_COLD:
							castSpell(uid, &spell_cold, true, false);
							break;
						case SPELLBOOK_FIREBALL:
							castSpell(uid, &spell_fireball, true, false);
							break;
						case SPELLBOOK_LIGHTNING:
							castSpell(uid, &spell_lightning, true, false);
							break;
						case SPELLBOOK_SLEEP:
							castSpell(uid, &spell_sleep, true, false);
							break;
						case SPELLBOOK_CONFUSE:
							castSpell(uid, &spell_confuse, true, false);
							break;
						case SPELLBOOK_SLOW:
							castSpell(uid, &spell_slow, true, false);
							break;
						case SPELLBOOK_DIG:
							castSpell(uid, &spell_dig, true, false);
							break;
						case SPELLBOOK_STONEBLOOD:
							castSpell(uid, &spell_stoneblood, true, false);
							break;
						case SPELLBOOK_BLEED:
							castSpell(uid, &spell_bleed, true, false);
							break;
						case SPELLBOOK_SUMMON:
							castSpell(uid, &spell_summon, true, false);
							break;
						//case SPELLBOOK_REFLECT_MAGIC: //TODO: Test monster support. Maybe better to just use a special ability that directly casts the spell.
							//castSpell(uid, &spell_reflectMagic, true, false)
							//break;
						default:
							break;
					}

					// DEPRECATED!!
					/*if( myStats->MP>0 ) {
						castMagic(my);

						// spells deplete MP
						myStats->MP--;
						if( multiplayer==SERVER && player!=clientnum ) {
							strcpy((char *)net_packet->data,"UPMP");
							SDLNet_Write32((Uint32)myStats->MP,&net_packet->data[4]);
							net_packet->address.host = net_clients[player-1].host;
							net_packet->address.port = net_clients[player-1].port;
							net_packet->len = 8;
							sendPacketSafe(net_sock, -1, net_packet, player-1);
						}
					} else {
						messagePlayer(player,"You lack the energy to cast magic!");
					}*/
				}
				return;
			}

			// ranged weapons (bows)
			else if ( myStats->weapon->type == SHORTBOW || myStats->weapon->type == CROSSBOW || myStats->weapon->type == SLING || myStats->weapon->type == ARTIFACT_BOW )
			{
				// damage weapon if applicable
				if ( rand() % 50 == 0 && myStats->weapon->type != ARTIFACT_BOW )
				{
					if ( myStats->weapon != NULL )
					{
						if ( player == clientnum )
						{
							if ( myStats->weapon->count > 1 )
							{
								newItem(myStats->weapon->type, myStats->weapon->status, myStats->weapon->beatitude, myStats->weapon->count - 1, myStats->weapon->appearance, myStats->weapon->identified, &myStats->inventory);
							}
						}
						myStats->weapon->count = 1;
						myStats->weapon->status = static_cast<Status>(myStats->weapon->status - 1);
						if ( myStats->weapon->status != BROKEN )
						{
							messagePlayer(player, language[661], myStats->weapon->getName());
						}
						else
						{
							playSoundEntity(this, 76, 64);
							messagePlayer(player, language[662], myStats->weapon->getName());
						}
						if ( player > 0 && multiplayer == SERVER )
						{
							strcpy((char*)net_packet->data, "ARMR");
							net_packet->data[4] = 5;
							net_packet->data[5] = myStats->weapon->status;
							net_packet->address.host = net_clients[player - 1].host;
							net_packet->address.port = net_clients[player - 1].port;
							net_packet->len = 6;
							sendPacketSafe(net_sock, -1, net_packet, player - 1);
						}
					}
				}
				if ( myStats->weapon->type == SLING )
				{
					entity = newEntity(78, 1, map.entities); // rock
					playSoundEntity(this, 239 + rand() % 3, 96);
				}
				else if ( myStats->weapon->type == CROSSBOW )
				{
					entity = newEntity(167, 1, map.entities); // bolt
					playSoundEntity(this, 239 + rand() % 3, 96);
				}
				else
				{
					entity = newEntity(166, 1, map.entities); // arrow
					playSoundEntity(this, 239 + rand() % 3, 96);
				}
				entity->parent = uid;
				entity->x = x;
				entity->y = y;
				entity->z = z;
				entity->yaw = yaw;
				entity->sizex = 1;
				entity->sizey = 1;
				entity->behavior = &actArrow;
				entity->flags[UPDATENEEDED] = true;
				entity->flags[PASSABLE] = true;

				// arrow power
				entity->skill[3] = getAttack() - 1 + myStats->PROFICIENCIES[PRO_RANGED] / 20;

				// poison arrow
				if ( myStats->weapon->type == ARTIFACT_BOW )
				{
					entity->skill[4] = 540;    // 9 seconds of poison
				}
				return;
			}

			// potions & gems (throwing), and thrown weapons
			if ( itemCategory(myStats->weapon) == POTION || itemCategory(myStats->weapon) == GEM || itemCategory(myStats->weapon) == THROWN )
			{
				bool drankPotion = false;
				if ( myStats->type == GOATMAN && itemCategory(myStats->weapon) == POTION )
				{
					//Goatmen chug potions & then toss them at you.
					if ( myStats->weapon->type == POTION_BOOZE && !myStats->EFFECTS[EFF_DRUNK] )
					{
						item_PotionBooze(myStats->weapon, this, false);
						drankPotion = true;
					}
					else if ( myStats->weapon->type == POTION_HEALING )
					{
						item_PotionHealing(myStats->weapon, this, false);
						drankPotion = true;
					}
					else if ( myStats->weapon->type == POTION_EXTRAHEALING )
					{
						item_PotionExtraHealing(myStats->weapon, this, false);
						drankPotion = true;
					}
				}

				playSoundEntity(this, 75, 64);
				if ( drankPotion )
				{
					Item* emptyBottle = newItem(POTION_EMPTY, myStats->weapon->status, myStats->weapon->beatitude, 1, myStats->weapon->appearance, myStats->weapon->appearance, nullptr);
					entity = newEntity(itemModel(emptyBottle), 1, map.entities); // thrown item
					entity->parent = uid;
					entity->x = x;
					entity->y = y;
					entity->z = z;
					entity->yaw = yaw;
					entity->sizex = 1;
					entity->sizey = 1;
					entity->behavior = &actThrown;
					entity->flags[UPDATENEEDED] = true;
					entity->flags[PASSABLE] = true;
					entity->skill[10] = emptyBottle->type;
					entity->skill[11] = emptyBottle->status;
					entity->skill[12] = emptyBottle->beatitude;
					entity->skill[13] = 1;
					entity->skill[14] = emptyBottle->appearance;
					entity->skill[15] = emptyBottle->identified;
				}
				else
				{
					entity = newEntity(itemModel(myStats->weapon), 1, map.entities); // thrown item
					entity->parent = uid;
					entity->x = x;
					entity->y = y;
					entity->z = z;
					entity->yaw = yaw;
					entity->sizex = 1;
					entity->sizey = 1;
					entity->behavior = &actThrown;
					entity->flags[UPDATENEEDED] = true;
					entity->flags[PASSABLE] = true;
					entity->skill[10] = myStats->weapon->type;
					entity->skill[11] = myStats->weapon->status;
					entity->skill[12] = myStats->weapon->beatitude;
					entity->skill[13] = 1;
					entity->skill[14] = myStats->weapon->appearance;
					entity->skill[15] = myStats->weapon->identified;
				}

				if ( itemCategory(myStats->weapon) == THROWN )
				{
					// thrown items have slightly faster velocities
					if ( (myStats->weapon->type == STEEL_CHAKRAM || myStats->weapon->type == CRYSTAL_SHURIKEN) )
					{
						if ( this->behavior == &actPlayer )
						{
							// todo: change velocity of chakram/shuriken?
							entity->vel_x = 6 * cos(players[player]->entity->yaw);
							entity->vel_y = 6 * sin(players[player]->entity->yaw);
							entity->vel_z = -.3;
						}
						else if ( this->behavior == &actMonster )
						{
							// todo: change velocity of chakram/shuriken?
							entity->vel_x = 6 * cos(this->yaw);
							entity->vel_y = 6 * sin(this->yaw);
							entity->vel_z = -.3;
						}
					}
					else
					{
						if ( this->behavior == &actPlayer )
						{
							entity->vel_x = 6 * cos(players[player]->entity->yaw);
							entity->vel_y = 6 * sin(players[player]->entity->yaw);
							entity->vel_z = -.3;
						}
						else if ( this->behavior == &actMonster )
						{
							entity->vel_x = 6 * cos(this->yaw);
							entity->vel_y = 6 * sin(this->yaw);
							entity->vel_z = -.3;
						}
					}
				}
				else
				{
					if ( this->behavior == &actPlayer )
					{
						entity->vel_x = 5 * cos(players[player]->entity->yaw);
						entity->vel_y = 5 * sin(players[player]->entity->yaw);
						entity->vel_z = -.5;
					}
					else if ( this->behavior == &actMonster )
					{
						entity->vel_x = 5 * cos(this->yaw);
						entity->vel_y = 5 * sin(this->yaw);
						entity->vel_z = -.5;
					}
				}

				//TODO: Refactor this so that we don't have to copy paste this check a million times whenever some-one uses up an item.
				myStats->weapon->count--;
				if ( myStats->weapon->count <= 0 )
				{
					if ( myStats->weapon->node )
					{
						list_RemoveNode(myStats->weapon->node);
					}
					else
					{
						free(myStats->weapon);
					}
					myStats->weapon = nullptr;
				}
				return;
			}
		}

		// normal attacks
		if ( target == nullptr )
		{
			playSoundEntity(this, 23 + rand() % 5, 128); // whoosh noise
			dist = lineTrace(this, x, y, yaw, STRIKERANGE, 0, false);
		}
		else
		{
			hit.entity = target;
		}

		if ( hit.entity != nullptr )
		{
			if ( !(svFlags & SV_FLAG_FRIENDLYFIRE) )
			{
				// test for friendly fire
				if ( checkFriend(hit.entity) )
				{
					return;
				}
			}

			if ( hit.entity->behavior == &actBoulder )
			{
				if ( myStats->weapon != nullptr )
				{
					if ( myStats->weapon->type == TOOL_PICKAXE )
					{
						// spawn several rock items
						int i = 8 + rand() % 4;

						int c;
						for ( c = 0; c < i; c++ )
						{
							Entity* entity = newEntity(-1, 1, map.entities);
							entity->flags[INVISIBLE] = true;
							entity->flags[UPDATENEEDED] = true;
							entity->x = hit.entity->x - 4 + rand() % 8;
							entity->y = hit.entity->y - 4 + rand() % 8;
							entity->z = -6 + rand() % 12;
							entity->sizex = 4;
							entity->sizey = 4;
							entity->yaw = rand() % 360 * PI / 180;
							entity->vel_x = (rand() % 20 - 10) / 10.0;
							entity->vel_y = (rand() % 20 - 10) / 10.0;
							entity->vel_z = -.25 - (rand() % 5) / 10.0;
							entity->flags[PASSABLE] = true;
							entity->behavior = &actItem;
							entity->flags[USERFLAG1] = true; // no collision: helps performance
							entity->skill[10] = GEM_ROCK;    // type
							entity->skill[11] = WORN;        // status
							entity->skill[12] = 0;           // beatitude
							entity->skill[13] = 1;           // count
							entity->skill[14] = 0;           // appearance
							entity->skill[15] = false;       // identified
						}

						double ox = hit.entity->x;
						double oy = hit.entity->y;

						// destroy the boulder
						playSoundEntity(hit.entity, 67, 128);
						list_RemoveNode(hit.entity->mynode);
						messagePlayer(player, language[663]);
						if ( rand() % 2 )
						{
							myStats->weapon->status = static_cast<Status>(myStats->weapon->status - 1);
							if ( myStats->weapon->status == BROKEN )
							{
								messagePlayer(player, language[664]);
								playSoundEntity(this, 76, 64);
							}
							else
							{
								messagePlayer(player, language[665]);
							}
							if ( player > 0 && multiplayer == SERVER )
							{
								strcpy((char*)net_packet->data, "ARMR");
								net_packet->data[4] = 5;
								net_packet->data[5] = myStats->weapon->status;
								net_packet->address.host = net_clients[player - 1].host;
								net_packet->address.port = net_clients[player - 1].port;
								net_packet->len = 6;
								sendPacketSafe(net_sock, -1, net_packet, player - 1);
							}
						}

						// on sokoban, destroying boulders spawns scorpions
						if ( !strcmp(map.name, "Sokoban") )
						{
							Entity* monster = summonMonster(SCORPION, ox, oy);
							if ( monster )
							{
								int c;
								for ( c = 0; c < MAXPLAYERS; c++ )
								{
									Uint32 color = SDL_MapRGB(mainsurface->format, 255, 128, 0);
									messagePlayerColor(c, color, language[406]);
								}
							}
						}
					}
					else
					{
						spawnBang(hit.x - cos(yaw) * 2, hit.y - sin(yaw) * 2, 0);
					}
				}
				else
				{
					//spawnBang(hit.x - cos(my->yaw)*2,hit.y - sin(my->yaw)*2,0);
					playSoundPos(hit.x, hit.y, 183, 64);
				}
			}
			else if ( hit.entity->behavior == &actMonster )
			{
				if ( hit.entity->children.first != nullptr )
				{
					if ( hit.entity->children.first->next != nullptr )
					{
						hitstats = (Stat*)hit.entity->children.first->next->element;

						// alert the monster!
						if ( hit.entity->monsterState != MONSTER_STATE_ATTACK && (hitstats->type < LICH || hitstats->type >= SHOPKEEPER) )
						{
							//hit.entity->skill[0]=0;
							//hit.entity->skill[4]=0;
							//hit.entity->fskill[4]=atan2(my->y-hit.entity->y,my->x-hit.entity->x);

							/*hit.entity->monsterState = MONSTER_STATE_PATH;
							hit.entity->monsterTarget = uid;
							hit.entity->monsterTargetX = x;
							hit.entity->monsterTargetY = y;*/

							Entity* attackTarget = uidToEntity(uid);

							if ( attackTarget )
							{
								hit.entity->monsterAcquireAttackTarget(*attackTarget, MONSTER_STATE_PATH);
							}
						}

						// alert other monsters too
						Entity* ohitentity = hit.entity;
						for ( node = map.entities->first; node != nullptr; node = node->next )
						{
							entity = (Entity*)node->element;
							if ( entity && entity->behavior == &actMonster && entity != ohitentity )
							{
								Stat* buddystats = entity->getStats();
								if ( buddystats != nullptr )
								{
									if ( entity->checkFriend(hit.entity) )
									{
										if ( entity->monsterState == MONSTER_STATE_WAIT )
										{
											tangent = atan2( entity->y - ohitentity->y, entity->x - ohitentity->x );
											lineTrace(ohitentity, ohitentity->x, ohitentity->y, tangent, 1024, 0, false);
											if ( hit.entity == entity )
											{
												/*entity->monsterState = MONSTER_STATE_PATH;
												entity->monsterTarget = uid;
												entity->monsterTargetX = x;
												entity->monsterTargetY = y;*/

												Entity* attackTarget = uidToEntity(uid);
												if ( attackTarget )
												{
													entity->monsterAcquireAttackTarget(*attackTarget, MONSTER_STATE_PATH);
												}
											}
										}
									}
								}
							}
						}
						hit.entity = ohitentity;
					}
				}
			}
			else if ( hit.entity->behavior == &actPlayer )
			{
				hitstats = stats[hit.entity->skill[2]];
				playerhit = hit.entity->skill[2];

				// alert the player's followers!
				//Maybe should send a signal to each follower, with some kind of attached priority, which determines if they change their target to bumrush the player's assailant.
				for ( node = hitstats->FOLLOWERS.first; node != nullptr; node = node->next )
				{
					Uint32* c = (Uint32*)node->element;
					entity = uidToEntity(*c);
					Entity* ohitentity = hit.entity;
					if ( entity )
					{
						Stat* buddystats = entity->getStats();
						if ( buddystats != nullptr )
						{
							if ( entity->monsterState == MONSTER_STATE_WAIT || (entity->monsterState == MONSTER_STATE_HUNT && entity->monsterTarget != uid) ) // monster is waiting or hunting
							{
								/*entity->monsterState = MONSTER_STATE_PATH;
								entity->monsterTarget = uid;
								entity->monsterTargetX = x;
								entity->monsterTargetY = y;*/

								Entity* attackTarget = uidToEntity(uid);
								if ( attackTarget )
								{
									entity->monsterAcquireAttackTarget(*attackTarget, MONSTER_STATE_PATH);
								}
							}
						}
					}
					hit.entity = ohitentity;
				}
			}
			else if ( hit.entity->behavior == &actDoor || hit.entity->behavior == &actFurniture || hit.entity->behavior == &::actChest )
			{
				int axe = 0;
				if ( myStats->weapon )
				{
					if ( myStats->weapon->type == BRONZE_AXE || myStats->weapon->type == IRON_AXE || myStats->weapon->type == STEEL_AXE )
					{
						axe = 1; // axes do extra damage to doors :)
					}
				}
				if ( hit.entity->behavior != &::actChest )
				{
					if ( charge < MAXCHARGE / 2 )
					{
						hit.entity->skill[4] -= 1 + axe; // decrease door/furniture health
					}
					else
					{
						hit.entity->skill[4] -= 2 + axe; // decrease door/furniture health extra
					}
				}
				else
				{
					if ( charge < MAXCHARGE / 2 )
					{
						hit.entity->skill[3] -= 1 + axe; // decrease chest health
					}
					else
					{
						hit.entity->skill[3] -= 2 + axe; // decrease chest health extra
					}
				}
				playSoundEntity(hit.entity, 28, 64);
				if ( (hit.entity->behavior != &::actChest && hit.entity->skill[4] > 0) || (hit.entity->behavior == &::actChest && hit.entity->skill[3] > 0) )
				{
					if ( hit.entity->behavior == &actDoor )
					{
						messagePlayer(player, language[666]);
					}
					else if ( hit.entity->behavior == &::actChest )
					{
						messagePlayer(player, language[667]);
					}
					else if ( hit.entity->behavior == &actFurniture )
					{
						if ( hit.entity->skill[0] == 0 )
						{
							messagePlayer(player, language[668]);
						}
						else
						{
							messagePlayer(player, language[669]);
						}
					}
				}
				else
				{
					hit.entity->skill[4] = 0;
					if ( hit.entity->behavior == &actDoor )
					{
						messagePlayer(player, language[670]);
						if ( !hit.entity->skill[0] )
						{
							hit.entity->skill[6] = (x > hit.entity->x);
						}
						else
						{
							hit.entity->skill[6] = (y < hit.entity->y);
						}
					}
					else if ( hit.entity->behavior == &::actChest )
					{
						messagePlayer(player, language[671]);
					}
					else if ( hit.entity->behavior == &actFurniture )
					{
						if ( hit.entity->skill[0] == 0 )
						{
							messagePlayer(player, language[672]);
						}
						else
						{
							messagePlayer(player, language[673]);
						}
					}
				}
				if ( hit.entity->behavior == &actDoor )
				{
					updateEnemyBar(this, hit.entity, language[674], hit.entity->skill[4], hit.entity->skill[9]);
				}
				else if ( hit.entity->behavior == &::actChest )
				{
					updateEnemyBar(this, hit.entity, language[675], hit.entity->skill[3], hit.entity->skill[8]);
				}
				else if ( hit.entity->behavior == &actFurniture )
				{
					if ( hit.entity->skill[0] == 0 )
					{
						updateEnemyBar(this, hit.entity, language[676], hit.entity->skill[4], hit.entity->skill[9]);
					}
					else
					{
						updateEnemyBar(this, hit.entity, language[677], hit.entity->skill[4], hit.entity->skill[9]);
					}
				}
			}
			else if ( hit.entity->behavior == &actSink )
			{
				playSoundEntity(hit.entity, 28, 64);
				playSoundEntity(hit.entity, 140 + rand(), 64);
				messagePlayer(player, language[678]);
				if (hit.entity->skill[0] > 0)
				{
					hit.entity->skill[0]--; //Deplete one usage.

					//50% chance spawn a slime.
					if (rand() % 2 == 0)
					{
						// spawn slime
						Entity* monster = summonMonster(SLIME, x, y);
						if ( monster )
						{
							messagePlayer(player, language[582]);
							Stat* monsterStats = monster->getStats();
							monsterStats->LVL = 4;
						}
					}

					if (hit.entity->skill[0] == 0)   //Depleted.
					{
						messagePlayer(player, language[585]); //TODO: Alert all players that see (or otherwise in range) it?
						playSoundEntity(hit.entity, 132, 64);
					}
				}
			}
			else
			{
				if ( myStats->weapon )
				{
					// bang
					spawnBang(hit.x - cos(yaw) * 2, hit.y - sin(yaw) * 2, 0);
				}
				else
				{
					playSoundPos(hit.x, hit.y, 183, 64);
				}
			}

			if ( hitstats != NULL )
			{
				// hit chance
				//int hitskill=5; // for unarmed combat

				weaponskill = getWeaponSkill(myStats->weapon);

				/*if( weaponskill>=0 )
					hitskill = myStats->PROFICIENCIES[weaponskill]/5;
				c = rand()%20 + hitskill + (weaponskill==PRO_POLEARM);
				bool hitsuccess=false;
				if( myStats->weapon ) {
					if( myStats->weapon->type == ARTIFACT_SPEAR ) {
						hitsuccess=true; // Gungnir always lands a hit!
					}
				}
				if( c > 10+std::min(std::max(-3,hit.entity->getDEX()-my->getDEX()),3) ) {
					hitsuccess=true;
				}
				if( hitsuccess )*/
				{
					// skill increase
					if ( weaponskill >= 0 )
						if ( rand() % 10 == 0 )
						{
							this->increaseSkill(weaponskill);
						}

					// calculate and perform damage to opponent
					int damage = 0;
					if ( weaponskill >= 0 )
					{
						damage = std::max(0, getAttack() - AC(hitstats)) * damagetables[hitstats->type][weaponskill - PRO_SWORD];
					}
					else
					{
						damage = std::max(0, getAttack() - AC(hitstats));
					}
					if ( weaponskill == PRO_AXE )
					{
						damage++;
					}

					bool gungnir = false;
					if ( myStats->weapon )
						if ( myStats->weapon->type == ARTIFACT_SPEAR )
						{
							gungnir = true;
						}
					if ( weaponskill >= PRO_SWORD && weaponskill < PRO_SHIELD && !gungnir )
					{
						int chance = 0;
						if ( weaponskill == PRO_POLEARM )
						{
							chance = (damage / 3) * (100 - myStats->PROFICIENCIES[weaponskill]) / 100.f;
						}
						else
						{
							chance = (damage / 2) * (100 - myStats->PROFICIENCIES[weaponskill]) / 100.f;
						}
						if ( chance > 0 )
						{
							damage = (damage - chance) + (rand() % chance) + 1;
						}
					}

					int olddamage = damage;
					damage *= std::max(charge, MAXCHARGE / 2) / ((double)(MAXCHARGE / 2));

					if ( myStats->weapon )
						if ( myStats->weapon->type == ARTIFACT_AXE )
							if ( rand() % 3 == 0 )
							{
								damage *= 2;    // Parashu sometimes doubles damage
							}
					hit.entity->modHP(-damage); // do the damage

					// write the obituary
					killedByMonsterObituary(hit.entity);

					// update enemy bar for attacker
					if ( !strcmp(hitstats->name, "") )
					{
						if ( hitstats->type < KOBOLD ) //Original monster count
						{
							updateEnemyBar(this, hit.entity, language[90 + hitstats->type], hitstats->HP, hitstats->MAXHP);
						}
						else if ( hitstats->type >= KOBOLD ) //New monsters
						{
							updateEnemyBar(this, hit.entity, language[2000 + (hitstats->type - KOBOLD)], hitstats->HP, hitstats->MAXHP);
						}
					}
					else
					{
						updateEnemyBar(this, hit.entity, hitstats->name, hitstats->HP, hitstats->MAXHP);
					}

					// damage weapon if applicable

					bool isWeakWeapon = false;
					bool artifactWeapon = false;
					bool degradeWeapon = false;
					ItemType weaponType = static_cast<ItemType>(WOODEN_SHIELD);

					if ( myStats->weapon != NULL )
					{
						weaponType = myStats->weapon->type;
						if ( weaponType == ARTIFACT_AXE || weaponType == ARTIFACT_MACE || weaponType == ARTIFACT_SPEAR || weaponType == ARTIFACT_SWORD )
						{
							artifactWeapon = true;
						}
						else if ( weaponType == CRYSTAL_BATTLEAXE || weaponType == CRYSTAL_MACE || weaponType == CRYSTAL_SWORD || weaponType == CRYSTAL_SPEAR )
						{
							// crystal weapons degrade faster.
							isWeakWeapon = true;
						}

						if ( !artifactWeapon )
						{
							// crystal weapons chance to not degrade 66% chance on 0 dmg, else 96%
							if ( isWeakWeapon && ((rand() % 3 == 0 && damage == 0) || (rand() % 25 == 0 && damage > 0)) )
							{
								degradeWeapon = true;
							}
							// other weapons chance to not degrade 75% chance on 0 dmg, else 98%
							else if ( !isWeakWeapon && ((rand() % 4 == 0 && damage == 0) || (rand() % 50 == 0 && damage > 0)) )
							{
								degradeWeapon = true;
							}

							if ( degradeWeapon )
							{
								if ( player == clientnum || player < 0 )
								{
									if ( myStats->weapon->count > 1 )
									{
										newItem(myStats->weapon->type, myStats->weapon->status, myStats->weapon->beatitude, myStats->weapon->count - 1, myStats->weapon->appearance, myStats->weapon->identified, &myStats->inventory);
									}
								}
								myStats->weapon->count = 1;
								myStats->weapon->status = static_cast<Status>(myStats->weapon->status - 1);
								if ( myStats->weapon->status != BROKEN )
								{
									messagePlayer(player, language[679]);
								}
								else
								{
									playSoundEntity(this, 76, 64);
									messagePlayer(player, language[680]);
								}
								if ( player > 0 && multiplayer == SERVER )
								{
									strcpy((char*)net_packet->data, "ARMR");
									net_packet->data[4] = 5;
									net_packet->data[5] = myStats->weapon->status;
									net_packet->address.host = net_clients[player - 1].host;
									net_packet->address.port = net_clients[player - 1].port;
									net_packet->len = 6;
									sendPacketSafe(net_sock, -1, net_packet, player - 1);
								}
							}
						}
					}

					// damage opponent armor if applicable
					Item* armor = NULL;
					int armornum = 0;
					bool isWeakArmor = false;

					if ( damage > 0 )
					{
						// choose random piece of equipment to target
						switch ( rand() % 6 )
						{
							case 0:
								armor = hitstats->helmet;
								armornum = 0;
								break;
							case 1:
								armor = hitstats->breastplate;
								armornum = 1;
								break;
							case 2:
								armor = hitstats->gloves;
								armornum = 2;
								break;
							case 3:
								armor = hitstats->shoes;
								armornum = 3;
								break;
							case 4:
								armor = hitstats->shield;
								armornum = 4;
								break;
							case 5:
								armor = hitstats->cloak;
								armornum = 6;
								break;
							default:
								break;
						}

						if ( armor != NULL )
						{
							switch ( armor->type )
							{
								case CRYSTAL_HELM:
								case CRYSTAL_SHIELD:
								case CRYSTAL_BREASTPIECE:
								case CRYSTAL_BOOTS:
								case CRYSTAL_GLOVES:
									isWeakArmor = true;
									break;
								default:
									isWeakArmor = false;
									break;
							}
						}

						if ( weaponskill == PRO_MACE )
						{
							if ( isWeakArmor )
							{
								// 80% chance to be deselected from degrading.
								if ( rand() % 5 > 0 )
								{
									armor = NULL;
									armornum = 0;
								}
							}
							else
							{
								// 90% chance to be deselected from degrading.
								if ( rand() % 10 > 0 )
								{
									armor = NULL;
									armornum = 0;
								}
							}
						}
						// crystal golem special attack increase chance for armor to break if hit. (25-33%)
						// special attack only degrades armor if primary target.
						else if ( pose == MONSTER_POSE_GOLEM_SMASH && target == nullptr )
						{
							if ( isWeakArmor )
							{
								// 66% chance to be deselected from degrading.
								if ( rand() % 3 > 0 )
								{
									armor = NULL;
									armornum = 0;
								}
							}
							else
							{
								// 75% chance to be deselected from degrading.
								if ( rand() % 4 > 0 )
								{
									armor = NULL;
									armornum = 0;
								}
							}
						}
						else
						{
							if ( isWeakArmor )
							{
								// 93% chance to be deselected from degrading.
								if ( rand() % 15 > 0 )
								{
									armor = NULL;
									armornum = 0;
								}
							}
							else
							{
								// 96% chance to be deselected from degrading.
								if ( rand() % 25 > 0 )
								{
									armor = NULL;
									armornum = 0;
								}
							}
						}
					}

					// if nothing chosen to degrade, check extra shield chances to degrade
					if ( hitstats->shield != NULL && armor == NULL )
					{
						if ( hitstats->shield->type == TOOL_CRYSTALSHARD && hitstats->defending )
						{
							// shards degrade by 1 stage each hit.
							armor = hitstats->shield;
							armornum = 4;
						}
						else if ( hitstats->shield->type == MIRROR_SHIELD && hitstats->defending )
						{
							// mirror shield degrade by 1 stage each hit.
							armor = hitstats->shield;
							armornum = 4;
						}
						else
						{
							// if no armor piece was chosen to break, grant chance to improve shield skill.
							if ( itemCategory(hitstats->shield) == ARMOR )
							{
								if ( (rand() % 10 == 0 && damage > 0) || (damage == 0 && rand() % 3 == 0) )
								{
									hit.entity->increaseSkill(PRO_SHIELD); // increase shield skill
								}
							}

							// shield still has chance to degrade after raising skill.
							// crystal golem special attack increase chance for shield to break if defended. (33%)
							// special attack only degrades shields if primary target.
							if ( (hitstats->defending && rand() % 10 == 0) 
								|| (hitstats->defending && pose == MONSTER_POSE_GOLEM_SMASH && target == nullptr && rand() % 3 == 0)
								&& armor == NULL )
							{
								armor = hitstats->shield;
								armornum = 4;
							}
						}
					}

					if ( armor != NULL )
					{
						if ( playerhit == clientnum || playerhit < 0 )
						{
							if ( armor->count > 1 )
							{
								newItem(armor->type, armor->status, armor->beatitude, armor->count - 1, armor->appearance, armor->identified, &hitstats->inventory);
							}
						}
						armor->count = 1;
						armor->status = static_cast<Status>(armor->status - 1);
						if ( armor->status > BROKEN )
						{
							if ( armor->type == TOOL_CRYSTALSHARD )
							{
								messagePlayer(playerhit, language[2350], armor->getName());
							}
							else
							{
								messagePlayer(playerhit, language[681], armor->getName());
							}
						}
						else
						{

							if ( armor->type == TOOL_CRYSTALSHARD )
							{
								playSoundEntity(hit.entity, 162, 64);
								messagePlayer(playerhit, language[2351], armor->getName());
							}
							else
							{
								playSoundEntity(hit.entity, 76, 64);
								messagePlayer(playerhit, language[682], armor->getName());
							}
						}
						if ( playerhit > 0 && multiplayer == SERVER )
						{
							strcpy((char*)net_packet->data, "ARMR");
							net_packet->data[4] = armornum;
							net_packet->data[5] = armor->status;
							net_packet->address.host = net_clients[playerhit - 1].host;
							net_packet->address.port = net_clients[playerhit - 1].port;
							net_packet->len = 6;
							sendPacketSafe(net_sock, -1, net_packet, playerhit - 1);
						}
					}

					// special weapon effects
					if ( myStats->weapon )
					{
						if ( myStats->weapon->type == ARTIFACT_SWORD )
						{
							if ( hit.entity->flags[BURNABLE] )
							{
								if ( hitstats )
								{
									hitstats->poisonKiller = uid;
								}
								hit.entity->flags[BURNING] = true;
								if ( playerhit > 0 && multiplayer == SERVER )
								{
									messagePlayer(playerhit, language[683]);
									serverUpdateEntityFlag(hit.entity, BURNING);
								}
							}
						}
					}

					// special monster effects
					if ( myStats->type == CRYSTALGOLEM && pose == MONSTER_POSE_GOLEM_SMASH )
					{
						if ( multiplayer != CLIENT )
						{
							createParticleRock(hit.entity);
							if ( multiplayer == SERVER )
							{
								serverSpawnMiscParticles(hit.entity, PARTICLE_EFFECT_ABILITY_ROCK);
							}
							if ( target == nullptr )
							{
								// only play sound once on primary target.
								playSoundEntity(hit.entity, 181, 64);
							}
						}
					}
					else if ( damage > 0 && rand() % 4 == 0 )
					{
						int armornum = 0;
						Item* armor = NULL;
						int armorstolen = rand() % 9;
						switch ( myStats->type )
						{
							case SCORPION:
								hitstats->EFFECTS[EFF_PARALYZED] = true;
								hitstats->EFFECTS_TIMERS[EFF_PARALYZED] = 120;
								messagePlayer(playerhit, language[684]);
								messagePlayer(playerhit, language[685]);
								serverUpdateEffects(playerhit);
								break;
							case SPIDER:
								hitstats->EFFECTS[EFF_POISONED] = true;
								hitstats->EFFECTS_TIMERS[EFF_POISONED] = 600;
								messagePlayer(playerhit, language[686]);
								messagePlayer(playerhit, language[687]);
								serverUpdateEffects(playerhit);
								break;
							case SUCCUBUS:
								switch ( armorstolen )
								{
									case 0:
										armor = hitstats->helmet;
										armornum = 0;
										break;
									case 1:
										armor = hitstats->breastplate;
										armornum = 1;
										break;
									case 2:
										armor = hitstats->gloves;
										armornum = 2;
										break;
									case 3:
										armor = hitstats->shoes;
										armornum = 3;
										break;
									case 4:
										armor = hitstats->shield;
										armornum = 4;
										break;
									case 5:
										armor = hitstats->cloak;
										armornum = 6;
										break;
									case 6:
										armor = hitstats->amulet;
										armornum = 7;
										break;
									case 7:
										armor = hitstats->ring;
										armornum = 8;
										break;
									case 8:
										armor = hitstats->mask;
										armornum = 9;
										break;
									default:
										break;
								}
								if ( armor != NULL )
								{
									if ( playerhit == clientnum || playerhit < 0 )
									{
										if ( armor->count > 1 )
										{
											newItem(armor->type, armor->status, armor->beatitude, armor->count - 1, armor->appearance, armor->identified, &hitstats->inventory);
										}
									}
									armor->count = 1;
									messagePlayer(playerhit, language[688], armor->getName());
									newItem(armor->type, armor->status, armor->beatitude, armor->count, armor->appearance, armor->identified, &myStats->inventory);
									Item** slot = itemSlot(hitstats, armor);
									if ( slot )
									{
										*slot = NULL;
									}
									if ( armor->node )
									{
										list_RemoveNode(armor->node);
									}
									else
									{
										free(armor);
									}
									if ( playerhit > 0 && multiplayer == SERVER )
									{
										strcpy((char*)net_packet->data, "STLA");
										net_packet->data[4] = armornum;
										net_packet->address.host = net_clients[playerhit - 1].host;
										net_packet->address.port = net_clients[playerhit - 1].port;
										net_packet->len = 5;
										sendPacketSafe(net_sock, -1, net_packet, playerhit - 1);
									}
									teleportRandom();

									// the succubus loses interest after this
									monsterState = 0;
									monsterTarget = 0;
								}
								break;
							default:
								break;
						}
					}

					// send messages
					if ( !strcmp(hitstats->name, "") )
					{
						if ( hitstats->HP > 0 )
						{
							if ( damage > olddamage )
							{
								Uint32 color = SDL_MapRGB(mainsurface->format, 0, 255, 0);
								if ( hitstats->type < KOBOLD ) //Original monster count
								{
									messagePlayerColor(player, color, language[689], language[90 + hitstats->type]);
								}
								else if ( hitstats->type >= KOBOLD ) //New monsters
								{
									messagePlayerColor(player, color, language[689], language[2000 + (hitstats->type - KOBOLD)]);
								}
							}
							else
							{
								Uint32 color = SDL_MapRGB(mainsurface->format, 0, 255, 0);
								if ( hitstats->type < KOBOLD ) //Original monster count
								{
									messagePlayerColor(player, color, language[690], language[90 + hitstats->type]);
								}
								else if ( hitstats->type >= KOBOLD ) //New monsters
								{
									messagePlayerColor(player, color, language[690], language[2000 + (hitstats->type - KOBOLD)]);
								}
							}
							if ( damage == 0 )
							{
								messagePlayer(player, language[691]);
							}
						}
						else
						{
							Uint32 color = SDL_MapRGB(mainsurface->format, 0, 255, 0);
							if ( hitstats->type < KOBOLD ) //Original monster count
							{
								messagePlayerColor(player, color, language[692], language[90 + hitstats->type]);
							}
							else if ( hitstats->type >= KOBOLD ) //New monsters
							{
								messagePlayerColor(player, color, language[692], language[2000 + (hitstats->type - KOBOLD)]);
							}
							awardXP( hit.entity, true, true );
						}
					}
					else
					{
						if ( hitstats->HP > 0 )
						{
							if ( damage > olddamage )
							{
								Uint32 color = SDL_MapRGB(mainsurface->format, 0, 255, 0);
								messagePlayerColor(player, color, language[693], hitstats->name);
							}
							else
							{
								Uint32 color = SDL_MapRGB(mainsurface->format, 0, 255, 0);
								messagePlayerColor(player, color, language[694], hitstats->name);
							}
							if ( damage == 0 )
							{
								if ( hitstats->sex )
								{
									messagePlayer(player, language[695]);
								}
								else
								{
									messagePlayer(player, language[696]);
								}
							}
						}
						else
						{
							Uint32 color = SDL_MapRGB(mainsurface->format, 0, 255, 0);
							messagePlayerColor(player, color, language[697], hitstats->name);
							awardXP( hit.entity, true, true );
						}
					}
					if ( playerhit > 0 && multiplayer == SERVER )
					{
						if ( pose == MONSTER_POSE_GOLEM_SMASH )
						{	
							if ( target == nullptr )
							{
								// primary target
								strcpy((char*)net_packet->data, "SHAK");
								net_packet->data[4] = 20; // turns into .2
								net_packet->data[5] = 20;
								net_packet->address.host = net_clients[playerhit - 1].host;
								net_packet->address.port = net_clients[playerhit - 1].port;
								net_packet->len = 6;
								sendPacketSafe(net_sock, -1, net_packet, playerhit - 1);
							}
							else
							{
								// secondary target
								strcpy((char*)net_packet->data, "SHAK");
								net_packet->data[4] = 10; // turns into .1
								net_packet->data[5] = 10;
								net_packet->address.host = net_clients[playerhit - 1].host;
								net_packet->address.port = net_clients[playerhit - 1].port;
								net_packet->len = 6;
								sendPacketSafe(net_sock, -1, net_packet, playerhit - 1);
							}
						
							strcpy((char*)net_packet->data, "UPHP");
							SDLNet_Write32((Uint32)hitstats->HP, &net_packet->data[4]);
							SDLNet_Write32((Uint32)myStats->type, &net_packet->data[8]);
							net_packet->address.host = net_clients[playerhit - 1].host;
							net_packet->address.port = net_clients[playerhit - 1].port;
							net_packet->len = 12;
							sendPacketSafe(net_sock, -1, net_packet, playerhit - 1);
						}
						else
						{
							strcpy((char*)net_packet->data, "UPHP");
							SDLNet_Write32((Uint32)hitstats->HP, &net_packet->data[4]);
							SDLNet_Write32((Uint32)myStats->type, &net_packet->data[8]);
							net_packet->address.host = net_clients[playerhit - 1].host;
							net_packet->address.port = net_clients[playerhit - 1].port;
							net_packet->len = 12;
							sendPacketSafe(net_sock, -1, net_packet, playerhit - 1);
						}
					}
					else if ( playerhit == 0 )
					{
						if ( pose == MONSTER_POSE_GOLEM_SMASH )
						{
							if ( target == nullptr )
							{
								// primary target
								camera_shakex += .2;
								camera_shakey += 20;
							}
							else
							{
								// secondary target
								camera_shakex += .1;
								camera_shakey += 10;
							}
						}
						else if ( damage > 0 )
						{
							camera_shakex += .1;
							camera_shakey += 10;
						}
						else
						{
							camera_shakex += .05;
							camera_shakey += 5;
						}
					}
					if ( !strcmp(myStats->name, "") )
					{
						Uint32 color = SDL_MapRGB(mainsurface->format, 255, 0, 0);
						if ( myStats->type < KOBOLD ) //Original monster count
						{
							messagePlayerColor(playerhit, color, language[698], language[90 + myStats->type], language[132 + myStats->type]);
						}
						else if ( myStats->type >= KOBOLD ) //New monsters
						{
							messagePlayerColor(playerhit, color, language[698], language[2000 + (myStats->type - KOBOLD)], language[2100 + (myStats->type - KOBOLD)]);
						}
					}
					else
					{
						Uint32 color = SDL_MapRGB(mainsurface->format, 255, 0, 0);
						if ( myStats->type < KOBOLD ) //Original monster count
						{
							messagePlayerColor(playerhit, color, language[699], myStats->name, language[132 + myStats->type]);
						}
						else if ( myStats->type >= KOBOLD ) //New monsters
						{
							messagePlayerColor(playerhit, color, language[699], myStats->name, language[2100 + (myStats->type - KOBOLD)]);
						}
					}
					if ( damage > 0 )
					{
						Entity* gib = spawnGib(hit.entity);
						serverSpawnGibForClient(gib);
					}
					else
					{
						messagePlayer(playerhit, language[700]);
					}
					playSoundEntity(hit.entity, 28, 64);

					// chance of bleeding
					if ( gibtype[(int)hitstats->type] == 1 )
					{
						if ( hitstats->HP > 5 && damage > 0 && !hitstats->EFFECTS[EFF_BLEEDING] )
						{
							if ( (rand() % 20 == 0 && weaponskill != PRO_SWORD) || (rand() % 10 == 0 && weaponskill == PRO_SWORD) 
								|| (rand() % 4 == 0 && pose == MONSTER_POSE_GOLEM_SMASH) )
							{
								hitstats->EFFECTS_TIMERS[EFF_BLEEDING] = std::max(480 + rand() % 360 - hit.entity->getCON() * 100, 120);
								hitstats->EFFECTS[EFF_BLEEDING] = true;
								if ( player > 0 && multiplayer == SERVER )
								{
									serverUpdateEffects(player);
								}
								if ( playerhit >= 0 )
								{
									Uint32 color = SDL_MapRGB(mainsurface->format, 255, 0, 0);
									messagePlayerColor(playerhit, color, language[701]);
								}
								else
								{
									Uint32 color = SDL_MapRGB(mainsurface->format, 0, 255, 0);
									if ( !strcmp(hitstats->name, "") )
									{
										if ( hitstats->type < KOBOLD ) //Original monster count
										{
											messagePlayerColor(player, color, language[702], language[90 + hitstats->type]);
										}
										else if ( hitstats->type >= KOBOLD ) //New monsters
										{
											messagePlayerColor(player, color, language[702], language[2000 + (hitstats->type - KOBOLD)]);
										}
									}
									else
									{
										messagePlayerColor(player, color, language[703], hitstats->name);
									}
								}
							}
						}
					}
					// apply AoE attack
					if ( pose == MONSTER_POSE_GOLEM_SMASH && target == nullptr )
					{
						list_t* aoeTargets = nullptr;
						list_t* shakeTargets = nullptr;
						Entity* tmpEntity = nullptr;
						getTargetsAroundEntity(this, hit.entity, STRIKERANGE, PI / 3, MONSTER_TARGET_ENEMY, &aoeTargets);
						if ( aoeTargets )
						{
							for ( node = aoeTargets->first; node != NULL; node = node->next )
							{
								tmpEntity = (Entity*)node->element;
								if ( tmpEntity != nullptr )
								{
									this->attack(MONSTER_POSE_GOLEM_SMASH, charge, tmpEntity);
								}
							}
							//Free the list.
							list_FreeAll(aoeTargets);
							free(aoeTargets);
						}
						getTargetsAroundEntity(this, hit.entity, STRIKERANGE, PI, MONSTER_TARGET_PLAYER, &shakeTargets);
						if ( shakeTargets )
						{
							// shake nearby players that were not the primary target.
							for ( node = shakeTargets->first; node != NULL; node = node->next )
							{
								tmpEntity = (Entity*)node->element;
								playerhit = tmpEntity->skill[2];
								if ( playerhit > 0 && multiplayer == SERVER )
								{
									strcpy((char*)net_packet->data, "SHAK");
									net_packet->data[4] = 10; // turns into .1
									net_packet->data[5] = 10;
									net_packet->address.host = net_clients[playerhit - 1].host;
									net_packet->address.port = net_clients[playerhit - 1].port;
									net_packet->len = 6;
									sendPacketSafe(net_sock, -1, net_packet, playerhit - 1);
								}
								else if ( playerhit == 0 )
								{
									camera_shakex += 0.1;
									camera_shakey += 10;
								}
							}
							//Free the list.
							list_FreeAll(shakeTargets);
							free(shakeTargets);
						}
					}
				}
			}
		}
		else
		{
			if ( dist != STRIKERANGE )
			{
				// hit a wall
				if ( myStats->weapon != NULL )
				{
					if ( myStats->weapon->type == TOOL_PICKAXE )
					{
						if ( hit.mapx >= 1 && hit.mapx < map.width - 1 && hit.mapy >= 1 && hit.mapy < map.height - 1 )
						{
							playSoundPos(hit.x, hit.y, 67, 128);

							// spawn several rock items
							i = 8 + rand() % 4;
							for ( c = 0; c < i; c++ )
							{
								entity = newEntity(-1, 1, map.entities);
								entity->flags[INVISIBLE] = true;
								entity->flags[UPDATENEEDED] = true;
								entity->x = hit.mapx * 16 + 4 + rand() % 8;
								entity->y = hit.mapy * 16 + 4 + rand() % 8;
								entity->z = -6 + rand() % 12;
								entity->sizex = 4;
								entity->sizey = 4;
								entity->yaw = rand() % 360 * PI / 180;
								entity->vel_x = (rand() % 20 - 10) / 10.0;
								entity->vel_y = (rand() % 20 - 10) / 10.0;
								entity->vel_z = -.25 - (rand() % 5) / 10.0;
								entity->flags[PASSABLE] = true;
								entity->behavior = &actItem;
								entity->flags[USERFLAG1] = true; // no collision: helps performance
								entity->skill[10] = GEM_ROCK;    // type
								entity->skill[11] = WORN;        // status
								entity->skill[12] = 0;           // beatitude
								entity->skill[13] = 1;           // count
								entity->skill[14] = 0;           // appearance
								entity->skill[15] = false;       // identified
							}

							map.tiles[OBSTACLELAYER + hit.mapy * MAPLAYERS + hit.mapx * MAPLAYERS * map.height] = 0;
							// send wall destroy info to clients
							if ( multiplayer == SERVER )
							{
								for ( c = 0; c < MAXPLAYERS; c++ )
								{
									if ( client_disconnected[c] == true )
									{
										continue;
									}
									strcpy((char*)net_packet->data, "WALD");
									SDLNet_Write16((Uint16)hit.mapx, &net_packet->data[4]);
									SDLNet_Write16((Uint16)hit.mapy, &net_packet->data[6]);
									net_packet->address.host = net_clients[c - 1].host;
									net_packet->address.port = net_clients[c - 1].port;
									net_packet->len = 8;
									sendPacketSafe(net_sock, -1, net_packet, c - 1);
								}
							}
							if ( rand() % 2 )
							{
								myStats->weapon->status = static_cast<Status>(myStats->weapon->status - 1);
								if ( myStats->weapon->status == BROKEN )
								{
									messagePlayer(player, language[704]);
									playSoundEntity(this, 76, 64);
								}
								else
								{
									messagePlayer(player, language[705]);
								}
								if ( player > 0 && multiplayer == SERVER )
								{
									strcpy((char*)net_packet->data, "ARMR");
									net_packet->data[4] = 5;
									net_packet->data[5] = myStats->weapon->status;
									net_packet->address.host = net_clients[player - 1].host;
									net_packet->address.port = net_clients[player - 1].port;
									net_packet->len = 6;
									sendPacketSafe(net_sock, -1, net_packet, player - 1);
								}
							}

							// Update the paths so that monsters know they can walk through it
							generatePathMaps();
						}
						else
						{
							spawnBang(hit.x - cos(yaw) * 2, hit.y - sin(yaw) * 2, 0);
							messagePlayer(player, language[706]);
						}
					}
					else
					{
						// bang
						spawnBang(hit.x - cos(yaw) * 2, hit.y - sin(yaw) * 2, 0);
					}
				}
				else
				{
					// bang
					//spawnBang(hit.x - cos(my->yaw)*2,hit.y - sin(my->yaw)*2,0);
					playSoundPos(hit.x, hit.y, 183, 64);
				}
			}

			// apply AoE shake effect
			if ( pose == MONSTER_POSE_GOLEM_SMASH && target == nullptr )
			{
				list_t* shakeTargets = nullptr;
				Entity* tmpEntity = nullptr;
				getTargetsAroundEntity(this, hit.entity, STRIKERANGE, PI, MONSTER_TARGET_PLAYER, &shakeTargets);
				if ( shakeTargets )
				{
					// shake nearby players that were not the primary target.
					for ( node = shakeTargets->first; node != NULL; node = node->next )
					{
						tmpEntity = (Entity*)node->element;
						playerhit = tmpEntity->skill[2];
						if ( playerhit > 0 && multiplayer == SERVER )
						{
							strcpy((char*)net_packet->data, "SHAK");
							net_packet->data[4] = 10; // turns into .1
							net_packet->data[5] = 10;
							net_packet->address.host = net_clients[playerhit - 1].host;
							net_packet->address.port = net_clients[playerhit - 1].port;
							net_packet->len = 6;
							sendPacketSafe(net_sock, -1, net_packet, playerhit - 1);
						}
						else if ( playerhit == 0 )
						{
							camera_shakex += .1;
							camera_shakey += 10;
						}
					}
					//Free the list.
					list_FreeAll(shakeTargets);
					free(shakeTargets);
				}
			}
		}
	}
	else
	{
		if ( player == -1 )
		{
			return;    // clients are NOT supposed to invoke monster attacks in the gamestate!
		}
		strcpy((char*)net_packet->data, "ATAK");
		net_packet->data[4] = player;
		net_packet->data[5] = pose;
		net_packet->data[6] = charge;
		net_packet->address.host = net_server.host;
		net_packet->address.port = net_server.port;
		net_packet->len = 7;
		sendPacketSafe(net_sock, -1, net_packet, 0);
	}
}

/*-------------------------------------------------------------------------------

	AC

	Returns armor class value from a Stat instance

-------------------------------------------------------------------------------*/

int AC(Stat* stat)
{
	if (!stat)
	{
		return 0;
	}

	int armor = stat->CON;

	if (stat->helmet)
	{
		armor += stat->helmet->armorGetAC();
	}
	if (stat->breastplate)
	{
		armor += stat->breastplate->armorGetAC();
	}
	if (stat->gloves)
	{
		armor += stat->gloves->armorGetAC();
	}
	if (stat->shoes)
	{
		armor += stat->shoes->armorGetAC();
	}
	if (stat->shield)
	{
		armor += stat->shield->armorGetAC();
	}
	if (stat->cloak)
	{
		armor += stat->cloak->armorGetAC();
	}
	if (stat->ring)
	{
		armor += stat->ring->armorGetAC();
	}

	if ( stat->shield )
	{
		int shieldskill = stat->PROFICIENCIES[PRO_SHIELD] / 25;
		armor += shieldskill;
		if ( stat->defending )
		{
			armor += 5 + stat->PROFICIENCIES[PRO_SHIELD] / 5;
		}
	}

	return armor;
}

/*-------------------------------------------------------------------------------

	Entity::teleport

	Teleports the given entity to the given (x, y) location on the map,
	in map coordinates. Will not teleport if the destination is an obstacle.

-------------------------------------------------------------------------------*/

void Entity::teleport(int tele_x, int tele_y)
{
	int player = -1;

	if (behavior == &actPlayer)
	{
		player = skill[2];
	}

	if ( strstr(map.name, "Minotaur") || checkObstacle((tele_x << 4) + 8, (tele_y << 4) + 8, this, NULL) )
	{
		messagePlayer(player, language[707]);
		return;
	}

	// play sound effect
	playSoundEntity(this, 77, 64);

	// relocate entity
	double oldx = x;
	double oldy = y;
	x = (tele_x << 4) + 8;
	y = (tele_y << 4) + 8;
	if ( entityInsideSomething(this) )
	{
		x = oldx;
		y = oldy;
		if ( multiplayer == SERVER && player > 0 )
		{
			messagePlayer(player, language[707]);
		}
		return;
	}
	if ( player > 0 && multiplayer == SERVER )
	{
		strcpy((char*)net_packet->data, "TELE");
		net_packet->data[4] = x;
		net_packet->data[5] = y;
		net_packet->address.host = net_clients[player - 1].host;
		net_packet->address.port = net_clients[player - 1].port;
		net_packet->len = 6;
		sendPacketSafe(net_sock, -1, net_packet, player - 1);
	}

	// play second sound effect
	playSoundEntity(this, 77, 64);
}

/*-------------------------------------------------------------------------------

	Entity::teleportRandom

	Teleports the given entity to a random location on the map.

-------------------------------------------------------------------------------*/

void Entity::teleportRandom()
{
	int numlocations = 0;
	int pickedlocation;
	int player = -1;

	if (behavior == &actPlayer )
	{
		player = skill[2];
	}
	for (int iy = 0; iy < map.height; ++iy )
	{
		for (int ix = 0; ix < map.width; ++ix )
		{
			if ( !checkObstacle((ix << 4) + 8, (iy << 4) + 8, this, NULL) )
			{
				numlocations++;
			}
		}
	}
	if ( numlocations == 0 )
	{
		messagePlayer(player, language[708]);
		return;
	}
	pickedlocation = rand() % numlocations;
	numlocations = 0;
	for (int iy = 0; iy < map.height; iy++ )
	{
		for (int ix = 0; ix < map.width; ix++ )
		{
			if ( !checkObstacle((ix << 4) + 8, (iy << 4) + 8, this, NULL) )
			{
				if ( numlocations == pickedlocation )
				{
					teleport(ix, iy);
					return;
				}
				numlocations++;
			}
		}
	}
}

/*-------------------------------------------------------------------------------

	Entity::awardXP

	Awards XP to the dest (ie killer) entity from the src (ie killed) entity

-------------------------------------------------------------------------------*/

void Entity::awardXP(Entity* src, bool share, bool root)
{
	if ( !src )
	{
		return;
	}

	Stat* destStats = getStats();
	Stat* srcStats = src->getStats();

	if ( !destStats || !srcStats )
	{
		return;
	}

	int player = -1;
	if ( behavior == &actPlayer )
	{
		player = skill[2];
	}

	// calculate XP gain
	int xpGain = 10 + rand() % 10 + std::max(0, srcStats->LVL - destStats->LVL) * 10;

	// save hit struct
	hit_t tempHit;
	tempHit.entity = hit.entity;
	tempHit.mapx = hit.mapx;
	tempHit.mapy = hit.mapy;
	tempHit.side = hit.side;
	tempHit.x = hit.x;
	tempHit.y = hit.y;

	// divide shares
	if ( player >= 0 )
	{
		int numshares = 0;
		Entity* shares[MAXPLAYERS];
		int c;

		for ( c = 0; c < MAXPLAYERS; c++ )
		{
			shares[c] = NULL;
		}

		// find other players to divide shares with
		node_t* node;
		for ( node = map.entities->first; node != NULL; node = node->next )
		{
			Entity* entity = (Entity*)node->element;
			if ( entity == this )
			{
				continue;
			}
			if ( entity->behavior == &actPlayer )
			{
				double tangent = atan2( entity->y - src->y, entity->x - src->x );
				lineTrace(src, src->x, src->y, tangent, XPSHARERANGE, 0, false);

				if ( hit.entity == entity )
				{
					numshares++;
					shares[numshares] = entity;
					if ( numshares == MAXPLAYERS - 1 )
					{
						break;
					}
				}
			}
		}

		// divide value of each share
		if ( numshares )
		{
			xpGain /= numshares;
		}

		// award XP to everyone else in the group
		if ( share )
		{
			for ( c = 0; c < MAXPLAYERS; c++ )
			{
				if ( shares[c] )
				{
					shares[c]->awardXP(src, false, false);
				}
			}
		}
	}

	// award XP to main victor
	destStats->EXP += xpGain;

	// award bonus XP and update kill counters
	if ( player >= 0 )
	{
		if ( player == 0 )
		{
			kills[srcStats->type]++;
		}
		else if ( multiplayer == SERVER && player > 0 )
		{
			// inform client of kill
			strcpy((char*)net_packet->data, "MKIL");
			net_packet->data[4] = srcStats->type;
			net_packet->address.host = net_clients[player - 1].host;
			net_packet->address.port = net_clients[player - 1].port;
			net_packet->len = 5;
			sendPacketSafe(net_sock, -1, net_packet, player - 1);

			// update client attributes
			strcpy((char*)net_packet->data, "ATTR");
			net_packet->data[4] = clientnum;
			net_packet->data[5] = (Sint8)destStats->STR;
			net_packet->data[6] = (Sint8)destStats->DEX;
			net_packet->data[7] = (Sint8)destStats->CON;
			net_packet->data[8] = (Sint8)destStats->INT;
			net_packet->data[9] = (Sint8)destStats->PER;
			net_packet->data[10] = (Sint8)destStats->CHR;
			net_packet->data[11] = (Sint8)destStats->EXP;
			net_packet->data[12] = (Sint8)destStats->LVL;
			SDLNet_Write16((Sint16)destStats->HP, &net_packet->data[13]);
			SDLNet_Write16((Sint16)destStats->MAXHP, &net_packet->data[15]);
			SDLNet_Write16((Sint16)destStats->MP, &net_packet->data[17]);
			SDLNet_Write16((Sint16)destStats->MAXMP, &net_packet->data[19]);
			net_packet->address.host = net_clients[player - 1].host;
			net_packet->address.port = net_clients[player - 1].port;
			net_packet->len = 21;
			sendPacketSafe(net_sock, -1, net_packet, player - 1);
		}
	}
	else
	{
		Entity* leader;

		// NPCs with leaders award equal XP to their master (so NPCs don't steal XP gainz)
		if ( (leader = uidToEntity(destStats->leader_uid)) != NULL )
		{
			leader->increaseSkill(PRO_LEADERSHIP);
			leader->awardXP(src, true, false);
		}
	}

	// restore hit struct
	if ( root )
	{
		hit.entity = tempHit.entity;
		hit.mapx = tempHit.mapx;
		hit.mapy = tempHit.mapy;
		hit.side = tempHit.side;
		hit.x = tempHit.x;
		hit.y = tempHit.y;
	}
}

/*-------------------------------------------------------------------------------

	Entity::checkEnemy

	Returns true if my and your are enemies, otherwise returns false

-------------------------------------------------------------------------------*/

bool Entity::checkEnemy(Entity* your)
{
	if (!your)
	{
		return false;
	}

	bool result;

	Stat* myStats = getStats();
	Stat* yourStats = your->getStats();

	if ( !myStats || !yourStats )
	{
		return false;
	}
	if ( everybodyfriendly )   // friendly monsters mode
	{
		return false;
	}

	if ( (your->behavior == &actPlayer || your->behavior == &actPlayerLimb) && (behavior == &actPlayer || behavior == &actPlayerLimb) )
	{
		return false;
	}

	// if you have a leader, check whether we are enemies instead
	Entity* yourLeader = NULL;
	if ( yourStats->leader_uid )
	{
		yourLeader = uidToEntity(yourStats->leader_uid);
	}
	if ( yourLeader )
	{
		Stat* yourLeaderStats = yourLeader->getStats();
		if ( yourLeaderStats )
		{
			if ( yourLeader == this )
			{
				return false;
			}
			else
			{
				return checkEnemy(yourLeader);
			}
		}
	}

	// first find out if I have a leader
	Entity* myLeader = NULL;
	if ( myStats->leader_uid )
	{
		myLeader = uidToEntity(myStats->leader_uid);
	}
	if ( myLeader )
	{
		Stat* myLeaderStats = myLeader->getStats();
		if ( myLeaderStats )
		{
			if ( myLeader == your )
			{
				result = false;
			}
			else
			{
				return myLeader->checkEnemy(your);
			}
		}
		else
		{
			// invalid leader, default to allegiance table
			result = swornenemies[myStats->type][yourStats->type];
		}
	}
	else
	{
		node_t* t_node;
		bool foundFollower = false;
		for ( t_node = myStats->FOLLOWERS.first; t_node != NULL; t_node = t_node->next )
		{
			Uint32* uid = (Uint32*)t_node->element;
			if ( *uid == your->uid )
			{
				foundFollower = true;
				result = false;
				break;
			}
		}
		if ( !foundFollower )
		{
			// no leader, default to allegiance table
			result = swornenemies[myStats->type][yourStats->type];
		}
	}

	// confused monsters mistake their allegiances
	if ( myStats->EFFECTS[EFF_CONFUSED] )
	{
		result = (result == false);
	}

	return result;
}

/*-------------------------------------------------------------------------------

	Entity::checkFriend

	Returns true if my and your are friends, otherwise returns false

-------------------------------------------------------------------------------*/

bool Entity::checkFriend(Entity* your)
{
	bool result;

	if (!your)
	{
		return false;    //Equivalent to if (!myStats || !yourStats)
	}

	Stat* myStats = getStats();
	Stat* yourStats = your->getStats();

	if ( !myStats || !yourStats )
	{
		return false;
	}

	if ( (your->behavior == &actPlayer || your->behavior == &actPlayerLimb) && (behavior == &actPlayer || behavior == &actPlayerLimb) )
	{
		return true;
	}

	// if you have a leader, check whether we are friends instead
	Entity* yourLeader = NULL;
	if ( yourStats->leader_uid )
	{
		yourLeader = uidToEntity(yourStats->leader_uid);
	}
	if ( yourLeader )
	{
		Stat* yourLeaderStats = yourLeader->getStats();
		if ( yourLeaderStats )
		{
			if ( yourLeader == this )
			{
				return true;
			}
			else
			{
				return checkFriend(yourLeader);
			}
		}
	}

	// first find out if I have a leader
	Entity* myLeader = NULL;
	if ( myStats->leader_uid )
	{
		myLeader = uidToEntity(myStats->leader_uid);
	}
	if ( myLeader )
	{
		Stat* myLeaderStats = myLeader->getStats();
		if ( myLeaderStats )
		{
			if ( myLeader == your )
			{
				result = true;
			}
			else
			{
				return myLeader->checkFriend(your);
			}
		}
		else
		{
			// invalid leader, default to allegiance table
			result = monsterally[myStats->type][yourStats->type];
		}
	}
	else
	{
		node_t* t_node;
		bool foundFollower = false;
		for ( t_node = myStats->FOLLOWERS.first; t_node != NULL; t_node = t_node->next )
		{
			Uint32* uid = (Uint32*)t_node->element;
			if ( *uid == your->uid )
			{
				foundFollower = true;
				result = true;
				break;
			}
		}
		if ( !foundFollower )
		{
			// no leader, default to allegiance table
			result = monsterally[myStats->type][yourStats->type];
		}
	}

	return result;
}


void createMonsterEquipment(Stat* stats)
{
	int itemIndex = 0;
	ItemType itemId;
	Status itemStatus;
	int itemBless;
	int itemAppearance = rand();
	int itemCount;
	int chance = 1;
	int category = 0;
	bool itemIdentified;
	if ( stats != nullptr )
	{
		for ( itemIndex = 0; itemIndex < 10; ++itemIndex )
		{
			category = stats->EDITOR_ITEMS[itemIndex * ITEM_SLOT_NUMPROPERTIES + ITEM_SLOT_CATEGORY];
			if ( category > 0 && stats->EDITOR_ITEMS[itemIndex * ITEM_SLOT_NUMPROPERTIES] == 1 )
			{
				if ( category > 0 && category <= 13 )
				{
					itemId = itemCurve(static_cast<Category>(category - 1));
				}
				else
				{
					int randType = 0;
					if ( category == 14 )
					{
						// equipment
						randType = rand() % 2;
						if ( randType == 0 )
						{
							itemId = itemCurve(static_cast<Category>(WEAPON));
						}
						else if ( randType == 1 )
						{
							itemId = itemCurve(static_cast<Category>(ARMOR));
						}
					}
					else if ( category == 15 )
					{
						// jewelry
						randType = rand() % 2;
						if ( randType == 0 )
						{
							itemId = itemCurve(static_cast<Category>(AMULET));
						}
						else
						{
							itemId = itemCurve(static_cast<Category>(RING));
						}
					}
					else if ( category == 16 )
					{
						// magical
						randType = rand() % 3;
						if ( randType == 0 )
						{
							itemId = itemCurve(static_cast<Category>(SCROLL));
						}
						else if ( randType == 1 )
						{
							itemId = itemCurve(static_cast<Category>(MAGICSTAFF));
						}
						else
						{
							itemId = itemCurve(static_cast<Category>(SPELLBOOK));
						}
					}
				}
			}
			else
			{
				itemId = static_cast<ItemType>(stats->EDITOR_ITEMS[itemIndex * ITEM_SLOT_NUMPROPERTIES] - 2);
			}

			if ( itemId >= 0 )
			{
				itemStatus = static_cast<Status>(stats->EDITOR_ITEMS[itemIndex * ITEM_SLOT_NUMPROPERTIES + 1]);
				if ( itemStatus == 0 )
				{
					itemStatus = static_cast<Status>(DECREPIT + rand() % 4);
				}
				itemBless = stats->EDITOR_ITEMS[itemIndex * ITEM_SLOT_NUMPROPERTIES + 2];
				if ( itemBless == 10 )
				{
					itemBless = -2 + rand() % 5;
				}
				itemCount = stats->EDITOR_ITEMS[itemIndex * ITEM_SLOT_NUMPROPERTIES + 3];
				if ( stats->EDITOR_ITEMS[itemIndex * ITEM_SLOT_NUMPROPERTIES + 4] == 1 )
				{
					itemIdentified = false;
				}
				else if ( stats->EDITOR_ITEMS[itemIndex * ITEM_SLOT_NUMPROPERTIES + 4] == 2 )
				{
					itemIdentified = true;
				}
				else
				{
					itemIdentified = rand() % 2;
				}
				itemAppearance = rand();
				chance = stats->EDITOR_ITEMS[itemIndex * ITEM_SLOT_NUMPROPERTIES + 5];

				if ( rand() % 100 < chance )
				{
					switch ( itemIndex ) {
					case 0:
						stats->helmet = newItem(itemId, itemStatus, itemBless, itemCount, itemAppearance, itemIdentified, NULL);
						break;
					case 1:
						stats->weapon = newItem(itemId, itemStatus, itemBless, itemCount, itemAppearance, itemIdentified, NULL);
						break;
					case 2:
						stats->shield = newItem(itemId, itemStatus, itemBless, itemCount, itemAppearance, itemIdentified, NULL);
						break;
					case 3:
						stats->breastplate = newItem(itemId, itemStatus, itemBless, itemCount, itemAppearance, itemIdentified, NULL);
						break;
					case 4:
						stats->shoes = newItem(itemId, itemStatus, itemBless, itemCount, itemAppearance, itemIdentified, NULL);
						break;
					case 5:
						stats->ring = newItem(itemId, itemStatus, itemBless, itemCount, itemAppearance, itemIdentified, NULL);
						break;
					case 6:
						stats->amulet = newItem(itemId, itemStatus, itemBless, itemCount, itemAppearance, itemIdentified, NULL);
						break;
					case 7:
						stats->cloak = newItem(itemId, itemStatus, itemBless, itemCount, itemAppearance, itemIdentified, NULL);
						break;
					case 8:
						stats->mask = newItem(itemId, itemStatus, itemBless, itemCount, itemAppearance, itemIdentified, NULL);
						break;
					case 9:
						stats->gloves = newItem(itemId, itemStatus, itemBless, itemCount, itemAppearance, itemIdentified, NULL);
						break;
					default:
						break;
					}
				}
			}
		}
	}
}

int countCustomItems(Stat* stats)
{
	int x = 0;
	int customItemSlotCount = 0;

	for ( x = ITEM_SLOT_INV_1; x <= ITEM_SLOT_INV_6; x = x + ITEM_SLOT_NUMPROPERTIES )
	{
		if ( stats->EDITOR_ITEMS[x] != 1 || (stats->EDITOR_ITEMS[x] == 1 && stats->EDITOR_ITEMS[x + ITEM_SLOT_CATEGORY] != 0) )
		{
			++customItemSlotCount; //found a custom item in inventory
		}
	}

	return customItemSlotCount; //use custom items from editor instead of default generation
}

int countDefaultItems(Stat* stats)
{
	int x = 0;
	int defaultItemSlotCount = 0;

	for ( x = ITEM_SLOT_INV_1; x <= ITEM_SLOT_INV_6; x = x + ITEM_SLOT_NUMPROPERTIES )
	{
		if ( stats->EDITOR_ITEMS[x] == 1 && stats->EDITOR_ITEMS[x + ITEM_SLOT_CATEGORY] == 0 )
		{
			defaultItemSlotCount++; //found a default item in inventory
		}
	}

	return defaultItemSlotCount;
}

void setRandomMonsterStats(Stat* stats)
{
	if ( stats != nullptr )
	{
		//**************************************
		// HEALTH
		//**************************************

		if ( stats->MAXHP == stats->HP )
		{
			stats->MAXHP += rand() % (stats->RANDOM_MAXHP + 1);

			if ( stats->RANDOM_MAXHP == stats->RANDOM_HP )
			{
				// if the max hp and normal hp range is the same, hp follows the roll of maxhp.
				stats->HP = stats->MAXHP;
			}
			else
			{
				// roll the current hp
				stats->HP += rand() % (stats->RANDOM_HP + 1);
			}
		}
		else
		{
			// roll both ranges independently
			stats->MAXHP += rand() % (stats->RANDOM_MAXHP + 1);
			stats->HP += rand() % (stats->RANDOM_HP + 1);
		}

		if ( stats->HP > stats->MAXHP )
		{
			// check if hp exceeds maximums
			stats->HP = stats->MAXHP;
		}
		stats->OLDHP = stats->HP;

		//**************************************
		// MANA
		//**************************************

		if ( stats->MAXMP == stats->MP )
		{
			stats->MAXMP += rand() % (stats->RANDOM_MAXMP + 1);

			if ( stats->RANDOM_MAXMP == stats->RANDOM_MP )
			{
				// if the max mp and normal mp range is the same, mp follows the roll of maxmp.
				stats->MP = stats->MAXMP;
			}
			else
			{
				// roll the current mp
				stats->MP += rand() % (stats->RANDOM_MP + 1);
			}
		}
		else
		{
			// roll both ranges independently
			stats->MAXMP += rand() % (stats->RANDOM_MAXMP + 1);
			stats->MP += rand() % (stats->RANDOM_MP + 1);
		}

		if ( stats->MP > stats->MAXMP )
		{
			// check if mp exceeds maximums
			stats->MP = stats->MAXMP;
		}

		//**************************************
		// REST OF STATS
		//**************************************

		stats->STR += rand() % (stats->RANDOM_STR + 1);
		stats->DEX += rand() % (stats->RANDOM_DEX + 1);
		stats->CON += rand() % (stats->RANDOM_CON + 1);
		stats->INT += rand() % (stats->RANDOM_INT + 1);
		stats->PER += rand() % (stats->RANDOM_PER + 1);
		stats->CHR += rand() % (stats->RANDOM_CHR + 1);

		stats->LVL += rand() % (stats->RANDOM_LVL + 1);
		stats->GOLD += rand() % (stats->RANDOM_GOLD + 1);
	}

	// debug print out each monster spawned

	/*messagePlayer(0, "Set stats to: ");
	messagePlayer(0, "MAXHP: %d", stats->MAXHP);
	messagePlayer(0, "HP: %d", stats->HP);
	messagePlayer(0, "MAXMP: %d", stats->MAXMP);
	messagePlayer(0, "MP: %d", stats->MP);
	messagePlayer(0, "Str: %d", stats->STR);
	messagePlayer(0, "Dex: %d", stats->DEX);
	messagePlayer(0, "Con: %d", stats->CON);
	messagePlayer(0, "Int: %d", stats->INT);
	messagePlayer(0, "Per: %d", stats->PER);
	messagePlayer(0, "Chr: %d", stats->CHR);
	messagePlayer(0, "LVL: %d", stats->LVL);
	messagePlayer(0, "GOLD: %d", stats->GOLD);*/


	return;
}


int checkEquipType(const Item *item)
{
	switch ( item->type ) {

		case LEATHER_BOOTS:
		case LEATHER_BOOTS_SPEED:
		case IRON_BOOTS:
		case IRON_BOOTS_WATERWALKING:
		case STEEL_BOOTS:
		case STEEL_BOOTS_LEVITATION:
		case STEEL_BOOTS_FEATHER:
		case CRYSTAL_BOOTS:
		case ARTIFACT_BOOTS:
			return TYPE_BOOTS;
			break;

		case LEATHER_HELM:
		case IRON_HELM:
		case STEEL_HELM:
		case CRYSTAL_HELM:
		case ARTIFACT_HELM:
			return TYPE_HELM;
			break;

		case LEATHER_BREASTPIECE:
		case IRON_BREASTPIECE:
		case STEEL_BREASTPIECE:
		case CRYSTAL_BREASTPIECE:
		case WIZARD_DOUBLET:
		case HEALER_DOUBLET:
		case VAMPIRE_DOUBLET:
		case ARTIFACT_BREASTPIECE:
			return TYPE_BREASTPIECE;
			break;

		case CRYSTAL_SHIELD:
		case WOODEN_SHIELD:
		case BRONZE_SHIELD:
		case IRON_SHIELD:
		case STEEL_SHIELD:
		case STEEL_SHIELD_RESISTANCE:
		case MIRROR_SHIELD:
			return TYPE_SHIELD;
			break;

		case TOOL_TORCH:
		case TOOL_LANTERN:
		case TOOL_CRYSTALSHARD:
			return TYPE_OFFHAND;
			break;

		case CLOAK:
		case CLOAK_MAGICREFLECTION:
		case CLOAK_INVISIBILITY:
		case CLOAK_PROTECTION:
		case ARTIFACT_CLOAK:
			return TYPE_CLOAK;
			break;

		case GLOVES:
		case GLOVES_DEXTERITY:
		case GAUNTLETS:
		case GAUNTLETS_STRENGTH:
		case BRACERS:
		case BRACERS_CONSTITUTION:
		case CRYSTAL_GLOVES:
		case ARTIFACT_GLOVES:
		case SPIKED_GAUNTLETS:
		case IRON_KNUCKLES:
		case BRASS_KNUCKLES:
			return TYPE_GLOVES;
			break;

		case HAT_HOOD:
		case HAT_JESTER:
		case HAT_PHRYGIAN:
		case HAT_WIZARD:
			return TYPE_HAT;
			break;

		default:
			break;
	}

	return TYPE_NONE;
}

int setGloveSprite(Stat* myStats, Entity* ent, int spriteOffset)
{
	if ( myStats == nullptr )
	{
		return 0;
	}
	if ( myStats->gloves == nullptr )
	{
		return 0;
	}

	if ( myStats->gloves->type == GLOVES || myStats->gloves->type == GLOVES_DEXTERITY) {
		ent->sprite = 132 + myStats->sex + spriteOffset;
	}
	else if ( myStats->gloves->type == BRACERS || myStats->gloves->type == BRACERS_CONSTITUTION ) {
		ent->sprite = 323 + myStats->sex + spriteOffset;
	}
	else if ( myStats->gloves->type == GAUNTLETS || myStats->gloves->type == GAUNTLETS_STRENGTH ) {
		ent->sprite = 140 + myStats->sex + spriteOffset;
	}
	else if ( myStats->gloves->type == CRYSTAL_GLOVES )
	{
		ent->sprite = 491 + myStats->sex + spriteOffset;
	}
	else if ( myStats->gloves->type == ARTIFACT_GLOVES )
	{
		ent->sprite = 513 + myStats->sex + spriteOffset;
	}
	else if ( myStats->gloves->type == BRASS_KNUCKLES )
	{
		ent->sprite = 531 + myStats->sex + spriteOffset;
	}
	else if ( myStats->gloves->type == IRON_KNUCKLES )
	{
		ent->sprite = 539 + myStats->sex + spriteOffset;
	}
	else if ( myStats->gloves->type == SPIKED_GAUNTLETS )
	{
		ent->sprite = 547 + myStats->sex + spriteOffset;
	}
	else
	{
		return 0;
	}
	return 1;
}

bool Entity::setBootSprite(Entity* leg, int spriteOffset)
{
	if ( multiplayer == CLIENT )
	{
		return false;
	}

	Stat* myStats;

	if ( this->behavior == &actPlayer )
	{
		myStats = stats[this->skill[2]]; // skill[2] contains the player number.
	}
	else
	{
		myStats = this->getStats();
	}

	if ( myStats == nullptr )
	{
		return false;
	}
	if ( myStats->shoes == nullptr )
	{
		return false;
	}

	switch ( myStats->type )
	{
		case HUMAN:
			if ( myStats->shoes->type == LEATHER_BOOTS || myStats->shoes->type == LEATHER_BOOTS_SPEED )
			{
				leg->sprite = 148 + myStats->sex + spriteOffset;
			}
			else if ( myStats->shoes->type == IRON_BOOTS || myStats->shoes->type == IRON_BOOTS_WATERWALKING )
			{
				leg->sprite = 152 + myStats->sex + spriteOffset;
			}
			else if ( myStats->shoes->type >= STEEL_BOOTS && myStats->shoes->type <= STEEL_BOOTS_FEATHER )
			{
				leg->sprite = 156 + myStats->sex + spriteOffset;
			}
			else if ( myStats->shoes->type == CRYSTAL_BOOTS )
			{
				leg->sprite = 499 + myStats->sex + spriteOffset;
			}
			else if ( myStats->shoes->type == ARTIFACT_BOOTS )
			{
				leg->sprite = 521 + myStats->sex + spriteOffset;
			}
			else
			{
				return false;
			}
			break;
		// fall throughs below
		case AUTOMATON:
		case GOATMAN:
		case INSECTOID:
		case KOBOLD:
		case GOBLIN:
		case SKELETON:
		case GNOME:
		case SHADOW:
			if ( myStats->shoes->type == LEATHER_BOOTS || myStats->shoes->type == LEATHER_BOOTS_SPEED )
			{
				leg->sprite = 148 + spriteOffset;
			}
			else if ( myStats->shoes->type == IRON_BOOTS || myStats->shoes->type == IRON_BOOTS_WATERWALKING )
			{
				leg->sprite = 152 + spriteOffset;
			}
			else if ( myStats->shoes->type >= STEEL_BOOTS && myStats->shoes->type <= STEEL_BOOTS_FEATHER )
			{
				leg->sprite = 156 + spriteOffset;
			}
			else if ( myStats->shoes->type == CRYSTAL_BOOTS )
			{
				leg->sprite = 499 + spriteOffset;
			}
			else if ( myStats->shoes->type == ARTIFACT_BOOTS )
			{
				leg->sprite = 521 + spriteOffset;
			}
			else
			{
				return false;
			}
			break;
		default:
			break;
	}
	
	return true;
}


/*-------------------------------------------------------------------------------

sLevitating

returns true if the given entity is levitating, or false if it cannot

-------------------------------------------------------------------------------*/

bool isLevitating(Stat* mystats)
{
	if ( mystats == NULL )
	{
		return false;
	}

	// check levitating value
	bool levitating = false;
	if ( mystats->EFFECTS[EFF_LEVITATING] == true )
	{
		return true;
	}
	if ( mystats->ring != NULL )
	{
		if ( mystats->ring->type == RING_LEVITATION )
		{
			return true;
		}
	}
	if ( mystats->shoes != NULL )
	{
		if ( mystats->shoes->type == STEEL_BOOTS_LEVITATION )
		{
			return true;
		}
	}
	if ( mystats->cloak != NULL )
	{
		if ( mystats->cloak->type == ARTIFACT_CLOAK )
		{
			return true;
		}
	}

	return false;
}

/*-------------------------------------------------------------------------------

getWeaponSkill

returns the proficiency for the weapon equipped.

-------------------------------------------------------------------------------*/

int getWeaponSkill(Item* weapon)
{
	if ( weapon == NULL )
	{
		return -1;
	}

	if ( weapon->type == QUARTERSTAFF || weapon->type == IRON_SPEAR || weapon->type == STEEL_HALBERD || weapon->type == ARTIFACT_SPEAR || weapon->type == CRYSTAL_SPEAR )
	{
		return PRO_POLEARM;
	}
	if ( weapon->type == BRONZE_SWORD || weapon->type == IRON_SWORD || weapon->type == STEEL_SWORD || weapon->type == ARTIFACT_SWORD || weapon->type == CRYSTAL_SWORD )
	{
		return PRO_SWORD;
	}
	if ( weapon->type == BRONZE_MACE || weapon->type == IRON_MACE || weapon->type == STEEL_MACE || weapon->type == ARTIFACT_MACE || weapon->type == CRYSTAL_MACE )
	{
		return PRO_MACE;
	}
	if ( weapon->type == BRONZE_AXE || weapon->type == IRON_AXE || weapon->type == STEEL_AXE || weapon->type == ARTIFACT_AXE || weapon->type == CRYSTAL_BATTLEAXE)
	{
		return PRO_AXE;
	}
	return -1;
}

/*-------------------------------------------------------------------------------

getStatForProficiency

returns the stat associated with the given proficiency.

-------------------------------------------------------------------------------*/

int getStatForProficiency(int skill)
{
	int statForProficiency = -1;

	switch ( skill )
	{
		case PRO_SWORD:			// base attribute: str
		case PRO_MACE:			// base attribute: str
		case PRO_AXE:			// base attribute: str
		case PRO_POLEARM:		// base attribute: str
			statForProficiency = STAT_STR;
			break;
		case PRO_LOCKPICKING:	// base attribute: dex
		case PRO_STEALTH:		// base attribute: dex
		case PRO_RANGED:        // base attribute: dex
			statForProficiency = STAT_DEX;
			break;
		case PRO_SWIMMING:      // base attribute: con
		case PRO_SHIELD:		// base attribute: con
			statForProficiency = STAT_CON;
			break;
		case PRO_SPELLCASTING:  // base attribute: int
		case PRO_MAGIC:         // base attribute: int
			statForProficiency = STAT_INT;
			break;
		case PRO_APPRAISAL:		// base attribute: per
			statForProficiency = STAT_PER;
			break;
		case PRO_TRADING:       // base attribute: chr
		case PRO_LEADERSHIP:    // base attribute: chr
			statForProficiency = STAT_CHR;
			break;
		default:
			statForProficiency = -1;
			break;
	}

	return statForProficiency;
}


int Entity::isEntityPlayer() const
{
   for ( int i = 0; i < numplayers; ++i )
   {
	   if ( this == players[i]->entity )
	   {
		   return i;
	   }
   }

   return -1;
}

int Entity::getReflection() const
{
	Stat *stats = getStats();
	if ( !stats )
	{
		return 0;
	}

	if ( stats->EFFECTS[EFF_MAGICREFLECT] )
	{
		return 3;
	}

	if ( stats->amulet )
	{
		if ( stats->amulet->type == AMULET_MAGICREFLECTION )
		{
			return 2;
		}
	}
	if ( stats->cloak )
	{
		if ( stats->cloak->type == CLOAK_MAGICREFLECTION )
		{
			return 1;
		}
	}
	if ( stats->shield )
	{
		if ( stats->shield->type == MIRROR_SHIELD && stats->defending )
		{
			return 3;
		}
	}
	return 0;
}

int Entity::getAttackPose() const
{
	Stat *myStats = getStats();
	if ( !myStats )
	{
		return -1;
	}

	int pose = 0;

	if ( myStats->weapon != nullptr )
	{
		if ( itemCategory(myStats->weapon) == MAGICSTAFF )
		{
			if ( myStats->type == KOBOLD || myStats->type == AUTOMATON || myStats->type == GOATMAN )
			{
				pose = MONSTER_POSE_MELEE_WINDUP1;
			}
			else
			{
				pose = 3;  // jab
			}
		}
		else if ( itemCategory(myStats->weapon) == SPELLBOOK )
		{
			if ( myStats->type == KOBOLD || myStats->type == AUTOMATON || myStats->type == GOATMAN )
			{
				pose = MONSTER_POSE_MAGIC_WINDUP1;
			}
			else if ( myStats->type == COCKATRICE )
			{
				if ( this->monsterSpecialTimer == MONSTER_SPECIAL_COOLDOWN_COCKATRICE_STONE )
				{
					pose = MONSTER_POSE_MAGIC_WINDUP2;
				}
				else
				{
					pose = MONSTER_POSE_MAGIC_WINDUP1;
				}
			}
			else
			{
				pose = 1;  // vertical swing
			}
		}
		/*else if ( itemCategory(myStats->weapon) == POTION )
		{
			//TODO:
			if ( this->monsterSpecial == MONSTER_SPECIAL_COOLDOWN_GOATMAN_DRINK )
			{
				//
			}
			else if ( this->monsterSpecial == MONSTER_SPECIAL_COOLDOWN_GOATMAN_THROW )
		}*/
		else if ( this->hasRangedWeapon() )
		{
			if ( myStats->type == KOBOLD || myStats->type == AUTOMATON || myStats->type == GOATMAN )
			{
				if ( myStats->weapon->type == CROSSBOW )
				{
					pose = MONSTER_POSE_RANGED_WINDUP1;
				}
				else if ( itemCategory(myStats->weapon) == THROWN )
				{
					pose = MONSTER_POSE_MELEE_WINDUP1;
				}
				else
				{
					pose = MONSTER_POSE_RANGED_WINDUP2;
				}
			}
			else
			{
				pose = 0;
			}
		}
		else
		{
			if ( myStats->type == KOBOLD || myStats->type == AUTOMATON || myStats->type == GOATMAN )
			{
				if ( getWeaponSkill(myStats->weapon) == PRO_AXE || getWeaponSkill(myStats->weapon) == PRO_MACE )
				{
					// axes and maces don't stab
					pose = MONSTER_POSE_MELEE_WINDUP1 + rand() % 2;
				}
				else
				{
					pose = MONSTER_POSE_MELEE_WINDUP1 + rand() % 3;
				}
			}
			else
			{
				pose = rand() % 3 + 1;
			}
		}
	}
	// fists
	else
	{
		if ( myStats->type == KOBOLD || myStats->type == AUTOMATON || myStats->type == GOATMAN )
		{
			pose = MONSTER_POSE_MELEE_WINDUP1;
		}
		else if ( myStats->type == CRYSTALGOLEM )
		{
			if ( this->monsterSpecialTimer == MONSTER_SPECIAL_COOLDOWN_GOLEM )
			{
				pose = MONSTER_POSE_MELEE_WINDUP3;
			}
			else
			{
				pose = MONSTER_POSE_MELEE_WINDUP1 + rand() % 2;
			}
		}
		else if ( myStats->type == COCKATRICE )
		{
			if ( this->monsterSpecialTimer == MONSTER_SPECIAL_COOLDOWN_COCKATRICE_ATK )
			{
				pose = MONSTER_POSE_MELEE_WINDUP3;
			}
			else
			{
				pose = MONSTER_POSE_MELEE_WINDUP1 + rand() % 2;
			}
		}
		else
		{
			pose = 1;
		}
	}

	return pose;
}

bool Entity::hasRangedWeapon() const
{
	Stat *myStats = getStats();
	if ( myStats == nullptr || myStats->weapon == nullptr )
	{
		return false;
	}

	if ( myStats->weapon->type == SLING )
	{
		return true;
	}
	else if ( myStats->weapon->type == SHORTBOW )
	{
		return true;
	}
	else if ( myStats->weapon->type == CROSSBOW )
	{
		return true;
	}
	else if ( myStats->weapon->type == ARTIFACT_BOW )
	{
		return true;
	}
	else if ( itemCategory(myStats->weapon) == MAGICSTAFF )
	{
		return true;
	}
	else if ( itemCategory(myStats->weapon) == SPELLBOOK )
	{
		return true;
	}
	else if ( itemCategory(myStats->weapon) == THROWN )
	{
		return true;
	}
	else if ( itemCategory(myStats->weapon) == GEM )
	{
		return true;
	}
	else if ( itemCategory(myStats->weapon) == POTION )
	{
		return true;
	}
	
	return false;
}

void Entity::handleWeaponArmAttack(Entity* weaponarm)
{
	if ( weaponarm == nullptr )
	{
		return;
	}

	Entity* rightbody = nullptr;
	// set rightbody to left leg.
	node_t* rightbodyNode = list_Node(&this->children, LIMB_HUMANOID_LEFTLEG);
	if ( rightbodyNode )
	{
		rightbody = (Entity*)rightbodyNode->element;
	}
	else
	{
		return;
	}

	// vertical chop windup
	if ( monsterAttack == MONSTER_POSE_MELEE_WINDUP1 )
	{
		if ( monsterAttackTime == 0 )
		{
			// init rotations
			weaponarm->pitch = 0;
			this->monsterArmbended = 0;
			this->monsterWeaponYaw = 0;
			weaponarm->roll = 0;
			weaponarm->skill[1] = 0;
		}

		limbAnimateToLimit(weaponarm, ANIMATE_PITCH, -0.25, 5 * PI / 4, false, 0.0);

		if ( monsterAttackTime >= ANIMATE_DURATION_WINDUP / (monsterGlobalAnimationMultiplier / 10.0) )
		{
			if ( multiplayer != CLIENT )
			{
				this->attack(1, 0, nullptr);
			}
		}
	}
	// vertical chop attack
	else if ( monsterAttack == 1 )
	{
		if ( weaponarm->pitch >= 3 * PI / 2 )
		{
			this->monsterArmbended = 1;
		}

		if ( weaponarm->skill[1] == 0 )
		{
			// chop forwards
			if ( limbAnimateToLimit(weaponarm, ANIMATE_PITCH, 0.4, PI / 3, false, 0.0) )
			{
				weaponarm->skill[1] = 1;
			}
		}
		else if ( weaponarm->skill[1] == 1 )
		{
			// return to neutral
			if ( limbAnimateToLimit(weaponarm, ANIMATE_PITCH, -0.25, 7 * PI / 4, false, 0.0) )
			{
				weaponarm->skill[0] = rightbody->skill[0];
				this->monsterWeaponYaw = 0;
				weaponarm->pitch = rightbody->pitch;
				weaponarm->roll = 0;
				this->monsterArmbended = 0;
				monsterAttack = 0;
			}
		}
	}
	// horizontal chop windup
	else if ( monsterAttack == MONSTER_POSE_MELEE_WINDUP2 )
	{
		if ( monsterAttackTime == 0 )
		{
			// init rotations
			weaponarm->pitch = PI / 4;
			weaponarm->roll = 0;
			this->monsterArmbended = 1;
			weaponarm->skill[1] = 0;
			this->monsterWeaponYaw = 6 * PI / 4;
		}

		limbAnimateToLimit(weaponarm, ANIMATE_ROLL, -0.2, 3 * PI / 2, false, 0.0);
		limbAnimateToLimit(weaponarm, ANIMATE_PITCH, -0.2, 0, false, 0.0);


		if ( monsterAttackTime >= ANIMATE_DURATION_WINDUP / (monsterGlobalAnimationMultiplier / 10.0) )
		{
			if ( multiplayer != CLIENT )
			{
				this->attack(2, 0, nullptr);
			}
		}
	}
	// horizontal chop attack
	else if ( monsterAttack == 2 )
	{
		if ( weaponarm->skill[1] == 0 )
		{
			// swing
			// this->weaponyaw is OK to change for clients, as server doesn't update it for them.
			if ( limbAnimateToLimit(this, ANIMATE_WEAPON_YAW, 0.3, 2 * PI / 8, false, 0.0) )
			{
				weaponarm->skill[1] = 1;
			}
		}
		else if ( weaponarm->skill[1] == 1 )
		{
			// post-swing return to normal weapon yaw
			if ( limbAnimateToLimit(this, ANIMATE_WEAPON_YAW, -0.5, 0, false, 0.0) )
			{
				// restore pitch and roll after yaw is set
				if ( limbAnimateToLimit(weaponarm, ANIMATE_ROLL, 0.4, 0, false, 0.0)
					&& limbAnimateToLimit(weaponarm, ANIMATE_PITCH, -0.4, 7 * PI / 4, false, 0.0) )
				{
					weaponarm->skill[0] = rightbody->skill[0];
					this->monsterWeaponYaw = 0;
					weaponarm->pitch = rightbody->pitch;
					weaponarm->roll = 0;
					this->monsterArmbended = 0;
					monsterAttack = 0;
				}
			}
		}
	}
	// stab windup
	else if ( monsterAttack == MONSTER_POSE_MELEE_WINDUP3 )
	{
		if ( monsterAttackTime == 0 )
		{
			// init rotations
			this->monsterArmbended = 0;
			this->monsterWeaponYaw = 0;
			weaponarm->roll = 0;
			weaponarm->pitch = 0;
			weaponarm->skill[1] = 0;
		}

		limbAnimateToLimit(weaponarm, ANIMATE_PITCH, 0.5, 2 * PI / 3, false, 0.0);

		if ( monsterAttackTime >= ANIMATE_DURATION_WINDUP / (monsterGlobalAnimationMultiplier / 10.0) )
		{
			if ( multiplayer != CLIENT )
			{
				this->attack(3, 0, nullptr);
			}
		}
	}
	// stab attack - refer to weapon limb code for additional animation
	else if ( monsterAttack == 3 )
	{
		if ( weaponarm->skill[1] == 0 )
		{
			if ( limbAnimateToLimit(weaponarm, ANIMATE_PITCH, -0.3, 0, false, 0.0) )
			{
				weaponarm->skill[1] = 1;
			}
		}
		else if ( weaponarm->skill[1] == 1 )
		{
			if ( limbAnimateToLimit(weaponarm, ANIMATE_PITCH, 0.3, 2 * PI / 3, false, 0.0) )
			{
				weaponarm->skill[1] = 2;
			}
		}
		else if ( weaponarm->skill[1] == 2 )
		{
			// return to neutral
			if ( limbAnimateToLimit(weaponarm, ANIMATE_PITCH, -0.2, 0, false, 0.0) )
			{
				weaponarm->skill[0] = rightbody->skill[0];
				this->monsterWeaponYaw = 0;
				weaponarm->pitch = rightbody->pitch;
				weaponarm->roll = 0;
				this->monsterArmbended = 0;
				monsterAttack = 0;
			}
		}
	}
	// ranged weapons
	else if ( monsterAttack == MONSTER_POSE_RANGED_WINDUP1 )
	{
		// crossbow
		if ( monsterAttackTime == 0 )
		{
			// init rotations
			this->monsterArmbended = 0;
			this->monsterWeaponYaw = 0;
			weaponarm->roll = 0;
			weaponarm->skill[1] = 0;
		}

		// draw the crossbow level... slowly
		if ( weaponarm->pitch > PI || weaponarm->pitch < 0 )
		{
			limbAnimateToLimit(weaponarm, ANIMATE_PITCH, 0.1, 0, false, 0.0);
		}
		else
		{
			limbAnimateToLimit(weaponarm, ANIMATE_PITCH, -0.1, 0, false, 0.0);
		}

		if ( monsterAttackTime >= ANIMATE_DURATION_WINDUP / (monsterGlobalAnimationMultiplier / 10.0) )
		{
			if ( multiplayer != CLIENT )
			{
				this->attack(MONSTER_POSE_RANGED_SHOOT1, 0, nullptr);
			}
		}
	}
	// shoot crossbow
	else if ( monsterAttack == MONSTER_POSE_RANGED_SHOOT1 )
	{
		// recoil upwards
		if ( weaponarm->skill[1] == 0 )
		{
			if ( limbAnimateToLimit(weaponarm, ANIMATE_PITCH, -0.2, 15 * PI / 8, false, 0.0) )
			{
				weaponarm->skill[1] = 1;
			}
		}
		// recoil downwards
		else if ( weaponarm->skill[1] == 1 )
		{
			if ( limbAnimateToLimit(weaponarm, ANIMATE_PITCH, 0.1, PI / 3, false, 0.0) )
			{
				weaponarm->skill[1] = 2;
			}
		}
		else if ( weaponarm->skill[1] == 2 )
		{
			// limbAngleWithinRange cuts off animation early so it doesn't snap too far back to position.
			if ( limbAnimateToLimit(weaponarm, ANIMATE_PITCH, -0.2, 0, false, 0.0) || limbAngleWithinRange(weaponarm->pitch, -0.2, rightbody->pitch) )
			{
				weaponarm->skill[0] = rightbody->skill[0];
				this->monsterWeaponYaw = 0;
				//if ( this->hasRangedWeapon() && this->monsterState == MONSTER_STATE_ATTACK )
				//{
				//	// don't move ranged weapons so far if ready to attack
				//	weaponarm->pitch = rightbody->pitch * 0.25;
				//}
				//else
				//{
				//	weaponarm->pitch = rightbody->pitch;
				//}
				weaponarm->roll = 0;
				this->monsterArmbended = 0;
				monsterAttack = 0;
			}
		}
	}
	// shortbow/sling
	else if ( monsterAttack == MONSTER_POSE_RANGED_WINDUP2 )
	{
		if ( monsterAttackTime == 0 )
		{
			// init rotations
			this->monsterArmbended = 0;
			this->monsterWeaponYaw = 0;
			weaponarm->roll = 0;
			weaponarm->skill[1] = 0;
		}

		// draw the weapon level... slowly and shake
		if ( weaponarm->pitch > PI || weaponarm->pitch < 0 )
		{
			limbAnimateToLimit(weaponarm, ANIMATE_PITCH, 0.1, 0, true, 0.1);
		}
		else
		{
			limbAnimateToLimit(weaponarm, ANIMATE_PITCH, -0.1, 0, true, 0.1);
		}

		if ( monsterAttackTime >= ANIMATE_DURATION_WINDUP / (monsterGlobalAnimationMultiplier / 10.0) )
		{
			if ( multiplayer != CLIENT )
			{
				this->attack(MONSTER_POSE_RANGED_SHOOT2, 0, nullptr);
			}
		}
	}
	// shoot shortbow/sling
	else if ( monsterAttack == MONSTER_POSE_RANGED_SHOOT2 )
	{
		// recoil upwards
		if ( weaponarm->skill[1] == 0 )
		{
			if ( limbAnimateToLimit(weaponarm, ANIMATE_PITCH, -0.2, 14 * PI / 8, false, 0.0) )
			{
				weaponarm->skill[1] = 1;
			}
		}
		// recoil downwards
		else if ( weaponarm->skill[1] == 1 )
		{
			if ( limbAnimateToLimit(weaponarm, ANIMATE_PITCH, 0.1, 1 * PI / 3, false, 0.0) )
			{
				weaponarm->skill[1] = 2;
			}
		}
		else if ( weaponarm->skill[1] == 2 )
		{
			// limbAngleWithinRange cuts off animation early so it doesn't snap too far back to position.
			if ( limbAnimateToLimit(weaponarm, ANIMATE_PITCH, -0.2, 0, false, 0.0) || limbAngleWithinRange(weaponarm->pitch, -0.2, rightbody->pitch) )
			{
				weaponarm->skill[0] = rightbody->skill[0];
				this->monsterWeaponYaw = 0;
				weaponarm->pitch = rightbody->pitch;
				weaponarm->roll = 0;
				this->monsterArmbended = 0;
				monsterAttack = 0;
				// play draw arrow sound
				playSoundEntityLocal(this, 246, 16);
			}
		}
	}
	else if ( monsterAttack == MONSTER_POSE_MAGIC_WINDUP1 )
	{
		// magic wiggle hands
		if ( monsterAttackTime == 0 )
		{
			// init rotations
			this->monsterArmbended = 0;
			this->monsterWeaponYaw = 0;
			weaponarm->roll = 0;
			weaponarm->pitch = 0;
			weaponarm->yaw = this->yaw;
			weaponarm->skill[1] = 0;
			// casting particles
			createParticleDot(this);
			// play casting sound
			playSoundEntityLocal(this, 170, 32);
		}

		double animationYawSetpoint = 0.f;
		double animationYawEndpoint = 0.f;
		double armSwingRate = 0.f;
		double animationPitchSetpoint = 0.f;
		double animationPitchEndpoint = 0.f;

		switch ( this->monsterSpellAnimation )
		{
			case MONSTER_SPELLCAST_NONE:
				break;
			case MONSTER_SPELLCAST_SMALL_HUMANOID:
				// smaller models so arms can wave in a larger radius and faster.
				animationYawSetpoint = normaliseAngle2PI(this->yaw + 2 * PI / 8);
				animationYawEndpoint = normaliseAngle2PI(this->yaw - 2 * PI / 8);
				animationPitchSetpoint = 2 * PI / 8;
				animationPitchEndpoint = 14 * PI / 8;
				armSwingRate = 0.3;
				if ( monsterAttackTime == 0 )
				{
					weaponarm->yaw = this->yaw - PI / 8;
				}
				break;
			case MONSTER_SPELLCAST_HUMANOID:
				animationYawSetpoint = normaliseAngle2PI(this->yaw + 1 * PI / 8);
				animationYawEndpoint = normaliseAngle2PI(this->yaw - 1 * PI / 8);
				animationPitchSetpoint = 1 * PI / 8;
				animationPitchEndpoint = 15 * PI / 8;
				armSwingRate = 0.15;
				break;
			default:
				break;
		}

		if ( weaponarm->skill[1] == 0 )
		{
			if ( limbAnimateToLimit(weaponarm, ANIMATE_PITCH, armSwingRate, animationPitchSetpoint, false, 0.0) )
			{
				if ( limbAnimateToLimit(weaponarm, ANIMATE_YAW, armSwingRate, animationYawSetpoint, false, 0.0) )
				{
					weaponarm->skill[1] = 1;
				}
			}
		}
		else
		{
			if ( limbAnimateToLimit(weaponarm, ANIMATE_PITCH, -armSwingRate, animationPitchEndpoint, false, 0.0) )
			{
				if ( limbAnimateToLimit(weaponarm, ANIMATE_YAW, -armSwingRate, animationYawEndpoint, false, 0.0) )
				{
					weaponarm->skill[1] = 0;
				}
			}
		}

		if ( monsterAttackTime >= 2 * ANIMATE_DURATION_WINDUP / (monsterGlobalAnimationMultiplier / 10.0) )
		{
			if ( multiplayer != CLIENT )
			{
				// swing the arm after we prepped the spell
				this->attack(MONSTER_POSE_MAGIC_WINDUP2, 0, nullptr);
			}
		}
	}
	// swing arm to cast spell 
	else if ( monsterAttack == MONSTER_POSE_MAGIC_WINDUP2 )
	{
		if ( monsterAttackTime == 0 )
		{
			// init rotations
			weaponarm->pitch = 0;
			this->monsterArmbended = 0;
			this->monsterWeaponYaw = 0;
			weaponarm->roll = 0;
			weaponarm->skill[1] = 0;
		}

		if ( limbAnimateToLimit(weaponarm, ANIMATE_PITCH, -0.3, 5 * PI / 4, false, 0.0) )
		{
			if ( multiplayer != CLIENT )
			{
				this->attack(1, 0, nullptr);
			}
		}
	}

	return;
}

void Entity::humanoidAnimateWalk(Entity* limb, node_t* bodypartNode, int bodypart, double walkSpeed, double dist, double distForFootstepSound)
{
	if ( bodypart == LIMB_HUMANOID_RIGHTLEG || bodypart == LIMB_HUMANOID_LEFTARM )
	{
		Entity* rightbody = nullptr;
		// set rightbody to left leg.
		node_t* rightbodyNode = list_Node(&this->children, LIMB_HUMANOID_LEFTLEG);
		if ( rightbodyNode )
		{
			rightbody = (Entity*)rightbodyNode->element;
		}
		else
		{
			return;
		}

		node_t* shieldNode = list_Node(&this->children, 8);
		if ( shieldNode )
		{
			Entity* shield = (Entity*)shieldNode->element;
			if ( dist > 0.1 && (bodypart != LIMB_HUMANOID_LEFTARM || shield->sprite == 0 ) )
			{
				// walking to destination
				if ( !rightbody->skill[0] )
				{
					limb->pitch -= dist * walkSpeed;
					if ( limb->pitch < -PI / 4.0 )
					{
						limb->pitch = -PI / 4.0;
						if ( bodypart == LIMB_HUMANOID_RIGHTLEG )
						{
							limb->skill[0] = 1;

							if ( dist > distForFootstepSound )
							{
								if ( this->monsterFootstepType == MONSTER_FOOTSTEP_USE_BOOTS )
								{
									node_t* tempNode = list_Node(&this->children, 3);
									if ( tempNode )
									{
										Entity* foot = (Entity*)tempNode->element;
										playSoundEntityLocal(this, getMonsterFootstepSound(this->monsterFootstepType, foot->sprite), 32);
									}
								}
								else
								{
									playSoundEntityLocal(this, getMonsterFootstepSound(this->monsterFootstepType, 0), 32);
								}
							}
						}
					}
				}
				else
				{
					limb->pitch += dist * walkSpeed;
					if ( limb->pitch > PI / 4.0 )
					{
						limb->pitch = PI / 4.0;
						if ( bodypart == LIMB_HUMANOID_RIGHTLEG )
						{
							limb->skill[0] = 0;
							if ( dist > distForFootstepSound )
							{
								if ( this->monsterFootstepType == MONSTER_FOOTSTEP_USE_BOOTS )
								{
									node_t* tempNode = list_Node(&this->children, 3);
									if ( tempNode )
									{
										Entity* foot = (Entity*)tempNode->element;
										playSoundEntityLocal(this, getMonsterFootstepSound(this->monsterFootstepType, foot->sprite), 32);
									}
								}
								else
								{
									playSoundEntityLocal(this, getMonsterFootstepSound(this->monsterFootstepType, 0), 32);
								}
							}
						}
					}
				}
			}
			else
			{
				// coming to a stop
				if ( limb->pitch < 0 )
				{
					limb->pitch += 1 / fmax(dist * .1, 10.0);
					if ( limb->pitch > 0 )
					{
						limb->pitch = 0;
					}
				}
				else if ( limb->pitch > 0 )
				{
					limb->pitch -= 1 / fmax(dist * .1, 10.0);
					if ( limb->pitch < 0 )
					{
						limb->pitch = 0;
					}
				}
			}
		}
	}
	else if ( bodypart == LIMB_HUMANOID_LEFTLEG || bodypart == LIMB_HUMANOID_RIGHTARM || bodypart == LIMB_HUMANOID_CLOAK )
	{
		if ( bodypart != LIMB_HUMANOID_RIGHTARM || (this->monsterAttack == 0 && this->monsterAttackTime == 0) )
		{
			if ( dist > 0.1 )
			{
				double armMoveSpeed = 1.0;
				if ( bodypart == LIMB_HUMANOID_RIGHTARM && this->hasRangedWeapon() && this->monsterState == MONSTER_STATE_ATTACK )
				{
					// don't move ranged weapons so far if ready to attack
					armMoveSpeed = 0.5;
				}

				if ( limb->skill[0] )
				{
					limb->pitch -= dist * walkSpeed * armMoveSpeed;
					if ( limb->pitch < -PI * armMoveSpeed / 4.0 )
					{
						limb->skill[0] = 0;
						limb->pitch = -PI * armMoveSpeed / 4.0;
					}
				}
				else
				{
					limb->pitch += dist * walkSpeed * armMoveSpeed;
					if ( limb->pitch > PI * armMoveSpeed / 4.0 )
					{
						limb->skill[0] = 1;
						limb->pitch = PI * armMoveSpeed / 4.0;
					}
				}
			}
			else
			{
				if ( limb->pitch < 0 )
				{
					limb->pitch += 1 / fmax(dist * .1, 10.0);
					if ( limb->pitch > 0 )
					{
						limb->pitch = 0;
					}
				}
				else if ( limb->pitch > 0 )
				{
					limb->pitch -= 1 / fmax(dist * .1, 10.0);
					if ( limb->pitch < 0 )
					{
						limb->pitch = 0;
					}
				}
			}
		}
	}

	return;
}

Uint32 Entity::getMonsterFootstepSound(int footstepType, int bootSprite)
{
	int sound = -1;

	switch ( footstepType )
	{
		case MONSTER_FOOTSTEP_SKELETON:
			sound = 95;
			break;
		case MONSTER_FOOTSTEP_STOMP:
			sound = 115;
			break;
		case MONSTER_FOOTSTEP_LEATHER:
			sound = rand() % 7;
			break;
		case MONSTER_FOOTSTEP_USE_BOOTS:
			if ( bootSprite >= 152 && bootSprite <= 155 ) // iron boots
			{
				sound = 7 + rand() % 7;
			}
			else if ( bootSprite >= 156 && bootSprite <= 159 ) // steel boots
			{
				sound = 14 + rand() % 7;
			}
			else if ( bootSprite >= 499 && bootSprite <= 502 ) // crystal boots
			{
				sound = 14 + rand() % 7;
			}
			else if ( bootSprite >= 521 && bootSprite <= 524 ) // artifact boots
			{
				sound = 14 + rand() % 7;
			}
			else
			{
				sound = rand() % 7;
			}
			break;
		case MONSTER_FOOTSTEP_NONE:
		default:
			break;
	}
	return static_cast<Uint32>(sound);
}

void Entity::handleHumanoidWeaponLimb(Entity* weaponLimb, Entity* weaponArmLimb)
{
	if ( weaponLimb == nullptr || weaponArmLimb == nullptr )
	{
		return;
	}

	int monsterType = this->getMonsterTypeFromSprite();

	if ( weaponLimb->flags[INVISIBLE] == false ) //TODO: isInvisible()?
	{
		if ( weaponLimb->sprite == items[SHORTBOW].index )
		{
			weaponLimb->x = weaponArmLimb->x - .5 * cos(weaponArmLimb->yaw);
			weaponLimb->y = weaponArmLimb->y - .5 * sin(weaponArmLimb->yaw);
			weaponLimb->z = weaponArmLimb->z + 1;
			weaponLimb->pitch = weaponArmLimb->pitch + .25;
		}
		else if ( weaponLimb->sprite == items[ARTIFACT_BOW].index )
		{
			weaponLimb->x = weaponArmLimb->x - 1.5 * cos(weaponArmLimb->yaw);
			weaponLimb->y = weaponArmLimb->y - 1.5 * sin(weaponArmLimb->yaw);
			weaponLimb->z = weaponArmLimb->z + 2;
			weaponLimb->pitch = weaponArmLimb->pitch + .25;
		}
		else if ( weaponLimb->sprite == items[CROSSBOW].index )
		{
			weaponLimb->x = weaponArmLimb->x;
			weaponLimb->y = weaponArmLimb->y;
			weaponLimb->z = weaponArmLimb->z + 1;
			weaponLimb->pitch = weaponArmLimb->pitch;
		}
		else
		{
			/*weaponLimb->focalx = limbs[monsterType][6][0];
			weaponLimb->focalz = limbs[monsterType][6][2];*/
			if ( this->monsterAttack == 3 )
			{
				// poking animation, weapon pointing straight ahead.
				if ( weaponArmLimb->skill[1] < 2 && weaponArmLimb->pitch < PI / 2 )
				{
					// cos(weaponArmLimb->pitch)) * cos(weaponArmLimb->yaw) allows forward/back motion dependent on the arm rotation.
					weaponLimb->x = weaponArmLimb->x + (3 * cos(weaponArmLimb->pitch)) * cos(weaponArmLimb->yaw);
					weaponLimb->y = weaponArmLimb->y + (3 * cos(weaponArmLimb->pitch)) * sin(weaponArmLimb->yaw);

					if ( weaponArmLimb->pitch < PI / 3 )
					{
						// adjust the z point halfway through swing.
						weaponLimb->z = weaponArmLimb->z + 1.5 - 2 * cos(weaponArmLimb->pitch / 2);
					}
					else
					{
						weaponLimb->z = weaponArmLimb->z - .5 * (this->monsterAttack == 0);
						if ( weaponLimb->pitch > PI / 2 )
						{
							limbAnimateToLimit(weaponLimb, ANIMATE_PITCH, -0.5, PI * 0.5, false, 0);
						}
						else
						{
							limbAnimateToLimit(weaponLimb, ANIMATE_PITCH, 0.5, PI * 0.5, false, 0);
						}
					}
				}
				// hold sword with pitch aligned to arm rotation.
				else
				{
					weaponLimb->x = weaponArmLimb->x + .5 * cos(weaponArmLimb->yaw) * (this->monsterAttack == 0);
					weaponLimb->y = weaponArmLimb->y + .5 * sin(weaponArmLimb->yaw) * (this->monsterAttack == 0);
					weaponLimb->z = weaponArmLimb->z - .5;
					weaponLimb->pitch = weaponArmLimb->pitch + .25 * (this->monsterAttack == 0);
				}
			}
			else
			{
				weaponLimb->x = weaponArmLimb->x + .5 * cos(weaponArmLimb->yaw) * (this->monsterAttack == 0);
				weaponLimb->y = weaponArmLimb->y + .5 * sin(weaponArmLimb->yaw) * (this->monsterAttack == 0);
				weaponLimb->z = weaponArmLimb->z - .5 * (this->monsterAttack == 0);
				weaponLimb->pitch = weaponArmLimb->pitch + .25 * (this->monsterAttack == 0);
			}
		}
	}

	weaponLimb->yaw = weaponArmLimb->yaw;
	weaponLimb->roll = weaponArmLimb->roll;

	if ( !this->monsterArmbended )
	{
		weaponLimb->focalx = limbs[monsterType][6][0]; // 2.5
		if ( weaponLimb->sprite == items[CROSSBOW].index )
		{
			weaponLimb->focalx += 2;
		}
		weaponLimb->focaly = limbs[monsterType][6][1]; // 0
		weaponLimb->focalz = limbs[monsterType][6][2]; // -.5
	}
	else
	{
		weaponLimb->focalx = limbs[monsterType][6][0] + 1; // 3.5
		weaponLimb->focaly = limbs[monsterType][6][1]; // 0
		weaponLimb->focalz = limbs[monsterType][6][2] - 2; // -2.5
		weaponLimb->yaw -= sin(weaponArmLimb->roll) * PI / 2;
		weaponLimb->pitch += cos(weaponArmLimb->roll) * PI / 2;
	}

	return;
}

void Entity::lookAtEntity(Entity& target)
{
	double tangent = atan2(target.y - y, target.x - x);
	monsterLookTime = 1;
	monsterMoveTime = rand() % 10 + 1;
	monsterLookDir = tangent;
}

spell_t* Entity::getActiveMagicEffect(int spellID)
{
	Stat* myStats = getStats();
	if ( !myStats )
	{
		return nullptr;
	}

	spell_t* spell = nullptr;
	spell_t* searchSpell = nullptr;

	for ( node_t *node = myStats->magic_effects.first; node; node = node->next )
	{
		searchSpell = (node->element? static_cast<spell_t*>(node->element) : nullptr);
		if ( searchSpell && searchSpell->ID == spellID )
		{
			spell = searchSpell;
			break;
		}
	}

	return spell;
}

void actAmbientParticleEffectIdle(Entity* my)
{
	if ( !my )
	{
		return;
	}

	if ( my->particleDuration < 0 )
	{
		list_RemoveNode(my->mynode);
		return;
	}
	else
	{
		--my->particleDuration;
		my->z += my->vel_z;
	}

	return;
}

void Entity::spawnAmbientParticles(int chance, int particleSprite, int duration)
{
	if ( rand() % chance == 0 )
	{
		Entity* spawnParticle = newEntity(particleSprite, 1, map.entities);
		spawnParticle->sizex = 1;
		spawnParticle->sizey = 1;
		spawnParticle->x = x + (-2 + rand() % 5);
		spawnParticle->y = y + (-2 + rand() % 5);
		spawnParticle->z = 7.5;
		spawnParticle->vel_z = -1;
		spawnParticle->particleDuration = duration;
		spawnParticle->behavior = &actAmbientParticleEffectIdle;
		spawnParticle->flags[PASSABLE] = true;
		spawnParticle->setUID(-3);
	}
}

void Entity::handleEffectsClient()
{
	Stat* myStats = getStats();

	if ( !myStats )
	{
		return;
	}

	if ( myStats->EFFECTS[EFF_MAGICREFLECT] )
	{
		spawnAmbientParticles(80, 579, 10 + rand() % 40);
	}
}

void Entity::serverUpdateEffectsForEntity(bool guarantee)
{
	if ( multiplayer != SERVER )
	{
		return;
	}

	Stat* myStats = getStats();

	if ( !myStats )
	{
		return;
	}

	for ( int player = 1; player < numplayers; ++player )
	{
		if ( client_disconnected[player] )
		{
			continue;
		}

		/*
		 * Packet breakdown:
		 * [0][1][2][3]: "EFFE"
		 * [4][5][6][7]: Entity's UID.
		 * [8][9][10][11]: Entity's effects.
		 */

		strcpy((char*)net_packet->data, "EFFE");
		SDLNet_Write32(static_cast<Uint32>(getUID()), &net_packet->data[4]);
		net_packet->data[8] = 0;
		net_packet->data[9] = 0;
		net_packet->data[10] = 0;
		net_packet->data[11] = 0;
		for ( int i = 0; i < NUMEFFECTS; ++i )
		{
			if ( myStats->EFFECTS[i] )
			{
				net_packet->data[8 + i / 8] |= power(2, i - (i / 8) * 8);
			}
		}
		net_packet->address.host = net_clients[player - 1].host;
		net_packet->address.port = net_clients[player - 1].port;
		net_packet->len = 12;
		if ( guarantee )
		{
			sendPacketSafe(net_sock, -1, net_packet, player - 1);
		}
		else
		{
			sendPacket(net_sock, -1, net_packet, player - 1);
		}
		clientsHaveItsStats = true;
	}
}

void Entity::setEffect(int effect, bool value, int duration, bool updateClients, bool guarantee)
{
	Stat* myStats = getStats();

	if ( !myStats )
	{
		return;
	}

	myStats->EFFECTS[effect] = value;
	myStats->EFFECTS_TIMERS[effect] = duration;

	int player = -1;
	for ( int i = 0; i < numplayers; ++i )
	{
		if ( players[i]->entity == this )
		{
			player = i;
			break;
		}
	}

	if ( multiplayer == SERVER && player > 0 )
	{
		serverUpdateEffects(player);
	}

	if ( updateClients )
	{
		serverUpdateEffectsForEntity(guarantee);
	}
}

void Entity::giveClientStats()
{
	if ( !clientStats )
	{
		clientStats = new Stat(0);
	}
}

void Entity::monsterAcquireAttackTarget(const Entity& target, Sint32 state)
{
	messagePlayer(clientnum, "Entity acquired target!");

	monsterState = state;
	monsterTarget = target.getUID();
	monsterTargetX = target.x;
	monsterTargetY = target.y;

	//TODO: Goatman switch equipped weapon.
	//Ranged at range, melee in close quarters. Special attacks = potions & throwing stuff.

	//On acquiring a target, some monsters switch out their weapons.
	//chooseWeapon();
}

/*void Entity::chooseWeapon()
{
	Stat* myStats = getStats();
	if ( !myStats )
	{
		return;
	}

	switch ( myStats->type )
	{
		case GOATMAN:
			goatmanChooseWeapon();
			break;
		default:
			break;
	}
}*/

void Entity::checkGroundForItems()
{
	Stat* myStats = getStats();
	if ( !myStats )
	{
		return;
	}

	//Calls the function for a monster to pick up an item, if it's a monster that picks up items.
	switch ( myStats->type )
	{
		case GOBLIN:
		case HUMAN:
			if ( !strcmp(myStats->name, "") )
			{
				//checkBetterEquipment(myStats);
				monsterAddNearbyItemToInventory(myStats, 16, 9);
			}
			break;
		case GOATMAN:
			//Goatman boss picks up items too.
			monsterAddNearbyItemToInventory(myStats, 16, 9); //Replaces checkBetterEquipment(), because more better. Adds items to inventory, and swaps out current equipped with better stuff on the ground.
			//checkBetterEquipment(myStats);
			break;
		case AUTOMATON:
			monsterAddNearbyItemToInventory(myStats, 8, 5);
			break;
		default:
			return;
	}
}

bool Entity::canWieldItem(const Item& item) const
{
	Stat* myStats = getStats();
	if ( !myStats )
	{
		return false;
	}

	switch ( myStats->type )
	{
		case GOBLIN:
			return goblinCanWieldItem(item);
		case HUMAN:
			return humanCanWieldItem(item);
		case GOATMAN:
			return goatmanCanWieldItem(item);
		case AUTOMATON:
			return automatonCanWieldItem(item);
		default:
			return false;
	}
}

void Entity::monsterAddNearbyItemToInventory(Stat* myStats, int rangeToFind, int maxInventoryItems)
{
	//TODO: Any networking/multiplayer needs?
	if ( !myStats )
	{
		return; //Can't continue without these.
	}

	list_t* items = nullptr;
	//X and Y in terms of tiles.
	int tx = x / 16;
	int ty = y / 16;
	getItemsOnTile(tx, ty, &items); //Check the tile the monster is on for items.
	getItemsOnTile(tx - 1, ty, &items); //Check tile to the left.
	getItemsOnTile(tx + 1, ty, &items); //Check tile to the right.
	getItemsOnTile(tx, ty - 1, &items); //Check tile up.
	getItemsOnTile(tx, ty + 1, &items); //Check tile down.
	getItemsOnTile(tx - 1, ty - 1, &items); //Check tile diagonal up left.
	getItemsOnTile(tx + 1, ty - 1, &items); //Check tile diagonal up right.
	getItemsOnTile(tx - 1, ty + 1, &items); //Check tile diagonal down left.
	getItemsOnTile(tx + 1, ty + 1, &items); //Check tile diagonal down right.
	node_t* node = nullptr;

	if ( items )
	{
		/*
		* Rundown of the function:
		* Loop through all items.
		* Add item to inventory.
		*/

		for ( node = items->first; node != nullptr; node = node->next )
		{
			//Turn the entity into an item.
			if ( node->element )
			{
				if ( list_Size(&myStats->inventory) >= maxInventoryItems )
				{
					break;
				}

				Entity* entity = (Entity*)node->element;
				Item* item = nullptr;
				if ( entity != nullptr )
				{
					item = newItemFromEntity(entity);
				}
				if ( !item )
				{
					continue;
				}

				double dist = sqrt(pow(this->x - entity->x, 2) + pow(this->y - entity->y, 2));
				if ( std::floor(dist) > rangeToFind )
				{
					// item was too far away, continue.
					if ( item != nullptr )
					{
						free(item);
					}
					continue;
				}

				if ( !entity->itemNotMoving && entity->parent && entity->parent != uid )
				{
					if ( itemCategory(item) == THROWN && entity->parent && entity->parent == uid )
					{
						//It's good. Can pick this one up, it's your THROWN now.
					}
					else
					{
						//Don't pick up non-THROWN items that are moving, or owned THROWN that are moving.
						if ( item != nullptr )
						{
							free(item);
						}
						continue; //Item still in motion, don't pick it up.
					}
				}

				Item** shouldWield = nullptr;
				node_t* replaceInventoryItem = nullptr;
				if ( !monsterWantsItem(*item, shouldWield, replaceInventoryItem) )
				{
					if ( item != nullptr )
					{
						free(item);
					}
					continue;
				}

				if ( shouldWield )
				{
					if ( (*shouldWield) && (*shouldWield)->beatitude < 0 )
					{
						if ( item != nullptr )
						{
							free(item);
						}
						continue;
					}
					dropItemMonster((*shouldWield), this, myStats); //And I threw it on the ground!
					(*shouldWield) = item;
					item = nullptr;
					list_RemoveNode(entity->mynode);
				}
				else if ( replaceInventoryItem )
				{
					//Drop that item out of the monster's inventory, and add this item to the monster's inventory.
					Item* itemToDrop = static_cast<Item*>(replaceInventoryItem->element);
					if ( itemToDrop )
					{
						dropItemMonster(itemToDrop, this, myStats, itemToDrop->count);
						//list_RemoveNode(replaceInventoryItem);
					}
					addItemToMonsterInventory(item);
					item = nullptr;
					list_RemoveNode(entity->mynode);
				}
				else
				{
					addItemToMonsterInventory(item);
					item = nullptr;
					list_RemoveNode(entity->mynode);
				}

				if ( item != nullptr )
				{
					free(item);
				}
			}
		}
		list_FreeAll(items);
		free(items);
	}
}

node_t* Entity::addItemToMonsterInventory(Item* item)
{
	//TODO: Sort into inventory...that is, if an item of this type already exists and they can stack, stack 'em instead of creating a new node.
	if ( !item )
	{
		return nullptr;
	}

	Stat* myStats = getStats();
	if ( !myStats )
	{
		return nullptr;
	}

	item->node = list_AddNodeLast(&myStats->inventory);
	if ( !item->node )
	{
		return nullptr;
	}
	item->node->element = item;
	item->node->deconstructor = &defaultDeconstructor;
	item->node->size = sizeof(Item);

	return item->node;
}

bool Entity::shouldMonsterEquipThisWeapon(const Item& itemToEquip) const
{
	Stat* myStats = getStats();
	if ( !myStats )
	{
		return false;
	}

	if (myStats->weapon == nullptr)
	{
		return true; //Something is better than nothing.
	}
	//Monster is already holding a weapon.

	if ( !Item::isThisABetterWeapon(itemToEquip, myStats->weapon) )
	{
		return false; //Don't want junk.
	}

	if ( myStats->weapon->beatitude < 0 )
	{
		//If monster already holding an item, can't swap it out if it's cursed.
		return false;
	}

	if ( itemCategory(myStats->weapon) == MAGICSTAFF || itemCategory(myStats->weapon) == POTION || itemCategory(myStats->weapon) == THROWN || itemCategory(myStats->weapon) == GEM )
	{
		//If current hand item is not cursed, but it's a certain item, don't want to equip this new one.
		return false;
	}

	return true;
}

bool Entity::monsterWantsItem(const Item& item, Item**& shouldEquip, node_t*& replaceInventoryItem) const
{
	Stat* myStats = getStats();
	if ( !myStats )
	{
		return false;
	}

	switch ( myStats->type )
	{
		case GOBLIN:
			if ( !goblinCanWieldItem(item) )
			{
				return false;
			}
			break;
		case HUMAN:
			if ( !humanCanWieldItem(item) )
			{
				return false;
			}
			break;
		case GOATMAN:
			if ( !goatmanCanWieldItem(item) )
			{
				return false;
			}
			break;
		case AUTOMATON:
			return true; //Can pick up all items, because recycler.
		default:
			return false;
	}

	switch ( itemCategory(&item) )
	{
		case WEAPON:
			if ( !myStats->weapon )
			{
				shouldEquip = &myStats->weapon;
			}

			if ( myStats->weapon && itemCategory(myStats->weapon) == WEAPON && shouldMonsterEquipThisWeapon(item) )
			{
				shouldEquip = &myStats->weapon;
				return true;
			}
			else
			{
				if ( myStats->weapon && itemCategory(myStats->weapon) == WEAPON )
				{
					//Weapon ain't better than weapon already holding. Don't want it.
					return false;
				}

				//Not holding a weapon. Make sure don't already have a weapon in the inventory. If doesn't have a weapon at all, then add it into the inventory since something is better than nothing.
				node_t* weaponNode = itemNodeInInventory(myStats, static_cast<ItemType>(-1), WEAPON);
				if ( !weaponNode )
				{
					//If no weapons found in inventory, then yes, the goatman wants it, and it should be added to the inventory.
					return true; //Want this item.
				}

				//Search inventory and replace weapon if this one is better.
				if ( Item::isThisABetterWeapon(item, static_cast<Item*>(weaponNode->element)) )
				{
					replaceInventoryItem = weaponNode;
					return true;
				}
				return false; //Don't want your junk.
			}
		case ARMOR:
			return (shouldEquip = shouldMonsterEquipThisArmor(item));
		case THROWN:
			if ( myStats->weapon == nullptr )
			{
				shouldEquip = &myStats->weapon;
				return true;
			}
			else
			{
				return true; //Store in inventory.
			}
		default:
			return true; //Already checked if monster likes this specific item in the racial calls.
	}

	return false;
}

Item** Entity::shouldMonsterEquipThisArmor(const Item& item) const
{
	Stat* myStats = getStats();
	if ( !myStats )
	{
		return nullptr;
	}

	switch ( checkEquipType(&item) )
	{
		case TYPE_HAT:
			if ( myStats->helmet && myStats->helmet->beatitude < 0 )
			{
				return nullptr; //No can has hats : (
			}

			return Item::isThisABetterArmor(item, myStats->helmet)? &myStats->helmet : nullptr;
		case TYPE_HELM:
			if ( myStats->helmet && myStats->helmet->beatitude < 0 )
			{
				return nullptr; //Can't swap out armor piece if current one is cursed!
			}

			if ( myStats->type == GOBLIN && myStats->helmet && checkEquipType(myStats->helmet) == TYPE_HAT )
			{
				return nullptr; //Goblins love hats.
			}

			return Item::isThisABetterArmor(item, myStats->helmet)? &myStats->helmet : nullptr;
			break;
		case TYPE_SHIELD:
			if ( myStats->shield && myStats->shield->beatitude < 0 )
			{
				return nullptr; //Can't swap out armor piece if current one is cursed!
			}

			return Item::isThisABetterArmor(item, myStats->shield)? &myStats->shield : nullptr;
		case TYPE_BREASTPIECE:
			if ( myStats->breastplate && myStats->breastplate->beatitude < 0 )
			{
				return nullptr; //Can't swap out armor piece if current one is cursed!
			}

			return Item::isThisABetterArmor(item, myStats->breastplate)? &myStats->breastplate : nullptr;
		case TYPE_CLOAK:
			if ( myStats->cloak && myStats->cloak->beatitude < 0 )
			{
				return nullptr; //Can't swap out armor piece if current one is cursed!
			}

			return Item::isThisABetterArmor(item, myStats->cloak)? &myStats->cloak : nullptr;
		case TYPE_BOOTS:
			if ( myStats->shoes && myStats->shoes->beatitude < 0 )
			{
				return nullptr; //Can't swap out armor piece if current one is cursed!
			}

			return Item::isThisABetterArmor(item, myStats->shoes)? &myStats->shoes : nullptr;
		case TYPE_GLOVES:
			if ( myStats->gloves && myStats->gloves->beatitude < 0 )
			{
				return nullptr; //Can't swap out armor piece if current one is cursed!
			}

			return Item::isThisABetterArmor(item, myStats->gloves)? &myStats->gloves : nullptr;
		default:
			return nullptr;
	}
}

