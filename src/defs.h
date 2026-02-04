#include <iostream>
#include <vector>

#define NAME "Ohagi"
#define BRD_SQ_NUM 120

#define MAXGAMEMOVES 2048
#define MAX_POSITION_MOVES 256
#define MAXDEPTH 245

#define START_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

#define INFINITE 200000
#define ISMATE (INFINITE - MAXDEPTH)

using namespace std;

// Debugging pointer
#define DEBUG //Switch on or off

#ifdef DEBUG
#define ASSERT(n) \
do { \
    if(!(n)) { \
        printf("%s - Failed ", #n); \
        printf("On %s ", __DATE__); \
        printf("At %s ", __TIME__); \
        printf("In File %s ", __FILE__); \
        printf("At Line %d\n", __LINE__); \
        exit(1); \
    } \
} while (0)
#else
#define ASSERT(n)
#endif

typedef unsigned long long U64;

enum { EMPTY, wP, wN, wB, wR, wQ, wK, bP, bN, bB, bR, bQ, bK };

enum { FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H, FILE_NONE };
enum { RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8, RANK_NONE };

enum { WHITE, BLACK, BOTH };
enum { UCIMODE, XBOARDMODE, CONSOLEMODE };

enum {
    A1 = 21, B1, C1, D1, E1, F1, G1, H1,
    A2 = 31, B2, C2, D2, E2, F2, G2, H2,
    A3 = 41, B3, C3, D3, E3, F3, G3, H3,
    A4 = 51, B4, C4, D4, E4, F4, G4, H4,
    A5 = 61, B5, C5, D5, E5, F5, G5, H5,
    A6 = 71, B6, C6, D6, E6, F6, G6, H6,
    A7 = 81, B7, C7, D7, E7, F7, G7, H7,
    A8 = 91, B8, C8, D8, E8, F8, G8, H8, NO_SQ, OFFBOARD
};

enum { FALSE, TRUE };

//Castling definitions: https://chatgpt.com/s/t_68766f6682b48191a9f2afe339975026
enum { WKCA = 1, WQCA = 2, BKCA = 4, BQCA = 8 };


typedef struct S_MOVE {
    int move;
    int score;
} S_MOVE;

typedef struct S_MOVELIST{
    S_MOVE moves[MAX_POSITION_MOVES];
    int count;
} S_MOVELIST;

typedef struct S_PVENTRY{
    U64 posKey;
    int move;
} S_PVENTRY; //PV Entry

typedef struct S_PVTABLE{
    S_PVENTRY *pTable;
    int numEntries;
} S_PVTABLE;

typedef struct S_UNDO {

    int move;
    int castlePerm;
    int enPas;
    int fiftyMove;
    U64 posKey;

} S_UNDO;

typedef struct S_BOARD {
    
    int pieces[BRD_SQ_NUM];
    U64 pawns[3]; 

    int KingSq[2];

    int side;
    int enPas;
    int fiftyMove;

    int ply;
    int hisPly;

    int castlePerm;
    
    U64 posKey;

    int pceNum[13];
    int bigPce[2];
    int majPce[2];
    int minPce[2];
    int material[2];

    S_UNDO history[MAXGAMEMOVES];

    //Piece List (Maximum 10 pieces for a piece set like pawn) - https://chatgpt.com/s/t_686fd9f539fc8191b87e12cf9425a46b
    int pList[13][10];

    S_PVTABLE PvTable[1];
    int PvArray[MAXDEPTH];

    int searchHistory[13][BRD_SQ_NUM];
    int searchKillers[2][MAXDEPTH];

} S_BOARD;

typedef struct S_SEARCHINFO {
    int startTime;
    int stopTime;
    int depth;
    int depthset;
    int timeSet;
    int movesTogo;
    int infinite;

    long nodes;

    int quit;
    int stopped;

    float fh; //Fail high
    float fhf; //Fail high first
    int nullCut;

    int GAME_MODE;
	int POST_THINKING;
    
} S_SEARCHINFO;


/* GAME MOVE */
#define FROMSQ(m) ((m) & 0x7F)
#define TOSQ(m) (((m) >> 7) & 0x7F)
#define CAPTURED(m) (((m) >> 14) & 0xF)
#define PROMOTED(m) (((m) >> 20) & 0xF)

#define MFLAG_EP    0x40000 //Move Flag _ En Passant
#define MFLAG_PS    0x80000
#define MFLAG_CA    0x1000000

#define MFLAG_CAP   0x7C000
#define MFLAG_PROM  0xF00000

#define NO_MOVE 0

/* MACROS */
#define FR2SQ(f,r) ( (21 + (f) ) + ( (r) * 10 ) )
#define SQ64(sq120) (Sq120ToSq64[sq120])
#define SQ120(sq64) (Sq64ToSq120[sq64])
#define POP(b) PopBit(b)
#define CNT(b) CountBits(b)
#define SETBIT(bb, sq) ((bb) |= SetMask[(sq)])
#define CLRBIT(bb, sq) ((bb) &= ClearMask[(sq)])

#define IsBQ(p) (PieceBishopQueen[(p)])
#define IsRQ(p) (PieceRookQueen[(p)])
#define IsKn(p) (PieceKnight[(p)])
#define IsKi(p) (PieceKing[(p)])

#define MIRROR64(sq) (Mirror64[(sq)])


/* GLOBALS */
extern int Sq120ToSq64[BRD_SQ_NUM];
extern int Sq64ToSq120[64];
extern U64 SetMask[64];
extern U64 ClearMask[64];
extern U64 PieceKeys[13][120];
extern U64 SideKey;
extern U64 CastleKeys[16];
extern char PceChar[];
extern char SideChar[];
extern char RankChar[];
extern char FileChar[];

extern int PieceBig[13];
extern int PieceMaj[13];
extern int PieceMin[13];
extern int PieceVal[13];
extern int PieceCol[13];

extern int FilesBrd[BRD_SQ_NUM];
extern int RanksBrd[BRD_SQ_NUM];

extern int PiecePawn[13];
extern int PieceKnight[13];
extern int PieceKing[13];
extern int PieceRookQueen[13];
extern int PieceBishopQueen[13];
extern int PieceSlides[13];

extern int Mirror64[64];

extern U64 FileBBMask[8];
extern U64 RankBBMask[8];

extern U64 BlackPassPawnMask[64];
extern U64 WhitePassPawnMask[64];
extern U64 IsolatePawnMask[64];

/* FUNCTIONS */
// init.c
extern void AllInit();

// bitboard.c
extern void PrintBitBoard(U64 bb);
extern int PopBit(U64 *bb);
extern int CountBits(U64 b);

// hashKeys.c
extern U64 GeneratePosKey(const S_BOARD *pos);

// board.c
extern void ResetBoard(S_BOARD *pos);
extern int Parse_FEN(char *fen, S_BOARD *pos);
extern void PrintBoard(const S_BOARD *pos);
extern void UpdateListsMaterials(S_BOARD *pos);
extern int CheckBoard(const S_BOARD *pos);
extern void MirrorBoard(S_BOARD *pos);

//moveLogic.c
extern int SqAttacked(const int sq, const int side, const S_BOARD *pos);

//io.c
extern char *PrintSq(const int sq);
extern char *PrintMove(const int move);
extern void PrintMoveList(const S_MOVELIST *list);
extern int ParseMove(char *ptrChar, S_BOARD *pos);

//moveGen.c
extern void GenerateAllMoves(const S_BOARD *pos, S_MOVELIST *list);
extern void GenerateAllCaps(const S_BOARD *pos, S_MOVELIST *list);
extern int MoveExists(S_BOARD *pos, const int move);
extern int InitMvvLva();

//validate.c
extern int SqOnBoard(const int sq);
extern int SideValid(const int side);
extern int FileRankValid(const int fr);
extern int PieceValidEmpty(const int pce);
extern int PieceValid(const int pce);
extern void MirrorEvalTest(S_BOARD *pos);

//makeMove.c
extern int MakeMove(S_BOARD *pos, int move);
extern void TakeMove(S_BOARD *pos);
extern void MakeNullMove(S_BOARD *pos);
extern void TakeNullMove(S_BOARD *pos);

//perfTest.c
extern void Perft(int depth, S_BOARD *pos);
extern void PerftTest (int depth, S_BOARD *pos);

//search.c
extern void SearchPosition(S_BOARD *pos, S_SEARCHINFO *info);

//utility.c
extern int GetTimeMs();
extern void ReadInput(S_SEARCHINFO *info);

//pvtable.c
extern void ClearPvTable(S_PVTABLE *table);
extern void InitPvTable(S_PVTABLE *table);
extern void StorePvMove(const S_BOARD *pos, const int move);
extern int ProbePvTable(const S_BOARD *pos);
extern int GetPvLine(const int depth, S_BOARD *pos);

//evaluate.c
extern int EvalPosition(const S_BOARD *pos);

//uci.c
extern void ParseGo(char* line, S_SEARCHINFO *info, S_BOARD *pos);
extern void ParsePosition(char* lineIn, S_BOARD *pos);
extern void UCI_Loop(S_BOARD *pos, S_SEARCHINFO *info);

//xboard.c
extern void XBoard_Loop(S_BOARD *pos, S_SEARCHINFO *info);
extern void Console_Loop(S_BOARD *pos, S_SEARCHINFO *info);
extern void MirrorEvalTest(S_BOARD *pos);