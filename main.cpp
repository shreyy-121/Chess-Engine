#include <iostream>
#include <string>
#include "board2.cpp"
#include "eval.cpp"
#include "search.cpp"

int parseSquare(const std::string& s) {
    if (s.size() != 2) return -1;
    int file = s[0] - 'a', rank = s[1] - '1';
    if (file < 0 || file > 7 || rank < 0 || rank > 7) return -1;
    return (7 - rank) * 8 + file;
}

int main() {
    board b;
    const int DEPTH = 4;
    std::string from, to;

    while (true) {
        b.print();
        auto moves = b.legalMoves();

        if (moves.empty()) {
            if (b.isInCheck(b.sideToMove()))
                std::cout << (b.sideToMove() ? "checkmate, black wins\n" : "checkmate, white wins\n");
            else
                std::cout << "stalemate\n";
            break;
        }

        if (b.sideToMove()) {                                   // you are white
            std::cout << "your move (e.g. e2 e4, q to quit): ";
            if (!(std::cin >> from) || from == "q") break;
            std::cin >> to;

            int f = parseSquare(from), t = parseSquare(to);
            bool played = false;
            for (const Move& m : moves)
                if ((int)m.from == f && (int)m.to == t) {        // first match = queen when promoting
                    b.makeMove(m.from, m.to, m.promo);
                    played = true;
                    break;
                }
            if (!played) std::cout << "illegal move\n";
        } else {                                                // engine is black
            Move m = findBestMove(b, DEPTH);
            std::cout << "engine plays " << squareName(m.from) << squareName(m.to) << '\n';
            b.makeMove(m.from, m.to, m.promo);
        }
    }
}