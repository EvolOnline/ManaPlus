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

#ifndef LOGINDATA_H
#define LOGINDATA_H

#include "being.h"

#include <string>

class LoginData
{
    public:
        LoginData():
            username(""),
            password(""),
            newPassword(""),
            updateHost(""),
            updateType(0),
            email(""),
            captchaResponse(""),
            gender(GENDER_UNSPECIFIED),
            remember(false),
            registerLogin(false)
        {
            resetCharacterSlots();
        }

        enum UpdateType
        {
            Upd_Normal = 0,
            Upd_Close = 1,
            Upd_Skip = 2,
            Upd_Custom = 4
        };

        std::string username;
        std::string password;
        std::string newPassword;
        std::string updateHost;
        StringVect updateHosts;
        std::string lastLogin;
        int updateType;

        std::string email;
        std::string captchaResponse;

        Gender gender;

        bool remember;            /**< Whether to store the username. */
        bool registerLogin;       /**< Whether an account
                                       is being registered. */

        unsigned short characterSlots; /**< The number of character slots */

        void clear()
        {
            username.clear();
            password.clear();
            newPassword.clear();
            updateHost.clear();
            updateHosts.clear();
            updateType = Upd_Normal;
            email.clear();
            captchaResponse.clear();
            gender = GENDER_UNSPECIFIED;
            lastLogin.clear();
            resetCharacterSlots();
        }

        void clearUpdateHost()
        {
            updateHost.clear();
            updateHosts.clear();
        }

        /**
         * Initialize character slots to 3 for TmwAthena compatibility
         */
        void resetCharacterSlots()
        {
            characterSlots = 9; // Default value, used for TmwAthena.
        }
};

#endif // LOGINDATA_H
