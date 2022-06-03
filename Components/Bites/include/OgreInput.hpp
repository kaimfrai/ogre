// This file is part of the OGRE project.
// It is subject to the license terms in the LICENSE file found in the top-level directory
// of this distribution and at https://www.ogre3d.org/licensing.
// SPDX-License-Identifier: MIT

#ifndef OGRE_COMPONENTS_BITES_INPUT_H
#define OGRE_COMPONENTS_BITES_INPUT_H

#include <utility>
#include <variant>
#include <vector>

namespace Ogre {
    struct FrameEvent;
}

namespace OgreBites {

/** \addtogroup Optional
*  @{
*/
/** \addtogroup Bites
*  @{
*/
/** @defgroup Input Input
 * SDL2 inspired input abstraction layer providing basic events
 * @{
 */
enum ButtonType {
    BUTTON_LEFT = 1,
    BUTTON_MIDDLE,
    BUTTON_RIGHT,
};

using Keycode = int;

struct Keysym {
    Keycode sym;
    unsigned short mod;
};

struct KeyboardEvent {
    Keysym keysym;
    unsigned char repeat;
};
struct KeyDownEvent : KeyboardEvent{};
struct KeyUpEvent : KeyboardEvent{};

struct MouseMotionEvent {
    int x, y;
    int xrel, yrel;
    int windowID;
};

struct MouseButtonEvent {
    int x, y;
    unsigned char button;
    unsigned char clicks;
};
struct MouseButtonUpEvent : MouseButtonEvent {};
struct MouseButtonDownEvent : MouseButtonEvent{};

struct MouseWheelEvent {
    int y;
};

struct TouchFingerEvent {
    int fingerId;
    float x, y;
    float dx, dy;
};
struct TouchFingerDownEvent : TouchFingerEvent{};
struct TouchFingerUpEvent : TouchFingerEvent{};
struct TouchFingerMotionEvent : TouchFingerEvent{};

struct TextInputEvent {
    const char* chars;
};
struct AxisEvent {
    int which;
    unsigned char axis;
    short value;
};
struct ButtonEvent {
    int which;
    unsigned char button;
};
struct ButtonDownEvent : ButtonEvent {};
struct ButtonUpEvent : ButtonEvent {};

using Event = ::std::variant
    <   ::std::monostate
    ,   KeyDownEvent
    ,   KeyUpEvent
    ,   MouseButtonUpEvent
    ,   MouseButtonDownEvent
    ,   MouseWheelEvent
    ,   MouseMotionEvent
    ,   TouchFingerDownEvent
    ,   TouchFingerUpEvent
    ,   TouchFingerMotionEvent
    ,   TextInputEvent
    ,   AxisEvent
    ,   ButtonDownEvent
    ,   ButtonUpEvent
    >
;

// SDL compat
enum {
    SDLK_DELETE = int('\177'),
    SDLK_RETURN = int('\r'),
    SDLK_ESCAPE = int('\033'),
    SDLK_SPACE = int(' '),
    SDLK_F1 = (1 << 30) | 0x3A,
    SDLK_F2,
    SDLK_F3,
    SDLK_F4,
    SDLK_F5,
    SDLK_F6,
    SDLK_F7,
    SDLK_F8,
    SDLK_F9,
    SDLK_F10,
    SDLK_F11,
    SDLK_F12,
    SDLK_PRINTSCREEN,
    SDLK_SCROLLLOCK,
    SDLK_PAUSE,
    SDLK_INSERT,
    SDLK_HOME,
    SDLK_PAGEUP,
    SDLK_END = (1 << 30) | 0x4D,
    SDLK_PAGEDOWN,
    SDLK_RIGHT,
    SDLK_LEFT,
    SDLK_DOWN,
    SDLK_UP,
    SDLK_NUMLOCKCLEAR,
    SDLK_KP_DIVIDE,
    SDLK_KP_MULTIPLY,
    SDLK_KP_MINUS,
    SDLK_KP_PLUS,
    SDLK_KP_ENTER,
    SDLK_KP_1,
    SDLK_KP_2,
    SDLK_KP_3,
    SDLK_KP_4,
    SDLK_KP_5,
    SDLK_KP_6,
    SDLK_KP_7,
    SDLK_KP_8,
    SDLK_KP_9,
    SDLK_KP_0,
    SDLK_KP_PERIOD,
    SDLK_LSHIFT = (1 << 30) | 0xE1,
    KMOD_ALT = 0x0100 | 0x0200,
    KMOD_CTRL = 0x0040 | 0x0080,
    KMOD_GUI = 0x0400 | 0x0800,
    KMOD_SHIFT = 0x0001 | 0x0002,
    KMOD_NUM = 0x1000,
};

/**
the return values of the callbacks are ignored by ApplicationContext
however they can be used to control event propagation in a hierarchy.
The convention is to return true if the event was handled and false if it should be further propagated.
*/
struct InputListener {
    virtual ~InputListener() = default;
    virtual void frameRendered(const Ogre::FrameEvent& evt) { }
    virtual bool keyPressed(const KeyDownEvent& evt) noexcept { return false;}
    virtual bool keyReleased(const KeyUpEvent& evt) noexcept { return false; }
    virtual bool touchMoved(const TouchFingerMotionEvent& evt) noexcept { return false; }
    virtual bool touchPressed(const TouchFingerDownEvent& evt) noexcept { return false; }
    virtual bool touchReleased(const TouchFingerUpEvent& evt) noexcept { return false; }
    virtual bool mouseMoved(const MouseMotionEvent& evt) noexcept { return false; }
    virtual bool mouseWheelRolled(const MouseWheelEvent& evt) noexcept { return false; }
    virtual bool mousePressed(const MouseButtonDownEvent& evt) noexcept { return false; }
    virtual bool mouseReleased(const MouseButtonUpEvent& evt) noexcept { return false; }
    virtual bool textInput(const TextInputEvent& evt) noexcept { return false; }
    virtual bool axisMoved(const AxisEvent& evt) noexcept { return false; }
    virtual bool buttonPressed(const ButtonDownEvent& evt) noexcept { return false; }
    virtual bool buttonReleased(const ButtonUpEvent& evt) noexcept { return false; }
};

/**
 * Chain of multiple InputListeners that acts as a single InputListener
 *
 * input events are propagated front to back until a listener returns true
 */
class InputListenerChain : public InputListener
{
protected:
    std::vector<InputListener*> mListenerChain;

public:
    InputListenerChain() = default;
    InputListenerChain(std::vector<InputListener*> chain) : mListenerChain(std::move(chain)) {}

