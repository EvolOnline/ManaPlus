/*
 *  The ManaPlus Client
 *  Copyright (C) 2004-2009  The Mana World Development Team
 *  Copyright (C) 2009-2010  The Mana Developers
 *  Copyright (C) 2011-2012  The ManaPlus Developers
 *
 *  This file is part of The ManaPlus Client.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "net/ea/beinghandler.h"

#include "net/ea/eaprotocol.h"

#include "actorspritemanager.h"
#include "being.h"
#include "client.h"
#include "effectmanager.h"
#include "guild.h"
#include "guildmanager.h"
#include "inputmanager.h"
#include "keyboardconfig.h"
#include "localplayer.h"
#include "logger.h"
#include "party.h"
#include "playerrelations.h"
#include "configuration.h"

#include "gui/botcheckerwindow.h"
#include "gui/outfitwindow.h"
#include "gui/socialwindow.h"
#include "gui/killstats.h"

#include "utils/gettext.h"
#include "utils/stringutils.h"

#include "net/playerhandler.h"
#include "net/net.h"

#include "resources/colordb.h"
#include "resources/itemdb.h"
#include "resources/iteminfo.h"

#include <iostream>

#include "debug.h"

namespace Ea
{
const int EMOTION_TIME = 500;    /**< Duration of emotion icon */

BeingHandler::BeingHandler(bool enableSync) :
    mSync(enableSync),
    mSpawnId(0)
{
}

Being *BeingHandler::createBeing(int id, short job)
{
    if (!actorSpriteManager)
        return nullptr;

    ActorSprite::Type type = ActorSprite::UNKNOWN;
    if (job <= 25 || (job >= 4001 && job <= 4049))
        type = ActorSprite::PLAYER;
    else if (job >= 46 && job <= 1000)
        type = ActorSprite::NPC;
    else if (job > 1000 && job <= 2000)
        type = ActorSprite::MONSTER;
    else if (job == 45)
        type = ActorSprite::PORTAL;

    Being *being = actorSpriteManager->createBeing(id, type, job);

    if (type == ActorSprite::PLAYER || type == ActorSprite::NPC)
    {
        being->updateFromCache();
        requestNameById(id);
        if (player_node)
            player_node->checkNewName(being);
    }
    if (type == Being::PLAYER)
    {
        if (botCheckerWindow)
            botCheckerWindow->updateList();
        if (socialWindow)
            socialWindow->updateActiveList();
    }
    return being;
}

void BeingHandler::setSprite(Being *being, unsigned int slot, int id,
                             std::string color, unsigned char colorId,
                             bool isWeapon, bool isTempSprite)
{
    if (!being)
        return;
    being->setSprite(slot, id, color, colorId, isWeapon, isTempSprite);
}

