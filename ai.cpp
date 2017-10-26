// ===============================================
//					 SEARCH
// ===============================================
// -------------------------------------------------
// Quiescence Search... Search all jumps, if there are no jumps, stop the search

// -------------------------------------------------
int QuiesceBoard(const int color, int ahead, int alpha, int beta)
{
	UINT jumpers = 0;
	if (color == WHITE) {
		jumpers = g_Boardlist[ahead - 1].C.GetJumpersWhite();
		if (jumpers)
			g_Movelist[ahead].FindJumpsWhite(g_Boardlist[ahead - 1].C, jumpers);
	}
	else {
		jumpers = g_Boardlist[ahead - 1].C.GetJumpersBlack();
		if (jumpers)
			g_Movelist[ahead].FindJumpsBlack(g_Boardlist[ahead - 1].C, jumpers);
	}

	// No more jump moves, return evaluation
	if (jumpers == 0 || (ahead + 1) >= MAX_SEARCHDEPTH) {
		if (ahead + 1 > g_SelectiveDepth)
			g_SelectiveDepth = ahead - 1;

		return g_Boardlist[ahead - 1].EvaluateBoard(ahead - 1, alpha, beta);
	}

	// There were jump moves, so we keep searching.
	for (int i = 1; i <= g_Movelist[ahead].numJumps; i++) {
		g_Boardlist[ahead] = g_Boardlist[ahead - 1];
		g_Boardlist[ahead].DoMove(g_Movelist[ahead].Moves[i], SEARCHED);
		nodes++;

		// Recursive Call
		int value = QuiesceBoard((color ^ 3), ahead + 1, alpha, beta);

		// Keep Track of Best Move and Alpha-Beta Prune
		if (color == WHITE && value > alpha) {
			alpha = value;
			if (alpha >= beta)
				return beta;
		}

		if (color == BLACK && value < beta) {
			beta = value;
			if (alpha >= beta)
				return alpha;
		}
	}

	return(color == WHITE) ? alpha : beta;
}

//
// Extensions
// fracDepth unit = 1/32 of a ply

//
void inline DoExtensions(int &nextDepth, int &fracDepth, int ahead, unsigned long nMove)
{
	unsigned int dst = (nMove >> 6) & 63;
	if (g_Movelist[ahead].numJumps == 1) {

		// Single move
		if (ahead > 1 && g_Movelist[ahead - 1].numJumps != 0)
			fracDepth += 22;	// after opponent jump
		else if (ahead > 2 && g_Movelist[ahead - 2].numJumps == 1)
			fracDepth += 17;	// we had to jump last move, now this move to
		else
			fracDepth += 8;
	}

	// Extend if the checker moves close to promotion (2 moves)
	else if
		(
			S[dst] & MASK_6TH &&
			g_Boardlist[ahead].GetPiece(dst) == BPIECE &&
			g_Boardlist[ahead].GetPiece(dst + 8) == EMPTY
		)
		fracDepth += 17;
	else if
		(
			S[dst] & MASK_3RD &&
			g_Boardlist[ahead].GetPiece(dst) == WPIECE &&
			g_Boardlist[ahead].GetPiece(dst - 8) == EMPTY
		)
		fracDepth += 17;

	// 2 Moves after opponent jump
	else if (g_Movelist[ahead].numJumps == 2 && ahead > 1 && g_Movelist[ahead - 1].numJumps != 0) {
		fracDepth += 16;
	}

	// Checker moves to 1 move from promotion
	else if
		(
			(dst > 23 && g_Boardlist[ahead].GetPiece(dst) == BPIECE) ||
			(dst < 8 && g_Boardlist[ahead].GetPiece(dst) == WPIECE)
		)
		fracDepth += 8;

	// increment nextDepth if extensions are over a ply
	if (fracDepth > 31) {
		fracDepth -= 32;
		nextDepth++;
	}
}

// -------------------------------------------------
//  Alpha Beta Search with Transposition(Hash) Table

