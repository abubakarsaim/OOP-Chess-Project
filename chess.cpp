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
                             const Piece* board[8][8]) const = 0;

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
                     const Piece* board[8][8]) const override
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
                     const Piece* board[8][8]) const override
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
                     const Piece* board[8][8]) const override
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
                     const Piece* board[8][8]) const override
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
                     const Piece* board[8][8]) const override
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
                     const Piece* board[8][8]) const override
    {
        int dr = abs(toR-fromR), dc = abs(toC-fromC);
        // One step in any direction only
        if (dr <= 1 && dc <= 1 && (dr+dc > 0))
            return board[toR][toC]==nullptr ||
                   board[toR][toC]->getColor()!=color;
        return false;
    }
};

class Board
{
    Piece* grid[8][8];

    int   enPassantCol;   // col of pawn that just moved 2 squares (-1=none)
    int   enPassantRow;   // row of that pawn

    Color currentTurn;
    bool  pieceSelected;
    int   selectedRow, selectedCol;


    void findKing(Color c, int &row, int &col) const
    {
        for(int r=0;r<8;r++)
            for(int cc=0;cc<8;cc++)
                if(grid[r][cc] &&
                   grid[r][cc]->getType()==KING &&
                   grid[r][cc]->getColor()==c)
                { row=r; col=cc; return; }
        row=col=-1;
    }

    bool isSquareAttacked(int r, int c, Color by) const
    {
        for(int fr=0;fr<8;fr++)
            for(int fc=0;fc<8;fc++)
            {
                Piece* p=grid[fr][fc];
                if(!p||p->getColor()!=by) continue;
                if(p->getType()==PAWN)
                {
                    int dir=(by==WHITE)?-1:1;
                    if(fr+dir==r&&(fc-1==c||fc+1==c)) return true;
                    continue;
                }
                if(p->isValidMove(fr,fc,r,c,(const Piece* (*)[8])grid)) return true;
            }
        return false;
    }

    bool kingInCheck(Color kc) const
    {
        int kr,kcc;
        findKing(kc,kr,kcc);
        if(kr==-1) return false;
        Color enemy=(kc==WHITE)?BLACK:WHITE;
        return isSquareAttacked(kr,kcc,enemy);
    }

    // --------------------------------------------------------
    //  Castling - all 5 conditions enforced here
    // --------------------------------------------------------
    bool isCastlingMove(int fR,int fC,int tR,int tC) const
    {
        // Must be a king that hasn't moved
        Piece* king=grid[fR][fC];
        if(!king||king->getType()!=KING) return false;
        if(king->getHasMoved()) return false;
        if(fR!=tR) return false;

        int dc=tC-fC;
        if(dc!=2&&dc!=-2) return false;

        // Correct rook must exist and not have moved
        int rookCol=(dc==2)?7:0;
        Piece* rook=grid[fR][rookCol];
        if(!rook||rook->getType()!=ROOK) return false;
        if(rook->getHasMoved()) return false;
        if(rook->getColor()!=king->getColor()) return false;

        // All squares between king and rook must be empty
        int minC=(rookCol<fC)?rookCol+1:fC+1;
        int maxC=(rookCol<fC)?fC-1:rookCol-1;
        for(int c=minC;c<=maxC;c++)
            if(grid[fR][c]) return false;

        // King must not currently be in check
        Color enemy=(king->getColor()==WHITE)?BLACK:WHITE;
        if(isSquareAttacked(fR,fC,enemy)) return false;

        // King must not pass through or land on an attacked square
        int step=(dc==2)?1:-1;
        if(isSquareAttacked(fR,fC+step,  enemy)) return false;
        if(isSquareAttacked(fR,fC+2*step,enemy)) return false;

        return true;
    }

    // --------------------------------------------------------
    //  En-passant
    // --------------------------------------------------------
    bool isEnPassantMove(int fR,int fC,int tR,int tC) const
    {
        Piece* p=grid[fR][fC];
        if(!p||p->getType()!=PAWN) return false;
        if(enPassantCol==-1) return false;

        int dir=(p->getColor()==WHITE)?-1:1;
        // Our pawn must be beside the enemy pawn (same row as enemy)
        // and move diagonally one step forward into the empty square
        return (fR==enPassantRow      &&  // we are beside the ep pawn
                tC==enPassantCol      &&  // we move into its column
                tR==fR+dir);             // one step forward
    }

