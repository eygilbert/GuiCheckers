// Gui Checkers by Jonathan Kreuzer
// copyright 2005, 2009
// =================================================
// I use my own bitboard representation detailed in movegen.cpp
// =================================================

#include <stdio.h>
#include "board.h"
#include "database.h"
#include "edDatabase.h"
#include "movegen.h"

const int KING_VAL = 33;
const int KV = KING_VAL;
const int KBoard[32] =
{
	KV - 3,
	KV + 0,
	KV + 0,
	KV + 0,
	KV + 0,
	KV + 1,
	KV + 1,
	KV + 0,
	KV + 1,
	KV + 3,
	KV + 3,
	KV + 2,
	KV + 3,
	KV + 5,
	KV + 5,
	KV + 1,
	KV + 1,
	KV + 5,
	KV + 5,
	KV + 3,
	KV + 2,
	KV + 3,
	KV + 3,
	KV + 1,
	KV + 0,
	KV + 1,
	KV + 1,
	KV + 0,
	KV + 0,
	KV + 0,
	KV + 0,
	KV - 3
};

CBoard g_CBoard;
CBoard g_StartBoard;

// Convert from 64 to 32 board
int BoardLoc[66] =
{
	INV,
	28,
	INV,
	29,
	INV,
	30,
	INV,
	31,
	24,
	INV,
	25,
	INV,
	26,
	INV,
	27,
	INV,
	INV,
	20,
	INV,
	21,
	INV,
	22,
	INV,
	23,
	16,
	INV,
	17,
	INV,
	18,
	INV,
	19,
	INV,
	INV,
	12,
	INV,
	13,
	INV,
	14,
	INV,
	15,
	8,
	INV,
	9,
	INV,
	10,
	INV,
	11,
	INV,
	INV,
	4,
	INV,
	5,
	INV,
	6,
	INV,
	7,
	0,
	INV,
	1,
	INV,
	2,
	INV,
	3,
	INV
};

// =================================================
//
//	 			TRANSPOSITION TABLE
//
// =================================================
unsigned char g_ucAge = 0;


// transposition table globals.
TEntry *TTable;
size_t TTable_entries;
int TTable_mb = 512;

/*
 * Allocate the hashtable.
 * Return true if the allocation failed.
 */
bool set_ttable_size(int size_mb)
{
	if (TTable)
		free(TTable);

	TTable_mb = size_mb;
	TTable_entries = ((size_t) size_mb * (1 << 20)) / sizeof(TEntry);
	TTable = (TEntry *)malloc(sizeof(TEntry) * TTable_entries);
	if (!TTable)
		return(true);
	else
		return(false);
}

