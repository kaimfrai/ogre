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

export import :AlignedAllocator;
export import :Animable;
export import :Animation;
export import :AnimationState;
export import :AnimationTrack;
export import :Any;
export import :Archive;
export import :ArchiveFactory;
export import :ArchiveManager;
export import :AutoParamDataSource;
export import :AxisAlignedBox;
export import :Billboard;
export import :BillboardChain;
export import :BillboardParticleRenderer;
export import :BillboardSet;
export import :Bitwise;
export import :BlendMode;
export import :Bone;
export import :Camera;
export import :Codec;
export import :ColourValue;
export import :Common;
export import :CompositionPass;
export import :CompositionTargetPass;
export import :CompositionTechnique;
export import :Compositor;
export import :CompositorChain;
export import :CompositorInstance;
export import :CompositorLogic;
export import :CompositorManager;
export import :Config;
export import :ConfigDialog;
export import :ConfigFile;
export import :ConfigOptionMap;
export import :Controller;
export import :ControllerManager;
export import :ConvexBody;
export import :CustomCompositionPass;
export import :DataStream;
export import :DefaultDebugDrawer;
export import :DefaultHardwareBufferManager;
export import :DefaultWorkQueueStandard;
export import :Deflate;
export import :DepthBuffer;
export import :DistanceLodStrategy;
export import :DualQuaternion;
export import :DynLib;
export import :DynLibManager;
export import :EdgeListBuilder;
export import :Entity;
export import :Exception;
export import :ExternalTextureSource;
export import :ExternalTextureSourceManager;
export import :FactoryObj;
export import :FileSystem;
export import :FileSystemLayer;
export import :FrameListener;
export import :Frustum;
export import :GpuProgram;
export import :GpuProgramManager;
export import :GpuProgramParams;
export import :GpuProgramUsage;
export import :HardwareBuffer;
export import :HardwareBufferManager;
export import :HardwareIndexBuffer;
export import :HardwareOcclusionQuery;
export import :HardwarePixelBuffer;
export import :HardwareVertexBuffer;
export import :HighLevelGpuProgram;
export import :Image;
export import :ImageCodec;
export import :InstanceBatch;
export import :InstanceBatchHW;
export import :InstanceBatchHW_VTF;
export import :InstanceBatchShader;
export import :InstanceBatchVTF;
export import :InstanceManager;
export import :InstancedEntity;
export import :IteratorWrapper;
export import :KeyFrame;
export import :Light;
export import :LodListener;
export import :LodStrategy;
export import :LodStrategyManager;
export import :Log;
export import :LogManager;
export import :ManualObject;
export import :Material;
export import :MaterialManager;
export import :MaterialSerializer;
export import :Math;
export import :Matrix3;
export import :Matrix4;
export import :MemoryAllocatorConfig;
export import :Mesh;
export import :MeshManager;
export import :MeshSerializer;
export import :MeshSerializerImpl;
export import :MovableObject;
export import :MovablePlane;
export import :MurmurHash3;
export import :NameGenerator;
export import :Node;
export import :OptimisedUtil;
export import :Particle;
export import :ParticleAffector;
export import :ParticleAffectorFactory;
export import :ParticleEmitter;
export import :ParticleEmitterFactory;
export import :ParticleSystem;
export import :ParticleSystemManager;
export import :ParticleSystemRenderer;
export import :Pass;
export import :PatchMesh;
export import :PatchSurface;
export import :PixelCountLodStrategy;
export import :PixelFormat;
export import :Plane;
export import :PlaneBoundedVolume;
export import :Platform;
export import :PlatformInformation;
export import :Plugin;
export import :Polygon;
export import :Pose;
export import :PredefinedControllers;
export import :Prerequisites;
export import :Profiler;
export import :Quaternion;
export import :Ray;
export import :Rectangle2D;
export import :RenderObjectListener;
export import :RenderOperation;
export import :RenderQueue;
export import :RenderQueueListener;
export import :RenderQueueSortingGrouping;
export import :RenderSystem;
export import :RenderSystemCapabilities;
export import :RenderSystemCapabilitiesManager;
export import :RenderSystemCapabilitiesSerializer;
export import :RenderTarget;
export import :RenderTargetListener;
export import :RenderTexture;
export import :RenderToVertexBuffer;
export import :RenderWindow;
export import :Renderable;
export import :Resource;
export import :ResourceBackgroundQueue;
export import :ResourceGroupManager;
export import :ResourceManager;
export import :RibbonTrail;
export import :Root;
export import :RotationalSpline;
export import :SceneManager;
export import :SceneManagerEnumerator;
export import :SceneNode;
export import :SceneQuery;
export import :ScriptCompiler;
export import :ScriptLoader;
export import :ScriptTranslator;
export import :Serializer;
export import :ShadowCameraSetup;
export import :ShadowCameraSetupFocused;
export import :ShadowCameraSetupLiSPSM;
export import :ShadowCaster;
export import :SharedPtr;
export import :SimpleRenderable;
export import :SimpleSpline;
export import :Singleton;
export import :Skeleton;
export import :SkeletonFileFormat;
export import :SkeletonInstance;
export import :SkeletonManager;
export import :SkeletonSerializer;
export import :Sphere;
export import :StaticGeometry;
export import :StdHeaders;
export import :StreamSerialiser;
export import :String;
export import :StringConverter;
export import :StringInterface;
export import :StringVector;
export import :SubEntity;
export import :SubMesh;
export import :TagPoint;
export import :TangentSpaceCalc;
export import :Technique;
export import :Texture;
export import :TextureManager;
export import :TextureUnitState;
export import :Timer;
export import :UnifiedHighLevelGpuProgram;
export import :UserObjectBindings;
export import :Vector;
export import :VertexBoneAssignment;
export import :VertexIndexData;
export import :Viewport;
export import :WorkQueue;
export import :Zip;
