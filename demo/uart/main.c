
#include <stdio.h>
#include <string.h>
#include "wm_hal.h"
#include "fifo.h"

UART_HandleTypeDef huart1;

static void UART1_Init(void);
void Error_Handler(void);

#define IT_LEN 0     // Greater than or equal to 0, 0: receiving data of indefinite length can trigger the interrupt callback; greater than 0: receiving N length data will trigger the interrupt callback
static uint8_t buf[32] = {0}; // Must be greater than or equal to 32 bytes
#define LEN 2048
static uint8_t pdata[LEN] = {0};

int main(void)
{
    volatile int tx_len = 0;
    uint8_t tx_buf[100] = {0};
    
    SystemClock_Config(CPU_CLK_160M);
    printf("enter main\r\n");
    
    UART1_Init();
    FifoInit(pdata, LEN);
    HAL_UART_Receive_IT(&huart1, buf, IT_LEN);  // It only needs to be called once. When receiving the set length,
                                                // it will enter the interrupt callback. The user needs to take the data
                                                // in the interrupt callback. 0 bytes are set here, that is, the length is indefinite.
    while(1)
    {
        tx_len = FifoDataLen();
        if (tx_len > 0)
        {
            tx_len = (tx_len > 100) ? 100 : tx_len;
            FifoRead(tx_buf, tx_len);
            HAL_UART_Transmit(&huart1, tx_buf, tx_len, 1000);
        }
    }
    
    return 0;
}

static void UART1_Init(void)
{
    huart1.Instance = UART1;
    huart1.Init.BaudRate = 115200;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX | UART_MODE_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    if (HAL_UART_Init(&huart1) != HAL_OK)
    {
        Error_Handler();
    }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (FifoSpaceLen() >= huart->RxXferCount)
    {
        FifoWrite(huart->pRxBuffPtr, huart->RxXferCount);
    }
}

void Error_Handler(void)
{
    while (1)
    {
    }
}

void assert_failed(uint8_t *file, uint32_t line)
{
    printf("Wrong parameters value: file %s on line %d\r\n", file, line);
}