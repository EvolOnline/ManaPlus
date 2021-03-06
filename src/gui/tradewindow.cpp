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

#include "gui/tradewindow.h"

#include "configuration.h"
#include "inventory.h"
#include "item.h"
#include "localplayer.h"
#include "playerinfo.h"
#include "playerrelations.h"
#include "units.h"

#include "gui/gui.h"
#include "gui/inventorywindow.h"
#include "gui/itemamountwindow.h"
#include "gui/sdlfont.h"
#include "gui/setup.h"
#include "gui/theme.h"

#include "gui/widgets/button.h"
#include "gui/widgets/chattab.h"
#include "gui/widgets/itemcontainer.h"
#include "gui/widgets/label.h"
#include "gui/widgets/scrollarea.h"
#include "gui/widgets/textfield.h"
#include "gui/widgets/layout.h"

#include "net/inventoryhandler.h"
#include "net/net.h"
#include "net/tradehandler.h"

#include "utils/gettext.h"
#include "utils/stringutils.h"

#include <guichan/font.hpp>

#include <sstream>

#include "debug.h"

#define CAPTION_PROPOSE _("Propose trade")
#define CAPTION_CONFIRMED _("Confirmed. Waiting...")
#define CAPTION_ACCEPT _("Agree trade")
#define CAPTION_ACCEPTED _("Agreed. Waiting...")

TradeWindow::TradeWindow():
    Window(_("Trade: You"), false, nullptr, "trade.xml"),
    mMyInventory(new Inventory(Inventory::TRADE)),
    mPartnerInventory(new Inventory(Inventory::TRADE)),
    mStatus(PROPOSING),
    mAutoAddItem(nullptr),
    mAutoAddToNick(""),
    mGotMoney(0),
    mGotMaxMoney(0),
    mAutoMoney(0)
{
    logger->log1("TradeWindow::TradeWindow nick");

    setWindowName("Trade");
    setResizable(true);
    setCloseButton(true);
    setStickyButtonLock(true);
    setDefaultSize(386, 180, ImageRect::CENTER);
    setMinWidth(386);
    setMinHeight(180);

    if (setupWindow)
        setupWindow->registerWindowForReset(this);

    mAddButton = new Button(_("Add"), "add", this);
    mOkButton = new Button("", "", this); // Will be filled in later

    gcn::Font *fnt = mOkButton->getFont();
    int width = std::max(fnt->getWidth(CAPTION_PROPOSE),
        fnt->getWidth(CAPTION_CONFIRMED));
    width = std::max(width, fnt->getWidth(CAPTION_ACCEPT));
    width = std::max(width, fnt->getWidth(CAPTION_ACCEPTED));

    mOkButton->setWidth(8 + width);

    mMyItemContainer = new ItemContainer(mMyInventory.get());
    mMyItemContainer->addSelectionListener(this);

    ScrollArea *myScroll = new ScrollArea(mMyItemContainer);
    myScroll->setHorizontalScrollPolicy(gcn::ScrollArea::SHOW_NEVER);

    mPartnerItemContainer = new ItemContainer(mPartnerInventory.get());
    mPartnerItemContainer->addSelectionListener(this);

    ScrollArea *partnerScroll = new ScrollArea(mPartnerItemContainer);
    partnerScroll->setHorizontalScrollPolicy(gcn::ScrollArea::SHOW_NEVER);

    mMoneyLabel = new Label(strprintf(_("You get %s"), ""));
    gcn::Label *mMoneyLabel2 = new Label(_("You give:"));

    mMoneyField = new TextField;
    mMoneyField->setWidth(40);
    mMoneyChangeButton = new Button(_("Change"), "money", this);

    place(1, 0, mMoneyLabel);
    place(0, 1, myScroll).setPadding(3);
    place(1, 1, partnerScroll).setPadding(3);
    ContainerPlacer placer;
    placer = getPlacer(0, 0);
    placer(0, 0, mMoneyLabel2);
    placer(1, 0, mMoneyField, 2);
    placer(3, 0, mMoneyChangeButton).setHAlign(LayoutCell::LEFT);
    placer = getPlacer(0, 2);
    placer(0, 0, mAddButton);
    placer(1, 0, mOkButton);
    Layout &layout = getLayout();
    layout.extend(0, 2, 2, 1);
    layout.setRowHeight(1, Layout::AUTO_SET);
    layout.setRowHeight(2, 0);
    layout.setColWidth(0, Layout::AUTO_SET);
    layout.setColWidth(1, Layout::AUTO_SET);

    loadWindowState();

    reset();
}

