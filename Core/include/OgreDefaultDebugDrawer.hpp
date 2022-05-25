// This file is part of the OGRE project.
// It is subject to the license terms in the LICENSE file found in the top-level directory
// of this distribution and at https://www.ogre3d.org/licensing.

#ifndef OGRE_CORE_DEFAULTDEBUGDRAWER_H
#define OGRE_CORE_DEFAULTDEBUGDRAWER_H

#include "OgreAxisAlignedBox.hpp"
#include "OgreColourValue.hpp"
#include "OgreManualObject.hpp"
#include "OgreSceneManager.hpp"

namespace Ogre
{
class Affine3;
class Frustum;
class Node;
class SceneNode;
class Viewport;

class DefaultDebugDrawer : public DebugDrawer
{
    ManualObject mLines;
    ManualObject mAxes;
    int mDrawType{0};
    bool mStatic{false};
    void preFindVisibleObjects(SceneManager* source, SceneManager::IlluminationRenderStage irs, Viewport* v) override;
    void postFindVisibleObjects(SceneManager* source, SceneManager::IlluminationRenderStage irs, Viewport* v) override;
    void beginLines();
public:
    enum DrawType
    {
        DT_AXES = 1 << 0,
        DT_WIREBOX = 1 << 1
    };

    DefaultDebugDrawer();

    /// if static, the drawer contents are preserved across frames. They are cleared otherwise.
    void setStatic(bool enable) { mStatic = enable; }

    void drawBone(const Node* node) override;
    void drawSceneNode(const SceneNode* node) override;
    void drawFrustum(const Frustum* frust) override;
    /// Allows the rendering of a wireframe bounding box.
    void drawWireBox(const AxisAlignedBox& aabb, const ColourValue& colour = ColourValue::White);
    /// draw coordinate axes
    void drawAxes(const Affine3& pose, float size = 1.0f);
};

} /* namespace Ogre */

#endif // OGRE_CORE_DEFAULTDEBUGDRAWER_H
