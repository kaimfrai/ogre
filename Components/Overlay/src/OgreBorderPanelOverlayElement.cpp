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

#include "OgreBorderPanelOverlayElement.hpp"

#include <string>
#include <vector>

#include "OgreException.hpp"
#include "OgreHardwareBuffer.hpp"
#include "OgreHardwareBufferManager.hpp"
#include "OgreHardwareIndexBuffer.hpp"
#include "OgreHardwareVertexBuffer.hpp"
#include "OgreMaterial.hpp"
#include "OgreMaterialManager.hpp"
#include "OgreRenderQueue.hpp"
#include "OgreRenderSystem.hpp"
#include "OgreRoot.hpp"
#include "OgreString.hpp"
#include "OgreStringConverter.hpp"
#include "OgreStringInterface.hpp"
#include "OgreVertexIndexData.hpp"

namespace Ogre {
    //---------------------------------------------------------------------
    String BorderPanelOverlayElement::msTypeName = "BorderPanel";
        /** Command object for specifying border sizes (see ParamCommand).*/
        class CmdBorderSize : public ParamCommand
        {
        public:
            auto doGet(const void* target) const -> String override;
            void doSet(void* target, std::string_view val) override;
        };
        /** Command object for specifying the Material for the border (see ParamCommand).*/
        class CmdBorderMaterial : public ParamCommand
        {
        public:
            auto doGet(const void* target) const -> String override;
            void doSet(void* target, std::string_view val) override;
        };
        /** Command object for specifying texture coordinates for the border (see ParamCommand).*/
        class CmdBorderLeftUV : public ParamCommand
        {
        public:
            auto doGet(const void* target) const -> String override;
            void doSet(void* target, std::string_view val) override;
        };
        /** Command object for specifying texture coordinates for the border (see ParamCommand).*/
        class CmdBorderTopUV : public ParamCommand
        {
        public:
            auto doGet(const void* target) const -> String override;
            void doSet(void* target, std::string_view val) override;
        };
        /** Command object for specifying texture coordinates for the border (see ParamCommand).*/
        class CmdBorderRightUV : public ParamCommand
        {
        public:
            auto doGet(const void* target) const -> String override;
            void doSet(void* target, std::string_view val) override;
        };
        /** Command object for specifying texture coordinates for the border (see ParamCommand).*/
        class CmdBorderBottomUV : public ParamCommand
        {
        public:
            auto doGet(const void* target) const -> String override;
            void doSet(void* target, std::string_view val) override;
        };
        /** Command object for specifying texture coordinates for the border (see ParamCommand).*/
        class CmdBorderTopLeftUV : public ParamCommand
        {
        public:
            auto doGet(const void* target) const -> String override;
            void doSet(void* target, std::string_view val) override;
        };
        /** Command object for specifying texture coordinates for the border (see ParamCommand).*/
        class CmdBorderBottomLeftUV : public ParamCommand
        {
        public:
            auto doGet(const void* target) const -> String override;
            void doSet(void* target, std::string_view val) override;
        };
        /** Command object for specifying texture coordinates for the border (see ParamCommand).*/
        class CmdBorderBottomRightUV : public ParamCommand
        {
        public:
            auto doGet(const void* target) const -> String override;
            void doSet(void* target, std::string_view val) override;
        };
        /** Command object for specifying texture coordinates for the border (see ParamCommand).*/
        class CmdBorderTopRightUV : public ParamCommand
        {
        public:
            auto doGet(const void* target) const -> String override;
            void doSet(void* target, std::string_view val) override;
        };
        // Command objects
        static CmdBorderSize msCmdBorderSize;
        static CmdBorderMaterial msCmdBorderMaterial;
        static CmdBorderLeftUV msCmdBorderLeftUV;
        static CmdBorderTopUV msCmdBorderTopUV;
        static CmdBorderBottomUV msCmdBorderBottomUV;
        static CmdBorderRightUV msCmdBorderRightUV;
        static CmdBorderTopLeftUV msCmdBorderTopLeftUV;
        static CmdBorderBottomLeftUV msCmdBorderBottomLeftUV;
        static CmdBorderTopRightUV msCmdBorderTopRightUV;
        static CmdBorderBottomRightUV msCmdBorderBottomRightUV;

    #define BCELL_UV(x) (x * 4 * 2)
    #define POSITION_BINDING 0
    #define TEXCOORD_BINDING 1
    //---------------------------------------------------------------------
    BorderPanelOverlayElement::BorderPanelOverlayElement(std::string_view name)
      : PanelOverlayElement(name), 
        
        mBorderMaterial()
               
