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

#include "gui/equipmentwindow.h"

#include "gui/widgets/button.h"

#include "being.h"
#include "equipment.h"
#include "graphics.h"
#include "inventory.h"
#include "item.h"
#include "localplayer.h"

#include "gui/itempopup.h"
#include "gui/theme.h"
#include "gui/setup.h"
#include "gui/viewport.h"

#include "gui/widgets/playerbox.h"

#include "net/inventoryhandler.h"
#include "net/net.h"

#include "resources/image.h"
#include "resources/imageset.h"
#include "resources/iteminfo.h"
#include "resources/resourcemanager.h"

#include "utils/dtor.h"
#include "utils/gettext.h"
#include "utils/stringutils.h"

#include <guichan/font.hpp>

#include "debug.h"

static const int BOX_WIDTH = 36;
static const int BOX_HEIGHT = 36;
static const int BOX_X_PAD = (BOX_WIDTH - 32) / 2;
static const int BOX_Y_PAD = (BOX_HEIGHT - 32) / 2;
static const int BOX_COUNT = 13;

EquipmentWindow::EquipmentWindow(Equipment *equipment, Being *being,
                                 bool foring):
    Window(_("Equipment"), false, nullptr, "equipment.xml"),
    mEquipment(equipment),
    mSelected(-1),
    mForing(foring),
    mImageSet(nullptr)
{
    mBeing = being;
    mItemPopup = new ItemPopup;
    if (setupWindow)
        setupWindow->registerWindowForReset(this);

    // Control that shows the Player
    mPlayerBox = new PlayerBox;
    mPlayerBox->setDimension(gcn::Rectangle(50, 80, 74, 168));
    mPlayerBox->setPlayer(being);

    if (foring)
        setWindowName("Being equipment");
    else
        setWindowName("Equipment");

    setCloseButton(true);
    setSaveVisible(true);
    setStickyButtonLock(true);

    setDefaultSize(180, 345, ImageRect::CENTER);

    mBoxes.reserve(BOX_COUNT);
    for (int f = 0; f < BOX_COUNT; f ++)
        mBoxes.push_back(nullptr);

    fillBoxes();

    loadWindowState();

    mUnequip = new Button(_("Unequip"), "unequip", this);
    const gcn::Rectangle &area = getChildrenArea();
    mUnequip->setPosition(area.width  - mUnequip->getWidth() - 5,
                          area.height - mUnequip->getHeight() - 5);
    mUnequip->setEnabled(false);

    add(mPlayerBox);
    add(mUnequip);

    mHighlightColor = Theme::getThemeColor(Theme::HIGHLIGHT);
    mBorderColor = Theme::getThemeColor(Theme::BORDER);
    setForegroundColor(Theme::getThemeColor(Theme::TEXT));
}

EquipmentWindow::~EquipmentWindow()
{
    delete mItemPopup;
    mItemPopup = nullptr;
    if (this == beingEquipmentWindow)
    {
        if (mEquipment)
            delete mEquipment->getBackend();
        delete mEquipment;
        mEquipment = nullptr;
    }
    delete_all(mBoxes);
    mBoxes.clear();
    if (mImageSet)
    {
        mImageSet->decRef();
        mImageSet = nullptr;
    }
}

void EquipmentWindow::draw(gcn::Graphics *graphics)
{
    // Draw window graphics
    Window::draw(graphics);

    Graphics *g = static_cast<Graphics*>(graphics);

    Window::drawChildren(graphics);

    int i = 0;
    const int fontHeight = getFont()->getHeight();

    for (std::vector<EquipmentBox*>::const_iterator it = mBoxes.begin(),
         it_end = mBoxes.end(); it != it_end; ++ it, ++ i)
    {
        EquipmentBox *box = *it;
        if (!box)
            continue;
        if (i == mSelected)
        {
            mHighlightColor.a = getGuiAlpha();
            // Set color to the highlight color
            g->setColor(mHighlightColor);
            g->fillRectangle(gcn::Rectangle(box->x, box->y,
                BOX_WIDTH, BOX_HEIGHT));
        }

        // Set color black
        g->setColor(mBorderColor);
        // Draw box border
        g->drawRectangle(gcn::Rectangle(box->x, box->y,
            BOX_WIDTH, BOX_HEIGHT));

        if (!mEquipment)
            continue;

        Item *item = mEquipment->getEquipment(i);
        if (item)
        {
            // Draw Item.
            Image *image = item->getImage();
            if (image)
            {
                image->setAlpha(1.0f); // Ensure the image is drawn
                                       // with maximum opacity
                g->drawImage(image, box->x + 2, box->y + 2);
                if (i == EQUIP_PROJECTILE_SLOT)
                {
                    g->setColor(getForegroundColor());
                    graphics->drawText(toString(item->getQuantity()),
                        box->x + (BOX_WIDTH / 2), box->y - fontHeight,
                        gcn::Graphics::CENTER);
                }
            }
        }
        else if (box->image)
        {
            g->drawImage(box->image, box->x + BOX_X_PAD,
                box->y + BOX_Y_PAD);
        }
    }
}

