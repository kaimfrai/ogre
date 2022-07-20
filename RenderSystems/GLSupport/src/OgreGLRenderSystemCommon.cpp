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
module Ogre.RenderSystems.GLSupport;

import :GLContext;
import :GLNativeSupport;
import :GLRenderSystemCommon;
import :GLRenderTexture;

import Ogre.Core;

import <algorithm>;
import <format>;
import <map>;
import <string>;
import <utility>;
import <vector>;

namespace Ogre {
    static void removeDuplicates(std::vector<String>& c)
    {
        std::ranges::sort(c);
        auto p = std::unique(c.begin(), c.end());
        c.erase(p, c.end());
    }

    auto VideoMode::getDescription() const -> String
    {
        return std::format("{:4} x {:4}", width, height);
    }

    void GLRenderSystemCommon::initConfigOptions()
    {
        mOptions = mGLSupport->getConfigOptions();

        RenderSystem::initConfigOptions();

        ConfigOption optDisplayFrequency;
        optDisplayFrequency.name = "Display Frequency";
        optDisplayFrequency.immutable = false;
        mOptions["Display Frequency"] = optDisplayFrequency;

        ConfigOption optVideoMode;
        optVideoMode.name = "Video Mode";
        optVideoMode.immutable = false;
        for (const auto& mode : mGLSupport->getVideoModes())
        {
            optVideoMode.possibleValues.push_back(mode.getDescription());
        }
        removeDuplicates(optVideoMode.possibleValues); // also sorts
        optVideoMode.currentValue = optVideoMode.possibleValues[0];
        mOptions["Video Mode"] = optVideoMode;

        ConfigOption optFSAA;
        optFSAA.name = "FSAA";
        optFSAA.immutable = false;
        for (int sampleLevel : mGLSupport->getFSAALevels())
        {
            optFSAA.possibleValues.push_back(StringConverter::toString(sampleLevel));
        }
        if (!optFSAA.possibleValues.empty())
        {
            removeDuplicates(optFSAA.possibleValues);
            optFSAA.currentValue = optFSAA.possibleValues[0];
        }
        mOptions["FSAA"] = optFSAA;

        refreshConfig();
    }

    void GLRenderSystemCommon::refreshConfig()
    {
        // set bpp and refresh rate as appropriate
        auto optVideoMode = mOptions.find("Video Mode");
        auto optDisplayFrequency = mOptions.find("Display Frequency");
        auto optFullScreen = mOptions.find("Full Screen");
        auto optColourDepth = mOptions.find("Colour Depth");

        // coulour depth is optional
        if (optColourDepth != mOptions.end())
        {
            for (const auto& mode : mGLSupport->getVideoModes())
            {
                if (mode.getDescription() == optVideoMode->second.currentValue)
                {
                    optColourDepth->second.possibleValues.push_back(
                        StringConverter::toString((unsigned int)mode.bpp));
                }
            }

            removeDuplicates(optColourDepth->second.possibleValues);
        }

        // we can only set refresh rate in full screen mode
        bool isFullscreen = false;
        if( optFullScreen != mOptions.end())
            isFullscreen = optFullScreen->second.currentValue == "Yes";

        if (optVideoMode == mOptions.end() || optDisplayFrequency == mOptions.end())
            return;

        optDisplayFrequency->second.possibleValues.clear();
        if (isFullscreen)
        {
            for (const auto& mode : mGLSupport->getVideoModes())
            {
                if (mode.getDescription() == optVideoMode->second.currentValue)
                {
                    String frequency = ::std::format("{} Hz", mode.refreshRate);
                    optDisplayFrequency->second.possibleValues.push_back(frequency);

                    if(optColourDepth != mOptions.end())
                        optColourDepth->second.possibleValues.push_back(
                            StringConverter::toString((unsigned int)mode.bpp));
                }
            }

            removeDuplicates(optDisplayFrequency->second.possibleValues);
        }

        if (optDisplayFrequency->second.possibleValues.empty())
        {
            optDisplayFrequency->second.possibleValues.push_back("N/A");
            optDisplayFrequency->second.immutable = true;
        }

        optDisplayFrequency->second.currentValue = optDisplayFrequency->second.possibleValues.front();
    }

