module Ogre.Samples;

import :NewInstancing;

import Ogre.Components.RTShaderSystem;
import Ogre.Core;

import <map>;
import <random>;

namespace Ogre::RTShader {
class SubRenderState;
}  // namespace Ogre

using namespace Ogre;

using namespace OgreBites;
//------------------------------------------------------------------------------
Sample_NewInstancing::Sample_NewInstancing()
{
    mInfo["Title"] = "New Instancing";
    mInfo["Description"] = "Demonstrates how to use the new InstancedManager to setup many dynamic"
        " instances of the same mesh with much less performance impact";
    mInfo["Thumbnail"] = "thumb_newinstancing.png";
    mInfo["Category"] = "Environment";
    mInfo["Help"] = "Press Space to switch Instancing Techniques.\n"
        "Press B to toggle bounding boxes.\n\n"
        "Changes in the slider take effect after switching instancing technique\n"
        "Different batch sizes give different results depending on CPU culling"
        " and instance numbers on the scene.\n\n"
        "If performance is too slow, try defragmenting batches once in a while";
}

//------------------------------------------------------------------------------
auto Sample_NewInstancing::frameRenderingQueued(const FrameEvent& evt) noexcept -> bool
{
    animateUnits( evt.timeSinceLastFrame);

    moveUnits( evt.timeSinceLastFrame);

    return SdkSample::frameRenderingQueued(evt);        // don't forget the parent class updates!
}

//------------------------------------------------------------------------------
auto Sample_NewInstancing::keyPressed(const KeyDownEvent& evt) noexcept -> bool
{
    return SdkSample::keyPressed(evt);
}

//------------------------------------------------------------------------------
void Sample_NewInstancing::setupContent()
{
    // Make this viewport work with shader generator scheme.
    mViewport->setMaterialScheme(RTShader::ShaderGenerator::DEFAULT_SCHEME_NAME);
    RTShader::ShaderGenerator& rtShaderGen = RTShader::ShaderGenerator::getSingleton();
    RTShader::RenderState* schemRenderState = rtShaderGen.getRenderState(RTShader::ShaderGenerator::DEFAULT_SCHEME_NAME);
    RTShader::SubRenderState* subRenderState = rtShaderGen.createSubRenderState<RTShader::IntegratedPSSM3>();
    schemRenderState->addTemplateSubRenderState(subRenderState);

    //Add the hardware skinning to the shader generator default render state
    subRenderState = mShaderGenerator->createSubRenderState<RTShader::HardwareSkinning>();
    schemRenderState->addTemplateSubRenderState(subRenderState);

    // increase max bone count for higher efficiency
    RTShader::HardwareSkinningFactory::getSingleton().setMaxCalculableBoneCount(80);

    // re-generate shaders to include new SRSs
    rtShaderGen.invalidateScheme(RTShader::ShaderGenerator::DEFAULT_SCHEME_NAME);
    rtShaderGen.validateScheme(RTShader::ShaderGenerator::DEFAULT_SCHEME_NAME);

    // update scheme for FFP supporting rendersystems
    MaterialManager::getSingleton().setActiveScheme(mViewport->getMaterialScheme());

    //Initialize the techniques and current mesh variables
    mCurrentManager         = nullptr;

    mSceneMgr->setShadowTechnique( ShadowTechnique::TEXTURE_ADDITIVE_INTEGRATED );
    mSceneMgr->setShadowTextureSelfShadow( true );
    mSceneMgr->setShadowCasterRenderBackFaces( true );

    if (Ogre::Root::getSingletonPtr()->getRenderSystem()->getName().find("OpenGL ES 2") == String::npos)
    {
        mSceneMgr->setShadowTextureConfig( 0, 2048, 2048, PixelFormat::DEPTH16 );
    }
    else
    {
        // Use a smaller texture for GL ES 3.0
        mSceneMgr->setShadowTextureConfig( 0, 512, 512, PixelFormat::DEPTH16 );
    }

    mSceneMgr->setShadowCameraSetup( LiSPSMShadowCameraSetup::create() );

    mEntities.reserve( NUM_INST_ROW * NUM_INST_COLUMN );
    mSceneNodes.reserve( NUM_INST_ROW * NUM_INST_COLUMN );

    mSceneMgr->setSkyBox(true, "Examples/CloudyNoonSkyBox");

    // create a mesh for our ground
    MeshManager::getSingleton().createPlane("ground", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
        Plane{Vector3::UNIT_Y, -0.0f}, 10000, 10000, 20, 20, true, 1, 6, 6, Vector3::UNIT_Z);

    // create a ground entity from our mesh and attach it to the origin
    Entity* ground = mSceneMgr->createEntity("Ground", "ground");
    ground->setMaterialName("Examples/GrassFloor");
    ground->setCastShadows(false);
    mSceneMgr->getRootSceneNode()->attachObject(ground);

    setupLighting();

    // set initial camera position and speed
    mCameraNode->setPosition( 0, 120, 100 );

    setDragLook(true);

    switchInstancingTechnique();
}

