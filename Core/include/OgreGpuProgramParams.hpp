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

#include <cassert>
#include <cstring>

export module Ogre.Core:GpuProgramParams;

export import :MemoryAllocatorConfig;
export import :Platform;
export import :Prerequisites;
export import :Serializer;
export import :SharedPtr;

export import <algorithm>;
export import <limits>;
export import <map>;
export import <string>;
export import <utility>;
export import <vector>;

export
namespace Ogre {
    struct TransformBaseReal;
    template <typename T> class ConstMapIterator;
class AutoParamDataSource;
struct ColourValue;
struct Matrix3;
struct Matrix4;
template <int dims, typename T> struct Vector;

    /** \addtogroup Core
     *  @{
     */
    /** \addtogroup Materials
     *  @{
     */

    enum class BaseConstantType
    {
        FLOAT = 0,
        INT = 0x10,
        DOUBLE = 0x20,
        UINT = 0x30,
        BOOL = 0x40,
        SAMPLER = 0x50,
        SPECIALIZATION = 0x60, //!< shader specialisation constant
        UNKNOWN = 0x70
    };

    /** Enumeration of the types of constant we may encounter in programs.
        @note Low-level programs, by definition, will always use either
        float4 or int4 constant types since that is the fundamental underlying
        type in assembler.
    */
    enum class GpuConstantType
    {
        FLOAT1 = std::to_underlying(BaseConstantType::FLOAT) + 1,
        FLOAT2 = std::to_underlying(BaseConstantType::FLOAT) + 2,
        FLOAT3 = std::to_underlying(BaseConstantType::FLOAT) + 3,
        FLOAT4 = std::to_underlying(BaseConstantType::FLOAT) + 4,
        SAMPLER1D = std::to_underlying(BaseConstantType::SAMPLER) + 1,
        SAMPLER2D = std::to_underlying(BaseConstantType::SAMPLER) + 2,
        SAMPLER3D = std::to_underlying(BaseConstantType::SAMPLER) + 3,
        SAMPLERCUBE = std::to_underlying(BaseConstantType::SAMPLER) + 4,
        SAMPLER1DSHADOW = std::to_underlying(BaseConstantType::SAMPLER) + 6,
        SAMPLER2DSHADOW = std::to_underlying(BaseConstantType::SAMPLER) + 7,
        SAMPLER2DARRAY = std::to_underlying(BaseConstantType::SAMPLER) + 8,
        SAMPLER_EXTERNAL_OES = std::to_underlying(BaseConstantType::SAMPLER) + 9,
        MATRIX_2X2 = std::to_underlying(BaseConstantType::FLOAT) + 5,
        MATRIX_2X3 = std::to_underlying(BaseConstantType::FLOAT) + 6,
        MATRIX_2X4 = std::to_underlying(BaseConstantType::FLOAT) + 7,
        MATRIX_3X2 = std::to_underlying(BaseConstantType::FLOAT) + 8,
        MATRIX_3X3 = std::to_underlying(BaseConstantType::FLOAT) + 9,
        MATRIX_3X4 = std::to_underlying(BaseConstantType::FLOAT) + 10,
        MATRIX_4X2 = std::to_underlying(BaseConstantType::FLOAT) + 11,
        MATRIX_4X3 = std::to_underlying(BaseConstantType::FLOAT) + 12,
        MATRIX_4X4 = std::to_underlying(BaseConstantType::FLOAT) + 13,
        INT1 = std::to_underlying(BaseConstantType::INT) + 1,
        INT2 = std::to_underlying(BaseConstantType::INT) + 2,
        INT3 = std::to_underlying(BaseConstantType::INT) + 3,
        INT4 = std::to_underlying(BaseConstantType::INT) + 4,
        SPECIALIZATION = std::to_underlying(BaseConstantType::SPECIALIZATION),
        DOUBLE1 = std::to_underlying(BaseConstantType::DOUBLE) + 1,
        DOUBLE2 = std::to_underlying(BaseConstantType::DOUBLE) + 2,
        DOUBLE3 = std::to_underlying(BaseConstantType::DOUBLE) + 3,
        DOUBLE4 = std::to_underlying(BaseConstantType::DOUBLE) + 4,
        MATRIX_DOUBLE_2X2 = std::to_underlying(BaseConstantType::DOUBLE) + 5,
        MATRIX_DOUBLE_2X3 = std::to_underlying(BaseConstantType::DOUBLE) + 6,
        MATRIX_DOUBLE_2X4 = std::to_underlying(BaseConstantType::DOUBLE) + 7,
        MATRIX_DOUBLE_3X2 = std::to_underlying(BaseConstantType::DOUBLE) + 8,
        MATRIX_DOUBLE_3X3 = std::to_underlying(BaseConstantType::DOUBLE) + 9,
        MATRIX_DOUBLE_3X4 = std::to_underlying(BaseConstantType::DOUBLE) + 10,
        MATRIX_DOUBLE_4X2 = std::to_underlying(BaseConstantType::DOUBLE) + 11,
        MATRIX_DOUBLE_4X3 = std::to_underlying(BaseConstantType::DOUBLE) + 12,
        MATRIX_DOUBLE_4X4 = std::to_underlying(BaseConstantType::DOUBLE) + 13,
        UINT1 = std::to_underlying(BaseConstantType::UINT) + 1,
        UINT2 = std::to_underlying(BaseConstantType::UINT) + 2,
        UINT3 = std::to_underlying(BaseConstantType::UINT) + 3,
        UINT4 = std::to_underlying(BaseConstantType::UINT) + 4,
        BOOL1 = std::to_underlying(BaseConstantType::BOOL) + 1,
        BOOL2 = std::to_underlying(BaseConstantType::BOOL) + 2,
        BOOL3 = std::to_underlying(BaseConstantType::BOOL) + 3,
        BOOL4 = std::to_underlying(BaseConstantType::BOOL) + 4,
        UNKNOWN = std::to_underlying(BaseConstantType::UNKNOWN)
    };

    auto constexpr operator + (GpuConstantType type, std::size_t offset) -> GpuConstantType
    {
        return static_cast<GpuConstantType>
        (   std::to_underlying(type)
        +   offset
        );
    }

    auto constexpr operator - (GpuConstantType type, std::size_t offset) -> GpuConstantType
    {
        return static_cast<GpuConstantType>
        (   std::to_underlying(type)
        -   offset
        );
    }

    auto constexpr operator compl (GpuConstantType type) -> GpuConstantType
    {
        return static_cast<GpuConstantType>
        (   compl
            std::to_underlying(type)
        );
    }

    auto constexpr operator not(GpuConstantType value) -> bool
    {
        return not std::to_underlying(value);
    }

    auto constexpr operator bitand (GpuConstantType left, GpuConstantType right) -> GpuConstantType
    {
        return static_cast<GpuConstantType>
        (   std::to_underlying(left)
        bitand
            std::to_underlying(right)
        );
    }

    /** The variability of a GPU parameter, as derived from auto-params targeting it.
        These values must be powers of two since they are used in masks.
    */
    enum class GpuParamVariability : uint16
    {
        /// No variation except by manual setting - the default
        GLOBAL = 1,
        /// Varies per object (based on an auto param usually), but not per light setup
        PER_OBJECT = 2,
        /// Varies with light setup
        LIGHTS = 4,
        /// Varies with pass iteration number
        PASS_ITERATION_NUMBER = 8,


        /// Full mask (16-bit)
        ALL = 0xFFFF
    };

    auto constexpr operator not (GpuParamVariability mask) -> bool
    {
        return not std::to_underlying(mask);
    }

    auto constexpr operator bitor(GpuParamVariability left, GpuParamVariability right) -> GpuParamVariability
    {
        return static_cast<GpuParamVariability>
        (   std::to_underlying(left)
        bitor
            std::to_underlying(right)
        );
    }

    auto constexpr operator |=(GpuParamVariability& left, GpuParamVariability right) -> GpuParamVariability&
    {
        return left = left bitor right;
    }

    auto constexpr operator bitand(GpuParamVariability left, GpuParamVariability right) -> GpuParamVariability
    {
        return static_cast<GpuParamVariability>
        (   std::to_underlying(left)
        bitand
            std::to_underlying(right)
        );
    }

    /** Information about predefined program constants.
        @note Only available for high-level programs but is referenced generically
        by GpuProgramParameters.
    */
    struct GpuConstantDefinition
    {
        /// Physical byte offset in buffer
        size_t physicalIndex{std::numeric_limits<size_t>::max()};
        /// Logical index - used to communicate this constant to the rendersystem
        size_t logicalIndex{0};
        /** Number of typed slots per element
            (some programs pack each array element to float4, some do not) */
        uint32 elementSize{0};
        /// Length of array
        uint32 arraySize{1};
        /// Data type
        GpuConstantType constType{GpuConstantType::UNKNOWN};
        /// How this parameter varies (bitwise combination of GpuProgramVariability)
        mutable GpuParamVariability variability{GpuParamVariability::GLOBAL};

        //TODO Should offset be added to list?
        // For instance, for GLSL atomic counters:
        // layout(binding = 1, offset = 10) atomic_uint atom_counter;
        // Binding goes in logicalIndex, but where does offset go?
        //size_t offset;

        auto isFloat() const noexcept -> bool { return isFloat(constType); }
        static auto isFloat(GpuConstantType c) -> bool { return getBaseType(c) == BaseConstantType::FLOAT; }

        auto isDouble() const noexcept -> bool { return isDouble(constType); }
        static auto isDouble(GpuConstantType c) -> bool { return getBaseType(c) == BaseConstantType::DOUBLE; }

        auto isInt() const noexcept -> bool { return isInt(constType); }
        static auto isInt(GpuConstantType c) -> bool { return getBaseType(c) == BaseConstantType::INT; }

        auto isUnsignedInt() const noexcept -> bool { return isUnsignedInt(constType); }
        static auto isUnsignedInt(GpuConstantType c) -> bool { return getBaseType(c) == BaseConstantType::UINT; }

        auto isBool() const noexcept -> bool { return isBool(constType); }
        static auto isBool(GpuConstantType c) -> bool { return getBaseType(c) == BaseConstantType::BOOL; }

        auto isSampler() const noexcept -> bool { return isSampler(constType); }
        static auto isSampler(GpuConstantType c) -> bool { return getBaseType(c) == BaseConstantType::SAMPLER; }

        auto isSpecialization() const noexcept -> bool { return isSpecialization(constType); }
        static auto isSpecialization(GpuConstantType c) -> bool { return getBaseType(c) == BaseConstantType::SPECIALIZATION; }

        static auto getBaseType(GpuConstantType ctype) -> BaseConstantType
        {
            return BaseConstantType(std::to_underlying(ctype) / 0x10 * 0x10);
        }

        /** Get the number of elements of a given type, including whether to pad the
            elements into multiples of 4 (e.g. SM1 and D3D does, GLSL doesn't)
        */
        static auto getElementSize(GpuConstantType ctype, bool padToMultiplesOf4) -> uint32
        {
            using enum GpuConstantType;
            if (padToMultiplesOf4)
            {
                switch(ctype)
                {
                case FLOAT1:
                case INT1:
                case UINT1:
                case BOOL1:
                case SAMPLER1D:
                case SAMPLER2D:
                case SAMPLER2DARRAY:
                case SAMPLER3D:
                case SAMPLERCUBE:
                case SAMPLER1DSHADOW:
                case SAMPLER2DSHADOW:
                case FLOAT2:
                case INT2:
                case UINT2:
                case BOOL2:
                case FLOAT3:
                case INT3:
                case UINT3:
                case BOOL3:
                case FLOAT4:
                case INT4:
                case UINT4:
                case BOOL4:
                    return 4;
                case MATRIX_2X2:
                case MATRIX_2X3:
                case MATRIX_2X4:
                case DOUBLE1:
                case DOUBLE2:
                case DOUBLE3:
                case DOUBLE4:
                    return 8; // 2 float4s
                case MATRIX_3X2:
                case MATRIX_3X3:
                case MATRIX_3X4:
                    return 12; // 3 float4s
                case MATRIX_4X2:
                case MATRIX_4X3:
                case MATRIX_4X4:
                case MATRIX_DOUBLE_2X2:
                case MATRIX_DOUBLE_2X3:
                case MATRIX_DOUBLE_2X4:
                    return 16; // 4 float4s
                case MATRIX_DOUBLE_3X2:
                case MATRIX_DOUBLE_3X3:
                case MATRIX_DOUBLE_3X4:
                    return 24;
                case MATRIX_DOUBLE_4X2:
                case MATRIX_DOUBLE_4X3:
                case MATRIX_DOUBLE_4X4:
                    return 32;
                default:
                    return 4;
                };
            }
            else
            {
                switch(ctype)
                {
                case SAMPLER1D:
                case SAMPLER2D:
                case SAMPLER2DARRAY:
                case SAMPLER3D:
                case SAMPLERCUBE:
                case SAMPLER1DSHADOW:
                case SAMPLER2DSHADOW:
                    return 1;
                case MATRIX_2X2:
                case MATRIX_DOUBLE_2X2:
                    return 4;
                case MATRIX_2X3:
                case MATRIX_3X2:
                case MATRIX_DOUBLE_2X3:
                case MATRIX_DOUBLE_3X2:
                    return 6;
                case MATRIX_2X4:
                case MATRIX_4X2:
                case MATRIX_DOUBLE_2X4:
                case MATRIX_DOUBLE_4X2:
                    return 8;
                case MATRIX_3X3:
                case MATRIX_DOUBLE_3X3:
                    return 9;
                case MATRIX_3X4:
                case MATRIX_4X3:
                case MATRIX_DOUBLE_3X4:
                case MATRIX_DOUBLE_4X3:
                    return 12;
                case MATRIX_4X4:
                case MATRIX_DOUBLE_4X4:
                    return 16;
                default:
                    return std::to_underlying(ctype) % 0x10;
                };

            }
        }

    };
    using GpuConstantDefinitionMap = std::map<std::string, GpuConstantDefinition, std::less<>>;
    using GpuConstantDefinitionIterator = ConstMapIterator<GpuConstantDefinitionMap>;

