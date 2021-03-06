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

#include "inputmanager.h"

#include "configuration.h"
#include "game.h"
#include "joystick.h"
#include "keyboardconfig.h"
#include "keyboarddata.h"
#include "localplayer.h"

#include "gui/chatwindow.h"
#include "gui/gui.h"
#include "gui/inventorywindow.h"
#include "gui/npcdialog.h"
#include "gui/npcpostdialog.h"
#include "gui/setup.h"
#include "gui/setup_input.h"
#include "gui/textdialog.h"
#include "gui/tradewindow.h"

#include <guichan/exception.hpp>
#include <guichan/focushandler.hpp>

#include <algorithm>

#include "debug.h"

InputManager inputManager;

extern QuitDialog *quitDialog;

class KeyFunctor
{
    public:
        bool operator() (int key1, int key2)
        {
            return keys[key1].priority >= keys[key2].priority;
        }

        const KeyData *keys;

} keySorter;


InputManager::InputManager() :
    mSetupInput(nullptr),
    mNewKeyIndex(Input::KEY_NO_VALUE),
    mMask(1)
{
}

void InputManager::init()
{
    for (int i = 0; i < Input::KEY_TOTAL; i++)
    {
        for (int f = 0; f < KeyFunctionSize; f ++)
        {
            mKey[i].values[f].type = INPUT_UNKNOWN;
            mKey[i].values[f].value = -1;
        }
    }

    mNewKeyIndex = Input::KEY_NO_VALUE;

    resetKeys();
    retrieve();
    update();
}

void InputManager::update()
{
    keyboard.update();
    if (joystick)
        joystick->update();
}

void InputManager::retrieve()
{
    for (int i = 0; i < Input::KEY_TOTAL; i++)
    {
        if (*keyData[i].configField)
        {
            KeyFunction &kf = mKey[i];
            std::string keyStr = config.getValue(keyData[i].configField, "");
            if (keyStr.empty())
                continue;

            StringVect keys;
            splitToStringVector(keys, keyStr, ',');
            int i2 = 0;
            for (StringVectCIter it = keys.begin(), it_end = keys.end();
                 it != it_end && i2 < KeyFunctionSize; ++ it)
            {
                std::string keyStr2 = *it;
                if (keyStr.size() < 2)
                    continue;
                int type = INPUT_KEYBOARD;
                if ((keyStr2[0] < '0' || keyStr2[0] > '9')
                    && keyStr2[0] != '-')
                {
                    switch (keyStr2[0])
                    {
                        case 'm':
                            type = INPUT_MOUSE;
                            break;
                        case 'j':
                            type = INPUT_JOYSTICK;
                            break;
                        default:
                            break;
                    }
                    keyStr2 = keyStr2.substr(1);
                }
                int key = atoi(keyStr2.c_str());
                if (key >= -255 && key < SDLK_LAST)
                {
                    kf.values[i2] = KeyItem(type, key);
                    i2 ++;
                }
            }
        }
    }
}

void InputManager::store()
{
    for (int i = 0; i < Input::KEY_TOTAL; i++)
    {
        if (*keyData[i].configField)
        {
            std::string keyStr;

            for (size_t i2 = 0; i2 < KeyFunctionSize; i2 ++)
            {
                const KeyItem &key = mKey[i].values[i2];
                if (key.type != INPUT_UNKNOWN)
                {
                    std::string tmp = "k";
                    switch (key.type)
                    {
                        case INPUT_MOUSE:
                            tmp = "m";
                            break;
                        case INPUT_JOYSTICK:
                            tmp = "j";
                            break;
                        default:
                            break;
                    }
                    if (key.value != -1)
                    {
                        if (keyStr.empty())
                        {
                            keyStr = tmp + toString(key.value);
                        }
                        else
                        {
                            keyStr += strprintf(",%s%d",
                                tmp.c_str(), key.value);
                        }
                    }
                }
            }
            if (keyStr.empty())
                keyStr = "-1";

            config.setValue(keyData[i].configField, keyStr);
        }
    }
}

void InputManager::resetKeys()
{
    for (int i = 0; i < Input::KEY_TOTAL; i++)
    {
        KeyFunction &key = mKey[i];
        for (size_t i2 = 1; i2 < KeyFunctionSize; i2 ++)
        {
            key.values[i2].type = INPUT_UNKNOWN;
            key.values[i2].value = -1;
        }
        key.values[0].type = keyData[i].defaultType1;
        key.values[0].value = keyData[i].defaultValue1;
        key.values[1].type = keyData[i].defaultType2;
        key.values[1].value = keyData[i].defaultValue2;
    }
}

