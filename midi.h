#include <stdio.h>
#include "music.h"
#include "bits.h"

int midi_write(music m, FILE *fp);
void midi_encode_time(unsigned long t, unsigned char result[4], unsigned int *len);
void midi_append_time(unsigned long t, string *s);
