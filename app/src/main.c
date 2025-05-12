/******************************************************************************
** 
 * \file        main.c
 * \author      IOsetting | iosetting@outlook.com
 * \date        
 * \brief       Demo code of PSRAM read/write
 * \note        
 * \version     v0.1
 * \ingroup     demo
 * \remarks     test-board: Air103
 *
******************************************************************************/

#include <stdio.h>
#include <wm_hal.h>
#include "wm_psram.h"
#include "psalloc.h"
#include "fifo.h"

PMU_HandleTypeDef hpmu;
// имя не может быть другим для данного UART
UART_HandleTypeDef huart1;
#define IT_LEN 0
static uint8_t buf[32] = {0};
#define LEN 2048
static uint8_t pdata[LEN] = {0};
uint32_t ticks_tm0_beg, ticks_tm0_end;


void HAL_DMA_MspInit(DMA_HandleTypeDef *hdma);

#ifndef USE_PSRAM
#warning PSRAM is not enabled! Please use USE_PSRAM=1 define
#endif

int i, j;

void Error_Handler(void);
// flash-linked const
const char _fish[]  __attribute__ ((section(".psram.goo"))) = "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.";
char _psbuf[1024] __attribute__ ((section(".bss")));
DMA_HandleTypeDef hdma_ram_tx;

void HAL_PMU_RTC_Callback(PMU_HandleTypeDef *hpmu)
{
    // 1s elapsed! Get tim0 counter!
    ticks_tm0_end = TIM->TIM0_CNT;
    printf("\n2s timer0 ticks elapsed: %d\n", ticks_tm0_end-ticks_tm0_beg);
}
/*
static void DMA_Init(void)
{
    __HAL_RCC_DMA_CLK_ENABLE();

    HAL_NVIC_SetPriority(DMA_Channel0_IRQn, 0);
    HAL_NVIC_EnableIRQ(DMA_Channel0_IRQn);

    HAL_NVIC_SetPriority(DMA_Channel1_IRQn, 0);
    HAL_NVIC_EnableIRQ(DMA_Channel1_IRQn);
}
*/