void InputManager::makeDefault(int i)
{
    if (i >= 0 && i < Input::KEY_TOTAL)
    {
        KeyFunction &key = mKey[i];
        for (size_t i2 = 1; i2 < KeyFunctionSize; i2 ++)
        {
            key.values[i2].type = INPUT_UNKNOWN;
            key.values[i2].value = -1;
        }
        key.values[0].type = keyData[i].defaultType1;
        key.values[0].value = keyData[i].defaultValue1;
        key.values[1].type = keyData[i].defaultType2;
        key.values[1].value = keyData[i].defaultValue2;

        update();
    }
}

bool InputManager::hasConflicts(int &key1, int &key2)
{
    size_t i;
    size_t j;
    /**
     * No need to parse the square matrix: only check one triangle
     * that's enough to detect conflicts
     */
    for (i = 0; i < Input::KEY_TOTAL; i++)
    {
        if (!*keyData[i].configField)
            continue;

        for (size_t i2 = 0; i2 < KeyFunctionSize; i2 ++)
        {
            if (mKey[i].values[i2].value == Input::KEY_NO_VALUE)
                continue;

            for (j = i, j++; j < Input::KEY_TOTAL; j++)
            {

                if ((keyData[i].grp & keyData[j].grp) == 0 ||
                    !*keyData[i].configField)
                {
                    continue;
                }

                for (size_t j2 = 0; j2 < KeyFunctionSize; j2 ++)
                {
                    // Allow for item shortcut and emote keys to overlap
                    // as well as emote and ignore keys, but no other keys
                    if (mKey[j].values[j2].type != INPUT_UNKNOWN
                        && mKey[i].values[i2].value == mKey[j].values[j2].value
                        && mKey[i].values[i2].type == mKey[j].values[j2].type)
                    {
                        key1 = i;
                        key2 = j;
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

void InputManager::callbackNewKey()
{
    mSetupInput->newKeyCallback(mNewKeyIndex);
}

bool InputManager::isActionActive(int index) const
{
    if (keyboard.isActionActive(index))
        return true;
    if (joystick)
        return joystick->isActionActive(index);
    return false;
}

KeyFunction &InputManager::getKey(int index)
{
    if (index < 0 || index >= Input::KEY_TOTAL)
        index = 0;
    return mKey[index];
}

std::string InputManager::getKeyStringLong(int index) const
{
    std::string keyStr;

    for (size_t i = 0; i < KeyFunctionSize; i ++)
    {
        const KeyItem &key = mKey[index].values[i];
        std::string str;
        if (key.type == INPUT_KEYBOARD)
        {
            if (key.value >= 0)
                str = keyboard.getKeyName(key.value);
            else if (key.value < -1)
                str = strprintf(_("key_%d"), -key.value);
        }
        else if (key.type == INPUT_JOYSTICK)
        {
            // TRANSLATORS: this is long joystick button name
            str = strprintf(_("JButton%d"), key.value + 1);
        }
        if (!str.empty())
        {
            if (keyStr.empty())
                keyStr = str;
            else
                keyStr += std::string(" ") + str;
        }
    }

    if (keyStr.empty())
        return _("unknown key");
    return keyStr;
}

std::string InputManager::getKeyValueString(int index) const
{
    std::string keyStr;

    for (size_t i = 0; i < KeyFunctionSize; i ++)
    {
        const KeyItem &key = mKey[index].values[i];
        std::string str;
        if (key.type == INPUT_KEYBOARD)
        {
            if (key.value >= 0)
            {
                str = keyboard.getKeyShortString(
                    keyboard.getKeyName(key.value));
            }
            else if (key.value < -1)
            {
                str = strprintf(_("key_%d"), -key.value);
            }
        }
        else if (key.type == INPUT_JOYSTICK)
        {
            // TRANSLATORS: this is short joystick button name
            str = strprintf(_("JB%d"), key.value + 1);
        }
        if (!str.empty())
        {
            if (keyStr.empty())
                keyStr = str;
            else
                keyStr += std::string(" ") + str;
        }
    }

    if (keyStr.empty())
        return _("u key");
    return keyStr;
}

void InputManager::addActionKey(int action, int type, int val)
{
    if (action < 0 || action >= Input::KEY_TOTAL)
        return;

    int idx = -1;
    KeyFunction &key = mKey[action];
    for (size_t i = 0; i < KeyFunctionSize; i ++)
    {
        if (key.values[i].type == INPUT_UNKNOWN
            || (key.values[i].type == type
            && key.values[i].value == val))
        {
            idx = i;
            break;
        }
    }
    if (idx == -1)
    {
        for (size_t i = 1; i < KeyFunctionSize; i ++)
        {
            key.values[i - 1].type = key.values[i].type;
            key.values[i - 1].value = key.values[i].value;
        }
        idx = KeyFunctionSize - 1;
    }

    key.values[idx] = KeyItem(type, val);
}

void InputManager::setNewKey(const SDL_Event &event, int type)
{
    int val = -1;
    if (type == INPUT_KEYBOARD)
        val = keyboard.getKeyValueFromEvent(event);
    else if (type == INPUT_JOYSTICK && joystick)
        val = joystick->getButtonFromEvent(event);

    if (val != -1)
    {
        addActionKey(mNewKeyIndex, type, val);
        update();
    }
}

void InputManager::unassignKey()
{
    KeyFunction &key = mKey[mNewKeyIndex];
    for (size_t i = 0; i < KeyFunctionSize; i ++)
    {
        key.values[i].type = INPUT_UNKNOWN;
        key.values[i].value = -1;
    }
    update();
}

bool InputManager::handleAssignKey(const SDL_Event &event, int type)
{
    if (setupWindow && setupWindow->isVisible() &&
        getNewKeyIndex() > Input::KEY_NO_VALUE)
    {
        setNewKey(event, type);
        callbackNewKey();
        setNewKeyIndex(Input::KEY_NO_VALUE);
        return true;
    }
    return false;
}

bool InputManager::handleEvent(const SDL_Event &event)
{
    switch (event.type)
    {
        case SDL_KEYDOWN:
        {
            updateConditionMask();
            if (handleAssignKey(event, INPUT_KEYBOARD))
                return true;

            keyboard.handleActicateKey(event);
            // send straight to gui for certain windows
            if (quitDialog || TextDialog::isActive() ||
                NpcPostDialog::isActive())
            {
                try
                {
                    if (guiInput)
                        guiInput->pushInput(event);
                    if (gui)
                        gui->handleInput();
                }
                catch (const gcn::Exception &e)
                {
                    const char* err = e.getMessage().c_str();
                    logger->log("Warning: guichan input exception: %s", err);
                }
                return true;
            }
            break;
        }
        case SDL_KEYUP:
        {
            updateConditionMask();
            keyboard.handleDeActicateKey(event);
            break;
        }
        case SDL_JOYBUTTONDOWN:
        {
            updateConditionMask();
//            joystick.handleActicateButton(event);
            if (handleAssignKey(event, INPUT_JOYSTICK))
                return true;
            break;
        }
        case SDL_JOYBUTTONUP:
        {
            updateConditionMask();
//            joystick.handleDeActicateButton(event);
            break;
        }
        default:
            break;
    }

    try
    {
        if (guiInput)
            guiInput->pushInput(event);
    }
    catch (const gcn::Exception &e)
    {
        const char *err = e.getMessage().c_str();
        logger->log("Warning: guichan input exception: %s", err);
    }
    if (gui)
    {
        bool res = gui->handleInput();
        if (res && event.type == SDL_KEYDOWN)
            return true;
    }

    switch (event.type)
    {
        case SDL_KEYDOWN:
            if (triggerAction(keyboard.getActionVector(event)))
                return true;
            break;

        case SDL_JOYBUTTONDOWN:
            if (joystick && joystick->validate())
            {
                if (triggerAction(joystick->getActionVector(event)))
                    return true;
            }
            break;
        default:
            break;
    }

    return false;
}

void InputManager::handleRepeat()
{
    const int time = tick_time;
    keyboard.handleRepeat(time);
    if (joystick)
        joystick->handleRepeat(time);
}

void InputManager::updateConditionMask()
{
    mMask = 1;
    if (keyboard.isEnabled())
        mMask += COND_ENABLED;
    if ((!chatWindow || !chatWindow->isInputFocused()) &&
        !NpcDialog::isAnyInputFocused() &&
        !InventoryWindow::isAnyInputFocused() &&
        (!tradeWindow || !tradeWindow->isInpupFocused()))
    {
        mMask += COND_NOINPUT;
    }

    if (!player_node || !player_node->getAway())
        mMask += COND_NOAWAY;

    if (!setupWindow || !setupWindow->isVisible())
        mMask += COND_NOSETUP;

    if (Game::instance() && Game::instance()->getValidSpeed())
        mMask += COND_VALIDSPEED;

    if (gui && !gui->getFocusHandler()->getModalFocused())
        mMask += COND_NOMODAL;

    NpcDialog *dialog = NpcDialog::getActive();
    if (!dialog || !dialog->isTextInputFocused())
        mMask += COND_NONPCINPUT;

    if (!player_node || !player_node->getDisableGameModifiers())
        mMask += COND_EMODS;

    if (!isActionActive(Input::KEY_STOP_ATTACK)
        && !isActionActive(Input::KEY_UNTARGET))
    {
        mMask += COND_NOTARGET;
    }

    if (!player_node || player_node->getFollow().empty())
        mMask += COND_NOFOLLOW;
}

bool InputManager::checkKey(const KeyData *key)
{
//    logger->log("mask=%d, condition=%d", mMask, key->condition);
    if (!key || (key->condition & mMask) != key->condition)
        return false;

    return (key->modKeyIndex == Input::KEY_NO_VALUE
        || isActionActive(key->modKeyIndex));
}

bool InputManager::invokeKey(const KeyData *key, int keyNum)
{
    if (checkKey(key))
    {
        InputEvent evt(keyNum, mMask);
        if ((*(keyData[keyNum].action))(evt))
            return true;
    }
    return false;
}

void InputManager::updateKeyActionMap(KeyToActionMap &actionMap,
                                      KeyToIdMap &idMap,
                                      KeyTimeMap &keyTimeMap,
                                      int type)
{
    actionMap.clear();
    keyTimeMap.clear();

    for (size_t i = 0; i < Input::KEY_TOTAL; i ++)
    {
        KeyFunction &key = mKey[i];
        if (keyData[i].action)
        {
            for (size_t i2 = 0; i2 < KeyFunctionSize; i2 ++)
            {
                const KeyItem &ki = key.values[i2];
                if (ki.type == type && ki.value != -1)
                    actionMap[ki.value].push_back(i);
            }
        }
        if (keyData[i].configField && (keyData[i].grp & Input::GRP_GUICHAN))
        {
            for (size_t i2 = 0; i2 < KeyFunctionSize; i2 ++)
            {
                const KeyItem &ki = key.values[i2];
                if (ki.type == type && ki.value != -1)
                    idMap[ki.value] = i;
            }
        }
        if (keyData[i].configField && (keyData[i].grp & Input::GRP_REPEAT))
        {
            for (size_t i2 = 0; i2 < KeyFunctionSize; i2 ++)
            {
                const KeyItem &ki = key.values[i2];
                if (ki.type == type && ki.value != -1)
                    keyTimeMap[ki.value] = 0;
            }
        }
    }

    keySorter.keys = &keyData[0];
    for (KeyToActionMapIter it = actionMap.begin(), it_end = actionMap.end();
         it != it_end; ++ it)
    {
        KeysVector *keys = &it->second;
        if (keys && keys->size() > 1)
            sort(keys->begin(), keys->end(), keySorter);
    }
}

bool InputManager::triggerAction(const KeysVector *ptrs)
{
    if (!ptrs)
        return false;

//    logger->log("ptrs: %d", (int)ptrs.size());

    for (KeysVectorCIter it = ptrs->begin(), it_end = ptrs->end();
         it != it_end; ++ it)
    {
        const int keyNum = *it;
        if (keyNum < 0 || keyNum >= Input::KEY_TOTAL)
            continue;

        if (invokeKey(&keyData[keyNum], keyNum))
            return true;
    }
    return false;
}

int InputManager::getKeyIndex(int value, int grp, int type) const
{
    for (size_t i = 0; i < Input::KEY_TOTAL; i++)
    {
        const KeyFunction &key = mKey[i];
        for (size_t i2 = 0; i2 < KeyFunctionSize; i2 ++)
        {
            if (value == key.values[i2].value
                && (grp & keyData[i].grp) != 0
                && key.values[i2].type == type)
            {
                return i;
            }
        }
    }
    return Input::KEY_NO_VALUE;
}

int InputManager::getActionByKey(const SDL_Event &event)
{
    // for now support only keyboard events
    if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP)
    {
        int idx = keyboard.getActionId(event);
        if (idx >= 0 && checkKey(&keyData[idx]))
            return idx;
    }
    return -1;
}
