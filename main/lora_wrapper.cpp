#include <RadioLib.h>
#include <cstring>
#include "EspHal.h"
#include <time.h>

#include "driver/gpio.h"
#include "esp_log.h"

extern volatile int interrupt_dio1;

//buffer of lora
uint8_t g_noncesBuffer[RADIOLIB_LORAWAN_NONCES_BUF_SIZE];
uint8_t g_sessionBuffer[RADIOLIB_LORAWAN_SESSION_BUF_SIZE];

uint8_t g_noncesBuffer_aftersend[RADIOLIB_LORAWAN_NONCES_BUF_SIZE];
uint8_t g_sessionBuffer_aftersend[RADIOLIB_LORAWAN_SESSION_BUF_SIZE];

uint16_t g_noncesSize;
uint16_t g_sessionSize;

uint16_t g_noncesSize_aftersend;
uint16_t g_sessionSize_aftersend;

int noncess_diff=0;
int session_diff=0;

///////////////////////////////////

// SPI pins:
// SCK  = GPIO14
// MISO = GPIO11
// MOSI = GPIO13
EspHal* hal = new EspHal(
    14,
    11,
    13
);




// SX1262 pins:
// NSS   = GPIO10
// DIO1  = GPIO46
// RESET = GPIO9
// BUSY  = GPIO21
SX1262 radio = new Module(
    hal,
    10,
    46,
    9,
    21
);

//adding code for transmitting
LoRaWANNode node(
    &radio,
    &EU868,
    0
);

// TTN OTAA credentials
static const uint64_t JOIN_EUI = 0x0000000000000000;
static const uint64_t DEV_EUI  = 0x70B3D57ED00787BF;

static const uint8_t APP_KEY[16] =
{
    0x9B, 0xB0, 0x36, 0x7D,
    0x49, 0xCA, 0xF7, 0x96,
    0x5C, 0xF5, 0x73, 0xE6,
    0xD9, 0x21, 0xFE, 0x6A
};


static const char* TAG = "LoRa";