//------------------------------------------------------------------------------
void Sample_NewInstancing::setupLighting()
{
    mSceneMgr->setAmbientLight( ColourValue{ 0.40f, 0.40f, 0.40f } );

    //Create main light
    Light* light = mSceneMgr->createLight();
    light->setType( Light::LightTypes::DIRECTIONAL );
    light->setDiffuseColour(1, 0.5, 0.3);
    auto n = mSceneMgr->getRootSceneNode()->createChildSceneNode();
    n->attachObject(light);
    n->setDirection(0, -1, -1);
    light->setSpecularColour( 0.6, 0.82, 1.0 );
}

//------------------------------------------------------------------------------
void Sample_NewInstancing::switchInstancingTechnique()
{
    //Instancing

    //Create the manager if we haven't already (i.e. first time)
    //Because we use InstanceManagerFlags::USEALL as flags, the actual num of instances per batch might be much lower
    //If you're not bandwidth limited, you may want to lift InstanceManagerFlags::VTFBESTFIT flag away

    mCurrentManager = mSceneMgr->createInstanceManager(
        "InstanceMgr", "robot.mesh",
        ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME, InstanceManager::ShaderBased,
        NUM_INST_ROW * NUM_INST_COLUMN, InstanceManagerFlags::USEALL);

    createInstancedEntities();

    createSceneNodes();
}

//------------------------------------------------------------------------------
void Sample_NewInstancing::createInstancedEntities()
{
    std::mt19937 rng;
    std::mt19937 orng; // separate oriention rng for consistency with other techniques
    for( int i=0; i<NUM_INST_ROW; ++i )
    {
        for( int j=0; j<NUM_INST_COLUMN; ++j )
        {
            //Create the instanced entity
            InstancedEntity *ent = mCurrentManager->createInstancedEntity("Examples/Instancing/RTSS/Robot" );
            mEntities.push_back( ent );

            //Get the animation
            AnimationState *anim = ent->getAnimationState( "Walk" );
            anim->setEnabled( true );
            anim->addTime( float(rng())/float(rng.max()) * 10); //Random start offset
            mAnimations.insert( anim );
        }
    }
}

//------------------------------------------------------------------------------
void Sample_NewInstancing::createSceneNodes()
{
    //Here the SceneNodes are created. Since InstancedEntities derive from MovableObject,
    //they behave like regular Entities on this.
    SceneNode *rootNode = mSceneMgr->getRootSceneNode();

    std::mt19937 rng;
    for( int i=0; i<NUM_INST_ROW; ++i )
    {
        for( int j=0; j<NUM_INST_COLUMN; ++j )
        {
            int idx = i * NUM_INST_COLUMN + j;
            SceneNode *sceneNode = rootNode->createChildSceneNode();
            sceneNode->attachObject( mEntities[idx] );
            sceneNode->yaw( Radian{ float(rng())/float(rng.max()) * 10 * Math::PI }); //Random orientation
            sceneNode->setPosition( mEntities[idx]->getBoundingRadius() * (i - NUM_INST_ROW * 0.5f), 0,
                mEntities[idx]->getBoundingRadius() * (j - NUM_INST_COLUMN * 0.5f) );
            mSceneNodes.push_back( sceneNode );
        }
    }
}

//------------------------------------------------------------------------------
void Sample_NewInstancing::clearScene()
{
    //Note: Destroying the instance manager automatically destroys all instanced entities
    //created by this manager (beware of not leaving reference to those pointers)
    for (auto const& itor : mEntities)
    {
        SceneNode *sceneNode = itor->getParentSceneNode();
        if (sceneNode)
        {
            sceneNode->detachAllObjects();
            sceneNode->getParentSceneNode()->removeAndDestroyChild( sceneNode );
        }

        mSceneMgr->destroyInstancedEntity( static_cast<InstancedEntity*>(itor) );
    }

    //Free some memory, but don't destroy the manager so when we switch this technique
    //back again it doesn't take too long
    if( mCurrentManager )
        mCurrentManager->cleanupEmptyBatches();

    mEntities.clear();
    mMovedInstances.clear();
    mSceneNodes.clear();
    mAnimations.clear();
}

