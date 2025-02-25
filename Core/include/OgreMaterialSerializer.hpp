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

#include <cstddef>

export module Ogre.Core:MaterialSerializer;

export import :BlendMode;
export import :Common;
export import :GpuProgramParams;
export import :MemoryAllocatorConfig;
export import :Prerequisites;
export import :TextureUnitState;

export import <algorithm>;
export import <format>;
export import <map>;
export import <set>;
export import <string>;
export import <vector>;

export
namespace Ogre {
struct ColourValue;
class Material;
class Pass;
class Technique;

    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup Materials
    *  @{
    */
    /** Class for serializing Materials to a .material script.*/
    class MaterialSerializer : public SerializerAlloc
    {   
    public:

        // Material serialize event.
        enum class SerializeEvent
        {
            PRE_WRITE,
            WRITE_BEGIN,
            WRITE_END,
            POST_WRITE
        };

        /** Class that allows listening in on the various stages of material serialization process.
        Sub-classing it enable extending the attribute set of any part in the material.
        */
        class Listener
        {
        public:
            virtual ~Listener() = default;
            
            /** Called when material section event raised.                  
            @param ser The MaterialSerializer instance that writes the given material.
            @param event The current section writing stage.
            @param skip May set to true by sub-class instances in order to skip the following section write.
            This parameter relevant only when stage equals SerializeEvent::PRE_WRITE.
            @param mat The material that is being written.          
            */
            virtual void materialEventRaised(MaterialSerializer* ser, 
                SerializeEvent event, bool& skip, const Material* mat)
                        { (void)ser; (void)event; (void)skip; (void)mat; }
            
            /** Called when technique section event raised.             
            @param ser The MaterialSerializer instance that writes the given material.
            @param event The current section writing stage.
            @param skip May set to true by sub-class instances in order to skip the following section write.
            This parameter relevant only when stage equals SerializeEvent::PRE_WRITE.
            @param tech The technique that is being written.        
            */
            virtual void techniqueEventRaised(MaterialSerializer* ser, 
                SerializeEvent event, bool& skip, const Technique* tech)
                        { (void)ser; (void)event; (void)skip; (void)tech; }
        
            /** Called when pass section event raised.                  
            @param ser The MaterialSerializer instance that writes the given material.
            @param event The current section writing stage.
            @param skip May set to true by sub-class instances in order to skip the following section write.
            This parameter relevant only when stage equals SerializeEvent::PRE_WRITE.
            @param pass The pass that is being written.     
            */
            virtual void passEventRaised(MaterialSerializer* ser, 
                SerializeEvent event, bool& skip, const Pass* pass)
                        { (void)ser; (void)event; (void)skip; (void)pass; }

            /** Called when GPU program reference section event raised.             
            @param ser The MaterialSerializer instance that writes the given material.
            @param event The current section writing stage.
            @param skip May set to true by sub-class instances in order to skip the following section write.
            This parameter relevant only when stage equals SerializeEvent::PRE_WRITE.
            @param attrib The GPU program reference description (vertex_program_ref, fragment_program_ref, etc).        
            @param program The program being written.
            @param params The program parameters.
            @param defaultParams The default program parameters.
            */
            void gpuProgramRefEventRaised(MaterialSerializer* ser, 
                SerializeEvent event, bool& skip,
                std::string_view attrib, 
                const GpuProgramPtr& program, 
                const GpuProgramParametersSharedPtr& params,
                GpuProgramParameters* defaultParams)
                        {
                            (void)ser;
                            (void)event;
                            (void)skip;
                            (void)attrib;
                            (void)program;
                            (void)params;
                            (void)defaultParams;
                        }

            /** Called when texture unit state section event raised.                    
            @param ser The MaterialSerializer instance that writes the given material.
            @param event The current section writing stage.
            @param skip May set to true by sub-class instances in order to skip the following section write.
            This parameter relevant only when stage equals SerializeEvent::PRE_WRITE.
            @param textureUnit The texture unit state that is being written.        
            */
            virtual void textureUnitStateEventRaised(MaterialSerializer* ser, 
                SerializeEvent event, bool& skip, const TextureUnitState* textureUnit)
                        {
                            (void)ser;
                            (void)event;
                            (void)skip;
                            (void)textureUnit;
                        }           
        };

    private:
        /** Internal method for saving a program definition which has been
            built up.
        */
        void finishProgramDefinition();

        /// Listeners list of this Serializer.
        using ListenerList = std::vector<Listener *>;
        using ListenerListIterator = ListenerList::iterator;
        using ListenerListConstIterator = ListenerList::const_iterator;
        ListenerList mListeners;


        void writeMaterial(const MaterialPtr& pMat, std::string_view materialName = "");
        void writeTechnique(const Technique* pTech);
        void writePass(const Pass* pPass);
        void writeVertexProgramRef(const Pass* pPass);
        void writeTesselationHullProgramRef(const Pass* pPass);
        void writeTesselationDomainProgramRef(const Pass* pPass);
        void writeGeometryProgramRef(const Pass* pPass);
        void writeFragmentProgramRef(const Pass* pPass);
        void writeGpuProgramRef(std::string_view attrib, const GpuProgramPtr& program, const GpuProgramParametersSharedPtr& params);
        void writeGpuPrograms();
        void writeGPUProgramParameters(const GpuProgramParametersSharedPtr& params, GpuProgramParameters* defaultParams,
                                       const unsigned short level = 4, const bool useMainBuffer = true);
        void writeNamedGpuProgramParameters(const GpuProgramParametersSharedPtr& params, GpuProgramParameters* defaultParams,
                                            const unsigned short level = 4, const bool useMainBuffer = true);
        void writeLowLevelGpuProgramParameters(const GpuProgramParametersSharedPtr& params, GpuProgramParameters* defaultParams,
                                               const unsigned short level = 4, const bool useMainBuffer = true);
        void writeGpuProgramParameter(
            std::string_view commandName, std::string_view identifier, 
            const GpuProgramParameters::AutoConstantEntry* autoEntry, 
            const GpuProgramParameters::AutoConstantEntry* defaultAutoEntry, 
            bool isFloat, bool isDouble, bool isInt, bool isUnsignedInt, 
            size_t physicalIndex, size_t physicalSize,
            const GpuProgramParametersSharedPtr& params, GpuProgramParameters* defaultParams,
            const unsigned short level, const bool useMainBuffer);
        void writeTextureUnit(const TextureUnitState *pTex);
        void writeSceneBlendFactor(const SceneBlendFactor c_src, const SceneBlendFactor c_dest, 
                                   const SceneBlendFactor a_src, const SceneBlendFactor a_dest);
        void writeSceneBlendFactor(const SceneBlendFactor sbf_src, const SceneBlendFactor sbf_dest);
        void writeSceneBlendFactor(const SceneBlendFactor sbf);
        void writeCompareFunction(const CompareFunction cf);
        void writeColourValue(const ColourValue &colour, bool writeAlpha = false);
        void writeLayerBlendOperationEx(const LayerBlendOperationEx op);
        void writeLayerBlendSource(const LayerBlendSource lbs);
        
        using EffectMap = std::multimap<TextureUnitState::TextureEffectType, TextureUnitState::TextureEffect>;

        void writeRotationEffect(const TextureUnitState::TextureEffect& effect, const TextureUnitState *pTex);
        void writeTransformEffect(const TextureUnitState::TextureEffect& effect, const TextureUnitState *pTex);
        void writeScrollEffect(const TextureUnitState::TextureEffect& effect, const TextureUnitState *pTex);
        void writeEnvironmentMapEffect(const TextureUnitState::TextureEffect& effect, const TextureUnitState *pTex);

        auto convertFiltering(FilterOptions fo) -> String;

        
        /** Internal methods that invokes registered listeners callback.
        @see Listener::materialEventRaised.
        */
        void fireMaterialEvent(SerializeEvent event, bool& skip, const Material* mat);

        /** Internal methods that invokes registered listeners callback.
        @see Listener::techniqueEventRaised.
        */
        void fireTechniqueEvent(SerializeEvent event, bool& skip, const Technique* tech);
        
        /** Internal methods that invokes registered listeners callback.
        @see Listener::passEventRaised.
        */
        void firePassEvent(SerializeEvent event, bool& skip, const Pass* pass);
        
        /** Internal methods that invokes registered listeners callback.
        @see Listener::gpuProgramRefEventRaised.
        */
        void fireGpuProgramRefEvent(SerializeEvent event, bool& skip,
            std::string_view attrib, 
            const GpuProgramPtr& program, 
            const GpuProgramParametersSharedPtr& params,
            GpuProgramParameters* defaultParams);
    

        /** Internal methods that invokes registered listeners callback.
        @see Listener::textureUnitStateEventRaised.
        */
        void fireTextureUnitStateEvent(SerializeEvent event, bool& skip, const TextureUnitState* textureUnit);
        
   public:      
        /** default constructor*/
        MaterialSerializer();
        /** default destructor*/
        virtual ~MaterialSerializer() = default;

        /** Queue an in-memory Material to the internal buffer for export.
        @param pMat Material pointer
        @param clearQueued If true, any materials already queued will be removed
        @param exportDefaults If true, attributes which are defaulted will be
            included in the script exported, otherwise they will be omitted
        @param materialName Allow exporting the given material under a different name.
            In case of empty string the original material name will be used.
        */
        void queueForExport(const MaterialPtr& pMat, bool clearQueued = false, 
            bool exportDefaults = false, std::string_view materialName = "");
        /** Exports queued material(s) to a named material script file.
        @param filename the file name of the material script to be exported
        @param includeProgDef If true, vertex program and fragment program 
            definitions will be written at the top of the material script
        @param programFilename the file name of the vertex / fragment program 
            script to be exported. This is only used if there are program definitions
            to be exported and includeProgDef is false 
            when calling queueForExport.
        */
        void exportQueued(std::string_view filename, const bool includeProgDef = false, std::string_view programFilename = "");
        /** Exports a single in-memory Material to the named material script file.
        @param pMat Material pointer
        @param filename the file name of the material script to be exported
        @param exportDefaults if true then exports all values including defaults
        @param includeProgDef if true includes Gpu shader program definitions in the
            export material script otherwise if false then program definitions will
            be exported to a separate file with name programFilename if
            programFilename is not empty
        @param programFilename the file name of the vertex / fragment program 
            script to be exported. This is only used if includeProgDef is false.
        @param materialName Allow exporting the given material under a different name.
            In case of empty string the original material name will be used.
        */
        void exportMaterial(const MaterialPtr& pMat, std::string_view filename, bool exportDefaults = false,
            const bool includeProgDef = false, std::string_view programFilename = "", 
            std::string_view materialName = "");
        /** Returns a string representing the parsed material(s) */
        [[nodiscard]] auto getQueuedAsString() const -> std::string_view ;
        /** Clears the internal buffer */
        void clearQueue();

        /** Register a listener to this Serializer.
        @see MaterialSerializer::Listener
        */
        void addListener(Listener* listener);

        /** Remove a listener from this Serializer.
        @see MaterialSerializer::Listener
        */
        void removeListener(Listener* listener);

    private:
        String mBuffer;
        String mGpuProgramBuffer;
        using GpuProgramDefinitionContainer = std::set<std::string_view>;
        using GpuProgramDefIterator = GpuProgramDefinitionContainer::iterator;
        GpuProgramDefinitionContainer mGpuProgramDefinitionContainer;
        bool mDefaults;
        
    public:
        void beginSection(unsigned short level, const bool useMainBuffer = true)
        {
            String& buffer = (useMainBuffer ? mBuffer : mGpuProgramBuffer);
            buffer += "\n";
            for (unsigned short i = 0; i < level; ++i)
            {
                buffer += "\t";
            }
            buffer += "{";
        }
        void endSection(unsigned short level, const bool useMainBuffer = true)
        {
            String& buffer = (useMainBuffer ? mBuffer : mGpuProgramBuffer);
            buffer += "\n";
            for (unsigned short i = 0; i < level; ++i)
            {
                buffer += "\t";
            }
            buffer += "}";
        }

        void writeAttribute(unsigned short level, std::string_view att, const bool useMainBuffer = true)
        {
            String& buffer = (useMainBuffer ? mBuffer : mGpuProgramBuffer);
            buffer += "\n";
            for (unsigned short i = 0; i < level; ++i)
            {
                buffer += "\t";
            }
            buffer += att;
        }

        void writeValue(std::string_view val, const bool useMainBuffer = true)
        {
            String& buffer = (useMainBuffer ? mBuffer : mGpuProgramBuffer);
            buffer += (::std::format(" {}", val));
        }

        auto quoteWord(std::string_view val) -> String
        {
            if (val.find_first_of("{}$: \t") != String::npos)
                return (::std::format("\"{}\"", val ));
            else return std::string{val};
        }


        void writeComment(unsigned short level, std::string_view comment, const bool useMainBuffer = true)
        {
            String& buffer = (useMainBuffer ? mBuffer : mGpuProgramBuffer);
            buffer += "\n";
            for (unsigned short i = 0; i < level; ++i)
            {
                buffer += "\t";
            }
            buffer += ::std::format("// {}", comment);
        }



    };
    /** @} */
    /** @} */
}
