#include "music.h"

#include <math.h>
#include "bits.h"

int load_instrument_list(FILE *fp)
{
	ninst=0;
	inst=NULL;
	if(!fp)
	{
		fprintf(stderr, "load_instrument_list: fp is NULL\n");
		return(1);
	}
	char *line=NULL;
	unsigned int lines=0;
	while(!feof(fp)&&(line=fgetl(fp)))
	{
		lines++;
		if((!*line)||(*line=='#'))
		{
			free(line);
			continue;
		}
		char *parts[8];
		for(unsigned int i=0;i<8;i++)
		{
			if(!(parts[i]=strtok(i?NULL:line, " \t")))
			{
				fprintf(stderr, "load_instrument_list: not enough parts on line %u\n", lines);
				free(line);
				return(1);
			}
		}
		instrument new;
		if(sscanf(parts[0], "%u", &new.few)!=1)
		{
			fprintf(stderr, "load_instrument_list: bad 'few' on line %u\n", lines);
			free(line);
			return(1);
		}
		if(new.few>8)
		{
			fprintf(stderr, "load_instrument_list: few>8 on line %u\n", lines);
			free(line);
			return(1);
		}
		if(sscanf(parts[1], "%u", &new.many)!=1)
		{
			fprintf(stderr, "load_instrument_list: bad 'many' on line %u\n", lines);
			free(line);
			return(1);
		}
		if(new.many>8)
		{
			fprintf(stderr, "load_instrument_list: many>8 on line %u\n", lines);
			free(line);
			return(1);
		}
		new.name=strdup(parts[2]);
		if(sscanf(parts[3], "%u", &new.midipat)!=1)
		{
			fprintf(stderr, "load_instrument_list: bad 'midipat' on line %u\n", lines);
			free(line);
			return(1);
		}
		if(sscanf(parts[4], "%u", &new.low)!=1)
		{
			fprintf(stderr, "load_instrument_list: bad 'low' on line %u\n", lines);
			free(line);
			return(1);
		}
		if(sscanf(parts[5], "%u", &new.high)!=1)
		{
			fprintf(stderr, "load_instrument_list: bad 'high' on line %u\n", lines);
			free(line);
			return(1);
		}
		if(strcmp(parts[6], "PLOSIVE")==0)
			new.ch=CH_PLOSIVE;
		else if(strcmp(parts[6], "BOWED")==0)
			new.ch=CH_BOWED;
		else if(strcmp(parts[6], "WIND")==0)
			new.ch=CH_WIND;
		else if(strcmp(parts[6], "BRASS")==0)
			new.ch=CH_BRASS;
		else if(strcmp(parts[6], "PERC")==0)
			new.ch=CH_PERC;
		else
		{
			fprintf(stderr, "load_instrument_list: bad 'ch' on line %u\n", lines);
			free(line);
			return(1);
		}
		if(sscanf(parts[7], "%u", &new.power)!=1)
		{
			fprintf(stderr, "load_instrument_list: bad 'power' on line %u\n", lines);
			free(line);
			return(1);
		}
		unsigned int n=ninst++;
		instrument *ni=realloc(inst, ninst*sizeof(instrument));
		if(!ni)
		{
			perror("load_instrument_list: realloc");
			free(line);
			ninst=n;
			return(1);
		}
		(inst=ni)[n]=new;
		free(line);
	}
	return(0);
}

int add_event(music *m, event e)
{
	if(!m)
	{
		fprintf(stderr, "add_event: m is NULL\n");
		return(1);
	}
	unsigned int n=m->nevts++;
	event *ne=realloc(m->evts, m->nevts*sizeof(event));
	if(!ne)
	{
		perror("add_event: realloc");
		m->nevts=n;
		return(1);
	}
	(m->evts=ne)[n]=e;
	return(0);
}

double rate_interval_m(int i) // melodic intervals
{
	int o=0;
	i=abs(i);
	while(i>9)
	{
		o++;
		i=abs(i-12);
	}
	double of=exp2((1-o)/1.5);
	if(o)
	{
		switch(i)
		{
			case 0: // octave
				return 0.5*of;
			case 1: // major seventh or augmented octave
				return 0.1*of;
			case 2: // minor seventh or major ninth
				return 0.3*of;
			case 3: // minor tenth (third)
				return 0.15*of;
			case 4: // major tenth (third)
				return 0.12*of;
			case 5: // perfect eleventh (fourth)
				return 0.35*of;
			case 6: // augmented eleventh (fourth)
				return 0.04*of;
			case 7: // perfect twelfth (fifth)
				return 0.38*of;
			case 8: // minor thirteenth (sixth)
				return 0.19*of;
			case 9: // major thirteenth (sixth)
				return 0.17*of;
		}
		fprintf(stderr, "rate_interval_m: internal error\n");
		return(0);
	}
	else
	{
		switch(i)
		{
			case 0: // unison
				return 0.32;
			case 1: // minor second
				return 0.45;
			case 2: // major second
				return 0.8;
			case 3: // minor third
				return 0.72;
			case 4: // major third
				return 0.7;
			case 5: // perfect fourth
				return 0.65;
			case 6: // augmented fourth
				return 0.12; // it's not great but it's nice to use occasionally
			case 7: // perfect fifth
				return 0.75;
			case 8: // minor sixth
				return 0.55;
			case 9: // major sixth
				return 0.5;
		}
		fprintf(stderr, "rate_interval_m: internal error\n");
		return(0);
	}
}

