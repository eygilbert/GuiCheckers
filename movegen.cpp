#include "movegen.h"

// =================================================

//

//	 			MOVE GENERATION

//

// =================================================


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
const int INV = 33; // invalid square

// This is a 32x4 ( 32 source squares x 4 move directions ) lookup table that will return the destination square index for the direction
const int nextSq[32 * 4] =
{
	4,
	5,
	6,
	7,
	9,
	10,
	11,
	INV,
	12,
	13,
	14,
	15,
	17,
	18,
	19,
	INV,
	20,
	21,
	22,
	23,
	25,
	26,
	27,
	INV,
	28,
	29,
	30,
	31,
	INV,
	INV,
	INV,
	INV,
	INV,
	4,
	5,
	6,
	8,
	9,
	10,
	11,
	INV,
	12,
	13,
	14,
	16,
	17,
	18,
	19,
	INV,
	20,
	21,
	22,
	24,
	25,
	26,
	27,
	INV,
	28,
	29,
	30,
	INV,
	INV,
	INV,
	INV,
	INV,
	INV,
	INV,
	INV,
	0,
	1,
	2,
	3,
	INV,
	4,
	5,
	6,
	8,
	9,
	10,
	11,
	INV,
	12,
	13,
	14,
	16,
	17,
	18,
	19,
	INV,
	20,
	21,
	22,
	24,
	25,
	26,
	27,
	INV,
	INV,
	INV,
	INV,
	1,
	2,
	3,
	INV,
	4,
	5,
	6,
	7,
	9,
	10,
	11,
	INV,
	12,
	13,
	14,
	15,
	17,
	18,
	19,
	INV,
	20,
	21,
	22,
	23,
	25,
	26,
	27,
	INV
};

// S[] contains 32-bit bitboards with a single bit for each of the 32 squares (plus 2 invalid squares with no bits set)
const UINT S[34] =
{
	(UINT) 1 << 0,
	(UINT) 1 << 1,
	(UINT) 1 << 2,
	(UINT) 1 << 3,
	(UINT) 1 << 4,
	(UINT) 1 << 5,
	(UINT) 1 << 6,
	(UINT) 1 << 7,
	(UINT) 1 << 8,
	(UINT) 1 << 9,
	(UINT) 1 << 10,
	(UINT) 1 << 11,
	(UINT) 1 << 12,
	(UINT) 1 << 13,
	(UINT) 1 << 14,
	(UINT) 1 << 15,
	(UINT) 1 << 16,
	(UINT) 1 << 17,
	(UINT) 1 << 18,
	(UINT) 1 << 19,
	(UINT) 1 << 20,
	(UINT) 1 << 21,
	(UINT) 1 << 22,
	(UINT) 1 << 23,
	(UINT) 1 << 24,
	(UINT) 1 << 25,
	(UINT) 1 << 26,
	(UINT) 1 << 27,
	(UINT) 1 << 28,
	(UINT) 1 << 29,
	(UINT) 1 << 30,
	(UINT) 1 << 31,
	0,
	0				// invalid no bits set
};

// These masks are used in move generation.
// They define squares for which a particular shift is possible.
// eg. L3 = Left 3 = squares where you can move from src to src+3

const UINT MASK_L3 = S[1] | S[2] | S[3] | S[9] | S[10] | S[11] | S[17] | S[18] | S[19] | S[25] | S[26] | S[27];
const UINT MASK_L5 = S[4] | S[5] | S[6] | S[12] | S[13] | S[14] | S[20] | S[21] | S[22];
const UINT MASK_R3 = S[28] | S[29] | S[30] | S[20] | S[21] | S[22] | S[12] | S[13] | S[14] | S[4] | S[5] | S[6];
const UINT MASK_R5 = S[25] | S[26] | S[27] | S[17] | S[18] | S[19] | S[9] | S[10] | S[11];