// ------------------
//  Hash Board
//
//  HashBoard is called to get the hash value of the start board.
// ------------------
unsigned __int64 HashFunction[32][12] =
{
	{ 0x00713BBE67CF1E6C, 0x2D4917525FA6B6F1, 0x5B32E1E901F6E1A6, 0x12F0BA87391B3E99, 0x0154A20D496953B7,
			0x159C17B32D199AC8, 0x64A9FD8B27167C03, 0x7AD0C2091273431F, 0x6E7837CB6C7C15F5, },
	{ 0x4E775D1326788C0A, 0x302831AE0733959A, 0x2373394058E392FD, 0x3E2CA8323C31177D, 0x5F5728AD31AD61F2,
			0x497253661D06D1C4, 0x42AF17322C50F522, 0x3EFE7B9140B05A8B, 0x129332B02700A202, },
	{ 0x7C10859D70B27680, 0x18952D993CE92980, 0x5DE60DC94923B3BF, 0x5CA3E5BF2F7EDA7E, 0x4282128E0DDFF8EF,
			0x46838561306B918F, 0x3A844AD1267CE59C, 0x1977DD7233268EF0, 0x0403984A06BB1AD7, },
	{ 0x6C388D2C19BEDAC9, 0x0E7146337898C90C, 0x12523FD45AEC33A4, 0x206B7E3508369F22, 0x1B0312CF01E1CA2D,
			0x60A0188F09ACFFD9, 0x594261E51E0A3B2B, 0x508CC1877B9D845F, 0x187B16D47F9C0CBE, },
	{ 0x0CCB1115387E4133, 0x72A73C1862A4ABDA, 0x50B18DCE3BFDD63E, 0x6DD364F85C8E9A04, 0x17F4BA0E73F82E2F,
			0x4DC0B2D42D36544A, 0x5F1EA66858DD0D16, 0x4A2A85AD4F0B2988, 0x55F6265249023B43, },
	{ 0x0E54A8F45F58FCD8, 0x0A32200B693AFE60, 0x347F7897403AB359, 0x5E9E96D07B3225CD, 0x26E0E44908CEBC25,
			0x4E826A407067DA3B, 0x0DAAC211259E078C, 0x54E4E80B30C6FC8C, 0x41604915446ACC95, },
	{ 0x011BA7E1502EBCE9, 0x5FE7CAA764CCEFB5, 0x7AA4D59A2FF83823, 0x7A1F58902A3F79D1, 0x1145DAA14CC2CB4E,
			0x01F18A9C4F19E171, 0x15EF0DA52911609B, 0x671B8A654E9E9D46, 0x12D9DC82208AE776, },
	{ 0x08AF0B6326F742DF, 0x27E8906D1162C6E0, 0x72FB71344EEDA6A6, 0x1DADA65E2036EB0E, 0x06EDAF2836AD3F20,
			0x723ADDC2107A4941, 0x060EAF0F75473C65, 0x79866C616C21F7E7, 0x212CD1467A6926DC, },
	{ 0x4B13FF8D79362FF2, 0x0163BDFF64E9E23B, 0x1CA6068C3588DB78, 0x6D7BB4D05516E8BE, 0x4432D446589E8E8C,
			0x0469827F5A7B36A7, 0x795BA9321312F95B, 0x25DE420B59020FF5, 0x1F0D317D0E12C18A, },
	{ 0x0AA530FD7D1BBE68, 0x1B30727B6022CCCD, 0x285211F0205C59B4, 0x13F5D753784C6ECB, 0x52F5845E3AC24103,
			0x139DBBE6367DF931, 0x4A52CAAD32FBD9B0, 0x65E32FFF3223A614, 0x196146793A74B242, },
	{ 0x38D3CEC16B43E68C, 0x79F752237E7C2AA6, 0x49BFE54C5AB67E8D, 0x21829F995B44AFB1, 0x329351B127B1F608,
			0x2D9F48675E9FB6D5, 0x19C23B1874CE5742, 0x0E743FFE42A00921, 0x580F9D2F09B6BC73, },
	{ 0x27AC52433293057E, 0x5CE3C7D54F2318A1, 0x0CDAD45A7CF9E127, 0x5D6E4D0758DCD2BE, 0x06CCBBB407F12469,
			0x1B5B720901648E01, 0x4DF04A562D32539D, 0x5047ACD84879B981, 0x275C646B4E9A1462, },
	{ 0x0433C1D0781ACE17, 0x4EAFE80D4ADE331D, 0x65C174994BBB4374, 0x5647FB4F2262D5B5, 0x62E004570F522F80,
			0x7283B18D73D8548B, 0x19B6CB117A7516F6, 0x50910BF7220B72B8, 0x64F63C603D99D53C, },
	{ 0x6D8B1B5B499EEB8A, 0x18ECA65C024A3D87, 0x16535E325706A840, 0x2FE7E3C351CEC130, 0x7C6E12EE62887503,
			0x015BF06A2C244550, 0x0BE121812B979928, 0x44D2FCE93BBF7DA1, 0x6EBF533148C730A4, },
	{ 0x6F0B03816EF5EB98, 0x6A726E9A74B80FCF, 0x3864F450698FD4BD, 0x00708A030833B05D, 0x57F072B9124F604E,
			0x02DD8EE507535398, 0x3744D10D327F1A14, 0x0355CCB85175F0E2, 0x3B6CA47D64491584, },
	{ 0x4DE09D0160D18A6B, 0x5B9639435270FC19, 0x583A60982571872E, 0x1FD1380E7A321DF0, 0x5003743D5FEBAE31,
			0x5106BFAF75CDF3A4, 0x3A024D5E178A59FC, 0x5DF8C602059D1B6B, 0x477BEA21085A8D5A, },
	{ 0x4CF0881E54ACB571, 0x53DCC2B81CB4EE4E, 0x3FF060BF37B0BF74, 0x302DDEE55E4E6258, 0x538837F25ED5BCA0,
			0x2E8DCFCB695743E9, 0x6FAA790235CCDAAC, 0x153519F74BB87523, 0x4A98B8A37588DF35, },
	{ 0x67B8E7AD62FB3FD5, 0x31C97C5D1984FD5A, 0x3102F87719634DB5, 0x61D9C53A39A2F998, 0x3B4CB6EB1ECEA5DF,
			0x7676D5B207E125E6, 0x6004E6BC74DC788D, 0x46C9D1D665842414, 0x5DF0490470772818, },
	{ 0x32CAB4BB53737E60, 0x60E633CF07B2DA2F, 0x2C9EB9293854F959, 0x26097CEA0A32FF55, 0x12C3A784304130F8,
			0x68562AED20F794A0, 0x1DC8BDFA720B6EFC, 0x05524D852DC798B0, 0x72CD04EE74F44185, },
	{ 0x806581076B9DF5EC, 0x247C5EA35F4EBCF4, 0x52F8643268C95004, 0x3055A22C71E48101, 0x3975154335A480A2,
			0x405676927423571C, 0x62D27E3E67A43872, 0x457A614652E98E09, 0x2FACFA7A480AFC55, },
	{ 0x2A0A2BCA78676EB6, 0x72A5246E5186FFE0, 0x363C8A9B12015F5F, 0x578857BA4E1E1A1B, 0x691EB761421F92E5,
			0x5FF32F783B8D5077, 0x50C66F597C4A686A, 0x227394C44C932A0F, 0x3B42F00102184F30, },
	{ 0x33E31D892544D622, 0x37C86D05443EC6B3, 0x6EF6568D2AEB58D0, 0x47A62E691E4C6804, 0x7BB537BF09FD4014,
			0x3FDBAEF827567F68, 0x4B6CD0C048350C02, 0x26F180EC1DACA230, 0x723F86AF7F6AE441, },
	{ 0x20F8CBDD1DE0D2DA, 0x06595C3824103BCA, 0x353BA8EF25A21869, 0x7A0A9ADE6615E7E2, 0x6F872E3622C41A00,
			0x10B96B8716876F54, 0x3AA4706C01F5A4C5, 0x666E1F4926A17AF3, 0x6BE94DD702FBB4F4, },
	{ 0x2DF3C374173E394F, 0x5F69467443755642, 0x14C73EDF03D5A1B6, 0x202F386745718A74, 0x77D50B7713E4D16E,
			0x729CB7A76975F93E, 0x563BDC047A051330, 0x4C44EDE419A31B61, 0x2D98AB3D5905B1E7, },
	{ 0x7DE2A8BC1EA22FFF, 0x2A0E150F2B3FA5A9, 0x2540871E5F149E41, 0x5807F428089BCFF5, 0x2BBDEBFD0314982B,
			0x457E99CA63D2452E, 0x61B42E393699ACBE, 0x3821A5CE227FDC57, 0x7DF421C90946844E, },
	{ 0x6E4150F625160510, 0x161BEE2D26E90A26, 0x0AD7B3781C2018D1, 0x50FBE913614BC159, 0x24824DC649153465,
			0x664F45474F826CEB, 0x195A68A20588B9A8, 0x3477E2E07364EE20, 0x1A7514622D7D9284, },
	{ 0x7D0B12A82CE36122, 0x19F4517F443D5B28, 0x0CCE03CB6C5767D9, 0x00FC316477DD808B, 0x600A4DC36A8DB506,
			0x64747C7458650220, 0x0D40B283581E38A2, 0x1D5C9F5C61B955DA, 0x07AB13E148DC7D75, },
	{ 0x70AB839A3541B363, 0x791F3D494EE7A7F9, 0x367C794E19EB3597, 0x5029799F0DAB9F70, 0x28F5ED3F27F99547,
			0x3C8996742F5FCB43, 0x4ABB843032D3FC2B, 0x35E23F6C0AC8749D, 0x0B56D86450B1A227, },
	{ 0x4EBA6BEE69D0B248, 0x1C55FF6E21DF3AC2, 0x62AF3F0E2FBCF7C7, 0x1CA6F93E27F76CE2, 0x6BEB04B75D0F1CB3,
			0x0AD6D06951D144CD, 0x0C4A401C2C60065A, 0x27CBEAB71777F45C, 0x163DF26040B9C095, },
	{ 0x3999F2F23DEBC680, 0x204A088326C0ADE9, 0x5105C4EC218C88FA, 0x6E44958F214022E1, 0x0C62C2D1245E7456,
			0x42254E037CD48DAA, 0x754C11AE18123B29, 0x5709EB7F6B4AFFEF, 0x6F7D8E424505478B, },
	{ 0x1A4A67B11A04C2F7, 0x5EB60504651563D6, 0x6C33337F085FC0FC, 0x72737B743101C49F, 0x71CADE75567C6DCD,
			0x43F70EA55FC5B4F4, 0x47B1DB70076304A6, 0x2F4E400A2991DBD8, 0x200CC28B70563A4E, },
	{ 0x196C328137C8DDFB, 0x167DC8495AE6B772, 0x066D259D10C9D1BE, 0x491755040A080540, 0x34B53D37694B4165,
			0x7EE95F9524C2B947, 0x25E6603668143C9E, 0x6A6A034441B2888E, 0x3CE3399D466F3232, },
};
unsigned __int64 HashSTM;