// -------------------------------------------------
short ABSearch(int color, int ahead, int depth, int fracDepth, short alpha, short beta, int &bestmove)
{
	int i, value, nextbest, nM;
	unsigned long indexTT, checksumTT;
	short temp;

	// Check to see if move time has run out every 2,000 nodes
	if (nodes > nodes2 + 2000) {
		if (bCheckerBoard && *g_pbPlayNow)
			return TIMEOUT;
		if (g_bStopThinking)
			return TIMEOUT;
		endtime = clock();
		nodes2 = nodes;

		// If time has run out, we allow running up to 2*Time if g_bEndHard == FALSE and we are still searching a depth
		if ((endtime - starttime) > (CLOCKS_PER_SEC * fMaxSeconds)) {
			if
			(
				(endtime - starttime) > (2 * CLOCKS_PER_SEC * fMaxSeconds * g_fPanic) ||
				g_bEndHard == TRUE ||
				g_SearchingMove == 0 ||
				abs(g_SearchEval) > 1500
			) {
				return TIMEOUT;
			}
		}

		if ((endtime - lastTime) > (CLOCKS_PER_SEC * .4f)) {
			lastTime = endtime;
			RunningDisplay(-1, 1);
		}
	}

	// Use Internal Iterative Deepening to set bestmove if there's no best move
	if (bestmove == NONE && depth > 4) {
		value = ABSearch(color, ahead, depth - 4, 0, alpha, beta, bestmove);
		if (value == TIMEOUT)
			return(value);
	}

	// Find possible moves (and set a couple variables)
	if (color == WHITE) {
		g_Movelist[ahead].FindMovesWhite(g_Boardlist[ahead - 1].C);
	}
	else {
		g_Movelist[ahead].FindMovesBlack(g_Boardlist[ahead - 1].C);
	}

	// If you can't move, you lose the game
	if (g_Movelist[ahead].numMoves == 0) {
		if (color == WHITE)
			return -2001 + ahead;
		else
			return 2001 - ahead;
	}

	// Run through the g_Movelist, doing each move
	for (i = 0; i <= g_Movelist[ahead].numMoves; i++) {

		// if bestmove is set, try Best Move at i=0, and skip it later
		if (i == bestmove)
			continue;
		if (i == 0) {
			if (bestmove == NONE)
				continue;
			nM = bestmove;
		}
		else
			nM = i;

		if (ahead == 1) {
			g_SearchingMove = i;
			if (i > bestmove)
				g_SearchingMove--;

			// On dramatic changes (such as fail lows) make sure to finish the iteration
			int g_NewEval = (color == WHITE) ? alpha : beta;
			if (abs(g_NewEval) < 3000) {
				if (abs(g_NewEval - g_SearchEval) >= 26)
					g_fPanic = 1.8f;
				if (abs(g_NewEval - g_SearchEval) >= 50)
					g_fPanic = 2.5f;
				g_SearchEval = g_NewEval;
			}

			if (SearchDepth > 11 && !g_bStopThinking)
				RunningDisplay(bestmove, 1);
		}

		// Reset the move board to the one it was called with, then do the move # nM in the Movelist
		g_Boardlist[ahead] = g_Boardlist[ahead - 1];
		temp = g_Boardlist[ahead].DoMove(g_Movelist[ahead].Moves[nM], SEARCHED);
		AddRepBoard(g_Boardlist[ahead].HashKey, g_numMoves + ahead);
		nodes++;

		// If the game is over, return a gameover value now
		if (g_Boardlist[ahead].C.WP == 0 || g_Boardlist[ahead].C.BP == 0) {
			bestmove = nM;
			return g_Boardlist[ahead].EvaluateBoard(ahead, -100000, 100000);
		}

		// If this is the max depth, quiesce then evaluate the board position
		if ((depth <= 1 || ahead >= MAX_SEARCHDEPTH)) {
			value = QuiesceBoard((color ^ 3), ahead + 1, alpha, beta);
		}

		// If this is the reptition of a position that has occured already in the search, return a draw score
		else if (depth >= 4 && ahead > 1 && Repetition(g_Boardlist[ahead].HashKey, g_numMoves - 24, g_numMoves + ahead)) {
			value = 0;
		}

		//If this isn't the max depth continue to look ahead
		else {
			value = -9999;
			nextbest = NONE;

			int nextDepth = depth - 1;
			DoExtensions(nextDepth, fracDepth, ahead, g_Movelist[ahead].Moves[nM].SrcDst);

			// First look up the this Position in the Transposition Table
			if (nextDepth > 1 && hashing == 1) {
				indexTT = ((unsigned long)g_Boardlist[ahead].HashKey) % TTable_entries;
				checksumTT = (unsigned long)(g_Boardlist[ahead].HashKey >> 32);
				TTable[indexTT].Read(checksumTT, alpha, beta, nextbest, value, nextDepth, ahead);
			}

			// Then try Forward Pruning
			if (value == -9999 && nextDepth > 2) {
				int bForced = false;
				if (ahead > 1 && g_Movelist[ahead - 1].numMoves == 1 && g_Movelist[ahead].numMoves == 1)
					bForced = true;

				const int nMargin = 32;
				const int R = 3;
				g_Boardlist[ahead].eval = g_Boardlist[ahead].EvaluateBoard(ahead, alpha - 100, beta + 100);

				// Stop searching if we know the exact value from the database
				if (g_Boardlist[ahead].InDatabase(g_dbInfo)) {
					if
					(
						g_Boardlist[ahead].eval == 0 ||
						(g_dbInfo.type == DB_EXACT_VALUES && abs(g_Boardlist[ahead].eval) > 1800)
					) value = g_Boardlist[ahead].eval;
				}

				switch (color) {
				case BLACK:
					if (g_Boardlist[ahead].eval < alpha && alpha < 1500 && !bForced) {
						value = ABSearch((color ^ 3),
										 ahead + 1,
										 nextDepth - R,
										 fracDepth + 4,
										 alpha - nMargin - 1,
										 alpha - nMargin,
										 nextbest);
						if (value == TIMEOUT)
							return(value);
						if (value >= alpha - nMargin)
							value = -9999;
					}
					break;

				case WHITE:
					if (g_Boardlist[ahead].eval > beta && beta > -1500 && !bForced) {
						value = ABSearch((color ^ 3),
										 ahead + 1,
										 nextDepth - R,
										 fracDepth + 4,
										 beta + nMargin,
										 beta + nMargin + 1,
										 nextbest);
						if (value == TIMEOUT)
							return(value);
						if (value <= beta + nMargin)
							value = -9999;
					}
					break;
				}
			}

			// If value wasn't set from the Transposition Table or Pruning, look ahead nextDepth with a recursive call
			if (value == -9999 || ahead < 3) {

				// call ABSearch with opposite color & ahead incremented
				value = ABSearch((color ^ 3), ahead + 1, nextDepth, fracDepth, alpha, beta, nextbest);
				if (value == TIMEOUT)
					return value;

				// Store for the search info for this position in the Transposition Table
				if (nextDepth > 1 && hashing == 1) {
					TTable[indexTT].Write(checksumTT, alpha, beta, nextbest, value, nextDepth, ahead);
				}
			}
		}

		// Penalize moves at root that repeat positions, so hopefully the computer will always make progress if possible
		if (ahead == 1 && abs(value) < 1000) {
			if (Repetition(g_Boardlist[ahead].HashKey, 0, g_numMoves + 1))
				value = (value >> 1) + (value >> 2);
			else if (temp == 0) {
				if (color == WHITE)
					value -= 1;
				else
					value += 1;
			}	// if moves are the same, prefer the non-repeatable move
		}

		// Keep Track of Best Move and Alpha-Beta Prune
		if (color == WHITE && value > alpha) {
			bestmove = nM;
			alpha = value;
			if (alpha >= beta)
				return beta;
		}

		if (color == BLACK && value < beta) {
			bestmove = nM;
			beta = value;
			if (alpha >= beta)
				return alpha;
		}
	}			// end for

	if (color == WHITE)
		return alpha;
	else
		return beta;
}

