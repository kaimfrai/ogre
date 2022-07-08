#include <utility>

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

#ifndef OGRE_TESTS_VISUALTESTS_TINYHTML_H
#define OGRE_TESTS_VISUALTESTS_TINYHTML_H

/** The following is a very simple representation of the HTML DOM.
 *    It's sole purpose is outputting formatted html documents, there
 *    are no provisions for traversal, parsing, etc. */

/** An abstract html node */
struct HtmlNode : public Ogre::GeneralAllocatedObject
{
    virtual ~HtmlNode() = default;
    virtual auto print(Ogre::String indent = "") -> Ogre::String = 0;
};
//-----------------------------------------------------------------------

/** A plain text node (cannot have children, text only). */
struct HtmlTextNode : public HtmlNode
{
    Ogre::String contents;

    HtmlTextNode(StringView text):contents(text){}
    //-------------------------------------------------------------------

    auto print(Ogre::String indent = "") -> Ogre::String override
    {
        return contents;
    }
};
//-----------------------------------------------------------------------

/** An HTML element, can contain children (either text or other elements), as well
     as storing a list of attributes */
struct HtmlElement : public HtmlNode
{
    Ogre::String tagname;
    std::list<std::pair<Ogre::String, Ogre::String> > attributes;
    std::list<HtmlNode*> children;

    HtmlElement(Ogre::String tag)
    {
        tagname = tag;
    }
    //-------------------------------------------------------------------

    ~HtmlElement() override
    {
        while (!children.empty())
        {
            delete children.back();
            children.pop_back();
        }
    }
    //-------------------------------------------------------------------

    void appendAttribute(Ogre::String name, Ogre::String value)
    {
        attributes.emplace_back(name,value);
    }
    //-------------------------------------------------------------------

    void pushChild(HtmlNode* node)
    {
        children.push_back(node);
    }
    //-------------------------------------------------------------------

    auto appendElement(Ogre::String type) -> HtmlElement*
    {
        auto* newNode = new HtmlElement(type);
        children.push_back(newNode);
        return newNode;
    }
    //-------------------------------------------------------------------

    auto appendText(Ogre::String text) -> HtmlTextNode*
    {
        auto* newNode = new HtmlTextNode(text);
        children.push_back(newNode);
        return newNode;
    }
    //-------------------------------------------------------------------

    auto print(Ogre::String indent = "") -> Ogre::String override
    {
        // stream we'll output to
        Ogre::StringStream out;

        out<<"\n"<<indent;

        // print opening tag
        // open bracket and name
        out<<"<"<<tagname;

        // attributes
        for (auto & attribute : attributes)
        {
            // name="value"
            out<<" "<<attribute.first<<"=\""<<attribute.second<<"\"";
        }

        // self-closing is done here
        if (children.empty())
        {
            out<<"/>";
            return out.str();
        }
        // closing bracket
        out<<">";

        // print children
        for (auto & it : children)
        {
            out<<it->print(::std::format("{}\t", indent));
        }

        // if the last child was an actual element start a newline (otherwise, if text, we'll close on the same line)
        if (dynamic_cast<HtmlElement*>(children.back()))
            out<<"\n"<<indent;

        // print closing tag
        out<<"</"<<tagname<<">";
        return out.str();
    }
};

#endif
