#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include "easy_fifo.h"
#include "fifo_mgr.h"

fifo_t *G_SPI_FIFO = NULL;
static int G_ready = 0;
u8 sPkgBuffer[DEF_FIXED_UNIT_SIZE] = {0};

/* dispatch frame , with ring buffer full handling  */
int dispatch_and_copy_one_pkg(pkg_info_t *bs_info, u8 *header_info)
{
	int ret;
  
	ret = fifo_write_one_packet(G_SPI_FIFO, header_info, bs_info->pkg_addr, bs_info->pkg_size);
  if(ret == 0 ){
    G_ready = 1;
  }
  
	return ret;
}
int bsreader_get_one_pkg(pkg_info_t * info, u8 *header_info)
{
	if (!G_ready) {
		return -1;
	}

	return fifo_read_one_packet(G_SPI_FIFO, header_info, &info->pkg_addr, &info->pkg_size);
}

int dispatch_and_copy_one_frame(u8 *pBuffer, u32 buffer_size, u8* pHeader)
{
  pkg_info_t tPgkInfo;
  
  tPgkInfo.pkg_addr = pBuffer;
  tPgkInfo.pkg_size = buffer_size;

  return dispatch_and_copy_one_pkg(&tPgkInfo, pHeader);

}
void fifo_mgr_flush(void)
{
  fifo_flush(G_SPI_FIFO);
}

int fifo_mgr_init(int buffer_size, int header_size, int buffer_num)
{
  G_SPI_FIFO = fifo_create(buffer_size, header_size, buffer_num);
  if(G_SPI_FIFO == NULL){
    return -1;
  }
  
  return 0;
}

void fifo_mgr_uninit(void)
{
  fifo_close(G_SPI_FIFO);
}

