#pragma once
#include <setjmp.h>
#include <stdint.h>
#include "egdb.h"
#include "movegen.h"

#define MAXREPDEPTH 64

struct MATERIAL {
	int npieces;
	int npieces_1_side;
};

typedef SCheckerBoard BOARD;
typedef uint32_t BITBOARD;

inline bool canjump(BOARD *board, int color)
{
	if (color == BLACK)
		return(board->GetJumpersBlack());
	else
		return(board->GetJumpersWhite());
}

class EGDB_INFO {

public:
	EGDB_INFO(void) { 
		maxnodes = 100000; 
		clear();
	}
	void clear(void) {
		handle = 0;
		dbpieces = 0;
		dbpieces_1side = 0;
		maxdepth_reached = 0;
		msg_fn = NULL;
		timeout = 0;
	}
	bool is_lookup_possible_pieces(MATERIAL *mat);
	bool is_lookup_possible(BOARD *board, int color, MATERIAL *mat);
	int lookup_with_search(BOARD *p, int color, bool force_root_search);
	int get_maxdepth() { return(maxdepth_reached); }
	void set_maxnodes(int nodes) { maxnodes = nodes; }
	int nodecount() { return(nodes); }
	void reset_maxdepth() { maxdepth_reached = 0; }
	void register_msgfn(void(__cdecl *fn)(char *)) { msg_fn = fn; }
	void set_search_timeout(int msec) { timeout = msec; }

	EGDB_DRIVER *handle;
	int dbpieces;
	int dbpieces_1side;

private:
	int timeout;
	int t0;
	int maxnodes;
	int nodes;
	int maxdepth_reached;
	int lookup_with_rep_check(BOARD *p, int color, int depth, int maxdepth, int alpha, int beta, bool force_root_search);
	int negate(int value);
	bool is_greater_or_equal(int left, int right);
	bool is_greater(int left, int right);
	int bestvalue_improve(int value, int bestvalue);
	void log_tree(int value, int depth, int color);
	jmp_buf env;
	void(__cdecl *msg_fn)(char *);
	BOARD rep_check_positions[MAXREPDEPTH + 1];
};

bool is_repetition(BOARD *history, BOARD *p, int depth);