void EquipmentWindow::action(const gcn::ActionEvent &event)
{
    if (!mEquipment)
        return;

    if (event.getId() == "unequip" && mSelected > -1)
    {
        Item *item = mEquipment->getEquipment(mSelected);
        Net::getInventoryHandler()->unequipItem(item);
        setSelected(-1);
    }
}

Item *EquipmentWindow::getItem(int x, int y) const
{
    if (!mEquipment)
        return nullptr;

    int i = 0;

    for (std::vector<EquipmentBox*>::const_iterator it = mBoxes.begin(),
         it_end = mBoxes.end(); it != it_end; ++ it, ++ i)
    {
        EquipmentBox *box = *it;
        if (!box)
            continue;
        const gcn::Rectangle tRect(box->x, box->y,
            BOX_WIDTH, BOX_HEIGHT);

        if (tRect.isPointInRect(x, y))
            return mEquipment->getEquipment(i);
    }
    return nullptr;
}

void EquipmentWindow::mousePressed(gcn::MouseEvent& mouseEvent)
{
    Window::mousePressed(mouseEvent);

    if (!mEquipment)
        return;

    const int x = mouseEvent.getX();
    const int y = mouseEvent.getY();

    if (mouseEvent.getButton() == gcn::MouseEvent::LEFT)
    {
        if (mForing)
            return;
        // Checks if any of the presses were in the equip boxes.
        int i = 0;

        for (std::vector<EquipmentBox*>::const_iterator it = mBoxes.begin(),
             it_end = mBoxes.end(); it != it_end; ++ it, ++ i)
        {
            EquipmentBox *box = *it;
            if (!box)
                continue;
            Item *item = mEquipment->getEquipment(i);
            const gcn::Rectangle tRect(box->x, box->y,
                BOX_WIDTH, BOX_HEIGHT);

            if (tRect.isPointInRect(x, y) && item)
                setSelected(i);
        }
    }
    else if (mouseEvent.getButton() == gcn::MouseEvent::RIGHT)
    {
        if (Item *item = getItem(x, y))
        {
            if (mItemPopup)
                mItemPopup->setVisible(false);

            /* Convert relative to the window coordinates to absolute screen
             * coordinates.
             */
            const int mx = x + getX();
            const int my = y + getY();
            if (viewport)
            {
                if (mForing)
                    viewport->showUndressPopup(mx, my, mBeing, item);
                else
                    viewport->showPopup(this, mx, my, item, true);
            }
        }
    }
}

// Show ItemTooltip
void EquipmentWindow::mouseMoved(gcn::MouseEvent &event)
{
    if (!mItemPopup)
        return;

    const int x = event.getX();
    const int y = event.getY();

    Item *item = getItem(x, y);

    if (item)
    {
        int mouseX, mouseY;
        SDL_GetMouseState(&mouseX, &mouseY);

        mItemPopup->setItem(item);
        mItemPopup->position(x + getX(), y + getY());
    }
    else
    {
        mItemPopup->setVisible(false);
    }
}

// Hide ItemTooltip
void EquipmentWindow::mouseExited(gcn::MouseEvent &event A_UNUSED)
{
    if (mItemPopup)
        mItemPopup->setVisible(false);
}

void EquipmentWindow::setSelected(int index)
{
    mSelected = index;
    if (mUnequip)
        mUnequip->setEnabled(mSelected != -1);
}

void EquipmentWindow::setBeing(Being *being)
{
    mPlayerBox->setPlayer(being);
    mBeing = being;
    if (mEquipment)
        delete mEquipment->getBackend();
    delete mEquipment;
    if (!being)
    {
        mEquipment = nullptr;
        return;
    }
    mEquipment = being->getEquipment();
}

void EquipmentWindow::updateBeing(Being *being)
{
    if (being == mBeing)
        setBeing(being);
}

void EquipmentWindow::resetBeing(Being *being)
{
    if (being == mBeing)
        setBeing(nullptr);
}