void BeingHandler::processBeingVisibleOrMove(Net::MessageIn &msg, bool visible)
{
    if (!actorSpriteManager)
        return;

    int id;
    short job, speed, gender;
    uint16_t headTop, headMid, headBottom;
    uint16_t shoes, gloves;
    uint16_t weapon, shield;
    uint16_t stunMode;
    uint32_t statusEffects;
    Being *dstBeing;
    int hairStyle, hairColor;
    int spawnId;

    // Information about a being in range
    id = msg.readInt32();
    if (id == mSpawnId)
        spawnId = mSpawnId;
    else
        spawnId = 0;
    mSpawnId = 0;
    speed = msg.readInt16();
    stunMode = msg.readInt16();  // opt1
    statusEffects = msg.readInt16();  // opt2
    statusEffects |= (static_cast<uint32_t>(msg.readInt16())) << 16;  // option
    job = msg.readInt16();  // class

    dstBeing = actorSpriteManager->findBeing(id);

    if (dstBeing && dstBeing->getType() == Being::MONSTER
        && !dstBeing->isAlive())
    {
        actorSpriteManager->destroy(dstBeing);
        actorSpriteManager->erase(dstBeing);
        dstBeing = nullptr;
    }

    if (!dstBeing)
    {
        // Being with id >= 110000000 and job 0 are better
        // known as ghosts, so don't create those.
        if (job == 0 && id >= 110000000)
            return;

        if (actorSpriteManager->isBlocked(id) == true)
            return;

        dstBeing = createBeing(id, job);

        if (!dstBeing)
            return;

        if (job == 1022 && killStats)
            killStats->jackoAlive(dstBeing->getId());
    }
    else
    {
        // undeleting marked for deletion being
        if (dstBeing->getType() == Being::NPC)
            actorSpriteManager->undelete(dstBeing);
    }

    if (dstBeing->getType() == Being::PLAYER)
        dstBeing->setMoveTime();

    if (spawnId)
    {
        dstBeing->setAction(Being::SPAWN);
    }
    else if (visible)
    {
        dstBeing->clearPath();
        dstBeing->setActionTime(tick_time);
        dstBeing->setAction(Being::STAND);
    }

    // Prevent division by 0 when calculating frame
    if (speed == 0)
        speed = 150;

    dstBeing->setWalkSpeed(Vector(speed, speed, 0));
    dstBeing->setSubtype(job);
    if (dstBeing->getType() == ActorSprite::MONSTER && player_node)
        player_node->checkNewName(dstBeing);

    hairStyle = msg.readInt16();
    weapon = msg.readInt16();
    headBottom = msg.readInt16();

    if (!visible)
        msg.readInt32(); // server tick

    shield = msg.readInt16();
    headTop = msg.readInt16();
    headMid = msg.readInt16();
    hairColor = msg.readInt16();
    shoes = msg.readInt16();  // clothes color - "abused" as shoes

    if (dstBeing->getType() == ActorSprite::MONSTER)
    {
        if (serverVersion > 0)
        {
            int hp = msg.readInt32();
            int maxHP = msg.readInt32();
            if (hp && maxHP)
            {
                dstBeing->setMaxHP(maxHP);
                int oldHP = dstBeing->getHP();
                if (!oldHP || oldHP > hp)
                    dstBeing->setHP(hp);
            }
        }
        else
        {
            msg.readInt32();
            msg.readInt32();
        }
        gloves = 0;
    }
    else
    {
        gloves = msg.readInt16();  // head dir - "abused" as gloves
        msg.readInt32();  // guild
        msg.readInt16();  // guild emblem
    }
//            logger->log("being guild: " + toString(guild));
/*
    if (guild == 0)
        dstBeing->clearGuilds();
    else
        dstBeing->setGuild(Guild::getGuild(static_cast<short>(guild)));
*/

    msg.readInt16();  // manner
    dstBeing->setStatusEffectBlock(32, msg.readInt16());  // opt3
    if (serverVersion > 0 && dstBeing->getType() == ActorSprite::MONSTER)
    {
        int attackRange = msg.readInt8();   // karma
        dstBeing->setAttackRange(attackRange);
    }
    else
    {
        msg.readInt8();   // karma
    }
    gender = msg.readInt8();

    // reserving bits for future usage

    if (dstBeing->getType() == ActorSprite::PLAYER)
    {
        gender &= 3;
        dstBeing->setGender(Being::intToGender(gender));
        // Set these after the gender, as the sprites may be gender-specific
        setSprite(dstBeing, EA_SPRITE_HAIR, hairStyle * -1,
            ItemDB::get(-hairStyle).getDyeColorsString(hairColor));
        setSprite(dstBeing, EA_SPRITE_BOTTOMCLOTHES, headBottom);
        setSprite(dstBeing, EA_SPRITE_TOPCLOTHES, headMid);
        setSprite(dstBeing, EA_SPRITE_HAT, headTop);
        setSprite(dstBeing, EA_SPRITE_SHOE, shoes);
        setSprite(dstBeing, EA_SPRITE_GLOVES, gloves);
        setSprite(dstBeing, EA_SPRITE_WEAPON, weapon, "", true);
        if (!config.getBoolValue("hideShield"))
            setSprite(dstBeing, EA_SPRITE_SHIELD, shield);
    }
    else if (dstBeing->getType() == ActorSprite::NPC)
    {
        switch (gender)
        {
            case 2:
                dstBeing->setGender(GENDER_FEMALE);
                break;
            case 3:
                dstBeing->setGender(GENDER_MALE);
                break;
            case 4:
                dstBeing->setGender(GENDER_OTHER);
                break;
            default:
                dstBeing->setGender(GENDER_UNSPECIFIED);
                break;
        }
    }

    if (!visible)
    {
        uint16_t srcX, srcY, dstX, dstY;
        msg.readCoordinatePair(srcX, srcY, dstX, dstY);
        dstBeing->setAction(Being::STAND);
        dstBeing->setTileCoords(srcX, srcY);
        dstBeing->setDestination(dstX, dstY);
    }
    else
    {
        uint8_t dir;
        uint16_t x, y;
        msg.readCoordinates(x, y, dir);
        dstBeing->setTileCoords(x, y);

        if (job == 45 && socialWindow && outfitWindow)
        {
            int num = socialWindow->getPortalIndex(x, y);
            if (num >= 0)
            {
                dstBeing->setName(keyboard.getKeyShortString(
                    outfitWindow->keyName(num)));
            }
            else
            {
                dstBeing->setName("");
            }
        }

        dstBeing->setDirection(dir);
    }

    msg.readInt8();   // unknown
    msg.readInt8();   // unknown
//            msg.readInt8();   // unknown / sit
    msg.readInt16();

    dstBeing->setStunMode(stunMode);
    dstBeing->setStatusEffectBlock(0, static_cast<uint16_t>(
        (statusEffects >> 16) & 0xffff));
    dstBeing->setStatusEffectBlock(16, statusEffects & 0xffff);

}

