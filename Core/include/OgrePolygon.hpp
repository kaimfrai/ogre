/*
-----------------------------------------------------------------------------
This source file is part of OGRE
(Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2014 Torus Knot Software Ltd
Copyright (c) 2006 Matthias Fink, netAllied GmbH <matthias.fink@web.de>                             

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

export module Ogre.Core:Polygon;

export import :Prerequisites;
export import :Vector;

export import <iosfwd>;
export import <map>;
export import <utility>;
export import <vector>;

export
namespace Ogre
{


    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup Math
    *  @{
    */
    /** The class represents a polygon in 3D space.
    @remarks
        It is made up of 3 or more vertices in a single plane, listed in 
        counter-clockwise order.
    */
    class Polygon
    {

    public:
        using VertexList = std::vector<Vector3>;

        using EdgeMap = std::multimap<Vector3, Vector3>;
        using Edge = std::pair<Vector3, Vector3>;

    private:
        VertexList      mVertexList;
        mutable Vector3 mNormal;
        mutable bool    mIsNormalSet{false};
        /** Updates the normal.
        */
        void updateNormal() const;


    public:
        Polygon();
        ~Polygon();
        Polygon( const Polygon& cpy );

        /** Inserts a vertex at a specific position.
        @note Vertices must be coplanar.
        */
        void insertVertex(const Vector3& vdata, size_t vertexIndex);
        /** Inserts a vertex at the end of the polygon.
        @note Vertices must be coplanar.
        */
        void insertVertex(const Vector3& vdata);

        /** Returns a vertex.
        */
        auto getVertex(size_t vertex) const -> const Vector3&;

        /** Sets a specific vertex of a polygon.
        @note Vertices must be coplanar.
        */
        void setVertex(const Vector3& vdata, size_t vertexIndex);

        /** Removes duplicate vertices from a polygon.
        */
        void removeDuplicates();

        /** Vertex count.
        */
        auto getVertexCount() const -> size_t;

        /** Returns the polygon normal.
        */
        auto getNormal() const noexcept -> const Vector3&;

        /** Deletes a specific vertex.
        */
        void deleteVertex(size_t vertex);

        /** Determines if a point is inside the polygon.
        @remarks
            A point is inside a polygon if it is both on the polygon's plane, 
            and within the polygon's bounds. Polygons are assumed to be convex
            and planar.
        */
        auto isPointInside(const Vector3& point) const -> bool;

        /** Stores the edges of the polygon in ccw order.
            The vertices are copied so the user has to take the 
            deletion into account.
        */
        void storeEdges(EdgeMap *edgeMap) const;

        /** Resets the object.
        */
        void reset();

        /** Determines if the current object is equal to the compared one.
        */
        [[nodiscard]] auto operator == (const Polygon& rhs) const noexcept -> bool;

        auto operator=(const Ogre::Polygon&) -> Polygon& ;

        /** Prints out the polygon data.
        */
        friend auto operator<< ( std::ostream& strm, const Polygon& poly ) -> std::ostream&;

    };
    /** @} */
    /** @} */

}
