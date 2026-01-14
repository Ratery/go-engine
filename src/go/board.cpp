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
        return {
            v - 1,
            v + 1,
            v - stride_,
            v + stride_
        };
    }

    std::array<int, 4> Board::diag_neigh(int v) const {
        return {
            v - stride_ - 1,
            v - stride_ + 1,
            v + stride_ - 1,
            v + stride_ + 1
        };
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

    bool Board::move(Move m) {
        Undo u{
            .move = m,
            .played = to_play_,
            .ko_point = ko_point_,
            .ko_age = ko_age_,
            .cap_begin = capture_pool_.size(),
            .cap_count = 0
        };

        if (m.is_pass()) {
            to_play_ = Opp(to_play_);
            return true;
        }

        if (board_[m.v] != Point::Empty) {
            return false;
        }

        if (m.v == ko_point_ && ko_age_ == ply_count()) {  // check simple ko rule
            return false;
        }

        bool in_enemy_eye = false;
        if (is_eyeish(m.v) == Opp(to_play_)) {
            in_enemy_eye = true;
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
            ko_age_ = ply_count() + 1;
        }

        to_play_ = Opp(to_play_);
        history_.push_back(u);
        return true;
    }

    void Board::undo(int count) {
        int size = static_cast<int>(history_.size());
        int new_size = size - count;
        for (int i = size - 1; i >= new_size; i--) {
            Undo& u = history_[i];
            to_play_ = u.played;
            ko_point_ = u.ko_point;
            ko_age_ = u.ko_age;
            if (!u.move.is_pass()) {
                board_[u.move.v] = Point::Empty;
                for (int v: captured_span(u)) {
                    board_[v] = ToPoint(Opp(u.played));
                }
            }
        }
        capture_pool_.resize(history_[new_size].cap_begin);
        history_.resize(new_size);
    }

    std::vector<Move> Board::pseudo_legal_moves() const {
        std::vector<Move> moves;
        for (int i = 1; i <= n_; i++) {
            for (int j = 1; j <= n_; j++) {
                int pos = i * stride_ + j;
                if (board_[pos] == Point::Empty && pos != ko_point_ && !is_eye(pos)) {
                    moves.push_back(Move(pos));
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

    std::optional<Color> Board::is_eyeish(int v) const {
        if (board_[v] != Point::Empty) {
            return std::nullopt;
        }
        std::optional<Color> eye_color = std::nullopt;
        for (int neigh : neigh4(v)) {
            if (board_[neigh] == Point::Empty) {
                return std::nullopt;
            }
            if (board_[neigh] == Point::Wall) {
                continue;
            }
            if (eye_color.has_value()) {
                if (eye_color != ToColor(board_[neigh])) {
                    return std::nullopt;
                }
            } else {
                eye_color = ToColor(board_[neigh]);
            }
        }
        return eye_color;
    }

    std::optional<Color> Board::is_eye(int v) const {
        std::optional<Color> eye_color = is_eyeish(v);
        if (!eye_color) {
            return std::nullopt;
        }
        bool at_edge = false;
        Color opp_color = Opp(eye_color.value());
        int opp_count = 0;
        for (int neigh : diag_neigh(v)) {
            if (board_[neigh] == Point::Wall) {
                at_edge = true;
            } else if (Matches(board_[neigh], opp_color)) {
                opp_count++;
            }
        }
        if (at_edge) {
            opp_count++;
        }
        if (opp_count >= 2) {
            return std::nullopt;
        }
        return eye_color;
    }

}  // namespace go
