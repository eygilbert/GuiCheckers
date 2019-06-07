#include <windows.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <limits.h>
#include <io.h>
#include <setjmp.h>
#include "board.h"
#include "movegen.h"
#include "egdb.h"
#include "egdb_utils.h"


/*
 * Return true if a db lookup is possible based only on the number of men and kings present
 * (ignoring capture restrictions).
 */
bool EGDB_INFO::is_lookup_possible_pieces(int npieces, int npieces_1side)
{
	return(npieces <= dbpieces && npieces_1side <= dbpieces_1side);
}


bool EGDB_INFO::is_lookup_possible(BOARD *board, int color)
{
	int npieces = BitCount(board->BP | board->WP);
	int npieces_1side = max(BitCount(board->BP), BitCount(board->WP));
	if (!is_lookup_possible_pieces(npieces, npieces_1side))
		return(false);
	if (canjump(board, color))
		return(false);
	if (canjump(board, other_color(color)))
		return(false);

	return(true);
}


int EGDB_INFO::negate(int value)
{
	if (value == EGDB_WIN)
		return(EGDB_LOSS);
	if (value == EGDB_DRAW)
		return(EGDB_DRAW);
	if (value == EGDB_LOSS)
		return(EGDB_WIN);
	if (value == EGDB_WIN_OR_DRAW)
		return(EGDB_DRAW_OR_LOSS);
	if (value == EGDB_DRAW_OR_LOSS)
		return(EGDB_WIN_OR_DRAW);
	return(value);		/* EGDB_UNKNOWN or EGDB_UNAVAILABLE */
}


bool EGDB_INFO::is_greater_or_equal(int left, int right)
{
	if (right == EGDB_WIN) {
		if (left == EGDB_WIN)
			return(true);
		else
			return(false);
	}
	if (right == EGDB_DRAW) {
		if (left == EGDB_DRAW || left == EGDB_WIN_OR_DRAW || left == EGDB_WIN)
			return(true);
		else
			return(false);
	}
	if (right == EGDB_LOSS)
		return(true);
	if (right == EGDB_WIN_OR_DRAW) {
		if (left == EGDB_WIN_OR_DRAW || left == EGDB_WIN)
			return(true);
		else
			return(false);
	}
	if (right == EGDB_DRAW_OR_LOSS) {
		if (left == EGDB_WIN || left == EGDB_WIN_OR_DRAW || left == EGDB_DRAW || left == EGDB_DRAW_OR_LOSS)
			return(true);
		else
			return(false);
	}
	if (right == EGDB_UNKNOWN || right == EGDB_SUBDB_UNAVAILABLE) {
		if (left == EGDB_LOSS)
			return(false);
		else
			return(true);
	}

	/* Should never fall through here. */
	return(true);
}


bool EGDB_INFO::is_greater(int left, int right)
{
	if (right == EGDB_WIN) {
		return(false);
	}
	if (right == EGDB_DRAW) {
		if (left == EGDB_WIN_OR_DRAW || left == EGDB_WIN)
			return(true);
		else
			return(false);
	}
	if (right == EGDB_LOSS) {
		if (left == EGDB_LOSS)
			return(false);
		else
			return(true);
	}
	if (right == EGDB_WIN_OR_DRAW) {
		if (left == EGDB_WIN)
			return(true);
		else
			return(false);
	}
	if (right == EGDB_DRAW_OR_LOSS) {
		if (left == EGDB_LOSS)
			return(false);
		else
			return(true);
	}

	/* right must be EGDB_UNKNOWN or EGDB_SUBDB_UNAVAILABLE */
	if (left == EGDB_LOSS || left == EGDB_UNKNOWN || left == EGDB_SUBDB_UNAVAILABLE)
		return(false);

	return(true);
}


int EGDB_INFO::bestvalue_improve(int value, int bestvalue)
{
	if (bestvalue == EGDB_WIN) {
		return(EGDB_WIN);
	}
	if (bestvalue == EGDB_DRAW) {
		if (value == EGDB_WIN_OR_DRAW || value == EGDB_WIN)
			return(value);
		if (value == EGDB_UNKNOWN || value == EGDB_SUBDB_UNAVAILABLE)
			return(EGDB_WIN_OR_DRAW);
		return(bestvalue);
	}
	if (bestvalue == EGDB_LOSS)
		return(value);
	if (bestvalue == EGDB_WIN_OR_DRAW) {
		if (value == EGDB_WIN)
			return(value);
		else
			return(bestvalue);
	}
	if (bestvalue == EGDB_DRAW_OR_LOSS) {
		if (value == EGDB_LOSS)
			return(bestvalue);
		else
			return(value);
	}
	if (bestvalue == EGDB_UNKNOWN || bestvalue == EGDB_SUBDB_UNAVAILABLE) {
		if (value == EGDB_WIN_OR_DRAW || value == EGDB_WIN)
			return(value);
		if (value == EGDB_DRAW)
			return(EGDB_WIN_OR_DRAW);
	}
	return(bestvalue);
}