// -------------------------------------------------
// The computer calculates a move then updates g_CBoard.

// -------------------------------------------------
int ComputerMove(char cColor, CBoard &InBoard)
{
	int LastEval = 0, Eval, nps = 0, nDoMove = NONE, bestmove = NONE;
	CBoard TempBoard;
	g_ucAge++;					// TT

	// return if game is over
	if (InBoard.C.BP == 0 || InBoard.C.WP == 0)
		return 0;
	if (cColor == BLACK)
		g_Movelist[1].FindMovesBlack(InBoard.C);
	if (cColor == WHITE)
		g_Movelist[1].FindMovesWhite(InBoard.C);
	if (g_Movelist[1].numMoves == 0)
		return 0;				// game over
	srand((unsigned int)time(0));
	bestmove = rand() % g_Movelist[1].numMoves + 1;
	starttime = clock();
	endtime = starttime;
	nodes = 0;
	nodes2 = 0;
	databaseNodes = 0;
	g_SelectiveDepth = 0;

	if (use_opendb)
		g_SearchEval = pBook->GetMove(InBoard, bestmove);
	else
		g_SearchEval = NOVAL;

	if (bestmove != NONE) {
		nDoMove = bestmove;
		SearchDepth = 0;
		g_SearchingMove = 0;
	}

	if (g_SearchEval == NOVAL) {

		// Make sure the repetition checker has all the values needed.
		if (!bCheckerBoard)
			ReplayGame(g_numMoves, InBoard);

		// Initialize variables (node count, start time, search depth)
		if (g_MaxDepth < 4)
			SearchDepth = g_MaxDepth - 2;
		else
			SearchDepth = 0;
		Eval = 0;

		// Iteratively deepen until until the computer runs out of time, or reaches g_MaxDepth ply
		while (SearchDepth < g_MaxDepth && Eval != TIMEOUT) {
			g_fPanic = 1.0f;	// multiplied max time by this, increased when search fails low
			SearchDepth += 2;

			g_Boardlist[0] = InBoard;
			g_Boardlist[0].HashKey = TEntry::HashBoard(g_Boardlist[0]);
			Eval = ABSearch(cColor, 1, SearchDepth, 0, -4000, 4000, bestmove);
			if (bestmove != NONE)
				nDoMove = bestmove;

			if (Eval != TIMEOUT) {
				g_SearchEval = LastEval = Eval;
				TempBoard = g_Boardlist[0];

				// Check if there is only one legal move, if so don't keep searching
				if (g_Movelist[1].numMoves == 1 && g_MaxDepth > 6)
					Eval = TIMEOUT;
				if
				(
					(clock() - starttime) > (CLOCKS_PER_SEC * new_iter_maxtime) // probably won't get any useful info before timeup
					||
					(abs(Eval) > 2001 - SearchDepth)
				) {

					// found a win, can stop searching now)
					Eval = TIMEOUT;
					g_SearchingMove++;
				}
			}
			else {
				if (abs(g_SearchEval) < 3000)
					LastEval = g_SearchEval;
			}
		}
	}

	if ((clock() - starttime) < (CLOCKS_PER_SEC / 4) && !bCheckerBoard)
		Sleep(200);				// pause a bit if move is really quick
	if (bCheckerBoard && nDoMove == NONE)
		nDoMove = 1;
	if (nDoMove != NONE) {
		InBoard.DoMove(g_Movelist[1].Moves[nDoMove], MAKEMOVE);
		g_GameMoves[g_numMoves++] = g_Movelist[1].Moves[nDoMove];
		g_GameMoves[g_numMoves].SrcDst = 0;
	}

	if (!g_bStopThinking)
		RunningDisplay(bestmove, 0);
	if (!bCheckerBoard) {
		DrawBoard(InBoard);
	}

	return LastEval;
}
