#include <glib.h>
#include <glib/gprintf.h>
#include <fstream>
#include <gtk/gtk.h>
#include <stdio.h>
#include <string.h>
#include "wtpen.h"
#include "wtlib.h"
#include <stdlib.h>
#include <iostream>
#include "utf8.h"
#include <gdk/gdkx.h>
#include <gdk/gdkkeysyms.h>

extern KeySettings conf;
extern GtkWidget *entry16;

typedef struct {
    gchar *Big5;
    gchar *GB;
    gchar *pinyin;
    gchar *translation;
    gint rating;
    glong score;
} Cedict;

typedef struct {
    gchar *number;
    gchar *tonemark;
} Pytone;

typedef struct {
	gchar *word;
	gchar rating[10];
} TPS;

typedef struct {
	gchar *kar;
	gchar *pin;
} FreqSpeech;


extern GtkTextBuffer *textbuffer1;
extern GtkWidget *textview1;
extern GtkWidget *scrolled;
extern TS ts;
extern Display *display;
extern GtkWidget *entry16;
int skip = 70; //30;		// skip the first 30 lines of the cedict file
bool found;
Cedict *first_found;		// the first item in the list of results
gchar selected[50];
GSequence *dictionary;
GTree *pintable;
GTree *freq;		// table with words and their frequency
GTree *speech;		// Most frequent pronunciations of the 3799 most common characters
GSList *other = NULL;		// temporary results of a search
GSList *equal = NULL;		// temporary results of a search - words of equal length as the search item

void get_pintones();
void import_cedict(char *filepath);
gchar *dict_search(const gchar *lookfor);
void display_entry(Cedict *word);
gint order_dict (gconstpointer a, gconstpointer b, gpointer user_data);
void search_char(gpointer data, gpointer userdata);
void search_pin(gpointer data, gpointer userdata);
void search_trans(gpointer data, gpointer userdata);

// check to see if a file is a valif UTF-8 file
bool valid_utf8_file(const char* file_name)
{
	using namespace std;
    ifstream ifs(file_name);
    if (!ifs)
        return false; // file error

    istreambuf_iterator<char> it(ifs.rdbuf());
    istreambuf_iterator<char> eos;

    return utf8::is_valid(it, eos);
}

// read a file into the output (string array) and return the number of items read
int readfile(gchar *path, const gchar *name, gchar ***output) {
    std::string line;
    gchar filename[200], **lines=NULL;
    int lengte=0;

    g_sprintf(filename, "%s/%s", path, name);

	if (valid_utf8_file(filename)) {
		std::ifstream fs8(filename);	// read the corpus file (utf8)
		if (!fs8.is_open()) {
			fprintf(stderr, "couldn't open %s\n", filename);
			fprintf(stderr, "Error: %s (%d)\n", __FILE__, __LINE__);
			return 0;
		}
		while (!fs8.eof()) {
			getline(fs8, line);
			lines = (gchar **)g_realloc_n(lines, lengte+1, sizeof(gchar *));
			lines[lengte++] = strdup(line.c_str());
		}
		lines[--lengte] = NULL;
	} else {
		fprintf(stderr, "file/path: %s is invalid\n", filename);
		fprintf(stderr, "Error: %s (%d)\n", __FILE__, __LINE__);
		return 0;
	}
	*output = lines;
	return lengte;
}

// free TPS list
void  free_TPS (gpointer data) {
    TPS *entry = (TPS*) data;
    g_free(entry->word);
    g_free(data);
}

// initialize the frequence table
void get_tps(gchar *filepath) {
    TPS *tps;
    gchar tmp[1000];
    const gchar *start;
    int i, partlen, lengte;
	gchar **lines = NULL;
	std::string line;

    lengte = readfile(filepath, "corpus.dat", &lines);

    freq = g_tree_new_full((GCompareDataFunc) strcmp, NULL, NULL, free_TPS);
    for (i=0; lines[i] != NULL; i++) {
        tps = (TPS*) g_malloc0(sizeof(TPS));
        start = lines[i];
        partlen = strlen(start);
        strncpy(tmp, start, partlen);
        tmp[partlen] = '\0';
        tps->word = g_strdup(tmp);					// pinyin with tonemarks
        sprintf(tps->rating, "%d", lengte-i);		// reverse the rating (subtract the total number of lines)

        g_tree_insert(freq, tps->word, tps);
    }
	g_strfreev(lines);
}

