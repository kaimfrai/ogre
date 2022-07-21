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
module Ogre.RenderSystems.GL:atifs.Compiler2Pass;

import <vector>;

// FIX ME - should not be hard coded
enum {
BAD_TOKEN = 999
};

using uint = unsigned int;

// Token ID enumeration
enum class SymbolID : uint {
    // Terminal Tokens section

    // DirectX pixel shader source formats
    PS_1_4, PS_1_1, PS_1_2, PS_1_3,

    // PS_BASE
    C0, C1, C2, C3, C4, C5, C6, C7,
    V0, V1,
    ADD, SUB, MUL, MAD, LRP, MOV, CMP, CND,
    DP3, DP4, DEF,
    R, RA, G, GA, B, BA, A, RGBA, RGB,
    RG, RGA, RB, RBA, GB, GBA,
    RRRR, GGGG, BBBB, AAAA,
    X2, X4, D2, SAT,
    BIAS, INVERT, NEGATE, BX2,
    COMMA, VALUE,

    //PS_1_4 sid
    R0, R1, R2, R3, R4, R5,
    T0, T1, T2, T3, T4, T5,
    DP2ADD,
    X8, D8, D4,
    TEXCRD, TEXLD,
    STR, STQ,
    STRDR, STQDQ,
    BEM,
    PHASE,

    //PS_1_1 sid
    _1R0, _1R1, _1T0, _1T1, _1T2, _1T3,
    TEX, TEXCOORD, TEXM3X2PAD,
    TEXM3X2TEX, TEXM3X3PAD, TEXM3X3TEX, TEXM3X3SPEC, TEXM3X3VSPEC,
    TEXREG2AR, TEXREG2GB,

    //PS_1_2 side
    TEXREG2RGB, TEXDP3, TEXDP3TEX,

    // common
    SKIP, PLUS,

    // non-terminal tokens section
    PROGRAM, PROGRAMTYPE, DECLCONSTS, DEFCONST,
    CONSTANT, COLOR,
    TEXSWIZZLE, UNARYOP,
    NUMVAL, SEPERATOR, ALUOPS, TEXMASK, TEXOP_PS1_1_3,
    TEXOP_PS1_4,
    ALU_STATEMENT, DSTMODSAT, UNARYOP_ARGS, REG_PS1_4,
    TEX_PS1_4, REG_PS1_1_3, TEX_PS1_1_3, DSTINFO,
    SRCINFO, BINARYOP_ARGS, TERNARYOP_ARGS, TEMPREG,
    DSTMASK, PRESRCMOD, SRCNAME, SRCREP, POSTSRCMOD,
    DSTMOD, DSTSAT, BINARYOP,  TERNARYOP,
    TEXOPS_PHASE1, COISSUE, PHASEMARKER, TEXOPS_PHASE2,
    TEXREG_PS1_4, TEXOPS_PS1_4, TEXOPS_PS1_1_3, TEXCISCOP_PS1_1_3,


    // last token
    INVALID = BAD_TOKEN // must be last in enumeration
};

/** Compiler2Pass is a generic compiler/assembler
@remarks
    provides a tokenizer in pass 1 and relies on the subclass to provide the virtual method for pass 2

    PASS 1 - tokenize source: this is a simple brute force lexical scanner/analyzer that also parses
             the formed token for proper semantics and context in one pass
             it uses Look Ahead Left-Right (LALR) ruling based on Backus - Naur From notation for semantic
             checking and also performs context checking allowing for language dialects

    PASS 2 - generate application specific instructions ie native instructions

@par
    this class must be subclassed with the subclass providing implementation for Pass 2.  The subclass
    is responsible for setting up the token libraries along with defining the language syntax.

*/
class Compiler2Pass {

public:
    // BNF operation types
    enum class OperationType {otRULE, otAND, otOR, otOPTIONAL, otREPEAT, otEND};


    /** structure used to build rule paths

    */
    struct TokenRule {
        OperationType mOperation;
        SymbolID mTokenID;
        const char* mSymbol;
        uint mErrorID;

    };

    /** structure used to build Symbol Type library */
    struct SymbolDef {
      SymbolID mID;                 // Token ID which is the index into the Token Type library
      uint mPass2Data;          // data used by pass 2 to build native instructions

      uint mContextKey;         // context key to fit the Active Context
      uint mContextPatternSet;  // new pattern to set for Active Context bits
      uint mContextPatternClear;// Contexts bits to clear Active Context bits

      int mDefTextID;           // index into text table for default name : set at runtime
      uint mRuleID;             // index into Rule database for non-terminal took rulepath
                                // if RuleID is zero the token is terminal

    };


    /** structure for Token instructions */
    struct TokenInst {
      SymbolID mNTTRuleID;          // Non-Terminal Token Rule ID that generated Token
      SymbolID mID;                 // Token ID
      int mLine;                // line number in source code where Token was found
      int mPos;                 // Character position in source where Token was found

    };

    using TokenInstContainer = std::vector<TokenInst>;
    //typedef TokenInstContainer::iterator TokenInstIterator;

protected:
    /// container for Tokens extracted from source
    TokenInstContainer mTokenInstructions;