void BeingHandler::processBeingMove2(Net::MessageIn &msg)
{
    if (!actorSpriteManager)
        return;

    /*
      * A simplified movement packet, used by the
      * later versions of eAthena for both mobs and
      * players
      */
    Being *dstBeing = actorSpriteManager->findBeing(msg.readInt32());

    /*
      * This packet doesn't have enough info to actually
      * create a new being, so if the being isn't found,
      * we'll just pretend the packet didn't happen
      */

    if (!dstBeing)
        return;

    uint16_t srcX, srcY, dstX, dstY;
    msg.readCoordinatePair(srcX, srcY, dstX, dstY);
    msg.readInt32();  // Server tick

    dstBeing->setAction(Being::STAND);
    dstBeing->setTileCoords(srcX, srcY);
    dstBeing->setDestination(dstX, dstY);
    if (dstBeing->getType() == Being::PLAYER)
        dstBeing->setMoveTime();
}

void BeingHandler::processBeingSpawn(Net::MessageIn &msg)
{
    // skipping this packet
    mSpawnId = msg.readInt32();    // id
    msg.readInt16();    // speed
    msg.readInt16();  // opt1
    msg.readInt16();  // opt2
    msg.readInt16();  // option
    msg.readInt16();    // disguise
}

void BeingHandler::processBeingRemove(Net::MessageIn &msg)
{
    if (!actorSpriteManager || !player_node)
        return;

    // A being should be removed or has died

    int id = msg.readInt32();
    Being *dstBeing = actorSpriteManager->findBeing(id);
    if (!dstBeing)
        return;

    player_node->followMoveTo(dstBeing, player_node->getNextDestX(),
        player_node->getNextDestY());

    // If this is player's current target, clear it.
    if (dstBeing == player_node->getTarget())
        player_node->stopAttack(true);

    if (msg.readInt8() == 1)
    {
        dstBeing->setAction(Being::DEAD);
        if (dstBeing->getName() == "Jack O" && killStats)
            killStats->jackoDead(id);
    }
    else
    {
        if (dstBeing->getType() == Being::PLAYER)
        {
            if (botCheckerWindow)
                botCheckerWindow->updateList();
            if (socialWindow)
                socialWindow->updateActiveList();
        }
        actorSpriteManager->destroy(dstBeing);
    }
}

