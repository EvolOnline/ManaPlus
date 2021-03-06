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

#include "gui/buyselldialog.h"

#include "gui/setup.h"

#include "gui/widgets/button.h"

#include "net/buysellhandler.h"
#include "net/net.h"
#include "net/npchandler.h"

#include "utils/gettext.h"

#include "debug.h"

BuySellDialog::DialogList BuySellDialog::instances;

BuySellDialog::BuySellDialog(int npcId):
    Window(_("Shop"), false, nullptr, "buysell.xml"),
    mNpcId(npcId),
    mNick(""),
    mBuyButton(nullptr)
{
    init();
}

BuySellDialog::BuySellDialog(std::string nick):
    Window(_("Shop"), false, nullptr, "buysell.xml"),
    mNpcId(-1),
    mNick(nick),
    mBuyButton(nullptr)
{
    init();
}

void BuySellDialog::init()
{
    setWindowName("BuySell");
    //setupWindow->registerWindowForReset(this);
    setCloseButton(true);

    static const char *buttonNames[] =
    {
        N_("Buy"), N_("Sell"), N_("Cancel"), nullptr
    };
    int x = 10, y = 10;

    for (const char **curBtn = buttonNames; *curBtn; curBtn++)
    {
        Button *btn = new Button(gettext(*curBtn), *curBtn, this);
        if (!mBuyButton)
            mBuyButton = btn; // For focus request
        btn->setPosition(x, y);
        add(btn);
        x += btn->getWidth() + 10;
    }
    mBuyButton->requestFocus();

    setContentSize(x, 2 * y + mBuyButton->getHeight());

    center();
    setDefaultSize();
    loadWindowState();

    instances.push_back(this);
    setVisible(true);
}

BuySellDialog::~BuySellDialog()
{
    instances.remove(this);
}

void BuySellDialog::setVisible(bool visible)
{
    Window::setVisible(visible);

    if (visible)
    {
        if (mBuyButton)
            mBuyButton->requestFocus();
    }
    else
    {
        scheduleDelete();
    }
}

void BuySellDialog::action(const gcn::ActionEvent &event)
{
    if (event.getId() == "Buy")
    {
        if (mNpcId != -1)
            Net::getNpcHandler()->buy(mNpcId);
        else
            Net::getBuySellHandler()->requestSellList(mNick);
    }
    else if (event.getId() == "Sell")
    {
        if (mNpcId != -1)
            Net::getNpcHandler()->sell(mNpcId);
        else
            Net::getBuySellHandler()->requestBuyList(mNick);
    }

    close();
}

void BuySellDialog::closeAll()
{
    for (DialogList::const_iterator it = instances.begin(),
         it_end = instances.end(); it != it_end; ++it)
    {
        if (*it)
            (*it)->close();
    }
}
