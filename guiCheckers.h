// Gui Checkers 1.10+
// by Jonathan Kreuzer
// copyright 2005
// http://www.3dkingdoms.com/checkers.htm
// I compiled with Microsoft Visual C++ Professional Edition & profile-guided optimizations
//
// NOTICES & CONDITIONS
//
// Commercial use of this code, in whole or in any part, is prohibited.
// Non-commercial use is welcome, but you should clearly mention this code, and where it came from.
// www.3dkingdoms.com/checkers.htm
//
// These conditions apply to the code itself only.
// You're welcome to read this code and learn from it without restrictions.

#pragma once
#include <Windows.h>
#include <time.h>

#define WHITE			2
#define BLACK			1				// Black == Red
#define EMPTY			0
#define BPIECE			1
#define WPIECE			2
#define KING			4
#define BKING			5
#define WKING			6
#define INVALID			8
#define NONE			255
#define TIMEOUT			31000
#define HUMAN			128
#define MAKEMOVE		129
#define SEARCHED		127
#define MAX_SEARCHDEPTH 96
#define MAX_GAMEMOVES	2048

// Comment out this line of code if you want GUI Checkers to play exactly like version 1.10
#define USE_ED_TRICE_CODE

//#define LOG_TIME_MGMT

enum eMoveResult { INVALID_MOVE = 0, VALID_MOVE = 1, DOUBLEJUMP = 2000 };

extern const int INVALID_VALUE;

// GLOBAL VARIABLES... ugg, too many?
extern char *g_sNameVer; // Checkerboard doesn't handle the longer name well
extern float fMaxSeconds;				// The number of seconds the computer is allowed to search
extern double new_iter_maxtime;			// Threshold for starting another iteration of iterative deepening
extern int g_bEndHard;					// Set to true to stop search after fMaxSeconds no matter what.
extern int g_MaxDepth;
extern int use_opendb;

#ifdef USE_ED_TRICE_CODE
extern unsigned long long nodes, nodes2;
extern unsigned long long databaseNodes;
#else
extern int nodes, nodes2;
extern int databaseNodes;
#endif
extern int SearchDepth, g_SelectiveDepth;
extern clock_t starttime, endtime, lastTime;
extern int g_SearchingMove, g_SearchEval;
extern int g_numMoves;						// Number of moves played so far in the current game
extern int bCheckerBoard;
extern int *g_pbPlayNow, g_bStopThinking;

struct CBoard;
struct SMove;
struct CMoveList;

// GUI function definitions
LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
INT_PTR CALLBACK InfoProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
void DrawBitmap(HDC hdc, HBITMAP bitmap, int x, int y, int nSize);
void HighlightSquare(HDC hdc, int Square, int sqSize, int bRedraw, unsigned long color, int border);
void DisplayText(const char *sText);
void DrawBoard(const CBoard &board);
void DisplaySearchInfo(CBoard &InBoard, int SearchDepth, int Eval, unsigned long long nodes);
void RunningDisplay(int bestmove, int bSearching);
void ReplayGame(int nMove, CBoard &Board);
int MakeMovePDN(CBoard &Board, int src, int dst);
int WINAPI enginecommand(char str[256], char reply[1024]);

// GUI Data
extern int hashing;

// Endgame Database
enum dbType_e { DB_WIN_LOSS_DRAW, DB_EXACT_VALUES };

struct SDatabaseInfo
{
	SDatabaseInfo() {
		loaded = false;
	}
	int numPieces;
	int numWhite;
	int numBlack;
	dbType_e type;
	bool loaded;
};

extern SDatabaseInfo g_dbInfo;
extern char wld_path[260];
extern int enable_wld;


// -----------------------
// Display search information
// -----------------------
extern const char* GetNodeCount(unsigned long long count, int nBuf);
void RunningDisplay(int bestMove, int bSearching);
void DisplayText(const char* sText);

inline int other_color(int color)
{
	return(color ^ (BLACK | WHITE));
}

