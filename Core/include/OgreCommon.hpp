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
#ifndef OGRE_CORE_COMMON_H
#define OGRE_CORE_COMMON_H
// Common stuff

#include <cassert>
#include <cstddef>
#include <map>
#include <ostream>
#include <string>
#include <vector>

#include "OgreMurmurHash3.hpp"
#include "OgrePlatform.hpp"
#include "OgrePrerequisites.hpp"
#include "OgreVector.hpp"

namespace Ogre {
class RenderWindow;

    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup General
    *  @{
    */

    /// Fast general hashing algorithm
    inline auto FastHash (const char * data, size_t len, uint32 hashSoFar = 0) -> uint32 {
        uint32 ret;
        MurmurHash3_x86_32(data, len, hashSoFar, &ret);
        return ret;
    }
    /// Combine hashes with same style as boost::hash_combine
    template <typename T>
    auto HashCombine (uint32 hashSoFar, const T& data) -> uint32
    {
        return FastHash((const char*)&data, sizeof(T), hashSoFar);
    }


    /** Comparison functions used for the depth/stencil buffer operations and 
        others. */
    enum class CompareFunction : uint8
    {
        ALWAYS_FAIL,  //!< Never writes a pixel to the render target
        ALWAYS_PASS,  //!< Always writes a pixel to the render target
        LESS,         //!< Write if (new_Z < existing_Z)
        LESS_EQUAL,   //!< Write if (new_Z <= existing_Z)
        EQUAL,        //!< Write if (new_Z == existing_Z)
        NOT_EQUAL,    //!< Write if (new_Z != existing_Z)
        GREATER_EQUAL,//!< Write if (new_Z >= existing_Z)
        GREATER       //!< Write if (new_Z >= existing_Z)
    };

    /** High-level filtering options providing shortcuts to settings the
        minification, magnification and mip filters. */
    enum class TextureFilterOptions
    {
        /// No filtering or mipmapping is used. 
        /// Equal to: min=Ogre::FilterOptions::POINT, mag=Ogre::FilterOptions::POINT, mip=Ogre::FilterOptions::NONE
        NONE,
        /// 2x2 box filtering is performed when magnifying or reducing a texture, and a mipmap is picked from the list but no filtering is done between the levels of the mipmaps. 
        /// Equal to: min=Ogre::FilterOptions::LINEAR, mag=Ogre::FilterOptions::LINEAR, mip=Ogre::FilterOptions::POINT
        BILINEAR,
        /// 2x2 box filtering is performed when magnifying and reducing a texture, and the closest 2 mipmaps are filtered together. 
        /// Equal to: min=Ogre::FilterOptions::LINEAR, mag=Ogre::FilterOptions::LINEAR, mip=Ogre::FilterOptions::LINEAR
        TRILINEAR,
        /// This is the same as ’trilinear’, except the filtering algorithm takes account of the slope of the triangle in relation to the camera rather than simply doing a 2x2 pixel filter in all cases.
        /// Equal to: min=Ogre::FilterOptions::ANISOTROPIC, max=Ogre::FilterOptions::ANISOTROPIC, mip=Ogre::FilterOptions::LINEAR
        ANISOTROPIC
    };

    enum class FilterType
    {
        /// The filter used when shrinking a texture
        Min,
        /// The filter used when magnifying a texture
        Mag,
        /// The filter used when determining the mipmap
        Mip
    };
    /** Filtering options for textures / mipmaps. */
    enum class FilterOptions : uint8
    {
        /// No filtering, used for FilterType::Mip to turn off mipmapping
        NONE,
        /// Use the closest pixel
        POINT,
        /// Average of a 2x2 pixel area, denotes bilinear for MIN and MAG, trilinear for MIP
        LINEAR,
        /// Similar to FilterOptions::LINEAR, but compensates for the angle of the texture plane. Note that in
        /// order for this to make any difference, you must also set the
        /// TextureUnitState::setTextureAnisotropy attribute too.
        ANISOTROPIC
    };

    /** Texture addressing modes - default is TextureAddressingMode::WRAP.
    */
    enum class TextureAddressingMode : uint8
    {
        /// %Any value beyond 1.0 wraps back to 0.0. %Texture is repeated.
        WRAP,
        /// %Texture flips every boundary, meaning texture is mirrored every 1.0 u or v
        MIRROR,
        /// Values beyond 1.0 are clamped to 1.0. %Texture ’streaks’ beyond 1.0 since last line
        /// of pixels is used across the rest of the address space. Useful for textures which
        /// need exact coverage from 0.0 to 1.0 without the ’fuzzy edge’ wrap gives when
        /// combined with filtering.
        CLAMP,
        /// %Texture coordinates outside the range [0.0, 1.0] are set to the border colour.
        BORDER,
        /// Unknown
        UNKNOWN = 99
    };

    /** %Light shading modes. */
    enum class ShadeOptions : uint8
    {
        FLAT, //!< No interpolation takes place. Each face is shaded with a single colour determined from the first vertex in the face.
        GOURAUD, //!< Colour at each vertex is linearly interpolated across the face.
        PHONG //!< Vertex normals are interpolated across the face, and these are used to determine colour at each pixel. Gives a more natural lighting effect but is more expensive and works better at high levels of tessellation. Not supported on all hardware.
    };

    /** Fog modes. */
    enum class FogMode : uint8
    {
        /// No fog. Duh.
        NONE,
        /// Fog density increases  exponentially from the camera (fog = 1/e^(distance * density))
        EXP,
        /// Fog density increases at the square of FogMode::EXP, i.e. even quicker (fog = 1/e^(distance * density)^2)
        EXP2,
        /// Fog density increases linearly between the start and end distances
        LINEAR
    };

    /** Hardware culling modes based on vertex winding.
        This setting applies to how the hardware API culls triangles it is sent. */
    enum class CullingMode : uint8
    {
        /// Hardware never culls triangles and renders everything it receives.
        NONE = 1,
        /// Hardware culls triangles whose vertices are listed clockwise in the view (default).
        CLOCKWISE = 2,
        /// Hardware culls triangles whose vertices are listed anticlockwise in the view.
        ANTICLOCKWISE = 3
    };

    /** Manual culling modes based on vertex normals.
        This setting applies to how the software culls triangles before sending them to the 
        hardware API. This culling mode is used by scene managers which choose to implement it -
        normally those which deal with large amounts of fixed world geometry which is often 
        planar (software culling movable variable geometry is expensive). */
    enum class ManualCullingMode : uint8
    {
        /// No culling so everything is sent to the hardware.
        NONE = 1,
        /// Cull triangles whose normal is pointing away from the camera (default).
        BACK = 2,
        /// Cull triangles whose normal is pointing towards the camera.
        FRONT = 3
    };

    /** Enumerates the wave types usable with the Ogre engine. */
    enum class WaveformType
    {
        /// Standard sine wave which smoothly changes from low to high and back again.
        SINE,
        /// An angular wave with a constant increase / decrease speed with pointed peaks.
        TRIANGLE,
        /// Half of the time is spent at the min, half at the max with instant transition between.
        SQUARE,
        /// Gradual steady increase from min to max over the period with an instant return to min at the end.
        SAWTOOTH,
        /// Gradual steady decrease from max to min over the period, with an instant return to max at the end.
        INVERSE_SAWTOOTH,
        /// Pulse Width Modulation. Works like WaveformType::SQUARE, except the high to low transition is controlled by duty cycle.
        /// With a duty cycle of 50% (0.5) will give the same output as WaveformType::SQUARE.
        PWM
    };

    /** The polygon mode to use when rasterising. */
    enum class PolygonMode : uint8
    {
        /// Only the points of each polygon are rendered.
        POINTS = 1,
        /// Polygons are drawn in outline only.
        WIREFRAME = 2,
        /// The normal situation - polygons are filled in.
        SOLID = 3
    };

    /** An enumeration of broad shadow techniques */
    enum class ShadowTechnique
    {
        /** No shadows */
        NONE = 0x00,
        /** Mask for additive shadows (not for direct use, use  ShadowTechnique:: enum class instead)
        */
        DETAIL_ADDITIVE = 0x01,
        /** Mask for modulative shadows (not for direct use, use  ShadowTechnique:: enum class instead)
        */
        DETAIL_MODULATIVE = 0x02,
        /** Mask for integrated shadows (not for direct use, use ShadowTechnique:: enum class instead)
        */
        DETAIL_INTEGRATED = 0x04,
        /** Mask for stencil shadows (not for direct use, use  ShadowTechnique:: enum class instead)
        */
        DETAIL_STENCIL = 0x10,
        /** Mask for texture shadows (not for direct use, use  ShadowTechnique:: enum class instead)
        */
        DETAIL_TEXTURE = 0x20,
        
        /** Stencil shadow technique which renders all shadow volumes as
            a modulation after all the non-transparent areas have been 
            rendered. This technique is considerably less fillrate intensive 
            than the additive stencil shadow approach when there are multiple
            lights, but is not an accurate model. 
        */
        STENCIL_MODULATIVE = DETAIL_STENCIL | DETAIL_MODULATIVE,
        /** Stencil shadow technique which renders each light as a separate
            additive pass to the scene. This technique can be very fillrate
            intensive because it requires at least 2 passes of the entire
            scene, more if there are multiple lights. However, it is a more
            accurate model than the modulative stencil approach and this is
            especially apparent when using coloured lights or bump mapping.
        */
        STENCIL_ADDITIVE = DETAIL_STENCIL | DETAIL_ADDITIVE,
        /** Texture-based shadow technique which involves a monochrome render-to-texture
            of the shadow caster and a projection of that texture onto the 
            shadow receivers as a modulative pass. 
        */
        TEXTURE_MODULATIVE = DETAIL_TEXTURE | DETAIL_MODULATIVE,
        
        /** Texture-based shadow technique which involves a render-to-texture
            of the shadow caster and a projection of that texture onto the 
            shadow receivers, built up per light as additive passes. 
            This technique can be very fillrate intensive because it requires numLights + 2 
            passes of the entire scene. However, it is a more accurate model than the 
            modulative approach and this is especially apparent when using coloured lights 
            or bump mapping.
        */
        TEXTURE_ADDITIVE = DETAIL_TEXTURE | DETAIL_ADDITIVE,

        /** Texture-based shadow technique which involves a render-to-texture
        of the shadow caster and a projection of that texture on to the shadow
        receivers, with the usage of those shadow textures completely controlled
        by the materials of the receivers.
        This technique is easily the most flexible of all techniques because 
        the material author is in complete control over how the shadows are
        combined with regular rendering. It can perform shadows as accurately
        as ShadowTechnique::TEXTURE_ADDITIVE but more efficiently because it requires
        less passes. However it also requires more expertise to use, and 
        in almost all cases, shader capable hardware to really use to the full.
        @note The 'additive' part of this mode means that the colour of
        the rendered shadow texture is by default plain black. It does
        not mean it does the adding on your receivers automatically though, how you
        use that result is up to you.
        */
        TEXTURE_ADDITIVE_INTEGRATED = TEXTURE_ADDITIVE | ShadowTechnique::DETAIL_INTEGRATED,
        /** Texture-based shadow technique which involves a render-to-texture
            of the shadow caster and a projection of that texture on to the shadow
            receivers, with the usage of those shadow textures completely controlled
            by the materials of the receivers.
            This technique is easily the most flexible of all techniques because 
            the material author is in complete control over how the shadows are
            combined with regular rendering. It can perform shadows as accurately
            as ShadowTechnique::TEXTURE_ADDITIVE but more efficiently because it requires
            less passes. However it also requires more expertise to use, and 
            in almost all cases, shader capable hardware to really use to the full.
            @note The 'modulative' part of this mode means that the colour of
            the rendered shadow texture is by default the 'shadow colour'. It does
            not mean it modulates on your receivers automatically though, how you
            use that result is up to you.
        */
        TEXTURE_MODULATIVE_INTEGRATED = TEXTURE_MODULATIVE | DETAIL_INTEGRATED
    };

    auto constexpr operator not(ShadowTechnique value) -> bool
    {
        return not std::to_underlying(value);
    }

    auto constexpr operator bitand(ShadowTechnique left, ShadowTechnique right) -> ShadowTechnique
    {
        return static_cast<ShadowTechnique>
        (    std::to_underlying(left)
        bitand
            std::to_underlying(right)
        );
    }

    /** An enumeration describing which material properties should track the vertex colours */
    enum class TrackVertexColourEnum {
        NONE        = 0x0,
        AMBIENT     = 0x1,
        DIFFUSE     = 0x2,
        SPECULAR    = 0x4,
        EMISSIVE    = 0x8
    };
    using TrackVertexColourType = TrackVertexColourEnum;

    auto constexpr operator bitor(TrackVertexColourEnum left, TrackVertexColourEnum right) -> TrackVertexColourEnum
    {
        return static_cast<TrackVertexColourEnum>
        (   std::to_underlying(left)
        bitand
            std::to_underlying(right)
        );
    }

    auto constexpr operator not(TrackVertexColourEnum value) -> bool
    {
        return not std::to_underlying(value);
    }

    auto constexpr operator bitand(TrackVertexColourEnum left, TrackVertexColourEnum right) -> TrackVertexColourEnum
    {
        return static_cast<TrackVertexColourEnum>
        (   std::to_underlying(left)
        bitand
            std::to_underlying(right)
        );
    }

    /** Function used compute the camera-distance for sorting objects */
    enum class SortMode : uint8
    {

        /** Sort by direction of the camera
         *
         * The distance along the camera view as in `cam->getDerivedDirection().dotProduct(diff)`
         * Best for @ref ProjectionType::ORTHOGRAPHIC
         */
        Direction,
        /** Sort by distance from the camera
         *
         * The euclidean distance as in `diff.squaredLength()`
         * Best for @ref ProjectionType::PERSPECTIVE
         */
        Distance
    };

    /** Defines the frame buffer types. */
    enum class FrameBufferType : uint32 {
        COLOUR  = 0x1,
        DEPTH   = 0x2,
        STENCIL = 0x4
    };

    auto constexpr operator not (FrameBufferType mask) -> bool
    {
        return not std::to_underlying(mask);
    }

    auto constexpr operator bitor(FrameBufferType left, FrameBufferType right) -> FrameBufferType
    {
        return
        static_cast<FrameBufferType>
        (   std::to_underlying(left)
        bitor
            std::to_underlying(right)
        );
    }

    auto constexpr operator |= (FrameBufferType& left, FrameBufferType right) -> FrameBufferType&
    {
        return left = left bitor right;
    }

    auto constexpr operator bitand(FrameBufferType left, FrameBufferType right) -> FrameBufferType
    {
        return
        static_cast<FrameBufferType>
        (   std::to_underlying(left)
        bitand
            std::to_underlying(right)
        );
    }

    auto constexpr operator &= (FrameBufferType& left, FrameBufferType right) -> FrameBufferType&
    {
        return left = left bitand right;
    }
	
	/** Defines the colour buffer types. */
    enum class ColourBufferType
    {
      BACK = 0x0,
      BACK_LEFT,
      BACK_RIGHT
    };
	
	/** Defines the stereo mode types. */
    enum class StereoModeType
    {
      NONE = 0x0,
      FRAME_SEQUENTIAL
    };

    /** Flags for the Instance Manager when calculating ideal number of instances per batch */
    enum class InstanceManagerFlags : uint16
    {
        /** Forces an amount of instances per batch low enough so that vertices * numInst < 65535
            since usually improves performance. In HW instanced techniques, this flag is ignored
        */
        USE16BIT     = 0x0001,

        /** The num. of instances is adjusted so that as few pixels as possible are wasted
            in the vertex texture */
        VTFBESTFIT   = 0x0002,

        /** Use a limited number of skeleton animations shared among all instances. 
        Update only that limited amount of animations in the vertex texture.*/
        VTFBONEMATRIXLOOKUP = 0x0004,

        USEBONEDUALQUATERNIONS = 0x0008,

        /** Use one weight per vertex when recommended (i.e. VTF). */
        USEONEWEIGHT = 0x0010,

        /** All techniques are forced to one weight per vertex. */
        FORCEONEWEIGHT = 0x0020,

        USEALL       = USE16BIT|VTFBESTFIT|USEONEWEIGHT
    };

    auto constexpr operator not(InstanceManagerFlags value) -> bool
    {
        return not std::to_underlying(value);
    }
    
    auto constexpr operator bitand(InstanceManagerFlags left, InstanceManagerFlags right) -> InstanceManagerFlags
    {
        return static_cast<InstanceManagerFlags>
        (   std::to_underlying(left)
        bitand
            std::to_underlying(right)
        );
    }
    
    /** A hashed vector.
    */
    template <typename T>
    class HashedVector
    {
    public:
        using VectorImpl = typename std::vector<T>;
    protected:
        VectorImpl mList;
        mutable uint32 mListHash;
        mutable bool mListHashDirty;

        void addToHash(const T& newPtr) const
        {
            mListHash = FastHash((const char*)&newPtr, sizeof(T), mListHash);
        }
        void recalcHash() const
        {
            mListHash = 0;
            for (auto const& i :mList)
                addToHash(i);
            mListHashDirty = false;
            
        }

    public:
        using value_type = typename VectorImpl::value_type;
        using pointer = typename VectorImpl::pointer;
        using reference = typename VectorImpl::reference;
        using const_reference = typename VectorImpl::const_reference;
        using size_type = typename VectorImpl::size_type;
        using difference_type = typename VectorImpl::difference_type;
        using iterator = typename VectorImpl::iterator;
        using const_iterator = typename VectorImpl::const_iterator;
        using reverse_iterator = typename VectorImpl::reverse_iterator;
        using const_reverse_iterator = typename VectorImpl::const_reverse_iterator;

        void dirtyHash()
        {
            mListHashDirty = true;
        }
        auto isHashDirty() const noexcept -> bool
        {
            return mListHashDirty;
        }

        auto begin() -> iterator 
        { 
            // we have to assume that hash needs recalculating on non-const
            dirtyHash();
            return mList.begin(); 
        }
        auto end() -> iterator { return mList.end(); }
        auto begin() const -> const_iterator { return mList.begin(); }
        auto end() const -> const_iterator { return mList.end(); }
        auto rbegin() -> reverse_iterator 
        { 
            // we have to assume that hash needs recalculating on non-const
            dirtyHash();
            return mList.rbegin(); 
        }
        auto rend() -> reverse_iterator { return mList.rend(); }
        auto rbegin() const -> const_reverse_iterator { return mList.rbegin(); }
        auto rend() const -> const_reverse_iterator { return mList.rend(); }
        auto size() const -> size_type { return mList.size(); }
        auto max_size() const -> size_type { return mList.max_size(); }
        auto capacity() const -> size_type { return mList.capacity(); }
        auto empty() const -> bool { return mList.empty(); }
        auto operator[](size_type n) -> reference 
        { 
            // we have to assume that hash needs recalculating on non-const
            dirtyHash();
            return mList[n]; 
        }
        auto operator[](size_type n) const -> const_reference { return mList[n]; }
        auto at(size_type n) -> reference 
        { 
            // we have to assume that hash needs recalculating on non-const
            dirtyHash();
            return mList.const_iterator(n); 
        }
        auto at(size_type n) const -> const_reference { return mList.at(n); }
        HashedVector() : mListHash(0), mListHashDirty(false) {}
        HashedVector(size_type n) : mList(n), mListHash(0), mListHashDirty(n > 0) {}
        HashedVector(size_type n, const T& t) : mList(n, t), mListHash(0), mListHashDirty(n > 0) {}
        HashedVector(const HashedVector<T>& rhs) 
            : mList(rhs.mList), mListHash(rhs.mListHash), mListHashDirty(rhs.mListHashDirty) {}

        template <class InputIterator>
        HashedVector(InputIterator a, InputIterator b)
            : mList(a, b), mListHash(0), mListHashDirty(false)
        {
            dirtyHash();
        }

        ~HashedVector() = default;
        auto operator=(const HashedVector<T>& rhs) -> HashedVector<T>&
        {
            mList = rhs.mList;
            mListHash = rhs.mListHash;
            mListHashDirty = rhs.mListHashDirty;
            return *this;
        }

        void reserve(size_t t) { mList.reserve(t); }
        auto front() -> reference 
        { 
            // we have to assume that hash needs recalculating on non-const
            dirtyHash();
            return mList.front(); 
        }
        auto front() const -> const_reference { return mList.front(); }
        auto back() -> reference  
        { 
            // we have to assume that hash needs recalculating on non-const
            dirtyHash();
            return mList.back(); 
        }
        auto back() const -> const_reference { return mList.back(); }
        void push_back(const T& t)
        { 
            mList.push_back(t);
            // Quick progressive hash add
            if (!isHashDirty())
                addToHash(t);
        }
        void pop_back()
        {
            mList.pop_back();
            dirtyHash();
        }
        void swap(HashedVector<T>& rhs)
        {
            mList.swap(rhs.mList);
            dirtyHash();
        }
        auto insert(iterator pos, const T& t) -> iterator
        {
            bool recalc = (pos != end());
            iterator ret = mList.insert(pos, t);
            if (recalc)
                dirtyHash();
            else
                addToHash(t);
            return ret;
        }

        template <class InputIterator>
        void insert(iterator pos,
            InputIterator f, InputIterator l)
        {
            mList.insert(pos, f, l);
            dirtyHash();
        }

        void insert(iterator pos, size_type n, const T& x)
        {
            mList.insert(pos, n, x);
            dirtyHash();
        }

        auto erase(iterator pos) -> iterator
        {
            iterator ret = mList.erase(pos);
            dirtyHash();
            return ret;
        }
        auto erase(iterator first, iterator last) -> iterator
        {
            auto ret = mList.erase(first, last);
            dirtyHash();
            return ret;
        }
        void clear()
        {
            mList.clear();
            mListHash = 0;
            mListHashDirty = false;
        }

        void resize(size_type n, const T& t = T())
        {
            bool recalc = false;
            if (n != size())
                recalc = true;

            mList.resize(n, t);
            if (recalc)
                dirtyHash();
        }

        [[nodiscard]] auto operator==(const HashedVector<T>& b) const noexcept -> bool
        { return mListHash == b.mListHash; }

        [[nodiscard]] auto operator<=> (const HashedVector<T>& b) const noexcept
        { return mListHash <=> b.mListHash; }


        /// Get the hash value
        auto getHash() const noexcept -> uint32 
        { 
            if (isHashDirty())
                recalcHash();

            return mListHash; 
        }
    public:



    };

    class Light;

    using LightList = HashedVector<Light *>;


    /// Constant blank string, useful for returning by ref where local does not exist
    const std::string_view BLANKSTRING;

    using UnaryOptionList = std::map<std::string_view, bool>;
    using BinaryOptionList = std::map<std::string_view, String>;

    /// Name / value parameter pair (first = name, second = value)
    using NameValuePairList = std::map<std::string_view, String>;

    /// Alias / Texture name pair (first = alias, second = texture name)
    using AliasTextureNamePairList = std::map<std::string_view, String>;

        template< typename T > struct TRect
        {
          T left, top, right, bottom;
          TRect() : left(0), top(0), right(0), bottom(0) {}
          TRect( T const & l, T const & t, T const & r, T const & b )
            : left( l ), top( t ), right( r ), bottom( b )
          {
          }
          TRect( TRect const & o )
            : left( o.left ), top( o.top ), right( o.right ), bottom( o.bottom )
          {
          }
          auto operator=( TRect const & o ) -> TRect &
          {
            left = o.left;
            top = o.top;
            right = o.right;
            bottom = o.bottom;
            return *this;
          }
          [[nodiscard]] auto width() const -> T
          {
            return right - left;
          }
          [[nodiscard]] auto height() const -> T
          {
            return bottom - top;
          }
          [[nodiscard]] auto isNull() const noexcept -> bool
          {
              return width() == 0 || height() == 0;
          }
          void setNull()
          {
              left = right = top = bottom = 0;
          }
          auto merge(const TRect& rhs) -> TRect &
          {
              assert(right >= left && bottom >= top);
              assert(rhs.right >= rhs.left && rhs.bottom >= rhs.top);
              if (isNull())
              {
                  *this = rhs;
              }
              else if (!rhs.isNull())
              {
                  left = std::min(left, rhs.left);
                  right = std::max(right, rhs.right);
                  top = std::min(top, rhs.top);
                  bottom = std::max(bottom, rhs.bottom);
              }

              return *this;

          }

          /**
           * Returns the intersection of the two rectangles.
           *
           * Note that the rectangles extend downwards. I.e. a valid box will
           * have "right > left" and "bottom > top".
           * @param rhs Another rectangle.
           * @return The intersection of the two rectangles. Zero size if they don't intersect.
           */
          [[nodiscard]] auto intersect(const TRect& rhs) const -> TRect
          {
              assert(right >= left && bottom >= top);
              assert(rhs.right >= rhs.left && rhs.bottom >= rhs.top);
              TRect ret;
              if (isNull() || rhs.isNull())
              {
                  // empty
                  return ret;
              }
              else
              {
                  ret.left = std::max(left, rhs.left);
                  ret.right = std::min(right, rhs.right);
                  ret.top = std::max(top, rhs.top);
                  ret.bottom = std::min(bottom, rhs.bottom);
              }

              if (ret.left > ret.right || ret.top > ret.bottom)
              {
                  // no intersection, return empty
                  ret.left = ret.top = ret.right = ret.bottom = 0;
              }

              return ret;

          }
          [[nodiscard]] auto operator==(const TRect& rhs) const noexcept -> bool = default;
        };
        template<typename T>
        auto operator<<(std::ostream& o, const TRect<T>& r) -> std::ostream&
        {
            o << "TRect<>(l:" << r.left << ", t:" << r.top << ", r:" << r.right << ", b:" << r.bottom << ")";
            return o;
        }

        /** Structure used to define a rectangle in a 2-D floating point space.
        */
        using FloatRect = TRect<float>;

        /** Structure used to define a rectangle in a 2-D floating point space, 
            subject to double / single floating point settings.
        */
        using RealRect = TRect<Real>;

        /** Structure used to define a rectangle in a 2-D integer space.
        */
        using Rect = TRect<int32>;

        /** Structure used to define a box in a 3-D integer space.
            Note that the left, top, and front edges are included but the right,
            bottom and back ones are not.
         */
        struct Box
        {
            uint32 left, top, right, bottom, front, back;
            /// Parameterless constructor for setting the members manually
            Box()
                : left(0), top(0), right(1), bottom(1), front(0), back(1)
            {
            }
            /** Define a box from left, top, right and bottom coordinates
                This box will have depth one (front=0 and back=1).
                @param  l   x value of left edge
                @param  t   y value of top edge
                @param  r   x value of right edge
                @param  b   y value of bottom edge
                @note @copydetails Ogre::Box
            */
            Box( uint32 l, uint32 t, uint32 r, uint32 b ):
                left(l),
                top(t),   
                right(r),
                bottom(b),
                front(0),
                back(1)
            {
                assert(right >= left && bottom >= top && back >= front);
            }

            /// @overload
            template <typename T> explicit Box(const TRect<T>& r) : Box(r.left, r.top, r.right, r.bottom) {}

            /** Define a box from left, top, front, right, bottom and back
                coordinates.
                @param  l   x value of left edge
                @param  t   y value of top edge
                @param  ff  z value of front edge
                @param  r   x value of right edge
                @param  b   y value of bottom edge
                @param  bb  z value of back edge
                @note @copydetails Ogre::Box
            */
            Box( uint32 l, uint32 t, uint32 ff, uint32 r, uint32 b, uint32 bb ):
                left(l),
                top(t),   
                right(r),
                bottom(b),
                front(ff),
                back(bb)
            {
                assert(right >= left && bottom >= top && back >= front);
            }

            /// @overload
            explicit Box(const Vector<3, uint32>& size)
                : left(0), top(0), right(size[0]), bottom(size[1]), front(0), back(size[2])
            {
            }

            /// Return true if the other box is a part of this one
            [[nodiscard]] auto contains(const Box &def) const -> bool
            {
                return (def.left >= left && def.top >= top && def.front >= front &&
                    def.right <= right && def.bottom <= bottom && def.back <= back);
            }
            
            /// Get the width of this box
            [[nodiscard]] auto getWidth() const noexcept -> uint32 { return right-left; }
            /// Get the height of this box
            [[nodiscard]] auto getHeight() const noexcept -> uint32 { return bottom-top; }
            /// Get the depth of this box
            [[nodiscard]] auto getDepth() const noexcept -> uint32 { return back-front; }

            /// origin (top, left, front) of the box
            [[nodiscard]] auto getOrigin() const -> Vector<3, uint32> { return {left, top, front}; }
            /// size (width, height, depth) of the box
            [[nodiscard]] auto getSize() const -> Vector<3, uint32> { return {getWidth(), getHeight(), getDepth()}; }
        };

    
    
    /** Locate command-line options of the unary form '-blah' and of the
        binary form '-blah foo', passing back the index of the next non-option.
    @param numargs, argv The standard parameters passed to the main method
    @param unaryOptList Map of unary options (i.e. those that do not require a parameter).
        Should be pre-populated with, for example '-e' in the key and false in the 
        value. Options which are found will be set to true on return.
    @param binOptList Map of binary options (i.e. those that require a parameter
        e.g. '-e afile.txt').
        Should be pre-populated with, for example '-e' and the default setting. 
        Options which are found will have the value updated.
    */
    auto findCommandLineOpts(int numargs, char** argv, UnaryOptionList& unaryOptList,
        BinaryOptionList& binOptList) -> int;

    /// Generic result of clipping
    enum class ClipResult
    {
        /// Nothing was clipped
        NONE = 0,
        /// Partially clipped
        SOME = 1,
        /// Everything was clipped away
        ALL = 2
    };

    /// Render window creation parameters.
    struct RenderWindowDescription
    {
        std::string_view    name;
        unsigned int        width;
        unsigned int        height;
        bool                useFullScreen;
        NameValuePairList   miscParams;
    };

    /// Render window creation parameters container.
    using RenderWindowDescriptionList = std::vector<RenderWindowDescription>;

    /// Render window container.
    using RenderWindowList = std::vector<RenderWindow *>;

    /** @} */
    /** @} */
}

#endif
