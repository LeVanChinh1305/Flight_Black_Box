#include "LoRa_SX1278.h"
#include <msp430.h>

/* ===========================================================================
 * ĐỊNH NGHĨA CHÂN ĐIỀU KHIỂN (GPIO thường, không qua USCI)
 * =========================================================================== */

/* P2.2 — NSS/CS (label "SPI CS Wireless" trên BoosterPack pinout) */
#define LORA_CS_PORT_DIR    P2DIR
#define LORA_CS_PORT_OUT    P2OUT
#define LORA_CS_PIN         BIT2

/* P7.4 — RST */
#define LORA_RST_PORT_DIR   P7DIR
#define LORA_RST_PORT_OUT   P7OUT
#define LORA_RST_PIN        BIT4

/* P2.0 — DIO0 (input, đọc TxDone) */
#define LORA_DIO0_PORT_DIR  P2DIR
#define LORA_DIO0_PORT_IN   P2IN
#define LORA_DIO0_PIN       BIT0

/* ===========================================================================
 * THANH GHI SX1278 (chỉ các thanh ghi cần dùng)
 * =========================================================================== */

#define REG_FIFO              0x00
#define REG_OP_MODE            0x01
#define REG_FRF_MSB             0x06
#define REG_FRF_MID              0x07
#define REG_FRF_LSB                0x08
#define REG_PA_CONFIG          0x09
#define REG_FIFO_ADDR_PTR      0x0D
#define REG_FIFO_TX_BASE_ADDR  0x0E
#define REG_IRQ_FLAGS          0x12
#define REG_MODEM_CONFIG_1     0x1D
#define REG_MODEM_CONFIG_2     0x1E
#define REG_MODEM_CONFIG_3     0x26
#define REG_PAYLOAD_LENGTH     0x22
#define REG_VERSION            0x42

#define MODE_LONG_RANGE_MODE   0x80
#define MODE_SLEEP              0x00
#define MODE_STDBY               0x01
#define MODE_TX                   0x03

#define IRQ_TX_DONE_MASK        0x08

/* ===========================================================================
 * HÀM NỘI BỘ — ĐIỀU KHIỂN CS
 * =========================================================================== */

static void cs_select(void)
{
    LORA_CS_PORT_OUT &= ~LORA_CS_PIN;   /* CS = 0 (active low) */
}

static void cs_deselect(void)
{
    LORA_CS_PORT_OUT |= LORA_CS_PIN;    /* CS = 1 */
}

/**
 * @brief  Truyền 1 byte qua SPI (USCI_B0) và nhận lại 1 byte (full-duplex).
 *         Blocking — chờ TX buffer trống và RX buffer đầy.
 */
static uint8_t spi_transfer(uint8_t data)
{
    while (!(UCB0IFG & UCTXIFG)) { /* chờ TX buffer sẵn sàng */ }
    UCB0TXBUF = data;
    while (!(UCB0IFG & UCRXIFG)) { /* chờ nhận xong */ }
    return UCB0RXBUF;
}

/* ===========================================================================
 * HÀM NỘI BỘ — GHI/ĐỌC THANH GHI SX1278
 * =========================================================================== */

static void lora_write_reg(uint8_t addr, uint8_t value)
{
    cs_select();
    spi_transfer(addr | 0x80);   /* bit 7 = 1 → write */
    spi_transfer(value);
    cs_deselect();
}

static uint8_t lora_read_reg(uint8_t addr)
{
    uint8_t value;
    cs_select();
    spi_transfer(addr & 0x7F);   /* bit 7 = 0 → read */
    value = spi_transfer(0x00);
    cs_deselect();
    return value;
}

/**
 * @brief  Delay đơn giản dựa trên vòng lặp — dùng cho reset timing.
 *         Với SMCLK ~1MHz, mỗi vòng lặp ước lượng vài us; đủ cho delay ms-order
 *         trong giai đoạn init (không cần chính xác cao).
 */
static void simple_delay_ms(uint16_t ms)
{
    volatile uint32_t i;
    for (i = 0; i < (uint32_t)ms * 1000; i++) {
        __no_operation();
    }
}

/* ===========================================================================
 * API CÔNG KHAI
 * =========================================================================== */

