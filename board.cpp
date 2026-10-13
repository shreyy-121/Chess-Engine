#pragma once
#include <iostream>
#include <cstdint>
#include <cstdlib> 
#include <vector>
#include <string>
#include <cctype>


using U64 = uint64_t;  //return type



enum Square {
    a8, b8, c8, d8, e8, f8, g8, h8,
    a7, b7, c7, d7, e7, f7, g7, h7,
    a6, b6, c6, d6, e6, f6, g6, h6,
    a5, b5, c5, d5, e5, f5, g5, h5,
    a4, b4, c4, d4, e4, f4, g4, h4,
    a3, b3, c3, d3, e3, f3, g3, h3,
    a2, b2, c2, d2, e2, f2, g2, h2,
    a1, b1, c1, d1, e1, f1, g1, h1
};



//perft->
std::string squareName(int sq) {                       // 0 -> "a8", 63 -> "h1"
    return std::string{char('a' + sq % 8), char('8' - sq / 8)};
}




struct Move { Square from, to; int promo = 0; };   // promo: 0 = none, else Queen/Rook/Bishop/Knight




class board{

U64 pieceBB[8]{};   //an array containing 8 U64 bitboards. the {} initialises all the 8 bitboards to 0



bool whiteToMove = true;   // turn tracking
int  epSquare = -1;        // square a pawn can capture onto en passant, -1 = none
int  castleRights = 15;
    
public:
    enum enumPiece{  //this is only the name creation of the bitboards, we dont have any bitboards at hand. for them we use the U64 which we defined above
        White,
        Black,
        Pawn,
        Rook,
        Knight,
        Bishop,
        Queen,
        King
    };
    enum Castle { WK = 1, WQ = 2, BK = 4, BQ = 8 };   // white kingside/queenside, black kingside/queenside

    bool sideToMove() const { return whiteToMove; }

    //this creates seperate bitboards for all the pieces:

   U64 getWhitePawns() const {return pieceBB[Pawn] & pieceBB[White];}
   U64 getWhiteKnights() const {return pieceBB[Knight] & pieceBB[White];}

   U64 getWhiteBishop() const {return pieceBB[Bishop] & pieceBB[White];}
   U64 getWhiteRook() const {return pieceBB[Rook] & pieceBB[White];}
   U64 getWhiteQueen() const {return pieceBB[Queen] & pieceBB[White];}
   U64 getWhiteKing() const {return pieceBB[King] & pieceBB[White];}

   

   U64 getBlackPawns() const {return pieceBB[Pawn] & pieceBB[Black];}
   U64 getBlackKnights() const {return pieceBB[Knight] & pieceBB[Black];}

   U64 getBlackBishop() const {return pieceBB[Bishop] & pieceBB[Black];}
   U64 getBlackRook() const {return pieceBB[Rook] & pieceBB[Black];}
   U64 getBlackQueen() const {return pieceBB[Queen] & pieceBB[Black];}
   U64 getBlackKing() const {return pieceBB[King] & pieceBB[Black];}


    board(){
        pieceBB[Pawn] |= (0xFFULL << a2);  // 0x means hexadecimal F = 1111, 0xFF = 8 bits set to 1, ULL: unsigned long long int
        pieceBB[Pawn] |= 0xFFULL << a7;

        pieceBB[White] |= 0xFFFFULL << a2;
        pieceBB[Black] |= 0xFFFFULL << a8;

        pieceBB[Knight]  |= (1ULL << b1) | (1ULL << g1) | (1ULL << b8) | (1ULL << g8);
        pieceBB[Rook]  |= (1ULL << a1) | (1ULL << h1) | (1ULL << a8) | (1ULL << h8);
        pieceBB[Bishop]  |= (1ULL << c1) | (1ULL << f1) | (1ULL << c8) | (1ULL << f8);

        pieceBB[Queen] |= (1ULL << d1) | (1ULL << d8);
        pieceBB[King] |= (1ULL << e1) | (1ULL << e8);


    }


    U64 getOccupied() const{return pieceBB[White] | pieceBB[Black];}   //fn to check if a square is occupied or not, gives a bitboard with 1's at every square that a piece is on
    U64 getEmpty() const{ return ~getOccupied();}   //bitwise not
    bool isOccupied(Square sq) const {return (getOccupied() & (1ULL<< sq)) != 0;}   //AND operation on a particular square and the get occupied fn
   

    U64 knightMoves(Square sq) const {
        int row = sq/8;
        int col = sq % 8;

        const int dr[8] = {-2, -2, -1, -1,  1,  1,  2,  2};
        const int dc[8] = {-1,  1, -2,  2, -2,  2, -1,  1};
    
        U64 targets =0;
        
        for (int i = 0; i<8;i++){
            int r = row + dr[i];
            int c = col + dc[i];

            if (r>= 0 && r<8 && c>=0 && c<8){
                targets |= 1ULL << (r*8+c);
            }

        }
        return targets; 
    } 


