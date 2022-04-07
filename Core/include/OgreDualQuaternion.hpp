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

#include <cassert>

export module Ogre.Core:DualQuaternion;

export import :Math;
export import :Prerequisites;
export import :Quaternion;

export import <cstring>;
export import <ostream>;
export import <utility>;

export
namespace Ogre {
class Affine3;

    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup Math
    *  @{
    */
    /** Implementation of a dual quaternion, i.e. a rotation around an axis and a translation.
        This implementation may note be appropriate as a general implementation, but is intended for use with
        dual quaternion skinning.
    */
    class DualQuaternion
    {
    public:
        /// Default constructor, initializes to identity rotation (aka 0°), and zero translation (0,0,0)
        inline DualQuaternion ()
            : w(1), x(0), y(0), z(0), dw(1), dx(0), dy(0), dz(0)
        {
        }
        
        /// Construct from an explicit list of values
        inline DualQuaternion (Real fW, Real fX, Real fY, Real fZ, 
                Real fdW, Real fdX, Real fdY, Real fdZ)
            : w(fW), x(fX), y(fY), z(fZ), dw(fdW), dx(fdX), dy(fdY), dz(fdZ)
        {
        }
        
        /// Construct a dual quaternion from a transformation matrix
        inline DualQuaternion(const Affine3& rot)
        {
            this->fromTransformationMatrix(rot);
        }
        
        /// Construct a dual quaternion from a unit quaternion and a translation vector
        inline DualQuaternion(const Quaternion& q, const Vector3& trans)
        {
            this->fromRotationTranslation(q, trans);
        }
        
        /// Construct a dual quaternion from 8 manual w/x/y/z/dw/dx/dy/dz values
        inline DualQuaternion(Real* valptr)
        {
            memcpy(&w, valptr, sizeof(Real)*8);
        }

        /// Array accessor operator
        inline auto operator [] ( const size_t i ) const -> Real
        {
            assert( i < 8 );

            return *(&w+i);
        }

        /// Array accessor operator
        inline auto operator [] ( const size_t i ) -> Real&
        {
            assert( i < 8 );

            return *(&w+i);
        }
        
        inline auto operator= (const DualQuaternion& rkQ) -> DualQuaternion&
        = default;

        inline auto operator== (const DualQuaternion& rhs) const -> bool
        {
            return (rhs.w == w) && (rhs.x == x) && (rhs.y == y) && (rhs.z == z) && 
                (rhs.dw == dw) && (rhs.dx == dx) && (rhs.dy == dy) && (rhs.dz == dz);
        }

        inline auto operator!= (const DualQuaternion& rhs) const -> bool
        {
            return !operator==(rhs);
        }

        /// Pointer accessor for direct copying
        inline auto ptr() -> Real*
        {
            return &w;
        }

        /// Pointer accessor for direct copying
        [[nodiscard]]
        inline auto ptr() const -> const Real*
        {
            return &w;
        }
        
        /// Exchange the contents of this dual quaternion with another. 
        inline void swap(DualQuaternion& other)
        {
            std::swap(w, other.w);
            std::swap(x, other.x);
            std::swap(y, other.y);
            std::swap(z, other.z);
            std::swap(dw, other.dw);
            std::swap(dx, other.dx);
            std::swap(dy, other.dy);
            std::swap(dz, other.dz);
        }
        
        /// Check whether this dual quaternion contains valid values
        [[nodiscard]]
        inline auto isNaN() const -> bool
        {
            return Math::isNaN(w) || Math::isNaN(x) || Math::isNaN(y) || Math::isNaN(z) ||  
                Math::isNaN(dw) || Math::isNaN(dx) || Math::isNaN(dy) || Math::isNaN(dz);
        }

        /// Construct a dual quaternion from a rotation described by a Quaternion and a translation described by a Vector3
        void fromRotationTranslation (const Quaternion& q, const Vector3& trans);
        
        /// Convert a dual quaternion into its two components, a Quaternion representing the rotation and a Vector3 representing the translation
        void toRotationTranslation (Quaternion& q, Vector3& translation) const;

        /// Construct a dual quaternion from a 4x4 transformation matrix
        void fromTransformationMatrix (const Affine3& kTrans);
        
        /// Convert a dual quaternion to a 4x4 transformation matrix
        void toTransformationMatrix (Affine3& kTrans) const;

        Real w, x, y, z, dw, dx, dy, dz;

        /** 
        Function for writing to a stream. Outputs "DualQuaternion(w, x, y, z, dw, dx, dy, dz)" with w, x, y, z, dw, dx, dy, dz
        being the member values of the dual quaternion.
        */
        inline friend auto operator <<
        ( std::ostream& o, const DualQuaternion& q ) -> std::ostream&
        {
            o << "DualQuaternion(" << q.w << ", " << q.x << ", " << q.y << ", " << q.z << ", " << q.dw << ", " << q.dx << ", " << q.dy << ", " << q.dz << ")";
            return o;
        }
    };
    /** @} */
    /** @} */

}
