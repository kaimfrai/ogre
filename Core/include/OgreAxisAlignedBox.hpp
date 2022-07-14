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
#ifndef OGRE_CORE_AXISALIGNEDBOX_H
#define OGRE_CORE_AXISALIGNEDBOX_H

#include <array>
#include <cassert>
#include <ostream>

// Precompiler options
#include "OgreMath.hpp"
#include "OgreMatrix4.hpp"
#include "OgrePrerequisites.hpp"
#include "OgreVector.hpp"

namespace Ogre {
class Plane;
class Sphere;
    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup Math
    *  @{
    */

    /** A 3D box aligned with the x/y/z axes.
    @remarks
    This class represents a simple box which is aligned with the
    axes. Internally it only stores 2 points as the extremeties of
    the box, one which is the minima of all 3 axes, and the other
    which is the maxima of all 3 axes. This class is typically used
    for an axis-aligned bounding box (AABB) for collision and
    visibility determination.
    */
    struct AxisAlignedBox
    {
        enum class Extent
        {
            Null,
            Finite,
            Infinite
        };

        Extent mExtent{Extent::Null};
        Vector3 mMinimum{-0.5, -0.5, -0.5};
        Vector3 mMaximum{0.5, 0.5, 0.5};

        /*
           1-------2
          /|      /|
         / |     / |
        5-------4  |
        |  0----|--3
        | /     | /
        |/      |/
        6-------7
        */
        enum class CornerEnum {
            FAR_LEFT_BOTTOM = 0,
            FAR_LEFT_TOP = 1,
            FAR_RIGHT_TOP = 2,
            FAR_RIGHT_BOTTOM = 3,
            NEAR_RIGHT_BOTTOM = 7,
            NEAR_LEFT_BOTTOM = 6,
            NEAR_LEFT_TOP = 5,
            NEAR_RIGHT_TOP = 4
        };
        using Corners = std::array<Vector3, 8>;

        /** Gets the minimum corner of the box.
        */
        [[nodiscard]] inline auto getMinimum() const noexcept -> const Vector3&
        { 
            return mMinimum; 
        }

        /** Gets a modifiable version of the minimum
        corner of the box.
        */
        inline auto getMinimum() noexcept -> Vector3&
        { 
            return mMinimum; 
        }

        /** Gets the maximum corner of the box.
        */
        [[nodiscard]] inline auto getMaximum() const noexcept -> const Vector3&
        { 
            return mMaximum;
        }

        /** Gets a modifiable version of the maximum
        corner of the box.
        */
        inline auto getMaximum() noexcept -> Vector3&
        { 
            return mMaximum;
        }


        /** Sets the minimum corner of the box.
        */
        inline void setMinimum( const Vector3& vec )
        {
            mExtent = Extent::Finite;
            mMinimum = vec;
        }

        inline void setMinimum( Real x, Real y, Real z )
        {
            mExtent = Extent::Finite;
            mMinimum.x = x;
            mMinimum.y = y;
            mMinimum.z = z;
        }

        /** Changes one of the components of the minimum corner of the box
        used to resize only one dimension of the box
        */
        inline void setMinimumX(Real x)
        {
            mMinimum.x = x;
        }

        inline void setMinimumY(Real y)
        {
            mMinimum.y = y;
        }

        inline void setMinimumZ(Real z)
        {
            mMinimum.z = z;
        }

        /** Sets the maximum corner of the box.
        */
        inline void setMaximum( const Vector3& vec )
        {
            mExtent = Extent::Finite;
            mMaximum = vec;
        }

        inline void setMaximum( Real x, Real y, Real z )
        {
            mExtent = Extent::Finite;
            mMaximum.x = x;
            mMaximum.y = y;
            mMaximum.z = z;
        }

        /** Changes one of the components of the maximum corner of the box
        used to resize only one dimension of the box
        */
        inline void setMaximumX( Real x )
        {
            mMaximum.x = x;
        }

        inline void setMaximumY( Real y )
        {
            mMaximum.y = y;
        }

        inline void setMaximumZ( Real z )
        {
            mMaximum.z = z;
        }

        /** Sets both minimum and maximum extents at once.
        */
        inline void setExtents( const Vector3& min, const Vector3& max )
        {
            assert( (min.x <= max.x && min.y <= max.y && min.z <= max.z) &&
                "The minimum corner of the box must be less than or equal to maximum corner" );

            mExtent = Extent::Finite;
            mMinimum = min;
            mMaximum = max;
        }

        inline void setExtents(
            Real mx, Real my, Real mz,
            Real Mx, Real My, Real Mz )
        {
            assert( (mx <= Mx && my <= My && mz <= Mz) &&
                "The minimum corner of the box must be less than or equal to maximum corner" );

            mExtent = Extent::Finite;

            mMinimum.x = mx;
            mMinimum.y = my;
            mMinimum.z = mz;

            mMaximum.x = Mx;
            mMaximum.y = My;
            mMaximum.z = Mz;

        }

        /** Returns a pointer to an array of 8 corner points, useful for
        collision vs. non-aligned objects.
        @remarks
        If the order of these corners is important, they are as
        follows: The 4 points of the minimum Z face (note that
        because Ogre uses right-handed coordinates, the minimum Z is
        at the 'back' of the box) starting with the minimum point of
        all, then anticlockwise around this face (if you are looking
        onto the face from outside the box). Then the 4 points of the
        maximum Z face, starting with maximum point of all, then
        anticlockwise around this face (looking onto the face from
        outside the box). Like this:
        <pre>
           1-------2
          /|      /|
         / |     / |
        5-------4  |
        |  0----|--3
        | /     | /
        |/      |/
        6-------7
        </pre>
        */
        [[nodiscard]] inline auto getAllCorners() const -> Corners
        {
            assert( (mExtent == Extent::Finite) && "Can't get corners of a null or infinite AAB" );

            // The order of these items is, using right-handed co-ordinates:
            // Minimum Z face, starting with Min(all), then anticlockwise
            //   around face (looking onto the face)
            // Maximum Z face, starting with Max(all), then anticlockwise
            //   around face (looking onto the face)
            // Only for optimization/compatibility.
            Corners corners;
            using enum CornerEnum;
            corners[0] = getCorner(FAR_LEFT_BOTTOM);
            corners[1] = getCorner(FAR_LEFT_TOP);
            corners[2] = getCorner(FAR_RIGHT_TOP);
            corners[3] = getCorner(FAR_RIGHT_BOTTOM);

            corners[4] = getCorner(NEAR_RIGHT_TOP);
            corners[5] = getCorner(NEAR_LEFT_TOP);
            corners[6] = getCorner(NEAR_LEFT_BOTTOM);
            corners[7] = getCorner(NEAR_RIGHT_BOTTOM);

            return corners;
        }

        /** Gets the position of one of the corners
        */
        [[nodiscard]] auto getCorner(CornerEnum cornerToGet) const -> Vector3
        {
            using enum CornerEnum;
            switch(cornerToGet)
            {
            case FAR_LEFT_BOTTOM:
                return mMinimum;
            case FAR_LEFT_TOP:
                return {mMinimum.x, mMaximum.y, mMinimum.z};
            case FAR_RIGHT_TOP:
                return {mMaximum.x, mMaximum.y, mMinimum.z};
            case FAR_RIGHT_BOTTOM:
                return {mMaximum.x, mMinimum.y, mMinimum.z};
            case NEAR_RIGHT_BOTTOM:
                return {mMaximum.x, mMinimum.y, mMaximum.z};
            case NEAR_LEFT_BOTTOM:
                return {mMinimum.x, mMinimum.y, mMaximum.z};
            case NEAR_LEFT_TOP:
                return {mMinimum.x, mMaximum.y, mMaximum.z};
            case NEAR_RIGHT_TOP:
                return mMaximum;
            default:
                return {};
            }
        }

        friend auto operator<<( std::ostream& o, const AxisAlignedBox &aab ) -> std::ostream&
        {
            using enum Extent;
            switch (aab.mExtent)
            {
            case Null:
                o << "AxisAlignedBox(null)";
                return o;

            case Finite:
                o << "AxisAlignedBox(min=" << aab.mMinimum << ", max=" << aab.mMaximum << ")";
                return o;

            case Infinite:
                o << "AxisAlignedBox(infinite)";
                return o;

            default: // shut up compiler
                assert( false && "Never reached" );
                return o;
            }
        }

        /** Merges the passed in box into the current box. The result is the
        box which encompasses both.
        */
        void merge( const AxisAlignedBox& rhs )
        {
            using enum Extent;
            // Do nothing if rhs null, or this is infinite
            if ((rhs.mExtent == Null) || (mExtent == Infinite))
            {
                return;
            }
            // Otherwise if rhs is infinite, make this infinite, too
            else if (rhs.mExtent == Infinite)
            {
                mExtent = Infinite;
            }
            // Otherwise if current null, just take rhs
            else if (mExtent == Null)
            {
                setExtents(rhs.mMinimum, rhs.mMaximum);
            }
            // Otherwise merge
            else
            {
                Vector3 min = mMinimum;
                Vector3 max = mMaximum;
                max.makeCeil(rhs.mMaximum);
                min.makeFloor(rhs.mMinimum);

                setExtents(min, max);
            }

        }

        /** Extends the box to encompass the specified point (if needed).
        */
        inline void merge( const Vector3& point )
        {
            using enum Extent;
            switch (mExtent)
            {
            case Null: // if null, use this point
                setExtents(point, point);
                return;

            case Finite:
                mMaximum.makeCeil(point);
                mMinimum.makeFloor(point);
                return;

            case Infinite: // if infinite, makes no difference
                return;
            }

            assert( false && "Never reached" );
        }

        /** Transforms the box according to the matrix supplied.
        @remarks
        By calling this method you get the axis-aligned box which
        surrounds the transformed version of this box. Therefore each
        corner of the box is transformed by the matrix, then the
        extents are mapped back onto the axes to produce another
        AABB. Useful when you have a local AABB for an object which
        is then transformed.
        */
        inline void transform( const Matrix4& matrix )
        {
            // Do nothing if current null or infinite
            if( mExtent != Extent::Finite )
                return;

            Vector3 oldMin, oldMax, currentCorner;

            // Getting the old values so that we can use the existing merge method.
            oldMin = mMinimum;
            oldMax = mMaximum;

            // reset
            setNull();

            // We sequentially compute the corners in the following order :
            // 0, 6, 5, 1, 2, 4 ,7 , 3
            // This sequence allows us to only change one member at a time to get at all corners.

            // For each one, we transform it using the matrix
            // Which gives the resulting point and merge the resulting point.

            // First corner 
            // min min min
            currentCorner = oldMin;
            merge( matrix * currentCorner );

            // min,min,max
            currentCorner.z = oldMax.z;
            merge( matrix * currentCorner );

            // min max max
            currentCorner.y = oldMax.y;
            merge( matrix * currentCorner );

            // min max min
            currentCorner.z = oldMin.z;
            merge( matrix * currentCorner );

            // max max min
            currentCorner.x = oldMax.x;
            merge( matrix * currentCorner );

            // max max max
            currentCorner.z = oldMax.z;
            merge( matrix * currentCorner );

            // max min max
            currentCorner.y = oldMin.y;
            merge( matrix * currentCorner );

            // max min min
            currentCorner.z = oldMin.z;
            merge( matrix * currentCorner ); 
        }

        /** Transforms the box according to the affine matrix supplied.
        @remarks
        By calling this method you get the axis-aligned box which
        surrounds the transformed version of this box. Therefore each
        corner of the box is transformed by the matrix, then the
        extents are mapped back onto the axes to produce another
        AABB. Useful when you have a local AABB for an object which
        is then transformed.
        */
        void transform(const Affine3& m)
        {
            // Do nothing if current null or infinite
            if ( mExtent != Extent::Finite )
                return;

            Vector3 centre = getCenter();
            Vector3 halfSize = getHalfSize();

            Vector3 newCentre = m * centre;
            Vector3 newHalfSize{
                Math::Abs(m[0][0]) * halfSize.x + Math::Abs(m[0][1]) * halfSize.y + Math::Abs(m[0][2]) * halfSize.z, 
                Math::Abs(m[1][0]) * halfSize.x + Math::Abs(m[1][1]) * halfSize.y + Math::Abs(m[1][2]) * halfSize.z,
                Math::Abs(m[2][0]) * halfSize.x + Math::Abs(m[2][1]) * halfSize.y + Math::Abs(m[2][2]) * halfSize.z};

            setExtents(newCentre - newHalfSize, newCentre + newHalfSize);
        }

        /** Sets the box to a 'null' value i.e. not a box.
        */
        inline void setNull()
        {
            mExtent = Extent::Null;
        }

        /** Returns true if the box is null i.e. empty.
        */
        [[nodiscard]] inline auto isNull() const noexcept -> bool
        {
            return (mExtent == Extent::Null);
        }

        /** Returns true if the box is finite.
        */
        [[nodiscard]] auto isFinite() const noexcept -> bool
        {
            return (mExtent == Extent::Finite);
        }

        /** Sets the box to 'infinite'
        */
        inline void setInfinite()
        {
            mExtent = Extent::Infinite;
        }

        /** Returns true if the box is infinite.
        */
        [[nodiscard]] auto isInfinite() const noexcept -> bool
        {
            return (mExtent == Extent::Infinite);
        }

        /** Returns whether or not this box intersects another. */
        [[nodiscard]] inline auto intersects(const AxisAlignedBox& b2) const -> bool
        {
            // Early-fail for nulls
            if (this->isNull() || b2.isNull())
                return false;

            // Early-success for infinites
            if (this->isInfinite() || b2.isInfinite())
                return true;

            // Use up to 6 separating planes
            if (mMaximum.x < b2.mMinimum.x)
                return false;
            if (mMaximum.y < b2.mMinimum.y)
                return false;
            if (mMaximum.z < b2.mMinimum.z)
                return false;

            if (mMinimum.x > b2.mMaximum.x)
                return false;
            if (mMinimum.y > b2.mMaximum.y)
                return false;
            if (mMinimum.z > b2.mMaximum.z)
                return false;

            // otherwise, must be intersecting
            return true;

        }

        /// Calculate the area of intersection of this box and another
        [[nodiscard]] inline auto intersection(const AxisAlignedBox& b2) const -> AxisAlignedBox
        {
            if (this->isNull() || b2.isNull())
            {
                return {};
            }
            else if (this->isInfinite())
            {
                return b2;
            }
            else if (b2.isInfinite())
            {
                return *this;
            }

            Vector3 intMin = mMinimum;
            Vector3 intMax = mMaximum;

            intMin.makeCeil(b2.getMinimum());
            intMax.makeFloor(b2.getMaximum());

            // Check intersection isn't null
            if (intMin.x < intMax.x &&
                intMin.y < intMax.y &&
                intMin.z < intMax.z)
            {
                return { AxisAlignedBox::Extent::Finite, intMin, intMax };
            }

            return {};
        }

        /// Calculate the volume of this box
        [[nodiscard]] auto volume() const -> Real
        {
            using enum Extent;
            switch (mExtent)
            {
            case Null:
                return 0.0f;

            case Finite:
                {
                    Vector3 diff = mMaximum - mMinimum;
                    return diff.x * diff.y * diff.z;
                }

            case Infinite:
                return Math::POS_INFINITY;

            default: // shut up compiler
                assert( false && "Never reached" );
                return 0.0f;
            }
        }

        /** Scales the AABB by the vector given. */
        inline void scale(const Vector3& s)
        {
            // Do nothing if current null or infinite
            if (mExtent != Extent::Finite)
                return;

            // NB assumes centered on origin
            Vector3 min = mMinimum * s;
            Vector3 max = mMaximum * s;
            setExtents(min, max);
        }

        /** Tests whether this box intersects a sphere. */
        [[nodiscard]] auto intersects(const Sphere& s) const -> bool
        {
            return Math::intersects(s, *this); 
        }
        /** Tests whether this box intersects a plane. */
        [[nodiscard]] auto intersects(const Plane& p) const -> bool
        {
            return Math::intersects(p, *this);
        }
        /** Tests whether the vector point is within this box. */
        [[nodiscard]] auto intersects(const Vector3& v) const -> bool
        {
            using enum Extent;
            switch (mExtent)
            {
            case Null:
                return false;

            case Finite:
                return(v.x >= mMinimum.x  &&  v.x <= mMaximum.x  && 
                    v.y >= mMinimum.y  &&  v.y <= mMaximum.y  && 
                    v.z >= mMinimum.z  &&  v.z <= mMaximum.z);

            case Infinite:
                return true;

            default: // shut up compiler
                assert( false && "Never reached" );
                return false;
            }
        }
        /// Gets the centre of the box
        [[nodiscard]] auto getCenter() const -> Vector3
        {
            assert( (mExtent == Extent::Finite) && "Can't get center of a null or infinite AAB" );

            return {
                (mMaximum.x + mMinimum.x) * 0.5f,
                (mMaximum.y + mMinimum.y) * 0.5f,
                (mMaximum.z + mMinimum.z) * 0.5f};
        }
        /// Gets the size of the box
        [[nodiscard]] auto getSize() const -> Vector3
        {
            using enum Extent;
            switch (mExtent)
            {
            case Null:
                return Vector3::ZERO;

            case Finite:
                return mMaximum - mMinimum;

            case Infinite:
                return {
                    Math::POS_INFINITY,
                    Math::POS_INFINITY,
                    Math::POS_INFINITY};

            default: // shut up compiler
                assert( false && "Never reached" );
                return Vector3::ZERO;
            }
        }
        /// Gets the half-size of the box
        [[nodiscard]] auto getHalfSize() const -> Vector3
        {
            using enum Extent;
            switch ( mExtent)
            {
            case Null:
                return Vector3::ZERO;

            case Finite:
                return (mMaximum - mMinimum) * 0.5;

            case Infinite:
                return {
                    Math::POS_INFINITY,
                    Math::POS_INFINITY,
                    Math::POS_INFINITY};

            default: // shut up compiler
                assert( false && "Never reached" );
                return Vector3::ZERO;
            }
        }

        /** Tests whether the given point contained by this box.
        */
        [[nodiscard]] auto contains(const Vector3& v) const -> bool
        {
            if (isNull())
                return false;
            if (isInfinite())
                return true;

            return mMinimum.x <= v.x && v.x <= mMaximum.x &&
                   mMinimum.y <= v.y && v.y <= mMaximum.y &&
                   mMinimum.z <= v.z && v.z <= mMaximum.z;
        }
        
        /** Returns the squared minimum distance between a given point and any part of the box.
         *  This is faster than distance since avoiding a squareroot, so use if you can. */
        [[nodiscard]] auto squaredDistance(const Vector3& v) const -> Real
        {

            if (this->contains(v))
                return 0;
            else
            {
                Vector3 maxDist{0,0,0};

                if (v.x < mMinimum.x)
                    maxDist.x = mMinimum.x - v.x;
                else if (v.x > mMaximum.x)
                    maxDist.x = v.x - mMaximum.x;

                if (v.y < mMinimum.y)
                    maxDist.y = mMinimum.y - v.y;
                else if (v.y > mMaximum.y)
                    maxDist.y = v.y - mMaximum.y;

                if (v.z < mMinimum.z)
                    maxDist.z = mMinimum.z - v.z;
                else if (v.z > mMaximum.z)
                    maxDist.z = v.z - mMaximum.z;

                return maxDist.squaredLength();
            }
        }
        
        /** Returns the minimum distance between a given point and any part of the box. */
        [[nodiscard]] auto distance (const Vector3& v) const -> Real
        {
            return Ogre::Math::Sqrt(squaredDistance(v));
        }

        /** Tests whether another box contained by this box.
        */
        [[nodiscard]] auto contains(const AxisAlignedBox& other) const -> bool
        {
            if (other.isNull() || this->isInfinite())
                return true;

            if (this->isNull() || other.isInfinite())
                return false;

            return this->mMinimum.x <= other.mMinimum.x &&
                   this->mMinimum.y <= other.mMinimum.y &&
                   this->mMinimum.z <= other.mMinimum.z &&
                   other.mMaximum.x <= this->mMaximum.x &&
                   other.mMaximum.y <= this->mMaximum.y &&
                   other.mMaximum.z <= this->mMaximum.z;
        }

        /** Tests 2 boxes for equality.
        */
        [[nodiscard]] auto operator== (const AxisAlignedBox& rhs) const noexcept -> bool
        {
            if (this->mExtent != rhs.mExtent)
                return false;

            if (!this->isFinite())
                return true;

            return this->mMinimum == rhs.mMinimum &&
                   this->mMaximum == rhs.mMaximum;
        }

        // special values
        static const AxisAlignedBox BOX_NULL;
        static const AxisAlignedBox BOX_INFINITE;


    };

    /** @} */
    /** @} */
} // namespace Ogre

#endif
