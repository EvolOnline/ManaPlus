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

#ifndef GUI_GUILD_DIALOG_H
#define GUI_GUILD_DIALOG_H

#include "gui/widgets/window.h"

#include <guichan/actionlistener.hpp>

class PasswordField;
class TextField;

namespace gcn
{
    class Button;
}

/**
* An option dialog.
 *
 * \ingroup GUI
 */
class TextDialog : public Window, public gcn::ActionListener
{
public:
    /**
     * Constructor.
     *
     * @see Window::Window
     */
    TextDialog(const std::string &title, const std::string &msg,
               Window *parent = nullptr, bool isPassword = false);

    ~TextDialog();

    /**
     * Called when receiving actions from the widgets.
     */
    void action(const gcn::ActionEvent &event);

    /**
     * Get the text in the textfield
     */
    const std::string &getText() const;

    void setText(std::string text);

    static bool isActive()
    { return instances; }

    void close();

private:
    static int instances;

    TextField *mTextField;
    PasswordField *mPasswordField;
    gcn::Button *mOkButton;
    bool mEnabledKeyboard;
};

#endif