TradeWindow::~TradeWindow()
{
}

void TradeWindow::setMoney(int amount)
{
    if (amount < 0 || amount < mGotMaxMoney)
    {
        if (config.getBoolValue("securetrades"))
        {
            close();
            return;
        }
        else
        {
            mMoneyLabel->setForegroundColor(Theme::getThemeColor(
                Theme::WARNING));
        }
    }
    else
    {
        mMoneyLabel->setForegroundColor(Theme::getThemeColor(Theme::TEXT));
        mGotMaxMoney = amount;
    }

    mGotMoney = amount;
    mMoneyLabel->setCaption(strprintf(_("You get %s"),
                            Units::formatCurrency(amount).c_str()));
    mMoneyLabel->adjustSize();
}

void TradeWindow::addItem(int id, bool own, int quantity,
                          int refine, unsigned char color)
{
    if (own)
        mMyInventory->addItem(id, quantity, refine, color);
    else
        mPartnerInventory->addItem(id, quantity, refine, color);
}

void TradeWindow::addItem2(int id, bool own, int quantity,
                           int refine, unsigned char color, bool equipment)
{
    if (own)
        mMyInventory->addItem(id, quantity, refine, color, equipment);
    else
        mPartnerInventory->addItem(id, quantity, refine, color, equipment);
}

void TradeWindow::changeQuantity(int index, bool own, int quantity)
{
    if (own)
    {
        if (mMyInventory->getItem(index))
            mMyInventory->getItem(index)->setQuantity(quantity);
    }
    else
    {
        if (mPartnerInventory->getItem(index))
            mPartnerInventory->getItem(index)->setQuantity(quantity);
    }
}

void TradeWindow::increaseQuantity(int index, bool own, int quantity)
{
    if (own)
    {
        if (mMyInventory->getItem(index))
            mMyInventory->getItem(index)->increaseQuantity(quantity);
    }
    else
    {
        if (mPartnerInventory->getItem(index))
            mPartnerInventory->getItem(index)->increaseQuantity(quantity);
    }
}

void TradeWindow::reset()
{
    mMyInventory->clear();
    mPartnerInventory->clear();
    mOkOther = false;
    mOkMe = false;
    setMoney(0);
    mMoneyField->setEnabled(true);
    mMoneyField->setText("");
    mMoneyLabel->setForegroundColor(Theme::getThemeColor(Theme::TEXT));
    mAddButton->setEnabled(true);
    mMoneyChangeButton->setEnabled(true);
    mGotMoney = 0;
    mGotMaxMoney = 0;
    setStatus(PREPARING);
}

void TradeWindow::receivedOk(bool own)
{
    if (own)
        mOkMe = true;
    else
        mOkOther = true;

    if (mOkMe && mOkOther)
    {
        //mOkMe = false;
        //mOkOther = false;
        setStatus(ACCEPTING);
    }
}

void TradeWindow::tradeItem(Item *item, int quantity, bool check)
{
    if (check && !checkItem(item))
        return;

    Net::getTradeHandler()->addItem(item, quantity);
}

void TradeWindow::valueChanged(const gcn::SelectionEvent &event)
{
    if (!mMyItemContainer || !mPartnerItemContainer)
        return;

    /* If an item is selected in one container, make sure no item is selected
     * in the other container.
     */
    if (event.getSource() == mMyItemContainer &&
        mMyItemContainer->getSelectedItem())
    {
        mPartnerItemContainer->selectNone();
    }
    else if (mPartnerItemContainer->getSelectedItem())
    {
        mMyItemContainer->selectNone();
    }
}

void TradeWindow::setStatus(Status s)
{
    if (s == mStatus)
        return;
    mStatus = s;

    switch (s)
    {
        case PREPARING:
            mOkButton->setCaption(CAPTION_PROPOSE);
            mOkButton->setActionEventId("ok");
            break;
        case PROPOSING:
            mOkButton->setCaption(CAPTION_CONFIRMED);
            mOkButton->setActionEventId("");
            break;
        case ACCEPTING:
            mOkButton->setCaption(CAPTION_ACCEPT);
            mOkButton->setActionEventId("trade");
            break;
        case ACCEPTED:
            mOkButton->setCaption(CAPTION_ACCEPTED);
            mOkButton->setActionEventId("");
            break;
        default:
            break;
    }

    mOkButton->setEnabled((s != PROPOSING && s != ACCEPTED));
}

