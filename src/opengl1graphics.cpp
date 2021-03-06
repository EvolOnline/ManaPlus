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

#include "main.h"

#ifdef USE_OPENGL
#include "opengl1graphics.h"

#include "configuration.h"
#include "graphicsvertexes.h"
#include "logger.h"

#include "resources/image.h"
#include "resources/openglimagehelper.h"

#include <SDL.h>

#include "debug.h"

#ifndef GL_TEXTURE_RECTANGLE_ARB
#define GL_TEXTURE_RECTANGLE_ARB 0x84F5
#define GL_MAX_RECTANGLE_TEXTURE_SIZE_ARB 0x84F8
#endif

GLuint OpenGL1Graphics::mLastImage = 0;

OpenGL1Graphics::OpenGL1Graphics():
    mAlpha(false), mTexture(false), mColorAlpha(false),
    mFboId(0), mTextureId(0), mRboId(0)
{
    mOpenGL = 2;
    mName = "safe OpenGL";
}

OpenGL1Graphics::~OpenGL1Graphics()
{
}

bool OpenGL1Graphics::setVideoMode(int w, int h, int bpp, bool fs,
                                   bool hwaccel, bool resize, bool noFrame)
{
    setMainFlags(w, h, bpp, fs, hwaccel, resize, noFrame);

    return setOpenGLMode();
}

static inline void drawQuad(const Image *image,
                            int srcX, int srcY, int dstX, int dstY,
                            int width, int height)
{
    if (OpenGLImageHelper::mTextureType == GL_TEXTURE_2D)
    {
        // Find OpenGL normalized texture coordinates.
        float texX1 = static_cast<float>(srcX)
            / static_cast<float>(image->getTextureWidth());
        float texY1 = static_cast<float>(srcY)
            / static_cast<float>(image->getTextureHeight());
        float texX2 = static_cast<float>(srcX + width) / static_cast<float>(
            image->getTextureWidth());
        float texY2 = static_cast<float>(srcY + height) / static_cast<float>(
            image->getTextureHeight());

        glTexCoord2f(texX1, texY1);
        glVertex2i(dstX, dstY);
        glTexCoord2f(texX2, texY1);
        glVertex2i(dstX + width, dstY);
        glTexCoord2f(texX2, texY2);
        glVertex2i(dstX + width, dstY + height);
        glTexCoord2f(texX1, texY2);
        glVertex2i(dstX, dstY + height);
    }
    else
    {
        glTexCoord2i(srcX, srcY);
        glVertex2i(dstX, dstY);
        glTexCoord2i(srcX + width, srcY);
        glVertex2i(dstX + width, dstY);
        glTexCoord2i(srcX + width, srcY + height);
        glVertex2i(dstX + width, dstY + height);
        glTexCoord2i(srcX, srcY + height);
        glVertex2i(dstX, dstY + height);
    }
}

static inline void drawRescaledQuad(Image *image, int srcX, int srcY,
                                    int dstX, int dstY, int width, int height,
                                    int desiredWidth, int desiredHeight)
{
    if (OpenGLImageHelper::mTextureType == GL_TEXTURE_2D)
    {
        // Find OpenGL normalized texture coordinates.
        float texX1 = static_cast<float>(srcX)
            / static_cast<float>(image->getTextureWidth());
        float texY1 = static_cast<float>(srcY)
            / static_cast<float>(image->getTextureHeight());
        float texX2 = static_cast<float>(srcX + width) / static_cast<float>(
            image->getTextureWidth());
        float texY2 = static_cast<float>(srcY + height) / static_cast<float>(
            image->getTextureHeight());

        glTexCoord2f(texX1, texY1);
        glVertex2i(dstX, dstY);
        glTexCoord2f(texX2, texY1);
        glVertex2i(dstX + desiredWidth, dstY);
        glTexCoord2f(texX2, texY2);
        glVertex2i(dstX + desiredWidth, dstY + desiredHeight);
        glTexCoord2f(texX1, texY2);
        glVertex2i(dstX, dstY + desiredHeight);
    }
    else
    {
        glTexCoord2i(srcX, srcY);
        glVertex2i(dstX, dstY);
        glTexCoord2i(srcX + width, srcY);
        glVertex2i(dstX + desiredWidth, dstY);
        glTexCoord2i(srcX + width, srcY + height);
        glVertex2i(dstX + desiredWidth, dstY + desiredHeight);
        glTexCoord2i(srcX, srcY + height);
        glVertex2i(dstX, dstY + desiredHeight);
    }
}


