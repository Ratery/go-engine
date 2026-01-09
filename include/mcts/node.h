#pragma once

#include <array>
#include <vector>
#include <string>

#include "go/types.h"

namespace mcts {

    struct Node {
        go::Move move;
        int parent = -1;
        std::vector<int> children;

        int visits = 0, wins = 0;
    };

}  // namespace mcts