    //-------------------------------------------------------------------------------------------------//
    void GLRenderSystemCommon::setConfigOption(std::string_view name, std::string_view value)
    {
        auto option = mOptions.find(name);
        if (option == mOptions.end()) {
            OGRE_EXCEPT(ExceptionCodes::INVALIDPARAMS, ::std::format("Option named '{}' does not exist.", name ));
        }
        option->second.currentValue = value;

        if( name == "Video Mode" || name == "Full Screen" )
            refreshConfig();
    }

    auto GLRenderSystemCommon::checkExtension(std::string_view ext) const -> bool
    {
        return mExtensionList.find(ext) != mExtensionList.end() || mGLSupport->checkExtension(ext);
    }

    auto GLRenderSystemCommon::hasMinGLVersion(int major, int minor) const -> bool
    {
        if (mDriverVersion.major == major) {
            return mDriverVersion.minor >= minor;
        }
        return mDriverVersion.major > major;
    }

    void GLRenderSystemCommon::_completeDeferredVaoFboDestruction()
    {
        if(GLContext* ctx = mCurrentContext)
        {
            std::vector<uint32>& vaos = ctx->_getVaoDeferredForDestruction();
            while(!vaos.empty())
            {
                _destroyVao(ctx, vaos.back());
                vaos.pop_back();
            }
            
            std::vector<uint32>& fbos = ctx->_getFboDeferredForDestruction();
            while(!fbos.empty())
            {
                _destroyFbo(ctx, fbos.back());
                fbos.pop_back();
            }

        }
    }

    void GLRenderSystemCommon::_convertProjectionMatrix(const Matrix4& matrix, Matrix4& dest, bool)
    {
        // no conversion required for OpenGL
        dest = matrix;

        if (mIsReverseDepthBufferEnabled)
        {
            // Convert depth range from [-1,+1] to [1,0]
            dest[2][0] = (dest[2][0] - dest[3][0]) * -0.5f;
            dest[2][1] = (dest[2][1] - dest[3][1]) * -0.5f;
            dest[2][2] = (dest[2][2] - dest[3][2]) * -0.5f;
            dest[2][3] = (dest[2][3] - dest[3][3]) * -0.5f;
        }
    }

    void GLRenderSystemCommon::_getDepthStencilFormatFor(PixelFormat internalColourFormat,
                                                         uint32* depthFormat, uint32* stencilFormat)
    {
        mRTTManager->getBestDepthStencil( internalColourFormat, depthFormat, stencilFormat );
    }

    auto GLRenderSystemCommon::getDisplayMonitorCount() const noexcept -> unsigned int
    {
        return mGLSupport->getDisplayMonitorCount();
    }

    void GLRenderSystemCommon::registerThread()
    {
        // This is only valid once we've created the main context
        if (!mMainContext)
        {
            OGRE_EXCEPT(ExceptionCodes::INVALIDPARAMS,
                        "Cannot register a background thread before the main context has been created");
        }

        // Create a new context for this thread. Cloning from the main context
        // will ensure that resources are shared with the main context
        // We want a separate context so that we can safely create GL
        // objects in parallel with the main thread
        GLContext* newContext = mMainContext->clone();
        mBackgroundContextList.push_back(newContext);

        // Bind this new context to this thread.
        newContext->setCurrent();

        _oneTimeContextInitialization();
        newContext->setInitialized();
    }

    void GLRenderSystemCommon::unregisterThread()
    {
        // nothing to do here?
        // Don't need to worry about active context, just make sure we delete
        // on shutdown.
    }

    void GLRenderSystemCommon::preExtraThreadsStarted()
    {
        // free context, we'll need this to share lists
        if (mCurrentContext)
            mCurrentContext->endCurrent();
    }

    void GLRenderSystemCommon::postExtraThreadsStarted()
    {
        // reacquire context
        if (mCurrentContext)
            mCurrentContext->setCurrent();
    }
}