    U64 slide(Square sq, int dr, int dc) const{

        U64 bit = 1ULL << (sq);
        bool isWhite= pieceBB[White] & bit;

        U64 own = isWhite ? pieceBB[White] : pieceBB[Black];        //condition ? expression_if_true : expression_if_false;
        U64 enemy = isWhite ? pieceBB[Black] : pieceBB[White];

        int r = sq / 8 + dr;
        int c = sq % 8 + dc;

        U64 result = 0;

        while(r>= 0 && r< 8 && c >=0 && c<8){

            U64 target = 1ULL << (r*8 + c);
            if(own & target){break;}
            result |= target;
            if(enemy & target){break;}

            r+= dr;
            c+= dc;

        }return result;
    }

    U64 rookMoves(Square sq) const {
    return slide(sq, -1, 0) | slide(sq, 1, 0) | slide(sq, 0, -1) | slide(sq, 0, 1);
    }

    U64 bishopMoves(Square sq) const {
    return slide(sq, -1, -1) | slide(sq, -1, 1) | slide(sq, 1, -1) | slide(sq, 1, 1);
    }

    U64 queenMoves(Square sq) const { return rookMoves(sq) | bishopMoves(sq); }


    U64 kingMoves(Square sq) const{
        int row = sq / 8;
        int col = sq % 8;

        U64 bit = 1ULL<< (sq);
        bool isWhite = pieceBB[White] & bit;

        U64 own = isWhite ? pieceBB[White] : pieceBB[Black];
        U64 targets = 0;

        for(int dr = -1; dr<=1; dr++){
            for (int dc = -1; dc<=1; dc++){
                if (dr == 0 && dc ==0){continue;}

                int r = row+ dr;
                int c = col + dc;

                if(r>=0 && r<8 && c>=0 && c<8){
                    U64 target = 1ULL<< (r*8+c);
                    if (!(own&target)){ targets |= target;}
                }
            }
        }
        return targets;


    }

