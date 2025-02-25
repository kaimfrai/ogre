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

export module Ogre.Core:InstanceManager;

export import :Common;
export import :InstanceBatch;
export import :IteratorWrapper;
export import :MemoryAllocatorConfig;
export import :Platform;
export import :Prerequisites;
export import :RenderOperation;
export import :SharedPtr;

export import <algorithm>;
export import <map>;
export import <string>;
export import <vector>;

export
namespace Ogre
{
    class InstancedEntity;
    class SceneManager;
    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup Scene
    *  @{
    */

    /** This is the main starting point for the new instancing system.
        Each InstanceManager can control one technique and one mesh, but it can manage
        multiple materials at the same time.
        @ref SceneManager::createInstanceManager, which creates this InstanceManager. Each one
        must have a unique name. It's wasteless to create two InstanceManagers with the same
        mesh reference, instancing technique and instances per batch count.
        This class takes care of managing batches automatically, so that more are created when
        needed, and reuse existing ones as much as possible; thus the user doesn't have to worry
        of managing all those low level issues.

        @see @ref InstanceBatch
        @see @ref InstancedEntity
        @see Design discussion thread: http://www.ogre3d.org/forums/viewtopic.php?f=4&t=59902
     */
    class InstanceManager : public FactoryAlloc
    {
    public:
        enum class InstancingTechnique
        {
            ShaderBased,            ///< %Any SM 2.0+ @ref InstanceBatchShader
            TextureVTF,             ///< Needs Vertex Texture Fetch & SM 3.0+ @ref InstanceBatchVTF
            HWInstancingBasic,      ///< Needs SM 3.0+ and HW instancing support @ref InstanceBatchHW
            HWInstancingVTF,        ///< Needs SM 3.0+, HW instancing support & VTF @ref InstanceBatchHW_VTF
            InstancingTechniquesCount
        };

        using enum InstancingTechnique;

        /** Values to be used in setSetting() & BatchSettings::setting */
        enum class BatchSettingId
        {
            /// Makes all batches from same material cast shadows
            CAST_SHADOWS        = 0,
            /// Makes each batch to display it's bounding box. Useful for debugging or profiling
            SHOW_BOUNDINGBOX,

            NUM_SETTINGS
        };

        using enum BatchSettingId;

    private:
        struct BatchSettings
        {
            //These are all per material
            bool setting[std::to_underlying(BatchSettingId::NUM_SETTINGS)];

            BatchSettings()
            {
                using enum BatchSettingId;
                setting[std::to_underlying(CAST_SHADOWS)]     = true;
                setting[std::to_underlying(SHOW_BOUNDINGBOX)] = false;
            }
        };

        using InstanceBatchVec = std::vector<InstanceBatch *>;   //vec[batchN] = Batch
        using InstanceBatchMap = std::map<std::string_view, ::std::vector<::std::unique_ptr<InstanceBatch>>>;   //map[materialName] = Vec

        using BatchSettingsMap = std::map<std::string_view, BatchSettings>;

        const String            mName;                  //Not the name of the mesh
        MeshPtr                 mMeshReference;
        InstanceBatchMap        mInstanceBatches;
        size_t                  mIdCount{ 0 };

        InstanceBatchVec        mDirtyBatches;

        RenderOperation         mSharedRenderOperation;

        size_t                  mInstancesPerBatch;
        InstancingTechnique     mInstancingTechnique;
        InstanceManagerFlags    mInstancingFlags;       ///< @see InstanceManagerFlags
        unsigned short          mSubMeshIdx;
        
        BatchSettingsMap        mBatchSettings;
        SceneManager*           mSceneManager;

        size_t                  mMaxLookupTableInstances{16};
        unsigned char           mNumCustomParams{ 0 };       //Number of custom params per instance.

        /** Finds a batch with at least one free instanced entity we can use.
            If none found, creates one.
        */
        inline auto getFreeBatch( std::string_view materialName ) -> InstanceBatch*;

        /** Called when batches are fully exhausted (can't return more instances) so a new batch
            is created.
            For the first time use, it can take big build time.
            It takes care of getting the render operation which will be shared by further batches,
            which decreases their build time, and prevents GPU RAM from skyrocketing.
        @param materialName The material name, to know where to put this batch in the map
        @param firstTime True if this is the first time it is called
        @return The created InstancedManager for convenience
        */
        auto buildNewBatch( std::string_view materialName, bool firstTime ) -> InstanceBatch*;

        /** @see defragmentBatches overload, this takes care of an array of batches
            for a specific material */
        void defragmentBatches( bool optimizeCull, InstanceBatch::InstancedEntityVec &entities,
                                InstanceBatch::CustomParamsVec &usedParams,
                                ::std::vector<::std::unique_ptr<InstanceBatch>> &fragmentedBatches );

        /** @see setSetting. This function helps it by setting the given parameter to all batches
            in container.
        */
        void applySettingToBatches( BatchSettingId id, bool value, const ::std::vector<::std::unique_ptr<InstanceBatch>> &container );

        /** Called when we you use a mesh which has shared vertices, the function creates separate
            vertex/index buffers and also recreates the bone assignments.
        */
        static void unshareVertices(const Ogre::MeshPtr &mesh);

    public:
        InstanceManager(std::string_view customName, SceneManager *sceneManager,
                         std::string_view meshName, std::string_view groupName,
                         InstancingTechnique instancingTechnique, InstanceManagerFlags instancingFlags,
                         size_t instancesPerBatch, unsigned short subMeshIdx, bool useBoneMatrixLookup = false);

        [[nodiscard]] auto getName() const noexcept -> std::string_view { return mName; }

        [[nodiscard]] auto getSceneManager() const noexcept -> SceneManager* { return mSceneManager; }

        /** Raises an exception if trying to change it after creating the first InstancedEntity
        The actual value may be less if the technique doesn't support having so much.
        See @ref getMaxOrBestNumInstancesPerBatch for the usefulness of this function
        @param instancesPerBatch New instances per batch number
        */
        void setInstancesPerBatch( size_t instancesPerBatch );

        /** Sets the size of the lookup table for techniques supporting bone lookup table.
            Raises an exception if trying to change it after creating the first InstancedEntity.
            Setting this value below the number of unique (non-sharing) entity instance animations
            will produce a crash during runtime. Setting this value above will increase memory
            consumption and reduce framerate.
        @remarks The value should be as close but not below the actual value. 
        @param maxLookupTableInstances New size of the lookup table
        */
        void setMaxLookupTableInstances( size_t maxLookupTableInstances );

        /** Sets the number of custom parameters per instance. Some techniques (i.e. HWInstancingBasic)
            support this, but not all of them. They also may have limitations to the max number. All
            instancing implementations assume each instance param is a Vector4 (4 floats).
        @remarks
            This function cannot be called after the first batch has been created. Otherwise
            it will raise an exception. If the technique doesn't support custom params, it will
            raise an exception at the time of building the first InstanceBatch.

            HWInstancingBasic:
                * Each custom params adds an additional float4 TEXCOORD.
            HWInstancingVTF:
                * Not implemented. (Recommendation: Implement this as an additional float4 VTF fetch)
            TextureVTF:
                * Not implemented. (see HWInstancingVTF's recommendation)
            ShaderBased:
                * Not supported.
        @param numCustomParams Number of custom parameters each instance will have. Default: 0
        */
        void setNumCustomParams( unsigned char numCustomParams );

        [[nodiscard]] auto getNumCustomParams() const
        noexcept -> unsigned char { return mNumCustomParams; }

        /** @return Instancing technique this manager was created for. Can't be changed after creation */
        [[nodiscard]] auto getInstancingTechnique() const
        noexcept -> InstancingTechnique { return mInstancingTechnique; }

        /** Calculates the maximum (or the best amount, depending on flags) of instances
            per batch given the suggested size for the technique this manager was created for.
        @remarks
            This is done automatically when creating an instanced entity, but this function in conjunction
            with @ref setInstancesPerBatch allows more flexible control over the amount of instances
            per batch
        @param materialName Name of the material to base on
        @param suggestedSize Suggested amount of instances per batch
        @param flags @ref InstanceManagerFlags to pass to the InstanceManager
        @return The max/best amount of instances per batch given the suggested size and flags
        */
        auto getMaxOrBestNumInstancesPerBatch( std::string_view materialName, size_t suggestedSize, InstanceManagerFlags flags ) -> size_t;

        /// Creates an InstancedEntity
        auto createInstancedEntity( std::string_view materialName ) -> InstancedEntity*;

        /** This function can be useful to improve CPU speed after having too many instances
            created, which where now removed, thus freeing many batches with zero used Instanced Entities
            However the batches aren't automatically removed from memory until the InstanceManager is
            destroyed, or this function is called. This function removes those batches which are completely
            unused (only wasting memory).
        */
        void cleanupEmptyBatches();

        /** After creating many entities (which turns in many batches) and then removing entities that
            are in the middle of these batches, there might be many batches with many free entities.
            Worst case scenario, there could be left one batch per entity. Imagine there can be
            80 entities per batch, there are 80 batches, making a total of 6400 entities. Then
            6320 of those entities are removed in a very specific way, which leads to having
            80 batches, 80 entities, and GPU vertex shader still needs to process 6400!
            This is called fragmentation. This function reparents the InstancedEntities
            to fewer batches, in this case leaving only one batch with 80 entities

        @remarks
            This function takes time. Make sure to call this only when you're sure there's
            too much of fragmentation and you won't be creating more InstancedEntities soon
            Also in many cases cleanupEmptyBatches() ought to be enough
            Defragmentation is done per material
            Static batches won't be defragmented. If you want to degragment them, set them
            to dynamic again, and switch back to static after calling this function.

        @param optimizeCulling When true, entities close together will be reorganized
            in the same batch for more efficient CPU culling. This can take more CPU
            time. You want this to be false if you now you're entities are moving very
            randomly which tends them to get separated and spread all over the scene
            (which nullifies any CPU culling)
        */
        void defragmentBatches( bool optimizeCulling );

        /** Applies a setting for all batches using the same material

            If the material name hasn't been used, the settings are still stored
            This allows setting up batches before they get even created.
            @par Examples
            `setSetting(InstanceManager::CAST_SHADOWS, false, "")` disables shadow
            casting for all instanced entities (see @ref MovableObject::setCastShadows)
            @par
            `setSetting(InstanceManager::SHOW_BOUNDINGBOX, true, "MyMat")`
            will display the bounding box of the batch (not individual InstancedEntities)
            from all batches using material "MyMat"
        @param id @ref BatchSettingId to setup
        @param enabled Boolean value. It's meaning depends on the id.
        @param materialName When Blank, the setting is applied to all existing materials
        */
        void setSetting( BatchSettingId id, bool enabled, std::string_view materialName = BLANKSTRING );

        /// If settings for the given material didn't exist, default value is returned
        [[nodiscard]] auto getSetting( BatchSettingId id, std::string_view materialName ) const -> bool;

        /** Returns true if settings were already created for the given material name.
            If false is returned, it means getSetting will return default settings.
        */
        [[nodiscard]] auto hasSettings( std::string_view materialName ) const -> bool;

        /** @copydoc InstanceBatch::setStaticAndUpdate */
        void setBatchesAsStaticAndUpdate( bool bStatic );

        /** Called by an InstanceBatch when it requests their bounds to be updated for proper culling
        @param dirtyBatch The batch which is dirty, usually same as caller.
        */
        void _addDirtyBatch( InstanceBatch *dirtyBatch );

        /** Called by SceneManager when we told it we have at least one dirty batch */
        void _updateDirtyBatches();

        using InstanceBatchMapIterator = ConstMapIterator<InstanceBatchMap>;
        using InstanceBatchIterator = ConstVectorIterator<::std::vector<::std::unique_ptr<InstanceBatch>>>;

        /// Get non-updateable iterator over instance batches per material
        [[nodiscard]] auto getInstanceBatchMapIterator() const -> InstanceBatchMapIterator
        { return { mInstanceBatches.begin(), mInstanceBatches.end() }; }

        /** Get non-updateable iterator over instance batches for given material
        @remarks
            Each InstanceBatch pointer may be modified for low level usage (i.e.
            setCustomParameter), but there's no synchronization mechanism when
            multithreading or creating more instances, that's up to the user.
        */
        [[nodiscard]] auto getInstanceBatchIterator( std::string_view materialName ) const -> InstanceBatchIterator;
    };
} // namespace Ogre
