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
module;

#include <cstddef>

export module Ogre.Core:RenderTarget;

export import :DepthBuffer;
export import :MemoryAllocatorConfig;
export import :PixelFormat;
export import :Platform;
export import :Prerequisites;

export import <algorithm>;
export import <map>;
export import <vector>;

export
auto constexpr inline OGRE_NUM_RENDERTARGET_GROUPS = 10;

export
auto constexpr inline OGRE_DEFAULT_RT_GROUP = 4;

export
auto constexpr inline OGRE_REND_TO_TEX_RT_GROUP = 2;

export
namespace Ogre {
class Camera;
class RenderTargetListener;
class Timer;
class Viewport;
struct Box;

    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup RenderSystem
    *  @{
    */
    /** A 'canvas' which can receive the results of a rendering
        operation.
        @remarks
            This abstract class defines a common root to all targets of rendering operations. A
            render target could be a window on a screen, or another
            offscreen surface like a texture or bump map etc.
        @author
            Steven Streeting
        @version
            1.0
     */
    class RenderTarget : public RenderSysAlloc
    {
    public:
        struct FrameStats
        {
            /// frames per second (FPS) based on the frames rendered in the last second
            float lastFPS;
            /// average frames per second (FPS) since call to Root::startRendering
            float avgFPS;
            /// best frames per second (FPS) since call to Root::startRendering
            float bestFPS;
            /// worst frames per second (FPS) since call to Root::startRendering
            float worstFPS;
            unsigned long bestFrameTime;
            unsigned long worstFrameTime;
            /// number of triangles rendered in the last update() call.
            size_t triangleCount;
            /// number of batches rendered in the last update() call.
            size_t batchCount;
            int vBlankMissCount; // -1 means that the value is not applicable
        };

        enum class FrameBuffer
        {
            FRONT,
            BACK,
            AUTO
        };

        RenderTarget();
        virtual ~RenderTarget();

        /// Retrieve target's name.
        [[nodiscard]] virtual auto getName() const noexcept -> std::string_view ;

        /// Retrieve information about the render target.
        void getMetrics(unsigned int& width, unsigned int& height);

        [[nodiscard]] virtual auto getWidth() const noexcept -> uint32;
        [[nodiscard]] virtual auto getHeight() const noexcept -> uint32;

        /**
         * Sets the pool ID this RenderTarget should query from. Default value is PoolId::DEFAULT.
         * Set to PoolId::NO_DEPTH to avoid using a DepthBuffer (or manually controlling it) @see DepthBuffer
         *  @remarks
         *      Changing the pool Id will cause the current depth buffer to be detached unless the old
         *      id and the new one are the same
         */
        void setDepthBufferPool( DepthBuffer::PoolId poolId );

        //Returns the pool ID this RenderTarget should query from. @see DepthBuffer
        [[nodiscard]] auto getDepthBufferPool() const noexcept -> DepthBuffer::PoolId;

        [[nodiscard]] auto getDepthBuffer() const noexcept -> DepthBuffer*;

        //Returns false if couldn't attach
        virtual auto attachDepthBuffer( DepthBuffer *depthBuffer ) -> bool;

        virtual void detachDepthBuffer();

        /** Detaches DepthBuffer without notifying it from the detach.
            Useful when called from the DepthBuffer while it iterates through attached
            RenderTargets (@see DepthBuffer::_setPoolId())
        */
        virtual void _detachDepthBuffer();

        /** Tells the target to update it's contents.
            @remarks
                If OGRE is not running in an automatic rendering loop
                (started using Root::startRendering),
                the user of the library is responsible for asking each render
                target to refresh. This is the method used to do this. It automatically
                re-renders the contents of the target using whatever cameras have been
                pointed at it (using Camera::setRenderTarget).
            @par
                This allows OGRE to be used in multi-windowed utilities
                and for contents to be refreshed only when required, rather than
                constantly as with the automatic rendering loop.
            @param swapBuffers For targets that support double-buffering, if set 
                to true, the target will immediately
                swap it's buffers after update. Otherwise, the buffers are
                not swapped, and you have to call swapBuffers yourself sometime
                later. You might want to do this on some rendersystems which 
                pause for queued rendering commands to complete before accepting
                swap buffers calls - so you could do other CPU tasks whilst the 
                queued commands complete. Or, you might do this if you want custom
                control over your windows, such as for externally created windows.
        */
        virtual void update(bool swapBuffers = true);
        /** Swaps the frame buffers to display the next frame.
            @remarks
                For targets that are double-buffered so that no
                'in-progress' versions of the scene are displayed
                during rendering. Once rendering has completed (to
                an off-screen version of the window) the buffers
                are swapped to display the new frame.
        */
        virtual void swapBuffers() {}

        /** Adds a viewport to the rendering target.
            @remarks
                A viewport is the rectangle into which rendering output is sent. This method adds
                a viewport to the render target, rendering from the supplied camera. The
                rest of the parameters are only required if you wish to add more than one viewport
                to a single rendering target. Note that size information passed to this method is
                passed as a parametric, i.e. it is relative rather than absolute. This is to allow
                viewports to automatically resize along with the target.
            @param
                cam The camera from which the viewport contents will be rendered (mandatory)
            @param
                ZOrder The relative order of the viewport with others on the target (allows overlapping
                viewports i.e. picture-in-picture). Higher Z-orders are on top of lower ones. The actual number
                is irrelevant, only the relative Z-order matters (you can leave gaps in the numbering)
            @param
                left The relative position of the left of the viewport on the target, as a value between 0 and 1.
            @param
                top The relative position of the top of the viewport on the target, as a value between 0 and 1.
            @param
                width The relative width of the viewport on the target, as a value between 0 and 1.
            @param
                height The relative height of the viewport on the target, as a value between 0 and 1.
        */
        virtual auto addViewport(Camera* cam, int ZOrder = 0, float left = 0.0f, float top = 0.0f ,
            float width = 1.0f, float height = 1.0f) -> Viewport*;

        /** Returns the number of viewports attached to this target.*/
        [[nodiscard]] virtual auto getNumViewports() const noexcept -> unsigned short;

        /** Retrieves a pointer to the viewport with the given index. */
        virtual auto getViewport(unsigned short index) -> Viewport*;

        /** Retrieves a pointer to the viewport with the given Z-order. 
            @remarks throws if not found.
        */
        virtual auto getViewportByZOrder(int ZOrder) -> Viewport*;

        /** Returns true if and only if a viewport exists at the given Z-order. */
        virtual auto hasViewportWithZOrder(int ZOrder) -> bool;

        /** Removes a viewport at a given Z-order.
        */
        virtual void removeViewport(int ZOrder);

        /** Removes all viewports on this target.
        */
        virtual void removeAllViewports();

        /** Retieves details of current rendering performance. */
        [[nodiscard]] auto getStatistics() const noexcept -> const FrameStats& {
            return mStats;
        }

        /** Resets saved frame-rate statistices.
        */
        void resetStatistics();

        /** Retrieve a platform or API-specific piece of information

            This method of retrieving information should only be used if you know what you're doing.

            | Name        | Description                        |
            |-------------|------------------------------------|
            | WINDOW      | The native window handle. (X11 Window XID/ HWND / NSWindow*) |
            | HWND        | deprecated (same as WINDOW) |
            | GL_FBOID | the id of the OpenGL framebuffer object |
            | GL_MULTISAMPLEFBOID | the id of the OpenGL framebuffer object used for multisampling |
            | GLFBO | id of the screen OpenGL framebuffer object on iOS |
            | GLCONTEXT   | deprecated, do not use |
            | FBO | deprecated, do not use |
            | TARGET | deprecated, do not use |
            | XDISPLAY     | The X Display connection behind that context. |
            | DISPLAYNAME | The X Server name for the connected display. |
            | ATOM        | The X Atom used in client delete events. |
            | VIEW | Cocoa NSView* |
            | NSOPENGLCONTEXT | Cocoa NSOpenGLContext* |
            | NSOPENGLPIXELFORMAT | Cocoa NSOpenGLPixelFormat* |
            
            @param name The name of the attribute.
            @param pData Pointer to memory of the right kind of structure to receive the info.
        */
        virtual void getCustomAttribute(std::string_view name, void* pData);

        /** simplified API for bindings
         * 
         * @overload
         */
        auto getCustomAttribute(std::string_view name) -> uint
        {
            uint ret = 0;
            getCustomAttribute(name, &ret);
            return ret;
        }

        /** Add a listener to this RenderTarget which will be called back before & after rendering.
        @remarks
            If you want notifications before and after a target is updated by the system, use
            this method to register your own custom RenderTargetListener class. This is useful
            for potentially adding your own manual rendering commands before and after the
            'normal' system rendering.
        @par NB this should not be used for frame-based scene updates, use Root::addFrameListener for that.
        */
        virtual void addListener(RenderTargetListener* listener);
        /** same as addListener, but force the position in the vector, so we can control the call order */
        virtual void insertListener(RenderTargetListener* listener, const unsigned int pos = 0);
        /** Removes a RenderTargetListener previously registered using addListener. */
        virtual void removeListener(RenderTargetListener* listener);
        /** Removes all listeners from this instance. */
        virtual void removeAllListeners();

        /** Sets the priority of this render target in relation to the others. 
        @remarks
            This can be used in order to schedule render target updates. Lower
            priorities will be rendered first. Note that the priority must be set
            at the time the render target is attached to the render system, changes
            afterwards will not affect the ordering.
        */
        virtual void setPriority( uchar priority ) { mPriority = priority; }
        /** Gets the priority of a render target. */
        [[nodiscard]] virtual auto getPriority() const noexcept -> uchar { return mPriority; }

        /** Used to retrieve or set the active state of the render target.
        */
        [[nodiscard]] virtual auto isActive() const noexcept -> bool;

        /** Used to set the active state of the render target.
        */
        virtual void setActive( bool state );

        /** Sets whether this target should be automatically updated if Ogre's rendering
            loop or Root::_updateAllRenderTargets is being used.
        @remarks
            By default, if you use Ogre's own rendering loop (Root::startRendering)
            or call Root::_updateAllRenderTargets, all render targets are updated 
            automatically. This method allows you to control that behaviour, if 
            for example you have a render target which you only want to update periodically.
        @param autoupdate If true, the render target is updated during the automatic render
            loop or when Root::_updateAllRenderTargets is called. If false, the 
            target is only updated when its update() method is called explicitly.
        */
        virtual void setAutoUpdated(bool autoupdate);
        /** Gets whether this target is automatically updated if Ogre's rendering
            loop or Root::_updateAllRenderTargets is being used.
        */
        [[nodiscard]] virtual auto isAutoUpdated() const noexcept -> bool;

        /** Copies the current contents of the render target to a pixelbox. 
        @remarks See suggestPixelFormat for a tip as to the best pixel format to
            extract into, although you can use whatever format you like and the 
            results will be converted.
        */
        virtual void copyContentsToMemory(const Box& src, const PixelBox &dst, FrameBuffer buffer = FrameBuffer::AUTO) = 0;

        /** Suggests a pixel format to use for extracting the data in this target,
            when calling copyContentsToMemory.
        */
        [[nodiscard]] virtual auto suggestPixelFormat() const noexcept -> PixelFormat { return PixelFormat::BYTE_RGBA; }
        
        /** Writes the current contents of the render target to the named file. */
        void writeContentsToFile(std::string_view filename);

        /** Writes the current contents of the render target to the (PREFIX)(time-stamp)(SUFFIX) file.
            @return the name of the file used.*/
        virtual auto writeContentsToTimestampedFile(std::string_view filenamePrefix, std::string_view filenameSuffix) -> String;

        [[nodiscard]] virtual auto requiresTextureFlipping() const -> bool = 0;

        /** Utility method to notify a render target that a camera has been removed,
        incase it was referring to it as a viewer.
        */
        virtual void _notifyCameraRemoved(const Camera* cam);

        /** Indicates whether this target is the primary window. The
            primary window is special in that it is destroyed when
            ogre is shut down, and cannot be destroyed directly.
            This is the case because it holds the context for vertex,
            index buffers and textures.
        */
        [[nodiscard]] virtual auto isPrimary() const noexcept -> bool;

		/** Indicates whether stereo is currently enabled for this target. Default is false. */
		[[nodiscard]] virtual auto isStereoEnabled() const noexcept -> bool;
		
        /** Indicates whether on rendering, linear colour space is converted to 
            sRGB gamma colour space. This is the exact opposite conversion of
            what is indicated by Texture::isHardwareGammaEnabled, and can only
            be enabled on creation of the render target. For render windows, it's
            enabled through the 'gamma' creation misc parameter. For textures, 
            it is enabled through the hwGamma parameter to the create call.
        */
        [[nodiscard]] virtual auto isHardwareGammaEnabled() const noexcept -> bool { return mHwGamma; }

        /** Indicates whether multisampling is performed on rendering and at what level.
        */
        [[nodiscard]] virtual auto getFSAA() const noexcept -> uint { return mFSAA; }

        /// RenderSystem specific FSAA option. See @ref RenderSystem::_createRenderWindow for details.
        [[nodiscard]] virtual auto getFSAAHint() const noexcept -> std::string_view { return mFSAAHint; }

        /** Set the level of multisample AA to be used if hardware support it.
            This option will be ignored if the hardware does not support it 
            or setting can not be changed on the fly on per-target level. 
            @param fsaa The number of samples
            @param fsaaHint @copybrief getFSAAHint
        */
        virtual void setFSAA(uint fsaa, std::string_view fsaaHint) { }

        /** Method for manual management of rendering : fires 'preRenderTargetUpdate'
            and initialises statistics etc.
        @remarks 
        <ul>
        <li>_beginUpdate resets statistics and fires 'preRenderTargetUpdate'.</li>
        <li>_updateViewport renders the given viewport (even if it is not autoupdated),
        fires preViewportUpdate and postViewportUpdate and manages statistics.</li>
        <li>_updateAutoUpdatedViewports renders only viewports that are auto updated,
        fires preViewportUpdate and postViewportUpdate and manages statistics.</li>
        <li>_endUpdate() ends statistics calculation and fires postRenderTargetUpdate.</li>
        </ul>
        you can use it like this for example :
        <pre>
            renderTarget->_beginUpdate();
            renderTarget->_updateViewport(1); // which is not auto updated
            renderTarget->_updateViewport(2); // which is not auto updated
            renderTarget->_updateAutoUpdatedViewports();
            renderTarget->_endUpdate();
            renderTarget->swapBuffers();
        </pre>
            Please note that in that case, the zorder may not work as you expect,
            since you are responsible for calling _updateViewport in the correct order.
        */
        virtual void _beginUpdate();

        /** Method for manual management of rendering - renders the given 
        viewport (even if it is not autoupdated)
        @remarks
        This also fires preViewportUpdate and postViewportUpdate, and manages statistics.
        You should call it between _beginUpdate() and _endUpdate().
        @see _beginUpdate for more details.
        @param zorder The zorder of the viewport to update.
        @param updateStatistics Whether you want to update statistics or not.
        */
        virtual void _updateViewport(int zorder, bool updateStatistics = true);

        /** Method for manual management of rendering - renders the given viewport (even if it is not autoupdated)
        @remarks
        This also fires preViewportUpdate and postViewportUpdate, and manages statistics
        if needed. You should call it between _beginUpdate() and _endUpdate().
        @see _beginUpdate for more details.
        @param viewport The viewport you want to update, it must be bound to the rendertarget.
        @param updateStatistics Whether you want to update statistics or not.
        */
        virtual void _updateViewport(Viewport* viewport, bool updateStatistics = true);

        /** Method for manual management of rendering - renders only viewports that are auto updated
        @remarks
        This also fires preViewportUpdate and postViewportUpdate, and manages statistics.
        You should call it between _beginUpdate() and _endUpdate().
        See _beginUpdate for more details.
        @param updateStatistics Whether you want to update statistics or not.
        @see _beginUpdate()
        */
        virtual void _updateAutoUpdatedViewports(bool updateStatistics = true);
        
        /** Method for manual management of rendering - finishes statistics calculation 
            and fires 'postRenderTargetUpdate'.
        @remarks
        You should call it after a _beginUpdate
        @see _beginUpdate for more details.
        */
        virtual void _endUpdate();

    protected:
        /// The name of this target.
        String mName;
        /// The priority of the render target.
        uchar mPriority{OGRE_DEFAULT_RT_GROUP};

        uint32 mWidth;
        uint32 mHeight;
        DepthBuffer::PoolId mDepthBufferPoolId{DepthBuffer::PoolId::DEFAULT};
        DepthBuffer *mDepthBuffer{nullptr};

        // Stats
        FrameStats mStats;
        
        Timer* mTimer ;
        unsigned long mLastSecond;
        unsigned long mLastTime;
        size_t mFrameCount;

        bool mActive{true};
        bool mAutoUpdate{true};
        // Hardware sRGB gamma conversion done on write?
        bool mHwGamma{false};
        // FSAA performed?
        uint mFSAA{0};
        String mFSAAHint;
		bool mStereoEnabled{false};

        virtual void updateStats();

        using ViewportList = std::map<int, ::std::unique_ptr<Viewport>>;
        /// List of viewports, map on Z-order
        ViewportList mViewportList;

        using RenderTargetListenerList = std::vector<RenderTargetListener *>;
        RenderTargetListenerList mListeners;
    

        /// internal method for firing events
        virtual void firePreUpdate();
        /// internal method for firing events
        virtual void firePostUpdate();
        /// internal method for firing events
        virtual void fireViewportPreUpdate(Viewport* vp);
        /// internal method for firing events
        virtual void fireViewportPostUpdate(Viewport* vp);
        /// internal method for firing events
        virtual void fireViewportAdded(Viewport* vp);
        /// internal method for firing events
        virtual void fireViewportRemoved(Viewport* vp);
        
        /// Internal implementation of update()
        virtual void updateImpl();
    };
    /** @} */
    /** @} */

} // Namespace