    /// Struct collecting together the information for named constants.
    struct GpuNamedConstants : public GpuParamsAlloc
    {
        /// Total size of the buffer required
        size_t bufferSize{0};
        /// Number of register type params (samplers)
        size_t registerCount{0};
        /// Map of parameter names to GpuConstantDefinition
        GpuConstantDefinitionMap map;

        /** Saves constant definitions to a file
         * compatible with @ref GpuProgram::setManualNamedConstantsFile.
         */
        void save(std::string_view filename) const;
        /** Loads constant definitions from a stream
         * compatible with @ref GpuProgram::setManualNamedConstantsFile.
         */
        void load(DataStreamPtr& stream);

        [[nodiscard]] auto calculateSize() const -> size_t;
    };

    /// Simple class for loading / saving GpuNamedConstants
    class GpuNamedConstantsSerializer : public Serializer
    {
    public:
        GpuNamedConstantsSerializer();
        virtual ~GpuNamedConstantsSerializer();
        void exportNamedConstants(const GpuNamedConstants* pConsts, std::string_view filename,
                                  std::endian endianMode = std::endian::native);
        void exportNamedConstants(const GpuNamedConstants* pConsts, DataStreamPtr stream,
                                  std::endian endianMode = std::endian::native);
        void importNamedConstants(DataStreamPtr& stream, GpuNamedConstants* pDest);
    };

    /** Structure recording the use of a physical buffer by a logical parameter
        index. Only used for low-level programs.
    */
    struct GpuLogicalIndexUse
    {
        /// Physical buffer index
        size_t physicalIndex{99999};
        /// Current physical size allocation
        size_t currentSize{0};
        /// How the contents of this slot vary
        mutable GpuParamVariability variability{GpuParamVariability::GLOBAL};
        /// Data type
        BaseConstantType baseType{BaseConstantType::UNKNOWN};

    };
    using GpuLogicalIndexUseMap = std::map<size_t, GpuLogicalIndexUse>;
    /// Container struct to allow params to safely & update shared list of logical buffer assignments
    struct GpuLogicalBufferStruct : public GpuParamsAlloc
    {
        /// Map from logical index to physical buffer location
        GpuLogicalIndexUseMap map;
        /// Shortcut to know the buffer size needs
        size_t bufferSize{0};
    };

    /** Definition of container that holds the current constants.
        @note Not necessarily in direct index order to constant indexes, logical
        to physical index map is derived from GpuProgram
    */
    using ConstantList = std::vector<uchar>;

    /** A group of manually updated parameters that are shared between many parameter sets.
        @remarks
        Sometimes you want to set some common parameters across many otherwise
        different parameter sets, and keep them all in sync together. This class
        allows you to define a set of parameters that you can share across many
        parameter sets and have the parameters that match automatically be pulled
        from the shared set, rather than you having to set them on all the parameter
        sets individually.
        @par
        Parameters in a shared set are matched up with instances in a GpuProgramParameters
        structure by matching names. It is up to you to define the named parameters
        that a shared set contains, and ensuring the definition matches.
        @note
        Shared parameter sets can be named, and looked up using the GpuProgramManager.
    */
    class GpuSharedParameters : public GpuParamsAlloc
    {
        /// Name of the shared parameter set.
        String mName;

        /// Shared parameter definitions and related data.
        GpuNamedConstants mNamedConstants;

        /// List of constant values.
        ConstantList mConstants;

        /// Optional rendersystem backed storage
        HardwareBufferPtr mHardwareBuffer;

        /// Version number of the definitions in this buffer.
        uint32 mVersion{0};

		/// Accumulated offset used to calculate uniform location.
		size_t mOffset{0};

        bool mDirty{false};

    public:
        GpuSharedParameters(std::string_view name);

        /// Get the name of this shared parameter set.
        auto getName() noexcept -> std::string_view { return mName; }

        /** Add a new constant definition to this shared set of parameters.
            @remarks
            Unlike GpuProgramParameters, where the parameter list is defined by the
            program being compiled, this shared parameter set is defined by the
            user. Only parameters which have been predefined here may be later
            updated.
        */
        void addConstantDefinition(std::string_view name, GpuConstantType constType, uint32 arraySize = 1);

        /** Remove a constant definition from this shared set of parameters.
         */
        void removeAllConstantDefinitions();

        /** Get the version number of this shared parameter set, can be used to identify when
            changes have occurred.
        */
        [[nodiscard]] auto getVersion() const noexcept -> uint32 { return mVersion; }

        /** Calculate the expected size of the shared parameter buffer based
            on constant definition data types.
        */
        [[nodiscard]] auto calculateSize() const -> size_t;

        /** True if this parameter set is dirty (values have been modified,
            but the render system has not updated them yet).
        */
        [[nodiscard]] auto isDirty() const noexcept -> bool { return mDirty; }

        /** Mark the shared set as being clean (values successfully updated
            by the render system).
            @remarks
            You do not need to call this yourself. The set is marked as clean
            whenever the render system updates dirty shared parameters.
        */
        void _markClean();

        /** Mark the shared set as being dirty (values modified and not yet
            updated in render system).
            @remarks
            You do not need to call this yourself. The set is marked as
            dirty whenever setNamedConstant or (non const) getFloatPointer
            et al are called.
        */
        void _markDirty();

        /** Get a specific GpuConstantDefinition for a named parameter.
         */
        [[nodiscard]] auto getConstantDefinition(std::string_view name) const -> const GpuConstantDefinition&;

        /** Get the full list of GpuConstantDefinition instances.
         */
        [[nodiscard]] auto getConstantDefinitions() const noexcept -> const GpuNamedConstants&;

        /** @copydoc GpuProgramParameters::setNamedConstant(std::string_view , Real) */
        template <typename T> void setNamedConstant(std::string_view name, T val)
        {
            setNamedConstant(name, &val, 1);
        }
        /// @overload
        template <int dims, typename T>
        void setNamedConstant(std::string_view name, const Vector<dims, T>& vec)
        {
            setNamedConstant(name, vec.ptr(), dims);
        }
        /** @copydoc GpuProgramParameters::setNamedConstant(std::string_view name, const Matrix4& m) */
        void setNamedConstant(std::string_view name, const Matrix4& m);
        /** @copydoc GpuProgramParameters::setNamedConstant(std::string_view name, const Matrix4* m, size_t numEntries) */
        void setNamedConstant(std::string_view name, const Matrix4* m, uint32 numEntries);
        void setNamedConstant(std::string_view name, const float *val, uint32 count);
        void setNamedConstant(std::string_view name, const double *val, uint32 count);
        /** @copydoc GpuProgramParameters::setNamedConstant(std::string_view name, const ColourValue& colour) */
        void setNamedConstant(std::string_view name, const ColourValue& colour);
        void setNamedConstant(std::string_view name, const int *val, uint32 count);
        void setNamedConstant(std::string_view name, const uint *val, uint32 count);
        /// Get a pointer to the 'nth' item in the float buffer
        auto getFloatPointer(size_t pos) -> float* { _markDirty(); return (float*)&mConstants[pos]; }
        /// Get a pointer to the 'nth' item in the float buffer
        [[nodiscard]] auto getFloatPointer(size_t pos) const -> const float* { return (const float*)&mConstants[pos]; }
        /// Get a pointer to the 'nth' item in the double buffer
        auto getDoublePointer(size_t pos) -> double* { _markDirty(); return (double*)&mConstants[pos]; }
        /// Get a pointer to the 'nth' item in the double buffer
        [[nodiscard]] auto getDoublePointer(size_t pos) const -> const double* { return (const double*)&mConstants[pos]; }
        /// Get a pointer to the 'nth' item in the int buffer
        auto getIntPointer(size_t pos) -> int* { _markDirty(); return (int*)&mConstants[pos]; }
        /// Get a pointer to the 'nth' item in the int buffer
        [[nodiscard]] auto getIntPointer(size_t pos) const -> const int* { return (const int*)&mConstants[pos]; }
        /// Get a pointer to the 'nth' item in the uint buffer
        auto getUnsignedIntPointer(size_t pos) -> uint* { _markDirty(); return (uint*)&mConstants[pos]; }
        /// Get a pointer to the 'nth' item in the uint buffer
        [[nodiscard]] auto getUnsignedIntPointer(size_t pos) const -> const uint* { return (const uint*)&mConstants[pos]; }
        /// Get a reference to the list of constants
        [[nodiscard]] auto getConstantList() const noexcept -> const ConstantList& { return mConstants; }
        /** Internal method that the RenderSystem might use to store optional data. */
        void _setHardwareBuffer(const HardwareBufferPtr& data) { mHardwareBuffer = data; }
        /** Internal method that the RenderSystem might use to store optional data. */
        [[nodiscard]] auto _getHardwareBuffer() const noexcept -> const HardwareBufferPtr& { return mHardwareBuffer; }
        /// upload parameter data to GPU memory. Must have a HardwareBuffer
        void _upload() const;
        /// download data from GPU memory. Must have a writable HardwareBuffer
        void download();
    };

    class GpuProgramParameters;

    /** This class records the usage of a set of shared parameters in a concrete
        set of GpuProgramParameters.
    */
    class GpuSharedParametersUsage : public GpuParamsAlloc
    {
    private:
        GpuSharedParametersPtr mSharedParams;
        // Not a shared pointer since this is also parent
        GpuProgramParameters* mParams;
        // list of physical mappings that we are going to bring in
        struct CopyDataEntry
        {
            const GpuConstantDefinition* srcDefinition;
            const GpuConstantDefinition* dstDefinition;
        };
        using CopyDataList = std::vector<CopyDataEntry>;

        CopyDataList mCopyDataList;

        /// Version of shared params we based the copydata on
        uint32 mCopyDataVersion;

        void initCopyData();


    public:
        /// Construct usage
        GpuSharedParametersUsage(GpuSharedParametersPtr sharedParams,
                                 GpuProgramParameters* params);

        /** Update the target parameters by copying the data from the shared
            parameters.
            @note This method  may not actually be called if the RenderSystem
            supports using shared parameters directly in their own shared buffer; in
            which case the values should not be copied out of the shared area
            into the individual parameter set, but bound separately.
        */
        void _copySharedParamsToTargetParams() const;

        /// Get the name of the shared parameter set
        [[nodiscard]] auto getName() const noexcept -> std::string_view { return mSharedParams->getName(); }

        [[nodiscard]] auto getSharedParams() const noexcept -> GpuSharedParametersPtr { return mSharedParams; }
        [[nodiscard]] auto getTargetParams() const noexcept -> GpuProgramParameters* { return mParams; }
    };

