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
export module Ogre.Core;

// This file includes all the other files which you will need to build a client application
import :Animation;
import :AnimationState;
import :AnimationTrack;
import :Any;
import :Archive;
import :ArchiveManager;
import :AxisAlignedBox;
import :Billboard;
import :BillboardChain;
import :BillboardSet;
import :Bone;
import :Camera;
import :CompositionPass;
import :CompositionTargetPass;
import :CompositionTechnique;
import :Compositor;
import :CompositorChain;
import :CompositorInstance;
import :CompositorManager;
import :ConfigFile;
import :ControllerManager;
import :DataStream;
import :Entity;
import :Exception;
import :FrameListener;
import :Frustum;
import :GpuProgram;
import :GpuProgramManager;
import :HardwareBufferManager;
import :HardwareIndexBuffer;
import :HardwareOcclusionQuery;
import :HardwarePixelBuffer;
import :HardwareVertexBuffer;
import :HighLevelGpuProgram;
import :InstanceBatch;
import :InstanceManager;
import :InstancedEntity;
import :KeyFrame;
import :Light;
import :LogManager;
import :ManualObject;
import :Material;
import :MaterialManager;
import :MaterialSerializer;
import :Math;
import :Matrix3;
import :Matrix4;
import :Mesh;
import :MeshManager;
import :MeshSerializer;
import :MovablePlane;
import :ParticleAffector;
import :ParticleEmitter;
import :ParticleSystem;
import :ParticleSystemManager;
import :Pass;
import :PatchMesh;
import :PatchSurface;
import :Prerequisites;
import :Profiler;
import :Rectangle2D;
import :RenderObjectListener;
import :RenderQueueListener;
import :RenderSystem;
import :RenderTargetListener;
import :RenderTexture;
import :RenderWindow;
import :ResourceBackgroundQueue;
import :ResourceGroupManager;
import :RibbonTrail;
import :Root;
import :SceneManager;
import :SceneManagerEnumerator;
import :SceneNode;
import :ShadowCameraSetup;
import :ShadowCameraSetupFocused;
import :ShadowCameraSetupLiSPSM;
import :SimpleRenderable;
import :Skeleton;
import :SkeletonInstance;
import :SkeletonManager;
import :SkeletonSerializer;
import :StaticGeometry;
import :String;
import :StringConverter;
import :StringVector;
import :SubEntity;
import :SubMesh;
import :Technique;
import :TextureManager;
import :TextureUnitState;
import :Timer;
import :UnifiedHighLevelGpuProgram;
import :Vector;
import :Viewport;

// .... more to come
