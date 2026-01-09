#pragma once

#include <span>
#include <array>
#include <vector>
#include <string>

#include "types.h"

namespace go {

    class Board {
    public:
        explicit Board(int n, double komi);

        int size() const noexcept {
            return n_;
        };

        Point at(int v) const noexcept {
            return board_[v];
        }

        Point at(int x, int y) const noexcept {
            return board_[(y + 1) * stride_ + x + 1];
        };

        Color to_play() const noexcept {
            return to_play_;
        }

        std::array<int, 4> neigh4(int v) const;

        bool move(Move m, Undo& u);
        void undo(const Undo& u);

        std::vector<Move> legal_moves();

        double evaluate(Color perspective) const;

        std::string dump(bool flip_vertical = true) const;

    private:
        int n_, stride_, ko_point_ = -1;
        double komi_;
        std::vector<Point> board_;
        std::vector<int> capture_pool_;
        Color to_play_ = Color::Black;

        std::span<const int> captured_span(const Undo& u) const noexcept;

        // DFS
        mutable std::vector<int> mark_;
        mutable int mark_id_ = 0;
        mutable std::vector<int> stack_;

        bool has_liberty(int v) const;
        int count_liberties(int v) const;
        void remove_group(int v, Undo& u);
    };

}  // namespace go