// convert pinyin with tonemark to numbered pinyin
gboolean find_number(gpointer key, gpointer value, gpointer data) {
    gchar *found = (gchar*) data;
    Pytone *pin = (Pytone*) value;

    if (strcmp(pin->tonemark, found) == 0) {
        strcpy(found, pin->number);
        return TRUE;
    }
    return FALSE;
}

// find space
gboolean on_textview_find_space(gunichar ch, gpointer user_data) {
    if (g_unichar_isspace(ch) == TRUE)
        return TRUE;
    if (g_unichar_ispunct(ch) == TRUE)
        return TRUE;
    return FALSE;
}

// search on a new word that is double-clicked in the textview
extern "C" G_MODULE_EXPORT gboolean on_textview_click_event(GtkWidget *widget, GdkEventMotion *event, gpointer user_data) {
    gchar *buf = NULL;

    if (event->type==GDK_2BUTTON_PRESS) {
        gtk_entry_set_text(GTK_ENTRY(entry16), selected);
        dict_search(gtk_entry_get_text(GTK_ENTRY(entry16)));
        if(buf != NULL) g_free(buf);
    }
    return FALSE;
}

// highlight the word under the cursor
extern "C" G_MODULE_EXPORT gboolean on_textview_motion_event(GtkWidget *widget, GdkEventMotion *event, gpointer user_data) {
    gint x = 0, bx = 0;
    gint y = 0, by = 0;
    GdkModifierType state = (GdkModifierType) 0;
    GdkWindow *window = NULL;
    GtkTextIter iter;
    GtkTextIter iter_start, start;
    GtkTextIter iter_end, end;
    gboolean found_start = FALSE;
    gboolean found_end = FALSE;
    gchar *buf = NULL, txt[50];
    gunichar kar;
    int len;

    strcpy(selected, "");
    if(event->is_hint) {
        window = gdk_window_get_pointer(event->window, &x, &y, &state);
    } else {
        x = event->x;
        y = event->y;
        state = (GdkModifierType) event->state;
    }

    gtk_text_view_window_to_buffer_coords(GTK_TEXT_VIEW(textview1), GTK_TEXT_WINDOW_TEXT, x, y, &bx, &by);
    gtk_text_view_get_iter_at_location(GTK_TEXT_VIEW(textview1), &iter, bx, by);

    kar = gtk_text_iter_get_char(&iter);
    len = g_unichar_to_utf8(kar, txt);
    txt[len] = '\0';

    if (g_unichar_isspace(kar))		return TRUE;
    if (g_unichar_ispunct(kar))		return TRUE;

    // find space around kar
    iter_start = iter;
    iter_end = iter;
    found_start = gtk_text_iter_backward_find_char(&iter_start, on_textview_find_space, NULL, NULL);
    found_end = gtk_text_iter_forward_find_char(&iter_end, on_textview_find_space, NULL, NULL);

    if(found_start == FALSE && found_end == FALSE)	return TRUE;

    if(gtk_text_iter_is_start(&iter_start) == FALSE)
        gtk_text_iter_forward_char(&iter_start);

	// remove previous highlight from text
	gtk_text_buffer_get_start_iter(textbuffer1, &start);
	gtk_text_buffer_get_end_iter(textbuffer1, &end);
	gtk_text_buffer_remove_tag_by_name(textbuffer1, "mark", &start, &end);

    if (kar > 0x02d0 && kar < 0xff00) {		// if kar is a character then only select one character
        iter_start = iter_end = iter;
        gtk_text_iter_forward_char(&iter_end);
    }
	gtk_text_buffer_apply_tag_by_name(textbuffer1, "mark", &iter_start, &iter_end);

    buf = gtk_text_buffer_get_slice(textbuffer1, &iter_start, &iter_end, FALSE);
    strcpy(selected, buf);

    if (buf != NULL) g_free(buf);

    return FALSE;
}

