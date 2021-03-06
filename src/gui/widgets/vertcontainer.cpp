/*
 *  The ManaPlus Client
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

#include "gui/widgets/vertcontainer.h"

#include "debug.h"

VertContainer::VertContainer(int verticalItemSize, bool resizable,
                             int leftSpacing):
    mVerticalItemSize(verticalItemSize),
    mCount(0),
    mNextY(0),
    mLeftSpacing(leftSpacing),
    mVerticalSpacing(0),
    mResizable(resizable)
{
    addWidgetListener(this);
}

void VertContainer::add1(gcn::Widget *widget, int spacing)
{
    add2(widget, mResizable, spacing);
}

void VertContainer::add2(gcn::Widget *widget, bool resizable, int spacing)
{
    if (!widget)
        return;

    Container::add(widget);
    widget->setPosition(mLeftSpacing, mNextY);
    if (resizable)
    {
        widget->setSize(getWidth() - mLeftSpacing, mVerticalItemSize * 5);
        mResizableWidgets.push_back(widget);
    }
    else if (widget->getHeight() > mVerticalItemSize)
    {
        widget->setSize(widget->getWidth(), mVerticalItemSize);
    }

    if (spacing == -1)
        mNextY += mVerticalItemSize + (mVerticalSpacing * 2);
    else
        mNextY += mVerticalItemSize + (spacing * 2);
    setHeight(mNextY);
}

void VertContainer::clear()
{
    Container::clear();

    mCount = 0;
    mNextY = 0;
    mResizableWidgets.clear();
}

void VertContainer::widgetResized(const gcn::Event &event A_UNUSED)
{
    for (std::vector<gcn::Widget*>::const_iterator
         it = mResizableWidgets.begin();
         it != mResizableWidgets.end(); ++ it)
    {
        (*it)->setWidth(getWidth());
    }
}
