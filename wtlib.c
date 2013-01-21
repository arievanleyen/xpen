/* file : wtlib.c */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <glib.h>

#include "wtpen.h"
#include "wtlib.h"
#include "conv.h"
#include "coordinate.h"

#define RAM_SIZE 2048
char *RamAddress = NULL;
static char *LibStartAddress;
TS ts = Simplified;
extern KeyState keystate;
extern int mode;


char *get_real_path();

void wtpen_init() {
    conv_open();
    conv_open_utf8 ();
}

void set_wtlib() {
    FILE * WTPENLibFile;
    char filename[256];
    long TotalFileLength;
	gchar *pad;

    RamAddress = (char *) g_malloc(RAM_SIZE);

	pad = get_real_path();
    if (ts == Simplified)
        sprintf(filename, "%s/data/WTPDA_GB2312.lib", pad);
    else
        sprintf(filename, "%s/data/WTPDA_traditional.lib", pad);

    if ((WTPENLibFile = fopen(filename, "rb")) == NULL)
		fprintf(stderr, "Error: %s (%d)\n", __FILE__, __LINE__);

    fseek(WTPENLibFile, 0, SEEK_END);
    TotalFileLength = ftell(WTPENLibFile);
    fseek(WTPENLibFile, 0, SEEK_SET);

    LibStartAddress = (char *) g_malloc(TotalFileLength);
	if (fread(LibStartAddress, sizeof(char), TotalFileLength, WTPENLibFile))	;	// workaround to prevent a silly warning
    fclose(WTPENLibFile);

    if (WTRecognizeInit(RamAddress, 5000, LibStartAddress))
		fprintf(stderr, "Error: %s (%d)\n", __FILE__, __LINE__);

    if (WTSetRange(mode) != 0)
		fprintf(stderr, "Error: %s (%d)\n", __FILE__, __LINE__);
    mode = 0;

	g_free(pad);
}

void  wtlib_done() {
    WTRecognizeEnd();
    g_free(LibStartAddress);
    g_free(RamAddress);
    conv_close ();
    conv_close_utf8 ();
}
