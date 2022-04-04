// This file is part of the OGRE project.
// It is subject to the license terms in the LICENSE file found in the top-level directory
// of this distribution and at https://www.ogre3d.org/licensing.
export module Ogre.RenderSystems.GLSupport:GLRenderTarget;

export
namespace Ogre
{
class GLContext;
class GLFrameBufferObjectCommon;

class GLRenderTarget
{
public:
    virtual ~GLRenderTarget() {}

    /** Get the GL context that needs to be active to render to this target
     */
    virtual GLContext* getContext() const = 0;
    virtual GLFrameBufferObjectCommon* getFBO() { return nullptr; }
};
} // namespace Ogre
