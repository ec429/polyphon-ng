#include <stdio.h>
#include <stdbool.h>

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
	EV_NOTEON,
	EV_NOTEOFF,
	EV_SETKEY,
	//EV_SETCHORD,
	//EV_DYNAMIC, // set
	//EV_CRESC, // includes both cresc and decresc
	//EV_ACCEL, // also rit, rall, and other rubatoids (allargando, perhaps)
}
ev_type;

typedef struct
{
	bool on;
	unsigned int chan;
	unsigned int pitch; // MIDI note value
	// articulation?
}
ev_note;

typedef enum
{
	MO_MAJOR,
	MO_MINOR,
	MO_AEOLIAN,
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
	unsigned int t_off;
	ev_type type;
	union
	{
		ev_note note;
		ev_key key;
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
