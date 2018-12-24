#ifndef __output_h_included__
#define __output_h_included__

const char * mtostr (int m);
void do_error (const char *format, ...);
void do_line (const char *description, const char *content);
void addlist (char *old, const char *new);

#endif