double rate_interval_h(int i)
{
	int o=0;
	i=abs(i);
	while(i>11)
	{
		o++;
		i=abs(i-12);
	}
	double v=0;
	switch(i)
	{
		case 0: // unison
			v=0.5;
		case 1: // minor second
			v=0.002;
		case 2: // major second
			v=0.06;
		case 3: // minor third
			v=0.82;
		case 4: // major third
			v=0.75;
		case 5: // perfect fourth
			v=0.8;
		case 6: // augmented fourth
			v=0.03;
		case 7: // perfect fifth
			v=1.0;
		case 8: // minor sixth
			v=0.85;
		case 9: // major sixth
			v=0.9;
		case 10: // minor seventh
			v=0.62;
		case 11: // major seventh
			v=0.04;
	}
	if(o) return((v+(o*0.15))/(1.0+(o*0.15))); // Dissonance matters less when it's spaced by octaves.
	return(v);
}

double rate_key(unsigned int n, ev_key key)
{
	unsigned int deg=(n+12-key.tonic)%12;
	switch(key.mode)
	{
		case MO_MAJOR:
			switch(deg)
			{
				case 0: // tonic
					return 1.3;
				case 1: // flattened supertonic
					return 0.02;
				case 2: // supertonic
					return 0.85;
				case 3: // minor mediant
					return 0.05;
				case 4: // major mediant
					return 0.8;
				case 5: // subdominant
					return 0.9;
				case 6: // tritone, wolf, call it what you will, no-one likes a sharpened subdominant
					return 0.01;
				case 7: // dominant
					return 0.95;
				case 8: // minor submediant
					return 0.06;
				case 9: // major submediant
					return 0.75;
				case 10: // flattened (or melodic minor) subtonic
					return 0.2;
				case 11: // (major or harmonic minor) subtonic
					return 0.65;
			}
		break;
		case MO_MINOR:
			switch(deg)
			{
				case 0: // tonic
					return 1.3;
				case 1: // flattened supertonic
					return 0.02;
				case 2: // supertonic
					return 0.85;
				case 3: // minor mediant
					return 0.8;
				case 4: // major mediant
					return 0.03;
				case 5: // subdominant
					return 0.9;
				case 6: // tritone
					return 0.01;
				case 7: // dominant
					return 0.95;
				case 8: // minor submediant
					return 0.7;
				case 9: // major submediant
					return 0.02;
				case 10: // flattened subtonic
					return 0.2;
				case 11: // subtonic
					return 0.65;
			}
		break;
		case MO_AEOLIAN:
			switch(deg)
			{
				case 0: // tonic
					return 1.3;
				case 1: // flattened supertonic
					return 0;
				case 2: // supertonic
					return 0.85;
				case 3: // minor mediant
					return 0.8;
				case 4: // major mediant
					return 0;
				case 5: // subdominant
					return 0.9;
				case 6: // tritone
					return 0;
				case 7: // dominant
					return 0.95;
				case 8: // minor submediant
					return 0.68;
				case 9: // major submediant
					return 0;
				case 10: // flattened subtonic
					return 0.72;
				case 11: // subtonic
					return 0;
			}
		break;
		default:
			fprintf(stderr, "rate_key: unknown modality %d\n", key.mode);
			return(0);
		break;
	}
	fprintf(stderr, "rate_key: internal error\n");
	return(0);
}

double cfactor(character ch, unsigned int age)
{
	switch(ch)
	{
		case CH_BOWED:
			return(0.8);
		case CH_PLOSIVE:
			return(3*QUAVER/(double)(age+QUAVER));
		case CH_WIND:
			return(1.6);
		case CH_BRASS:
			return(2+QUAVER/(double)(age+QUAVER));
		case CH_PERC:
			return(0);
	}
	fprintf(stderr, "cfactor: unrecognised character %u\n", ch);
	return(1);
}
