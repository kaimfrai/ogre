/*
 -----------------------------------------------------------------------------
 This source file is part of OGRE
 (Object-oriented Graphics Rendering Engine)
 For the latest info, see http://www.ogre3d.org/
 
 Copyright (c) 2000-2014 Torus Knot Software Ltd
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 -----------------------------------------------------------------------------
 */
module;

#include "glad/glad.h"
#include <GL/gl.h>

module Ogre.RenderSystems.GL;

import :RenderSystem;
import :StateCacheManager;

import Ogre.Core;

namespace Ogre {
    
    GLStateCacheManager::GLStateCacheManager()
    {
        clearCache();
    }
    
    void GLStateCacheManager::initializeCache()
    {
        glBlendEquation(GL_FUNC_ADD);

        if(GLAD_GL_VERSION_2_0)
        {
            glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
        }
        else if(GLAD_GL_EXT_blend_equation_separate)
        {
            glBlendEquationSeparateEXT(GL_FUNC_ADD, GL_FUNC_ADD);
        }

        glBlendFunc(GL_ONE, GL_ZERO);
        
        glCullFace(mCullFace);

        glDepthFunc(mDepthFunc);

        glDepthMask(mDepthMask);

        glStencilMask(mStencilMask);

        glClearDepth(mClearDepth);

        glBindTexture(GL_TEXTURE_2D, 0);

        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        glBindFramebufferEXT(GL_FRAMEBUFFER, 0);

        glBindRenderbufferEXT(GL_RENDERBUFFER, 0);

        glActiveTexture(GL_TEXTURE0);

        glClearColor(mClearColour[0], mClearColour[1], mClearColour[2], mClearColour[3]);

        glColorMask(mColourMask[0], mColourMask[1], mColourMask[2], mColourMask[3]);

        glPolygonMode(GL_FRONT_AND_BACK, mPolygonMode);
    }

    void GLStateCacheManager::clearCache()
    {
        mDepthMask = GL_TRUE;
        mBlendEquationRGB = GL_FUNC_ADD;
        mBlendEquationAlpha = GL_FUNC_ADD;
        mCullFace = GL_BACK;
        mDepthFunc = GL_LESS;
        mStencilMask = 0xFFFFFFFF;
        mActiveTextureUnit = 0;
        mClearDepth = 1.0f;
        mLastBoundTexID = 0;
        mShininess = 0.0f;
        mPolygonMode = GL_FILL;
        mShadeModel = GL_SMOOTH;

        // Initialize our cache variables and also the GL so that the
        // stored values match the GL state
        mBlendFuncSource = GL_ONE;
        mBlendFuncDest = GL_ZERO;
        mBlendFuncSourceAlpha = GL_ONE;
        mBlendFuncDestAlpha = GL_ZERO;
        
        mClearColour[0] = mClearColour[1] = mClearColour[2] = mClearColour[3] = 0.0f;
        mColourMask[0] = mColourMask[1] = mColourMask[2] = mColourMask[3] = GL_TRUE;

        mActiveBufferMap.clear();
        mTexUnitsMap.clear();
        mTextureCoordGen.clear();

        mAmbient[0] = 0.2f;
        mAmbient[1] = 0.2f;
        mAmbient[2] = 0.2f;
        mAmbient[3] = 1.0f;

        mDiffuse[0] = 0.8f;
        mDiffuse[1] = 0.8f;
        mDiffuse[2] = 0.8f;
        mDiffuse[3] = 1.0f;

        mSpecular[0] = 0.0f;
        mSpecular[1] = 0.0f;
        mSpecular[2] = 0.0f;
        mSpecular[3] = 1.0f;

        mEmissive[0] = 0.0f;
        mEmissive[1] = 0.0f;
        mEmissive[2] = 0.0f;
        mEmissive[3] = 1.0f;

        mLightAmbient[0] = 0.2f;
        mLightAmbient[1] = 0.2f;
        mLightAmbient[2] = 0.2f;
        mLightAmbient[3] = 1.0f;

        mPointSize = 1.0f;
        mPointSizeMin = 1.0f;
        mPointSizeMax = 1.0f;
        mPointAttenuation[0] = 1.0f;
        mPointAttenuation[1] = 0.0f;
        mPointAttenuation[2] = 0.0f;
    }

