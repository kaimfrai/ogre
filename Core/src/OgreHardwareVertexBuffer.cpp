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
#include <algorithm>
#include <cassert>
#include <iterator>
#include <memory>
#include <string>

#include "OgreDefaultHardwareBufferManager.hpp"
#include "OgreException.hpp"
#include "OgreHardwareBufferManager.hpp"
#include "OgreHardwareVertexBuffer.hpp"
#include "OgreRenderSystem.hpp"
#include "OgreRenderSystemCapabilities.hpp"
#include "OgreRoot.hpp"
#include "OgreStringConverter.hpp"

namespace Ogre {

    //-----------------------------------------------------------------------------
    HardwareVertexBuffer::HardwareVertexBuffer(HardwareBufferManagerBase* mgr, size_t vertexSize,  
        size_t numVertices, HardwareBuffer::Usage usage, 
        bool useSystemMemory, bool useShadowBuffer) 
        : HardwareBuffer(usage, useSystemMemory, useShadowBuffer),
          mIsInstanceData(false),
          mMgr(mgr),
          mNumVertices(numVertices),
          mVertexSize(vertexSize),
          mInstanceDataStepRate(1)
    {
        // Calculate the size of the vertices
        mSizeInBytes = mVertexSize * numVertices;

        // Create a shadow buffer if required
        if (useShadowBuffer)
        {
            mShadowBuffer = std::make_unique<DefaultHardwareBuffer>(mSizeInBytes);
        }

    }
    HardwareVertexBuffer::HardwareVertexBuffer(HardwareBufferManagerBase* mgr, size_t vertexSize,
                                               size_t numVertices, HardwareBuffer* delegate)
        : HardwareVertexBuffer(mgr, vertexSize, numVertices, delegate->getUsage(), delegate->isSystemMemory(),
                               false)
    {
        mDelegate.reset(delegate);
    }
    //-----------------------------------------------------------------------------
    HardwareVertexBuffer::~HardwareVertexBuffer()
    {
        if (mMgr)
        {
            mMgr->_notifyVertexBufferDestroyed(this);
        }
    }
    //-----------------------------------------------------------------------------
    auto HardwareVertexBuffer::checkIfVertexInstanceDataIsSupported() -> bool
    {
        // Use the current render system
        RenderSystem* rs = Root::getSingleton().getRenderSystem();

        // Check if the supported  
        return rs->getCapabilities()->hasCapability(Capabilities::VERTEX_BUFFER_INSTANCE_DATA);
    }
    //-----------------------------------------------------------------------------
    void HardwareVertexBuffer::setIsInstanceData( const bool val )
    {
        if (val && !checkIfVertexInstanceDataIsSupported())
        {
            OGRE_EXCEPT(ExceptionCodes::RENDERINGAPI_ERROR, 
                "vertex instance data is not supported by the render system.", 
                "HardwareVertexBuffer::checkIfInstanceDataSupported");
        }
        else
        {
            mIsInstanceData = val;  
        }
    }
    //-----------------------------------------------------------------------------
    auto HardwareVertexBuffer::getInstanceDataStepRate() const -> size_t
    {
        return mInstanceDataStepRate;
    }
    //-----------------------------------------------------------------------------
    void HardwareVertexBuffer::setInstanceDataStepRate( const size_t val )
    {
        if (val > 0)
        {
            mInstanceDataStepRate = val;
        }
        else
        {
            OGRE_EXCEPT(ExceptionCodes::RENDERINGAPI_ERROR, 
                "Instance data step rate must be bigger then 0.", 
                "HardwareVertexBuffer::setInstanceDataStepRate");
        }
    }
    //-----------------------------------------------------------------------------
    // VertexElement
    //-----------------------------------------------------------------------------
    VertexElement::VertexElement(unsigned short source, size_t offset, VertexElementType theType,
                                 VertexElementSemantic semantic, unsigned short index)
        : mOffset(offset), mSource(source), mIndex(index), mType(theType), mSemantic(semantic)
    {
    }
    //-----------------------------------------------------------------------------
    auto VertexElement::getSize() const -> size_t
    {
        return getTypeSize(mType);
    }
    //-----------------------------------------------------------------------------
    auto VertexElement::getTypeSize(VertexElementType etype) -> size_t
    {
        switch(etype)
        {
        case VertexElementType::FLOAT1:
            return sizeof(float);
        case VertexElementType::FLOAT2:
            return sizeof(float)*2;
        case VertexElementType::FLOAT3:
            return sizeof(float)*3;
        case VertexElementType::FLOAT4:
            return sizeof(float)*4;
        case VertexElementType::DOUBLE1:
            return sizeof(double);
        case VertexElementType::DOUBLE2:
            return sizeof(double)*2;
        case VertexElementType::DOUBLE3:
            return sizeof(double)*3;
        case VertexElementType::DOUBLE4:
            return sizeof(double)*4;
        case VertexElementType::SHORT1:
        case VertexElementType::USHORT1:
            return sizeof( short );
        case VertexElementType::SHORT2:
        case VertexElementType::SHORT2_NORM:
        case VertexElementType::USHORT2:
        case VertexElementType::USHORT2_NORM:
            return sizeof( short ) * 2;
        case VertexElementType::SHORT3:
        case VertexElementType::USHORT3:
            return sizeof( short ) * 3;
        case VertexElementType::SHORT4:
        case VertexElementType::SHORT4_NORM:
        case VertexElementType::USHORT4:
        case VertexElementType::USHORT4_NORM:
            return sizeof( short ) * 4;
        case VertexElementType::INT1:
        case VertexElementType::UINT1:
            return sizeof( int );
        case VertexElementType::INT2:
        case VertexElementType::UINT2:
            return sizeof( int ) * 2;
        case VertexElementType::INT3:
        case VertexElementType::UINT3:
            return sizeof( int ) * 3;
        case VertexElementType::INT4:
        case VertexElementType::UINT4:
            return sizeof( int ) * 4;
        case VertexElementType::BYTE4:
        case VertexElementType::BYTE4_NORM:
        case VertexElementType::UBYTE4:
        case VertexElementType::UBYTE4_NORM:
        case VertexElementType::_DETAIL_SWAP_RB:
            return sizeof(char)*4;
        }
        return 0;
    }
    //-----------------------------------------------------------------------------
    auto VertexElement::getTypeCount(VertexElementType etype) -> unsigned short
    {
        switch (etype)
        {
        case VertexElementType::FLOAT1:
        case VertexElementType::SHORT1:
        case VertexElementType::USHORT1:
        case VertexElementType::UINT1:
        case VertexElementType::INT1:
        case VertexElementType::DOUBLE1:
            return 1;
        case VertexElementType::FLOAT2:
        case VertexElementType::SHORT2:
        case VertexElementType::SHORT2_NORM:
        case VertexElementType::USHORT2:
        case VertexElementType::USHORT2_NORM:
        case VertexElementType::UINT2:
        case VertexElementType::INT2:
        case VertexElementType::DOUBLE2:
            return 2;
        case VertexElementType::FLOAT3:
        case VertexElementType::SHORT3:
        case VertexElementType::USHORT3:
        case VertexElementType::UINT3:
        case VertexElementType::INT3:
        case VertexElementType::DOUBLE3:
            return 3;
        case VertexElementType::FLOAT4:
        case VertexElementType::SHORT4:
        case VertexElementType::SHORT4_NORM:
        case VertexElementType::USHORT4:
        case VertexElementType::USHORT4_NORM:
        case VertexElementType::UINT4:
        case VertexElementType::INT4:
        case VertexElementType::DOUBLE4:
        case VertexElementType::BYTE4:
        case VertexElementType::UBYTE4:
        case VertexElementType::BYTE4_NORM:
        case VertexElementType::UBYTE4_NORM:
        case VertexElementType::_DETAIL_SWAP_RB:
            return 4;
        }
        OGRE_EXCEPT(ExceptionCodes::INVALIDPARAMS, "Invalid type", 
            "VertexElement::getTypeCount");
    }
    //-----------------------------------------------------------------------------
    auto VertexElement::multiplyTypeCount(VertexElementType baseType, 
        unsigned short count) -> VertexElementType
    {
        OgreAssert(count > 0 && count < 5, "Count out of range");

        switch (baseType)
        {
        case VertexElementType::FLOAT1:
        case VertexElementType::DOUBLE1:
        case VertexElementType::INT1:
        case VertexElementType::UINT1:
            // evil enumeration arithmetic
            return static_cast<VertexElementType>( std::to_underlying(baseType) + count - 1 );

        case VertexElementType::SHORT1:
        case VertexElementType::SHORT2:
            if ( count <= 2 )
            {
                return VertexElementType::SHORT2;
            }
            return VertexElementType::SHORT4;

        case VertexElementType::USHORT1:
        case VertexElementType::USHORT2:
            if ( count <= 2 )
            {
                return VertexElementType::USHORT2;
            }
            return VertexElementType::USHORT4;

        case VertexElementType::SHORT2_NORM:
            if ( count <= 2 )
            {
                return VertexElementType::SHORT2_NORM;
            }
            return VertexElementType::SHORT4_NORM;

        case VertexElementType::USHORT2_NORM:
            if ( count <= 2 )
            {
                return VertexElementType::USHORT2_NORM;
            }
            return VertexElementType::USHORT4_NORM;

        case VertexElementType::BYTE4:
        case VertexElementType::BYTE4_NORM:
        case VertexElementType::UBYTE4:
        case VertexElementType::UBYTE4_NORM:
            return baseType;

        default:
            break;
        }
        OGRE_EXCEPT(ExceptionCodes::INVALIDPARAMS, "Invalid base type", 
            "VertexElement::multiplyTypeCount");
    }
    //--------------------------------------------------------------------------
    auto VertexElement::getBestColourVertexElementType() -> VertexElementType
    {
        return VertexElementType::UBYTE4_NORM;
    }
    //--------------------------------------------------------------------------
    void VertexElement::convertColourValue(VertexElementType srcType, 
        VertexElementType dstType, uint32* ptr)
    {
        if (srcType == dstType)
            return;

        // Conversion between ARGB and ABGR is always a case of flipping R/B
        *ptr = 
           ((*ptr&0x00FF0000)>>16)|((*ptr&0x000000FF)<<16)|(*ptr&0xFF00FF00);               
    }
    //-----------------------------------------------------------------------------
    auto VertexElement::getBaseType(VertexElementType multiType) -> VertexElementType
    {
        switch (multiType)
        {
            case VertexElementType::FLOAT1:
            case VertexElementType::FLOAT2:
            case VertexElementType::FLOAT3:
            case VertexElementType::FLOAT4:
                return VertexElementType::FLOAT1;
            case VertexElementType::DOUBLE1:
            case VertexElementType::DOUBLE2:
            case VertexElementType::DOUBLE3:
            case VertexElementType::DOUBLE4:
                return VertexElementType::DOUBLE1;
            case VertexElementType::INT1:
            case VertexElementType::INT2:
            case VertexElementType::INT3:
            case VertexElementType::INT4:
                return VertexElementType::INT1;
            case VertexElementType::UINT1:
            case VertexElementType::UINT2:
            case VertexElementType::UINT3:
            case VertexElementType::UINT4:
                return VertexElementType::UINT1;
            case VertexElementType::SHORT1:
            case VertexElementType::SHORT2:
            case VertexElementType::SHORT3:
            case VertexElementType::SHORT4:
                return VertexElementType::SHORT1;
            case VertexElementType::USHORT1:
            case VertexElementType::USHORT2:
            case VertexElementType::USHORT3:
            case VertexElementType::USHORT4:
                return VertexElementType::USHORT1;
            case VertexElementType::SHORT2_NORM:
            case VertexElementType::SHORT4_NORM:
                return VertexElementType::SHORT2_NORM;
            case VertexElementType::USHORT2_NORM:
            case VertexElementType::USHORT4_NORM:
                return VertexElementType::USHORT2_NORM;
            case VertexElementType::BYTE4:
                return VertexElementType::BYTE4;
            case VertexElementType::BYTE4_NORM:
                return VertexElementType::BYTE4_NORM;
            case VertexElementType::UBYTE4:
                return VertexElementType::UBYTE4;
            case VertexElementType::UBYTE4_NORM:
            case VertexElementType::_DETAIL_SWAP_RB:
                return VertexElementType::UBYTE4_NORM;
        };
        // To keep compiler happy
        return VertexElementType::FLOAT1;
    }
    //-----------------------------------------------------------------------------
    VertexDeclaration::VertexDeclaration()
    = default;
    //-----------------------------------------------------------------------------
    VertexDeclaration::~VertexDeclaration()
    = default;
    //-----------------------------------------------------------------------------
    auto VertexDeclaration::getElements() const noexcept -> const VertexDeclaration::VertexElementList&
    {
        return mElementList;
    }
    //-----------------------------------------------------------------------------
    auto VertexDeclaration::addElement(unsigned short source, 
        size_t offset, VertexElementType theType,
        VertexElementSemantic semantic, unsigned short index) -> const VertexElement&
    {
        // Refine colour type to a specific type
        if (theType == VertexElementType::COLOUR)
        {
            theType = VertexElement::getBestColourVertexElementType();
        }
        mElementList.push_back(
            VertexElement(source, offset, theType, semantic, index));

        notifyChanged();
        return mElementList.back();
    }
    //-----------------------------------------------------------------------------
    auto VertexDeclaration::insertElement(unsigned short atPosition,
        unsigned short source, size_t offset, VertexElementType theType,
        VertexElementSemantic semantic, unsigned short index) -> const VertexElement&
    {
        if (atPosition >= mElementList.size())
        {
            return addElement(source, offset, theType, semantic, index);
        }

        auto i = mElementList.begin();
        for (unsigned short n = 0; n < atPosition; ++n)
            ++i;

        i = mElementList.insert(i, 
            VertexElement(source, offset, theType, semantic, index));

        notifyChanged();
        return *i;
    }
    //-----------------------------------------------------------------------------
    auto VertexDeclaration::getElement(unsigned short index) const -> const VertexElement*
    {
        assert(index < mElementList.size() && "Index out of bounds");

        auto i = mElementList.begin();
        for (unsigned short n = 0; n < index; ++n)
            ++i;

        return &(*i);

    }
    //-----------------------------------------------------------------------------
    void VertexDeclaration::removeElement(unsigned short elem_index)
    {
        assert(elem_index < mElementList.size() && "Index out of bounds");
        auto i = mElementList.begin();
        for (unsigned short n = 0; n < elem_index; ++n)
            ++i;
        mElementList.erase(i);
        notifyChanged();
    }
    //-----------------------------------------------------------------------------
    void VertexDeclaration::removeElement(VertexElementSemantic semantic, unsigned short index)
    {
        for (auto ei = mElementList.begin(); ei != mElementList.end(); ++ei)
        {
            if (ei->getSemantic() == semantic && ei->getIndex() == index)
            {
                mElementList.erase(ei);
                notifyChanged();
                break;
            }
        }
    }
    //-----------------------------------------------------------------------------
    void VertexDeclaration::removeAllElements()
    {
        mElementList.clear();
        notifyChanged();
    }
    //-----------------------------------------------------------------------------
    void VertexDeclaration::modifyElement(unsigned short elem_index, 
        unsigned short source, size_t offset, VertexElementType theType,
        VertexElementSemantic semantic, unsigned short index)
    {
        assert(elem_index < mElementList.size() && "Index out of bounds");
        auto i = mElementList.begin();
        std::advance(i, elem_index);
        (*i) = VertexElement(source, offset, theType, semantic, index);
        notifyChanged();
    }
    //-----------------------------------------------------------------------------
    auto VertexDeclaration::findElementBySemantic(
        VertexElementSemantic sem, unsigned short index) const -> const VertexElement*
    {
        for (const auto & ei : mElementList)
        {
            if (ei.getSemantic() == sem && ei.getIndex() == index)
            {
                return &ei;
            }
        }

        return nullptr;


    }
    //-----------------------------------------------------------------------------
    auto VertexDeclaration::findElementsBySource(
        unsigned short source) const -> VertexDeclaration::VertexElementList
    {
        VertexElementList retList;
        for (const auto & ei : mElementList)
        {
            if (ei.getSource() == source)
            {
                retList.push_back(ei);
            }
        }
        return retList;

    }

