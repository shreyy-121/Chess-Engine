// gui.cpp - play against your engine with the mouse (you are white, the engine is black)
//
// Build with THIS file as the active tab. It includes search.cpp, which already pulls in
// board2.cpp and eval.cpp, so only gui.cpp is compiled. Don't compile main.cpp at the same time.
//
// Piece images: put the "pieces" folder (wP.png, wN.png, ... bK.png) next to the .exe.
// If the images can't be found, the pieces are drawn as letters in circles instead.

#include <string>
#include <vector>
#include "raylib.h"
#include "search.cpp"

// ---------------------------------------------------------------- settings
constexpr int SQ  = 120;      // size of one square in pixels (make it smaller if the window is too tall)
constexpr int BAR = 90;      // height of the status bar under the board

const Color LIGHT_SQ = {238, 238, 210, 255};     // cream
const Color DARK_SQ  = {118, 150,  86, 255};     // green

// the four pieces a pawn can promote to, in the order they appear in the popup
const int PROMO_CHOICES[4] = {board::Queen, board::Rook, board::Bishop, board::Knight};

// ---------------------------------------------------------------- game state
struct Game {
    board b;
    std::vector<Move> moves;     // all legal moves in the current position
    int  selected    = -1;       // square of the piece you clicked, -1 = nothing selected
    int  lastFrom    = -1;       // last move played, for the highlight
    int  lastTo      = -1;
    bool promoting   = false;    // true while the popup asks which piece to promote to
    int  promoFrom   = -1;
    int  promoTo     = -1;
    bool over        = false;    // checkmate or stalemate
    std::string result;
    int  thinkFrames = 0;        // lets the window draw "thinking..." before the search blocks it

    Game() { refresh(); }

    // recompute the legal moves and check whether the game has ended
    void refresh() {
        moves = b.legalMoves();
        over = moves.empty();
        if (over) {
            if (b.isInCheck(b.sideToMove()))
                result = b.sideToMove() ? "Checkmate - black wins" : "Checkmate - you win!";
            else
                result = "Stalemate - draw";
        }
    }

    // m is taken by value on purpose: refresh() replaces the moves list
    void play(Move m) {
        b.makeMove(m.from, m.to, m.promo);
        lastFrom = m.from;
        lastTo   = m.to;
        selected = -1;
        refresh();
    }
};

// ---------------------------------------------------------------- input
void handleClick(Game& g) {
    if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) return;
    Vector2 m = GetMousePosition();

    // promotion popup: four cells in the middle of the board
    if (g.promoting) {
        int x0 = 2 * SQ;
        int y0 = 7 * SQ / 2;
        if (m.x >= x0 && m.x < x0 + 4 * SQ && m.y >= y0 && m.y < y0 + SQ) {
            int promo = PROMO_CHOICES[(int)((m.x - x0) / SQ)];
            for (const Move& mv : g.moves)
                if (mv.from == g.promoFrom && mv.to == g.promoTo && mv.promo == promo) {
                    g.play(mv);
                    break;
                }
        }
        g.promoting = false;          // a click anywhere else cancels the promotion
        g.selected  = -1;
        return;
    }

    if (m.x < 0 || m.x >= 8 * SQ || m.y < 0 || m.y >= 8 * SQ) return;   // clicked the status bar
    int sq = (int)(m.y / SQ) * 8 + (int)(m.x / SQ);

    if (sq == g.selected) { g.selected = -1; return; }                  // click again to deselect

    // a piece is selected: is the clicked square one of its legal destinations?
    if (g.selected != -1) {
        std::vector<Move> candidates;
        for (const Move& mv : g.moves)
            if (mv.from == g.selected && mv.to == sq) candidates.push_back(mv);

        if (candidates.size() == 1) { g.play(candidates[0]); return; }
        if (candidates.size() > 1) {                // 4 moves = a promotion, ask which piece
            g.promoting = true;
            g.promoFrom = g.selected;
            g.promoTo   = sq;
            return;
        }
    }

    // otherwise try to select one of your own pieces (any square that has a legal move from it)
    g.selected = -1;
    for (const Move& mv : g.moves)
        if (mv.from == sq) { g.selected = sq; break; }
}

