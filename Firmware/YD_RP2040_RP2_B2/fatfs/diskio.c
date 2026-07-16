/*-----------------------------------------------------------------------*/
/* diskio.c – FatFs glue layer cho driver SD card (components/sdcard)    */
/* Kết nối 5 hàm FatFs yêu cầu với SDCARD_Init/ReadBlock/WriteBlock...  */
/*-----------------------------------------------------------------------*/
#include "fatfs/ff.h"
#include "fatfs/diskio.h"
#include "components/sdcard/sdcard.h"
#include "components/xpt2046/xpt2046.h"  // can de deselect chan CS cua XPT2046 (dung chung bus SPI0)
#include "hardware/gpio.h"
#include <stdio.h>

/* Chỉ dùng 1 ổ đĩa vật lý duy nhất (pdrv = 0 = thẻ SD) */
static sd_dev_t s_sd;
static bool     s_ready = false;

/*-----------------------------------------------------------------------*/
/* Khởi tạo ổ đĩa – FatFs gọi hàm này 1 lần trước khi mount            */
/*-----------------------------------------------------------------------*/
DSTATUS disk_initialize(BYTE pdrv)
{
    if (pdrv != 0) return STA_NOINIT;

    /* Cấu hình GPIO_FUNC_SPI cho bus SPI0 (dùng chung với XPT2046).
       Gọi spi_init() ở đây để đảm bảo bus ở trạng thái đúng ngay cả khi
       XPT2046_Init() chưa được gọi trước đó (an toàn khi gọi nhiều lần). */
    gpio_set_function(SDCARD_PIN_MISO, GPIO_FUNC_SPI);
    gpio_set_function(SDCARD_PIN_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(SDCARD_PIN_SCK,  GPIO_FUNC_SPI);
    spi_init(SDCARD_SPI_PORT, SDCARD_BAUD_INIT);

    /* QUAN TRONG: XPT2046 (cam ung) dung CHUNG bus SPI0 (MISO/MOSI/SCK) voi
       the SD, chi khac chan CS rieng (GPIO17). Neu chua co task nao goi
       XPT2046_Init() (vd build hien tai chi test rieng SD/GPS/BMI160), chan
       CS nay bi THA NOI -> IC XPT2046 co the ngau nhien tuong minh dang duoc
       chon va tu y lai tin hieu ra MISO cung luc voi the SD, gay xung dot bus
       (loi doc du lieu SAI NGAU NHIEN, luc duoc luc khong - dung trieu chung
       da gap: CMD8/CMD16 luc pass luc fail). Ep CS cua XPT2046 len muc cao
       (deselect) ngay tai day de dam bao no khong bao gio choi vao bus. */
    gpio_init(XPT2046_PIN_CS);
    gpio_set_dir(XPT2046_PIN_CS, GPIO_OUT);
    gpio_put(XPT2046_PIN_CS, 1);

    SD_Status st = SDCARD_Init(&s_sd, SDCARD_SPI_PORT);
    s_ready = (st == SD_OK);

    if (!s_ready) {
        printf("[diskio] LOI: SDCARD_Init that bai (ma=%d)\n", st);
    }
    return s_ready ? 0 : STA_NOINIT;
}

/*-----------------------------------------------------------------------*/
/* Trả về trạng thái ổ đĩa                                              */
/*-----------------------------------------------------------------------*/
DSTATUS disk_status(BYTE pdrv)
{
    if (pdrv != 0) return STA_NOINIT;
    return s_ready ? 0 : STA_NOINIT;
}

/*-----------------------------------------------------------------------*/
/* Đọc sector(s)                                                         */
/*-----------------------------------------------------------------------*/
DRESULT disk_read(BYTE pdrv, BYTE *buff, LBA_t sector, UINT count)
{
    if (pdrv != 0 || !s_ready) return RES_NOTRDY;

    for (UINT i = 0; i < count; i++) {
        if (SDCARD_ReadBlock(&s_sd,
                             (uint32_t)(sector + i),
                             buff + (i * SDCARD_BLOCK_SIZE)) != SD_OK) {
            return RES_ERROR;
        }
    }
    return RES_OK;
}

/*-----------------------------------------------------------------------*/
/* Ghi sector(s)                                                         */
/*-----------------------------------------------------------------------*/
#if FF_FS_READONLY == 0
DRESULT disk_write(BYTE pdrv, const BYTE *buff, LBA_t sector, UINT count)
{
    if (pdrv != 0 || !s_ready) return RES_NOTRDY;

    for (UINT i = 0; i < count; i++) {
        if (SDCARD_WriteBlock(&s_sd,
                              (uint32_t)(sector + i),
                              buff + (i * SDCARD_BLOCK_SIZE)) != SD_OK) {
            return RES_ERROR;
        }
    }
    return RES_OK;
}
#endif

/*-----------------------------------------------------------------------*/
/* Lệnh điều khiển ổ đĩa (FatFs yêu cầu ít nhất CTRL_SYNC,             */
/* GET_SECTOR_COUNT, GET_SECTOR_SIZE, GET_BLOCK_SIZE khi dùng f_mkfs)   */
/*-----------------------------------------------------------------------*/
DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buff)
{
    if (pdrv != 0 || !s_ready) return RES_NOTRDY;

    switch (cmd) {

    case CTRL_SYNC:
        /* Driver ghi đồng bộ ngay lập tức, không có write-cache */
        return RES_OK;

    case GET_SECTOR_COUNT: {
        uint64_t total_bytes = 0;
        if (SDCARD_GetCapacityBytes(&s_sd, &total_bytes) != SD_OK)
            return RES_ERROR;
        *(LBA_t *)buff = (LBA_t)(total_bytes / SDCARD_BLOCK_SIZE);
        return RES_OK;
    }

    case GET_SECTOR_SIZE:
        *(WORD *)buff = (WORD)SDCARD_BLOCK_SIZE;
        return RES_OK;

    case GET_BLOCK_SIZE:
        /* Không biết chính xác erase-block size của thẻ → trả về 1
           (FatFs dùng giá trị mặc định khi f_mkfs() căn chỉnh cluster) */
        *(DWORD *)buff = 1;
        return RES_OK;

    default:
        return RES_PARERR;
    }
}