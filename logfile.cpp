#include <ShlObj.h>
#include <Shlwapi.h>
#include <stdio.h>
#include "logfile.h"

static char logfilename[MAX_PATH];

void init_logfile(void)
{
	FILE *fp;
	char path[MAX_PATH];

	/* Create directories for GuiCheckers under My Documents. */
	if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, 0, path))) {
		PathAppend(path, "GuiCheckers");
		CreateDirectory(path, NULL);
	}
	sprintf(logfilename, "%s\\GuiCheckers.log", path);
	fp = fopen(logfilename, "w");
	fclose(fp);
}

void log(const char *fmt, ...)
{
	FILE *fp;
	va_list args;
	va_start(args, fmt);

	if (fmt == NULL)
		return;

	fp = fopen(logfilename, "a");
	if (fp == NULL)
		return;

	vfprintf(fp, fmt, args);
	fclose(fp);
}
