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

#include <algorithm>
#include <compare>
#include <cstddef>
#include <format>
#include <memory>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

#include "OgreException.hpp"
#include "OgreGpuProgramParams.hpp"
#include "OgrePrerequisites.hpp"
#include "OgreShaderFunctionAtom.hpp"
#include "OgreShaderParameter.hpp"
#include "OgreShaderPrerequisites.hpp"
#include "OgreString.hpp"

namespace Ogre::RTShader {
//-----------------------------------------------------------------------------
Operand::Operand(ParameterPtr parameter, OpSemantic opSemantic, OpMask opMask, ushort indirectionLevel) : mParameter(parameter), mSemantic(opSemantic), mMask(opMask), mIndirectionLevel(indirectionLevel)
{
    // delay null check until FunctionInvocation
    if(parameter)
        parameter->setUsed(true);
}
//-----------------------------------------------------------------------------
Operand::Operand(const Operand& other) 
{
    *this = other;
}
//-----------------------------------------------------------------------------
auto Operand::operator= (const Operand & other) -> Operand&
{
    if (this != &other) 
    {
        mParameter = other.mParameter;
        mSemantic = other.mSemantic;
        mMask = other.mMask;
        mIndirectionLevel = other.mIndirectionLevel;
    }       
    return *this;
}

void Operand::setMaskToParamType()
{
    switch (mParameter->getType())
    {
    case GCT_FLOAT1:
        mMask = OPM_X;
        break;
    case GCT_FLOAT2:
        mMask = OPM_XY;
        break;
    case GCT_FLOAT3:
        mMask = OPM_XYZ;
        break;
    default:
        mMask = OPM_ALL;
        break;
    }
}

//-----------------------------------------------------------------------------
static void writeMask(std::ostream& os, int mask)
{
    if (mask != Operand::OPM_ALL)
    {
        os << '.';
        if (mask & Operand::OPM_X)
        {
            os << 'x';
        }

        if (mask & Operand::OPM_Y)
        {
            os << 'y';
        }

        if (mask & Operand::OPM_Z)
        {
            os << 'z';
        }

        if (mask & Operand::OPM_W)
        {
            os << 'w';
        }
    }
}

//-----------------------------------------------------------------------------
auto Operand::getFloatCount(int mask) -> int
{
    int floatCount = 0;

    while (mask != 0)
    {
        if ((mask & Operand::OPM_X) != 0)
        {
            floatCount++;

        }           
        mask = mask >> 1;
    }

    return floatCount;
}

//-----------------------------------------------------------------------------
void Operand::write(std::ostream& os) const
{
    os << mParameter->toString();
    writeMask(os, mMask);
}

//-----------------------------------------------------------------------------
FunctionAtom::FunctionAtom()
{
    mGroupExecutionOrder   = -1;
}

//-----------------------------------------------------------------------------
auto FunctionAtom::getGroupExecutionOrder() const noexcept -> int
{
    return mGroupExecutionOrder;
}

//-----------------------------------------------------------------------
FunctionInvocation::FunctionInvocation(std::string_view functionName, int groupOrder,
                                       std::string_view returnType)
    : mReturnType(returnType)
{
    mFunctionName = functionName;
    mGroupExecutionOrder = groupOrder;
}

//-----------------------------------------------------------------------------
FunctionInvocation::FunctionInvocation(const FunctionInvocation& other) :
    mReturnType(other.mReturnType)
{
    mFunctionName = other.mFunctionName;
    mGroupExecutionOrder = other.mGroupExecutionOrder;
    
    for (const auto & mOperand : other.mOperands)
        mOperands.push_back(Operand(mOperand));
}

//-----------------------------------------------------------------------
void FunctionInvocation::writeSourceCode(std::ostream& os, std::string_view targetLanguage) const
{
    // Write function name.
    os << mFunctionName << "(";
    writeOperands(os, mOperands.begin(), mOperands.end());
    // Write function call closer.
    os << ");";
}

//-----------------------------------------------------------------------
static auto parameterNullMsg(std::string_view name, size_t pos) -> String
{
    return std::format("{}: parameter #{} is NULL", name, pos);
}

void FunctionAtom::pushOperand(ParameterPtr parameter, Operand::OpSemantic opSemantic, Operand::OpMask opMask, int indirectionLevel)
{
    if (!parameter)
        OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS, parameterNullMsg(mFunctionName, mOperands.size()));
    mOperands.push_back(Operand(parameter, opSemantic, opMask, indirectionLevel));
}

void FunctionAtom::setOperands(const OperandVector& ops)
{
    for (size_t i = 0; i < ops.size(); i++)
        if(!ops[i].getParameter())
            OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS, parameterNullMsg(mFunctionName, i));

    mOperands = ops;
}

void FunctionAtom::writeOperands(std::ostream& os, OperandVector::const_iterator begin,
                                       OperandVector::const_iterator end) const
{
    // Write parameters.
    ushort curIndLevel = 0;
    for (auto it = begin; it != end; )
    {
        it->write(os);
        ++it;

        ushort opIndLevel = 0;
        if (it != mOperands.end())
        {
            opIndLevel = it->getIndirectionLevel();
        }

        if (curIndLevel != 0)
        {
            os << ")";
        }
        if (curIndLevel < opIndLevel)
        {
            while (curIndLevel < opIndLevel)
            {
                ++curIndLevel;
                os << "[";
            }
        }
        else //if (curIndLevel >= opIndLevel)
        {
            while (curIndLevel > opIndLevel)
            {
                --curIndLevel;
                os << "]";
            }
            if (opIndLevel != 0)
            {
                os << "][";
            }
            else if (it != end)
            {
                os << ", ";
            }
        }
        if (curIndLevel != 0)
        {
            os << "int("; // required by GLSL
        }
    }
}

