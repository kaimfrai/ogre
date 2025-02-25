// This file is part of the OGRE project.
// It is subject to the license terms in the LICENSE file found in the top-level directory
// of this distribution and at https://www.ogre3d.org/licensing.
module;

#include <SDL.h>
#include <SDL_events.h>
#include <SDL_gamecontroller.h>
#include <SDL_mouse.h>
#include <SDL_stdinc.h>
#include <SDL_syswm.h>
#include <SDL_version.h>
#include <SDL_video.h>
#include <cstddef>

module Ogre.Components.Bites;

import :ApplicationContext;
import :ApplicationContextBase;
import :SDLInputMapping;

import Ogre.Core;

import <map>;
import <string>;
import <utility>;
import <vector>;

namespace OgreBites {

ApplicationContextSDL::ApplicationContextSDL(std::string_view appName) : ApplicationContextBase(appName)
{
}

void ApplicationContextSDL::addInputListener(NativeWindowType* win, InputListener* lis)
{
    mInputListeners.insert(std::make_pair(SDL_GetWindowID(win), lis));
}


void ApplicationContextSDL::removeInputListener(NativeWindowType* win, InputListener* lis)
{
    mInputListeners.erase(std::make_pair(SDL_GetWindowID(win), lis));
}

auto ApplicationContextSDL::createWindow(std::string_view name, Ogre::uint32 w, Ogre::uint32 h, Ogre::NameValuePairList miscParams) -> NativeWindowPair
{
    NativeWindowPair ret = {nullptr, nullptr};

    if(!SDL_WasInit(SDL_INIT_VIDEO)) {

        SDL_InitSubSystem(SDL_INIT_VIDEO);
    }

    auto p = mRoot->getRenderSystem()->getRenderWindowDescription();
    miscParams.insert(p.miscParams.begin(), p.miscParams.end());
    p.miscParams = miscParams;
    p.name = name;

    if(w > 0 && h > 0)
    {
        p.width = w;
        p.height= h;
    }

    int flags = p.useFullScreen ? SDL_WINDOW_FULLSCREEN : SDL_WINDOW_RESIZABLE;
    int d = Ogre::StringConverter::parseInt(miscParams["monitorIndex"], 1) - 1;
    ret.native =
        SDL_CreateWindow(p.name.data(), SDL_WINDOWPOS_UNDEFINED_DISPLAY(d),
                         SDL_WINDOWPOS_UNDEFINED_DISPLAY(d), p.width, p.height, flags);

    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    SDL_GetWindowWMInfo(ret.native, &wmInfo);

    // for tiny rendersystem
    p.miscParams["sdlwin"] = Ogre::StringConverter::toString(size_t(ret.native));

    p.miscParams["externalWindowHandle"] = Ogre::StringConverter::toString(size_t(wmInfo.info.x11.window));

    if(!mWindows.empty())
    {
        // additional windows should reuse the context
        p.miscParams["currentGLContext"] = "true";
    }

    ret.render = mRoot->createRenderWindow(p);
    mWindows.push_back(ret);
    return ret;
}

void ApplicationContextSDL::_destroyWindow(const NativeWindowPair& win)
{
    ApplicationContextBase::_destroyWindow(win);
    if(win.native)
        SDL_DestroyWindow(win.native);
}

void ApplicationContextSDL::setWindowGrab(NativeWindowType* win, bool _grab)
{
    auto grab = SDL_bool(_grab);

    SDL_SetWindowGrab(win, grab);
    // osx workaround: mouse motion events are gone otherwise
    SDL_SetRelativeMouseMode(grab);
}

auto ApplicationContextSDL::getDisplayDPI() const noexcept -> float
{
    Ogre::OgreAssert(!mWindows.empty(), "create a window first");
    float vdpi = -1;
    if(SDL_GetDisplayDPI(0, nullptr, nullptr, &vdpi) == 0 && vdpi > 0)
        return vdpi;

    return ApplicationContextBase::getDisplayDPI();
}

void ApplicationContextSDL::shutdown()
{
    ApplicationContextBase::shutdown();

    if(SDL_WasInit(SDL_INIT_VIDEO)) {
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
    }
    SDL_Quit();
}

void ApplicationContextSDL::pollEvents()
{
    if(mWindows.empty())
    {
        // SDL events not initialized
        return;
    }

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
        case SDL_QUIT:
            mRoot->queueEndRendering();
            break;
        case SDL_WINDOWEVENT:
            if(event.window.event != SDL_WINDOWEVENT_RESIZED)
                continue;

            for(auto & mWindow : mWindows)
            {
                if(event.window.windowID != SDL_GetWindowID(mWindow.native))
                    continue;

                Ogre::RenderWindow* win = mWindow.render;
                win->resize(event.window.data1, event.window.data2);
                windowResized(win);
            }
            break;
        case SDL_CONTROLLERDEVICEADDED:
            if(auto c = SDL_GameControllerOpen(event.cdevice.which))
            {
                const char* name = SDL_GameControllerName(c);
                Ogre::LogManager::getSingleton().stream() << "Opened Gamepad: " << (name ? name : "unnamed");
            }
            break;
        default:
            _fireInputEvent(convert(event), event.window.windowID);
            break;
        }
    }
}

}
