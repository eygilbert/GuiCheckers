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
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <memory.h>
#include <windows.h>
#include <assert.h>
#include "ai.h"
#include "edDatabase.h"
#include "guiCheckers.h"
#include "board.h"
#include "database.h"
#include "obook.h"
#include "resource.h"
#include "cb_interface.h"
#include "logfile.h"
#include "registry.h"
#include "egdb.h"
#include "egdb_utils.h"

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
// Max depth for each level
const int BEGINNER_DEPTH = 2;
const int NORMAL_DEPTH = 8;
const int EXPERT_DEPTH = 52;

const int INVALID_VALUE = -100000;

const int OFF = 0;

// GLOBAL VARIABLES... ugg, too many?
#ifdef USE_ED_TRICE_CODE
#ifdef DLL_BUILD
char *g_sNameVer = "Gui Checkers 1.11"; // Checkerboard doesn't handle the longer name well
#else
char *g_sNameVer = "Gui Checkers 1.11 August 31, 2016 (64-bit conversion by Ed Trice)";
#endif
#else
char *g_sNameVer = "Gui Checkers 1.10";
#endif
char *g_sInfo = NULL;
float fMaxSeconds = 2.0f;				// The number of seconds the computer is allowed to search
double new_iter_maxtime;				// Threshold for starting another iteration of iterative deepening
int g_bEndHard = FALSE;					// Set to true to stop search after fMaxSeconds no matter what.
int g_MaxDepth = EXPERT_DEPTH;
int use_opendb = 1;

#ifdef USE_ED_TRICE_CODE
unsigned long long nodes, nodes2;
unsigned long long databaseNodes = 0;
#else
int nodes, nodes2;
int databaseNodes = 0;
#endif
int SearchDepth, g_SelectiveDepth;
char g_CompColor = WHITE;
clock_t starttime, endtime, lastTime = 0;
int g_SearchingMove = 0, g_SearchEval = 0;
int g_nDouble = 0;
int g_numMoves = 0;						// Number of moves played so far in the current game
int g_bSetupBoard, g_bThinking = false;
int bCheckerBoard = 0;
int *g_pbPlayNow, g_bStopThinking = 0;

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

// GUI Data
int hashing = 1, BoardFlip = 0;
int g_xAdd = 44, g_yAdd = 20, g_nSqSize = 64;
HBITMAP PieceBitmaps[32];
HWND MainWnd = NULL;
HWND BottomWnd = NULL;
int g_nSelSquare = NONE;
char g_buffer[32768];					// For PDN copying/pasting/loading/saving

SDatabaseInfo g_dbInfo;
EGDB_INFO wld;
EGDB_DRIVER *aux;
EGDB_TYPE wld_egdb_type;
EGDB_TYPE aux_egdb_type;
int aux_pieces;
char wld_path[260] = "db_dtw";
char aux_path[260];
int enable_wld = 1;
int enable_aux;
int wld_cache_mb;
int max_dbpieces;
int did_egdb_init;
int request_wld_init;
int request_aux_init;

//------------------------
// includes
//------------------------
int QueryEdsDatabase(const CBoard &Board, int ahead);
int QueryGuiDatabase(const CBoard &Board);

COpeningBook *pBook = NULL;


// -----------------------
// Display search information
// -----------------------
#ifdef USE_ED_TRICE_CODE
const char *GetNodeCount(unsigned long long count, int nBuf)
{
	static char buffer[4][100];
	sprintf(buffer[nBuf], "%llu", count);
	return buffer[nBuf];
}

#else
const char *GetNodeCount(int count, int nBuf)
{
	static char buffer[4][100];

	if (count < 1000)
		sprintf(buffer[nBuf], "%d n", count);
	else if (count < 1000000)
		sprintf(buffer[nBuf], "%.2f kn", (float)count / 1000.0f);
	else
		sprintf(buffer[nBuf], "%.2f Mn", (float)count / 1000000.0f);

	return buffer[nBuf];
}
#endif
void RunningDisplay(int bestMove, int bSearching)
{
#ifdef USE_ED_TRICE_CODE
	int game_over_distance = 300;
#endif
	char sTemp[4096];
	static int LastEval = 0, nps = 0;
	static int LastBest;
	int j = 0;
	if (bestMove != -1) {
		LastBest = bestMove;
	}

	bestMove = g_Movelist[1].Moves[LastBest].SrcDst;
	if (!bCheckerBoard) {
		j += sprintf(sTemp + j, "Red: %d   White: %d                           ", g_CBoard.nBlack, g_CBoard.nWhite);
		if (g_MaxDepth == BEGINNER_DEPTH)
			j += sprintf(sTemp + j, "Level: Beginner  %ds  ", (int)fMaxSeconds);
		else if (g_MaxDepth == EXPERT_DEPTH)
			j += sprintf(sTemp + j, "Level: Expert  %ds  ", (int)fMaxSeconds);
		else if (g_MaxDepth == NORMAL_DEPTH)
			j += sprintf(sTemp + j, "Level: Advanced  %ds  ", (int)fMaxSeconds);
		if (bSearching)
			j += sprintf(sTemp + j, "(searching...)\n");
		else
			j += sprintf(sTemp + j, "\n");
	}

	float seconds = float(clock() - starttime) / CLOCKS_PER_SEC;
	if (seconds > 0.0f)
		nps = int(float(nodes) / (1000.0 * seconds));
	else
		nps = 0;
	if (abs(g_SearchEval) < 3000)
		LastEval = g_SearchEval;

	char cCap = (bestMove >> 12) ? 'x' : '-';

	// Also announce distance to win when the databases are returning high scores
#ifdef USE_ED_TRICE_CODE
	if (abs(2000 - abs(LastEval)) <= game_over_distance) {
		char *prefix, *suffix;

		if (bCheckerBoard) {
			prefix = "";
			suffix = "   ";
		}
		else {
			prefix = "#### ";
			suffix = " ####";
		}

		if (LastEval < 0) {
			if (g_CBoard.SideToMove == WHITE)
				j += sprintf(sTemp + j, "%sRed wins in %d%s", prefix, abs(2001 - abs(LastEval)), suffix);
			else
				j += sprintf(sTemp + j, "%sWhite loses in %d%s", prefix, abs(2001 - abs(LastEval)), suffix);
		}
		else {
			if (g_CBoard.SideToMove == WHITE)
				j += sprintf(sTemp + j, "%sRed loses in %d%s", prefix, abs(2001 - abs(LastEval)), suffix);
			else
				j += sprintf(sTemp + j, "%sWhite wins in %d%s", prefix, abs(2001 - LastEval), suffix);
		}
	}
	else {
		j += sprintf(sTemp + j,
					 "Depth: %d/%d (%d/%d)   Eval: %d  ",
					 SearchDepth,
					 g_SelectiveDepth,
					 g_SearchingMove,
					 g_Movelist[1].numMoves,
					 -LastEval);
	}
#endif
	if (!bCheckerBoard)
		j += sprintf(sTemp + j, "\n");

	j += sprintf(sTemp + j,
				 "Move: %d%c%d   Time: %.2fs   Speed %d KN/s   Nodes: %s (db: %s)   ",
				 FlipX(bestMove & 63) + 1,
				 cCap,
				 FlipX((bestMove >> 6) & 63) + 1,
				 seconds,
				 nps,
				 GetNodeCount(nodes, 0),
				 GetNodeCount(databaseNodes, 1));
	DisplayText(sTemp);
}

