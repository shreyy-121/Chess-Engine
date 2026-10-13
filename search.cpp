#pragma once
#include <iostream>
#include <cstdint>
#include <cstdlib> 
#include <vector>
#include <string>
#include "board2.cpp"
#include "eval.cpp"
#include <utility>
#include <algorithm>


constexpr int MATE = 100000;    // score for checkmate
constexpr int INF  = 1000000;   // bigger than any real score

// move ordering: try captures (valuable victim, cheap attacker) and promotions first
inline int moveScore(const board& b, const Move& m) {
    static const int value[8] = {0, 0, 100, 500, 300, 300, 900, 10000};   // indexed by board's piece enum
    int score = 0;
    int victim = b.pieceTypeAt(m.to);
    if (victim != -1)
        score += 10000 + 10 * value[victim] - value[b.pieceTypeAt(m.from)];
    if (m.promo)
        score += value[m.promo];
    return score;
}

inline void orderMoves(const board& b, std::vector<Move>& moves) {
    std::vector<std::pair<int, Move>> scored;
    for (const Move& m : moves) scored.push_back({moveScore(b, m), m});
    std::stable_sort(scored.begin(), scored.end(),
                     [](const auto& x, const auto& y) { return x.first > y.first; });
    for (size_t i = 0; i < moves.size(); i++) moves[i] = scored[i].second;
}

// score of the position for the side to move, searching depth more plies
inline int negamax(const board& b, int depth, int alpha, int beta, int ply) {
    std::vector<Move> moves = b.legalMoves();

    if (moves.empty()) {
        if (b.isInCheck(b.sideToMove())) return -MATE + ply;   // checkmated: sooner mate = worse
        return 0;                                              // stalemate
    }
    if (depth == 0) return evaluate(b);

    orderMoves(b, moves);

    for (const Move& m : moves) {
        board copy = b;
        copy.makeMove(m.from, m.to, m.promo);

        int score = -negamax(copy, depth - 1, -beta, -alpha, ply + 1);

        if (score >= beta) return beta;      
        if (score > alpha) alpha = score;    
    }
    return alpha;
}

// returns the best move; call it only when legalMoves() is not empty
inline Move findBestMove(const board& b, int depth) {
    std::vector<Move> moves = b.legalMoves();
    orderMoves(b, moves);

    Move best = moves[0];
    int alpha = -INF, beta = INF;

    for (const Move& m : moves) {
        board copy = b;
        copy.makeMove(m.from, m.to, m.promo);

        int score = -negamax(copy, depth - 1, -beta, -alpha, 1);
        if (score > alpha) {
            alpha = score;
            best = m;
        }
    }
    return best;
}
