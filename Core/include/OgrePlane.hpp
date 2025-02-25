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
// This file is based on material originally from:
// Geometric Tools, LLC
// Copyright (c) 1998-2010
// Distributed under the Boost Software License, Version 1.0.
// http://www.boost.org/LICENSE_1_0.txt
// http://www.geometrictools.com/License/Boost/LICENSE_1_0.txt
export module Ogre.Core:Plane;

export import :AxisAlignedBox;
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
    /** Defines a plane in 3D space.
        @remarks
            A plane is defined in 3D space by the equation
            Ax + By + Cz + D = 0
        @par
            This equates to a vector (the normal of the plane, whose x, y
            and z components equate to the coefficients A, B and C
            respectively), and a constant (D) which is the distance along
            the normal you have to go to move the plane back to the origin.
     */
    struct Plane
    {
        Vector3 normal{Vector3::ZERO};
        Real d{0.0f};

        [[nodiscard]]
        static auto constexpr Redefine(const Vector3& rkNormal, const Vector3& rkPoint) -> Plane
        {
            Plane plane;
            plane.redefine(rkNormal, rkPoint);
            return plane;
        }

        [[nodiscard]]
        static auto constexpr Redefine(const Vector3& p0, const Vector3& p1, const Vector3& p2) -> Plane
        {
            Plane plane;
            plane.redefine(p0, p1, p2);
            return plane;
        }

        /** The "positive side" of the plane is the half space to which the
            plane normal points. The "negative side" is the other half
            space. The flag "no side" indicates the plane itself.
        */
        enum class Side
        {
            NONE,
            Positive,
            Negative,
            Both
        };

        [[nodiscard]] auto getSide(const Vector3& rkPoint) const -> Side
        {
            using enum Side;
            Real fDistance = getDistance(rkPoint);

            if (fDistance < 0.0)
                return Negative;

            if (fDistance > 0.0)
                return Positive;

            return NONE;
        }

        /**
        Returns the side where the alignedBox is. The flag Side::Both indicates an intersecting box.
        One corner ON the plane is sufficient to consider the box and the plane intersecting.
        */
        [[nodiscard]] auto getSide(const AxisAlignedBox& box) const -> Side
        {
            if (box.isNull())
                return Side::NONE;
            if (box.isInfinite())
                return Side::Both;

            return getSide(box.getCenter(), box.getHalfSize());
        }

        /** Returns which side of the plane that the given box lies on.
            The box is defined as centre/half-size pairs for effectively.
        @param centre The centre of the box.
        @param halfSize The half-size of the box.
        @return
            Side::Positive if the box complete lies on the "positive side" of the plane,
            Side::Negative if the box complete lies on the "negative side" of the plane,
            and Side::Both if the box intersects the plane.
        */
        [[nodiscard]] auto getSide(const Vector3& centre, const Vector3& halfSize) const -> Side
        {
            // Calculate the distance between box centre and the plane
            Real dist = getDistance(centre);

            // Calculate the maximise allows absolute distance for
            // the distance between box centre and plane
            Real maxAbsDist = normal.absDotProduct(halfSize);

            if (dist < -maxAbsDist)
                return Side::Negative;

            if (dist > +maxAbsDist)
                return Side::Positive;

            return Side::Both;
        }

        /** This is a pseudodistance. The sign of the return value is
            positive if the point is on the positive side of the plane,
            negative if the point is on the negative side, and zero if the
            point is on the plane.
            @par
            The absolute value of the return value is the true distance only
            when the plane normal is a unit length vector.
        */
        [[nodiscard]] auto getDistance(const Vector3& rkPoint) const -> Real
        {
            return normal.dotProduct(rkPoint) + d;
        }

        /** Redefine this plane based on 3 points. */
        void redefine(const Vector3& p0, const Vector3& p1, const Vector3& p2)
        {
            normal = Math::calculateBasicFaceNormal(p0, p1, p2);
            d = -normal.dotProduct(p0);
        }

        /** Redefine this plane based on a normal and a point. */
        void redefine(const Vector3& rkNormal, const Vector3& rkPoint)
        {
            normal = rkNormal;
            d = -rkNormal.dotProduct(rkPoint);
        }

        /** Project a vector onto the plane. 
        @remarks This gives you the element of the input vector that is perpendicular 
            to the normal of the plane. You can get the element which is parallel
            to the normal of the plane by subtracting the result of this method
            from the original vector, since parallel + perpendicular = original.
        @param v The input vector
        */
        [[nodiscard]] auto projectVector(const Vector3& v) const -> Vector3
        {
            // We know plane normal is unit length, so use simple method
            Matrix3 xform;
            xform[0][0] = 1.0f - normal.x * normal.x;
            xform[0][1] = -normal.x * normal.y;
            xform[0][2] = -normal.x * normal.z;
            xform[1][0] = -normal.y * normal.x;
            xform[1][1] = 1.0f - normal.y * normal.y;
            xform[1][2] = -normal.y * normal.z;
            xform[2][0] = -normal.z * normal.x;
            xform[2][1] = -normal.z * normal.y;
            xform[2][2] = 1.0f - normal.z * normal.z;
            return xform * v;
        }

        /** Normalises the plane.
            @remarks
                This method normalises the plane's normal and the length scale of d
                is as well.
            @note
                This function will not crash for zero-sized vectors, but there
                will be no changes made to their components.
            @return The previous length of the plane's normal.
        */
        auto normalise() -> Real
        {
            Real fLength = normal.length();

            // Will also work for zero-sized vectors, but will change nothing
            // We're not using epsilons because we don't need to.
            // Read http://www.ogre3d.org/forums/viewtopic.php?f=4&t=61259
            if (fLength > Real(0.0f))
            {
                Real fInvLength = 1.0f / fLength;
                normal *= fInvLength;
                d *= fInvLength;
            }

            return fLength;
        }

        /// Get flipped plane, with same location but reverted orientation
        auto operator - () const -> Plane
        {
            return { {-(normal.x), -(normal.y), -(normal.z) }, -d}; // not equal to Plane{-normal, -d}
        }

        /// Comparison operator
        [[nodiscard]] auto operator==(const Plane& rhs) const noexcept -> bool = default;

        friend auto operator<<(std::ostream& o, const Plane& p) -> std::ostream&
        {
            o << "Plane{normal=" << p.normal << ", d=" << p.d << "}";
            return o;
        }
    };

    inline auto operator * (const Matrix4& mat, const Plane& p) -> Plane
    {
        Plane ret;
        Matrix4 invTrans = mat.inverse().transpose();
        Vector4 v4{ p.normal.x, p.normal.y, p.normal.z, p.d };
        v4 = invTrans * v4;
        ret.normal.x = v4.x;
        ret.normal.y = v4.y;
        ret.normal.z = v4.z;
        ret.d = v4.w / ret.normal.normalise();

        return ret;
    }

    inline auto Math::intersects(const Plane& plane, const AxisAlignedBox& box) -> bool
    {
        return plane.getSide(box) == Plane::Side::Both;
    }

    using PlaneList = std::vector<Plane>;
    /** @} */
    /** @} */

} // namespace Ogre
static_assert(std::is_aggregate_v<Ogre::Plane>);
static_assert(std::is_standard_layout_v<Ogre::Plane>);
