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
double durand(double x);
bool randb(void);
bool randp(double p);
int select_instruments(music *piece);
int fill_flat(music *m, double power);

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
	music piece={.nchans=0, .instru=NULL, .count=NULL, .bars=length*3/4, .nevts=0, .evts=NULL};
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
	for(unsigned int c=0;c<piece.nchans;c++)
		if(add_event(&piece, (event){.t_off=0, .type=EV_DYN, .data.dyn={.chan=c, .vol=0.6}}))
			return(1);
	if(fill_flat(&piece, bpm/160.0))
		return(1);
	piece.bars=length;
	if(fill_flat(&piece, 1.0))
		return(1);
	fprintf(stderr, "polyphon: writing output\n");
	if(midi_write(piece, stdout))
		return(1);
	fprintf(stderr, "polyphon: wrote output file; done\n");
	return(0);
}

int fill_flat(music *m, double power)
{
	if(!m)
	{
		fprintf(stderr, "fill_flat: m=NULL\n");
		return(1);
	}
	unsigned int t=0;
	ev_key key={.tonic=0, .mode=0};
	ev_time time={.ts_n=4, .ts_d=4, .ts_qpb=2, .bpm=120};
	unsigned int note[m->nchans][8], old[m->nchans][8];
	for(unsigned int c=0;c<m->nchans;c++)
		for(unsigned int p=0;p<8;p++)
			note[c][p]=old[c][p]=0;
	unsigned int dyn[m->nchans];
	for(unsigned int c=0;c<m->nchans;c++)
		dyn[c]=0;
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
				if(note[c][p]&&(t>=m->evts[note[c][p]].t_off+m->evts[note[c][p]].data.note.length))
				{
					old[c][p]=note[c][p];
					note[c][p]=0;
				}
		switch(e->type)
		{
			case EV_SETKEY:
				key=e->data.key;
			break;
			case EV_TIME:
				time=e->data.time;
			break;
			case EV_NOTE:
				for(unsigned int p=0;p<m->count[e->data.note.chan];p++)
					if(!note[e->data.note.chan][p])
					{
						note[e->data.note.chan][p]=i;
						break;
					}
			break;
			default:
				// ignore
			break;
		}
	}
	double energy=power*4096;
	while(bar<m->bars)
	{
		t++;
		energy+=power*32/QUAVER;
		if(randp(energy/4096.0))
		{
			unsigned int c=urand(0, m->nchans-1);
			double v=dyn[c]?m->evts[dyn[c]].data.dyn.vol:0.6;
			double d=durand((1-v)/3.0);
			dyn[c]=m->nevts;
			if(add_event(m, (event){.t_off=t, .type=EV_DYN, .data.dyn={.chan=c, .vol=v+d}}))
				return(1);
		}
		if(randp(0.5-(energy/4096.0)))
		{
			unsigned int c=urand(0, m->nchans-1);
			double v=dyn[c]?m->evts[dyn[c]].data.dyn.vol:0.6;
			double d=durand(v/3.0);
			dyn[c]=m->nevts;
			if(add_event(m, (event){.t_off=t, .type=EV_DYN, .data.dyn={.chan=c, .vol=v-d}}))
				return(1);
		}
		while(t-bart>=time.ts_n*time.ts_qpb*QUAVER)
		{
			bar++;
			bart+=time.ts_n*time.ts_qpb*QUAVER;
		}
		for(unsigned int c=0;c<m->nchans;c++)
			for(unsigned int p=0;p<8;p++)
				if(note[c][p]&&(t>=m->evts[note[c][p]].t_off+m->evts[note[c][p]].data.note.length))
				{
					old[c][p]=note[c][p];
					note[c][p]=0;
				}
		for(unsigned int c=0;c<m->nchans;c++)
			for(unsigned int p=0;p<m->count[c];p++)
			{
				if(!note[c][p])
				{
					unsigned int i=m->instru[c];
					double startp;
					unsigned int u=t-bart;
					double ef=1;
					if(energy<inst[i].power*2) ef=inst[i].power/(double)(inst[i].power*3-energy);
					if((u%SEMIBREVE)==0)
						startp=0.85;
					else if((u%MINIM)==0)
						startp=0.72;
					else if((u%CROTCHET)==0)
						startp=0.64;
					else if((u%QUAVER)==0)
						startp=0.5*ef;
					else
						startp=0.3*ef;
					if(energy<inst[i].power*16) startp*=exp2((energy-inst[i].power*16)/64.0);
					if(randp(startp))
					{
						unsigned int centre=(inst[i].low+inst[i].high)/2, width=inst[i].high-inst[i].low;
						double rating[128];
						double total=0;
						for(unsigned int n=inst[i].low;n<=inst[i].high;n++)
						{
							character ch=inst[i].ch;
							double r_melodic=1;
							if(old[c][p])
							{
								r_melodic=rate_interval_m(n-m->evts[old[c][p]].data.note.pitch);
								unsigned int age=t-m->evts[old[c][p]].t_off-m->evts[old[c][p]].data.note.length;
								if((ch==CH_WIND)||(ch==CH_BRASS))
								{
									r_melodic*=exp2(-abs(n-m->evts[old[c][p]].data.note.pitch)/10.0);
								}
								r_melodic=pow(r_melodic, MINIM/(double)(MINIM+age));
								old[c][p]=0;
							}
							double r_range=exp(-abs(n-centre)*0.5/(double)width);
							double r_key=rate_key(n, key);
							double r_harmonic=1;
							unsigned int harms=0;
							unsigned int oc=0;
							for(unsigned int c2=0;c2<m->nchans;c2++)
								for(unsigned int p2=0;p2<m->count[c2];p2++)
								{
									if((c==c2)&&(p==p2)) continue;
									if(note[c2][p2])
									{
										int interval=(int)n-(int)m->evts[note[c2][p2]].data.note.pitch;
										double hrm=rate_interval_h(interval);
										if(!(abs(interval)%12)) oc++;
										unsigned int age=t-m->evts[note[c2][p2]].t_off;
										character ch2=inst[m->instru[c2]].ch;
										double cf=cfactor(ch, 0), cf2=cfactor(ch2, age);
										if((cf==0)||(cf2==0)) hrm=1;
										hrm=pow(hrm, 3.0*max(cf, cf2));
										r_harmonic*=hrm;
										harms++;
									}
								}
							r_harmonic*=harms/(double)(harms+1.5*oc);
							r_harmonic=pow(r_harmonic, sqrt(8.0/harms));
							rating[n]=r_melodic*r_range*r_key*r_harmonic;
							total+=rating[n];
						}
						unsigned int n=inst[i].low;
						if(randp(0.03))
						{
							double r=durand(total);
							while(r>rating[n])
							{
								r-=rating[n];
								n++;
								if(n>inst[i].high) // shouldn't happen
								{
									fprintf(stderr, "flat_fill: warning: n>inst[i].high\n");
									break;
								}
							}
						}
						else
						{
							unsigned int nc=randp(0.2)?urand(2, 3):1, tops[nc], least;
							for(unsigned int k=0;k<nc;k++)
								tops[k]=n+k;
							least=0;
							for(unsigned int k=1;k<nc;k++)
								if(rating[tops[k]]<rating[tops[least]]) least=k;
							for(unsigned int j=n+nc;j<=inst[i].high;j++)
								if(rating[j]>rating[tops[least]])
								{
									tops[least]=j;
									least=0;
									for(unsigned int k=1;k<nc;k++)
										if(rating[tops[k]]<rating[tops[least]]) least=k;
								}
							n=tops[urand(0, nc-1)];
						}
						if(n<=inst[i].high)
						{
							unsigned int l=0;
							while(true)
							{
								l++;
								unsigned int u=t+l, baru=bart;
								while(u-baru>=time.ts_n*time.ts_qpb*QUAVER) baru+=time.ts_n*time.ts_qpb*QUAVER;
								u-=baru;
								double r=1;
								if((u%SEMIBREVE)==0)
									r=2.0;
								else
								{
									if((u%SEMIBREVE)<u)
										r=0.2;
									if((u%MINIM)==0)
										r*=1.8;
									else
									{
										if((u%MINIM)<(u%SEMIBREVE))
											r*=0.55;
										if((u%CROTCHET)==0)
											r*=1.45;
										else
										{
											if((u%CROTCHET)<(u%MINIM))
												r*=0.75;
											if((u%QUAVER)==0)
												r*=1.35;
											else if((u%QUAVER)<(u%CROTCHET))
												r*=0.65;
										}
									}
								}
								if(l<QUAVER) r*=ef;
								else if(l<CROTCHET) r*=sqrt(ef);
								r*=pow(l/(double)(1.0*QUAVER+l), centre<60?2.5:2);
								unsigned int hl=l;
								while(hl&&!(hl&1))
								{
									hl>>=1;
									r*=1.3;
								}
								hl=l;
								while(hl>=SEMIBREVE)
								{
									hl-=SEMIBREVE;
									r*=2;
								}
								if(!randp(4.0/(4.0+r))) break;
							}
							note[c][p]=m->nevts;
							if(add_event(m, (event){.t_off=t, .type=EV_NOTE, .data.note={.chan=c, .length=l, .pitch=n}}))
								return(1);
							energy-=inst[i].power*(log2(n)/6.0)*(dyn[c]?m->evts[dyn[c]].data.dyn.vol:0.6)/8.0;
						}
					}
				}
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

double durand(double x)
{
	#ifdef WINDOWS
		return(rand()*x/RAND_MAX);
	#else
		return(drand48()*x);
	#endif
}

bool randb(void)
{
	#ifdef WINDOWS
		return(rand()<(RAND_MAX>>1));
	#else
		return(drand48()<0.5);
	#endif
}

bool randp(double p)
{
	#ifdef WINDOWS
		return(rand()<(RAND_MAX*p));
	#else
		return(drand48()<p);
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