// start searching when press enter
extern "C" G_MODULE_EXPORT gboolean on_enter (GtkWidget *widget, GdkEventKey *event, gpointer user_data) {
    const gchar *lookfor = gtk_entry_get_text(GTK_ENTRY(entry16));
    int key = XKeysymToKeycode(display, event->keyval);

    switch (XKeycodeToKeysym(display, key, 0)) {
    case GDK_KEY_Return:
    case GDK_KEY_KP_Enter:
        dict_search(lookfor);
    }
    return FALSE;
}

// also free pinyin when free pin-table
void  free_pin (gpointer data) {
    Pytone *entry = (Pytone*) data;
    g_free(entry->number);
    g_free(entry->tonemark);
    g_free(data);
}

// import pinyin.dat (pinyin notation table)
void get_pintones(char *filepath) {
    Pytone *pytone;
    gchar **lines=NULL, tmp[1000];
    const gchar *start, *end, *keep=NULL;
    int i, partlen, lengte=0;
    std::string line;

	lengte = readfile(filepath, "pinyin.dat", &lines);

    pintable = g_tree_new_full((GCompareDataFunc) strcmp, NULL, NULL, free_pin);

    for (i=0; lines[i] != NULL; i++) {
        pytone = (Pytone*) g_malloc0(sizeof(Pytone));
        lengte = strlen(lines[i]);

        start = lines[i];
        end = g_utf8_strchr(start, -1, ' ');				// find the next part
        if (end) {											// see if the entry consist of both words (pin-number and pin-tonemark)
            partlen = lengte - strlen(end);
            strncpy(tmp, start, partlen);
            tmp[partlen] = '\0';
            pytone->number = g_ascii_strdown(tmp, -1);		// pinyin with numerical tonemark

            start = g_utf8_find_next_char(end, NULL);			// skip delimiter
            partlen = strlen(start);
            strncpy(tmp, start, partlen);
            tmp[partlen] = '\0';
            pytone->tonemark = g_strdup(tmp);					// pinyin with tonemarks
            keep = pytone->tonemark;							// remember the last tonemark
        } else {
            partlen = strlen(start);
            strncpy(tmp, start, partlen);
            tmp[partlen] = '\0';
            pytone->number = g_ascii_strdown(tmp, -1);
            pytone->tonemark = g_strdup(keep);							// use the last tonemark
        }
        // insert the word into the table
        g_tree_insert(pintable, pytone->number, pytone);
    }
	g_strfreev(lines);
}

// also free speech when free speech-table
void  free_speech (gpointer data) {
    FreqSpeech *entry = (FreqSpeech*) data;
    g_free(entry->kar);
    g_free(entry->pin);
    g_free(data);
}

// import speech.dat (pinyin notation table)
void get_speech(char *filepath) {
    FreqSpeech *freq;
    gchar **lines=NULL;
    int i;

	readfile(filepath, "speech.dat", &lines);

    speech = g_tree_new_full((GCompareDataFunc) strcmp, NULL, NULL, free_speech);

    for (i=0; lines[i] != NULL; i+=2) {
        freq = (FreqSpeech*) g_malloc0(sizeof(FreqSpeech));

        freq->kar = strdup(lines[i]);
        freq->pin = strdup(lines[i+1]);

        // insert the word into the table
        g_tree_insert(speech, freq->kar, freq);
    }
	g_strfreev(lines);
}

void iter_result(gpointer item, gpointer prefix) {
	Cedict *dict = (Cedict*) item;
	//printf("%05d: %s\n", dict->rating, dict->GB);
	display_entry(dict);

	if (first_found == NULL)
		first_found = dict;
}

// sort result based on frequence rating
gint sort_found(gconstpointer a, gconstpointer b) {
	Cedict *x = (Cedict*) a;
	Cedict *y = (Cedict*) b;

	return (y->score - x->score);
}