void engineTurn(Game& g, int depth) {
    if (g.thinkFrames < 2) { g.thinkFrames++; return; }   // draw a couple of frames first
    g.thinkFrames = 0;
    Move m = findBestMove(g.b, depth);                    // the window is frozen while this runs
    g.play(m);
}

// ---------------------------------------------------------------- piece images
Texture2D pieceTex[2][8];        // [0 = white, 1 = black][board::Pawn .. board::King]
bool haveImages = false;         // false -> fall back to letters in circles

// looks for pieces/<name>.png next to the .exe first, then in the current folder
Texture2D loadPiece(const std::string& name) {
    std::string paths[2] = { std::string(GetApplicationDirectory()) + "pieces/" + name + ".png",
                             "pieces/" + name + ".png" };
    for (const std::string& p : paths) {
        Image img = LoadImage(p.c_str());
        if (img.data == nullptr) continue;
        ImageResize(&img, SQ, SQ);                    // scale to exactly one square
        Texture2D t = LoadTextureFromImage(img);
        UnloadImage(img);
        return t;
    }
    return Texture2D{};                               // id = 0 means "not loaded"
}

void loadPieces() {                                   // call after InitWindow
    const char* names = "PRNBQK";                     // same order as board::Pawn .. board::King
    haveImages = true;
    for (int c = 0; c < 2; c++)
        for (int i = 0; i < 6; i++) {
            Texture2D t = loadPiece(std::string(1, c == 0 ? 'w' : 'b') + names[i]);
            pieceTex[c][board::Pawn + i] = t;
            if (t.id == 0) haveImages = false;
        }
}

void unloadPieces() {
    for (int c = 0; c < 2; c++)
        for (int p = board::Pawn; p <= board::King; p++)
            if (pieceTex[c][p].id != 0) UnloadTexture(pieceTex[c][p]);
}

// ---------------------------------------------------------------- drawing
// type is board::Pawn .. board::King; (cx, cy) is the centre of the square
void drawPiece(int type, bool white, int cx, int cy) {
    if (haveImages) {
        DrawTexture(pieceTex[white ? 0 : 1][type], cx - SQ / 2, cy - SQ / 2, WHITE);
        return;
    }

    // fallback: a letter in a circle
    const char letters[] = {'P', 'R', 'N', 'B', 'Q', 'K'};
    char letter = letters[type - board::Pawn];
    Color fill = white ? Color{250, 250, 250, 255} : Color{35, 35, 35, 255};
    Color edge = white ? Color{20, 20, 20, 255}    : Color{235, 235, 235, 255};
    Color text = white ? Color{20, 20, 20, 255}    : Color{245, 245, 245, 255};

    int r = SQ * 38 / 100;
    DrawCircle(cx, cy, (float)r, fill);
    DrawCircleLines(cx, cy, (float)r, edge);

    char s[2] = {letter, 0};
    int size = SQ / 2;
    DrawText(s, cx - MeasureText(s, size) / 2, cy - size / 2, size, text);
}