bool is_repetition(BOARD *history, BOARD *p, int depth)
{
	int i;
	uint32_t men, earlier_men;

	men = (p->BP | p->WP) & ~p->K;
	for (i = depth - 4; i >= 0; i -= 4) {

		/* Stop searching if a man (not king) was moved of either color. */
		earlier_men = (history[i].BP | history[i].WP) & ~history[i].K;
		if (men ^ earlier_men)
			break;

		if (!memcmp(p, history + i, sizeof(*p)))
			return(true);
	}

	return(false);
}


/*
 * Caution: this function is not thread-safe.
 */
int EGDB_INFO::lookup_with_rep_check(BOARD *p, int color, int depth, int maxdepth, int alpha, int beta, bool force_root_search)
{
	int i;
	int movecount;
	int capture;
	int value, bestvalue;
	BOARD next_board;
	CMoveList movelist;

	rep_check_positions[depth] = *p;
	if (++nodes > maxnodes)
		longjmp(env, 1);

	if (timeout && (nodes & 127) == 127)
		if ((clock() - t0) > timeout)
			longjmp(env, 1);

	/* Check for one side with no pieces. */
	if (p->BP == 0)
		if (color == BLACK)
			return(EGDB_LOSS);
		else
			return(EGDB_WIN);
	if (p->WP == 0)
		if (color == BLACK)
			return(EGDB_WIN);
		else
			return(EGDB_LOSS);

	/* Check for draw by repetition. */
	if (is_repetition(rep_check_positions, p, depth))
		return(EGDB_DRAW);

	capture = 0;
	if (canjump(p, color))
		capture = 1;
	else if (canjump(p, other_color(color)) == 0) {
		if (depth != 0 || !force_root_search) {
			EGDB_BITBOARD kingsrow_pos;

			gui_2_kingsrow_pos(kingsrow_pos, *p);
			value = (*handle->lookup)(handle, &kingsrow_pos, gui_2_kingsrow_color(color), 0);
			if (value != EGDB_UNKNOWN)
				return(value);
		}
	}

	if (depth > maxdepth_reached)
		maxdepth_reached = depth;

	if (depth >= maxdepth) {
		return(EGDB_UNKNOWN);
	}

	/* Can't lookup this pos.  Have to do a search. */
	movecount = movelist.build_movelist(*p, color);

	/* If no moves then its a loss. */
	if (movecount == 0)
		return(EGDB_LOSS);

	/* look up all successors. */
	int bestmove = 0;
	bestvalue = EGDB_LOSS;
	for (i = 1; i <= movecount; ++i) {
		next_board = *p;
		next_board.successor(movelist.Moves[i], color);
		value = negate(lookup_with_rep_check(&next_board, other_color(color), depth + 1, maxdepth, negate(beta), negate(alpha), force_root_search));
		log_tree(value, depth + 1, color);
		if (is_greater_or_equal(value, beta)) {
			bestvalue = bestvalue_improve(value, beta);
			bestmove = i;
			break;
		}

		bestvalue = bestvalue_improve(value, bestvalue);
		if (is_greater(bestvalue, alpha)) {
			alpha = bestvalue;
			bestmove = i;
		}
	}
	return(bestvalue);
}


/*
 * Lookup the value of a position in the db.
 * It may require a search if the position is a capture,
 * or if the position has a non-side capture and this slice of the db doesn't store those positions,
 * or if the function is called with force_root_search set true. This is used for self-verification. It
 * forces the value to be computed from the successor positions even if a direct lookup of the 
 * root position is possible.
 * Because some slices have neither positions with captures nor non-side captures, it is possible that the search has to go quite 
 * deep to resolve the root value, and can take a long time. There are 2 limits to control this. The maxdepth argument
 * limits the depth of the lookup search, and there is also a maxnodes limit. If either of these limits are hit,
 * the search returns the value EGDB_UNKNOWN.
 */
int EGDB_INFO::lookup_with_search(BOARD *p, int color, bool force_root_search)
{
	int status, i;
	int value = EGDB_UNKNOWN;	// prevent using unitialized memory

	nodes = 0;
	t0 = clock();
	reset_maxdepth();

	status = setjmp(env);
	if (status) {
		char buf[150];

		sprintf(buf, "search aborted; nodes: %d, maxdepth %d; time %d\n", nodes, get_maxdepth(), clock() - t0);
		printf(buf);
		if (msg_fn)
			(*msg_fn)(buf);
		return(EGDB_UNKNOWN);
	}

	for (i = 1; i < MAXREPDEPTH; ++i) {
		value = lookup_with_rep_check(p, color, 0, i, EGDB_LOSS, EGDB_WIN, force_root_search);
		if (value == EGDB_WIN)
			break;
		if (value == EGDB_DRAW)
			break;
		if (value == EGDB_LOSS)
			break;
		if (value == EGDB_SUBDB_UNAVAILABLE) {
			value = EGDB_UNKNOWN;
			break;
		}
	}

	if (msg_fn) {
		char buf[150];
		sprintf(buf, "lookup_with_search(): nodes %d, maxdepth %d, time %d\n", nodes, get_maxdepth(), clock() - t0);
		(*msg_fn)(buf);
	}
	return(value);
}


