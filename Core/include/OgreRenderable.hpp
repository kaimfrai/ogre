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
module;

#include <cstddef>

export module Ogre.Core:Renderable;

export import :Common;
export import :GpuProgramParams;
export import :Material;
export import :Prerequisites;
export import :SharedPtr;
export import :UserObjectBindings;
export import :Vector;

export import <any>;
export import <map>;

export
namespace Ogre {
    class Camera;
    struct Matrix4;
    class RenderOperation;
    class RenderSystem;
    class SceneManager;
    class Technique;

    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup Scene
    *  @{
    */
    /** Abstract class defining the interface all renderable objects must implement.
    @remarks
        This interface abstracts renderable discrete objects which will be queued in the render pipeline,
        grouped by material. Classes implementing this interface must be based on a single material, a single
        world matrix (or a collection of world matrices which are blended by weights), and must be 
        renderable via a single render operation.
    @par
        Note that deciding whether to put these objects in the rendering pipeline is done from the more specific
        classes e.g. entities. Only once it is decided that the specific class is to be rendered is the abstract version
        created (could be more than one per visible object) and pushed onto the rendering queue.
    */
    class Renderable
    {
    public:
        enum
        {
            DEFAULT_PRIORITY = 100
        };

        Renderable()  = default;
        /** Virtual destructor needed as class has virtual methods. */
        virtual ~Renderable() = default;
        /** Retrieves a weak reference to the material this renderable object uses.
        @remarks
            Note that the Renderable also has the option to override the getTechnique method
            to specify a particular Technique to use instead of the best one available.
        */
        [[nodiscard]] virtual auto getMaterial() const noexcept -> const MaterialPtr& = 0;
        /** Retrieves a pointer to the Material Technique this renderable object uses.
        @remarks
            This is to allow Renderables to use a chosen Technique if they wish, otherwise
            they will use the best Technique available for the Material they are using.
        */
        [[nodiscard]] virtual auto getTechnique() const noexcept -> Technique* { return getMaterial()->getBestTechnique(0, this); }
        /** Gets the render operation required to send this object to the frame buffer.
        */
        virtual void getRenderOperation(RenderOperation& op) = 0;

        /** Called just prior to the Renderable being rendered. 
        @remarks
            OGRE is a queued renderer, so the actual render commands are executed 
            at a later time than the point at which an object is discovered to be
            visible. This allows ordering & grouping of renders without the discovery
            process having to be aware of it. It also means OGRE uses declarative
            render information rather than immediate mode rendering - this is very useful
            in that certain effects and processes can automatically be applied to 
            a wide range of scenes, but the downside is that special cases are
            more difficult to handle, because there is not the declared state to 
            cope with it. 
        @par
            This method allows a Renderable to do something special at the actual
            point of rendering if it wishes to. When this method is called, all the
            material render state as declared by this Renderable has already been set, 
            all that is left to do is to bind the buffers and perform the render. 
            The Renderable may modify render state itself if it wants to (and restore it in the 
            postRender call) before the automated render happens, or by returning
            'false' from this method can actually suppress the automatic render
            and perform one of its own.
        @return
            true if the automatic render should proceed, false to skip it on 
            the assumption that the Renderable has done it manually.
        */
        virtual auto preRender(SceneManager* sm, RenderSystem* rsys) -> bool
                { (void)sm; (void)rsys; return true; }

        /** Called immediately after the Renderable has been rendered. 
        */
        virtual void postRender(SceneManager* sm, RenderSystem* rsys)
                { (void)sm; (void)rsys; }

        /** Gets the world transform matrix / matrices for this renderable object.
        @remarks
            If the object has any derived transforms, these are expected to be up to date as long as
            all the SceneNode structures have been updated before this is called.
        @par
            This method will populate transform with 1 matrix if it does not use vertex blending. If it
            does use vertex blending it will fill the passed in pointer with an array of matrices,
            the length being the value returned from getNumWorldTransforms.
        @note
            Internal Ogre never supports non-affine matrix for world transform matrix/matrices,
            the behavior is undefined if returns non-affine matrix here.
        */
        virtual void getWorldTransforms(Matrix4* xform) const = 0;

        /** Returns the number of world transform matrices this renderable requires.
        @remarks
            When a renderable uses vertex blending, it uses multiple world matrices instead of a single
            one. Each vertex sent to the pipeline can reference one or more matrices in this list
            with given weights.
            If a renderable does not use vertex blending this method returns 1, which is the default for 
            simplicity.
        */
        [[nodiscard]] virtual auto getNumWorldTransforms() const noexcept -> unsigned short { return 1; }

        /** Sets whether or not to use an 'identity' projection.
        @remarks
            Usually Renderable objects will use a projection matrix as determined
            by the active camera. However, if they want they can cancel this out
            and use an identity projection, which effectively projects in 2D using
            a {-1, 1} view space. Useful for overlay rendering. Normal renderables
            need not change this. The default is false.
        @see Renderable::getUseIdentityProjection
        */
        void setUseIdentityProjection(bool useIdentityProjection)
        {
            mUseIdentityProjection = useIdentityProjection;
        }

        /** Returns whether or not to use an 'identity' projection.
        @remarks
            Usually Renderable objects will use a projection matrix as determined
            by the active camera. However, if they want they can cancel this out
            and use an identity projection, which effectively projects in 2D using
            a {-1, 1} view space. Useful for overlay rendering. Normal renderables
            need not change this.
        @see Renderable::setUseIdentityProjection
        */
        [[nodiscard]] auto getUseIdentityProjection() const noexcept -> bool { return mUseIdentityProjection; }

        /** Sets whether or not to use an 'identity' view.
        @remarks
            Usually Renderable objects will use a view matrix as determined
            by the active camera. However, if they want they can cancel this out
            and use an identity matrix, which means all geometry is assumed
            to be relative to camera space already. Useful for overlay rendering. 
            Normal renderables need not change this. The default is false.
        @see Renderable::getUseIdentityView
        */
        void setUseIdentityView(bool useIdentityView)
        {
            mUseIdentityView = useIdentityView;
        }

        /** Returns whether or not to use an 'identity' view.
        @remarks
            Usually Renderable objects will use a view matrix as determined
            by the active camera. However, if they want they can cancel this out
            and use an identity matrix, which means all geometry is assumed
            to be relative to camera space already. Useful for overlay rendering. 
            Normal renderables need not change this.
        @see Renderable::setUseIdentityView
        */
        [[nodiscard]] auto getUseIdentityView() const noexcept -> bool { return mUseIdentityView; }

        /** Returns the squared distance between the camera and this renderable.

            Used to sort transparent objects. Squared distance is used
            to avoid having to perform a square root on the result.
        */
        virtual auto getSquaredViewDepth(const Camera* cam) const -> Real = 0;

        /** Gets a list of lights, ordered relative to how close they are to this renderable.
        @remarks
            Directional lights, which have no position, will always be first on this list.
        */
        [[nodiscard]] virtual auto getLights() const noexcept -> const LightList& = 0;

        /** Method which reports whether this renderable would normally cast a
            shadow. 
        @remarks
            Subclasses should override this if they could have been used to 
            generate a shadow.
        */
        [[nodiscard]] virtual auto getCastsShadows() const noexcept -> bool { return false; }

        /** Sets a custom parameter for this Renderable, which may be used to 
            drive calculations for this specific Renderable, like GPU program parameters.
        @remarks
            Calling this method simply associates a numeric index with a 4-dimensional
            value for this specific Renderable. This is most useful if the material
            which this Renderable uses a vertex or fragment program, and has an 
            AutoConstantType::CUSTOM parameter entry. This parameter entry can refer to the
            index you specify as part of this call, thereby mapping a custom
            parameter for this renderable to a program parameter.
        @param index The index with which to associate the value. Note that this
            does not have to start at 0, and can include gaps. It also has no direct
            correlation with a GPU program parameter index - the mapping between the
            two is performed by the AutoConstantType::CUSTOM entry, if that is used.
        @param value The value to associate.
        */
        void setCustomParameter(size_t index, const Vector4& value);

        /** Removes a custom value which is associated with this Renderable at the given index.
        @param index Index of the parameter to remove.
            @see setCustomParameter for full details.
        */
        void removeCustomParameter(size_t index);

        /** Checks whether a custom value is associated with this Renderable at the given index.
        @param index Index of the parameter to check for existence.
            @see setCustomParameter for full details.
        */
        [[nodiscard]] auto hasCustomParameter(size_t index) const -> bool;

        /** Gets the custom value associated with this Renderable at the given index.
        @param index Index of the parameter to retrieve.
            @see setCustomParameter for full details.
        */
        [[nodiscard]] auto getCustomParameter(size_t index) const -> const Vector4&;

        /** Update a custom GpuProgramParameters constant which is derived from 
            information only this Renderable knows.
        @remarks
            This method allows a Renderable to map in a custom GPU program parameter
            based on it's own data. This is represented by a GPU auto parameter
            of AutoConstantType::CUSTOM, and to allow there to be more than one of these per
            Renderable, the 'data' field on the auto parameter will identify
            which parameter is being updated. The implementation of this method
            must identify the parameter being updated, and call a 'setConstant' 
            method on the passed in GpuProgramParameters object, using the details
            provided in the incoming auto constant setting to identify the index
            at which to set the parameter.
        @par
            You do not need to override this method if you're using the standard
            sets of data associated with the Renderable as provided by setCustomParameter
            and getCustomParameter. By default, the implementation will map from the
            value indexed by the 'constantEntry.data' parameter to a value previously
            set by setCustomParameter. But custom Renderables are free to override
            this if they want, in any case.
        @param constantEntry The auto constant entry referring to the parameter
            being updated
        @param params The parameters object which this method should call to 
            set the updated parameters.
        */
        virtual void _updateCustomGpuParameter(const GpuProgramParameters::AutoConstantEntry& constantEntry,
                                               GpuProgramParameters* params) const;

        /** Sets whether this renderable's chosen detail level can be
            overridden (downgraded) by the camera setting. 
        @param override true means that a lower camera detail will override this
            renderables detail level, false means it won't.
        */
        void setPolygonModeOverrideable(bool override)
        {
            mPolygonModeOverrideable = override;
        }

        /** Gets whether this renderable's chosen detail level can be
            overridden (downgraded) by the camera setting. 
        */
        [[nodiscard]] auto getPolygonModeOverrideable() const noexcept -> bool
        {
            return mPolygonModeOverrideable;
        }

        /** Return an instance of user objects binding associated with this class.
            You can use it to associate one or more custom objects with this class instance.
        @see UserObjectBindings::setUserAny.
        */
        auto getUserObjectBindings() noexcept -> UserObjectBindings& { return mUserObjectBindings; }

        /** Return an instance of user objects binding associated with this class.
            You can use it to associate one or more custom objects with this class instance.
        @see UserObjectBindings::setUserAny.
        */
        [[nodiscard]] auto getUserObjectBindings() const noexcept -> const UserObjectBindings& { return mUserObjectBindings; }


        /** Visitor object that can be used to iterate over a collection of Renderable
            instances abstractly.
        @remarks
            Different scene objects use Renderable differently; some will have a 
            single Renderable, others will have many. This visitor interface allows
            classes using Renderable to expose a clean way for external code to
            get access to the contained Renderable instance(s) that it will
            eventually add to the render queue.
        @par
            To actually have this method called, you have to call a method on the
            class containing the Renderable instances. One example is 
            MovableObject::visitRenderables.
        */
        class Visitor
        {
        public:
            /** Virtual destructor needed as class has virtual methods. */
            virtual ~Visitor() = default;
            /** Generic visitor method. 
            @param rend The Renderable instance being visited
            @param lodIndex The LOD index to which this Renderable belongs. Some
                objects support LOD and this will tell you whether the Renderable
                you're looking at is from the top LOD (0) or otherwise
            @param isDebug Whether this is a debug renderable or not.
            @param pAny Optional pointer to some additional data that the class
                calling the visitor may populate if it chooses to.
            */
            virtual void visit(Renderable* rend, ushort lodIndex, bool isDebug, 
                ::std::any* pAny = nullptr) = 0;
        };

    protected:
        using CustomParameterMap = std::map<size_t, Vector4>;
        CustomParameterMap mCustomParameters;
        UserObjectBindings mUserObjectBindings;      /// User objects binding.
        bool mPolygonModeOverrideable{true};
        bool mUseIdentityProjection{false};
        bool mUseIdentityView{false};
    };

    /** @} */
    /** @} */

} // namespace Ogre
