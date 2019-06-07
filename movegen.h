#pragma once
#include <Windows.h>
#include <stdint.h>
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
extern const UINT MASK_ODD_ROW;
extern const UINT MASK_BOTTOM;
extern const UINT MASK_TRAPPED_W;
extern const UINT MASK_TRAPPED_B;

extern const uint32_t sq1, sq2, sq3, sq4, sq5, sq6, sq7, sq8, sq9, sq10, sq11, sq12, sq13, sq14, sq15, sq16,
			sq17, sq18, sq19, sq20, sq21, sq22, sq23, sq24, sq25, sq26, sq27, sq28, sq29, sq30, sq31, sq32;


// S[] contains 32-bit bitboards with a single bit for each of the 32 squares (plus 2 invalid squares with no bits set)
extern const UINT S[34];

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
	int inline GetPiece(int sq) const
	{
		if (S[sq] & BP) {
			return(S[sq] & K) ? BKING : BPIECE;
		}

		if (S[sq] & WP) {
			return(S[sq] & K) ? WKING : WPIECE;
		}

		if (sq >= 32)
			return INVALID;

		return EMPTY;
	}
	// This function will test if a checker needs to be upgraded to a king, and upgrade if necessary
	void inline CheckKing(const int src, const int dst, int const nPiece)
	{
		if (dst <= 3)
			K |= S[dst];

		if (dst >= 28)
			K |= S[dst];
	}

	void inline DoSingleJump(int src, int dst, const int nPiece, const int SideToMove)
	{
		int jumpedSq = ((dst + src) >> 1);
		if (S[jumpedSq] & MASK_ODD_ROW)
			jumpedSq += 1;	// correct for square number since the jumpedSq sometimes up 1 sometimes down 1 of the average
		int jumpedPiece = GetPiece(jumpedSq);

		// Update the bitboards
		UINT BitMove = (S[src] | S[dst]);
		if (SideToMove == BLACK) {
			WP ^= BitMove;
			BP ^= S[jumpedSq];
			K &= ~S[jumpedSq];
		}
		else {
			BP ^= BitMove;
			WP ^= S[jumpedSq];
			K &= ~S[jumpedSq];
		}

		empty = ~(WP | BP);
		if (nPiece & KING)
			K ^= BitMove;
	}

	void successor(SMove &move, int color);
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

	inline int build_movelist(SCheckerBoard &board, int color)
	{
		if (color == WHITE)
			FindMovesWhite(board);
		else
			FindMovesBlack(board);
	
		return(numMoves);
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

