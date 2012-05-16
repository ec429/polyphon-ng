#pragma once
#include <stdio.h>
#include <stdbool.h>

#define SEMIQUAVER	1
#define QUAVER		(SEMIQUAVER*2)
#define D_QUAVER	(SEMIQUAVER*3)
#define CROTCHET	(QUAVER*2)
#define D_CROTCHET	(QUAVER*3)
#define MINIM		(CROTCHET*2)
#define D_MINIM		(CROTCHET*3)
#define SEMIBREVE	(MINIM*2)

typedef enum
{
	CH_BOWED,
	CH_PLOSIVE,
	CH_WIND,
	CH_BRASS,
	CH_PERC,
}
character;

typedef struct
{
	char *name;
	unsigned int midipat;
	unsigned int low, high; // range, MIDI note numbers
	unsigned int few, many; // number of parts
	character ch;
}
instrument;

typedef enum
{
	EV_NOTE,
	EV_SETKEY,
	EV_TIME, // tempo and timesig
	//EV_SETCHORD,
	//EV_DYNAMIC, // set
	//EV_CRESC, // includes both cresc and decresc
	//EV_ACCEL, // also rit, rall, and other rubatoids (allargando, perhaps)
}
ev_type;

typedef struct
{
	unsigned int chan;
	unsigned int length;
	unsigned int pitch; // MIDI note value
	// articulation?
}
ev_note;

typedef enum
{
	MO_MAJOR,
	MO_MINOR,
	MO_AEOLIAN,
	N_MO
}
modality;

typedef struct
{
	unsigned int tonic; // 0 to 11
	modality mode;
}
ev_key;

typedef struct
{
	unsigned int ts_n;
	unsigned int ts_d;
	unsigned int ts_qpb; // quavers per beat, eg. 2 for 4/4, 3 for 6/8
	unsigned int bpm; // interpreted relative to ts_qpb
}
ev_time;

typedef struct
{
	unsigned int t_off;
	ev_type type;
	union
	{
		ev_note note;
		ev_key key;
		ev_time time;
		//ev_chord chord;
		//ev_dyn dyn;
		//ev_cresc cresc;
		//ev_accel accel;
	}
	data;
}
event;

typedef struct
{
	unsigned int nchans;
	unsigned int *instru; // [channel] offset into the instrument list
	unsigned int *count; // [channel] number of parts
	unsigned int bars; // length
	unsigned int nevts;
	event *evts;
}
music;

unsigned int ninst;
instrument *inst;

int load_instrument_list(FILE *fp);
int add_event(music *m, event e);

double rate_interval_m(int i);
double rate_key(unsigned int n, ev_key key);
