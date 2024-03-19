#ifndef __WM_PSRAM_H__
#define __WM_PSRAM_H__

#include "wm_hal.h"

#define PSRAM_ADDR_START          0x30000000

#define PSRAM	((PSRAM_TypeDef *)PSRAM_BASE)

#define __HAL_RCC_PSRAM_CLK_ENABLE() SET_BIT(RCC->CLK_EN, RCC_CLK_EN_QSRAM)

//           psram_ck   PB00    psram_ck   PA15	clk - air103
//           psram_cs   PB01    psram_cs   PB27	cs  - air103
//           psram_dat0 PB02    psram_dat0 PB02	mosi
//           psram_dat1 PB03    psram_dat1 PB03	miso
//           psram_dat2 PB04    psram_dat2 PB04	d2
//           psram_dat3 PB05    psram_dat3 PB05	d3

#define __HAL_AFIO_REMAP_PSRAM_CS(__HANDLE__, __IOPOSITION__) do{ \
                                            if ((__HANDLE__ == GPIOB) && (__IOPOSITION__ == GPIO_PIN_27))    \
                                            {                                                    \
                                                __AFIO_REMAP_SET_OPT1(__HANDLE__, __IOPOSITION__);    \
                                            }                                                    \
                                        }while (0)
#define __HAL_AFIO_REMAP_PSRAM_CLK(__HANDLE__, __IOPOSITION__) do{ \
                                            if ((__HANDLE__ == GPIOA) && (__IOPOSITION__ == GPIO_PIN_15))    \
                                            {                                                    \
                                                __AFIO_REMAP_SET_OPT1(__HANDLE__, __IOPOSITION__);    \
                                            }                                                    \
                                        }while (0)
#define __HAL_AFIO_REMAP_PSRAM_MISO(__HANDLE__, __IOPOSITION__) do{ \
                                            if ((__HANDLE__ == GPIOB) && (__IOPOSITION__ == GPIO_PIN_3))    \
                                            {                                                    \
                                                __AFIO_REMAP_SET_OPT4(__HANDLE__, __IOPOSITION__);    \
                                            }                                                    \
                                        }while (0)
#define __HAL_AFIO_REMAP_PSRAM_MOSI(__HANDLE__, __IOPOSITION__) do{ \
                                            if ((__HANDLE__ == GPIOB) && (__IOPOSITION__ == GPIO_PIN_2))    \
                                            {                                                    \
                                                __AFIO_REMAP_SET_OPT4(__HANDLE__, __IOPOSITION__);    \
                                            }                                                    \
                                        }while (0)
#define __HAL_AFIO_REMAP_PSRAM_D2(__HANDLE__, __IOPOSITION__) do{ \
                                            if ((__HANDLE__ == GPIOB) && (__IOPOSITION__ == GPIO_PIN_4))    \
                                            {                                                    \
                                                __AFIO_REMAP_SET_OPT4(__HANDLE__, __IOPOSITION__);    \
                                            }                                                    \
                                        }while (0)
#define __HAL_AFIO_REMAP_PSRAM_D3(__HANDLE__, __IOPOSITION__) do{ \
                                            if ((__HANDLE__ == GPIOB) && (__IOPOSITION__ == GPIO_PIN_5))    \
                                            {                                                    \
                                                __AFIO_REMAP_SET_OPT4(__HANDLE__, __IOPOSITION__);    \
                                            }                                                    \
                                        }while (0)

typedef struct
{
    uint32_t Div;
    uint32_t Mode;
} PSRAM_InitTypeDef;

typedef struct
{
    PSRAM_TypeDef       *Instance;
    PSRAM_InitTypeDef   Init;
} PSRAM_HandleTypeDef;

#define PSRAM_MODE_SPI      (PSRAM_CR_SPI_EN)
#define PSRAM_MODE_QSPI     (PSRAM_CR_QUAD_EN)

#define IS_PSRAM_INSTANCE(INSTANCE) ((INSTANCE) == PSRAM)
#define IS_PSRAM_DIV(DIV)           (((DIV) > 2) && ((DIV) <= 0x0F))
#define IS_PSRAM_MODE(MODE)         (((MODE) == PSRAM_MODE_SPI) || ((MODE == PSRAM_MODE_QSPI)))


HAL_StatusTypeDef HAL_PSRAM_Init(PSRAM_HandleTypeDef *hpsram);
HAL_StatusTypeDef HAL_PSRAM_DeInit(PSRAM_HandleTypeDef *hpsram);
void HAL_PSRAM_MspInit(PSRAM_HandleTypeDef *hpsram);
void HAL_PSRAM_MspDeInit(PSRAM_HandleTypeDef *hpsram);

#endif
