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
#ifndef OGRE_CORE_SHAREDPTR_H
#define OGRE_CORE_SHAREDPTR_H

#include "OgreMemoryAllocatorConfig.h"
#include "OgrePrerequisites.h"
#include <memory>

namespace Ogre {

    using std::static_pointer_cast;
    using std::dynamic_pointer_cast;

    /// @deprecated for backwards compatibility only, rather use shared_ptr directly
    template<class T> class SharedPtr : public ::std::shared_ptr<T>
    {
    public:
        SharedPtr(std::nullptr_t) {}
        SharedPtr() {}
        template< class Y>
        explicit SharedPtr(Y* ptr) : ::std::shared_ptr<T>(ptr) {}
        template< class Y, class Deleter >
        SharedPtr( Y* ptr, Deleter d ) : ::std::shared_ptr<T>(ptr, d) {}
        SharedPtr(const SharedPtr& r) : ::std::shared_ptr<T>(r) {}
        template<class Y>
        SharedPtr(const SharedPtr<Y>& r) : ::std::shared_ptr<T>(r) {}

        // implicit conversion from and to shared_ptr
        template<class Y>
        SharedPtr(const ::std::shared_ptr<Y>& r) : ::std::shared_ptr<T>(r) {}
        operator const ::std::shared_ptr<T>&() { return static_cast<::std::shared_ptr<T>&>(*this); }
        SharedPtr<T>& operator=(const Ogre::SharedPtr<T>& rhs) {::std::shared_ptr<T>::operator=(rhs); return *this;}
        // so swig recognizes it should forward the operators
        T* operator->() const { return ::std::shared_ptr<T>::operator->(); }
    };
    /** @} */
    /** @} */
}



#endif
