#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/extensions/XTest.h>
#include <X11/extensions/record.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <X11/Intrinsic.h>
#include <iostream>
#include <fstream>

#include <time.h>
#include <locale.h>
#include <langinfo.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "window.h"
#include "wtlib.h"
#include "wtpen.h"
#include "audio.h"
#include <gdk/gdkkeysyms.h>


// the skin needs 2 files: skin.glade (interface) and skin.png (mask and background)
// SKIN = basename of those file
#define SKIN "scroll"

#define GW(name) 	(GTK_WIDGET (gtk_builder_get_object (builder, name)))

//#define DEBUG


typedef struct {
    gchar *Big5;
    gchar *GB;
    gchar *pinyin;
    gchar *translation;
} Cedict;

typedef struct {
	gchar *number;
	gchar *tonemark;
} Pytone;

GdkPixbuf *pixelbuffer = NULL;
GdkBitmap *win_mask = NULL;
GdkPixmap *gdk_pixmap = NULL;
extern GdkGC *raster;
GtkWidget *preferences, *lookup;
GtkWidget *textview1;
GtkAdjustment *opacity, *speed;
GtkWidget *colorbutton1, *colorbutton2;
GtkWidget *scrolled;
GtkWidget *entry16;
GtkWidget *knop[9], *debug;
GtkWidget *event[9], *drag;
GtkWidget *checkbutton1, *checkbutton9;
GtkWidget *gtktable1;
GtkWidget *combo;
GtkWidget *combobox1;
GtkWidget *fontbutton;
GtkWidget *papier;
GtkWidget *checkbutton8;
GtkTextBuffer *textbuffer1;
GdkColor lijnkleur, rasterkleur;
GKeyFile *settings = NULL;
KeySettings conf;
KeyState keystate;
int width, height;
Window focuswin = None;
bool ready = FALSE;	// configure scribble after everything is set up
GtkWidget *entry1, *entry2;
static int add=0, key=0;
char toetsen[30];
extern Display *display;
extern GtkWidget *window;
bool ascii=FALSE;		// if switched to alphanumerical characters use the lower codepoints of GB
ModeBits modebits[7];
gchar *pad;
extern Cedict *first_found;		// the first item in the list of results

void selecteer(TS keus);
void clear_draw_area(GtkWidget *widget);
void load_preferences (char *filepad);
void *intercept_key_thread(void *data);
void save_preferences (char *filepad);
void import_cedict (char *filepad);
bool dict_search(const gchar *lookfor);
void play_voice(const gchar *str, int id);

int skipkeys = 0;
int mode = 0;			// which recognize modes are enabled
int hotkey[NUMKEYS];	// stores the recognition mode that is used when pressing that key

extern GSequence *dictionary;
extern GTree *pintable;
extern SOUNDS sounds[1];

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// START MODIFICATION
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "coordinate.h"
#include <X11/cursorfont.h>
#include <sys/wait.h>

void commit_draw_result();
void output_result();

extern CoordinateList *cl;
extern CoordinateList *cl_end;
extern int num;
static pid_t pid = -1;
static int pipes[2];
struct timeval tv;
extern int timer;
static int full_screen = 0;
Display *fulldisplay;
GC drawgc;

void adjust_coordinate()
{
	CoordinateList *pcl = cl;
	int min_x = 1000, min_y = 1000;
	int max_x = 0, max_y = 0;
	float x_rate = 1.0;
	float y_rate = 1.0;

	while (pcl){
		if (pcl->x == 255 && (pcl->y == 0 || pcl->y == 255)) {
			pcl = pcl->next;
			continue;
		}

		if (pcl->x<min_x)
			min_x = pcl->x;

		if (pcl->x>max_x)
			max_x = pcl->x;

		if (pcl->y<min_y)
			min_y = pcl->y;

		if (pcl->y>max_y)
			max_y = pcl->y;

		 pcl = pcl->next;
	}

	while ((max_x - min_x)*x_rate >255)
		x_rate = x_rate - 0.1;

	while ((max_y - min_y)*y_rate >255)
		y_rate = y_rate - 0.1;

	pcl = cl;

	while (pcl) {
		if (pcl->x == 255 && (pcl->y == 0 || pcl->y == 255))	{
			pcl = pcl->next;
			continue;
		}
		pcl->x = (int)((pcl->x - min_x)*x_rate);
		pcl->y = (int)((pcl->y - min_y)*y_rate);
		pcl = pcl->next;
	}
}