void TradeWindow::action(const gcn::ActionEvent &event)
{
    if (!inventoryWindow)
        return;

    Item *item = inventoryWindow->getSelectedItem();

    if (event.getId() == "add")
    {
        if (mStatus != PREPARING)
            return;

        if (!inventoryWindow->isVisible())
        {
            inventoryWindow->setVisible(true);
            return;
        }

        if (!item)
            return;

        if (mMyInventory->getFreeSlot() == -1)
            return;


        if (!checkItem(item))
            return;

        // Choose amount of items to trade
        ItemAmountWindow::showWindow(ItemAmountWindow::TradeAdd, this, item);

        setStatus(PREPARING);
    }
    else if (event.getId() == "cancel")
    {
        setVisible(false);
        reset();
        PlayerInfo::setTrading(false);

        Net::getTradeHandler()->cancel();
    }
    else if (event.getId() == "ok")
    {
        mMoneyField->setEnabled(false);
        mAddButton->setEnabled(false);
        mMoneyChangeButton->setEnabled(false);
        receivedOk(true);
        setStatus(PROPOSING);
        Net::getTradeHandler()->confirm();
    }
    else if (event.getId() == "trade")
    {
        receivedOk(true);
        setStatus(ACCEPTED);
        Net::getTradeHandler()->finish();
    }
    else if (event.getId() == "money")
    {
        if (mStatus != PREPARING) 
            return;

        int v = atoi(mMoneyField->getText().c_str());
        int curMoney = PlayerInfo::getAttribute(MONEY);
        if (v > curMoney)
        {
            if (localChatTab)
            {
                localChatTab->chatLog(_("You don't have enough money."),
                                      BY_SERVER);
            }
            v = curMoney;
        }
        Net::getTradeHandler()->setMoney(v);
        mMoneyField->setText(strprintf("%d", v));
    }
}

void TradeWindow::close()
{
    Net::getTradeHandler()->cancel();
    clear();
}

void TradeWindow::clear()
{
    mAutoAddItem = nullptr;
    mAutoAddToNick = "";
    mAutoMoney = 0;
    mAutoAddAmount = 0;
    mGotMoney = 0;
    mGotMaxMoney = 0;
    mMoneyLabel->setForegroundColor(Theme::getThemeColor(Theme::TEXT));
}

void TradeWindow::addAutoItem(std::string nick, Item* item, int amount)
{
    mAutoAddToNick = nick;
    mAutoAddItem = item;
    mAutoAddAmount = amount;
}

void TradeWindow::addAutoMoney(std::string nick, int money)
{
    mAutoAddToNick = nick;
    mAutoMoney = money;
}

void TradeWindow::initTrade(std::string nick)
{
    if (!player_node)
        return;

    if (!mAutoAddToNick.empty() && mAutoAddToNick == nick)
    {
        if (mAutoAddItem && mAutoAddItem->getQuantity())
        {
            Inventory *inv = PlayerInfo::getInventory();
            if (inv)
            {
                Item *item = inv->findItem(mAutoAddItem->getId(),
                    mAutoAddItem->getColor());
                if (item)
                    tradeItem(item, mAutoAddItem->getQuantity());
            }
        }
        if (mAutoMoney)
        {
            Net::getTradeHandler()->setMoney(mAutoMoney);
            mMoneyField->setText(strprintf("%d", mAutoMoney));
        }
    }
    clear();
    if (!player_relations.isGoodName(nick))
        setCaptionFont(gui->getSecureFont());
}

bool TradeWindow::checkItem(Item *item)
{
    Item *tItem = mMyInventory->findItem(
        item->getId(), item->getColor());

    if (tItem && (tItem->getQuantity() > 1
        || item->getQuantity() > 1))
    {
        if (localChatTab)
        {
            localChatTab->chatLog(_("Failed adding item. You can not "
                "overlap one kind of item on the window."), BY_SERVER);
        }
        return false;
    }
    return true;
}

bool TradeWindow::isInpupFocused()
{
    return (mMoneyField && mMoneyField->isFocused());
}
