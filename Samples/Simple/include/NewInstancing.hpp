#ifndef OGRE_SAMPLES_NEWINSTANCING_H
#define OGRE_SAMPLES_NEWINSTANCING_H

#include <set>
#include <vector>

#include "OgreInput.hpp"
#include "OgreInstanceManager.hpp"
#include "OgrePrerequisites.hpp"
#include "OgreQuaternion.hpp"
#include "SdkSample.hpp"

namespace Ogre {
class AnimationState;
class InstancedEntity;
class MovableObject;
class SceneNode;
struct FrameEvent;
}  // namespace Ogre

using namespace Ogre;
using namespace OgreBites;

#define NUM_TECHNIQUES (((int)InstanceManager::InstancingTechniquesCount) + 1)

class Sample_NewInstancing : public SdkSample
{
public:

    Sample_NewInstancing();

    bool frameRenderingQueued(const FrameEvent& evt) override;

    bool keyPressed(const KeyboardEvent& evt) override;

protected:
    void setupContent() override;

    void setupLighting();

    void switchInstancingTechnique();

    void createEntities();

    void createInstancedEntities();

    void createSceneNodes();
    
    void clearScene();

    void destroyManagers();

    void cleanupContent() override;

    void animateUnits( float timeSinceLast );

    void moveUnits( float timeSinceLast );

    //Helper function to look towards normDir, where this vector is normalized, with fixed Yaw
    Quaternion lookAt( const Vector3 &normDir );

    static int constexpr NUM_INST_ROW = 100;
    static int constexpr NUM_INST_COLUMN = 100;
    std::vector<MovableObject*>     mEntities;
    std::vector<InstancedEntity*>   mMovedInstances;
    std::vector<SceneNode*>         mSceneNodes;
    std::set<AnimationState*>       mAnimations;
    InstanceManager                 *mCurrentManager;
};

#endif