void BeingHandler::processBeingResurrect(Net::MessageIn &msg)
{
    if (!actorSpriteManager || !player_node)
        return;

    // A being changed mortality status

    int id = msg.readInt32();
    Being *dstBeing = actorSpriteManager->findBeing(id);
    if (!dstBeing)
        return;

    // If this is player's current target, clear it.
    if (dstBeing == player_node->getTarget())
        player_node->stopAttack();

    if (msg.readInt8() == 1)
        dstBeing->setAction(Being::STAND);
}


void BeingHandler::processSkillDamage(Net::MessageIn &msg)
{
    if (!actorSpriteManager)
        return;

    Being *srcBeing;
    Being *dstBeing;
    int param1;

    int id = msg.readInt16(); // Skill Id
    srcBeing = actorSpriteManager->findBeing(msg.readInt32());
    dstBeing = actorSpriteManager->findBeing(msg.readInt32());
    msg.readInt32(); // Server tick
    msg.readInt32(); // src speed
    msg.readInt32(); // dst speed
    param1 = msg.readInt32(); // Damage
    msg.readInt16(); // Skill level
    msg.readInt16(); // Div
    msg.readInt8(); // Skill hit/type (?)
    if (dstBeing)
    {
//                if (dstSpeed)
//                    dstBeing->setAttackDelay(dstSpeed);
        dstBeing->takeDamage(srcBeing, param1, Being::SKILL, id);
    }
    if (srcBeing)
    {
//                if (srcSpeed)
//                    srcBeing->setAttackDelay(srcSpeed);
//        srcBeing->handleAttack(dstBeing, param1, Being::HIT);
        srcBeing->handleSkill(dstBeing, param1, id);
    }
}

void BeingHandler::processBeingAction(Net::MessageIn &msg)
{
    if (!actorSpriteManager)
        return;

    Being *srcBeing = actorSpriteManager->findBeing(msg.readInt32());
    Being *dstBeing = actorSpriteManager->findBeing(msg.readInt32());

    msg.readInt32();   // server tick
    int srcSpeed = msg.readInt32();   // src speed
    msg.readInt32();   // dst speed
    int param1 = msg.readInt16();
    msg.readInt16();  // param 2
    int type = msg.readInt8();
    msg.readInt16();  // param 3

    switch (type)
    {
        case Being::HIT: // Damage
        case Being::CRITICAL: // Critical Damage
        case Being::MULTI: // Critical Damage
        case Being::REFLECT: // Reflected Damage
        case Being::FLEE: // Lucky Dodge
            if (dstBeing)
            {
//                        if (dstSpeed)
//                            dstBeing->setAttackDelay(dstSpeed);
                dstBeing->takeDamage(srcBeing, param1,
                    static_cast<Being::AttackType>(type));
            }
            if (srcBeing)
            {
                if (srcSpeed && srcBeing->getType() == Being::PLAYER)
                    srcBeing->setAttackDelay(srcSpeed);
                srcBeing->handleAttack(dstBeing, param1,
                    static_cast<Being::AttackType>(type));
                if (srcBeing->getType() == Being::PLAYER)
                    srcBeing->setAttackTime();
            }
            break;

        case 0x01: // dead
            break;
            // tmw server can send here garbage?
//            if (srcBeing)
//                srcBeing->setAction(Being::DEAD);

        case 0x02: // Sit
            if (srcBeing)
            {
                srcBeing->setAction(Being::SIT);
                if (srcBeing->getType() == Being::PLAYER)
                {
                    srcBeing->setMoveTime();
                    if (player_node)
                        player_node->imitateAction(srcBeing, Being::SIT);
                }
            }
            break;

        case 0x03: // Stand up
            if (srcBeing)
            {
                srcBeing->setAction(Being::STAND);
                if (srcBeing->getType() == Being::PLAYER)
                {
                    srcBeing->setMoveTime();
                    if (player_node)
                        player_node->imitateAction(srcBeing, Being::STAND);
                }
            }
            break;
        default:
            logger->log("QQQ1 SMSG_BEING_ACTION:");
            if (srcBeing)
                logger->log("srcBeing:" + toString(srcBeing->getId()));
            if (dstBeing)
                logger->log("dstBeing:" + toString(dstBeing->getId()));
            logger->log("type: " + toString(type));
            break;
    }
}