bool OpenGL1Graphics::drawImage(const Image *image, int srcX, int srcY,
                                int dstX, int dstY,
                                int width, int height, bool useColor)
{
    if (!image)
        return false;

    srcX += image->mBounds.x;
    srcY += image->mBounds.y;

    if (!useColor)
        glColor4f(1.0f, 1.0f, 1.0f, image->mAlpha);

    bindTexture(OpenGLImageHelper::mTextureType, image->mGLImage);

    setTexturingAndBlending(true);

    // Draw a textured quad.
    glBegin(GL_QUADS);
    drawQuad(image, srcX, srcY, dstX, dstY, width, height);
    glEnd();

    if (!useColor)
    {
        glColor4ub(static_cast<GLubyte>(mColor.r),
                   static_cast<GLubyte>(mColor.g),
                   static_cast<GLubyte>(mColor.b),
                   static_cast<GLubyte>(mColor.a));
    }

    return true;
}

bool OpenGL1Graphics::drawRescaledImage(Image *image, int srcX, int srcY,
                                        int dstX, int dstY,
                                        int width, int height,
                                        int desiredWidth, int desiredHeight,
                                        bool useColor)
{
    return drawRescaledImage(image, srcX, srcY,
                             dstX, dstY,
                             width, height,
                             desiredWidth, desiredHeight,
                             useColor, true);
}

bool OpenGL1Graphics::drawRescaledImage(Image *image, int srcX, int srcY,
                                        int dstX, int dstY,
                                        int width, int height,
                                        int desiredWidth, int desiredHeight,
                                        bool useColor, bool smooth)
{
    if (!image)
        return false;

    // Just draw the image normally when no resizing is necessary,
    if (width == desiredWidth && height == desiredHeight)
    {
        return drawImage(image, srcX, srcY, dstX, dstY,
                         width, height, useColor);
    }

    // When the desired image is smaller than the current one,
    // disable smooth effect.
    if (width > desiredWidth && height > desiredHeight)
        smooth = false;

    srcX += image->mBounds.x;
    srcY += image->mBounds.y;

    if (!useColor)
        glColor4f(1.0f, 1.0f, 1.0f, image->mAlpha);

    bindTexture(OpenGLImageHelper::mTextureType, image->mGLImage);

    setTexturingAndBlending(true);

    // Draw a textured quad.
    glBegin(GL_QUADS);
    drawRescaledQuad(image, srcX, srcY, dstX, dstY, width, height,
                     desiredWidth, desiredHeight);

    if (smooth) // A basic smooth effect...
    {
        glColor4f(1.0f, 1.0f, 1.0f, 0.2f);
        drawRescaledQuad(image, srcX, srcY, dstX - 1, dstY - 1, width, height,
                        desiredWidth + 1, desiredHeight + 1);
        drawRescaledQuad(image, srcX, srcY, dstX + 1, dstY + 1, width, height,
                        desiredWidth - 1, desiredHeight - 1);

        drawRescaledQuad(image, srcX, srcY, dstX + 1, dstY, width, height,
                        desiredWidth - 1, desiredHeight);
        drawRescaledQuad(image, srcX, srcY, dstX, dstY + 1, width, height,
                        desiredWidth, desiredHeight - 1);
    }

    glEnd();

    if (!useColor)
    {
        glColor4ub(static_cast<GLubyte>(mColor.r),
                   static_cast<GLubyte>(mColor.g),
                   static_cast<GLubyte>(mColor.b),
                   static_cast<GLubyte>(mColor.a));
    }

    return true;
}

/* Optimising the functions that Graphics::drawImagePattern would call,
 * so that glBegin...glEnd are outside the main loop. */
