#include <sstream>
#include <iomanip>

#include "go/board.h"

namespace {

    char col_letter(int x) {
        return static_cast<char>('A' + x + (x >= 8 ? 1 : 0));
    }

    char point_char(go::Point p) {
        switch (p) {
            case go::Point::Black: return 'X';
            case go::Point::White: return 'O';
            case go::Point::Empty: return '.';
            default: return '#';
        }
    }

}

namespace go {

    Board::Board(int n)
        : n_(n),
          stride_(n + 2),
          board_(stride_ * stride_, Point::Wall)
    {
        for (int i = 1; i <= n; i++) {
            for (int j = 1; j <= n; j++) {
                board_[i * stride_ + j] = Point::Empty;
            }
        }
        mark_.assign(board_.size(), 0);
    }

    int Board::size() const {
        return n_;
    }

    Point Board::at(int v) const {
        return board_[v];
    }

    Point Board::at(int x, int y) const {
        return board_[(y + 1) * stride_ + x + 1];
    }

    std::array<int, 4> Board::neigh4(int v) const {
        return {v - 1, v + 1, v - stride_, v + stride_};
    }

    void Board::collect_group(int start, std::vector<int>& out) const {
        mark_id_++;
        out.clear();
        stack_.clear();
        stack_.push_back(start);
        mark_[start] = mark_id_;
        auto color = board_[start];
        while (!stack_.empty()) {
            int cur = stack_.back();
            stack_.pop_back();
            out.push_back(cur);
            for (int neigh : neigh4(cur)) {
                if (mark_[neigh] != mark_id_ && board_[neigh] == color) {
                    mark_[neigh] = mark_id_;
                    stack_.push_back(neigh);
                }
            }
        }
    }

    int Board::count_liberties(const std::vector<int>& group) const {
        mark_id_++;
        int liberties = 0;
        for (int v : group) {
            for (int neigh : neigh4(v)) {
                if (board_[neigh] == Point::Empty && mark_[neigh] != mark_id_) {
                    mark_[neigh] = mark_id_;
                    liberties++;
                }
            }
        }
        return liberties;
    }

    void Board::remove_group(const std::vector<int>& group, go::Undo& u) {
        for (int v : group) {
            board_[v] = Point::Empty;
            u.captured.push_back(v);
        }
    }

    bool Board::move(Move m, Undo& u) {
        u.captured.clear();
        u.move = m;
        u.played = to_play_;
        u.prev_ko = ko_point_;

        if (m.is_pass()) {
            ko_point_ = -1;
            to_play_ = Opp(to_play_);
            return true;
        }

        if (board_[m.v] != Point::Empty) {
            return false;
        }

        if (m.v == ko_point_) {  // check simple ko rule
            return false;
        }

        board_[m.v] = ToPoint(to_play_);

        std::vector<int> group;
        for (int neigh : neigh4(m.v)) {
            if (Matches(board_[neigh], Opp(to_play_))) {
                group.clear();
                collect_group(neigh, group);
                if (count_liberties(group) == 0) {
                    remove_group(group, u);
                }
            }
        }

        group.clear();
        collect_group(m.v, group);
        int liberties = count_liberties(group);

        if (liberties == 0) {  // suicidal move
            board_[m.v] = Point::Empty;
            for (int v : u.captured) {
                board_[v] = ToPoint(Opp(to_play_));
            }
            return false;
        }

        if (u.captured.size() == 1 && liberties == 1) {  // update ko point
            ko_point_ = u.captured.front();
        } else {
            ko_point_ = -1;
        }

        to_play_ = Opp(to_play_);
        return true;
    }

    void Board::undo(const go::Undo& u) {
        to_play_ = u.played;
        ko_point_ = u.prev_ko;
        if (!u.move.is_pass()) {
            board_[u.move.v] = Point::Empty;
            for (int v: u.captured) {
                board_[v] = ToPoint(Opp(u.played));
            }
        }
    }

    std::vector<Move> Board::legal_moves() const {
        std::vector<Move> moves;
        for (int i = 1; i <= n_; i++) {
            for (int j = 1; j <= n_; j++) {
                int pos = i * stride_ + j;
                if (board_[pos] == Point::Empty && pos != ko_point_) {
                    moves.push_back(Move(pos));
                }
            }
        }
        return moves;
    }

    std::string Board::dump(bool flip_vertical) const {
        std::ostringstream out;

        out << "   ";
        for (int x = 0; x < n_; x++) {
            out << col_letter(x) << ' ';
        }
        out << '\n';

        for (int ry = 0; ry < n_; ry++) {
            int y = flip_vertical ? (n_ - 1 - ry) : ry;
            int label = y + 1;

            out << std::setw(2) << label << ' ';

            for (int x = 0; x < n_; x++) {
                int v = (y + 1) * stride_ + (x + 1);
                out << point_char(board_[v]) << ' ';
            }

            out << std::setw(2) << label << '\n';
        }

        out << "   ";
        for (int x = 0; x < N; ++x) {
            out << col_letter(x) << ' ';
        }
        out << '\n';

        return out.str();
    }

}  // namespace go