int LoRa_SX1278_Init(void)
{
    /* --- Cấu hình GPIO điều khiển (CS, RST) là output --- */
    LORA_CS_PORT_DIR  |= LORA_CS_PIN;
    LORA_RST_PORT_DIR |= LORA_RST_PIN;
    cs_deselect();

    /* --- DIO0 là input --- */
    LORA_DIO0_PORT_DIR &= ~LORA_DIO0_PIN;

    /* --- Cấu hình SPI (USCI_B0) — chân theo BoosterPack: P3.0/P3.1/P2.6 --- */
    P3SEL |= BIT0 | BIT1;       /* P3.0 = UCB0SIMO, P3.1 = UCB0SOMI */
    P2SEL |= BIT6;               /* P2.6 = UCB0SCL (SPI clock) */

    UCB0CTL1 |= UCSWRST;                                  /* Reset USCI_B0 trước khi cấu hình */
    UCB0CTL0  = UCCKPH | UCMSB | UCMST | UCSYNC;          /* SPI Mode 0, MSB first, Master, đồng bộ */
    UCB0CTL1 |= UCSSEL__SMCLK;                             /* Clock nguồn: SMCLK */
    UCB0BR0   = 8;                                          /* SMCLK(1MHz) / 8 = 125kHz SPI clock (an toàn cho SX1278) */
    UCB0BR1   = 0;
    UCB0CTL1 &= ~UCSWRST;                                   /* Thoát reset, USCI_B0 bắt đầu hoạt động */

    /* --- Reset phần cứng SX1278 --- */
    LORA_RST_PORT_OUT &= ~LORA_RST_PIN;   /* RST = 0 */
    simple_delay_ms(10);
    LORA_RST_PORT_OUT |= LORA_RST_PIN;    /* RST = 1 */
    simple_delay_ms(10);

    /* --- Kiểm tra giao tiếp: đọc thanh ghi Version, SX1278 trả về 0x12 --- */
    if (lora_read_reg(REG_VERSION) != 0x12) {
        return -1;   /* Không phản hồi đúng — kiểm tra lại dây SPI */
    }

    /* --- Chuyển sang Sleep + LoRa mode để cấu hình các thanh ghi khác --- */
    lora_write_reg(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_SLEEP);
    simple_delay_ms(10);

    /* --- Tần số 433 MHz: Frf = 433e6 / (32e6/2^19) = 7208960 = 0x6C8000 --- */
    lora_write_reg(REG_FRF_MSB, 0x6C);
    lora_write_reg(REG_FRF_MID, 0x80);
    lora_write_reg(REG_FRF_LSB, 0x00);

    /* --- Công suất phát: PA_BOOST, mức tối đa --- */
    lora_write_reg(REG_PA_CONFIG, 0x8F);

    /* --- Modem config: BW=125kHz, CR=4/5, Explicit header --- */
    lora_write_reg(REG_MODEM_CONFIG_1, 0x72);
    /* --- SF=7, CRC on --- */
    lora_write_reg(REG_MODEM_CONFIG_2, 0x74);
    /* --- LNA gain tự động --- */
    lora_write_reg(REG_MODEM_CONFIG_3, 0x04);

    /* --- Payload length cố định = FRAME_SIZE (42 byte) --- */
    lora_write_reg(REG_PAYLOAD_LENGTH, (uint8_t)FRAME_SIZE);

    /* --- Trở về Standby mode, sẵn sàng gửi --- */
    lora_write_reg(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_STDBY);
    simple_delay_ms(10);

    return 0;
}

int LoRa_SX1278_Send(const UartFrame_t *frame)
{
    const uint8_t *data = (const uint8_t *)frame;
    uint16_t       i;
    uint16_t       timeout = 2000;   /* ~2000 vòng lặp, tương đương vài trăm ms */

    /* --- Trỏ FIFO về địa chỉ TX base, vào Standby --- */
    lora_write_reg(REG_FIFO_ADDR_PTR, 0x80);   /* TxBaseAddr mặc định = 0x80 */
    lora_write_reg(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_STDBY);

    /* --- Ghi từng byte của frame vào FIFO --- */
    for (i = 0; i < FRAME_SIZE; i++) {
        lora_write_reg(REG_FIFO, data[i]);
    }
    lora_write_reg(REG_PAYLOAD_LENGTH, (uint8_t)FRAME_SIZE);

    /* --- Bắt đầu truyền --- */
    lora_write_reg(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_TX);

    /* --- Chờ TxDone (đọc cờ IRQ, hoặc có thể đọc trực tiếp chân DIO0) --- */
    while (timeout--) {
        uint8_t irq = lora_read_reg(REG_IRQ_FLAGS);
        if (irq & IRQ_TX_DONE_MASK) {
            lora_write_reg(REG_IRQ_FLAGS, IRQ_TX_DONE_MASK);  /* Clear cờ */
            lora_write_reg(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_STDBY);
            return 0;
        }
        simple_delay_ms(1);
    }

    /* Timeout — trở về Standby để tránh kẹt ở TX mode */
    lora_write_reg(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_STDBY);
    return -1;
}