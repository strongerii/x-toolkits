#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "shmmap.h"
#include "sh_tbitstream.h"

shm_t *creat_shm_t(u32 ukey)
{
	int shmid = -1;
	shm_t *pshm_t = NULL;
	void *shmaddr = NULL;

	if((shmid = shmget((key_t)ukey, sizeof(shm_t), 0666 | IPC_CREAT)) < 0){
		printf("creat share map [%d] failed!\n",ukey);
		return NULL;
	}
	if((shmaddr = shmat(shmid, NULL, 0)) == (void *)-1){
		printf("shmat share map [%d] failed \n",ukey);
		return NULL;
	}
	pshm_t = (shm_t *)shmaddr;

	if(tbsb_init(pshm_t) < 0){
		printf("tbsb_init failed !\n");
		return NULL;
	}

	return pshm_t;
}

shm_t *capture_shm_t(u32 ukey)
{
	int shmid = -1;
	shm_t *pshm_t = NULL;
	void *shmaddr = NULL;

	if((shmid = shmget((key_t)ukey,0,0)) < 0){
		perror("capture pshm_t Fail to shmget\n");
		return NULL;
	}
	
	//map the shared memory to current process
	if((shmaddr = shmat(shmid,NULL,0)) == (void *)-1){
		perror("capture pshm_t Fail to shmat\n");
		return NULL;
	}
	pshm_t = (shm_t *)shmaddr;
	return pshm_t;
}

s32 release_shm_t(shm_t *pbsb)
{
	 void *shmaddr = NULL;
	 if(NULL == pbsb){
	 	perror("release_shm_vbsb NULL==pbsb\n");
		return -1;
	 }
	shmaddr = (void *)pbsb;
	if(shmdt(shmaddr) < 0)
	{
		perror("release_shm_vbsb Fail to shmdt\n");
		return -1;
	}
	return 0;

}

s32 destroy_shm_t(u32 ukey,shm_t *pbsb)
{
	 int shmid = -1;
	 void *shmaddr = NULL;
	 if(NULL == pbsb){
	 	perror("destroy_shm_vbsb NULL==pbsb\n");
		return -1;
	 }
	 
	 if(pbsb){
		tbsb_uninit(pbsb);
	 }
	
	//unmount
	shmaddr = (void *)pbsb;
	if(shmdt(shmaddr) < 0)
	{
		perror("destroy_shm_vbsb Fail to shmdt\n");
		return -1;
	}
	
	 //get shared memory that already be created
	if((shmid = shmget((key_t)ukey,0,0)) < 0)
	{
		perror("destroy_shm_vbsb Fail to shmget\n");
		return -1;
	}
	//delete it
	if(shmctl(shmid,IPC_RMID,NULL) < 0)
	{
		perror("destroy_shm_vbsb Fail to shmctl\n");
		return -1;
	}

	return 0;

}