    InputListenerChain& operator=(const InputListenerChain& o)
    {
        mListenerChain = o.mListenerChain;
        return *this;
    }

    bool keyPressed(const KeyDownEvent& evt) noexcept override
    {
        for (auto listner : mListenerChain)
        {
            if (listner->keyPressed(evt))
                return true;
        }
        return false;
    }
    bool keyReleased(const KeyUpEvent& evt) noexcept override
    {
        for (auto listner : mListenerChain)
        {
            if (listner->keyReleased(evt))
                return true;
        }
        return false;
    }
    bool touchMoved(const TouchFingerMotionEvent& evt) noexcept override
    {
        for (auto listner : mListenerChain)
        {
            if (listner->touchMoved(evt))
                return true;
        }
        return false;
    }
    bool touchPressed(const TouchFingerDownEvent& evt) noexcept override
    {
        for (auto listner : mListenerChain)
        {
            if (listner->touchPressed(evt))
                return true;
        }
        return false;
    }
    bool touchReleased(const TouchFingerUpEvent& evt) noexcept override
    {
        for (auto listner : mListenerChain)
        {
            if (listner->touchReleased(evt))
                return true;
        }
        return false;
    }
    bool mouseMoved(const MouseMotionEvent& evt) noexcept override
    {
        for (auto listner : mListenerChain)
        {
            if (listner->mouseMoved(evt))
                return true;
        }
        return false;
    }
    bool mouseWheelRolled(const MouseWheelEvent& evt) noexcept override
    {
        for (auto listner : mListenerChain)
        {
            if (listner->mouseWheelRolled(evt))
                return true;
        }
        return false;
    }
    bool mousePressed(const MouseButtonDownEvent& evt) noexcept override
    {
        for (auto listner : mListenerChain)
        {
            if (listner->mousePressed(evt))
                return true;
        }
        return false;
    }
    bool mouseReleased(const MouseButtonUpEvent& evt) noexcept override
    {
        for (auto listner : mListenerChain)
        {
            if (listner->mouseReleased(evt))
                return true;
        }
        return false;
    }
    bool textInput (const TextInputEvent& evt) noexcept override
    {
        for (auto listner : mListenerChain)
        {
            if (listner->textInput (evt))
                return true;
        }
        return false;
    }
};
/** @} */
/** @} */
/** @} */
}

#endif // OGRE_COMPONENTS_BITES_INPUT_H
