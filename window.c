#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <gtk/gtk.h>
#include <X11/extensions/XTest.h>
#include <X11/Intrinsic.h>
#include <math.h>
#include <glib/gprintf.h>
#include "conv.h"
#include "wtlib.h"
#include "coordinate.h"
#include "wtpen.h"
#include "window.h"
#include "wtpen.h"
#include "audio.h"
#include <stdlib.h>


#define SETPIXEL(c) \
	c.pixel = (c.red/0xff); \
	c.pixel <<= 8; \
	c.pixel |= (c.green/0xff); \
	c.pixel <<= 8; \
	c.pixel |= (c.blue/0xff);

typedef struct _CursorOffset {
    gint x;
    gint y;
} CursorOffset;

typedef struct _WTConfig {
    int fullScreen;
    int timeControl;
} WTConfig;

extern GTree *speech;
extern gchar *pad;

typedef struct {
	gchar *kar;
	gchar *pin;
} FreqSpeech;

//static gint text_view_pos = 0;

extern GtkWidget *lookup;
extern GdkPixbuf *pixelbuffer;
extern GdkBitmap *win_mask;
GtkWidget *window = NULL;
extern GtkWidget *papier;
extern bool ready;
Display *display;
extern GtkWidget *knop[9];
extern GtkWidget *event[9];
extern GdkPixmap *gdk_pixmap;
extern GtkWidget *combo;

extern Window focuswin;
extern int skipkeys;	// the number of keys to skip by the key monitor function (key_pressed_cb)
extern CoordinateList *cl;
extern CoordinateList *cl_end;
extern int num;
extern int width, height;
extern bool thuis;
extern TS ts;
extern KeySettings conf;
extern 	GdkColor lijnkleur, rasterkleur;
extern bool ascii;

GdkPixmap *pixmap = NULL;
GdkBitmap *gdk_mask = NULL;
GdkGC *raster = NULL;

int lijndikte = 20;
static GdkGC *gc = NULL;
static unsigned short CandidateResult[10];
int cand_num;	// het aantal gevonden karakters
int timer;

static void commit_string(const char *string, gboolean herstel);
void commit_draw_result();
void output_result();
void clear_draw_area(GtkWidget *widget);
static gboolean scribble_configure_event (GtkWidget *widget, GdkEventConfigure *event, gpointer data);
static gboolean scribble_expose_event (GtkWidget *widget, GdkEventExpose *event, gpointer data);
static void draw_brush (GtkWidget *widget, GdkInputSource source, gdouble x, gdouble y, gdouble x2, gdouble y2, gdouble pressure);
static gboolean scribble_button_press_event (GtkWidget *widget, GdkEventButton *event, gpointer data);
static gboolean scribble_button_release_event(GtkWidget *widget, GdkEventButton *event, gpointer data);
static gboolean scribble_motion_notify_event(GtkWidget *widget, GdkEventMotion *event, gpointer data);
static gboolean time_out(gpointer data);
gchar *dict_search(const gchar *lookfor);

static void on_candidate_pressed (GtkWidget *widget, GdkEventFocus *event, GtkLabel *label);
void selecteer(TS keus);
static void on_commit_button_pressed(void);

void commit_draw_result() {
    unsigned char *lines;
    int i = 0;

    lines = (unsigned char *) g_malloc0(sizeof(unsigned char) * num);
    while (cl) {
        if (cl->x >= 0 && cl->y >= 0) {
            lines[i++] = cl->x;
            lines[i++] = cl->y;
        }
        if (i >= 1000)
            break;
        cl = cl->next;
    }

    if (i >= 1000) {
        if(lines[i-2] == 0xff && lines[i-1] == 0x00) {
            lines[i++] = 0xff;
            lines[i++] = 0xff;
        } else if(lines[i-2] == 0xff && lines[i-1] == 0xff) {
            ;
        } else {
            lines[i++] = 0xff;
            lines[i++] = 0x00;
            lines[i++] = 0xff;
            lines[i++] = 0xff;
        }
        num = i;
    }
    memset(CandidateResult, 0, sizeof(short) * 10);
    if (WTRecognize(lines, num, CandidateResult) != 0)
		fprintf(stderr, "Error: %s (%d)\n", __FILE__, __LINE__);
    CandidateResult[9] = 0;

    g_free(lines);
}

