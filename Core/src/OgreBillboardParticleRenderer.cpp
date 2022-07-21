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
module Ogre.Core;

import :Billboard;
import :BillboardParticleRenderer;
import :Exception;
import :Math;
import :Particle;
import :StringConverter;
import :StringInterface;

import <string>;

namespace Ogre {
    static std::string_view const constexpr rendererTypeName = "billboard";

    /** Command object for billboard type (see ParamCommand).*/
    class CmdBillboardType : public ParamCommand
    {
    public:
        auto doGet(const void* target) const -> String override;
        void doSet(void* target, std::string_view val) override;
    };
    /** Command object for billboard origin (see ParamCommand).*/
    class CmdBillboardOrigin : public ParamCommand
    {
    public:
        auto doGet(const void* target) const -> String override;
        void doSet(void* target, std::string_view val) override;
    };
    /** Command object for billboard rotation type (see ParamCommand).*/
    class CmdBillboardRotationType : public ParamCommand
    {
    public:
        auto doGet(const void* target) const -> String override;
        void doSet(void* target, std::string_view val) override;
    };
    /** Command object for common direction (see ParamCommand).*/
    class CmdCommonDirection : public ParamCommand
    {
    public:
        auto doGet(const void* target) const -> String override;
        void doSet(void* target, std::string_view val) override;
    };
    /** Command object for common up-vector (see ParamCommand).*/
    class CmdCommonUpVector : public ParamCommand
    {
    public:
        auto doGet(const void* target) const -> String override;
        void doSet(void* target, std::string_view val) override;
    };
    /** Command object for point rendering (see ParamCommand).*/
    class CmdPointRendering : public ParamCommand
    {
    public:
        auto doGet(const void* target) const -> String override;
        void doSet(void* target, std::string_view val) override;
    };
    /** Command object for accurate facing(see ParamCommand).*/
    class CmdAccurateFacing : public ParamCommand
    {
    public:
        auto doGet(const void* target) const -> String override;
        void doSet(void* target, std::string_view val) override;
    };
    static CmdBillboardType msBillboardTypeCmd;
    static CmdBillboardOrigin msBillboardOriginCmd;
    static CmdBillboardRotationType msBillboardRotationTypeCmd;
    static CmdCommonDirection msCommonDirectionCmd;
    static CmdCommonUpVector msCommonUpVectorCmd;
    static CmdPointRendering msPointRenderingCmd;
    static CmdAccurateFacing msAccurateFacingCmd;

    static class CmdStacksAndSlices : public ParamCommand
    {
    public:
        auto doGet(const void* target) const -> String override
        {
            return StringConverter::toString(
                static_cast<const BillboardParticleRenderer*>(target)->getTextureStacksAndSlices());
        }
        void doSet(void* target, std::string_view val) override
        {
            Vector2 tmp = StringConverter::parseVector2(val);
            static_cast<BillboardParticleRenderer*>(target)->setTextureStacksAndSlices(tmp.x, tmp.y);
        }
    } msStacksAndSlicesCmd;