// These masks are used in evaluation
const UINT MASK_TOP = S[28] | S[29] | S[30] | S[31] | S[24] | S[25] | S[26] | S[27] | S[20] | S[21] | S[22] | S[23];
const UINT MASK_BOTTOM = S[0] | S[1] | S[2] | S[3] | S[4] | S[5] | S[6] | S[7] | S[8] | S[9] | S[10] | S[11];
const UINT MASK_TRAPPED_W = S[0] | S[1] | S[2] | S[3] | S[4];
const UINT MASK_TRAPPED_B = S[28] | S[29] | S[30] | S[31] | S[27];
const UINT MASK_1ST = S[0] | S[1] | S[2] | S[3];
const UINT MASK_2ND = S[4] | S[5] | S[6] | S[7];
const UINT MASK_3RD = S[8] | S[9] | S[10] | S[11];
const UINT MASK_4TH = S[12] | S[13] | S[14] | S[15];
const UINT MASK_5TH = S[16] | S[17] | S[18] | S[19];
const UINT MASK_6TH = S[20] | S[21] | S[22] | S[23];
const UINT MASK_7TH = S[24] | S[25] | S[26] | S[27];
const UINT MASK_8TH = S[28] | S[29] | S[30] | S[31];
const UINT MASK_ODD_ROW = MASK_1ST | MASK_3RD | MASK_5TH | MASK_7TH;
const UINT MASK_EDGES = S[24] |
	S[20] |
	S[16] |
	S[12] |
	S[8] |
	S[4] |
	S[0] |
	S[7] |
	S[11] |
	S[15] |
	S[19] |
	S[23] |
	S[27] |
	S[31];
const UINT MASK_2CORNER1 = S[3] | S[7];
const UINT MASK_2CORNER2 = S[24] | S[28];

unsigned char aBitCount[65536];
unsigned char aLowBit[65536];
unsigned char aHighBit[65536];

// Create lookup tables used for finding the lowest 1 bit, highest 1 bit, and the number of 1 bits in a 16-bit integer
void InitBitTables()
{
	for (int Moves = 0; Moves < 65536; Moves++) {
		int nBits = 0;
		int nLow = 255, nHigh = 255;
		for (int i = 0; i < 16; i++) {
			if (Moves & S[i]) {
				nBits++;
				if (nLow == 255)
					nLow = i;
				nHigh = i;
			}
		}

		aLowBit[Moves] = nLow;
		aHighBit[Moves] = nHigh;
		aBitCount[Moves] = nBits;
	}
}

// Find the "lowest" 1 bit of a 32-bit int
UINT inline FindLowBit(UINT Moves)
{
	if ((Moves & 65535))
		return aLowBit[(Moves & 65535)];
	if (((Moves >> 16) & 65535))
		return aLowBit[((Moves >> 16) & 65535)] + 16;
	return 0;
}

// Find the "highest" 1 bit of a 32-bit int
UINT inline FindHighBit(UINT Moves)
{
	if (((Moves >> 16) & 65535))
		return aHighBit[((Moves >> 16) & 65535)] + 16;
	if ((Moves & 65535))
		return aHighBit[(Moves & 65535)];
	return 0;
}

//
// BIT BOARD FUNCTIONS
//
// Return a bitboard of white pieces that have a possible non-jumping move.
// We start with the empty sq bitboard, shift for each possible move directions

// and or all the directions together to get the movable pieces bitboard.
UINT SCheckerBoard::GetMoversWhite()
{
	UINT Moves = (empty << 4);
	const UINT WK = WP & K; // Kings
	Moves |= ((empty & MASK_L3) << 3);
	Moves |= ((empty & MASK_L5) << 5);
	Moves &= WP;
	if (WK) {
		Moves |= (empty >> 4) & WK;
		Moves |= ((empty & MASK_R3) >> 3) & WK;
		Moves |= ((empty & MASK_R5) >> 5) & WK;
	}

	return Moves;
}

UINT SCheckerBoard::GetMoversBlack()
{
	UINT Moves = (empty >> 4);
	const UINT BK = BP & K;
	Moves |= ((empty & MASK_R3) >> 3);
	Moves |= ((empty & MASK_R5) >> 5);
	Moves &= BP;
	if (BK) {
		Moves |= (empty << 4) & BK;
		Moves |= ((empty & MASK_L3) << 3) & BK;
		Moves |= ((empty & MASK_L5) << 5) & BK;
	}

	return Moves;
}

// Return the number of white moves possible
int SCheckerBoard::GetWhiteMoves()
{
	const UINT WK = WP & K;
	int numMoves = BitCount((empty << 4) & WP);
	numMoves += BitCount((((empty & MASK_L3) << 3) | ((empty & MASK_L5) << 5)) & WP);
	if (WK) {
		numMoves += BitCount((empty >> 4) & WK);
		numMoves += BitCount((((empty & MASK_R3) >> 3) | ((empty & MASK_R5) >> 5)) & WK);
	}

	return numMoves;
}

