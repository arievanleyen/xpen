#ifndef __WTPEN_HEAD_H
#define __WTPEN_HEAD_H

#include <gtk/gtk.h>

typedef struct {		// for the buttons/entries on the preference-tab (keys)
	GtkWidget *button;
	GtkWidget *entry;
	int key;				// keycode
	gchar *label;			// label
} DefKeys;

// used to store configuration settings
typedef struct {
	int key;				// the key to bind
	unsigned short mode;	// the recognition mode attached to that key
} HotKey;

typedef struct {
	int x, y;			// window position
	int lx, ly;			// lookup window position
	int dx, dy;			// loopup window size
	double opacity;		// opacity of the main window
	gchar *font;	// font to be used for the 9 other choices
	short speed;		// recognition speed (1..10)
	long lijnkleur;		// de kleur van de lijn
	long rasterkleur;	// de kleur van het raster
	gboolean raster;	// show raster
	int paste[2];		// paste key-sequence
	int backspace[2];	// backspace key-sequence
	gchar *device;		// input device
	HotKey keymode[7];	// key/mode settings for each mode
	DefKeys defkey[13];	// 13 predefined labels for keycodes
	gboolean tips;		// enable tooltips
	gboolean speech;	// enable speech
	int voice;			// voice number to be used (0=male, 1=female, 2=teacher)
} KeySettings;

typedef struct {
	int state;			// keystate (pressed or released)
	int key;			// the key number
} KeyState;

typedef struct {
	GtkWidget *check;		// checkbutton
	GtkWidget *button;		// button
	unsigned short bit;		// recognition bit set
	unsigned short mask;	// recognition bit unset
} ModeBits;

#define GET_DEF_KEY		1
#define GET_HOT_KEY		2

// bits for recognition setting (see WTSetRange)
#define GB1			0x0001		// GB ordered by pinyin
#define GB2			0x0002		// GB ordered by radicals
#define BIG5		0x0004		// traditional characters
#define DIGITS		0x0020		// the digits 0..9 and the minus sign
#define LOWERCASE	0x0040		// lowercase alphabeth
#define UPPERCASE	0x0080		// uppercase alphabeth
#define PUNC		0x0100		// punctuation symbols
#define DEFAULT		0x1000		// default

#define KEY_PRESS 	2
#define KEY_RELEASE 3
#define PRESS		TRUE
#define RELEASE 	FALSE

#define NUMKEYS	120		// the maximum number of keys on a keyboard

// key (scan) codes that might not be the same on other keyboards
// that is why there is an additional page in the preferences to define other labels
#define CTRL_L 	37
#define SHIFT_L 50
#define ALT_L 	64
#define CTRL_R 	105
#define SHIFT_R 62
#define ALT_R 	108
#define ESC		9
#define CAPS	66
#define NUMS	77
#define SCROLLS 78
#define PAUSE	127

//#include <X11/keysymdef.h>
/*
 * Variable type definition
 */
typedef short	WTError;
typedef long	WT_int32;	/*Defines four byte variables*/

/*
 * Define Macros
 */
//#define WTPEN_abs(value)	  (((value)<0)?(-(value)):(value))

// define to use a thread for the focus detection loop (else it is an on-iddle routine)
//#define WITH_THREADS

/*
 * Declaration of functioins
 */
#ifdef __cplusplus
extern "C" {
#endif

WTError WTRecognizeInit(char *RamAddress,WT_int32 RamSize,char *LibStartAddress);
/*
 *	function： Initialization. Call this function once to start the recognition system
 *	Parameter:
 *		RamAddress:
 *			Memory to use, at least 2K
 *		RamSize:
 *			RamAddress Aims spatial size
 *		LibStartAddress:
 *			start of library in the memory
 *	Returns value:
 *		0- success
 *		other values- error
 */

WTError WTRecognize(unsigned char * PointBuf, short PointsNumber, unsigned short *CandidateResult);
/*
 *	function: Recognitize. carries on recognition based on the input path, finally places in CandidateResult
 *	Parameter:
 *		PointBuf：
 *			input: (X1,Y1,X2,Y2,...0xff,0x00,Xn,Yn,Xn+1,Yn+1,
 *			...0xff,0x00,0xff,0xff),In which spot is right (0xff,0x00) Expressed that a stroke ended
 *			To (0xff,0xff) expressed that this character finished
 *           In the recognition process this part of memory's content will be rewritten.
 *		PointsNumber:
 *			Altogether path points, also separately calculated a spot including the stroke and the character end mark.
 *		CandidateResult:
 *			The depositing recognition result, opens by the main tuning function, should have a 2*10=20 byte at least
 *	Returns value:
 *		0--Expresses successfully
 *		Other values--The expression makes a mistake
 *	  CandidateResult If the value is the character which 0xffff is resists to know.
 */

WTError WTRecognizeEnd();
/*
 *	Function: Essential other work. This function when withdrawal recognition system transfers one time then
 *	parameter:none
 *	Returns value:
 *		0--Expresses successfully
 *		Other values--The expression makes a mistake
 */

WTError WTSetRange(short Range);
/*
 *	Function: When the establishment recognition scope, the function parameter's corresponding position establishment is 1,
 *				the recognition scope will include the corresponding character repertoire,
 *	 			When the establishment is 0, the corresponding character repertoire will not distinguish in the scope.
 *				Moreover, whether establishes to have an effect is also decided by the time
 *	 			Whether other storehouse does support the corresponding character repertoire.
 *	parameters:
 *		Range:
 *			bit0 GB level 1 (3755)
 * 			bit1 GB level 2 (6763)
 *			bit2 Traditional character
 *			bit5 Arabic numeral (digits)
 *			bit6 Lowercase letter
 *			bit7 Uppercase letter
 *			bit8 Punctuation mark
 *			bit14 Laboratory stipulation function symbol
 * 			bit15 USER Specific
 *			Other retentions
 *	Returns value:
 *		0--Expresses successfully
 *		Other values--The expression makes a mistake
 */

WTError WTSetSpeed(short Speed);
/*
 *	Function: Establishment recognition speed (Limited function)
 *	parameter：
 *		Speed:
 *			0-10, 0 slowest, 10 quickest, a speed quicker recognition rate is lower
 *			If does not transfer this function, the default value is 6
 *	return value:
 *		0--success
 *		Other values--The expression makes a mistake
 */


#ifdef __cplusplus
}
#endif

#endif
