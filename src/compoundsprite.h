/*
 *  The ManaPlus Client
 *  Copyright (C) 2010  The Mana Developers
 *  Copyright (C) 2011  The ManaPlus Developers
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

#ifndef COMPOUNDSPRITE_H
#define COMPOUNDSPRITE_H

#include "sprite.h"

#include <vector>

class Image;

class CompoundSprite : public Sprite, private std::vector<Sprite*>
{
public:
    typedef CompoundSprite::iterator SpriteIterator;
    typedef CompoundSprite::const_iterator SpriteConstIterator;

    CompoundSprite();

    ~CompoundSprite();

    virtual bool reset();

    virtual bool play(std::string action);

    virtual bool update(int time);

    virtual bool draw(Graphics* graphics, int posX, int posY) const;

    /**
     * Gets the width in pixels of the first sprite in the list.
     */
    virtual int getWidth() const;

    /**
     * Gets the height in pixels of the first sprite in the list.
     */
    virtual int getHeight() const;

    virtual const Image* getImage() const;

    virtual bool setSpriteDirection(SpriteDirection direction);

    int getNumberOfLayers() const;

    unsigned int getCurrentFrame() const;

    unsigned int getFrameCount() const;

    size_t size() const
    { return std::vector<Sprite*>::size(); }

    void addSprite(Sprite* sprite);

    void setSprite(int layer, Sprite* sprite);

    Sprite *getSprite(int layer) const
    { return at(layer); }

    void removeSprite(int layer);

    void clear();

    void ensureSize(size_t layerCount);

    virtual void drawSprites(Graphics* graphics,
                             int posX, int posY) const;

    virtual void drawSpritesSDL(Graphics* graphics,
                                int posX, int posY) const;

    /**
     * Returns the curent frame in the current animation of the given layer.
     */
    virtual unsigned int getCurrentFrame(unsigned int layer);

    /**
     * Returns the frame count in the current animation of the given layer.
     */
    virtual unsigned int getFrameCount(unsigned int layer);

    virtual void setAlpha(float alpha);

private:

    void redraw() const;

    mutable Image *mImage;
    mutable Image *mAlphaImage;

    mutable int mOffsetX, mOffsetY;

    mutable bool mNeedsRedraw;
};

#endif // COMPOUNDSPRITE_H
