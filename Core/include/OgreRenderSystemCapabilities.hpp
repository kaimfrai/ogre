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
export module Ogre.Core:RenderSystemCapabilities;

export import :MemoryAllocatorConfig;
export import :Prerequisites;

export import <set>;
export import <string>;

export
namespace Ogre {
class Log;
}  // namespace Ogre

// Because there are more than 32 possible Capabilities, more than 1 int is needed to store them all.
// In fact, an array of integers is used to store capabilities. However all the capabilities are defined in the single
// enum. The only way to know which capabilities should be stored where in the array is to use some of the 32 bits
// to record the category of the capability.  These top few bits are used as an index into mCapabilities array
// The lower bits are used to identify each capability individually by setting 1 bit for each
// Identifies how many bits are reserved for categories
// NOTE: Although 4 bits (currently) are enough
export
auto constexpr CAPS_CATEGORY_SIZE = 4;

export
auto constexpr inline OGRE_CAPS_BITSHIFT = (32 - CAPS_CATEGORY_SIZE);

export
auto constexpr inline CAPS_CATEGORY_MASK = (((1 << CAPS_CATEGORY_SIZE) - 1) << OGRE_CAPS_BITSHIFT);

export
auto constexpr OGRE_CAPS_VALUE(auto cat, int val)
{
    return ((std::to_underlying(cat) << OGRE_CAPS_BITSHIFT) | (1 << val));
}