    //-----------------------------------------------------------------------
    BillboardParticleRenderer::BillboardParticleRenderer() : mStacksSlices{1, 1}
    {
        if (createParamDictionary("BillboardParticleRenderer"))
        {
            ParamDictionary* dict = getParamDictionary();
            dict->addParameter(ParameterDef("billboard_type", 
                "The type of billboard to use. 'point' means a simulated spherical particle, " 
                "'oriented_common' means all particles in the set are oriented around common_direction, "
                "'oriented_self' means particles are oriented around their own direction, "
                "'perpendicular_common' means all particles are perpendicular to common_direction, "
                "and 'perpendicular_self' means particles are perpendicular to their own direction.",
                ParameterType::STRING),
                &msBillboardTypeCmd);

            dict->addParameter(ParameterDef("billboard_origin", 
                "This setting controls the fine tuning of where a billboard appears in relation to it's position. "
                "Possible value are: 'top_left', 'top_center', 'top_right', 'center_left', 'center', 'center_right', "
                "'bottom_left', 'bottom_center' and 'bottom_right'. Default value is 'center'.",
                ParameterType::STRING),
                &msBillboardOriginCmd);

            dict->addParameter(ParameterDef("billboard_rotation_type", 
                "This setting controls the billboard rotation type. "
                "'vertex' means rotate the billboard's vertices around their facing direction."
                "'texcoord' means rotate the billboard's texture coordinates. Default value is 'texcoord'.",
                ParameterType::STRING),
                &msBillboardRotationTypeCmd);

            dict->addParameter(ParameterDef("common_direction", 
                "Only useful when billboard_type is oriented_common or perpendicular_common. "
                "When billboard_type is oriented_common, this parameter sets the common orientation for "
                "all particles in the set (e.g. raindrops may all be oriented downwards). "
                "When billboard_type is perpendicular_common, this parameter sets the perpendicular vector for "
                "all particles in the set (e.g. an aureola around the player and parallel to the ground).",
                ParameterType::VECTOR3),
                &msCommonDirectionCmd);

            dict->addParameter(ParameterDef("common_up_vector",
                "Only useful when billboard_type is perpendicular_self or perpendicular_common. This "
                "parameter sets the common up-vector for all particles in the set (e.g. an aureola around "
                "the player and parallel to the ground).",
                ParameterType::VECTOR3),
                &msCommonUpVectorCmd);
            dict->addParameter(ParameterDef("point_rendering",
                "Set whether or not particles will use point rendering "
                "rather than manually generated quads. This allows for faster "
                "rendering of point-oriented particles although introduces some "
                "limitations too such as requiring a common particle size."
                "Possible values are 'true' or 'false'.",
                ParameterType::BOOL),
                &msPointRenderingCmd);
            dict->addParameter(ParameterDef("accurate_facing",
                "Set whether or not particles will be oriented to the camera "
                "based on the relative position to the camera rather than just "
                "the camera direction. This is more accurate but less optimal. "
                "Cannot be combined with point rendering.",
                ParameterType::BOOL),
                &msAccurateFacingCmd);

            dict->addParameter(ParameterDef("texture_sheet_size", "",
                ParameterType::UNSIGNED_INT),
                &msStacksAndSlicesCmd);
        }

        // Create billboard set
        mBillboardSet = ::std::make_unique<BillboardSet>("", 0, true);
        // World-relative axes
        mBillboardSet->setBillboardsInWorldSpace(true);
    }
    //-----------------------------------------------------------------------
    BillboardParticleRenderer::~BillboardParticleRenderer()
    {
        // mBillboardSet is never actually attached to a node, we just passthrough
        // based on the particle system's attachment. So manually notify that it's
        // no longer attached.
        mBillboardSet->_notifyAttached(nullptr);
    }
    //-----------------------------------------------------------------------
    auto BillboardParticleRenderer::getType() const noexcept -> std::string_view
    {
        return rendererTypeName;
    }
    //-----------------------------------------------------------------------
    void BillboardParticleRenderer::_updateRenderQueue(RenderQueue* queue, 
        std::vector<Particle*>& currentParticles, bool cullIndividually)
    {
        mBillboardSet->setCullIndividually(cullIndividually);

        // Update billboard set geometry
        mBillboardSet->beginBillboards(currentParticles.size());
        Billboard bb;

        for (Particle* p : currentParticles)
        {
            bb.mPosition = p->mPosition;

            if (mBillboardSet->getBillboardType() == BillboardType::ORIENTED_SELF ||
                mBillboardSet->getBillboardType() == BillboardType::PERPENDICULAR_SELF)
            {
                // Normalise direction vector
                bb.mDirection = p->mDirection;
                bb.mDirection.normalise();
            }
            bb.mColour = p->mColour;
            bb.mRotation = p->mRotation;
            bb.mTexcoordIndex = p->mTexcoordIndex;
            bb.mOwnDimensions = p->mWidth != mBillboardSet->getDefaultWidth() ||
                                p->mHeight != mBillboardSet->getDefaultHeight();
            if (bb.mOwnDimensions)
            {
                bb.mWidth = p->mWidth;
                bb.mHeight = p->mHeight;
            }
            mBillboardSet->injectBillboard(bb);
        }

        mBillboardSet->endBillboards();

        // Update the queue
        mBillboardSet->_updateRenderQueue(queue);
    }

