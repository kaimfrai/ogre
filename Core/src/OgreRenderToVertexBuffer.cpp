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

#include <memory>
#include <string>

#include "OgreException.hpp"
#include "OgreMaterial.hpp"
#include "OgreMaterialManager.hpp"
#include "OgreRenderToVertexBuffer.hpp"
#include "OgreVertexIndexData.hpp"

namespace Ogre {
class VertexDeclaration;

    //-----------------------------------------------------------------------
    RenderToVertexBuffer::RenderToVertexBuffer() 
        
    {
        mVertexData = std::make_unique<VertexData>();
    }
    RenderToVertexBuffer::~RenderToVertexBuffer() = default; // ensure unique_ptr destructors are in cpp
    //-----------------------------------------------------------------------
    VertexDeclaration* RenderToVertexBuffer::getVertexDeclaration() noexcept
    {
        //TODO : Mark dirty?
        return mVertexData->vertexDeclaration;
    }
    //-----------------------------------------------------------------------
    void RenderToVertexBuffer::setRenderToBufferMaterialName(const String& materialName)
    {
        mMaterial = MaterialManager::getSingleton().getByName(materialName);

        if (!mMaterial)
            OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND, ::std::format("Could not find material {}", materialName),
                         "RenderToVertexBuffer::setRenderToBufferMaterialName" );

        /* Ensure that the new material was loaded (will not load again if
           already loaded anyway)
        */
        mMaterial->load();
    }
}