    {   
        mBorderUV->u1 = mBorderUV->u2 = mBorderUV->v1 = mBorderUV->v2 = 0;

        if (createParamDictionary("BorderPanelOverlayElement"))
        {
            addBaseParameters();
        }
    }
    //---------------------------------------------------------------------
    BorderPanelOverlayElement::~BorderPanelOverlayElement()
    {
        delete mRenderOp2.vertexData;
        delete mRenderOp2.indexData;
    }
    //---------------------------------------------------------------------
    void BorderPanelOverlayElement::initialise()
    {
        bool init = !mInitialised;

        // init mRenderOp2 before calling superclass, as virtual _restoreManualHardwareResources would be called within
        if (init)
        {
            // Setup render op in advance
            mRenderOp2.vertexData = new VertexData();
            mRenderOp2.vertexData->vertexCount = 4 * 8; // 8 cells, can't necessarily share vertices cos
                                                        // texcoords may differ
            mRenderOp2.vertexData->vertexStart = 0;

            // Vertex declaration
            VertexDeclaration* decl = mRenderOp2.vertexData->vertexDeclaration;
            // Position and texture coords each have their own buffers to allow
            // each to be edited separately with the discard flag
            decl->addElement(POSITION_BINDING, 0, VertexElementType::FLOAT3, VertexElementSemantic::POSITION);
            decl->addElement(TEXCOORD_BINDING, 0, VertexElementType::FLOAT2, VertexElementSemantic::TEXTURE_COORDINATES, 0);

            // Index data
            mRenderOp2.operationType = RenderOperation::OperationType::TRIANGLE_LIST;
            mRenderOp2.useIndexes = true;
            mRenderOp2.indexData = new IndexData();
            mRenderOp2.indexData->indexCount = 8 * 6;
            mRenderOp2.indexData->indexStart = 0;
            mRenderOp2.useGlobalInstancingVertexBufferIsAvailable = false;

            // Create sub-object for rendering border
            mBorderRenderable = ::std::make_unique<BorderRenderable>(this);
        }

        // superclass will handle the interior panel area and call _restoreManualHardwareResources
        PanelOverlayElement::initialise();
    }
    //---------------------------------------------------------------------
    void BorderPanelOverlayElement::_restoreManualHardwareResources()
    {
        if(!mInitialised)
            return;

        PanelOverlayElement::_restoreManualHardwareResources();

        // Vertex data

        VertexDeclaration* decl = mRenderOp2.vertexData->vertexDeclaration;

        // Vertex buffer #1, position
        HardwareVertexBufferSharedPtr vbuf =
            HardwareBufferManager::getSingleton().createVertexBuffer(
                decl->getVertexSize(POSITION_BINDING), 
                mRenderOp2.vertexData->vertexCount,
                HardwareBuffer::DYNAMIC_WRITE_ONLY,
                true);//Workaround, using shadow buffer to avoid stall due to buffer mapping
        // bind position
        VertexBufferBinding* binding = mRenderOp2.vertexData->vertexBufferBinding;
        binding->setBinding(POSITION_BINDING, vbuf);

        // Vertex buffer #2, texcoords
        vbuf = HardwareBufferManager::getSingleton().createVertexBuffer(
                decl->getVertexSize(TEXCOORD_BINDING), 
                mRenderOp2.vertexData->vertexCount,
                HardwareBuffer::DYNAMIC_WRITE_ONLY,
                true);//Workaround, using shadow buffer to avoid stall due to buffer mapping
        // bind texcoord
        binding->setBinding(TEXCOORD_BINDING, vbuf);


        // Index data

        /* Each cell is
            0-----2
            |    /|
            |  /  |
            |/    |
            1-----3
        */
        mRenderOp2.indexData->indexBuffer =
            HardwareBufferManager::getSingleton().createIndexBuffer(
                HardwareIndexBuffer::IndexType::_16BIT, 
                mRenderOp2.indexData->indexCount, 
                HardwareBuffer::DYNAMIC_WRITE_ONLY,
                true);//Workaround, using shadow buffer to avoid stall due to buffer mapping

        HardwareBufferLockGuard indexLock(mRenderOp2.indexData->indexBuffer, HardwareBuffer::LockOptions::DISCARD);
        auto* pIdx = static_cast<ushort*>(indexLock.pData);

        for (ushort cell = 0; cell < 8; ++cell)
        {
            ushort base = cell * 4;
            *pIdx++ = base;
            *pIdx++ = base + 1;
            *pIdx++ = base + 2;

            *pIdx++ = base + 2;
            *pIdx++ = base + 1;
            *pIdx++ = base + 3;
        }
    }
    //---------------------------------------------------------------------
    void BorderPanelOverlayElement::_releaseManualHardwareResources()
    {
        if(!mInitialised)
            return;

        mRenderOp2.vertexData->vertexBufferBinding->unsetAllBindings();
        mRenderOp2.indexData->indexBuffer.reset();

        PanelOverlayElement::_releaseManualHardwareResources();
    }
    //---------------------------------------------------------------------
    void BorderPanelOverlayElement::addBaseParameters()
    {
        PanelOverlayElement::addBaseParameters();
        ParamDictionary* dict = getParamDictionary();

        dict->addParameter(ParameterDef("border_size", 
            "The sizes of the borders relative to the screen size, in the order "
            "left, right, top, bottom."
            , ParameterType::STRING),
            &msCmdBorderSize);
        dict->addParameter(ParameterDef("border_material", 
            "The material to use for the border."
            , ParameterType::STRING),
            &msCmdBorderMaterial);
        dict->addParameter(ParameterDef("border_topleft_uv", 
            "The texture coordinates for the top-left corner border texture. 2 sets of uv values, "
            "one for the top-left corner, the other for the bottom-right corner."
            , ParameterType::STRING),
            &msCmdBorderTopLeftUV);
        dict->addParameter(ParameterDef("border_topright_uv", 
            "The texture coordinates for the top-right corner border texture. 2 sets of uv values, "
            "one for the top-left corner, the other for the bottom-right corner."
            , ParameterType::STRING),
            &msCmdBorderTopRightUV);
        dict->addParameter(ParameterDef("border_bottomright_uv", 
            "The texture coordinates for the bottom-right corner border texture. 2 sets of uv values, "
            "one for the top-left corner, the other for the bottom-right corner."
            , ParameterType::STRING),
            &msCmdBorderBottomRightUV);
        dict->addParameter(ParameterDef("border_bottomleft_uv", 
            "The texture coordinates for the bottom-left corner border texture. 2 sets of uv values, "
            "one for the top-left corner, the other for the bottom-right corner."
            , ParameterType::STRING),
            &msCmdBorderBottomLeftUV);
        dict->addParameter(ParameterDef("border_left_uv", 
            "The texture coordinates for the left edge border texture. 2 sets of uv values, "
            "one for the top-left corner, the other for the bottom-right corner."
            , ParameterType::STRING),
            &msCmdBorderLeftUV);
        dict->addParameter(ParameterDef("border_top_uv", 
            "The texture coordinates for the top edge border texture. 2 sets of uv values, "
            "one for the top-left corner, the other for the bottom-right corner."
            , ParameterType::STRING),
            &msCmdBorderTopUV);
        dict->addParameter(ParameterDef("border_right_uv", 
            "The texture coordinates for the right edge border texture. 2 sets of uv values, "
            "one for the top-left corner, the other for the bottom-right corner."
            , ParameterType::STRING),
            &msCmdBorderRightUV);
        dict->addParameter(ParameterDef("border_bottom_uv", 
            "The texture coordinates for the bottom edge border texture. 2 sets of uv values, "
            "one for the top-left corner, the other for the bottom-right corner."
            , ParameterType::STRING),
            &msCmdBorderBottomUV);

    }
    //---------------------------------------------------------------------
    void BorderPanelOverlayElement::setBorderSize(Real size)
    {
        if (mMetricsMode != GuiMetricsMode::RELATIVE)
        {
            mPixelLeftBorderSize = mPixelRightBorderSize = 
                mPixelTopBorderSize = mPixelBottomBorderSize = static_cast<unsigned short>(size);
        }
        else
        {
            mLeftBorderSize = mRightBorderSize = 
                mTopBorderSize = mBottomBorderSize = size;
        }
        mGeomPositionsOutOfDate = true;
    }
    //---------------------------------------------------------------------
    void BorderPanelOverlayElement::setBorderSize(Real sides, Real topAndBottom)
    {
        if (mMetricsMode != GuiMetricsMode::RELATIVE)
        {
            mPixelLeftBorderSize = mPixelRightBorderSize = static_cast<unsigned short>(sides);
            mPixelTopBorderSize = mPixelBottomBorderSize = static_cast<unsigned short>(topAndBottom);
        }
        else
        {
            mLeftBorderSize = mRightBorderSize = sides;
            mTopBorderSize = mBottomBorderSize = topAndBottom;
        }
        mGeomPositionsOutOfDate = true;


    }
    //---------------------------------------------------------------------
    void BorderPanelOverlayElement::setBorderSize(Real left, Real right, Real top, Real bottom)
    {
        if (mMetricsMode != GuiMetricsMode::RELATIVE)
        {
            mPixelLeftBorderSize = static_cast<unsigned short>(left);
            mPixelRightBorderSize = static_cast<unsigned short>(right);
            mPixelTopBorderSize = static_cast<unsigned short>(top);
            mPixelBottomBorderSize = static_cast<unsigned short>(bottom);
        }
        else
        {
            mLeftBorderSize = left;
            mRightBorderSize = right;
            mTopBorderSize = top;
            mBottomBorderSize = bottom;
        }
        mGeomPositionsOutOfDate = true;
    }
    //---------------------------------------------------------------------
    auto BorderPanelOverlayElement::getLeftBorderSize() const -> Real
    {
        if (mMetricsMode == GuiMetricsMode::PIXELS)
        {
            return mPixelLeftBorderSize;
        }
        else
        {
            return mLeftBorderSize;
        }
    }
    //---------------------------------------------------------------------
    auto BorderPanelOverlayElement::getRightBorderSize() const -> Real
    {
        if (mMetricsMode == GuiMetricsMode::PIXELS)
        {
            return mPixelRightBorderSize;
        }
        else
        {
            return mRightBorderSize;
        }
    }
    //---------------------------------------------------------------------
    auto BorderPanelOverlayElement::getTopBorderSize() const -> Real
    {
        if (mMetricsMode == GuiMetricsMode::PIXELS)
        {
            return mPixelTopBorderSize;
        }
        else
        {
            return mTopBorderSize;
        }
    }
    //---------------------------------------------------------------------
    auto BorderPanelOverlayElement::getBottomBorderSize() const -> Real
    {
        if (mMetricsMode == GuiMetricsMode::PIXELS)
        {
            return mPixelBottomBorderSize;
        }
        else
        {
            return mBottomBorderSize;
        }
    }
    //---------------------------------------------------------------------
    void BorderPanelOverlayElement::updateTextureGeometry()
    {
        PanelOverlayElement::updateTextureGeometry();
        /* Each cell is
            0-----2
            |    /|
            |  /  |
            |/    |
            1-----3
        */
        // No choice but to lock / unlock each time here, but lock only small sections
        
        HardwareVertexBufferSharedPtr vbuf = 
            mRenderOp2.vertexData->vertexBufferBinding->getBuffer(TEXCOORD_BINDING);
        // Can't use discard since this discards whole buffer
        HardwareBufferLockGuard vbufLock(vbuf, HardwareBuffer::LockOptions::DISCARD);
        auto* pUV = static_cast<float*>(vbufLock.pData);
        
        for (auto & i : mBorderUV)
        {
            *pUV++ = i.u1; *pUV++ = i.v1;
            *pUV++ = i.u1; *pUV++ = i.v2;
            *pUV++ = i.u2; *pUV++ = i.v1;
            *pUV++ = i.u2; *pUV++ = i.v2;
        }
    }
    //---------------------------------------------------------------------
    auto BorderPanelOverlayElement::getCellUVString(BorderCellIndex idx) const -> String
    {
        return
        ::std::format
        (   "{} {} {} {}"
        ,   mBorderUV[std::to_underlying(idx)].u1
        ,   mBorderUV[std::to_underlying(idx)].v1
        ,   mBorderUV[std::to_underlying(idx)].u2
        ,   mBorderUV[std::to_underlying(idx)].v2
        );
    }
    //---------------------------------------------------------------------
    void BorderPanelOverlayElement::setLeftBorderUV(Real u1, Real v1, Real u2, Real v2)
    {
        mBorderUV[std::to_underlying(BorderCellIndex::LEFT)].u1 = u1;
        mBorderUV[std::to_underlying(BorderCellIndex::LEFT)].u2 = u2;
        mBorderUV[std::to_underlying(BorderCellIndex::LEFT)].v1 = v1;
        mBorderUV[std::to_underlying(BorderCellIndex::LEFT)].v2 = v2;
        mGeomUVsOutOfDate = true;
    }
    //---------------------------------------------------------------------
    void BorderPanelOverlayElement::setRightBorderUV(Real u1, Real v1, Real u2, Real v2)
    {
        mBorderUV[std::to_underlying(BorderCellIndex::RIGHT)].u1 = u1;
        mBorderUV[std::to_underlying(BorderCellIndex::RIGHT)].u2 = u2;
        mBorderUV[std::to_underlying(BorderCellIndex::RIGHT)].v1 = v1;
        mBorderUV[std::to_underlying(BorderCellIndex::RIGHT)].v2 = v2;
        mGeomUVsOutOfDate = true;
    }
    //---------------------------------------------------------------------
    void BorderPanelOverlayElement::setTopBorderUV(Real u1, Real v1, Real u2, Real v2)
    {
        mBorderUV[std::to_underlying(BorderCellIndex::TOP)].u1 = u1;
        mBorderUV[std::to_underlying(BorderCellIndex::TOP)].u2 = u2;
        mBorderUV[std::to_underlying(BorderCellIndex::TOP)].v1 = v1;
        mBorderUV[std::to_underlying(BorderCellIndex::TOP)].v2 = v2;
        mGeomUVsOutOfDate = true;
    }
    //---------------------------------------------------------------------
    void BorderPanelOverlayElement::setBottomBorderUV(Real u1, Real v1, Real u2, Real v2)
    {
        mBorderUV[std::to_underlying(BorderCellIndex::BOTTOM)].u1 = u1;
        mBorderUV[std::to_underlying(BorderCellIndex::BOTTOM)].u2 = u2;
        mBorderUV[std::to_underlying(BorderCellIndex::BOTTOM)].v1 = v1;
        mBorderUV[std::to_underlying(BorderCellIndex::BOTTOM)].v2 = v2;
        mGeomUVsOutOfDate = true;
    }
    //---------------------------------------------------------------------
    void BorderPanelOverlayElement::setTopLeftBorderUV(Real u1, Real v1, Real u2, Real v2)
    {
        mBorderUV[std::to_underlying(BorderCellIndex::TOP_LEFT)].u1 = u1;
        mBorderUV[std::to_underlying(BorderCellIndex::TOP_LEFT)].u2 = u2;
        mBorderUV[std::to_underlying(BorderCellIndex::TOP_LEFT)].v1 = v1;
        mBorderUV[std::to_underlying(BorderCellIndex::TOP_LEFT)].v2 = v2;
        mGeomUVsOutOfDate = true;
    }
    //---------------------------------------------------------------------
    void BorderPanelOverlayElement::setTopRightBorderUV(Real u1, Real v1, Real u2, Real v2)
    {
        mBorderUV[std::to_underlying(BorderCellIndex::TOP_RIGHT)].u1 = u1;
        mBorderUV[std::to_underlying(BorderCellIndex::TOP_RIGHT)].u2 = u2;
        mBorderUV[std::to_underlying(BorderCellIndex::TOP_RIGHT)].v1 = v1;
        mBorderUV[std::to_underlying(BorderCellIndex::TOP_RIGHT)].v2 = v2;
        mGeomUVsOutOfDate = true;
    }
    //---------------------------------------------------------------------
    void BorderPanelOverlayElement::setBottomLeftBorderUV(Real u1, Real v1, Real u2, Real v2)
    {
        mBorderUV[std::to_underlying(BorderCellIndex::BOTTOM_LEFT)].u1 = u1;
        mBorderUV[std::to_underlying(BorderCellIndex::BOTTOM_LEFT)].u2 = u2;
        mBorderUV[std::to_underlying(BorderCellIndex::BOTTOM_LEFT)].v1 = v1;
        mBorderUV[std::to_underlying(BorderCellIndex::BOTTOM_LEFT)].v2 = v2;
        mGeomUVsOutOfDate = true;
    }
    //---------------------------------------------------------------------
    void BorderPanelOverlayElement::setBottomRightBorderUV(Real u1, Real v1, Real u2, Real v2)
    {
        mBorderUV[std::to_underlying(BorderCellIndex::BOTTOM_RIGHT)].u1 = u1;
        mBorderUV[std::to_underlying(BorderCellIndex::BOTTOM_RIGHT)].u2 = u2;
        mBorderUV[std::to_underlying(BorderCellIndex::BOTTOM_RIGHT)].v1 = v1;
        mBorderUV[std::to_underlying(BorderCellIndex::BOTTOM_RIGHT)].v2 = v2;
        mGeomUVsOutOfDate = true;
    }

