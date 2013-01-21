#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <gtk/gtk.h>
#include <X11/Intrinsic.h>
#include <X11/extensions/record.h>
#include <stdio.h>
#include <stdlib.h>
#include "wtlib.h"
#include "wtpen.h"

//#define PATH get_real_path()	// path to the fysical location of program

#define SETPIXEL(c) \
	c.pixel = (c.red/0xff); \
	c.pixel <<= 8; \
	c.pixel |= (c.green/0xff); \
	c.pixel <<= 8; \
	c.pixel |= (c.blue/0xff);

extern GtkWidget *preferences;
extern GtkAdjustment *opacity, *speed;
extern GtkWidget *colorbutton1, *colorbutton2;
extern GtkWidget *knop[9], *debug;
extern GtkWidget *checkbutton1, *checkbutton9;
extern GtkWidget *combo;
extern GtkWidget *fontbutton;
extern GtkWidget *papier;
extern GtkWidget *window;
extern GtkWidget *checkbutton8;
extern GtkWidget *combobox1;
extern GdkColor lijnkleur, rasterkleur;
extern GKeyFile *settings;
extern KeySettings conf;
extern GtkWidget *entry1, *entry2;
extern ModeBits modebits[7];
extern int mode;			// which recognize modes are enabled
extern int hotkey[NUMKEYS];	// stores the recognition mode that is used when pressing that key
extern KeyState keystate;
extern bool ascii;
extern int skipkeys;
extern gchar *pad;
//extern DefKeys defkeys[13];

XRecordContext rc;
short button_nr, action = 0;

char *key2string (int key, int add);
void selecteer(TS keus);
//char *get_real_path();
void clear_draw_area(GtkWidget *widget);

