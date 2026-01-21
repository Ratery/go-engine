#pragma once

#include <span>
#include <array>
#include <vector>
#include <string>
#include <optional>
#include <utility>

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

        int ko_point() const noexcept {
            return ko_point_;
        }

        int ko_age() const noexcept {
            return ko_age_;
        }

        std::array<int, 4> neigh4(int v) const;
        std::array<int, 4> diag_neigh(int v) const;
        std::array<int, 8> neigh8(int v) const;

        std::pair<std::array<int, 18>, int> last_moves_neigh() const;

        bool move(Move m);
        void undo(int count = 1);

        void gen_pseudo_legal_moves(std::vector<Move>& moves) const;

        bool is_capture(Move m);

        double evaluate(Color perspective) const;

        std::string dump(bool flip_vertical = true) const;

    private:
        int n_, stride_;
        int ko_point_ = -1, ko_age_ = -1;
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
