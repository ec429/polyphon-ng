/*
	Polyphon-NG - procedural music generator
	Copyright (C) 2012 Edward Cree

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#include "bits.h"
#include "midi.h"
#include "music.h"

int urand(int a, int b);
bool randb(void);
int select_instruments(music *piece);
int fill_flat(music *m);

int main(void)
{
	unsigned int length=64;
	unsigned int seed;
	#ifdef WINDOWS
		seed=time(NULL);
		seed^=0x26F3500*(seed&0xff); // mix it a bit
	#else
		unsigned short t[3]={time(NULL), ~time(NULL), time(NULL)^(time(NULL)<<1)};
		seed48(t);
		seed=lrand48();
	#endif
	fprintf(stderr, "polyphon: seed is %u\n", seed);
	#ifdef WINDOWS
		srand(seed);
	#else
		seed48((unsigned short[3]){seed, seed>>16, seed^(seed>>16)});
	#endif
	FILE *patchlist=fopen("patchlist", "r");
	if(!patchlist)
	{
		perror("patchlist: fopen");
		return(1);
	}
	if(load_instrument_list(patchlist))
	{
		fprintf(stderr, "patchlist: Instrument list not loaded\n");
		return(1);
	}
	fclose(patchlist);
	fprintf(stderr, "patchlist: Loaded %u instruments\n", ninst);
	music piece={.nchans=0, .instru=NULL, .count=NULL, .bars=length, .nevts=0, .evts=NULL};
	if(select_instruments(&piece))
		return(1);
	fprintf(stderr, "polyphon: instruments selected\n");
	unsigned int tonic=urand(0, 11); // C Db D Eb E F Gb G Ab A Bb B
	unsigned int mode=urand(0, N_MO-1);
	const char *t_mode="";
	if(mode==MO_MINOR) t_mode=" minor";
	else if(mode==MO_AEOLIAN) t_mode=" aeolian";
	bool flat=(tonic==1)||(tonic==3)||(tonic==6)||(tonic==8)||(tonic==10);
	fprintf(stderr, "polyphon: chose key %c%s%s\n", "CDDEEFGGAABB"[tonic], flat?"♭":"", t_mode);
	if(add_event(&piece, (event){.t_off=0, .type=EV_SETKEY, .data.key={.tonic=tonic, .mode=mode}}))
		return(1);
	unsigned int bpm=(urand(12, 40)+urand(12, 40))*2;
	fprintf(stderr, "polyphon: chose tempo %ubpm\n", bpm);
	// TODO random time sig
	if(add_event(&piece, (event){.t_off=0, .type=EV_TIME, .data.time={.ts_n=4, .ts_d=4, .ts_qpb=2, .bpm=bpm}}))
		return(1);
	if(fill_flat(&piece))
		return(1);
	return(0);
}

int fill_flat(music *m)
{
	if(!m)
	{
		fprintf(stderr, "fill_flat: m=NULL\n");
		return(1);
	}
	unsigned int t=0;
	ev_key __attribute__((unused)) key={.tonic=0, .mode=0};
	ev_time time={.ts_n=4, .ts_d=4, .ts_qpb=2, .bpm=120};
	event *note[m->nchans][8];
	for(unsigned int c=0;c<m->nchans;c++)
		for(unsigned int p=0;p<8;p++)
			note[c][p]=NULL;
	unsigned int bar=0, bart=t;
	for(unsigned int i=0;i<m->nevts;i++)
	{
		event *e=m->evts+i;
		if(e->t_off<t)
		{
			fprintf(stderr, "fill_flat: internal error, t_off decreased\n");
			return(1);
		}
		t=e->t_off;
		while(t-bart>=time.ts_n*time.ts_qpb*QUAVER)
		{
			bar++;
			bart+=time.ts_n*time.ts_qpb*QUAVER;
		}
		for(unsigned int c=0;c<m->nchans;c++)
			for(unsigned int p=0;p<8;p++)
				if(note[c][p]&&(t>=note[c][p]->t_off+note[c][p]->data.note.length))
					note[c][p]=NULL;
		switch(e->type)
		{
			case EV_SETKEY:
				key=e->data.key;
			break;
			case EV_TIME:
				time=e->data.time;
			break;
			case EV_NOTE:
				for(unsigned int i=0;i<m->count[e->data.note.chan];i++)
					if(!note[e->data.note.chan][i])
					{
						note[e->data.note.chan][i]=e;
						break;
					}
			break;
			default:
				// ignore
			break;
		}
	}
	return(0);
}

int urand(int a, int b)
{
	#ifdef WINDOWS
		int v=floor(a+((float)rand()*(b+0.99-a)/RAND_MAX));
	#else
		int v=floor(a+(drand48()*(b+0.99-a)));
	#endif
	return(v);
}

bool randb(void)
{
	#ifdef WINDOWS
		return(rand()<(RAND_MAX>>1));
	#else
		return(drand48()<0.5);
	#endif
}

int select_instruments(music *piece)
{
	if(!piece)
	{
		fprintf(stderr, "select_instruments: piece=NULL\n");
		return(1);
	}
	while(true)
	{
		piece->nchans=urand(3, 8);
		piece->nchans=min(piece->nchans, ninst);
		free(piece->instru);
		free(piece->count);
		piece->instru=malloc(piece->nchans*sizeof(unsigned int));
		piece->count=malloc(piece->nchans*sizeof(unsigned int));
		fprintf(stderr, "nchans = %u\n", piece->nchans);
		unsigned int have[128];
		for(unsigned int i=0;i<128;i++)
			have[i]=0;
		bool ch[ninst];
		for(unsigned int i=0;i<ninst;i++)
			ch[i]=false;
		for(unsigned int i=0;i<piece->nchans;i++)
		{
			unsigned int j;
			do j=urand(0, ninst-1); while(ch[j]); /* XXX infinite worst-case running time */
			ch[j]=true;
		}
		unsigned i=0;
		for(unsigned int j=0;j<ninst;j++)
		{
			if(ch[j])
			{
				piece->instru[i]=j;
				piece->count[i]=urand(inst[j].few, inst[j].many);
				fprintf(stderr, "instr %u: %s*%d\n", i, inst[j].name, piece->count[i]);
				for(unsigned int k=inst[j].low;k<=inst[j].high;k++)
					have[k]++;
				i++;
			}
		}
		if(i!=piece->nchans)
		{
			fprintf(stderr, "select_instruments: internal error\n");
			return(1);
		}
		bool reject=false;
		for(unsigned int j=40;j<=80;j++)
		{
			if(!have[j])
			{
				fprintf(stderr, "rejected, no covering of %u\n", j);
				reject=true;
				break;
			}
			if((j>=52)&&(j<=72)&&(have[j]<2))
			{
				fprintf(stderr, "rejected, insufficient covering of %u\n", j);
				reject=true;
				break;
			}
		}
		if(!reject) break;
	}
	return(0);
}