CoordinateList *coordinate_list_head() {
	return cl;
}

CoordinateList *coordinate_list_end() {
	return cl_end;
}

int get_coordinates_num() {
	return num;
}

void send_coordinates(int pipe) {
    int buf[2];
    int i;
    int num = get_coordinates_num();
    CoordinateList *cl = coordinate_list_head();

    if (write(pipe, &num, sizeof(int)) == -1)
        fprintf(stderr, "WTPEN : pipe write num : %d failed\n", num);

    for(i=0; i<num/2; i++) {
        if (cl) {
            buf[0] = cl->x;
            buf[1] = cl->y;
        } else {
            buf[0] = 0xff;
            buf[1] = 0xff;
        }
        write(pipe, &buf, sizeof(int)*2);
        cl = cl->next;
    }
}


void send_data_after_fake_right_button(void) {
    adjust_coordinate();
    send_coordinates(pipes[1]);
    free_coordinates();

    tv.tv_sec=0;
    tv.tv_usec=20000;
    select(1,NULL,NULL,NULL,&tv);

    kill(getppid(), SIGUSR1);
    XFlush(fulldisplay);
}

// note: this is a redeclaration
static void clear_draw_area(Display *dpy, Window rootw, GC drawgc) {
    CoordinateList *c1 = coordinate_list_head();
    CoordinateList *c2 = c1->next;

    while (c2) {
        if (c2->x == 0xff) {
            if (c2->y == 0x00) {
                c1 = c1->next->next;
                c2 = c2->next->next;
                continue;
            }

            if (c2->y == 0xff) {
                break;
            }
        }
        XDrawLine(dpy, rootw, drawgc,
                  c1->x, c1->y,
                  c2->x, c2->y);
        c1 = c1->next;
        c2 = c2->next;
    }
}

static void forward_click_event(Display *dpy, XEvent *event) {
#if 1
    Window subwindow, rootw;

    rootw = DefaultRootWindow(dpy);

    subwindow = event->xbutton.subwindow;

    event->type = ButtonPress;
    event->xbutton.send_event = True;
    event->xbutton.time = CurrentTime;
    event->xbutton.send_event = True;
    event->xbutton.window = subwindow;
    //event->xbutton.subwindow = subwindow;
    XSendEvent(dpy, subwindow, False, ButtonPressMask|ButtonReleaseMask|OwnerGrabButtonMask, event);
    XFlush(dpy);
    XSync(dpy, False);


    event->type = ButtonRelease;
    event->xbutton.time = CurrentTime;
    XSendEvent(dpy, subwindow, True, ButtonPressMask|ButtonReleaseMask|OwnerGrabButtonMask, event);
    XFlush(dpy);
    XSync(dpy, False);

#else
    Window owner;
    const char *string = NULL;

    owner = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy), 0, 0, 1, 1, 0, 0, 0);
    XSetSelectionOwner(fulldisplay, XInternAtom(fulldisplay, "_BUTTON_CLICK", False), owner, CurrentTime);

    while (1) {
        XEvent xev;
        XSelectionRequestEvent *ev;
        XSelectionEvent snev;

        XNextEvent(fulldisplay, &xev);
        if (xev.type != SelectionRequest)
            continue;
        ev = &(xev.xselectionrequest);

        if (strcmp("_BUTTON_CLICK", XGetAtomName(fulldisplay, ev->selection)))
            continue;
        if (strcmp("UTF8_STRING", XGetAtomName(fulldisplay, ev->target)))
            continue;

        fprintf(stderr, "commit: %s, req %x own %x prop %s\n", string,
                (int)ev->requestor, (int)ev->owner,
                XGetAtomName(fulldisplay, ev->property));
        XChangeProperty(fulldisplay, ev->requestor, ev->property,
                        XInternAtom(fulldisplay, "UTF8_STRING", False),
                        8, PropModeReplace, (const unsigned char*)string, strlen(string));

        snev.type = SelectionNotify;
        snev.serial = 0;
        snev.send_event = True;
        snev.display = fulldisplay;
        snev.requestor = ev->requestor;
        snev.selection = ev->selection;
        snev.target = ev->target;
        snev.property = ev->property;
        snev.time = CurrentTime;
        XSendEvent(fulldisplay, ev->requestor, False, 0, (XEvent *)&snev);
        XFlush(fulldisplay);
        break;
    }

    XCloseDisplay(fulldisplay);
#endif
}

