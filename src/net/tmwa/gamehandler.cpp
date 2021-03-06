/*
 *  The ManaPlus Client
 *  Copyright (C) 2009  The Mana World Development Team
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

#include "net/tmwa/gamehandler.h"

#include "client.h"
#include "depricatedevent.h"
#include "game.h"
#include "localplayer.h"
#include "logger.h"

#include "net/messagein.h"
#include "net/messageout.h"

#include "net/tmwa/loginhandler.h"
#include "net/tmwa/network.h"
#include "net/tmwa/protocol.h"

#include "debug.h"

extern Net::GameHandler *gameHandler;

namespace TmwAthena
{

extern ServerInfo mapServer;

GameHandler::GameHandler()
{
    static const uint16_t _messages[] =
    {
        SMSG_MAP_LOGIN_SUCCESS,
        SMSG_SERVER_PING,
        SMSG_WHO_ANSWER,
        SMSG_CHAR_SWITCH_RESPONSE,
        SMSG_MAP_QUIT_RESPONSE,
        0
    };
    handledMessages = _messages;
    gameHandler = this;
}

void GameHandler::handleMessage(Net::MessageIn &msg)
{
    switch (msg.getId())
    {
        case SMSG_MAP_LOGIN_SUCCESS:
            processMapLogin(msg);
            break;

        case SMSG_SERVER_PING:
            // We ignore this for now
            // int tick = msg.readInt32()
            break;

        case SMSG_WHO_ANSWER:
            processWhoAnswer(msg);
            break;

        case SMSG_CHAR_SWITCH_RESPONSE:
            processCharSwitchResponse(msg);
            break;

        case SMSG_MAP_QUIT_RESPONSE:
            processMapQuitResponse(msg);
            break;

        default:
            break;
    }
}

void GameHandler::mapLoadedEvent()
{
    MessageOut outMsg(CMSG_MAP_LOADED);
}

void GameHandler::connect()
{
    if (!mNetwork)
        return;

    mNetwork->connect(mapServer);

    const Token &token =
            static_cast<LoginHandler*>(Net::getLoginHandler())->getToken();


    if (Client::getState() == STATE_CONNECT_GAME)
    {
        // Change the player's ID to the account ID to match what eAthena uses
        if (player_node)
        {
            mCharID = player_node->getId();
            player_node->setId(token.account_ID);
        }
        else
        {
            mCharID = 0;
        }
    }

    // Send login infos
    MessageOut outMsg(CMSG_MAP_SERVER_CONNECT);
    outMsg.writeInt32(token.account_ID);
    outMsg.writeInt32(mCharID);
    outMsg.writeInt32(token.session_ID1);
    outMsg.writeInt32(token.session_ID2);
    outMsg.writeInt8(Being::genderToInt(token.sex));

/*
    if (player_node)
    {
        // Change the player's ID to the account ID to match what eAthena uses
        player_node->setId(token.account_ID);
    }
*/
    // We get 4 useless bytes before the real answer comes in (what are these?)
    mNetwork->skip(4);
}

bool GameHandler::isConnected()
{
    if (!mNetwork)
        return false;
    return mNetwork->isConnected();
}

void GameHandler::disconnect()
{
    if (mNetwork)
        mNetwork->disconnect();
}

void GameHandler::quit()
{
    MessageOut outMsg(CMSG_CLIENT_QUIT);
}

void GameHandler::ping(int tick)
{
    MessageOut msg(CMSG_CLIENT_PING);
    msg.writeInt32(tick);
}

void GameHandler::disconnect2()
{
    MessageOut outMsg(CMSG_CLIENT_DISCONNECT);
}

} // namespace TmwAthena