// ---------------------------------------------
//  Check Possiblity/Execute Move from one selected square to another
//  returns INVALID_MOVE if the move is not possible, VALID_MOVE if it is or DOUBLEJUMP if the move is an uncompleted jump

// ---------------------------------------------
int SquareMove(CBoard &Board, int x, int y, int xloc, int yloc, char Color)
{
	int i, nMSrc, nMDst;
	CMoveList MoveList;

	if (Color == BLACK)
		MoveList.FindMovesBlack(Board.C);
	else
		MoveList.FindMovesWhite(Board.C);

	int dst = BoardLoc[xloc + yloc * 8];
	int src = BoardLoc[x + y * 8];

	for (i = 1; i <= MoveList.numMoves; i++) {
		nMSrc = MoveList.Moves[i].SrcDst & 63;
		nMDst = (MoveList.Moves[i].SrcDst >> 6) & 63;
		if (nMSrc == src && nMDst == dst) {

			// Check if the src & dst match a move from the generated movelist
			if (g_nDouble > 0) {

				// Build double jump move for game transcript
				g_GameMoves[g_numMoves - 1].cPath[g_nDouble - 1] = nMDst;
				g_GameMoves[g_numMoves - 1].cPath[g_nDouble] = 33;
			}
			else {
				g_GameMoves[g_numMoves].SrcDst = MoveList.Moves[i].SrcDst;
				g_GameMoves[g_numMoves++].cPath[0] = 33;
				g_GameMoves[g_numMoves].SrcDst = 0;
			}

			if (Board.DoMove(MoveList.Moves[i], HUMAN) == DOUBLEJUMP) {
				Board.SideToMove ^= 3;	// switch SideToMove back to what it was
				g_nDouble++;
				return DOUBLEJUMP;
			}

			g_nDouble = 0;
			return VALID_MOVE;
		}
	}

	return INVALID_MOVE;
}

