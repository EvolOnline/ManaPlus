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

#ifndef NPCDIALOG_H
#define NPCDIALOG_H

#include "configlistener.h"

#include "gui/widgets/window.h"

#include "utils/stringvector.h"

#include <guichan/actionlistener.hpp>
#include <guichan/listmodel.hpp>

#include <list>

class Being;
class BrowserBox;
class ItemLinkHandler;
class IntTextField;
class ListBox;
class PlayerBox;
class TextBox;
class TextField;
class Button;

namespace gcn
{
    class ScrollArea;
}

/**
 * The npc dialog.
 *
 * \ingroup Interface
 */
class NpcDialog : public Window, public gcn::ActionListener,
                  public gcn::ListModel, public ConfigListener
{
    public:
        /**
         * Constructor.
         *
         * @see Window::Window
         */
        NpcDialog(int npcId);

        ~NpcDialog();

        /**
         * Called when receiving actions from the widgets.
         */
        void action(const gcn::ActionEvent &event);

        /**
        * Sets the text shows in the dialog.
        *
        * @param string The new text.
        */
//        void setText(const std::string &string);

        /**
         * Adds the text to the text shows in the dialog. Also adds a newline
         * to the end.
         *
         * @param string The text to add.
         */
        void addText(const std::string &string, bool save = true);

        /**
         * When called, the widget will show a "Next" button.
         */
        void showNextButton();

        /**
         * When called, the widget will show a "Close" button and will close
         * the dialog when clicked.
         */
        void showCloseButton();

        /**
         * Notifies the server that client has performed a next action.
         */
        void nextDialog();

        /**
         * Notifies the server that the client has performed a close action.
         */
        void closeDialog();

        /**
         * Returns the number of items in the choices list.
         */
        int getNumberOfElements();

        /**
         * Returns the name of item number i of the choices list.
         */
        std::string getElementAt(int i);

        /**
         * Makes this dialog request a choice selection from the user.
         */
        void choiceRequest();

        /**
         * Adds a choice to the list box.
         */
        void addChoice(const std::string &);

        /**
          * Fills the options list for an NPC dialog.
          *
          * @param itemString A string with the options separated with colons.
          */
        void parseListItems(const std::string &itemString);

        /**
         * Requests a text string from the user.
         */
        void textRequest(const std::string &defaultText = "");

        bool isInputFocused() const;

        bool isTextInputFocused() const;

        static bool isAnyInputFocused();

        /**
         * Requests a interger from the user.
         */
        void integerRequest(int defaultValue = 0, int min = 0,
                            int max = 2147483647);

        void move(int amount);

        void setVisible(bool visible);

        void optionChanged(const std::string &name);

        /**
         * Returns true if any instances exist.
         */
        static bool isActive()
        { return !instances.empty(); }

        /**
         * Returns the first active instance. Useful for pushing user
         * interaction.
         */
        static NpcDialog *getActive();

        /**
         * Closes all instances.
         */
        static void closeAll();

        /**
         * Closes all instances and destroy also net handler dialogs.
         */
        static void destroyAll();

        void saveCamera();

        void restoreCamera();

        void refocus();

        void showAvatar(int avatarId);

        void setAvatarDirection(uint8_t direction);

        void setAvatarAction(int actionId);

        void logic();

    private:
        typedef std::list<NpcDialog*> DialogList;
        static DialogList instances;

        void buildLayout();

        void placeNormalControls();

        void placeMenuControls();

        void placeTextInputControls();

        void placeIntInputControls();

        int mNpcId;
        bool mLogInteraction;

        int mDefaultInt;
        std::string mDefaultString;

        // Used for the main input area
        gcn::ScrollArea *mScrollArea;
        BrowserBox *mTextBox;
        //TextBox *mTextBox;
        std::string mText;
        std::string mNewText;

        // Used for choice input
        ListBox *mItemList;
        gcn::ScrollArea *mListScrollArea;
        StringVect mItems;
        ItemLinkHandler *mItemLinkHandler;

        // Used for string and integer input
        TextField *mTextField;
        IntTextField *mIntField;
        Button *mPlusButton;
        Button *mMinusButton;

        Button *mClearButton;

        // Used for the button
        Button *mButton;

        // Will reset the text and integer input to the provided default
        Button *mResetButton;

        enum NpcInputState
        {
            NPC_INPUT_NONE = 0,
            NPC_INPUT_LIST,
            NPC_INPUT_STRING,
            NPC_INPUT_INTEGER
        };

        enum NpcActionState
        {
            NPC_ACTION_WAIT = 0,
            NPC_ACTION_NEXT,
            NPC_ACTION_INPUT,
            NPC_ACTION_CLOSE
        };

        NpcInputState mInputState;
        NpcActionState mActionState;
        int mLastNextTime;
        int mCameraMode;
        int mCameraX;
        int mCameraY;
        PlayerBox *mPlayerBox;
        Being *mAvatarBeing;
        bool mShowAvatar;
};

#endif // NPCDIALOG_H