void OpenGL1Graphics::drawImagePattern(const Image *image, int x, int y,
                                       int w, int h)
{
    if (!image)
        return;

    const int srcX = image->mBounds.x;
    const int srcY = image->mBounds.y;

    const int iw = image->mBounds.w;
    const int ih = image->mBounds.h;
    if (iw == 0 || ih == 0)
        return;

    glColor4f(1.0f, 1.0f, 1.0f, image->mAlpha);

    bindTexture(OpenGLImageHelper::mTextureType, image->mGLImage);

    setTexturingAndBlending(true);

    // Draw a set of textured rectangles
    glBegin(GL_QUADS);

    for (int py = 0; py < h; py += ih)
    {
        const int height = (py + ih >= h) ? h - py : ih;
        const int dstY = y + py;
        for (int px = 0; px < w; px += iw)
        {
            int width = (px + iw >= w) ? w - px : iw;
            int dstX = x + px;

            drawQuad(image, srcX, srcY, dstX, dstY, width, height);
        }
    }

    glEnd();

    glColor4ub(static_cast<GLubyte>(mColor.r),
               static_cast<GLubyte>(mColor.g),
               static_cast<GLubyte>(mColor.b),
               static_cast<GLubyte>(mColor.a));
}

void OpenGL1Graphics::drawRescaledImagePattern(Image *image, int x, int y,
                                               int w, int h,
                                               int scaledWidth,
                                               int scaledHeight)
{
    if (!image)
        return;

    const int srcX = image->mBounds.x;
    const int srcY = image->mBounds.y;

    const int iw = scaledWidth;
    const int ih = scaledHeight;
    if (iw == 0 || ih == 0)
        return;

    glColor4f(1.0f, 1.0f, 1.0f, image->mAlpha);

    bindTexture(OpenGLImageHelper::mTextureType, image->mGLImage);

    setTexturingAndBlending(true);

    // Draw a set of textured rectangles
    glBegin(GL_QUADS);

    const float scaleFactorW = static_cast<float>(scaledWidth)
        / image->getWidth();
    const float scaleFactorH = static_cast<float>(scaledHeight)
        / image->getHeight();

    for (int py = 0; py < h; py += ih)
    {
        const int height = (py + ih >= h) ? h - py : ih;
        const int dstY = y + py;
        for (int px = 0; px < w; px += iw)
        {
            int width = (px + iw >= w) ? w - px : iw;
            int dstX = x + px;

            drawRescaledQuad(image, srcX, srcY, dstX, dstY,
                width / scaleFactorW, height / scaleFactorH,
                scaledWidth, scaledHeight);
        }
    }

    glEnd();

    glColor4ub(static_cast<GLubyte>(mColor.r), static_cast<GLubyte>(mColor.g),
        static_cast<GLubyte>(mColor.b), static_cast<GLubyte>(mColor.a));
}

bool OpenGL1Graphics::calcImageRect(GraphicsVertexes* vert,
                                    int x, int y, int w, int h,
                                    Image *topLeft A_UNUSED,
                                    Image *topRight A_UNUSED,
                                    Image *bottomLeft A_UNUSED,
                                    Image *bottomRight A_UNUSED,
                                    Image *top A_UNUSED, Image *right A_UNUSED,
                                    Image *bottom A_UNUSED,
                                    Image *left A_UNUSED,
                                    Image *center A_UNUSED)
{
    if (!vert)
        return false;

    vert->init(x, y, w, h);
    return true;
}

void OpenGL1Graphics::calcTile(ImageVertexes *vert A_UNUSED,
                               int x A_UNUSED, int y A_UNUSED)
{

}

void OpenGL1Graphics::drawTile(ImageVertexes *vert A_UNUSED)
{

}

void OpenGL1Graphics::drawImageRect2(GraphicsVertexes* vert,
                                     const ImageRect &imgRect)
{
    if (!vert)
        return;

    drawImageRect(vert->getX(), vert->getY(), vert->getW(), vert->getH(),
            imgRect.grid[0], imgRect.grid[2], imgRect.grid[6], imgRect.grid[8],
            imgRect.grid[1], imgRect.grid[5], imgRect.grid[7], imgRect.grid[3],
            imgRect.grid[4]);
}

