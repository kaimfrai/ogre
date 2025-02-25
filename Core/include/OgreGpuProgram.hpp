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
module;

#include <cstddef>

export module Ogre.Core:GpuProgram;

export import :Platform;
export import :Prerequisites;
export import :Resource;
export import :SharedPtr;

export
namespace Ogre {
class ResourceManager;
struct GpuNamedConstants;

    /** \addtogroup Core
     *  @{
     */
    /** \addtogroup Resources
     *  @{
     */
    /** Enumerates the types of programs which can run on the GPU. */
    enum class GpuProgramType : uint8
    {
        VERTEX_PROGRAM = 0,
        FRAGMENT_PROGRAM,
        GEOMETRY_PROGRAM,
        DOMAIN_PROGRAM,
        HULL_PROGRAM,
        COMPUTE_PROGRAM,
        COUNT
    };

    /** Defines a program which runs on the GPU such as a vertex or fragment program.
        @remarks
        This class defines the low-level program in assembler code, the sort used to
        directly assemble into machine instructions for the GPU to execute. By nature,
        this means that the assembler source is rendersystem specific, which is why this
        is an abstract class - real instances are created through the RenderSystem.
        If you wish to use higher level shading languages like HLSL and Cg, you need to
        use the HighLevelGpuProgram class instead.
    */
    class GpuProgram : public Resource
    {
    protected:
    /// The name of the file to load source from (may be blank)
    String mFilename;
    /// The assembler source of the program (may be blank until file loaded)
    String mSource;
    /// Syntax code e.g. arbvp1, vs_2_0 etc
    String mSyntaxCode;
    /// The type of the program
    GpuProgramType mType{GpuProgramType::VERTEX_PROGRAM};
    /// Whether we need to load source from file or not
    bool mLoadFromFile{true};
    /// Does this (vertex) program include skeletal animation?
    bool mSkeletalAnimation{false};
    /// Does this (vertex) program include morph animation?
    bool mMorphAnimation{false};
    /// Does this (vertex) program require support for vertex texture fetch?
    bool mVertexTextureFetch{false};
    /// Does this (geometry) program require adjacency information?
    bool mNeedsAdjacencyInfo{false};
    /// Did we encounter a compilation error?
    bool mCompileError{false};
    bool mLoadedManualNamedConstants{false};
    /// The default parameters for use with this object
    GpuProgramParametersSharedPtr mDefaultParams;
    /** Record of logical to physical buffer maps. Mandatory for low-level
        programs or high-level programs which set their params the same way.
        This is a shared pointer because if the program is recompiled and the parameters
        change, this definition will alter, but previous params may reference the old def. */
    GpuLogicalBufferStructPtr mLogicalToPhysical;
    /** Parameter name -> ConstantDefinition map, shared instance used by all parameter objects.
        This is a shared pointer because if the program is recompiled and the parameters
        change, this definition will alter, but previous params may reference the old def.
    */
    GpuNamedConstantsPtr mConstantDefs;
    /// File from which to load named constants manually
    String mManualNamedConstantsFile;
    /// Does this (vertex) program include pose animation (count of number of poses supported)
    ushort mPoseAnimation{0};

    /** Internal method for setting up the basic parameter definitions for a subclass.
        @remarks
        Because StringInterface holds a dictionary of parameters per class, subclasses need to
        call this to ask the base class to add it's parameters to their dictionary as well.
        Can't do this in the constructor because that runs in a non-virtual context.
        @par
        The subclass must have called it's own createParamDictionary before calling this method.
    */
    virtual void setupBaseParamDictionary();

    /** Internal method returns whether required capabilities for this program is supported.
     */
    auto isRequiredCapabilitiesSupported() const noexcept -> bool;

    // catches errors during prepare
    void safePrepare();

    void prepareImpl() override;

    void loadImpl() override;

    void postLoadImpl() override;

    /// Create the internal params logical & named mapping structures
    void createParameterMappingStructures(bool recreateIfExists = true);
    /// Create the internal params logical mapping structures
    void createLogicalParameterMappingStructures(bool recreateIfExists = true);
    /// Create the internal params named mapping structures
    void createNamedParameterMappingStructures(bool recreateIfExists = true);

    public:

    GpuProgram(ResourceManager* creator, std::string_view name, ResourceHandle handle,
               std::string_view group, bool isManual = false, ManualResourceLoader* loader = nullptr);

    static auto getProgramTypeName(GpuProgramType programType) -> const String;