    void BillboardParticleRenderer::_notifyBoundingBox(const AxisAlignedBox& aabb)
    {
        mBillboardSet->setBounds(aabb, Math::boundingRadiusFromAABB(aabb));
    }
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    auto BillboardParticleRendererFactory::getType() const noexcept -> std::string_view
    {
        return rendererTypeName;
    }
    //-----------------------------------------------------------------------
    auto BillboardParticleRendererFactory::createInstance( 
        std::string_view name ) -> ParticleSystemRenderer*
    {
        return new BillboardParticleRenderer();
    }
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    auto CmdBillboardType::doGet(const void* target) const -> String
    {
        BillboardType t = static_cast<const BillboardParticleRenderer*>(target)->getBillboardType();
        using enum BillboardType;
        switch(t)
        {
        case POINT:
            return "point";
            break;
        case ORIENTED_COMMON:
            return "oriented_common";
            break;
        case ORIENTED_SELF:
            return "oriented_self";
            break;
        case PERPENDICULAR_COMMON:
            return "perpendicular_common";
        case PERPENDICULAR_SELF:
            return "perpendicular_self";
        }
        // Compiler nicety
        return "";
    }
    void CmdBillboardType::doSet(void* target, std::string_view val)
    {
        BillboardType t;
        if (val == "point")
        {
            t = BillboardType::POINT;
        }
        else if (val == "oriented_common")
        {
            t = BillboardType::ORIENTED_COMMON;
        }
        else if (val == "oriented_self")
        {
            t = BillboardType::ORIENTED_SELF;
        }
        else if (val == "perpendicular_common")
        {
            t = BillboardType::PERPENDICULAR_COMMON;
        }
        else if (val == "perpendicular_self")
        {
            t = BillboardType::PERPENDICULAR_SELF;
        }
        else
        {
            OGRE_EXCEPT(ExceptionCodes::INVALIDPARAMS, 
                ::std::format("Invalid billboard_type '{}'", val ), 
                "ParticleSystem::CmdBillboardType::doSet");
        }

        static_cast<BillboardParticleRenderer*>(target)->setBillboardType(t);
    }
    //-----------------------------------------------------------------------
    auto CmdBillboardOrigin::doGet(const void* target) const -> String
    {
        BillboardOrigin o = static_cast<const BillboardParticleRenderer*>(target)->getBillboardOrigin();
        using enum BillboardOrigin;
        switch (o)
        {
        case TOP_LEFT:
            return "top_left";
        case TOP_CENTER:
            return "top_center";
        case TOP_RIGHT:
            return "top_right";
        case CENTER_LEFT:
            return "center_left";
        case CENTER:
            return "center";
        case CENTER_RIGHT:
            return "center_right";
        case BOTTOM_LEFT:
            return "bottom_left";
        case BOTTOM_CENTER:
            return "bottom_center";
        case BOTTOM_RIGHT:
            return "bottom_right";
        }
        // Compiler nicety
        return "";
    }
    void CmdBillboardOrigin::doSet(void* target, std::string_view val)
    {
        BillboardOrigin o;
        if (val == "top_left")
            o = BillboardOrigin::TOP_LEFT;
        else if (val =="top_center")
            o = BillboardOrigin::TOP_CENTER;
        else if (val =="top_right")
            o = BillboardOrigin::TOP_RIGHT;
        else if (val =="center_left")
            o = BillboardOrigin::CENTER_LEFT;
        else if (val =="center")
            o = BillboardOrigin::CENTER;
        else if (val =="center_right")
            o = BillboardOrigin::CENTER_RIGHT;
        else if (val =="bottom_left")
            o = BillboardOrigin::BOTTOM_LEFT;
        else if (val =="bottom_center")
            o = BillboardOrigin::BOTTOM_CENTER;
        else if (val =="bottom_right")
            o = BillboardOrigin::BOTTOM_RIGHT;
        else
        {
            OGRE_EXCEPT(ExceptionCodes::INVALIDPARAMS, 
                ::std::format("Invalid billboard_origin '{}'", val ), 
                "ParticleSystem::CmdBillboardOrigin::doSet");
        }

        static_cast<BillboardParticleRenderer*>(target)->setBillboardOrigin(o);
    }
    //-----------------------------------------------------------------------
    auto CmdBillboardRotationType::doGet(const void* target) const -> String
    {
        BillboardRotationType r = static_cast<const BillboardParticleRenderer*>(target)->getBillboardRotationType();
        using enum BillboardRotationType;
        switch(r)
        {
        case VERTEX:
            return "vertex";
        case TEXCOORD:
            return "texcoord";
        }
        // Compiler nicety
        return "";
    }
    void CmdBillboardRotationType::doSet(void* target, std::string_view val)
    {
        BillboardRotationType r;
        if (val == "vertex")
            r = BillboardRotationType::VERTEX;
        else if (val == "texcoord")
            r = BillboardRotationType::TEXCOORD;
        else
        {
            OGRE_EXCEPT(ExceptionCodes::INVALIDPARAMS, 
                ::std::format("Invalid billboard_rotation_type '{}'", val ), 
                "ParticleSystem::CmdBillboardRotationType::doSet");
        }

        static_cast<BillboardParticleRenderer*>(target)->setBillboardRotationType(r);
    }
    //-----------------------------------------------------------------------
    auto CmdCommonDirection::doGet(const void* target) const -> String
    {
        return StringConverter::toString(
            static_cast<const BillboardParticleRenderer*>(target)->getCommonDirection() );
    }
    void CmdCommonDirection::doSet(void* target, std::string_view val)
    {
        static_cast<BillboardParticleRenderer*>(target)->setCommonDirection(
            StringConverter::parseVector3(val));
    }
    //-----------------------------------------------------------------------
    auto CmdCommonUpVector::doGet(const void* target) const -> String
    {
        return StringConverter::toString(
            static_cast<const BillboardParticleRenderer*>(target)->getCommonUpVector() );
    }
    void CmdCommonUpVector::doSet(void* target, std::string_view val)
    {
        static_cast<BillboardParticleRenderer*>(target)->setCommonUpVector(
            StringConverter::parseVector3(val));
    }
    //-----------------------------------------------------------------------
    auto CmdPointRendering::doGet(const void* target) const -> String
    {
        return StringConverter::toString(
            static_cast<const BillboardParticleRenderer*>(target)->isPointRenderingEnabled() );
    }
    void CmdPointRendering::doSet(void* target, std::string_view val)
    {
        static_cast<BillboardParticleRenderer*>(target)->setPointRenderingEnabled(
            StringConverter::parseBool(val));
    }
    //-----------------------------------------------------------------------
    auto CmdAccurateFacing::doGet(const void* target) const -> String
    {
        return StringConverter::toString(
            static_cast<const BillboardParticleRenderer*>(target)->getUseAccurateFacing() );
    }
    void CmdAccurateFacing::doSet(void* target, std::string_view val)
    {
        static_cast<BillboardParticleRenderer*>(target)->setUseAccurateFacing(
            StringConverter::parseBool(val));
    }

}
