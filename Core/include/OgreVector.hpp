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
#ifndef OGRE_CORE_VECTOR_H
#define OGRE_CORE_VECTOR_H


#include <algorithm>
#include <cassert>
#include <cstddef>
#include <ostream>
#include <type_traits>

#include "OgreMath.hpp"
#include "OgrePrerequisites.hpp"
#include "OgreQuaternion.hpp"

namespace Ogre
{

    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup Math
    *  @{
    */
    /// helper class to implement legacy API. Notably x, y, z access
    template <int dims, typename T> struct VectorBase
    {
        VectorBase() = default;
        VectorBase(T _x, T _y)
        {
            static_assert(dims > 1, "must have at least 2 dimensions");
            data[0] = _x; data[1] = _y;
        }
        VectorBase(T _x, T _y, T _z)
        {
            static_assert(dims > 2, "must have at least 3 dimensions");
            data[0] = _x; data[1] = _y; data[2] = _z;
        }
        VectorBase(T _x, T _y, T _z, T _w)
        {
            static_assert(dims > 3, "must have at least 4 dimensions");
            data[0] = _x; data[1] = _y; data[2] = _z; data[3] = _w;
        }
        T data[dims];
        auto ptr() -> T* { return data; }
        [[nodiscard]]
        auto ptr() const -> const T* { return data; }
    };
    template <> struct VectorBase<2, Real>
    {
        VectorBase() = default;
        VectorBase(Real _x, Real _y) : x(_x), y(_y) {}
        Real x, y;
        auto ptr() -> Real* { return &x; }
        [[nodiscard]]
        auto ptr() const -> const Real* { return &x; }

        /** Returns a vector at a point half way between this and the passed
            in vector.
        */
        [[nodiscard]]
        auto midPoint( const Vector2& vec ) const -> Vector2;

        /** Calculates the 2 dimensional cross-product of 2 vectors, which results
            in a single floating point value which is 2 times the area of the triangle.
        */
        [[nodiscard]]
        auto crossProduct( const VectorBase& rkVector ) const -> Real
        {
            return x * rkVector.y - y * rkVector.x;
        }

        /** Generates a vector perpendicular to this vector (eg an 'up' vector).
            @remarks
                This method will return a vector which is perpendicular to this
                vector. There are an infinite number of possibilities but this
                method will guarantee to generate one of them. If you need more
                control you should use the Quaternion class.
        */
        [[nodiscard]]
        auto perpendicular() const -> Vector2;

        /** Generates a new random vector which deviates from this vector by a
            given angle in a random direction.
            @remarks
                This method assumes that the random number generator has already
                been seeded appropriately.
            @param angle
                The angle at which to deviate in radians
            @return
                A random vector which deviates from this vector by angle. This
                vector will not be normalised, normalise it if you wish
                afterwards.
        */
        [[nodiscard]]
        auto randomDeviant(Radian angle) const -> Vector2;

        /**  Gets the oriented angle between 2 vectors.
        @remarks
            Vectors do not have to be unit-length but must represent directions.
            The angle is comprised between 0 and 2 PI.
        */
        [[nodiscard]]
        auto angleTo(const Vector2& other) const -> Radian;

        // special points
        static const Vector2 ZERO;
        static const Vector2 UNIT_X;
        static const Vector2 UNIT_Y;
        static const Vector2 NEGATIVE_UNIT_X;
        static const Vector2 NEGATIVE_UNIT_Y;
        static const Vector2 UNIT_SCALE;
    };

    template <> struct VectorBase<3, Real>
    {
        VectorBase() = default;
        VectorBase(Real _x, Real _y, Real _z) : x(_x), y(_y), z(_z) {}
        Real x, y, z;
        auto ptr() -> Real* { return &x; }
        [[nodiscard]]
        auto ptr() const -> const Real* { return &x; }

        /** Calculates the cross-product of 2 vectors, i.e. the vector that
            lies perpendicular to them both.
            @remarks
                The cross-product is normally used to calculate the normal
                vector of a plane, by calculating the cross-product of 2
                non-equivalent vectors which lie on the plane (e.g. 2 edges
                of a triangle).
            @param rkVector
                Vector which, together with this one, will be used to
                calculate the cross-product.
            @return
                A vector which is the result of the cross-product. This
                vector will <b>NOT</b> be normalised, to maximise efficiency
                - call Vector3::normalise on the result if you wish this to
                be done. As for which side the resultant vector will be on, the
                returned vector will be on the side from which the arc from 'this'
                to rkVector is anticlockwise, e.g. UNIT_Y.crossProduct(UNIT_Z)
                = UNIT_X, whilst UNIT_Z.crossProduct(UNIT_Y) = -UNIT_X.
                This is because OGRE uses a right-handed coordinate system.
            @par
                For a clearer explanation, look a the left and the bottom edges
                of your monitor's screen. Assume that the first vector is the
                left edge and the second vector is the bottom edge, both of
                them starting from the lower-left corner of the screen. The
                resulting vector is going to be perpendicular to both of them
                and will go <i>inside</i> the screen, towards the cathode tube
                (assuming you're using a CRT monitor, of course).
        */
        [[nodiscard]]
        auto crossProduct( const Vector3& rkVector ) const -> Vector3;

        /** Generates a vector perpendicular to this vector (eg an 'up' vector).
            @remarks
                This method will return a vector which is perpendicular to this
                vector. There are an infinite number of possibilities but this
                method will guarantee to generate one of them. If you need more
                control you should use the Quaternion class.
        */
        [[nodiscard]]
        auto perpendicular() const -> Vector3;

        auto operator = ( const Real fScaler ) -> Vector3&
        {
            x = fScaler;
            y = fScaler;
            z = fScaler;

            return (Vector3&)*this;
        }

        /** Calculates the absolute dot (scalar) product of this vector with another.
            @remarks
                This function work similar dotProduct, except it use absolute value
                of each component of the vector to computing.
            @param
                vec Vector with which to calculate the absolute dot product (together
                with this one).
            @return
                A Real representing the absolute dot product value.
        */
        [[nodiscard]]
        auto absDotProduct(const VectorBase& vec) const -> Real
        {
            return Math::Abs(x * vec.x) + Math::Abs(y * vec.y) + Math::Abs(z * vec.z);
        }

        /** Returns a vector at a point half way between this and the passed
            in vector.
        */
        [[nodiscard]]
        auto midPoint( const Vector3& vec ) const -> Vector3;

        /** Generates a new random vector which deviates from this vector by a
            given angle in a random direction.
            @remarks
                This method assumes that the random number generator has already
                been seeded appropriately.
            @param
                angle The angle at which to deviate
            @param
                up Any vector perpendicular to this one (which could generated
                by cross-product of this vector and any other non-colinear
                vector). If you choose not to provide this the function will
                derive one on it's own, however if you provide one yourself the
                function will be faster (this allows you to reuse up vectors if
                you call this method more than once)
            @return
                A random vector which deviates from this vector by angle. This
                vector will not be normalised, normalise it if you wish
                afterwards.
        */
        [[nodiscard]]
        auto randomDeviant(const Radian& angle, const Vector3& up = ZERO) const -> Vector3;

        /** Gets the shortest arc quaternion to rotate this vector to the destination
            vector.
        @remarks
            If you call this with a dest vector that is close to the inverse
            of this vector, we will rotate 180 degrees around the 'fallbackAxis'
            (if specified, or a generated axis if not) since in this case
            ANY axis of rotation is valid.
        */
        [[nodiscard]]
        auto getRotationTo(const Vector3& dest, const Vector3& fallbackAxis = ZERO) const -> Quaternion;

        /** Returns whether this vector is within a positional tolerance
            of another vector, also take scale of the vectors into account.
        @param rhs The vector to compare with
        @param tolerance The amount (related to the scale of vectors) that distance
            of the vector may vary by and still be considered close
        */
        [[nodiscard]]
        auto positionCloses(const Vector3& rhs, Real tolerance = 1e-03f) const -> bool;

        /** Returns whether this vector is within a directional tolerance
            of another vector.
        @param rhs The vector to compare with
        @param tolerance The maximum angle by which the vectors may vary and
            still be considered equal
        @note Both vectors should be normalised.
        */
        [[nodiscard]]
        auto directionEquals(const Vector3& rhs, const Radian& tolerance) const -> bool;

        /// Extract the primary (dominant) axis from this direction vector
        [[nodiscard]]
        auto primaryAxis() const -> const Vector3&;

        // special points
        static const Vector3 ZERO;
        static const Vector3 UNIT_X;
        static const Vector3 UNIT_Y;
        static const Vector3 UNIT_Z;
        static const Vector3 NEGATIVE_UNIT_X;
        static const Vector3 NEGATIVE_UNIT_Y;
        static const Vector3 NEGATIVE_UNIT_Z;
        static const Vector3 UNIT_SCALE;
    };

    template <> struct VectorBase<4, Real>
    {
        VectorBase() = default;
        VectorBase(Real _x, Real _y, Real _z, Real _w) : x(_x), y(_y), z(_z), w(_w) {}
        Real x, y, z, w;
        auto ptr() -> Real* { return &x; }
        [[nodiscard]]
        auto ptr() const -> const Real* { return &x; }

        auto operator = ( const Real fScalar) -> Vector4&
        {
            x = fScalar;
            y = fScalar;
            z = fScalar;
            w = fScalar;
            return (Vector4&)*this;
        }

        auto operator = (const VectorBase<3, Real>& rhs) -> Vector4&
        {
            x = rhs.x;
            y = rhs.y;
            z = rhs.z;
            w = 1.0f;
            return (Vector4&)*this;
        }

        // special points
        static const Vector4 ZERO;
    };

    /** Standard N-dimensional vector.

        A direction in N-D space represented as distances along the
        orthogonal axes. Note that positions, directions and
        scaling factors can be represented by a vector, depending on how
        you interpret the values.
    */
    template<int dims, typename T>
    class Vector : public VectorBase<dims, T>
    {
    public:
        using VectorBase<dims, T>::ptr;

        /** Default constructor.
            @note It does <b>NOT</b> initialize the vector for efficiency.
        */
        Vector() = default;
        Vector(T _x, T _y) : VectorBase<dims, T>(_x, _y) {}
        Vector(T _x, T _y, T _z) : VectorBase<dims, T>(_x, _y, _z) {}
        Vector(T _x, T _y, T _z, T _w) : VectorBase<dims, T>(_x, _y, _z, _w) {}

        // use enable_if as function parameter for VC < 2017 compatibility
        template <int N = dims>
        explicit Vector(const typename std::enable_if<N == 4, Vector3>::type& rhs, T fW = 1.0f) : VectorBase<dims, T>(rhs.x, rhs.y, rhs.z, fW) {}

        template<typename U>
        explicit Vector(const U* _ptr) {
            for (int i = 0; i < dims; i++)
                ptr()[i] = T(_ptr[i]);
        }

        template<typename U>
        explicit Vector(const Vector<dims, U>& o) : Vector(o.ptr()) {}


        explicit Vector(T s)
        {
            for (int i = 0; i < dims; i++)
                ptr()[i] = s;
        }

        /** Swizzle-like narrowing operations
        */
        [[nodiscard]]
        auto xyz() const -> Vector<3, T>
        {
            static_assert(dims > 3, "just use assignment");
            return Vector<3, T>(ptr());
        }
        [[nodiscard]]
        auto xy() const -> Vector<2, T>
        {
            static_assert(dims > 2, "just use assignment");
            return Vector<2, T>(ptr());
        }

        auto operator[](size_t i) const -> T
        {
            assert(i < dims);
            return ptr()[i];
        }

        auto operator[](size_t i) -> T&
        {
            assert(i < dims);
            return ptr()[i];
        }

        auto operator==(const Vector& v) const -> bool
        {
            for (int i = 0; i < dims; i++)
                if (ptr()[i] != v[i])
                    return false;
            return true;
        }

        /** Returns whether this vector is within a positional tolerance
            of another vector.
        @param rhs The vector to compare with
        @param tolerance The amount that each element of the vector may vary by
            and still be considered equal
        */
        [[nodiscard]]
        auto positionEquals(const Vector& rhs, Real tolerance = 1e-03f) const -> bool
        {
            for (int i = 0; i < dims; i++)
                if (!Math::RealEqual(ptr()[i], rhs[i], tolerance))
                    return false;
            return true;
        }

        auto operator!=(const Vector& v) const -> bool { return !(*this == v); }

        /** Returns true if the vector's scalar components are all greater
            that the ones of the vector it is compared against.
        */
        auto operator<(const Vector& rhs) const -> bool
        {
            for (int i = 0; i < dims; i++)
                if (!(ptr()[i] < rhs[i]))
                    return false;
            return true;
        }

        /** Returns true if the vector's scalar components are all smaller
            that the ones of the vector it is compared against.
        */
        auto operator>(const Vector& rhs) const -> bool
        {
            for (int i = 0; i < dims; i++)
                if (!(ptr()[i] > rhs[i]))
                    return false;
            return true;
        }

        /** Sets this vector's components to the minimum of its own and the
            ones of the passed in vector.
            @remarks
                'Minimum' in this case means the combination of the lowest
                value of x, y and z from both vectors. Lowest is taken just
                numerically, not magnitude, so -1 < 0.
        */
        void makeFloor(const Vector& cmp)
        {
            for (int i = 0; i < dims; i++)
                if (cmp[i] < ptr()[i])
                    ptr()[i] = cmp[i];
        }

        /** Sets this vector's components to the maximum of its own and the
            ones of the passed in vector.
            @remarks
                'Maximum' in this case means the combination of the highest
                value of x, y and z from both vectors. Highest is taken just
                numerically, not magnitude, so 1 > -3.
        */
        void makeCeil(const Vector& cmp)
        {
            for (int i = 0; i < dims; i++)
                if (cmp[i] > ptr()[i])
                    ptr()[i] = cmp[i];
        }

        /** Calculates the dot (scalar) product of this vector with another.
            @remarks
                The dot product can be used to calculate the angle between 2
                vectors. If both are unit vectors, the dot product is the
                cosine of the angle; otherwise the dot product must be
                divided by the product of the lengths of both vectors to get
                the cosine of the angle. This result can further be used to
                calculate the distance of a point from a plane.
            @param
                vec Vector with which to calculate the dot product (together
                with this one).
            @return
                A float representing the dot product value.
        */
        [[nodiscard]]
        auto dotProduct(const VectorBase<dims, T>& vec) const -> T
        {
            T ret = 0;
            for (int i = 0; i < dims; i++)
                ret += ptr()[i] * vec.ptr()[i];
            return ret;
        }

        /** Returns the square of the length(magnitude) of the vector.
            @remarks
                This  method is for efficiency - calculating the actual
                length of a vector requires a square root, which is expensive
                in terms of the operations required. This method returns the
                square of the length of the vector, i.e. the same as the
                length but before the square root is taken. Use this if you
                want to find the longest / shortest vector without incurring
                the square root.
        */
        [[nodiscard]]
        auto squaredLength() const -> T { return dotProduct(*this); }

        /** Returns true if this vector is zero length. */
        [[nodiscard]]
        auto isZeroLength() const -> bool
        {
            return squaredLength() < 1e-06 * 1e-06;
        }

        /** Returns the length (magnitude) of the vector.
            @warning
                This operation requires a square root and is expensive in
                terms of CPU operations. If you don't need to know the exact
                length (e.g. for just comparing lengths) use squaredLength()
                instead.
        */
        [[nodiscard]]
        auto length() const -> Real { return Math::Sqrt(squaredLength()); }

        /** Returns the distance to another vector.
            @warning
                This operation requires a square root and is expensive in
                terms of CPU operations. If you don't need to know the exact
                distance (e.g. for just comparing distances) use squaredDistance()
                instead.
        */
        [[nodiscard]]
        auto distance(const Vector& rhs) const -> Real
        {
            return (*this - rhs).length();
        }

        /** Returns the square of the distance to another vector.
            @remarks
                This method is for efficiency - calculating the actual
                distance to another vector requires a square root, which is
                expensive in terms of the operations required. This method
                returns the square of the distance to another vector, i.e.
                the same as the distance but before the square root is taken.
                Use this if you want to find the longest / shortest distance
                without incurring the square root.
        */
        [[nodiscard]]
        auto squaredDistance(const Vector& rhs) const -> T
        {
            return (*this - rhs).squaredLength();
        }

        /** Normalises the vector.
            @remarks
                This method normalises the vector such that it's
                length / magnitude is 1. The result is called a unit vector.
            @note
                This function will not crash for zero-sized vectors, but there
                will be no changes made to their components.
            @return The previous length of the vector.
        */
        auto normalise() -> Real
        {
            Real fLength = length();

            // Will also work for zero-sized vectors, but will change nothing
            // We're not using epsilons because we don't need to.
            // Read http://www.ogre3d.org/forums/viewtopic.php?f=4&t=61259
            if (fLength > Real(0.0f))
            {
                Real fInvLength = 1.0f / fLength;
                for (int i = 0; i < dims; i++)
                    ptr()[i] *= fInvLength;
            }

            return fLength;
        }

        /** As normalise, except that this vector is unaffected and the
            normalised vector is returned as a copy. */
        [[nodiscard]]
        auto normalisedCopy() const -> Vector
        {
            Vector ret = *this;
            ret.normalise();
            return ret;
        }

        /// Check whether this vector contains valid values
        [[nodiscard]]
        auto isNaN() const -> bool
        {
            for (int i = 0; i < dims; i++)
                if (Math::isNaN(ptr()[i]))
                    return true;
            return false;
        }

        /** Gets the angle between 2 vectors.
        @remarks
            Vectors do not have to be unit-length but must represent directions.
        */
        [[nodiscard]]
        auto angleBetween(const Vector& dest) const -> Radian
        {
            Real lenProduct = length() * dest.length();

            // Divide by zero check
            if(lenProduct < 1e-6f)
                lenProduct = 1e-6f;

            Real f = dotProduct(dest) / lenProduct;

            f = Math::Clamp(f, (Real)-1.0, (Real)1.0);
            return Math::ACos(f);

        }

        /** Calculates a reflection vector to the plane with the given normal .
        @remarks NB assumes 'this' is pointing AWAY FROM the plane, invert if it is not.
        */
        [[nodiscard]]
        auto reflect(const Vector& normal) const -> Vector { return *this - (2 * dotProduct(normal) * normal); }

        // Vector: arithmetic updates
        auto operator*=(Real s) -> Vector&
        {
            for (int i = 0; i < dims; i++)
                ptr()[i] *= s;
            return *this;
        }

        auto operator/=(Real s) -> Vector&
        {
            assert( s != 0.0 ); // legacy assert
            Real fInv = 1.0f/s;
            for (int i = 0; i < dims; i++)
                ptr()[i] *= fInv;
            return *this;
        }

        auto operator+=(Real s) -> Vector&
        {
            for (int i = 0; i < dims; i++)
                ptr()[i] += s;
            return *this;
        }

        auto operator-=(Real s) -> Vector&
        {
            for (int i = 0; i < dims; i++)
                ptr()[i] -= s;
            return *this;
        }

        auto operator+=(const Vector& b) -> Vector&
        {
            for (int i = 0; i < dims; i++)
                ptr()[i] += b[i];
            return *this;
        }

        auto operator-=(const Vector& b) -> Vector&
        {
            for (int i = 0; i < dims; i++)
                ptr()[i] -= b[i];
            return *this;
        }

        auto operator*=(const Vector& b) -> Vector&
        {
            for (int i = 0; i < dims; i++)
                ptr()[i] *= b[i];
            return *this;
        }

        auto operator/=(const Vector& b) -> Vector&
        {
            for (int i = 0; i < dims; i++)
                ptr()[i] /= b[i];
            return *this;
        }

        // Scalar * Vector
        friend auto operator*(Real s, Vector v) -> Vector
        {
            v *= s;
            return v;
        }

        friend auto operator+(Real s, Vector v) -> Vector
        {
            v += s;
            return v;
        }

        friend auto operator-(Real s, const Vector& v) -> Vector
        {
            Vector ret;
            for (int i = 0; i < dims; i++)
                ret[i] = s - v[i];
            return ret;
        }

        friend auto operator/(Real s, const Vector& v) -> Vector
        {
            Vector ret;
            for (int i = 0; i < dims; i++)
                ret[i] = s / v[i];
            return ret;
        }

        // Vector * Scalar
        auto operator-() const -> Vector
        {
            return -1 * *this;
        }

        auto operator+() const -> const Vector&
        {
            return *this;
        }

        auto operator*(Real s) const -> Vector
        {
            return s * *this;
        }

        auto operator/(Real s) const -> Vector
        {
            assert( s != 0.0 ); // legacy assert
            Real fInv = 1.0f / s;
            return fInv * *this;
        }

        auto operator-(Real s) const -> Vector
        {
            return -s + *this;
        }

        auto operator+(Real s) const -> Vector
        {
            return s + *this;
        }

        // Vector * Vector
        auto operator+(const Vector& b) const -> Vector
        {
            Vector ret = *this;
            ret += b;
            return ret;
        }

        auto operator-(const Vector& b) const -> Vector
        {
            Vector ret = *this;
            ret -= b;
            return ret;
        }

        auto operator*(const Vector& b) const -> Vector
        {
            Vector ret = *this;
            ret *= b;
            return ret;
        }

        auto operator/(const Vector& b) const -> Vector
        {
            Vector ret = *this;
            ret /= b;
            return ret;
        }

        friend auto operator<<(std::ostream& o, const Vector& v) -> std::ostream&
        {
            o << "Vector" << dims << "(";
            for (int i = 0; i < dims; i++) {
                o << v[i];
                if(i != dims - 1) o << ", ";
            }
            o <<  ")";
            return o;
        }
    };

    inline auto VectorBase<2, Real>::midPoint( const Vector2& vec ) const -> Vector2
    {
        return {
            ( x + vec.x ) * 0.5f,
            ( y + vec.y ) * 0.5f };
    }

    inline auto VectorBase<2, Real>::randomDeviant(Radian angle) const -> Vector2
    {
        angle *= Math::RangeRandom(-1, 1);
        Real cosa = Math::Cos(angle);
        Real sina = Math::Sin(angle);
        return {cosa * x - sina * y,
                       sina * x + cosa * y};
    }

    inline auto VectorBase<2, Real>::angleTo(const Vector2& other) const -> Radian
    {
        Radian angle = ((const Vector2*)this)->angleBetween(other);

        if (crossProduct(other)<0)
            angle = Radian(Math::TWO_PI) - angle;

        return angle;
    }

    inline auto VectorBase<2, Real>::perpendicular() const -> Vector2
    {
        return {-y, x};
    }

    inline auto VectorBase<3, Real>::perpendicular() const -> Vector3
    {
        // From Sam Hocevar's article "On picking an orthogonal
        // vector (and combing coconuts)"
        Vector3 perp = Math::Abs(x) > Math::Abs(z)
                     ? Vector3(-y, x, 0.0) : Vector3(0.0, -z, y);
        return perp.normalisedCopy();
    }

    inline auto VectorBase<3, Real>::crossProduct( const Vector3& rkVector ) const -> Vector3
    {
        return {
            y * rkVector.z - z * rkVector.y,
            z * rkVector.x - x * rkVector.z,
            x * rkVector.y - y * rkVector.x};
    }

    inline auto VectorBase<3, Real>::midPoint( const Vector3& vec ) const -> Vector3
    {
        return {
            ( x + vec.x ) * 0.5f,
            ( y + vec.y ) * 0.5f,
            ( z + vec.z ) * 0.5f };
    }

    inline auto VectorBase<3, Real>::randomDeviant(const Radian& angle, const Vector3& up) const -> Vector3
    {
        Vector3 newUp;

        if (up == ZERO)
        {
            // Generate an up vector
            newUp = ((const Vector3*)this)->perpendicular();
        }
        else
        {
            newUp = up;
        }

        // Rotate up vector by random amount around this
        Quaternion q;
        q.FromAngleAxis( Radian(Math::UnitRandom() * Math::TWO_PI), (const Vector3&)*this );
        newUp = q * newUp;

        // Finally rotate this by given angle around randomised up
        q.FromAngleAxis( angle, newUp );
        return q * (const Vector3&)(*this);
    }

    inline auto VectorBase<3, Real>::getRotationTo(const Vector3& dest, const Vector3& fallbackAxis) const -> Quaternion
    {
        // From Sam Hocevar's article "Quaternion from two vectors:
        // the final version"
        Real a = Math::Sqrt(((const Vector3*)this)->squaredLength() * dest.squaredLength());
        Real b = a + dest.dotProduct(*this);

        if (Math::RealEqual(b, 2 * a) || a == 0)
            return Quaternion::IDENTITY;

        Vector3 axis;

        if (b < (Real)1e-06 * a)
        {
            b = (Real)0.0;
            axis = fallbackAxis != Vector3::ZERO ? fallbackAxis
                 : Math::Abs(x) > Math::Abs(z) ? Vector3(-y, x, (Real)0.0)
                 : Vector3((Real)0.0, -z, y);
        }
        else
        {
            axis = this->crossProduct(dest);
        }

        Quaternion q(b, axis.x, axis.y, axis.z);
        q.normalise();
        return q;
    }

    inline auto VectorBase<3, Real>::positionCloses(const Vector3& rhs, Real tolerance) const -> bool
    {
        return ((const Vector3*)this)->squaredDistance(rhs) <=
            (((const Vector3*)this)->squaredLength() + rhs.squaredLength()) * tolerance;
    }

    inline auto VectorBase<3, Real>::directionEquals(const Vector3& rhs, const Radian& tolerance) const -> bool
    {
        Real dot = rhs.dotProduct(*this);
        Radian angle = Math::ACos(dot);

        return Math::Abs(angle.valueRadians()) <= tolerance.valueRadians();
    }

    inline auto VectorBase<3, Real>::primaryAxis() const -> const Vector3&
    {
        Real absx = Math::Abs(x);
        Real absy = Math::Abs(y);
        Real absz = Math::Abs(z);
        if (absx > absy)
            if (absx > absz)
                return x > 0 ? UNIT_X : NEGATIVE_UNIT_X;
            else
                return z > 0 ? UNIT_Z : NEGATIVE_UNIT_Z;
        else // absx <= absy
            if (absy > absz)
                return y > 0 ? UNIT_Y : NEGATIVE_UNIT_Y;
            else
                return z > 0 ? UNIT_Z : NEGATIVE_UNIT_Z;
    }

    // Math functions
    inline auto Math::calculateBasicFaceNormal(const Vector3& v1, const Vector3& v2, const Vector3& v3) -> Vector3
    {
        Vector3 normal = (v2 - v1).crossProduct(v3 - v1);
        normal.normalise();
        return normal;
    }
    inline auto Math::calculateFaceNormal(const Vector3& v1, const Vector3& v2, const Vector3& v3) -> Vector4
    {
        Vector3 normal = calculateBasicFaceNormal(v1, v2, v3);
        // Now set up the w (distance of tri from origin
        return {normal.x, normal.y, normal.z, -(normal.dotProduct(v1))};
    }
    inline auto Math::calculateBasicFaceNormalWithoutNormalize(
        const Vector3& v1, const Vector3& v2, const Vector3& v3) -> Vector3
    {
        return (v2 - v1).crossProduct(v3 - v1);
    }

    inline auto Math::calculateFaceNormalWithoutNormalize(const Vector3& v1,
                                                             const Vector3& v2,
                                                             const Vector3& v3) -> Vector4
    {
        Vector3 normal = calculateBasicFaceNormalWithoutNormalize(v1, v2, v3);
        // Now set up the w (distance of tri from origin)
        return {normal.x, normal.y, normal.z, -(normal.dotProduct(v1))};
    }
    /** @} */
    /** @} */

}
#endif