// load preferences from file
void load_preferences (char *filepad) {
    char txt[100];
    GString *label = g_string_new_len("", 20);
    int i, p, choice = -1;
    GdkDevice *device, *nodevice;
    GList *list;
    TS keus;

    if (settings == NULL) settings = g_key_file_new();
    if (g_key_file_load_from_file(settings, filepad, G_KEY_FILE_NONE, NULL) == FALSE) {	// set defaults
        gchar *data;
        gsize len;

		// default settings when xpen.cfg not found
		g_key_file_set_integer (settings, "display", "x", -1);
		g_key_file_set_integer (settings, "display", "y", -1);
		g_key_file_set_integer (settings, "display", "lx", -1);
		g_key_file_set_integer (settings, "display", "ly", -1);
		g_key_file_set_integer (settings, "display", "dx", 300);
		g_key_file_set_integer (settings, "display", "dy", 400);
        g_key_file_set_double (settings, "display", "opacity", 0.7);
        g_key_file_set_string (settings, "display", "font", gtk_font_button_get_font_name(GTK_FONT_BUTTON(fontbutton)));
        g_key_file_set_boolean (settings, "display", "tooltips", TRUE);
        g_key_file_set_string (settings, "reader", "device", "stylus");
        g_key_file_set_integer (settings, "reader", "speed", 6);
        g_key_file_set_uint64 (settings, "reader", "lijnkleur", 0x514942);
        g_key_file_set_uint64 (settings, "reader", "rasterkleur", 0xAB8C6D);
        g_key_file_set_boolean (settings, "reader", "raster", TRUE);
        g_key_file_set_boolean (settings, "reader", "speech", FALSE);
        g_key_file_set_string (settings, "reader", "command", "aplay -q");
        g_key_file_set_integer (settings, "reader", "voice", 0);
        g_key_file_set_integer (settings, "keys", "paste0", 37);
        g_key_file_set_integer (settings, "keys", "paste1", 55);
        g_key_file_set_integer (settings, "keys", "backspace0", 0);
        g_key_file_set_integer (settings, "keys", "backspace1", 22);

        g_key_file_set_integer (settings, "GB1", "key", 0);
        g_key_file_set_integer (settings, "GB1", "mode", 0);
        g_key_file_set_integer (settings, "BIG5", "key", CTRL_R);
        g_key_file_set_integer (settings, "BIG5", "mode", BIG5);
        g_key_file_set_integer (settings, "DIGITS", "key", ALT_L);
        g_key_file_set_integer (settings, "DIGITS", "mode", DIGITS);
        g_key_file_set_integer (settings, "LOWERCASE", "key", ALT_L);
        g_key_file_set_integer (settings, "LOWERCASE", "mode", LOWERCASE);
        g_key_file_set_integer (settings, "UPPERCASE", "key", 0);
        g_key_file_set_integer (settings, "UPPERCASE", "mode", 0);
        g_key_file_set_integer (settings, "PUNC", "key", 0);
        g_key_file_set_integer (settings, "PUNC", "mode", 0);
        g_key_file_set_integer (settings, "DEFAULT", "key", ESC);
        g_key_file_set_integer (settings, "DEFAULT", "mode", (GB1|GB2|DIGITS));

		g_key_file_set_string (settings, "keydefs", "label1", "Control-Left");
		g_key_file_set_integer (settings, "keydefs", "key1", CTRL_L);
		g_key_file_set_string (settings, "keydefs", "label2", "Control-Right");
		g_key_file_set_integer (settings, "keydefs", "key2", CTRL_R);
		g_key_file_set_string (settings, "keydefs", "label3", "Shift-Left");
		g_key_file_set_integer (settings, "keydefs", "key3", SHIFT_L);
		g_key_file_set_string (settings, "keydefs", "label4", "Shift-Right");
		g_key_file_set_integer (settings, "keydefs", "key4", SHIFT_R);
		g_key_file_set_string (settings, "keydefs", "label5", "Alt-Left");
		g_key_file_set_integer (settings, "keydefs", "key5", ALT_L);
		g_key_file_set_string (settings, "keydefs", "label6", "Alt-Right");
		g_key_file_set_integer (settings, "keydefs", "key6", ALT_R);
		g_key_file_set_string (settings, "keydefs", "label7", "Escape");
		g_key_file_set_integer (settings, "keydefs", "key7", ESC);
		g_key_file_set_string (settings, "keydefs", "label8", "Caps-Lock");
		g_key_file_set_integer (settings, "keydefs", "key8", CAPS);
		g_key_file_set_string (settings, "keydefs", "label9", "Num-Lock");
		g_key_file_set_integer (settings, "keydefs", "key9", NUMS);
		g_key_file_set_string (settings, "keydefs", "label10", "Scroll-Lock");
		g_key_file_set_integer (settings, "keydefs", "key10", SCROLLS);
		g_key_file_set_string (settings, "keydefs", "label11", "Pause/Break");
		g_key_file_set_integer (settings, "keydefs", "key11", PAUSE);
		g_key_file_set_string (settings, "keydefs", "label12", "not set");
		g_key_file_set_integer (settings, "keydefs", "key12", 0);
		g_key_file_set_string (settings, "keydefs", "label13", "not set");
		g_key_file_set_integer (settings, "keydefs", "key13", 0);

        data = g_key_file_to_data (settings, &len, NULL);	// save defaults
        g_file_set_contents (filepad, data, len, NULL);
        g_free(data);
    }
    // fill the preferences structure
    conf.x 				= g_key_file_get_integer (settings, "display", "x", NULL);
    conf.y 				= g_key_file_get_integer (settings, "display", "y", NULL);
    conf.lx 			= g_key_file_get_integer (settings, "display", "lx", NULL);
    conf.ly 			= g_key_file_get_integer (settings, "display", "ly", NULL);
    conf.dx 			= g_key_file_get_integer (settings, "display", "dx", NULL);
    conf.dy 			= g_key_file_get_integer (settings, "display", "dy", NULL);
    conf.opacity 		= g_key_file_get_double (settings, "display", "opacity", NULL);
    conf.tips			= g_key_file_get_boolean (settings, "display", "tooltips", NULL);
    conf.font 			= g_key_file_get_string (settings, "display", "font", NULL);
    conf.device 		= g_key_file_get_string (settings, "reader", "device", NULL);
    conf.speed 			= g_key_file_get_integer (settings, "reader", "speed", NULL);
    conf.lijnkleur		= g_key_file_get_uint64 (settings, "reader", "lijnkleur", NULL);
    conf.rasterkleur	= g_key_file_get_uint64 (settings, "reader", "rasterkleur", NULL);
    conf.raster 		= g_key_file_get_boolean (settings, "reader", "raster", NULL);
	conf.speech 		= g_key_file_get_boolean (settings, "reader", "speech", NULL);
	conf.voice	 		= g_key_file_get_integer (settings, "reader", "voice", NULL);
    conf.paste[0] 		= g_key_file_get_integer (settings, "keys", "paste0", NULL);
    conf.paste[1] 		= g_key_file_get_integer (settings, "keys", "paste1", NULL);
    conf.backspace[0]	= g_key_file_get_integer (settings, "keys", "backspace0", NULL);
    conf.backspace[1]	= g_key_file_get_integer (settings, "keys", "backspace1", NULL);
    conf.keymode[0].key	= g_key_file_get_integer (settings, "GB1", "key", NULL);
	conf.keymode[0].mode	= g_key_file_get_integer (settings, "GB1", "mode", NULL);
    conf.keymode[1].key	= g_key_file_get_integer (settings, "BIG5", "key", NULL);
	conf.keymode[1].mode	= g_key_file_get_integer (settings, "BIG5", "mode", NULL);
	conf.keymode[2].key	= g_key_file_get_integer (settings, "DIGITS", "key", NULL);
	conf.keymode[2].mode	= g_key_file_get_integer (settings, "DIGITS", "mode", NULL);
    conf.keymode[3].key	= g_key_file_get_integer (settings, "LOWERCASE", "key", NULL);
	conf.keymode[3].mode	= g_key_file_get_integer (settings, "LOWERCASE", "mode", NULL);
    conf.keymode[4].key	= g_key_file_get_integer (settings, "UPPERCASE", "key", NULL);
	conf.keymode[4].mode	= g_key_file_get_integer (settings, "UPPERCASE", "mode", NULL);
    conf.keymode[5].key	= g_key_file_get_integer (settings, "PUNC", "key", NULL);
	conf.keymode[5].mode	= g_key_file_get_integer (settings, "PUNC", "mode", NULL);
    conf.keymode[6].key	= g_key_file_get_integer (settings, "DEFAULT", "key", NULL);
	conf.keymode[6].mode	= g_key_file_get_integer (settings, "DEFAULT", "mode", NULL);

	// set speech + command
	GTK_TOGGLE_BUTTON(checkbutton9)->active = conf.speech;

	gtk_combo_box_set_active(GTK_COMBO_BOX(combobox1), conf.voice);

	for (i=0; i<13; i++) {
		g_string_sprintf(label, "key%d", i+1);
		conf.defkey[i].key = g_key_file_get_integer (settings, "keydefs", label->str, NULL);
		g_string_sprintf(label, "label%d", i+1);
		conf.defkey[i].label = g_key_file_get_string (settings, "keydefs", label->str, NULL);
	}

	// start setting all widgets with the values from the configuration (conf)

	// set tooltips
    GTK_TOGGLE_BUTTON(checkbutton8)->active = conf.tips;

	// set the 13 buttons/entries in preferences
	for (i=0; i<13; i++) {
		if (conf.defkey[i].key > 0) {
			g_string_sprintf(label, "key = %d", conf.defkey[i].key);
			gtk_button_set_label(GTK_BUTTON(conf.defkey[i].button), label->str);
			gtk_entry_set_text(GTK_ENTRY(conf.defkey[i].entry), conf.defkey[i].label);
		} else {
			gtk_button_set_label(GTK_BUTTON(conf.defkey[i].button), "not set");
			gtk_entry_set_text(GTK_ENTRY(conf.defkey[i].entry), "");
		}
	}
	g_string_free(label, TRUE);

	// set the default recognition mode
	mode = conf.keymode[6].mode;
	keus = (mode & BIG5) ? Traditional : Simplified;
	selecteer(keus);

	// set the labels of the hotkeys (preferences)
	for (i=0; i<7; i++) {
		bool found;

		hotkey[conf.keymode[i].key] |= conf.keymode[i].mode;		// set the hotkeys
		found = FALSE;
		for (p=0; p<13; p++) {
			if (conf.defkey[p].key > 0) {
				if (conf.keymode[i].key == conf.defkey[p].key) {
					strcpy(txt, conf.defkey[p].label);
					found = TRUE;
					break;
				}
			}
		}
		if (found == FALSE) {
			if (conf.keymode[i].key > 0)
				sprintf(txt, "key = %d", conf.keymode[i].key);
			else
				strcpy(txt, "not set");
		}
		gtk_button_set_label(GTK_BUTTON(modebits[i].button), txt);

		// set the default checkboxes (preferences)
		if (conf.keymode[6].mode & GB1)
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(modebits[0].check), TRUE);
		if (conf.keymode[6].mode & BIG5)
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(modebits[1].check), TRUE);
		if (conf.keymode[6].mode & DIGITS)
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(modebits[2].check), TRUE);
		if (conf.keymode[6].mode & LOWERCASE)
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(modebits[3].check), TRUE);
		if (conf.keymode[6].mode & UPPERCASE)
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(modebits[4].check), TRUE);
		if (conf.keymode[6].mode & PUNC)
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(modebits[5].check), TRUE);
	}

    // fill combobox with input-devices
    list = gdk_devices_list();
    device = (GdkDevice *) list->data;	// default device
    nodevice = NULL;
    for (p=0; list->next; p++) {
        strcpy(txt, gdk_device_get_name((GdkDevice *)list->data));
        gtk_combo_box_append_text(GTK_COMBO_BOX(combo), txt);
        if (strstr(txt, conf.device)) {
            device = (GdkDevice *) list->data;
            choice = p;
        }
        nodevice = (GdkDevice *) list->data;
        list = list->next;
    }
    if (choice == -1) {		// prefered device not found
		choice = p-1;
		device = nodevice;
    }

    //g_list_free(list);
    gtk_combo_box_set_active(GTK_COMBO_BOX(combo), choice);
    gdk_device_set_source (device, GDK_SOURCE_PEN);
    gdk_device_set_mode(device, GDK_MODE_SCREEN);

    // set line color
    lijnkleur.pixel = conf.lijnkleur;
    sprintf(txt, "#%.6X", lijnkleur.pixel);
    gdk_color_parse(txt, &lijnkleur);
    gtk_color_button_set_color(GTK_COLOR_BUTTON(colorbutton1),  &lijnkleur);

    // set raster color
    rasterkleur.pixel = conf.rasterkleur;
    sprintf(txt, "#%.6X", rasterkleur.pixel);
    gdk_color_parse(txt, &rasterkleur);
    gtk_color_button_set_color(GTK_COLOR_BUTTON(colorbutton2),  &rasterkleur);

    // set raster on/off check button
    GTK_TOGGLE_BUTTON(checkbutton1)->active = conf.raster;

    // set main window opacity slider
    gtk_adjustment_set_value(opacity, conf.opacity);

    // set the 9 candidate buttons with a default font
    for (i=0; i< 9; i++) {
		PangoFontDescription *pfont;
    	pfont = pango_font_description_from_string(conf.font);
        gtk_widget_modify_font (GTK_WIDGET(knop[i]), pfont );
        pango_font_description_free(pfont);
    }

    // set the font selection button with the font title
    gtk_font_button_set_font_name(GTK_FONT_BUTTON(fontbutton), conf.font);

    // set the recognize speed slider
    gtk_adjustment_set_value(speed, conf.speed);
    WTSetSpeed(conf.speed);

    // set the default paste and backspace entry fields
    gtk_entry_set_text(GTK_ENTRY(entry1), key2string(conf.paste[1], conf.paste[0]));
    gtk_entry_set_text(GTK_ENTRY(entry2), key2string(conf.backspace[1], conf.backspace[0]));
}