void TEntry::Create_HashFunction()
{
	/* Changed to a compile time table because of differences in rand() seeding between
	 * various runtime environments.
	 */
	HashSTM = HashFunction[0][0];
}

unsigned __int64 TEntry::HashBoard(const CBoard &Board)
{
	unsigned __int64 CheckSum = 0;

	for (int index = 0; index < 32; index++) {
		int nPiece = Board.GetPiece(index);
		if (nPiece != EMPTY)
			CheckSum ^= HashFunction[index][nPiece];
	}

	if (Board.SideToMove == BLACK) {
		CheckSum ^= HashSTM;
	}

	return CheckSum;
}

// Check for a repeated board
// (only checks every other move, because side to move must be the same)
__int64 RepNum[MAX_GAMEMOVES];


// =================================================
void CBoard::Clear()
{
	C.BP = 0;
	C.WP = 0;
	C.K = 0;
	SetFlags();
}

void CBoard::StartPosition(int bResetRep)
{
	Clear();
	C.BP = MASK_1ST | MASK_2ND | MASK_3RD;
	C.WP = MASK_6TH | MASK_7TH | MASK_8TH;

	SideToMove = BLACK;
	if (bResetRep)
		g_numMoves = 0;

	SetFlags();
	g_StartBoard = *this;
}