    ~GpuProgram() override = default;

    /** Sets the filename of the source assembly for this program.
        @remarks
        Setting this will have no effect until you (re)load the program.
    */
    void setSourceFile(std::string_view filename);

    /** Sets the source assembly for this program from an in-memory string.
        @remarks
        Setting this will have no effect until you (re)load the program.
    */
    void setSource(std::string_view source);

    /** Gets the syntax code for this program e.g. arbvp1, fp20, vs_1_1 etc */
    auto getSyntaxCode() const noexcept -> std::string_view { return mSyntaxCode; }

    /** Sets the syntax code for this program e.g. arbvp1, fp20, vs_1_1 etc */
    void setSyntaxCode(std::string_view syntax);

    /** Gets the name of the file used as source for this program. */
    auto getSourceFile() const noexcept -> std::string_view { return mFilename; }
    /** Gets the assembler source for this program. */
    virtual auto getSource() const noexcept -> std::string_view { return mSource; }
    /// Set the program type (only valid before load)
    void setType(GpuProgramType t);
    /// Get the program type
    auto getType() const noexcept -> GpuProgramType { return mType; }

    /** Returns the GpuProgram which should be bound to the pipeline.
        @remarks
        This method is simply to allow some subclasses of GpuProgram to delegate
        the program which is bound to the pipeline to a delegate, if required. */
    virtual auto _getBindingDelegate() noexcept -> GpuProgram* { return this; }

    /** Returns whether this program can be supported on the current renderer and hardware. */
    virtual auto isSupported() const noexcept -> bool;

    /** Creates a new parameters object compatible with this program definition.
        @remarks
        It is recommended that you use this method of creating parameters objects
        rather than going direct to GpuProgramManager, because this method will
        populate any implementation-specific extras (like named parameters) where
        they are appropriate.
    */
    virtual auto createParameters() -> GpuProgramParametersSharedPtr;

    /** Sets whether a vertex program includes the required instructions
        to perform skeletal animation.
        @remarks
        If this is set to true, OGRE will not blend the geometry according to
        skeletal animation, it will expect the vertex program to do it.
    */
    virtual void setSkeletalAnimationIncluded(bool included)
    { mSkeletalAnimation = included; }

    /** Returns whether a vertex program includes the required instructions
        to perform skeletal animation.
        @remarks
        If this returns true, OGRE will not blend the geometry according to
        skeletal animation, it will expect the vertex program to do it.
    */
    virtual auto isSkeletalAnimationIncluded() const noexcept -> bool { return mSkeletalAnimation; }

    /** Sets whether a vertex program includes the required instructions
        to perform morph animation.
        @remarks
        If this is set to true, OGRE will not blend the geometry according to
        morph animation, it will expect the vertex program to do it.
    */
    virtual void setMorphAnimationIncluded(bool included)
    { mMorphAnimation = included; }

    /** Sets whether a vertex program includes the required instructions
        to perform pose animation.
        @remarks
        If this is set to true, OGRE will not blend the geometry according to
        pose animation, it will expect the vertex program to do it.
        @param poseCount The number of simultaneous poses the program can blend
    */
    virtual void setPoseAnimationIncluded(ushort poseCount)
    { mPoseAnimation = poseCount; }

    /** Returns whether a vertex program includes the required instructions
        to perform morph animation.
        @remarks
        If this returns true, OGRE will not blend the geometry according to
        morph animation, it will expect the vertex program to do it.
    */
    virtual auto isMorphAnimationIncluded() const noexcept -> bool { return mMorphAnimation; }

    /** Returns whether a vertex program includes the required instructions
        to perform pose animation.
        @remarks
        If this returns true, OGRE will not blend the geometry according to
        pose animation, it will expect the vertex program to do it.
    */
    virtual auto isPoseAnimationIncluded() const noexcept -> bool { return mPoseAnimation > 0; }
    /** Returns the number of simultaneous poses the vertex program can
        blend, for use in pose animation.
    */
    virtual auto getNumberOfPosesIncluded() const noexcept -> ushort { return mPoseAnimation; }
    /** Sets whether this vertex program requires support for vertex
        texture fetch from the hardware.
    */
    virtual void setVertexTextureFetchRequired(bool r) { mVertexTextureFetch = r; }
    /** Returns whether this vertex program requires support for vertex
        texture fetch from the hardware.
    */
    virtual auto isVertexTextureFetchRequired() const noexcept -> bool { return mVertexTextureFetch; }

