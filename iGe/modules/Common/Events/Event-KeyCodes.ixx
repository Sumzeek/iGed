module;
#include "iGeMacro.h"

export module iGe.Event:KeyCodes;

import std;

export enum class iGeKey : int {
    None = -1,

    // Mouse Buttons
    MouseLeft = 0, // Left Mouse Button
    MouseRight,    // Right Mouse Button
    MouseMiddle,   // Middle Mouse Button (usually scroll wheel button)
    MouseButton4,  // Additional Mouse Button 4 (e.g., forward navigation)
    MouseButton5,  // Additional Mouse Button 5 (e.g., backward navigation)

    // Number Keys (Top row, above QWERTY)
    _0 = 48, // 0
    _1,      // 1
    _2,      // 2
    _3,      // 3
    _4,      // 4
    _5,      // 5
    _6,      // 6
    _7,      // 7
    _8,      // 8
    _9,      // 9

    // Alphabet Keys
    A = 65, // A
    B,      // B
    C,      // C
    D,      // D
    E,      // E
    F,      // F
    G,      // G
    H,      // H
    I,      // I
    J,      // J
    K,      // K
    L,      // L
    M,      // M
    N,      // N
    O,      // O
    P,      // P
    Q,      // Q
    R,      // R
    S,      // S
    T,      // T
    U,      // U
    V,      // V
    W,      // W
    X,      // X
    Y,      // Y
    Z,      // Z

    // Function Keys
    F1,  // F1
    F2,  // F2
    F3,  // F3
    F4,  // F4
    F5,  // F5
    F6,  // F6
    F7,  // F7
    F8,  // F8
    F9,  // F9
    F10, // F10
    F11, // F11
    F12, // F12

    // Numpad Keys
    Numpad0,        // Numpad 0
    Numpad1,        // Numpad 1
    Numpad2,        // Numpad 2
    Numpad3,        // Numpad 3
    Numpad4,        // Numpad 4
    Numpad5,        // Numpad 5
    Numpad6,        // Numpad 6
    Numpad7,        // Numpad 7
    Numpad8,        // Numpad 8
    Numpad9,        // Numpad 9
    NumpadAdd,      // Numpad +
    NumpadSubtract, // Numpad -
    NumpadMultiply, // Numpad *
    NumpadDivide,   // Numpad /
    NumpadEnter,    // Numpad Enter
    NumpadDecimal,  // Numpad .

    // Control Keys
    Tab,          // Tab
    Enter,        // Enter / Return
    LeftShift,    // Left Shift
    RightShift,   // Right Shift
    LeftControl,  // Left Control (Ctrl)
    RightControl, // Right Control (Ctrl)
    LeftAlt,      // Left Alt
    RightAlt,     // Right Alt
    LeftSuper,    // Left Super (Windows in windows platform)
    RightSuper,   // Right Super (Windows in windows platform)
    Space,        // Spacebar
    CapsLock,     // Caps Lock
    Escape,       // Escape
    Backspace,    // Backspace
    PageUp,       // Page Up
    PageDown,     // Page Down
    Home,         // Home
    End,          // End
    Insert,       // Insert
    Delete,       // Delete
    LeftArrow,    // Left Arrow
    UpArrow,      // Up Arrow
    RightArrow,   // Right Arrow
    DownArrow,    // Down Arrow
    NumLock,      // Num Lock
    ScrollLock,   // Scroll Lock

    // Additional Keyboard Keys
    Apostrophe,   // ' (Apostrophe)
    Comma,        // , (Comma)
    Minus,        // - (Minus)
    Period,       // . (Period)
    Slash,        // / (Slash)
    Semicolon,    // ; (Semicolon)
    Equal,        // = (Equal Sign)
    LeftBracket,  // [ (Left Bracket)
    Backslash,    // \ (Backslash)
    RightBracket, // ] (Right Bracket)
    GraveAccent,  // ` (Grave Accent / Tilde)
};

export enum class iGeMouseButton : int { Left = 0, Right, Middle, Button4, Button5 };