void CBoard::SetFlags()
{
	nWhite = 0;
	nBlack = 0;
	nPSq = 0;
	C.empty = ~(C.WP | C.BP);
	for (int i = 0; i < 32; i++) {
		int piece = GetPiece(i);
		if ((piece & WHITE)) {
			nWhite++;
			if (piece == WKING) {
				nPSq += KBoard[i];
			}
		}

		if ((piece & BLACK)) {
			nBlack++;
			if (piece == BKING) {
				nPSq -= KBoard[i];
			}
		}
	}

	HashKey = TEntry::HashBoard(*this);
}

// ------------------
//  Evaluate Board
//
//  I don't know much about checkers, this is the best I could do for an evaluation function
// ------------------
const int LAZY_EVAL_MARGIN = 64;

int CBoard::EvaluateBoard(int ahead, int alpha, int beta)
{
	// Game is over?
	if (C.WP == 0)
		return -2001 + ahead;
	if (C.BP == 0)
		return 2001 - ahead;

	//
	if (g_dbInfo.type == DB_EXACT_VALUES && InDatabase(g_dbInfo)) {
		int value = QueryEdsDatabase(*this, ahead);

		if (value != INVALID_VALUE) {

			// If we get an exact score from the database we want to return right away
			databaseNodes++;
			return value;
		}
	}

	int eval;

	// Evaluate number of pieces present. Scaled higher for less material
	if ((nWhite + nBlack) > 12)
		eval = (nWhite - nBlack) * 100;
	else
		eval = (nWhite - nBlack) * (160 - (nWhite + nBlack) * 5);

	eval += nPSq;

	// Probe the W/L/D bitbase
	if (g_dbInfo.type == DB_WIN_LOSS_DRAW && InDatabase(g_dbInfo)) {
		int Result = QueryGuiDatabase(*this);
		if (Result <= 2)
			databaseNodes++;
		if (Result == 2)
			eval += 400;
		if (Result == 1)
			eval -= 400;
		if (Result == 0)
			return 0;
	}
	else {

		// surely winning advantage? note : < 8 is to not overflow the eval
		if (nWhite == 1 && nBlack >= 3 && nBlack < 8)
			eval += (eval >> 1);
		if (nBlack == 1 && nWhite >= 3 && nWhite < 8)
			eval += (eval >> 1);
	}

	// Too far from the alpha beta window? Forget about the rest of the eval, it probably won't get value back within the window
	if (eval + LAZY_EVAL_MARGIN < alpha)
		return eval;
	if (eval - LAZY_EVAL_MARGIN > beta)
		return eval;

	// Keeping checkers off edges
	eval -= 2 * (BitCount(C.WP &~C.K & MASK_EDGES) - BitCount(C.BP &~C.K & MASK_EDGES));

	// Mobility
	// eval += ( C.GetWhiteMoves() - C.GetBlackMoves() ) * 2;
	// The losing side can make it tough to win the game by moving a King back and forth on the double corner squares.
	int WK = C.WP & C.K;
	int BK = C.BP & C.K;

	if (eval > 18) {
		if ((BK & MASK_2CORNER1)) {
			eval -= 8;
			if ((MASK_2CORNER1 &~C.BP &~C.WP))
				eval -= 10;
		}

		if ((BK & MASK_2CORNER2)) {
			eval -= 8;
			if ((MASK_2CORNER2 &~C.BP &~C.WP))
				eval -= 10;
		}
	}

	if (eval < -18) {
		if ((WK & MASK_2CORNER1)) {
			eval += 8;
			if ((MASK_2CORNER1 &~C.BP &~C.WP))
				eval += 10;
		}

		if ((WK & MASK_2CORNER2)) {
			eval += 8;
			if ((MASK_2CORNER2 &~C.BP &~C.WP))
				eval += 10;
		}
	}

	int nWK = BitCount(WK);
	int nBK = BitCount(BK);
	int WPP = C.WP &~C.K;
	int BPP = C.BP &~C.K;

	// Advancing checkers in endgame
	if ((nWK * 2) >= nWhite || (nBK * 2) >= nBlack || (nBK + nWK) >= 4) {
		int Mul;
		if (nWK == nBK && (nWhite + nBlack) < (nWK + nBK + 2))
			Mul = 8;
		else
			Mul = 2;
		eval -= Mul * (BitCount((WPP & MASK_TOP)) - BitCount((WPP & MASK_BOTTOM)) - BitCount((BPP & MASK_BOTTOM)) + BitCount((BPP & MASK_TOP)));
	}

	static int BackRowGuardW[8] = { 0, 4, 5, 13, 4, 20, 18, 25 };
	static int BackRowGuardB[8] = { 0, 4, 5, 20, 4, 18, 13, 25 };
	int nBackB = 0, nBackW = 0;
	if ((nWK * 2) < nWhite) {
		nBackB = ((BPP) >> 1) & 7;
		eval -= BackRowGuardB[nBackB];
	}

	if ((nBK * 2) < nBlack) {
		nBackW = ((WPP) >> 28) & 7;
		eval += BackRowGuardW[nBackW];
	}

	// Number of Active Kings
	int nAWK = nWK;
	int nABK = nBK;

	// Kings trapped on back row
	if (WK & MASK_TRAPPED_W) {
		if ((S[3] & WK) && (S[7] &~C.empty) && (S[2] & BPP) && (S[11] & C.empty)) {
			eval -= 16;
			nAWK--;
		}

		if ((S[2] & WK) && (S[10] & C.empty) && (S[1] & BPP) && (S[3] & BPP)) {
			eval -= 14;
			nAWK--;
		}

		if ((S[1] & WK) && (S[9] & C.empty) && (S[0] & BPP) && (S[2] & BPP)) {
			eval -= 14;
			nAWK--;
		}

		if ((S[4] & WK) && (S[1] & BPP) && (S[8] &~C.empty)) {
			eval -= 10;
			nAWK--;
		}

		if ((S[0] & WK) && (S[1] & BPP)) {
			eval -= 22;
			nAWK--;
			if (S[8] &~C.empty)
				eval += 7;
		}
	}

	if (BK & MASK_TRAPPED_B) {
		if ((S[28] & BK) && (S[24] &~C.empty) && (S[29] & WPP) && (S[20] & C.empty)) {
			eval += 16;
			nABK--;
		}

		if ((S[29] & BK) && (S[21] & C.empty) && (S[30] & WPP) && (S[28] & WPP)) {
			eval += 14;
			nABK--;
		}

		if ((S[30] & BK) && (S[22] & C.empty) && (S[31] & WPP) && (S[29] & WPP)) {
			eval += 14;
			nABK--;
		}

		if ((S[27] & BK) && (S[30] & WPP) && (S[23] &~C.empty)) {
			eval += 10;
			nABK--;
		}

		if ((S[31] & BK) && (S[30] & WPP)) {
			eval += 22;
			nABK--;
			if (S[23] &~C.empty)
				eval -= 7;
		}
	}

	// Reward checkers that will king on the next move
	int BNearKing = 0;
	if ((BPP & MASK_7TH)) {

		// Checker can king next move
		if (C.CanBlackCheckerMove(BPP & MASK_7TH)) {
			eval -= KING_VAL - 5;
			BNearKing = 1;
		}
	}

	// Opponent has no Active Kings and backrow is still protected, so give a bonus
	if (nAWK > 0 && !BNearKing && nABK == 0) {
		eval += 20;
		if (nAWK > 1)
			eval += 10;
		if (BackRowGuardW[nBackW] > 10)
			eval += 15;
		if (BackRowGuardW[nBackW] > 20)
			eval += 20;
	}

	int WNearKing = 0;
	if ((WPP & MASK_2ND)) {

		// Checker can king next move
		if (C.CanWhiteCheckerMove(WPP & MASK_2ND)) {
			eval += KING_VAL - 5;
			WNearKing = 1;
		}
	}

	if (nABK > 0 && !WNearKing && nAWK == 0) {
		eval -= 20;
		if (nABK > 1)
			eval -= 10;
		if (BackRowGuardB[nBackB] > 10)
			eval -= 15;
		if (BackRowGuardB[nBackB] > 20)
			eval -= 20;
	}

	if (nWhite == nBlack) {

		// equal number of pieces, but one side has all kinged versus all but one kinged (or all kings)
		// score should be about equal, unless it's in a database, then don't reduce
		if (eval > 0 && eval < 200 && nWK >= nBK && nBK >= nWhite - 1)
			eval = (eval >> 1) + (eval >> 3);
		if (eval < 0 && eval > -200 && nBK >= nWK && nWK >= nBlack - 1)
			eval = (eval >> 1) + (eval >> 3);
	}

	return eval;
}

