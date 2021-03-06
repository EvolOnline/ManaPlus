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

#ifndef EA_GUILDTAB_H
#define EA_GUILDTAB_H

#include "gui/widgets/chattab.h"

namespace Ea
{

/**
 * A tab for a guild chat channel.
 */
class GuildTab : public ChatTab
{
    public:
        GuildTab();

        ~GuildTab();

        bool handleCommand(const std::string &type, const std::string &args);

        void showHelp();

        void saveToLogFile(std::string &msg);

        int getType() const
        { return ChatTab::TAB_GUILD; }

        void playNewMessageSound();

    protected:
        void handleInput(const std::string &msg);

        void getAutoCompleteList(StringVect &names) const;
};

} // namespace Ea

#endif // EA_GUILDTAB_H