    //---------------------------------------------------------------------
    auto BorderPanelOverlayElement::getLeftBorderUVString() const -> String
    {
        return getCellUVString(BorderCellIndex::LEFT);
    }
    //---------------------------------------------------------------------
    auto BorderPanelOverlayElement::getRightBorderUVString() const -> String
    {
        return getCellUVString(BorderCellIndex::RIGHT);
    }
    //---------------------------------------------------------------------
    auto BorderPanelOverlayElement::getTopBorderUVString() const -> String
    {
        return getCellUVString(BorderCellIndex::TOP);
    }
    //---------------------------------------------------------------------
    auto BorderPanelOverlayElement::getBottomBorderUVString() const -> String
    {
        return getCellUVString(BorderCellIndex::BOTTOM);
    }
    //---------------------------------------------------------------------
    auto BorderPanelOverlayElement::getTopLeftBorderUVString() const -> String
    {
        return getCellUVString(BorderCellIndex::TOP_LEFT);
    }
    //---------------------------------------------------------------------
    auto BorderPanelOverlayElement::getTopRightBorderUVString() const -> String
    {
        return getCellUVString(BorderCellIndex::TOP_RIGHT);
    }
    //---------------------------------------------------------------------
    auto BorderPanelOverlayElement::getBottomLeftBorderUVString() const -> String
    {
        return getCellUVString(BorderCellIndex::BOTTOM_LEFT);
    }
    //---------------------------------------------------------------------
    auto BorderPanelOverlayElement::getBottomRightBorderUVString() const -> String
    {
        return getCellUVString(BorderCellIndex::BOTTOM_RIGHT);
    }





