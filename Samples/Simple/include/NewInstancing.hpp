export module Ogre.Samples:NewInstancing;

export import :SdkSample;

export import Ogre.Components.Bites;
export import Ogre.Core;

export import <set>;
export import <vector>;

using namespace Ogre;

using namespace OgreBites;

export
enum
{   NUM_TECHNIQUES = (((int)InstanceManager::InstancingTechniquesCount) + 1)
};

export
class Sample_NewInstancing : public SdkSample
{
public:

    Sample_NewInstancing();

    auto frameRenderingQueued(const FrameEvent& evt) noexcept -> bool override;

    auto keyPressed(const KeyDownEvent& evt) noexcept -> bool override;

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
    auto lookAt( const Vector3 &normDir ) -> Quaternion;

    static int constexpr NUM_INST_ROW = 100;
    static int constexpr NUM_INST_COLUMN = 100;
    std::vector<MovableObject*>     mEntities;
    std::vector<InstancedEntity*>   mMovedInstances;
    std::vector<SceneNode*>         mSceneNodes;
    std::set<AnimationState*>       mAnimations;
    InstanceManager                 *mCurrentManager;
};