// search in the cedict dictionary and fill the lookup window
gchar *dict_search(const gchar *lookfor) {
    GtkAdjustment *hadj = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(scrolled));
    gchar tmp[50], out[50];
	char **parts;
	int count;
	GSList *result=NULL;
	Cedict *word;

	g_strstrip((gchar*) lookfor);

	first_found = NULL;

    if (strcmp(lookfor, "") == 0)
        return (gchar*)("");

    gtk_adjustment_set_value(hadj, 0);			// scroll to the start of the page

    gtk_text_buffer_set_text (textbuffer1, "", -1);	// empty the last result

	parts = g_strsplit(lookfor, " ", NULL);		// convert each word with tonemark to word with number
	strcpy(out, "");
	for (count=0; parts[count] != NULL; count++) {
		strcpy(tmp, parts[count]);
		g_tree_foreach(pintable, find_number, &tmp);
		sprintf(out, "%s %s", out, tmp);
	}
	g_strfreev(parts);
	strcpy(tmp, &out[1]);
	gtk_entry_set_text(GTK_ENTRY(entry16), tmp);

    if (isalpha(tmp[0])) {														// decide on what to search for
        if (strcspn(lookfor, "012345:") == strlen(tmp) ) {						// pinyin or translation?
            g_sequence_foreach(dictionary, (GFunc)search_trans, (gpointer)tmp);	// search on translation and display the result
            equal = g_slist_sort(equal, (GCompareFunc) sort_found);			// equal length words
            other = g_slist_sort(other, (GCompareFunc) sort_found);			// other found words
            result = g_slist_concat(equal, other);
			g_slist_foreach(result, (GFunc)iter_result, NULL);
        } else {
            g_sequence_foreach(dictionary, (GFunc)search_pin, (gpointer)tmp);		// search on pinyin
            equal = g_slist_sort(equal, (GCompareFunc) sort_found);			// equal length words
            other = g_slist_sort(other, (GCompareFunc) sort_found);			// other found words
            result = g_slist_concat(equal, other);
			g_slist_foreach(result, (GFunc)iter_result, NULL);
        }
    } else {
        g_sequence_foreach(dictionary, (GFunc)search_char, (gpointer)tmp);	// search on characters
        equal = g_slist_sort(equal, (GCompareFunc) sort_found);		// equal length words
        other = g_slist_sort(other, (GCompareFunc) sort_found);		// other found words
        result = g_slist_concat(equal, other);
		g_slist_foreach(result, (GFunc)iter_result, NULL);
    }
	word = (Cedict*) g_slist_nth_data(result, 0);

	g_slist_free(result);
	other = NULL;
	equal = NULL;

    return (word != NULL) ? word->pinyin : (gchar*)("");
}

// insert new data in dictionary ordered by the length of GB一
gint order_dict (gconstpointer a, gconstpointer b, gpointer user_data) {
    Cedict *x = (Cedict*) a;
    Cedict *y = (Cedict*) b;
    return strlen(x->GB) - strlen(y->GB);
}

// also free words when free dictionary
void  free_dict (gpointer data) {
    Cedict *word = (Cedict*) data;

    g_free(word->Big5);
    g_free(word->GB);
    g_free(word->pinyin);
    g_free(word->translation);
    g_free(data);
}

// 	import the original cedict file
void import_cedict(char *filepath) {
    gchar **lines=NULL, *start, *end;
    gchar tmp[1000];
    int i, lengte, partlen;
    Cedict *word;
	TPS *tps;
	std::string line;

	get_tps(filepath);
    get_pintones(filepath);			// get pinyin convert table
    get_speech(filepath);

	lengte = readfile(filepath, "cedict_ts.u8", &lines);

    dictionary = g_sequence_new (free_dict);

    for (i=skip; lines[i]; i++) {
        word = (Cedict*) g_malloc0 (sizeof(Cedict));

        lengte = strlen(lines[i]);

        start = lines[i];
        end = g_utf8_strchr(start, -1, ' ');				// find the next part
        partlen = lengte - strlen(end);
        lengte -= partlen;
        strncpy(tmp, start, partlen);
        tmp[partlen] = '\0';
        word->Big5 = g_strdup(tmp);							// traditional

        start = g_utf8_find_next_char(end, NULL);			// skip delimiter
        end = g_utf8_strchr(start, -1, '[');				// find the next part
        partlen = lengte - strlen(end);
        lengte -= partlen;
        strncpy(tmp, start, partlen-2);
        tmp[partlen-2] = '\0';
        word->GB = g_strdup(tmp);							// simplified

        start = g_utf8_find_next_char(end, NULL);			// skip delimiter
        end = g_utf8_strchr(start, -1, '/');				// find the next part
        partlen = lengte - strlen(end);
        lengte -= partlen;
        strncpy(tmp, start, partlen-3);
        tmp[partlen-3] = '\0';
        word->pinyin = g_utf8_strdown(tmp, -1);				// pinyin (force lowercase)

        start = g_utf8_find_next_char(end, NULL);			// skip delimiter '/'
        partlen = strlen(start);
        strncpy(tmp, start, partlen-2);
        tmp[partlen-2] = '\0';
        word->translation = g_strdup(tmp);					// translation

        tps = (TPS *) g_tree_lookup(freq, word->GB);		// find the TPSuence
		word->rating = (tps != NULL) ? atoi(tps->rating) : 1;

        // insert the word into the dictionary in sorted order
        g_sequence_insert_sorted(dictionary, word, (GCompareDataFunc)order_dict, NULL);
    }
    g_strfreev(lines);
}

