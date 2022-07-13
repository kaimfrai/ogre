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
#include <memory>
#include <string>

#include "OgreException.hpp"
#include "OgreGpuProgramManager.hpp"
#include "OgreGpuProgramParams.hpp"
#include "OgreGpuProgramUsage.hpp"
#include "OgrePass.hpp"
#include "OgreResourceGroupManager.hpp"

namespace Ogre
{
    //-----------------------------------------------------------------------------
    GpuProgramUsage::GpuProgramUsage(GpuProgramType gptype, Pass* parent) :
        mParent(parent), mProgram(), mRecreateParams(false),mType(gptype)
    {
    }
    //-----------------------------------------------------------------------------
    GpuProgramUsage::GpuProgramUsage(const GpuProgramUsage& oth, Pass* parent)
        : mParent(parent)
        , mProgram(oth.mProgram)
        // nfz: parameters should be copied not just use a shared ptr to the original
        , mParameters(new GpuProgramParameters(*oth.mParameters))
        , mRecreateParams(false)
        , mType(oth.mType)
    {
    }
    //---------------------------------------------------------------------
    GpuProgramUsage::~GpuProgramUsage()
    {
        if (mProgram)
            mProgram->removeListener(this);
    }
    //-----------------------------------------------------------------------------

    auto GpuProgramUsage::_getProgramByName(std::string_view name, std::string_view group,
                                                     GpuProgramType type) -> GpuProgramPtr
    {
        GpuProgramPtr program =
            GpuProgramManager::getSingleton().getByName(name, group);

        //Look again without the group if not found
        if (!program)
            program = GpuProgramManager::getSingleton().getByName(
                name, ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME);

        if (!program)
        {
            String progType = GpuProgram::getProgramTypeName(type);
            OGRE_EXCEPT(ExceptionCodes::ITEM_NOT_FOUND,
                        ::std::format("Unable to locate {} program called {}", progType, name));
        }

        return program;
    }

    void GpuProgramUsage::setProgramName(std::string_view name, bool resetParams)
    {
        setProgram(_getProgramByName(name, mParent->getResourceGroup(), mType), resetParams);
    }
    //-----------------------------------------------------------------------------
    void GpuProgramUsage::setParameters(const GpuProgramParametersSharedPtr& params)
    {
        mParameters = params;
    }
    //-----------------------------------------------------------------------------
    auto GpuProgramUsage::getParameters() const noexcept -> const GpuProgramParametersSharedPtr&
    {
        if (!mParameters)
        {
            OGRE_EXCEPT(ExceptionCodes::INVALIDPARAMS, "You must specify a program before "
                "you can retrieve parameters.", "GpuProgramUsage::getParameters");
        }

        return mParameters;
    }
    //-----------------------------------------------------------------------------
    void GpuProgramUsage::setProgram(const GpuProgramPtr& prog, bool resetParams)
    {
        if (mProgram)
        {
            mProgram->removeListener(this);
            mRecreateParams = true;
        }

        mProgram = prog;

        // Reset parameters
        if (resetParams || !mParameters || mRecreateParams)
        {
            recreateParameters();
        }

        // Listen in on reload events so we can regenerate params
        mProgram->addListener(this);
    }
    //-----------------------------------------------------------------------------
    auto GpuProgramUsage::calculateSize() const -> size_t
    {
        size_t memSize = sizeof(*this);

        // Tally up passes
        if(mProgram)
            memSize += mProgram->calculateSize();
        if(mParameters)
            memSize += mParameters->calculateSize();

        return memSize;
    }
    //-----------------------------------------------------------------------------
    void GpuProgramUsage::_load()
    {
        if (!mProgram->isLoaded())
            mProgram->load();

        // check type
        if (mProgram->isLoaded() && mProgram->getType() != mType)
        {
            String myType = GpuProgram::getProgramTypeName(mType);
            String yourType = GpuProgram::getProgramTypeName(mProgram->getType());

            OGRE_EXCEPT(ExceptionCodes::INVALIDPARAMS, 
                ::std::format("{} is a {} program, but you are assigning it to a "
                "{} program slot. This is invalid", mProgram->getName(), yourType, myType));

        }
    }
    //-----------------------------------------------------------------------------
    void GpuProgramUsage::_unload()
    {
        // TODO?
    }
    //---------------------------------------------------------------------
    void GpuProgramUsage::unloadingComplete(Resource* prog)
    {
        mRecreateParams = true;

    }
    //---------------------------------------------------------------------
    void GpuProgramUsage::loadingComplete(Resource* prog)
    {
        // Need to re-create parameters
        if (mRecreateParams)
            recreateParameters();

    }
    //---------------------------------------------------------------------
    void GpuProgramUsage::recreateParameters()
    {
        // Keep a reference to old ones to copy
        GpuProgramParametersSharedPtr savedParams = mParameters;

        // Create new params
        mParameters = mProgram->createParameters();

        // Copy old (matching) values across
        // Don't use copyConstantsFrom since program may be different
        if (savedParams)
            mParameters->copyMatchingNamedConstantsFrom(*savedParams.get());

        mRecreateParams = false;

    }



}