    /** Collects together the program parameters used for a GpuProgram.
        @remarks
        Gpu program state includes constant parameters used by the program, and
        bindings to render system state which is propagated into the constants
        by the engine automatically if requested.
        @par
        GpuProgramParameters objects should be created through the GpuProgram and
        may be shared between multiple Pass instances. For this reason they
        are managed using a shared pointer, which will ensure they are automatically
        deleted when no Pass is using them anymore.
        @par
        High-level programs use named parameters (uniforms), low-level programs
        use indexed constants. This class supports both, but you can tell whether
        named constants are supported by calling hasNamedParameters(). There are
        references in the documentation below to 'logical' and 'physical' indexes;
        logical indexes are the indexes used by low-level programs and represent
        indexes into an array of float4's, some of which may be settable, some of
        which may be predefined constants in the program. We only store those
        constants which have actually been set, therefore our buffer could have
        gaps if we used the logical indexes in our own buffers. So instead we map
        these logical indexes to physical indexes in our buffer. When using
        high-level programs, logical indexes don't necessarily exist, although they
        might if the high-level program has a direct, exposed mapping from parameter
        names to logical indexes. In addition, high-level languages may or may not pack
        arrays of elements that are smaller than float4 (e.g. float2/vec2) contiguously.
        This kind of information is held in the ConstantDefinition structure which
        is only populated for high-level programs. You don't have to worry about
        any of this unless you intend to read parameters back from this structure
        rather than just setting them.
    */
    class GpuProgramParameters : public GpuParamsAlloc
    {
    public:
        /** Defines the types of automatically updated values that may be bound to GpuProgram
            parameters, or used to modify parameters on a per-object basis.

            For use in @ref Program-Parameter-Specification, drop the `AutoConstantType::` prefix.
            E.g. `AutoConstantType::WORLD_MATRIX` becomes `world_matrix`.
        */
        enum class AutoConstantType : unsigned int
        {
            /// The current world matrix
            WORLD_MATRIX,
            /// The current world matrix, inverted
            INVERSE_WORLD_MATRIX,
            /** Provides transpose of world matrix.
                Equivalent to RenderMonkey's "WorldTranspose".
            */
            TRANSPOSE_WORLD_MATRIX,
            /// The current world matrix, inverted & transposed
            INVERSE_TRANSPOSE_WORLD_MATRIX,

            /// An array of world matrices, each represented as only a 3x4 matrix (3 rows of
            /// 4columns) usually for doing hardware skinning.
            /// You should make enough entries available in your vertex program for the number of
            /// bones in use, i.e. an array of numBones*3 float4’s.
            WORLD_MATRIX_ARRAY_3x4,
            /// The current array of world matrices, used for blending
            WORLD_MATRIX_ARRAY,
            /// The current array of world matrices transformed to an array of dual quaternions,
            /// represented as a 2x4 matrix
            WORLD_DUALQUATERNION_ARRAY_2x4,
            /// The scale and shear components of the current array of world matrices
            WORLD_SCALE_SHEAR_MATRIX_ARRAY_3x4,

            /// The current view matrix
            VIEW_MATRIX,
            /// The current view matrix, inverted
            INVERSE_VIEW_MATRIX,
            /** Provides transpose of view matrix.
                Equivalent to RenderMonkey's "ViewTranspose".
            */
            TRANSPOSE_VIEW_MATRIX,
            /** Provides inverse transpose of view matrix.
                Equivalent to RenderMonkey's "ViewInverseTranspose".
            */
            INVERSE_TRANSPOSE_VIEW_MATRIX,

            /// The current projection matrix
            PROJECTION_MATRIX,
            /** Provides inverse of projection matrix.
                Equivalent to RenderMonkey's "ProjectionInverse".
            */
            INVERSE_PROJECTION_MATRIX,
            /** Provides transpose of projection matrix.
                Equivalent to RenderMonkey's "ProjectionTranspose".
            */
            TRANSPOSE_PROJECTION_MATRIX,
            /** Provides inverse transpose of projection matrix.
                Equivalent to RenderMonkey's "ProjectionInverseTranspose".
            */
            INVERSE_TRANSPOSE_PROJECTION_MATRIX,

            /// The current view & projection matrices concatenated
            VIEWPROJ_MATRIX,
            /** Provides inverse of concatenated view and projection matrices.
                Equivalent to RenderMonkey's "ViewProjectionInverse".
            */
            INVERSE_VIEWPROJ_MATRIX,
            /** Provides transpose of concatenated view and projection matrices.
                Equivalent to RenderMonkey's "ViewProjectionTranspose".
            */
            TRANSPOSE_VIEWPROJ_MATRIX,
            /** Provides inverse transpose of concatenated view and projection matrices.
                Equivalent to RenderMonkey's "ViewProjectionInverseTranspose".
            */
            INVERSE_TRANSPOSE_VIEWPROJ_MATRIX,

            /// The current world & view matrices concatenated
            WORLDVIEW_MATRIX,
            /// The current world & view matrices concatenated, then inverted
            INVERSE_WORLDVIEW_MATRIX,
            /** Provides transpose of concatenated world and view matrices.
                Equivalent to RenderMonkey's "WorldViewTranspose".
            */
            TRANSPOSE_WORLDVIEW_MATRIX,
            /// The current world & view matrices concatenated, then inverted & transposed
            INVERSE_TRANSPOSE_WORLDVIEW_MATRIX,
            /** Provides inverse transpose of the upper 3x3 of the worldview matrix.
                Equivalent to "gl_NormalMatrix".
            */
            NORMAL_MATRIX,

            /// The current world, view & projection matrices concatenated
            WORLDVIEWPROJ_MATRIX,
            /** Provides inverse of concatenated world, view and projection matrices.
                Equivalent to RenderMonkey's "WorldViewProjectionInverse".
            */
            INVERSE_WORLDVIEWPROJ_MATRIX,
            /** Provides transpose of concatenated world, view and projection matrices.
                Equivalent to RenderMonkey's "WorldViewProjectionTranspose".
            */
            TRANSPOSE_WORLDVIEWPROJ_MATRIX,
            /** Provides inverse transpose of concatenated world, view and projection
                matrices. Equivalent to RenderMonkey's "WorldViewProjectionInverseTranspose".
            */
            INVERSE_TRANSPOSE_WORLDVIEWPROJ_MATRIX,

            // render target related values
            /** -1 if requires texture flipping, +1 otherwise. It's useful when you bypassed
                projection matrix transform, still able use this value to adjust transformed y
               position.
            */
            RENDER_TARGET_FLIPPING,

            /** -1 if the winding has been inverted (e.g. for reflections), +1 otherwise.
             */
            VERTEX_WINDING,

            /// Fog colour
            FOG_COLOUR,
            /// Fog params: density, linear start, linear end, 1/(end-start)
            FOG_PARAMS,

            /// Surface ambient colour, as set in Pass::setAmbient
            SURFACE_AMBIENT_COLOUR,
            /// Surface diffuse colour, as set in Pass::setDiffuse
            SURFACE_DIFFUSE_COLOUR,
            /// Surface specular colour, as set in Pass::setSpecular
            SURFACE_SPECULAR_COLOUR,
            /// Surface emissive colour, as set in Pass::setSelfIllumination
            SURFACE_EMISSIVE_COLOUR,
            /// Surface shininess, as set in Pass::setShininess
            SURFACE_SHININESS,
            /// Surface alpha rejection value, not as set in @ref Pass::setAlphaRejectValue, but a
            /// floating number between 0.0f and 1.0f instead (255.0f /
            /// @ref Pass::getAlphaRejectValue())
            SURFACE_ALPHA_REJECTION_VALUE,

            /// The number of active light sources (better than gl_MaxLights)
            LIGHT_COUNT,

            /// The ambient light colour set in the scene
            AMBIENT_LIGHT_COLOUR,

            /// Light diffuse colour (index determined by setAutoConstant call)
            ///
            /// this requires an index in the ’extra_params’ field, and relates to the ’nth’ closest
            /// light which could affect this object
            /// (i.e. 0 refers to the closest light - note that directional lights are always first
            /// in the list and always present).
            /// NB if there are no lights this close, then the parameter will be set to black.
            LIGHT_DIFFUSE_COLOUR,
            /// Light specular colour (index determined by setAutoConstant call)
            LIGHT_SPECULAR_COLOUR,
            /// Light attenuation parameters, Vector4{range, constant, linear, quadric}
            LIGHT_ATTENUATION,
            /** Spotlight parameters, Vector4{innerFactor, outerFactor, falloff, isSpot}
                innerFactor and outerFactor are cos(angle/2)
                The isSpot parameter is 0.0f for non-spotlights, 1.0f for spotlights.
                Also for non-spotlights the inner and outer factors are 1 and nearly 1 respectively
            */
            SPOTLIGHT_PARAMS,
            /** A light position in world space (index determined by setAutoConstant call)

             This requires an index in the ’extra_params’ field, and relates to the ’nth’ closest
             light which could affect this object (i.e. 0 refers to the closest light).
             NB if there are no lights this close, then the parameter will be set to all zeroes.
             Note that this property will work with all kinds of lights, even directional lights,
             since the parameter is set as a 4D vector.
             Point lights will be (pos.x, pos.y, pos.z, 1.0f) whilst directional lights will be
             (-dir.x, -dir.y, -dir.z, 0.0f).
             Operations like dot products will work consistently on both.
             */
            LIGHT_POSITION,
            /// A light position in object space (index determined by setAutoConstant call)
            LIGHT_POSITION_OBJECT_SPACE,
            /// A light position in view space (index determined by setAutoConstant call)
            LIGHT_POSITION_VIEW_SPACE,
            /// A light direction in world space (index determined by setAutoConstant call)
            /// @deprecated this property only works on directional lights, and we recommend that
            /// you use light_position instead since that returns a generic 4D vector.
            LIGHT_DIRECTION,
            /// A light direction in object space (index determined by setAutoConstant call)
            LIGHT_DIRECTION_OBJECT_SPACE,
            /// A light direction in view space (index determined by setAutoConstant call)
            LIGHT_DIRECTION_VIEW_SPACE,
            /** The distance of the light from the center of the object
                a useful approximation as an alternative to per-vertex distance
                calculations.
            */
            LIGHT_DISTANCE_OBJECT_SPACE,
            /** Light power level, a single scalar as set in Light::setPowerScale  (index determined
               by setAutoConstant call) */
            LIGHT_POWER_SCALE,
            /// Light diffuse colour pre-scaled by Light::setPowerScale (index determined by
            /// setAutoConstant call)
            LIGHT_DIFFUSE_COLOUR_POWER_SCALED,
            /// Light specular colour pre-scaled by Light::setPowerScale (index determined by
            /// setAutoConstant call)
            LIGHT_SPECULAR_COLOUR_POWER_SCALED,
            /// Array of light diffuse colours (count set by extra param)
            LIGHT_DIFFUSE_COLOUR_ARRAY,
            /// Array of light specular colours (count set by extra param)
            LIGHT_SPECULAR_COLOUR_ARRAY,
            /// Array of light diffuse colours scaled by light power (count set by extra param)
            LIGHT_DIFFUSE_COLOUR_POWER_SCALED_ARRAY,
            /// Array of light specular colours scaled by light power (count set by extra param)
            LIGHT_SPECULAR_COLOUR_POWER_SCALED_ARRAY,
            /// Array of light attenuation parameters, Vector4{range, constant, linear, quadric}
            /// (count set by extra param)
            LIGHT_ATTENUATION_ARRAY,
            /// Array of light positions in world space (count set by extra param)
            LIGHT_POSITION_ARRAY,
            /// Array of light positions in object space (count set by extra param)
            LIGHT_POSITION_OBJECT_SPACE_ARRAY,
            /// Array of light positions in view space (count set by extra param)
            LIGHT_POSITION_VIEW_SPACE_ARRAY,
            /// Array of light directions in world space (count set by extra param)
            LIGHT_DIRECTION_ARRAY,
            /// Array of light directions in object space (count set by extra param)
            LIGHT_DIRECTION_OBJECT_SPACE_ARRAY,
            /// Array of light directions in view space (count set by extra param)
            LIGHT_DIRECTION_VIEW_SPACE_ARRAY,
            /** Array of distances of the lights from the center of the object
                a useful approximation as an alternative to per-vertex distance
                calculations. (count set by extra param)
            */
            LIGHT_DISTANCE_OBJECT_SPACE_ARRAY,
            /** Array of light power levels, a single scalar as set in Light::setPowerScale
                (count set by extra param)
            */
            LIGHT_POWER_SCALE_ARRAY,
            /** Spotlight parameters array of Vector4{innerFactor, outerFactor, falloff, isSpot}
                innerFactor and outerFactor are cos(angle/2)
                The isSpot parameter is 0.0f for non-spotlights, 1.0f for spotlights.
                Also for non-spotlights the inner and outer factors are 1 and nearly 1 respectively.
                (count set by extra param)
            */
            SPOTLIGHT_PARAMS_ARRAY,

            /** The derived ambient light colour, with 'r', 'g', 'b' components filled with
                product of surface ambient colour and ambient light colour, respectively,
                and 'a' component filled with surface ambient alpha component.
            */
            DERIVED_AMBIENT_LIGHT_COLOUR,
            /** The derived scene colour, with 'r', 'g' and 'b' components filled with sum
                of derived ambient light colour and surface emissive colour, respectively,
                and 'a' component filled with surface diffuse alpha component.
            */
            DERIVED_SCENE_COLOUR,

            /** The derived light diffuse colour (index determined by setAutoConstant call),
                with 'r', 'g' and 'b' components filled with product of surface diffuse colour,
                light power scale and light diffuse colour, respectively, and 'a' component filled
               with surface
                diffuse alpha component.
            */
            DERIVED_LIGHT_DIFFUSE_COLOUR,
            /** The derived light specular colour (index determined by setAutoConstant call),
                with 'r', 'g' and 'b' components filled with product of surface specular colour
                and light specular colour, respectively, and 'a' component filled with surface
                specular alpha component.
            */
            DERIVED_LIGHT_SPECULAR_COLOUR,