// sets the 9 candidate buttons and outputs the best candidate
void output_result() {
    char convd_string[9][10];
    int i, j;
    char *p = (char *) CandidateResult;

    if (!num)	return;

    cand_num = strlen((char *) CandidateResult) / 2;
    if (!cand_num)	return;

    for (i = 0; i < cand_num; i++) {
        memset(convd_string[i], 0, 10);
		if (!conv_string_utf8(convd_string[i], 10, p + i*2, 2)) {
		//if (!conv_string(convd_string[i], 10, p + i*2, 2)) {
            fprintf(stderr, "Error: %s (%d)\n", __FILE__, __LINE__);
            for(j = i; j < 9; j++)
				gtk_label_set_text(GTK_LABEL(knop[j]), " ");
            if (i == 0)	return;
            else
                break;
        }
        convd_string[i][9] = '\0';		// make sure the string is terminated
        gtk_label_set_text(GTK_LABEL(knop[i]), (gchar *) convd_string[i]);
    }
    on_commit_button_pressed();
}

	//GdkGC *raster = NULL;
// teken een raster op de achtergrond
void draw_raster(GtkAllocation plaats) {
	GdkSegment punt[4];

	if (conf.raster) {
		if (raster == NULL)
			raster = gdk_gc_new(pixmap);
		punt[0].x1 = 1; 					punt[0].y1 = 1;
		punt[0].x2 = plaats.width-1;		punt[0].y2 = plaats.height-1;
		punt[1].x1 = 1; 					punt[1].y1 = plaats.height-1;
		punt[1].x2 = plaats.width-1;		punt[1].y2 = 1;
		punt[2].x1 = (plaats.width-1)/2; 	punt[2].y1 = 1;
		punt[2].x2 = (plaats.width-1)/2;	punt[2].y2 = plaats.height-1;
		punt[3].x1 = 1; 					punt[3].y1 = (plaats.height-1)/2;
		punt[3].x2 = plaats.width-1;		punt[3].y2 = (plaats.height-1)/2;
		gdk_gc_set_foreground(raster, &rasterkleur);
		gdk_gc_set_line_attributes(raster, 1, GDK_LINE_ON_OFF_DASH, GDK_CAP_ROUND, GDK_JOIN_ROUND);
		gdk_draw_rectangle(pixmap, raster, FALSE, 0, 0, plaats.width-1, plaats.height-1);
		gdk_draw_segments(pixmap, raster, punt, 4);
	}
}

void clear_draw_area(GtkWidget *widget) {
    GdkRectangle update_rect;
    GtkAllocation plaats;

    update_rect.x = 0x00;
    update_rect.y = 0x00;
    update_rect.width = 0xffff;
    update_rect.height = 0xffff;

	gtk_widget_get_allocation(GTK_WIDGET(papier), &plaats);
	gdk_window_copy_area(pixmap, gc, 0, 0, gdk_pixmap, plaats.x, plaats.y, plaats.width, plaats.height);	// copy background
    gdk_draw_rectangle(pixmap, widget->style->white_gc, FALSE,
                       update_rect.x-1, update_rect.y-1, update_rect.width, update_rect.height);
	draw_raster(plaats);
    gdk_window_invalidate_rect(widget->window, &update_rect, FALSE);
}

static gboolean scribble_configure_event (GtkWidget *widget, GdkEventConfigure *event, gpointer data) {
	GtkAllocation plaats;

	if (ready) {
		if (pixmap != NULL)
			g_object_unref(G_OBJECT(pixmap));
		pixmap = gdk_pixmap_new(widget->window, widget->allocation.width+2, widget->allocation.height+2, -1);

		if (gc == NULL)
			gc = gdk_gc_new(pixmap);

		gtk_widget_get_allocation(GTK_WIDGET(papier), &plaats);

		gdk_window_copy_area(pixmap, gc, 0, 0, gdk_pixmap, plaats.x, plaats.y, plaats.width, plaats.height);	// copy background

		gdk_draw_rectangle(pixmap, widget->style->white_gc, FALSE,
						-1, -1, widget->allocation.width+1, widget->allocation.height+1);

		draw_raster(plaats);
	}
    return TRUE;
}

static gboolean scribble_expose_event (GtkWidget *widget, GdkEventExpose *event, gpointer data) {
    gdk_draw_drawable(widget->window,
                      widget->style->fg_gc[GTK_WIDGET_STATE(widget)],
                      pixmap,
                      event->area.x, event->area.y,
                      event->area.x, event->area.y,
                      event->area.width, event->area.height);
    return FALSE;
}

