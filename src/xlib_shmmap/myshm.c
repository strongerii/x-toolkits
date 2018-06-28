#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// for shm
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

// sysconf
#include <unistd.h>

#include "myshm.h"

int CreatShareMem(int Key, int MaxFrameNum, int MaxBuffLen, int MaxOneFrameLen)
{
    int mng_len = sizeof(SHM_FRAME_MNG_STRU);
    int unit_len = sizeof(SHM_FRAME_UNIT_STRU)*MaxFrameNum;
    int page_size = sysconf(_SC_PAGESIZE);
    int max_buff_len = mng_len + unit_len + MaxBuffLen;
    int shmid = -1;

    if(max_buff_len%page_size)
    {
        max_buff_len = (page_size - max_buff_len%page_size) + max_buff_len;
    }

    shmid = shmget(Key, max_buff_len, IPC_CREAT| 0777);

    printf("Key:%d, PageSize:%d, MaxLen:%d, MngLen:%d, UnitLen:%d, BuffLen:%d, MaxOneFrameLen:%d, ShmId:%d"
        , Key, page_size, max_buff_len, mng_len, unit_len, max_buff_len-mng_len-unit_len, MaxOneFrameLen, shmid);

    if (shmid<0)
    {
        perror("shmget");
        return shmid;
    }
    else
    {
        char *p_buff = (char*)shmat(shmid, 0, 0);
        if (p_buff==(void *)-1)
        {
            printf("Shmid:%d, shmat err", shmid);
            perror("shmat");
            shmctl(shmid, IPC_RMID, 0);
            return -2;
        }
        else if(NULL == p_buff)
        {
            printf("attach share mem err:%p", p_buff);
            shmctl(shmid, IPC_RMID, 0);
            return -3;
        }
        else
        {
            SHM_FRAME_MNG_STRU *p_shm = (SHM_FRAME_MNG_STRU *)p_buff;
            int i;
            pthread_mutexattr_t attr;
            pthread_mutexattr_init(&attr);
            pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
            
            memset(p_shm, 0, max_buff_len);

            p_shm->MaxFrameCnt = MaxFrameNum;
            p_shm->MaxFrameBuffLen = max_buff_len - mng_len - unit_len;
            p_shm->MaxWriteOffset = p_shm->MaxFrameBuffLen - MaxOneFrameLen;
            p_shm->BuffOffset = mng_len + unit_len;

            if(1)
            {
                SHM_FRAME_UNIT_STRU *frame_unit = (SHM_FRAME_UNIT_STRU *)&p_shm[1];
                for(i = 0; i < p_shm->MaxFrameCnt; i++)
                {
                    pthread_mutex_init(&frame_unit[i].FrameMutex, &attr);
                }
            }
  
            return shmid;
        }
        
    }

    return shmid;
}
/*****************************************************************************
 
 
 *****************************************************************************/
char *AttachShareMem(int ShmId)
{
    char *shmptr = (char*)shmat(ShmId, 0, 0);
    if (shmptr==(void *)-1)
    {
        printf("Shmid:%d, err", ShmId);
        perror("shmat");
        return NULL;
    }
    else
    {
        SHM_FRAME_MNG_STRU *p_shm = (SHM_FRAME_MNG_STRU *)shmptr;
      
        printf("Shmid:%d, %p, FrameCnt:%d, BuffLen:%d", ShmId, shmptr, p_shm->MaxFrameCnt, p_shm->MaxFrameBuffLen);
    }
    return shmptr;
}
/*****************************************************************************
 
 
 *****************************************************************************/
void DetachShareMem(void *ShmPtr)
{
    if(ShmPtr)
    {
        shmdt(ShmPtr);
    }
}
/*****************************************************************************
 
 
 *****************************************************************************/
void ReleaseShareMem(int ShmId)
{
    shmctl(ShmId, IPC_RMID, 0);
}

/*****************************************************************************
 
 
 *****************************************************************************/
