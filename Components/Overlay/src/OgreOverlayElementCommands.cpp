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
module Ogre.Components.Overlay;

import :Element;
import :ElementCommands;

import Ogre.Core;

import <string>;

namespace Ogre::OverlayElementCommands {

        //-----------------------------------------------------------------------
        //-----------------------------------------------------------------------
        auto CmdMaterial::doGet(const void* target) const -> String
        {
            return std::string{ static_cast<const OverlayElement*>(target)->getMaterialName() };
        }
        void CmdMaterial::doSet(void* target, std::string_view val)
        {
            if (val != "")
            {
                static_cast<OverlayElement*>(target)->setMaterialName(val);
            }
        }
        //-----------------------------------------------------------------------
        //-----------------------------------------------------------------------
        auto CmdCaption::doGet(const void* target) const -> String
        {
            return std::string{ static_cast<const OverlayElement*>(target)->getCaption() };
        }
        void CmdCaption::doSet(void* target, std::string_view val)
        {
            static_cast<OverlayElement*>(target)->setCaption(val);
        }
        //-----------------------------------------------------------------------
        //-----------------------------------------------------------------------
        //-----------------------------------------------------------------------
        auto CmdMetricsMode::doGet(const void* target) const -> String
        {
            GuiMetricsMode gmm = 
                static_cast<const OverlayElement*>(target)->getMetricsMode();

            switch (gmm)
            {
            case GuiMetricsMode::PIXELS :
                return "pixels";

            case GuiMetricsMode::RELATIVE_ASPECT_ADJUSTED :
                return "relative_aspect_adjusted";

            default :
                return "relative";
            }
        }
        void CmdMetricsMode::doSet(void* target, std::string_view val)
        {
            if (val == "pixels")
            {
                static_cast<OverlayElement*>(target)->setMetricsMode(GuiMetricsMode::PIXELS);
            }
            else if (val == "relative_aspect_adjusted")
            {
                static_cast<OverlayElement*>(target)->setMetricsMode(GuiMetricsMode::RELATIVE_ASPECT_ADJUSTED);
            }
            else
            {
                static_cast<OverlayElement*>(target)->setMetricsMode(GuiMetricsMode::RELATIVE);
            }
        }
        //-----------------------------------------------------------------------
        //-----------------------------------------------------------------------
        //-----------------------------------------------------------------------
        auto CmdHorizontalAlign::doGet(const void* target) const -> String
        {
            GuiHorizontalAlignment gha = 
                static_cast<const OverlayElement*>(target)->getHorizontalAlignment();
            using enum GuiHorizontalAlignment;
            switch(gha)
            {
            case LEFT:
                return "left";
            case RIGHT:
                return "right";
            case CENTER:
                return "center";
            }
            // To keep compiler happy
            return "center";
        }
        void CmdHorizontalAlign::doSet(void* target, std::string_view val)
        {
            if (val == "left")
            {
                static_cast<OverlayElement*>(target)->setHorizontalAlignment(GuiHorizontalAlignment::LEFT);
            }
            else if (val == "right")
            {
                static_cast<OverlayElement*>(target)->setHorizontalAlignment(GuiHorizontalAlignment::RIGHT);
            }
            else
            {
                static_cast<OverlayElement*>(target)->setHorizontalAlignment(GuiHorizontalAlignment::CENTER);
            }
        }
        //-----------------------------------------------------------------------
        //-----------------------------------------------------------------------
        //-----------------------------------------------------------------------
        auto CmdVerticalAlign::doGet(const void* target) const -> String
        {
            using enum GuiVerticalAlignment;
            GuiVerticalAlignment gva = 
                static_cast<const OverlayElement*>(target)->getVerticalAlignment();
            switch(gva)
            {
            case TOP:
                return "top";
            case BOTTOM:
                return "bottom";
            case CENTER:
                return "center";
            }
            // To keep compiler happy
            return "center";
        }
        void CmdVerticalAlign::doSet(void* target, std::string_view val)
        {
            if (val == "top")
            {
                static_cast<OverlayElement*>(target)->setVerticalAlignment(GuiVerticalAlignment::TOP);
            }
            else if (val == "bottom")
            {
                static_cast<OverlayElement*>(target)->setVerticalAlignment(GuiVerticalAlignment::BOTTOM);
            }
            else
            {
                static_cast<OverlayElement*>(target)->setVerticalAlignment(GuiVerticalAlignment::CENTER);
            }
        }
        //-----------------------------------------------------------------------
        //-----------------------------------------------------------------------
        //-----------------------------------------------------------------------
        //-----------------------------------------------------------------------
        auto CmdVisible::doGet(const void* target) const -> String
        {
            return StringConverter::toString(static_cast<const OverlayElement*>(target)->isVisible());
        }
        void CmdVisible::doSet(void* target, std::string_view val)
        {
            if (val == "true")
            {
                static_cast<OverlayElement*>(target)->show();
            }
            else if (val == "false")
            {
                static_cast<OverlayElement*>(target)->hide();
            }
        }
        //-----------------------------------------------------------------------
    }
