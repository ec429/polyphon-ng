#include "midi.h"

#include <math.h>

void midi_encode_time(unsigned long t, unsigned char result[4], unsigned int *len)
{
	unsigned char bytes[4];
	unsigned int pos=0;
	bytes[0]=t&0x7f;
	bytes[1]=(t>>7)&0x7f;
	bytes[2]=(t>>14)&0x7f;
	bytes[3]=(t>>21)&0x7f;
	int m=3;
	while(m&&!bytes[m]) m--;
	for(;m>=0;m--)
	{
		result[pos++]=bytes[m]|(m?0x80:0);
	}
	if(len) *len=pos;
}

int midi_write(music m, FILE *fp)
{
	/* MIDI HEADER */
	// 4D 54 68 64 00 00 00 06 ff ff nn nn dd dd
	fputs("MThd", fp);
	fputc(0, fp);
	fputc(0, fp);
	fputc(0, fp);
	fputc(6, fp);
	fputc(0, fp);
	fputc(1, fp);
	fputc(m.nchans>>8, fp);
	fputc(m.nchans, fp);
	fputc(CROTCHET>>8, fp);
	fputc(CROTCHET, fp);
	for(unsigned int c=0;c<m.nchans;c++)
	{
		string track=null_string();
		/* TRACK HEADER */
		// 4D 54 72 6B xx xx xx xx
		append_str(&track, "MTrk");
		append_char(&track, 0);
		append_char(&track, 0);
		append_char(&track, 0);
		append_char(&track, 0);
		// prepare for event loop
		event *note[8];
		for(unsigned int p=0;p<8;p++)
			note[p]=NULL;
		unsigned int t=0, t_last=t;
		/* PatchChange */
		// (time) Cx pp
		midi_append_time(t-t_last, &track);
		t_last=t;
		append_char(&track, 0xc0|(c&0xf));
		append_char(&track, inst[m.instru[c]].midipat);
		double vol=0.6;
		for(unsigned int i=0;i<m.nevts;i++)
		{
			event *e=m.evts+i;
			if(e->t_off<t)
			{
				fprintf(stderr, "midi_write: internal error, t_off decreased\n");
				return(1);
			}
			while(t<e->t_off)
			{
				t++;
				for(unsigned int p=0;p<8;p++)
					if(note[p]&&(t>=note[p]->t_off+note[p]->data.note.length))
					{
						/* NoteOff */
						// (time) 8x nn vv
						midi_append_time(t-t_last, &track);
						t_last=t;
						append_char(&track, 0x80|(c&0xf));
						append_char(&track, note[p]->data.note.pitch);
						append_char(&track, 0);
						note[p]=NULL;
					}
			}
			switch(e->type)
			{
				case EV_SETKEY:
					if(c==0)
					{
						// TODO write key event
						//key=e->data.key;
						midi_append_time(t-t_last, &track);
						t_last=t;
						append_char(&track, 0xff);
						append_char(&track, 0x59);
						append_char(&track, 0x02);
						append_char(&track, key_sf(e->data.key));
						append_char(&track, (e->data.key.mode==MO_MAJOR)?0:1);
					}
				break;
				case EV_TIME:
					if(c==0)
					{
						/* Meta Tempo */
						// (time) ff 51 03 tt tt tt
						midi_append_time(t-t_last, &track);
						t_last=t;
						append_char(&track, 0xff);
						append_char(&track, 0x51);
						append_char(&track, 0x03);
						double qps=e->data.time.ts_qpb*e->data.time.bpm/120.0;
						unsigned int us=floor(1e6/qps);
						append_char(&track, us>>16);
						append_char(&track, us>>8);
						append_char(&track, us);
						/* Meta TimeSig */
						// (time) ff 58 04 nn dd cc bb
						midi_append_time(0, &track);
						append_char(&track, 0xff);
						append_char(&track, 0x58);
						append_char(&track, 0x04);
						append_char(&track, e->data.time.ts_n);
						append_char(&track, log2(e->data.time.ts_d));
						append_char(&track, CROTCHET);
						append_char(&track, 8);
					}
				break;
				case EV_NOTE:
					if(e->data.note.chan==c)
					{
						for(unsigned int p=0;p<8;p++)
						{
							if(!note[p])
							{
								note[p]=e;
								/* NoteOn */
								// (time) 9x nn vv
								midi_append_time(t-t_last, &track);
								t_last=t;
								append_char(&track, 0x90|(c&0xf));
								append_char(&track, note[p]->data.note.pitch);
								append_char(&track, floor(vol*127));
								break;
							}
						}
					}
				break;
				case EV_DYN:
					if(e->data.dyn.chan==c)
					{
						vol=e->data.dyn.vol;
						/* Channel AfterTouch */
						// (time) Dx vv
						midi_append_time(t-t_last, &track);
						t_last=t;
						append_char(&track, 0xd0|(c&0xf));
						append_char(&track, floor(vol*127));
					}
				break;
				default:
					fprintf(stderr, "midi_write: warning, unrecognised event type %u\n", e->type);
				break;
			}
		}
		/* Meta TrackEnd */
		// (time) ff 2f 00
		midi_append_time(0, &track);
		append_char(&track, 0xff);
		append_char(&track, 0x2f);
		append_char(&track, 0);
		// Fill in length in track header
		size_t tl=track.i;
		if(track.l<8)
		{
			fprintf(stderr, "midi_write: internal error, track.l<8\n");
			return(1);
		}
		if(tl<8)
		{
			fprintf(stderr, "midi_write: internal error, tl<8\n");
			return(1);
		}
		tl-=8;
		track.buf[4]=tl>>24;
		track.buf[5]=tl>>16;
		track.buf[6]=tl>>8;
		track.buf[7]=tl;
		// Write track
		fwrite(track.buf, 1, track.i, fp);
		free_string(&track);
	}
	return(0);
}

void midi_append_time(unsigned long t, string *s)
{
	unsigned char tbuf[4];
	unsigned int len=0;
	midi_encode_time(t, tbuf, &len);
	for(unsigned int i=0;i<len;i++)
		append_char(s, tbuf[i]);
}
