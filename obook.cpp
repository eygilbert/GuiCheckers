#include <stdio.h>
#include <time.h>

#define BOOK_EXTRA_SIZE 200000
#define NOVAL			- 30000

// =======================================
// OPENING BOOK
// for Gui Checkers
// =======================================
class COpeningBook
{
	// structures needed
	struct SBookEntry
	{
		SBookEntry() {
			pNext = NULL;
			wValue = NOVAL;
		}
		~SBookEntry() {
		}
		void Save(FILE *pFile, unsigned int wPosKey)
		{
			wPosKey &= 65535;
			fwrite(&ulCheck, 4, 1, pFile);
			fwrite(&wKey, 2, 1, pFile);
			fwrite(&wValue, 2, 1, pFile);
			fwrite(&wPosKey, 2, 1, pFile);
		}

		unsigned int Load(FILE *pFile)
		{
			unsigned int wPosKey = 0;
			fread(&ulCheck, 4, 1, pFile);
			fread(&wKey, 2, 1, pFile);
			fread(&wValue, 2, 1, pFile);
			fread(&wPosKey, 2, 1, pFile);
			return wPosKey;
		}

		unsigned long ulCheck;
		SBookEntry *pNext;
		unsigned short wKey;
		short wValue;
	};

/* */
public:

	// ctor&dtor
	COpeningBook()
	{
		m_pHash = new SBookEntry[131072];
		m_pExtra = new SBookEntry[BOOK_EXTRA_SIZE];
		m_nListSize = 0;
		m_nPositions = 0;
	}

	~COpeningBook()
	{
		delete[] m_pHash;
		delete[] m_pExtra;
	}

	// Functions
	void LearnGame(int numMoves, int Adjust);
	int FindMoves(CBoard &Board, int Moves[], short wValues[]);
	int GetMove(CBoard &Board, int &nBestMove);
	void RemovePosition(CBoard &Board, int bQuiet);
	int GetValue(CBoard &Board);
	void AddPosition(CBoard &Board, short wValue, int bQuiet);
	void AddPosition(unsigned long ulKey, unsigned long ulCheck, short wValue, int bQuiet);
	int Load(char *sFileName);
	int LoadFEN(char *filename);
	void Save(char *sFileName);

	// Data
	SBookEntry *m_pHash;
	SBookEntry *m_pExtra;
	int m_nListSize;
	int m_nPositions;
};

// ============================================
//
//   OPENING BOOK Functions.
//

// ============================================
void COpeningBook::AddPosition(CBoard &Board, short wValue, int bQuiet)
{
	unsigned long ulKey = (unsigned long)Board.HashKey;
	unsigned long ulCheck = (unsigned long)(Board.HashKey >> 32);
	AddPosition(ulKey, ulCheck, wValue, bQuiet);
}

//

//
int COpeningBook::GetValue(CBoard &Board)
{
	unsigned long ulKey = (unsigned long)Board.HashKey;
	unsigned long ulCheck = (unsigned long)(Board.HashKey >> 32);

	int nEntry = ulKey & 131071;
	SBookEntry *pEntry;

	pEntry = &m_pHash[nEntry];
	while (pEntry != NULL && pEntry->wValue != NOVAL) {
		if (pEntry->ulCheck == ulCheck && pEntry->wKey == (ulKey >> 16)) {
			return pEntry->wValue;
		}

		pEntry = pEntry->pNext;
	}

	return NOVAL;
}

//
//