int ShmFrameIn(SHM_FRAME_MNG_STRU *ShmPtr, char *Buff, int Len)
{
    char *p_buff = ((char *)&ShmPtr[1])+ShmPtr->BuffOffset;
    unsigned int last_frame_idx = ShmPtr->LastFrameIdx % ShmPtr->MaxFrameCnt;

    SHM_FRAME_UNIT_STRU *p_shm_frame = (SHM_FRAME_UNIT_STRU *)&ShmPtr[1];   

    pthread_mutex_lock(&p_shm_frame[last_frame_idx].FrameMutex);
    if(1)
    {
        unsigned int iframe = 0;

        if(p_shm_frame[last_frame_idx].FrameLen)
        {
            if(p_shm_frame[last_frame_idx].FrameOffset > ShmPtr->MaxWriteOffset)
            {
                ShmPtr->CurUsedBuffLen -= ShmPtr->CurWritePadding;
                ShmPtr->CurWritePadding = 0;
            }
            ShmPtr->CurUsedBuffLen -= p_shm_frame[last_frame_idx].FrameLen;
        }
        
        if(ShmPtr->CurUsedBuffLen + Len > ShmPtr->MaxFrameBuffLen)
        {
            printf("WriteIdx:%d, FramIdx:%d, Len:%d, UsedSize:%d, MaxSize:%d"
                ,last_frame_idx, ShmPtr->LastFrameIdx, Len, ShmPtr->CurUsedBuffLen, ShmPtr->MaxFrameBuffLen);
        }

        memcpy(&p_buff[ShmPtr->CurWriteOffset], Buff, Len);
        if(Buff[4]==0x67)
        {
            iframe = 1;
        }
        
        p_shm_frame[last_frame_idx].FrameOffset = ShmPtr->CurWriteOffset;
        p_shm_frame[last_frame_idx].FrameFlag = iframe;
        p_shm_frame[last_frame_idx].FrameIdx = ShmPtr->LastFrameIdx;
        p_shm_frame[last_frame_idx].FrameLen = Len;
        p_shm_frame[last_frame_idx].FrameReadFlag = 0;

        ShmPtr->CurUsedBuffLen+=Len;
        ShmPtr->CurWriteOffset+=Len;

        //padding
        if(p_shm_frame[last_frame_idx].FrameOffset > ShmPtr->MaxWriteOffset)
        {
            ShmPtr->CurWritePadding = ShmPtr->MaxFrameBuffLen - ShmPtr->CurWriteOffset;
            
            ShmPtr->CurUsedBuffLen += ShmPtr->CurWritePadding;
            ShmPtr->CurWriteOffset = 0;
        }
        
        printf("idx:%7d(%2d), offset:%7d, nextwrite:%7d, len:%7d, total:%7d, padding:%7d, iframe:%d"
                 , p_shm_frame[last_frame_idx].FrameIdx
                 , last_frame_idx
                 , p_shm_frame[last_frame_idx].FrameOffset
                 , ShmPtr->CurWriteOffset
                 , p_shm_frame[last_frame_idx].FrameLen
                 , ShmPtr->CurUsedBuffLen
                 , ShmPtr->CurWritePadding
                 , iframe);
    }
    pthread_mutex_unlock(&p_shm_frame[last_frame_idx].FrameMutex);
    
    
    //update last frame idx
    ShmPtr->LastFrameIdx++;
    return 0;
}


/*****************************************************************************
 
    return recv len

    1. sync --> get read idx & frame idx
    2. frame read --> read idx & frame idx --> frame 

 
 *****************************************************************************/