/*
 * Иногда двойная звездочка, значит, FifoSpaceLen() не хватает для текущей порции данных. 
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (FifoSpaceLen() >= huart->RxXferCount)
    {
        FifoWrite(huart->pRxBuffPtr, huart->RxXferCount);
    }
    else printf("_");
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
    WRITE_REG(huart1.Instance->FIFOC, 0x00);    // no FIFO!
}

void heapdump(void)
{
    _HEAPINFO hinfo;
    int heapstatus;
    int numLoops;
    hinfo._pentry = NULL;
    numLoops = 0;
    while((heapstatus = _heapwalk(&hinfo)) == _HEAPOK &&
          numLoops < 100)
    {
        printf("%8s block at %p of size %4.4X\n",
               (hinfo._useflag == _USEDENTRY ? "USED" : "FREE"),
               hinfo._pentry, hinfo._size);	/* _pentry must contains an actual PTR */
        numLoops++;
    }
    if(heapstatus == _HEAPEND) {
        printf("    FINAL block at %p of size %4.4X\n",
               hinfo._pentry, hinfo._size);
    }
    else if(heapstatus == _HEAPEMPTY) {
        printf("    FINAL block at %p of size %4.4X\n",
               hinfo._pentry, hinfo._size);
    }
    else if(heapstatus == _HEAPBADNODE) printf("_HEAPBADNODE\n");
    else if(heapstatus == _HEAPBADBEGIN) printf("_HEAPBADBEGIN\n");
    // _HEAPBADPTR - The _pentry field of the _HEAPINFO structure doesn't contain a valid pointer into the heap or entryinfo is a null pointer.
}
int parse_string(char *membuf)
{
    float a,b,c;
    int32_t ai;
    sscanf(membuf, "%f;%f;%f", &a, &b, &c);
    ai = a * 1000;
    printf("%d\t%f\t%f\n", ai, b, c);
    return 0;
}
int main(void)
{
    TIM_HandleTypeDef my_tim;
    RTC_TimeTypeDef rtc_time;
    uint32_t ticks0, ticks1, ticks2, ticks3, ticks4;
    HAL_StatusTypeDef stat;
    char *membuf1, *membuf2;
    char *mempos;
    volatile int tx_len = 0;
    uint8_t rx_buf[200] = {0};

    SystemClock_Config(CPU_CLK_240M);
    printf("enter main\r\n");
    UART1_Init();
    memset(_psbuf, -1, 1024);

    my_tim.Instance = TIM0;
    my_tim.Init.Unit = TIM_UNIT_US;
    my_tim.Init.AutoReload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    // APB всегда 40М
    // период - это то что загружается в счетный регистр. А в предделителе 39 по дефолту!
    my_tim.Init.Period = 4000*1000; // считать до 4М и снова с 0. Переполнение раз в 4 сек.

    __HAL_RCC_TIM_CLK_ENABLE();	// enable timer clocking!
    HAL_TIM_Base_Init(&my_tim); //HAL_TIM_Base_Init
    
    // GPIO Init A1
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    __HAL_RCC_GPIO_CLK_ENABLE();

    GPIO_InitStruct.Pin = GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);
    
    
    ticks_tm0_beg = TIM->TIM0_CNT;
    HAL_Delay(1000);
    ticks_tm0_end = TIM->TIM0_CNT;
    printf("HAL_Delay(1000) ticks elapsed: %d\n", ticks_tm0_end-ticks_tm0_beg);
    
    // init RTC
	hpmu.Instance = PMU;
    hpmu.ClkSource = PMU_CLKSOURCE_32RC;
    HAL_PMU_Init(&hpmu);
    rtc_time.Year = 125;
    rtc_time.Month = 5;
    rtc_time.Date = 10;
    rtc_time.Hours = 14;
    rtc_time.Minutes = 28;
    rtc_time.Seconds = 10;
    // Calibration!
    uint32_t c32 = hpmu.Instance->CR;
    printf("c32=%x\n", c32);
    CLEAR_BIT(hpmu.Instance->CR, 8);
    SET_BIT(hpmu.Instance->CR, 8);
    //hpmu.Instance->CR = 22;
    
    HAL_PMU_RTC_Start(&hpmu, &rtc_time);
    
    rtc_time.Seconds = 12;  // current + 10 sec (?)
    ticks_tm0_beg = TIM->TIM0_CNT;
    HAL_PMU_RTC_Alarm_Enable(&hpmu, &rtc_time); // callback after 3 sec
      

    /*mem2mem*/
    membuf1 = malloc(0x10000);
    membuf2 = malloc(0x10000);
    if(membuf1 == NULL || membuf2 == NULL){
        printf("malloc error!\n");
        while(1);
    }
    memset(membuf2, 'A', 0x10000);
    HAL_TIM_Base_Start(&my_tim);	// start counter (memset & memcpy)
    ticks0 = TIM->TIM0_CNT;
    memcpy(membuf1, membuf2, 0x10000); // 'A' to membuf1
    ticks1 = TIM->TIM0_CNT;

    printf("mem2mem @64k is %dus\n", ticks1-ticks0);
    printf("Calculated value is %3.3f MB/s\n", 1000000.0/((ticks1-ticks0)*16)); // 64-128-256-512-1024

    char *psblock = psalloc(0x10000);
    if(psblock == NULL){		// how to detect PSRAM presence?
       printf("psalloc error!\n");
       while(1);
    }
    heapdump(); // show current free size
    //memset(psblock, 'R', 0x10000);
    ticks0 = TIM->TIM0_CNT;
    memcpy(psblock, membuf1, 0x10000);	// 'A' to psblock
    ticks1 = TIM->TIM0_CNT;

    printf("mem2psram @64k is %dus\n", ticks1-ticks0);
    printf("Calculated value is %3.3f MB/s\n", 1000000.0/((ticks1-ticks0)*16));

    HAL_DMA_MspInit(&hdma_ram_tx);
    memset(membuf1, '5', 0x10000);
    ticks0 = TIM->TIM0_CNT;
    HAL_DMA_Start(&hdma_ram_tx, (uint32_t)membuf1, (uint32_t)psblock, 0x8000); // 32 kB '5' to psblock
    ticks1 = TIM->TIM0_CNT;
    stat = HAL_DMA_PollForTransfer(&hdma_ram_tx, HAL_DMA_FULL_TRANSFER, 2000);
    if(stat != HAL_OK) {
        printf("DMA Error #%d, %d\n", stat, hdma_ram_tx.ErrorCode);
        Error_Handler();
    }
    ticks2 = TIM->TIM0_CNT;
    HAL_DMA_Start(&hdma_ram_tx, (uint32_t)membuf1+0x8000, (uint32_t)psblock+0x8000, 0x8000); // Next 32kB to psblock
    ticks3 = TIM->TIM0_CNT;
    stat = HAL_DMA_PollForTransfer(&hdma_ram_tx, HAL_DMA_FULL_TRANSFER, 1000);
    if(stat!= HAL_OK){
        printf("DMA Error #%d, %d\n", stat, hdma_ram_tx.ErrorCode);
        Error_Handler();
    }
    ticks4 = TIM->TIM0_CNT;
    printf("DMA Transfer OK in %d (%u+%u+%u+%u) us!\n", ticks4-ticks0, ticks1-ticks0, ticks2-ticks1, ticks3-ticks2, ticks4-ticks3);
    printf("Last byte: %x\n", psblock[0xFFFF]);
    printf("Mid byte: %x\n", psblock[0x7FFF]);

    char **q = malloc(64 * sizeof(char *));		// array of 64 char * in regular RAM

    for(i=0; i<5; i++){
        q[i] = (char *)psalloc(strlen(_fish)+1);
        if(!q){
            printf("psalloc error!\n");
            continue;
        }
        else{
            strcpy(q[i], _fish);
            heapdump();
            printf("q[%d]=[%s]\n", i, q[i]);
        }
        HAL_Delay(1000);        // 1s delay
        HAL_PMU_RTC_GetTime(&hpmu, &rtc_time);
        printf("%d-%d-%d %d:%d:%d\r\n", (rtc_time.Year + 1900), rtc_time.Month, rtc_time.Date, rtc_time.Hours, rtc_time.Minutes, rtc_time.Seconds);
    }
    for(i=0;i<5;i++) psfree(q[i]);

    heapdump();
    printf("Freeing big block...\n");
    psfree(psblock);
    printf("Now heap MUST be totally empty!\n");
    heapdump();
    free(q);
    free(membuf1);
    free(membuf2);
    
    FifoInit(pdata, LEN);
    HAL_UART_Receive_IT(&huart1, buf, IT_LEN);  // It only needs to be called once. When receiving the set length,

    membuf1 = malloc(256);  // buffer for input string
    mempos = membuf1;
    while(1){	// loop forewer
        tx_len = FifoDataLen();
        if (tx_len > 0)
        {
            tx_len = (tx_len > 100) ? 100 : tx_len;
            // приходят порции по 16 байт (аппаратный FIFO?) и "хвостик", если есть пауза между строками.
            FifoRead(rx_buf, tx_len);
            rx_buf[tx_len] = 0;
            if(rx_buf[tx_len-1] == '\n'){ // line end detected
                strcpy(mempos, (const char *)rx_buf);
                parse_string(membuf1);
                mempos = membuf1;       // clear buf
            }
            else{
                strcpy(mempos, (const char *)rx_buf);
                mempos += tx_len;       // move pointer to end of line
            }
            //HAL_UART_Transmit(&huart1, rx_buf, tx_len, 1000);
            //printf("{%s}\n", rx_buf);
        }
    }
}

void assert_failed(uint8_t *file, uint32_t line)
{
    printf("Wrong parameters value: file %s on line %d\r\n", file, line);
}
