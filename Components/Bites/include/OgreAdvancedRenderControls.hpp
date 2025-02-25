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
export module Ogre.Components.Bites:AdvancedRenderControls;

export import :Input;

export import Ogre.Core;
export import Ogre.Components.RTShaderSystem;

export
namespace OgreBites {

class TrayManager;
class ParamsPanel;

/** \addtogroup Optional
*  @{
*/
/** \addtogroup Bites
*  @{
*/

/**
   - F:        Toggle frame rate stats on/off
   - G:        Toggle advanced frame stats on/off
   - P         Toggle profiler window on/ off
   - R:        Render mode
               - Wireframe
               - Points
               - Solid
   - T:        Cycle texture filtering
               - Bilinear
               - Trilinear
               - Anisotropic(8)
               - None
   - F2:       RTSS: Set the main viewport material scheme to default material manager scheme.
   - F3:       RTSS: Toggle default shader generator lighting model from per vertex to per pixel.
   - F4:       RTSS: Switch vertex shader outputs compaction policy.
   - F5:       Reload all textures
   - F6:       Take a screenshot
 */
class AdvancedRenderControls : public InputListener {
public:
    AdvancedRenderControls(TrayManager* trayMgr, Ogre::Camera* cam);
    ~AdvancedRenderControls() override;

    auto keyPressed(const KeyDownEvent& evt) noexcept -> bool override;

    void frameRendered(const Ogre::FrameEvent& evt) override;

protected:
    Ogre::Root* mRoot;
    Ogre::Camera* mCamera;      // main camera
    TrayManager* mTrayMgr;      // tray interface manager
    ParamsPanel* mDetailsPanel; // sample details panel

    Ogre::RTShader::ShaderGenerator* mShaderGenerator;
};
/** @} */
/** @} */
} /* namespace OgreBites */