int SCheckerBoard::GetBlackMoves()
{
	const UINT BK = BP & K;
	int numMoves = BitCount((empty >> 4) & BP);
	numMoves += BitCount((((empty & MASK_R3) >> 3) | ((empty & MASK_R5) >> 5)) & BP);
	if (BK) {
		numMoves += BitCount((empty << 4) & BK);
		numMoves += BitCount((((empty & MASK_L3) << 3) | ((empty & MASK_L5) << 5)) & BK);
	}

	return numMoves;
}

// Returns non-zero of any of the pieces in the checkers bitboard can move forward
int SCheckerBoard::CanWhiteCheckerMove(UINT checkers)
{
	return((empty << 4) & checkers) | ((((empty & MASK_L3) << 3) | ((empty & MASK_L5) << 5)) & checkers);
}

int SCheckerBoard::CanBlackCheckerMove(UINT checkers)
{
	return((empty >> 4) & checkers) | ((((empty & MASK_R3) >> 3) | ((empty & MASK_R5) >> 5)) & checkers);
}

// Return a list of white pieces that have a possible jump move
// To do this we start with a bitboard of empty squares and shift it to check each direction for a black piece,

// then shift the passing bits to find the white pieces with a jump. Moveable pieces from the different direction checks are or'd together.
UINT SCheckerBoard::GetJumpersWhite()
{
	const UINT WK = WP & K; // WK = White Kings bitboard
	UINT Movers = 0;
	UINT Temp = (empty << 4) & BP;
	Movers |= (((Temp & MASK_L3) << 3) | ((Temp & MASK_L5) << 5));
	Temp = (((empty & MASK_L3) << 3) | ((empty & MASK_L5) << 5)) & BP;
	Movers |= (Temp << 4);
	Movers &= WP;
	if (WK) {
		Temp = (empty >> 4) & BP;
		Movers |= (((Temp & MASK_R3) >> 3) | ((Temp & MASK_R5) >> 5)) & WK;
		Temp = (((empty & MASK_R3) >> 3) | ((empty & MASK_R5) >> 5)) & BP;
		Movers |= (Temp >> 4) & WK;
	}

	return Movers;
}

UINT SCheckerBoard::GetJumpersBlack()
{
	const UINT BK = BP & K;
	UINT Movers = 0;
	UINT Temp = (empty >> 4) & WP;
	Movers |= (((Temp & MASK_R3) >> 3) | ((Temp & MASK_R5) >> 5));
	Temp = (((empty & MASK_R3) >> 3) | ((empty & MASK_R5) >> 5)) & WP;
	Movers |= (Temp >> 4);
	Movers &= BP;
	if (BK) {
		Temp = (empty << 4) & WP;
		Movers |= (((Temp & MASK_L3) << 3) | ((Temp & MASK_L5) << 5)) & BK;
		Temp = (((empty & MASK_L3) << 3) | ((empty & MASK_L5) << 5)) & WP;
		Movers |= (Temp << 4) & BK;
	}

	return Movers;
}

void SCheckerBoard::successor(SMove &move, int color)
{
	int SideToMove = other_color(color);
	unsigned int src = (move.SrcDst & 63);
	unsigned int dst = (move.SrcDst >> 6) & 63;
	unsigned int bJump = (move.SrcDst >> 12);
	unsigned int nPiece = GetPiece(src);	// FIXME : AVOID GET PIECE HERE
	
	if (bJump == 0) {

		// Update the bitboards
		UINT BitMove = S[src] | S[dst];
		if (SideToMove == BLACK)
			WP ^= BitMove;
		else
			BP ^= BitMove;
		if (nPiece & KING)
			K ^= BitMove;
		empty ^= BitMove;

		if (nPiece < KING) {
			CheckKing(src, dst, nPiece);
			return;
		}

		return;
	}

	DoSingleJump(src, dst, nPiece, SideToMove);
	if (nPiece < KING)
		CheckKing(src, dst, nPiece);

	// Double Jump?
	if (move.cPath[0] == 33)
		return;

	for (int i = 0; i < 8; i++) {
		int nDst = move.cPath[i];

		if (nDst == 33)
			break;

		DoSingleJump(dst, nDst, nPiece, SideToMove);
		dst = nDst;
	}

	if (nPiece < KING)
		CheckKing(src, dst, nPiece);
}

