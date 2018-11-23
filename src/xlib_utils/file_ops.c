#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

///////////////////////////////////////////////////////////////////////////////////////////////
// r 只读  读模式--打开文件，从文件头开始读。
// r+  只读    读写模式--打开文件，从文件头开始读写。
// w   只写    写模式--打开文件，从文件头开始读。如果该文件已经存在，将删除所有文件已有内容。如果该文
//             件不存在，函数将创建这个文件。
// w+  只写    写模式--打开文件，从文件头开始读写。如果该文件已经存在，将删除所有文件已有内容。如果该
//             文件不存在，函数将创建这个文件。
// x   谨慎写  写模式打开文件，从文件头开始写。如果文件已经存在，该文件将不会被打开，fopen()函数将返
//             回false，而且PHP将产生一个警告。
// x+  谨慎写  读/写模式打开文件，从文件头开始写。如果文件已经存在，该文件将不会被打开，fopen()函数将
//             返回false，而且PHP将产生一个警告。
// a   追加    追加模式--打开文件，如果该文件已有内容，将从文件末尾开始追加(写)，如果该文件不存在，函
//             数将创建这个文件。
// a+  追加    追加模式--打开文件，如果该文件已有内容，将从文件末尾开始追加(写)或者读，如果该文件不存
//             在，函数将创建这个文件。
// b   二进制  二进制模式--用于与其他模式进行连接。如果文件系统能够区分二进制文件和文本文件，你可能会
//             使用它。Windows系统可以区分，而UNIX则不区分。推荐一直使用这个选项，以便获得最大程度的
//             可移植性。二进制模式是默认的模式。
// t   文本    用于与其他模式的结合。这个模式只是Windows系统下一个选项。它不是推荐选项，除非你曾经在
//             代码中使用了b选项。
///////////////////////////////////////////////////////////////////////////////////////////////

int is_path_exist(char *pdirs)
{
    if(NULL == pdirs){
        return -1;
    }
    if(access(pdirs, F_OK) != 0){
        printf("dir:%s not exist\n", pdirs);
        return -1;
    }

    return 0;
}
int file_text_read()
{
    int pid = 0;
    char szLine[32] = {0};
    FILE *pFile = NULL;

    pFile = fopen("/tmp/test.pid", "r");
    if(NULL == pFile){
        return -1;
    }

    while (fgets(szLine, sizeof(szLine), pFile)) {
        sscanf(szLine, "%d", &pid);
        printf("udhcpc -i wlan0 pid = %d\n", pid);
    }

    fclose(pFile);

    return pid;
}
int file_text_write(char *pstr)
{
    FILE *pFile = NULL;
    char szLog[1024] = {0};
    char sTimeString[64] = {0};

    //debug_current_time(sTimeString);

    sprintf(szLog, "%s:%s", sTimeString, pstr);

    pFile = fopen("/tmp/test.pid", "at");
    if(pFile){
        fwrite(szLog, strlen(szLog), 1, pFile);
        fclose(pFile);
    }
    return 0;
}

////////////////////////////////////////////////////////////////////
int file_read_and_write()
{
    FILE *pFileSrc = NULL;
    FILE *pFileDst = NULL;
    char szTempBuf[1024] = {0};
    int nSize = 0;
    int nTotalSize = 0;

    pFileSrc = fopen("/tmp/test_src.bin", "rb");
    if (NULL == pFileSrc) {
        printf("fopen failed.[%m]\n");
        goto EXIT;
    }

    pFileDst = fopen("/tmp/test_dst.bin", "wb+");
    if (NULL == pFileDst) {
        printf("fopen failed![%m]\n");
        goto EXIT;
    }

    while ((nSize = fread(szTempBuf, 1, 1024, pFileSrc)) > 0) {
        fwrite(szTempBuf, 1, nSize, pFileDst);
        nTotalSize += nSize;
    }

EXIT:
    if (pFileSrc) {
        fclose(pFileSrc);
        pFileSrc = NULL;
    }
    if (pFileDst) {
        fclose(pFileDst);
        pFileDst = NULL;
    }

    printf("write %d bytes.\n", nTotalSize);
    return 0;
}
