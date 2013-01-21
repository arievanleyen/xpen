#include "SDL/SDL.h"
//#include "SDL/SDL_audio.h"


typedef struct sample {
    Uint8 *data;
    Uint32 dpos;
    Uint32 dlen;
} SOUNDS;


int PlaySound(char *file);
void InitAudio(void);
void mixaudio(void *unused, Uint8 *stream, int len);
