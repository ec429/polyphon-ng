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
