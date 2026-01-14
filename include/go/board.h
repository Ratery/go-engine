#pragma once

#include <span>
#include <array>
#include <vector>
#include <string>
#include <optional>

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

        int ply_count() const noexcept {
            return static_cast<int>(history_.size());
        }

        std::array<int, 4> neigh4(int v) const;
        std::array<int, 4> diag_neigh(int v) const;

        bool move(Move m);
        void undo(int count = 1);

        std::vector<Move> pseudo_legal_moves() const;

        double evaluate(Color perspective) const;

        std::string dump(bool flip_vertical = true) const;

    private:
        int n_, stride_, ko_point_ = -1;
        double komi_;
        std::vector<Point> board_;
        std::vector<Undo> history_;
        std::vector<int> capture_pool_;
        Color to_play_ = Color::Black;

        std::span<const int> captured_span(const Undo& u) const noexcept;

        std::optional<Color> is_eyeish(int v) const;
        std::optional<Color> is_eye(int v) const;

        // DFS
        mutable std::vector<int> mark_;
        mutable int mark_id_ = 0;
        mutable std::vector<int> stack_;

        bool has_liberty(int v) const;
        int count_liberties(int v) const;
        void remove_group(int v, Undo& u);
    };

}  // namespace go