void UpdateMenuChecks()
{
	CheckMenuItem(GetMenu(MainWnd), ID_OPTIONS_BEGINNER, (g_MaxDepth == BEGINNER_DEPTH) ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(GetMenu(MainWnd), ID_OPTIONS_NORMAL, (g_MaxDepth == NORMAL_DEPTH) ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(GetMenu(MainWnd), ID_OPTIONS_EXPERT, (g_MaxDepth == EXPERT_DEPTH) ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(GetMenu(MainWnd), ID_GAME_HASHING, (hashing) ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(GetMenu(MainWnd), ID_OPTIONS_COMPUTERWHITE, (g_CompColor == WHITE) ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(GetMenu(MainWnd), ID_OPTIONS_COMPUTERBLACK, (g_CompColor == BLACK) ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(GetMenu(MainWnd), ID_GAME_COMPUTEROFF, (g_CompColor == OFF) ? MF_CHECKED : MF_UNCHECKED);
}

void SetComputerColor(int Color)
{
	g_CompColor = (char)Color;
	UpdateMenuChecks();
}

//
// New Game

//
void NewGame()
{
	g_nSelSquare = NONE;
	g_nDouble = 0;
	g_numMoves = 0;
	g_GameMoves[g_numMoves].SrcDst = 0;
	SetComputerColor(WHITE);

	DisplayText("The Game Begins...");
}

const char *GetInfoString()
{
	static char displayString[1024];
	if (g_dbInfo.loaded == false)
		sprintf(displayString, "No Database Loaded\n");
	else {
		sprintf(displayString,
				"Database : %d-piece %s\n",
				g_dbInfo.numPieces,
				g_dbInfo.type == DB_WIN_LOSS_DRAW ? "Win/Loss/Draw" : "Trice DTW");
	}

	if (pBook)
		sprintf(displayString + strlen(displayString), "Opening Book : %d positions\n", pBook->m_nPositions);
	else
		sprintf(displayString + strlen(displayString), "Opening Book not loaded yet\n");

	return displayString;
}

// ===============================================
//
//                 G U I   S T U F F
//

// ===============================================
void DisplayText(const char *sText)
{
	if (bCheckerBoard) {
		if (g_sInfo) {
			strcpy(g_sInfo, sText);
		}

		return;
	}

	if (BottomWnd)
		SetDlgItemText(BottomWnd, 150, sText);
}

// -------------------------------
// WINDOWS CLIPBOARD FUNCTIONS

// -------------------------------
int TextToClipboard(char *sText)
{
	DisplayText(sText);

	char *bufferPtr;
	static HGLOBAL clipTranscript;
	int nLen = (int)strlen(sText);
	if (OpenClipboard(MainWnd) == TRUE) {
		EmptyClipboard();
		clipTranscript = GlobalAlloc(GMEM_DDESHARE | GMEM_MOVEABLE, nLen + 10);
		bufferPtr = (char *)GlobalLock(clipTranscript);
		memcpy(bufferPtr, sText, nLen + 1);
		GlobalUnlock(clipTranscript);
		SetClipboardData(CF_TEXT, clipTranscript);
		CloseClipboard();
		return 1;
	}

	DisplayText("Cannot Open Clipboard");
	return 0;
}

int TextFromClipboard(char *sText, int nMaxBytes)
{
	if (!IsClipboardFormatAvailable(CF_TEXT)) {
		DisplayText("No Clipboard Data");
		return 0;
	}

	OpenClipboard(MainWnd);

	HANDLE hData = GetClipboardData(CF_TEXT);
	LPVOID pData = GlobalLock(hData);

	int nSize = (int)strlen((char *)pData);
	if (nSize > nMaxBytes)
		nSize = nMaxBytes;
	memcpy(sText, (LPSTR) pData, nSize);
	sText[nSize] = NULL;

	GlobalUnlock(hData);
	CloseClipboard();

	return 1;
}

// Gray out certain items
void ThinkingMenu(int bOn)
{
	int SwitchItems[10] =
	{
		ID_GAME_NEW,
		ID_FILE_LOADGAME,
		ID_EDIT_SETUPBOARD,
		ID_EDIT_PASTE_PDN,
		ID_EDIT_PASTE_POS,
		ID_GAME_CLEAR_HASH,
		ID_GAME_COMPUTEROFF,
		ID_OPTIONS_COMPUTERBLACK,
		ID_OPTIONS_COMPUTERWHITE,
		ID_GAME_HASHING
	};
	int nSet = (bOn) ? MF_GRAYED : MF_ENABLED;

	for (int i = 0; i < 8; i++) {
		EnableMenuItem(GetMenu(MainWnd), SwitchItems[i], nSet);
	}

	EnableMenuItem(GetMenu(MainWnd), ID_GAME_MOVENOW, (bOn) ? MF_ENABLED : MF_GRAYED);
}

// =================================================================================\
// Threaded Function (for multitasking while the computer thinks)
// =================================================================================/
HANDLE hEngineReady, hAction;
HANDLE hThread;

DWORD WINAPI ThinkingThread(void * /* param */ )
{
	hEngineReady = CreateEvent(NULL, TRUE, FALSE, NULL);

	// Think of Move
	while (1 == 1) {
		SetEvent(hEngineReady);
		WaitForSingleObject(hAction, INFINITE);
		ResetEvent(hEngineReady);

		SetComputerColor((char)g_CBoard.SideToMove);
		ComputerMove(g_CompColor, g_CBoard);
		g_bThinking = false;
		ThinkingMenu(false);
	}

	CloseHandle(hThread);
}

/*
 * Return the size of installed RAM in mbytes.
 */
int get_mem_installed()
{
	MEMORYSTATUSEX memstatus;
	int ramsize;

	memstatus.dwLength = sizeof(memstatus);
	if (GlobalMemoryStatusEx(&memstatus)) {

		/* Convert physical memory to mbytes. */
		ramsize = (int)(memstatus.ullTotalPhys / (1024 * 1024));
	}
	else
		ramsize = 256;			/* Just say something small. */
	return(ramsize);
}

void get_dflt_buffer_sizes(int mem_installed, int *dflt_egdb_cache, int *dflt_hashsize)
{
	int i, best_setting, smallest_diff;

	if (mem_installed > 16000) {
		*dflt_hashsize = 128;
		*dflt_egdb_cache = (int)mem_installed - *dflt_hashsize - 3500;
	}
	else if (mem_installed > 8000) {
		*dflt_hashsize = 128;
		*dflt_egdb_cache = (int)mem_installed - *dflt_hashsize - 2200;
	}
	else if (mem_installed > 6000) {
		*dflt_hashsize = 128;
		*dflt_egdb_cache = (int)mem_installed - *dflt_hashsize - 1900;
	}
	else if (mem_installed > 4000) {
		*dflt_hashsize = 128;
		*dflt_egdb_cache = (int)mem_installed - *dflt_hashsize - 1500;
	}
	else if (mem_installed > 2000) {
		*dflt_hashsize = 128;
		*dflt_egdb_cache = (int)mem_installed - *dflt_hashsize - 700;
	}
	else if (mem_installed < 250) {
		*dflt_hashsize = 32;
		*dflt_egdb_cache = (int)mem_installed - *dflt_hashsize - 150;
		if (*dflt_egdb_cache < 32)
			* dflt_egdb_cache = 32;
	}
	else if (mem_installed < 700) {
		*dflt_hashsize = 32;
		*dflt_egdb_cache = (int)mem_installed - *dflt_hashsize - 300;
		if (*dflt_egdb_cache < 32)
			* dflt_egdb_cache = 32;
	}
	else {
		/* Between 700 and 2000 mbytes. */
		*dflt_hashsize = 64;
		*dflt_egdb_cache = (int)mem_installed - *dflt_hashsize - 400;
	}
#ifndef _WIN64
	/* Assume that /3GB switch is not present for 32-bit version. */
	if (*dflt_egdb_cache > (1800 - *dflt_hashsize))
		* dflt_egdb_cache = 1800 - *dflt_hashsize;
#endif

	/* Max dflt cache size is 16384mb (diminishing returns from using more). */
	* dflt_egdb_cache = min(*dflt_egdb_cache, 16384);

	/* Round dflt_egdb_cache to the closest CheckerBoard setting. */
	best_setting = 0;
	smallest_diff = mem_installed;
	for (i = 0; i < mem_installed - 64; i += 32) {
		if (i > 16384 && (i % 1024))
			continue;
		if (i > 4096 && (i % 256))
			continue;
		if (abs(i - *dflt_egdb_cache) < smallest_diff) {
			smallest_diff = abs(i - *dflt_egdb_cache);
			best_setting = i;
		}
	}
	*dflt_egdb_cache = best_setting;
}


void log_egdb_mem_settings()
{
	log_msg("WLD cachesize: %d\n", wld_cache_mb);
	log_msg("limit max WLD pieces: %d\n", max_dbpieces);
	log_msg("WLD directory: %s\n", wld_path);
	log_msg("Aux directory: %s\n", aux_path);
	log_msg("WLD enable: %d\n", enable_wld);
	log_msg("Aux enable: %d\n\n", enable_aux);
}


//
// ENGINE INITILIZATION
//
// status_str is NULL if not called from CheckerBoard
//
void InitEngine(char *status_str)
{
	static bool did_init = false;
	int mem_installed, dflt_egdb_cache, dflt_hashsize;
	time_t aclock;
	struct tm* newtime;
	char msg[MAX_PATH + 30];
	char current_dir[MAX_PATH];

	/* Only call init once. */
	if (did_init)
		return;

	did_init = true;
	init_logfile("GuiCheckers", "GuiCheckers.log");
	time(&aclock);						/* Get time in seconds */
	newtime = localtime(&aclock);		/* Convert time to struct tm form */
	log_msg(asctime(newtime));

	/* Write engine name to the log file. */
	enginecommand("name", msg);
	log_msg("%s\n", msg);

	mem_installed = get_mem_installed();
	log_msg("installed RAM: %dmb\n", mem_installed);
	GetCurrentDirectory(sizeof(current_dir) - 1, current_dir);
	log_msg("current directory: %s\n", current_dir);

	InitBitTables();
	TEntry::Create_HashFunction();

	if (bCheckerBoard) {
		get_dbpath(wld_path, sizeof(wld_path));
		get_enable_wld(&enable_wld);
		get_book_setting(&use_opendb);
		if (get_max_dbpieces(&max_dbpieces))
			save_max_dbpieces(24);		/* Set to no limit. */
		
		/* If cant find egdb cache size or hashtable size in registry, set them to reasonable defaults. */
		if (get_dbmbytes(&wld_cache_mb) || get_hashsize(&TTable_mb)) {
			get_dflt_buffer_sizes(mem_installed, &dflt_egdb_cache, &dflt_hashsize);
			wld_cache_mb = dflt_egdb_cache;
			save_dbmbytes(wld_cache_mb);			/* Have to save both this and hashsize to avoid using defaults on the next session. */
			TTable_mb = dflt_hashsize;
			save_hashsize(TTable_mb);
		}
	}
	if (set_ttable_size(TTable_mb))
		set_ttable_size(64);

	pBook = new COpeningBook;
#if 1
	if (!bCheckerBoard || !pBook->Load("engines\\opening.gbk"))
		pBook->Load("opening.gbk");
#else
	/* This code was used to convert the book hashcodes from version 1.05. 
	 * The codes changed because of a dependency on rand() and its startup usage.
	 */
	if (!bCheckerBoard || !pBook->LoadFEN("engines\\opening.fen"))
		pBook->LoadFEN("opening.fen");
#endif

	if (!g_dbInfo.loaded && enable_wld) {
		if (status_str)
			sprintf(status_str, "Initializing endgame db...");
		InitializeEdsDatabases(g_dbInfo);
	}

	g_CBoard.StartPosition(1);

	if (!bCheckerBoard) {
		static DWORD ThreadId;
		hAction = CreateEvent(NULL, FALSE, FALSE, NULL);
		hThread = CreateThread(NULL, 0, ThinkingThread, 0, 0, &ThreadId);
		if (hThread == NULL) {
			MessageBox(MainWnd, "Cannot Create Thread", "Error", MB_OK);
		}
	}
}


void init_egdb(char msg[255])
{
	if (!did_egdb_init || request_wld_init || request_aux_init) {
		log_egdb_mem_settings();
		if (!did_egdb_init || request_wld_init) {
			request_wld_init = 0;
			request_aux_init = 1;
			if (wld.handle) {
				wld.handle->close(wld.handle);
				wld.clear();
			}
			if (aux) {
				aux->close(aux);
				aux = 0;
			}
			if (enable_wld) {
				int egdb_found;

				egdb_found = !egdb_identify(wld_path, &wld_egdb_type, &wld.dbpieces);
				wld.dbpieces = min(max_dbpieces, wld.dbpieces);
				if (egdb_found) {
					log_msg("Found db type %d in %s\n", wld_egdb_type, wld_path);

					switch (wld_egdb_type) {
					case EGDB_KINGSROW32_WLD:
					case EGDB_KINGSROW32_WLD_TUN:
						sprintf(msg, "Wait for db init; Kingsrow db, %d pieces, %d mb cache, %d mb hashtable ...",
							wld.dbpieces, wld_cache_mb, TTable_mb);
						wld.handle = egdb_open(EGDB_NORMAL, wld.dbpieces, wld_cache_mb, wld_path, log_msg);
						wld.dbpieces_1side = 5;
						break;

					case EGDB_CAKE_WLD:
						sprintf(msg, "Wait for db init; Cake db, %d pieces, %d mb cache, %d mb hashtable ...",
							wld.dbpieces, wld_cache_mb, TTable_mb);
						wld.handle = egdb_open(EGDB_NORMAL, wld.dbpieces, wld_cache_mb, wld_path, log_msg);
						wld.dbpieces_1side = 4;
						break;

					case EGDB_CHINOOK_WLD:
						sprintf(msg, "Wait for db init; Chinook db, %d pieces, %d mb cache, %d mb hashtable ...",
							wld.dbpieces, wld_cache_mb, TTable_mb);
						wld.handle = egdb_open(EGDB_NORMAL, wld.dbpieces, wld_cache_mb, wld_path, log_msg);
						if (wld.handle) {

							/* See if he's got the full db, or just the 4x4. */
							if (wld.dbpieces > 6) {
								int val;
								BOARD testpos = { 3, 0xf1000000, 0 };

								val = (*wld.handle->lookup)(wld.handle, (EGDB_BITBOARD*)& testpos, EGDB_WHITE, 0);
								if (val == EGDB_WIN)
									wld.dbpieces_1side = 7;
								else
									wld.dbpieces_1side = 4;
							}
							else
								/* Use the full 6pc db. */
								wld.dbpieces_1side = 5;
						}
						break;

					case EGDB_KINGSROW32_ITALIAN_WLD:
					case EGDB_CHINOOK_ITALIAN_WLD:
					case EGDB_KINGSROW32_ITALIAN_WLD_TUN:
					default:
						wld.dbpieces = 0;
						return;
					}

					if (!wld.handle) {
						wld.clear();
						sprintf(msg, "Cannot open endgame db driver.");
						Sleep(3000);
						sprintf(msg, "See %s for details", logfilename());
						Sleep(10000);
					}
				}
				else {
					log_msg("Cannot find database in %s\n", wld_path);
					wld.clear();
					sprintf(msg, "Cannot find endgame database files.");
					Sleep(5000);
				}
			}
		}

		if (!did_egdb_init || request_aux_init) {
			request_aux_init = 0;
			if (aux) {
				aux->close(aux);
				aux = 0;
			}
			if (wld.handle && aux_path[0] && enable_aux) {
				sprintf(msg, "Opening aux db; %s", aux_path);
				if (!egdb_identify(aux_path, &aux_egdb_type, &aux_pieces)) {
					log_msg("Found db type %d in %s\n", aux_egdb_type, aux_path);
					switch (aux_egdb_type) {
					case EGDB_KINGSROW32_MTC:
					case EGDB_KINGSROW_DTW:
						aux_pieces = min(aux_pieces, wld.dbpieces);
						aux = egdb_open(EGDB_NORMAL, aux_pieces, 1, aux_path, log_msg);
						break;
					}
				}
				else {
					log_msg("Cannot find database in %s\n", aux_path);
				}
			}
			if (!aux)
				aux_pieces = 0;
		}
	}
	did_egdb_init = 1;
}

// INITIALIZATION FUNCTION
static int AnyInstance(HINSTANCE this_inst)
{
	// create main window
	MainWnd = CreateWindow("GuiCheckersClass",
						   g_sNameVer,
						   WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
						   CW_USEDEFAULT,
						   CW_USEDEFAULT,
						   620,
						   660, // x,y Position  x,y Size
						   NULL,
						   NULL,
						   this_inst,
						   NULL);
	if (MainWnd == NULL)
		return FALSE;

	BottomWnd = CreateDialog(this_inst, "BotWnd", MainWnd, InfoProc);

	// Load Piece Bitmaps
	char *psBitmapNames[10] = { NULL, "Rcheck", "Wcheck", NULL, NULL, "RKing", "Wking", "Wsquare", "Bsquare", NULL };
	for (int i = 0; i < 9; i++) {
		if (psBitmapNames[i] != NULL)
			PieceBitmaps[i] = (HBITMAP) LoadImage(this_inst, psBitmapNames[i], IMAGE_BITMAP, 0, 0, 0);
	}

	MoveWindow(BottomWnd, 0, 550, 680, 150, TRUE);
	ShowWindow(MainWnd, SW_SHOW);
	ThinkingMenu(false);

	g_CBoard.StartPosition(1);
	DrawBoard(g_CBoard);
	InitEngine(NULL);
	NewGame();
	DisplayText(GetInfoString());

	return(TRUE);
}

// ------------------------------------
// FirstInstance - register window class for the application,

// ------------------------------------
int RegisterClass(HINSTANCE this_inst)
{
	WNDCLASSEX wc;
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WindowProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = this_inst;
	wc.hIcon = LoadIcon(this_inst, "chIcon");
	wc.hIconSm = LoadIcon(this_inst, "chIcon");
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH) GetStockObject(DKGRAY_BRUSH);
	wc.lpszMenuName = "CheckMenu";
	wc.lpszClassName = "GuiCheckersClass";

	BOOL rc = RegisterClassEx(&wc);

	return rc;
}

// ------------------------------------
//  WinMain - initialization, message loop

// ------------------------------------
int WINAPI WinMain(HINSTANCE this_inst, HINSTANCE prev_inst, LPSTR cmdline, int cmdshow)
{
	if (!prev_inst)
		RegisterClass(this_inst);

	if (AnyInstance(this_inst) == FALSE)
		return(FALSE);

	MSG msg;
	while (GetMessage(&msg, NULL, NULL, NULL)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return((int)msg.wParam);
}

//---------------------------------------------------------
//Draw a frame around a square clicked on by user

//----------------------------------------------------------
void HighlightSquare(HDC hdc, int Square, int sqSize, int bRedraw, unsigned long color, int border)
{
	HBRUSH brush = CreateSolidBrush(color);

	if (Square >= 0 && Square <= 63) {
		int x = Square % 8, y = Square / 8;
		if (BoardFlip == 1) {
			x = 7 - x;
			y = 7 - y;
		}

		RECT rect;
		rect.left = x * sqSize + g_xAdd;
		rect.right = (x + 1) * sqSize + g_xAdd;
		rect.top = y * sqSize + g_yAdd;
		rect.bottom = (y + 1) * sqSize + g_yAdd;

		FrameRect(hdc, &rect, brush);

		if (border == 2) {
			rect.left += 1;
			rect.right -= 1;
			rect.top += 1;
			rect.bottom -= 1;
			FrameRect(hdc, &rect, brush);
		}
	}

	DeleteObject(brush);
}

//---------------------------------------------------------------
// Function to Draw a single Bitmap

// --------------------------------------------------------------
void DrawBitmap(HDC hdc, HBITMAP bitmap, int x, int y, int nSize)
{
	BITMAP bitmapbuff;
	HDC memorydc;
	POINT origin, size;

	memorydc = CreateCompatibleDC(hdc);
	SelectObject(memorydc, bitmap);
	SetMapMode(memorydc, GetMapMode(hdc));

	int result = GetObject(bitmap, sizeof(BITMAP), &bitmapbuff);

	origin.x = x;
	origin.y = y;

	if (nSize == 0) {
		size.x = bitmapbuff.bmWidth;
		size.y = bitmapbuff.bmHeight;
	}
	else {
		size.x = nSize;
		size.y = nSize;
	}

	DPtoLP(hdc, &origin, 1);
	DPtoLP(memorydc, &size, 1);
	if (nSize == bitmapbuff.bmWidth) {
		BitBlt(hdc, origin.x, origin.y, size.x, size.y, memorydc, 0, 0, SRCCOPY);
	}
	else {
		SetStretchBltMode(hdc, COLORONCOLOR);	//STRETCH_HALFTONE
		SetBrushOrgEx(hdc, 0, 0, NULL);
		StretchBlt(hdc,
				   origin.x,
				   origin.y,
				   size.x,
				   size.y,
				   memorydc,
				   0,
				   0,
				   bitmapbuff.bmWidth,
				   bitmapbuff.bmHeight,
				   SRCCOPY);
	}

	DeleteDC(memorydc);
}

// ------------------
// Draw Board

// ------------------
void DrawBoard(const CBoard &board)
{
	HDC hdc = GetDC(MainWnd);
	int start = 0, add = 1, add2 = 1, x = 0, y = 0, mul = 1;
	int nSize = 64;

	if (BoardFlip == 1) {
		start = 63;
		add = -1;
		add2 = 0;
	}

	for (int i = start; i >= 0 && i <= 63; i += add) {

		// piece
		if (board.GetPiece(BoardLoc[i]) != EMPTY && board.GetPiece(BoardLoc[i]) != INVALID) {
			DrawBitmap(hdc,
					   PieceBitmaps[board.GetPiece(BoardLoc[i])],
					   x * nSize * mul + g_xAdd,
					   y * nSize * mul + g_yAdd,
					   nSize);
		}

		// empty square
		else if (((i % 2 == 1) && (i / 8) % 2 == 1) || ((i % 2 == 0) && (i / 8) % 2 == 0))
			DrawBitmap(hdc, PieceBitmaps[7], x * nSize * mul + g_xAdd, y * nSize * mul + g_yAdd, nSize);
		else
			DrawBitmap(hdc, PieceBitmaps[8], x * nSize * mul + g_xAdd, y * nSize * mul + g_yAdd, nSize);

		x++;
		if (x == 8) {
			x = 0;
			y++;
		}
	}

	if (g_nSelSquare != NONE)
		HighlightSquare(hdc, g_nSelSquare, g_nSqSize, TRUE, 0xFFFFFF, 1);

	ReleaseDC(MainWnd, hdc);
}

// ------------------
// Replay Game from Game Move History up to nMove

// ------------------
void ReplayGame(int nMove, CBoard &Board)
{
	Board = g_StartBoard;
	g_numMoves = 0;

	int i = 0;
	while (g_GameMoves[i].SrcDst != 0 && i < nMove) {
		AddRepBoard(Board.HashKey, i);
		Board.DoMove(g_GameMoves[i], SEARCHED);
		i++;
	}

	g_numMoves = i;
	g_nSelSquare = NONE;
	g_nDouble = 0;
}

// --------------------
// GetFileName - get a file name using common dialog
// --------------------
static unsigned char sSaveGame[] = "Checkers Game (*.pdn)\0*.pdn\0\0";

static BOOL GetFileName(HWND hwnd, BOOL save, char *fname, unsigned char *filterList)
{
	OPENFILENAME of;
	int rc;

	memset(&of, 0, sizeof(OPENFILENAME));
	of.lStructSize = sizeof(OPENFILENAME);
	of.hwndOwner = hwnd;
	of.lpstrFilter = (LPSTR) filterList;
	of.lpstrDefExt = "";
	of.nFilterIndex = 1L;
	of.lpstrFile = fname;
	of.nMaxFile = _MAX_PATH;
	of.lpstrTitle = NULL;
	of.Flags = OFN_HIDEREADONLY;
	if (save) {
		rc = GetSaveFileName(&of);
	}
	else {
		rc = GetOpenFileName(&of);
	}

	return(rc);
}

// ------------------
// File I/O

// ------------------
void SaveGame(char *sFilename)
{
	FILE *pFile = fopen(sFilename, "wb");
	if (pFile == NULL)
		return;

	g_CBoard.ToPDN(g_buffer);
	fwrite(g_buffer, 1, strlen(g_buffer), pFile);

	fclose(pFile);
}

void LoadGame(char *sFilename)
{
	FILE *pFile = fopen(sFilename, "rb");
	if (pFile == NULL)
		return;

	int x = 0;
	while (x < 32000 && !feof(pFile))
		g_buffer[x++] = getc(pFile);
	g_buffer[x - 1] = 0;

	NewGame();
	g_CBoard.FromPDN(g_buffer);

	fclose(pFile);
	ReplayGame(1000, g_CBoard);
	DrawBoard(g_CBoard);
}

//
// GUI HELPER FUNCTIONS

//
void EndSetup()
{
	g_CBoard.SetFlags();
	g_StartBoard = g_CBoard;
	g_numMoves = 0;
	g_bSetupBoard = FALSE;
	DisplayText("");
}

void MoveNow()
{
	if (g_bThinking) {
		g_bStopThinking = true;
		WaitForSingleObject(hEngineReady, 500);
		RunningDisplay(-1, 0);
	}
}

void ComputerGo()
{
	if (g_bSetupBoard) {
		EndSetup();
	}

	if (g_bThinking) {
		MoveNow();
	}
	else {
		ThinkingMenu(true);
		WaitForSingleObject(hEngineReady, 1000);
		g_bStopThinking = false;
		g_bThinking = true;
		new_iter_maxtime = fMaxSeconds * .60f;
		SetEvent(hAction);
	}
}

int GetSquare(int &x, int &y)
{
	// Calculate the square the user clicked on (0-63)
	x = (x - g_xAdd) / g_nSqSize;
	y = (y - g_yAdd) / g_nSqSize;

	// Make sure it's valid, preferrably this function would only be called with valid input
	if (x < 0)
		x = 0;
	if (x > 7)
		x = 7;
	if (y < 0)
		y = 0;
	if (y > 7)
		y = 7;

	if (BoardFlip == 1) {
		x = 7 - x;
		y = 7 - y;
	}

	return x + y * 8;
}

// ===============================================
// Process messages to the Bottom Window

// ===============================================
INT_PTR CALLBACK InfoProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg) {
	case WM_COMMAND:
		switch (LOWORD(wparam)) {
		case IDC_TAKEBACK:
			MoveNow();
			if (g_numMoves < 0)
				g_numMoves = 0;
			g_numMoves -= 2;
			ReplayGame(g_numMoves, g_CBoard);
			DrawBoard(g_CBoard);
			SetFocus(MainWnd);
			break;

		case IDC_PREV:
			MoveNow();
			if (g_numMoves > 0) {
				g_numMoves--;
				ReplayGame(g_numMoves, g_CBoard);
				DrawBoard(g_CBoard);
			}

			SetFocus(MainWnd);
			break;

		case IDC_NEXT:
			MoveNow();
			g_numMoves++;
			ReplayGame(g_numMoves, g_CBoard);
			DrawBoard(g_CBoard);
			SetFocus(MainWnd);
			break;

		case IDC_START:
			MoveNow();
			g_numMoves = 0;
			ReplayGame(g_numMoves, g_CBoard);
			DrawBoard(g_CBoard);
			SetFocus(MainWnd);
			break;

		case IDC_END:
			MoveNow();
			g_numMoves = 1000;
			ReplayGame(g_numMoves, g_CBoard);
			DrawBoard(g_CBoard);
			SetFocus(MainWnd);
			break;

		case IDC_GO:
			if (g_bSetupBoard) {
				EndSetup();
				break;
			}

			ComputerGo();
			SetFocus(MainWnd);
			break;
		}
	}

	return(0);
}

// ==============================================================
//  Level Select Dialog Procedure

// ==============================================================
INT_PTR CALLBACK LevelDlgProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM /*lparam*/ )
{
	switch (msg) {
	case WM_INITDIALOG:
		SetDlgItemInt(hwnd, IDC_DEPTH, g_MaxDepth, 1);
		SetDlgItemInt(hwnd, IDC_TIME, (int)fMaxSeconds, 1);
		return(TRUE);

	case WM_COMMAND:
		if (LOWORD(wparam) == IDOK) {
			g_MaxDepth = GetDlgItemInt(hwnd, IDC_DEPTH, 0, 1);
			if (g_MaxDepth > 52)
				g_MaxDepth = 52;
			fMaxSeconds = (float)GetDlgItemInt(hwnd, IDC_TIME, 0, 1);
		}

		if (LOWORD(wparam) == IDOK || LOWORD(wparam) == IDCANCEL) {
			EndDialog(hwnd, TRUE);
			return(TRUE);
		}
		break;
	}

	return FALSE;
}

// ===============================================
// Process messages to the MAIN Window

// ===============================================
LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	HDC hdc;
	int nSquare;
	int x, y;
	int Eval;
	static char sFilename[256 * 20];

	switch (msg) {
	// Process Menu Commands
	case WM_COMMAND:
		switch (LOWORD(wparam)) {
		case ID_GAME_FLIPBOARD:
			BoardFlip ^= 1;
			DrawBoard(g_CBoard);
			break;

		case ID_GAME_NEW:
			NewGame();
			g_CBoard.StartPosition(1);
			DrawBoard(g_CBoard);
			break;

		case ID_GAME_COMPUTERMOVE:
			ComputerGo();
			break;

		case ID_GAME_COMPUTEROFF:
			SetComputerColor(OFF);
			break;

		case ID_OPTIONS_COMPUTERBLACK:
			SetComputerColor(BLACK);
			break;

		case ID_OPTIONS_COMPUTERWHITE:
			SetComputerColor(WHITE);
			break;

		case ID_GAME_HASHING:
			hashing ^= 1;
			UpdateMenuChecks();
			break;

		case ID_GAME_CLEAR_HASH:
			ZeroMemory(TTable, sizeof(TEntry) * TTable_entries);
			break;

		case ID_OPTIONS_BEGINNER:
			g_MaxDepth = BEGINNER_DEPTH;
			UpdateMenuChecks();
			break;

		case ID_OPTIONS_NORMAL:
			g_MaxDepth = NORMAL_DEPTH;
			UpdateMenuChecks();
			break;

		case ID_OPTIONS_EXPERT:
			g_MaxDepth = EXPERT_DEPTH;
			UpdateMenuChecks();
			break;

		case ID_OPTIONS_2SECONDS:
			fMaxSeconds = 2.0f;
			break;

		case ID_OPTIONS_5SECONDS:
			fMaxSeconds = 5.0f;
			break;

		case ID_OPTIONS_10SECONDS:
			fMaxSeconds = 10.0f;
			break;

		case ID_OPTIONS_30SECONDS:
			fMaxSeconds = 30.0f;
			break;

		case ID_OPTIONS_CUSTOMLEVEL:
			DialogBox((HINSTANCE) GetWindowLongPtr(hwnd, GWLP_HINSTANCE), "LevelWnd", hwnd, LevelDlgProc);
			break;

		case ID_FILE_SAVEGAME:
			if (GetFileName(hwnd, 1, sFilename, sSaveGame))
				SaveGame(sFilename);
			break;

		case ID_FILE_LOADGAME:
			if (GetFileName(hwnd, 0, sFilename, sSaveGame))
				LoadGame(sFilename);
			break;

		case ID_FILE_EXIT:
			PostMessage(hwnd, WM_DESTROY, 0, 0);
			break;

		case ID_EDIT_SETUPBOARD:
			if (g_bSetupBoard)
				EndSetup();
			else {
				g_bSetupBoard = TRUE;
				DisplayText("BOARD SETUP MODE.       (Click GO to end) \n(shift+click to erase pieces) \n(alt+click on a piece to set that color to move)");
			}
			break;

		case ID_EDIT_PASTE_POS:
			if (TextFromClipboard(g_buffer, 512)) {
				if (g_CBoard.FromFen(g_buffer)) {
					NewGame();
					g_StartBoard = g_CBoard;
				}
			}

			DrawBoard(g_CBoard);
			break;

		case ID_EDIT_COPY_POS:
			g_CBoard.ToFen(g_buffer);
			TextToClipboard(g_buffer);
			break;

		case ID_EDIT_COPY_PDN:
			g_CBoard.ToPDN(g_buffer);
			TextToClipboard(g_buffer);
			break;

		case ID_GAME_MOVENOW:
			MoveNow();
			break;

		case ID_EDIT_PASTE_PDN:
			if (TextFromClipboard(g_buffer, 16380)) {
				NewGame();
				g_CBoard.FromPDN(g_buffer);
			}

			DrawBoard(g_CBoard);
			break;
		}
		break;

	// Keyboard Commands
	case WM_KEYDOWN:
		if (int(wparam) == '2')
			pBook->AddPosition(g_CBoard, -1, false);
		if (int(wparam) == '3')
			pBook->AddPosition(g_CBoard, 0, false);
		if (int(wparam) == '4')
			pBook->AddPosition(g_CBoard, 1, false);
		if (int(wparam) == '6')
			pBook->RemovePosition(g_CBoard, false);
		if (int(wparam) == 'C') {
			delete pBook;
			pBook = new COpeningBook;
			DisplayText("Book Cleared");
		}

		if (int(wparam) == 'S')
			pBook->Save("opening.gbk");
		if (int(wparam) == 'G')
			ComputerGo();
		if (int(wparam) == 'M')
			MoveNow();

		break;

	// Process Mouse Clicks on the board
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
		SetFocus(hwnd);

		x = (int) (short)LOWORD(lparam);
		y = (int) (short)HIWORD(lparam);
		nSquare = GetSquare(x, y);

		if (g_bSetupBoard) {
			if (msg == WM_LBUTTONUP)
				return TRUE;

			int nBoardLoc = BoardLoc[nSquare];
			if (nBoardLoc == INV)
				break;
			if (GetKeyState(VK_SHIFT) < 0) {
				g_CBoard.SetPiece(nBoardLoc, EMPTY);
				DrawBoard(g_CBoard);
				break;
			}

			if (GetKeyState(VK_MENU) < 0) {
				g_CBoard.SideToMove = (g_CBoard.GetPiece(nBoardLoc) & BPIECE) ? BLACK : WHITE;
				break;
			}

			if (g_CBoard.GetPiece(nBoardLoc) == BPIECE)
				g_CBoard.SetPiece(nBoardLoc, BKING);
			else if (g_CBoard.GetPiece(nBoardLoc) == BKING)
				g_CBoard.SetPiece(nBoardLoc, EMPTY);
			else
				g_CBoard.SetPiece(nBoardLoc, (nBoardLoc < 28) ? BPIECE : BKING);

			DrawBoard(g_CBoard);
			break;
		}

		if (g_bThinking)
			return TRUE;						// Don't move pieces when computer is thinking

		// Did the user click on his/her own piece?
		if (nSquare >= 0 && nSquare <= 63 && g_CBoard.GetPiece(BoardLoc[nSquare]) != EMPTY) {
			if (msg == WM_LBUTTONUP)
				return TRUE;
			if (g_nDouble != 0)
				return TRUE;					// can't switch to a different piece when double jumping
			if
			(
				((g_CBoard.GetPiece(BoardLoc[nSquare]) & BPIECE) && g_CBoard.SideToMove == BLACK) ||
				((g_CBoard.GetPiece(BoardLoc[nSquare]) & WPIECE) && g_CBoard.SideToMove == WHITE)
			) {
				g_nSelSquare = nSquare;
				if (g_nSelSquare != NONE) {
					DrawBoard(g_CBoard);
				}

				hdc = GetDC(MainWnd);
				HighlightSquare(hdc, g_nSelSquare, g_nSqSize, TRUE, 0xFFFFFF, 1);
				ReleaseDC(MainWnd, hdc);
			}
		}
		else if (g_nSelSquare >= 0 && g_nSelSquare <= 63) {

			// Did the user click on a valid destination square?
			int MoveResult = SquareMove(g_CBoard, g_nSelSquare % 8, g_nSelSquare / 8, x, y, g_CBoard.SideToMove);
			if (MoveResult == VALID_MOVE) {
				g_nSelSquare = NONE;
				DrawBoard(g_CBoard);

				if (g_CompColor == g_CBoard.SideToMove)
					ComputerGo();
			}
			else if (MoveResult == DOUBLEJUMP) {
				g_nSelSquare = nSquare;
				DrawBoard(g_CBoard);
			}
		}

		return TRUE;

	case WM_RBUTTONDOWN:
		x = (int) (short)LOWORD(lparam);
		y = (int) (short)HIWORD(lparam);
		nSquare = GetSquare(x, y);

		if (g_bSetupBoard) {
			int nBoardLoc = BoardLoc[nSquare];
			if (nBoardLoc == INV)
				break;

			if (GetKeyState(VK_SHIFT) < 0) {
				g_CBoard.SetPiece(nBoardLoc, EMPTY);
				DrawBoard(g_CBoard);
				break;
			}

			if (GetKeyState(VK_MENU) < 0) {
				g_CBoard.SideToMove = (g_CBoard.GetPiece(nBoardLoc) & BPIECE) ? BLACK : WHITE;
				break;
			}

			if (g_CBoard.GetPiece(nBoardLoc) == WPIECE)
				g_CBoard.SetPiece(nBoardLoc, WKING);
			else if (g_CBoard.GetPiece(nBoardLoc) == WKING)
				g_CBoard.SetPiece(nBoardLoc, EMPTY);
			else
				g_CBoard.SetPiece(nBoardLoc, (nBoardLoc > 3) ? WPIECE : WKING);

			DrawBoard(g_CBoard);
			break;
		}

		Eval = -g_CBoard.EvaluateBoard(0, -100000, 10000);
		sprintf(g_buffer,
				"Eval: %d  Mobility B: %d  W: %d\nWhite : %d   Black : %d",
				Eval,
				g_CBoard.C.GetBlackMoves(),
				g_CBoard.C.GetWhiteMoves(),
				g_CBoard.nWhite,
				g_CBoard.nBlack);

		if (abs(Eval) != 2001 && g_CBoard.InDatabase(g_dbInfo)) {
			if (g_dbInfo.type == DB_WIN_LOSS_DRAW) {

				// FIXME : uncomment this when database exists again
				int result = QueryGuiDatabase(g_CBoard);
				switch (result) {
				case DRAW:
					strcat(g_buffer, "\nDRAW");
					break;

				case WHITEWIN:
					strcat(g_buffer, "\nWHITE WIN");
					break;

				case BLACKWIN:
					strcat(g_buffer, "\nBLACK WIN");
					break;

				default:
					strcat(g_buffer, "\nDATABASE ERROR");
					break;
				}
			}

			if (g_dbInfo.type == DB_EXACT_VALUES) {
				int result = QueryEdsDatabase(g_CBoard, 0);
				char buffer[32];
				if (result == 0)
					strcat(g_buffer, "\nDRAW");
				if (result > 1800) {
					strcat(g_buffer, "\nWHITE WINS IN ");
					strcat(g_buffer, _itoa(abs(2001 - result), buffer, 10));
				}

				if (result < -1800) {
					strcat(g_buffer, "\nBLACK WINS IN ");
					strcat(g_buffer, _itoa(abs(-2001 - result), buffer, 10));
				}
			}
		}

		DisplayText(g_buffer);

		return(TRUE);

	case WM_SETCURSOR:
		if (!g_bThinking)
			SetCursor(LoadCursor(NULL, IDC_ARROW));
		return(TRUE);

	/*case WM_ERASEBKGND:
	return TRUE;*/
	case WM_PAINT:
		PAINTSTRUCT ps;
		hdc = BeginPaint(hwnd, &ps);
		DrawBoard(g_CBoard);
		EndPaint(hwnd, &ps);
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	default:
		return(DefWindowProc(hwnd, msg, wparam, lparam));
	}

	return 0;
}

//
// This will interface CheckerBoard, you can get it at http://www.fierz.ch/
//
int ConvertFromCB[16] = { 0, 0, 0, 0, 0, 2, 1, 0, 0, 6, 5, 0, 0, 0, 0, 0 };
int ConvertToCB[16] = { 0, 6, 5, 0, 0, 10, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

int WINAPI getmove
		(
			int board[8][8],
			int color,
			double maxtime,
			char str[1024],
			int *playnow,
			int info,
			int moreinfo,
			struct CBmove *move
		)
{
	bool incremental;
	double remaining, increment, desired;
	static int nDraw = 0, Eval = 0;

	bCheckerBoard = TRUE;

	// initialization
	InitEngine(str);
	init_egdb(str);

	if (g_numMoves > 500)
		g_numMoves = 0;
	g_CBoard.StartPosition(info & CB_RESET_MOVES);
	g_sInfo = str;
	g_pbPlayNow = playnow;

	int i;
	for (i = 0; i < 64; i++)
		g_CBoard.SetPiece(BoardLoc[i], ConvertFromCB[board[i % 8][7 - i / 8]]);

	/* Convert CheckerBoard color to Gui Checkers color. */
	g_CBoard.SideToMove = color == CB_WHITE ? WHITE : BLACK;
	g_CBoard.SetFlags();
	AddRepBoard(TEntry::HashBoard(g_CBoard), g_numMoves);

	// Work around for a bug in checkerboard not sending reset
	static int LastTotal = 0;
	if ((g_CBoard.nBlack + g_CBoard.nWhite) > LastTotal) {

		/*
		int Adjust = 0;
		if (Eval < -50) Adjust = -1;
		if (Eval > 50)  Adjust = 1;
		if (g_numMoves > 150 && abs(Eval) < 90) Adjust = 0;
		pBook->LearnGame ( 15, Adjust );
		pBook->Save ( "opening.gbk" );
		*/
		g_numMoves = 0;
		nDraw = 0;
		AddRepBoard(TEntry::HashBoard(g_CBoard), g_numMoves);
	}

	LastTotal = g_CBoard.nBlack + g_CBoard.nWhite;

	/* Set search time limits, depending on the time control mode. */
	if (incremental = get_incremental_times(info, moreinfo, &increment, &remaining)) {
		const double ratio = 1.6;

		/* Using incremental time. */
		g_bEndHard = TRUE;						/* Dont allow stretching of fMaxSeconds during the search. */
		if (remaining < increment) {
			desired = remaining / ratio;
			fMaxSeconds = (float)remaining;
			new_iter_maxtime = 0.5 * fMaxSeconds / ratio;
		}
		else {
			desired = increment + remaining / 9;
			fMaxSeconds = (float)min(ratio * desired, remaining);
			new_iter_maxtime = 0.5 * fMaxSeconds / ratio;
		}

		/* Allow a few msec for overhead. */
		if (fMaxSeconds > .01)
			fMaxSeconds -= .003f;
	}
	else {

		/* Using fixed time per move. These params result in an average search time of maxtime. */
		fMaxSeconds = (float)maxtime * .985f;
		if (info & CB_EXACT_TIME) {
			g_bEndHard = TRUE;
			new_iter_maxtime = fMaxSeconds;
		}
		else {
			g_bEndHard = FALSE;
			new_iter_maxtime = 0.6 * maxtime;
		}
	}

	Eval = ComputerMove(g_CBoard.SideToMove, g_CBoard);

#ifdef LOG_TIME_MGMT
	if (incremental) {
		char *tag;
		double elapsed = (clock() - starttime) / (double)CLK_TCK;

		if (remaining - elapsed < 0)
			tag = "***";
		else if (elapsed > fMaxSeconds + .003)
			tag = "**";
		else if (elapsed >= fMaxSeconds)
			tag = "*";
		else
			tag = "";
		log_msg("incr %.1f, remaining %.3f, abs maxt %.3f, desired %.3f, new iter maxt %.3f, actual %.3f, margin %.3f %s\n",
		increment,
			remaining,
			fMaxSeconds,
			desired,
			new_iter_maxtime,
			elapsed,
			remaining - elapsed,
			tag);
	}
#endif
	for (i = 0; i < 64; i++)
		if (BoardLoc[i] >= 0)
			board[i % 8][7 - i / 8] = ConvertToCB[g_CBoard.GetPiece(BoardLoc[i])];

	AddRepBoard(TEntry::HashBoard(g_CBoard), g_numMoves);

	int retVal = CB_UNKNOWN;
	if ((Eval > 320 && g_CBoard.SideToMove == BLACK) || (Eval < -320 && g_CBoard.SideToMove == WHITE))
		retVal = CB_WIN;
	if ((Eval > 320 && g_CBoard.SideToMove == WHITE) || (Eval < -320 && g_CBoard.SideToMove == BLACK))
		retVal = CB_LOSS;
	if ((g_numMoves > 90 && abs(Eval) < 11) || (g_numMoves > 150 && abs(Eval) < 50))
		nDraw++;
	else
		nDraw = 0;
	if (nDraw >= 8)
		retVal = CB_DRAW;
	if (g_CBoard.InDatabase(g_dbInfo) && abs(Eval) < 2)
		retVal = CB_DRAW;

	g_numMoves++;								// Increment again since the next board coming in will be after the opponent has played his move
	return retVal;
}

int WINAPI enginecommand(char str[256], char reply[1024])
{
	char command[256], param1[256], param2[256];
	char *stopstring;

	command[0] = 0;
	param1[0] = 0;
	param2[0] = 0;
	sscanf(str, "%s %s %s", command, param1, param2);

	if (strcmp(command, "name") == 0) {
		sprintf(reply, g_sNameVer);
		return 1;
	}

	if (strcmp(command, "about") == 0) {

		// TODO : Print endgame database info
		sprintf(reply, "%s\nby Jonathan Kreuzer\n\n%s ", g_sNameVer, GetInfoString());
		return 1;
	}

	if (strcmp(command, "set") == 0) {
		int val;

		if (strcmp(param1, "hashsize") == 0) {
			bool status;

			int nMegs = strtol(param2, &stopstring, 10);
			if (nMegs < 1)
				return 0;
			if (nMegs > 4096)
				nMegs = 4096;
			while (status = set_ttable_size(nMegs)) {
				nMegs /= 2;
				sprintf(reply, "allocation failed, downsizing to %dmb", nMegs);
				set_ttable_size(nMegs);
			}

			save_hashsize(nMegs);
			return 1;
		}

		if (strcmp(param1, "dbpath") == 0) {
			char *p = strstr(str, "dbpath");	/* Cannot use param2 here because it does not allow spaces in the path. */
			while (!isspace(*p))				/* Skip 'dbpath' and following space. */
				++p;
			while (isspace(*p))
				++p;
			if (strcmp(p, wld_path)) {
				strcpy(wld_path, p);
				save_dbpath(wld_path);
			}

			sprintf(reply, "dbpath set to %s", wld_path);
			return(1);
		}

		if (strcmp(param1, "enable_wld") == 0) {
			val = strtol(param2, &stopstring, 10);
			if (val != enable_wld) {
				enable_wld = val;
				save_enable_wld(enable_wld);
				request_wld_init = 1;
				if (!g_dbInfo.loaded && enable_wld) {
					sprintf(reply, "Initializing endgame db...");
					InitializeEdsDatabases(g_dbInfo);
				}
			}

			sprintf(reply, "enable_wld set to %d", enable_wld);
			return(1);
		}

		if (strcmp(param1, "book") == 0) {
			val = strtol(param2, &stopstring, 10);
			if (val != use_opendb) {
				use_opendb = val;
				save_book_setting(use_opendb);
			}

			sprintf(reply, "book set to %d", use_opendb);
			return(1);
		}

		if (strcmp(param1, "max_dbpieces") == 0) {
			val = strtol(param2, &stopstring, 10);
			if (val != max_dbpieces) {
				max_dbpieces = val;
				save_max_dbpieces(max_dbpieces);
			}

			sprintf(reply, "max_dbpieces set to %d", max_dbpieces);
			return(1);
		}
		
		if (strcmp(param1, "dbmbytes") == 0) {
			val = strtol(param2, &stopstring, 10);
			if (val != wld_cache_mb) {
				wld_cache_mb = val;
				save_dbmbytes(wld_cache_mb);
			}

			sprintf(reply, "dbmbytes set to %d", wld_cache_mb);
			return(1);
		}
	}

	if (strcmp(command, "get") == 0) {
		if (strcmp(param1, "hashsize") == 0) {
			get_hashsize(&TTable_mb);
			sprintf(reply, "%d", TTable_mb);
			return 1;
		}

		if (strcmp(param1, "protocolversion") == 0) {
			sprintf(reply, "2");
			return 1;
		}

		if (strcmp(param1, "gametype") == 0) {
			sprintf(reply, "%d", GT_ENGLISH);
			return 1;
		}

		if (strcmp(param1, "dbpath") == 0) {
			get_dbpath(wld_path, sizeof(wld_path));
			sprintf(reply, wld_path);
			return(1);
		}

		if (strcmp(param1, "enable_wld") == 0) {
			get_enable_wld(&enable_wld);
			sprintf(reply, "%d", enable_wld);
			return(1);
		}

		if (strcmp(param1, "book") == 0) {
			get_book_setting(&use_opendb);
			sprintf(reply, "%d", use_opendb);
			return(1);
		}

		if (strcmp(param1, "max_dbpieces") == 0) {
			get_max_dbpieces(&max_dbpieces);
			sprintf(reply, "%d", max_dbpieces);
			return(1);
		}

		if (strcmp(param1, "dbmbytes") == 0) {
			get_dbmbytes(&wld_cache_mb);
			sprintf(reply, "%d", wld_cache_mb);
			return(1);
		}
	}

	strcpy(reply, "?");
	return 0;
}
