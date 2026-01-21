#include "mcts/playout.h"

#include <random>
#include <algorithm>

namespace mcts {

    void gen_playout_moves_ko(go::Board& pos, std::vector<go::Move>& moves) {
        moves.clear();
        if (pos.ko_point() == -1) {
            return;
        }
        int age = pos.ply_count() - pos.ko_age();
        if (age > 0 && age <= 4) {
            moves.push_back(go::Move(pos.ko_point()));
        }
    }

    void gen_playout_moves_capture(go::Board& pos, std::vector<go::Move>& moves) {
        moves.clear();
        auto [neigh, n] = pos.last_moves_neigh();
        for (int i = 0; i < n; i++) {
            go::Move m(neigh[i]);
            if (pos.is_capture(m)) {
                moves.push_back(m);
            }
        }
    }

    go::Move play_heuristic_move(go::Board& pos, RNG& rng) {
        std::vector<go::Move> moves;
        std::uniform_real_distribution<double> dist(0.0, 1.0);

        auto random_move = [&]() {
            std::shuffle(moves.begin(), moves.end(), rng);
            for (go::Move& m : moves) {
                if (pos.move(m)) {
                    return m;
                }
            }
            return go::Move::Pass();
        };

        auto m = go::Move::Pass();
        if (dist(rng) < 0.4) {
            gen_playout_moves_ko(pos, moves);
            m = random_move();
            if (!m.is_pass()) {
                return m;
            }
        }
        if (dist(rng) < 0.3) {
            gen_playout_moves_capture(pos, moves);
            m = random_move();
            if (!m.is_pass()) {
                return m;
            }
        }

        pos.gen_pseudo_legal_moves(moves);
        m = random_move();
        if (m.is_pass()) {
            pos.move(m);
        }
        return m;
    }

}
