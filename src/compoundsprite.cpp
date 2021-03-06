/*
 *  The ManaPlus Client
 *  Copyright (C) 2010  The Mana Developers
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

#include "compoundsprite.h"

#include "configuration.h"
#include "game.h"
#include "graphics.h"

#ifdef USE_OPENGL
#include "openglgraphics.h"
#include "opengl1graphics.h"
#endif

#include "client.h"
#include "map.h"

#include "resources/image.h"
#include "resources/imagehelper.h"
#include "resources/openglimagehelper.h"

#include "utils/dtor.h"

#include <SDL.h>

#include "debug.h"

#define BUFFER_WIDTH 100
#define BUFFER_HEIGHT 100

static const unsigned cache_max_size = 10;
static const unsigned cache_clean_part = 3;
bool CompoundSprite::mEnableDelay = true;

CompoundSprite::CompoundSprite() :
    mCacheItem(nullptr),
    mImage(nullptr),
    mAlphaImage(nullptr),
    mOffsetX(0),
    mOffsetY(0),
    mNeedsRedraw(false),
    mEnableAlphaFix(config.getBoolValue("enableAlphaFix")),
    mDisableAdvBeingCaching(config.getBoolValue("disableAdvBeingCaching")),
    mDisableBeingCaching(config.getBoolValue("disableBeingCaching")),
    mNextRedrawTime(0)
{
    mAlpha = 1.0f;
}

CompoundSprite::~CompoundSprite()
{
    clear();

//    delete mImage;
    mImage = nullptr;
//    delete mAlphaImage;
    mAlphaImage = nullptr;
}

bool CompoundSprite::reset()
{
    bool ret = false;

    for (SpriteIterator it = mSprites.begin(), it_end = mSprites.end();
         it != it_end; ++ it)
    {
        if (*it)
            ret |= (*it)->reset();
    }

    mNeedsRedraw |= ret;
    return ret;
}

bool CompoundSprite::play(std::string action)
{
    bool ret = false;

    for (SpriteIterator it = mSprites.begin(), it_end = mSprites.end();
         it != it_end; ++ it)
    {
        if (*it)
            ret |= (*it)->play(action);
    }

    mNeedsRedraw |= ret;
    return ret;
}

bool CompoundSprite::update(int time)
{
    bool ret = false;

    for (SpriteIterator it = mSprites.begin(), it_end = mSprites.end();
         it != it_end; ++ it)
    {
        if (*it)
            ret |= (*it)->update(time);
    }

    mNeedsRedraw |= ret;
    return ret;
}

bool CompoundSprite::draw(Graphics *graphics, int posX, int posY) const
{
    if (mNeedsRedraw)
        updateImages();

    if (mSprites.empty()) // Nothing to draw
        return false;

    if (mAlpha == 1.0f && mImage)
    {
        return graphics->drawImage(mImage, posX + mOffsetX, posY + mOffsetY);
    }
    else if (mAlpha && mAlphaImage)
    {
        mAlphaImage->setAlpha(mAlpha);

        return graphics->drawImage(mAlphaImage,
                                   posX + mOffsetX, posY + mOffsetY);
    }
    else
    {
        drawSprites(graphics, posX, posY);
    }
    return false;
}

void CompoundSprite::drawSprites(Graphics* graphics, int posX, int posY) const
{
    for (SpriteConstIterator it = mSprites.begin(), it_end = mSprites.end();
         it != it_end; ++ it)
    {
        if (*it)
        {
            (*it)->setAlpha(mAlpha);
            (*it)->draw(graphics, posX, posY);
        }
    }
}

void CompoundSprite::drawSpritesSDL(Graphics* graphics,
                                    int posX, int posY) const
{
    for (SpriteConstIterator it = mSprites.begin(), it_end = mSprites.end();
         it != it_end; ++ it)
    {
        if (*it)
            (*it)->draw(graphics, posX, posY);
    }
}

int CompoundSprite::getWidth() const
{
    Sprite *base = nullptr;

    for (SpriteConstIterator it = mSprites.begin(), it_end = mSprites.end();
         it != it_end; ++ it)
    {
        if ((base = (*it)))
            break;
    }

    if (base)
        return base->getWidth();

    return 0;
}

int CompoundSprite::getHeight() const
{
    Sprite *base = nullptr;

    for (SpriteConstIterator it = mSprites.begin(), it_end = mSprites.end();
         it != it_end; ++ it)
    {
        if ((base = (*it)))
            break;
    }

    if (base)
        return base->getHeight();

    return 0;
}

const Image *CompoundSprite::getImage() const
{
    return mImage;
}

bool CompoundSprite::setSpriteDirection(SpriteDirection direction)
{
    bool ret = false;

    for (SpriteIterator it = mSprites.begin(), it_end = mSprites.end();
         it != it_end; ++ it)
    {
        if (*it)
            ret |= (*it)->setSpriteDirection(direction);
    }

    mNeedsRedraw |= ret;
    return ret;
}

int CompoundSprite::getNumberOfLayers() const
{
    if (mImage || mAlphaImage)
        return 1;
    else
        return size();
}

unsigned int CompoundSprite::getCurrentFrame() const
{
    for (SpriteConstIterator it = mSprites.begin(), it_end = mSprites.end();
         it != it_end; ++ it)
    {
        if (*it)
            return (*it)->getCurrentFrame();
    }

    return 0;
}

unsigned int CompoundSprite::getFrameCount() const
{
    for (SpriteConstIterator it = mSprites.begin(), it_end = mSprites.end();
         it != it_end; ++ it)
    {
        if (*it)
            return (*it)->getFrameCount();
    }

    return 0;
}

void CompoundSprite::addSprite(Sprite *sprite)
{
    mSprites.push_back(sprite);
    mNeedsRedraw = true;
}

void CompoundSprite::setSprite(int layer, Sprite* sprite)
{
    // Skip if it won't change anything
    if (mSprites.at(layer) == sprite)
        return;

    if (mSprites.at(layer))
        delete mSprites.at(layer);
    mSprites[layer] = sprite;
    mNeedsRedraw = true;
}

void CompoundSprite::removeSprite(int layer)
{
    // Skip if it won't change anything
    if (!mSprites.at(layer))
        return;

    delete mSprites.at(layer);
    mSprites.at(layer) = nullptr;
    mNeedsRedraw = true;
}

void CompoundSprite::clear()
{
    // Skip if it won't change anything
    if (!mSprites.empty())
    {
        delete_all(mSprites);
        mSprites.clear();
    }
    mNeedsRedraw = true;
    delete_all(imagesCache);
    imagesCache.clear();
    delete mCacheItem;
    mCacheItem = nullptr;
}

void CompoundSprite::ensureSize(size_t layerCount)
{
    // Skip if it won't change anything
    if (mSprites.size() >= layerCount)
        return;

//    resize(layerCount, nullptr);
    mSprites.resize(layerCount);
}

/**
 * Returns the curent frame in the current animation of the given layer.
 */
