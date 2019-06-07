#pragma once
#include "guiCheckers.h"

bool InitializeEdsDatabases(SDatabaseInfo& dbInfo);
int QueryEdsDatabase(const CBoard &Board, int ahead);