// teken op het papier
static void draw_brush (GtkWidget *widget, GdkInputSource source, gdouble x, gdouble y, gdouble x2, gdouble y2, gdouble pressure) {
    GdkRectangle update_rect;
    double dikte;
    update_rect.x = x<x2?x:x2;
    update_rect.x -= 2;
    update_rect.y = y<y2?y:y2;
    update_rect.y -= 2;
    update_rect.width = x>x2?(x-update_rect.x):(x2-update_rect.x);
    update_rect.width += 6;
    update_rect.height = y>y2?(y-update_rect.y):(y2-update_rect.y);
    update_rect.height += 6;
	int lijn, delta, speed;
	GdkColor kleur;

	delta = abs(x-x2)+abs(y-y2);
	speed = 500 * delta;					// change linecolor based on the speed
	kleur.blue = lijnkleur.blue + speed;
	kleur.green = lijnkleur.green + speed;
	kleur.red = lijnkleur.red + speed;
	SETPIXEL(kleur);

	if (raster == NULL)
		raster = gdk_gc_new(pixmap);
    gdk_gc_set_foreground(raster, &kleur);

	lijn = lijndikte - (MIN(lijndikte, delta/3));	// change line width based on the speed of drawing
	dikte = (pressure > 0.0001) ? (lijn * pow(pressure, 1.2)) : lijn/2;

	gdk_gc_set_line_attributes(raster, (int)dikte, GDK_LINE_SOLID, GDK_CAP_ROUND, GDK_JOIN_ROUND);	// 2e argument is lijnbreedte (10)
    gdk_draw_line(pixmap, raster, x, y, x2, y2);

    gdk_window_invalidate_rect(widget->window, &update_rect, FALSE);
}

// start of stroke
static gboolean scribble_button_press_event (GtkWidget *widget, GdkEventButton *event, gpointer data) {
    if (timer)
        gtk_timeout_remove(timer);
    if (event->button == 1) {
		gdouble pressure;
		gdk_event_get_axis ((GdkEvent *)event, GDK_AXIS_PRESSURE, &pressure);		// get pressure
        draw_brush (widget, event->device->source, event->x, event->y, event->x, event->y, pressure);
        record_coordinate(event->x, event->y);
    } else if (event->button == 3) {
        if (num == 0)	return TRUE;
        record_coordinate(0xff, 0xff);
        commit_draw_result();
        output_result();
        clear_draw_area(GTK_WIDGET(papier));
        free_coordinates();
    }
    return TRUE;
}

// end of stroke
static gboolean scribble_button_release_event(GtkWidget *widget, GdkEventButton *event, gpointer data) {
    if (event->button == 1) {
        timer = gtk_timeout_add((100*conf.speed), time_out, NULL);
        record_coordinate(0xff, 0x00);
    }
    return TRUE;
}

// during a stroke
static gboolean scribble_motion_notify_event(GtkWidget *widget, GdkEventMotion *event, gpointer data) {
    int x, y;
    GdkModifierType state;
    gdouble pressure;

    if (pixmap == NULL)
        return FALSE;
    gdk_window_get_pointer(event->window, &x, &y, &state);

    gdk_event_get_axis ((GdkEvent *)event, GDK_AXIS_PRESSURE, &pressure);

    if (state & GDK_BUTTON1_MASK) {
        if (cl_end) {
            if (!(cl_end->x == 0xff && cl_end->y == 0x00)) {
                draw_brush(widget, event->device->source, x, y, cl_end->x, cl_end->y, pressure);
            }
        } else {
            draw_brush(widget, event->device->source, x, y, x, y, pressure);
        }
        record_coordinate(x, y);
    }
    return TRUE;
}

static void on_candidate_pressed (GtkWidget *widget, GdkEventFocus *event, GtkLabel *label) {
    commit_string("---", TRUE); 	// send backspace
    commit_string(gtk_label_get_text(label), FALSE);	// send other candidate				//////////////////////////////////
    if (gtk_widget_get_visible(lookup))
		dict_search(gtk_label_get_text(label));
}

static gboolean time_out(gpointer data) {
    if (num == 0)	return FALSE;
    record_coordinate(0xff, 0xff);
    commit_draw_result();
    output_result();
    clear_draw_area(GTK_WIDGET(papier));
    free_coordinates();
    return FALSE;
}

// switch to another recognition mode
void selecteer(TS keus) {
    ts = keus;
    wtlib_done();
    set_wtlib();
    conv_close();
    conv_open();
    conv_open_utf8();
}

