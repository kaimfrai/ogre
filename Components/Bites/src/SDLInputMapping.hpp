// This file is part of the OGRE project.
// It is subject to the license terms in the LICENSE file found in the top-level directory
// of this distribution and at https://www.ogre3d.org/licensing.
// SPDX-License-Identifier: MIT

#ifndef OGRE_COMPONENTS_BITES_SDLINPUTMAPPING_H
#define OGRE_COMPONENTS_BITES_SDLINPUTMAPPING_H

#include "OgreInput.hpp"

#include <bit>

namespace {
    auto convert(const SDL_Event& in) -> OgreBites::Event
    {
        using namespace OgreBites;
        switch(in.type)
        {
        case SDL_KEYDOWN: [[fallthrough]];
        case SDL_KEYUP:
        {
            KeyboardEvent const key
            {   .keysym = {
                    .sym = in.key.keysym.sym
                ,   .mod = in.key.keysym.mod
                }
            ,   .repeat = in.key.repeat
            };
            return in.type == SDL_KEYDOWN
                ?   Event{::std::bit_cast<KeyDownEvent>(key) }
                :   Event{::std::bit_cast<KeyUpEvent>(key) }
                ;
        }
        case SDL_MOUSEBUTTONUP: [[fallthrough]];
        case SDL_MOUSEBUTTONDOWN:
        {
            MouseButtonEvent const button
            {   .x = in.button.x
            ,   .y = in.button.y
            ,   .button = in.button.button
            ,   .clicks = in.button.clicks
            };
            return in.type == SDL_MOUSEBUTTONUP
                ?   Event{::std::bit_cast<MouseButtonUpEvent>(button) }
                :   Event{::std::bit_cast<MouseButtonDownEvent>(button) }
                ;
        }
        case SDL_MOUSEWHEEL:
            return MouseWheelEvent{in.wheel.y};
        case SDL_MOUSEMOTION:
            return MouseMotionEvent
            {   .x = in.motion.x
            ,   .y = in.motion.y
            ,   .xrel = in.motion.xrel
            ,   .yrel = in.motion.yrel
            ,   .windowID = static_cast<int>(in.motion.windowID)
            };

        case SDL_FINGERDOWN: [[fallthrough]];
        case SDL_FINGERUP: [[fallthrough]];
        case SDL_FINGERMOTION:
        {    TouchFingerEvent const tfinger
            {   .fingerId = static_cast<int>(in.tfinger.fingerId)
            ,   .x = in.tfinger.x
            ,   .y = in.tfinger.y
            ,   .dx = in.tfinger.dx
            ,   .dy = in.tfinger.dy
            };
            return in.type == SDL_FINGERDOWN
                ?   Event{ ::std::bit_cast<TouchFingerDownEvent>(tfinger) }
                :   in.type == SDL_FINGERUP
                ?   Event{ ::std::bit_cast<TouchFingerUpEvent>(tfinger) }
                :   Event{ ::std::bit_cast<TouchFingerMotionEvent>(tfinger) }
                ;
        }
        case SDL_TEXTINPUT:
            return TextInputEvent{in.text.text};

        case SDL_CONTROLLERAXISMOTION:
            return AxisEvent
            {   .which = in.caxis.which
            ,   .axis = in.caxis.axis
            ,   .value = in.caxis.value
            };
        case SDL_CONTROLLERBUTTONDOWN: [[fallthrough]];
        case SDL_CONTROLLERBUTTONUP:
        {    ButtonEvent const cButton =
            {   .which = in.cbutton.which
            ,   .button = in.cbutton.button
            };
            return in.type == SDL_CONTROLLERBUTTONDOWN
                ?   Event{ ::std::bit_cast<ButtonDownEvent>(cButton) }
                :   Event{ ::std::bit_cast<ButtonUpEvent>(cButton) }
                ;
        }
        }

        return {};
    }
}

#endif
