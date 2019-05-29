#pragma once
#include <Windows.h>
#include <assert.h>
#include "guiCheckers.h"

extern const UINT MASK_1ST;
extern const UINT MASK_2ND;
extern const UINT MASK_3RD;
extern const UINT MASK_4TH;
extern const UINT MASK_5TH;
extern const UINT MASK_6TH;
extern const UINT MASK_7TH;
extern const UINT MASK_8TH;
extern const UINT MASK_TOP;
extern const UINT MASK_BOTTOM;
extern const UINT MASK_TRAPPED_W;
extern const UINT MASK_TRAPPED_B;

struct SMove
{
	unsigned short SrcDst;
	char cPath[10];
};

// This is now the only board structure used for GUI checkers. It is 16 bytes in size.
struct SCheckerBoard
{
	// data
	UINT WP;		// White Pieces
	UINT BP;		// Black Pieces
	UINT K;			// Kings (of either color)
	UINT empty;		// Empty squares

	// functions
	int GetBlackMoves();
	int GetWhiteMoves();
	int CanWhiteCheckerMove(UINT checkers);
	int CanBlackCheckerMove(UINT checkers);
	UINT inline GetJumpersWhite();
	UINT inline GetJumpersBlack();
	UINT inline GetMoversWhite();
	UINT inline GetMoversBlack();
};

//
// BITBOARD INITIALIZATION
//
//  28 29 30 31
// 24 25 26 27
//  20 21 22 23
// 16 17 18 19
//  12 13 14 15
// 08 09 10 11
//  04 05 06 07
// 00 01 02 03
extern const int INV; // invalid square

// This is a 32x4 ( 32 source squares x 4 move directions ) lookup table that will return the destination square index for the direction
extern const int nextSq[32 * 4];

// S[] contains 32-bit bitboards with a single bit for each of the 32 squares (plus 2 invalid squares with no bits set)
extern const UINT S[34];

extern const UINT MASK_EDGES;
extern const UINT MASK_2CORNER1;
extern const UINT MASK_2CORNER2;

// Create lookup tables used for finding the lowest 1 bit, highest 1 bit, and the number of 1 bits in a 16-bit integer
void InitBitTables();

// Return the number of 1 bits in a 32-bit int
inline UINT BitCount(UINT Moves)
{
	extern unsigned char aBitCount[65536];
	if (Moves == 0)
		return 0;
	return aBitCount[(Moves & 65535)] + aBitCount[((Moves >> 16) & 65535)];
}


// Find the "lowest" 1 bit of a 32-bit int
UINT inline FindLowBit(UINT Moves);

// Find the "highest" 1 bit of a 32-bit int
UINT inline FindHighBit(UINT Moves);


// note : Moves[0] is empty, the first move is Moves[1]
struct CMoveList
{
	// DATA
	int numMoves;
	int numJumps;
	SMove m_JumpMove;
	SMove Moves[36];

	// FUNCTIONS
	void inline Clear()
	{
		numJumps = 0;
		numMoves = 0;
	}

	void inline StartJumpMove(int src, int dst)
	{
		m_JumpMove.SrcDst = (src) + (dst << 6) + (1 << 12);
	}

	void inline AddJump(SMove &Move, int pathNum)
	{
		assert(pathNum < 10);
		assert(numJumps < 36);

		Move.cPath[pathNum] = 33;
		Moves[++numJumps] = Move;
	}

	void inline AddMove(int src, int dst)
	{
		assert(numMoves < 36);

		Moves[++numMoves].SrcDst = (src) + (dst << 6);
		return;
	}

	int inline AddSqDirBlack(SCheckerBoard &C, int square, const UINT isKing, int pathNum, const int DIR);
	int inline AddSqDirWhite(SCheckerBoard &C, int square, const UINT isKing, int pathNum, const int DIR);
	void inline CheckJumpDirBlack(SCheckerBoard &C, int square, const int DIR);
	void inline CheckJumpDirWhite(SCheckerBoard &C, int square, const int DIR);
	void FindMovesBlack(SCheckerBoard &C);
	void FindMovesWhite(SCheckerBoard &C);
	void FindJumpsBlack(SCheckerBoard &C, int Movers);
	void FindJumpsWhite(SCheckerBoard &C, int Movers);
	void FindNonJumpsBlack(SCheckerBoard &C, int Movers);
	void FindNonJumpsWhite(SCheckerBoard &C, int Movers);
	void inline FindSqJumpsBlack(SCheckerBoard &C, int square, int pathNum, int jumpSquare, const UINT isKing);
	void inline FindSqJumpsWhite(SCheckerBoard &C, int square, int pathNum, int jumpSquare, const UINT isKing);
	void inline AddNormalMove(SCheckerBoard &C, int src, int dst);
};

extern CMoveList g_Movelist[MAX_SEARCHDEPTH + 1];
extern SMove g_GameMoves[MAX_GAMEMOVES];

