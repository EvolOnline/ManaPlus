/*
 *  The ManaPlus Client
 *  Copyright (C) 2007-2009  The Mana World Development Team
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

#ifndef DYE_H
#define DYE_H

#include <string>
#include <vector>

#include <SDL_stdinc.h>

const int dyePalateSize = 8;

/**
 * Class for performing a linear interpolation between colors.
 */
class DyePalette
{
    public:

        /**
         * Creates a palette based on the given string.
         * The string is either a file name or a sequence of hexadecimal RGB
         * values separated by ',' and starting with '#'.
         */
        DyePalette(const std::string &pallete);

/*
        void addFirstColor(const int color[3]);

        void addLastColor(const int color[3]);
*/

        /**
         * Gets a pixel color depending on its intensity. First color is
         * implicitly black (0, 0, 0).
         */
        void getColor(int intensity, int color[3]) const;

        /**
         * Gets a pixel color depending on its intensity.
         */
        void getColor(double intensity, int color[3]) const;

        void replaceColor(uint8_t *color) const;

        void replaceOGLColor(uint8_t *color) const;

    private:
        struct Color
        { unsigned char value[3]; };

        std::vector<Color> mColors;
};

/**
 * Class for dispatching pixel-recoloring amongst several palettes.
 */
class Dye
{
    public:

        /**
         * Creates a set of palettes based on the given string.
         *
         * The parts of string are separated by semi-colons. Each part starts
         * by an uppercase letter, followed by a colon and then a palette name.
         */
        Dye(const std::string &dye);

        /**
         * Destroys the associated palettes.
         */
        ~Dye();

        /**
         * Modifies a pixel color.
         */
        void update(int color[3]) const;

        /**
         * Fills the blank in a dye placeholder with some palette names.
         */
        static void instantiate(std::string &target,
                                const std::string &palettes);

        /**
         * Check if dye is special dye (S)
         */
        bool isSpecialDye() const
        { return mDyePalettes[dyePalateSize - 1]; }

        /**
         * Return special dye palete (S)
         */
        DyePalette *getSPalete() const
        { return mDyePalettes[dyePalateSize - 1]; }

    private:

        /**
         * The order of the palettes, as well as their uppercase letter, is:
         *
         * Red, Green, Yellow, Blue, Magenta, White (or rather gray), Simple.
         */
        DyePalette *mDyePalettes[dyePalateSize];
};

#endif