    //-----------------------------------------------------------------------------
    auto VertexDeclaration::getVertexSize(unsigned short source) const -> size_t
    {
        size_t sz = 0;

        for (const auto & i : mElementList)
        {
            if (i.getSource() == source)
            {
                sz += i.getSize();

            }
        }
        return sz;
    }
    //-----------------------------------------------------------------------------
    auto VertexDeclaration::clone(HardwareBufferManagerBase* mgr) const -> VertexDeclaration*
    {
        HardwareBufferManagerBase* pManager = mgr ? mgr : HardwareBufferManager::getSingletonPtr(); 
        VertexDeclaration* ret = pManager->createVertexDeclaration();

        for (const auto & i : mElementList)
        {
            ret->addElement(i.getSource(), i.getOffset(), i.getType(), i.getSemantic(), i.getIndex());
        }
        return ret;
    }
    //-----------------------------------------------------------------------------
    // Sort routine for VertexElement
    auto VertexDeclaration::vertexElementLess(const VertexElement& e1, const VertexElement& e2) -> bool
    {
        // Sort by source first
        if (e1.getSource() < e2.getSource())
        {
            return true;
        }
        else if (e1.getSource() == e2.getSource())
        {
            // Use ordering of semantics to sort
            if (e1.getSemantic() < e2.getSemantic())
            {
                return true;
            }
            else if (e1.getSemantic() == e2.getSemantic())
            {
                // Use index to sort
                if (e1.getIndex() < e2.getIndex())
                {
                    return true;
                }
            }
        }
        return false;
    }
    void VertexDeclaration::sort()
    {
        mElementList.sort(VertexDeclaration::vertexElementLess);
    }
    //-----------------------------------------------------------------------------
    void VertexDeclaration::closeGapsInSource()
    {
        if (mElementList.empty())
            return;

        // Sort first
        sort();

        unsigned short targetIdx = 0;
        unsigned short lastIdx = getElement(0)->getSource();
        for (unsigned short c = 0;
             VertexElement& elem : mElementList)
        {
            if (lastIdx != elem.getSource())
            {
                targetIdx++;
                lastIdx = elem.getSource();
            }
            if (targetIdx != elem.getSource())
            {
                modifyElement(c, targetIdx, elem.getOffset(), elem.getType(), 
                    elem.getSemantic(), elem.getIndex());
            }
            ++c;
        }

    }
    //-----------------------------------------------------------------------
    auto VertexDeclaration::getAutoOrganisedDeclaration(
        bool skeletalAnimation, bool vertexAnimation, bool vertexAnimationNormals) const -> VertexDeclaration*
    {
        VertexDeclaration* newDecl = this->clone();
        // Set all sources to the same buffer (for now)
        const VertexDeclaration::VertexElementList& elems = newDecl->getElements();
        for (unsigned short c = 0;
             const VertexElement& elem : elems)
        {
            // Set source & offset to 0 for now, before sort
            newDecl->modifyElement(c, 0, 0, elem.getType(), elem.getSemantic(), elem.getIndex());
            ++c;
        }
        newDecl->sort();
        // Now sort out proper buffer assignments and offsets
        size_t offset = 0;
        unsigned short buffer = 0;
        VertexElementSemantic prevSemantic = VertexElementSemantic::POSITION;
        for (unsigned short c = 0;
             const auto & elem : elems)
        {
            bool splitWithPrev = false;
            bool splitWithNext = false;
            switch (elem.getSemantic())
            {
            case VertexElementSemantic::POSITION:
                // Split positions if vertex animated with only positions
                // group with normals otherwise
                splitWithPrev = false;
                splitWithNext = vertexAnimation && !vertexAnimationNormals;
                break;
            case VertexElementSemantic::NORMAL:
                // Normals can't share with blend weights/indices
                splitWithPrev = (prevSemantic == VertexElementSemantic::BLEND_WEIGHTS || prevSemantic == VertexElementSemantic::BLEND_INDICES);
                // All animated meshes have to split after normal
                splitWithNext = (skeletalAnimation || (vertexAnimation && vertexAnimationNormals));
                break;
            case VertexElementSemantic::BLEND_WEIGHTS:
                // Blend weights/indices can be sharing with their own buffer only
                splitWithPrev = true;
                break;
            case VertexElementSemantic::BLEND_INDICES:
                // Blend weights/indices can be sharing with their own buffer only
                splitWithNext = true;
                break;
            default:
            case VertexElementSemantic::DIFFUSE:
            case VertexElementSemantic::SPECULAR:
            case VertexElementSemantic::TEXTURE_COORDINATES:
            case VertexElementSemantic::BINORMAL:
            case VertexElementSemantic::TANGENT:
                // Make sure position is separate if animated & there were no normals
                splitWithPrev = prevSemantic == VertexElementSemantic::POSITION && 
                    (skeletalAnimation || vertexAnimation);
                break;
            }

            if (splitWithPrev && offset)
            {
                ++buffer;
                offset = 0;
            }

            prevSemantic = elem.getSemantic();
            newDecl->modifyElement(c, buffer, offset,
                elem.getType(), elem.getSemantic(), elem.getIndex());

            if (splitWithNext)
            {
                ++buffer;
                offset = 0;
            }
            else
            {
                offset += elem.getSize();
            }
            ++c;
        }

        return newDecl;


    }
    //-----------------------------------------------------------------------------
    auto VertexDeclaration::getMaxSource() const noexcept -> unsigned short
    {
        unsigned short ret = 0;
        for (const auto & i : mElementList)
        {
            if (i.getSource() > ret)
            {
                ret = i.getSource();
            }

        }
        return ret;
    }
    //-----------------------------------------------------------------------------
    auto VertexDeclaration::getNextFreeTextureCoordinate() const noexcept -> unsigned short
    {
        unsigned short texCoord = 0;
        for (const auto & el : mElementList)
        {
            if (el.getSemantic() == VertexElementSemantic::TEXTURE_COORDINATES)
            {
                ++texCoord;
            }
        }
        return texCoord;
    }
    //-----------------------------------------------------------------------------
    VertexBufferBinding::VertexBufferBinding()  
    = default;
    //-----------------------------------------------------------------------------
    VertexBufferBinding::~VertexBufferBinding()
    {
        unsetAllBindings();
    }
    //-----------------------------------------------------------------------------
    void VertexBufferBinding::setBinding(unsigned short index, const HardwareVertexBufferSharedPtr& buffer)
    {
        // NB will replace any existing buffer ptr at this index, and will thus cause
        // reference count to decrement on that buffer (possibly destroying it)
        mBindingMap[index] = buffer;
        mHighIndex = std::max(mHighIndex, (unsigned short)(index+1));
    }
    //-----------------------------------------------------------------------------
    void VertexBufferBinding::unsetBinding(unsigned short index)
    {
        auto i = mBindingMap.find(index);
        if (i == mBindingMap.end())
        {
            OGRE_EXCEPT(ExceptionCodes::ITEM_NOT_FOUND,
                ::std::format("Cannot find buffer binding for index {}", index),
                "VertexBufferBinding::unsetBinding");
        }
        mBindingMap.erase(i);
    }
    //-----------------------------------------------------------------------------
    void VertexBufferBinding::unsetAllBindings()
    {
        mBindingMap.clear();
        mHighIndex = 0;
    }
    //-----------------------------------------------------------------------------
    auto 
    VertexBufferBinding::getBindings() const noexcept -> const VertexBufferBinding::VertexBufferBindingMap&
    {
        return mBindingMap;
    }
    //-----------------------------------------------------------------------------
    auto VertexBufferBinding::getBuffer(unsigned short index) const -> const HardwareVertexBufferSharedPtr&
    {
        auto i = mBindingMap.find(index);
        if (i == mBindingMap.end())
        {
            OGRE_EXCEPT(ExceptionCodes::ITEM_NOT_FOUND, "No buffer is bound to that index.",
                "VertexBufferBinding::getBuffer");
        }
        return i->second;
    }
    //-----------------------------------------------------------------------------
    auto VertexBufferBinding::isBufferBound(unsigned short index) const -> bool
    {
        return mBindingMap.find(index) != mBindingMap.end();
    }
    //-----------------------------------------------------------------------------
    auto VertexBufferBinding::getLastBoundIndex() const noexcept -> unsigned short
    {
        return mBindingMap.empty() ? 0 : mBindingMap.rbegin()->first + 1;
    }
    //-----------------------------------------------------------------------------
    auto VertexBufferBinding::hasGaps() const -> bool
    {
        if (mBindingMap.empty())
            return false;
        if (mBindingMap.rbegin()->first + 1 == (int) mBindingMap.size())
            return false;
        return true;
    }
    //-----------------------------------------------------------------------------
    void VertexBufferBinding::closeGaps(BindingIndexMap& bindingIndexMap)
    {
        bindingIndexMap.clear();

        VertexBufferBindingMap newBindingMap;
        ushort targetIndex = 0;
        for (auto const& [key, value] : mBindingMap)
        {
            bindingIndexMap[key] = targetIndex;
            newBindingMap[targetIndex] = value;
             ++targetIndex;
        }

        mBindingMap.swap(newBindingMap);
        mHighIndex = targetIndex;
    }
}
