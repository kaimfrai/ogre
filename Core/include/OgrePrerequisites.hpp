/*-------------------------------------------------------------------------
This source file is a part of OGRE
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
THE SOFTWARE
-------------------------------------------------------------------------*/
#ifndef OGRE_CORE_PREREQUISITES_H
#define OGRE_CORE_PREREQUISITES_H

// Platform-specific stuff
#include "OgrePlatform.hpp"

#include <compare>
#include <format>
#include <memory>
#include <string>

namespace Ogre {

    #define OGRE_VERSION    ((/*OGRE_VERSION_MAJOR*/13 << 16) | (/*OGRE_VERSION_MINOR*/3 << 8) | /*OGRE_VERSION_PATCH*/3)

    #define OGRE_MIN_VERSION(MAJOR, MINOR, PATCH) OGRE_VERSION >= ((MAJOR << 16) | (MINOR << 8) | PATCH)

    // define the real number values to be used
    // default to use 'float' unless precompiler option set

    /** Software floating point type.
    @note Not valid as a pointer to GPU buffers / parameters
    */
    using Real = float;

    /** In order to avoid finger-aches :)
    */
    using uchar = unsigned char;
    using ushort = unsigned short;
    using uint = unsigned int;
    using ulong = unsigned long;

// Pre-declare classes
// Allows use of pointers in header files without including individual .h
// so decreases dependencies between files
    struct Affine3;
    class Angle;
    class AnimableValue;
    class Animation;
    class AnimationState;
    class AnimationStateSet;
    class AnimationTrack;
    class ArchiveFactory;
    class ArchiveManager;
    class AutoParamDataSource;
    struct AxisAlignedBox;
    struct AxisAlignedBoxSceneQuery;
    class Billboard;
    class BillboardChain;
    class BillboardSet;
    class Bone;
    class Camera;
    class Codec;
    struct ColourValue;
    class ConfigDialog;
    template <typename T> class Controller;
    template <typename T> class ControllerFunction;
    class ControllerManager;
    template <typename T> class ControllerValue;
    class DataStream;
    class DebugDrawer;
    class DefaultWorkQueue;
    class Degree;
    class DepthBuffer;
    class DynLib;
    class DynLibManager;
    class EdgeData;
    class EdgeListBuilder;
    class Entity;
    class ExternalTextureSourceManager;
    class Factory;
    struct FrameEvent;
    class FrameListener;
    class Frustum;
    struct GpuLogicalBufferStruct;
    struct GpuNamedConstants;
    class GpuProgramParameters;
    class GpuSharedParameters;
    class GpuProgram;
    class GpuProgramFactory;
    using HighLevelGpuProgramFactory = GpuProgramFactory; //!< @deprecated
    class GpuProgramManager;
    using HighLevelGpuProgramManager = GpuProgramManager; //!< @deprecated
    class GpuProgramUsage;
    class HardwareBuffer;
    class HardwareIndexBuffer;
    class HardwareOcclusionQuery;
    class HardwareVertexBuffer;
    class HardwarePixelBuffer;
    class HighLevelGpuProgram;
    class IndexData;
    class InstanceBatch;
    class InstanceBatchHW;
    class InstanceBatchHW_VTF;
    class InstanceBatchShader;
    class InstanceBatchVTF;
    class InstanceManager;
    class InstancedEntity;
    class IntersectionSceneQuery;
    class IntersectionSceneQueryListener;
    class Image;
    class KeyFrame;
    class Light;
    class Log;
    class LogManager;
    class LodStrategy;
    class LodStrategyManager;
    class ManualResourceLoader;
    class ManualObject;
    class Material;
    class MaterialManager;
    class Math;
    struct Matrix3;
    struct Matrix4;
    class MemoryDataStream;
    class MemoryManager;
    class Mesh;
    class MeshSerializer;
    class MeshManager;
    class MovableObject;
    class MovablePlane;
    class Node;
    class NodeAnimationTrack;
    class NodeKeyFrame;
    class NumericAnimationTrack;
    class NumericKeyFrame;
    class Particle;
    class ParticleAffector;
    class ParticleAffectorFactory;
    class ParticleEmitter;
    class ParticleEmitterFactory;
    class ParticleSystem;
    class ParticleSystemManager;
    class ParticleSystemRenderer;
    template<typename T> class FactoryObj;
    using ParticleSystemRendererFactory = FactoryObj<ParticleSystemRenderer>;
    class ParticleVisualData;
    class Pass;
    class PatchMesh;
    class PixelBox;
    class Plane;
    class PlaneBoundedVolume;
    class Plugin;
    class Pose;
    class Profile;
    class Profiler;
    struct Quaternion;
    class Radian;
    class Ray;
    class RaySceneQuery;
    class RaySceneQueryListener;
    class Renderable;
    class RenderPriorityGroup;
    class RenderQueue;
    class RenderQueueGroup;
    class RenderQueueListener;
    class RenderObjectListener;
    class RenderSystem;
    class RenderSystemCapabilities;
    class RenderSystemCapabilitiesManager;
    class RenderSystemCapabilitiesSerializer;
    class RenderTarget;
    class RenderTargetListener;
    class RenderTexture;
    class RenderToVertexBuffer;
    class MultiRenderTarget;
    class RenderWindow;
    class RenderOperation;
    class Resource;
    class ResourceBackgroundQueue;
    class ResourceGroupManager;
    class ResourceManager;
    class RibbonTrail;
    class Root;
    class SceneManager;
    class SceneManagerEnumerator;
    class SceneNode;
    class SceneQuery;
    class SceneQueryListener;
    class ScriptCompiler;
    class ScriptCompilerManager;
    class ScriptLoader;
    class Serializer;
    class ShadowCameraSetup;
    class ShadowCaster;
    class ShadowRenderable;
    class SimpleRenderable;
    class SimpleSpline;
    class Skeleton;
    class SkeletonInstance;
    class SkeletonManager;
    class Sphere;
    class SphereSceneQuery;
    class StaticGeometry;
    class StreamSerialiser;
    class StringConverter;
    class StringInterface;
    class SubEntity;
    class SubMesh;
    class TagPoint;
    class Technique;
    class TempBlendedBufferInfo;
    class ExternalTextureSource;
    class TextureUnitState;
    class Texture;
    class TextureManager;
    class TransformKeyFrame;
    class Timer;
    class UserObjectBindings;
    template <int dims, typename T> struct Vector;
    using Vector2 = Vector<2, Real>;
    using Vector2i = Vector<2, int>;
    using Vector3 = Vector<3, Real>;
    using Vector3f = Vector<3, float>;
    using Vector3i = Vector<3, int>;
    using Vector4 = Vector<4, Real>;
    using Vector4f = Vector<4, float>;
    class Viewport;
    class VertexAnimationTrack;
    class VertexBufferBinding;
    class VertexData;
    class VertexDeclaration;
    class VertexMorphKeyFrame;
    class WireBoundingBox;
    class WorkQueue;
    class Compositor;
    class CompositorManager;
    class CompositorChain;
    class CompositorInstance;
    class CompositorLogic;
    class CompositionTechnique;
    class CompositionPass;
    class CompositionTargetPass;
    class CustomCompositionPass;