//
void COpeningBook::AddPosition(unsigned long ulKey, unsigned long ulCheck, short wValue, int bQuiet)
{
	char sTemp[255];
	int nEntry = ulKey & 131071;

	SBookEntry *pEntry = &m_pHash[nEntry];

	while (pEntry->wValue != NOVAL) {
		if (pEntry->ulCheck == ulCheck && pEntry->wKey == (ulKey >> 16)) {
			if (wValue == 0) {
				if (pEntry->wValue > 0)
					wValue--;
				if (pEntry->wValue < 0)
					wValue++;
			}

			if (!bQuiet) {
				sprintf(sTemp,
						"Position Exists. %d\nValue was %d now %d",
						nEntry,
						pEntry->wValue,
						pEntry->wValue + wValue);
				DisplayText(sTemp);
			}

			pEntry->wValue += wValue;
			return;
		}

		if (pEntry->pNext == NULL) {
			if (m_nListSize >= BOOK_EXTRA_SIZE) {
				DisplayText("Book Full!");
				return;
			}

			pEntry->pNext = &m_pExtra[m_nListSize];
			m_nListSize++;
		}

		pEntry = pEntry->pNext;
	}

	// New Position
	m_nPositions++;
	pEntry->ulCheck = ulCheck;
	pEntry->wKey = unsigned short(ulKey >> 16);
	pEntry->wValue = wValue;
	if (!bQuiet) {
		sprintf(sTemp, "Position Added. %d\nValue %d", nEntry, wValue);
		DisplayText(sTemp);
	}
}

// ----------------------------------
//  Remove a position from the book.
//  (if this position is in the list, it stays in memory until the book is saved and loaded.)

// ----------------------------------
void COpeningBook::RemovePosition(CBoard &Board, int bQuiet)
{
	char sTemp[255];
	unsigned long ulKey = (unsigned long)Board.HashKey;
	unsigned long ulCheck = (unsigned long)(Board.HashKey >> 32);
	int nEntry = ulKey & 131071;
	SBookEntry *pEntry = &m_pHash[nEntry];
	SBookEntry *pPrev = NULL;

	while (pEntry != NULL && pEntry->wValue != NOVAL) {
		if (pEntry->ulCheck == ulCheck && pEntry->wKey == (ulKey >> 16)) {
			m_nPositions--;
			if (!bQuiet) {
				sprintf(sTemp, "Position Removed. %d\nValue was %d", nEntry, m_pHash[nEntry].wValue);
				DisplayText(sTemp);
			}

			if (pPrev != NULL)
				pPrev->pNext = pEntry->pNext;
			else {
				if (pEntry->pNext != NULL)
					*pEntry = *pEntry->pNext;
				else
					pEntry->wValue = NOVAL;
			}

			return;
		}

		pPrev = pEntry;
		pEntry = pEntry->pNext;
	}

	if (!bQuiet) {
		sprintf(sTemp, "Position does not exist. %d\n", nEntry);
		DisplayText(sTemp);
	}
}

// --------------------
//
// FILE I/O For Opening Book
//
// --------------------

// LOAD
int COpeningBook::Load(char *sFileName)
{
	FILE *fp = fopen(sFileName, "rb");
	SBookEntry TempEntry;
	int i = 0;

	if (fp == NULL)
		return false;

	unsigned long wKey = TempEntry.Load(fp);
	while (!feof(fp)) {
		i++;
		wKey += (TempEntry.wKey << 16);
		AddPosition(wKey, TempEntry.ulCheck, TempEntry.wValue, TRUE);
		wKey = TempEntry.Load(fp);
	}

	fclose(fp);

	char sTemp[255];
	sprintf(sTemp, "%d Positions Loaded", i);
	DisplayText(sTemp);
	return true;
}

int COpeningBook::LoadFEN(char *filename)
{
	int count, value, status;
	FILE *fp;
	char fenbuf[200];
	CBoard board;

	fp = fopen(filename, "r");
	if (!fp)
		return(false);

	for (count = 0;; ++count) {
		status = fscanf(fp, "%s\n", fenbuf);
		if (status != 1)
			break;
		if (!board.FromFen(fenbuf))
			break;
		status = fscanf(fp, "value %d\n", &value);
		if (status != 1)
			break;
		AddPosition(board, value, true);
	}

	sprintf(fenbuf, "%d positions loaded", count);
	DisplayText(fenbuf);
	return(count);
}

