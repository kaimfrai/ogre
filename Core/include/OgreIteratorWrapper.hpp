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
export module Ogre.Core:IteratorWrapper;

export
namespace Ogre{

/** 
 * 
 * @brief Basefunctionality for IteratorWrappers
 *
 * 
 * @tparam T a Container like vector list map ...
 * @tparam IteratorType  T::iterator or T::const_iterator
 * @tparam ValType  T::mapped_type in case of a map, T::value_type for vector, list,...
 * 
 * Have a look at VectorIteratorWrapper and MapIteratorWrapper for a concrete usage
*/
template <typename T, typename IteratorType, typename ValType>
class IteratorWrapper
{

    public:
        /// Private constructor since only the parameterised constructor should be used
        IteratorWrapper() = delete;

    protected:
        IteratorType mBegin;
        IteratorType mCurrent;
        IteratorType mEnd;
    

    public:
    
        /// Type you expect to get by funktions like peekNext(Value)
        using ValueType = ValType;
        /// Type you expect to get by funktions like peekNext(Value)Ptr
        using PointerType = ValType *;

        /**
        @brief Typedef to fulfill container interface
        
        Useful if you want to use BOOST_FOREACH
        @note there is no distinction between const_iterator and iterator.
        \n keep this in mind if you want to derivate from this class. 
        */
        using iterator = IteratorType;
        
        /**
        @brief Typedef to fulfill container interface
        
        Useful if you want to use BOOST_FOREACH
        @note there is no distinction between const_iterator and iterator.
        \n keep this in mind if you want to derivate from this class. 
        */
        using const_iterator = IteratorType;

        
        /** Constructor.
        @remarks
        Provide a start and end iterator to initialise.
        */
        IteratorWrapper ( IteratorType start, IteratorType last )
        : mBegin( start ), mCurrent ( start ), mEnd ( last )
        {
        }


        /** Returns true if there are more items in the collection. */
        [[nodiscard]] auto hasMoreElements ( ) const -> bool
        {
            return mCurrent != mEnd;
        }


        /** Moves the iterator on one element. */
        void moveNext (  )
        {
            ++mCurrent;
        }

        /** Bookmark to the begin of the underlying collection */
        auto begin() -> const IteratorType& {return mBegin;}
        
        
        /** Full access to the current  iterator */
        auto current() -> IteratorType&{return mCurrent;}
        
        /** Bookmark to the end (one behind the last element) of the underlying collection */
        auto end() -> const IteratorType& {return mEnd;}
        

};



/** 
 * 
 * @brief Prepared IteratorWrapper for container like std::vector 
 *
 * 
 * @tparam T = Container eg vector
 * @tparam IteratorType  T::iterator or T::const_iterator
 * 
 * Have a look at VectorIterator and ConstVectorIterator for a more concrete usage
*/
template <typename T, typename IteratorType>
class VectorIteratorWrapper : public IteratorWrapper<T, IteratorType, typename  T::value_type>
{

    public:
        using ValueType = typename IteratorWrapper<T, IteratorType, typename T::value_type>::ValueType ; 
        using PointerType = typename IteratorWrapper<T, IteratorType, typename T::value_type>::PointerType ;
    

        /**
         * @brief c'tor
         * 
         * Constructor that provide a start and end iterator to initialise.
         * 
         * @param start Start iterator 
         * @param last End iterator 
         */
        VectorIteratorWrapper ( IteratorType start, IteratorType last )
        : IteratorWrapper<T, IteratorType, typename T::value_type>( start, last ) 
        {
        }


        /** Returns the next(=current) element in the collection, without advancing to the next. */
        [[nodiscard]] auto peekNext (  ) const -> ValueType
        {
            return *(this->mCurrent);
        }

        /** Returns a pointer to the next(=current) element in the collection, without advancing to the next afterwards. */
        [[nodiscard]] auto peekNextPtr (  )  const -> PointerType
        {
            return &(*(this->mCurrent) );
        }

        /** Returns the next(=current) value element in the collection, and advances to the next. */
        auto getNext (  ) -> ValueType 
        {
            return *(this->mCurrent++);
        }   

};


/** 
 * 
 * @brief Concrete IteratorWrapper for nonconst access to the underlying container
 * 
 * @tparam T  Container
 * 
*/
template <typename T>
class VectorIterator : public VectorIteratorWrapper<T,  typename T::iterator>{
    public:
        /** Constructor.
        @remarks
            Provide a start and end iterator to initialise.
        */  
        VectorIterator( typename T::iterator start, typename T::iterator last )
        : VectorIteratorWrapper<T,  typename T::iterator>(start , last )
        {
        }

