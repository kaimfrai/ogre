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

export module Ogre.Core:InstanceBatchHW_VTF;

export import :InstanceBatchVTF;
export import :Mesh;
export import :Platform;
export import :Prerequisites;
export import :SharedPtr;

export
namespace Ogre
{
class Camera;
class InstanceManager;
class RenderQueue;
class SubMesh;
class VertexData;
    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup Scene
    *  @{
    */

    /** Instancing implementation using vertex texture through Vertex Texture Fetch (VTF) and
        hardware instancing.
        @see BaseInstanceBatchVTF and @see InstanceBatchHW

        The advantage over TextureVTF technique, is that this implements a basic culling algorithm
        to avoid useless processing in vertex shader and uses a lot less VRAM and memory bandwidth

        Basically it has the benefits of both TextureVTF (skeleton animations) and HWInstancingBasic
        (lower memory consumption and basic culling) techniques

        @remarks
            Design discussion webpage: http://www.ogre3d.org/forums/viewtopic.php?f=4&t=59902
        @author
            Matias N. Goldberg ("dark_sylinc")
        @version
            1.2
     */
    class InstanceBatchHW_VTF : public BaseInstanceBatchVTF
    {
    protected:
        bool    mKeepStatic{ false };

        //Pointer to the buffer containing the per instance vertex data
        HardwareVertexBufferSharedPtr mInstanceVertexBuffer;

        void setupVertices( const SubMesh* baseSubMesh ) override;
        void setupIndices( const SubMesh* baseSubMesh ) override;

        /** Creates 2 TEXCOORD semantics that will be used to sample the vertex texture */
        void createVertexSemantics( VertexData *thisVertexData, VertexData *baseVertexData,
            const HWBoneIdxVec &hwBoneIdx, const HWBoneWgtVec& hwBoneWgt ) override;

        /** updates the vertex buffer containing the per instance data 
        @param[in] isFirstTime Tells if this is the first time the buffer is being updated
        @param[in] currentCamera The camera being used for render (valid when using bone matrix lookup)
        @return The number of instances to be rendered
        */
        virtual auto updateInstanceDataBuffer(bool isFirstTime, Camera* currentCamera) -> size_t;


        auto checkSubMeshCompatibility( const SubMesh* baseSubMesh ) -> bool override;

        /** Keeps filling the VTF with world matrix data. Overloaded to avoid culled objects
            and update visible instances' animation
        */
        auto updateVertexTexture( Camera *currentCamera ) -> size_t;

        auto matricesTogetherPerRow() const noexcept -> bool override { return true; }
    public:
        InstanceBatchHW_VTF( InstanceManager *creator, MeshPtr &meshReference, const MaterialPtr &material,
                            size_t instancesPerBatch, const Mesh::IndexMap *indexToBoneMap,
                            std::string_view batchName );
        ~InstanceBatchHW_VTF() override;
        /** @see InstanceBatch::calculateMaxNumInstances */
        auto calculateMaxNumInstances( const SubMesh *baseSubMesh, InstanceManagerFlags flags ) const -> size_t override;

        /** @copydoc InstanceBatchHW::_boundsDirty */
        void _boundsDirty() override;

        /** @copydoc InstanceBatchHW::setStaticAndUpdate */
        void setStaticAndUpdate( bool bStatic ) override;

        auto isStatic() const noexcept -> bool override { return mKeepStatic; }

        /** Overloaded to visibility on a per unit basis and finally updated the vertex texture */
        void _updateRenderQueue( RenderQueue* queue ) override;
    };

}