//
// SAVE
//
// the book generation code muddles this up a bit, nTimes is
// the number of times this position must have occured to be saved.

//
void COpeningBook::Save(char *sFileName)
{
	FILE *fp;
	fp = fopen(sFileName, "wb");
	if (fp == NULL)
		return;

	SBookEntry *pEntry;
	int Num = 0;

	for (int i = 0; i < 131072; i++) {
		pEntry = &m_pHash[i];
		if (pEntry->wValue != NOVAL) {
			if (pEntry->wValue < 100) {
				pEntry->Save(fp, i);
				Num++;
			}

			while (pEntry->pNext != NULL) {
				pEntry = pEntry->pNext;
				pEntry->Save(fp, i);
				Num++;
			}
		}
	}

	fclose(fp);

	char sTemp[255];
	sprintf(sTemp, "%d Positions Saved", Num);
	DisplayText(sTemp);
}

// --------------------
// Find a book move by doing all legal moves, then checking to see if the position is in the opening book.

// --------------------
int COpeningBook::FindMoves(CBoard &Board, int OutMoves[], short wValues[])
{
	int i, val, nFound;
	CMoveList Moves;
	CBoard TempBoard;

	if (Board.SideToMove == BLACK)
		Moves.FindMovesBlack(Board.C);
	if (Board.SideToMove == WHITE)
		Moves.FindMovesWhite(Board.C);

	nFound = 0;

	for (i = 1; i <= Moves.numMoves; i++) {
		TempBoard = Board;
		TempBoard.DoMove(Moves.Moves[i], SEARCHED);
		val = GetValue(TempBoard);

		// Add move if it leads to a position in the book
		if (val != NOVAL) {
			OutMoves[nFound] = i;
			wValues[nFound] = val;
			nFound++;
		}
	}

	return nFound;
}

// --------------------
// Try to do a move in the opening book

// --------------------
int COpeningBook::GetMove(CBoard &Board, int &nBestMove)
{
	int nVal = NOVAL, nMove = -1;
	int nMax, i, nGood;
	int Moves[60];
	int GoodMoves[60];
	short wValues[60], wGoodVal[60];

	int nFound = FindMoves(Board, Moves, wValues);

	srand((unsigned)time(0));	// Randomize
	nGood = 0;
	nMax = 0;
	for (i = 0; i < nFound; i++) {
		if ((Board.SideToMove == BLACK && wValues[i] <= 0) || (Board.SideToMove == WHITE && wValues[i] >= 0)) {
			if (abs(wValues[i]) > nMax)
				nMax = abs(wValues[i]);
		}
	}

	for (i = 0; i < nFound; i++) {
		if ((Board.SideToMove == BLACK && wValues[i] <= 0) || (Board.SideToMove == WHITE && wValues[i] >= 0)) {
			if (wValues[i] == 0 && nMax > 0)
				continue;
			wGoodVal[nGood] = wValues[i];
			GoodMoves[nGood] = Moves[i];
			nGood++;
		}
	}

	if (nGood == 0)
		return NOVAL;

	nMove = rand() % nGood;
	nVal = wGoodVal[nMove];

	if (abs(nVal) < abs(nMax)) {
		nMove = rand() % nGood;
		nVal = wGoodVal[nMove];
	}

	if (abs(nVal) < abs(nMax) - 1) {
		nMove = rand() % nGood;
		nVal = wGoodVal[nMove];
	}

	if (nMove != -1)
		nBestMove = GoodMoves[nMove];
	else
		nBestMove = NONE;

	return nVal;
}

//
// Book Learning

//
void COpeningBook::LearnGame(int numMoves, int nAdjust)
{
	for (int i = 0; i <= (numMoves - 1) * 2; i++) {
		unsigned long ulKey = (unsigned long)RepNum[i];
		unsigned long ulCheck = (unsigned long)(RepNum[i] >> 32);
		AddPosition(ulKey, ulCheck, nAdjust, true);
	}
}
