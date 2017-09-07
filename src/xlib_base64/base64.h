#ifndef _BASE64_H_
#define _BASE64_H_

void base64Encode_file(int fd , FILE *pipe_fd , int fileSize);

void base64Encode(char *in , char *out ,int len ,int *dlen);

void base64Decode(void *dst,char *src,int maxlen);

#endif //_BASE64_H_