            /// Array of derived light diffuse colours (count set by extra param)
            DERIVED_LIGHT_DIFFUSE_COLOUR_ARRAY,
            /// Array of derived light specular colours (count set by extra param)
            DERIVED_LIGHT_SPECULAR_COLOUR_ARRAY,
            /** The absolute light number of a local light index. Each pass may have
                a number of lights passed to it, and each of these lights will have
                an index in the overall light list, which will differ from the local
                light index due to factors like setStartLight and setIteratePerLight.
                This binding provides the global light index for a local index.
            */
            LIGHT_NUMBER,
            /// Returns (int) 1 if the  given light casts shadows, 0 otherwise (index set in extra
            /// param)
            LIGHT_CASTS_SHADOWS,
            /// Returns (int) 1 if the  given light casts shadows, 0 otherwise (index set in extra
            /// param)
            LIGHT_CASTS_SHADOWS_ARRAY,

            /** The distance a shadow volume should be extruded when using
                finite extrusion programs.
            */
            SHADOW_EXTRUSION_DISTANCE,
            /// The current camera's position in world space
            CAMERA_POSITION,
            /// The current camera's position in object space
            CAMERA_POSITION_OBJECT_SPACE,
            /// The current camera's position in world space even when camera relative rendering is enabled
            CAMERA_RELATIVE_POSITION,

            /** The view/projection matrix of the assigned texture projection frustum

             Applicable to vertex programs which have been specified as the ’shadow receiver’ vertex
             program alternative, or where a texture unit is marked as content_type shadow; this
             provides details of the view/projection matrix for the current shadow projector. The
             optional ’extra_params’ entry specifies which light the projector refers to (for the
             case of content_type shadow where more than one shadow texture may be present in a
             single pass), where 0 is the default and refers to the first light referenced in this
             pass.
             */
            TEXTURE_VIEWPROJ_MATRIX,
            /// Array of view/projection matrices of the first n texture projection frustums
            TEXTURE_VIEWPROJ_MATRIX_ARRAY,
            /** The view/projection matrix of the assigned texture projection frustum,
                combined with the current world matrix
            */
            TEXTURE_WORLDVIEWPROJ_MATRIX,
            /// Array of world/view/projection matrices of the first n texture projection frustums
            TEXTURE_WORLDVIEWPROJ_MATRIX_ARRAY,
            /// The view/projection matrix of a given spotlight
            SPOTLIGHT_VIEWPROJ_MATRIX,
            /// Array of view/projection matrix of a given spotlight
            SPOTLIGHT_VIEWPROJ_MATRIX_ARRAY,
            /** The view/projection matrix of a given spotlight projection frustum,
                combined with the current world matrix
            */
            SPOTLIGHT_WORLDVIEWPROJ_MATRIX,
            /** An array of the view/projection matrix of a given spotlight projection frustum,
                combined with the current world matrix
            */
            SPOTLIGHT_WORLDVIEWPROJ_MATRIX_ARRAY,
            /** A custom parameter which will come from the renderable, using 'data' as the
             identifier

             This allows you to map a custom parameter on an individual Renderable (see
             Renderable::setCustomParameter) to a parameter on a GPU program. It requires that you
             complete the ’extra_params’ field with the index that was used in the
             Renderable::setCustomParameter call, and this will ensure that whenever this Renderable
             is used, it will have it’s custom parameter mapped in. It’s very important that this
             parameter has been defined on all Renderables that are assigned the material that
             contains this automatic mapping, otherwise the process will fail.
             */
            CUSTOM,
            /** provides current elapsed time
             */
            TIME,
            /** Single float value, which repeats itself based on given as
                parameter "cycle time". Equivalent to RenderMonkey's "Time0_X".
            */
            TIME_0_X,
            /// Cosine of "Time0_X". Equivalent to RenderMonkey's "CosTime0_X".
            COSTIME_0_X,
            /// Sine of "Time0_X". Equivalent to RenderMonkey's "SinTime0_X".
            SINTIME_0_X,
            /// Tangent of "Time0_X". Equivalent to RenderMonkey's "TanTime0_X".
            TANTIME_0_X,
            /** Vector of "Time0_X", "SinTime0_X", "CosTime0_X",
                "TanTime0_X". Equivalent to RenderMonkey's "Time0_X_Packed".
            */
            TIME_0_X_PACKED,
            /** Single float value, which represents scaled time value [0..1],
                which repeats itself based on given as parameter "cycle time".
                Equivalent to RenderMonkey's "Time0_1".
            */
            TIME_0_1,
            /// Cosine of "Time0_1". Equivalent to RenderMonkey's "CosTime0_1".
            COSTIME_0_1,
            /// Sine of "Time0_1". Equivalent to RenderMonkey's "SinTime0_1".
            SINTIME_0_1,
            /// Tangent of "Time0_1". Equivalent to RenderMonkey's "TanTime0_1".
            TANTIME_0_1,
            /** Vector of "Time0_1", "SinTime0_1", "CosTime0_1",
                "TanTime0_1". Equivalent to RenderMonkey's "Time0_1_Packed".
            */
            TIME_0_1_PACKED,
            /** Single float value, which represents scaled time value [0..2*Pi],
                which repeats itself based on given as parameter "cycle time".
                Equivalent to RenderMonkey's "Time0_2PI".
            */
            TIME_0_2PI,
            /// Cosine of "Time0_2PI". Equivalent to RenderMonkey's "CosTime0_2PI".
            COSTIME_0_2PI,
            /// Sine of "Time0_2PI". Equivalent to RenderMonkey's "SinTime0_2PI".
            SINTIME_0_2PI,
            /// Tangent of "Time0_2PI". Equivalent to RenderMonkey's "TanTime0_2PI".
            TANTIME_0_2PI,
            /** Vector of "Time0_2PI", "SinTime0_2PI", "CosTime0_2PI",
                "TanTime0_2PI". Equivalent to RenderMonkey's "Time0_2PI_Packed".
            */
            TIME_0_2PI_PACKED,
            /// provides the scaled frame time, returned as a floating point value.
            FRAME_TIME,
            /// provides the calculated frames per second, returned as a floating point value.
            FPS,
            // viewport-related values
            /** Current viewport width (in pixels) as floating point value.
                Equivalent to RenderMonkey's "ViewportWidth".
            */
            VIEWPORT_WIDTH,
            /** Current viewport height (in pixels) as floating point value.
                Equivalent to RenderMonkey's "ViewportHeight".
            */
            VIEWPORT_HEIGHT,
            /** This variable represents 1.0/ViewportWidth.
                Equivalent to RenderMonkey's "ViewportWidthInverse".
            */
            INVERSE_VIEWPORT_WIDTH,
            /** This variable represents 1.0/ViewportHeight.
                Equivalent to RenderMonkey's "ViewportHeightInverse".
            */
            INVERSE_VIEWPORT_HEIGHT,
            /** Packed of "ViewportWidth", "ViewportHeight", "ViewportWidthInverse",
                "ViewportHeightInverse".
            */
            VIEWPORT_SIZE,

            // view parameters
            /** This variable provides the view direction vector (world space).
                Equivalent to RenderMonkey's "ViewDirection".
            */
            VIEW_DIRECTION,
            /** This variable provides the view side vector (world space).
                Equivalent to RenderMonkey's "ViewSideVector".
            */
            VIEW_SIDE_VECTOR,
            /** This variable provides the view up vector (world space).
                Equivalent to RenderMonkey's "ViewUpVector".
            */
            VIEW_UP_VECTOR,
            /** This variable provides the field of view as a floating point value.
                Equivalent to RenderMonkey's "FOV".
            */
            FOV,
            /** This variable provides the near clip distance as a floating point value.
                Equivalent to RenderMonkey's "NearClipPlane".
            */
            NEAR_CLIP_DISTANCE,
            /** This variable provides the far clip distance as a floating point value.
                Equivalent to RenderMonkey's "FarClipPlane".
            */
            FAR_CLIP_DISTANCE,

            /** provides the pass index number within the technique
                of the active materil.
            */
            PASS_NUMBER,

            /** provides the current iteration number of the pass. The iteration
                number is the number of times the current render operation has
                been drawn for the active pass.
            */
            PASS_ITERATION_NUMBER,

            /** Provides a parametric animation value [0..1], only available
                where the renderable specifically implements it.

               For morph animation, sets the parametric value
               (0..1) representing the distance between the first position keyframe (bound to
               positions) and the second position keyframe (bound to the first free texture
               coordinate) so that the vertex program can interpolate between them. For pose
               animation, indicates a group of up to 4 parametric weight values applying to a
               sequence of up to 4 poses (each one bound to x, y, z and w of the constant), one for
               each pose. The original positions are held in the usual position buffer, and the
               offsets to take those positions to the pose where weight == 1.0 are in the first ’n’
               free texture coordinates; ’n’ being determined by the value passed to
               includes_pose_animation. If more than 4 simultaneous poses are required, then you’ll
               need more than 1 shader constant to hold the parametric values, in which case you
               should use this binding more than once, referencing a different constant entry; the
               second one will contain the parametrics for poses 5-8, the third for poses 9-12, and
               so on.
            */
            ANIMATION_PARAMETRIC,

            /** Provides the texel offsets required by this rendersystem to map
                texels to pixels. Packed as
                float4(absoluteHorizontalOffset, absoluteVerticalOffset,
                horizontalOffset / viewportWidth, verticalOffset / viewportHeight)
            */
            TEXEL_OFFSETS,

            /** Provides information about the depth range of the scene as viewed
                from the current camera.
                Passed as float4(minDepth, maxDepth, depthRange, 1 / depthRange)
            */
            SCENE_DEPTH_RANGE,

            /** Provides information about the depth range of the scene as viewed
                from a given shadow camera. Requires an index parameter which maps
                to a light index relative to the current light list.
                Passed as float4(minDepth, maxDepth, depthRange, 1 / depthRange)
            */
            SHADOW_SCENE_DEPTH_RANGE,

            /** Provides an array of information about the depth range of the scene as viewed
                from a given shadow camera. Requires an index parameter which maps
                to a light index relative to the current light list.
                Passed as float4(minDepth, maxDepth, depthRange, 1 / depthRange)
            */
            SHADOW_SCENE_DEPTH_RANGE_ARRAY,

            /** Provides the fixed shadow colour as configured via SceneManager::setShadowColour;
                useful for integrated modulative shadows.
            */
            SHADOW_COLOUR,
            /** Provides texture size of the texture unit (index determined by setAutoConstant
                call). Packed as float4(width, height, depth, 1)
            */
            TEXTURE_SIZE,
            /** Provides inverse texture size of the texture unit (index determined by
               setAutoConstant
                call). Packed as float4(1 / width, 1 / height, 1 / depth, 1)
            */
            INVERSE_TEXTURE_SIZE,
            /** Provides packed texture size of the texture unit (index determined by
               setAutoConstant
                call). Packed as float4(width, height, 1 / width, 1 / height)
            */
            PACKED_TEXTURE_SIZE,

            /** Provides the current transform matrix of the texture unit (index determined by
               setAutoConstant
                call), as seen by the fixed-function pipeline.

                This requires an index in the ’extra_params’ field, and relates to the ’nth’ texture
               unit of the pass in question.
                NB if the given index exceeds the number of texture units available for this pass,
               then the parameter will be set to Matrix4::IDENTITY.
            */
            TEXTURE_MATRIX,

            /** Provides the position of the LOD camera in world space, allowing you
                to perform separate LOD calculations in shaders independent of the rendering
                camera. If there is no separate LOD camera then this is the real camera
                position. See Camera::setLodCamera.
            */
            LOD_CAMERA_POSITION,
            /** Provides the position of the LOD camera in object space, allowing you
                to perform separate LOD calculations in shaders independent of the rendering
                camera. If there is no separate LOD camera then this is the real camera
                position. See Camera::setLodCamera.
            */
            LOD_CAMERA_POSITION_OBJECT_SPACE,
            /** Binds custom per-light constants to the shaders. */
            LIGHT_CUSTOM,
            /// Point params: size; constant, linear, quadratic attenuation
            POINT_PARAMS,

            UNKNOWN = 999
        };

        /** Defines the type of the extra data item used by the auto constant.

         */
        enum class ACDataType {
            /// no data is required
            NONE,
            /// the auto constant requires data of type int
            INT,
            /// the auto constant requires data of type float
            REAL
        };

        /** Defines the base element type of the auto constant
         */
        enum class ElementType {
            INT = std::to_underlying(BaseConstantType::INT),
            // float
            REAL = std::to_underlying(BaseConstantType::FLOAT)
        };