int create_wtpen_window(void) {
    get_locale();

    g_signal_connect(GTK_WIDGET(papier), "expose_event",
                     G_CALLBACK(scribble_expose_event), NULL);
    g_signal_connect(GTK_WIDGET(papier), "configure_event",
                     G_CALLBACK(scribble_configure_event), NULL);
    g_signal_connect(GTK_WIDGET(papier), "motion_notify_event",
                     G_CALLBACK(scribble_motion_notify_event), NULL);
    g_signal_connect(GTK_WIDGET(papier), "button_press_event",
                     G_CALLBACK(scribble_button_press_event), NULL);
    g_signal_connect(GTK_WIDGET(papier), "button_release_event",
                     G_CALLBACK(scribble_button_release_event), NULL);

    gtk_widget_set_events(GTK_WIDGET(papier), gtk_widget_get_events(GTK_WIDGET(papier))
                          | GDK_LEAVE_NOTIFY_MASK
                          | GDK_BUTTON_PRESS_MASK
                          | GDK_BUTTON_RELEASE_MASK
                          | GDK_POINTER_MOTION_MASK
                          | GDK_POINTER_MOTION_HINT_MASK);

	gtk_widget_set_extension_events (GTK_WIDGET(papier), GDK_EXTENSION_EVENTS_CURSOR);

    for (int i=0; i<9; i++)
        g_signal_connect(G_OBJECT(event[i]), "button-press-event", G_CALLBACK(on_candidate_pressed), (gpointer) knop[i]);

	//tijd = g_timer_new();
    return TRUE;
}

// send the best candidate
static void on_commit_button_pressed() {
    if (cand_num > 0) {
		commit_string (gtk_label_get_text(GTK_LABEL(knop[0])), FALSE);
		if (gtk_widget_get_visible(lookup))
			dict_search(gtk_label_get_text(GTK_LABEL(knop[0])));
    }
}

// send a single key (press or release)
void send_key(int key, bool press) {
	if (key) {
		XTestFakeKeyEvent(display, key, press, CurrentTime);
		//XFlush(display);
		skipkeys++;
	}
}

// play the sound of the character in the voice id
void play_voice(const gchar *str, int id) {
	FreqSpeech *freq;
	gchar speak[300], voice[12], *pinyin;

	freq = (FreqSpeech *) g_tree_lookup(speech, str);			// lookup the common pronunciation of the character
	pinyin = strdup((freq == NULL) ? dict_search(str) : freq->pin);		// when not found use the best from the dictionary
	switch (id) {
		case 0:	strcpy(voice, "male");		break;
		case 1: strcpy(voice, "female");	break;
		case 2: strcpy(voice, "teacher");
				pinyin[strlen(pinyin)-1] = '\0';	// strip the tone (all tones will be played)
				break;
	}
	sprintf(speak, "%s/voice/%s/%s.wav", pad, voice, pinyin);
	g_free(pinyin);

    PlaySound(speak);
    SDL_PauseAudio(0);
}

// play the sound of the character in the prefered voice
void play(const gchar *str) {
	play_voice(str, conf.voice);
}

/* 	stuur een keystroke naar het andere window
	herstel=false : stuur een karakter
	herstel=true  : stuur een backspace */
static void commit_string(const gchar *str, gboolean herstel) {
	unsigned int kar, key;
    // focus target window
    //XSetInputFocus(display, focuswin, RevertToNone, CurrentTime);
    //fprintf(stdout, "%s\n", str);
	if (strnlen(str, 6) > 5)		// just to make sure the string is terminated
		return;

    if (herstel) {
        send_key(conf.backspace[0], PRESS);
        send_key(conf.backspace[1], PRESS);
        send_key(conf.backspace[1], RELEASE);
        send_key(conf.backspace[0], RELEASE);
    } else {
		// don't use the high codepoints for alphanumerical characters from the GB table
		// instead use the low codepoints which are the same for ASCII (and look better)
		if (ascii) {
			// send the key directly (not using the clipboard)
			kar = g_utf8_get_char(str) - 0xfee0;
			key = XKeysymToKeycode (display, kar);
			send_key(key, PRESS);
			send_key(key, RELEASE);
		} else {
			// send str to clipboard
			gtk_clipboard_set_text(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD), str, -1);
			// send a paste command to the target window
			send_key(conf.paste[0], PRESS);
			send_key(conf.paste[1], PRESS);
			send_key(conf.paste[1], RELEASE);
			send_key(conf.paste[0], RELEASE);

			if (conf.speech)
				play(str);
		}
    }
}

void wtpen_window_done(void) {
    free_coordinates();
    if(gdk_pixmap)
        gdk_pixmap_unref(gdk_pixmap);
    if(gdk_mask)
        gdk_bitmap_unref(gdk_mask);
	if (pixmap)
		gdk_pixmap_unref(pixmap);
	if (pixelbuffer)
		gdk_pixbuf_unref(pixelbuffer);
	if (win_mask)
		gdk_bitmap_unref(win_mask);
}