int ShmFrameOut(SHM_FRAME_MNG_STRU *ShmPtr, unsigned int *ReadIdx, unsigned int *FrameIdx, char *Buff, int *BuffLen, unsigned int *FrameFlag)
{
    int ret = SHM_RET_PARAM_ERR;
    if(ShmPtr)
    //if(ShmPtr && ReadIdx && FrameIdx && Buff && BuffLen && FrameFlag)
    {
        unsigned int frame_idx = *FrameIdx;
        unsigned int lock_idx = (*ReadIdx)%ShmPtr->MaxFrameCnt;
        char *p_buff = ((char *)&ShmPtr[1])+ShmPtr->BuffOffset;
        SHM_FRAME_UNIT_STRU *p_shm_frame = (SHM_FRAME_UNIT_STRU *)&ShmPtr[1];   


        pthread_mutex_lock(&p_shm_frame[lock_idx].FrameMutex);
        //ShmLockUnit(ShmPtr, lock_idx);
        
        //p_shm_frame = &ShmPtr->FrameInfo[lock_idx];
        if(p_shm_frame[lock_idx].FrameLen>0)
        {
            if(*BuffLen>=p_shm_frame[lock_idx].FrameLen)
            {
                if(frame_idx==p_shm_frame[lock_idx].FrameIdx)
                {
                    *ReadIdx = lock_idx+1;
                    *FrameIdx = frame_idx+1;
                    *BuffLen = p_shm_frame[lock_idx].FrameLen;
                    *FrameFlag = p_shm_frame[lock_idx].FrameFlag;
                    memcpy(Buff, &p_buff[p_shm_frame[lock_idx].FrameOffset], p_shm_frame[lock_idx].FrameLen);
                    //printf("ReadIdx:%d, FrameIdx:%d, FrameLen:%d InQue(%d)", lock_idx, frame_idx, p_shm_frame[lock_idx].FrameLen, p_shm_frame[lock_idx].FrameIdx);
                    ret = SHM_RET_OK;
                }
                else if(frame_idx == p_shm_frame[lock_idx].FrameIdx + ShmPtr->MaxFrameCnt)
                {
                    //printf("no frame ReadIdx:%d, FrameIdx:%d, FrameLen:%u(%u)", lock_idx, frame_idx, p_shm_frame[lock_idx].FrameLen, *BuffLen);
                    ret = SHM_RET_NO_FRAME;
                }
                else
                {
                    printf("ReadIdx:%d, FrameIdx:%d, InQue(%d)", lock_idx, frame_idx, p_shm_frame[lock_idx].FrameIdx);
                    ret = SHM_RET_NOT_SYNC;
                }
            }
            else
            {
                printf("ReadIdx:%d, FrameIdx:%d, FrameLen:%u(%u)", lock_idx, frame_idx, p_shm_frame[lock_idx].FrameLen, *BuffLen);
                *BuffLen = p_shm_frame[lock_idx].FrameLen;
                ret = SHM_RET_BUFF_SIZE_ERR;
            }
        }
        else
        {
            //printf("no frame ReadIdx:%d, FrameIdx:%d, FrameLen:%u(%u)", lock_idx, frame_idx, p_shm_frame[lock_idx].FrameLen, *BuffLen);
            ret = SHM_RET_NO_FRAME;
        }

        //ShmUnlockUnit(ShmPtr, lock_idx);
        pthread_mutex_unlock(&p_shm_frame[lock_idx].FrameMutex);

    }

    return ret;
}

/*****************************************************************************

    return  
        0: wait
        1: sync ok
        -1: err 

 *****************************************************************************/
int ShmFrameSync(SHM_FRAME_MNG_STRU *ShmPtr, unsigned int *ReadIdx, unsigned int *FrameIdx)
{
    if(ShmPtr)
    {
        int i;
        unsigned int read_idx = 0;
        unsigned int frame_idx = 0;
        //char *p_buff = ((char *)&ShmPtr[1])+ShmPtr->BuffOffset;
        SHM_FRAME_UNIT_STRU *p_shm_frame = (SHM_FRAME_UNIT_STRU *)&ShmPtr[1];   


        pthread_mutex_lock(&p_shm_frame[0].FrameMutex);
        if(p_shm_frame[0].FrameLen)
        {
            frame_idx = p_shm_frame[0].FrameIdx;
        }
        else
        {
            pthread_mutex_unlock(&p_shm_frame[0].FrameMutex);
            return SHM_RET_NOT_SYNC;
        }
        pthread_mutex_unlock(&p_shm_frame[0].FrameMutex);



        for(i=1;i<ShmPtr->MaxFrameCnt;i++)
        {
            pthread_mutex_lock(&p_shm_frame[i].FrameMutex);

            if(p_shm_frame[i].FrameLen)
            {
                if(p_shm_frame[i].FrameIdx!=frame_idx+1)
                {
                    pthread_mutex_unlock(&p_shm_frame[i].FrameMutex);
                    break;
                }
                else
                {
                    frame_idx++;
                    read_idx++;
                }
            }
            else
            {
                pthread_mutex_unlock(&p_shm_frame[i].FrameMutex);
                break;
            }
            
            pthread_mutex_unlock(&p_shm_frame[i].FrameMutex);
        }

        printf("ReadIdx:%d, FrameIdx:%d, WriteFrameIdx:%d",read_idx, frame_idx, ShmPtr->LastFrameIdx);
        *FrameIdx = frame_idx;
        *ReadIdx = read_idx;
        return SHM_RET_OK;
    }

    return SHM_RET_PARAM_ERR;
}
