/*
-----------------------------------------------------------------------------
This source file is part of OGRE
(Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org

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
#ifndef OGRE_COMPONENTS_RTSHADERSYSTEM_FUNCTIONATOM_H
#define OGRE_COMPONENTS_RTSHADERSYSTEM_FUNCTIONATOM_H

#include <algorithm>
#include <compare>
#include <iosfwd>
#include <vector>

#include "OgreException.hpp"
#include "OgrePrerequisites.hpp"
#include "OgreShaderParameter.hpp"
#include "OgreShaderPrerequisites.hpp"

namespace Ogre::RTShader {

/** \addtogroup Optional
*  @{
*/
/** \addtogroup RTShader
*  @{
*/

/** A class that represents a function operand (its the combination of a parameter the in/out semantic and the used fields)
*/
class Operand : public RTShaderSystemAlloc
{
public:

    // InOut semantic
    enum class OpSemantic
    {
        /// The parameter is a input parameter
        IN,
        /// The parameter is a output parameter
        OUT,
        /// The parameter is a input/output parameter
        INOUT
    };

    // Used field mask
    enum class OpMask : uchar
    {
        NONE    = 0,
        X       = 0x0001,
        Y       = 0x0002,
        Z       = 0x0004,
        W       = 0x0008,
        XY      = X | Y,
        XZ      = X | Z,
        XW      = X | W,
        YZ      = Y | Z,
        YW      = Y | W,
        ZW      = Z | W,
        XYZ     = X | Y | Z,
        XYW     = X | Y | W,
        XZW     = X | Z | W,
        YZW     = Y | Z | W,
        XYZW    = X | Y | Z | W,
        ALL     = XYZW
    };

    friend auto constexpr operator not(OpMask value) -> bool
    {
        return not std::to_underlying(value);
    }

    friend auto constexpr operator bitand(OpMask left, OpMask right) -> OpMask
    {
        return static_cast<OpMask>
        (   std::to_underlying(left)
        bitand
            std::to_underlying(right)
        );
    }

    friend auto constexpr operator << (OpMask left, std::size_t shift) -> OpMask
    {
        return static_cast<OpMask>
        (   std::to_underlying(left)
        <<  shift
        );
    }

    friend auto constexpr operator >> (OpMask left, std::size_t shift) -> OpMask
    {
        return static_cast<OpMask>
        (   std::to_underlying(left)
        >>  shift
        );
    }

    /** Class constructor 
    @param parameter A function parameter.
    @param opSemantic The in/out semantic of the parameter.
    @param opMask The field mask of the parameter.
    @param indirectionLevel
    */
    Operand(ParameterPtr parameter, OpSemantic opSemantic, OpMask opMask = OpMask::ALL, ushort indirectionLevel = 0);

    /** Copy constructor */
    Operand(const Operand& rhs);

    /** Copy the given Operand to this Operand.
    @param rhs The other Operand to copy to this state.
    */
    auto operator= (const Operand & rhs) -> Operand&;

    /** Class destructor */
    ~Operand() = default;

    /** Returns the parameter object as weak reference */
    [[nodiscard]] auto getParameter() const noexcept -> const ParameterPtr& { return mParameter; }

    /** Returns true if not all fields used. (usage is described through semantic)*/
    [[nodiscard]] auto hasFreeFields()    const -> bool { return mMask != OpMask::ALL; }
    
    /** Returns the mask bitfield. */
    [[nodiscard]] auto getMask()   const noexcept -> OpMask { return mMask; }

    auto x() -> Operand& { return mask(OpMask::X); }
    auto y() -> Operand& { return mask(OpMask::Y); }
    auto z() -> Operand& { return mask(OpMask::Z); }
    auto w() -> Operand& { return mask(OpMask::W); }
    auto xy() -> Operand& { return mask(OpMask::XY); }
    auto xyz() -> Operand& { return mask(OpMask::XYZ); }

    auto mask(OpMask opMask) -> Operand&
    {
        mMask = opMask;
        return *this;
    }

    /// automatically set swizzle to match parameter arity
    void setMaskToParamType();

    /** Returns the operand semantic (do we read/write or both with the parameter). */
    [[nodiscard]] auto getSemantic()    const noexcept -> OpSemantic { return mSemantic; }

    /** Returns the level of indirection. 
    The greater the indirection level the more the parameter needs to be nested in brackets.
    For example given 4 parameters x1...x4 with the indirections levels 0,1,1,2 
    respectively. The parameters should form the following string: x1[x2][x3[x4]].
    */
    [[nodiscard]] auto getIndirectionLevel() const noexcept -> ushort { return mIndirectionLevel; }

    /** write the parameter name and the usage mask like this 'color.xyz' */
    void write(std::ostream& os) const;

    /** Return the float count of the given mask. */
    static auto getFloatCount(OpMask mask) -> int;
protected:
    /// The parameter being carried by the operand
    ParameterPtr mParameter;
    /// Tells if the parameter is of type input,output or both
    OpSemantic mSemantic;
    /// Which part of the parameter should be passed (x,y,z,w)
    OpMask mMask;
    /// The level of indirection. @see getIndirectionLevel
    ushort mIndirectionLevel;
};

struct In : Operand 
{
    In(const Operand& rhs) : Operand(rhs) { OgreAssert(mSemantic == OpSemantic::IN, "invalid semantic"); }
    In(ParameterPtr p) : Operand(p, OpSemantic::IN) {}
    In(UniformParameterPtr p) : Operand(p, OpSemantic::IN) {}

    // implicitly construct const params
    In(float f) : Operand(ParameterFactory::createConstParam(f), OpSemantic::IN) {}
    In(const Vector2& v) : Operand(ParameterFactory::createConstParam(v), OpSemantic::IN) {}
    In(const Vector3& v) : Operand(ParameterFactory::createConstParam(v), OpSemantic::IN) {}
    In(const Vector4& v) : Operand(ParameterFactory::createConstParam(v), OpSemantic::IN) {}
};