        /** Structure defining an auto constant that's available for use in
            a parameters object.
        */
        struct AutoConstantDefinition
        {
            AutoConstantType acType;
            std::string_view name;
            size_t elementCount;
            /// The type of the constant in the program
            ElementType elementType;
            /// The type of any extra data
            ACDataType dataType;
        };

        /** Structure recording the use of an automatic parameter. */
        class AutoConstantEntry
        {
        public:
            /// The target (physical) constant index
            size_t physicalIndex;
            /// The type of parameter
            AutoConstantType paramType;
            /// Additional information to go with the parameter
            union{
                uint32 data;
                float fData;
            };
            /// The variability of this parameter (see GpuParamVariability)
            GpuParamVariability variability;
            /** The number of elements per individual entry in this constant
                Used in case people used packed elements smaller than 4 (e.g. GLSL)
                and bind an auto which is 4-element packed to it */
            uint8 elementCount;

        AutoConstantEntry(AutoConstantType theType, size_t theIndex, uint32 theData,
                          GpuParamVariability theVariability, uint8 theElemCount = 4)
            : physicalIndex(theIndex), paramType(theType),
                data(theData), variability(theVariability), elementCount(theElemCount) {}

        AutoConstantEntry(AutoConstantType theType, size_t theIndex, float theData,
                          GpuParamVariability theVariability, uint8 theElemCount = 4)
            : physicalIndex(theIndex), paramType(theType),
                fData(theData), variability(theVariability), elementCount(theElemCount) {}

        };
        // Auto parameter storage
        using AutoConstantList = std::vector<AutoConstantEntry>;