export
namespace Ogre
{
    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup RenderSystem
    *  @{
    */

    /// Enumerates the categories of capabilities
    enum class CapabilitiesCategory
    {
        COMMON = 0,
        COMMON_2 = 1,
        D3D9 = 2,
        GL = 3,
        COMMON_3 = 4,
        /// Placeholder for max value
        COUNT = 5
    };

    /// Enum describing the different hardware capabilities we want to check for
    /// OGRE_CAPS_VALUE(a, b) defines each capability
    /// a is the category (which can be from 0 to 15)
    /// b is the value (from 0 to 27)
    enum class Capabilities
    {
        /// specifying a "-1" in the index buffer starts a new draw command.
        PRIMITIVE_RESTART = OGRE_CAPS_VALUE(CapabilitiesCategory::COMMON, 0),
        /// GL ES2/ES3 does not support generating mipmaps for compressed formats in hardware
        AUTOMIPMAP_COMPRESSED = OGRE_CAPS_VALUE(CapabilitiesCategory::COMMON, 1),
        /// Supports anisotropic texture filtering
        ANISOTROPY              = OGRE_CAPS_VALUE(CapabilitiesCategory::COMMON, 2),
        /// Supports depth clamping
        DEPTH_CLAMP             = OGRE_CAPS_VALUE(CapabilitiesCategory::COMMON, 3),
        /// Supports linewidth != 1.0
        WIDE_LINES              = OGRE_CAPS_VALUE(CapabilitiesCategory::COMMON, 4),
        /// Supports hardware stencil buffer
        HWSTENCIL               = OGRE_CAPS_VALUE(CapabilitiesCategory::COMMON, 5),
        /// Supports read/write buffers with atomic counters (e.g. RWStructuredBuffer or SSBO)
        READ_WRITE_BUFFERS      = OGRE_CAPS_VALUE(CapabilitiesCategory::COMMON, 6),
        /// Supports compressed textures in the ASTC format
        TEXTURE_COMPRESSION_ASTC = OGRE_CAPS_VALUE(CapabilitiesCategory::COMMON, 7),
        /// Supports 32bit hardware index buffers
        _32BIT_INDEX             = OGRE_CAPS_VALUE(CapabilitiesCategory::COMMON, 8),
        /// Supports vertex programs (vertex shaders)
        /// @deprecated All targetted APIs by Ogre support this feature
        VERTEX_PROGRAM          = OGRE_CAPS_VALUE(CapabilitiesCategory::COMMON, 9),
        /// Supports hardware tessellation domain programs
        TESSELLATION_DOMAIN_PROGRAM = OGRE_CAPS_VALUE(CapabilitiesCategory::COMMON, 10),
        /// Supports 2D Texture Arrays
        TEXTURE_2D_ARRAY        = OGRE_CAPS_VALUE(CapabilitiesCategory::COMMON, 11),
        /// Supports separate stencil updates for both front and back faces
        TWO_SIDED_STENCIL       = OGRE_CAPS_VALUE(CapabilitiesCategory::COMMON, 12),
        /// Supports wrapping the stencil value at the range extremeties
        STENCIL_WRAP            = OGRE_CAPS_VALUE(CapabilitiesCategory::COMMON, 13),
        /// Supports hardware occlusion queries
        HWOCCLUSION             = OGRE_CAPS_VALUE(CapabilitiesCategory::COMMON, 14),
        /// Supports user clipping planes
        USER_CLIP_PLANES        = OGRE_CAPS_VALUE(CapabilitiesCategory::COMMON, 15),
        /// Supports hardware compute programs
        COMPUTE_PROGRAM         = OGRE_CAPS_VALUE(CapabilitiesCategory::COMMON, 16),
        /// Supports 1d textures
        TEXTURE_1D              = OGRE_CAPS_VALUE(CapabilitiesCategory::COMMON, 17),
        /// Supports hardware render-to-texture (bigger than framebuffer)
        HWRENDER_TO_TEXTURE     = OGRE_CAPS_VALUE(CapabilitiesCategory::COMMON, 18),
        /// Supports float textures and render targets
        TEXTURE_FLOAT           = OGRE_CAPS_VALUE(CapabilitiesCategory::COMMON, 19),
        /// Supports non-power of two textures
        NON_POWER_OF_2_TEXTURES = OGRE_CAPS_VALUE(CapabilitiesCategory::COMMON, 20),
        /// Supports 3d (volume) textures
        TEXTURE_3D              = OGRE_CAPS_VALUE(CapabilitiesCategory::COMMON, 21),
        /// Supports basic point sprite rendering
        POINT_SPRITES           = OGRE_CAPS_VALUE(CapabilitiesCategory::COMMON, 22),
        /// Supports extra point parameters (minsize, maxsize, attenuation)
        POINT_EXTENDED_PARAMETERS = OGRE_CAPS_VALUE(CapabilitiesCategory::COMMON, 23),
        /// Supports vertex texture fetch
        VERTEX_TEXTURE_FETCH = OGRE_CAPS_VALUE(CapabilitiesCategory::COMMON, 24),
        /// Supports mipmap LOD biasing
        MIPMAP_LOD_BIAS = OGRE_CAPS_VALUE(CapabilitiesCategory::COMMON, 25),
        /// Supports hardware geometry programs
        GEOMETRY_PROGRAM = OGRE_CAPS_VALUE(CapabilitiesCategory::COMMON, 26),
        /// Supports rendering to vertex buffers
        HWRENDER_TO_VERTEX_BUFFER = OGRE_CAPS_VALUE(CapabilitiesCategory::COMMON, 27),

        /// Supports compressed textures
        TEXTURE_COMPRESSION = OGRE_CAPS_VALUE(CapabilitiesCategory::COMMON_2, 0),
        /// Supports compressed textures in the DXT/ST3C formats
        TEXTURE_COMPRESSION_DXT = OGRE_CAPS_VALUE(CapabilitiesCategory::COMMON_2, 1),
        /// Supports compressed textures in the VTC format
        TEXTURE_COMPRESSION_VTC = OGRE_CAPS_VALUE(CapabilitiesCategory::COMMON_2, 2),
        /// Supports compressed textures in the PVRTC format
        TEXTURE_COMPRESSION_PVRTC = OGRE_CAPS_VALUE(CapabilitiesCategory::COMMON_2, 3),
        /// Supports compressed textures in the ATC format
        TEXTURE_COMPRESSION_ATC = OGRE_CAPS_VALUE(CapabilitiesCategory::COMMON_2, 4),
        /// Supports compressed textures in the ETC1 format
        TEXTURE_COMPRESSION_ETC1 = OGRE_CAPS_VALUE(CapabilitiesCategory::COMMON_2, 5),
        /// Supports compressed textures in the ETC2 format
        TEXTURE_COMPRESSION_ETC2 = OGRE_CAPS_VALUE(CapabilitiesCategory::COMMON_2, 6),
        /// Supports compressed textures in BC4 and BC5 format (DirectX feature level 10_0)
        TEXTURE_COMPRESSION_BC4_BC5 = OGRE_CAPS_VALUE(CapabilitiesCategory::COMMON_2, 7),
        /// Supports compressed textures in BC6H and BC7 format (DirectX feature level 11_0)
        TEXTURE_COMPRESSION_BC6H_BC7 = OGRE_CAPS_VALUE(CapabilitiesCategory::COMMON_2, 8),
        /// Supports fixed-function pipeline
        FIXED_FUNCTION = OGRE_CAPS_VALUE(CapabilitiesCategory::COMMON_2, 9),
        /// Supports MRTs with different bit depths
        MRT_DIFFERENT_BIT_DEPTHS = OGRE_CAPS_VALUE(CapabilitiesCategory::COMMON_2, 10),
        /// Supports Alpha to Coverage (A2C)
        ALPHA_TO_COVERAGE = OGRE_CAPS_VALUE(CapabilitiesCategory::COMMON_2, 11),
        /// Supports reading back compiled shaders
        CAN_GET_COMPILED_SHADER_BUFFER = OGRE_CAPS_VALUE(CapabilitiesCategory::COMMON_2, 12),
        /// Supports HW gamma, both in the framebuffer and as texture.
        HW_GAMMA = OGRE_CAPS_VALUE(CapabilitiesCategory::COMMON_2, 13),
        /// Supports using the MAIN depth buffer for RTTs. D3D 9&10, OGL w/FBO support unknown
        /// (undefined behavior?), OGL w/ copy supports it
        RTT_MAIN_DEPTHBUFFER_ATTACHABLE = OGRE_CAPS_VALUE(CapabilitiesCategory::COMMON_2, 14),
        /// Supports attaching a depth buffer to an RTT that has width & height less or equal than RTT's.
        /// Otherwise must be of _exact_ same resolution. D3D 9, OGL 3.0 (not 2.0, not D3D10)
        RTT_DEPTHBUFFER_RESOLUTION_LESSEQUAL = OGRE_CAPS_VALUE(CapabilitiesCategory::COMMON_2, 15),
        /// Supports using vertex buffers for instance data
        VERTEX_BUFFER_INSTANCE_DATA = OGRE_CAPS_VALUE(CapabilitiesCategory::COMMON_2, 16),
        /// Supports hardware tessellation hull programs
        TESSELLATION_HULL_PROGRAM = OGRE_CAPS_VALUE(CapabilitiesCategory::COMMON_2, 17),

        // ***** DirectX specific caps *****
        /// Is DirectX feature "per stage constants" supported
        PERSTAGECONSTANT = OGRE_CAPS_VALUE(CapabilitiesCategory::D3D9, 0),
        /// D3D11: supports reading back the inactive depth-stencil buffer as texture
        READ_BACK_AS_TEXTURE = OGRE_CAPS_VALUE(CapabilitiesCategory::D3D9, 1),
        /// the renderer will try to use W-buffers when available
        /// W-buffers are enabled by default for 16bit depth buffers and disabled for all other
        /// depths.
        WBUFFER              = OGRE_CAPS_VALUE(CapabilitiesCategory::D3D9, 2),
        /// D3D11: Supports asynchronous hardware occlusion queries
        HWOCCLUSION_ASYNCHRONOUS = OGRE_CAPS_VALUE(CapabilitiesCategory::D3D9, 3),
        HWRENDER_TO_TEXTURE_3D = OGRE_CAPS_VALUE(CapabilitiesCategory::D3D9, 4),

        // ***** GL Specific Caps *****
        /// Support for PBuffer
        PBUFFER          = OGRE_CAPS_VALUE(CapabilitiesCategory::GL, 0),
        /// Support for Separate Shader Objects
        SEPARATE_SHADER_OBJECTS = OGRE_CAPS_VALUE(CapabilitiesCategory::GL, 1),
        /// Support for Vertex Array Objects (VAOs)
        VAO              = OGRE_CAPS_VALUE(CapabilitiesCategory::GL, 2),
        /// with Separate Shader Objects the gl_PerVertex interface block must be redeclared
        /// but some drivers misbehave and do not compile if we do so
        GLSL_SSO_REDECLARE = OGRE_CAPS_VALUE(CapabilitiesCategory::GL, 3),
        /// Supports debugging/ profiling events
        DEBUG = OGRE_CAPS_VALUE(CapabilitiesCategory::GL, 4),
        /// RS can map driver buffer storage directly instead of using a shadow buffer
        MAPBUFFER = OGRE_CAPS_VALUE(CapabilitiesCategory::GL, 5),

        // deprecated caps, all aliasing to VERTEX_PROGRAM
        /// @deprecated assume present
        INFINITE_FAR_PLANE = VERTEX_PROGRAM,
        /// @deprecated assume present
        FRAGMENT_PROGRAM = VERTEX_PROGRAM
    };

    /// DriverVersion is used by RenderSystemCapabilities and both GL and D3D9
    /// to store the version of the current GPU driver
    struct DriverVersion 
    {
        int major;
        int minor;
        int release;
        int build;

        DriverVersion() 
        {
            major = minor = release = build = 0;
        }

        [[nodiscard]] auto toString() const -> String;
        void fromString(std::string_view versionString);
    };

    /** Enumeration of GPU vendors. */
    enum class GPUVendor
    {
        UNKNOWN = 0,
        NVIDIA,
        AMD,
        INTEL,
        IMAGINATION_TECHNOLOGIES,
        APPLE,  //!< Apple Software Renderer
        NOKIA,
        MS_SOFTWARE, //!< Microsoft software device
        MS_WARP, //!< Microsoft WARP (Windows Advanced Rasterization Platform) software device - http://msdn.microsoft.com/en-us/library/dd285359.aspx
        ARM, //!< For the Mali chipsets
        QUALCOMM,
        MOZILLA, //!<  WebGL on Mozilla/Firefox based browser
        WEBKIT, //!< WebGL on WebKit/Chrome base browser
        /// placeholder
        VENDOR_COUNT
    };

    /** This class stores the capabilities of the graphics card.
    @remarks
    This information is set by the individual render systems.
    */
    class RenderSystemCapabilities : public RenderSysAlloc
    {

    public:

        using ShaderProfiles = std::set<std::string, std::less<>>;
    private:
        /// This is used to build a database of RSC's
        /// if a RSC with same name, but newer version is introduced, the older one 
        /// will be removed
        DriverVersion mDriverVersion;
        /// GPU Vendor
        GPUVendor mVendor{GPUVendor::UNKNOWN};

        static std::string_view msGPUVendorStrings[std::to_underlying(GPUVendor::VENDOR_COUNT)];
        static void initVendorStrings();

        /// The number of texture units available
        ushort mNumTextureUnits{0};
        /// The stencil buffer bit depth
        ushort mStencilBufferBitDepth{8};
        /// Stores the capabilities flags.
        int mCapabilities[std::to_underlying(CapabilitiesCategory::COUNT)];
        /// Which categories are relevant
        bool mCategoryRelevant[std::to_underlying(CapabilitiesCategory::COUNT)];
        /// The name of the device as reported by the render system
        String mDeviceName;
        /// The identifier associated with the render system for which these capabilities are valid
        String mRenderSystemName;

        /// The number of floating-point 4-vector constants vertex programs support
        ushort mVertexProgramConstantFloatCount;
        /// The number of floating-point 4-vector constants geometry programs support
        ushort mGeometryProgramConstantFloatCount;
        /// The number of floating-point 4-vector constants fragment programs support
        ushort mFragmentProgramConstantFloatCount;
        /// The number of simultaneous render targets supported
        ushort mNumMultiRenderTargets{1};
        /// The maximum point size
        Real mMaxPointSize;
        /// Are non-POW2 textures feature-limited?
        bool mNonPOW2TexturesLimited{false};
        /// The maximum supported anisotropy
        Real mMaxSupportedAnisotropy{0};
        /// The number of vertex texture units supported
        ushort mNumVertexTextureUnits;
        /// Are vertex texture units shared with fragment processor?
        bool mVertexTextureUnitsShared{false};
        /// The number of vertices a geometry program can emit in a single run
        int mGeometryProgramNumOutputVertices{0};


        /// The list of supported shader profiles
        ShaderProfiles mSupportedShaderProfiles;

        /// The number of floating-point 4-vector constants tessellation Hull programs support
        ushort mTessellationHullProgramConstantFloatCount;
        /// The number of floating-point 4-vector constants tessellation Domain programs support
        ushort mTessellationDomainProgramConstantFloatCount;
        /// The number of floating-point 4-vector constants compute programs support
        ushort mComputeProgramConstantFloatCount;

        /// The number of vertex attributes available
        ushort mNumVertexAttributes{1};
    public: 
        RenderSystemCapabilities ();

        /** Set the driver version. */
        void setDriverVersion(const DriverVersion& version)
        {
            mDriverVersion = version;
        }

        void parseDriverVersionFromString(std::string_view versionString)
        {
            DriverVersion version;
            version.fromString(versionString);
            setDriverVersion(version);
        }


        [[nodiscard]] auto getDriverVersion() const -> DriverVersion
        {
            return mDriverVersion;
        }

        [[nodiscard]] auto getVendor() const -> GPUVendor
        {
            return mVendor;
        }

        void setVendor(GPUVendor v)
        {
            mVendor = v;
        }

        /// Parse and set vendor
        void parseVendorFromString(std::string_view vendorString)
        {
            setVendor(vendorFromString(vendorString));
        }

        /// Convert a vendor string to an enum
        static auto vendorFromString(std::string_view vendorString) -> GPUVendor;
        /// Convert a vendor enum class to a string
        static auto vendorToString(GPUVendor v) -> std::string_view ;

        [[nodiscard]] auto isDriverOlderThanVersion(const DriverVersion &v) const -> bool
        {
            if (mDriverVersion.major < v.major)
                return true;
            else if (mDriverVersion.major == v.major && 
                mDriverVersion.minor < v.minor)
                return true;
            else if (mDriverVersion.major == v.major && 
                mDriverVersion.minor == v.minor && 
                mDriverVersion.release < v.release)
                return true;
            else if (mDriverVersion.major == v.major && 
                mDriverVersion.minor == v.minor && 
                mDriverVersion.release == v.release &&
                mDriverVersion.build < v.build)
                return true;
            return false;
        }

        void setNumTextureUnits(ushort num)
        {
            mNumTextureUnits = num;
        }

        /// @deprecated do not use
        void setStencilBufferBitDepth(ushort num)
        {
            mStencilBufferBitDepth = num;
        }

        /// The number of simultaneous render targets supported
        void setNumMultiRenderTargets(ushort num)
        {
            mNumMultiRenderTargets = num;
        }

        void setNumVertexAttributes(ushort num)
        {
            mNumVertexAttributes = num;
        }

        [[nodiscard]] auto getNumVertexAttributes() const noexcept -> ushort
        {
            return mNumVertexAttributes;
        }

        /** Returns the number of texture units the current output hardware
        supports.

        For use in rendering, this determines how many texture units the
        are available for multitexturing (i.e. rendering multiple 
        textures in a single pass). Where a Material has multiple 
        texture layers, it will try to use multitexturing where 
        available, and where it is not available, will perform multipass
        rendering to achieve the same effect. This property only applies
        to the fixed-function pipeline, the number available to the 
        programmable pipeline depends on the shader model in use.
        */
        [[nodiscard]] auto getNumTextureUnits() const noexcept -> ushort
        {
            return mNumTextureUnits;
        }

        /// @deprecated assume 8-bit stencil buffer
        [[nodiscard]] auto getStencilBufferBitDepth() const noexcept -> ushort
        {
            return mStencilBufferBitDepth;
        }

        /// The number of simultaneous render targets supported
        [[nodiscard]] auto getNumMultiRenderTargets() const noexcept -> ushort
        {
            return mNumMultiRenderTargets;
        }

        /** Returns true if capability is render system specific
        */
        [[nodiscard]] auto isCapabilityRenderSystemSpecific(const Capabilities c) const -> bool
        {
            int cat = std::to_underlying(c) >> OGRE_CAPS_BITSHIFT;
            if(cat == std::to_underlying(CapabilitiesCategory::GL) || cat == std::to_underlying(CapabilitiesCategory::D3D9))
                return true;
            return false;
        }

        /** Adds a capability flag
        */
        void setCapability(const Capabilities c) 
        { 
            int index = (CAPS_CATEGORY_MASK & std::to_underlying(c)) >> OGRE_CAPS_BITSHIFT;
            // zero out the index from the stored capability
            mCapabilities[index] |= (std::to_underlying(c) & ~CAPS_CATEGORY_MASK);
        }

        /** Remove a capability flag
        */
        void unsetCapability(const Capabilities c) 
        { 
            int index = (CAPS_CATEGORY_MASK & std::to_underlying(c)) >> OGRE_CAPS_BITSHIFT;
            // zero out the index from the stored capability
            mCapabilities[index] &= (~std::to_underlying(c) | CAPS_CATEGORY_MASK);
        }

        /** Checks for a capability
        */
        [[nodiscard]] auto hasCapability(const Capabilities c) const -> bool
        {
            int index = (CAPS_CATEGORY_MASK & std::to_underlying(c)) >> OGRE_CAPS_BITSHIFT;
            // test against
            if(mCapabilities[index] & (std::to_underlying(c) & ~CAPS_CATEGORY_MASK))
            {
                return true;
            }
            else
            {
                return false;
            }
        }

        /** Adds the profile to the list of supported profiles
        */
        void addShaderProfile(std::string_view profile);

        /** Remove a given shader profile, if present.
        */
        void removeShaderProfile(std::string_view profile);

        /** Returns true if profile is in the list of supported profiles
        */
        [[nodiscard]] auto isShaderProfileSupported(std::string_view profile) const -> bool;

        /** Returns a set of all supported shader profiles
        * */
        [[nodiscard]] auto getSupportedShaderProfiles() const noexcept -> const ShaderProfiles&
        {
            return mSupportedShaderProfiles;
        }


        /// The number of floating-point 4-vector constants vertex programs support
        [[nodiscard]] auto getVertexProgramConstantFloatCount() const noexcept -> ushort
        {
            return mVertexProgramConstantFloatCount;
        }
        /// The number of floating-point 4-vector constants geometry programs support
        [[nodiscard]] auto getGeometryProgramConstantFloatCount() const noexcept -> ushort
        {
            return mGeometryProgramConstantFloatCount;
        }
        /// The number of floating-point 4-vector constants fragment programs support
        [[nodiscard]] auto getFragmentProgramConstantFloatCount() const noexcept -> ushort
        {
            return mFragmentProgramConstantFloatCount;
        }

        /// sets the device name for Render system
        void setDeviceName(std::string_view name)
        {
            mDeviceName = name;
        }

        /// gets the device name for render system
        [[nodiscard]] auto getDeviceName() const -> String
        {
            return mDeviceName;
        }

        /// The number of floating-point 4-vector constants vertex programs support
        void setVertexProgramConstantFloatCount(ushort c)
        {
            mVertexProgramConstantFloatCount = c;
        }
        /// The number of floating-point 4-vector constants geometry programs support
        void setGeometryProgramConstantFloatCount(ushort c)
        {
            mGeometryProgramConstantFloatCount = c;
        }
        /// The number of floating-point 4-vector constants fragment programs support
        void setFragmentProgramConstantFloatCount(ushort c)
        {
            mFragmentProgramConstantFloatCount = c;
        }

        /// Maximum point screen size in pixels
        void setMaxPointSize(Real s)
        {
            mMaxPointSize = s;
        }
        /// Maximum point screen size in pixels
        [[nodiscard]] auto getMaxPointSize() const -> Real
        {
            return mMaxPointSize;
        }
        /// Non-POW2 textures limited
        void setNonPOW2TexturesLimited(bool l)
        {
            mNonPOW2TexturesLimited = l;
        }
        /** Are non-power of two textures limited in features?
        @remarks
        If the Capabilities::NON_POWER_OF_2_TEXTURES capability is set, but this
        method returns true, you can use non power of 2 textures only if:
        <ul><li>You load them explicitly with no mip maps</li>
        <li>You don't use DXT texture compression</li>
        <li>You use clamp texture addressing</li></ul>
        */
        [[nodiscard]] auto getNonPOW2TexturesLimited() const noexcept -> bool
        {
            return mNonPOW2TexturesLimited;
        }
        /// Set the maximum supported anisotropic filtering
        void setMaxSupportedAnisotropy(Real s)
        {
            mMaxSupportedAnisotropy = s;
        }
        /// Get the maximum supported anisotropic filtering
        [[nodiscard]] auto getMaxSupportedAnisotropy() const -> Real
        {
            return mMaxSupportedAnisotropy;
        }

        /// Set the number of vertex texture units supported
        void setNumVertexTextureUnits(ushort n)
        {
            mNumVertexTextureUnits = n;
        }
        /// Get the number of vertex texture units supported
        [[nodiscard]] auto getNumVertexTextureUnits() const noexcept -> ushort
        {
            return mNumVertexTextureUnits;
        }

        /// Set the number of vertices a single geometry program run can emit
        void setGeometryProgramNumOutputVertices(int numOutputVertices)
        {
            mGeometryProgramNumOutputVertices = numOutputVertices;
        }
        /// Get the number of vertices a single geometry program run can emit
        [[nodiscard]] auto getGeometryProgramNumOutputVertices() const noexcept -> int
        {
            return mGeometryProgramNumOutputVertices;
        }

        /// Get the identifier of the rendersystem from which these capabilities were generated
        [[nodiscard]] auto getRenderSystemName() const noexcept -> std::string_view
        {
            return mRenderSystemName;
        }
        ///  Set the identifier of the rendersystem from which these capabilities were generated
        void setRenderSystemName(std::string_view rs)
        {
            mRenderSystemName = rs;
        }

        /// Mark a category as 'relevant' or not, ie will it be reported
        void setCategoryRelevant(CapabilitiesCategory cat, bool relevant)
        {
            mCategoryRelevant[std::to_underlying(cat)] = relevant;
        }

        /// Return whether a category is 'relevant' or not, ie will it be reported
        auto isCategoryRelevant(CapabilitiesCategory cat) -> bool
        {
            return mCategoryRelevant[std::to_underlying(cat)];
        }



        /** Write the capabilities to the pass in Log */
        void log(Log* pLog) const;

        /// The number of floating-point 4-vector constants compute programs support
        void setComputeProgramConstantFloatCount(ushort c)
        {
            mComputeProgramConstantFloatCount = c;
        }
        /// The number of floating-point 4-vector constants fragment programs support
        [[nodiscard]] auto getComputeProgramConstantFloatCount() const noexcept -> ushort
        {
            return mComputeProgramConstantFloatCount;
        }
        /// The number of floating-point 4-vector constants fragment programs support
        [[nodiscard]] auto getTessellationDomainProgramConstantFloatCount() const noexcept -> ushort
        {
            return mTessellationDomainProgramConstantFloatCount;
        }
        /// The number of floating-point 4-vector constants tessellation Domain programs support
        void setTessellationDomainProgramConstantFloatCount(ushort c)
        {
            mTessellationDomainProgramConstantFloatCount = c;
        }
        /// The number of floating-point 4-vector constants fragment programs support
        [[nodiscard]] auto getTessellationHullProgramConstantFloatCount() const noexcept -> ushort
        {
            return mTessellationHullProgramConstantFloatCount;
        }
        /// The number of floating-point 4-vector constants tessellation Hull programs support
        void setTessellationHullProgramConstantFloatCount(ushort c)
        {
            mTessellationHullProgramConstantFloatCount = c;
        }
    };

    inline auto to_string(GPUVendor v) -> String { return std::string{RenderSystemCapabilities::vendorToString(v) }; }
    inline auto to_string(const DriverVersion& v) -> String { return v.toString(); }

    /** @} */
    /** @} */
} // namespace
