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

int main(void)
{
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
	music piece={.nchans=0, .instru=NULL, .count=NULL, .bars=0, .nevts=0, .evts=NULL};
	while(true)
	{
		piece.nchans=urand(3, 8);
		piece.nchans=min(piece.nchans, ninst);
		free(piece.instru);
		free(piece.count);
		piece.instru=malloc(piece.nchans*sizeof(unsigned int));
		piece.count=malloc(piece.nchans*sizeof(unsigned int));
		fprintf(stderr, "nchans = %u\n", piece.nchans);
		unsigned int have[128];
		for(unsigned int i=0;i<128;i++)
			have[i]=0;
		bool ch[ninst];
		for(unsigned int i=0;i<ninst;i++)
			ch[i]=false;
		for(unsigned int i=0;i<piece.nchans;i++)
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
				piece.instru[i]=j;
				piece.count[i]=urand(inst[j].few, inst[j].many);
				fprintf(stderr, "instr %u: %s*%d\n", i, inst[j].name, piece.count[i]);
				for(unsigned int k=inst[j].low;k<=inst[j].high;k++)
					have[k]++;
				i++;
			}
		}
		if(i!=piece.nchans)
		{
			fprintf(stderr, "polyphon: instrument selection internal error\n");
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
	fprintf(stderr, "polyphon: instruments selected\n");
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