void drawGame(const Game& g, int depth) {
    // all white pieces in one bitboard, so we know each piece's colour
    U64 whiteBB = g.b.getWhitePawns() | g.b.getWhiteKnights() | g.b.getWhiteBishop()
                | g.b.getWhiteRook()  | g.b.getWhiteQueen()   | g.b.getWhiteKing();

    // squares the selected piece can move to
    bool isTarget[64] = {};
    if (g.selected != -1)
        for (const Move& mv : g.moves)
            if (mv.from == g.selected) isTarget[mv.to] = true;

    // red highlight on the king of the side to move if it is in check
    bool stm = g.b.sideToMove();
    bool inCheck = !g.over && g.b.isInCheck(stm);
    int checkSq = -1;
    if (g.b.isInCheck(stm)) checkSq = __builtin_ctzll(stm ? g.b.getWhiteKing() : g.b.getBlackKing());

    // squares, highlights and coordinates
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            int sq = row * 8 + col;
            int x = col * SQ, y = row * SQ;
            bool light = (row + col) % 2 == 0;

            DrawRectangle(x, y, SQ, SQ, light ? LIGHT_SQ : DARK_SQ);
            if (sq == g.lastFrom || sq == g.lastTo) DrawRectangle(x, y, SQ, SQ, Color{246, 246, 105, 110});
            if (sq == g.selected)                   DrawRectangle(x, y, SQ, SQ, Color{255, 255, 60, 170});
            if (sq == checkSq)                      DrawRectangle(x, y, SQ, SQ, Color{235, 60, 60, 170});

            if (isTarget[sq]) {
                if (g.b.pieceTypeAt(sq) != -1)
                    DrawRectangle(x, y, SQ, SQ, Color{200, 50, 50, 110});             // capture
                else
                    DrawCircle(x + SQ / 2, y + SQ / 2, SQ / 7.0f, Color{20, 20, 20, 70});   // quiet move
            }

            Color label = light ? DARK_SQ : LIGHT_SQ;
            if (col == 0) DrawText(std::to_string(8 - row).c_str(), x + 4, y + 3, 14, label);
            if (row == 7) {
                char f[2] = {(char)('a' + col), 0};
                DrawText(f, x + SQ - 12, y + SQ - 18, 14, label);
            }
        }
    }

    // pieces
    for (int sq = 0; sq < 64; sq++) {
        int p = g.b.pieceTypeAt(sq);
        if (p == -1) continue;
        int cx = (sq % 8) * SQ + SQ / 2;
        int cy = (sq / 8) * SQ + SQ / 2;
        drawPiece(p, (whiteBB >> sq) & 1, cx, cy);
    }

    // promotion popup
    if (g.promoting) {
        DrawRectangle(0, 0, 8 * SQ, 8 * SQ, Color{0, 0, 0, 150});
        int x0 = 2 * SQ, y0 = 7 * SQ / 2;
        DrawRectangle(x0 - 6, y0 - 6, 4 * SQ + 12, SQ + 12, Color{40, 40, 46, 255});
        for (int i = 0; i < 4; i++) {
            DrawRectangle(x0 + i * SQ, y0, SQ, SQ, i % 2 == 0 ? LIGHT_SQ : DARK_SQ);
            drawPiece(PROMO_CHOICES[i], true, x0 + i * SQ + SQ / 2, y0 + SQ / 2);
        }
    }

    // game over banner
    if (g.over) {
        int h = 90;
        DrawRectangle(0, 4 * SQ - h / 2, 8 * SQ, h, Color{0, 0, 0, 200});
        int size = 36;
        DrawText(g.result.c_str(), (8 * SQ - MeasureText(g.result.c_str(), size)) / 2, 4 * SQ - size / 2, size, RAYWHITE);
    }

    // status bar
    DrawRectangle(0, 8 * SQ, 8 * SQ, BAR, Color{30, 30, 36, 255});
    std::string msg;
    if (g.over)           msg = g.result + "  -  press R for a new game";
    else if (g.promoting) msg = "Choose a piece to promote to (click elsewhere to cancel)";
    else if (!stm)        msg = "Engine is thinking...";
    else                  msg = std::string(inCheck ? "Check!  " : "") + "Your move (white)";
    DrawText(msg.c_str(), 12, 8 * SQ + 6, 20, RAYWHITE);

    std::string hint = "Engine depth " + std::to_string(depth) + " (press 1-7 to change)    R = new game";
    DrawText(hint.c_str(), 12, 8 * SQ + 30, 14, LIGHTGRAY);
}

// ---------------------------------------------------------------- main
int main() {
    InitWindow(8 * SQ, 8 * SQ + BAR, "Chess engine");
    SetTargetFPS(60);
    loadPieces();

    Game g;
    int depth = 4;                                    // how many plies the engine looks ahead

    while (!WindowShouldClose()) {                    // closes with the X button or Esc
        // keys
        if (IsKeyPressed(KEY_R)) g = Game();          // new game
        for (int k = 1; k <= 7; k++)
            if (IsKeyPressed(KEY_ZERO + k)) depth = k;

        // update
        if (!g.over) {
            if (g.b.sideToMove()) handleClick(g);     // white to move = you
            else                  engineTurn(g, depth);
        }

        // draw
        BeginDrawing();
        ClearBackground(Color{30, 30, 36, 255});
        drawGame(g, depth);
        EndDrawing();
    }

    unloadPieces();
    CloseWindow();
    return 0;
}
