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
        go::Color just_played;

        int v = 0;
        int w = 0;
        int av = 0;
        int aw = 0;
        int pv = 10;
        int pw = 5;
    };

}  // namespace mcts