        using GpuSharedParamUsageList = std::vector<GpuSharedParametersUsage>;
    private:
        static AutoConstantDefinition constexpr AutoConstantDictionary[] = {
			AutoConstantDefinition{AutoConstantType::WORLD_MATRIX, "world_matrix", 16, ElementType::REAL, ACDataType::NONE},
			AutoConstantDefinition{AutoConstantType::INVERSE_WORLD_MATRIX, "inverse_world_matrix", 16, ElementType::REAL, ACDataType::NONE},
			AutoConstantDefinition{AutoConstantType::TRANSPOSE_WORLD_MATRIX, "transpose_world_matrix", 16, ElementType::REAL, ACDataType::NONE},
			AutoConstantDefinition{AutoConstantType::INVERSE_TRANSPOSE_WORLD_MATRIX, "inverse_transpose_world_matrix", 16, ElementType::REAL, ACDataType::NONE},

			AutoConstantDefinition{AutoConstantType::WORLD_MATRIX_ARRAY_3x4, "world_matrix_array_3x4", 12, ElementType::REAL, ACDataType::NONE},
			AutoConstantDefinition{AutoConstantType::WORLD_MATRIX_ARRAY, "world_matrix_array", 16, ElementType::REAL, ACDataType::NONE},
			AutoConstantDefinition{AutoConstantType::WORLD_DUALQUATERNION_ARRAY_2x4, "world_dualquaternion_array_2x4", 8, ElementType::REAL, ACDataType::NONE},
			AutoConstantDefinition{AutoConstantType::WORLD_SCALE_SHEAR_MATRIX_ARRAY_3x4, "world_scale_shear_matrix_array_3x4", 12, ElementType::REAL, ACDataType::NONE},
			AutoConstantDefinition{AutoConstantType::VIEW_MATRIX, "view_matrix", 16, ElementType::REAL, ACDataType::NONE},
			AutoConstantDefinition{AutoConstantType::INVERSE_VIEW_MATRIX, "inverse_view_matrix", 16, ElementType::REAL, ACDataType::NONE},
			AutoConstantDefinition{AutoConstantType::TRANSPOSE_VIEW_MATRIX, "transpose_view_matrix", 16, ElementType::REAL, ACDataType::NONE},
			AutoConstantDefinition{AutoConstantType::INVERSE_TRANSPOSE_VIEW_MATRIX, "inverse_transpose_view_matrix", 16, ElementType::REAL, ACDataType::NONE},

			AutoConstantDefinition{AutoConstantType::PROJECTION_MATRIX, "projection_matrix", 16, ElementType::REAL, ACDataType::NONE},
			AutoConstantDefinition{AutoConstantType::INVERSE_PROJECTION_MATRIX, "inverse_projection_matrix", 16, ElementType::REAL, ACDataType::NONE},
			AutoConstantDefinition{AutoConstantType::TRANSPOSE_PROJECTION_MATRIX, "transpose_projection_matrix", 16, ElementType::REAL, ACDataType::NONE},
			AutoConstantDefinition{AutoConstantType::INVERSE_TRANSPOSE_PROJECTION_MATRIX, "inverse_transpose_projection_matrix", 16, ElementType::REAL, ACDataType::NONE},

			AutoConstantDefinition{AutoConstantType::VIEWPROJ_MATRIX, "viewproj_matrix", 16, ElementType::REAL, ACDataType::NONE},
			AutoConstantDefinition{AutoConstantType::INVERSE_VIEWPROJ_MATRIX, "inverse_viewproj_matrix", 16, ElementType::REAL, ACDataType::NONE},
			AutoConstantDefinition{AutoConstantType::TRANSPOSE_VIEWPROJ_MATRIX, "transpose_viewproj_matrix", 16, ElementType::REAL, ACDataType::NONE},
			AutoConstantDefinition{AutoConstantType::INVERSE_TRANSPOSE_VIEWPROJ_MATRIX, "inverse_transpose_viewproj_matrix", 16, ElementType::REAL, ACDataType::NONE},

			AutoConstantDefinition{AutoConstantType::WORLDVIEW_MATRIX, "worldview_matrix", 16, ElementType::REAL, ACDataType::NONE},
			AutoConstantDefinition{AutoConstantType::INVERSE_WORLDVIEW_MATRIX, "inverse_worldview_matrix", 16, ElementType::REAL, ACDataType::NONE},
			AutoConstantDefinition{AutoConstantType::TRANSPOSE_WORLDVIEW_MATRIX, "transpose_worldview_matrix", 16, ElementType::REAL, ACDataType::NONE},
			AutoConstantDefinition{AutoConstantType::INVERSE_TRANSPOSE_WORLDVIEW_MATRIX, "inverse_transpose_worldview_matrix", 16, ElementType::REAL, ACDataType::NONE},
			AutoConstantDefinition{AutoConstantType::NORMAL_MATRIX, "normal_matrix", 9, ElementType::REAL, ACDataType::NONE},

			AutoConstantDefinition{AutoConstantType::WORLDVIEWPROJ_MATRIX, "worldviewproj_matrix", 16, ElementType::REAL, ACDataType::NONE},
			AutoConstantDefinition{AutoConstantType::INVERSE_WORLDVIEWPROJ_MATRIX, "inverse_worldviewproj_matrix", 16, ElementType::REAL, ACDataType::NONE},
			AutoConstantDefinition{AutoConstantType::TRANSPOSE_WORLDVIEWPROJ_MATRIX, "transpose_worldviewproj_matrix", 16, ElementType::REAL, ACDataType::NONE},
			AutoConstantDefinition{AutoConstantType::INVERSE_TRANSPOSE_WORLDVIEWPROJ_MATRIX, "inverse_transpose_worldviewproj_matrix", 16, ElementType::REAL, ACDataType::NONE},

			AutoConstantDefinition{AutoConstantType::RENDER_TARGET_FLIPPING, "render_target_flipping", 1, ElementType::REAL, ACDataType::NONE},
			AutoConstantDefinition{AutoConstantType::VERTEX_WINDING, "vertex_winding", 1, ElementType::REAL, ACDataType::NONE},

			AutoConstantDefinition{AutoConstantType::FOG_COLOUR, "fog_colour", 4, ElementType::REAL, ACDataType::NONE},
			AutoConstantDefinition{AutoConstantType::FOG_PARAMS, "fog_params", 4, ElementType::REAL, ACDataType::NONE},

			AutoConstantDefinition{AutoConstantType::SURFACE_AMBIENT_COLOUR, "surface_ambient_colour", 4, ElementType::REAL, ACDataType::NONE},
			AutoConstantDefinition{AutoConstantType::SURFACE_DIFFUSE_COLOUR, "surface_diffuse_colour", 4, ElementType::REAL, ACDataType::NONE},
			AutoConstantDefinition{AutoConstantType::SURFACE_SPECULAR_COLOUR, "surface_specular_colour", 4, ElementType::REAL, ACDataType::NONE},
			AutoConstantDefinition{AutoConstantType::SURFACE_EMISSIVE_COLOUR, "surface_emissive_colour", 4, ElementType::REAL, ACDataType::NONE},
			AutoConstantDefinition{AutoConstantType::SURFACE_SHININESS, "surface_shininess", 1, ElementType::REAL, ACDataType::NONE},
			AutoConstantDefinition{AutoConstantType::SURFACE_ALPHA_REJECTION_VALUE, "surface_alpha_rejection_value", 1, ElementType::REAL, ACDataType::NONE},

			AutoConstantDefinition{AutoConstantType::LIGHT_COUNT, "light_count", 1, ElementType::REAL, ACDataType::NONE},

			AutoConstantDefinition{AutoConstantType::AMBIENT_LIGHT_COLOUR, "ambient_light_colour", 4, ElementType::REAL, ACDataType::NONE},
			AutoConstantDefinition{AutoConstantType::LIGHT_DIFFUSE_COLOUR, "light_diffuse_colour", 4, ElementType::REAL, ACDataType::INT},
			AutoConstantDefinition{AutoConstantType::LIGHT_SPECULAR_COLOUR, "light_specular_colour", 4, ElementType::REAL, ACDataType::INT},
			AutoConstantDefinition{AutoConstantType::LIGHT_ATTENUATION, "light_attenuation", 4, ElementType::REAL, ACDataType::INT},
			AutoConstantDefinition{AutoConstantType::SPOTLIGHT_PARAMS, "spotlight_params", 4, ElementType::REAL, ACDataType::INT},
			AutoConstantDefinition{AutoConstantType::LIGHT_POSITION, "light_position", 4, ElementType::REAL, ACDataType::INT},
			AutoConstantDefinition{AutoConstantType::LIGHT_POSITION_OBJECT_SPACE, "light_position_object_space", 4, ElementType::REAL, ACDataType::INT},
			AutoConstantDefinition{AutoConstantType::LIGHT_POSITION_VIEW_SPACE, "light_position_view_space", 4, ElementType::REAL, ACDataType::INT},
			AutoConstantDefinition{AutoConstantType::LIGHT_DIRECTION, "light_direction", 4, ElementType::REAL, ACDataType::INT},
			AutoConstantDefinition{AutoConstantType::LIGHT_DIRECTION_OBJECT_SPACE, "light_direction_object_space", 4, ElementType::REAL, ACDataType::INT},
			AutoConstantDefinition{AutoConstantType::LIGHT_DIRECTION_VIEW_SPACE, "light_direction_view_space", 4, ElementType::REAL, ACDataType::INT},
			AutoConstantDefinition{AutoConstantType::LIGHT_DISTANCE_OBJECT_SPACE, "light_distance_object_space", 1, ElementType::REAL, ACDataType::INT},
			AutoConstantDefinition{AutoConstantType::LIGHT_POWER_SCALE, "light_power", 1, ElementType::REAL, ACDataType::INT},
			AutoConstantDefinition{AutoConstantType::LIGHT_DIFFUSE_COLOUR_POWER_SCALED, "light_diffuse_colour_power_scaled", 4, ElementType::REAL, ACDataType::INT},
			AutoConstantDefinition{AutoConstantType::LIGHT_SPECULAR_COLOUR_POWER_SCALED, "light_specular_colour_power_scaled", 4, ElementType::REAL, ACDataType::INT},
			AutoConstantDefinition{AutoConstantType::LIGHT_DIFFUSE_COLOUR_ARRAY, "light_diffuse_colour_array", 4, ElementType::REAL, ACDataType::INT},
			AutoConstantDefinition{AutoConstantType::LIGHT_SPECULAR_COLOUR_ARRAY, "light_specular_colour_array", 4, ElementType::REAL, ACDataType::INT},
			AutoConstantDefinition{AutoConstantType::LIGHT_DIFFUSE_COLOUR_POWER_SCALED_ARRAY, "light_diffuse_colour_power_scaled_array", 4, ElementType::REAL, ACDataType::INT},
			AutoConstantDefinition{AutoConstantType::LIGHT_SPECULAR_COLOUR_POWER_SCALED_ARRAY, "light_specular_colour_power_scaled_array", 4, ElementType::REAL, ACDataType::INT},
			AutoConstantDefinition{AutoConstantType::LIGHT_ATTENUATION_ARRAY, "light_attenuation_array", 4, ElementType::REAL, ACDataType::INT},
			AutoConstantDefinition{AutoConstantType::LIGHT_POSITION_ARRAY, "light_position_array", 4, ElementType::REAL, ACDataType::INT},
			AutoConstantDefinition{AutoConstantType::LIGHT_POSITION_OBJECT_SPACE_ARRAY, "light_position_object_space_array", 4, ElementType::REAL, ACDataType::INT},
			AutoConstantDefinition{AutoConstantType::LIGHT_POSITION_VIEW_SPACE_ARRAY, "light_position_view_space_array", 4, ElementType::REAL, ACDataType::INT},
			AutoConstantDefinition{AutoConstantType::LIGHT_DIRECTION_ARRAY, "light_direction_array", 4, ElementType::REAL, ACDataType::INT},
			AutoConstantDefinition{AutoConstantType::LIGHT_DIRECTION_OBJECT_SPACE_ARRAY, "light_direction_object_space_array", 4, ElementType::REAL, ACDataType::INT},
			AutoConstantDefinition{AutoConstantType::LIGHT_DIRECTION_VIEW_SPACE_ARRAY, "light_direction_view_space_array", 4, ElementType::REAL, ACDataType::INT},
			AutoConstantDefinition{AutoConstantType::LIGHT_DISTANCE_OBJECT_SPACE_ARRAY, "light_distance_object_space_array", 1, ElementType::REAL, ACDataType::INT},
			AutoConstantDefinition{AutoConstantType::LIGHT_POWER_SCALE_ARRAY, "light_power_array", 1, ElementType::REAL, ACDataType::INT},
			AutoConstantDefinition{AutoConstantType::SPOTLIGHT_PARAMS_ARRAY, "spotlight_params_array", 4, ElementType::REAL, ACDataType::INT},

			AutoConstantDefinition{AutoConstantType::DERIVED_AMBIENT_LIGHT_COLOUR, "derived_ambient_light_colour", 4, ElementType::REAL, ACDataType::NONE},
			AutoConstantDefinition{AutoConstantType::DERIVED_SCENE_COLOUR, "derived_scene_colour", 4, ElementType::REAL, ACDataType::NONE},
			AutoConstantDefinition{AutoConstantType::DERIVED_LIGHT_DIFFUSE_COLOUR, "derived_light_diffuse_colour", 4, ElementType::REAL, ACDataType::INT},
			AutoConstantDefinition{AutoConstantType::DERIVED_LIGHT_SPECULAR_COLOUR, "derived_light_specular_colour", 4, ElementType::REAL, ACDataType::INT},
			AutoConstantDefinition{AutoConstantType::DERIVED_LIGHT_DIFFUSE_COLOUR_ARRAY, "derived_light_diffuse_colour_array", 4, ElementType::REAL, ACDataType::INT},
			AutoConstantDefinition{AutoConstantType::DERIVED_LIGHT_SPECULAR_COLOUR_ARRAY, "derived_light_specular_colour_array", 4, ElementType::REAL, ACDataType::INT},

			AutoConstantDefinition{AutoConstantType::LIGHT_NUMBER, "light_number", 1, ElementType::REAL, ACDataType::INT},
			AutoConstantDefinition{AutoConstantType::LIGHT_CASTS_SHADOWS, "light_casts_shadows", 1, ElementType::REAL, ACDataType::INT},
			AutoConstantDefinition{AutoConstantType::LIGHT_CASTS_SHADOWS_ARRAY, "light_casts_shadows_array", 1, ElementType::REAL, ACDataType::INT},

			AutoConstantDefinition{AutoConstantType::SHADOW_EXTRUSION_DISTANCE, "shadow_extrusion_distance", 1, ElementType::REAL, ACDataType::INT},
			AutoConstantDefinition{AutoConstantType::CAMERA_POSITION, "camera_position", 3, ElementType::REAL, ACDataType::NONE},
			AutoConstantDefinition{AutoConstantType::CAMERA_POSITION_OBJECT_SPACE, "camera_position_object_space", 3, ElementType::REAL, ACDataType::NONE},
			AutoConstantDefinition{AutoConstantType::CAMERA_RELATIVE_POSITION, "camera_relative_position", 3, ElementType::REAL, ACDataType::NONE},
			AutoConstantDefinition{AutoConstantType::TEXTURE_VIEWPROJ_MATRIX, "texture_viewproj_matrix", 16, ElementType::REAL, ACDataType::INT},
			AutoConstantDefinition{AutoConstantType::TEXTURE_VIEWPROJ_MATRIX_ARRAY, "texture_viewproj_matrix_array", 16, ElementType::REAL, ACDataType::INT},
			AutoConstantDefinition{AutoConstantType::TEXTURE_WORLDVIEWPROJ_MATRIX, "texture_worldviewproj_matrix",16, ElementType::REAL, ACDataType::INT},
			AutoConstantDefinition{AutoConstantType::TEXTURE_WORLDVIEWPROJ_MATRIX_ARRAY, "texture_worldviewproj_matrix_array",16, ElementType::REAL, ACDataType::INT},
			AutoConstantDefinition{AutoConstantType::SPOTLIGHT_VIEWPROJ_MATRIX, "spotlight_viewproj_matrix", 16, ElementType::REAL, ACDataType::INT},
			AutoConstantDefinition{AutoConstantType::SPOTLIGHT_VIEWPROJ_MATRIX_ARRAY, "spotlight_viewproj_matrix_array", 16, ElementType::REAL, ACDataType::INT},
			AutoConstantDefinition{AutoConstantType::SPOTLIGHT_WORLDVIEWPROJ_MATRIX, "spotlight_worldviewproj_matrix",16, ElementType::REAL, ACDataType::INT},
			AutoConstantDefinition{AutoConstantType::SPOTLIGHT_WORLDVIEWPROJ_MATRIX_ARRAY, "spotlight_worldviewproj_matrix_array",16, ElementType::REAL, ACDataType::INT},
			AutoConstantDefinition{AutoConstantType::CUSTOM, "custom", 4, ElementType::REAL, ACDataType::INT}, // *** needs to be tested
			AutoConstantDefinition{AutoConstantType::TIME, "time", 1, ElementType::REAL, ACDataType::REAL},
			AutoConstantDefinition{AutoConstantType::TIME_0_X, "time_0_x", 4, ElementType::REAL, ACDataType::REAL},
			AutoConstantDefinition{AutoConstantType::COSTIME_0_X, "costime_0_x", 4, ElementType::REAL, ACDataType::REAL},
			AutoConstantDefinition{AutoConstantType::SINTIME_0_X, "sintime_0_x", 4, ElementType::REAL, ACDataType::REAL},
			AutoConstantDefinition{AutoConstantType::TANTIME_0_X, "tantime_0_x", 4, ElementType::REAL, ACDataType::REAL},
			AutoConstantDefinition{AutoConstantType::TIME_0_X_PACKED, "time_0_x_packed", 4, ElementType::REAL, ACDataType::REAL},
			AutoConstantDefinition{AutoConstantType::TIME_0_1, "time_0_1", 4, ElementType::REAL, ACDataType::REAL},
			AutoConstantDefinition{AutoConstantType::COSTIME_0_1, "costime_0_1", 4, ElementType::REAL, ACDataType::REAL},
			AutoConstantDefinition{AutoConstantType::SINTIME_0_1, "sintime_0_1", 4, ElementType::REAL, ACDataType::REAL},
			AutoConstantDefinition{AutoConstantType::TANTIME_0_1, "tantime_0_1", 4, ElementType::REAL, ACDataType::REAL},
			AutoConstantDefinition{AutoConstantType::TIME_0_1_PACKED, "time_0_1_packed", 4, ElementType::REAL, ACDataType::REAL},
			AutoConstantDefinition{AutoConstantType::TIME_0_2PI, "time_0_2pi", 4, ElementType::REAL, ACDataType::REAL},
			AutoConstantDefinition{AutoConstantType::COSTIME_0_2PI, "costime_0_2pi", 4, ElementType::REAL, ACDataType::REAL},
			AutoConstantDefinition{AutoConstantType::SINTIME_0_2PI, "sintime_0_2pi", 4, ElementType::REAL, ACDataType::REAL},
			AutoConstantDefinition{AutoConstantType::TANTIME_0_2PI, "tantime_0_2pi", 4, ElementType::REAL, ACDataType::REAL},
			AutoConstantDefinition{AutoConstantType::TIME_0_2PI_PACKED, "time_0_2pi_packed", 4, ElementType::REAL, ACDataType::REAL},
			AutoConstantDefinition{AutoConstantType::FRAME_TIME, "frame_time", 1, ElementType::REAL, ACDataType::REAL},
			AutoConstantDefinition{AutoConstantType::FPS, "fps", 1, ElementType::REAL, ACDataType::NONE},
			AutoConstantDefinition{AutoConstantType::VIEWPORT_WIDTH, "viewport_width", 1, ElementType::REAL, ACDataType::NONE},
			AutoConstantDefinition{AutoConstantType::VIEWPORT_HEIGHT, "viewport_height", 1, ElementType::REAL, ACDataType::NONE},
			AutoConstantDefinition{AutoConstantType::INVERSE_VIEWPORT_WIDTH, "inverse_viewport_width", 1, ElementType::REAL, ACDataType::NONE},
			AutoConstantDefinition{AutoConstantType::INVERSE_VIEWPORT_HEIGHT, "inverse_viewport_height", 1, ElementType::REAL, ACDataType::NONE},
			AutoConstantDefinition{AutoConstantType::VIEWPORT_SIZE, "viewport_size", 4, ElementType::REAL, ACDataType::NONE},
			AutoConstantDefinition{AutoConstantType::VIEW_DIRECTION, "view_direction", 3, ElementType::REAL, ACDataType::NONE},
			AutoConstantDefinition{AutoConstantType::VIEW_SIDE_VECTOR, "view_side_vector", 3, ElementType::REAL, ACDataType::NONE},
			AutoConstantDefinition{AutoConstantType::VIEW_UP_VECTOR, "view_up_vector", 3, ElementType::REAL, ACDataType::NONE},
			AutoConstantDefinition{AutoConstantType::FOV, "fov", 1, ElementType::REAL, ACDataType::NONE},
			AutoConstantDefinition{AutoConstantType::NEAR_CLIP_DISTANCE, "near_clip_distance", 1, ElementType::REAL, ACDataType::NONE},
			AutoConstantDefinition{AutoConstantType::FAR_CLIP_DISTANCE, "far_clip_distance", 1, ElementType::REAL, ACDataType::NONE},
			AutoConstantDefinition{AutoConstantType::PASS_NUMBER, "pass_number", 1, ElementType::REAL, ACDataType::NONE},
			AutoConstantDefinition{AutoConstantType::PASS_ITERATION_NUMBER, "pass_iteration_number", 1, ElementType::REAL, ACDataType::NONE},
			AutoConstantDefinition{AutoConstantType::ANIMATION_PARAMETRIC, "animation_parametric", 4, ElementType::REAL, ACDataType::INT},
			AutoConstantDefinition{AutoConstantType::TEXEL_OFFSETS, "texel_offsets", 4, ElementType::REAL, ACDataType::NONE},
			AutoConstantDefinition{AutoConstantType::SCENE_DEPTH_RANGE, "scene_depth_range", 4, ElementType::REAL, ACDataType::NONE},
			AutoConstantDefinition{AutoConstantType::SHADOW_SCENE_DEPTH_RANGE, "shadow_scene_depth_range", 4, ElementType::REAL, ACDataType::INT},
			AutoConstantDefinition{AutoConstantType::SHADOW_SCENE_DEPTH_RANGE_ARRAY, "shadow_scene_depth_range_array", 4, ElementType::REAL, ACDataType::INT},
			AutoConstantDefinition{AutoConstantType::SHADOW_COLOUR, "shadow_colour", 4, ElementType::REAL, ACDataType::NONE},
			AutoConstantDefinition{AutoConstantType::TEXTURE_SIZE, "texture_size", 4, ElementType::REAL, ACDataType::INT},
			AutoConstantDefinition{AutoConstantType::INVERSE_TEXTURE_SIZE, "inverse_texture_size", 4, ElementType::REAL, ACDataType::INT},
			AutoConstantDefinition{AutoConstantType::PACKED_TEXTURE_SIZE, "packed_texture_size", 4, ElementType::REAL, ACDataType::INT},
			AutoConstantDefinition{AutoConstantType::TEXTURE_MATRIX, "texture_matrix", 16, ElementType::REAL, ACDataType::INT},
			AutoConstantDefinition{AutoConstantType::LOD_CAMERA_POSITION, "lod_camera_position", 3, ElementType::REAL, ACDataType::NONE},
			AutoConstantDefinition{AutoConstantType::LOD_CAMERA_POSITION_OBJECT_SPACE, "lod_camera_position_object_space", 3, ElementType::REAL, ACDataType::NONE},
			AutoConstantDefinition{AutoConstantType::LIGHT_CUSTOM, "light_custom", 4, ElementType::REAL, ACDataType::INT},
			AutoConstantDefinition{AutoConstantType::POINT_PARAMS, "point_params", 4, ElementType::REAL, ACDataType::NONE},
		};

