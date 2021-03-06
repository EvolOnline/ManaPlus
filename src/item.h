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

#ifndef ITEM_H
#define ITEM_H

#include "resources/itemdb.h"

#include <map>

class Image;

/**
 * Represents one or more instances of a certain item type.
 */
class Item
{
    public:
        /**
         * Constructor.
         */
        Item(int id = -1, int quantity = 0, int refine = 0,
             unsigned char color = 1, bool equipment = false,
             bool equipped = false);

        /**
         * Destructor.
         */
        virtual ~Item();

        /**
         * Sets the item id, identifying the item type.
         */
        void setId(int id, unsigned char color);

        /**
         * Returns the item id.
         */
        int getId() const
        { return mId; }

        /**
         * Returns the item image.
         */
        Image *getImage() const
        { return mImage; }

        /**
         * Returns the item image.
         */
        Image *getDrawImage() const
        { return mDrawImage; }

        /**
         * Sets the number of items.
         */
        void setQuantity(int quantity)
        { mQuantity = quantity; }

        /**
         * Increases the number of items by the given amount.
         */
        void increaseQuantity(int amount)
        { mQuantity += amount; }

        /**
         * Returns the number of items.
         */
        int getQuantity() const
        { return mQuantity; }

        /**
         * Sets whether this item is considered equipment.
         */
        void setEquipment(bool equipment)
        { mEquipment = equipment; }

        /**
         * Returns whether this item is considered equipment.
         */
        bool isEquipment() const
        { return mEquipment; }

        /**
         * Sets whether this item is equipped.
         */
        void setEquipped(bool equipped)
        { mEquipped = equipped; }

        /**
         * Returns whether this item is equipped.
         */
        bool isEquipped() const
        { return mEquipped; }

        /**
         * Sets this item refine level.
         */
        void setRefine(int refine)
        { mRefine = refine; }

        /**
         * Returns this item refine level.
         */
        int getRefine() const
        { return mRefine; }

        /**
         * Sets whether this item is in equipment.
         */
        void setInEquipment(bool inEquipment)
        { mInEquipment = inEquipment; }

        /**
         * Returns whether this item is in equipment.
         */
        bool isInEquipment() const
        { return mInEquipment; }

        /**
         * Sets the inventory index of this item.
         */
        void setInvIndex(int index)
        { mInvIndex = index; }

        /**
         * Returns the inventory index of this item.
         */
        int getInvIndex() const
        { return mInvIndex; }

        /**
         * Returns information about this item type.
         */
        const ItemInfo &getInfo() const
        { return ItemDB::get(mId); }

        std::string getName();

        static Image *getImage(int id, unsigned char color);

        bool isHaveTag(int tagId);

        unsigned char getColor() const
        { return mColor; }

        std::string &getDescription()
        { return mDescription; }

    protected:
        int mId;              /**< Item type id. */
        Image *mImage;        /**< Item image. */
        Image *mDrawImage;    /**< Draw image. */
        int mQuantity;        /**< Number of items. */
        bool mEquipment;      /**< Item is equipment. */
        bool mEquipped;       /**< Item is equipped. */
        bool mInEquipment;    /**< Item is in equipment */
        int mRefine;          /**< Item refine level. */
        int mInvIndex;        /**< Inventory index. */
        std::string mDescription;
        unsigned char mColor;
        std::map <int, int> mTags;
};

#endif