// print a single entry
void display_entry(Cedict *word) {
    gchar tmp[1000], **parts;
    Pytone *gevonden;
    int i, ll;
    GtkTextIter iterator;

    gtk_text_buffer_get_end_iter(textbuffer1, &iterator);

    found = TRUE;
    ll = 380;		// line length - todo: calculate dynamically

    g_sprintf(tmp, "%s\n", (ts == Simplified) ? word->GB : word->Big5);	// show character(s)
    gtk_text_buffer_insert_with_tags_by_name (textbuffer1, &iterator, tmp, -1, "character", NULL);

    strcpy(tmp, "");
    parts = g_strsplit(word->pinyin, " ", NULL);				// convert pinyin with numbers to pinyin with tonemarks
    for (i=0; parts[i] != NULL; i++) {
        gevonden = (Pytone *) g_tree_lookup(pintable, parts[i]);
        if (gevonden)
            g_sprintf(tmp, "%s %s", tmp, gevonden->tonemark);
		else
			g_sprintf(tmp, "%s %s", tmp, parts[i]);	// use original pinyin with numbers
    }
    g_strfreev(parts);
    g_sprintf(tmp, "%s\n", &tmp[1]);
    gtk_text_buffer_insert_with_tags_by_name (textbuffer1, &iterator, tmp, -1, "pinyin",  NULL);

    parts = g_strsplit(word->translation, "/", NULL);
    g_sprintf(tmp, "%s ", parts[0]);
    for (i = 1; parts[i]; i++)
        g_sprintf(tmp, "%s / %s ", tmp, parts[i]);				// place extra space between individual translations // verplaatsen!!!
    g_sprintf(tmp, "%s\n\n", tmp);
    g_strfreev(parts);
    gtk_text_buffer_insert_with_tags_by_name (textbuffer1, &iterator, tmp, -1, "translation",  NULL);
}

// search on character and display the result
void search_char(gpointer data, gpointer userdata) {
    Cedict *word = (Cedict*) data;
    gchar *lookfor = (gchar*) userdata;

	word->score = word->rating;

    if (strstr(word->GB, lookfor) != 0) {	// search simplified
		if (g_utf8_strlen(lookfor, -1) == g_utf8_strlen(word->GB, -1))
			equal = g_slist_append(equal, word);
		else
			other = g_slist_append(other, word);
    } else if (strstr(word->Big5, lookfor) != 0) {	// search traditional
		if (g_utf8_strlen(lookfor, -1) == g_utf8_strlen(word->Big5, -1))
			equal = g_slist_append(equal, word);
		else
			other = g_slist_append(other, word);
	}
}

// search on pinyin and display the result
void search_pin(gpointer data, gpointer userdata) {
    Cedict *word = (Cedict*) data;
    gchar *lookfor = (gchar*) userdata;

	word->score = word->rating;

    if (strstr(word->pinyin, lookfor) != 0) {
		if (g_utf8_strlen(lookfor, -1) == g_utf8_strlen(word->pinyin, -1))
			equal = g_slist_append(equal, word);
		else
			other = g_slist_append(other, word);
    }
}

