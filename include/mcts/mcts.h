#pragma once

#include <array>
#include <vector>
#include <string>
#include <limits>
#include <random>

#include "go/types.h"
#include "go/board.h"
#include "mcts/node.h"
#include "mcts/playout.h"

namespace mcts {

    class MCTS {
    public:
        explicit MCTS(uint64_t seed = std::random_device{}()) : rng_(seed) {}

        go::Move search(go::Board root, int iters);

    private:
        go::Color root_color_ = go::Color::Black;
        std::vector<Node> nodes_;

        RNG rng_;

        int select_child(int parent_id);

        int descend(go::Board& pos, std::vector<go::Undo>& undos);
        void expand(int node_id, go::Board& pos);
        int playout(go::Board& pos, std::vector<go::Undo>& undos);
        void backprop(int node_id, int result);
        void rollback(go::Board& pos, std::vector<go::Undo>& undos);
    };

}  // namespace mcts
