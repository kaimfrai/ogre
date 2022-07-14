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


#ifndef OGRE_CORE_QUATERNION_H
#define OGRE_CORE_QUATERNION_H

#include <cassert>
#include <cmath>
#include <cstring>
#include <ostream>
#include <utility>

#include "OgreMath.hpp"
#include "OgrePrerequisites.hpp"

namespace Ogre {
struct Matrix3;

    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup Math
    *  @{
    */
    /** Implementation of a Quaternion, i.e. a rotation around an axis.
        For more information about Quaternions and the theory behind it, we recommend reading:
        http://www.ogre3d.org/tikiwiki/Quaternion+and+Rotation+Primer and
        http://www.cprogramming.com/tutorial/3d/quaternions.html and
        http://www.gamedev.net/page/resources/_/reference/programming/math-and-physics/quaternions/quaternion-powers-r1095
    */
    class Quaternion
    {
    public:
        /// Default constructor, initializes to identity rotation (aka 0°)
        inline Quaternion ()
            : w(1), x(0), y(0), z(0)
        {
        }
        /// Copy constructor
        inline Quaternion(const Ogre::Quaternion& rhs)
             
        = default;
        /// Construct from an explicit list of values
        inline Quaternion (
            float fW,
            float fX, float fY, float fZ)
            : w(fW), x(fX), y(fY), z(fZ)
        {
        }
        /// Construct a quaternion from a rotation matrix
        inline Quaternion(const Matrix3& rot)
        {
            this->FromRotationMatrix(rot);
        }
        /// Construct a quaternion from an angle/axis
        inline Quaternion(const Radian& rfAngle, const Vector3& rkAxis)
        {
            this->FromAngleAxis(rfAngle, rkAxis);
        }
        /// Construct a quaternion from 3 orthonormal local axes
        inline Quaternion(const Vector3& xaxis, const Vector3& yaxis, const Vector3& zaxis)
        {
            this->FromAxes(xaxis, yaxis, zaxis);
        }
        /// Construct a quaternion from 3 orthonormal local axes
        inline Quaternion(const Vector3* akAxis)
        {
            this->FromAxes(akAxis);
        }
        /// Construct a quaternion from 4 manual w/x/y/z values
        inline Quaternion(float* valptr)
        {
            memcpy(&w, valptr, sizeof(float)*4);
        }

        /** Exchange the contents of this quaternion with another. 
        */
        inline void swap(Quaternion& other)
        {
            std::swap(w, other.w);
            std::swap(x, other.x);
            std::swap(y, other.y);
            std::swap(z, other.z);
        }

        /// Array accessor operator
        inline auto operator [] ( const size_t i ) const -> float
        {
            assert( i < 4 );

            return *(&w+i);
        }

        /// Array accessor operator
        inline auto operator [] ( const size_t i ) -> float&
        {
            assert( i < 4 );

            return *(&w+i);
        }

        /// Pointer accessor for direct copying
        inline auto ptr() -> float*
        {
            return &w;
        }

        /// Pointer accessor for direct copying
        [[nodiscard]] inline auto ptr() const -> const float*
        {
            return &w;
        }

        void FromRotationMatrix (const Matrix3& kRot);
        void ToRotationMatrix (Matrix3& kRot) const;
        /** Setups the quaternion using the supplied vector, and "roll" around
            that vector by the specified radians.
        */
        void FromAngleAxis (const Radian& rfAngle, const Vector3& rkAxis);
        void ToAngleAxis (Radian& rfAngle, Vector3& rkAxis) const;
        inline void ToAngleAxis (Degree& dAngle, Vector3& rkAxis) const {
            Radian rAngle;
            ToAngleAxis ( rAngle, rkAxis );
            dAngle = rAngle;
        }
        /** Constructs the quaternion using 3 axes, the axes are assumed to be orthonormal
            @see FromAxes
        */
        void FromAxes (const Vector3* akAxis);
        void FromAxes (const Vector3& xAxis, const Vector3& yAxis, const Vector3& zAxis);
        /** Gets the 3 orthonormal axes defining the quaternion. @see FromAxes */
        void ToAxes (Vector3* akAxis) const;
        void ToAxes (Vector3& xAxis, Vector3& yAxis, Vector3& zAxis) const;

        /** Returns the X orthonormal axis defining the quaternion. Same as doing
            xAxis = Vector3::UNIT_X * this. Also called the local X-axis
        */
        [[nodiscard]] auto xAxis() const -> Vector3;

        /** Returns the Y orthonormal axis defining the quaternion. Same as doing
            yAxis = Vector3::UNIT_Y * this. Also called the local Y-axis
        */
        [[nodiscard]] auto yAxis() const -> Vector3;

        /** Returns the Z orthonormal axis defining the quaternion. Same as doing
            zAxis = Vector3::UNIT_Z * this. Also called the local Z-axis
        */
        [[nodiscard]] auto zAxis() const -> Vector3;

        inline auto operator= (const Quaternion& rkQ) -> Quaternion&
        = default;
        auto operator+ (const Quaternion& rkQ) const -> Quaternion;
        auto operator- (const Quaternion& rkQ) const -> Quaternion;
        auto operator*(const Quaternion& rkQ) const -> Quaternion;
        auto operator*(float s) const -> Quaternion
        {
            return {s * w, s * x, s * y, s * z};
        }
        friend auto operator*(float s, const Quaternion& q) -> Quaternion
        {
            return q * s;
        }
        auto operator-() const -> Quaternion { return {-w, -x, -y, -z}; }

        [[nodiscard]] inline auto operator== (const Quaternion& rhs) const noexcept -> bool = default;

        // functions of a quaternion
        /// Returns the dot product of the quaternion
        [[nodiscard]] auto Dot(const Quaternion& rkQ) const -> float
        {
            return w * rkQ.w + x * rkQ.x + y * rkQ.y + z * rkQ.z;
        }
        /// Returns the normal length of this quaternion.
        [[nodiscard]] auto Norm() const -> float { return std::sqrt(w * w + x * x + y * y + z * z); }
        /// Normalises this quaternion, and returns the previous length
        auto normalise() -> float
        {
            float len = Norm();
            *this = 1.0f / len * *this;
            return len;
        }
        [[nodiscard]] auto Inverse () const -> Quaternion;  /// Apply to non-zero quaternion
        [[nodiscard]] auto UnitInverse () const -> Quaternion;  /// Apply to unit-length quaternion
        [[nodiscard]] auto Exp () const -> Quaternion;
        [[nodiscard]] auto Log () const -> Quaternion;

        /// Rotation of a vector by a quaternion
        auto operator* (const Vector3& rkVector) const -> Vector3;

        /** Calculate the local roll element of this quaternion.
        @param reprojectAxis By default the method returns the 'intuitive' result
            that is, if you projected the local X of the quaternion onto the XY plane,
            the angle between it and global X is returned. The co-domain of the returned
            value is from -180 to 180 degrees. If set to false though, the result is
            the rotation around Z axis that could be used to implement the quaternion
            using some non-intuitive order of rotations. This behavior is preserved for
            backward compatibility, to decompose quaternion into yaw, pitch and roll use
            q.ToRotationMatrix().ToEulerAnglesYXZ(yaw, pitch, roll) instead.
        */
        [[nodiscard]] auto getRoll(bool reprojectAxis = true) const -> Radian;
        /** Calculate the local pitch element of this quaternion
        @param reprojectAxis By default the method returns the 'intuitive' result
            that is, if you projected the local Y of the quaternion onto the YZ plane,
            the angle between it and global Y is returned. The co-domain of the returned
            value is from -180 to 180 degrees. If set to false though, the result is
            the rotation around X axis that could be used to implement the quaternion
            using some non-intuitive order of rotations. This behavior is preserved for
            backward compatibility, to decompose quaternion into yaw, pitch and roll use
            q.ToRotationMatrix().ToEulerAnglesYXZ(yaw, pitch, roll) instead.
        */
        [[nodiscard]] auto getPitch(bool reprojectAxis = true) const -> Radian;
        /** Calculate the local yaw element of this quaternion
        @param reprojectAxis By default the method returns the 'intuitive' result
            that is, if you projected the local Z of the quaternion onto the ZX plane,
            the angle between it and global Z is returned. The co-domain of the returned
            value is from -180 to 180 degrees. If set to false though, the result is
            the rotation around Y axis that could be used to implement the quaternion 
            using some non-intuitive order of rotations. This behavior is preserved for
            backward compatibility, to decompose quaternion into yaw, pitch and roll use
            q.ToRotationMatrix().ToEulerAnglesYXZ(yaw, pitch, roll) instead.
        */
        [[nodiscard]] auto getYaw(bool reprojectAxis = true) const -> Radian;
        
        /** Equality with tolerance (tolerance is max angle difference)
        @remark Both equals() and orientationEquals() measure the exact same thing.
                One measures the difference by angle, the other by a different, non-linear metric.
        */
        [[nodiscard]] auto equals(const Quaternion& rhs, const Radian& tolerance) const -> bool
        {
            float d = Dot(rhs);
            Radian angle = Math::ACos(2.0f * d*d - 1.0f);

            return Math::Abs(angle.valueRadians()) <= tolerance.valueRadians();
        }
        
        /** Compare two quaternions which are assumed to be used as orientations.
        @remark Both equals() and orientationEquals() measure the exact same thing.
                One measures the difference by angle, the other by a different, non-linear metric.
        @return true if the two orientations are the same or very close, relative to the given tolerance.
            Slerp ( 0.75f, A, B ) != Slerp ( 0.25f, B, A );
            therefore be careful if your code relies in the order of the operands.
            This is specially important in IK animation.
        */
        [[nodiscard]] inline auto orientationEquals( const Quaternion& other, float tolerance = 1e-3f ) const -> bool
        {
            float d = this->Dot(other);
            return 1 - d*d < tolerance;
        }
        
        /** Performs Spherical linear interpolation between two quaternions, and returns the result.
            Slerp ( 0.0f, A, B ) = A
            Slerp ( 1.0f, A, B ) = B
            @return Interpolated quaternion
            @remarks
            Slerp has the proprieties of performing the interpolation at constant
            velocity, and being torque-minimal (unless shortestPath=false).
            However, it's NOT commutative, which means
            Slerp ( 0.75f, A, B ) != Slerp ( 0.25f, B, A );
            therefore be careful if your code relies in the order of the operands.
            This is specially important in IK animation.
        */
        static auto Slerp (Real fT, const Quaternion& rkP,
            const Quaternion& rkQ, bool shortestPath = false) -> Quaternion;

        /** @see Slerp. It adds extra "spins" (i.e. rotates several times) specified
            by parameter 'iExtraSpins' while interpolating before arriving to the
            final values
        */
        static auto SlerpExtraSpins (Real fT,
            const Quaternion& rkP, const Quaternion& rkQ,
            int iExtraSpins) -> Quaternion;

        /// Setup for spherical quadratic interpolation
        static void Intermediate (const Quaternion& rkQ0,
            const Quaternion& rkQ1, const Quaternion& rkQ2,
            Quaternion& rka, Quaternion& rkB);

        /// Spherical quadratic interpolation
        static auto Squad (Real fT, const Quaternion& rkP,
            const Quaternion& rkA, const Quaternion& rkB,
            const Quaternion& rkQ, bool shortestPath = false) -> Quaternion;

        /** Performs Normalised linear interpolation between two quaternions, and returns the result.
            nlerp ( 0.0f, A, B ) = A
            nlerp ( 1.0f, A, B ) = B
            @remarks
            Nlerp is faster than Slerp.
            Nlerp has the proprieties of being commutative (@see Slerp;
            commutativity is desired in certain places, like IK animation), and
            being torque-minimal (unless shortestPath=false). However, it's performing
            the interpolation at non-constant velocity; sometimes this is desired,
            sometimes it is not. Having a non-constant velocity can produce a more
            natural rotation feeling without the need of tweaking the weights; however
            if your scene relies on the timing of the rotation or assumes it will point
            at a specific angle at a specific weight value, Slerp is a better choice.
        */
        static auto nlerp(Real fT, const Quaternion& rkP, 
            const Quaternion& rkQ, bool shortestPath = false) -> Quaternion;

        /// Cutoff for sine near zero
        static const float msEpsilon;

        // special values
        static const Quaternion ZERO;
        static const Quaternion IDENTITY;

        float w, x, y, z;

        /// Check whether this quaternion contains valid values
        [[nodiscard]] inline auto isNaN() const noexcept -> bool
        {
            return Math::isNaN(x) || Math::isNaN(y) || Math::isNaN(z) || Math::isNaN(w);
        }

        /** Function for writing to a stream. Outputs "Quaternion(w, x, y, z)" with w,x,y,z
            being the member values of the quaternion.
        */
        inline friend auto operator <<
            ( std::ostream& o, const Quaternion& q ) -> std::ostream&
        {
            o << "Quaternion(" << q.w << ", " << q.x << ", " << q.y << ", " << q.z << ")";
            return o;
        }

    };
    /** @} */
    /** @} */

}




#endif 