//
// MOVE EXECUTION
//
// ---------------------
//  Helper Functions for DoMove
// ---------------------
const UINT MASK_ODD_ROW = MASK_1ST | MASK_3RD | MASK_5TH | MASK_7TH;

void inline CBoard::DoSingleJump(int src, int dst, const int nPiece)
{
	int jumpedSq = ((dst + src) >> 1);
	if (S[jumpedSq] & MASK_ODD_ROW)
		jumpedSq += 1;	// correct for square number since the jumpedSq sometimes up 1 sometimes down 1 of the average
	int jumpedPiece = GetPiece(jumpedSq);

	// Update Piece Count
	if (jumpedPiece == WPIECE) {
		nWhite--;
	}
	else if (jumpedPiece == BPIECE) {
		nBlack--;
	}
	else if (jumpedPiece == BKING) {
		nBlack--;
		nPSq += KBoard[jumpedSq];
	}
	else if (jumpedPiece == WKING) {
		nWhite--;
		nPSq -= KBoard[jumpedSq];
	}

	// Update Hash Key
	HashKey ^= HashFunction[src][nPiece] ^ HashFunction[dst][nPiece] ^ HashFunction[jumpedSq][jumpedPiece];

	// Update the bitboards
	UINT BitMove = (S[src] | S[dst]);
	if (SideToMove == BLACK) {
		C.WP ^= BitMove;
		C.BP ^= S[jumpedSq];
		C.K &= ~S[jumpedSq];
	}
	else {
		C.BP ^= BitMove;
		C.WP ^= S[jumpedSq];
		C.K &= ~S[jumpedSq];
	}

	C.empty = ~(C.WP | C.BP);
	if (nPiece & KING)
		C.K ^= BitMove;
}

