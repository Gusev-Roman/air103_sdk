#include <wm_type_def.h>
#include <wm_regs.h>
#include <wm_hal.h>

extern float *bigbuf;
extern TIM_HandleTypeDef my_tim;

int parse_string(char *membuf)
{
    float a,b,c,d,e,f,g,h,i,j,k;
    int32_t ai=0;
    int diff, num, nrow;
    static bool _debug = false;
    static bool _loaded = false;
    static uint8_t *for_dac = NULL;

    
    if(_debug) printf("%s", membuf);
    
    if(membuf[0] == '['){
        memcmp_s(membuf, 7, "[begin:", 7, &diff);
        if(diff == 0){
            num = atoi(membuf+7);
            printf("Loading:[waveform #%d]\n", num);
            _loaded = false;
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_24, GPIO_PIN_RESET);
        }
        memcmp_s(membuf, 5, "[row:", 5, &diff);
        if(diff == 0){
            nrow = atoi(membuf+5);
            printf("Selected row #%d; downsampling to RAM...\n", nrow);
            if(!_loaded){
                printf("Error: \n");
                return -1;
            }
            else{
                if(for_dac == NULL) for_dac = malloc(30000); // 8 bit per sample
            }
            if(nrow > -1 && nrow < 10){
                // print selected row
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_25, GPIO_PIN_RESET);
                for(int ii=0; ii<30000; ii++){
                    for_dac[ii] = 0.1+((6.0 + bigbuf[ii*10+nrow])/0.046875);
                    //printf("%d:%f\n", ii,bigbuf[ii*10+nrow]);
                }
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_25, GPIO_PIN_SET);
            }
        }
        // теперь, когда в память загружены все треки, можно сделать play() на внешний DAC. Предварительно придется 
        // сделать downscale до 8 (14) бит.
        // выставить данные, ждать изменения таймера, кликнуть строб, небольшая пауза, кликнуть обратно.
        // повторять пока не закончатся данные. 
        memcmp_s(membuf,7,"[start:",7, &diff);
        if(diff == 0){
            uint32_t clk;
            int y = atoi(membuf+7);
            printf("Playing waveform at %d us/sample\n", y);
            HAL_TIM_Base_Stop(&my_tim);
            //MODIFY_REG(GPIOB->DATA, 0, (0xFF << 6)); // маска нулей, маска единиц
            // не трогаем data_en, нули запишутся во все невыбранные биты
            WRITE_REG(GPIOB->DATA, (0xFF << 6));
            HAL_TIM_Base_Start(&my_tim);
            clk = TIM->TIM0_CNT;
            while(TIM->TIM0_CNT - clk < y) __NOP;

            printf("TIM0:%x\n", clk);
            HAL_Delay(1000);
            printf("ending TIM0:%x\n", TIM->TIM0_CNT);
        }
    }

    else{
        sscanf(membuf, "%f;%f;%f;%f;%f;%f;%f;%f;%f;%f;%f", &a, &b, &c, &d, &e, &f, &g, &h, &i, &j, &k);
        ai = (0.1 + a * 1000.0);
        if(ai % 25 == 0) HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_24);    // blink every 50 records
        //if(ai == 250) _debug = true;
        //if(ai == 254) _debug = false;
        if(_debug) printf("%d:\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\n", ai, b, c,d,e,f,g,h,i,j,k);
        bigbuf[ai*10] = b;
        bigbuf[ai*10+1] = c;
        bigbuf[ai*10+2] = d;
        bigbuf[ai*10+3] = e;
        bigbuf[ai*10+4] = f;
        bigbuf[ai*10+5] = g;
        bigbuf[ai*10+6] = h;
        bigbuf[ai*10+7] = i;
        bigbuf[ai*10+8] = j;
        bigbuf[ai*10+9] = k;
    }
    if(ai == 29999){
        _loaded = true;
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_24, GPIO_PIN_SET);    // data loaded
    }
    return 0;
}
