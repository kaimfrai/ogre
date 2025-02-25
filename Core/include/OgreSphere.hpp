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
export module Ogre.Core:Sphere;

// Precompiler options
export import :Plane;
export import :Prerequisites;
export import :Vector;

export
namespace Ogre {


    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup Math
    *  @{
    */
    /** A sphere primitive, mostly used for bounds checking. 
    @remarks
        A sphere in math texts is normally represented by the function
        x^2 + y^2 + z^2 = r^2 (for sphere's centered on the origin). Ogre stores spheres
        simply as a center point and a radius.
    */
    struct Sphere
    {
        Vector3 mCenter{Vector3::ZERO};
        Real mRadius{1.0};

        /** Returns the radius of the sphere. */
        [[nodiscard]] auto getRadius() const noexcept -> Real { return mRadius; }

        /** Sets the radius of the sphere. */
        void setRadius(Real radius) { mRadius = radius; }

        /** Returns the center point of the sphere. */
        [[nodiscard]] auto getCenter() const noexcept -> const Vector3& { return mCenter; }

        /** Sets the center point of the sphere. */
        void setCenter(const Vector3& center) { mCenter = center; }

        /** Returns whether or not this sphere intersects another sphere. */
        [[nodiscard]] auto intersects(const Sphere& s) const -> bool
        {
            return (s.mCenter - mCenter).squaredLength() <=
                Math::Sqr(s.mRadius + mRadius);
        }
        /** Returns whether or not this sphere intersects a box. */
        [[nodiscard]] auto intersects(const AxisAlignedBox& box) const -> bool
        {
            return Math::intersects(*this, box);
        }
        /** Returns whether or not this sphere intersects a plane. */
        [[nodiscard]] auto intersects(const Plane& plane) const -> bool
        {
            return Math::Abs(plane.getDistance(getCenter())) <= getRadius();
        }
        /** Returns whether or not this sphere intersects a point. */
        [[nodiscard]] auto intersects(const Vector3& v) const -> bool
        {
            return ((v - mCenter).squaredLength() <= Math::Sqr(mRadius));
        }
        /** Merges another Sphere into the current sphere */
        void merge(const Sphere& oth)
        {
            Vector3 diff = oth.getCenter() - mCenter;
            Real lengthSq = diff.squaredLength();
            Real radiusDiff = oth.getRadius() - mRadius;
            
            // Early-out
            if (Math::Sqr(radiusDiff) >= lengthSq) 
            {
                // One fully contains the other
                if (radiusDiff <= 0.0f) 
                    return; // no change
                else 
                {
                    mCenter = oth.getCenter();
                    mRadius = oth.getRadius();
                    return;
                }
            }

            Real length = Math::Sqrt(lengthSq);
            Real t = (length + radiusDiff) / (2.0f * length);
            mCenter = mCenter + diff * t;
            mRadius = 0.5f * (length + mRadius + oth.getRadius());
        }
    };

    inline auto Math::intersects(const Sphere& sphere, const Plane& plane) -> bool
    {
        return sphere.intersects(plane);
    }
    /** @} */
    /** @} */

}
static_assert(std::is_aggregate_v<Ogre::Sphere>);
static_assert(std::is_standard_layout_v<Ogre::Sphere>);