// This function will test if a checker needs to be upgraded to a king, and upgrade if necessary
void inline CBoard::CheckKing(const int src, const int dst, int const nPiece)
{
	if (dst <= 3) {
		nPSq += KBoard[dst];
		HashKey ^= HashFunction[dst][nPiece] ^ HashFunction[dst][WKING];
		C.K |= S[dst];
	}

	if (dst >= 28) {
		nPSq -= KBoard[dst];
		HashKey ^= HashFunction[dst][nPiece] ^ HashFunction[dst][BKING];
		C.K |= S[dst];
	}
}

void inline CBoard::UpdateSqTable(const int nPiece, const int src, const int dst)
{
	switch (nPiece) {
	case BKING:
		nPSq -= KBoard[dst] - KBoard[src];
		return;

	case WKING:
		nPSq += KBoard[dst] - KBoard[src];
		return;
	}
}

// ---------------------
//  Execute a Move

// ---------------------
int CBoard::DoMove(SMove &Move, int nType)
{
	unsigned int src = (Move.SrcDst & 63);
	unsigned int dst = (Move.SrcDst >> 6) & 63;
	unsigned int bJump = (Move.SrcDst >> 12);
	unsigned int nPiece = GetPiece(src);	// FIXME : AVOID GET PIECE HERE
	SideToMove ^= 3;
	HashKey ^= HashSTM;						// Flip hash stm
	if (bJump == 0) {

		// Update the bitboards
		UINT BitMove = S[src] | S[dst];
		if (SideToMove == BLACK)
			C.WP ^= BitMove;
		else
			C.BP ^= BitMove;
		if (nPiece & KING)
			C.K ^= BitMove;
		C.empty ^= BitMove;

		// Update hash values
		HashKey ^= HashFunction[src][nPiece] ^ HashFunction[dst][nPiece];

		UpdateSqTable(nPiece, src, dst);
		if (nPiece < KING) {
			CheckKing(src, dst, nPiece);
			return 1;
		}

		return 0;
	}

	DoSingleJump(src, dst, nPiece);
	if (nPiece < KING)
		CheckKing(src, dst, nPiece);

	// Double Jump?
	if (Move.cPath[0] == 33) {
		UpdateSqTable(nPiece, src, dst);
		return 1;
	}

	if (nType == HUMAN)
		return DOUBLEJUMP;

	for (int i = 0; i < 8; i++) {
		int nDst = Move.cPath[i];

		if (nDst == 33)
			break;

		if (!bCheckerBoard && nType == MAKEMOVE) {

			// pause a bit on displaying double jumps
			DrawBoard(*this);
			Sleep(300);
		}

		DoSingleJump(dst, nDst, nPiece);
		dst = nDst;
	}

	if (nPiece < KING)
		CheckKing(src, dst, nPiece);
	else
		UpdateSqTable(nPiece, src, dst);

	return 1;
}