//

//	E N D    B I T B O A R D S

//


CMoveList g_Movelist[MAX_SEARCHDEPTH + 1];
SMove g_GameMoves[MAX_GAMEMOVES];

// -------------------------------------------------
// Find the Moves available on board, and store them in Movelist

// -------------------------------------------------
void CMoveList::FindMovesBlack(SCheckerBoard &C)
{
	UINT Jumpers = C.GetJumpersBlack();
	if (Jumpers)					// If there are any jump moves possible
		FindJumpsBlack(C, Jumpers); // We fill this movelist with the jump moves
	else
		FindNonJumpsBlack(C, C.GetMoversBlack());	// Otherwise we fill the movelist with non-jump moves
}

void CMoveList::FindMovesWhite(SCheckerBoard &C)
{
	UINT Jumpers = C.GetJumpersWhite();
	if (Jumpers)
		FindJumpsWhite(C, Jumpers);
	else
		FindNonJumpsWhite(C, C.GetMoversWhite());
}

// -------------------------------------------------
// These two functions only add non-jumps

// -------------------------------------------------
void CMoveList::FindNonJumpsWhite(SCheckerBoard &C, int Movers)
{
	Clear();

	while (Movers) {
		UINT sq = FindLowBit(Movers);						// Pop a piece from the list of of movable pieces
		Movers ^= S[sq];

		AddNormalMove(C, sq, nextSq[sq + (2 << 5)]);		// Check the 2 forward directions and add valid moves
		AddNormalMove(C, sq, nextSq[sq + (3 << 5)]);

		if (S[sq] & C.K) {

			// For Kings only
			AddNormalMove(C, sq, nextSq[sq + (0 << 5)]);	// Check the 2 backward directions and add valid moves
			AddNormalMove(C, sq, nextSq[sq + (1 << 5)]);
		}
	}
}

void CMoveList::FindNonJumpsBlack(SCheckerBoard &C, int Movers)
{
	Clear();

	while (Movers) {
		UINT sq = FindHighBit(Movers);
		Movers ^= S[sq];

		AddNormalMove(C, sq, nextSq[sq + (0 << 5)]);
		AddNormalMove(C, sq, nextSq[sq + (1 << 5)]);

		if (S[sq] & C.K) {
			AddNormalMove(C, sq, nextSq[sq + (2 << 5)]);
			AddNormalMove(C, sq, nextSq[sq + (3 << 5)]);
		}
	}
}

// -------------------------------------------------
// These two functions only add jumps

// -------------------------------------------------
void CMoveList::FindJumpsBlack(SCheckerBoard &C, int Movers)
{
	Clear();
	while (Movers) {
		UINT sq = FindHighBit(Movers);			// Pop a piece from the list of of movable pieces
		Movers ^= S[sq];

		int oldEmpty = C.empty;					// Temporarly mark the piece square as empty to allow cyclic jump moves
		C.empty |= S[sq];

		CheckJumpDirBlack(C, sq, (0 << 5));		// Check the two forward directions
		CheckJumpDirBlack(C, sq, (1 << 5));
		if (C.K & S[sq]) {

			// For Kings only
			CheckJumpDirBlack(C, sq, (2 << 5)); // Check the two backward directions
			CheckJumpDirBlack(C, sq, (3 << 5));
		}

		C.empty = oldEmpty;
	}

	numMoves = numJumps;
}

void CMoveList::FindJumpsWhite(SCheckerBoard &C, int Movers)
{
	Clear();
	while (Movers) {
		UINT sq = FindLowBit(Movers);
		Movers ^= S[sq];

		int oldEmpty = C.empty;
		C.empty |= S[sq];

		CheckJumpDirWhite(C, sq, (2 << 5));
		CheckJumpDirWhite(C, sq, (3 << 5));
		if (C.K & S[sq]) {
			CheckJumpDirWhite(C, sq, (0 << 5));
			CheckJumpDirWhite(C, sq, (1 << 5));
		}

		C.empty = oldEmpty;
	}

	numMoves = numJumps;
}

// Add a move if the destination square is empty
void CMoveList::AddNormalMove(SCheckerBoard &C, int src, int dst)
{
	if (S[dst] & C.empty)
		AddMove(src, dst);
}

