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
#include "OgreWindowEventUtilities.hpp"

#include <X11/X.h>
#include <X11/Xlib.h>
#include <algorithm>
#include <map>
#include <utility>
#include <vector>

#include "OgreCommon.hpp"
#include "OgreRenderWindow.hpp"

using namespace Ogre;

namespace OgreBites {
    using WindowEventListeners = std::multimap<RenderWindow *, WindowEventListener *>;
    static WindowEventListeners _msListeners;
    static RenderWindowList _msWindows;

static void GLXProc( RenderWindow *win, const XEvent &event );

//--------------------------------------------------------------------------------//
void WindowEventUtilities::messagePump()
{

    for (Display* xDisplay = nullptr; // same for all windows
        auto const& win : _msWindows)
    {
        XID xid;
        XEvent event;

        if (!xDisplay)
            win->getCustomAttribute("XDISPLAY", &xDisplay);

        win->getCustomAttribute("WINDOW", &xid);

        while (XCheckWindowEvent (xDisplay, xid, StructureNotifyMask | VisibilityChangeMask | FocusChangeMask, &event))
        {
            GLXProc(win, event);
        }

        // The ClientMessage event does not appear under any Event Mask
        while (XCheckTypedWindowEvent (xDisplay, xid, ClientMessage, &event))
        {
            GLXProc(win, event);
        }
    }
}

//--------------------------------------------------------------------------------//
void WindowEventUtilities::addWindowEventListener( RenderWindow* window, WindowEventListener* listener )
{
    _msListeners.insert(std::make_pair(window, listener));
}

//--------------------------------------------------------------------------------//
void WindowEventUtilities::removeWindowEventListener( RenderWindow* window, WindowEventListener* listener )
{
    auto i = _msListeners.begin(), e = _msListeners.end();

    for( ; i != e; ++i )
    {
        if( i->first == window && i->second == listener )
        {
            _msListeners.erase(i);
            break;
        }
    }
}

//--------------------------------------------------------------------------------//
void WindowEventUtilities::_addRenderWindow(RenderWindow* window)
{
    _msWindows.push_back(window);
}

//--------------------------------------------------------------------------------//
void WindowEventUtilities::_removeRenderWindow(RenderWindow* window)
{
    auto i = std::ranges::find(_msWindows, window);
    if( i != _msWindows.end() )
        _msWindows.erase( i );
}

//--------------------------------------------------------------------------------//
static void GLXProc( Ogre::RenderWindow *win, const XEvent &event )
{
    //An iterator for the window listeners
    auto
        start = _msListeners.lower_bound(win),
        end   = _msListeners.upper_bound(win);

    struct
        Span
    {
        decltype(start) Begin;
        decltype(end) End;

        auto begin() { return Begin; }
        auto end() { return End; }
    }   span
    {   start
    ,   end
    };

    switch(event.type)
    {
    case ClientMessage:
    {
        ::Atom atom;
        win->getCustomAttribute("ATOM", &atom);
        if(event.xclient.format == 32 && event.xclient.data.l[0] == (long)atom)
        {   //Window closed by window manager
            //Send message first, to allow app chance to unregister things that need done before
            //window is shutdown
            bool close = true;
            for(auto const& index : span)
            {
                if (!index.second->windowClosing(win))
                    close = false;
            }
            if (!close) return;

            for(auto const& index : span)
                index.second->windowClosed(win);
            win->destroy();
        }
        break;
    }
    case DestroyNotify:
    {
        if (!win->isClosed())
        {
            // Window closed without window manager warning.
            for(auto const& index : span)
                index.second->windowClosed(win);
            win->destroy();
        }
        break;
    }
    case ConfigureNotify:
    {
        unsigned int oldWidth, oldHeight;
        int oldLeft, oldTop;
        win->getMetrics(oldWidth, oldHeight, oldLeft, oldTop);
        win->resize(event.xconfigurerequest.width, event.xconfigurerequest.height);

        unsigned int newWidth, newHeight;
        int newLeft, newTop;
        win->getMetrics(newWidth, newHeight, newLeft, newTop);

        if (newLeft != oldLeft || newTop != oldTop)
        {
            for(auto const& index : span)
                index.second->windowMoved(win);
        }

        if (newWidth != oldWidth || newHeight != oldHeight)
        {
            for(auto const& index : span)
                index.second->windowResized(win);
        }
        break;
    }
    case FocusIn:     // Gained keyboard focus
    case FocusOut:    // Lost keyboard focus
        for(auto const& index : span)
            index.second->windowFocusChange(win);
        break;
    case MapNotify:   //Restored
        win->setActive( true );
        for(auto const& index : span)
            index.second->windowFocusChange(win);
        break;
    case UnmapNotify: //Minimised
        win->setActive( false );
        win->setVisible( false );
        for(auto const& index : span)
            index.second->windowFocusChange(win);
        break;
    case VisibilityNotify:
        switch(event.xvisibility.state)
        {
        case VisibilityUnobscured:
            win->setActive( true );
            win->setVisible( true );
            break;
        case VisibilityPartiallyObscured:
            win->setActive( true );
            win->setVisible( true );
            break;
        case VisibilityFullyObscured:
            win->setActive( false );
            win->setVisible( false );
            break;
        }
        for(auto const& index : span)
            index.second->windowFocusChange(win);
        break;
    default:
        break;
    } //End switch event.type
}

}