void OpenGL1Graphics::updateScreen()
{
    glFlush();
    glFinish();
    SDL_GL_SwapBuffers();
}

void OpenGL1Graphics::_beginDraw()
{
    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    glOrtho(0.0, static_cast<double>(mTarget->w),
        static_cast<double>(mTarget->h), 0.0, -1.0, 1.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glEnable(GL_SCISSOR_TEST);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    pushClipArea(gcn::Rectangle(0, 0, mTarget->w, mTarget->h));
}

void OpenGL1Graphics::_endDraw()
{
    popClipArea();
}

void OpenGL1Graphics::prepareScreenshot()
{
#if !defined(_WIN32)
    if (config.getBoolValue("usefbo"))
    {
        int h = mTarget->h;
        int w = mTarget->w;

        // create a texture object
        glGenTextures(1, &mTextureId);
        glBindTexture(GL_TEXTURE_2D, mTextureId);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0,
            GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glBindTexture(GL_TEXTURE_2D, 0);

        // create a renderbuffer object to store depth info
        glGenRenderbuffersEXT(1, &mRboId);
        glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, mRboId);
        glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT,
            GL_DEPTH_COMPONENT, w, h);
        glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, 0);

        // create a framebuffer object
        glGenFramebuffersEXT(1, &mFboId);
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, mFboId);

        // attach the texture to FBO color attachment point
        glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
            GL_TEXTURE_2D, mTextureId, 0);

        // attach the renderbuffer to depth attachment point
        glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT,
            GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, mRboId);

        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, mFboId);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
#endif
}

SDL_Surface* OpenGL1Graphics::getScreenshot()
{
    const int h = mTarget->h;
    const int w = mTarget->w - (mTarget->w % 4);
    GLint pack = 1;

    SDL_Surface *screenshot = SDL_CreateRGBSurface(
            SDL_SWSURFACE,
            w, h, 24,
            0xff0000, 0x00ff00, 0x0000ff, 0x000000);

    if (!screenshot)
        return nullptr;

    if (SDL_MUSTLOCK(screenshot))
        SDL_LockSurface(screenshot);

    // Grap the pixel buffer and write it to the SDL surface
    glGetIntegerv(GL_PACK_ALIGNMENT, &pack);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, screenshot->pixels);

    // Flip the screenshot, as OpenGL has 0,0 in bottom left
    unsigned int lineSize = 3 * w;
    GLubyte* buf = static_cast<GLubyte*>(malloc(lineSize));

    for (int i = 0; i < (h / 2); i++)
    {
        GLubyte *top = static_cast<GLubyte*>(
            screenshot->pixels) + lineSize * i;
        GLubyte *bot = static_cast<GLubyte*>(
            screenshot->pixels) + lineSize * (h - 1 - i);

        memcpy(buf, top, lineSize);
        memcpy(top, bot, lineSize);
        memcpy(bot, buf, lineSize);
    }

    free(buf);

#if !defined(_WIN32)
    if (config.getBoolValue("usefbo"))
    {
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
        if (mFboId)
        {
            glDeleteFramebuffersEXT(1, &mFboId);
            mFboId = 0;
        }
        glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, 0);
        if (mRboId)
        {
            glDeleteRenderbuffersEXT(1, &mRboId);
            mRboId = 0;
        }
        if (mTextureId)
        {
            glDeleteTextures(1, &mTextureId);
            mTextureId = 0;
        }
    }
#endif

    glPixelStorei(GL_PACK_ALIGNMENT, pack);

    if (SDL_MUSTLOCK(screenshot))
        SDL_UnlockSurface(screenshot);

    return screenshot;
}

