#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "output.h"

const char * mtostr (int m) {
    static char buffer [32];
    if (m < 1024) snprintf (buffer, 31, "%dM", m);
    else snprintf (buffer, 31, "%dG", m/1024);

    return buffer;
}

void do_error (const char *format, ...) {
    char linebuf[256];
    va_list vl;

    va_start (vl, format);
    vsnprintf (linebuf, sizeof (linebuf), format, vl);
    va_end (vl);
    strncat (linebuf, "\n", sizeof (linebuf) - 1);
    fprintf (stderr, "%s", linebuf);
}

void do_line (const char *description, const char *content) {
    if (description)
        printf ("%-20s%c %s\n", description, description[0] ? ':' : ' ', content);
    else
        printf ("\n");
}

void addlist (char *old, const char *new) {
    if (old[0])
        strcat (old, " ");
    strcat (old, new);
}
