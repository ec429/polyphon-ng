#include "midi.h"

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
		event *note[8];
		for(unsigned int p=0;p<8;p++)
			note[p]=NULL;
		unsigned int t=0, t_last=t;
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
					// TODO write key event
					//key=e->data.key;
				break;
				case EV_TIME:
					// TODO write timesig & tempo events
					//time=e->data.time;
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
								append_char(&track, 96); // volume (dynamics not done yet)
								break;
							}
						}
					}
				break;
				default:
					// ignore
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
