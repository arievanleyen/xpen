/* file : conv.c */
#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <iconv.h>

#include "conv.h"

static char gb2312[] = "GB2312";
static char gbk[] = "GBK";
static char unicode[] = "UNICODE";
static char *lang = gb2312;

static char locale[30];
static iconv_t iconvd=0, iconvd_utf8=0;

int is_valid_gb2312(char []);
int is_valid_gbk(char []);
int is_valid_unicode(char []);
int (* is_valid_char)(char []);

void get_locale() {
    char *p;
    char *point;
    p = getenv("LANG");
    point = strchr(p, '.');
    if (point) {
        point++;
        strcpy(locale, point);
    } else {
        strcpy(locale, "GB2312");
    }
    //fprintf(stderr, "WTPEN : get locale : %s\n", locale);
    set_lang_gb2312();
}

int
conv_open() {
    iconvd = iconv_open(locale, "UTF-8");
    if (!iconvd)
        return 0;
    else
        return 1;
}
//ａ		// output by xpen
//ａ		U+ff41	in GB2312
//a		U+0061	in GB2312
// verschil = fee0



int
conv_open_utf8() {
    iconvd_utf8 = iconv_open("UTF-8", "GB18030");
    if (!iconvd_utf8)
        return 0;
    else
        return 1;
}

void
conv_close() {
    if (iconvd) {
        iconv_close(iconvd);
        iconvd = 0;
    }
}

void
conv_close_utf8() {
    if (iconvd_utf8) {
        iconv_close(iconvd_utf8);
        iconvd_utf8 = 0;
    }
}

int
conv_string(char dest[], int dlen, char orig[], int olen) {
    int in_len = olen;
    int out_len = dlen;

	if ( (int) iconv(iconvd, &orig, (size_t *) &in_len, &dest, (size_t *) &out_len) == -1)
        return 0;
    dest[9] = 0;
    return 1;
}

int
conv_string_utf8(char dest[], int dlen, char orig[], int olen) {
    int in_len = olen;
    int out_len = dlen;

    if ( (int) iconv(iconvd_utf8, &orig, (size_t *) &in_len, &dest, (size_t *) &out_len) == -1)
        return 0;
    dest[9] = 0;
    return 1;
}

int
need_conv() {
    if (strstr(locale, "UTF-8") != 0)
        return 0;
    else
        return 1;
}

void
set_lang_gb2312() {
    lang = gb2312;
    is_valid_char = is_valid_gb2312;
}

void
set_lang_gbk() {
    lang = gbk;
    is_valid_char = is_valid_gbk;
}

void
set_lang_unicode() {
    lang = unicode;
    is_valid_char = is_valid_unicode;
}

int is_valid_gb2312(char string[]) {
    if (string[0]> '\xb0' && string[0]< '\xf7') {
        if (string[1]>'\xa1' && string[1]<'\xfe')
            return 1;
    }
    return 0;
}

int is_valid_gbk(char string[]) {
    return 0;
}

int is_valid_unicode(char string[]) {
    return 0;
}
