#pragma once
#include <iostream>
#include <cstdint>
#include <cstdlib> 
#include <vector>
#include <string>
#include "board2.cpp"

//piece values->
//pawn = 1
//knight = 3
//bishop = 3
//rook = 5
//queen = 9



int evaluate(const board& b) {
    int score = 0;                       // positive = good for white

    for (int team = 1; team >= -1; team -= 2) {      // team = +1 (white), then -1 (black)
        bool white = (team == 1);

        U64 pawns   = white ? b.getWhitePawns()   : b.getBlackPawns();
        U64 knights = white ? b.getWhiteKnights() : b.getBlackKnights();
        U64 bishops = white ? b.getWhiteBishop()  : b.getBlackBishop();
        U64 rooks   = white ? b.getWhiteRook()    : b.getBlackRook();
        U64 queens  = white ? b.getWhiteQueen()   : b.getBlackQueen();
        U64 kings   = white ? b.getWhiteKing()    : b.getBlackKing();

        // material: popcount counts the set bits, so no loop is needed
        score += team * (100 * __builtin_popcountll(pawns)
                       + 300 * __builtin_popcountll(knights)
                       + 300 * __builtin_popcountll(bishops)
                       + 500 * __builtin_popcountll(rooks)
                       + 900 * __builtin_popcountll(queens));

        // doubled pawns: a friendly pawn directly behind this one
        U64 p = pawns;
        while (p) {
            int sq = __builtin_ctzll(p);
            p &= p - 1;
            int ri = sq / 8;
            bool hasBehind = white ? (ri + 1 <= 7) : (ri - 1 >= 0);
            int behind = white ? sq + 8 : sq - 8;
            if (hasBehind && (pawns & (1ULL << behind))) score -= team * 50;
        }

        // knights
        U64 n = knights;
        while (n) {
            int sq = __builtin_ctzll(n);
            n &= n - 1;
            int ri = sq / 8, ci = sq % 8;
            int bonus = 0;
            if ((ri == 3 || ri == 4) && (ci == 3 || ci == 4)) bonus += 50;   // center
            if ((ri == 2 || ri == 5) && (ci == 2 || ci == 5)) bonus += 25;   // semi-center
            if ((ri == 1 || ri == 6) && (ci == 1 || ci == 6)) bonus -= 25;   // outer
            if ((ri == 0 || ri == 7) && (ci == 0 || ci == 7)) bonus -= 50;   // corners
            score += team * bonus;
        }

        // bishops
        U64 bi = bishops;
        while (bi) {
            int sq = __builtin_ctzll(bi);
            bi &= bi - 1;
            int ri = sq / 8, ci = sq % 8;
            if ((ri == 3 || ri == 4) && (ci == 3 || ci == 4)) score += team * 50;
            if ((ri == 2 || ri == 5) && (ci == 2 || ci == 5)) score += team * 25;
        }

        // king safety (castled positions)
        if (kings) {
            int sq = __builtin_ctzll(kings);
            int ri = sq / 8, ci = sq % 8;
            int backRow = white ? 7 : 0;
            if (ri == backRow) {
                if (ci == 2 || ci == 6)      score += team * 75;   // safe
                else if (ci == 1 || ci == 7) score += team * 25;   // semi-safe
            }
        }
    }

    return b.sideToMove() ? score : -score;   // from the side to move's point of view
}