void EGDB_INFO::log_tree(int value, int depth, int color)
{
#if 0
	int i;
	bool white_is_even;
	char buf[50];

	if (color == WHITE)
		if (depth & 1)
			white_is_even = false;
		else
			white_is_even = true;
	else
		if (depth & 1)
			white_is_even = true;
		else
			white_is_even = false;

	for (i = 1; i <= depth; ++i) {
		if (white_is_even) {
			if (i & 1)
				color = BLACK;
			else
				color = WHITE;
		}
		else {
			if (i & 1)
				color = WHITE;
			else
				color = BLACK;
		}

		print_move(rep_check_positions + i - 1, rep_check_positions + i, color, buf);
		printf("%s, ", buf);
	}

	printf("value %d, depth %d\n", value, depth);
#endif
}


inline int black_tempo(uint32_t bm)
{
	return((4 * BitCount((MASK_5TH | MASK_6TH | MASK_7TH) & (bm))) +
		(2 * BitCount((MASK_3RD | MASK_4TH | MASK_7TH) & (bm))) +
		BitCount((MASK_2ND | MASK_4TH | MASK_6TH) & (bm)));
}


inline int white_tempo(uint32_t wm)
{
	return((4 * BitCount((MASK_4TH | MASK_3RD | MASK_2ND) & (wm))) +
		(2 * BitCount((MASK_6TH | MASK_5TH | MASK_2ND) & (wm))) +
		BitCount((MASK_7TH | MASK_5TH | MASK_3RD) & (wm)));
}


const uint32_t center_squares = sq10 | sq11 | sq14 | sq15 | sq18 | sq19 | sq22 | sq23;
const uint32_t edge_squares = sq1 | sq2 | sq3 | sq4 | sq12 | sq20 | sq28 | sq32 | sq31 | sq30 | sq29 | sq21 | sq13 | sq5;

int egdb_black_win_eval(BOARD *board)
{
	int nbm, nbk, nwm, nwk, tempo, edge, center;
	int value;

	nbm = BitCount(board->BP & ~board->K);
	nbk = BitCount(board->BP & board->K);
	nwm = BitCount(board->WP & ~board->K);
	nwk = BitCount(board->WP & board->K);
	edge = BitCount((board->WP & board->K) & edge_squares);
	center = BitCount((board->BP & board->K) & center_squares);
	tempo = black_tempo(board->BP & ~board->K);

	/* If equal material, black wants white men to advance. */
	if (nbm + nbk == nwm + nwk)
		tempo += white_tempo(board->WP & ~board->K);

	value = -1700 + 60 * (nbm + nwm) + 30 * (nbk + nwk);
	value -= tempo;
	value -= (center + edge);
	return(value);
}


int egdb_white_win_eval(BOARD *board)
{
	int nbm, nbk, nwm, nwk, tempo, edge, center;
	int value;

	nbm = BitCount(board->BP & ~board->K);
	nbk = BitCount(board->BP & board->K);
	nwm = BitCount(board->WP & ~board->K);
	nwk = BitCount(board->WP & board->K);
	edge = BitCount((board->BP & board->K) & edge_squares);
	center = BitCount((board->WP & board->K) & center_squares);
	tempo = white_tempo(board->WP & ~board->K);

	/* If equal material, white wants black men to advance. */
	if (nbm + nbk == nwm + nwk)
		tempo += black_tempo(board->BP & ~board->K);

	value = 1700 - 60 * (nbm + nwm) - 30 * (nbk + nwk);
	value += tempo;
	value += (center + edge);
	return(value);
}


int egdb_eval(BOARD *board, int color, int egdb_value)
{
	/* Eval for a BLACK color, db win. */
	if (color == BLACK) {
		switch (egdb_value) {
		case EGDB_WIN:
			/* Black win. */
			egdb_value = egdb_black_win_eval(board);
			return(egdb_value);

		case EGDB_LOSS:
			/* White win. */
			egdb_value = egdb_white_win_eval(board);
			return(egdb_value);

		default:
			return(0);
		}
	}
	else {
		/* color WHITE */
		switch (egdb_value) {
		case EGDB_WIN:
			/* White win. */
			egdb_value = egdb_white_win_eval(board);
			return(egdb_value);

		case EGDB_LOSS:
			/* Black win. */
			egdb_value = egdb_black_win_eval(board);
			return(egdb_value);

		default:
			return(0);
		}
	}
}