    using std::shared_ptr;
    using std::unique_ptr;
    template<typename T> class SharedPtr;

    using AnimableValuePtr = SharedPtr<AnimableValue>;
    using CompositorPtr = SharedPtr<Compositor>;
    using DataStreamPtr = SharedPtr<DataStream>;
    using GpuProgramPtr = SharedPtr<GpuProgram>;
    using GpuNamedConstantsPtr = SharedPtr<GpuNamedConstants>;
    using GpuLogicalBufferStructPtr = SharedPtr<GpuLogicalBufferStruct>;
    using GpuSharedParametersPtr = SharedPtr<GpuSharedParameters>;
    using GpuProgramParametersPtr = SharedPtr<GpuProgramParameters>;
    using HardwareBufferPtr = SharedPtr<HardwareBuffer>;
    using HardwareIndexBufferPtr = SharedPtr<HardwareIndexBuffer>;
    using HardwarePixelBufferPtr = SharedPtr<HardwarePixelBuffer>;
    using HardwareVertexBufferPtr = SharedPtr<HardwareVertexBuffer>;
    using MaterialPtr = SharedPtr<Material>;
    using MemoryDataStreamPtr = SharedPtr<MemoryDataStream>;
    using MeshPtr = SharedPtr<Mesh>;
    using PatchMeshPtr = SharedPtr<PatchMesh>;
    using RenderToVertexBufferPtr = SharedPtr<RenderToVertexBuffer>;
    using ResourcePtr = SharedPtr<Resource>;
    using ShadowCameraSetupPtr = SharedPtr<ShadowCameraSetup>;
    using SkeletonPtr = SharedPtr<Skeleton>;
    using TexturePtr = SharedPtr<Texture>;

    using RenderToVertexBufferSharedPtr = RenderToVertexBufferPtr; //!< @deprecated
    using HardwareIndexBufferSharedPtr = HardwareIndexBufferPtr; //!< @deprecated
    using HardwarePixelBufferSharedPtr = HardwarePixelBufferPtr; //!< @deprecated
    using HardwareVertexBufferSharedPtr = HardwareVertexBufferPtr; //!< @deprecated
    using HighLevelGpuProgramPtr = GpuProgramPtr; //!< @deprecated
    using HardwareUniformBufferSharedPtr = HardwareBufferPtr; //!< @deprecated
    using HardwareCounterBufferSharedPtr = HardwareBufferPtr; //!< @deprecated
    using GpuProgramParametersSharedPtr = GpuProgramParametersPtr; //!< @deprecated
}

/* Include all the standard header *after* all the configuration
settings have been made.
*/
#include "OgreMemoryAllocatorConfig.hpp"
#include "OgreStdHeaders.hpp"


namespace Ogre
{
    using String = std::string;

    // FIXME: as of 2022 Juli 08 libc++ does not provide operator<=> for std::string_view
    [[nodiscard]] auto inline operator <=> (std::string_view left, std::string_view right) noexcept -> ::std::weak_ordering
    {
        auto const cmp = left.compare(right);
        if (cmp < 0) return ::std::strong_ordering::less;
        if (cmp > 0) return ::std::strong_ordering::greater;
        return ::std::strong_ordering::equal;
    }

    // FIXME: as of 2022 June 08 libc++ does not provide operator<=> for std::string
    [[nodiscard]] auto inline operator <=> (String const& left, String const& right) noexcept -> ::std::weak_ordering
    {
        return std::string_view{left} <=> std::string_view{right};
    }

    using StringStream = std::stringstream;

    template <typename T, size_t Alignment = SIMD_ALIGNMENT>
    using aligned_vector = std::vector<T, AlignedAllocator<T, Alignment>>;
}

#endif // OGRE_CORE_PREREQUISITES_H


