#include "mcts/mcts.h"

#include <cmath>

#include "mcts/playout.h"

namespace mcts {

    go::Move MCTS::search(go::Board pos, int iters) {
        nodes_.clear();
        nodes_.emplace_back(go::Move::Pass(), -1);
        root_color_ = pos.to_play();
        int root_ply_count = pos.ply_count();

        for (int it = 0; it < iters; it++) {
            int leaf = descend(pos);

            if (nodes_[leaf].children.empty()) {
                expand(leaf, pos);
                int child = nodes_[leaf].children[0];
                pos.move(nodes_[child].move);
                leaf = child;
            }

            int result = playout(pos);
            backprop(leaf, result);
            pos.undo(pos.ply_count() - root_ply_count);  // rollback
        }

        if (nodes_[0].children.empty()) {
            return go::Move::Pass();
        }

        int best_child = -1, max_visits = 0;
        for (int child_id : nodes_[0].children) {
            if (nodes_[child_id].visits > max_visits) {
                max_visits = nodes_[child_id].visits;
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
            if (child.visits == 0) {
                score = std::numeric_limits<double>::infinity();
            } else {
                // UCB1 score
                const double C = 1.41421356;
                double exploitation = static_cast<double>(child.wins) / child.visits;
                double exploration = C * std::sqrt(std::log(parent.visits + 1) / child.visits);
                score = exploitation + exploration;
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
        for (go::Move m : gen_playout_moves(pos)) {
            Node child(m, node_id);
            nodes_.push_back(child);
            int child_id = (int)nodes_.size() - 1;
            nodes_[node_id].children.push_back(child_id);
        }
    }

    int MCTS::descend(go::Board& pos) {
        int cur_id = 0;
        while (!nodes_[cur_id].children.empty()) {
            int child_id = select_child(cur_id);
            Node& child = nodes_[child_id];
            pos.move(child.move);
            cur_id = child_id;
        }
        return cur_id;
    }

    int MCTS::playout(go::Board& pos) {
        int passes = 0, moves = 0;
        const int max_moves = 3 * pos.size() * pos.size();

        while (passes < 2 && moves++ < max_moves) {
            go::Move m = pick_playout_move(pos, rng_);
            if (!pos.move(m)) {
                continue;
            }

            if (m.is_pass()) {
                passes++;
            } else {
                passes = 0;
            }
        }
        double score = pos.evaluate(root_color_);
        return score > 0;
    }

    void MCTS::backprop(int node_id, int result) {
        int cur_id = node_id;
        while (cur_id != -1) {
            Node& cur = nodes_[cur_id];
            cur.visits++;
            if (result == 1) {
                cur.wins++;
            }
            cur_id = cur.parent;
        }
    }

}  // namespace mcts