// Check for a jump
void CMoveList::CheckJumpDirWhite(SCheckerBoard &C, int square, const int DIR_INDEX)
{
	int jumpSquare = nextSq[square + DIR_INDEX];
	if (S[jumpSquare] & C.BP) {

		// If there is an opponent's piece in this direction
		int dstSq = nextSq[jumpSquare + DIR_INDEX];
		if (C.empty & S[dstSq]) {

			// And the next square in this direction is empty
			// Then a jump is possible, call FindSqJumps to detect double/triple/etc. jumps
			StartJumpMove(square, dstSq);
			FindSqJumpsWhite(C, dstSq, 0, jumpSquare, S[square] & C.K);
		}
	}
}

void inline CMoveList::CheckJumpDirBlack(SCheckerBoard &C, int square, const int DIR_INDEX)
{
	int jumpSquare = nextSq[square + DIR_INDEX];
	if (S[jumpSquare] & C.WP) {
		int dstSq = nextSq[jumpSquare + DIR_INDEX];
		if (C.empty & S[dstSq]) {
			StartJumpMove(square, dstSq);
			FindSqJumpsBlack(C, dstSq, 0, jumpSquare, S[square] & C.K);
		}
	}
}

// -------------
//  If a jump move was possible, we must make the jump then check to see if the same piece can jump again.
//  There might be multiple paths a piece can take on a double jump, these functions store each possible path as a move.

// -------------
int inline CMoveList::AddSqDirWhite(SCheckerBoard &C, int square, const UINT isKing, int pathNum, const int DIR_INDEX)
{
	int jumpSquare = nextSq[square + DIR_INDEX];
	if (S[jumpSquare] & C.BP) {
		int dstSq = nextSq[jumpSquare + DIR_INDEX];
		if (C.empty & S[dstSq]) {

			// jump is possible in this direction, see if the piece can jump more times
			m_JumpMove.cPath[pathNum] = dstSq;
			FindSqJumpsWhite(C, dstSq, pathNum + 1, jumpSquare, isKing);
			return 1;
		}
	}

	return 0;
}

int inline CMoveList::AddSqDirBlack(SCheckerBoard &C, int square, const UINT isKing, int pathNum, const int DIR_INDEX)
{
	int jumpSquare = nextSq[square + DIR_INDEX];
	if (S[jumpSquare] & C.WP) {
		int dstSq = nextSq[jumpSquare + DIR_INDEX];
		if (C.empty & S[dstSq]) {

			// jump is possible in this direction, see if the piece can jump more times
			m_JumpMove.cPath[pathNum] = dstSq;
			FindSqJumpsBlack(C, dstSq, pathNum + 1, jumpSquare, isKing);
			return 1;
		}
	}

	return 0;
}

void CMoveList::FindSqJumpsBlack(SCheckerBoard &C, int square, int pathNum, int jumpSquare, const UINT isKing)
{
	// Remove the jumped piece (until the end of this function), so we can't jump it again
	int oldWP = C.WP;
	C.WP ^= S[jumpSquare];

	// Now see if a piece on this square has more jumps
	int numMoves = AddSqDirBlack(C, square, isKing, pathNum, (0 << 5));
	numMoves += AddSqDirBlack(C, square, isKing, pathNum, (1 << 5));
	if (isKing) {

		// If this piece is a king, it can also jump backwards	
		numMoves += AddSqDirBlack(C, square, isKing, pathNum, (2 << 5));
		numMoves += AddSqDirBlack(C, square, isKing, pathNum, (3 << 5));
	}

	// if this is a leaf store the move
	if (numMoves == 0)
		AddJump(m_JumpMove, pathNum);

	// Put back the jumped piece
	C.WP = oldWP;
}

void CMoveList::FindSqJumpsWhite(SCheckerBoard &C, int square, int pathNum, int jumpSquare, const UINT isKing)
{
	int oldBP = C.BP;
	C.BP ^= S[jumpSquare];

	int numMoves = AddSqDirWhite(C, square, isKing, pathNum, (2 << 5));
	numMoves += AddSqDirWhite(C, square, isKing, pathNum, (3 << 5));

	if (isKing) {
		numMoves += AddSqDirWhite(C, square, isKing, pathNum, (0 << 5));
		numMoves += AddSqDirWhite(C, square, isKing, pathNum, (1 << 5));
	}

	if (numMoves == 0)
		AddJump(m_JumpMove, pathNum);

	C.BP = oldBP;
}


