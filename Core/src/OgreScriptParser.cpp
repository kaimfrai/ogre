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
module Ogre.Core;

import :Exception;
import :Platform;
import :ScriptParser;
import :SharedPtr;
import :StringConverter;

import <format>;
import <list>;
import <string>;
import <vector>;

namespace Ogre
{
    static auto unquoted(std::string_view str, bool trim = true) -> String
    {
        return String{trim ? str.substr(1, str.size() - 2) : str};
    }

    auto ScriptParser::parse(const ScriptTokenList &tokens, std::string_view file) -> ConcreteNodeListPtr
    {
        // MEMCATEGORY_GENERAL because SharedPtr can only free using that category
        ConcreteNodeListPtr nodes(new ConcreteNodeList());

        enum{READY, OBJECT};
        uint32 state = READY;

        ConcreteNode *parent = nullptr;
        ConcreteNodePtr node;
        const ScriptToken *token = nullptr;
        auto i = tokens.begin(), end = tokens.end();
        while(i != end)
        {
            token = &*i;

            switch(state)
            {
            case READY:
                if(token->type == TID_WORD)
                {
                    if(token->lexeme == "import")
                    {
                        node = ConcreteNodePtr(new ConcreteNode());
                        node->token = token->lexeme;
                        node->file = file;
                        node->line = token->line;
                        node->type = ConcreteNodeType::IMPORT;

                        // The next token is the target
                        ++i;
                        if(i == end || (i->type != TID_WORD && i->type != TID_QUOTE))
                            OGRE_EXCEPT(ExceptionCodes::INVALID_STATE, 
                                std::format("expected import target at line {}", node->line),
                                "ScriptParser::parse");
                        ConcreteNodePtr temp(new ConcreteNode());
                        temp->parent = node.get();
                        temp->file = file;
                        temp->line = i->line;
                        temp->type = i->type == TID_WORD ? ConcreteNodeType::WORD : ConcreteNodeType::QUOTE;
                        temp->token = unquoted(i->lexeme, temp->type == ConcreteNodeType::QUOTE);
                        node->children.push_back(temp);

                        // The second-next token is the source
                        ++i;
                        ++i;
                        if(i == end || (i->type != TID_WORD && i->type != TID_QUOTE))
                            OGRE_EXCEPT(ExceptionCodes::INVALID_STATE, 
                                std::format("expected import source at line {}", node->line),
                                "ScriptParser::parse");
                        temp = ConcreteNodePtr(new ConcreteNode());
                        temp->parent = node.get();
                        temp->file = file;
                        temp->line = i->line;
                        temp->type = i->type == TID_WORD ? ConcreteNodeType::WORD : ConcreteNodeType::QUOTE;
                        temp->token = unquoted(i->lexeme, temp->type == ConcreteNodeType::QUOTE);
                        node->children.push_back(temp);

                        // Consume all the newlines
                        i = skipNewlines(i, end);

                        // Insert the node
                        if(parent)
                        {
                            node->parent = parent;
                            parent->children.push_back(node);
                        }
                        else
                        {
                            node->parent = nullptr;
                            nodes->push_back(node);
                        }
                        node = ConcreteNodePtr();
                    }
                    else if(token->lexeme == "set")
                    {
                        node = ConcreteNodePtr(new ConcreteNode());
                        node->token = token->lexeme;
                        node->file = file;
                        node->line = token->line;
                        node->type = ConcreteNodeType::VARIABLE_ASSIGN;

                        // The next token is the variable
                        ++i;
                        if(i == end || i->type != TID_VARIABLE)
                            OGRE_EXCEPT(ExceptionCodes::INVALID_STATE, 
                                std::format("expected variable name at line {}", node->line),
                                "ScriptParser::parse");
                        ConcreteNodePtr temp(new ConcreteNode());
                        temp->parent = node.get();
                        temp->file = file;
                        temp->line = i->line;
                        temp->type = ConcreteNodeType::VARIABLE;
                        temp->token = i->lexeme;
                        node->children.push_back(temp);

                        // The next token is the assignment
                        ++i;
                        if(i == end || (i->type != TID_WORD && i->type != TID_QUOTE))
                            OGRE_EXCEPT(ExceptionCodes::INVALID_STATE, 
                                std::format("expected variable value at line {}", node->line),
                                "ScriptParser::parse");
                        temp = ConcreteNodePtr(new ConcreteNode());
                        temp->parent = node.get();
                        temp->file = file;
                        temp->line = i->line;
                        temp->type = i->type == TID_WORD ? ConcreteNodeType::WORD : ConcreteNodeType::QUOTE;
                        temp->token = unquoted(i->lexeme, temp->type == ConcreteNodeType::QUOTE);
                        node->children.push_back(temp);

                        // Consume all the newlines
                        i = skipNewlines(i, end);

                        // Insert the node
                        if(parent)
                        {
                            node->parent = parent;
                            parent->children.push_back(node);
                        }
                        else
                        {
                            node->parent = nullptr;
                            nodes->push_back(node);
                        }
                        node = ConcreteNodePtr();
                    }
                    else
                    {
                        node = ConcreteNodePtr(new ConcreteNode());
                        node->file = file;
                        node->line = token->line;
                        node->type = token->type == TID_WORD ? ConcreteNodeType::WORD : ConcreteNodeType::QUOTE;
                        node->token = unquoted(token->lexeme, node->type == ConcreteNodeType::QUOTE);

                        // Insert the node
                        if(parent)
                        {
                            node->parent = parent;
                            parent->children.push_back(node);
                        }
                        else
                        {
                            node->parent = nullptr;
                            nodes->push_back(node);
                        }

                        // Set the parent
                        parent = node.get();

                        // Switch states
                        state = OBJECT;

                        node = ConcreteNodePtr();
                    }
                }
                else if(token->type == TID_RBRACKET)
                {
                    // Go up one level if we can
                    if(parent)
                        parent = parent->parent;

                    node = ConcreteNodePtr(new ConcreteNode());
                    node->token = token->lexeme;
                    node->file = file;
                    node->line = token->line;
                    node->type = ConcreteNodeType::RBRACE;

                    // Consume all the newlines
                    i = skipNewlines(i, end);

                    // Insert the node
                    if(parent)
                    {
                        node->parent = parent;
                        parent->children.push_back(node);
                    }
                    else
                    {
                        node->parent = nullptr;
                        nodes->push_back(node);
                    }

                    // Move up another level
                    if(parent)
                        parent = parent->parent;

                    node = ConcreteNodePtr();
                }
                break;
            case OBJECT:
                if(token->type == TID_NEWLINE)
                {
                    // Look ahead to the next non-newline token and if it isn't an {, this was a property
                    auto next = skipNewlines(i, end);
                    if(next == end || next->type != TID_LBRACKET)
                    {
                        // Ended a property here
                        if(parent)
                            parent = parent->parent;
                        state = READY;
                    }
                }
                else if(token->type == TID_COLON)
                {
                    node = ConcreteNodePtr(new ConcreteNode());
                    node->token = token->lexeme;
                    node->file = file;
                    node->line = token->line;
                    node->type = ConcreteNodeType::COLON;

                    // The following token are the parent objects (base classes).
                    // Require at least one of them.

                    auto j = i + 1;
                    j = skipNewlines(j, end);
                    if(j == end || (j->type != TID_WORD && j->type != TID_QUOTE)) {
                        OGRE_EXCEPT(ExceptionCodes::INVALID_STATE, 
                            std::format("expected object identifier at line {}", node->line),
                            "ScriptParser::parse");
                    }

                    while(j != end && (j->type == TID_WORD || j->type == TID_QUOTE))
                    {
                        ConcreteNodePtr tempNode = ConcreteNodePtr(new ConcreteNode());
                        tempNode->token = j->lexeme;
                        tempNode->file = file;
                        tempNode->line = j->line;
                        tempNode->type = j->type == TID_WORD ? ConcreteNodeType::WORD : ConcreteNodeType::QUOTE;
                        tempNode->parent = node.get();
                        node->children.push_back(tempNode);
                        ++j;
                    }

                    // Move it backwards once, since the end of the loop moves it forwards again anyway
                    i = --j;

                    // Insert the node
                    if(parent)
                    {
                        node->parent = parent;
                        parent->children.push_back(node);
                    }
                    else
                    {
                        node->parent = nullptr;
                        nodes->push_back(node);
                    }
                    node = ConcreteNodePtr();
                }
                else if(token->type == TID_LBRACKET)
                {
                    node = ConcreteNodePtr(new ConcreteNode());
                    node->token = token->lexeme;
                    node->file = file;
                    node->line = token->line;
                    node->type = ConcreteNodeType::LBRACE;

                    // Consume all the newlines
                    i = skipNewlines(i, end);

                    // Insert the node
                    if(parent)
                    {
                        node->parent = parent;
                        parent->children.push_back(node);
                    }
                    else
                    {
                        node->parent = nullptr;
                        nodes->push_back(node);
                    }

                    // Set the parent
                    parent = node.get();

                    // Change the state
                    state = READY;

                    node = ConcreteNodePtr();
                }
                else if(token->type == TID_RBRACKET)
                {
                    // Go up one level if we can
                    if(parent)
                        parent = parent->parent;

                    // If the parent is currently a { then go up again
                    if(parent && parent->type == ConcreteNodeType::LBRACE && parent->parent)
                        parent = parent->parent;

                    node = ConcreteNodePtr(new ConcreteNode());
                    node->token = token->lexeme;
                    node->file = file;
                    node->line = token->line;
                    node->type = ConcreteNodeType::RBRACE;

                    // Consume all the newlines
                    i = skipNewlines(i, end);

                    // Insert the node
                    if(parent)
                    {
                        node->parent = parent;
                        parent->children.push_back(node);
                    }
                    else
                    {
                        node->parent = nullptr;
                        nodes->push_back(node);
                    }

                    // Move up another level
                    if(parent)
                        parent = parent->parent;

                    node = ConcreteNodePtr();
                    state = READY;
                }
                else if(token->type == TID_VARIABLE)
                {
                    node = ConcreteNodePtr(new ConcreteNode());
                    node->token = token->lexeme;
                    node->file = file;
                    node->line = token->line;
                    node->type = ConcreteNodeType::VARIABLE;

                    // Insert the node
                    if(parent)
                    {
                        node->parent = parent;
                        parent->children.push_back(node);
                    }
                    else
                    {
                        node->parent = nullptr;
                        nodes->push_back(node);
                    }
                    node = ConcreteNodePtr();
                }
                else if(token->type == TID_QUOTE)
                {
                    node = ConcreteNodePtr(new ConcreteNode());
                    node->token = unquoted(token->lexeme);
                    node->file = file;
                    node->line = token->line;
                    node->type = ConcreteNodeType::QUOTE;

                    // Insert the node
                    if(parent)
                    {
                        node->parent = parent;
                        parent->children.push_back(node);
                    }
                    else
                    {
                        node->parent = nullptr;
                        nodes->push_back(node);
                    }
                    node = ConcreteNodePtr();
                }
                else if(token->type == TID_WORD)
                {
                    node = ConcreteNodePtr(new ConcreteNode());
                    node->token = token->lexeme;
                    node->file = file;
                    node->line = token->line;
                    node->type = ConcreteNodeType::WORD;

                    // Insert the node
                    if(parent)
                    {
                        node->parent = parent;
                        parent->children.push_back(node);
                    }
                    else
                    {
                        node->parent = nullptr;
                        nodes->push_back(node);
                    }
                    node = ConcreteNodePtr();
                }
                break;
            }

            ++i;
        }

        return nodes;
    }

