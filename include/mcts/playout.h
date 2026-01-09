#pragma once

#include <vector>
#include <random>

#include "go/types.h"
#include "go/board.h"

namespace mcts {

    using RNG = std::mt19937_64;

    std::vector<go::Move> gen_playout_moves(go::Board& pos);

    go::Move pick_playout_move(go::Board& pos, RNG& rng);

}  // namespace mcts
