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
        unsigned short mFixedFunctionTextureUnits;

        void setGLLight(size_t index, bool lt);
        void makeGLMatrix(GLfloat gl_matrix[16], const Matrix4& m);
 
        [[nodiscard]]
        auto getBlendMode(SceneBlendFactor ogreBlend) const -> GLint;
        [[nodiscard]]
        auto getTextureAddressingMode(TextureAddressingMode tam) const -> GLint;
                void initialiseContext(RenderWindow* primary);

        /// Store last stencil mask state
        uint32 mStencilWriteMask;
        /// Store last depth write state
        bool mDepthWrite;

        [[nodiscard]]
        auto convertCompareFunction(CompareFunction func) const -> GLint;
        [[nodiscard]]
        auto convertStencilOp(StencilOperation op, bool invert = false) const -> GLint;

        bool mUseAutoTextureMatrix;
        GLfloat mAutoTextureMatrix[16];

        /// Check if the GL system has already been initialised
        bool mGLInitialised;

        HardwareBufferManager* mHardwareBufferManager;
        GLGpuProgramManager* mGpuProgramManager;
        GLSL::GLSLProgramFactory* mGLSLProgramFactory;

        unsigned short mCurrentLights;

        GLGpuProgramBase* mCurrentVertexProgram;
        GLGpuProgramBase* mCurrentFragmentProgram;
        GLGpuProgramBase* mCurrentGeometryProgram;

        // statecaches are per context
        GLStateCacheManager* mStateCacheManager;

        ushort mActiveTextureUnit;
        ushort mMaxBuiltInTextureAttribIndex;

        // local data members of _render that were moved here to improve performance
        // (save allocations)
        std::vector<GLuint> mRenderAttribsBound;
        std::vector<GLuint> mRenderInstanceAttribsBound;

        /// is fixed pipeline enabled
        bool mEnableFixedPipeline;

    protected:
        void setClipPlanesImpl(const PlaneList& clipPlanes);
        void bindVertexElementToGpu(const VertexElement& elem,
                                    const HardwareVertexBufferSharedPtr& vertexBuffer,
                                    const size_t vertexStart);

        /** Initialises GL extensions, must be done AFTER the GL context has been
            established.
        */
        void initialiseExtensions();
    public:
        // Default constructor / destructor
        GLRenderSystem();
        ~GLRenderSystem();

        // ----------------------------------
        // Overridden RenderSystem functions
        // ----------------------------------

        auto getFixedFunctionParams(TrackVertexColourType tracking, FogMode fog) -> const GpuProgramParametersPtr&;

        void applyFixedFunctionParams(const GpuProgramParametersPtr& params, uint16 variabilityMask);

        [[nodiscard]]
        auto getName() const -> const String&;

        void _initialise() override;

        void initConfigOptions() override;

        [[nodiscard]]
        virtual auto createRenderSystemCapabilities() const -> RenderSystemCapabilities*;

        void initialiseFromRenderSystemCapabilities(RenderSystemCapabilities* caps, RenderTarget* primary);

        void shutdown();

        void setShadingType(ShadeOptions so);

        void setLightingEnabled(bool enabled);
        
        /// @copydoc RenderSystem::_createRenderWindow
        auto _createRenderWindow(const String &name, unsigned int width, unsigned int height, 
                                          bool fullScreen, const NameValuePairList *miscParams = 0) -> RenderWindow*;

        /// @copydoc RenderSystem::_createDepthBufferFor
        auto _createDepthBufferFor( RenderTarget *renderTarget ) -> DepthBuffer*;
        
        /// @copydoc RenderSystem::createMultiRenderTarget
        virtual auto createMultiRenderTarget(const String & name) -> MultiRenderTarget *; 
        

        void destroyRenderWindow(const String& name);

        void setNormaliseNormals(bool normalise);

        // -----------------------------
        // Low-level overridden members
        // -----------------------------

        void _useLights(unsigned short limit);

        void setWorldMatrix(const Matrix4 &m);

        void setViewMatrix(const Matrix4 &m);

        void setProjectionMatrix(const Matrix4 &m);

        void _setSurfaceTracking(TrackVertexColourType tracking);

        void _setPointParameters(bool attenuationEnabled, Real minSize, Real maxSize);

        void _setLineWidth(float width);

        void _setPointSpritesEnabled(bool enabled);

        void _setTexture(size_t unit, bool enabled, const TexturePtr &tex);

        void _setSampler(size_t unit, Sampler& sampler);

        void _setTextureCoordSet(size_t stage, size_t index);

        void _setTextureCoordCalculation(size_t stage, TexCoordCalcMethod m, 
            const Frustum* frustum = 0);

        void _setTextureBlendMode(size_t stage, const LayerBlendModeEx& bm);

        void _setTextureAddressingMode(size_t stage, const Sampler::UVWAddressingMode& uvw);

        void _setTextureMatrix(size_t stage, const Matrix4& xform);

        void _setAlphaRejectSettings(CompareFunction func, unsigned char value, bool alphaToCoverage);

        void _setViewport(Viewport *vp);

        void _endFrame();

        void _setCullingMode(CullingMode mode);

        void _setDepthBufferParams(bool depthTest = true, bool depthWrite = true, CompareFunction depthFunction = CMPF_LESS_EQUAL);

        void _setDepthBufferCheckEnabled(bool enabled = true);

        void _setDepthBufferWriteEnabled(bool enabled = true);

        void _setDepthBufferFunction(CompareFunction func = CMPF_LESS_EQUAL);

        void _setDepthBias(float constantBias, float slopeScaleBias);

        void setColourBlendState(const ColourBlendState& state);

        void _setFog(FogMode mode);

        void setClipPlane (ushort index, Real A, Real B, Real C, Real D);

        void enableClipPlane (ushort index, bool enable);

        void _setPolygonMode(PolygonMode level);

        void setStencilState(const StencilState& state) override;

        void _setTextureUnitFiltering(size_t unit, FilterType ftype, FilterOptions filter);

        void _render(const RenderOperation& op);

        void bindGpuProgram(GpuProgram* prg);

        void unbindGpuProgram(GpuProgramType gptype);

        void bindGpuProgramParameters(GpuProgramType gptype, 
                                      const GpuProgramParametersPtr& params, uint16 variabilityMask);

        void setScissorTest(bool enabled, const Rect& rect = Rect()) ;
        void clearFrameBuffer(unsigned int buffers, 
                              const ColourValue& colour = ColourValue::Black, 
                              float depth = 1.0f, unsigned short stencil = 0);
        auto createHardwareOcclusionQuery() -> HardwareOcclusionQuery*;

        // ----------------------------------
        // GLRenderSystem specific members
        // ----------------------------------
        void _oneTimeContextInitialization();
        /** Switch GL context, dealing with involved internal cached states too
        */
        void _switchContext(GLContext *context);
        /**
         * Set current render target to target, enabling its GL context if needed
         */
        void _setRenderTarget(RenderTarget *target);
        /** Unregister a render target->context mapping. If the context of target 
            is the current context, change the context to the main context so it
            can be destroyed safely. 
            
            @note This is automatically called by the destructor of 
            GLContext.
         */
        void _unregisterContext(GLContext *context);

        auto _getStateCacheManager() -> GLStateCacheManager * { return mStateCacheManager; }
        
        /// @copydoc RenderSystem::beginProfileEvent
        virtual void beginProfileEvent( const String &eventName );

        /// @copydoc RenderSystem::endProfileEvent
        virtual void endProfileEvent( );

        /// @copydoc RenderSystem::markProfileEvent
        virtual void markProfileEvent( const String &eventName );

        /** @copydoc RenderTarget::copyContentsToMemory */
        void _copyContentsToMemory(Viewport* vp, const Box& src, const PixelBox &dst, RenderWindow::FrameBuffer buffer);
    };
    /** @} */
    /** @} */
}
#endif

