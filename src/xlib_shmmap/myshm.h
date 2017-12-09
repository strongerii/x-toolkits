//for mutex
#include <pthread.h>


#define DEFAULT_SHM_KEY 0x45

/*******************************************************************************
 
 
 *******************************************************************************/

//share mem stru
#define MAX_SHM_FRAME_NUM 50
#define MAX_SHM_BUFF_LEN 2048000
#define MAX_SHM_FRAME_LEN 307200
//#define MAX_SHM_WRITE_OFFSET (MAX_SHM_BUFF_LEN-MAX_SHM_FRAME_LEN) // 2/10


#define SHM_RET_OK 1
#define SHM_RET_NO_FRAME 0
#define SHM_RET_PARAM_ERR -1
#define SHM_RET_BUFF_SIZE_ERR -2
#define SHM_RET_NOT_SYNC -3

typedef struct
{
    pthread_mutex_t FrameMutex;
    unsigned int FrameOffset;
    unsigned int FrameIdx;
    unsigned int FrameFlag; //i frame  = 0x01
    unsigned int FrameLen;
    unsigned int FrameReadFlag; //every module use one bit
}SHM_FRAME_UNIT_STRU;

typedef struct
{
    unsigned int MaxFrameCnt; 
    unsigned int MaxFrameBuffLen;  
    unsigned int MaxWriteOffset; 
    unsigned int LastFrameIdx;
    
    unsigned int CurWriteOffset;
    unsigned int CurWritePadding;
    unsigned int CurUsedBuffLen;
    unsigned int BuffOffset;    // buffoffset  = sizeof(mng) + unit_len 
    
}SHM_FRAME_MNG_STRU;

#ifdef __cplusplus
extern "C"
{
#endif //__cplusplus
/*****************************************************************************
 
 
 *****************************************************************************/
int CreatShareMem(int Key, int MaxFrameNum, int MaxBuffLen, int MaxOneFrameLen);
void DetachShareMem(void *ShmPtr);
char *AttachShareMem(int ShmId);
void ReleaseShareMem(int ShmId);

int ShmFrameIn(SHM_FRAME_MNG_STRU *ShmPtr, char *Buff, int Len);
int ShmFrameOut(SHM_FRAME_MNG_STRU *ShmPtr, unsigned int *ReadIdx, unsigned int *FrameIdx, char *Buff, int *BuffLen, unsigned int *FrameFlag);
int ShmFrameSync(SHM_FRAME_MNG_STRU *ShmPtr, unsigned int *ReadIdx, unsigned int *FrameIdx);

#ifdef __cplusplus
}
#endif //__cplusplus