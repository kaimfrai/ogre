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
/* Original version Copyright (C) Scott Bilas, 2000.
 * All rights reserved worldwide.
 *
 * This software is provided "as is" without express or implied
 * warranties. You may freely copy and compile this source into
 * applications you distribute provided that the copyright text
 * below is included in the resulting source code, for example:
 * "Portions Copyright (C) Scott Bilas, 2000"
 */
module;

#include <cassert>

export module Ogre.Core:Singleton;

// Added by Steve Streeting for Ogre
export import :Exception;
export import :Prerequisites;

export
namespace Ogre {
    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup General
    *  @{
    */

// End SJS additions
/** Template class for creating single-instance global classes.
 *
 * This implementation @cite bilas2000automatic slightly derives from the textbook pattern, by requiring
 * manual instantiation, instead of implicitly doing it in #getSingleton. This is useful for classes that
 * want to do some involved initialization, which should be done at a well defined time-point or need some
 * additional parmeters in their constructor.
 *
 * It also allows you to manage the singleton lifetime through RAII.
 *
 * @note Be aware that #getSingleton will fail before the global instance is created. (check via
 * #getSingletonPtr)
 */
template <typename T> class Singleton
{
public:
    /** @brief Explicit private copy constructor. This is a forbidden operation.*/
    Singleton(const Singleton<T>&) = delete;

    /** @brief Private operator= . This is a forbidden operation. */
    auto operator=(const Singleton<T>&) -> Singleton& = delete;

protected:
    static T* msSingleton;

public:
    Singleton()
    {
        OgreAssert(!msSingleton, "There can be only one singleton");
        msSingleton = static_cast<T*>(this);
    }
    ~Singleton()
    {
        assert(msSingleton);
        msSingleton = nullptr;
    }
    /// Get the singleton instance
    static auto getSingleton() noexcept -> T&
    {
        assert(msSingleton);
        return (*msSingleton);
    }
    /// @copydoc getSingleton
    static auto getSingletonPtr() noexcept -> T* { return msSingleton; }
    };
    /** @} */
    /** @} */

    template <typename T>
    T* Singleton<T>::msSingleton = nullptr;

}
