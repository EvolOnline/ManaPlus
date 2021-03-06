/*
 *  The ManaPlus Client
 *  Copyright (C) 2008  The Legend of Mazzeroth Development Team
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

#ifndef SPELLPOPUP_H
#define SPELLPOPUP_H

#include "gui/widgets/popup.h"

#include "textcommand.h"

#include <guichan/mouselistener.hpp>

class TextBox;

namespace gcn
{
    class Label;
}

/**
 * A popup that displays information about an item.
 */
class SpellPopup : public Popup
{
    public:
        /**
         * Constructor. Initializes the item popup.
         */
        SpellPopup();

        /**
         * Destructor. Cleans up the item popup on deletion.
         */
        ~SpellPopup();

        /**
         * Sets the info to be displayed given a particular item.
         */
        void setItem(TextCommand *spell);

        /**
         * Sets the location to display the item popup.
         */
        void view(int x, int y);

        void mouseMoved(gcn::MouseEvent &mouseEvent);

    private:
        gcn::Label *mItemName;

        gcn::Label *mItemComment;
};

#endif // SPELLPOPUP_H
