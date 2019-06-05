#include <stdio.h>
#pragma warning(disable : 4091)
#include <shlwapi.h>
#include <shlobj.h>
#include "logfile.h"


static char fullfilename[MAX_PATH];


char *logfilename(void)
{
	return(fullfilename);
}


char *build_logfilename(char *enginename, char *filename)
{
	if (fullfilename[0] == 0) {

		/* Get path for the My Documents directory.
		 * On WinXP, this is \Documents and Settings\<user>\My Documents.
		 * On Vista, this is \Users\<user>\Documents.
		 */
		if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, 0, fullfilename))) {

			/* Create the directories in case they do not exist. */
			PathAppend(fullfilename, "Jon Kreuzer");
			CreateDirectory(fullfilename, NULL);

			PathAppend(fullfilename, enginename);
			CreateDirectory(fullfilename, NULL);

			PathAppend(fullfilename, filename);
		}
	}
	return(fullfilename);
}


void init_logfile(char *enginename, char *filename)
{
	FILE *fp;

	build_logfilename(enginename, filename);

	/* Erase any previous logfiles. */
	fp = fopen(fullfilename, "w");
	fclose(fp);
}


/*
 * Write a message string to the log file.
 */
void log_msg(char *pvstr)
{
	FILE *fp;

	fp = fopen(logfilename(), "a");
	if (fp) {
		fprintf(fp, "%s", pvstr);
		fclose(fp);
	}
}


void log_msg(const char *fmt, ...)
{
	FILE *fp;
	va_list args;
	va_start(args, fmt);

	if (fmt == NULL)
		return;

	fp = fopen(logfilename(), "a");
	if (fp == NULL)
		return;

	vfprintf(fp, fmt, args);
	fclose(fp);
}
