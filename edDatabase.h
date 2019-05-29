#pragma once
#include "guiCheckers.h"

void InitializeEdsDatabases(SDatabaseInfo& dbInfo);
int QueryEdsDatabase(const CBoard &Board, int ahead);
