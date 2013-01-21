/* file wtlib.h */
#ifndef _WTLIB_H
#define _WTLIB_H

typedef enum _TS {
	Traditional,
	Simplified
} TS;

void wtpen_init(void);
void set_wtlib(void);
void wtlib_done(void);

#endif /* WTLIB_H */
