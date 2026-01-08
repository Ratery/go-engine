#pragma once

#include <cstdint>

namespace go {

    enum class Color : std::uint8_t {
        Black,
        White
    };

    enum class Point : uint8_t {
        Empty,
        Black,
        White,
        Wall
    };

    inline Color Opp(Color c) {
        return c == Color::Black ? Color::White : Color::Black;
    }

    inline Point ToPoint(Color c) {
        return c == Color::Black ? Point::Black : Point::White;
    }

    inline bool Matches(Point p, Color c) {
        return p == ToPoint(c);
    }

    struct Move {
        int v;

        static Move Pass() {
            return Move(-1);
        }

        bool is_pass() const {
            return v < 0;
        }
    };

    struct Undo {
        Move move;
        Color played;
        int prev_ko;
        size_t cap_begin;
        size_t cap_count;
    };

}  // namespace go