    /// pointer to the source to be compiled
    const char* mSource;
    int mEndOfSource;

    /// pointers to Text and Token Type libraries setup by subclass
    SymbolDef* mSymbolTypeLib;

    /// pointer to root rule path - has to be set by subclass constructor
    TokenRule* mRootRulePath;

    /// number of entries in Text and Token Type libraries
    int mRulePathLibCnt;
    int mSymbolTypeLibCnt;

    /// mVauleID needs to be initialized by the subclass before compiling occurs
    /// it defines the token ID used in the symbol type library
    SymbolID mValueID;


    /// storage container for constants defined in source
    std::vector<float> mConstants;

    /// Active Contexts pattern used in pass 1 to determine which tokens are valid for a certain context
    uint mActiveContexts;

    /** check token semantics between ID1 and ID2 using left/right semantic data in Token Type library
    @param ID1 token ID on the left
    @param ID2 token ID on the right
    @return
        true if both will bind to each other
        false if either fails the semantic bind test

    */
    //bool checkTokenSemantics(uint ID1, uint ID2);

    /** perform pass 1 of compile process
        scans source for symbols that can be tokenized and then
        performs general semantic and context verification on each symbol before it is tokenized.
        A tokenized instruction list is built to be used by Pass 2.

    */
    auto doPass1() -> bool;

    /** pure virtual method that must be set up by subclass to perform Pass 2 of compile process
    @remark
        Pass 2 is for the subclass to take the token instructions generated in Pass 1 and
        build the application specific instructions along with verifying
        symantic and context rules that could not be checked in Pass 1

    */
    virtual auto doPass2() -> bool = 0;

    void findEOL();

    /** get the text symbol for this token
    @remark
        mainly used for debugging and in test routines
    @param sid is the token ID
    @return a pointer to the string text
    */
    auto getTypeDefText(const uint sid) -> const char*;

    /** check to see if the text at the present position in the source is a numerical constant
    @param fvalue is a reference that will receive the float value that is in the source
    @param charsize reference to receive number of characters that make of the value in the source
    @return
        true if characters form a valid float representation
        false if a number value could not be extracted
    */
    auto isFloatValue(float & fvalue, int & charsize) -> bool;

    /** check to see if the text is in the symbol text library
    @param symbol points to beginning of text where a symbol token might exist
    @param symbolsize reference that will receive the size value of the symbol found
    @return
        true if a matching token could be found in the token type library
        false if could not be tokenized
    */
    auto isSymbol(const char* symbol, int & symbolsize) -> bool;


    /// position to the next possible valid sysmbol
    auto positionToNextSymbol() -> bool;


    /** process input source text using rulepath to determine allowed tokens
    @remarks
        the method is reentrant and recursive
        if a non-terminal token is encountered in the current rule path then the method is
        called using the new rule path referenced by the non-terminal token
        Tokens can have the following operation states which effects the flow path of the rule
            RULE: defines a rule path for the non-terminal token
            AND: the token is required for the rule to pass
            OR: if the previous tokens failed then try these ones
            OPTIONAL: the token is optional and does not cause the rule to fail if the token is not found
            REPEAT: the token is required but there can be more than one in a sequence
            END: end of the rule path - the method returns the succuss of the rule

    @param rulepathIDX index into to array of Token Rules that define a rule path to be processed
    @return
        true if rule passed - all required tokens found
        false if one or more tokens required to complete the rule were not found
    */
    auto processRulePath( uint rulepathIDX) -> bool;


    // setup ActiveContexts - should be called by subclass to setup initial language contexts
    void setActiveContexts(const uint contexts){ mActiveContexts = contexts; }


    /// comment specifiers are hard coded
    void skipComments();

    /// find end of line marker and move past it
    void skipEOL();

    /// skip all the white space which includes spaces and tabs
    void skipWhiteSpace();


    /** check if current position in source has the symbol text equivalent to the TokenID
    @param rulepathIDX index into rule path database of token to validate
    @param activeRuleID index of non-terminal rule that generated the token
    @return
        true if token was found
        false if token symbol text does not match the source text
        if token is non-terminal then processRulePath is called
    */
    auto ValidateToken(const uint rulepathIDX, const SymbolID activeRuleID) -> bool;


public:
    // ** these probably should not be public
    int mCurrentLine;
    int mCharPos;


    /// constructor
    Compiler2Pass();
    virtual ~Compiler2Pass() = default;
    /** compile the source - performs 2 passes
        first pass is to tokinize, check semantics and context
        second pass is performed by subclass and converts tokens to application specific instructions
    @remark
        Pass 2 only gets executed if Pass 1 has no errors
    @param source a pointer to the source text to be compiled
    @return
        true if Pass 1 and Pass 2 are successful
        false if any errors occur in Pass 1 or Pass 2
    */
    auto compile(const char* source) -> bool;

    /** Initialize the type library with matching symbol text found in symbol text library
        find a default text for all Symbol Types in library

        scan through all the rules and initialize TypeLib with index to text and index to rules for non-terminal tokens

        must be called by subclass after libraries and rule database setup
    */

    void InitSymbolTypeLib();

};