    void GLStateCacheManager::bindGLBuffer(GLenum target, GLuint buffer, bool force)
    {
        if(target == GL_FRAMEBUFFER)
        {
            OgreAssert(false, "not implemented");
        }
        else if(target == GL_RENDERBUFFER)
        {
            glBindRenderbufferEXT(target, buffer);
        }
        else
        {
            glBindBuffer(target, buffer);
        }
    }

    void GLStateCacheManager::deleteGLBuffer(GLenum target, GLuint buffer)
    {
        // Buffer name 0 is reserved and we should never try to delete it
        if(buffer == 0)
            return;
        
        if(target == GL_FRAMEBUFFER)
        {
            glDeleteFramebuffers(1, &buffer);
        }
        else if(target == GL_RENDERBUFFER)
        {
            glDeleteRenderbuffers(1, &buffer);
        }
        else
        {
            glDeleteBuffers(1, &buffer);
        }
    }

    void GLStateCacheManager::invalidateStateForTexture(GLuint texture)
    {
    }

    // TODO: Store as high/low bits of a GLuint, use vector instead of map for TexParameteriMap
    void GLStateCacheManager::setTexParameteri(GLenum target, GLenum pname, GLint param)
    {
        glTexParameteri(target, pname, param);
    }
    
    void GLStateCacheManager::bindGLTexture(GLenum target, GLuint texture)
    {
        mLastBoundTexID = texture;
        
        // Update GL
        glBindTexture(target, texture);
    }
    
    auto GLStateCacheManager::activateGLTextureUnit(size_t unit) -> bool
    {
        if (unit >= Root::getSingleton().getRenderSystem()->getCapabilities()->getNumTextureUnits())
            return false;

        glActiveTexture(GL_TEXTURE0 + unit);
        mActiveTextureUnit = unit;
        return true;
    }

    void GLStateCacheManager::setBlendFunc(GLenum source, GLenum dest, GLenum sourceA, GLenum destA)
    {
        mBlendFuncSource = source;
        mBlendFuncDest = dest;
        mBlendFuncSourceAlpha = sourceA;
        mBlendFuncDestAlpha = destA;

        glBlendFuncSeparate(source, dest, sourceA, destA);
    }

    void GLStateCacheManager::setDepthMask(GLboolean mask)
    {
        mDepthMask = mask;

        glDepthMask(mask);
    }
    
    void GLStateCacheManager::setDepthFunc(GLenum func)
    {
        mDepthFunc = func;

        glDepthFunc(func);
    }
    
    void GLStateCacheManager::setClearDepth(GLclampf depth)
    {
        mClearDepth = depth;

        glClearDepth(depth);
    }
    
    void GLStateCacheManager::setClearColour(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
    {
        mClearColour[0] = red;
        mClearColour[1] = green;
        mClearColour[2] = blue;
        mClearColour[3] = alpha;

        glClearColor(mClearColour[0], mClearColour[1], mClearColour[2], mClearColour[3]);
    }
    
    void GLStateCacheManager::setColourMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)
    {
        mColourMask[0] = red;
        mColourMask[1] = green;
        mColourMask[2] = blue;
        mColourMask[3] = alpha;

        glColorMask(mColourMask[0], mColourMask[1], mColourMask[2], mColourMask[3]);
    }
    
    void GLStateCacheManager::setStencilMask(GLuint mask)
    {
        mStencilMask = mask;

        glStencilMask(mask);
    }
    
    void GLStateCacheManager::setEnabled(GLenum flag, bool enabled)
    {
        if(!enabled)
        {
            glDisable(flag);
        }
        else
        {
            glEnable(flag);
        }
    }

    void GLStateCacheManager::setViewport(const Rect& r)
    {
        mViewport = r;
        glViewport(r.left, r.top, r.width(), r.height());
    }

    void GLStateCacheManager::setCullFace(GLenum face)
    {
        mCullFace = face;

        glCullFace(face);
    }

    void GLStateCacheManager::setBlendEquation(GLenum eqRGB, GLenum eqAlpha)
    {
        mBlendEquationRGB = eqRGB;
        mBlendEquationAlpha = eqAlpha;

        if(GLAD_GL_VERSION_2_0)
        {
            glBlendEquationSeparate(eqRGB, eqAlpha);
        }
        else if(GLAD_GL_EXT_blend_equation_separate)
        {
            glBlendEquationSeparateEXT(eqRGB, eqAlpha);
        }
        else
        {
            glBlendEquation(eqRGB);
        }
    }

