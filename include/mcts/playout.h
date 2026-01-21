#pragma once

#include <vector>
#include <random>

#include "go/types.h"
#include "go/board.h"

namespace mcts {

    using RNG = std::mt19937_64;

    void gen_playout_moves_ko(go::Board& pos, std::vector<go::Move>& moves);

    void gen_playout_moves_capture(go::Board& pos, std::vector<go::Move>& moves);

    go::Move play_heuristic_move(go::Board& pos, RNG& rng);

}  // namespace mcts