        /** Constructor.
        @remarks
            Provide a container to initialise.
        */
        explicit VectorIterator( T& c )
        : VectorIteratorWrapper<T,  typename T::iterator> ( c.begin(), c.end() )
        {
        }
        
};

/** 
 * 
 * @brief Concrete IteratorWrapper for const access to the underlying container
 *
 * 
 * @tparam T = Container
 * 
*/
template <typename T>
class ConstVectorIterator : public VectorIteratorWrapper<T,  typename T::const_iterator>{
    public:
        /** Constructor.
        @remarks
            Provide a start and end iterator to initialise.
        */  
        ConstVectorIterator( typename T::const_iterator start, typename T::const_iterator last )
        : VectorIteratorWrapper<T,  typename T::const_iterator> (start , last )
        {
        }

        /** Constructor.
        @remarks
            Provide a container to initialise.
        */
        explicit ConstVectorIterator ( const T& c )
         : VectorIteratorWrapper<T,  typename T::const_iterator> (c.begin() , c.end() )
        {
        }
};





/** 
 * 
 * @brief Prepared IteratorWrapper for key-value container
 *
 * 
 * @tparam T  Container  (map - or also set )
 * @tparam  IteratorType T::iterator or T::const_iterator
 * 
 * Have a look at MapIterator and ConstMapIterator for a concrete usage
*/
template <typename T, typename IteratorType>
class MapIteratorWrapper  : public IteratorWrapper<T, IteratorType, typename T::mapped_type>
{

    public:
        /// Redefined ValueType for a map/set
        using ValueType = typename IteratorWrapper<T, IteratorType, typename T::mapped_type>::ValueType ; 
        /// Redefined PointerType for a map/set
        using PointerType = typename IteratorWrapper<T, IteratorType, typename T::mapped_type>::PointerType ;  
        
        /// Unused, just to make it clear that map/set\::value_type is not a ValueType
        using PairType = typename T::value_type ; 
        /// Type you expect to get by funktions like peekNextKey
        using KeyType = typename T::key_type;
        
        /** Constructor.
        @remarks
            Provide a start and end iterator to initialise.
        */
        MapIteratorWrapper ( IteratorType start, IteratorType last )
        : IteratorWrapper<T, IteratorType, typename T::mapped_type>( start, last ) 
        {
        }

        /** Returns the next(=current) key element in the collection, without advancing to the next. */
        [[nodiscard]] auto peekNextKey() const -> KeyType
        {
            return this->mCurrent->first;
        }


        /** Returns the next(=current) value element in the collection, without advancing to the next. */
        [[nodiscard]] auto peekNextValue (  ) const -> ValueType
        {
            return this->mCurrent->second;
        }


        /** Returns a pointer to the next/current value element in the collection, without 
        advancing to the next afterwards. */
        [[nodiscard]] auto peekNextValuePtr (  )  const -> const PointerType
        {
            return &(this->mCurrent->second);
        }


        /** Returns the next(=current) value element in the collection, and advances to the next. */
        auto getNext() -> ValueType
        {
            return ((this->mCurrent++)->second) ;
        }   
    

};




/** 
 * 
 * @brief Concrete IteratorWrapper for nonconst access to the underlying key-value container
 *
 * 
 * @remarks Template key-value container
 * 
*/
template <typename T>
class MapIterator : public MapIteratorWrapper<T,  typename T::iterator>{
    public:
    
        /** Constructor.
        @remarks
            Provide a start and end iterator to initialise.
        */  
        MapIterator( typename T::iterator start, typename T::iterator last )
        : MapIteratorWrapper<T,  typename T::iterator>(start , last )
        {
        }
        
        /** Constructor.
        @remarks
            Provide a container to initialise.
        */
        explicit MapIterator( T& c )
        : MapIteratorWrapper<T,  typename T::iterator> ( c.begin(), c.end() )
        {
        }
        
};


/** 
 * 
 * @brief Concrete IteratorWrapper for const access to the underlying key-value container
 *
 * 
 * @tparam T key-value container
 * 
*/
template <typename T>
class ConstMapIterator : public MapIteratorWrapper<T,  typename T::const_iterator>{
    public:
    
        /** Constructor.
        @remarks
            Provide a start and end iterator to initialise.
        */  
        ConstMapIterator( typename T::const_iterator start, typename T::const_iterator last )
        : MapIteratorWrapper<T,  typename T::const_iterator> (start , last )
        {
        }

        /** Constructor.
        @remarks
            Provide a container to initialise.
        */
        explicit ConstMapIterator ( const T& c )
         : MapIteratorWrapper<T,  typename T::const_iterator> (c.begin() , c.end() )
        {
        }
};




}
