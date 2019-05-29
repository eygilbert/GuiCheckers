// Gui Checkers by Jonathan Kreuzer
// copyright 2005, 2009
// =================================================
// I use my own bitboard representation detailed in movegen.cpp
// =================================================

#pragma once
#include "guiCheckers.h"
#include "movegen.h"


struct CBoard
{
	void StartPosition(int bResetRep);
	void Clear();
	void SetFlags();
	int EvaluateBoard(int ahead, int alpha, int beta);

	void ToFen(char *sFEN);
	int FromFen(char *sFEN);
	int FromPDN(char *sPDN);
	int ToPDN(char *sPDN);

	bool InDatabase(const SDatabaseInfo &dbInfo)
	{
		return(enable_wld && dbInfo.loaded && nWhite <= dbInfo.numWhite && nBlack <= dbInfo.numBlack);
	}

	int MakeMovePDN(int src, int dst);
	int DoMove(SMove &Move, int nType);
	void DoSingleJump(int src, int dst, int nPiece);
	void CheckKing(int src, int dst, int nPiece);
	void UpdateSqTable(int nPiece, int src, int dst);

	int inline GetPiece(int sq) const
	{
		if (S[sq] & C.BP) {
			return(S[sq] & C.K) ? BKING : BPIECE;
		}

		if (S[sq] & C.WP) {
			return(S[sq] & C.K) ? WKING : WPIECE;
		}

		if (sq >= 32)
			return INVALID;

		return EMPTY;
	}

	void SetPiece(int sq, int piece)
	{
		// Clear square first
		C.WP &= ~S[sq];
		C.BP &= ~S[sq];
		C.K &= ~S[sq];

		// Set the piece
		if (piece & WPIECE)
			C.WP |= S[sq];
		if (piece & BPIECE)
			C.BP |= S[sq];
		if (piece & KING)
			C.K |= S[sq];
	}

	// Data
	char Sqs105[48];	/* Gui Checkers version 1.05 board representation. */
	SCheckerBoard C;
	char nWhite, nBlack, SideToMove, extra;
	short nPSq, eval;
	unsigned __int64 HashKey;
};

extern CBoard g_CBoard;
extern CBoard g_StartBoard;

// Convert from 64 to 32 board
extern int BoardLoc[66];

// =================================================
//
//	 			TRANSPOSITION TABLE
//
// =================================================
extern unsigned char g_ucAge;

struct TEntry
{
// FUNCTIONS
public:
	void inline Read(unsigned long CheckSum, short alpha, short beta, int &bestmove, int &value, int depth, int ahead)
	{
		if (m_checksum == CheckSum) {

			//To be almost totally sure these are really the same position.
			int tempVal;

			// Get the Value if the search was deep enough, and bounds usable
			if (m_depth >= depth) {
				if (abs(m_eval) > 1800) {

					// This is a game ending value, must adjust it since it depends on the variable ahead
					if (m_eval > 0)
						tempVal = m_eval - ahead + m_ahead;
					if (m_eval < 0)
						tempVal = m_eval + ahead - m_ahead;
				}
				else
					tempVal = m_eval;
				switch (m_failtype) {
				case 0:
					value = tempVal;		// Exact value
					break;

				case 1:
					if (tempVal <= alpha)
						value = tempVal;	// Alpha Bound (check to make sure it's usuable)
					break;

				case 2:
					if (tempVal >= beta)
						value = tempVal;	//  Beta Bound (check to make sure it's usuable)
					break;
				}
			}

			// Otherwise take the best move from Transposition Table
			bestmove = m_bestmove;
		}
	}

	void inline Write(unsigned long CheckSum, short alpha, short beta, int &bestmove, int &value, int depth, int ahead)
	{
		if (m_age == g_ucAge && m_depth > depth && m_depth > 14)
			return; // Don't write over deeper entries from same search
		m_checksum = CheckSum;
		m_eval = value;
		m_ahead = ahead;
		m_depth = depth;
		m_bestmove = bestmove;
		m_age = g_ucAge;
		if (value <= alpha)
			m_failtype = 1;
		else if (value >= beta)
			m_failtype = 2;
		else
			m_failtype = 0;
	}

	static void Create_HashFunction();
	static unsigned __int64 HashBoard(const CBoard &Board);

// DATA
private:
	unsigned long m_checksum;
	short m_eval;
	short m_bestmove;
	char m_depth;
	char m_failtype, m_ahead;
	unsigned char m_age;
};

// transposition table globals.
extern TEntry *TTable;
extern size_t TTable_entries;
extern int TTable_mb;

bool set_ttable_size(int size_mb);

// Check for a repeated board
// (only checks every other move, because side to move must be the same)
extern __int64 RepNum[MAX_GAMEMOVES];

// with Hashvalue passed
int inline Repetition(const __int64 HashKey, int nStart, int ahead)
{
	int i;
	if (nStart > 0)
		i = nStart;
	else
		i = 0;

	if ((i & 1) != (ahead & 1))
		i++;

	ahead -= 2;
	for (; i < ahead; i += 2)
		if (RepNum[i] == HashKey)
			return TRUE;

	return FALSE;
}

void inline AddRepBoard(const __int64 HashKey, int ahead)
{
	RepNum[ahead] = HashKey;
}


// Flip square horizontally because the internal board is flipped.
inline long FlipX(int x)
{
	int y = x & 3;
	x ^= y;
	x += 3 - y;
	return x;
}


//
inline int GetFinalDst(SMove &Move)
{
	int sq = ((Move.SrcDst >> 6) & 63);
	if ((Move.SrcDst >> 12))
		for (int i = 0; i < 8; i++) {
			if (Move.cPath[i] == 33)
				break;
			sq = Move.cPath[i];
		}

	return sq;
}