unsigned int CompoundSprite::getCurrentFrame(unsigned int layer)
{
    if (layer >= mSprites.size())
        return 0;

    Sprite *s = getSprite(layer);
    if (s)
        return s->getCurrentFrame();

    return 0;
}

/**
 * Returns the frame count in the current animation of the given layer.
 */
unsigned int CompoundSprite::getFrameCount(unsigned int layer)
{
    if (layer >= mSprites.size())
        return 0;

    Sprite *s = getSprite(layer);
    if (s)
        return s->getFrameCount();

    return 0;
}

void CompoundSprite::redraw() const
{

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    int rmask = 0xff000000;
    int gmask = 0x00ff0000;
    int bmask = 0x0000ff00;
    int amask = 0x000000ff;
#else
    int rmask = 0x000000ff;
    int gmask = 0x0000ff00;
    int bmask = 0x00ff0000;
    int amask = 0xff000000;
#endif

    SDL_Surface *surface = SDL_CreateRGBSurface(SDL_HWSURFACE,
        BUFFER_WIDTH, BUFFER_HEIGHT, 32, rmask, gmask, bmask, amask);

    if (!surface)
        return;

    Graphics *graphics = new Graphics();
    graphics->setBlitMode(Graphics::BLIT_GFX);
    graphics->setTarget(surface);
    graphics->_beginDraw();

    int tileX = 32 / 2;
    int tileY = 32;

    Game *game = Game::instance();
    if (game)
    {
        Map *map = game->getCurrentMap();
        if (map)
        {
            tileX = map->getTileWidth() / 2;
            tileY = map->getTileWidth();
        }
    }

    int posX = BUFFER_WIDTH / 2 - tileX;
    int posY = BUFFER_HEIGHT - tileY;

    mOffsetX = tileX - BUFFER_WIDTH / 2;
    mOffsetY = tileY - BUFFER_HEIGHT;

    drawSpritesSDL(graphics, posX, posY);

    delete graphics;
    graphics = nullptr;

    SDL_Surface *surfaceA = SDL_CreateRGBSurface(SDL_HWSURFACE,
        BUFFER_WIDTH, BUFFER_HEIGHT, 32, rmask, gmask, bmask, amask);

    SDL_SetAlpha(surface, 0, SDL_ALPHA_OPAQUE);
    SDL_BlitSurface(surface, nullptr, surfaceA, nullptr);

    delete mImage;
    delete mAlphaImage;

    mImage = imageHelper->load(surface);
    SDL_FreeSurface(surface);

    if (ImageHelper::mEnableAlpha)
    {
        mAlphaImage = imageHelper->load(surfaceA);
        SDL_FreeSurface(surfaceA);
    }
    else
    {
        mAlphaImage = nullptr;
    }
}

