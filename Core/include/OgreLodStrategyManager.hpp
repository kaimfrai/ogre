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
export module Ogre.Core:LodStrategyManager;

export import :MemoryAllocatorConfig;
export import :Prerequisites;
export import :Singleton;

export import <map>;
export import <string>;

export
namespace Ogre {
    template <typename T> class MapIterator;
class LodStrategy;

    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup LOD
    *  @{
    */
    /** Manager for LOD strategies. */
    class LodStrategyManager : public Singleton<LodStrategyManager>, public LodAlloc
    {
        /** Map of strategies. */
        using StrategyMap = std::map<std::string_view, LodStrategy *>;

        /** Internal map of strategies. */
        StrategyMap mStrategies;

        /** Default strategy. */
        LodStrategy *mDefaultStrategy;

    public:
        /** Default constructor. */
        LodStrategyManager();

        /** Destructor. */
        ~LodStrategyManager();

        /** Add a strategy to the manager. */
        void addStrategy(LodStrategy *strategy);

        /** Remove a strategy from the manager with a specified name.
        @remarks
            The removed strategy is returned so the user can control
            how it is destroyed.
        */
        auto removeStrategy(std::string_view name) -> LodStrategy *;

        /** Remove and delete all strategies from the manager.
        @remarks
            All strategies are deleted.  If finer control is required
            over strategy destruction, use removeStrategy.
        */
        void removeAllStrategies();

        /** Get the strategy with the specified name. */
        auto getStrategy(std::string_view name) -> LodStrategy *;

        /** Set the default strategy. */
        void setDefaultStrategy(LodStrategy *strategy);

        /** Set the default strategy by name. */
        void setDefaultStrategy(std::string_view name);

        /** Get the current default strategy. */
        auto getDefaultStrategy() -> LodStrategy *;

        /** Get an iterator for all contained strategies. */
        auto getIterator() -> MapIterator<StrategyMap>;

        /// @copydoc Singleton::getSingleton()
        static auto getSingleton() noexcept -> LodStrategyManager&;
        /// @copydoc Singleton::getSingleton()
        static auto getSingletonPtr() noexcept -> LodStrategyManager*;
    };
    /** @} */
    /** @} */
}
