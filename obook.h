#pragma once
#include <stdio.h>
#include <time.h>
#include "board.h"

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

