# Chess Engine

A chess engine and playable GUI written in C++17 with no chess library. It uses bitboards, implements the complete rules of chess (castling, en passant, promotion, check, checkmate, stalemate), and chooses moves with a negamax search with alpha-beta pruning. You play White with the mouse and the engine plays Black.


Move generation is checked against the published perft node counts from the starting position through depth 7 (3,195,901,860 positions).

## Features

- **Bitboard board:** eight 64-bit integers (two colours and six piece types), with squares numbered a8 = 0 to h1 = 63.
- **Full rules:** castling (rights tracking and attacked-square checks), en passant, promotion to queen, rook, bishop or knight, check, checkmate and stalemate.
- **Legal move generation:** pseudo-legal moves per piece, filtered by a king-safety test that uses bitboard attack detection.
- **Search:** negamax with alpha-beta pruning, MVV-LVA move ordering and mate-distance scoring.
- **Evaluation:** material, centralisation of knights and bishops, doubled pawns and king safety.
- **Perft:** a node counter and a per-move breakdown (`perftDivide`) for debugging move generation.
- **GUI (raylib):** click-to-move, legal-move highlighting, a promotion picker, last-move and check highlights, and an adjustable search depth.
- **Console version** for playing from the terminal.

## Build and run

You need a C++17 compiler. The GUI also needs [raylib](https://www.raylib.com/).

On Windows with MSYS2 (UCRT64 shell):

```
pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-raylib
g++ -std=c++17 -O2 gui.cpp -o chess_gui.exe -lraylib -lopengl32 -lgdi32 -lwinmm
./chess_gui.exe
```

Console version (no raylib needed):

```
g++ -std=c++17 -O2 main.cpp -o chess_console
./chess_console          # type moves like "e2 e4"
```

On Linux the GUI links with `-lraylib -lGL -lm -lpthread -ldl -lrt -lX11` (see the raylib documentation for your platform).

There is no build system. Each entry file includes the others as source (`#include "search.cpp"`), so compile only `gui.cpp` or `main.cpp`, never several files together. Always build with `-O2`; an unoptimised build is several times slower.

Keep the `pieces/` folder next to the executable. If the images are missing, the GUI draws letters in circles instead.

### GUI controls

| Input | Action |
|---|---|
| Click a piece, then a highlighted square | Move. Dots mark legal moves and red squares mark captures. |
| Click a pawn onto the last rank | A popup asks which piece to promote to. |
| Number keys `1` to `5` | Change the engine's search depth. |
| `R` | Start a new game. |

The window freezes while the engine is thinking, because the search runs on the same thread as the interface.

## How it works

| File | Contents |
|---|---|
| `board2.cpp` | Board state, move generation, legality, make-move, attack detection, perft |
| `eval.cpp` | The evaluation function |
| `search.cpp` | Negamax search with alpha-beta pruning and move ordering |
| `main.cpp` | Console game loop |
| `gui.cpp` | raylib interface |

**Board.** The position is held in `pieceBB[8]`: White, Black, Pawn, Rook, Knight, Bishop, Queen, King. A square's contents are found by testing one bit in the relevant boards, and set operations such as "all white pieces" are single ANDs and ORs. State beyond the pieces is a side-to-move flag, an en passant target square and four castling-right bits.

**Move generation.** Each piece type has its own function that returns a bitboard of target squares, and `getMoves(square)` dispatches to the right one. Sliding pieces walk each direction until they hit a piece. Knights and kings test their eight offsets with row and column bounds checks, which prevents wrap-around at the board edge. Pawns push only onto empty squares, double-push from their starting rank, and capture diagonally only onto enemy pieces (or onto the en passant square).

**Legality.** `legalMoves()` plays every pseudo-legal move on a copy of the board and keeps it only if the mover's own king is not attacked afterwards. `isSquareAttacked` looks outward from the target square for pawn, knight and king patterns and for sliding pieces along rays. That single test covers pins, a king stepping into check, and failing to answer a check. Castling uses the same function to check that the king's start square and the squares it crosses are not attacked.

**Special moves.**
- *Castling:* each side's rights are cleared when its king or rook moves, or when the rook is captured. The rook is relocated in `makeMove`.
- *En passant:* a double push stores the skipped square for exactly one move, and the capture removes the pawn behind the target square.
- *Promotion:* each promoting move is generated four times, once per piece. The console version promotes to a queen automatically.

**Search.** `findBestMove` runs a fixed-depth negamax. Moves are ordered so captures of valuable pieces by cheap pieces come first (MVV-LVA), then promotions, which makes alpha-beta cut off far more of the tree. A checkmate scores `-MATE + ply`, so a faster mate is preferred and the engine does not delay a win. A position with no legal moves and no check is scored as a draw.

**Evaluation.** The score is in centipawns from the side to move's point of view. It combines material (pawn 100, knight 300, bishop 300, rook 500, queen 900), bonuses for knights and bishops near the centre, a 50-point penalty for each doubled pawn, and a bonus for a king on a castled square.

## Correctness: perft

Perft counts every position reachable in exactly *n* moves. Matching the published numbers is strong evidence that move generation is right, because a single wrong move anywhere changes the total.

| Depth | Positions | Matches the published count | Time |
|---|---|---|---|
| 1 | 20 | yes | |
| 2 | 400 | yes | |
| 3 | 8,902 | yes | |
| 4 | 197,281 | yes | |
| 5 | 4,865,609 | yes | 0.3 s |
| 6 | 119,060,324 | yes | 7 s |
| 7 | 3,195,901,860 | yes | 191 s |

Timings are for one core of a Linux cloud VM built with `-O2`; other machines will differ. Reference values are from the Chess Programming Wiki.

To reproduce, put this in a `main`:

```cpp
board b;
for (int d = 1; d <= 6; d++)
    std::cout << "perft(" << d << ") = " << b.perft(d) << "\n";
```

`b.perftDivide(depth)` prints the count under each first move, which is the quickest way to find a bug by comparing against another engine.

**What this does not cover yet.** From the starting position, White's first castling happens on move 7, Black's on move 8 and a promotion much later. Castling and promotion are therefore only lightly exercised. The standard test positions with these moves available from the start (Kiwipete and the other positions on the Chess Programming Wiki perft page) have not been run. A FEN loader would make that possible and is the next correctness step.

## Performance

Time for the engine to choose one move, one core, `-O2`:

| Depth | Opening (Italian Game) | Middlegame (Open Sicilian) |
|---|---|---|
| 5 | 0.24 s | 0.88 s |
| 6 | 1.2 s | 5.6 s |
| 7 | 21 s | 62 s |

Each extra ply costs roughly 5 to 18 times more here. The search has no transposition table, no iterative deepening and copies the board for every move, so it spends most of its time on repeated work. Perft runs at about 17 million positions per second.

## Playing strength

Informal playtests, with the engine searching 7 plies: it won 38 of 40 games against Chess.com bots and players rated around 1500. This is not a formal rating. The opponents and time controls were not controlled, so treat it as a rough indication.

## Limitations and next steps

- **No draw rules:** threefold repetition, the 50-move rule and insufficient material are not implemented, so in a winning position the engine can shuffle pieces instead of finishing.
- **No quiescence search:** the search stops at a fixed depth even in the middle of a capture sequence, which causes the "horizon effect" and some tactical mistakes.
- **No transposition table or iterative deepening:** a Zobrist-hash table and time-managed deepening are the biggest speed and strength gains available.
- **Simple evaluation:** there are no piece-square tables, mobility, passed pawns or endgame knowledge.
- **Slower data structures than possible:** the board is copied for every move instead of using make and unmake, and sliding pieces use loops instead of magic bitboards.
- **No UCI support,** so the engine cannot yet be plugged into chess GUIs or tested against other engines automatically.
- **Single-threaded** and fixed-depth, with no opening book.

## Credits

- [raylib](https://www.raylib.com/) for the window and drawing.
- Piece images: the "cburnett" set by Colin M.L. Burnett (GPLv2+), taken from the lichess-org/lila repository and converted from SVG to PNG. See `pieces/CREDITS.txt`.
- Reference perft counts: [Chess Programming Wiki](https://www.chessprogramming.org/Perft_Results).