// search on translation and display the result
/*
	aantal woorden in lookup vergelijken met aantal woorden in vertaling, bv:
	lookup: "green tea"	= 2 woorden (100%)
	antwoord: "green tea" = 2 woorden = (100% relevantie)
	antwoord: "green tea leaves" = 3 woorden = (2:3 = 66% relevantie)
	verdere mogelijkheid is de relevanties ook vergelijken met de relevanties binnen een vertaling:
	ieder woord heeft immers meer vertalingen - als er veel van die vertalingen gelijkend zijn is dit woord een goede kandidaat
	de mogelijke antwoorden met gelijke relevantie verder filteren met de frequentie tabel (TPS)
*/
void search_trans(gpointer data, gpointer userdata) {
    Cedict *word = (Cedict*) data;
    gchar *lookfor = (gchar*) userdata;
    gchar tmp[1000];
    gchar **zin, **woord, **sub;
    int i, j, k;
	unsigned int len, p1, p2;
    gboolean found = FALSE;
    double lookfor_count, trans_count=0, answer_count, score=0;

	woord = g_strsplit(lookfor, " ", NULL);
	lookfor_count = g_strv_length(woord);						// number of words in the lookup string
	g_strfreev(woord);

    zin = g_strsplit(word->translation, "/", NULL);				// split in individual translations
	answer_count = g_strv_length(zin);

    for (i=0; zin[i] != NULL; i++) {
    	strcpy(tmp, zin[i]);

    	len = strlen(tmp);				// remove additional comment -> part between ()
    	p1 = strcspn(tmp, "(");
    	p2 = strcspn(tmp, ")");
		if (p1 < len && p2 < len) {
			tmp[p1] = '\0';
			g_strstrip(tmp);
			sprintf(tmp, "%s%s", tmp, &tmp[p2+1]);
    	}
		len = strlen(tmp);				// remove pinyin addition -> part between []
    	p1 = strcspn(tmp, "[");
    	p2 = strcspn(tmp, "]");
		if (p1 < len && p2 < len) {
			tmp[p1] = '\0';
			g_strstrip(tmp);
			sprintf(tmp, "%s%s", tmp, &tmp[p2+1]);
    	}

    	gchar out[1000];
    	j=0;
		for (k=0; tmp[k]; k++) {		// strip all other strange characters
			if (g_unichar_ispunct(tmp[k]))
				out[j++] = ' ';
			else if (g_unichar_isalpha(tmp[k]))
				out[j++] = tmp[k];
			else if (g_unichar_isspace(tmp[k]))
				out[j++] = tmp[k];
		}
		out[j] = '\0';

		//fprintf(stderr, "%s\n", out);
		strcpy(tmp, out);

		sub = g_strsplit(tmp, " or ", NULL);		// split a sentence that contains 'or'
		answer_count += g_strv_length(sub);

		for (int x=0; sub[x] && !found; x++) {
			strcpy(tmp, sub[x]);

			woord = g_strsplit(tmp, " ", NULL);					// split translation into individual words
			trans_count = g_strv_length(woord);

			if (strstr(tmp, lookfor) != 0) {
				for (j=0; woord[j] && !found; j++){
					strcpy(tmp, "");
					for (k=0; k<lookfor_count; k++) {
						g_strstrip(woord[j+k]);
						if (woord[j+k])
							sprintf(tmp, "%s %s", tmp, woord[j+k]);
					}
					if (strcmp(&tmp[1], lookfor) == 0) {
						//fprintf(stderr, "%s|\n", &tmp[1]);
						found = TRUE;
						score = MAX((lookfor_count/trans_count), score);
					}
				}
			}
			g_strfreev(woord);
		}
		g_strfreev(sub);
    }
    if (found) {
		word->score = (long)(word->rating * (score*score*score) / (2.0-1.0/answer_count) );
		//word->score = (long)(word->rating * (score*score*score) );
		//fprintf(stderr, "%s - rating:%d score:%f word score: %ld\n", word->GB, word->rating, score, word->score);
		other = g_slist_append(other, word);
    }
    g_strfreev(zin);
}