// store all preference locally
extern "C" G_MODULE_EXPORT void store_preferences (GtkObject *object, gpointer user_data) {
	TS keus;

	// get color and fill the pixel values
    gtk_color_button_get_color(GTK_COLOR_BUTTON(colorbutton1),  &lijnkleur);
    SETPIXEL(lijnkleur);
    gtk_color_button_get_color(GTK_COLOR_BUTTON(colorbutton2),  &rasterkleur);
    SETPIXEL(rasterkleur);

	// get some conf settings that are not set already
    conf.opacity 		= gtk_adjustment_get_value(GTK_ADJUSTMENT(opacity));
    conf.font 			= strdup(gtk_font_button_get_font_name(GTK_FONT_BUTTON(fontbutton)));
    conf.device			= gtk_combo_box_get_active_text (GTK_COMBO_BOX(combo));
    conf.speed 			= gtk_adjustment_get_value(GTK_ADJUSTMENT(speed));
    conf.lijnkleur		= lijnkleur.pixel;
    conf.rasterkleur	= rasterkleur.pixel;
    conf.raster 		= GTK_TOGGLE_BUTTON(checkbutton1)->active;
    conf.tips			= GTK_TOGGLE_BUTTON(checkbutton8)->active;
    conf.speech			= GTK_TOGGLE_BUTTON(checkbutton9)->active;
    conf.voice			= gtk_combo_box_get_active(GTK_COMBO_BOX(combobox1));

    // get conf.keymode[6].mode (the default mode)
	mode = 0;
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(modebits[0].check)))
		mode |= (GB1|GB2);
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(modebits[1].check)))
		mode |= BIG5;
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(modebits[2].check)))
		mode |= DIGITS;
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(modebits[3].check)))
		mode |= LOWERCASE;
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(modebits[4].check)))
		mode |= UPPERCASE;
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(modebits[5].check)))
		mode |= PUNC;
	conf.keymode[6].mode = mode;

	// switch back to default recognition mode
    keus = (mode & BIG5) ? Traditional : Simplified;
	selecteer(keus);

    WTSetSpeed(conf.speed);

    gtk_widget_hide(GTK_WIDGET(preferences));	// close preferences window
    clear_draw_area(GTK_WIDGET(papier));		// redraw paper and raster
    mode = 0;
}

