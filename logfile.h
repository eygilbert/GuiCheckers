void init_logfile(char *enginename, char *filename);
char *build_logfilename(char *enginename, char *filename);
char *logfilename(void);
void log_msg(char *pvstr);
void log_msg(const char *fmt, ...);

