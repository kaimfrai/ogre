#ifndef OGRE_SAMPLES_NEWINSTANCING_H
#define OGRE_SAMPLES_NEWINSTANCING_H

#include <set>
#include <vector>

#include "OgreInput.hpp"
#include "OgreInstanceManager.hpp"
#include "OgrePlatform.hpp"
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
namespace OgreBites {
class Button;
class CheckBox;
class SelectMenu;
class Slider;
}  // namespace OgreBites

using namespace Ogre;
using namespace OgreBites;

#define NUM_TECHNIQUES (((int)InstanceManager::InstancingTechniquesCount) + 1)

class Sample_NewInstancing : public SdkSample
{
public:

    Sample_NewInstancing();

    auto frameRenderingQueued(const FrameEvent& evt) -> bool override;

    auto keyPressed(const KeyboardEvent& evt) -> bool override;

protected:
    void setupContent() override;

    void setupLighting();
    
    void switchInstancingTechnique();

    void switchSkinningTechnique(int index);

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

    void defragmentBatches();

    void setupGUI();

    void itemSelected(SelectMenu* menu) override;

    void buttonHit( OgreBites::Button* button ) override;

    void checkBoxToggled(CheckBox* box) override;

    void sliderMoved(Slider* slider) override;

    //The difference between testCapabilities() is that features checked here aren't fatal errors.
    //which means the sample can run (with limited functionality) on those computers
    void checkHardwareSupport();

    //You can also use a union type to switch between Entity and InstancedEntity almost flawlessly:
    /*
    union FusionEntity
    {
        Entity          entity
        InstancedEntity instancedEntity;
    };
    */
    int NUM_INST_ROW{100};
    int NUM_INST_COLUMN{100};
    int                             mInstancingTechnique;
    int                             mCurrentMesh;
    std::vector<MovableObject*>     mEntities;
    std::vector<InstancedEntity*>   mMovedInstances;
    std::vector<SceneNode*>         mSceneNodes;
    std::set<AnimationState*>       mAnimations;
    InstanceManager                 *mCurrentManager{nullptr};
    bool                            mSupportedTechniques[NUM_TECHNIQUES+1];
    const char**                        mCurrentMaterialSet;
    uint16                          mCurrentFlags{0};

    SelectMenu                      *mTechniqueMenu;
    CheckBox                        *mMoveInstances;
    CheckBox                        *mAnimateInstances;
    SelectMenu                      *mSkinningTechniques{nullptr};
    CheckBox                        *mEnableShadows;
    CheckBox                        *mSetStatic;
    CheckBox                        *mUseSceneNodes;
    OgreBites::Button                   *mDefragmentBatches;
    CheckBox                        *mDefragmentOptimumCull;
    Slider                          *mInstancesSlider;
};

#endif
