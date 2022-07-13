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


/**
    A number of invaluable references were used to put together this ps.1.x compiler for ATI_fragment_shader execution

    References:
        1. MSDN: DirectX 8.1 Reference
        2. Wolfgang F. Engel "Fundamentals of Pixel Shaders - Introduction to Shader Programming Part III" on gamedev.net
        3. Martin Ecker - XEngine
        4. Shawn Kirst - ps14toATIfs
        5. Jason L. Mitchell "Real-Time 3D Graphics With Pixel Shaders" 
        6. Jason L. Mitchell "1.4 Pixel Shaders"
        7. Jason L. Mitchell and Evan Hart "Hardware Shading with EXT_vertex_shader and ATI_fragment_shader"
        6. ATI 8500 SDK
        7. GL_ATI_fragment_shader extension reference

*/
//---------------------------------------------------------------------------
#ifndef ps_1_4H
#define ps_1_4H

#include <cstdio>
#include <vector>

#include "Compiler2Pass.hpp"
#include "glad/glad.h"

template<::std::size_t N>
auto constexpr inline ARRAYSIZE(auto const (& array)[N]) -> ::std::size_t
{   return N;   }

auto constexpr inline ALPHA_BIT = 0x08;
auto constexpr inline RGB_BITS = 0x07;

// Context key patterns
auto constexpr inline ckp_PS_BASE = 0x1;
auto constexpr inline ckp_PS_1_1 = 0x2;
auto constexpr inline ckp_PS_1_2 = 0x4;
auto constexpr inline ckp_PS_1_3 = 0x8;
auto constexpr inline ckp_PS_1_4 = 0x10;

auto constexpr inline ckp_PS_1_4_BASE = (ckp_PS_BASE + ckp_PS_1_4);




/** Subclasses Compiler2Pass to provide a ps_1_x compiler that takes DirectX pixel shader assembly
    and converts it to a form that can be used by ATI_fragment_shader OpenGL API
@remarks
    all ps_1_1, ps_1_2, ps_1_3, ps_1_4 assembly instructions are recognized but not all are passed
    on to ATI_fragment_shader.  ATI_fragment_shader does not have an equivalent directive for
    texkill or texdepth instructions.

    The user must provide the GL binding interfaces.

    A Test method is provided to verify the basic operation of the compiler which outputs the test
    results to a file.


*/
class PS_1_4 : public Compiler2Pass{
private:
    enum class RWAflags {rwa_NONE = 0, rwa_READ = 1, rwa_WRITE = 2};

    enum class MachineInstID {mi_COLOROP1, mi_COLOROP2, mi_COLOROP3, mi_ALPHAOP1, mi_ALPHAOP2,
                        mi_ALPHAOP3, mi_SETCONSTANTS, mi_PASSTEXCOORD, mi_SAMPLEMAP, mi_TEX,
                        mi_TEXCOORD, mi_TEXREG2RGB, mi_NOP
    };

    struct  TokenInstType{
      char* Name;
      GLuint ID;

    };

    struct RegisterUsage {
        bool Phase1Write;
        bool Phase2Write;
    };

    /// structure used to keep track of arguments and instruction parameters
    struct OpParram {
      GLuint Arg;       // type of argument
      bool Filled;      // has it been filled yet
      GLuint MaskRep;   // Mask/Replicator flags
      GLuint Mod;       // argument modifier
    };

    using MachineInstContainer = std::vector<MachineInstID>;
    //typedef MachineInstContainer::iterator MachineInstIterator;


    // there are 2 phases with 2 subphases each
    enum class PhaseType {ptPHASE1TEX, ptPHASE1ALU, ptPHASE2TEX, ptPHASE2ALU };

    struct RegModOffset {
        uint MacroOffset;
        uint RegisterBase;
        uint OpParramsIndex;
    };

    struct MacroRegModify {
        TokenInst *     Macro;
        uint            MacroSize;
        RegModOffset *  RegMods;
        uint            RegModSize;

    };

    static auto constexpr inline R_BASE = static_cast<unsigned int>((std::to_underlying(SymbolID::R0) - GL_REG_0_ATI));
    static auto constexpr inline C_BASE = static_cast<unsigned int>((std::to_underlying(SymbolID::C0) - GL_CON_0_ATI));
    static auto constexpr inline T_BASE = static_cast<unsigned int>((std::to_underlying(SymbolID::_1T0) - GL_REG_0_ATI));

    // static library database for tokens and BNF rules
    static SymbolDef PS_1_4_SymbolTypeLib[];
    static TokenRule PS_1_x_RulePath[];
    static bool LibInitialized;

    // Static Macro database for ps.1.1 ps.1.2 ps.1.3 instructions

    static TokenInst texreg2ar[];
    static RegModOffset texreg2xx_RegMods[];
    static MacroRegModify texreg2ar_MacroMods;

    static TokenInst texreg2gb[];
    static MacroRegModify texreg2gb_MacroMods;

    static TokenInst texdp3[];
    static RegModOffset texdp3_RegMods[];
    static MacroRegModify texdp3_MacroMods;

    static TokenInst texdp3tex[];
    static RegModOffset texdp3tex_RegMods[];
    static MacroRegModify texdp3tex_MacroMods;