export template<>
struct std::formatter<iGeKey> {
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    auto format(const iGeKey& key, format_context& ctx) const {
        std::string keyName;
        switch (key) {
            // Mouse Buttons
            case iGeKey::MouseLeft:
                keyName = "MouseLeft";
                break;
            case iGeKey::MouseRight:
                keyName = "MouseRight";
                break;
            case iGeKey::MouseMiddle:
                keyName = "MouseMiddle";
                break;
            case iGeKey::MouseButton4:
                keyName = "MouseButton4";
                break;
            case iGeKey::MouseButton5:
                keyName = "MouseButton5";
                break;

            // Number Keys
            case iGeKey::_0:
                keyName = "0";
                break;
            case iGeKey::_1:
                keyName = "1";
                break;
            case iGeKey::_2:
                keyName = "2";
                break;
            case iGeKey::_3:
                keyName = "3";
                break;
            case iGeKey::_4:
                keyName = "4";
                break;
            case iGeKey::_5:
                keyName = "5";
                break;
            case iGeKey::_6:
                keyName = "6";
                break;
            case iGeKey::_7:
                keyName = "7";
                break;
            case iGeKey::_8:
                keyName = "8";
                break;
            case iGeKey::_9:
                keyName = "9";
                break;

            // Alphabet Keys
            case iGeKey::A:
                keyName = "A";
                break;
            case iGeKey::B:
                keyName = "B";
                break;
            case iGeKey::C:
                keyName = "C";
                break;
            case iGeKey::D:
                keyName = "D";
                break;
            case iGeKey::E:
                keyName = "E";
                break;
            case iGeKey::F:
                keyName = "F";
                break;
            case iGeKey::G:
                keyName = "G";
                break;
            case iGeKey::H:
                keyName = "H";
                break;
            case iGeKey::I:
                keyName = "I";
                break;
            case iGeKey::J:
                keyName = "J";
                break;
            case iGeKey::K:
                keyName = "K";
                break;
            case iGeKey::L:
                keyName = "L";
                break;
            case iGeKey::M:
                keyName = "M";
                break;
            case iGeKey::N:
                keyName = "N";
                break;
            case iGeKey::O:
                keyName = "O";
                break;
            case iGeKey::P:
                keyName = "P";
                break;
            case iGeKey::Q:
                keyName = "Q";
                break;
            case iGeKey::R:
                keyName = "R";
                break;
            case iGeKey::S:
                keyName = "S";
                break;
            case iGeKey::T:
                keyName = "T";
                break;
            case iGeKey::U:
                keyName = "U";
                break;
            case iGeKey::V:
                keyName = "V";
                break;
            case iGeKey::W:
                keyName = "W";
                break;
            case iGeKey::X:
                keyName = "X";
                break;
            case iGeKey::Y:
                keyName = "Y";
                break;
            case iGeKey::Z:
                keyName = "Z";
                break;

            // Function Keys
            case iGeKey::F1:
                keyName = "F1";
                break;
            case iGeKey::F2:
                keyName = "F2";
                break;
            case iGeKey::F3:
                keyName = "F3";
                break;
            case iGeKey::F4:
                keyName = "F4";
                break;
            case iGeKey::F5:
                keyName = "F5";
                break;
            case iGeKey::F6:
                keyName = "F6";
                break;
            case iGeKey::F7:
                keyName = "F7";
                break;
            case iGeKey::F8:
                keyName = "F8";
                break;
            case iGeKey::F9:
                keyName = "F9";
                break;
            case iGeKey::F10:
                keyName = "F10";
                break;
            case iGeKey::F11:
                keyName = "F11";
                break;
            case iGeKey::F12:
                keyName = "F12";
                break;

            // Numpad Keys
            case iGeKey::Numpad0:
                keyName = "Numpad0";
                break;
            case iGeKey::Numpad1:
                keyName = "Numpad1";
                break;
            case iGeKey::Numpad2:
                keyName = "Numpad2";
                break;
            case iGeKey::Numpad3:
                keyName = "Numpad3";
                break;
            case iGeKey::Numpad4:
                keyName = "Numpad4";
                break;
            case iGeKey::Numpad5:
                keyName = "Numpad5";
                break;
            case iGeKey::Numpad6:
                keyName = "Numpad6";
                break;
            case iGeKey::Numpad7:
                keyName = "Numpad7";
                break;
            case iGeKey::Numpad8:
                keyName = "Numpad8";
                break;
            case iGeKey::Numpad9:
                keyName = "Numpad9";
                break;
            case iGeKey::NumpadAdd:
                keyName = "NumpadAdd";
                break;
            case iGeKey::NumpadSubtract:
                keyName = "NumpadSubtract";
                break;
            case iGeKey::NumpadMultiply:
                keyName = "NumpadMultiply";
                break;
            case iGeKey::NumpadDivide:
                keyName = "NumpadDivide";
                break;
            case iGeKey::NumpadEnter:
                keyName = "NumpadEnter";
                break;
            case iGeKey::NumpadDecimal:
                keyName = "NumpadDecimal";
                break;

            // Control Keys
            case iGeKey::Tab:
                keyName = "Tab";
                break;
            case iGeKey::Enter:
                keyName = "Enter";
                break;
            case iGeKey::LeftShift:
                keyName = "LeftShift";
                break;
            case iGeKey::RightShift:
                keyName = "RightShift";
                break;
            case iGeKey::LeftControl:
                keyName = "LeftControl";
                break;
            case iGeKey::RightControl:
                keyName = "RightControl";
                break;
            case iGeKey::LeftAlt:
                keyName = "LeftAlt";
                break;
            case iGeKey::RightAlt:
                keyName = "RightAlt";
                break;
            case iGeKey::LeftSuper:
                keyName = "LeftSuper";
                break;
            case iGeKey::RightSuper:
                keyName = "RightSuper";
                break;
            case iGeKey::Space:
                keyName = "Space";
                break;
            case iGeKey::CapsLock:
                keyName = "CapsLock";
                break;
            case iGeKey::Escape:
                keyName = "Escape";
                break;
            case iGeKey::Backspace:
                keyName = "Backspace";
                break;
            case iGeKey::PageUp:
                keyName = "PageUp";
                break;
            case iGeKey::PageDown:
                keyName = "PageDown";
                break;
            case iGeKey::Home:
                keyName = "Home";
                break;
            case iGeKey::End:
                keyName = "End";
                break;
            case iGeKey::Insert:
                keyName = "Insert";
                break;
            case iGeKey::Delete:
                keyName = "Delete";
                break;
            case iGeKey::LeftArrow:
                keyName = "LeftArrow";
                break;
            case iGeKey::UpArrow:
                keyName = "UpArrow";
                break;
            case iGeKey::RightArrow:
                keyName = "RightArrow";
                break;
            case iGeKey::DownArrow:
                keyName = "DownArrow";
                break;
            case iGeKey::NumLock:
                keyName = "NumLock";
                break;
            case iGeKey::ScrollLock:
                keyName = "ScrollLock";
                break;

            // Additional Keyboard Keys
            case iGeKey::Apostrophe:
                keyName = "Apostrophe";
                break;
            case iGeKey::Comma:
                keyName = "Comma";
                break;
            case iGeKey::Minus:
                keyName = "Minus";
                break;
            case iGeKey::Period:
                keyName = "Period";
                break;
            case iGeKey::Slash:
                keyName = "Slash";
                break;
            case iGeKey::Semicolon:
                keyName = "Semicolon";
                break;
            case iGeKey::Equal:
                keyName = "Equal";
                break;
            case iGeKey::LeftBracket:
                keyName = "LeftBracket";
                break;
            case iGeKey::Backslash:
                keyName = "Backslash";
                break;
            case iGeKey::RightBracket:
                keyName = "RightBracket";
                break;
            case iGeKey::GraveAccent:
                keyName = "GraveAccent";
                break;

            default:
                keyName = "UnknownKey";
                break;
        }

        return format_to(ctx.out(), "{}", keyName);
    }
};