    // --------------------------------------------------------
    //  Full legality
    // --------------------------------------------------------
    bool isLegalMove(int fR,int fC,int tR,int tC) const
    {
        if(!Piece::inBounds(fR,fC)||!Piece::inBounds(tR,tC)) return false;

        Piece* p=grid[fR][fC];
        if(!p) return false;
        if(p->getColor()!=currentTurn) return false;

        // Cannot capture own piece
        if(grid[tR][tC]&&grid[tR][tC]->getColor()==currentTurn) return false;

        // Check special moves first
        bool castle    = isCastlingMove(fR,fC,tR,tC);
        bool enPassant = isEnPassantMove(fR,fC,tR,tC);

        if(!castle&&!enPassant)
        {
            // Normal piece geometry check
            if(!p->isValidMove(fR,fC,tR,tC,(const Piece* (*)[8])grid)) return false;
        }

        // ---- Simulate the move to ensure king is not left in check ----
        Board* self=const_cast<Board*>(this);

        Piece* savedTo  =self->grid[tR][tC];
        Piece* savedFrom=self->grid[fR][fC];
        Piece* epCap    =nullptr;

        self->grid[tR][tC]  =savedFrom;
        self->grid[fR][fC]  =nullptr;

        if(enPassant)
        {
            // Remove the en-passant captured pawn for simulation
            epCap=self->grid[enPassantRow][enPassantCol];
            self->grid[enPassantRow][enPassantCol]=nullptr;
        }

        // Move rook for castling simulation
        int crFrom=-1,crTo=-1;
        if(castle)
        {
            int dc=tC-fC;
            crFrom=(dc==2)?7:0;
            crTo  =(dc==2)?5:3;
            self->grid[fR][crTo]  =self->grid[fR][crFrom];
            self->grid[fR][crFrom]=nullptr;
        }

        bool inCheck=self->kingInCheck(savedFrom->getColor());

        // ---- Restore board exactly ----
        self->grid[fR][fC]  =savedFrom;
        self->grid[tR][tC]  =savedTo;
        if(enPassant&&epCap)
            self->grid[enPassantRow][enPassantCol]=epCap;
        if(castle)
        {
            self->grid[fR][crFrom]=self->grid[fR][crTo];
            self->grid[fR][crTo]  =nullptr;
        }

        return !inCheck;
    }

    bool hasLegalMoves(Color forColor)
    {
        Color saved=currentTurn;
        currentTurn=forColor;
        for(int fr=0;fr<8;fr++)
            for(int fc=0;fc<8;fc++)
            {
                if(!grid[fr][fc]||grid[fr][fc]->getColor()!=forColor) continue;
                for(int tr=0;tr<8;tr++)
                    for(int tc=0;tc<8;tc++)
                        if(isLegalMove(fr,fc,tr,tc))
                        { currentTurn=saved; return true; }
            }
        currentTurn=saved;
        return false;
    }

};

struct MoveRecord
{
    int    fromR, fromC, toR, toC;

    Piece* capturedPiece;   
    bool   captureWasDeleted; 

    bool   wasEnPassant;
    int    capturedEpRow;   // row of the pawn removed by en-passant
    int    capturedEpCol;   // col of the pawn removed by en-passant

    bool   wasCastle;
    bool   wasPromotion;
    bool   pieceHadMoved;   // hasMoved flag of the moved piece before move
    bool   rookHadMoved;    // hasMoved flag of the rook before castling
    int    prevEpCol;       // enPassantCol before this move
    int    prevEpRow;       // enPassantRow before this move
};