// ------------------
// Position Copy & Paste Functions

// ------------------
void CBoard::ToFen(char *sFEN)
{
	char buffer[80];
	int i;
	if (SideToMove == WHITE)
		strcpy(sFEN, "W:");
	else
		strcpy(sFEN, "B:");

	strcat(sFEN, "W");
	for (i = 0; i < 32; i++) {
		if (GetPiece(i) == WKING)
			strcat(sFEN, "K");
		if (GetPiece(i) & WPIECE) {
			strcat(sFEN, _ltoa(FlipX(i) + 1, buffer, 10));
			strcat(sFEN, ",");
		}
	}

	if (strlen(sFEN) > 3)
		sFEN[strlen(sFEN) - 1] = NULL;
	strcat(sFEN, ":B");
	for (i = 0; i < 32; i++) {
		if (GetPiece(i) == BKING)
			strcat(sFEN, "K");
		if (GetPiece(i) & BPIECE) {
			strcat(sFEN, _ltoa(FlipX(i) + 1, buffer, 10));
			strcat(sFEN, ",");
		}
	}

	sFEN[strlen(sFEN) - 1] = '.';
}

int CBoard::FromFen(char *sFEN)
{
	if (!bCheckerBoard)
		DisplayText(sFEN);
	if ((sFEN[0] != 'W' && sFEN[0] != 'B') || sFEN[1] != ':')
		return 0;
	Clear();
	if (sFEN[0] == 'W')
		SideToMove = WHITE;
	if (sFEN[0] == 'B')
		SideToMove = BLACK;

	int nColor = 0, i = 0;
	while (sFEN[i] != 0 && sFEN[i] != '.' && sFEN[i - 1] != '.') {
		int nKing = 0;
		if (sFEN[i] == 'W')
			nColor = WPIECE;
		if (sFEN[i] == 'B')
			nColor = BPIECE;
		if (sFEN[i] == 'K') {
			nKing = 4;
			i++;
		}

		if (sFEN[i] >= '0' && sFEN[i] <= '9') {
			int sq = sFEN[i] - '0';
			i++;
			if (sFEN[i] >= '0' && sFEN[i] <= '9')
				sq = sq * 10 + sFEN[i] - '0';
			SetPiece(FlipX(sq - 1), nColor | nKing);
		}

		i++;
	}

	SetFlags();
	g_StartBoard = *this;
	return 1;
}


