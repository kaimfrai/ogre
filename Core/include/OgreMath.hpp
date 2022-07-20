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
#include <cmath>
#include <cstdint>
#include <cstdlib>

export module Ogre.Core:Math;

export import :Platform;
export import :Prerequisites;

export import <limits>;
export import <ostream>;
export import <utility>;
export import <vector>;

export
namespace Ogre
{
struct Affine3;
struct AxisAlignedBox;
struct Degree;
struct Matrix3;
struct Matrix4;
struct Plane;
struct Quaternion;
struct Ray;
struct Sphere;
    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup Math
    *  @{
    */

    /** A pair structure where the first element indicates whether
        an intersection occurs

        if true, the second element will
        indicate the distance along the ray at which it intersects.
        This can be converted to a point in space by calling Ray::getPoint().
     */
    using RayTestResult = std::pair<bool, Real>;

    /** Wrapper class which indicates a given angle value is in Radians.
    @remarks
        Radian values are interchangeable with Degree values, and conversions
        will be done automatically between them.
    */
    struct Radian
    {
        float mRad{0};

        auto operator = ( const float& f ) -> Radian& { mRad = f; return *this; }
        auto operator = ( const Degree& d ) -> Radian&;

        [[nodiscard]] auto constexpr valueDegrees() const -> float; // see bottom of this file
        constexpr operator Degree() const;
        [[nodiscard]] auto constexpr valueRadians() const noexcept -> float { return mRad; }
        [[nodiscard]] auto valueAngleUnits() const -> float;

        auto operator + () const -> const Radian& { return *this; }
        auto operator + ( const Radian& r ) const -> Radian { return Radian{mRad + r.mRad}; }
        auto operator + ( const Degree& d ) const -> Radian;
        auto operator += ( const Radian& r ) -> Radian& { mRad += r.mRad; return *this; }
        auto operator += ( const Degree& d ) -> Radian&;
        auto operator - () const -> Radian { return Radian{-mRad}; }
        auto operator - ( const Radian& r ) const -> Radian { return Radian{mRad - r.mRad}; }
        auto operator - ( const Degree& d ) const -> Radian;
        auto operator -= ( const Radian& r ) -> Radian& { mRad -= r.mRad; return *this; }
        auto operator -= ( const Degree& d ) -> Radian&;
        auto operator * ( float f ) const -> Radian { return Radian{ mRad * f }; }
        auto operator * ( const Radian& f ) const -> Radian { return Radian{ mRad * f.mRad }; }
        auto operator *= ( float f ) -> Radian& { mRad *= f; return *this; }
        auto operator / ( float f ) const -> Radian { return Radian{ mRad / f }; }
        auto operator /= ( float f ) -> Radian& { mRad /= f; return *this; }

        [[nodiscard]] auto operator <=>  ( const Radian& r ) const noexcept = default;
        [[nodiscard]] auto operator == ( const Radian& r ) const noexcept -> bool = default;

        inline friend auto operator <<
            ( std::ostream& o, const Radian& v ) -> std::ostream&
        {
            o << "Radian{" << v.valueRadians() << "}";
            return o;
        }
    };

    /** Wrapper class which indicates a given angle value is in Degrees.
    @remarks
        Degree values are interchangeable with Radian values, and conversions
        will be done automatically between them.
    */
    struct Degree
    {
        float mDeg{0}; // if you get an error here - make sure to define/typedef 'Real' first

        auto operator = ( const float& f ) -> Degree& { mDeg = f; return *this; }
        auto operator = ( const Radian& r ) -> Degree& { mDeg = r.valueDegrees(); return *this; }

        [[nodiscard]] auto constexpr valueDegrees() const noexcept -> float { return mDeg; }
        [[nodiscard]] auto constexpr valueRadians() const -> float; // see bottom of this file
        constexpr operator Radian() const;
        [[nodiscard]] auto valueAngleUnits() const -> float;

        auto operator + () const -> const Degree& { return *this; }
        auto operator + ( const Degree& d ) const -> Degree { return Degree{ mDeg + d.mDeg }; }
        auto operator + ( const Radian& r ) const -> Degree { return Degree{ mDeg + r.valueDegrees() }; }
        auto operator += ( const Degree& d ) -> Degree& { mDeg += d.mDeg; return *this; }
        auto operator += ( const Radian& r ) -> Degree& { mDeg += r.valueDegrees(); return *this; }
        auto operator - () const -> Degree { return Degree{-mDeg}; }
        auto operator - ( const Degree& d ) const -> Degree { return Degree{ mDeg - d.mDeg }; }
        auto operator - ( const Radian& r ) const -> Degree { return Degree{ mDeg - r.valueDegrees() }; }
        auto operator -= ( const Degree& d ) -> Degree& { mDeg -= d.mDeg; return *this; }
        auto operator -= ( const Radian& r ) -> Degree& { mDeg -= r.valueDegrees(); return *this; }
        auto operator * ( float f ) const -> Degree { return Degree{ mDeg * f }; }
        auto operator * ( const Degree& f ) const -> Degree { return Degree{ mDeg * f.mDeg }; }
        auto operator *= ( float f ) -> Degree& { mDeg *= f; return *this; }
        auto operator / ( float f ) const -> Degree { return Degree{ mDeg / f }; }
        auto operator /= ( float f ) -> Degree& { mDeg /= f; return *this; }

        [[nodiscard]] auto operator <=> ( const Degree& d ) const noexcept = default;
        [[nodiscard]] auto operator == ( const Degree& d ) const noexcept -> bool = default;

        inline friend auto operator <<
            ( std::ostream& o, const Degree& v ) -> std::ostream&
        {
            o << "Degree{" << v.valueDegrees() << "}";
            return o;
        }
    };

    /** Wrapper class which identifies a value as the currently default angle 
        type, as defined by Math::setAngleUnit.
    @remarks
        Angle values will be automatically converted between radians and degrees,
        as appropriate.
    */
    struct Angle
    {
        float mAngle;
        constexpr operator Radian() const;
        constexpr operator Degree() const;
    };

    // these functions could not be defined within the class definition of class
    // Radian because they required struct Degree to be defined
    inline auto Radian::operator = ( const Degree& d ) -> Radian& {
        mRad = d.valueRadians(); return *this;
    }
    inline auto Radian::operator + ( const Degree& d ) const -> Radian {
        return Radian{ mRad + d.valueRadians() };
    }
    inline auto Radian::operator += ( const Degree& d ) -> Radian& {
        mRad += d.valueRadians();
        return *this;
    }
    inline auto Radian::operator - ( const Degree& d ) const -> Radian {
        return Radian{ mRad - d.valueRadians() };
    }
    inline auto Radian::operator -= ( const Degree& d ) -> Radian& {
        mRad -= d.valueRadians();
        return *this;
    }

    /** Class to provide access to common mathematical functions.
        @remarks
            Most of the maths functions are aliased versions of the C runtime
            library functions. They are aliased here to provide future
            optimisation opportunities, either from faster RTLs or custom
            math approximations.
        @note
            <br>This is based on MgcMath.h from
            <a href="http://www.geometrictools.com/">Wild Magic</a>.
    */
    namespace Math
    {
       /** The angular units used by the API. This functionality is now deprecated in favor
           of discreet angular unit types ( see Degree and Radian above ). The only place
           this functionality is actually still used is when parsing files. Search for
           usage of the Angle class for those instances
       */
       enum class AngleUnit
       {
           DEGREE,
           RADIAN
       };


       /** This class is used to provide an external random value provider. 
      */
       class RandomValueProvider
       {
       public:
            virtual ~RandomValueProvider() = default;
            /** When called should return a random values in the range of [0,1] */
            virtual auto getRandomUnit() -> Real = 0;
       };

       class Math
       {
        private:
            /// Angle units used by the api
            static AngleUnit msAngleUnit;

            /// Size of the trig tables as determined by constructor.
            static int mTrigTableSize;

            /// Radian -> index factor value ( mTrigTableSize / 2 * PI )
            static float mTrigTableFactor;
            static float* mSinTable;
            static float* mTanTable;

            /// A random value provider. overriding the default random number generator.
            static RandomValueProvider* mRandProvider;

            /** Private function to build trig tables.
            */
            void buildTrigTables();

            static auto SinTable (float fValue) -> float;
            static auto TanTable (float fValue) -> float;
        public:
            /** Default constructor.
                @param
                    trigTableSize Optional parameter to set the size of the
                    tables used to implement Sin, Cos, Tan
            */
            Math(unsigned int trigTableSize = 4096);

            /** Default destructor.
            */
            ~Math();

            friend inline auto Cos (const Radian& fValue, bool useTables) -> float;
            friend inline auto Cos (float fValue, bool useTables) -> float;
            friend inline auto Sin (const Radian& fValue, bool useTables) -> float;
            friend inline auto Sin (Real fValue, bool useTables) -> float;
            friend inline auto Tan (const Radian& fValue, bool useTables) -> float;
            friend inline auto Tan (Real fValue, bool useTables) -> float;

            static auto UnitRandom () -> Real;
            static void SetRandomValueProvider(RandomValueProvider* provider);
            static void setAngleUnit(AngleUnit unit);
            static auto getAngleUnit() -> AngleUnit;
        };

        constexpr Real inline POS_INFINITY = std::numeric_limits<Real>::infinity();
        constexpr Real inline NEG_INFINITY = -std::numeric_limits<Real>::infinity();
        constexpr Real inline PI = 3.14159265358979323846;
        constexpr Real inline TWO_PI = Real( 2.0 * PI );
        constexpr Real inline HALF_PI = Real( 0.5 * PI );
        constexpr float inline fDeg2Rad = PI / Real(180.0);
        constexpr float inline fRad2Deg = Real(180.0) / PI;

        inline auto IAbs (int iValue) -> int { return ( iValue >= 0 ? iValue : -iValue ); }
        inline auto ICeil (float fValue) -> int { return int(std::ceil(fValue)); }
        inline auto IFloor (float fValue) -> int { return int(std::floor(fValue)); }
        auto ISign (int iValue) -> int {
            return ( iValue > 0 ? +1 : ( iValue < 0 ? -1 : 0 ) );
        }

        /** Absolute value function
            @param
                fValue The value whose absolute value will be returned.
        */
        inline auto Abs (Real fValue) -> Real { return std::abs(fValue); }

        /** Absolute value function
            @param dValue
                The value, in degrees, whose absolute value will be returned.
            */
        inline auto Abs (const Degree& dValue) -> Degree { return Degree{std::abs(dValue.valueDegrees()) }; }

        /** Absolute value function
            @param rValue
                The value, in radians, whose absolute value will be returned.
            */
        inline auto Abs (const Radian& rValue) -> Radian { return Radian{std::abs(rValue.valueRadians()) }; }

        /** Arc cosine function
            @param fValue
                The value whose arc cosine will be returned.
            */
        auto ACos (Real fValue) -> Radian;

        /** Arc sine function
            @param fValue
                The value whose arc sine will be returned.
            */
        auto ASin (Real fValue) -> Radian;

        /** Arc tangent function
            @param fValue
                The value whose arc tangent will be returned.
            */
        inline auto ATan (float fValue) -> Radian { return Radian{std::atan(fValue)}; }

        /** Arc tangent between two values function
            @param fY
                The first value to calculate the arc tangent with.
            @param fX
                The second value to calculate the arc tangent with.
            */
        inline auto ATan2 (float fY, float fX) -> Radian { return Radian{std::atan2(fY,fX)}; }

        /** Ceiling function
            Returns the smallest following integer. (example: Ceil(1.1) = 2)

            @param fValue
                The value to round up to the nearest integer.
            */
        inline auto Ceil (Real fValue) -> Real { return std::ceil(fValue); }
        inline auto isNaN(Real f) -> bool
        {
            // std::isnan() is C99, not supported by all compilers
            // However NaN always fails this next test, no other number does.
            return f != f;
        }

        /** Cosine function.
            @param fValue
                Angle in radians
            @param useTables
                If true, uses lookup tables rather than
                calculation - faster but less accurate.
        */
        inline auto Cos (const Radian& fValue, bool useTables = false) -> float {
            return (!useTables) ? std::cos(fValue.valueRadians()) : Math::SinTable(fValue.valueRadians() + HALF_PI);
        }
        /** Cosine function.
            @param fValue
                Angle in radians
            @param useTables
                If true, uses lookup tables rather than
                calculation - faster but less accurate.
        */
        inline auto Cos (float fValue, bool useTables = false) -> float {
            return (!useTables) ? std::cos(fValue) : Math::SinTable(fValue + HALF_PI);
        }

        inline auto Exp (Real fValue) -> Real { return std::exp(fValue); }

        /** Floor function
            Returns the largest previous integer. (example: Floor(1.9) = 1)

            @param fValue
                The value to round down to the nearest integer.
            */
        inline auto Floor (Real fValue) -> Real { return std::floor(fValue); }

        inline auto Log (Real fValue) -> Real { return std::log(fValue); }

        /// Stored value of log(2) for frequent use
        constexpr Real LOG2 = 0.69314718055994530942;

        inline auto Log2 (Real fValue) -> Real { return std::log2(fValue); }

        inline auto LogN (Real base, Real fValue) -> Real { return std::log(fValue)/std::log(base); }

        inline auto Pow (Real fBase, Real fExponent) -> Real { return std::pow(fBase,fExponent); }

        auto Sign(Real fValue) -> Real
        {
            if (fValue > 0.0)
                return 1.0;
            if (fValue < 0.0)
                return -1.0;
            return 0.0;
        }

        inline auto Sign ( const Radian& rValue ) -> Radian
        {
            return Radian{Sign(rValue.valueRadians())};
        }
        inline auto Sign ( const Degree& dValue ) -> Degree
        {
            return Degree{Sign(dValue.valueDegrees())};
        }

        /// Simulate the shader function saturate that clamps a parameter value between 0 and 1
        inline auto saturate(float t) -> float { return (t < 0) ? 0 : ((t > 1) ? 1 : t); }
        inline auto saturate(double t) -> double { return (t < 0) ? 0 : ((t > 1) ? 1 : t); }

        /// saturated cast of size_t to uint16
        inline auto uint16Cast(size_t t) -> uint16 { return t < UINT16_MAX ? uint16(t) : UINT16_MAX; }

        /** Simulate the shader function lerp which performers linear interpolation

            given 3 parameters v0, v1 and t the function returns the value of (1 - t)* v0 + t * v1.
            where v0 and v1 are matching vector or scalar types and t can be either a scalar or a
            vector of the same type as a and b.
        */
        template <typename V, typename T> auto lerp(const V& v0, const V& v1, const T& t) -> V
        {
            return v0 * (1 - t) + v1 * t;
        }

        /** Sine function.
            @param fValue
                Angle in radians
            @param useTables
                If true, uses lookup tables rather than
                calculation - faster but less accurate.
        */
        inline auto Sin (const Radian& fValue, bool useTables = false) -> float {
            return (!useTables) ? std::sin(fValue.valueRadians()) : Math::SinTable(fValue.valueRadians());
        }
        /** Sine function.
            @param fValue
                Angle in radians
            @param useTables
                If true, uses lookup tables rather than
                calculation - faster but less accurate.
        */
        inline auto Sin (Real fValue, bool useTables = false) -> float {
            return (!useTables) ? std::sin(fValue) : Math::SinTable(fValue);
        }

        /** Squared function.
            @param fValue
                The value to be squared (fValue^2)
        */
        inline auto Sqr (Real fValue) -> Real { return fValue*fValue; }

        /** Square root function.
            @param fValue
                The value whose square root will be calculated.
            */
        inline auto Sqrt (Real fValue) -> Real { return std::sqrt(fValue); }

        /** Square root function.
            @param fValue
                The value, in radians, whose square root will be calculated.
            @return
                The square root of the angle in radians.
            */
        inline auto Sqrt (const Radian& fValue) -> Radian { return Radian{std::sqrt(fValue.valueRadians()) }; }

        /** Square root function.
            @param fValue
                The value, in degrees, whose square root will be calculated.
            @return
                The square root of the angle in degrees.
            */
        inline auto Sqrt (const Degree& fValue) -> Degree { return Degree{std::sqrt(fValue.valueDegrees()) }; }

        /** Inverse square root i.e. 1 / Sqrt(x), good for vector
            normalisation.
            @param fValue
                The value whose inverse square root will be calculated.
        */
        auto InvSqrt (Real fValue) -> Real {
            return Real(1.) / std::sqrt(fValue);
        }

        /** Generate a random number of unit length.
            @return
                A random number in the range from [0,1].
        */
        auto inline UnitRandom () -> Real
        {
            return Math::UnitRandom();
        }

        /** Generate a random number within the range provided.
            @param fLow
                The lower bound of the range.
            @param fHigh
                The upper bound of the range.
            @return
                A random number in the range from [fLow,fHigh].
            */
        auto RangeRandom (Real fLow, Real fHigh) -> Real {
            return (fHigh-fLow)*UnitRandom() + fLow;
        }

        /** Generate a random number in the range [-1,1].
            @return
                A random number in the range from [-1,1].
            */
        auto SymmetricRandom () -> Real {
            return 2.0f * UnitRandom() - 1.0f;
        }

        void inline SetRandomValueProvider(RandomValueProvider* provider)
        {
            Math::SetRandomValueProvider(provider);
        }

        /** Tangent function.
            @param fValue
                Angle in radians
            @param useTables
                If true, uses lookup tables rather than
                calculation - faster but less accurate.
        */
        inline auto Tan (const Radian& fValue, bool useTables = false) -> float {
            return (!useTables) ? std::tan(fValue.valueRadians()) : Math::TanTable(fValue.valueRadians());
        }
        /** Tangent function.
            @param fValue
                Angle in radians
            @param useTables
                If true, uses lookup tables rather than
                calculation - faster but less accurate.
        */
        inline auto Tan (Real fValue, bool useTables = false) -> float {
            return (!useTables) ? std::tan(fValue) : Math::TanTable(fValue);
        }

        constexpr auto DegreesToRadians(float degrees) -> float { return degrees * fDeg2Rad; }
        constexpr auto RadiansToDegrees(float radians) -> float { return radians * fRad2Deg; }

        /** These functions used to set the assumed angle units (radians or degrees)
                expected when using the Angle type.
        @par
                You can set this directly after creating a new Root, and also before/after resource creation,
                depending on whether you want the change to affect resource files.
        */
        void inline setAngleUnit(AngleUnit unit)
        {
            return Math::setAngleUnit(unit);
        }
        /** Get the unit being used for angles. */
        auto inline getAngleUnit() -> AngleUnit
        {
            return Math::getAngleUnit();
        }

        /** Convert from the current AngleUnit to radians. */
        auto AngleUnitsToRadians(float units) -> float;
        /** Convert from radians to the current AngleUnit . */
        auto RadiansToAngleUnits(float radians) -> float;
        /** Convert from the current AngleUnit to degrees. */
        auto AngleUnitsToDegrees(float units) -> float;
        /** Convert from degrees to the current AngleUnit. */
        auto DegreesToAngleUnits(float degrees) -> float;

        /** Checks whether a given point is inside a triangle, in a
            2-dimensional (Cartesian) space.
            @remarks
                The vertices of the triangle must be given in either
                trigonometrical (anticlockwise) or inverse trigonometrical
                (clockwise) order.
            @param p
                The point.
            @param a
                The triangle's first vertex.
            @param b
                The triangle's second vertex.
            @param c
                The triangle's third vertex.
            @return
                If the point resides in the triangle, <b>true</b> is
                returned.
            @par
                If the point is outside the triangle, <b>false</b> is
                returned.
        */
        auto pointInTri2D(const Vector2& p, const Vector2& a,
            const Vector2& b, const Vector2& c) -> bool;

        /** Checks whether a given 3D point is inside a triangle.
        @remarks
            The vertices of the triangle must be given in either
            trigonometrical (anticlockwise) or inverse trigonometrical
            (clockwise) order, and the point must be guaranteed to be in the
            same plane as the triangle
        @param p
            p The point.
        @param a
            The triangle's first vertex.
        @param b
            The triangle's second vertex.
        @param c
            The triangle's third vertex.
        @param normal
            The triangle plane's normal (passed in rather than calculated
            on demand since the caller may already have it)
        @return
            If the point resides in the triangle, <b>true</b> is
            returned.
        @par
            If the point is outside the triangle, <b>false</b> is
            returned.
        */
        auto pointInTri3D(const Vector3& p, const Vector3& a,
            const Vector3& b, const Vector3& c, const Vector3& normal) -> bool;
        /** Ray / plane intersection */
        inline auto intersects(const Ray& ray, const Plane& plane) -> RayTestResult;
        /** Ray / sphere intersection */
        auto intersects(const Ray& ray, const Sphere& sphere, bool discardInside = true) -> RayTestResult;
        /** Ray / box intersection */
        auto intersects(const Ray& ray, const AxisAlignedBox& box) -> RayTestResult;

        /** Ray / box intersection, returns boolean result and two intersection distance.
        @param ray
            The ray.
        @param box
            The box.
        @param d1
            A real pointer to retrieve the near intersection distance
            from the ray origin, maybe <b>null</b> which means don't care
            about the near intersection distance.
        @param d2
            A real pointer to retrieve the far intersection distance
            from the ray origin, maybe <b>null</b> which means don't care
            about the far intersection distance.
        @return
            If the ray is intersects the box, <b>true</b> is returned, and
            the near intersection distance is return by <i>d1</i>, the
            far intersection distance is return by <i>d2</i>. Guarantee
            <b>0</b> <= <i>d1</i> <= <i>d2</i>.
        @par
            If the ray isn't intersects the box, <b>false</b> is returned, and
            <i>d1</i> and <i>d2</i> is unmodified.
        */
        auto intersects(const Ray& ray, const AxisAlignedBox& box,
            Real* d1, Real* d2) -> bool;

        /** Ray / triangle intersection @cite moller1997fast, returns boolean result and distance.
        @param ray
            The ray.
        @param a
            The triangle's first vertex.
        @param b
            The triangle's second vertex.
        @param c
            The triangle's third vertex.
        @param positiveSide
            Intersect with "positive side" of the triangle (as determined by vertex winding)
        @param negativeSide
            Intersect with "negative side" of the triangle (as determined by vertex winding)
        */
        auto intersects(const Ray& ray, const Vector3& a,
            const Vector3& b, const Vector3& c,
            bool positiveSide = true, bool negativeSide = true) -> RayTestResult;

        /** Sphere / box intersection test. */
        auto intersects(const Sphere& sphere, const AxisAlignedBox& box) -> bool;

        /** Plane / box intersection test. */
        auto intersects(const Plane& plane, const AxisAlignedBox& box) -> bool;

        /** Ray / convex plane list intersection test.
        @param ray The ray to test with
        @param planeList List of planes which form a convex volume
        @param normalIsOutside Does the normal point outside the volume
        */
        auto intersects(const Ray& ray, const std::vector<Plane>& planeList, bool normalIsOutside) -> RayTestResult;

        /** Sphere / plane intersection test.
        @remarks NB just do a plane.getDistance(sphere.getCenter()) for more detail!
        */
        auto intersects(const Sphere& sphere, const Plane& plane) -> bool;

        /** Compare 2 reals, using tolerance for inaccuracies.
        */
        auto RealEqual(Real a, Real b,
            Real tolerance = std::numeric_limits<Real>::epsilon()) noexcept -> bool {
            return std::abs(b-a) <= tolerance;
        }

        /** Calculates the tangent space vector for a given set of positions / texture coords. */
        auto calculateTangentSpaceVector(
            const Vector3& position1, const Vector3& position2, const Vector3& position3,
            Real u1, Real v1, Real u2, Real v2, Real u3, Real v3) -> Vector3;

        /** Build a reflection matrix for the passed in plane. */
        auto buildReflectionMatrix(const Plane& p) -> Affine3;

        /** Generates a value based on the Gaussian (normal) distribution function
            with the given offset and scale parameters.
        */
        auto gaussianDistribution(Real x, Real offset = 0.0f, Real scale = 1.0f) -> Real;

        /** Clamp a value within an inclusive range. */
        template <typename T>
        auto Clamp(T val, T minval, T maxval) -> T
        {
            assert (minval <= maxval && "Invalid clamp range");
            return std::max(std::min(val, maxval), minval);
        }

        /** This creates a view matrix

            [ Lx  Uy  Dz  Tx  ]
            [ Lx  Uy  Dz  Ty  ]
            [ Lx  Uy  Dz  Tz  ]
            [ 0   0   0   1   ]

            Where T = -(Transposed(Rot) * Pos)
        */
        auto makeViewMatrix(const Vector3& position, const Quaternion& orientation,
            const Affine3* reflectMatrix = nullptr) -> Affine3;


        /** This creates 'uniform' perspective projection matrix,
            which depth range [-1,1], right-handed rules

        [ A   0   C   0  ]
        [ 0   B   D   0  ]
        [ 0   0   q   qn ]
        [ 0   0   -1  0  ]

        A = 2 * near / (right - left)
        B = 2 * near / (top - bottom)
        C = (right + left) / (right - left)
        D = (top + bottom) / (top - bottom)
        q = - (far + near) / (far - near)
        qn = - 2 * (far * near) / (far - near)
        */
        auto makePerspectiveMatrix(Real left, Real right, Real bottom, Real top, Real zNear, Real zFar) -> Matrix4;

        /** Get the radius of the origin-centered bounding sphere from the bounding box. */
        auto boundingRadiusFromAABB(const AxisAlignedBox& aabb) -> Real;

        /** Get the radius of the bbox-centered bounding sphere from the bounding box. */
        auto boundingRadiusFromAABBCentered(const AxisAlignedBox &aabb) -> Real;

        int Math::mTrigTableSize;
        AngleUnit Math::msAngleUnit;

        float  Math::mTrigTableFactor;
        float *Math::mSinTable = nullptr;
        float *Math::mTanTable = nullptr;

        RandomValueProvider* Math::mRandProvider = nullptr;
    }


    // these functions must be defined down here, because they rely on the
    // angle unit conversion functions in class Math:

    constexpr auto Radian::valueDegrees() const -> float
    {
        return Math::RadiansToDegrees ( mRad );
    }

    constexpr Radian::operator Degree() const
    {
        return { valueDegrees() };
    }

    inline auto Radian::valueAngleUnits() const -> float
    {
        return Math::RadiansToAngleUnits ( mRad );
    }

    constexpr auto Degree::valueRadians() const -> float
    {
        return Math::DegreesToRadians ( mDeg );
    }

    constexpr Degree::operator Radian() const
    {
        return { valueRadians() };
    }

    inline auto Degree::valueAngleUnits() const -> float
    {
        return Math::DegreesToAngleUnits ( mDeg );
    }

    constexpr Angle::operator Radian() const
    {
        return Radian{Math::AngleUnitsToRadians(mAngle)};
    }

    constexpr Angle::operator Degree() const
    {
        return Degree{Math::AngleUnitsToDegrees(mAngle)};
    }

    inline auto operator * ( float a, const Radian& b ) -> Radian
    {
        return Radian{ a * b.valueRadians() };
    }

    inline auto operator / ( float a, const Radian& b ) -> Radian
    {
        return Radian{ a / b.valueRadians() };
    }

    inline auto operator * ( float a, const Degree& b ) -> Degree
    {
        return Degree{ a * b.valueDegrees() };
    }

    inline auto operator / ( float a, const Degree& b ) -> Degree
    {
        return Degree{ a / b.valueDegrees() };
    }
    /** @} */
    /** @} */

}
static_assert(std::is_aggregate_v<Ogre::Radian>);
static_assert(std::is_standard_layout_v<Ogre::Radian>);
static_assert(std::is_aggregate_v<Ogre::Degree>);
static_assert(std::is_standard_layout_v<Ogre::Degree>);
static_assert(std::is_aggregate_v<Ogre::Angle>);
static_assert(std::is_standard_layout_v<Ogre::Angle>);