void fake_right_button(int signo) {
    Window rootw = DefaultRootWindow(fulldisplay);
    record_coordinate(0xff, 0xff);
    clear_draw_area(fulldisplay, rootw, drawgc);
    send_data_after_fake_right_button();
}

int set_full_screen(Display *dpy) {
    signal(SIGUSR1, fake_right_button);
    fulldisplay = dpy;
    Window curwin, rootw;
    Cursor hand_cursor;
    int x1, y1, winx, winy;
    unsigned int mask;
    XEvent ev;
    int screen_num;

    if (!dpy) {
        fprintf(stderr, "WTPEN : cannot get default display\n");
        exit(1);
    }

    /* style for line */
    unsigned int line_width = 8;
    int line_style = LineSolid;
    int cap_style  = CapRound;
    int join_style = JoinRound;

    screen_num = DefaultScreen(dpy);
    rootw = DefaultRootWindow(dpy);

    if (rootw == None) {
        fprintf(stderr, "WTPEN : full screen mode cannot get root window\n");
        exit(1);
    }

    hand_cursor = XCreateFontCursor(dpy, XC_hand2);

    drawgc = XCreateGC(dpy, rootw, 0, NULL);			// hier wordt getekend (met xor)
    XSetSubwindowMode(dpy, drawgc, IncludeInferiors);
    XSetForeground(dpy, drawgc,
                   WhitePixel(dpy, screen_num)
                   ^ BlackPixel(dpy, screen_num));
    XSetLineAttributes(dpy, drawgc, line_width, line_style,
                       cap_style, join_style);
    XSetFunction(dpy, drawgc, GXandInverted);
    //XSetFunction(dpy, drawgc, GXxor);

    fprintf(stderr, "full screen mode grab button\n");
    XGrabButton(dpy, AnyButton, 0, rootw, False,
                ButtonPressMask | ButtonReleaseMask | ButtonMotionMask | OwnerGrabButtonMask,
                GrabModeSync, GrabModeAsync, None, hand_cursor);

	while (1) {
		fprintf (stderr, "fullscreen\n");	// wordt bij tekenen aangeroepen
        XAllowEvents(dpy, SyncPointer, CurrentTime);
        XWindowEvent(dpy, rootw, ButtonPressMask|ButtonReleaseMask|ButtonMotionMask, &ev);

        switch(ev.type) {
        case ButtonPress:
            kill(getppid(), SIGUSR2);
            if(ev.xbutton.button != Button1) {
                int num;
                XUngrabButton(dpy, AnyButton, 0, rootw);
                XFlush(dpy);
                record_coordinate(0xff, 0xff);
                clear_draw_area(dpy, rootw, drawgc);
                num = get_coordinates_num();
                return num;
            }
            XQueryPointer(dpy, rootw,
                          &rootw, &curwin,
                          &x1, &y1,	//root x, root y
                          &winx, &winy,
                          &mask);
            record_coordinate(x1, y1);
            break;
        case ButtonRelease:
            if (ev.xbutton.button == Button1) {
                if (get_coordinates_num() == 2) {
                    free_coordinates();
                    XUngrabButton(dpy, AnyButton, 0, rootw);
                    forward_click_event(dpy, &ev);
                    XFlush(dpy);
                    return 0;
                }
                record_coordinate(0xff, 0x00);
                kill(getppid(), SIGALRM);
            }
            break;
        case MotionNotify:
            if (ev.xmotion.state & Button1MotionMask) {
                CoordinateList *cl_end;
                XQueryPointer(dpy, rootw,
                              &rootw, &curwin,
                              &x1, &y1,	//root x, root y
                              &winx, &winy,
                              &mask);
                cl_end = coordinate_list_end();
                if (cl_end) {
                    if (!(cl_end->x == 0xff && cl_end->y == 0x00)) {
                        XDrawLine(dpy, rootw,
                                  drawgc,
                                  x1,
                                  y1,
                                  cl_end->x,
                                  cl_end->y);
                        record_coordinate(x1, y1);
                    }
                } else {
                    record_coordinate(x1, y1);
                }
            }
            break;
        default:
            break;
        }
    }
    fprintf(stderr, "exit fullscreen\n");		// wordt nooit bereikt
    return 1;
}

static gboolean time_out(gpointer data) {
    if (full_screen)
        kill(pid, SIGUSR1);
    else {
        if (get_coordinates_num() == 0)
            return FALSE;
        record_coordinate(0xff, 0xff);
        commit_draw_result();
        output_result();
        clear_draw_area(GTK_WIDGET(papier));
        free_coordinates();
    }
    return FALSE;
}

