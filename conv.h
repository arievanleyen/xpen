/* file : conv.h */
#ifndef _CONV_H
#define _CONV_H

void get_locale (void);
int need_conv (void);
int conv_string (char dest[], int dlen, char orig[], int olen);
int conv_string_utf8 (char dest[], int dlen, char orig[], int olen);
int conv_open (void);
int conv_open_utf8 (void);
void conv_close (void);
void conv_close_utf8 (void);
void set_lang_gb2312 (void);
void set_lang_gbk (void);
void set_lang_unicode (void);

#endif /* CONV_H */
