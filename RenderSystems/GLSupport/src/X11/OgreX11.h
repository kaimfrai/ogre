// This file is part of the OGRE project.
// It is subject to the license terms in the LICENSE file found in the top-level directory
// of this distribution and at https://www.ogre3d.org/licensing.
// SPDX-License-Identifier: MIT
module;

#include <X11/X.h>
#ifndef Status
#define Status int
#endif
#include <X11/Xlib.h>
#include <X11/Xutil.h>

export module Ogre.RenderSystems.GLSupport.X11;

import Ogre.Core;
import Ogre.RenderSystems.GLSupport;

export
namespace Ogre
{
Display* getXDisplay(Display* glDisplay, Atom& deleteWindow, Atom& fullScreen, Atom& state);

void validateParentWindow(Display* display, Window parentWindow);

Window createXWindow(Display* display, Window parent, XVisualInfo* visualInfo, int& left, int& top, uint& width,
                     uint& height, Atom wmFullScreen, bool fullScreen);

void destroyXWindow(Display* display, Window window);

void queryRect(Display* display, Window window, int& left, int& top, uint& width, uint& height, bool queryOffset);

void finaliseTopLevel(Display* display, Window window, int& left, int& top, uint& width, uint& height, String& title,
                      Atom wmDelete);

bool getXVideoModes(Display* display, VideoMode& currentMode, VideoModes& videoModes);
} // namespace Ogre
