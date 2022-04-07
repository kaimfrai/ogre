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
export module Ogre.Components.Overlay:ElementFactory;

export import :BorderPanelOverlayElement;
export import :Element;
export import :PanelOverlayElement;
export import :TextAreaOverlayElement;

export
namespace Ogre {

    /** \addtogroup Optional
    *  @{
    */
    /** \addtogroup Overlays
    *  @{
    */
    /** Defines the interface which all components wishing to 
        supply OverlayElement subclasses must implement.
    @remarks
        To allow the OverlayElement types available for inclusion on 
        overlays to be extended, OGRE allows external apps or plugins
        to register their ability to create custom OverlayElements with
        the OverlayManager, using the addOverlayElementFactory method. Classes
        wanting to do this must implement this interface.
    @par
        Each OverlayElementFactory creates a single type of OverlayElement, 
        identified by a 'type name' which must be unique.
    */
    class OverlayElementFactory : public OverlayAlloc
    {
    public:
        /** Destroy the overlay element factory */
        virtual ~OverlayElementFactory () = default;
        /** Creates a new OverlayElement instance with the name supplied. */
        [[nodiscard]]
        virtual auto createOverlayElement(const String& instanceName) -> OverlayElement* = 0;
        /** Destroys a OverlayElement which this factory created previously. */
        virtual void destroyOverlayElement(OverlayElement* pElement) { delete pElement; }
        /** Gets the string uniquely identifying the type of element this factory creates. */
        [[nodiscard]]
        virtual auto getTypeName() const -> const String& = 0;
    };
    /** @} */
    /** @} */

}