struct Out : Operand 
{
    Out(const Operand& rhs) : Operand(rhs) { OgreAssert(mSemantic == OpSemantic::OUT, "invalid semantic"); }
    Out(ParameterPtr p) : Operand(p, OpSemantic::OUT) {}
    Out(UniformParameterPtr p) : Operand(p, OpSemantic::OUT) {}
};

struct InOut : Operand 
{
    InOut(const Operand& rhs) : Operand(rhs) { OgreAssert(mSemantic == OpSemantic::INOUT, "invalid semantic"); }
    InOut(ParameterPtr p) : Operand(p, OpSemantic::INOUT) {}
    InOut(UniformParameterPtr p) : Operand(p, OpSemantic::INOUT) {}
};

/// shorthand for operator[]  on preceding operand. e.g. myArray[p]
struct At : Operand
{
    At(ParameterPtr p) : Operand(p, OpSemantic::IN, OpMask::ALL, 1) {}
};

/** A class that represents an atomic code section of shader based program function.
*/
class FunctionAtom : public RTShaderSystemAlloc
{
// Interface.
public:
    using OperandVector = std::vector<Operand>;

    /** Class default destructor. */
    virtual ~FunctionAtom() = default;

    /** Get the group execution order of this function atom. */
    [[nodiscard]] auto getGroupExecutionOrder() const noexcept -> int;

    /** Get a list of parameters this function invocation will use in the function call as arguments. */
    auto getOperandList() noexcept -> OperandVector& { return mOperands; }

    /** Push a new operand (on the end) to the function.
    @param parameter A function parameter.
    @param opSemantic The in/out semantic of the parameter.
    @param opMask The field mask of the parameter.
    @param indirectionLevel The level of nesting inside brackets
    */
    void pushOperand(ParameterPtr parameter, Operand::OpSemantic opSemantic, Operand::OpMask opMask = Operand::OpMask::ALL, int indirectionLevel = 0);

    void setOperands(const OperandVector& ops);

    /** Abstract method that writes a source code to the given output stream in the target shader language. */
    virtual void writeSourceCode(std::ostream& os, std::string_view targetLanguage) const = 0;

// Attributes.
protected:
    /** Class default constructor. */
    FunctionAtom();

    void writeOperands(std::ostream& os, OperandVector::const_iterator begin, OperandVector::const_iterator end) const;

    // The owner group execution order.
    int mGroupExecutionOrder;
    OperandVector mOperands;
    String mFunctionName;
};

/** A class that represents function invocation code from shader based program function.
*/
class FunctionInvocation : public FunctionAtom
{
    // Interface.
public:
    /** Class constructor 
    @param functionName The name of the function to invoke.
    @param groupOrder The group order of this invocation.
    @param returnType The return type of the used function.
    */
    FunctionInvocation(std::string_view functionName, int groupOrder, std::string_view returnType = "void");

    /** Copy constructor */
    FunctionInvocation(const FunctionInvocation& rhs);

    /** 
    @see FunctionAtom::writeSourceCode
    */
    void writeSourceCode(std::ostream& os, std::string_view targetLanguage) const override;

    /** Return the function name */
    [[nodiscard]] auto getFunctionName() const noexcept -> std::string_view { return mFunctionName; }

    /** Return the return type */
    [[nodiscard]] auto getReturnType() const noexcept -> std::string_view { return mReturnType; }

    /** Determines if the current object is equal to the compared one. */
    [[nodiscard]] auto operator == ( const FunctionInvocation& rhs ) const noexcept -> bool
    {
        return (*this <=> rhs) == ::std::weak_ordering::equivalent;
    }

    /** Determines if the current object is less than the compared one. */
    [[nodiscard]] auto operator <=> ( const FunctionInvocation& rhs ) const noexcept -> ::std::weak_ordering;

private:
    FunctionInvocation() = default;

    String mReturnType;
};

/// shorthand for "lhs = rhs;" insted of using FFP_Assign(rhs, lhs)
class AssignmentAtom : public FunctionAtom
{
public:
    explicit AssignmentAtom(int groupOrder) { mGroupExecutionOrder = groupOrder; }
    /// @note the argument order is reversed comered to all other function invocations
    AssignmentAtom(const Out& lhs, const In& rhs, int groupOrder);
    void writeSourceCode(std::ostream& os, std::string_view targetLanguage) const override;
};

/// shorthand for "dst = texture(sampler, uv);" instead of using FFP_SampleTexture
class SampleTextureAtom : public FunctionAtom
{
public:
    explicit SampleTextureAtom(int groupOrder) { mGroupExecutionOrder = groupOrder; }
    SampleTextureAtom(const In& sampler, const In& texcoord, const Out& dst, int groupOrder);
    void writeSourceCode(std::ostream& os, std::string_view targetLanguage) const override;
};

/// shorthand for "dst = a OP b;"
class BinaryOpAtom : public FunctionAtom
{
    char mOp;
public:
    explicit BinaryOpAtom(char op, int groupOrder) : mOp(op) { mGroupExecutionOrder = groupOrder; }
    BinaryOpAtom(char op, const In& a, const In& b, const Out& dst, int groupOrder);
    void writeSourceCode(std::ostream& os, std::string_view targetLanguage) const override;
};

using FunctionAtomInstanceList = std::vector<FunctionAtom *>;
using FunctionAtomInstanceIterator = FunctionAtomInstanceList::iterator;
using FunctionAtomInstanceConstIterator = FunctionAtomInstanceList::const_iterator;

/** @} */
/** @} */

}

#endif
