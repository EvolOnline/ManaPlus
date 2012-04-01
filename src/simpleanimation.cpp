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

#include "simpleanimation.h"

#include "graphics.h"
#include "logger.h"

#include "utils/stringutils.h"

#include "resources/animation.h"
#include "resources/dye.h"
#include "resources/image.h"
#include "resources/imageset.h"
#include "resources/resourcemanager.h"

#include "debug.h"

SimpleAnimation::SimpleAnimation(Animation *animation):
    mAnimation(animation),
    mAnimationTime(0),
    mAnimationPhase(0),
    mCurrentFrame(mAnimation->getFrame(0)),
    mInitialized(true)
{
}

SimpleAnimation::SimpleAnimation(XmlNodePtr animationNode,
                                 const std::string& dyePalettes):
    mAnimation(new Animation),
    mAnimationTime(0),
    mAnimationPhase(0),
    mInitialized(false)
{
    initializeAnimation(animationNode, dyePalettes);
    if (mAnimation)
        mCurrentFrame = mAnimation->getFrame(0);
    else
        mCurrentFrame = nullptr;
}

SimpleAnimation::~SimpleAnimation()
{
    delete mAnimation;
    mAnimation = nullptr;
}

bool SimpleAnimation::draw(Graphics *graphics, int posX, int posY) const
{
    if (!mCurrentFrame || !mCurrentFrame->image)
        return false;

    return graphics->drawImage(mCurrentFrame->image,
                               posX + mCurrentFrame->offsetX,
                               posY + mCurrentFrame->offsetY);
}

void SimpleAnimation::reset()
{
    mAnimationTime = 0;
    mAnimationPhase = 0;
}

void SimpleAnimation::setFrame(int frame)
{
    if (!mAnimation)
        return;

    if (frame < 0)
        frame = 0;
    if (static_cast<unsigned>(frame) >= mAnimation->getLength())
        frame = mAnimation->getLength() - 1;
    mAnimationPhase = frame;
    mCurrentFrame = mAnimation->getFrame(mAnimationPhase);
}

bool SimpleAnimation::update(int timePassed)
{
    if (!mCurrentFrame || !mAnimation)
        return false;

    bool updated(false);
    if (mInitialized && mAnimation)
    {
        mAnimationTime += timePassed;

        while (mAnimationTime > mCurrentFrame->delay
               && mCurrentFrame->delay > 0)
        {
            updated = true;
            mAnimationTime -= mCurrentFrame->delay;
            mAnimationPhase++;

            if (static_cast<unsigned>(mAnimationPhase)
                >= mAnimation->getLength())
            {
                mAnimationPhase = 0;
            }

            mCurrentFrame = mAnimation->getFrame(mAnimationPhase);
        }
    }
    return updated;
}

int SimpleAnimation::getLength() const
{
    if (!mAnimation)
        return 0;

    return mAnimation->getLength();
}

Image *SimpleAnimation::getCurrentImage() const
{
    if (mCurrentFrame)
        return mCurrentFrame->image;
    else
        return nullptr;
}

void SimpleAnimation::initializeAnimation(XmlNodePtr animationNode,
                                          const std::string& dyePalettes)
{
    mInitialized = false;

    if (!animationNode)
        return;

    std::string imagePath = XML::getProperty(animationNode,
                                                   "imageset", "");

    // Instanciate the dye coloration.
    if (!imagePath.empty() && !dyePalettes.empty())
        Dye::instantiate(imagePath, dyePalettes);

    ImageSet *imageset = ResourceManager::getInstance()->getImageSet(
        XML::getProperty(animationNode, "imageset", ""),
        XML::getProperty(animationNode, "width", 0),
        XML::getProperty(animationNode, "height", 0)
    );

    if (!imageset)
        return;

    // Get animation frames
    for (XmlNodePtr frameNode = animationNode->xmlChildrenNode;
         frameNode; frameNode = frameNode->next)
    {
        int delay = XML::getProperty(frameNode, "delay", 0);
        int offsetX = XML::getProperty(frameNode, "offsetX", 0);
        int offsetY = XML::getProperty(frameNode, "offsetY", 0);
        int rand = XML::getProperty(frameNode, "rand", 100);
        offsetY -= imageset->getHeight() - 32;
        offsetX -= imageset->getWidth() / 2 - 16;

        if (xmlNameEqual(frameNode, "frame"))
        {
            int index = XML::getProperty(frameNode, "index", -1);

            if (index < 0)
            {
                logger->log1("No valid value for 'index'");
                continue;
            }

            Image *img = imageset->get(index);

            if (!img)
            {
                logger->log("No image at index %d", index);
                continue;
            }

            if (mAnimation)
                mAnimation->addFrame(img, delay, offsetX, offsetY, rand);
        }
        else if (xmlNameEqual(frameNode, "sequence"))
        {
            int start = XML::getProperty(frameNode, "start", -1);
            int end = XML::getProperty(frameNode, "end", -1);

            if (start < 0 || end < 0)
            {
                logger->log1("No valid value for 'start' or 'end'");
                continue;
            }

            while (end >= start)
            {
                Image *img = imageset->get(start);

                if (!img)
                {
                    logger->log("No image at index %d", start);
                    continue;
                }

                if (mAnimation)
                    mAnimation->addFrame(img, delay, offsetX, offsetY, rand);
                start++;
            }
        }
        else if (xmlNameEqual(frameNode, "end"))
        {
            if (mAnimation)
                mAnimation->addTerminator(rand);
        }
    }

    mInitialized = true;
}