// save all stored preferences to file
void save_preferences (char *filepad) {
    gchar *data;
    gsize len;
	GString *label = g_string_new_len("", 20);
	int i;

    g_key_file_set_double (settings, "display", "opacity", 		conf.opacity);
    g_key_file_set_string (settings, "display", "font", 		conf.font);
    g_key_file_set_integer (settings, "display", "x", 			conf.x);
    g_key_file_set_integer (settings, "display", "y", 			conf.y);
    g_key_file_set_integer (settings, "display", "lx", 			conf.lx);
    g_key_file_set_integer (settings, "display", "ly", 			conf.ly);
    g_key_file_set_integer (settings, "display", "dx", 			conf.dx);
    g_key_file_set_integer (settings, "display", "dy", 			conf.dy);
	g_key_file_set_boolean (settings, "display", "tooltips", 	conf.tips);
    g_key_file_set_string (settings, "reader", "device", 		conf.device);
    g_key_file_set_integer (settings, "reader", "speed", 		conf.speed);
    g_key_file_set_uint64 (settings, "reader", "lijnkleur", 	conf.lijnkleur);
    g_key_file_set_uint64 (settings, "reader", "rasterkleur", 	conf.rasterkleur);
    g_key_file_set_boolean (settings, "reader", "raster", 		conf.raster);
	g_key_file_set_boolean (settings, "reader", "speech", 		conf.speech);
	g_key_file_set_integer (settings, "reader", "voice",		conf.voice);
    g_key_file_set_integer (settings, "keys", "paste0",			conf.paste[0]);
    g_key_file_set_integer (settings, "keys", "paste1",			conf.paste[1]);
    g_key_file_set_integer (settings, "keys", "backspace0",		conf.backspace[0]);
    g_key_file_set_integer (settings, "keys", "backspace1",		conf.backspace[1]);

	g_key_file_set_integer (settings, "GB1", "key",			conf.keymode[0].key);
    g_key_file_set_integer (settings, "GB1", "mode",		conf.keymode[0].mode);
    g_key_file_set_integer (settings, "BIG5", "key",		conf.keymode[1].key);
    g_key_file_set_integer (settings, "BIG5", "mode",		conf.keymode[1].mode);
    g_key_file_set_integer (settings, "DIGITS", "key", 		conf.keymode[2].key);
    g_key_file_set_integer (settings, "DIGITS", "mode", 	conf.keymode[2].mode);
    g_key_file_set_integer (settings, "LOWERCASE", "key", 	conf.keymode[3].key);
    g_key_file_set_integer (settings, "LOWERCASE", "mode", 	conf.keymode[3].mode);
    g_key_file_set_integer (settings, "UPPERCASE", "key", 	conf.keymode[4].key);
    g_key_file_set_integer (settings, "UPPERCASE", "mode",	conf.keymode[4].mode);
    g_key_file_set_integer (settings, "PUNC", "key",		conf.keymode[5].key);
    g_key_file_set_integer (settings, "PUNC", "mode",		conf.keymode[5].mode);
    g_key_file_set_integer (settings, "DEFAULT", "key",		conf.keymode[6].key);
    g_key_file_set_integer (settings, "DEFAULT", "mode", 	conf.keymode[6].mode);

	for (i=0; i<13; i++) {
		g_string_sprintf(label, "key%d", i+1);
		g_key_file_set_integer (settings, "keydefs", label->str, conf.defkey[i].key);
		g_string_sprintf(label, "label%d", i+1);
		g_key_file_set_string (settings, "keydefs", label->str, conf.defkey[i].label);
	}

    data = g_key_file_to_data (settings, &len, NULL);
    g_file_set_contents (filepad, data, len, NULL);

    g_string_free(label, TRUE);
    g_free(data);
}