//
int CBoard::MakeMovePDN(int src, int dst)
{
	CMoveList Moves;
	if (SideToMove == BLACK)
		Moves.FindMovesBlack(C);
	if (SideToMove == WHITE)
		Moves.FindMovesWhite(C);
	for (int x = 1; x <= Moves.numMoves; x++) {
		int nMove = Moves.Moves[x].SrcDst;
		if ((nMove & 63) == src) {
			if (((nMove >> 6) & 63) == dst || GetFinalDst(Moves.Moves[x]) == dst) {
				DoMove(Moves.Moves[x], SEARCHED);
				g_GameMoves[g_numMoves++] = Moves.Moves[x];
				g_GameMoves[g_numMoves].SrcDst = 0;
				return 1;
			}
		}
	}

	return 0;
}

//

//
int CBoard::FromPDN(char *sPDN)
{
	int i = 0;
	int nEnd = (int)strlen(sPDN);
	int src = 0, dst = 0;
	char sFEN[512];

	StartPosition(1);

	while (i < nEnd) {
		if
		(
			!memcmp(&sPDN[i], "1-0", 3) ||
			!memcmp(&sPDN[i], "0-1", 3) ||
			!memcmp(&sPDN[i], "1/2-1/2", 7) ||
			sPDN[i] == '*'
		) break;
		if (!memcmp(&sPDN[i], "[FEN \"", 6)) {
			i += 6;

			int x = 0;
			while (sPDN[i] != '"' && i < nEnd)
				sFEN[x++] = sPDN[i++];
			sFEN[x++] = 0;
			FromFen(sFEN);
		}

		if (sPDN[i] == '[')
			while (sPDN[i] != ']' && i < nEnd)
				i++;
		if (sPDN[i] == '{')
			while (sPDN[i] != '}' && i < nEnd)
				i++;
		if (sPDN[i] >= '0' && sPDN[i] <= '9' && sPDN[i + 1] == '.')
			i++;
		if (sPDN[i] >= '0' && sPDN[i] <= '9') {
			int sq = sPDN[i] - '0';
			i++;
			if (sPDN[i] >= '0' && sPDN[i] <= '9')
				sq = sq * 10 + sPDN[i] - '0';
			src = FlipX(sq - 1);
		}

		if ((sPDN[i] == '-' || sPDN[i] == 'x') && sPDN[i + 1] >= '0' && sPDN[i + 1] <= '9') {
			i++;

			int sq = sPDN[i] - '0';
			i++;
			if (sPDN[i] >= '0' && sPDN[i] <= '9')
				sq = sq * 10 + sPDN[i] - '0';
			dst = FlipX(sq - 1);

			MakeMovePDN(src, dst);
		}

		i++;
	}

	DisplayText(sPDN);
	return 1;
}

int CBoard::ToPDN(char *sPDN)
{
	sPDN[0] = 0;

	int i = 0, j = 0;
	char cType;
	j += sprintf(sPDN + j, "[Event \"%s game\"]\015\012", g_sNameVer);

	char sFEN[512];
	g_StartBoard.ToFen(sFEN);
	if (strcmp(sFEN, "B:W24,23,22,21,28,27,26,25,32,31,30,29:B4,3,2,1,8,7,6,5,12,11,10,9.")) {
		j += sprintf(sPDN + j, "[SetUp \"1\"]\015\012");
		j += sprintf(sPDN + j, "[FEN \"%s\"]\015\012", sFEN);
	}

	while (g_GameMoves[i].SrcDst != 0) {
		if ((i % 2) == 0)
			j += sprintf(sPDN + j, "%d. ", (i / 2) + 1);

		unsigned int src = (g_GameMoves[i].SrcDst & 63);
		unsigned int dst = GetFinalDst(g_GameMoves[i]);
		if ((g_GameMoves[i].SrcDst >> 12))
			cType = 'x';
		else
			cType = '-';
		j += sprintf(sPDN + j, "%d%c%d ", FlipX(src) + 1, cType, FlipX(dst) + 1);
		i++;
		if ((i % 12) == 0)
			j += sprintf(sPDN + j, "\015\012");
	}

	sprintf(sPDN + j, "*");
	return 1;
}
