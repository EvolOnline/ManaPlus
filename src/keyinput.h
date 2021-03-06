/*
 *  The ManaPlus Client
 *  Copyright (C) 2012  The ManaPlus Developers
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

#ifndef KEYINPUT_H
#define KEYINPUT_H

#include <guichan/keyinput.hpp>

#include <string>

class KeyInput : public gcn::KeyInput
{
    public:
        KeyInput();

        ~KeyInput();

        void setActionId(int n)
        { mActionId = n; }

        int getActionId()
        { return mActionId; }

    protected:
        int mActionId;
};

#endif