void EquipmentWindow::fillBoxes()
{
    XML::Document *doc = new XML::Document("equipmentwindow.xml");
    XmlNodePtr root = doc->rootNode();
    if (!root)
    {
        delete doc;
        fillDefault();
        return;
    }

    if (mImageSet)
        mImageSet->decRef();

    mImageSet = Theme::getImageSetFromTheme(XML::getProperty(
        root, "image", "equipmentbox.png"), 32, 32);

    for_each_xml_child_node(node, root)
    {
        if (xmlNameEqual(node, "window"))
            loadWindow(node);
        else if (xmlNameEqual(node, "playerbox"))
            loadPlayerBox(node);
        else if (xmlNameEqual(node, "slot"))
            loadSlot(node, mImageSet);
    }
    delete doc;
}

void EquipmentWindow::loadWindow(XmlNodePtr windowNode)
{
    setDefaultSize(XML::getProperty(windowNode, "width", 180),
        XML::getProperty(windowNode, "height", 345), ImageRect::CENTER);
}

void EquipmentWindow::loadPlayerBox(XmlNodePtr playerBoxNode)
{
    mPlayerBox->setDimension(gcn::Rectangle(
        XML::getProperty(playerBoxNode, "x", 50),
        XML::getProperty(playerBoxNode, "y", 80),
        XML::getProperty(playerBoxNode, "width", 74),
        XML::getProperty(playerBoxNode, "height", 168)));
}

void EquipmentWindow::loadSlot(XmlNodePtr slotNode, ImageSet *imageset)
{
    int slot = parseSlotName(XML::getProperty(slotNode, "name", ""));
    if (slot < 0)
        return;

    const int x = XML::getProperty(slotNode, "x", 0) + getPadding();
    const int y = XML::getProperty(slotNode, "y", 0) + getTitleBarHeight();
    const int imageIndex = XML::getProperty(slotNode, "image", -1);
    Image *image = nullptr;

    if (imageset && imageIndex >= 0 && imageIndex
        < static_cast<signed>(imageset->size()))
    {
        image = imageset->get(imageIndex);
    }

    if (mBoxes[slot])
    {
        EquipmentBox *box = mBoxes[slot];
        box->x = x;
        box->y = y;
        box->image = image;
    }
    else
    {
        mBoxes[slot] = new EquipmentBox(x, y, image);
    }
}

int EquipmentWindow::parseSlotName(std::string name)
{
    int id = -1;
    if (name == "shoes" || name == "boot" || name == "boots")
    {
        id = 4;
    }
    else if (name == "bottomclothes" || name == "bottom" || name == "pants")
    {
        id = 3;
    }
    else if (name == "topclothes" || name == "top"
             || name == "torso" || name == "body")
    {
        id = 0;
    }
    else if (name == "free" || name == "misc1")
    {
        id = 5;
    }
    else if (name == "misc2" || name == "scarf" || name == "scarfs")
    {
        id = 7;
    }
    else if (name == "hat" || name == "hats")
    {
        id = 2;
    }
    else if (name == "wings")
    {
        id = 6;
    }
    else if (name == "glove" || name == "gloves")
    {
        id = 1;
    }
    else if (name == "weapon" || name == "weapons")
    {
        id = 8;
    }
    else if (name == "shield" || name == "shields")
    {
        id = 9;
    }
    else if (name == "amulet" || name == "amulets")
    {
        id = 11;
    }
    else if (name == "ring" || name == "rings")
    {
        id = 12;
    }
    else if (name == "arrow" || name == "arrows" || name == "ammo")
    {
        id = 10;
    }

    return id;
}

void EquipmentWindow::fillDefault()
{
    if (mImageSet)
        mImageSet->decRef();

    mImageSet = Theme::getImageSetFromTheme(
        "equipmentbox.png", 32, 32);

    addBox(0, 90, 40, 0);    // torso
    addBox(1, 8, 78, 1);     // gloves
    addBox(2, 70, 0, 2);     // hat
    addBox(3, 50, 253, 3);   // pants
    addBox(4, 90, 253, 4);   // boots
    addBox(5, 8, 213, 5);    // FREE
    addBox(6, 129, 213, 6);  // wings
    addBox(7, 50, 40, 5);    // scarf
    addBox(8, 8, 168, 7);    // weapon
    addBox(9, 129, 168, 8);  // shield
    addBox(10, 129, 78, 9);  // ammo
    addBox(11, 8, 123, 5);   // amulet
    addBox(12, 129, 123, 5); // ring
}

void EquipmentWindow::addBox(int idx, int x, int y, int imageIndex)
{
    Image *image = nullptr;

    if (mImageSet && imageIndex >= 0 && imageIndex
        < static_cast<signed>(mImageSet->size()))
    {
        image = mImageSet->get(imageIndex);
    }

    mBoxes[idx] = new EquipmentBox(x + getPadding(), y + getTitleBarHeight(),
        image);
}
