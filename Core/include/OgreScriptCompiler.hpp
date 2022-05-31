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

#ifndef OGRE_CORE_SCRIPTCOMPILER_H
#define OGRE_CORE_SCRIPTCOMPILER_H

#include <algorithm>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "OgreAny.hpp"
#include "OgreCommon.hpp"
#include "OgreGpuProgram.hpp"
#include "OgreMemoryAllocatorConfig.hpp"
#include "OgrePlatform.hpp"
#include "OgrePrerequisites.hpp"
#include "OgreScriptLoader.hpp"
#include "OgreScriptTranslator.hpp"
#include "OgreSharedPtr.hpp"
#include "OgreSingleton.hpp"
#include "OgreStringVector.hpp"

namespace Ogre
{
class Material;

    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup Script
    *  @{
    */
    /** These enums hold the types of the concrete parsed nodes */
    enum ConcreteNodeType
    {
        CNT_VARIABLE,
        CNT_VARIABLE_ASSIGN,
        CNT_WORD,
        CNT_IMPORT,
        CNT_QUOTE,
        CNT_LBRACE,
        CNT_RBRACE,
        CNT_COLON
    };

    /** The ConcreteNode is the struct that holds an un-conditioned sub-tree of parsed input */
    struct ConcreteNode;

    using ConcreteNodePtr = SharedPtr<ConcreteNode>;
    using ConcreteNodeList = std::list<ConcreteNodePtr>;
    using ConcreteNodeListPtr = SharedPtr<ConcreteNodeList>;
    struct ConcreteNode : public ScriptCompilerAlloc
    {
        String token, file;
        unsigned int line;
        ConcreteNodeType type;
        ConcreteNodeList children;
        ConcreteNode *parent;
    };

    /** This enum holds the types of the possible abstract nodes */
    enum AbstractNodeType
    {
        ANT_UNKNOWN,
        ANT_ATOM,
        ANT_OBJECT,
        ANT_PROPERTY,
        ANT_IMPORT,
        ANT_VARIABLE_SET,
        ANT_VARIABLE_ACCESS
    };
    class AbstractNode;
    using AbstractNodePtr = SharedPtr<AbstractNode>;
    using AbstractNodeList = std::list<AbstractNodePtr>;
    using AbstractNodeListPtr = SharedPtr<AbstractNodeList>;

    class AbstractNode : public AbstractNodeAlloc
    {
    public:
        String file;
        unsigned int line{0};
        AbstractNodeType type{ANT_UNKNOWN};
        AbstractNode *parent;
        Any context; // A holder for translation context data
    public:
        AbstractNode(AbstractNode *ptr);
        virtual ~AbstractNode()= default;
        /// Returns a new AbstractNode which is a replica of this one.
        [[nodiscard]] virtual AbstractNode *clone() const = 0;
        /// Returns a string value depending on the type of the AbstractNode.
        [[nodiscard]] virtual const String& getValue() const noexcept = 0;
    };

    /** This is an abstract node which cannot be broken down further */
    class AtomAbstractNode : public AbstractNode
    {
    public:
        String value;
        uint32 id{0};
    public:
        AtomAbstractNode(AbstractNode *ptr);
        [[nodiscard]] AbstractNode *clone() const override;
        [[nodiscard]] const String& getValue() const noexcept override { return value; }
    };

    /** This specific abstract node represents a script object */
    class ObjectAbstractNode : public AbstractNode
    {
    private:
        std::map<String,String> mEnv;
    public:
        String name, cls;
        std::vector<String> bases;
        uint32 id{0};
        bool abstract{false};
        AbstractNodeList children;
        AbstractNodeList values;
        AbstractNodeList overrides; // For use when processing object inheritance and overriding
    public:
        ObjectAbstractNode(AbstractNode *ptr);
        [[nodiscard]] AbstractNode *clone() const override;
        [[nodiscard]] const String& getValue() const noexcept override { return cls; }

        void addVariable(const String &name);
        void setVariable(const String &name, const String &value);
        [[nodiscard]] std::pair<bool,String> getVariable(const String &name) const;
        [[nodiscard]] const std::map<String,String> &getVariables() const;
    };

    /** This abstract node represents a script property */
    class PropertyAbstractNode : public AbstractNode
    {
    public:
        String name;
        uint32 id{0};
        AbstractNodeList values;
    public:
        PropertyAbstractNode(AbstractNode *ptr);
        [[nodiscard]] AbstractNode *clone() const override;
        [[nodiscard]] const String& getValue() const noexcept override { return name; }
    };

    /** This abstract node represents an import statement */
    class ImportAbstractNode : public AbstractNode
    {
    public:
        String target, source;
    public:
        ImportAbstractNode();
        [[nodiscard]] AbstractNode *clone() const override;
        [[nodiscard]] const String& getValue() const noexcept override { return target; }
    };

    /** This abstract node represents a variable assignment */
    class VariableAccessAbstractNode : public AbstractNode
    {
    public:
        String name;
    public:
        VariableAccessAbstractNode(AbstractNode *ptr);
        [[nodiscard]] AbstractNode *clone() const override;
        [[nodiscard]] const String& getValue() const noexcept override { return name; }
    };

    class ScriptCompilerEvent;
    class ScriptCompilerListener;

    /** This is the main class for the compiler. It calls the parser
        and processes the CST into an AST and then uses translators
        to translate the AST into the final resources.
    */
    class ScriptCompiler : public ScriptCompilerAlloc
    {
    public: // Externally accessible types
        //typedef std::map<String,uint32> IdMap;
        using IdMap = std::unordered_map<String, uint32>;

        // These are the built-in error codes
        enum{
            CE_STRINGEXPECTED,
            CE_NUMBEREXPECTED,
            CE_FEWERPARAMETERSEXPECTED,
            CE_VARIABLEEXPECTED,
            CE_UNDEFINEDVARIABLE,
            CE_OBJECTNAMEEXPECTED,
            CE_OBJECTALLOCATIONERROR,
            CE_INVALIDPARAMETERS,
            CE_DUPLICATEOVERRIDE,
            CE_UNEXPECTEDTOKEN,
            CE_OBJECTBASENOTFOUND,
            CE_REFERENCETOANONEXISTINGOBJECT,
            CE_DEPRECATEDSYMBOL
        };
        static String formatErrorCode(uint32 code);
    public:
        ScriptCompiler();
        virtual ~ScriptCompiler() = default;

        /// Takes in a string of script code and compiles it into resources
        /**
         * @param str The script code
         * @param source The source of the script code (e.g. a script file)
         * @param group The resource group to place the compiled resources into
         */
        bool compile(const String &str, const String &source, const String &group);
        /// Compiles resources from the given concrete node list
        bool compile(const ConcreteNodeListPtr &nodes, const String &group);
        /// Adds the given error to the compiler's list of errors
        void addError(uint32 code, const String &file, int line, const String &msg = "");
        /// Sets the listener used by the compiler
        void setListener(ScriptCompilerListener *listener);
        /// Returns the currently set listener
        ScriptCompilerListener *getListener();
        /// Returns the resource group currently set for this compiler
        [[nodiscard]] const String &getResourceGroup() const;
        /// Adds a name exclusion to the map
        /**
         * Name exclusions identify object types which cannot accept
         * names. This means that excluded types will always have empty names.
         * All values in the object header are stored as object values.
         */
        //void addNameExclusion(const String &type);
        /// Removes a name exclusion
        //void removeNameExclusion(const String &type);
        /// Internal method for firing the handleEvent method
        bool _fireEvent(ScriptCompilerEvent *evt, void *retval);

		/// Adds a custom word id which can be used for custom script translators
		/** 
		@param
		word The word to be registered.

		@return
		The word id for the registered word.
		
		@note
		If the word is already registered, the already registered id is returned.
		*/
		uint32 registerCustomWordId(const String &word);

    private: // Tree processing
        AbstractNodeListPtr convertToAST(const ConcreteNodeList &nodes);
        /// This built-in function processes import nodes
        void processImports(AbstractNodeList &nodes);
        /// Loads the requested script and converts it to an AST
        AbstractNodeListPtr loadImportPath(const String &name);
        /// Returns the abstract nodes from the given tree which represent the target
        AbstractNodeList locateTarget(const AbstractNodeList& nodes, const String &target);
        /// Handles object inheritance and variable expansion
        void processObjects(AbstractNodeList& nodes, const AbstractNodeList &top);
        /// Handles processing the variables
        void processVariables(AbstractNodeList& nodes);
        /// This function overlays the given object on the destination object following inheritance rules
        void overlayObject(const ObjectAbstractNode &source, ObjectAbstractNode& dest);
        /// Returns true if the given class is name excluded
        bool isNameExcluded(const ObjectAbstractNode& node, AbstractNode *parent);
        /// This function sets up the initial values in word id map
        void initWordMap();
    private:
        friend String getPropertyName(const ScriptCompiler *compiler, uint32 id);
        // Resource group
        String mGroup;
        // The word -> id conversion table
        IdMap mIds;

		// The largest registered id
		uint32 mLargestRegisteredWordId;

        // This is an environment map
        using Environment = std::map<String, String>;
        Environment mEnv;

        using ImportCacheMap = std::map<String, AbstractNodeListPtr>;
        ImportCacheMap mImports; // The set of imported scripts to avoid circular dependencies
        using ImportRequestMap = std::multimap<String, String>;
        ImportRequestMap mImportRequests; // This holds the target objects for each script to be imported

        // This stores the imports of the scripts, so they are separated and can be treated specially
        AbstractNodeList mImportTable;

        // Error list
        // The container for errors
        struct Error
        {
            String file, message;
            int line;
            uint32 code;
        };
        using ErrorList = std::list<Error>;
        ErrorList mErrors;

        // The listener
        ScriptCompilerListener *mListener{nullptr};
    private: // Internal helper classes and processors
        class AbstractTreeBuilder
        {
        private:
            AbstractNodeListPtr mNodes;
            AbstractNode *mCurrent{nullptr};
            ScriptCompiler *mCompiler;
        public:
            AbstractTreeBuilder(ScriptCompiler *compiler);
            [[nodiscard]] const AbstractNodeListPtr &getResult() const;
            void visit(ConcreteNode *node);
            static void visit(AbstractTreeBuilder *visitor, const ConcreteNodeList &nodes);
        };
        friend class AbstractTreeBuilder;
    public: // Public translator definitions
        // This enum are built-in word id values
        enum
        {
            ID_ON = 1,
            ID_OFF = 2,
            ID_TRUE = 1,
            ID_FALSE = 2,
            ID_YES = 1,
            ID_NO = 2
        };  
    };

    /**
     * This struct is a base class for events which can be thrown by the compilers and caught by
     * subscribers. There are a set number of standard events which are used by Ogre's core.
     * New event types may be derived for more custom compiler processing.
     */
    class ScriptCompilerEvent
    {
    public:
        String mType;

        ScriptCompilerEvent(String type):mType(std::move(type)){}
        virtual ~ScriptCompilerEvent()= default;
        ScriptCompilerEvent(const ScriptCompilerEvent&) = delete;
        ScriptCompilerEvent &operator = (const ScriptCompilerEvent&) = delete;
    };

    /** This is a listener for the compiler. The compiler can be customized with
        this listener. It lets you listen in on events occurring during compilation,
        hook them, and change the behavior.
    */
    class ScriptCompilerListener
    {
    public:
        ScriptCompilerListener();
        virtual ~ScriptCompilerListener() = default;

        /// Returns the concrete node list from the given file
        virtual ConcreteNodeListPtr importFile(ScriptCompiler *compiler, const String &name);
        /// Allows for responding to and overriding behavior before a CST is translated into an AST
        virtual void preConversion(ScriptCompiler *compiler, ConcreteNodeListPtr nodes);
        /// Allows vetoing of continued compilation after the entire AST conversion process finishes
        /**
         @remarks   Once the script is turned completely into an AST, including import
                    and override handling, this function allows a listener to exit
                    the compilation process.
         @return True continues compilation, false aborts
         */
        virtual bool postConversion(ScriptCompiler *compiler, const AbstractNodeListPtr&);
        /// Called when an error occurred
        virtual void handleError(ScriptCompiler *compiler, uint32 code, const String &file, int line, const String &msg);
        /// Called when an event occurs during translation, return true if handled
        /**
         @remarks   This function is called from the translators when an event occurs that
                    that can be responded to. Often this is overriding names, or it can be a request for
                    custom resource creation.
         @arg compiler A reference to the compiler
         @arg evt The event object holding information about the event to be processed
         @arg retval A possible return value from handlers
         @return True if the handler processed the event
        */
        virtual bool handleEvent(ScriptCompiler *compiler, ScriptCompilerEvent *evt, void *retval);
    };

    class ScriptTranslator;

    /** Manages threaded compilation of scripts. This script loader forwards
        scripts compilations to a specific compiler instance.
    */
    class ScriptCompilerManager : public Singleton<ScriptCompilerManager>, public ScriptLoader, public ScriptCompilerAlloc
    {
    private:
        // A list of patterns loaded by this compiler manager
        StringVector mScriptPatterns;

        // Stores a map from object types to the translators that handle them
        std::vector<ScriptTranslatorManager*> mManagers;

        // A pointer to the built-in ScriptTranslatorManager
        ::std::unique_ptr<ScriptTranslatorManager> mBuiltinTranslatorManager;

        // the specific compiler instance used
        ScriptCompiler mScriptCompiler;
    public:
        ScriptCompilerManager();
        ~ScriptCompilerManager() override = default;

        /// Sets the listener used for compiler instances
        void setListener(ScriptCompilerListener *listener);
        /// Returns the currently set listener used for compiler instances
        ScriptCompilerListener *getListener();

        /// Adds the given translator manager to the list of managers
        void addTranslatorManager(ScriptTranslatorManager *man);
        /// Removes the given translator manager from the list of managers
        void removeTranslatorManager(ScriptTranslatorManager *man);
        /// Clears all translator managers
        void clearTranslatorManagers();
        /// Retrieves a ScriptTranslator from the supported managers
        ScriptTranslator *getTranslator(const AbstractNodePtr &node);

		/// Adds a custom word id which can be used for custom script translators
		/** 
		@param
		word The word to be registered.

		@return
		The word id for the registered word.
		
		@note
		If the word is already registered, the already registered id is returned.
		*/
		uint32 registerCustomWordId(const String &word);

        /// Adds a script extension that can be handled (e.g. *.material, *.pu, etc.)
        void addScriptPattern(const String &pattern);
        /// @copydoc ScriptLoader::getScriptPatterns
        [[nodiscard]] const StringVector& getScriptPatterns() const noexcept override;
        /// @copydoc ScriptLoader::parseScript
        void parseScript(DataStreamPtr& stream, const String& groupName) override;
        /// @copydoc ScriptLoader::getLoadingOrder
        [[nodiscard]] Real getLoadingOrder() const override;

        /// @copydoc Singleton::getSingleton()
        static ScriptCompilerManager& getSingleton() noexcept;
        /// @copydoc Singleton::getSingleton()
        static ScriptCompilerManager* getSingletonPtr() noexcept;
    };

    /// @deprecated do not use
    class PreApplyTextureAliasesScriptCompilerEvent : public ScriptCompilerEvent
    {
    public:
        Material *mMaterial;
        AliasTextureNamePairList *mAliases;
        static String eventType;

        PreApplyTextureAliasesScriptCompilerEvent(Material *material, AliasTextureNamePairList *aliases)
            :ScriptCompilerEvent(eventType), mMaterial(material), mAliases(aliases){}
    };

    class ProcessResourceNameScriptCompilerEvent : public ScriptCompilerEvent
    {
    public:
        enum ResourceType
        {
            TEXTURE,
            MATERIAL,
            GPU_PROGRAM,
            COMPOSITOR
        };
        ResourceType mResourceType;
        String mName;
        static String eventType;

        ProcessResourceNameScriptCompilerEvent(ResourceType resourceType, String name)
            :ScriptCompilerEvent(eventType), mResourceType(resourceType), mName(std::move(name)){}     
    };

    class ProcessNameExclusionScriptCompilerEvent : public ScriptCompilerEvent
    {
    public:
        String mClass;
        AbstractNode *mParent;
        static String eventType;

        ProcessNameExclusionScriptCompilerEvent(String cls, AbstractNode *parent)
            :ScriptCompilerEvent(eventType), mClass(std::move(cls)), mParent(parent){}     
    };

    class CreateMaterialScriptCompilerEvent : public ScriptCompilerEvent
    {
    public:
        String mFile, mName, mResourceGroup;
        static String eventType;

        CreateMaterialScriptCompilerEvent(String file, String name, String resourceGroup)
            :ScriptCompilerEvent(eventType), mFile(std::move(file)), mName(std::move(name)), mResourceGroup(std::move(resourceGroup)){}  
    };

    class CreateGpuProgramScriptCompilerEvent : public ScriptCompilerEvent
    {
    public:
        String mFile, mName, mResourceGroup, mSource, mSyntax;
        GpuProgramType mProgramType;
        static String eventType;

        CreateGpuProgramScriptCompilerEvent(String file, String name, String resourceGroup, String source, 
            String syntax, GpuProgramType programType)
            :ScriptCompilerEvent(eventType), mFile(std::move(file)), mName(std::move(name)), mResourceGroup(std::move(resourceGroup)), mSource(std::move(source)), 
             mSyntax(std::move(syntax)), mProgramType(programType)
        {}  
    };

    class CreateGpuSharedParametersScriptCompilerEvent : public ScriptCompilerEvent
    {
    public:
        String mFile, mName, mResourceGroup;
        static String eventType;

        CreateGpuSharedParametersScriptCompilerEvent(String file, String name, String resourceGroup)
            :ScriptCompilerEvent(eventType), mFile(std::move(file)), mName(std::move(name)), mResourceGroup(std::move(resourceGroup)){}  
    };

    class CreateParticleSystemScriptCompilerEvent : public ScriptCompilerEvent
    {
    public:
        String mFile, mName, mResourceGroup;
        static String eventType;

        CreateParticleSystemScriptCompilerEvent(String file, String name, String resourceGroup)
            :ScriptCompilerEvent(eventType), mFile(std::move(file)), mName(std::move(name)), mResourceGroup(std::move(resourceGroup)){}  
    };

    class CreateCompositorScriptCompilerEvent : public ScriptCompilerEvent
    {
    public:
        String mFile, mName, mResourceGroup;
        static String eventType;

        CreateCompositorScriptCompilerEvent(String file, String name, String resourceGroup)
            :ScriptCompilerEvent(eventType), mFile(std::move(file)), mName(std::move(name)), mResourceGroup(std::move(resourceGroup)){}  
    };

    /// This enum defines the integer ids for keywords this compiler handles
    enum
    {
        ID_MATERIAL = 3,
        ID_VERTEX_PROGRAM,
        ID_GEOMETRY_PROGRAM,
        ID_FRAGMENT_PROGRAM,
        ID_TECHNIQUE,
        ID_PASS,
        ID_TEXTURE_UNIT,
        ID_VERTEX_PROGRAM_REF,
        ID_GEOMETRY_PROGRAM_REF,
        ID_FRAGMENT_PROGRAM_REF,
        ID_SHADOW_CASTER_VERTEX_PROGRAM_REF,
        ID_SHADOW_CASTER_FRAGMENT_PROGRAM_REF,
        ID_SHADOW_RECEIVER_VERTEX_PROGRAM_REF,
        ID_SHADOW_RECEIVER_FRAGMENT_PROGRAM_REF,
        ID_SHADOW_CASTER_MATERIAL,
        ID_SHADOW_RECEIVER_MATERIAL,
        
        ID_LOD_VALUES,
        ID_LOD_STRATEGY,
        ID_LOD_DISTANCES,
        ID_RECEIVE_SHADOWS,
        ID_TRANSPARENCY_CASTS_SHADOWS,
        ID_SET_TEXTURE_ALIAS,

        ID_SOURCE,
        ID_SYNTAX,
        ID_DEFAULT_PARAMS,
        ID_PARAM_INDEXED,
        ID_PARAM_NAMED,
        ID_PARAM_INDEXED_AUTO,
        ID_PARAM_NAMED_AUTO,

        ID_SCHEME,
        ID_LOD_INDEX,
        ID_GPU_VENDOR_RULE,
        ID_GPU_DEVICE_RULE,
        ID_INCLUDE, 
        ID_EXCLUDE, 

        ID_AMBIENT,
        ID_DIFFUSE,
        ID_SPECULAR,
        ID_EMISSIVE,
            ID_VERTEXCOLOUR,
        ID_SCENE_BLEND,
            ID_COLOUR_BLEND,
            ID_ONE,
            ID_ZERO,
            ID_DEST_COLOUR,
            ID_SRC_COLOUR,
            ID_ONE_MINUS_DEST_COLOUR,
            ID_ONE_MINUS_SRC_COLOUR,
            ID_DEST_ALPHA,
            ID_SRC_ALPHA,
            ID_ONE_MINUS_DEST_ALPHA,
            ID_ONE_MINUS_SRC_ALPHA,
        ID_SEPARATE_SCENE_BLEND,
        ID_SCENE_BLEND_OP,
            ID_REVERSE_SUBTRACT,
            ID_MIN,
            ID_MAX,
        ID_SEPARATE_SCENE_BLEND_OP,
        ID_DEPTH_CHECK,
        ID_DEPTH_WRITE,
        ID_DEPTH_FUNC,
        ID_DEPTH_BIAS,
        ID_ITERATION_DEPTH_BIAS,
            ID_ALWAYS_FAIL,
            ID_ALWAYS_PASS,
            ID_LESS_EQUAL,
            ID_LESS,
            ID_EQUAL,
            ID_NOT_EQUAL,
            ID_GREATER_EQUAL,
            ID_GREATER,
        ID_ALPHA_REJECTION,
        ID_ALPHA_TO_COVERAGE,
        ID_LIGHT_SCISSOR,
        ID_LIGHT_CLIP_PLANES,
        ID_TRANSPARENT_SORTING,
        ID_ILLUMINATION_STAGE,
            ID_DECAL,
        ID_CULL_HARDWARE,
            ID_CLOCKWISE,
            ID_ANTICLOCKWISE,
        ID_CULL_SOFTWARE,
            ID_BACK,
            ID_FRONT,
        ID_NORMALISE_NORMALS,
        ID_LIGHTING,
        ID_SHADING,
            ID_FLAT, 
            ID_GOURAUD,
            ID_PHONG,
        ID_POLYGON_MODE,
            ID_SOLID,
            ID_WIREFRAME,
            ID_POINTS,
        ID_POLYGON_MODE_OVERRIDEABLE,
        ID_FOG_OVERRIDE,
            ID_NONE,
            ID_LINEAR,
            ID_EXP,
            ID_EXP2,
        ID_COLOUR_WRITE,
        ID_MAX_LIGHTS,
        ID_START_LIGHT,
        ID_ITERATION,
            ID_ONCE,
            ID_ONCE_PER_LIGHT,
            ID_PER_LIGHT,
            ID_PER_N_LIGHTS,
            ID_POINT,
            ID_SPOT,
            ID_DIRECTIONAL,
        ID_LIGHT_MASK,
        ID_POINT_SIZE,
        ID_POINT_SPRITES,
        ID_POINT_SIZE_ATTENUATION,
        ID_POINT_SIZE_MIN,
        ID_POINT_SIZE_MAX,

        ID_TEXTURE_ALIAS,
        ID_TEXTURE,
            ID_1D,
            ID_2D,
            ID_3D,
            ID_CUBIC,
            ID_2DARRAY,
            ID_UNLIMITED,
            ID_ALPHA,
            ID_GAMMA,
        ID_ANIM_TEXTURE,
        ID_CUBIC_TEXTURE,
            ID_SEPARATE_UV,
            ID_COMBINED_UVW,
        ID_TEX_COORD_SET,
        ID_TEX_ADDRESS_MODE,
            ID_WRAP,
            ID_CLAMP,
            ID_BORDER,
            ID_MIRROR,
        ID_TEX_BORDER_COLOUR,
        ID_FILTERING,
            ID_BILINEAR,
            ID_TRILINEAR,
            ID_ANISOTROPIC,
        ID_CMPTEST,
            ID_ON,
            ID_OFF,
        ID_CMPFUNC,
        ID_MAX_ANISOTROPY,
        ID_MIPMAP_BIAS,
        ID_COLOUR_OP,
            ID_REPLACE,
            ID_ADD,
            ID_MODULATE,
            ID_ALPHA_BLEND,
        ID_COLOUR_OP_EX,
            ID_SOURCE1,
            ID_SOURCE2,
            ID_MODULATE_X2,
            ID_MODULATE_X4,
            ID_ADD_SIGNED,
            ID_ADD_SMOOTH,
            ID_SUBTRACT,
            ID_BLEND_DIFFUSE_COLOUR,
            ID_BLEND_DIFFUSE_ALPHA,
            ID_BLEND_TEXTURE_ALPHA,
            ID_BLEND_CURRENT_ALPHA,
            ID_BLEND_MANUAL,
            ID_DOT_PRODUCT,
            ID_SRC_CURRENT,
            ID_SRC_TEXTURE,
            ID_SRC_DIFFUSE,
            ID_SRC_SPECULAR,
            ID_SRC_MANUAL,
        ID_COLOUR_OP_MULTIPASS_FALLBACK,
        ID_ALPHA_OP_EX,
        ID_ENV_MAP,
            ID_SPHERICAL,
            ID_PLANAR,
            ID_CUBIC_REFLECTION,
            ID_CUBIC_NORMAL,
        ID_SCROLL,
        ID_SCROLL_ANIM,
        ID_ROTATE,
        ID_ROTATE_ANIM,
        ID_SCALE,
        ID_WAVE_XFORM,
            ID_SCROLL_X,
            ID_SCROLL_Y,
            ID_SCALE_X,
            ID_SCALE_Y,
            ID_SINE,
            ID_TRIANGLE,
            ID_SQUARE,
            ID_SAWTOOTH,
            ID_INVERSE_SAWTOOTH,
        ID_TRANSFORM,
        ID_BINDING_TYPE,
            ID_VERTEX,
            ID_FRAGMENT,
        ID_CONTENT_TYPE,
            ID_NAMED,
            ID_SHADOW,
        ID_TEXTURE_SOURCE,
        ID_SHARED_PARAMS,
        ID_SHARED_PARAM_NAMED,
        ID_SHARED_PARAMS_REF,

        ID_PARTICLE_SYSTEM,
        ID_EMITTER,
        ID_AFFECTOR,

        ID_COMPOSITOR,
            ID_TARGET,
            ID_TARGET_OUTPUT,

            ID_INPUT,
                ID_PREVIOUS,
                ID_TARGET_WIDTH,
                ID_TARGET_HEIGHT,
                ID_TARGET_WIDTH_SCALED,
                ID_TARGET_HEIGHT_SCALED,
            ID_COMPOSITOR_LOGIC,
            ID_TEXTURE_REF,
            ID_SCOPE_LOCAL,
            ID_SCOPE_CHAIN,
            ID_SCOPE_GLOBAL,
            ID_POOLED,
            //ID_GAMMA, - already registered for material
            ID_NO_FSAA,
            ID_DEPTH_POOL,
            ID_ONLY_INITIAL,
            ID_VISIBILITY_MASK,
            ID_LOD_BIAS,
            ID_MATERIAL_SCHEME,
            ID_SHADOWS_ENABLED,

            ID_CLEAR,
            ID_STENCIL,
            ID_RENDER_SCENE,
            ID_RENDER_QUAD,
            ID_IDENTIFIER,
            ID_FIRST_RENDER_QUEUE,
            ID_LAST_RENDER_QUEUE,
            ID_QUAD_NORMALS,
                ID_CAMERA_FAR_CORNERS_VIEW_SPACE,
                ID_CAMERA_FAR_CORNERS_WORLD_SPACE,

            ID_BUFFERS,
                ID_COLOUR,
                ID_DEPTH,
            ID_COLOUR_VALUE,
            ID_DEPTH_VALUE,
            ID_STENCIL_VALUE,

            ID_CHECK,
            ID_COMP_FUNC,
            ID_REF_VALUE,
            ID_MASK,
            ID_FAIL_OP,
                ID_KEEP,
                ID_INCREMENT,
                ID_DECREMENT,
                ID_INCREMENT_WRAP,
                ID_DECREMENT_WRAP,
                ID_INVERT,
            ID_DEPTH_FAIL_OP,
            ID_PASS_OP,
            ID_TWO_SIDED,
        // Suport for shader model 5.0
        // More program IDs
        ID_TESSELLATION_HULL_PROGRAM,
        ID_TESSELLATION_DOMAIN_PROGRAM,
        ID_COMPUTE_PROGRAM,
        ID_TESSELLATION_HULL_PROGRAM_REF,
        ID_TESSELLATION_DOMAIN_PROGRAM_REF,
        ID_COMPUTE_PROGRAM_REF,
        // More binding IDs
        ID_GEOMETRY,
        ID_TESSELLATION_HULL,
        ID_TESSELLATION_DOMAIN,
        ID_COMPUTE,

        // added during 1.11. re-sort for 1.12
        ID_LINE_WIDTH,
        ID_SAMPLER,
        ID_SAMPLER_REF,
        ID_THREAD_GROUPS,
        ID_RENDER_CUSTOM,
        ID_AUTO,
        ID_CAMERA,
        ID_ALIGN_TO_FACE,

        ID_END_BUILTIN_IDS
    };
    /** @} */
    /** @} */
}

#endif