// monitor system wide keypresses
// 	if mode is set: set the mode in preferences
// 	else switch between predefined recognition modes
void key_pressed_cb(XPointer arg, XRecordInterceptData *d) {
    char txt[20];
    short range, p;
    static short oldrange = 0;
	TS keus;
	bool found;

	if (d->category != XRecordFromServer)
        return;

	keystate.state= (int)d->data[0];
	keystate.key = (int)d->data[1];

	if (skipkeys > 0) {	// skip the keystrokes that are last sent to the receiving window
		skipkeys--;
		return;
	}

	if (action == GET_DEF_KEY) {
		conf.defkey[button_nr].key = keystate.key;
		conf.defkey[button_nr].label = (gchar*) gtk_entry_get_text(GTK_ENTRY(conf.defkey[button_nr].entry));
		sprintf(txt, "key = %d", keystate.key);
		gtk_button_set_label(GTK_BUTTON(conf.defkey[button_nr].button), txt);
		action = 0;
	}
	if (mode) {	// set the hotkeys in preferences
		found = FALSE;
		for (p=0; p<13; p++) {
			if (keystate.key == conf.defkey[p].key) {
				strcpy(txt, conf.defkey[p].label);
				found = TRUE;
				break;
			}
		}
		if (!found) {
			if (keystate.key > 0)
				sprintf(txt, "key = %d", keystate.key);
			else
				strcpy(txt, "not set");
		}

		hotkey[keystate.key] |= mode;

		if (mode & GB1) {
			gtk_button_set_label(GTK_BUTTON(modebits[0].button), txt);
			conf.keymode[0].key = keystate.key;
			conf.keymode[0].mode = hotkey[keystate.key];
		}
		if (mode & BIG5) {
			gtk_button_set_label(GTK_BUTTON(modebits[1].button), txt);
			conf.keymode[1].key = keystate.key;
			conf.keymode[1].mode = hotkey[keystate.key];
		}
		if (mode & DIGITS) {
			gtk_button_set_label(GTK_BUTTON(modebits[2].button), txt);
			conf.keymode[2].key = keystate.key;
			conf.keymode[2].mode = hotkey[keystate.key];
		}
		if (mode & LOWERCASE) {
			gtk_button_set_label(GTK_BUTTON(modebits[3].button), txt);
			conf.keymode[3].key = keystate.key;
			conf.keymode[3].mode = hotkey[keystate.key];
		}
		if (mode & UPPERCASE) {
			gtk_button_set_label(GTK_BUTTON(modebits[4].button), txt);
			conf.keymode[4].key = keystate.key;
			conf.keymode[4].mode = hotkey[keystate.key];
		}
		if (mode & PUNC) {
			gtk_button_set_label(GTK_BUTTON(modebits[5].button), txt);
			conf.keymode[5].key = keystate.key;
			conf.keymode[5].mode = hotkey[keystate.key];
		}
		if (mode & DEFAULT) {
			gtk_button_set_label(GTK_BUTTON(modebits[6].button), txt);
			conf.keymode[6].key = keystate.key;
			mode = 0;
			if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(modebits[0].check)))
				mode |= (GB1|GB2);
			if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(modebits[1].check)))
				mode |= BIG5;
			if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(modebits[2].check)))
				mode |= DIGITS;
			if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(modebits[3].check)))
				mode |= LOWERCASE;
			if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(modebits[4].check)))
				mode |= UPPERCASE;
			if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(modebits[5].check)))
				mode |= PUNC;
			conf.keymode[6].mode = mode;
		}
		mode = 0;
	} else {	// select a recognition mode
		if (keystate.state == KEY_PRESS) {
			bool choosen;
			range = hotkey[keystate.key];
			choosen = (range > 0);
			if (choosen)
				ascii = (range & (GB1 | GB2 | BIG5)) ? FALSE : TRUE;
			if (choosen && (range != oldrange)) {
				oldrange = range;
				mode = range;
				keus = (mode & BIG5) ? Traditional : Simplified;
				selecteer(keus);
			}
		}
	}
}