//-----------------------------------------------------------------------------------
void Sample_NewInstancing::destroyManagers()
{
    mSceneMgr->destroyInstanceManager(mCurrentManager);;
}

//------------------------------------------------------------------------------
void Sample_NewInstancing::cleanupContent()
{
    MeshManager::getSingleton().remove("ground", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
    clearScene();
    destroyManagers();
}

//------------------------------------------------------------------------------
void Sample_NewInstancing::animateUnits( float timeSinceLast )
{
    //Iterates through all AnimationSets and updates the animation being played. Demonstrates the
    //animation is unique and independent to each instance

    for (auto const& itor : mAnimations)
    {
        itor->addTime( timeSinceLast );
    }
}

//------------------------------------------------------------------------------
void Sample_NewInstancing::moveUnits( float timeSinceLast )
{
    Real fMovSpeed = 1.0f;

    if( !mEntities.empty() )
        fMovSpeed = mEntities[0]->getBoundingRadius() * 0.30f;

    if (!mSceneNodes.empty())
    {
        //Randomly move the units along their normal, bouncing around invisible walls
        for (auto const& itor : mSceneNodes)
        {
            //Calculate bounces
            Vector3 entityPos = itor->getPosition();
            Vector3 planeNormal = Vector3::ZERO;
            if(itor->getPosition().x < -5000.0f )
            {
                planeNormal = Vector3::UNIT_X;
                entityPos.x = -4999.0f;
            }
            else if(itor->getPosition().x > 5000.0f )
            {
                planeNormal = Vector3::NEGATIVE_UNIT_X;
                entityPos.x = 4999.0f;
            }
            else if(itor->getPosition().z < -5000.0f )
            {
                planeNormal = Vector3::UNIT_Z;
                entityPos.z = -4999.0f;
            }
            else if(itor->getPosition().z > 5000.0f )
            {
                planeNormal = Vector3::NEGATIVE_UNIT_Z;
                entityPos.z = 4999.0f;
            }

            if( planeNormal != Vector3::ZERO )
            {
                const auto vDir = itor->getOrientation().xAxis().normalisedCopy();
                itor->setOrientation( lookAt( planeNormal.reflect( vDir ) ) );
                itor->setPosition( entityPos );
            }

            //Move along the direction we're looking to
            itor->translate( Vector3::UNIT_X * timeSinceLast * fMovSpeed, Node::TransformSpace::LOCAL );
        }
    }
    else
    {
        //No scene nodes (instanced entities only)
        //Update instanced entities directly

        //Randomly move the units along their normal, bouncing around invisible walls
        for (InstancedEntity* pEnt : mMovedInstances)
        {
            //Calculate bounces
            Vector3 entityPos = pEnt->getPosition();
            Vector3 planeNormal = Vector3::ZERO;
            if( pEnt->getPosition().x < -5000.0f )
            {
                planeNormal = Vector3::UNIT_X;
                entityPos.x = -4999.0f;
            }
            else if( pEnt->getPosition().x > 5000.0f )
            {
                planeNormal = Vector3::NEGATIVE_UNIT_X;
                entityPos.x = 4999.0f;
            }
            else if( pEnt->getPosition().z < -5000.0f )
            {
                planeNormal = Vector3::UNIT_Z;
                entityPos.z = -4999.0f;
            }
            else if( pEnt->getPosition().z > 5000.0f )
            {
                planeNormal = Vector3::NEGATIVE_UNIT_Z;
                entityPos.z = 4999.0f;
            }

            if( planeNormal != Vector3::ZERO )
            {
                const auto vDir = pEnt->getOrientation().xAxis().normalisedCopy();
                pEnt->setOrientation( lookAt( planeNormal.reflect( vDir ) ), false );
                pEnt->setPosition( entityPos, false);
            }

            //Move along the direction we're looking to
            Vector3 transAmount = Vector3::UNIT_X * timeSinceLast * fMovSpeed;
            pEnt->setPosition( pEnt->getPosition() + pEnt->getOrientation() * transAmount );
        }
    }
}

//------------------------------------------------------------------------------
auto Sample_NewInstancing::lookAt( const Vector3 &normDir ) -> Quaternion
{
    return Quaternion::FromMatrix3(Math::lookRotation(normDir.normalisedCopy(), Vector3::UNIT_Y));
}
