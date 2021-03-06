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

#include "net/tmwa/chathandler.h"

#include "actorspritemanager.h"
#include "being.h"
#include "configuration.h"
#include "depricatedevent.h"
#include "game.h"
#include "localplayer.h"
#include "playerrelations.h"
#include "logger.h"

#include "gui/chatwindow.h"
#include "gui/shopwindow.h"

#include "gui/widgets/chattab.h"

#include "net/messagein.h"
#include "net/messageout.h"

#include "net/tmwa/protocol.h"

#include <string>

#include "debug.h"

extern Net::ChatHandler *chatHandler;

namespace TmwAthena
{

ChatHandler::ChatHandler()
{
    static const uint16_t _messages[] =
    {
        SMSG_BEING_CHAT,
        SMSG_PLAYER_CHAT,
        SMSG_WHISPER,
        SMSG_WHISPER_RESPONSE,
        SMSG_GM_CHAT,
        SMSG_MVP, // MVP
        SMSG_IGNORE_ALL_RESPONSE,
        0
    };
    handledMessages = _messages;
    chatHandler = this;
}

void ChatHandler::handleMessage(Net::MessageIn &msg)
{
    if (!localChatTab)
        return;

    switch (msg.getId())
    {
        case SMSG_WHISPER_RESPONSE:
            processWhisperResponse(msg);
            break;

        // Received whisper
        case SMSG_WHISPER:
            processWhisper(msg);
            break;

        // Received speech from being
        case SMSG_BEING_CHAT:
            processBeingChat(msg);
            break;

        case SMSG_PLAYER_CHAT:
        case SMSG_GM_CHAT:
            processChat(msg, msg.getId() == SMSG_PLAYER_CHAT);
            break;

        case SMSG_MVP:
            processMVP(msg);
            break;

        case SMSG_IGNORE_ALL_RESPONSE:
            processIgnoreAllResponse(msg);

        default:
            break;
    }
}

void ChatHandler::talk(const std::string &text)
{
    if (!player_node)
        return;

    std::string mes = player_node->getName() + " : " + text;

    MessageOut outMsg(CMSG_CHAT_MESSAGE);
    // Added + 1 in order to let eAthena parse admin commands correctly
    outMsg.writeInt16(static_cast<short>(mes.length() + 4 + 1));
    outMsg.writeString(mes, static_cast<int>(mes.length() + 1));
}

void ChatHandler::talkRaw(const std::string &mes)
{
    MessageOut outMsg(CMSG_CHAT_MESSAGE);
    // Added + 1 in order to let eAthena parse admin commands correctly
    outMsg.writeInt16(static_cast<short>(mes.length() + 4 + 1));
    outMsg.writeString(mes, static_cast<int>(mes.length() + 1));
}

void ChatHandler::privateMessage(const std::string &recipient,
                                 const std::string &text)
{
    MessageOut outMsg(CMSG_CHAT_WHISPER);
    outMsg.writeInt16(static_cast<short>(text.length() + 28));
    outMsg.writeString(recipient, 24);
    outMsg.writeString(text, static_cast<int>(text.length()));
    mSentWhispers.push(recipient);
}

void ChatHandler::who()
{
    MessageOut outMsg(CMSG_WHO_REQUEST);
}

void ChatHandler::sendRaw(const std::string &args)
{
    std::string line = args;
    std::string str;
    MessageOut *outMsg = nullptr;

    if (line == "")
        return;

    size_t pos = line.find(" ");
    if (pos != std::string::npos)
    {
        str = line.substr(0, pos);
        outMsg = new MessageOut(static_cast<short>(atoi(str.c_str())));
        line = line.substr(pos + 1);
        pos = line.find(" ");
    }
    else
    {
        outMsg = new MessageOut(static_cast<short>(atoi(line.c_str())));
        delete outMsg;
        return;
    }

    while (pos != std::string::npos)
    {
        str = line.substr(0, pos);
        processRaw(*outMsg, str);
        line = line.substr(pos + 1);
        pos = line.find(" ");
    }
    if (line != "")
        processRaw(*outMsg, line);
    delete outMsg;
}

void ChatHandler::processRaw(MessageOut &outMsg, std::string &line)
{
    size_t pos = line.find(":");
    if (pos == std::string::npos)
    {
        int i = atoi(line.c_str());
        if (line.length() <= 3)
            outMsg.writeInt8(static_cast<char>(i));
        else if (line.length() <= 5)
            outMsg.writeInt16(static_cast<short>(i));
        else
            outMsg.writeInt32(i);
    }
    else
    {
        std::string header = line.substr(0, pos);
        std::string data = line.substr(pos + 1);
        if (header.length() != 1)
            return;

        int i = 0;

        switch (header[0])
        {
            case '1':
            case '2':
            case '4':
                i = atoi(data.c_str());
                break;
            default:
                break;
        }

        switch (header[0])
        {
            case '1':
                outMsg.writeInt8(static_cast<char>(i));
                break;
            case '2':
                outMsg.writeInt16(static_cast<short>(i));
                break;
            case '4':
                outMsg.writeInt32(i);
                break;
            case 'c':
            {
                pos = line.find(",");
                if (pos != std::string::npos)
                {
                    unsigned short x = static_cast<unsigned short>(
                        atoi(data.substr(0, pos).c_str()));
                    data = data.substr(pos + 1);
                    pos = line.find(",");
                    if (pos == std::string::npos)
                        break;

                    unsigned short y = static_cast<unsigned short>(
                        atoi(data.substr(0, pos).c_str()));
                    int dir = atoi(data.substr(pos + 1).c_str());
                    outMsg.writeCoordinates(x, y,
                        static_cast<unsigned char>(dir));
                }
                break;
            }
            case 't':
                outMsg.writeString(data, static_cast<int>(data.length()));
                break;
            default:
                break;
        }
    }
}

void ChatHandler::ignoreAll()
{
    MessageOut outMsg(CMSG_IGNORE_ALL);
    outMsg.writeInt8(0);
}

void ChatHandler::unIgnoreAll()
{
    MessageOut outMsg(CMSG_IGNORE_ALL);
    outMsg.writeInt8(1);
}

} // namespace TmwAthena
