#include <SFML/Graphics.hpp>
#include <cmath>
#include <cstdio>

// ============================================================
//  SECTION 1 - ENUMS
// ============================================================

enum Color        { WHITE, BLACK };
enum PieceType    { PAWN, ROOK, KNIGHT, BISHOP, QUEEN, KING };
enum PromotionChoice { PROMO_NONE, PROMO_QUEEN, PROMO_ROOK,
                       PROMO_BISHOP, PROMO_KNIGHT };

// ============================================================
//  SECTION 2 - BASE CLASS: Piece
// ============================================================

class Piece
{
protected:
    Color     color;
    PieceType type;
    bool      hasMoved;

public:
    Piece(Color c, PieceType t) : color(c), type(t), hasMoved(false) {}
    virtual ~Piece() {}

    Color     getColor()    const { return color;    }
    PieceType getType()     const { return type;     }
    bool      getHasMoved() const { return hasMoved; }
    void      setHasMoved(bool v) { hasMoved = v;    }

    // Pure virtual - each subclass implements its own geometry
    virtual bool isValidMove(int fromR, int fromC,
                             int toR,   int toC,
                             Piece* board[8][8]) const = 0;

    static bool inBounds(int r, int c)
    {
        return r >= 0 && r < 8 && c >= 0 && c < 8;
    }
};

// ============================================================
//  SECTION 3 - DERIVED PIECE CLASSES
// ============================================================

class Pawn : public Piece
{
public:
    Pawn(Color c) : Piece(c, PAWN) {}

    bool isValidMove(int fromR, int fromC,
                     int toR,   int toC,
                     Piece* board[8][8]) const override
    {
        int dir      = (color == WHITE) ? -1 : 1;
        int dr       = toR - fromR;
        int dc       = toC - fromC;
        int startRow = (color == WHITE) ? 6 : 1;

        // One square forward - must be empty
        if (dc == 0 && dr == dir)
            return board[toR][toC] == nullptr;

        // Two squares forward from starting rank - both squares empty
        if (dc == 0 && dr == 2*dir && fromR == startRow)
            return board[fromR+dir][fromC] == nullptr &&
                   board[toR][toC]         == nullptr;

        // Diagonal capture - enemy piece on destination
        if (abs(dc) == 1 && dr == dir)
            if (board[toR][toC] != nullptr &&
                board[toR][toC]->getColor() != color)
                return true;

        return false;
        // En-passant diagonal is validated separately in Board
    }
};

class Rook : public Piece
{
public:
    Rook(Color c) : Piece(c, ROOK) {}

    bool isValidMove(int fromR, int fromC,
                     int toR,   int toC,
                     Piece* board[8][8]) const override
    {
        if (fromR != toR && fromC != toC) return false;

        int dr = (toR == fromR) ? 0 : (toR > fromR ? 1 : -1);
        int dc = (toC == fromC) ? 0 : (toC > fromC ? 1 : -1);

        int r = fromR+dr, c = fromC+dc;
        while (r != toR || c != toC)
        {
            if (board[r][c]) return false;
            r += dr; c += dc;
        }
        return board[toR][toC] == nullptr ||
               board[toR][toC]->getColor() != color;
    }
};

class Knight : public Piece
{
public:
    Knight(Color c) : Piece(c, KNIGHT) {}

    bool isValidMove(int fromR, int fromC,
                     int toR,   int toC,
                     Piece* board[8][8]) const override
    {
        int dr = abs(toR - fromR);
        int dc = abs(toC - fromC);
        if (!((dr==2&&dc==1)||(dr==1&&dc==2))) return false;
        return board[toR][toC]==nullptr ||
               board[toR][toC]->getColor()!=color;
    }
};


class Bishop : public Piece
{
public:
    Bishop(Color c) : Piece(c, BISHOP) {}

    bool isValidMove(int fromR, int fromC,
                     int toR,   int toC,
                     Piece* board[8][8]) const override
    {
        int dr = abs(toR - fromR);
        int dc = abs(toC - fromC);
        if (dr != dc || dr == 0) return false;

        int sR = (toR>fromR)?1:-1, sC = (toC>fromC)?1:-1;
        int r = fromR+sR, c = fromC+sC;
        while (r != toR || c != toC)
        {
            if (board[r][c]) return false;
            r+=sR; c+=sC;
        }
        return board[toR][toC]==nullptr ||
               board[toR][toC]->getColor()!=color;
    }
};

class Queen : public Piece
{
public:
    Queen(Color c) : Piece(c, QUEEN) {}

    bool isValidMove(int fromR, int fromC,
                     int toR,   int toC,
                     Piece* board[8][8]) const override
    {
        int dr = abs(toR-fromR), dc = abs(toC-fromC);
        bool straight = (fromR==toR || fromC==toC);
        bool diagonal = (dr==dc && dr>0);
        if (!straight && !diagonal) return false;

        int sR = (toR==fromR)?0:(toR>fromR?1:-1);
        int sC = (toC==fromC)?0:(toC>fromC?1:-1);
        int r = fromR+sR, c = fromC+sC;
        while (r!=toR || c!=toC)
        {
            if (board[r][c]) return false;
            r+=sR; c+=sC;
        }
        return board[toR][toC]==nullptr ||
               board[toR][toC]->getColor()!=color;
    }
};

class King : public Piece
{
public:
    King(Color c) : Piece(c, KING) {}

    bool isValidMove(int fromR, int fromC,
                     int toR,   int toC,
                     Piece* board[8][8]) const override
    {
        int dr = abs(toR-fromR), dc = abs(toC-fromC);
        // One step in any direction only
        if (dr <= 1 && dc <= 1 && (dr+dc > 0))
            return board[toR][toC]==nullptr ||
                   board[toR][toC]->getColor()!=color;
        return false;
    }
};