    void GLStateCacheManager::setMaterialDiffuse(GLfloat r, GLfloat g, GLfloat b, GLfloat a)
    {
        mDiffuse[0] = r;
        mDiffuse[1] = g;
        mDiffuse[2] = b;
        mDiffuse[3] = a;

        glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, &mDiffuse[0]);
    }

    void GLStateCacheManager::setMaterialAmbient(GLfloat r, GLfloat g, GLfloat b, GLfloat a)
    {
        mAmbient[0] = r;
        mAmbient[1] = g;
        mAmbient[2] = b;
        mAmbient[3] = a;

        glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, &mAmbient[0]);
    }

    void GLStateCacheManager::setMaterialEmissive(GLfloat r, GLfloat g, GLfloat b, GLfloat a)
    {
        mEmissive[0] = r;
        mEmissive[1] = g;
        mEmissive[2] = b;
        mEmissive[3] = a;

        glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, &mEmissive[0]);
    }

    void GLStateCacheManager::setMaterialSpecular(GLfloat r, GLfloat g, GLfloat b, GLfloat a)
    {
        mSpecular[0] = r;
        mSpecular[1] = g;
        mSpecular[2] = b;
        mSpecular[3] = a;

        glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, &mSpecular[0]);
    }

    void GLStateCacheManager::setMaterialShininess(GLfloat shininess)
    {
        mShininess = shininess;
        glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, mShininess);
    }

    void GLStateCacheManager::setPolygonMode(GLenum mode)
    {
        mPolygonMode = mode;
        glPolygonMode(GL_FRONT_AND_BACK, mPolygonMode);
    }

    void GLStateCacheManager::setShadeModel(GLenum model)
    {
        mShadeModel = model;
        glShadeModel(model);
    }

    void GLStateCacheManager::setLightAmbient(GLfloat r, GLfloat g, GLfloat b)
    {
        mLightAmbient[0] = r;
        mLightAmbient[1] = g;
        mLightAmbient[2] = b;

        glLightModelfv(GL_LIGHT_MODEL_AMBIENT, &mLightAmbient[0]);
    }

    void GLStateCacheManager::setPointSize(GLfloat size)
    {
        mPointSize = size;
        glPointSize(mPointSize);
    }

    void GLStateCacheManager::setPointParameters(const GLfloat *attenuation, float minSize, float maxSize)
    {
        if(minSize > -1)
        {
            mPointSizeMin = minSize;
            const Ogre::RenderSystemCapabilities* caps = dynamic_cast<GLRenderSystem*>(Root::getSingleton().getRenderSystem())->getCapabilities();
            if (caps->hasCapability(Capabilities::POINT_EXTENDED_PARAMETERS))
                glPointParameterf(GL_POINT_SIZE_MIN, mPointSizeMin);
        }

        if(maxSize > -1)
        {
            mPointSizeMax = maxSize;
            const Ogre::RenderSystemCapabilities* caps = dynamic_cast<GLRenderSystem*>(Root::getSingleton().getRenderSystem())->getCapabilities();
            if (caps->hasCapability(Capabilities::POINT_EXTENDED_PARAMETERS))
                glPointParameterf(GL_POINT_SIZE_MAX, mPointSizeMax);
        }

        if(attenuation)
        {
            mPointAttenuation[0] = attenuation[0];
            mPointAttenuation[1] = attenuation[1];
            mPointAttenuation[2] = attenuation[2];
            const Ogre::RenderSystemCapabilities* caps = dynamic_cast<GLRenderSystem*>(Root::getSingleton().getRenderSystem())->getCapabilities();
            if (caps->hasCapability(Capabilities::POINT_EXTENDED_PARAMETERS))
                glPointParameterfv(GL_POINT_DISTANCE_ATTENUATION, &mPointAttenuation[0]);
        }
    }

    void GLStateCacheManager::enableTextureCoordGen(GLenum type)
    {
        glEnable(type);
    }

    void GLStateCacheManager::disableTextureCoordGen(GLenum type)
    {
        glDisable(type);
    }
}
