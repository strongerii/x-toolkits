#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BUFFER_SIZE 3072
#define BASE64_BUFFER (BUFFER_SIZE/24 + BUFFER_SIZE*4/3)
const char *Base64Table = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

void base64Encode_file(int fd , FILE *pipe_fd , int fileSize){
	char temp;
	char *in;  //for read()
	char *out;
	unsigned int source_index = 0,target_index = 0;
	int bits_8_group;
  	unsigned int x = 0 , y = 0;
  	//unsigned int buf_byte_num = BUFFER_SIZE , tmp;

  	in = (char *)malloc(BUFFER_SIZE);
  	out = (char *)malloc(BASE64_BUFFER);
  	bits_8_group = fileSize/3;

  	if(read(fd, in, BUFFER_SIZE) < 0){
  		printf("read error\n");
  		return;
  	}

	while( x < bits_8_group ){
		temp = in[source_index] >> 2;
		out[target_index++] = Base64Table[temp & 0x3F];
		temp = in[source_index] << 4;
		temp = temp & 0x30;
		temp = (in[source_index + 1] >> 4) | temp;
		out[target_index++] = Base64Table[temp & 0x3F];
		temp = in[source_index + 1] & 0x0f;
		temp <<= 2;
		temp = temp & 0x3c;
		temp = (in[source_index + 2] >> 6) | temp;
		out[target_index++] = Base64Table[temp & 0x3F];
		temp = in[source_index + 2];
		out[target_index++] = Base64Table[temp & 0x3F];

		source_index += 3;
		x++;
		y++;
		if( y == 18 ){
			y = 0;
			out[target_index++] = '\n';
		}
		if(source_index == BUFFER_SIZE){
			fwrite(out,sizeof(char),target_index,pipe_fd);
			//tmp = read(fd, in, BUFFER_SIZE);
			if(read(fd, in, BUFFER_SIZE) < 0){
				printf("read error\n");
  				return;
			}
			source_index = 0;
			target_index = 0;
		}
	}
	fwrite(out,sizeof(char),target_index,pipe_fd);
	source_index = 0;
	target_index = 0;

	switch( fileSize%3 ){
		case 1:
			temp = in[source_index] >> 2;
			out[target_index++] = Base64Table[temp & 0x3F];
			temp = in[source_index] << 4;
			temp = temp & 0x30;
			temp = (in[source_index + 1] >> 4) | temp;
			out[target_index++] = Base64Table[temp & 0x3F];
			out[target_index++] = '=';
			out[target_index++] = '=';
			fwrite(out,sizeof(char), 3 ,pipe_fd);
			break;
		case 2:
			temp = in[source_index] >> 2;
			out[target_index++] = Base64Table[temp & 0x3F];
			temp = in[source_index] << 4;
			temp = temp & 0x30;
			temp = (in[source_index + 1] >> 4) | temp;
			out[target_index++] = Base64Table[temp & 0x3F];
			temp = in[source_index + 1] & 0x0f;
			temp <<= 2;
			temp = temp & 0x3c;
			out[target_index++] = Base64Table[temp & 0x3F];
			out[target_index++] = '=';
			fwrite(out,sizeof(char), 3 ,pipe_fd);
			break;
	}
}

void base64Encode(char *in , char *out ,int len ,int *dlen){
        char temp;
        int source_index = 0,target_index = 0;
        int bits_8_group;
        int x = 0 , y = 0;

        bits_8_group = len/3;

        while( x < bits_8_group ){
                temp = in[source_index] >> 2;
                out[target_index++] = Base64Table[temp & 0x3F];
                temp = in[source_index] << 4;
                temp = temp & 0x30;
                temp = (in[source_index + 1] >> 4) | temp;
                out[target_index++] = Base64Table[temp & 0x3F];
                temp = in[source_index + 1] & 0x0f;
                temp <<= 2;
                temp = temp & 0x3c;
                temp = (in[source_index + 2] >> 6) | temp;
                out[target_index++] = Base64Table[temp & 0x3F];
                temp = in[source_index + 2];
                out[target_index++] = Base64Table[temp & 0x3F];

                source_index += 3;
                x++;
                y++;
                if( y == 18 ){
                        y = 0;
                        out[target_index++] = '\n';
                }
        }
        switch( len % 3 ){
        case 1:
                temp = in[source_index] >> 2;
                out[target_index++] = Base64Table[temp & 0x3F];
                temp = in[source_index] << 4;
                temp = temp & 0x30;
                temp = (in[source_index + 1] >> 4) | temp;
                out[target_index++] = Base64Table[temp & 0x3F];
                out[target_index++] = '=';
                out[target_index++] = '=';
                break;
        case 2:
                temp = in[source_index] >> 2;
                out[target_index++] = Base64Table[temp & 0x3F];
                temp = in[source_index] << 4;
                temp = temp & 0x30;
                temp = (in[source_index + 1] >> 4) | temp;
                out[target_index++] = Base64Table[temp & 0x3F];
                temp = in[source_index + 1] & 0x0f;
                temp <<= 2;
                temp = temp & 0x3c;
                out[target_index++] = Base64Table[temp & 0x3F];
                out[target_index++] = '=';
                break;
        }
        *dlen = target_index;
}
void base64Decode(void *dst,char *src,int maxlen)
{
	int bitval,bits;
	int val;
	int len,x,y;

	len = strlen(src);
	bitval=0;
	bits=0;
	y=0;

	for(x=0;x<len;x++)
	{
		if((src[x]>='A')&&(src[x]<='Z')) val=src[x]-'A'; else
		if((src[x]>='a')&&(src[x]<='z')) val=src[x]-'a'+26; else
		if((src[x]>='0')&&(src[x]<='9')) val=src[x]-'0'+52; else
		if(src[x]=='+') val=62; else
		if(src[x]=='-') val=63; else
		   	val=-1;
		if(val>=0)
		{
		    bitval=bitval<<6;
		    bitval+=val;
		    bits+=6;
		    while (bits>=8)
		    {
				if(y<maxlen)
		       		((char *)dst)[y++]=(bitval>>(bits-8))&0xFF;
				bits-=8;
				bitval &= (1<<bits)-1;
		    }
		}
	}
	if (y<maxlen)
	   ((char *)dst)[y++]=0;
}