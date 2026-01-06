#pragma once

#include <array>
#include <vector>
#include <string>

#include "types.h"

namespace go {

    class Board {
    public:
        explicit Board(int n);

        int size() const;
        Point at(int v) const;
        Point at(int x, int y) const;

        std::array<int, 4> neigh4(int v) const;

        bool move(Move m, Undo& u);
        void undo(const Undo& u);

        std::vector<Move> legal_moves();

        std::string dump(bool flip_vertical = true) const;

    private:
        int n_, stride_, ko_point_;
        std::vector<Point> board_;
        Color to_play_ = Color::Black;

        // DFS
        mutable std::vector<int> mark_;
        mutable int mark_id_ = 0;
        mutable std::vector<int> stack_;

        void collect_group(int start, std::vector<int>& out) const;
        int count_liberties(const std::vector<int>& group) const;
        void remove_group(const std::vector<int>& group, Undo& u);
    };

}  // namespace go