void timer_remove(int signo) {
    if (timer)
        gtk_timeout_remove(timer);
}

void timer_start(int signo) {
    timer = gtk_timeout_add(1000, time_out, NULL);
}

void pipe_read(int signo) {
    int i,num;
    int buf[2];

    read(pipes[0], &num, sizeof(int));
    for (i=0; i<(num/2); i++) {
        read(pipes[0], &buf, sizeof(int)*2);
        record_coordinate(buf[0], buf[1]);
    }
    commit_draw_result();
    output_result();
    free_coordinates();
}

void exit_full_screen(int signo) {
    /* close read pipe */
    close(pipes[0]);
    waitpid(pid, NULL, 0);
    pid = -1;
}

void *fullscreen_thread(void *data) {
	static GStaticMutex mutex = G_STATIC_MUTEX_INIT;

	g_static_mutex_lock (&mutex);	// ensure the thread is not called again while it is running
	close(pipes[0]);
	do {
		if (set_full_screen(fulldisplay)) {
			gdk_threads_enter();
			adjust_coordinate();
			send_coordinates(pipes[1]);
			free_coordinates();

			tv.tv_sec=0;
			tv.tv_usec=20000;
			select(1,NULL,NULL,NULL,&tv);

		//kill(getppid(), SIGUSR1);
			XFlush(fulldisplay);
			gdk_threads_leave();
		}
	} while (1);
    g_static_mutex_unlock (&mutex);
	return 0;
}

