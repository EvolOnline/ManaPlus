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

#ifndef OPENGLIMAGEHELPER_H
#define OPENGLIMAGEHELPER_H

#include "localconsts.h"
#include "main.h"

#include "resources/imagehelper.h"

#include <SDL.h>

#ifdef USE_OPENGL

/* The definition of OpenGL extensions by SDL is giving problems with recent
 * gl.h headers, since they also include these definitions. As we're not using
 * extensions anyway it's safe to just disable the SDL version.
 */
//#define NO_SDL_GLEXT
#define GL_GLEXT_PROTOTYPES 1

#include <SDL_opengl.h>
#endif

class Dye;
class Image;

/**
 * Defines a class for loading and storing images.
 */
class OpenGLImageHelper : public ImageHelper
{
    friend class CompoundSprite;
    friend class Graphics;
    friend class Image;

    public:
        virtual ~OpenGLImageHelper()
        { }

        /**
         * Loads an image from an SDL_RWops structure and recolors it.
         *
         * @param rw         The SDL_RWops to load the image from.
         * @param dye        The dye used to recolor the image.
         *
         * @return <code>NULL</code> if an error occurred, a valid pointer
         *         otherwise.
         */
        Resource *load(SDL_RWops *rw, Dye const &dye);

        /**
         * Loads an image from an SDL surface.
         */
        Image *load(SDL_Surface *);

        Image *createTextSurface(SDL_Surface *tmpImage, float alpha);

        // OpenGL only public functions

        /**
         * Sets the target image format. Use <code>false</code> for SDL and
         * <code>true</code> for OpenGL.
         */
        static void setLoadAsOpenGL(int useOpenGL);

        static int getTextureType()
        { return mTextureType; }

        static int getInternalTextureType()
        { return mInternalTextureType; }

        static void setInternalTextureType(int n)
        { mInternalTextureType = n; }

        static void setBlur(bool n)
        { mBlur = n; }

        static int mTextureType;

        static int mInternalTextureType;

         /**
         * Tells if the image was loaded using OpenGL or SDL
         * @return true if OpenGL, false if SDL.
         */
        int useOpenGL();

    protected:
        /**
         * Returns the first power of two equal or bigger than the input.
         */
        int powerOfTwo(int input);

        Image *_GLload(SDL_Surface *tmpImage);

        static int mUseOpenGL;
        static int mTextureSize;
        static bool mBlur;
};

#endif