    static TokenInst texm3x2pad[];
    static RegModOffset texm3xxpad_RegMods[];
    static MacroRegModify texm3x2pad_MacroMods;

    static TokenInst texm3x2tex[];
    static RegModOffset texm3xxtex_RegMods[];
    static MacroRegModify texm3x2tex_MacroMods;

    static TokenInst texm3x3pad[];
    static MacroRegModify texm3x3pad_MacroMods;

    static TokenInst texm3x3tex[];
    static MacroRegModify texm3x3tex_MacroMods;

    static TokenInst texm3x3spec[];
    static RegModOffset texm3x3spec_RegMods[];
    static MacroRegModify texm3x3spec_MacroMods;

    static TokenInst texm3x3vspec[];
    static RegModOffset texm3x3vspec_RegMods[];
    static MacroRegModify texm3x3vspec_MacroMods;


    MachineInstContainer mPhase1TEX_mi; /// machine instructions for phase one texture section
    MachineInstContainer mPhase1ALU_mi; /// machine instructions for phase one ALU section
    MachineInstContainer mPhase2TEX_mi; /// machine instructions for phase two texture section
    MachineInstContainer mPhase2ALU_mi; /// machine instructions for phase two ALU section

    // vars used during pass 2
    MachineInstID mOpType;
    SymbolID mOpInst;
    bool mDo_Alpha;
    PhaseType mInstructionPhase;
    int mArgCnt;
    int mConstantsPos;

    static auto constexpr inline MAXOPPARRAMS = 5; // max number of parrams bound to an instruction
    
    OpParram mOpParrams[MAXOPPARRAMS];

    /// keeps track of which registers are written to in each phase
    /// if a register is read from but has not been written to in phase 2
    /// then if it was written to in phase 1 perform a register pass function
    /// at the beginning of phase2 so that the register has something worthwhile in it
    /// NB: check ALU and TEX section of phase 1 and phase 2
    /// there are 6 temp registers r0 to r5 to keep track off
    /// checks are performed in pass 2 when building machine instructions
    RegisterUsage Phase_RegisterUsage[6];

    bool mMacroOn; // if true then put all ALU instructions in phase 1

    uint mTexm3x3padCount; // keep track of how many texm3x3pad instructions are used so know which mask to use

    size_t mLastInstructionPos; // keep track of last phase 2 ALU instruction to check for R0 setting
    size_t mSecondLastInstructionPos;

    // keep track if phase marker found: determines which phase the ALU instructions go into
    bool mPhaseMarkerFound; 

#ifdef _DEBUG
    FILE* fp;
    // full compiler test with output results going to a text file
    void testCompile(char* testname, char* teststr, SymbolID* testresult,
        uint testresultsize, GLuint* MachinInstResults = NULL, uint MachinInstResultsSize = 0);
#endif // _DEBUG


    /** attempt to build a machine instruction using current tokens
        determines what phase machine insturction should be in and if an Alpha Op is required
        calls expandMachineInstruction() to expand the token into machine instructions
    */
    auto BuildMachineInst() -> bool;
    
    void clearMachineInstState();

    auto setOpParram(const SymbolDef* symboldef) -> bool;

    /** optimizes machine instructions depending on pixel shader context
        only applies to ps.1.1 ps.1.2 and ps.1.3 since they use CISC instructions
        that must be transformed into RISC instructions
    */
    void optimize();

    // the method is expected to be recursive to allow for inline expansion of instructions if required
    auto Pass2scan(const TokenInst * Tokens, const size_t size) -> bool;

    // supply virtual functions for Compiler2Pass
    /// Pass 1 is completed so now take tokens generated and build machine instructions
    auto doPass2() -> bool override;

    /** Build a machine instruction from token and ready it for expansion
        will expand CISC tokens using macro database

    */
    auto bindMachineInstInPassToFragmentShader(const MachineInstContainer & PassMachineInstructions) -> bool;

    /** Expand CISC tokens into PS1_4 token equivalents

    */
    auto expandMacro(const MacroRegModify & MacroMod) -> bool;

    /** Expand Machine instruction into operation type and arguments and put into proper machine
        instruction container
        also expands scaler alpha machine instructions if required

    */
    auto expandMachineInstruction() -> bool;

    // mainly used by tests - too slow for use in binding
    auto getMachineInst(size_t Idx) -> MachineInstID;

    auto getMachineInstCount() -> size_t;

    void addMachineInst(PhaseType phase, const MachineInstID inst);
    void addMachineInst(PhaseType phase, unsigned int inst)
    {
        addMachineInst(phase, static_cast<MachineInstID>(inst));
    }

    void clearAllMachineInst();

    void updateRegisterWriteState(const PhaseType phase);

    auto isRegisterReadValid(const PhaseType phase, const int param) -> bool;

public:

    /// constructor
    PS_1_4();

    /// binds machine instructions generated in Pass 2 to the ATI GL fragment shader
    auto bindAllMachineInstToFragmentShader() -> bool;

#ifdef _DEBUG
    /// perform compiler tests - only available in _DEBUG mode
    void test();
    void testbinder();

#endif
};


#endif

