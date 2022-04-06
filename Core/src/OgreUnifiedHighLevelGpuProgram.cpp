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
#include <string>
#include <vector>

#include "OgreException.hpp"
#include "OgreGpuProgramManager.hpp"
#include "OgreGpuProgramParams.hpp"
#include "OgreLogManager.hpp"
#include "OgreResourceGroupManager.hpp"
#include "OgreStringInterface.hpp"
#include "OgreUnifiedHighLevelGpuProgram.hpp"

namespace Ogre
{
class ResourceManager;

    //-----------------------------------------------------------------------
    /// Command object for setting delegate (can set more than once)
    class CmdDelegate : public ParamCommand
    {
    public:
        auto doGet(const void* target) const -> String;
        void doSet(void* target, const String& val);
    };
    static CmdDelegate msCmdDelegate;
    static const String sLanguage = "unified";

    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    UnifiedHighLevelGpuProgram::UnifiedHighLevelGpuProgram(
        ResourceManager* creator, const String& name, ResourceHandle handle,
        const String& group, bool isManual, ManualResourceLoader* loader)
        :GpuProgram(creator, name, handle, group, isManual, loader)
    {
        if (createParamDictionary("UnifiedHighLevelGpuProgram"))
        {
            setupBaseParamDictionary();

            ParamDictionary* dict = getParamDictionary();

            dict->addParameter(ParameterDef("delegate", 
                "Additional delegate programs containing implementations.",
                PT_STRING),&msCmdDelegate);
        }

    }
    //-----------------------------------------------------------------------
    UnifiedHighLevelGpuProgram::~UnifiedHighLevelGpuProgram()
    {

    }
    //-----------------------------------------------------------------------
    void UnifiedHighLevelGpuProgram::chooseDelegate() const
    {
        mChosenDelegate.reset();

        for (const String& dn : mDelegateNames)
        {
            GpuProgramPtr deleg = GpuProgramManager::getSingleton().getByName(dn, mGroup);

            //recheck with auto resource group
            if (!deleg)
                deleg = GpuProgramManager::getSingleton().getByName(dn, RGN_AUTODETECT);

            // Silently ignore missing links
            if(!deleg || (!deleg->isSupported() && !deleg->hasCompileError()))
                continue;

            if (deleg->getType() != getType())
            {
                LogManager::getSingleton().logError("unified program '" + getName() +
                                                    "' delegating to program with different type '" + dn + "'");
                continue;
            }

            mChosenDelegate = deleg;
            break;
        }
    }
    //-----------------------------------------------------------------------
    auto UnifiedHighLevelGpuProgram::_getDelegate() const -> const GpuProgramPtr&
    {
        if (!mChosenDelegate)
        {
            chooseDelegate();
        }
        return mChosenDelegate;
    }
    //-----------------------------------------------------------------------
    void UnifiedHighLevelGpuProgram::addDelegateProgram(const String& name)
    {
        mDelegateNames.push_back(name);

        // reset chosen delegate
        mChosenDelegate.reset();
    }
    //-----------------------------------------------------------------------
    void UnifiedHighLevelGpuProgram::clearDelegatePrograms()
    {
        mDelegateNames.clear();
        mChosenDelegate.reset();
    }
    //-----------------------------------------------------------------------------
    auto UnifiedHighLevelGpuProgram::calculateSize() const -> size_t
    {
        size_t memSize = 0;

        memSize += GpuProgram::calculateSize();

        // Delegate Names
        for (StringVector::const_iterator i = mDelegateNames.begin(); i != mDelegateNames.end(); ++i)
            memSize += (*i).size() * sizeof(char);

        return memSize;
    }
    //-----------------------------------------------------------------------
    auto UnifiedHighLevelGpuProgram::getLanguage() const -> const String&
    {
        return sLanguage;
    }
    //-----------------------------------------------------------------------
    auto UnifiedHighLevelGpuProgram::createParameters() -> GpuProgramParametersSharedPtr
    {
        if (isSupported())
        {
            return _getDelegate()->createParameters();
        }
        else
        {
            // return a default set
            GpuProgramParametersSharedPtr params = GpuProgramManager::getSingleton().createParameters();
            // avoid any errors on parameter names that don't exist
            params->setIgnoreMissingParams(true);
            return params;
        }
    }
    //-----------------------------------------------------------------------
    auto UnifiedHighLevelGpuProgram::_getBindingDelegate() -> GpuProgram*
    {
        if (_getDelegate())
            return _getDelegate()->_getBindingDelegate();
        else
            return 0;
    }
    //-----------------------------------------------------------------------
    auto UnifiedHighLevelGpuProgram::isSupported() const -> bool
    {
        // Supported if one of the delegates is
        return _getDelegate() && _getDelegate()->isSupported();
    }
    //-----------------------------------------------------------------------
    auto UnifiedHighLevelGpuProgram::isSkeletalAnimationIncluded() const -> bool
    {
        if (_getDelegate())
            return _getDelegate()->isSkeletalAnimationIncluded();
        else
            return false;
    }
    //-----------------------------------------------------------------------
    auto UnifiedHighLevelGpuProgram::isMorphAnimationIncluded() const -> bool
    {
        if (_getDelegate())
            return _getDelegate()->isMorphAnimationIncluded();
        else
            return false;
    }
    //-----------------------------------------------------------------------
    auto UnifiedHighLevelGpuProgram::isPoseAnimationIncluded() const -> bool
    {
        if (_getDelegate())
            return _getDelegate()->isPoseAnimationIncluded();
        else
            return false;
    }
    //-----------------------------------------------------------------------
    auto UnifiedHighLevelGpuProgram::getNumberOfPosesIncluded() const -> ushort
    {
        if (_getDelegate())
            return _getDelegate()->getNumberOfPosesIncluded();
        else
            return 0;
    }
    //-----------------------------------------------------------------------
    auto UnifiedHighLevelGpuProgram::isVertexTextureFetchRequired() const -> bool
    {
        if (_getDelegate())
            return _getDelegate()->isVertexTextureFetchRequired();
        else
            return false;
    }
    //-----------------------------------------------------------------------
    auto UnifiedHighLevelGpuProgram::getDefaultParameters() -> const GpuProgramParametersPtr&
    {
        if (_getDelegate())
            return _getDelegate()->getDefaultParameters();

        static GpuProgramParametersSharedPtr nullPtr;
        return nullPtr;
    }
    //-----------------------------------------------------------------------
    auto UnifiedHighLevelGpuProgram::hasDefaultParameters() const -> bool
    {
        if (_getDelegate())
            return _getDelegate()->hasDefaultParameters();
        else
            return false;
    }
    //-----------------------------------------------------------------------
    auto UnifiedHighLevelGpuProgram::getPassSurfaceAndLightStates() const -> bool
    {
        if (_getDelegate())
            return _getDelegate()->getPassSurfaceAndLightStates();
        else
            return GpuProgram::getPassSurfaceAndLightStates();
    }
    //---------------------------------------------------------------------
    auto UnifiedHighLevelGpuProgram::getPassFogStates() const -> bool
    {
        if (_getDelegate())
            return _getDelegate()->getPassFogStates();
        else
            return GpuProgram::getPassFogStates();
    }
    //---------------------------------------------------------------------
    auto UnifiedHighLevelGpuProgram::getPassTransformStates() const -> bool
    {
        if (_getDelegate())
            return _getDelegate()->getPassTransformStates();
        else
            return GpuProgram::getPassTransformStates();

    }
    //-----------------------------------------------------------------------
    auto UnifiedHighLevelGpuProgram::hasCompileError() const -> bool
    {
        if (!_getDelegate())
        {
            return false;
        }
        else
        {
            return _getDelegate()->hasCompileError();
        }
    }
    //-----------------------------------------------------------------------
    void UnifiedHighLevelGpuProgram::resetCompileError()
    {
        if (_getDelegate())
            _getDelegate()->resetCompileError();
    }
    //-----------------------------------------------------------------------
    void UnifiedHighLevelGpuProgram::load(bool backgroundThread)
    {
        if (_getDelegate())
            _getDelegate()->load(backgroundThread);
    }
    //-----------------------------------------------------------------------
    void UnifiedHighLevelGpuProgram::reload(LoadingFlags flags)
    {
        if (_getDelegate())
            _getDelegate()->reload(flags);
    }
    //-----------------------------------------------------------------------
    auto UnifiedHighLevelGpuProgram::isReloadable() const -> bool
    {
        if (_getDelegate())
            return _getDelegate()->isReloadable();
        else
            return true;
    }
    //-----------------------------------------------------------------------
    void UnifiedHighLevelGpuProgram::unload()
    {
        if (_getDelegate())
            _getDelegate()->unload();
    }
    //-----------------------------------------------------------------------
    auto UnifiedHighLevelGpuProgram::isLoaded() const -> bool
    {
        if (_getDelegate())
            return _getDelegate()->isLoaded();
        else
            return false;
    }
    //-----------------------------------------------------------------------
    auto UnifiedHighLevelGpuProgram::isLoading() const -> bool
    {
        if (_getDelegate())
            return _getDelegate()->isLoading();
        else
            return false;
    }
    //-----------------------------------------------------------------------
    auto UnifiedHighLevelGpuProgram::getLoadingState() const -> Resource::LoadingState
    {
        if (_getDelegate())
            return _getDelegate()->getLoadingState();
        else
            return Resource::LOADSTATE_UNLOADED;
    }
    //-----------------------------------------------------------------------
    auto UnifiedHighLevelGpuProgram::getSize() const -> size_t
    {
        if (_getDelegate())
            return _getDelegate()->getSize();
        else
            return 0;
    }
    //-----------------------------------------------------------------------
    void UnifiedHighLevelGpuProgram::touch()
    {
        if (_getDelegate())
            _getDelegate()->touch();
    }
    //-----------------------------------------------------------------------
    auto UnifiedHighLevelGpuProgram::isBackgroundLoaded() const -> bool
    {
        if (_getDelegate())
            return _getDelegate()->isBackgroundLoaded();
        else
            return false;
    }
    //-----------------------------------------------------------------------
    void UnifiedHighLevelGpuProgram::setBackgroundLoaded(bool bl)
    {
        if (_getDelegate())
            _getDelegate()->setBackgroundLoaded(bl);
    }
    //-----------------------------------------------------------------------
    void UnifiedHighLevelGpuProgram::escalateLoading()
    {
        if (_getDelegate())
            _getDelegate()->escalateLoading();
    }
    //-----------------------------------------------------------------------
    void UnifiedHighLevelGpuProgram::addListener(Resource::Listener* lis)
    {
        if (_getDelegate())
            _getDelegate()->addListener(lis);
    }
    //-----------------------------------------------------------------------
    void UnifiedHighLevelGpuProgram::removeListener(Resource::Listener* lis)
    {
        if (_getDelegate())
            _getDelegate()->removeListener(lis);
    }
    //-----------------------------------------------------------------------
    void UnifiedHighLevelGpuProgram::createLowLevelImpl()
    {
        OGRE_EXCEPT(Exception::ERR_NOT_IMPLEMENTED, 
            "This method should never get called!",
            "UnifiedHighLevelGpuProgram::createLowLevelImpl");
    }
    //-----------------------------------------------------------------------
    void UnifiedHighLevelGpuProgram::unloadHighLevelImpl()
    {
        OGRE_EXCEPT(Exception::ERR_NOT_IMPLEMENTED, 
            "This method should never get called!",
            "UnifiedHighLevelGpuProgram::unloadHighLevelImpl");
    }
    //-----------------------------------------------------------------------
    void UnifiedHighLevelGpuProgram::loadFromSource()
    {
        OGRE_EXCEPT(Exception::ERR_NOT_IMPLEMENTED, 
            "This method should never get called!",
            "UnifiedHighLevelGpuProgram::loadFromSource");
    }
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    auto CmdDelegate::doGet(const void* target) const -> String
    {
        // Can't do this (not one delegate), shouldn't matter
        return BLANKSTRING;
    }
    //-----------------------------------------------------------------------
    void CmdDelegate::doSet(void* target, const String& val)
    {
        static_cast<UnifiedHighLevelGpuProgram*>(target)->addDelegateProgram(val);
    }
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    UnifiedHighLevelGpuProgramFactory::UnifiedHighLevelGpuProgramFactory()
    {
    }
    //-----------------------------------------------------------------------
    UnifiedHighLevelGpuProgramFactory::~UnifiedHighLevelGpuProgramFactory()
    {
    }
    //-----------------------------------------------------------------------
    auto UnifiedHighLevelGpuProgramFactory::getLanguage() const -> const String&
    {
        return sLanguage;
    }
    //-----------------------------------------------------------------------
    auto UnifiedHighLevelGpuProgramFactory::create(ResourceManager* creator,
        const String& name, ResourceHandle handle,
        const String& group, bool isManual, ManualResourceLoader* loader) -> GpuProgram*
    {
        return new UnifiedHighLevelGpuProgram(creator, name, handle, group, isManual, loader);
    }
}

