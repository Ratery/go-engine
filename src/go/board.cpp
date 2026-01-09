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

    Board::Board(int n, double komi)
        : n_(n),
          komi_(komi),
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

    std::array<int, 4> Board::neigh4(int v) const {
        return {v - 1, v + 1, v - stride_, v + stride_};
    }

    std::span<const int> Board::captured_span(const Undo& u) const noexcept {
        return {
            capture_pool_.data() + u.cap_begin,
            u.cap_count
        };
    }

    bool Board::has_liberty(int v) const {
        mark_id_++;
        stack_.clear();
        stack_.push_back(v);
        mark_[v] = mark_id_;
        Point color = board_[v];
        while (!stack_.empty()) {
            int cur = stack_.back();
            stack_.pop_back();
            for (int neigh : neigh4(cur)) {
                if (board_[neigh] == Point::Empty) {
                    return true;
                }
                if (mark_[neigh] != mark_id_ && board_[neigh] == color) {
                    mark_[neigh] = mark_id_;
                    stack_.push_back(neigh);
                }
            }
        }
        return false;
    }

    int Board::count_liberties(int v) const {
        int liberties = 0;
        mark_id_++;
        stack_.clear();
        stack_.push_back(v);
        mark_[v] = mark_id_;
        Point color = board_[v];
        while (!stack_.empty()) {
            int cur = stack_.back();
            stack_.pop_back();
            for (int neigh : neigh4(cur)) {
                if (board_[neigh] == Point::Empty) {
                    liberties++;
                }
                if (mark_[neigh] != mark_id_ && board_[neigh] == color) {
                    mark_[neigh] = mark_id_;
                    stack_.push_back(neigh);
                }
            }
        }
        return liberties;
    }

    void Board::remove_group(int v, go::Undo& u) {
        mark_id_++;
        stack_.clear();
        stack_.push_back(v);
        mark_[v] = mark_id_;
        Point color = board_[v];
        while (!stack_.empty()) {
            int cur = stack_.back();
            stack_.pop_back();

            board_[cur] = Point::Empty;
            capture_pool_.push_back(cur);
            u.cap_count++;

            for (int neigh : neigh4(cur)) {
                if (mark_[neigh] != mark_id_ && board_[neigh] == color) {
                    mark_[neigh] = mark_id_;
                    stack_.push_back(neigh);
                }
            }
        }
    }

    bool Board::move(Move m, Undo& u) {
        u.move = m;
        u.played = to_play_;
        u.prev_ko = ko_point_;
        u.cap_begin = capture_pool_.size();
        u.cap_count = 0;

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

        bool in_enemy_eye = true;
        for (int neigh : neigh4(m.v)) {
            if (board_[neigh] == Point::Empty || Matches(board_[neigh], to_play_)) {
                in_enemy_eye = false;
            }
        }

        board_[m.v] = ToPoint(to_play_);

        for (int neigh : neigh4(m.v)) {
            if (Matches(board_[neigh], Opp(to_play_))) {
                if (!has_liberty(neigh)) {
                    remove_group(neigh, u);
                }
            }
        }

        if (!has_liberty(m.v)) {  // suicidal move
            board_[m.v] = Point::Empty;
            for (int cap : captured_span(u)) {
                board_[cap] = ToPoint(Opp(to_play_));
            }
            return false;
        }

        if (in_enemy_eye && u.cap_count == 1) {  // update ko point
            ko_point_ = captured_span(u).front();
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
            for (int v: captured_span(u)) {
                board_[v] = ToPoint(Opp(u.played));
            }
        }
        capture_pool_.resize(u.cap_begin);
    }

    std::vector<Move> Board::legal_moves() {  // TODO: make this method const
        std::vector<Move> moves;
        for (int i = 1; i <= n_; i++) {
            for (int j = 1; j <= n_; j++) {
                int pos = i * stride_ + j;
                Move m(pos);
                Undo u;
                if (move(m, u)) {
                    moves.push_back(m);
                    undo(u);
                }
            }
        }
        return moves;
    }

    double Board::evaluate(go::Color perspective) const {
        double score = 0;
        mark_id_++;
        stack_.clear();
        for (int i = 1; i <= n_; i++) {
            for (int j = 1; j <= n_; j++) {
                int pos = i * stride_ + j;
                Point p = board_[pos];
                if (Matches(p, perspective)) {
                    score++;
                    continue;
                }
                if (Matches(p, Opp(perspective))) {
                    score--;
                    continue;
                }
                if (p != Point::Empty || mark_[pos] == mark_id_) {
                    continue;
                }
                bool perspective_c = false, opposite_c = false;
                stack_.push_back(pos);
                mark_[pos] = mark_id_;
                int points = 0;
                while (!stack_.empty()) {
                    points++;
                    int cur = stack_.back();
                    stack_.pop_back();
                    for (int neigh: neigh4(cur)) {
                        if (Matches(board_[neigh], perspective)) {
                            perspective_c = true;
                        } else if (Matches(board_[neigh], Opp(perspective))) {
                            opposite_c = true;
                        }
                        if (mark_[neigh] != mark_id_ && board_[neigh] == Point::Empty) {
                            mark_[neigh] = mark_id_;
                            stack_.push_back(neigh);
                        }
                    }
                }
                if (perspective_c && !opposite_c) {
                    score += points;
                } else if (!perspective_c && opposite_c) {
                    score -= points;
                }
            }
        }
        score += (perspective == Color::White ? komi_ : -komi_);
        return score;
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
        for (int x = 0; x < n_; ++x) {
            out << col_letter(x) << ' ';
        }
        out << '\n';

        return out.str();
    }

}  // namespace go
