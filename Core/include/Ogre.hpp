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
#ifndef OGRE_H
#define OGRE_H
// This file includes all the other files which you will need to build a client application
#include "OgrePrerequisites.hpp"

#include "OgreAnimation.hpp"
#include "OgreAnimationState.hpp"
#include "OgreAnimationTrack.hpp"
#include "OgreAny.hpp"
#include "OgreArchive.hpp"
#include "OgreArchiveManager.hpp"
#include "OgreAxisAlignedBox.hpp"
#include "OgreBillboard.hpp"
#include "OgreBillboardChain.hpp"
#include "OgreBillboardSet.hpp"
#include "OgreBone.hpp"
#include "OgreCamera.hpp"
#include "OgreCompositionPass.hpp"
#include "OgreCompositionTargetPass.hpp"
#include "OgreCompositionTechnique.hpp"
#include "OgreCompositor.hpp"
#include "OgreCompositorChain.hpp"
#include "OgreCompositorInstance.hpp"
#include "OgreCompositorManager.hpp"
#include "OgreConfigFile.hpp"
#include "OgreControllerManager.hpp"
#include "OgreDataStream.hpp"
#include "OgreEntity.hpp"
#include "OgreException.hpp"
#include "OgreFrameListener.hpp"
#include "OgreFrustum.hpp"
#include "OgreGpuProgram.hpp"
#include "OgreGpuProgramManager.hpp"
#include "OgreHardwareBufferManager.hpp"
#include "OgreHardwareIndexBuffer.hpp"
#include "OgreHardwareOcclusionQuery.hpp"
#include "OgreHardwarePixelBuffer.hpp"
#include "OgreHardwareVertexBuffer.hpp"
#include "OgreHighLevelGpuProgram.hpp"
#include "OgreInstanceBatch.hpp"
#include "OgreInstanceManager.hpp"
#include "OgreInstancedEntity.hpp"
#include "OgreKeyFrame.hpp"
#include "OgreLight.hpp"
#include "OgreLogManager.hpp"
#include "OgreManualObject.hpp"
#include "OgreMaterial.hpp"
#include "OgreMaterialManager.hpp"
#include "OgreMaterialSerializer.hpp"
#include "OgreMath.hpp"
#include "OgreMatrix3.hpp"
#include "OgreMatrix4.hpp"
#include "OgreMesh.hpp"
#include "OgreMeshManager.hpp"
#include "OgreMeshSerializer.hpp"
#include "OgreMovablePlane.hpp"
#include "OgreParticleAffector.hpp"
#include "OgreParticleEmitter.hpp"
#include "OgreParticleSystem.hpp"
#include "OgreParticleSystemManager.hpp"
#include "OgrePass.hpp"
#include "OgrePatchMesh.hpp"
#include "OgrePatchSurface.hpp"
#include "OgreProfiler.hpp"
#include "OgreRectangle2D.hpp"
#include "OgreRenderObjectListener.hpp"
#include "OgreRenderQueueListener.hpp"
#include "OgreRenderSystem.hpp"
#include "OgreRenderTargetListener.hpp"
#include "OgreRenderTexture.hpp"
#include "OgreRenderWindow.hpp"
#include "OgreResourceBackgroundQueue.hpp"
#include "OgreResourceGroupManager.hpp"
#include "OgreRibbonTrail.hpp"
#include "OgreRoot.hpp"
#include "OgreSceneManager.hpp"
#include "OgreSceneManagerEnumerator.hpp"
#include "OgreSceneNode.hpp"
#include "OgreShadowCameraSetup.hpp"
#include "OgreShadowCameraSetupFocused.hpp"
#include "OgreShadowCameraSetupLiSPSM.hpp"
#include "OgreSimpleRenderable.hpp"
#include "OgreSkeleton.hpp"
#include "OgreSkeletonInstance.hpp"
#include "OgreSkeletonManager.hpp"
#include "OgreSkeletonSerializer.hpp"
#include "OgreStaticGeometry.hpp"
#include "OgreString.hpp"
#include "OgreStringConverter.hpp"
#include "OgreStringVector.hpp"
#include "OgreSubEntity.hpp"
#include "OgreSubMesh.hpp"
#include "OgreTechnique.hpp"
#include "OgreTextureManager.hpp"
#include "OgreTextureUnitState.hpp"
#include "OgreTimer.hpp"
#include "OgreUnifiedHighLevelGpuProgram.hpp"
#include "OgreVector.hpp"
#include "OgreViewport.hpp"
// .... more to come

#endif