static auto getSwizzledSize(const Operand& op) -> uchar
{
    auto gct = op.getParameter()->getType();
    if (op.getMask() == Operand::OPM_ALL)
        return GpuConstantDefinition::getElementSize(gct, false);

    return Operand::getFloatCount(op.getMask());
}

auto FunctionInvocation::operator<=>(FunctionInvocation const& rhs) const noexcept -> ::std::weak_ordering
{
    // Check the function names first
    // Adding an exception to std::string sorting.  I feel that functions beginning with an underscore should be placed before
    // functions beginning with an alphanumeric character.  By default strings are sorted based on the ASCII value of each character.
    // Underscores have an ASCII value in between capital and lowercase characters.  This is why the exception is needed.
    auto const
        fSpecialCompare
        =   []  (char left, char right)
            {
                bool const leftUnderscore = left == '_';
                bool const rightUnderscore = right == '_';
                return not rightUnderscore and (leftUnderscore or left < right);
            }
    ;

    if  (   ::std::ranges::lexicographical_compare
            (   getFunctionName()
            ,   rhs.getFunctionName()
            ,   fSpecialCompare
            )
        )
        return ::std::strong_ordering::less;

    if  (   ::std::ranges::lexicographical_compare
            (   rhs.getFunctionName()
            ,   getFunctionName()
            ,   fSpecialCompare
            )
        )
        return ::std::strong_ordering::greater;

    // Next check the return type
    if  (   auto const returnType = getReturnType() <=> rhs.getReturnType()
        ;   returnType != ::std::strong_ordering::equal
        )
        return returnType;

    // Check the number of operands
    if  (   auto const operands = mOperands.size() <=> rhs.mOperands.size()
        ; operands != ::std::strong_ordering::equal
        )
        return operands;

    // Now that we've gotten past the two quick tests, iterate over operands
    // Check the semantic and type.  The operands must be in the same order as well.
    for (   auto itLHSOps = mOperands.begin(), itRHSOps = rhs.mOperands.begin()
        ;   itLHSOps != mOperands.end()
        and itRHSOps != rhs.mOperands.end()
        ; ++itLHSOps, ++itRHSOps
        )
    {
        if  (   auto const semantic = itLHSOps->getSemantic() <=> itRHSOps->getSemantic()
            ;   semantic != ::std::strong_ordering::equal
            )
            return semantic;

        uchar const leftType    = getSwizzledSize(*itLHSOps);
        uchar const rightType   = getSwizzledSize(*itRHSOps);

        if  (   auto const type = leftType <=> rightType
            ;   type != ::std::strong_ordering::equal
            )
            return type;
    }

    return ::std::strong_ordering::equal;
}

AssignmentAtom::AssignmentAtom(const Out& lhs, const In& rhs, int groupOrder) {
    // do this backwards for compatibility with FFP_FUNC_ASSIGN calls
    setOperands({rhs, lhs});
    mGroupExecutionOrder = groupOrder;
    mFunctionName = "assign";
}

void AssignmentAtom::writeSourceCode(std::ostream& os, std::string_view targetLanguage) const
{
    auto outOp = mOperands.begin();
    // find the output operand
    while(outOp->getSemantic() != Operand::OPS_OUT)
        outOp++;
    writeOperands(os, outOp, mOperands.end());
    os << "\t=\t";
    writeOperands(os, mOperands.begin(), outOp);
    os << ";";
}

SampleTextureAtom::SampleTextureAtom(const In& sampler, const In& texcoord, const Out& lhs, int groupOrder)
{
    setOperands({sampler, texcoord, lhs});
    mGroupExecutionOrder = groupOrder;
    mFunctionName = "sampleTexture";
}

void SampleTextureAtom::writeSourceCode(std::ostream& os, std::string_view targetLanguage) const
{
    auto outOp = mOperands.begin();
    // find the output operand
    while(outOp->getSemantic() != Operand::OPS_OUT)
        outOp++;
    writeOperands(os, outOp, mOperands.end());
    os << "\t=\t";

    os << "texture";
    const auto& sampler = mOperands.front().getParameter();
    switch(sampler->getType())
    {
    case GCT_SAMPLER1D:
        os << "1D";
        break;
    case GCT_SAMPLER_EXTERNAL_OES:
    case GCT_SAMPLER2D:
        os << "2D";
        break;
    case GCT_SAMPLER3D:
        os << "3D";
        break;
    case GCT_SAMPLERCUBE:
        os << "Cube";
        break;
    default:
        OGRE_EXCEPT(Exception::ERR_INVALID_STATE, "unknown sampler");
        break;
    }

    os << "(";
    writeOperands(os, mOperands.begin(), outOp);
    os << ");";
}

BinaryOpAtom::BinaryOpAtom(char op, const In& a, const In& b, const Out& dst, int groupOrder) {
    // do this backwards for compatibility with FFP_FUNC_ASSIGN calls
    setOperands({a, b, dst});
    mGroupExecutionOrder = groupOrder;
    mOp = op;
    mFunctionName = op;
}

void BinaryOpAtom::writeSourceCode(std::ostream& os, std::string_view targetLanguage) const
{
    // find the output operand
    auto outOp = mOperands.begin();
    while(outOp->getSemantic() != Operand::OPS_OUT)
        outOp++;

    // find the second operand
    auto secondOp = ++(mOperands.begin());
    while(outOp->getIndirectionLevel() != 0)
        secondOp++;

    writeOperands(os, outOp, mOperands.end());
    os << "\t=\t";
    writeOperands(os, mOperands.begin(), secondOp);
    os << mOp;
    writeOperands(os, secondOp, outOp);
    os << ";";
}

}