void BeingHandler::processBeingSelfEffect(Net::MessageIn &msg)
{
    if (!effectManager || !actorSpriteManager)
        return;

    int id;

    id = static_cast<uint32_t>(msg.readInt32());
    Being* being = actorSpriteManager->findBeing(id);
    if (!being)
        return;

    int effectType = msg.readInt32();

    effectManager->trigger(effectType, being);

    //+++ need dehard code effectType == 3
    if (being && effectType == 3
        && being->getType() == Being::PLAYER
        && socialWindow)
    {   //reset received damage
        socialWindow->resetDamage(being->getName());
    }
}

void BeingHandler::processBeingEmotion(Net::MessageIn &msg)
{
    if (!player_node || !actorSpriteManager)
        return;

    Being *dstBeing;

    if (!(dstBeing = actorSpriteManager->findBeing(msg.readInt32())))
        return;

    if (player_relations.hasPermission(dstBeing, PlayerRelation::EMOTE))
    {
        unsigned char emote = msg.readInt8();
        if (emote)
        {
            dstBeing->setEmote(emote, EMOTION_TIME);
            player_node->imitateEmote(dstBeing, emote);
        }
    }
    if (dstBeing->getType() == Being::PLAYER)
        dstBeing->setOtherTime();
}

void BeingHandler::processNameResponse(Net::MessageIn &msg)
{
    if (!player_node || !actorSpriteManager)
        return;

    Being *dstBeing;

    int beingId = msg.readInt32();
    if ((dstBeing = actorSpriteManager->findBeing(beingId)))
    {
        if (beingId == player_node->getId())
        {
            player_node->pingResponse();
        }
        else
        {
            dstBeing->setName(msg.readString(24));
            dstBeing->updateGuild();
            dstBeing->addToCache();

            if (dstBeing->getType() == Being::PLAYER)
                dstBeing->updateColors();

            if (player_node)
            {
                Party *party = player_node->getParty();
                if (party && party->isMember(dstBeing->getId()))
                {
                    PartyMember *member = party->getMember(dstBeing->getId());

                    if (member)
                        member->setName(dstBeing->getName());
                }
                player_node->checkNewName(dstBeing);
            }
        }
    }
}

void BeingHandler::processIpResponse(Net::MessageIn &msg)
{
    if (!actorSpriteManager)
        return;

    Being *dstBeing;

    if ((dstBeing = actorSpriteManager->findBeing(
        msg.readInt32())))
    {
        dstBeing->setIp(ipToString(msg.readInt32()));
    }
}

void BeingHandler::processPlayerGuilPartyInfo(Net::MessageIn &msg)
{
    if (!actorSpriteManager)
        return;

    Being *dstBeing;

    if ((dstBeing = actorSpriteManager->findBeing(msg.readInt32())))
    {
        dstBeing->setPartyName(msg.readString(24));
        if (!guildManager || !GuildManager::getEnableGuildBot())
            dstBeing->setGuildName(msg.readString(24));
        dstBeing->setGuildPos(msg.readString(24));
        dstBeing->addToCache();
        msg.readString(24); // Discard this
    }
}

void BeingHandler::processBeingChangeDirection(Net::MessageIn &msg)
{
    if (!actorSpriteManager)
        return;

    Being *dstBeing;

    if (!(dstBeing = actorSpriteManager->findBeing(msg.readInt32())))
        return;

    msg.readInt16(); // unused

    unsigned char dir = msg.readInt8();
    dstBeing->setDirection(dir);
    if (player_node)
        player_node->imitateDirection(dstBeing, dir);
}