    //---------------------------------------------------------------------
    void BorderPanelOverlayElement::setBorderMaterialName(std::string_view name, std::string_view group)
    {
        mBorderMaterial = MaterialManager::getSingleton().getByName(name, group);
        if (!mBorderMaterial)
            OGRE_EXCEPT( ExceptionCodes::ITEM_NOT_FOUND, ::std::format("Could not find material {}", name),
                "BorderPanelOverlayElement::setBorderMaterialName" );
        mBorderMaterial->load();
        // Set some prerequisites to be sure
        mBorderMaterial->setLightingEnabled(false);
        mBorderMaterial->setDepthCheckEnabled(false);
        mBorderMaterial->setReceiveShadows(false);

    }
    //---------------------------------------------------------------------
    auto BorderPanelOverlayElement::getBorderMaterialName() const noexcept -> std::string_view
    {
        return mBorderMaterial ? mBorderMaterial->getName() : BLANKSTRING;
    }
    //---------------------------------------------------------------------
    void BorderPanelOverlayElement::updatePositionGeometry()
    {
        /*
        Grid is like this:
        +--+---------------+--+
        |0 |       1       |2 |
        +--+---------------+--+
        |  |               |  |
        |  |               |  |
        |3 |    center     |4 |
        |  |               |  |
        +--+---------------+--+
        |5 |       6       |7 |
        +--+---------------+--+
        */
        // Convert positions into -1, 1 coordinate space (homogenous clip space)
        // Top / bottom also need inverting since y is upside down
        Real left[8], right[8], top[8], bottom[8];
        // Horizontal
        left[0] = left[3] = left[5] = _getDerivedLeft() * 2 - 1;
        left[1] = left[6] = right[0] = right[3] = right[5] = left[0] + (mLeftBorderSize * 2);
        right[2] = right[4] = right[7] = left[0] + (mWidth * 2);
        left[2] = left[4] = left[7] = right[1] = right[6] = right[2] - (mRightBorderSize * 2);
        // Vertical
        top[0] = top[1] = top[2] = -((_getDerivedTop() * 2) - 1);
        top[3] = top[4] = bottom[0] = bottom[1] = bottom[2] = top[0] - (mTopBorderSize * 2);
        bottom[5] = bottom[6] = bottom[7] = top[0] -  (mHeight * 2);
        top[5] = top[6] = top[7] = bottom[3] = bottom[4] = bottom[5] + (mBottomBorderSize * 2);

        // Lock the whole position buffer in discard mode
        HardwareVertexBufferSharedPtr vbuf = 
            mRenderOp2.vertexData->vertexBufferBinding->getBuffer(POSITION_BINDING);
        HardwareBufferLockGuard vbufLock(vbuf, HardwareBuffer::LockOptions::DISCARD);
        auto* pPos = static_cast<float*>(vbufLock.pData);
        // Use the furthest away depth value, since materials should have depth-check off
        // This initialised the depth buffer for any 3D objects in front
        Real zValue = Root::getSingleton().getRenderSystem()->getMaximumDepthInputValue();
        for (ushort cell = 0; cell < 8; ++cell)
        {
            /*
                0-----2
                |    /|
                |  /  |
                |/    |
                1-----3
            */
            *pPos++ = left[cell];
            *pPos++ = top[cell];
            *pPos++ = zValue;

            *pPos++ = left[cell];
            *pPos++ = bottom[cell];
            *pPos++ = zValue;

            *pPos++ = right[cell];
            *pPos++ = top[cell];
            *pPos++ = zValue;

            *pPos++ = right[cell];
            *pPos++ = bottom[cell];
            *pPos++ = zValue;

        }
        vbufLock.unlock();

        // Also update center geometry
        // NB don't use superclass because we need to make it smaller because of border
        vbuf = mRenderOp.vertexData->vertexBufferBinding->getBuffer(POSITION_BINDING);
        vbufLock.lock(vbuf, HardwareBuffer::LockOptions::DISCARD);
        pPos = static_cast<float*>(vbufLock.pData);
        // Use cell 1 and 3 to determine positions
        *pPos++ = left[1];
        *pPos++ = top[3];
        *pPos++ = zValue;

        *pPos++ = left[1];
        *pPos++ = bottom[3];
        *pPos++ = zValue;

        *pPos++ = right[1];
        *pPos++ = top[3];
        *pPos++ = zValue;

        *pPos++ = right[1];
        *pPos++ = bottom[3];
        *pPos++ = zValue;

        vbufLock.unlock();
    }
    //---------------------------------------------------------------------
    void BorderPanelOverlayElement::_updateRenderQueue(RenderQueue* queue)
    {
        // Add self twice to the queue
        // Have to do this to allow 2 materials
        if (mVisible)
        {

            // Add outer
            queue->addRenderable(mBorderRenderable.get(), RenderQueueGroupID::OVERLAY, mZOrder);

            // do inner last so the border artifacts don't overwrite the children
            // Add inner
            PanelOverlayElement::_updateRenderQueue(queue);
        }
    }
    //---------------------------------------------------------------------
    void BorderPanelOverlayElement::visitRenderables(Renderable::Visitor* visitor, 
        bool debugRenderables)
    {
        visitor->visit(mBorderRenderable.get(), 0, false);
        PanelOverlayElement::visitRenderables(visitor, debugRenderables);
    }
    //-----------------------------------------------------------------------
    void BorderPanelOverlayElement::setMetricsMode(GuiMetricsMode gmm)
    {
        PanelOverlayElement::setMetricsMode(gmm);
        if (gmm != GuiMetricsMode::RELATIVE)
        {
            mPixelBottomBorderSize = static_cast<unsigned short>(mBottomBorderSize);
            mPixelLeftBorderSize = static_cast<unsigned short>(mLeftBorderSize);
            mPixelRightBorderSize = static_cast<unsigned short>(mRightBorderSize);
            mPixelTopBorderSize = static_cast<unsigned short>(mTopBorderSize);
        }
    }
    //-----------------------------------------------------------------------
    void BorderPanelOverlayElement::_update()
    {
        if (mMetricsMode != GuiMetricsMode::RELATIVE && 
            mGeomPositionsOutOfDate)
        {
            mLeftBorderSize = mPixelLeftBorderSize * mPixelScaleX;
            mRightBorderSize = mPixelRightBorderSize * mPixelScaleX;
            mTopBorderSize = mPixelTopBorderSize * mPixelScaleY;
            mBottomBorderSize = mPixelBottomBorderSize * mPixelScaleY;
            mGeomPositionsOutOfDate = true;
        }
        PanelOverlayElement::_update();
    }
    //-----------------------------------------------------------------------
    //---------------------------------------------------------------------
    //---------------------------------------------------------------------
    // Command objects
    //---------------------------------------------------------------------
    //-----------------------------------------------------------------------
    auto CmdBorderSize::doGet(const void* target) const -> String
    {
        const auto* t = static_cast<const BorderPanelOverlayElement*>(target);
        return
        ::std::format
        (   "{} {} {} {}"
        ,   t->getLeftBorderSize()
        ,   t->getRightBorderSize()
        ,   t->getTopBorderSize()
        ,   t->getBottomBorderSize()
        );
    }
    void CmdBorderSize::doSet(void* target, std::string_view val)
    {
        auto const vec = StringUtil::split(val);

        static_cast<BorderPanelOverlayElement*>(target)->setBorderSize(
            StringConverter::parseReal(vec[0]),
            StringConverter::parseReal(vec[1]),
            StringConverter::parseReal(vec[2]),
            StringConverter::parseReal(vec[3])
            );
    }
    //-----------------------------------------------------------------------
    auto CmdBorderMaterial::doGet(const void* target) const -> String
    {
        // No need right now..
        return std::string{ static_cast<const BorderPanelOverlayElement*>(target)->getBorderMaterialName() };
    }
    void CmdBorderMaterial::doSet(void* target, std::string_view val)
    {
        auto const vec = StringUtil::split(val);

        static_cast<BorderPanelOverlayElement*>(target)->setBorderMaterialName(val);
    }
    //-----------------------------------------------------------------------
    auto CmdBorderBottomLeftUV::doGet(const void* target) const -> String
    {
        // No need right now..
        return  static_cast<const BorderPanelOverlayElement*>(target)->getBottomLeftBorderUVString();
    }
    void CmdBorderBottomLeftUV::doSet(void* target, std::string_view val)
    {
        auto const vec = StringUtil::split(val);

        static_cast<BorderPanelOverlayElement*>(target)->setBottomLeftBorderUV(
            StringConverter::parseReal(vec[0]),
            StringConverter::parseReal(vec[1]),
            StringConverter::parseReal(vec[2]),
            StringConverter::parseReal(vec[3])
            );
    }
    //-----------------------------------------------------------------------
    auto CmdBorderBottomRightUV::doGet(const void* target) const -> String
    {
        // No need right now..
        return  static_cast<const BorderPanelOverlayElement*>(target)->getBottomRightBorderUVString();
    }
    void CmdBorderBottomRightUV::doSet(void* target, std::string_view val)
    {
        auto const vec = StringUtil::split(val);

        static_cast<BorderPanelOverlayElement*>(target)->setBottomRightBorderUV(
            StringConverter::parseReal(vec[0]),
            StringConverter::parseReal(vec[1]),
            StringConverter::parseReal(vec[2]),
            StringConverter::parseReal(vec[3])
            );
    }
    //-----------------------------------------------------------------------
    auto CmdBorderTopLeftUV::doGet(const void* target) const -> String
    {
        // No need right now..
        return  static_cast<const BorderPanelOverlayElement*>(target)->getTopLeftBorderUVString();
    }
    void CmdBorderTopLeftUV::doSet(void* target, std::string_view val)
    {
        auto const vec = StringUtil::split(val);

        static_cast<BorderPanelOverlayElement*>(target)->setTopLeftBorderUV(
            StringConverter::parseReal(vec[0]),
            StringConverter::parseReal(vec[1]),
            StringConverter::parseReal(vec[2]),
            StringConverter::parseReal(vec[3])
            );
    }
    //-----------------------------------------------------------------------
    auto CmdBorderTopRightUV::doGet(const void* target) const -> String
    {
        // No need right now..
        return  static_cast<const BorderPanelOverlayElement*>(target)->getTopRightBorderUVString();
    }
    void CmdBorderTopRightUV::doSet(void* target, std::string_view val)
    {
        auto const vec = StringUtil::split(val);

        static_cast<BorderPanelOverlayElement*>(target)->setTopRightBorderUV(
            StringConverter::parseReal(vec[0]),
            StringConverter::parseReal(vec[1]),
            StringConverter::parseReal(vec[2]),
            StringConverter::parseReal(vec[3])
            );
    }
    //-----------------------------------------------------------------------
    auto CmdBorderLeftUV::doGet(const void* target) const -> String
    {
        // No need right now..
        return  static_cast<const BorderPanelOverlayElement*>(target)->getLeftBorderUVString();
    }
    void CmdBorderLeftUV::doSet(void* target, std::string_view val)
    {
        auto const vec = StringUtil::split(val);

        static_cast<BorderPanelOverlayElement*>(target)->setLeftBorderUV(
            StringConverter::parseReal(vec[0]),
            StringConverter::parseReal(vec[1]),
            StringConverter::parseReal(vec[2]),
            StringConverter::parseReal(vec[3])
            );
    }
    //-----------------------------------------------------------------------
    auto CmdBorderRightUV::doGet(const void* target) const -> String
    {
        // No need right now..
        return  static_cast<const BorderPanelOverlayElement*>(target)->getRightBorderUVString();
    }
    void CmdBorderRightUV::doSet(void* target, std::string_view val)
    {
        auto const vec = StringUtil::split(val);

        static_cast<BorderPanelOverlayElement*>(target)->setRightBorderUV(
            StringConverter::parseReal(vec[0]),
            StringConverter::parseReal(vec[1]),
            StringConverter::parseReal(vec[2]),
            StringConverter::parseReal(vec[3])
            );
    }
    //-----------------------------------------------------------------------
    auto CmdBorderTopUV::doGet(const void* target) const -> String
    {
        // No need right now..
        return  static_cast<const BorderPanelOverlayElement*>(target)->getTopBorderUVString();
    }
    void CmdBorderTopUV::doSet(void* target, std::string_view val)
    {
        auto const vec = StringUtil::split(val);

        static_cast<BorderPanelOverlayElement*>(target)->setTopBorderUV(
            StringConverter::parseReal(vec[0]),
            StringConverter::parseReal(vec[1]),
            StringConverter::parseReal(vec[2]),
            StringConverter::parseReal(vec[3])
            );
    }
    //-----------------------------------------------------------------------
    auto CmdBorderBottomUV::doGet(const void* target) const -> String
    {
        // No need right now..
        return  static_cast<const BorderPanelOverlayElement*>(target)->getBottomBorderUVString();
    }
    void CmdBorderBottomUV::doSet(void* target, std::string_view val)
    {
        auto const vec = StringUtil::split(val);

        static_cast<BorderPanelOverlayElement*>(target)->setBottomBorderUV(
            StringConverter::parseReal(vec[0]),
            StringConverter::parseReal(vec[1]),
            StringConverter::parseReal(vec[2]),
            StringConverter::parseReal(vec[3])
            );
    }
    //---------------------------------------------------------------------
    auto BorderPanelOverlayElement::getTypeName() const noexcept -> std::string_view
    {
        return msTypeName;
    }



}