static void on_full_screen_button_pressed() {
    full_screen = full_screen^1;
    if (full_screen) {
        int ret;
        ret = pipe(pipes);
        if (ret == -1) {
            perror("pipe");
            return;
        }

        //fulldisplay = XOpenDisplay(NULL);
		//g_thread_create (fullscreen_thread, NULL, FALSE, NULL);

        pid = fork();
        if (pid < 0) {
            fprintf(stderr, "WTPEN : cannot fork for full screen\n");
        } else if (pid == 0) {		// child process?
        	fprintf (stderr, "child\n");
            fulldisplay = XOpenDisplay(NULL);

            pid = getpid();
            if (!fulldisplay)
                exit(1);

            close(pipes[0]);
            do {
            	fprintf(stderr, "x\n");
                if (set_full_screen(fulldisplay)) {
                    adjust_coordinate();
                    send_coordinates(pipes[1]);
                    free_coordinates();

                    tv.tv_sec=0;
                    tv.tv_usec=20000;
                    select(1,NULL,NULL,NULL,&tv);

                    kill(getppid(), SIGUSR1);
                    XFlush(fulldisplay);
                }
            } while (1);
        } else {			// parent process??
        	fprintf (stderr, "parent\n");
            close(pipes[1]);
            signal(SIGUSR2, timer_remove);
            signal(SIGALRM, timer_start);
            signal(SIGUSR1, pipe_read);
            signal(SIGCHLD, exit_full_screen);
        }
    } else {
    	fprintf (stderr, "exit\n");
        if (pid > 0) {
            kill(pid, SIGKILL);	//SIGTERM);
            fprintf(stderr, "WTPEN : full screen exit pid : %d\n", pid);
        }
        signal(SIGUSR1, SIG_DFL);
        signal(SIGCHLD, SIG_DFL);
        signal(SIGUSR2, SIG_DFL);
        signal(SIGALRM, SIG_DFL);
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//	END
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// returned een string gemaakt van key en add (shift)
char *key2string (int key, int add) {
    char soort[15], ascii[10];

    if (XKeycodeToKeysym(display, key, 0) == XK_BackSpace)
        strcpy(ascii, "Backspace");
    else
        sprintf(ascii, "%c", (int) XKeycodeToKeysym(display, key, 0));

    switch (XKeycodeToKeysym(display, add, 0)) {
		case GDK_Control_L:
		case GDK_Control_R:
			strcpy(soort, "control - ");
			break;
		case GDK_Shift_L:
		case GDK_Shift_R:
			strcpy(soort, "shift - ");
			break;
		case GDK_Alt_L:
			strcpy(soort, "alt - ");
			break;
		default:
			strcpy(soort, "");
    }
    sprintf(toetsen, "%s%s", soort, ascii);
    return toetsen;
}

// get path of the executable (where it resides on the disk)
char *get_real_path() {
    char pBuf[100], tmp[100];

    sprintf(tmp, "/proc/%d/exe", getpid());
    int bytes = MIN(readlink(tmp, pBuf, 100), 100 - 1);
    if (bytes >= 0)
        pBuf[bytes] = '\0';
    return g_path_get_dirname(pBuf);	// free after use
}

// set the shape and background of the main window
void set_window_shape(char *filepad) {
    GtkRcStyle *style;
    GError *error=NULL;

    pixelbuffer = gdk_pixbuf_new_from_file(filepad, &error);

    if (error)
		fprintf(stderr, "set windowshape error: %s\n", error->message);

    gdk_pixbuf_render_pixmap_and_mask(pixelbuffer, &gdk_pixmap, &win_mask, 1);

    // set background image
    style = gtk_widget_get_modifier_style(GTK_WIDGET(window));
    style->bg_pixmap_name[GTK_STATE_NORMAL] = g_strdup(filepad);
    gtk_widget_modify_style(GTK_WIDGET(window), style);

    // set the shape
    gdk_window_shape_combine_mask(window->window, win_mask,0,0);

    // set the size
    width = gdk_pixbuf_get_width(pixelbuffer);
    height = gdk_pixbuf_get_height(pixelbuffer);
    gtk_widget_set_usize(window, width, height);
}
/* all callback functions, used by glade file*/

// save window position on program termination
extern "C" G_MODULE_EXPORT void on_window_destroy (GtkObject *object, gpointer user_data) {
	gtk_label_set_text(GTK_LABEL(debug), "Terminating...");
	gtk_widget_queue_draw (debug);	// force a refresh
	while (gtk_events_pending ())
		gtk_main_iteration ();

 	gtk_window_get_position(GTK_WINDOW(window), &conf.x, &conf.y);
 	gtk_window_get_position(GTK_WINDOW(lookup), &conf.lx, &conf.ly);
 	gtk_window_get_size(GTK_WINDOW(lookup), &conf.dx, &conf.dy);

    gtk_main_quit ();
}

// start searching for character or pinyin
extern "C" G_MODULE_EXPORT void on_search (GtkObject *object, gpointer user_data) {
	const gchar *lookfor;

	lookfor = gtk_entry_get_text(GTK_ENTRY(entry16));
	dict_search(lookfor);
}

// hide window instead of destroying it
extern "C" G_MODULE_EXPORT gboolean on_close_window (GtkWidget *widget, GdkEvent  *event, gpointer user_data)  {
    gtk_widget_hide(preferences);
    gtk_widget_hide(lookup);
    mode = 0;		// recognition mode
    return TRUE;
}

// open the lookup window
extern "C" G_MODULE_EXPORT gboolean on_show_window (GtkWidget *widget, GdkEvent  *event, gpointer user_data)  {
	const gchar *lookfor;

	lookfor = gtk_label_get_text(GTK_LABEL(knop[0]));
	gtk_entry_set_text(GTK_ENTRY(entry16), lookfor);

	dict_search(lookfor);

    gtk_widget_show(lookup);
    return FALSE;
}

// adjust opacity of the main window
extern "C" G_MODULE_EXPORT void on_opacity_change (GtkObject *object, gpointer user_data) {
    gtk_window_set_opacity (GTK_WINDOW(window), gtk_adjustment_get_value(opacity));
}

// use the top bar to drag the window (dragbar)
extern "C" G_MODULE_EXPORT gboolean on_drag (GtkWidget* widget, GdkEventButton * event, GdkWindowEdge edge) {
    if (event->type == GDK_BUTTON_PRESS) {
        if (event->button == 1) {
            gtk_window_begin_move_drag(GTK_WINDOW(gtk_widget_get_toplevel(widget)),
                                       event->button,
                                       event->x_root,
                                       event->y_root,
                                       event->time);
        } else {	// open the preferences window
            gtk_widget_show(preferences);
        }
    }
    return FALSE;
}

// set font for the 9 buttons
extern "C" G_MODULE_EXPORT void apply_font (GtkObject *object, gpointer user_data) {
    gchar *font 				= (gchar*) gtk_font_button_get_font_name(GTK_FONT_BUTTON(fontbutton));
    PangoFontDescription *pfont = pango_font_description_from_string (font);
    for (int i=0; i< 9; i++)
        gtk_widget_modify_font (GTK_WIDGET(knop[i]), pfont);
	//g_free(font);
	pango_font_description_free(pfont);
}

// get the keycodes for paste and backspace
extern "C" G_MODULE_EXPORT gboolean on_key_press (GtkWidget *widget, GdkEventKey *event, gpointer user_data) {
    if (event->state & GDK_SHIFT_MASK) {
        add = key;
        key = XKeysymToKeycode(display, event->keyval);
    } else if (event->state & GDK_CONTROL_MASK) {
        add = key;
        key = XKeysymToKeycode(display, event->keyval);
    } else if (event->state & GDK_MOD1_MASK) {
        add = key;
        key = XKeysymToKeycode(display, event->keyval);
    } else {
        key = XKeysymToKeycode(display, event->keyval);
    }
    //fprintf(stderr, "%d\n", key);
    return TRUE;
}

// 2nd part of get keycodes for paste and backspace
extern "C" G_MODULE_EXPORT gboolean on_key_release (GtkWidget *widget, GdkEventKey *event, gpointer user_data) {
    if (key+add > 0) {
        gtk_entry_set_text(GTK_ENTRY(widget), key2string(key, add));
        if (widget == entry1) {
            conf.paste[0] = add;
            conf.paste[1] = key;
        } else {
            conf.backspace[0] = add;
            conf.backspace[1] = key;
        }
        add = key = 0;
        strcpy(toetsen, "");
    }
    return TRUE;
}

// get tooltip text fromn widget and display if conf.tips is true
extern "C" G_MODULE_EXPORT gboolean on_tooltip (GtkWidget *widget, gint x, gint y, gboolean mode, GtkTooltip *tooltip, gpointer data) {
	gchar *tip;

	tip = gtk_widget_get_tooltip_text(widget);
	gtk_tooltip_set_text(tooltip, tip);
	g_free(tip);

	return conf.tips;
}

#include <SDL/SDL_thread.h>
#include <SDL/SDL_timer.h>
SDL_Thread *thread = NULL;
bool stop;
int PlayThread( void *ptr )
{
	gchar speak[300], **woord;
	gchar *voice = (gchar*) ptr;
	long len;

	if (first_found == NULL)				return 0;
	if (strlen(first_found->pinyin) == 0)	return 0;

	//fprintf(stderr, "%s\n", first_found->pinyin);
	woord = g_strsplit(first_found->pinyin, " ", NULL);
	for (int i=0; woord[i] && !stop; i++) {
		if (strcmp(voice, "teacher") == 0) {
			woord[i][strlen(woord[i])-1] = '\0';	// remove the tone number
			sprintf(speak, "%s/voice/%s/%s.wav", pad, voice, woord[i]);
			PlaySound(speak);
			stop = true;		// only play the first character
		} else {
			sprintf(speak, "%s/voice/%s/%s.wav", pad, voice, woord[i]);
			len = PlaySound(speak);
			SDL_Delay( MAX(len, 500));		// wait until sample is finished
			SDL_PauseAudio(1);
		}
	}
	g_strfreev(woord);
	return 1;		// indicate that play is finished
}

void Play_Voice(const gchar *voice) {
	int threadReturnValue = 1;

	if (first_found == NULL)				return;
	if (strlen(first_found->pinyin) == 0)	return;

	stop = true;
	if (thread)	SDL_WaitThread(thread, &threadReturnValue);
	if (threadReturnValue == 1) {
		stop = false;
		thread = SDL_CreateThread(PlayThread, (void *) voice);
	}
}
// play male voice
extern "C" G_MODULE_EXPORT gboolean on_male_press (GtkWidget *widget, GdkEvent *event, gpointer user_data) {
	Play_Voice("male");
	return TRUE;
}

// play female voice
extern "C" G_MODULE_EXPORT gboolean on_female_press (GtkWidget *widget, GdkEvent *event, gpointer user_data) {
	Play_Voice("female");
	return TRUE;
}

// play teacher voice (only the first character)
extern "C" G_MODULE_EXPORT gboolean on_teacher_press (GtkWidget *widget, GdkEvent *event, gpointer user_data) {
	Play_Voice("teacher");
	return TRUE;
}

/* end callback functions */

void free_all() {
	gtk_widget_destroy(combo);
	g_object_unref(raster);
	g_key_file_free(settings);
	g_tree_destroy(pintable);
	g_sequence_free(dictionary);
}

// set a lock file and an expiration time for the lock
void setlock(char *filepad, int secs, bool lock) {
	struct stat     statbuf;
	time_t	now;
	double dif;

	if (lock) {
		if (stat(filepad, &statbuf) == -1) {
			creat (filepad, O_RDWR);
		} else {
			time(&now);
			dif = difftime (now, statbuf.st_mtime);
			if (dif < secs)
				exit(0);
			else
				unlink(filepad);
		}
	} else {
		unlink(filepad);	// remove the lock
	}
}

// kijk welk window inputfocus heeft
gboolean checkfocus (gpointer user_data)
{
    Window tmp;
    int revert;

    XGetInputFocus(display, &tmp, &revert);
    return TRUE;
}

int main(int argc, char *argv[]) {
    GtkBuilder *builder;
    GString *txt = g_string_new_len("", 20);
    char filepad[100];
    unsigned short i, mask=0;
    GdkScreen* screen = NULL;
	int xx, yy, screen_width, screen_height;

	InitAudio();

	pad = get_real_path();					// get the path of the executable

	sprintf(filepad, "%s/.lock", pad);
	setlock(filepad, 10, TRUE);				// disable multiple instances for at least 10 seconds

	g_thread_init (NULL);

    display = XOpenDisplay(0);
    if (display == NULL)
        exit(1);

    gtk_set_locale();

    gtk_init(&argc, &argv);

    builder = gtk_builder_new ();

    sprintf(filepad, "%s/%s.glade", pad, SKIN);

    gtk_builder_add_from_file(builder, filepad, NULL);

    gtk_builder_connect_signals (builder, NULL);

    // set widgets to be handled by the event handlers
    window 			= GW ("window");
    preferences 	= GW ("preferences");
    lookup		 	= GW ("lookup");
    fontbutton 		= GW ("fontbutton1");
    drag 			= GW ("dragbar");
    debug 			= GW ("debug");
	papier 			= GW ("drawingarea1");
    colorbutton1 	= GW ("colorbutton1");
    colorbutton2 	= GW ("colorbutton2");
    checkbutton1	= GW ("checkbutton1");
    entry1 			= GW ("entry1");
    entry2 			= GW ("entry2");
    gtktable1 		= GW ("table1");
    checkbutton8	= GW ("checkbutton8");
    checkbutton9	= GW ("checkbutton9");
    textview1 		= GW ("textview1");
    entry16 		= GW ("entry16");
    combobox1 		= GW ("combobox1");
    scrolled		= GW ("scrolledwindow2");
    opacity 		= GTK_ADJUSTMENT (gtk_builder_get_object (builder, "adjustment1"));
    speed 			= GTK_ADJUSTMENT (gtk_builder_get_object (builder, "adjustment2"));
    textbuffer1		= GTK_TEXT_BUFFER(gtk_builder_get_object (builder, "textbuffer1"));

	for (i=0; i<sizeof(unsigned short); i++) {	// set each bit of mask
		mask <<= 8;
		mask |= 0xFF;
	}

	// fill the struct that keeps the 7 recognize buttons & checkboxes (in preferences)
    for (i=0; i<7; i++) {
    	g_string_sprintf(txt, "checkbutton%d", i+2);
		modebits[i].check = GTK_WIDGET(gtk_builder_get_object (builder, txt->str));
		g_string_sprintf(txt, "button%d", i+3);
		modebits[i].button = GTK_WIDGET(gtk_builder_get_object (builder, txt->str));
		switch (i) {
			case 0: modebits[i].bit = (GB1|GB2);	modebits[i].mask = ((GB1|GB2) ^ mask);	break;
			case 1: modebits[i].bit = BIG5;			modebits[i].mask = (BIG5 ^ mask);		break;
			case 2: modebits[i].bit = DIGITS;		modebits[i].mask = (DIGITS ^ mask);		break;
			case 3: modebits[i].bit = LOWERCASE;	modebits[i].mask = (LOWERCASE ^ mask);	break;
			case 4: modebits[i].bit = UPPERCASE;	modebits[i].mask = (UPPERCASE ^ mask);	break;
			case 5: modebits[i].bit = PUNC;			modebits[i].mask = (PUNC ^ mask);		break;
			case 6: modebits[i].bit = DEFAULT;		modebits[i].mask = (DEFAULT ^ mask);	break;
		}
	}

	// fill the structure that keeps the 13 labels for the keys (preferences)
    for (i=0; i<13; i++) {
    	g_string_sprintf(txt, "button%d", i+10);
		conf.defkey[i].button = GTK_WIDGET(gtk_builder_get_object (builder, txt->str));
    	g_string_sprintf(txt, "entry%d", i+3);
		conf.defkey[i].entry = GTK_WIDGET(gtk_builder_get_object (builder, txt->str));
		conf.defkey[i].key = 0;
    }

    // place a simple combobox for the input selection (preferences)
    combo = gtk_combo_box_new_text();
    gtk_table_attach_defaults(GTK_TABLE(gtktable1), combo, 1, 2, 2, 3);
    gtk_widget_show (combo);

    // get events and labels for the 9 candidates
    for (int i=0; i< 9; i++) {
        g_string_sprintf(txt, "knop%d", i+1);
        knop[i] = GTK_WIDGET (gtk_builder_get_object (builder, txt->str));
        g_string_sprintf(txt, "event%d", i+1);
        event[i] = GTK_WIDGET (gtk_builder_get_object (builder, txt->str));
    }

    // set events for paste and backspace entries (preferences)
    g_signal_connect(entry1, "key_press_event", G_CALLBACK(on_key_press), NULL);
    g_signal_connect(entry1, "key_release_event", G_CALLBACK(on_key_release), NULL);
    g_signal_connect(entry2, "key_press_event", G_CALLBACK(on_key_press), NULL);
    g_signal_connect(entry2, "key_release_event", G_CALLBACK(on_key_release), NULL);

    if (!create_wtpen_window())
        exit(1);

    wtpen_init();

	g_object_unref (G_OBJECT (builder));

    gtk_widget_show_all(GTK_WIDGET(window));

	for (i=0; i<NUMKEYS; i++)	hotkey[i] = 0;	// reset all hotkeys

    // load cedict from file
    sprintf(filepad, "%s/data", pad);
    import_cedict(filepad);

    // load settings from file
    sprintf(filepad, "%s/xpen.cfg", pad);
    load_preferences(filepad);

    // set background and shape
    sprintf(filepad, "%s/%s.png", pad, SKIN);
    set_window_shape(filepad);

	// some styles to be used in the cedict browser (lookup window)一
	gtk_text_buffer_create_tag(textbuffer1, "mark", "background", "yellow", "foreground", "black", NULL);
	gtk_text_buffer_create_tag(textbuffer1, "bold", "weight", PANGO_WEIGHT_BOLD, NULL);
	gtk_text_buffer_create_tag(textbuffer1, "character", "font", conf.font, "scale", 1.3, NULL);
	gtk_text_buffer_create_tag(textbuffer1, "translation", "scale", 0.9, NULL);
	gtk_text_buffer_create_tag(textbuffer1, "pinyin", "scale", 1.1, NULL);

    ready = TRUE;	// let_configure_event() know that it can start its setup

	// start monitoring system-wide keypresses
	g_thread_create (intercept_key_thread, NULL, FALSE, NULL);

	// make sure the window positions and size are legal
	screen = gtk_window_get_screen(GTK_WINDOW(window));		// get screen size
	screen_width = gdk_screen_get_width(screen);
	screen_height = gdk_screen_get_height(screen);

    xx = MAX(MIN(screen_width-width, conf.x), 0);			// set the position of the main window
    yy = MAX(MIN(screen_height-height, conf.y), 0);
	gtk_widget_set_uposition(window, xx, yy);

    xx = MIN(screen_width, conf.dx);						// set the size of the lookup window
    yy = MIN(screen_height, conf.dy);
	gtk_widget_set_usize(lookup, xx, yy);

	xx = MAX(MIN(screen_width-conf.dx, conf.lx), 0);		// set the position of the lookup window
	yy = MAX(MIN(screen_height-conf.dy, conf.ly), 0);
	gtk_widget_set_uposition(lookup, xx, yy);

	g_timeout_add (100, checkfocus, NULL);	// check the inputfocus (each 1/10s)

//////////////////////////////////////////////////////////////
//on_full_screen_button_pressed();	// start the full screen mode
//////////////////////////////////////////////////////////////

    gtk_main();						// start the main loop

	sprintf(filepad, "%s/.lock", pad);		// remove the lock
	setlock(filepad, 0, FALSE);

	// save all settings to file
    sprintf(filepad, "%s/xpen.cfg", pad);
	save_preferences (filepad);

    g_free(pad);
	free_all();

    wtpen_window_done();
    wtlib_done();

    SDL_CloseAudio();
    SDL_Quit();

//on_full_screen_button_pressed();

    return 0;
}