extern "C" void app_main(void)
{
    int step=1;
    int step_1error=0;
    int step_2error=0;
    int step_3error=0;
    int step_4error=0;
    int step_5error=0;
    int step_6error=0;
    int first_run=0;
    int start=0;
    int joinerror=0;
    int senderror=0;
    int sendsuccessful=0;
    int16_t state=-1116;
    ///////////////////////////////////////errors
    if(step_1error>9)
    {
        while(1)
        {
         ESP_LOGI(TAG, "STEP 1 ERROR (STOPPED TRYING)");
         hal->delay(1000);
        }

    }

    if(step_2error>9)
    {
        while(1)
        {
         ESP_LOGI(TAG, "STEP 2 ERROR (STOPPED TRYING)");
         hal->delay(1000);
        }

    }

    if(step_3error>50000)
    {
        while(1)
        {
         ESP_LOGI(TAG, "STEP 3 ERROR (STOPPED TRYING)");
         hal->delay(1000);
        }

    }

    if(step_4error>15)
    {
        while(1)
        {
         ESP_LOGI(TAG, "STEP 4 ERROR (STOPPED TRYING)");
         hal->delay(1000);
        }

    }

    if(step_5error>50)
    {
        while(1)
        {
         ESP_LOGI(TAG, "STEP 5 ERROR (STOPPED TRYING)");
         hal->delay(1000);
        }

    }

    if(step_6error>20)
    {
        while(1)
        {
         ESP_LOGI(TAG, "STEP 6 ERROR (STOPPED TRYING)");
         hal->delay(1000);
        }

    }

    

    while(1)  ///////////////////////////////////FOR ALL PROCESS
    {
    ///////////// GPIO8 controls power to the LoRa module.//////////////////////////////////

        if(step==1)
        {
            ESP_LOGI(TAG, "STEP 1: Turning ON LoRa module...");

                while(1) ///////////////////////////for turn on lora and reset(step1)
                {

                gpio_reset_pin(GPIO_NUM_47);
                gpio_set_direction(GPIO_NUM_47,GPIO_MODE_OUTPUT);
                gpio_set_level(GPIO_NUM_47,1); //TURN ON LED


                gpio_reset_pin(GPIO_NUM_21);
                gpio_set_direction(GPIO_NUM_21, GPIO_MODE_INPUT);


                gpio_reset_pin(GPIO_NUM_8);
                gpio_set_direction(GPIO_NUM_8, GPIO_MODE_OUTPUT);
                gpio_set_level(GPIO_NUM_8, 0);      // LOW = LoRa ON
                ///////// Wait for the module power to become stable.
                hal->delay(200);
                ESP_LOGI(TAG, "LoRa power enabled");

                /////////////////////////////////////reset////////////////////////////////////////
                gpio_set_direction(GPIO_NUM_9, GPIO_MODE_INPUT_OUTPUT);
                gpio_set_level(GPIO_NUM_9, 0);
                hal->delay(10);
                ESP_LOGI(TAG, "RESET LORA (GPIO LEVE IS:) = %d", gpio_get_level(GPIO_NUM_9));

                gpio_set_level(GPIO_NUM_9, 1);
                ESP_LOGI(TAG, "RELEASE RESET (GPIO LEVE IS:) = %d", gpio_get_level(GPIO_NUM_9));

                bool busy_high = false;
                gpio_set_level(GPIO_NUM_47,0); //TURN OFF LED

                    for(int i = 0; i < 1000; i++)      // wait up to 1000 ms
                    {

                        if(gpio_get_level(GPIO_NUM_21) == 1)
                        {
                        ESP_LOGI(TAG, "BUSY became HIGH after %d ms", i * 10);
                        busy_high = true;

                            if(gpio_get_level(GPIO_NUM_21)==0)
                            {
                            ESP_LOGI(TAG, "STEP 1 PASSED --BUSY=0");
                            step=2;
                            break;
                            }
            
                        }

                    vTaskDelay(pdMS_TO_TICKS(1));
                    }
                gpio_set_level(GPIO_NUM_47,1);//TURN On LED
        

                    if(!busy_high)
                    {
                    ESP_LOGE(TAG, "FAIL: BUSY never became HIGH after power ON");

                        while(1)
                        {
                        gpio_set_level(GPIO_NUM_47,1);
                        hal->delay(500);
                        gpio_set_level(GPIO_NUM_47,0);
                        hal->delay(500); 
                        start++;
                        printf("\n");
                        printf("\r WAIT 30 SEC FOR NEXT TRY,count: %d\r",start);
                            if(start>30)
                            {
                                start=0;
                                step_1error++;
                                break;
                            }  
        
                        }
                    }

                    if (busy_high)
                    {
                    step=2;
                    break;
                    }
                }//while
        }//if for step=1
                            //end gpio8 test

        

        if(step==2)
        {

            gpio_set_direction(GPIO_NUM_46, GPIO_MODE_INPUT);
            gpio_set_level(GPIO_NUM_47,0); //TURN OFF LED
            ESP_LOGI(TAG, "STEP2:Initializing SX1262...");

                while(1)
                {
                    /////////////////////////////TEST SPI BUSY
                    gpio_reset_pin(GPIO_NUM_10);      // NSS
                    gpio_set_direction(GPIO_NUM_10, GPIO_MODE_INPUT_OUTPUT);
                    gpio_set_level(GPIO_NUM_10, 1);   // Idle state

                        while(gpio_get_level(GPIO_NUM_21) == 1)////BUSY IS 1 SO WAITING
                        {
                            vTaskDelay(pdMS_TO_TICKS(1));
                            start++;
                                if(start>60000)
                                {
                                    ESP_LOGI(TAG, "ERROR IN BUSY LINE(NEVER BECOME LOW)"); 
                                    start=0;
                                    step_2error++;
                                    step=1;
                                    break; 
                                }
                        }       

        
                ESP_LOGI(TAG, "STEP 2 PASSED ,BUSY LINE(LOW)");
                start=0;
                step=3;
                step_2error=0;
                break;

                } //while
        }//if for step 2
        
        if(step==3)
        {
            ESP_LOGI(TAG, "STEP3:TESTING SPI...");

                while(1)
                {
                    hal->spiBegin();
                    hal->spiBeginTransaction();

                    gpio_set_level(GPIO_NUM_10, 0);   // CS LOW
                    ESP_LOGI(TAG, "NSS = %d", gpio_get_level(GPIO_NUM_10));

                    uint8_t tx[2] = {0xC0, 0x00};
                    uint8_t rx[2] = {0};

                    hal->spiTransfer(tx, 2, rx);

                    gpio_set_level(GPIO_NUM_10, 1);   // CS HIGH
                    ESP_LOGI(TAG, "NSS = %d", gpio_get_level(GPIO_NUM_10));

                    hal->spiEndTransaction();

                    ESP_LOGI(TAG, "GetStatus RX = %02X %02X", rx[0], rx[1]);


                    uint8_t status = rx[1];

                    uint8_t chip_mode = (status >> 4) & 0x07;
                    uint8_t command_status = (status >> 1) & 0x07;

                    ESP_LOGI(TAG, "Status byte = 0x%02X", status);
                    ESP_LOGI(TAG, "Chip mode = %u", chip_mode);
                    ESP_LOGI(TAG, "Command status = %u", command_status);

                    //start testing crystal voltage
                    /*for(float voltage = 0.0f; voltage <= 3.3f; voltage += 0.1f)
                    {
                    printf("Voltage test = %.1f V\n", voltage);

                    int16_t state = radio.begin(868.0, 125.0, 9, 7, 0x12, 14, 8, 1.8);

                    if(state!=RADIOLIB_ERR_NONE){
                    ESP_LOGE(TAG,"SX1262 initialization failed, code: %d",state); 
                    if(voltage>=3.3)
                    {
                    break;
                    }
                    }

                    if(state == RADIOLIB_ERR_NONE){
                    ESP_LOGE(TAG,"voltage was, code: %.1f",voltage);
                    break;
                    }

                    }
                    }*/

                    state = radio.begin(868.0, 125.0, 9, 7, 0x12, 14, 8, 1.8);

                        for(start=0;start<20;start++)
                        {
                            if(state != RADIOLIB_ERR_NONE)
                            {
                            ESP_LOGE(TAG,"SX1262 initialization failed, code: %d",state);
                            ESP_LOGE(TAG,"TRY AGAIN initialization, TRIED FOR: %d",start);
                            }

                            if(state == RADIOLIB_ERR_NONE)
                            {
                            break;
                            }
                            gpio_set_level(GPIO_NUM_47,1); //TURN ON LED
                            hal->delay(2000);
                            
                            if(start>18)
                            {
                                step_3error++;
                                step=1;
                            }
                        }//for

                        if(state == RADIOLIB_ERR_NONE)
                        {
                        ESP_LOGI(TAG,"SX1262 initialization successful");
                        step=4;
                        step_3error=0;
                        break;
                        }
                }//while
        }//if step=3

                            // Configure LoRaWAN OTAA credentials

        if(step==4)
        {
            gpio_set_level(GPIO_NUM_47,0); //TURN OFF LED

            ESP_LOGI(TAG, "STEP4:Configuring LoRaWAN OTAA...");

                while(1)
                {
                    ESP_LOGI(TAG, "first_run:%d",first_run);
                    if(first_run==1)
                    {
                        for(start=0;start<20;start++)
                        {
                            state = node.setBufferNonces(g_noncesBuffer_aftersend);

                            if(state == RADIOLIB_ERR_NONE)
                            {
                                ESP_LOGI(TAG, "Nonces buffer restored");
                                break;
                            }
                            else
                            {
                                ESP_LOGE(TAG, "Nonces restore failed: %d", state);
                            }
                            gpio_set_level(GPIO_NUM_47,1); //TURN ON LED
                            hal->delay(2000);

                            if(start>18)
                            {
                                step_4error++;
                                step=1;
                            }
                        }//for

                        for(start=0;start<20;start++)
                        {
                            state = node.setBufferSession(g_sessionBuffer_aftersend);

                            if(state == RADIOLIB_ERR_NONE)
                            {
                                ESP_LOGI(TAG, "Session buffer restored");
                                break;
                            }
                            else
                            {
                                ESP_LOGE(TAG, "Session restore failed: %d", state);
                            }
                        gpio_set_level(GPIO_NUM_47,1); //TURN ON LED
                        hal->delay(2000);
                            if(start>18)
                            {
                                step_4error++;
                                step=1;
                            }
                        }//for
                    }//first_run=1


                    if(first_run==0)
                    {
                        for(start=0;start<20;start++)
                        {
                        state = node.beginOTAA(JOIN_EUI,DEV_EUI,nullptr,APP_KEY);

                            if(state != RADIOLIB_ERR_NONE)
                            {
                            ESP_LOGE(TAG,"LoRaWAN configuration failed, code: %d",state);
                            ESP_LOGE(TAG,"TRY AGAIN LORAWAN configuration, TRIED FOR: %d",start);

                            }

                        if(state == RADIOLIB_ERR_NONE)
                        {
                        break;
                        }
                        gpio_set_level(GPIO_NUM_47,1); //TURN ON LED
                        hal->delay(2000);
                        if(start>18)
                            {
                                step_4error++;
                                step=1;
                            }
                        }//for
                    }//first_run=0


                    if(state == RADIOLIB_ERR_NONE)
                    {
                    ESP_LOGI(TAG, "LoRaWAN credentials accepted");
                        if(first_run==1)
                        {
                            step=6;
                            step_4error=0;
                            ESP_LOGI(TAG, "JUMP TO STEP 6");
                        }
                        else
                        {
                            step=5;   
                        }
                        break;
                    }
                }//while
            
        }//fpr step=4
    // --------------------------------------------------
    // Join The Things Network
    // --------------------------------------------------

        if(step==5)
        {

        gpio_set_level(GPIO_NUM_47,0); //TURN OFF LED
        ESP_LOGI(TAG, "STEP5:Sending LoRaWAN join request...");
        hal->delay(100);
            while(1)
            {
                for(start=0;start<11;start++)
                {

                state = node.activateOTAA();

                    if(state < RADIOLIB_ERR_NONE && state!=-1118 )
                    {
                    ESP_LOGE(TAG,"LoRaWAN join failed, code: %d",state);
                    ESP_LOGE(TAG,"TRY AGAIN, TRIED FOR: %d",start);
                    joinerror++;
                        if(start>9)
                        {
                        ESP_LOGE(TAG,"timeout waiting for 1min and then try again"); 
                        start=0;
                            while(1)
                            {
                            gpio_set_level(GPIO_NUM_47,1); //TURN On LED
                            hal->delay(250);
                            gpio_set_level(GPIO_NUM_47,0); //TURN OFF LED
                            hal->delay(250);
                            start++;
                            printf("\rWAITING 1 MIN TO TRY AGAIN(count:): %d",start);
                                if(start>59)
                                {
                                start=0;
                                step_5error++;
                                step=1;
                                break;
                                }
                            }
        
                            start=0;
                        }
        
                    }

                    if(state == RADIOLIB_ERR_NONE||state==-1118)
                    {
                    break;
                    }
                hal->delay(2000);

                }//for
        
                if(state == RADIOLIB_ERR_NONE||state==-1118)
                {
                ESP_LOGI(TAG,"LoRaWAN join successful, result: %d",state);
        /////////////////////////////////////////////saving buffer////////////////
                memcpy(g_noncesBuffer,node.getBufferNonces(),RADIOLIB_LORAWAN_NONCES_BUF_SIZE);
                memcpy(g_sessionBuffer,node.getBufferSession(),RADIOLIB_LORAWAN_SESSION_BUF_SIZE);
                g_noncesSize = RADIOLIB_LORAWAN_NONCES_BUF_SIZE;
                g_sessionSize = RADIOLIB_LORAWAN_SESSION_BUF_SIZE;
        /////////////////////////////////////////////////////////
                step=6;
                step_6error=0;
                gpio_set_level(GPIO_NUM_47,1); //TURN On LED
                break; 
                } 

            }//while

        }//if for step=5
    // --------------------------------------------------
    // Send LoRaWAN uplinks
    // --------------------------------------------------
        if(step==6)
        {
        ESP_LOGI(TAG, "STEP6:Sending and receiving message...");
        gpio_set_level(GPIO_NUM_47,0); //TURN OFF LED
        char send_message[200];
        char message[200];
        int send_turn=0;
        int16_t timeState ;
        char timemessage[200];
        int year   = 0;
        int month  = 0;
        int day    = 0;
        int hour   = 0;
        int minute = 0;
        int second = 0;

            while(1)
            {
             snprintf(message,sizeof(message),"MSG:%d**MSGerror:%d**NONCESSsdiff:%d**SESSIONdiff:%d**",sendsuccessful,senderror,noncess_diff,session_diff);
        
             printf("\nSTART TO SEND=>%s\n",message);
    
             uint8_t dataDown[RADIOLIB_LORAWAN_MAX_PAYLOAD_SIZE + 1] = {0};
             size_t lenDown = 0;

             LoRaWANEvent_t eventUp = {};
             LoRaWANEvent_t eventDown = {};

    ////////////////////////////////////sending two message by turn/////////////////////
                if(send_turn == 0)
                {
                strcpy(send_message, message);
                }
                else if(send_turn == 1)
                {
                strcpy(send_message, timemessage);
                }
            send_turn++;
                if(send_turn==2)
                {
                send_turn=0;
                }
    //////////////////////////// Ask network time//////////////////////////////
            uint32_t unixTime =0;
            uint16_t ms =0;
            int16_t timestate =node.sendMacCommandReq(RADIOLIB_LORAWAN_MAC_DEVICE_TIME);

    //////////////////////SENDING AND RECEIVING/////////////////////////////////////
            state = node.sendReceive(
            send_message,
            1,
            dataDown,
            &lenDown,
            true,
            &eventUp,
            &eventDown
            );
    /////////////////////////////////////////////////////////////////////////////////
    //////////////////////////getting time//////////////////////////////////////////////
            timeState =node.getMacDeviceTimeAns(&unixTime,&ms,true);

    //////////////////////////////calculating the time////////////////////////////

                if(timeState == RADIOLIB_ERR_NONE)
                {
                time_t rawTime = static_cast<time_t>(unixTime);

                struct tm timeInfo = {};

                gmtime_r(&rawTime, &timeInfo);

                year   = timeInfo.tm_year + 1900;
                month  = timeInfo.tm_mon + 1;
                day    = timeInfo.tm_mday;
                hour   = timeInfo.tm_hour+2;
                minute = timeInfo.tm_min;
                second = timeInfo.tm_sec;

                printf("Date: %02d/%02d/%04d\n", day, month, year);
                printf("Time: %02d:%02d:%02d\n", hour, minute, second);

                snprintf(timemessage,sizeof(timemessage),"Time: %02d:%02d:%02d|||Date: %02d/%02d/%04d"
                ,hour, minute, second, day, month, year);

                }
                else
                {
                printf("No DeviceTimeAns received.\r\n");
                }
    /////////////////////////////////check message has been send successfully///////////////

                if(state >= RADIOLIB_ERR_NONE)
                {
                ESP_LOGI(TAG,"LoRaWAN uplink sent, result: %d",state);
                sendsuccessful++;
                gpio_set_level(GPIO_NUM_8, 1);      // LOW = LoRa Off
                    while(1)
                    {
                    printf("\r WAITING FOR SENDING NEXT MESSAGE(count: %d)\r",start);
                    gpio_set_level(GPIO_NUM_47,1);
                    hal->delay(10);
                    gpio_set_level(GPIO_NUM_47,0);
                    hal->delay(990);
                    start++;
                        if(start>30)
                        {
                        start=0;
                        step=1;//////////////////////////////test
                        first_run=1;
                        step_6error=0;
                        break;
                        }
                    }
                }
                else
                {
                ESP_LOGE(TAG,"LoRaWAN uplink failed, code: %d",state);
                senderror++;
                    while(1)
                    {
                    gpio_set_level(GPIO_NUM_47,1);
                    hal->delay(500);
                    gpio_set_level(GPIO_NUM_47,0);
                    hal->delay(500); 
                    start++;
                    printf("\r wait until next try,count: %d\r",start);
                        if(start>60)
                        {
                        start=0;
                        step_6error++;
                        step=1;
                        break;
                        }  
                    }
                }
        
    /////////////////////////////////////////////saving buffer after sending////////////////
            memcpy(g_noncesBuffer_aftersend,node.getBufferNonces(),RADIOLIB_LORAWAN_NONCES_BUF_SIZE);
            memcpy(g_sessionBuffer_aftersend,node.getBufferSession(),RADIOLIB_LORAWAN_SESSION_BUF_SIZE);
            g_noncesSize_aftersend = RADIOLIB_LORAWAN_NONCES_BUF_SIZE;
            g_sessionSize_aftersend = RADIOLIB_LORAWAN_SESSION_BUF_SIZE;
    /////////////////////////////////////////////////////////
    //showing income message
                if(state == RADIOLIB_LORAWAN_RX1 ||state == RADIOLIB_LORAWAN_RX2)
                {
                dataDown[lenDown] = '\0';

                ESP_LOGI(TAG,"Downlink received in RX%d, FPort=%u, length=%u",state,
                eventDown.fPort,(unsigned int)lenDown);

                printf("Received text: %s\r\n",reinterpret_cast<char*>(dataDown));
                }
                else if(state == RADIOLIB_ERR_NONE)
                {
                ESP_LOGI(TAG,"Uplink sent successfully, no downlink received");
                }
                else
                {
                ESP_LOGE(TAG,"LoRaWAN sendReceive failed, code: %d",state);
                }

        //////////////////////////////////////////showing buffer///////////////////////////
    
            printf("\n===== NONCES BUFFER =====\n");
            printf("\nNONCES SIZE IS:%d\n",g_noncesSize);
                for(int i = 0; i < g_noncesSize; i++)
                {
                printf("%02X ", g_noncesBuffer[i]);

                    if((i + 1) % 16 == 0)
                    {
                    printf("\n");
                    }
                }

            printf("\n\n===== SESSION BUFFER =====\n");
            printf("\nSESSION SIZE IS:%d\n",g_sessionSize);
                for(int i = 0; i < g_sessionSize; i++)
                {
                printf("%02X ", g_sessionBuffer[i]);

                    if((i + 1) % 16 == 0)
                    {
                    printf("\n");
                    }
                }

            printf("\n");
    //////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////showing buffer after send///////////////////////////
    
            printf("\n===== NONCES BUFFER AFTER SEND =====\n");
            printf("\nNONCES SIZE AFTER SEND IS:%d\n",g_noncesSize_aftersend);

                for(int i = 0; i < g_noncesSize_aftersend; i++)
                {
                printf("%02X ", g_noncesBuffer_aftersend[i]);

                    if((i + 1) % 16 == 0)
                    {
                    printf("\n");
                    }
                }

            printf("\n\n===== SESSION BUFFER AFTER SEND =====\n");
            printf("\nSESSION SIZE AFTER SEND IS:%d\n",g_sessionSize_aftersend);
                for(int i = 0; i < g_sessionSize_aftersend; i++)
                {
                printf("%02X ", g_sessionBuffer_aftersend[i]);

                    if((i + 1) % 16 == 0)
                    {
                    printf("\n");
                    }
                }

            printf("\n");
    //////////////////////////////////////////////////////////////////////

                if(memcmp(g_noncesBuffer,g_noncesBuffer_aftersend,g_noncesSize) != 0)
                {
                noncess_diff++;
                }

                if(memcmp(g_sessionBuffer,g_sessionBuffer_aftersend,g_sessionSize) != 0)
                {
                session_diff++;
                }

             printf("\nNONCES DIFFERENT IS:%d///SESSION DIFFERENT IS:%d\n",noncess_diff,session_diff);
        
             printf("\n\r\033[34mRESULT OF ERROR ARE= ERROR OF JOIN:%d//ERROR OF SEND MESSAGE:%d//successful send:%d\033[0m\n",joinerror,senderror,sendsuccessful);

             break;
        
             }//while
        }//if for step=6
    
    }
}