    U64 pawnmoves(Square sq) const{

        int row = sq / 8;
        int col = sq % 8;

        U64 bit = 1ULL << sq;

        bool isWhite = pieceBB[White] & bit;

        U64 enemy = isWhite ? pieceBB[Black] : pieceBB[White];
        U64 occupied = pieceBB[White] | pieceBB[Black];

        int dr = isWhite? -1 : 1;
        int startRow = isWhite? 6: 1;

        int r = row+ dr;
        if (r<0 || r>7){return 0;}

        U64 targets = 0;

        U64 one = 1ULL << (r*8+col);

        if(!(occupied & one)){
            targets|= one;



            if (row== startRow)
            {
                U64 two = 1ULL<<((r+dr) * 8 + col);
                if (!(occupied & two)){targets |= two;}
            }
            
        }
        
        //capturing->

        int epRow = isWhite ? 3 : 4;      // white captures en passant from rank 5, black from rank 4

        for (int dc = -1; dc <= 1; dc += 2) {
            int c = col + dc;
            if (c >= 0 && c < 8) {
                int idx = r * 8 + c;
                U64 target = 1ULL << idx;
                if (enemy & target)                       targets |= target;   // normal capture
                else if (idx == epSquare && row == epRow) targets |= target;   // en passant
            }   
        }

    return targets;
}


// the dispatcher: finds which piece is on sq and returns where it can go ie all the possible places a piece can go on the baord
U64 getMoves(Square sq) const {
    U64 bit = 1ULL << sq;
    U64 own = (pieceBB[White] & bit) ? pieceBB[White] : pieceBB[Black];
    U64 targets = 0;

    if      (pieceBB[Knight] & bit) targets = knightMoves(sq);
    else if (pieceBB[King] & bit) targets = kingMoves(sq) | castlingMoves(sq);    
    else if (pieceBB[Rook]   & bit) targets = rookMoves(sq);
    else if (pieceBB[Bishop] & bit) targets = bishopMoves(sq);
    else if (pieceBB[Queen]  & bit) targets = queenMoves(sq);
    else if (pieceBB[Pawn]   & bit) targets = pawnmoves(sq);

    return targets & ~own;    // KnightLogic doesn't filter own pieces, so it's done here
}


// applies a move that has already been checked ie actually makes a legal/valid move 
void makeMove(Square from, Square to, int promo = 0) {
    U64 fromBB = 1ULL << from;
    U64 toBB   = 1ULL << to;

    bool isWhite = pieceBB[White] & fromBB;
    bool isPawn  = pieceBB[Pawn]  & fromBB;
    bool isKing  = pieceBB[King]  & fromBB;

    // en passant: the captured pawn is behind the destination square, not on it
    if (isPawn && to == epSquare) {
        int capSq = isWhite ? to + 8 : to - 8;
        for (int i = 0; i < 8; i++) pieceBB[i] &= ~(1ULL << capSq);
    }

    // normal capture
    for (int i = 0; i < 8; i++) pieceBB[i] &= ~toBB;

    // movement
    for (int i = 0; i < 8; i++)
        if (pieceBB[i] & fromBB) pieceBB[i] ^= (fromBB | toBB);



    // castling-> the king moved two squares, so move the rook moves too
    if (isKing && std::abs(to - from) == 2) {
        int rookFrom = (to > from) ? from + 3 : from - 4;   // h-file or a-file rook
        int rookTo   = (to > from) ? from + 1 : from - 1;   // f-file or d-file
        U64 rookMove = (1ULL << rookFrom) | (1ULL << rookTo);
        pieceBB[Rook] ^= rookMove;
        pieceBB[isWhite ? White : Black] ^= rookMove;
    }   

    // promotion(qeueen by default)
    if (isPawn && (to < 8 || to >= 56)) {
        if (!promo) promo = Queen;
        pieceBB[Pawn]  &= ~toBB;
        pieceBB[promo] |= toBB;
    }

    // after a double push, remember the square that was skipped; otherwise clear it
    epSquare = (isPawn && std::abs(to - from) == 16) ? (from + to) / 2 : -1;

    // castling rights: anything touching a king or rook home square removes them
    if (from == e1 || to == e1) castleRights &= ~(WK | WQ);
    if (from == h1 || to == h1) castleRights &= ~WK;
    if (from == a1 || to == a1) castleRights &= ~WQ;
    if (from == e8 || to == e8) castleRights &= ~(BK | BQ);
    if (from == h8 || to == h8) castleRights &= ~BK;
    if (from == a8 || to == a8) castleRights &= ~BQ;


    whiteToMove = !isWhite;
}











// squares a pawn of this colour on sq attacks, whether or not anything is on them
U64 pawnAttacks(Square sq, bool white) const {
    int row = sq / 8, col = sq % 8;
    int r = row + (white ? -1 : 1);
    if (r < 0 || r > 7) return 0;

    U64 targets = 0;
    if (col > 0) targets |= 1ULL << (r * 8 + col - 1);
    if (col < 7) targets |= 1ULL << (r * 8 + col + 1);
    return targets;
}

// every square along a ray, up to and including the first piece of EITHER colour
U64 ray(Square sq, int dr, int dc) const {
    U64 occupied = getOccupied();
    int r = sq / 8 + dr, c = sq % 8 + dc;
    U64 result = 0;
    while (r >= 0 && r < 8 && c >= 0 && c < 8) {
        U64 target = 1ULL << (r * 8 + c);
        result |= target;
        if (occupied & target) break;
        r += dr;
        c += dc;
    }
    return result;
}



// can any piece of the given side capture on sq? (sq can be empty)
bool isSquareAttacked(Square sq, bool byWhite) const {
    U64 by = byWhite ? pieceBB[White] : pieceBB[Black];

    // pawns: a white pawn attacks sq if a black pawn standing on sq would attack that pawn
    if (pawnAttacks(sq, !byWhite) & pieceBB[Pawn] & by) return true;

    // knights: knightMOves dont depend on color, so it everywhere
    if (knightMoves(sq) & pieceBB[Knight] & by) return true;

    // king: check the 8 neighbours
    int row = sq / 8, col = sq % 8;
    for (int dr = -1; dr <= 1; dr++)
        for (int dc = -1; dc <= 1; dc++) {
            int r = row + dr, c = col + dc;
            if ((dr || dc) && r >= 0 && r < 8 && c >= 0 && c < 8)
                if (pieceBB[King] & by & (1ULL << (r * 8 + c))) return true;
        }

    // sliders: the first piece hit along each ray must be an enemy rook/bishop/queen
    U64 rookLike   = (pieceBB[Rook]   | pieceBB[Queen]) & by;
    U64 bishopLike = (pieceBB[Bishop] | pieceBB[Queen]) & by;

    if ((ray(sq,-1,0) | ray(sq,1,0) | ray(sq,0,-1) | ray(sq,0,1)) & rookLike)     return true;
    if ((ray(sq,-1,-1) | ray(sq,-1,1) | ray(sq,1,-1) | ray(sq,1,1)) & bishopLike) return true;

    return false;
}

bool isInCheck(bool white) const {
    U64 king = white ? getWhiteKing() : getBlackKing();
    return isSquareAttacked((Square)__builtin_ctzll(king), !white);
}



// all fully legal moves for the side to move
std::vector<Move> legalMoves() const {
    std::vector<Move> moves;
    bool white = whiteToMove;
    U64 pieces = white ? pieceBB[White] : pieceBB[Black];

    while (pieces) {                                   // each of my pieces
        int from = __builtin_ctzll(pieces);
        pieces &= pieces - 1;

        bool isPawn = pieceBB[Pawn] & (1ULL << from);

        U64 targets = getMoves((Square)from);          // pseudo-legal targets
        while (targets) {                              // each target square
            int to = __builtin_ctzll(targets);
            targets &= targets - 1;

            bool promotes = isPawn && (to < 8 || to >= 56);
            const int promos[4] = {Queen, Rook, Bishop, Knight};
            int n = promotes ? 4 : 1;

            for (int k = 0; k < n; k++) {
                int promo = promotes ? promos[k] : 0;

                board copy = *this;                    // play the move on a copy
                copy.makeMove((Square)from, (Square)to, promo);
                if (!copy.isInCheck(white))            // keep it only if my king is safe
                    moves.push_back({(Square)from, (Square)to, promo});
            }
        }
    }
    return moves;
}

bool isCheckmate() const { return legalMoves().empty() &&  isInCheck(whiteToMove); }
bool isStalemate() const { return legalMoves().empty() && !isInCheck(whiteToMove); }









// squares the king on sq can reach by castling (g1/c1 for white, g8/c8 for black)
U64 castlingMoves(Square sq) const {
    bool white = pieceBB[White] & (1ULL << sq);
    if (sq != (white ? e1 : e8)) return 0;          // king isn't on its home square
    if (isSquareAttacked(sq, !white)) return 0;     // can't castle out of check

    U64 occupied = getOccupied();
    U64 targets = 0;

    if (white) {
        // kingside: f1, g1 empty; f1, g1 not attacked
        if ((castleRights & WK)
            && !(occupied & ((1ULL << f1) | (1ULL << g1)))
            && !isSquareAttacked(f1, false) && !isSquareAttacked(g1, false))
            targets |= 1ULL << g1;

        // queenside: b1, c1, d1 empty; c1, d1 not attacked (b1 may be attacked)
        if ((castleRights & WQ)
            && !(occupied & ((1ULL << b1) | (1ULL << c1) | (1ULL << d1)))
            && !isSquareAttacked(d1, false) && !isSquareAttacked(c1, false))
            targets |= 1ULL << c1;
    } else {
        if ((castleRights & BK)
            && !(occupied & ((1ULL << f8) | (1ULL << g8)))
            && !isSquareAttacked(f8, true) && !isSquareAttacked(g8, true))
            targets |= 1ULL << g8;

        if ((castleRights & BQ)
            && !(occupied & ((1ULL << b8) | (1ULL << c8) | (1ULL << d8)))
            && !isSquareAttacked(d8, true) && !isSquareAttacked(c8, true))
            targets |= 1ULL << c8;
    }
    return targets;
}









//perft->


// counts the positions reachable in exactly `depth` half-moves
U64 perft(int depth) const {
    if (depth == 0) return 1;

    std::vector<Move> moves = legalMoves();
    if (depth == 1) return moves.size();      // no need to play the last ply, just count it

    U64 nodes = 0;
    for (const Move& m : moves) {
        board copy = *this;
        copy.makeMove(m.from, m.to, m.promo);
        nodes += copy.perft(depth - 1);
    }
    return nodes;
}

// same, but prints the count under each first move (the main debugging tool)
void perftDivide(int depth) const {
    U64 total = 0;
    for (const Move& m : legalMoves()) {
        board copy = *this;
        copy.makeMove(m.from, m.to, m.promo);
        U64 n = copy.perft(depth - 1);

        std::cout << squareName(m.from) << squareName(m.to);
        if (m.promo) std::cout << "..prnbqk"[m.promo];   // piece enum index -> letter
        std::cout << ": " << n << '\n';
        total += n;
    }
    std::cout << "total: " << total << '\n';
}





















// which piece type is on sq (Pawn..King), or -1 if empty
int pieceTypeAt(int sq) const {
    U64 bit = 1ULL << sq;
    for (int p = Pawn; p <= King; p++)
        if (pieceBB[p] & bit) return p;
    return -1;
}

void print() const {
    const char symbols[] = {'P', 'R', 'N', 'B', 'Q', 'K'};   // same order as Pawn..King
    for (int rank = 0; rank < 8; rank++) {
        std::cout << (8 - rank) << "  ";
        for (int file = 0; file < 8; file++) {
            int sq = rank * 8 + file;
            int p = pieceTypeAt(sq);
            char c = '.';
            if (p != -1) {
                c = symbols[p - Pawn];
                if (pieceBB[Black] & (1ULL << sq)) c = std::tolower(c);   // black = lowercase
            }
            std::cout << c << ' ';
        }
        std::cout << '\n';
    }
    std::cout << "\n   a b c d e f g h\n";
}


};


