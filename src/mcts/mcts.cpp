#include "mcts/mcts.h"

#include <cmath>

#include "mcts/playout.h"

namespace mcts {

    go::Move MCTS::search(go::Board pos, int iters) {
        nodes_.clear();
        nodes_.emplace_back(go::Move::Pass(), -1);
        int root_ply_count = pos.ply_count();

        std::vector<go::Point> amaf_map;
        for (int it = 0; it < iters; it++) {
            amaf_map.assign((pos.size() + 1) * (pos.size() + 1), go::Point::Empty);

            int leaf = descend(pos, amaf_map);

            if (nodes_[leaf].children.empty()) {
                expand(leaf, pos);
                int child = nodes_[leaf].children[0];
                pos.move(nodes_[child].move);
                leaf = child;
            }

            double score = playout(pos, amaf_map);
            backprop(leaf, score, amaf_map);
            pos.undo(pos.ply_count() - root_ply_count);  // rollback
        }

        if (nodes_[0].children.empty()) {
            return go::Move::Pass();
        }

        int best_child = -1, max_visits = 0;
        for (int child_id : nodes_[0].children) {
            if (nodes_[child_id].v > max_visits) {
                max_visits = nodes_[child_id].v;
                best_child = child_id;
            }
        }
        return nodes_[best_child].move;
    }


    int MCTS::select_child(int parent_id) {
        const Node& parent = nodes_[parent_id];
        const std::vector<int>& children = parent.children;

        if (children.empty()) {
            return -1;
        }

        int best_child = -1;
        double best_score = -std::numeric_limits<double>::infinity();

        for (int child_id : children) {
            const Node& child = nodes_[child_id];

            double score;
            double v = child.v + child.pv;
            double expectation = (child.w + child.pw) / v;
            if (child.av == 0) {
                score = expectation;
            } else {
                const int RAVE_EQUIV = 3500;
                double rave_expectation = static_cast<double>(child.aw) / child.av;
                double beta = child.av / (child.av + v + v * child.av / RAVE_EQUIV);
                score = beta * rave_expectation + (1 - beta) * expectation;
            }

            if (score > best_score) {
                best_score = score;
                best_child = child_id;
            }
        }
        return best_child;
    }

    void MCTS::expand(int node_id, go::Board& pos) {
        if (!nodes_[node_id].children.empty()) {
            return;
        }
        std::vector<go::Move> moves;
        pos.gen_pseudo_legal_moves(moves);
        for (go::Move m : moves) {
            Node child{
                .move = m,
                .parent = node_id,
                .just_played = pos.to_play()
            };
            nodes_.push_back(child);
            int child_id = static_cast<int>(nodes_.size()) - 1;
            nodes_[node_id].children.push_back(child_id);
        }
    }

    int MCTS::descend(go::Board& pos, std::vector<go::Point>& amaf_map) {
        int cur_id = 0;
        while (!nodes_[cur_id].children.empty()) {
            int child_id = select_child(cur_id);
            Node& child = nodes_[child_id];
            pos.move(child.move);

            if (amaf_map[child.move.v] == go::Point::Empty) {
                amaf_map[child.move.v] = go::ToPoint(child.just_played);
            }

            cur_id = child_id;
        }
        return cur_id;
    }

    double MCTS::playout(go::Board& pos, std::vector<go::Point>& amaf_map) {
        int passes = 0, moves = 0;
        const int max_moves = 3 * pos.size() * pos.size();
        go::Color perspective = pos.to_play();

        while (passes < 2 && moves++ < max_moves) {
            go::Move m = play_heuristic_move(pos, rng_);
            if (m.is_pass()) {
                passes++;
            } else {
                passes = 0;
                if (amaf_map[m.v] == go::Point::Empty) {
                    amaf_map[m.v] = go::ToPoint(go::Opp(pos.to_play()));
                }
            }
        }

        return pos.evaluate(perspective);  // score for to-play color in start position
    }

    void MCTS::backprop(int node_id, double score, const std::vector<go::Point>& amaf_map) {
        int cur_id = node_id;
        while (cur_id != -1) {
            Node& cur = nodes_[cur_id];
            cur.v++;
            if (score < 0) {  // score is for to-play, w is for just-played (parent perspective)
                cur.w++;  // if node is loss for to-play, it is winning move for parent
            }

            for (int child_id : cur.children) {
                Node& child = nodes_[child_id];
                if (go::Matches(amaf_map[child.move.v], child.just_played)) {
                    child.av++;
                    if (score > 0) {
                        child.aw++;
                    }
                }
            }

            cur_id = cur.parent;
            score *= -1;
        }
    }

}  // namespace mcts
