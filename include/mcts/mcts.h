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
        std::vector<Node> nodes_;

        RNG rng_;

        int select_child(int parent_id);

        int descend(go::Board& pos);
        void expand(int node_id, go::Board& pos);
        double playout(go::Board& pos);
        void backprop(int node_id, double score);
    };

}  // namespace mcts