        /// Packed list of constants (physical indexing)
        ConstantList mConstants;

        /// Sampler handles (logical indexing)
        std::vector<int> mRegisters;

        /** Logical index to physical index map - for low-level programs
            or high-level programs which pass params this way. */
        GpuLogicalBufferStructPtr mLogicalToPhysical;

        /** Gets the physical buffer index associated with a logical int constant index.
         */
        auto getConstantLogicalIndexUse(size_t logicalIndex, size_t requestedSize,
                                                       GpuParamVariability variability, BaseConstantType type) -> GpuLogicalIndexUse*;

        /// Mapping from parameter names to def - high-level programs are expected to populate this
        GpuNamedConstantsPtr mNamedConstants;
        /// List of automatically updated parameters
        AutoConstantList mAutoConstants;
        /// The combined variability masks of all parameters
        GpuParamVariability mCombinedVariability{GpuParamVariability::GLOBAL};
        /// Do we need to transpose matrices?
        bool mTransposeMatrices{false};
        /// flag to indicate if names not found will be ignored
        bool mIgnoreMissingParams{false};
        /// physical index for active pass iteration parameter real constant entry;
        size_t mActivePassIterationIndex;

        /// Return the variability for an auto constant
        static auto deriveVariability(AutoConstantType act) -> GpuParamVariability;

        void copySharedParamSetUsage(const GpuSharedParamUsageList& srcList);

        GpuSharedParamUsageList mSharedParamSets;

    public:
        GpuProgramParameters();
        ~GpuProgramParameters();

        /// Copy constructor
        GpuProgramParameters(const GpuProgramParameters& oth);
        /// Operator = overload
        auto operator=(const GpuProgramParameters& oth) -> GpuProgramParameters&;

        /** Internal method for providing a link to a name->definition map for parameters. */
        void _setNamedConstants(const GpuNamedConstantsPtr& constantmap);

        /** Internal method for providing a link to a logical index->physical index map for
         * parameters. */
        void _setLogicalIndexes(const GpuLogicalBufferStructPtr& indexMap);

        /// Does this parameter set include named parameters?
        [[nodiscard]] auto hasNamedParameters() const -> bool { return mNamedConstants.get() != nullptr; }
        /** Does this parameter set include logically indexed parameters?
            @note Not mutually exclusive with hasNamedParameters since some high-level
            programs still use logical indexes to set the parameters on the
            rendersystem.
        */
        [[nodiscard]] auto hasLogicalIndexedParameters() const -> bool { return mLogicalToPhysical.get() != nullptr; }

        /// @name Set constant by logical index
        /// @{
        /** Sets a 4-element floating-point parameter to the program.
            @param index The logical constant index at which to place the parameter
            (each constant is a 4D float)
            @param vec The value to set
        */
        void setConstant(size_t index, const Vector4& vec);
        /** Sets a single floating-point parameter to the program.
            @note This is actually equivalent to calling
            setConstant(index Vector4{val, 0, 0, 0}) since all constants are 4D.
            @param index The logical constant index at which to place the parameter (each constant is
            a 4D float)
            @param val The value to set
        */
        void setConstant(size_t index, Real val);
        /** Sets a 4-element floating-point parameter to the program via Vector3.
            @param index The logical constant index at which to place the parameter (each constant is
            a 4D float).
            Note that since you're passing a Vector3, the last element of the 4-element
            value will be set to 1 (a homogeneous vector)
            @param vec The value to set
        */
        void setConstant(size_t index, const Vector3& vec);
        /** Sets a 4-element floating-point parameter to the program via Vector2.
            @param index The logical constant index at which to place the parameter (each constant is
            a 4D float).
            Note that since you're passing a Vector2, the last 2 elements of the 4-element
            value will be set to 1 (a homogeneous vector)
            @param vec The value to set
        */
        void setConstant(size_t index, const Vector2& vec);
        /** Sets a Matrix4 parameter to the program.
            @param index The logical constant index at which to place the parameter (each constant is
            a 4D float).
            NB since a Matrix4 is 16 floats long, this parameter will take up 4 indexes.
            @param m The value to set
        */
        void setConstant(size_t index, const Matrix4& m);
        /** Sets a list of Matrix4 parameters to the program.
            @param index The logical constant index at which to start placing the parameter (each constant is
            a 4D float).
            NB since a Matrix4 is 16 floats long, so each entry will take up 4 indexes.
            @param m Pointer to an array of matrices to set
            @param numEntries Number of Matrix4 entries
        */
        void setConstant(size_t index, const Matrix4* m, size_t numEntries);
        /** Sets a ColourValue parameter to the program.
            @param index The logical constant index at which to place the parameter (each constant is
            a 4D float)
            @param colour The value to set
        */
        void setConstant(size_t index, const ColourValue& colour);
        /** Sets a multiple value constant floating-point parameter to the program.
            @param index The logical constant index at which to start placing parameters (each constant is
            a 4D float)
            @param val Pointer to the values to write, must contain 4*count floats
            @param count The number of groups of 4 floats to write
        */
        void setConstant(size_t index, const float *val, size_t count);
        /** Sets a multiple value constant floating-point parameter to the program.
            @param index The logical constant index at which to start placing parameters (each constant is
            a 4D float)
            @param val Pointer to the values to write, must contain 4*count floats
            @param count The number of groups of 4 floats to write
        */
        void setConstant(size_t index, const double *val, size_t count);
        /** Sets a multiple value constant integer parameter to the program.
            @remarks
            Different types of GPU programs support different types of constant parameters.
            For example, it's relatively common to find that vertex programs only support
            floating point constants, and that fragment programs only support integer (fixed point)
            parameters. This can vary depending on the program version supported by the
            graphics card being used. You should consult the documentation for the type of
            low level program you are using, or alternatively use the methods
            provided on RenderSystemCapabilities to determine the options.
            @param index The logical constant index at which to place the parameter (each constant is
            a 4D integer)
            @param val Pointer to the values to write, must contain 4*count ints
            @param count The number of groups of 4 ints to write
        */
        void setConstant(size_t index, const int *val, size_t count);
        /** Sets a multiple value constant unsigned integer parameter to the program.
            @remarks
            Different types of GPU programs support different types of constant parameters.
            For example, it's relatively common to find that vertex programs only support
            floating point constants, and that fragment programs only support integer (fixed point)
            parameters. This can vary depending on the program version supported by the
            graphics card being used. You should consult the documentation for the type of
            low level program you are using, or alternatively use the methods
            provided on RenderSystemCapabilities to determine the options.
            @param index The logical constant index at which to place the parameter (each constant is
            a 4D integer)
            @param val Pointer to the values to write, must contain 4*count ints
            @param count The number of groups of 4 ints to write
        */
        void setConstant(size_t index, const uint *val, size_t count);
        /// @}

        /** @name Set constant by physical index
            You can use these methods if you have already derived the physical
            constant buffer location, for a slight speed improvement over using
            the named / logical index versions.
        */
        /// @{
        /** Write a series of values into the underlying
            constant buffer at the given physical index.
            @param physicalIndex The buffer position to start writing
            @param val Pointer to a list of values to write
            @param count The number of floats to write
        */
        template<typename T>
        void _writeRawConstants(size_t physicalIndex, const T* val, size_t count)
        {
            assert(physicalIndex + sizeof(T) * count <= mConstants.size());
            memcpy(&mConstants[physicalIndex], val, sizeof(T) * count);
        }
        /// @overload
        void _writeRawConstants(size_t physicalIndex, const double* val, size_t count);
        /// write values into register storage
        void _writeRegisters(size_t index, const int* val, size_t count);
        /** Write a Vector parameter to the program directly to
            the underlying constants buffer.
            @param physicalIndex The physical buffer index at which to place the parameter
            @param vec The value to set
            @param count The number of floats to write; if for example
            the uniform constant 'slot' is smaller than a Vector4
        */
        template <int dims, typename T>
        void _writeRawConstant(size_t physicalIndex, const Vector<dims, T>& vec, size_t count = dims)
        {
            _writeRawConstants(physicalIndex, vec.ptr(), std::min(count, (size_t)dims));
        }
        /** Write a single parameter to the program.
            @param physicalIndex The physical buffer index at which to place the parameter
            @param val The value to set
        */
        template<typename T>
        void _writeRawConstant(size_t physicalIndex, T val)
        {
            _writeRawConstants(physicalIndex, &val, 1);
        }
        /** Write a Matrix4 parameter to the program.
            @param physicalIndex The physical buffer index at which to place the parameter
            @param m The value to set
            @param elementCount actual element count used with shader
        */
        void _writeRawConstant(size_t physicalIndex, const Matrix4& m, size_t elementCount);
        /// @overload
        void _writeRawConstant(size_t physicalIndex, const Matrix3& m, size_t elementCount);
        /** Write a list of Matrix4 parameters to the program.
            @param physicalIndex The physical buffer index at which to place the parameter
            @param m The value to set
            @param numEntries Number of Matrix4 entries
        */
        void _writeRawConstant(size_t physicalIndex, const TransformBaseReal* m, size_t numEntries);
        /** Write a ColourValue parameter to the program.
            @param physicalIndex The physical buffer index at which to place the parameter
            @param colour The value to set
            @param count The number of floats to write; if for example
            the uniform constant 'slot' is smaller than a Vector4
        */
        void _writeRawConstant(size_t physicalIndex, const ColourValue& colour,
                               size_t count = 4);
        /// @}

        /** Read a series of floating point values from the underlying float
            constant buffer at the given physical index.
            @param physicalIndex The buffer position to start reading
            @param count The number of floats to read
            @param dest Pointer to a buffer to receive the values
        */
        void _readRawConstants(size_t physicalIndex, size_t count, float* dest);
        /** Read a series of integer values from the underlying integer
            constant buffer at the given physical index.
            @param physicalIndex The buffer position to start reading
            @param count The number of ints to read
            @param dest Pointer to a buffer to receive the values
        */
        void _readRawConstants(size_t physicalIndex, size_t count, int* dest);

        /** Get a specific GpuConstantDefinition for a named parameter.
            @note
            Only available if this parameters object has named parameters.
        */
        [[nodiscard]] auto getConstantDefinition(std::string_view name) const -> const GpuConstantDefinition&;

        /** Get the full list of GpuConstantDefinition instances.
            @note
            Only available if this parameters object has named parameters.
        */
        [[nodiscard]] auto getConstantDefinitions() const noexcept -> const GpuNamedConstants&;

        /** Get the current list of mappings from low-level logical param indexes
            to physical buffer locations in the float buffer.
            @note
            Only applicable to low-level programs.
        */
        [[nodiscard]] auto getLogicalBufferStruct() const noexcept -> const GpuLogicalBufferStructPtr& { return mLogicalToPhysical; }

        /** Retrieves the logical index relating to a physical index in the
            buffer, for programs which support that (low-level programs and
            high-level programs which use logical parameter indexes).
            @return std::numeric_limits<size_t>::max() if not found
        */
        auto getLogicalIndexForPhysicalIndex(size_t physicalIndex) -> size_t;
        /// Get a reference to the list of constants
        [[nodiscard]] auto getConstantList() const noexcept -> const ConstantList& { return mConstants; }
        /// Get a pointer to the 'nth' item in the float buffer
        auto getFloatPointer(size_t pos) -> float* { return (float*)&mConstants[pos]; }
        /// Get a pointer to the 'nth' item in the float buffer
        [[nodiscard]] auto getFloatPointer(size_t pos) const -> const float* { return (const float*)&mConstants[pos]; }
        /// Get a pointer to the 'nth' item in the double buffer
        auto getDoublePointer(size_t pos) -> double* { return (double*)&mConstants[pos]; }
        /// Get a pointer to the 'nth' item in the double buffer
        [[nodiscard]] auto getDoublePointer(size_t pos) const -> const double* { return (const double*)&mConstants[pos]; }
        /// Get a pointer to the 'nth' item in the int buffer
        auto getIntPointer(size_t pos) -> int* { return (int*)&mConstants[pos]; }
        /// Get a pointer to the 'nth' item in the int buffer
        [[nodiscard]] auto getIntPointer(size_t pos) const -> const int* { return (const int*)&mConstants[pos]; }
        /// Get a pointer to the 'nth' item in the uint buffer
        auto getUnsignedIntPointer(size_t pos) -> uint* { return (uint*)&mConstants[pos]; }
        /// Get a pointer to the 'nth' item in the uint buffer
        [[nodiscard]] auto getUnsignedIntPointer(size_t pos) const -> const uint* { return (const uint*)&mConstants[pos]; }

