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
module;

#include <X11/X.h>
#include <X11/Xlib.h>

module Ogre.Components.Bites;

import :WindowEventUtilities;

import Ogre.Core;

import <algorithm>;
import <map>;
import <utility>;
import <vector>;

using namespace Ogre;

namespace OgreBites {
    using WindowEventListeners = std::multimap<RenderWindow *, WindowEventListener *>;
    static WindowEventListeners _msListeners;
    static RenderWindowList _msWindows;

static void GLXProc( RenderWindow *win, const XEvent &event );

//--------------------------------------------------------------------------------//
void WindowEventUtilities::messagePump()
{
    //GLX Message Pump
    auto win = _msWindows.begin();
    auto end = _msWindows.end();

    Display* xDisplay = nullptr; // same for all windows

    for (; win != end; win++)
    {
        XID xid;
        XEvent event;

        if (!xDisplay)
        (*win)->getCustomAttribute("XDISPLAY", &xDisplay);

        (*win)->getCustomAttribute("WINDOW", &xid);

        while (XCheckWindowEvent (xDisplay, xid, StructureNotifyMask | VisibilityChangeMask | FocusChangeMask, &event))
        {
        GLXProc(*win, event);
        }

        // The ClientMessage event does not appear under any Event Mask
        while (XCheckTypedWindowEvent (xDisplay, xid, ClientMessage, &event))
        {
        GLXProc(*win, event);
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
    auto i = std::find(_msWindows.begin(), _msWindows.end(), window);
    if( i != _msWindows.end() )
        _msWindows.erase( i );
}

//--------------------------------------------------------------------------------//
static void GLXProc( Ogre::RenderWindow *win, const XEvent &event )
{
    //An iterator for the window listeners
    WindowEventListeners::iterator index,
        start = _msListeners.lower_bound(win),
        end   = _msListeners.upper_bound(win);

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
            for(index = start ; index != end; ++index)
            {
                if (!(index->second)->windowClosing(win))
                    close = false;
            }
            if (!close) return;

            for(index = start ; index != end; ++index)
                (index->second)->windowClosed(win);
            win->destroy();
        }
        break;
    }
    case DestroyNotify:
    {
        if (!win->isClosed())
        {
            // Window closed without window manager warning.
            for(index = start ; index != end; ++index)
                (index->second)->windowClosed(win);
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
            for(index = start ; index != end; ++index)
                (index->second)->windowMoved(win);
        }

        if (newWidth != oldWidth || newHeight != oldHeight)
        {
            for(index = start ; index != end; ++index)
                (index->second)->windowResized(win);
        }
        break;
    }
    case FocusIn:     // Gained keyboard focus
    case FocusOut:    // Lost keyboard focus
        for(index = start ; index != end; ++index)
            (index->second)->windowFocusChange(win);
        break;
    case MapNotify:   //Restored
        win->setActive( true );
        for(index = start ; index != end; ++index)
            (index->second)->windowFocusChange(win);
        break;
    case UnmapNotify: //Minimised
        win->setActive( false );
        win->setVisible( false );
        for(index = start ; index != end; ++index)
            (index->second)->windowFocusChange(win);
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
        for(index = start ; index != end; ++index)
            (index->second)->windowFocusChange(win);
        break;
    default:
        break;
    } //End switch event.type
}

}