void executeMove(int fR,int fC,int tR,int tC)
    {
        Piece* moving=grid[fR][fC];

        // FIX 4: Detect castle BEFORE we clear enPassantCol,
        // so detection is always based on the correct board state.
        bool castle=isCastlingMove(fR,fC,tR,tC);
        bool ep    =isEnPassantMove(fR,fC,tR,tC);

        // Build undo record
        MoveRecord rec;
        rec.fromR          =fR; rec.fromC=fC;
        rec.toR            =tR; rec.toC  =tC;
        rec.wasEnPassant   =ep;
        rec.wasCastle      =castle;
        rec.wasPromotion   =false;
        rec.pieceHadMoved  =moving->getHasMoved();
        rec.rookHadMoved   =false;
        rec.prevEpCol      =enPassantCol;
        rec.prevEpRow      =enPassantRow;
        rec.capturedEpRow  =-1;
        rec.capturedEpCol  =-1;
        rec.capturedPiece  =nullptr;
        rec.captureWasDeleted=false;

        if(ep)
        {
            // FIX 1: Save actual coordinates of the captured pawn
            rec.capturedEpRow =enPassantRow;
            rec.capturedEpCol =enPassantCol;
            rec.capturedPiece =grid[enPassantRow][enPassantCol];
            // Remove from board but DO NOT delete - stored in record
            grid[enPassantRow][enPassantCol]=nullptr;
        }
        else
        {
            // FIX 2: Save the captured piece without deleting it
            rec.capturedPiece=grid[tR][tC];
            // Remove from board but DO NOT delete yet
            grid[tR][tC]=nullptr;
        }

        // Update en-passant state
        enPassantCol=enPassantRow=-1;
        if(moving->getType()==PAWN && abs(tR-fR)==2)
        {
            enPassantCol=tC;
            enPassantRow=tR; // row where the pawn landed
        }

        // Move the rook for castling
        if(castle)
        {
            int dc=tC-fC;
            int rFrom=(dc==2)?7:0;
            int rTo  =(dc==2)?5:3;
            Piece* rook=grid[fR][rFrom];
            rec.rookHadMoved=rook->getHasMoved();
            grid[fR][rTo]  =rook;
            grid[fR][rFrom]=nullptr;
            rook->setHasMoved(true);
        }

        // Move the piece
        grid[tR][tC]  =moving;
        grid[fR][fC]  =nullptr;
        moving->setHasMoved(true);

        // Pawn promotion
        if(moving->getType()==PAWN&&(tR==0||tR==7))
        {
            rec.wasPromotion  =true;
            awaitingPromotion =true;
            promoRow=tR; promoCol=tC;
            promoColor=moving->getColor();
        }

        // Before saving, free any old captured piece sitting in this slot
        if(historySize<MAX_HISTORY)
        {
            // If this slot was used before and had a piece, free it now
            // (only relevant if we somehow wrap around - safety measure)
            history[historySize]=rec;
            historySize++;
        }
        else
        {
            // History full - free oldest captured piece to avoid leak
            if(history[0].capturedPiece)
                delete history[0].capturedPiece;
            // Shift history down by one
            for(int i=0;i<MAX_HISTORY-1;i++)
                history[i]=history[i+1];
            history[MAX_HISTORY-1]=rec;
        }
    }

    // --------------------------------------------------------
    //  promoteAndPlace - swap pawn for chosen piece
    // --------------------------------------------------------
    void promoteAndPlace(PromotionChoice choice)
    {
        if(!awaitingPromotion) return;

        // The pawn on promoRow,promoCol belongs to us now
        // We recorded it in history as the moving piece - delete it
        delete grid[promoRow][promoCol];

        Piece* np=nullptr;
        switch(choice)
        {
            case PROMO_QUEEN:  np=new Queen (promoColor); break;
            case PROMO_ROOK:   np=new Rook  (promoColor); break;
            case PROMO_BISHOP: np=new Bishop(promoColor); break;
            case PROMO_KNIGHT: np=new Knight(promoColor); break;
            default:           np=new Queen (promoColor); break;
        }
        np->setHasMoved(true);
        grid[promoRow][promoCol]=np;

        // Mark the last history record with the promoted type
        // so undo knows to replace with a pawn
        if(historySize>0)
            history[historySize-1].wasPromotion=true;

        awaitingPromotion=false;

        currentTurn=(currentTurn==WHITE)?BLACK:WHITE;
        isCheck    =kingInCheck(currentTurn);
        if(!hasLegalMoves(currentTurn))
        {
            gameOver   =true;
            isCheckmate=isCheck;
            isStalemate=!isCheck;
        }
    }

    // --------------------------------------------------------
    //  undoLastMove
    //
    //  FIX 3: gameOver guard removed so player can undo after
    //  checkmate or stalemate.
    // --------------------------------------------------------
    void undoLastMove()
    {
        if(historySize==0) return;  // nothing to undo

        MoveRecord& rec=history[--historySize];

        // If promotion: the queen/rook/etc on the square is NOT the
        // original pawn. Delete the promoted piece and put pawn back.
        if(rec.wasPromotion)
        {
            Color c=grid[rec.toR][rec.toC]->getColor();
            delete grid[rec.toR][rec.toC];
            grid[rec.toR][rec.toC]=nullptr;

            // Place original pawn back
            Piece* pawn=new Pawn(c);
            pawn->setHasMoved(rec.pieceHadMoved);
            grid[rec.fromR][rec.fromC]=pawn;
        }
        else
        {
            // Move piece back to its origin
            Piece* moving=grid[rec.toR][rec.toC];
            moving->setHasMoved(rec.pieceHadMoved);
            grid[rec.fromR][rec.fromC]=moving;
            grid[rec.toR  ][rec.toC  ]=nullptr;
        }

        // FIX 2: Restore normal captured piece (was never deleted)
        if(!rec.wasEnPassant && rec.capturedPiece)
            grid[rec.toR][rec.toC]=rec.capturedPiece;

        // FIX 1: Restore en-passant captured pawn at its real location
        if(rec.wasEnPassant && rec.capturedPiece)
        {
            grid[rec.capturedEpRow][rec.capturedEpCol]=rec.capturedPiece;
        }

        // Restore castling rook
        if(rec.wasCastle)
        {
            int dc       =rec.toC-rec.fromC;
            int rookCurC =(dc==2)?5:3;
            int rookOrigC=(dc==2)?7:0;
            Piece* rook  =grid[rec.fromR][rookCurC];
            if(rook) rook->setHasMoved(rec.rookHadMoved);
            grid[rec.fromR][rookOrigC]=rook;
            grid[rec.fromR][rookCurC ]=nullptr;
        }

        // Restore en-passant tracking variables
        enPassantCol=rec.prevEpCol;
        enPassantRow=rec.prevEpRow;

        // Flip turn back and recalculate check state
        currentTurn =(currentTurn==WHITE)?BLACK:WHITE;
        isCheck     =kingInCheck(currentTurn);
        gameOver    =false;
        isCheckmate =false;
        isStalemate =false;
        pieceSelected=false;
    }

    // --------------------------------------------------------
    //  Free all captured pieces stored in history (for destructor)
    // --------------------------------------------------------
    void freeHistory()
    {
        for(int i=0;i<historySize;i++)
            if(history[i].capturedPiece)
            {
                delete history[i].capturedPiece;
                history[i].capturedPiece=nullptr;
            }
        historySize=0;
    }

    //  handlePromoClick
    // --------------------------------------------------------
    void handlePromoClick(int px,int py)
    {
        if(!awaitingPromotion) return;
        int dX=(8*SQ-4*SQ)/2;
        int dY=(8*SQ-(SQ+40))/2;
        if(py<dY+33||py>dY+33+SQ-4) return;
        if(px<dX   ||px>dX+4*SQ   ) return;
        int idx=(px-dX)/SQ;
        PromotionChoice ch[4]={PROMO_QUEEN,PROMO_ROOK,
                                PROMO_BISHOP,PROMO_KNIGHT};
        if(idx>=0&&idx<4) promoteAndPlace(ch[idx]);
    }

    void handleUndoKey() { undoLastMove(); 
    }

    ~Board()
    {
        // Free pieces still on the board
        for(int r=0;r<8;r++)
            for(int c=0;c<8;c++)
                delete grid[r][c];

        // Free any captured pieces stored in undo history
        freeHistory();
    }

    


