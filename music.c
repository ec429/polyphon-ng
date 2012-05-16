#include "music.h"

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
		char *parts[7];
		for(unsigned int i=0;i<7;i++)
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