void BeingHandler::processPlayerStop(Net::MessageIn &msg)
{
    if (!actorSpriteManager || !player_node)
        return;

    /*
      *  Instruction from server to stop walking at x, y.
      *
      *  Some people like having this enabled.  Others absolutely
      *  despise it.  So I'm setting to so that it only affects the
      *  local player if the person has set a key "EnableSync" to "1"
      *  in their config.xml file.
      *
      *  This packet will be honored for all other beings, regardless
      *  of the config setting.
      */

    int id = msg.readInt32();

    if (mSync || id != player_node->getId())
    {
        Being *dstBeing = actorSpriteManager->findBeing(id);
        if (dstBeing)
        {
            uint16_t x, y;
            x = msg.readInt16();
            y = msg.readInt16();
            dstBeing->setTileCoords(x, y);
            if (dstBeing->getCurrentAction() == Being::MOVE)
                dstBeing->setAction(Being::STAND);
        }
    }
}

void BeingHandler::processPlayerMoveToAttack(Net::MessageIn &msg A_UNUSED)
{
    /*
      * This is an *advisory* message, telling the client that
      * it needs to move the character before attacking
      * a target (out of range, obstruction in line of fire).
      * We can safely ignore this...
      */
    if (player_node)
        player_node->fixAttackTarget();
}

void BeingHandler::processPlaterStatusChange(Net::MessageIn &msg)
{
    if (!actorSpriteManager)
        return;

    // Change in players' flags

    int id = msg.readInt32();
    Being *dstBeing = actorSpriteManager->findBeing(id);
    if (!dstBeing)
        return;

    uint16_t stunMode = msg.readInt16();
    uint32_t statusEffects = msg.readInt16();
    statusEffects |= (static_cast<uint32_t>(msg.readInt16())) << 16;
    msg.readInt8(); // Unused?

    dstBeing->setStunMode(stunMode);
    dstBeing->setStatusEffectBlock(0, static_cast<uint16_t>(
        (statusEffects >> 16) & 0xffff));
    dstBeing->setStatusEffectBlock(16, statusEffects & 0xffff);
}

void BeingHandler::processBeingStatusChange(Net::MessageIn &msg)
{
    if (!actorSpriteManager)
        return;

    // Status change
    uint16_t status = msg.readInt16();
    int id = msg.readInt32();
    int flag = msg.readInt8(); // 0: stop, 1: start

    Being *dstBeing = actorSpriteManager->findBeing(id);
    if (dstBeing)
        dstBeing->setStatusEffect(status, flag);
}

void BeingHandler::processSkilCasting(Net::MessageIn &msg)
{
    msg.readInt32();    // src id
    msg.readInt32();    // dst id
    msg.readInt16();    // dst x
    msg.readInt16();    // dst y
    msg.readInt16();    // skill num
    msg.readInt32();    // skill get pl
    msg.readInt32();    // cast time
}

void BeingHandler::processSkillNoDamage(Net::MessageIn &msg)
{
    msg.readInt16();    // skill id
    msg.readInt16();    // heal
    msg.readInt32();    // dst id
    msg.readInt32();    // src id
    msg.readInt8();     // fail
}

void BeingHandler::processPvpMapMode(Net::MessageIn &msg)
{
    Game *game = Game::instance();
    if (!game)
        return;

    Map *map = game->getCurrentMap();
    if (map)
        map->setPvpMode(msg.readInt16());
}

void BeingHandler::processPvpSet(Net::MessageIn &msg)
{
    int id = msg.readInt32();    // id
    int rank = msg.readInt32();  // rank
    msg.readInt32();             // num
    if (actorSpriteManager)
    {
        Being *dstBeing = actorSpriteManager->findBeing(id);
        if (dstBeing)
            dstBeing->setPvpRank(rank);
    }
}

} // namespace Ea