    auto ScriptParser::parseChunk(const ScriptTokenList &tokens, std::string_view file) -> ConcreteNodeListPtr
    {
        // MEMCATEGORY_GENERAL because SharedPtr can only free using that category
        ConcreteNodeListPtr nodes(new ConcreteNodeList());

        ConcreteNodePtr node;
        const ScriptToken *token = nullptr;
        for(const auto & i : tokens)
        {
            token = &i;

            switch(token->type)
            {
            case TID_VARIABLE:
                node = ConcreteNodePtr(new ConcreteNode());
                node->file = file;
                node->line = token->line;
                node->parent = nullptr;
                node->token = token->lexeme;
                node->type = ConcreteNodeType::VARIABLE;
                break;
            case TID_WORD:
                node = ConcreteNodePtr(new ConcreteNode());
                node->file = file;
                node->line = token->line;
                node->parent = nullptr;
                node->token = token->lexeme;
                node->type = ConcreteNodeType::WORD;
                break;
            case TID_QUOTE:
                node = ConcreteNodePtr(new ConcreteNode());
                node->file = file;
                node->line = token->line;
                node->parent = nullptr;
                node->token= unquoted(token->lexeme);
                node->type = ConcreteNodeType::QUOTE;
                break;
            default:
                OGRE_EXCEPT(ExceptionCodes::INVALID_STATE, 
                    ::std::format("unexpected token {} at line {}",
                        token->lexeme, token->line),
                    "ScriptParser::parseChunk");
            }

            if(node)
                nodes->push_back(node);
        }

        return nodes;
    }

    auto ScriptParser::getToken(ScriptTokenList::const_iterator i, ScriptTokenList::const_iterator end, int offset) -> const ScriptToken *
    {
        const ScriptToken *token = nullptr;
        auto iter = i + offset;
        if(iter != end)
            token = &*i;
        return token;
    }

    auto ScriptParser::skipNewlines(ScriptTokenList::const_iterator i, ScriptTokenList::const_iterator end) -> ScriptTokenList::const_iterator
    {
        while(i != end && i->type == TID_NEWLINE)
            ++i;
        return i;
    }

}