    /// @deprecated use OperationType::DETAIL_ADJACENCY_BIT
    virtual void setAdjacencyInfoRequired(bool r) { mNeedsAdjacencyInfo = r; }
    /// @deprecated use OperationType::DETAIL_ADJACENCY_BIT
    virtual auto isAdjacencyInfoRequired() const noexcept -> bool { return mNeedsAdjacencyInfo; }

    /** Get a reference to the default parameters which are to be used for all
        uses of this program.
        @remarks
        A program can be set up with a list of default parameters, which can save time when
        using a program many times in a material with roughly the same settings. By
        retrieving the default parameters and populating it with the most used options,
        any new parameter objects created from this program afterwards will automatically include
        the default parameters; thus users of the program need only change the parameters
        which are unique to their own usage of the program.
    */
    virtual auto getDefaultParameters() noexcept -> const GpuProgramParametersPtr&;

    /** Returns true if default parameters have been set up.
     */
    virtual auto hasDefaultParameters() const -> bool { return mDefaultParams.get() != nullptr; }

    /** Returns whether a vertex program wants light and material states to be passed
        through fixed pipeline low level API rendering calls (default false, subclasses can override)
        @remarks
        Most vertex programs do not need this material information, however GLSL
        shaders can refer to this material and lighting state so enable this option
    */
    virtual auto getPassSurfaceAndLightStates() const noexcept -> bool { return false; }

    /** Returns whether a fragment program wants fog state to be passed
        through fixed pipeline low level API rendering calls (default true, subclasses can override)
        @remarks
        On DirectX, shader model 2 and earlier continues to have fixed-function fog
        applied to it, so fog state is still passed (you should disable fog on the
        pass if you want to perform fog in the shader). In OpenGL it is also
        common to be able to access the fixed-function fog state inside the shader.
    */
    virtual auto getPassFogStates() const noexcept -> bool { return true; }

    /** Returns whether a vertex program wants transform state to be passed
        through fixed pipeline low level API rendering calls
        @remarks
        Most vertex programs do not need fixed-function transform information, however GLSL
        shaders can refer to this state so enable this option
    */
    virtual auto getPassTransformStates() const noexcept -> bool { return false; }

    /** Returns a string that specifies the language of the gpu programs as specified
        in a material script. ie: asm, cg, hlsl, glsl
    */
    virtual auto getLanguage() const noexcept -> std::string_view ;

    /** Did this program encounter a compile error when loading?
     */
    virtual auto hasCompileError() const noexcept -> bool { return mCompileError; }

    /** Reset a compile error if it occurred, allowing the load to be retried
     */
    virtual void resetCompileError() { mCompileError = false; }

    /** Allows you to manually provide a set of named parameter mappings
        to a program which would not be able to derive named parameters itself.
        @remarks
        You may wish to use this if you have assembler programs that were compiled
        from a high-level source, and want the convenience of still being able
        to use the named parameters from the original high-level source.
        @see setManualNamedConstantsFile
    */
    void setManualNamedConstants(const GpuNamedConstants& namedConstants);

    /** Specifies the name of a file from which to load named parameters mapping
        for a program which would not be able to derive named parameters itself.
        @remarks
        You may wish to use this if you have assembler programs that were compiled
        from a high-level source, and want the convenience of still being able
        to use the named parameters from the original high-level source. This
        method will make a low-level program search in the resource group of the
        program for the named file from which to load parameter names from.
        The file must be in the format produced by GpuNamedConstants::save.
    */
    void setManualNamedConstantsFile(std::string_view paramDefFile);

    /** Gets the name of a file from which to load named parameters mapping
        for a program which would not be able to derive named parameters itself.
    */
    auto getManualNamedConstantsFile() const noexcept -> std::string_view { return mManualNamedConstantsFile; }
    /** Get the full list of named constants.
        @note
        Only available if this parameters object has named parameters, which means either
        a high-level program which loads them, or a low-level program which has them
        specified manually.
    */
    virtual auto getConstantDefinitions() noexcept -> const GpuNamedConstants& { return *mConstantDefs.get(); }

    auto calculateSize() const -> size_t override;

    /// internal method to get the microcode cache id
    auto _getHash(uint32 seed = 0) const -> uint32;

    protected:
    /// Virtual method which must be implemented by subclasses, load from mSource
    virtual void loadFromSource() = 0;

    };

    inline auto to_string(const GpuProgramType& v) -> String { return GpuProgram::getProgramTypeName(v); }
    /** @} */
    /** @} */
}