bool OpenGL1Graphics::pushClipArea(gcn::Rectangle area)
{
    int transX = 0;
    int transY = 0;

    if (!mClipStack.empty())
    {
        const gcn::ClipRectangle &clipArea = mClipStack.top();
        transX = -clipArea.xOffset;
        transY = -clipArea.yOffset;
    }

    bool result = gcn::Graphics::pushClipArea(area);

    const gcn::ClipRectangle &clipArea = mClipStack.top();
    transX += clipArea.xOffset;
    transY += clipArea.yOffset;

    glPushMatrix();
    glTranslatef(static_cast<GLfloat>(transX),
                 static_cast<GLfloat>(transY), 0);
    glScissor(clipArea.x, mTarget->h - clipArea.y - clipArea.height,
        clipArea.width, clipArea.height);

    return result;
}

void OpenGL1Graphics::popClipArea()
{
    gcn::Graphics::popClipArea();

    if (mClipStack.empty())
        return;

    glPopMatrix();
    const gcn::ClipRectangle &clipArea = mClipStack.top();
    glScissor(clipArea.x, mTarget->h - clipArea.y - clipArea.height,
        clipArea.width, clipArea.height);
}

void OpenGL1Graphics::setColor(const gcn::Color& color)
{
    mColor = color;
    glColor4ub(static_cast<GLubyte>(color.r),
               static_cast<GLubyte>(color.g),
               static_cast<GLubyte>(color.b),
               static_cast<GLubyte>(color.a));

    mColorAlpha = (color.a != 255);
}

void OpenGL1Graphics::drawPoint(int x, int y)
{
    setTexturingAndBlending(false);

    glBegin(GL_POINTS);
    glVertex2i(x, y);
    glEnd();
}

void OpenGL1Graphics::drawLine(int x1, int y1, int x2, int y2)
{
    setTexturingAndBlending(false);

    glBegin(GL_LINES);
    glVertex2f(static_cast<float>(x1) + 0.5f, static_cast<float>(y1) + 0.5f);
    glVertex2f(static_cast<float>(x2) + 0.5f, static_cast<float>(y2) + 0.5f);
    glEnd();

    glBegin(GL_POINTS);
    glVertex2f(static_cast<float>(x2) + 0.5f, static_cast<float>(y2) + 0.5f);
    glEnd();
}

void OpenGL1Graphics::drawRectangle(const gcn::Rectangle& rect)
{
    drawRectangle(rect, false);
}

void OpenGL1Graphics::fillRectangle(const gcn::Rectangle& rect)
{
    drawRectangle(rect, true);
}

void OpenGL1Graphics::setTargetPlane(int width A_UNUSED, int height A_UNUSED)
{
}

void OpenGL1Graphics::setTexturingAndBlending(bool enable)
{
    if (enable)
    {
        if (!mTexture)
        {
            glEnable(OpenGLImageHelper::mTextureType);
            mTexture = true;
        }

        if (!mAlpha)
        {
            glEnable(GL_BLEND);
            mAlpha = true;
        }
    }
    else
    {
        mLastImage = 0;
        if (mAlpha && !mColorAlpha)
        {
            glDisable(GL_BLEND);
            mAlpha = false;
        }
        else if (!mAlpha && mColorAlpha)
        {
            glEnable(GL_BLEND);
            mAlpha = true;
        }

        if (mTexture)
        {
            glDisable(OpenGLImageHelper::mTextureType);
            mTexture = false;
        }
    }
}

void OpenGL1Graphics::drawRectangle(const gcn::Rectangle& rect, bool filled)
{
    const float offset = filled ? 0 : 0.5f;

    setTexturingAndBlending(false);

    glBegin(filled ? GL_QUADS : GL_LINE_LOOP);
    glVertex2f(static_cast<float>(rect.x) + offset,
        static_cast<float>(rect.y) + offset);
    glVertex2f(static_cast<float>(rect.x + rect.width) - offset,
        static_cast<float>(rect.y) + offset);
    glVertex2f(static_cast<float>(rect.x + rect.width) - offset,
        static_cast<float>(rect.y + rect.height) - offset);
    glVertex2f(static_cast<float>(rect.x) + offset,
        static_cast<float>(rect.y + rect.height) - offset);
    glEnd();
}

void OpenGL1Graphics::bindTexture(GLenum target, GLuint texture)
{
    if (mLastImage != texture)
    {
        mLastImage = texture;
        glBindTexture(target, texture);
    }
}

#endif // USE_OPENGL