        /// get a pointer to register storage
        auto getRegPointer(size_t pos) -> int* { return &mRegisters[pos]; }
        /// @overload
        [[nodiscard]] auto getRegPointer(size_t pos) const -> const int* { return &mRegisters[pos]; }

        /// @name Automatically derived constants
        /// @{

        /// Get a reference to the list of auto constant bindings
        [[nodiscard]] auto getAutoConstantList() const noexcept -> const AutoConstantList& { return mAutoConstants; }

        /** Sets up a constant which will automatically be updated by the system.
            @remarks
            Vertex and fragment programs often need parameters which are to do with the
            current render state, or particular values which may very well change over time,
            and often between objects which are being rendered. This feature allows you
            to set up a certain number of predefined parameter mappings that are kept up to
            date for you.
            @param index The location in the constant list to place this updated constant every time
            it is changed. Note that because of the nature of the types, we know how big the
            parameter details will be so you don't need to set that like you do for manual constants.
            @param acType The type of automatic constant to set
            @param extraInfo If the constant type needs more information (like a light index or array size) put it here.
        */
        void setAutoConstant(size_t index, AutoConstantType acType, uint32 extraInfo = 0);
        /// @overload
        void setAutoConstantReal(size_t index, AutoConstantType acType, float rData);
        /// @overload
        void setAutoConstant(size_t index, AutoConstantType acType, uint16 extraInfo1, uint16 extraInfo2)
        {
            setAutoConstant(index, acType, (uint32)extraInfo1 | ((uint32)extraInfo2) << 16);
        }

        /** As setAutoConstant, but sets up the auto constant directly against a
            physical buffer index.
        */
        void _setRawAutoConstant(size_t physicalIndex, AutoConstantType acType, uint32 extraInfo,
                                 GpuParamVariability variability, uint8 elementSize = 4);
        /** As setAutoConstantReal, but sets up the auto constant directly against a
            physical buffer index.
        */
        void _setRawAutoConstantReal(size_t physicalIndex, AutoConstantType acType, float rData,
                                     GpuParamVariability variability, uint8 elementSize = 4);


        /** Unbind an auto constant so that the constant is manually controlled again. */
        void clearAutoConstant(size_t index);

        /** Clears all the existing automatic constants. */
        void clearAutoConstants();

        /** Gets the automatic constant bindings currently in place. */
        [[nodiscard]] auto getAutoConstants() const noexcept -> const AutoConstantList& {
            return mAutoConstants;
        }

        /// Gets the number of int constants that have been set
        [[nodiscard]] auto getAutoConstantCount() const -> size_t { return mAutoConstants.size(); }
        /** Gets a specific Auto Constant entry if index is in valid range
            otherwise returns a NULL
            @param index which entry is to be retrieved
        */
        auto getAutoConstantEntry(const size_t index) -> AutoConstantEntry*;
        /** Returns true if this instance has any automatic constants. */
        [[nodiscard]] auto hasAutoConstants() const -> bool { return !(mAutoConstants.empty()); }
        /** Finds an auto constant that's affecting a given logical parameter
            index for floating-point values.
            @note Only applicable for low-level programs.
        */
        auto findFloatAutoConstantEntry(size_t logicalIndex) -> const AutoConstantEntry*;
        /** Finds an auto constant that's affecting a given named parameter index.
            @note Only applicable to high-level programs.
        */
        [[nodiscard]] auto findAutoConstantEntry(std::string_view paramName) const -> const AutoConstantEntry*;
        /** Finds an auto constant that's affecting a given physical position in
            the floating-point buffer
        */
        [[nodiscard]] auto _findRawAutoConstantEntryFloat(size_t physicalIndex) const -> const AutoConstantEntry*;
        /** Sets up a constant which will automatically be updated by the system.
            @remarks
            Vertex and fragment programs often need parameters which are to do with the
            current render state, or particular values which may very well change over time,
            and often between objects which are being rendered. This feature allows you
            to set up a certain number of predefined parameter mappings that are kept up to
            date for you.
            @note
            This named option will only work if you are using a parameters object created
            from a high-level program (HighLevelGpuProgram).
            @param name The name of the parameter
            @param acType The type of automatic constant to set
            @param extraInfo If the constant type needs more information (like a light index) put it here.
        */
        void setNamedAutoConstant(std::string_view name, AutoConstantType acType, uint32 extraInfo = 0);
        /// @overload
        void setNamedAutoConstantReal(std::string_view name, AutoConstantType acType, Real rData);
        /// @overload
        void setNamedAutoConstant(std::string_view name, AutoConstantType acType, uint16 extraInfo1, uint16 extraInfo2)
        {
            setNamedAutoConstant(name, acType, (uint32)extraInfo1 | ((uint32)extraInfo2) << 16);
        }

        /// @deprecated use AutoConstantType::TIME directly
        void setNamedConstantFromTime(std::string_view name, Real factor)
        {
            setNamedAutoConstantReal(name, AutoConstantType::TIME, factor);
        }

        /** Unbind an auto constant so that the constant is manually controlled again. */
        void clearNamedAutoConstant(std::string_view name);
        /// @}

        /** Update automatic parameters.
            @param source The source of the parameters
            @param variabilityMask A mask of GpuParamVariability which identifies which autos will need updating
        */
        void _updateAutoParams(const AutoParamDataSource* source, GpuParamVariability variabilityMask);

        /** Tells the program whether to ignore missing parameters or not.
         */
        void setIgnoreMissingParams(bool state) { mIgnoreMissingParams = state; }

        /// @name Set constant by name
        /// @{
        /** Sets a single value constant parameter to the program.

            Different types of GPU programs support different types of constant parameters.
            For example, it's relatively common to find that vertex programs only support
            floating point constants, and that fragment programs only support integer (fixed point)
            parameters. This can vary depending on the program version supported by the
            graphics card being used. You should consult the documentation for the type of
            low level program you are using, or alternatively use the methods
            provided on RenderSystemCapabilities to determine the options.

            Another possible limitation is that some systems only allow constants to be set
            on certain boundaries, e.g. in sets of 4 values for example. Again, see
            RenderSystemCapabilities for full details.
            @note
            This named option will only work if you are using a parameters object created
            from a high-level program (HighLevelGpuProgram).
            @param name The name of the parameter
            @param val The value to set
        */
        void setNamedConstant(std::string_view name, Real val);
        /// @overload
        void setNamedConstant(std::string_view name, int val);
        /// @overload
        void setNamedConstant(std::string_view name, uint val);
        /// @overload
        void setNamedConstant(std::string_view name, const Vector4& val);
        /// @overload
        void setNamedConstant(std::string_view name, const Vector3& val);
        /// @overload
        void setNamedConstant(std::string_view name, const Vector2& val);
        /// @overload
        void setNamedConstant(std::string_view name, const Matrix4& val);
        /// @overload
        void setNamedConstant(std::string_view name, const ColourValue& colour);
        /** Sets a list of Matrix4 parameters to the program.
            @param name The name of the parameter; this must be the first index of an array,
            for examples 'matrices[0]'
            NB since a Matrix4 is 16 floats long, so each entry will take up 4 indexes.
            @param m Pointer to an array of matrices to set
            @param numEntries Number of Matrix4 entries
        */
        void setNamedConstant(std::string_view name, const Matrix4* m, size_t numEntries);
        /** Sets a multiple value constant parameter to the program.

            Some systems only allow constants to be set on certain boundaries,
            e.g. in sets of 4 values for example. The 'multiple' parameter allows
            you to control that although you should only change it if you know
            your chosen language supports that (at the time of writing, only
            GLSL allows constants which are not a multiple of 4).
            @note
            This named option will only work if you are using a parameters object created
            from a high-level program (HighLevelGpuProgram).
            @param name The name of the parameter
            @param val Pointer to the values to write
            @param count The number of 'multiples' of floats to write
            @param multiple The number of raw entries in each element to write,
            the default is 4 so count = 1 would write 4 floats.
        */
        void setNamedConstant(std::string_view name, const float *val, size_t count,
                              size_t multiple = 4);
        /// @overload
        void setNamedConstant(std::string_view name, const double *val, size_t count,
                              size_t multiple = 4);
        /// @overload
        void setNamedConstant(std::string_view name, const int *val, size_t count,
                              size_t multiple = 4);
        /// @overload
        void setNamedConstant(std::string_view name, const uint *val, size_t count,
                              size_t multiple = 4);
        /// @}
        /** Find a constant definition for a named parameter.
            @remarks
            This method returns null if the named parameter did not exist, unlike
            getConstantDefinition which is more strict; unless you set the
            last parameter to true.
            @param name The name to look up
            @param throwExceptionIfMissing If set to true, failure to find an entry
            will throw an exception.
        */
        [[nodiscard]] auto _findNamedConstantDefinition(
            std::string_view name, bool throwExceptionIfMissing = false) const -> const GpuConstantDefinition*;
        /** Gets the physical buffer index associated with a logical float constant index.
            @note Only applicable to low-level programs.
            @param logicalIndex The logical parameter index
            @param requestedSize The requested size - pass 0 to ignore missing entries
            and return std::numeric_limits<size_t>::max()
            @param variability
            @param type
        */
        auto _getConstantPhysicalIndex(size_t logicalIndex, size_t requestedSize, GpuParamVariability variability, BaseConstantType type) -> size_t;
        /** Sets whether or not we need to transpose the matrices passed in from the rest of OGRE.
            @remarks
            D3D uses transposed matrices compared to GL and OGRE; this is not important when you
            use programs which are written to process row-major matrices, such as those generated
            by Cg, but if you use a program written to D3D's matrix layout you will need to enable
            this flag.
        */
        void setTransposeMatrices(bool val) { mTransposeMatrices = val; }
        /// Gets whether or not matrices are to be transposed when set
        [[nodiscard]] auto getTransposeMatrices() const noexcept -> bool { return mTransposeMatrices; }

        /** Copies the values of all constants (including auto constants) from another
            GpuProgramParameters object.
            @note This copes the internal storage of the paarameters object and therefore
            can only be used for parameters objects created from the same GpuProgram.
            To merge parameters that match from different programs, use copyMatchingNamedConstantsFrom.
        */
        void copyConstantsFrom(const GpuProgramParameters& source);

        /** Copies the values of all matching named constants (including auto constants) from
            another GpuProgramParameters object.
            @remarks
            This method iterates over the named constants in another parameters object
            and copies across the values where they match. This method is safe to
            use when the 2 parameters objects came from different programs, but only
            works for named parameters.
        */
        void copyMatchingNamedConstantsFrom(const GpuProgramParameters& source);

        /** gets the auto constant definition associated with name if found else returns NULL
            @param name The name of the auto constant
        */
        static auto getAutoConstantDefinition(std::string_view name) -> const AutoConstantDefinition*;
        /** gets the auto constant definition using an index into the auto constant definition array.
            If the index is out of bounds then NULL is returned;
            @param idx The auto constant index
        */
        static auto getAutoConstantDefinition(GpuProgramParameters::AutoConstantType idx) -> const AutoConstantDefinition*;
        /** Returns the number of auto constant definitions
         */
        static auto getNumAutoConstantDefinitions() -> size_t;


        /** increments the multipass number entry by 1 if it exists
         */
        void incPassIterationNumber();

        /// @name Shared Parameters
        /// @{
        /** Use a set of shared parameters in this parameters object.
            @remarks
            Allows you to use a set of shared parameters to automatically update
            this parameter set.
        */
        void addSharedParameters(GpuSharedParametersPtr sharedParams);

        /** Use a set of shared parameters in this parameters object.
            @remarks
            Allows you to use a set of shared parameters to automatically update
            this parameter set.
            @param sharedParamsName The name of a shared parameter set as defined in
            GpuProgramManager
        */
        void addSharedParameters(std::string_view sharedParamsName);

        /** Returns whether this parameter set is using the named shared parameter set. */
        [[nodiscard]] auto isUsingSharedParameters(std::string_view sharedParamsName) const -> bool;

        /** Stop using the named shared parameter set. */
        void removeSharedParameters(std::string_view sharedParamsName);

        /** Stop using all shared parameter sets. */
        void removeAllSharedParameters();

        /** Get the list of shared parameter sets. */
        [[nodiscard]] auto getSharedParameters() const noexcept -> const GpuSharedParamUsageList&;

        /** Update the parameters by copying the data from the shared
            parameters.
            @note This method  may not actually be called if the RenderSystem
            supports using shared parameters directly in their own shared buffer; in
            which case the values should not be copied out of the shared area
            into the individual parameter set, but bound separately.
        */
        void _copySharedParams();

        /** Update the HardwareBuffer based backing of referenced shared parameters
         *
         * falls back to _copySharedParams() if a shared parameter is not hardware backed
         */
        void _updateSharedParams();
        /// @}

        [[nodiscard]] auto calculateSize() const -> size_t;
    };

    /** @} */
    /** @} */
}