void CompoundSprite::setAlpha(float alpha)
{
    if (alpha != mAlpha)
    {
#ifdef USE_OPENGL
        if (mEnableAlphaFix && imageHelper->useOpenGL() == 0
            && size() > 3)
#else
        if (mEnableAlphaFix && size() > 3)
#endif
        {
            for (SpriteConstIterator it = mSprites.begin(),
                 it_end = mSprites.end(); it != it_end; ++ it)
            {
                if (*it)
                    (*it)->setAlpha(alpha);
            }
        }
        mAlpha = alpha;
    }
}

void CompoundSprite::updateImages() const
{
#ifdef USE_OPENGL
    if (imageHelper->useOpenGL())
        return;
#endif

    if (mEnableDelay)
    {
        if (get_elapsed_time1(mNextRedrawTime) < 10)
            return;
        mNextRedrawTime = tick_time;
    }
    mNeedsRedraw = false;

    if (!mDisableBeingCaching)
    {
        if (size() <= 3)
            return;

        if (!mDisableAdvBeingCaching)
        {
            if (updateFromCache())
                return;

            redraw();

            if (mImage)
                initCurrentCacheItem();
        }
        else
        {
            redraw();
        }
    }
}

bool CompoundSprite::updateFromCache() const
{
//    static int hits = 0;
//    static int miss = 0;

    if (mCacheItem && mCacheItem->image)
    {
        imagesCache.push_front(mCacheItem);
        mCacheItem = nullptr;
        if (imagesCache.size() > cache_max_size)
        {
            for (unsigned f = 0; f < cache_clean_part; f ++)
            {
                CompoundItem *item = imagesCache.back();
                imagesCache.pop_back();
                delete item;
            }
        }
    }

//    logger->log("cache size: %d, hit %d, miss %d",
//        (int)imagesCache.size(), hits, miss);

    for (ImagesCache::iterator it = imagesCache.begin(),
         it_end = imagesCache.end(); it != it_end; ++ it)
    {
        CompoundItem *ic = *it;
        if (ic && ic->data.size() == size())
        {
            bool fail(false);
            VectorPointers::const_iterator it2 = ic->data.begin();
            VectorPointers::const_iterator it2_end = ic->data.end();

            for (SpriteConstIterator it1 = mSprites.begin(),
                 it1_end = mSprites.end();
                 it1 != it1_end && it2 != it2_end;
                 ++ it1, ++ it2)
            {
                void *ptr1 = nullptr;
                void *ptr2 = nullptr;
                if (*it1)
                    ptr1 = (*it1)->getHash();
                if (*it2)
                    ptr2 = *it2;
                if (ptr1 != ptr2)
                {
                    fail = true;
                    break;
                }
            }
            if (!fail)
            {
//                hits ++;
                mImage = (*it)->image;
                mAlphaImage = (*it)->alphaImage;
                imagesCache.erase(it);
                mCacheItem = ic;
                return true;
            }
        }
    }
    mImage = nullptr;
    mAlphaImage = nullptr;
//    miss++;
    return false;
}

void CompoundSprite::initCurrentCacheItem() const
{
    delete mCacheItem;
    mCacheItem = new CompoundItem();
    mCacheItem->image = mImage;
    mCacheItem->alphaImage = mAlphaImage;
//    mCacheItem->alpha = mAlpha;

    for (SpriteConstIterator it = mSprites.begin(), it_end = mSprites.end();
         it != it_end; ++ it)
    {
        if (*it)
            mCacheItem->data.push_back((*it)->getHash());
        else
            mCacheItem->data.push_back(nullptr);
    }
}

bool CompoundSprite::updateNumber(unsigned num)
{
    bool res(false);
    for (SpriteConstIterator it = mSprites.begin(),
         it_end = mSprites.end(); it != it_end; ++ it)
    {
        if (*it)
        {
            if ((*it)->updateNumber(num))
                res = true;
        }
    }
    return res;
}

CompoundItem::CompoundItem() :
//    alpha(1.0f),
    image(nullptr),
    alphaImage(nullptr)
{
}

CompoundItem::~CompoundItem()
{
    delete image;
    delete alphaImage;
}
