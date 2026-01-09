#include "mcts/playout.h"

#include <random>

namespace mcts {

    std::vector<go::Move> gen_playout_moves(go::Board& pos) {
        return pos.legal_moves();
    }

    go::Move pick_playout_move(go::Board& pos, RNG& rng) {
        auto moves = gen_playout_moves(pos);
        if (moves.empty()) {
            return go::Move::Pass();
        }
        std::uniform_int_distribution<int> dist(0, (int)moves.size() - 1);
        return moves[dist(rng)];
    }

}
