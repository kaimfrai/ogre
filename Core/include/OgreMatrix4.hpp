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
#ifndef OGRE_CORE_MATRIX4_H
#define OGRE_CORE_MATRIX4_H

#include <cassert>
#include <cstring>
#include <ostream>

// Precompiler options
#include "OgreMatrix3.hpp"
#include "OgrePrerequisites.hpp"
#include "OgreQuaternion.hpp"
#include "OgreVector.hpp"

namespace Ogre
{
    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup Math
    *  @{
    */
    struct Matrix4;

    auto operator*(const Matrix4 &m, const Matrix4 &m2) -> Matrix4;
    /** Class encapsulating a standard 4x4 homogeneous matrix.
        @remarks
            OGRE uses column vectors when applying matrix multiplications,
            This means a vector is represented as a single column, 4-row
            matrix. This has the effect that the transformations implemented
            by the matrices happens right-to-left e.g. if vector V is to be
            transformed by M1 then M2 then M3, the calculation would be
            M3 * M2 * M1 * V. The order that matrices are concatenated is
            vital since matrix multiplication is not commutative, i.e. you
            can get a different result if you concatenate in the wrong order.
        @par
            The use of column vectors and right-to-left ordering is the
            standard in most mathematical texts, and is the same as used in
            OpenGL. It is, however, the opposite of Direct3D, which has
            inexplicably chosen to differ from the accepted standard and uses
            row vectors and left-to-right matrix multiplication.
        @par
            OGRE deals with the differences between D3D and OpenGL etc.
            internally when operating through different render systems. OGRE
            users only need to conform to standard maths conventions, i.e.
            right-to-left matrix multiplication, (OGRE transposes matrices it
            passes to D3D to compensate).
        @par
            The generic form M * V which shows the layout of the matrix
            entries is shown below:
            <pre>
                [ m[0][0]  m[0][1]  m[0][2]  m[0][3] ]   {x}
                | m[1][0]  m[1][1]  m[1][2]  m[1][3] | * {y}
                | m[2][0]  m[2][1]  m[2][2]  m[2][3] |   {z}
                [ m[3][0]  m[3][1]  m[3][2]  m[3][3] ]   {1}
            </pre>
    */
    template<int rows, typename T> struct TransformBase
    {
        T m[rows][4];

        [[nodiscard]] inline auto operator == ( const TransformBase& m2 ) const noexcept -> bool = default;

        template<typename U>
        [[nodiscard]]
        static auto constexpr FromPtr(const U* ptr)
        {
            TransformBase transform;
            std::ranges::copy(std::span{ptr, rows * 4}, &transform.m[0][0]);
            return transform;
        }

        auto operator[](size_t iRow) -> T*
        {
            assert(iRow < rows);
            return m[iRow];
        }

        auto operator[](size_t iRow) const -> const T*
        {
            assert(iRow < rows);
            return m[iRow];
        }

        /// Sets the translation transformation part of the matrix.
        void setTrans( const Vector<3, T>& v )
        {
            assert(rows > 2);
            m[0][3] = v[0];
            m[1][3] = v[1];
            m[2][3] = v[2];
        }
        /// Extracts the translation transformation part of the matrix.
        [[nodiscard]] auto getTrans() const -> Vector<3, T>
        {
            assert(rows > 2);
            return Vector<3, T>{m[0][3], m[1][3], m[2][3]};
        }
        /// Sets the scale part of the matrix.
        void setScale( const Vector<3, T>& v )
        {
            assert(rows > 2);
            m[0][0] = v[0];
            m[1][1] = v[1];
            m[2][2] = v[2];
        }

        /** Function for writing to a stream.
         */
        inline friend auto operator<<(std::ostream& o, const TransformBase& mat) -> std::ostream&
        {
            o << "Matrix" << rows << "x4(";
            for (size_t i = 0; i < rows; ++i)
            {
                for (size_t j = 0; j < 4; ++j)
                {
                    o << mat[i][j];
                    if(j != 3)
                        o << ", ";
                }

                if(i != (rows - 1))
                    o << "; ";
            }
            o << ")";
            return o;
        }
    };

    struct TransformBaseReal : public TransformBase<4, Real>
    {
        /** Builds a translation matrix
        */
        void makeTrans( const Vector3& v )
        {
            makeTrans(v.x, v.y, v.z);
        }

        void makeTrans( Real tx, Real ty, Real tz )
        {
            m[0][0] = 1.0; m[0][1] = 0.0; m[0][2] = 0.0; m[0][3] = tx;
            m[1][0] = 0.0; m[1][1] = 1.0; m[1][2] = 0.0; m[1][3] = ty;
            m[2][0] = 0.0; m[2][1] = 0.0; m[2][2] = 1.0; m[2][3] = tz;
            m[3][0] = 0.0; m[3][1] = 0.0; m[3][2] = 0.0; m[3][3] = 1.0;
        }

        /** Assignment from 3x3 matrix.
        */
        void set3x3Matrix(const Matrix3& mat3)
        {
            m[0][0] = mat3[0][0]; m[0][1] = mat3[0][1]; m[0][2] = mat3[0][2];
            m[1][0] = mat3[1][0]; m[1][1] = mat3[1][1]; m[1][2] = mat3[1][2];
            m[2][0] = mat3[2][0]; m[2][1] = mat3[2][1]; m[2][2] = mat3[2][2];
        }

        /** Extracts the rotation / scaling part of the Matrix as a 3x3 matrix.
        */
        [[nodiscard]] auto linear() const -> Matrix3
        {
            return {m[0][0], m[0][1], m[0][2],
                           m[1][0], m[1][1], m[1][2],
                           m[2][0], m[2][1], m[2][2]};
        }

        [[nodiscard]] auto determinant() const -> Real;

        [[nodiscard]] auto transpose() const -> Matrix4;

        /** Building a Affine3 from orientation / scale / position.
        @remarks
            Transform is performed in the order scale, rotate, translation, i.e. translation is independent
            of orientation axes, scale does not affect size of translation, rotation and scaling are always
            centered on the origin.
        */
        void makeTransform(const Vector3& position, const Vector3& scale, const Quaternion& orientation);

        /** Building an inverse Affine3 from orientation / scale / position.
        @remarks
            As makeTransform except it build the inverse given the same data as makeTransform, so
            performing -translation, -rotate, 1/scale in that order.
        */
        void makeInverseTransform(const Vector3& position, const Vector3& scale, const Quaternion& orientation);
    };

    /// Transform specialization for projective - encapsulating a 4x4 Matrix
    struct Matrix4 : public TransformBaseReal
    {
        template<typename U>
        [[nodiscard]]
        static auto constexpr FromPtr(U const* ptr) -> Matrix4
        {
            return std::bit_cast<Matrix4>(TransformBaseReal::FromPtr(ptr));
        }

        /** Creates a standard 4x4 transformation matrix with a zero translation part from a rotation/scaling 3x3 matrix.
         */
        [[nodiscard]]
        static auto constexpr FromMatrix3(const Matrix3& m3x3) -> Matrix4
        {
            Matrix4 mat4;
            mat4 = IDENTITY;
            mat4 = m3x3;
            return mat4;
        }

        /** Creates a standard 4x4 transformation matrix with a zero translation part from a rotation/scaling Quaternion.
         */
        [[nodiscard]]
        static auto constexpr FromQuaternion(const Quaternion& rot) -> Matrix4
        {
            Matrix4 mat4;
            Matrix3 m3x3;
            rot.ToRotationMatrix(m3x3);
            mat4 = IDENTITY;
            mat4 = m3x3;
            return mat4;
        }
        
        auto operator=(const Matrix3& mat3) -> Matrix4& {
            set3x3Matrix(mat3);
            return *this;
        }

        static const Matrix4 ZERO;
        static const Matrix4 IDENTITY;
        /** Useful little matrix which takes 2D clipspace {-1, 1} to {0,1}
            and inverts the Y. */
        static const Matrix4 CLIPSPACE2DTOIMAGESPACE;

        inline auto operator*(Real scalar) const -> Matrix4
        {
            return {
                scalar*m[0][0], scalar*m[0][1], scalar*m[0][2], scalar*m[0][3],
                scalar*m[1][0], scalar*m[1][1], scalar*m[1][2], scalar*m[1][3],
                scalar*m[2][0], scalar*m[2][1], scalar*m[2][2], scalar*m[2][3],
                scalar*m[3][0], scalar*m[3][1], scalar*m[3][2], scalar*m[3][3]};
        }
        
        [[nodiscard]] auto adjoint() const -> Matrix4;
        [[nodiscard]] auto inverse() const -> Matrix4;
    };

    /// Transform specialization for 3D Affine - encapsulating a 3x4 Matrix
    struct Affine3 : public TransformBaseReal
    {
        /// @copydoc TransformBaseReal::makeTransform
        [[nodiscard]]
        static auto constexpr MakeTransform(const Vector3& position, const Quaternion& orientation, const Vector3& scale = Vector3::UNIT_SCALE) -> Affine3
        {
            Affine3 affine;
            affine.makeTransform(position, scale, orientation);
            return affine;
        }

        template<typename U>
        [[nodiscard]]
        static auto constexpr FromPtr(const U* ptr) -> Affine3
        {
            Affine3 affine;
            std::ranges::copy(std::span{ptr, 3 * 4}, &affine.m[0][0]);
            affine.m[3][0] = 0;
            affine.m[3][1] = 0;
            affine.m[3][2] = 0;
            affine.m[3][3] = 1;
            return affine;
        }

        /// extract the Affine part of a Matrix4
        [[nodiscard]]
        static auto constexpr FromMatrix4(const Matrix4& mat) -> Affine3
        {
            Affine3 affine;
            auto& m = affine.m;
            m[0][0] = mat[0][0]; m[0][1] = mat[0][1]; m[0][2] = mat[0][2]; m[0][3] = mat[0][3];
            m[1][0] = mat[1][0]; m[1][1] = mat[1][1]; m[1][2] = mat[1][2]; m[1][3] = mat[1][3];
            m[2][0] = mat[2][0]; m[2][1] = mat[2][1]; m[2][2] = mat[2][2]; m[2][3] = mat[2][3];
            m[3][0] = 0;         m[3][1] = 0;         m[3][2] = 0;         m[3][3] = 1;
            return affine;
        }

        auto operator=(const Matrix3& mat3) -> Affine3& {
            set3x3Matrix(mat3);
            return *this;
        }

        /** Tests 2 matrices for equality.
        */
        [[nodiscard]] auto operator==(const Affine3& m2) const noexcept -> bool = default;

        [[nodiscard]] auto inverse() const -> Affine3;

        /** Decompose to orientation / scale / position.
        */
        void decomposition(Vector3& position, Vector3& scale, Quaternion& orientation) const;

        /// every Affine3 transform is also a _const_ Matrix4
        operator const Matrix4&() const { return reinterpret_cast<const Matrix4&>(*this); }

        using TransformBaseReal::getTrans;

        /** Gets a translation matrix.
        */
        static auto getTrans( const Vector3& v ) -> Affine3
        {
            return getTrans(v.x, v.y, v.z);
        }

        /** Gets a translation matrix - variation for not using a vector.
        */
        static auto getTrans( Real t_x, Real t_y, Real t_z ) -> Affine3
        {
            return
            {   1, 0, 0, t_x,
                0, 1, 0, t_y,
                0, 0, 1, t_z,
                0, 0, 0, 1
            };
        }

        /** Gets a scale matrix.
        */
        static auto getScale( const Vector3& v ) -> Affine3
        {
            return getScale(v.x, v.y, v.z);
        }

        /** Gets a scale matrix - variation for not using a vector.
        */
        static auto getScale( Real s_x, Real s_y, Real s_z ) -> Affine3
        {
            return
            {   s_x, 0, 0, 0,
                0, s_y, 0, 0,
                0, 0, s_z, 0,
                0, 0, 0, 1
            };
        }


        static const Affine3 ZERO;
        static const Affine3 IDENTITY;
    };

    inline auto TransformBaseReal::transpose() const -> Matrix4
    {
        return {m[0][0], m[1][0], m[2][0], m[3][0],
                       m[0][1], m[1][1], m[2][1], m[3][1],
                       m[0][2], m[1][2], m[2][2], m[3][2],
                       m[0][3], m[1][3], m[2][3], m[3][3]};
    }

    /** Matrix addition.
    */
    inline auto operator+(const Matrix4& m, const Matrix4& m2) -> Matrix4
    {
        Matrix4 r;

        r[0][0] = m[0][0] + m2[0][0];
        r[0][1] = m[0][1] + m2[0][1];
        r[0][2] = m[0][2] + m2[0][2];
        r[0][3] = m[0][3] + m2[0][3];

        r[1][0] = m[1][0] + m2[1][0];
        r[1][1] = m[1][1] + m2[1][1];
        r[1][2] = m[1][2] + m2[1][2];
        r[1][3] = m[1][3] + m2[1][3];

        r[2][0] = m[2][0] + m2[2][0];
        r[2][1] = m[2][1] + m2[2][1];
        r[2][2] = m[2][2] + m2[2][2];
        r[2][3] = m[2][3] + m2[2][3];

        r[3][0] = m[3][0] + m2[3][0];
        r[3][1] = m[3][1] + m2[3][1];
        r[3][2] = m[3][2] + m2[3][2];
        r[3][3] = m[3][3] + m2[3][3];

        return r;
    }

    /** Matrix subtraction.
    */
    inline auto operator-(const Matrix4& m, const Matrix4& m2) -> Matrix4
    {
        Matrix4 r;
        r[0][0] = m[0][0] - m2[0][0];
        r[0][1] = m[0][1] - m2[0][1];
        r[0][2] = m[0][2] - m2[0][2];
        r[0][3] = m[0][3] - m2[0][3];

        r[1][0] = m[1][0] - m2[1][0];
        r[1][1] = m[1][1] - m2[1][1];
        r[1][2] = m[1][2] - m2[1][2];
        r[1][3] = m[1][3] - m2[1][3];

        r[2][0] = m[2][0] - m2[2][0];
        r[2][1] = m[2][1] - m2[2][1];
        r[2][2] = m[2][2] - m2[2][2];
        r[2][3] = m[2][3] - m2[2][3];

        r[3][0] = m[3][0] - m2[3][0];
        r[3][1] = m[3][1] - m2[3][1];
        r[3][2] = m[3][2] - m2[3][2];
        r[3][3] = m[3][3] - m2[3][3];

        return r;
    }

    inline auto operator*(const Matrix4 &m, const Matrix4 &m2) -> Matrix4
    {
        Matrix4 r;
        r[0][0] = m[0][0] * m2[0][0] + m[0][1] * m2[1][0] + m[0][2] * m2[2][0] + m[0][3] * m2[3][0];
        r[0][1] = m[0][0] * m2[0][1] + m[0][1] * m2[1][1] + m[0][2] * m2[2][1] + m[0][3] * m2[3][1];
        r[0][2] = m[0][0] * m2[0][2] + m[0][1] * m2[1][2] + m[0][2] * m2[2][2] + m[0][3] * m2[3][2];
        r[0][3] = m[0][0] * m2[0][3] + m[0][1] * m2[1][3] + m[0][2] * m2[2][3] + m[0][3] * m2[3][3];

        r[1][0] = m[1][0] * m2[0][0] + m[1][1] * m2[1][0] + m[1][2] * m2[2][0] + m[1][3] * m2[3][0];
        r[1][1] = m[1][0] * m2[0][1] + m[1][1] * m2[1][1] + m[1][2] * m2[2][1] + m[1][3] * m2[3][1];
        r[1][2] = m[1][0] * m2[0][2] + m[1][1] * m2[1][2] + m[1][2] * m2[2][2] + m[1][3] * m2[3][2];
        r[1][3] = m[1][0] * m2[0][3] + m[1][1] * m2[1][3] + m[1][2] * m2[2][3] + m[1][3] * m2[3][3];

        r[2][0] = m[2][0] * m2[0][0] + m[2][1] * m2[1][0] + m[2][2] * m2[2][0] + m[2][3] * m2[3][0];
        r[2][1] = m[2][0] * m2[0][1] + m[2][1] * m2[1][1] + m[2][2] * m2[2][1] + m[2][3] * m2[3][1];
        r[2][2] = m[2][0] * m2[0][2] + m[2][1] * m2[1][2] + m[2][2] * m2[2][2] + m[2][3] * m2[3][2];
        r[2][3] = m[2][0] * m2[0][3] + m[2][1] * m2[1][3] + m[2][2] * m2[2][3] + m[2][3] * m2[3][3];

        r[3][0] = m[3][0] * m2[0][0] + m[3][1] * m2[1][0] + m[3][2] * m2[2][0] + m[3][3] * m2[3][0];
        r[3][1] = m[3][0] * m2[0][1] + m[3][1] * m2[1][1] + m[3][2] * m2[2][1] + m[3][3] * m2[3][1];
        r[3][2] = m[3][0] * m2[0][2] + m[3][1] * m2[1][2] + m[3][2] * m2[2][2] + m[3][3] * m2[3][2];
        r[3][3] = m[3][0] * m2[0][3] + m[3][1] * m2[1][3] + m[3][2] * m2[2][3] + m[3][3] * m2[3][3];

        return r;
    }
    inline auto operator*(const Affine3 &m, const Affine3 &m2) -> Affine3
    {
        return {
            m[0][0] * m2[0][0] + m[0][1] * m2[1][0] + m[0][2] * m2[2][0],
            m[0][0] * m2[0][1] + m[0][1] * m2[1][1] + m[0][2] * m2[2][1],
            m[0][0] * m2[0][2] + m[0][1] * m2[1][2] + m[0][2] * m2[2][2],
            m[0][0] * m2[0][3] + m[0][1] * m2[1][3] + m[0][2] * m2[2][3] + m[0][3],

            m[1][0] * m2[0][0] + m[1][1] * m2[1][0] + m[1][2] * m2[2][0],
            m[1][0] * m2[0][1] + m[1][1] * m2[1][1] + m[1][2] * m2[2][1],
            m[1][0] * m2[0][2] + m[1][1] * m2[1][2] + m[1][2] * m2[2][2],
            m[1][0] * m2[0][3] + m[1][1] * m2[1][3] + m[1][2] * m2[2][3] + m[1][3],

            m[2][0] * m2[0][0] + m[2][1] * m2[1][0] + m[2][2] * m2[2][0],
            m[2][0] * m2[0][1] + m[2][1] * m2[1][1] + m[2][2] * m2[2][1],
            m[2][0] * m2[0][2] + m[2][1] * m2[1][2] + m[2][2] * m2[2][2],
            m[2][0] * m2[0][3] + m[2][1] * m2[1][3] + m[2][2] * m2[2][3] + m[2][3],

            0,
            0,
            0,
            1
        };
    }

    /** Vector transformation using '*'.
        @remarks
            Transforms the given 3-D vector by the matrix, projecting the
            result back into <i>w</i> = 1.
        @note
            This means that the initial <i>w</i> is considered to be 1.0,
            and then all the tree elements of the resulting 3-D vector are
            divided by the resulting <i>w</i>.
    */
    inline auto operator*(const Matrix4& m, const Vector3& v) -> Vector3
    {
        Vector3 r;

        Real fInvW = 1.0f / ( m[3][0] * v.x + m[3][1] * v.y + m[3][2] * v.z + m[3][3] );

        r.x = ( m[0][0] * v.x + m[0][1] * v.y + m[0][2] * v.z + m[0][3] ) * fInvW;
        r.y = ( m[1][0] * v.x + m[1][1] * v.y + m[1][2] * v.z + m[1][3] ) * fInvW;
        r.z = ( m[2][0] * v.x + m[2][1] * v.y + m[2][2] * v.z + m[2][3] ) * fInvW;

        return r;
    }
    /// @overload
    inline auto operator*(const Affine3& m,const Vector3& v) -> Vector3
    {
        return {
                m[0][0] * v.x + m[0][1] * v.y + m[0][2] * v.z + m[0][3],
                m[1][0] * v.x + m[1][1] * v.y + m[1][2] * v.z + m[1][3],
                m[2][0] * v.x + m[2][1] * v.y + m[2][2] * v.z + m[2][3]};
    }

    inline auto operator*(const Matrix4& m, const Vector4& v) -> Vector4
    {
        return {
            m[0][0] * v.x + m[0][1] * v.y + m[0][2] * v.z + m[0][3] * v.w,
            m[1][0] * v.x + m[1][1] * v.y + m[1][2] * v.z + m[1][3] * v.w,
            m[2][0] * v.x + m[2][1] * v.y + m[2][2] * v.z + m[2][3] * v.w,
            m[3][0] * v.x + m[3][1] * v.y + m[3][2] * v.z + m[3][3] * v.w};
    }
    inline auto operator*(const Affine3& m, const Vector4& v) -> Vector4
    {
        return {
            m[0][0] * v.x + m[0][1] * v.y + m[0][2] * v.z + m[0][3] * v.w,
            m[1][0] * v.x + m[1][1] * v.y + m[1][2] * v.z + m[1][3] * v.w,
            m[2][0] * v.x + m[2][1] * v.y + m[2][2] * v.z + m[2][3] * v.w,
            v.w};
    }

    inline auto operator * (const Vector4& v, const Matrix4& mat) -> Vector4
    {
        return {
            v.x*mat[0][0] + v.y*mat[1][0] + v.z*mat[2][0] + v.w*mat[3][0],
            v.x*mat[0][1] + v.y*mat[1][1] + v.z*mat[2][1] + v.w*mat[3][1],
            v.x*mat[0][2] + v.y*mat[1][2] + v.z*mat[2][2] + v.w*mat[3][2],
            v.x*mat[0][3] + v.y*mat[1][3] + v.z*mat[2][3] + v.w*mat[3][3]
            };
    }
    /** @} */
    /** @} */

}
#endif
