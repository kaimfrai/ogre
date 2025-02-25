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

export module Ogre.Core:SkeletonSerializer;

export import :Prerequisites;
export import :Serializer;

export
namespace Ogre {

    struct LinkedSkeletonAnimationSource;
class Animation;
class Bone;
class NodeAnimationTrack;
class Skeleton;
class TransformKeyFrame;

    /// Skeleton compatibility versions
    enum class SkeletonVersion 
    {
        /// OGRE version v1.0+
        _1_0,
        /// OGRE version v1.8+
        _1_8,
        
        /// Latest version available
        Latest = 100
    };

    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup Animation
    *  @{
    */
    /** Class for serialising skeleton data to/from an OGRE .skeleton file.
    @remarks
        This class allows exporters to write OGRE .skeleton files easily, and allows the
        OGRE engine to import .skeleton files into instantiated OGRE Skeleton objects.
        Note that a .skeleton file includes not only the Skeleton, but also definitions of
        any Animations it uses.
    @par
        To export a Skeleton:<OL>
        <LI>Create a Skeleton object and populate it using it's methods.</LI>
        <LI>Call the exportSkeleton method</LI>
        </OL>
    */
    class SkeletonSerializer : public Serializer
    {
        
    public:
        SkeletonSerializer();

        /** Exports a skeleton to the file specified. 
        @remarks
            This method takes an externally created Skeleton object, and exports both it
            and animations it uses to a .skeleton file.
        @param pSkeleton Weak reference to the Skeleton to export
        @param filename The destination filename
        @param ver @copydoc SkeletonVersion
        @param endianMode The endian mode to write in
        */
        void exportSkeleton(const Skeleton* pSkeleton, std::string_view filename,
            SkeletonVersion ver = SkeletonVersion::Latest, std::endian endianMode = std::endian::native);

        /** Exports a skeleton to the stream specified. 
        @remarks
            This method takes an externally created Skeleton object, and exports both it
            and animations it uses to a .skeleton file.
        @param pSkeleton Weak reference to the Skeleton to export
        @param stream The destination stream
        @param ver @copydoc SkeletonVersion
        @param endianMode The endian mode to write in
        */
        void exportSkeleton(const Skeleton* pSkeleton, DataStreamPtr stream,
            SkeletonVersion ver = SkeletonVersion::Latest, std::endian endianMode = std::endian::native);
        /** Imports Skeleton and animation data from a .skeleton file DataStream.
        @remarks
            This method imports data from a DataStream opened from a .skeleton file and places it's
            contents into the Skeleton object which is passed in. 
        @param stream The DataStream holding the .skeleton data. Must be initialised (pos at the start of the buffer).
        @param pDest Weak reference to the Skeleton object which will receive the data. Should be blank already.
        */
        void importSkeleton(DataStreamPtr& stream, Skeleton* pDest);

        // TODO: provide Cal3D importer?

    private:
        
        void setWorkingVersion(SkeletonVersion ver);
        
        // Internal export methods
        void writeSkeleton(const Skeleton* pSkel, SkeletonVersion ver);
        void writeBone(const Skeleton* pSkel, const Bone* pBone);
        void writeBoneParent(const Skeleton* pSkel, unsigned short boneId, unsigned short parentId);
        void writeAnimation(const Skeleton* pSkel, const Animation* anim, SkeletonVersion ver);
        void writeAnimationTrack(const Skeleton* pSkel, const NodeAnimationTrack* track);
        void writeKeyFrame(const Skeleton* pSkel, const TransformKeyFrame* key);
        void writeSkeletonAnimationLink(const Skeleton* pSkel, 
            const LinkedSkeletonAnimationSource& link);

        // Internal import methods
        void readFileHeader(DataStreamPtr& stream);
        void readBone(DataStreamPtr& stream, Skeleton* pSkel);
        void readBoneParent(DataStreamPtr& stream, Skeleton* pSkel);
        void readAnimation(DataStreamPtr& stream, Skeleton* pSkel);
        void readAnimationTrack(DataStreamPtr& stream, Animation* anim, Skeleton* pSkel);
        void readKeyFrame(DataStreamPtr& stream, NodeAnimationTrack* track, Skeleton* pSkel);
        void readSkeletonAnimationLink(DataStreamPtr& stream, Skeleton* pSkel);

        auto calcBoneSize(const Skeleton* pSkel, const Bone* pBone) -> size_t;
        auto calcBoneSizeWithoutScale(const Skeleton* pSkel, const Bone* pBone) -> size_t;
        auto calcBoneParentSize(const Skeleton* pSkel) -> size_t;
        auto calcAnimationSize(const Skeleton* pSkel, const Animation* pAnim, SkeletonVersion ver) -> size_t;
        auto calcAnimationTrackSize(const Skeleton* pSkel, const NodeAnimationTrack* pTrack) -> size_t;
        auto calcKeyFrameSize(const Skeleton* pSkel, const TransformKeyFrame* pKey) -> size_t;
        auto calcKeyFrameSizeWithoutScale(const Skeleton* pSkel, const TransformKeyFrame* pKey) -> size_t;
        auto calcSkeletonAnimationLinkSize(const Skeleton* pSkel, 
            const LinkedSkeletonAnimationSource& link) -> size_t;




    };
    /** @} */
    /** @} */

}
