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
export module Ogre.RenderSystems.GLSupport:GLTextureCommon;

export import Ogre.Core;

export
namespace Ogre
{
class GLTextureCommon  : public Texture
{
public:
    GLTextureCommon(ResourceManager* creator, std::string_view name, ResourceHandle handle,
                    std::string_view group, bool isManual, ManualResourceLoader* loader)
        : Texture(creator, name, handle, group, isManual, loader) 
    {
    }

    auto getGLID() const noexcept -> uint { return mTextureID; }

    void getCustomAttribute(std::string_view name, void* pData) override;

protected:
    /** Returns the maximum number of Mipmaps that can be generated until we reach
        the mininum format possible. This does not count the base level.

        @return how many times we can divide this texture in 2 until we reach 1x1.
    */
    auto getMaxMipmaps() noexcept -> TextureMipmap;

    uint mTextureID{0};
};

} /* namespace Ogre */
