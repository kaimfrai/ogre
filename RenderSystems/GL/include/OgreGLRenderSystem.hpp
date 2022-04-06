/*
-----------------------------------------------------------------------------
This source file is part of OGRE
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org

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
#ifndef OGRE_RENDERSYSTEMS_GL_RENDERSYSTEM_H
#define OGRE_RENDERSYSTEMS_GL_RENDERSYSTEM_H

#include <cstddef>
#include <vector>

#include "OgreBlendMode.hpp"
#include "OgreColourValue.hpp"
#include "OgreCommon.hpp"
#include "OgreConfig.hpp"
#include "OgreGLRenderSystemCommon.hpp"
#include "OgreGpuProgram.hpp"
#include "OgreMatrix4.hpp"
#include "OgrePlane.hpp"
#include "OgrePlatform.hpp"
#include "OgrePrerequisites.hpp"
#include "OgreRenderSystem.hpp"
#include "OgreRenderTarget.hpp"
#include "OgreRenderWindow.hpp"
#include "OgreTextureUnitState.hpp"
#include "glad/glad.h"

namespace Ogre {
class DepthBuffer;
class Frustum;
class GLContext;
class GLGpuProgramManager;
class GLStateCacheManager;
class HardwareBufferManager;
class HardwareOcclusionQuery;
class MultiRenderTarget;
class PixelBox;
class RenderOperation;
class RenderSystemCapabilities;
class VertexElement;
class Viewport;
struct GLGpuProgramBase;
    /** \addtogroup RenderSystems RenderSystems
    *  @{
    */
    /** \defgroup GL GL
    * Implementation of GL as a rendering system.
    *  @{
    */

    namespace GLSL {
        class GLSLProgramFactory;
    }

    /**
      Implementation of GL as a rendering system.
     */
    class GLRenderSystem : public GLRenderSystemCommon
    {
    private:
        /// View matrix to set world against
        Matrix4 mViewMatrix;
        Matrix4 mWorldMatrix;
        Matrix4 mTextureMatrix;

        /// Last min & mip filtering options, so we can combine them
        FilterOptions mMinFilter;
        FilterOptions mMipFilter;

        /// What texture coord set each texture unit is using
        size_t mTextureCoordIndex[OGRE_MAX_TEXTURE_LAYERS];

        /// Holds texture type settings for every stage
        GLenum mTextureTypes[OGRE_MAX_TEXTURE_LAYERS];

        /// Number of fixed-function texture units
        unsigned short mFixedFunctionTextureUnits{0};

        void setGLLight(size_t index, bool lt);
        void makeGLMatrix(GLfloat gl_matrix[16], const Matrix4& m);
 
        [[nodiscard]]
        auto getBlendMode(SceneBlendFactor ogreBlend) const -> GLint;
        [[nodiscard]]
        auto getTextureAddressingMode(TextureAddressingMode tam) const -> GLint;
                void initialiseContext(RenderWindow* primary);

        /// Store last stencil mask state
        uint32 mStencilWriteMask{0xFFFFFFFF};
        /// Store last depth write state
        bool mDepthWrite{true};

        [[nodiscard]]
        auto convertCompareFunction(CompareFunction func) const -> GLint;
        [[nodiscard]]
        auto convertStencilOp(StencilOperation op, bool invert = false) const -> GLint;

        bool mUseAutoTextureMatrix{false};
        GLfloat mAutoTextureMatrix[16];

        /// Check if the GL system has already been initialised
        bool mGLInitialised;

        HardwareBufferManager* mHardwareBufferManager{nullptr};
        GLGpuProgramManager* mGpuProgramManager{nullptr};
        GLSL::GLSLProgramFactory* mGLSLProgramFactory{nullptr};

        unsigned short mCurrentLights;

        GLGpuProgramBase* mCurrentVertexProgram;
        GLGpuProgramBase* mCurrentFragmentProgram;
        GLGpuProgramBase* mCurrentGeometryProgram;

        // statecaches are per context
        GLStateCacheManager* mStateCacheManager{nullptr};

        ushort mActiveTextureUnit{0};
        ushort mMaxBuiltInTextureAttribIndex{0};

        // local data members of _render that were moved here to improve performance
        // (save allocations)
        std::vector<GLuint> mRenderAttribsBound;
        std::vector<GLuint> mRenderInstanceAttribsBound;

        /// is fixed pipeline enabled
        bool mEnableFixedPipeline;

    protected:
        void setClipPlanesImpl(const PlaneList& clipPlanes) override;
        void bindVertexElementToGpu(const VertexElement& elem,
                                    const HardwareVertexBufferSharedPtr& vertexBuffer,
                                    const size_t vertexStart) override;

        /** Initialises GL extensions, must be done AFTER the GL context has been
            established.
        */
        void initialiseExtensions();
    public:
        // Default constructor / destructor
        GLRenderSystem();
        ~GLRenderSystem() override;

        // ----------------------------------
        // Overridden RenderSystem functions
        // ----------------------------------

        auto getFixedFunctionParams(TrackVertexColourType tracking, FogMode fog) -> const GpuProgramParametersPtr& override;

        void applyFixedFunctionParams(const GpuProgramParametersPtr& params, uint16 variabilityMask) override;

        [[nodiscard]]
        auto getName() const -> const String& override;

        void _initialise() override;

        void initConfigOptions() override;

        [[nodiscard]]
        auto createRenderSystemCapabilities() const -> RenderSystemCapabilities* override;

        void initialiseFromRenderSystemCapabilities(RenderSystemCapabilities* caps, RenderTarget* primary) override;

        void shutdown() override;

        void setShadingType(ShadeOptions so) override;

        void setLightingEnabled(bool enabled) override;
        
        /// @copydoc RenderSystem::_createRenderWindow
        auto _createRenderWindow(const String &name, unsigned int width, unsigned int height, 
                                          bool fullScreen, const NameValuePairList *miscParams = nullptr) -> RenderWindow* override;

        /// @copydoc RenderSystem::_createDepthBufferFor
        auto _createDepthBufferFor( RenderTarget *renderTarget ) -> DepthBuffer* override;
        
        /// @copydoc RenderSystem::createMultiRenderTarget
        auto createMultiRenderTarget(const String & name) -> MultiRenderTarget * override; 
        

        void destroyRenderWindow(const String& name) override;

        void setNormaliseNormals(bool normalise) override;

        // -----------------------------
        // Low-level overridden members
        // -----------------------------

        void _useLights(unsigned short limit) override;

        void setWorldMatrix(const Matrix4 &m);

        void setViewMatrix(const Matrix4 &m);

        void setProjectionMatrix(const Matrix4 &m);

        void _setSurfaceTracking(TrackVertexColourType tracking);

        void _setPointParameters(bool attenuationEnabled, Real minSize, Real maxSize) override;

        void _setLineWidth(float width) override;

        void _setPointSpritesEnabled(bool enabled) override;

        void _setTexture(size_t unit, bool enabled, const TexturePtr &tex) override;

        void _setSampler(size_t unit, Sampler& sampler) override;

        void _setTextureCoordSet(size_t stage, size_t index) override;

        void _setTextureCoordCalculation(size_t stage, TexCoordCalcMethod m, 
            const Frustum* frustum = nullptr) override;

        void _setTextureBlendMode(size_t stage, const LayerBlendModeEx& bm) override;

        void _setTextureAddressingMode(size_t stage, const Sampler::UVWAddressingMode& uvw);

        void _setTextureMatrix(size_t stage, const Matrix4& xform) override;

        void _setAlphaRejectSettings(CompareFunction func, unsigned char value, bool alphaToCoverage) override;

        void _setViewport(Viewport *vp) override;

        void _endFrame() override;

        void _setCullingMode(CullingMode mode) override;

        void _setDepthBufferParams(bool depthTest = true, bool depthWrite = true, CompareFunction depthFunction = CMPF_LESS_EQUAL) override;

        void _setDepthBufferCheckEnabled(bool enabled = true);

        void _setDepthBufferWriteEnabled(bool enabled = true);

        void _setDepthBufferFunction(CompareFunction func = CMPF_LESS_EQUAL);

        void _setDepthBias(float constantBias, float slopeScaleBias) override;

        void setColourBlendState(const ColourBlendState& state) override;

        void _setFog(FogMode mode);

        void setClipPlane (ushort index, Real A, Real B, Real C, Real D);

        void enableClipPlane (ushort index, bool enable);

        void _setPolygonMode(PolygonMode level) override;

        void setStencilState(const StencilState& state) override;

        void _setTextureUnitFiltering(size_t unit, FilterType ftype, FilterOptions filter);

        void _render(const RenderOperation& op) override;

        void bindGpuProgram(GpuProgram* prg) override;

        void unbindGpuProgram(GpuProgramType gptype) override;

        void bindGpuProgramParameters(GpuProgramType gptype, 
                                      const GpuProgramParametersPtr& params, uint16 variabilityMask) override;

        void setScissorTest(bool enabled, const Rect& rect = Rect()) override ;
        void clearFrameBuffer(unsigned int buffers, 
                              const ColourValue& colour = ColourValue::Black, 
                              float depth = 1.0f, unsigned short stencil = 0) override;
        auto createHardwareOcclusionQuery() -> HardwareOcclusionQuery* override;

        // ----------------------------------
        // GLRenderSystem specific members
        // ----------------------------------
        void _oneTimeContextInitialization() override;
        /** Switch GL context, dealing with involved internal cached states too
        */
        void _switchContext(GLContext *context);
        /**
         * Set current render target to target, enabling its GL context if needed
         */
        void _setRenderTarget(RenderTarget *target) override;
        /** Unregister a render target->context mapping. If the context of target 
            is the current context, change the context to the main context so it
            can be destroyed safely. 
            
            @note This is automatically called by the destructor of 
            GLContext.
         */
        void _unregisterContext(GLContext *context) override;

        auto _getStateCacheManager() -> GLStateCacheManager * { return mStateCacheManager; }
        
        /// @copydoc RenderSystem::beginProfileEvent
        void beginProfileEvent( const String &eventName ) override;

        /// @copydoc RenderSystem::endProfileEvent
        void endProfileEvent( ) override;

        /// @copydoc RenderSystem::markProfileEvent
        void markProfileEvent( const String &eventName ) override;

        /** @copydoc RenderTarget::copyContentsToMemory */
        void _copyContentsToMemory(Viewport* vp, const Box& src, const PixelBox &dst, RenderWindow::FrameBuffer buffer) override;
    };
    /** @} */
    /** @} */
}
#endif

