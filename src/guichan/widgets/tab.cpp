/*      _______   __   __   __   ______   __   __   _______   __   __
 *     / _____/\ / /\ / /\ / /\ / ____/\ / /\ / /\ / ___  /\ /  |\/ /\
 *    / /\____\// / // / // / // /\___\// /_// / // /\_/ / // , |/ / /
 *   / / /__   / / // / // / // / /    / ___  / // ___  / // /| ' / /
 *  / /_// /\ / /_// / // / // /_/_   / / // / // /\_/ / // / |  / /
 * /______/ //______/ //_/ //_____/\ /_/ //_/ //_/ //_/ //_/ /|_/ /
 * \______\/ \______\/ \_\/ \_____\/ \_\/ \_\/ \_\/ \_\/ \_\/ \_\/
 *
 * Copyright (c) 2004 - 2008 Olof Naess�n and Per Larsson
 * Copyright (C) 2011-2012  The ManaPlus Developers
 *
 *
 * Per Larsson a.k.a finalman
 * Olof Naess�n a.k.a jansem/yakslem
 *
 * Visit: http://guichan.sourceforge.net
 *
 * License: (BSD)
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of Guichan nor the names of its contributors may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * For comments regarding functions please see the header file.
 */

#include "guichan/widgets/tab.hpp"

#include "guichan/font.hpp"
#include "guichan/graphics.hpp"
#include "guichan/widgets/button.hpp"
#include "guichan/widgets/label.hpp"
#include "guichan/widgets/tabbedarea.hpp"

#include "debug.h"

namespace gcn
{
    Tab::Tab() :
        mLabel(new Label()),
        mHasMouse(false),
        mTabbedArea(nullptr)
    {
        mLabel->setPosition(4, 4);
        add(mLabel);

        addMouseListener(this);
    }

    Tab::~Tab()
    {
        delete mLabel;
        mLabel = nullptr;
    }

    void Tab::adjustSize()
    {
        setSize(mLabel->getWidth() + 8,
                mLabel->getHeight() + 8);

        if (mTabbedArea)
            mTabbedArea->adjustTabPositions();
    }

    void Tab::setTabbedArea(TabbedArea* tabbedArea)
    {
        mTabbedArea = tabbedArea;
    }

    TabbedArea* Tab::getTabbedArea()
    {
        return mTabbedArea;
    }

    void Tab::setCaption(const std::string& caption)
    {
        mLabel->setCaption(caption);
        mLabel->adjustSize();
        adjustSize();
    }

    const std::string& Tab::getCaption() const
    {
        return mLabel->getCaption();
    }

    void Tab::mouseEntered(MouseEvent& mouseEvent A_UNUSED)
    {
        mHasMouse = true;
    }

    void Tab::mouseExited(MouseEvent& mouseEvent A_UNUSED)
    {
        mHasMouse = false;
    }
}
