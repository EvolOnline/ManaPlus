/*
 *  The ManaPlus Client
 *  Copyright (C) 2009  The Mana World Development Team
 *  Copyright (C) 2009-2010  Andrei Karas
 *  Copyright (C) 2011-2012  The ManaPlus developers
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

#ifndef SPELLMANAGER_H
#define SPELLMANAGER_H

#include <string>
#include <vector>
#include "textcommand.h"

#include "being.h"

#define SPELL_MIN_ID 100000
#define SPELL_SHORTCUT_ITEMS 49
#define SPELL_SHORTCUT_TABS 5

class SpellManager
{
    public:
        SpellManager();

        ~SpellManager();

        TextCommand *getSpell(int spellId);

        TextCommand* getSpellByItem(int itemId);

        bool addSpell(TextCommand *spell);

        TextCommand *createNewSpell();

        std::vector<TextCommand*> getAll();

        void useItem(int itemId);

        void invoke(int spellId);

        void load(bool oldConfig = false);

        void save();

        std::string autoComplete(std::string partName);

    private:
        void fillSpells();

        void invokeSpell(TextCommand* spell, Being* target) const;

        void invokeSpell(TextCommand* spell) const;

        std::string parseCommand(std::string command, Being* target) const;

        std::map<unsigned int, TextCommand*> mSpells;
        std::vector<TextCommand*> mSpellsVector;
};

extern SpellManager *spellManager;

#endif // SPELLMANAGER_H