void *intercept_key_thread(void *data) {
    XRecordClientSpec rcs;
    XRecordRange* rr;
	Display *dpy;
	static GStaticMutex mutex = G_STATIC_MUTEX_INIT;

	g_static_mutex_lock (&mutex);	// ensure the thread is not called again while it is running

	dpy = XOpenDisplay(0);

    if (!(rr = XRecordAllocRange())) {
        fprintf(stderr, "XRecordAllocRange error\n");
        fprintf(stderr, "Error: %s (%d)\n", __FILE__, __LINE__);
        exit(1);
    }
    rr->device_events.first = KeyPress;
    rr->device_events.last = KeyRelease;
    rcs = XRecordAllClients;

    if (!(rc = XRecordCreateContext(dpy, 0, &rcs, 1, &rr, 1))) {
        fprintf(stderr, "XRecordCreateContext error\n");
        fprintf(stderr, "Error: %s (%d)\n", __FILE__, __LINE__);
        exit(1);
    }
    XFree(rr);

    if (!XRecordEnableContext(dpy, rc, key_pressed_cb, (char*) data)) {
        fprintf(stderr, "XRecordEnableContext error\n");
        fprintf(stderr, "Error: %s (%d)\n", __FILE__, __LINE__);
        exit(1);
    }
    XFree(dpy);	// free?

    g_static_mutex_unlock (&mutex);
    return 0;
}

// set the 7 hotkeys
extern "C" G_MODULE_EXPORT gboolean on_select_key (GtkWidget* widget, GdkEventButton * event, GdkWindowEdge edge) {
	char txt[30];
	unsigned short i;

	strcpy(txt, gtk_button_get_label(GTK_BUTTON(widget)));
	if (strcmp(txt, "not set") == 0) {
		strcpy(txt, "Waiting for key..");
		for (i=0; i<7; i++) {
			if (modebits[i].button == widget) {
				mode = modebits[i].bit;
			}
		}
	} else {
		strcpy(txt, "not set");
		for (i=0; i<7; i++) {
			if (modebits[i].button == widget) {
				hotkey[conf.keymode[i].key] &= modebits[i].mask;
				conf.keymode[i].key = 0;
				conf.keymode[i].mode &= modebits[i].mask;
			}
		}
	}
	gtk_button_set_label(GTK_BUTTON(widget), txt);
	return FALSE;
}

// set a preset key (the 13 keys on the preference tab)
extern "C" G_MODULE_EXPORT gboolean on_def_key (GtkWidget* widget, GdkEventButton * event, GdkWindowEdge edge) {
	char txt[30];
	unsigned short i;

	strcpy(txt, gtk_button_get_label(GTK_BUTTON(widget)));
	if (strcmp(txt, "not set") == 0) {
		strcpy(txt, "Waiting for key..");
		for (i=0; i<13; i++) {
			if (conf.defkey[i].button == widget) {
				action = GET_DEF_KEY;
				button_nr = i;
				skipkeys = 1;
				break;
			}
		}
	} else {
		strcpy(txt, "not set");
		for (i=0; i<13; i++) {
			if (conf.defkey[i].button == widget) {
				conf.defkey[i].key = 0;
				strcpy(conf.defkey[i].label, "");
				gtk_entry_set_text(GTK_ENTRY(conf.defkey[i].entry), "");
				break;
			}
		}
	}
	gtk_button_set_label(GTK_BUTTON(widget), txt);

	return FALSE;
}
