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
    // GPIO8 controls power to the LoRa module.
 
ESP_LOGI(TAG, "STEP 1: Turning ON LoRa module...");

gpio_reset_pin(GPIO_NUM_47);
gpio_set_direction(GPIO_NUM_47,GPIO_MODE_OUTPUT);
gpio_set_level(GPIO_NUM_47,1); //TURN ON LED


gpio_reset_pin(GPIO_NUM_21);
gpio_set_direction(GPIO_NUM_21, GPIO_MODE_INPUT);

gpio_reset_pin(GPIO_NUM_8);
gpio_set_direction(GPIO_NUM_8, GPIO_MODE_OUTPUT);
gpio_set_level(GPIO_NUM_8, 0);      // LOW = LoRa ON

//reset
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
        ESP_LOGI(TAG, "PASS: BUSY became HIGH after %d ms", i * 10);
        busy_high = true;

        if(gpio_get_level(GPIO_NUM_21)==0)
        {
            ESP_LOGI(TAG, "STEP 1 PASSED --BUSY=0");
            break;
        }
        
    }

    vTaskDelay(pdMS_TO_TICKS(1));
    gpio_set_level(GPIO_NUM_47,1);
}

if(!busy_high)
{
    ESP_LOGE(TAG, "FAIL: BUSY never became HIGH after power ON");

    while(true)
    {
        gpio_set_level(GPIO_NUM_47,0);
        vTaskDelay(pdMS_TO_TICKS(1000));
        gpio_set_level(GPIO_NUM_47,1);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}


//end gpio8 test




gpio_set_direction(GPIO_NUM_46, GPIO_MODE_INPUT);

ESP_LOGI(TAG, "DIO1 level = %d", gpio_get_level(GPIO_NUM_46));

ESP_LOGI(TAG, "LoRa power enabled");
    // Wait for the module power to become stable.
    hal->delay(200);

    gpio_set_level(GPIO_NUM_47,0); //TURN OFF LED
    ESP_LOGI(TAG, "Initializing SX1262...");


    //RESET LORA BEFORE START




//TEST SPI BUSY
gpio_reset_pin(GPIO_NUM_10);      // NSS
gpio_set_direction(GPIO_NUM_10, GPIO_MODE_INPUT_OUTPUT);
gpio_set_level(GPIO_NUM_10, 1);   // Idle state

while(gpio_get_level(GPIO_NUM_21) == 1)
{
    vTaskDelay(pdMS_TO_TICKS(1));
}

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
int16_t state = radio.begin(868.0, 125.0, 9, 7, 0x12, 14, 8, 1.8);

int start=0;
int joinerror=0;
int senderror=0;
int sendsuccessful=0;

for(start=0;start<20;start++)
{


    if(state != RADIOLIB_ERR_NONE)
    {
        ESP_LOGE(TAG,"SX1262 initialization failed, code: %d",state);
        ESP_LOGE(TAG,"TRY AGAIN initialization, TRIED FOR: %d",start);
       /* while(true)
        {
            hal->delay(1000);
        }*/
    }
if(state == RADIOLIB_ERR_NONE)
    {
    ESP_LOGI(TAG,"SX1262 initialization successful");
    break;
}
   gpio_set_level(GPIO_NUM_47,1); //TURN ON LED
   hal->delay(2000);

}//for
// --------------------------------------------------
// Configure LoRaWAN OTAA credentials
// --------------------------------------------------
gpio_set_level(GPIO_NUM_47,0); //TURN OFF LED

ESP_LOGI(TAG, "Configuring LoRaWAN OTAA...");
for(start=0;start<20;start++)
{
state = node.beginOTAA(
    JOIN_EUI,
    DEV_EUI,
    nullptr,
    APP_KEY
);

if(state != RADIOLIB_ERR_NONE)
{
    ESP_LOGE(TAG,"LoRaWAN configuration failed, code: %d",state);
    ESP_LOGE(TAG,"TRY AGAIN LORAWAN configuration, TRIED FOR: %d",start);

   /* while(true)
    {
        hal->delay(1000);
    }*/
}

if(state == RADIOLIB_ERR_NONE)
    {
ESP_LOGI(TAG, "LoRaWAN credentials accepted");
    break;
}
   gpio_set_level(GPIO_NUM_47,1); //TURN ON LED
   hal->delay(2000);
}//for

// --------------------------------------------------
// Join The Things Network
// --------------------------------------------------

gpio_set_level(GPIO_NUM_47,0); //TURN OFF LED
ESP_LOGI(TAG, "Sending LoRaWAN join request...");
hal->delay(100);
for(start=0;start<11;start++)
{

state = node.activateOTAA();

if(state < RADIOLIB_ERR_NONE && state!=-1118 )
{
    ESP_LOGE(TAG,"LoRaWAN join failed, code: %d",state);
    ESP_LOGE(TAG,"TRY AGAIN, TRIED FOR: %d",start);
    joinerror++;
    if(start>9){
     ESP_LOGE(TAG,"timeout waiting for 1min and then try again"); 
    start=0;
     while(1)
     {
        gpio_set_level(GPIO_NUM_47,1); //TURN On LED
        hal->delay(250);
        gpio_set_level(GPIO_NUM_47,0); //TURN OFF LED
        hal->delay(250);
        start++;
        printf("\rcounting for: %d",start);
        if(start>59)
        {
            start=0;
            break;
        }
     }
       
      start=0;
    }
    
}

if(state == RADIOLIB_ERR_NONE||state==-1118)
{
ESP_LOGI(TAG,"LoRaWAN join successful, result: %d",state);
/////////////////////////////////////////////saving buffer////////////////
memcpy(g_noncesBuffer,node.getBufferNonces(),RADIOLIB_LORAWAN_NONCES_BUF_SIZE);

memcpy(g_sessionBuffer,node.getBufferSession(),RADIOLIB_LORAWAN_SESSION_BUF_SIZE);

g_noncesSize = RADIOLIB_LORAWAN_NONCES_BUF_SIZE;
g_sessionSize = RADIOLIB_LORAWAN_SESSION_BUF_SIZE;
/////////////////////////////////////////////////////////
 break;
  gpio_set_level(GPIO_NUM_47,1); //TURN On LED
  
}
   hal->delay(2000);

}//for
// --------------------------------------------------
// Send LoRaWAN uplinks
// --------------------------------------------------
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
while(true)
{
    snprintf(message,sizeof(message),"MSG:%d**MSGerror:%d**NONCESSsdiff:%d**SESSIONdiff:%d**",sendsuccessful,senderror,noncess_diff,session_diff);
    
    printf("\nSTART TO SEND=%s\n",message);
    /*state = node.sendReceive(
    "Hello my name is SAJJAD",
        1,
        true
    );*/

uint8_t dataDown[RADIOLIB_LORAWAN_MAX_PAYLOAD_SIZE + 1] = {0};
size_t lenDown = 0;

LoRaWANEvent_t eventUp = {};
LoRaWANEvent_t eventDown = {};




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
///////////////////////////////////////////////////////////////////////////

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
timeState =
    node.getMacDeviceTimeAns(
        &unixTime,
        &ms,
        true
    );


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


snprintf(timemessage,sizeof(timemessage),"Time: %02d:%02d:%02d|||Date: %02d/%02d/%04d\n"
,hour, minute, second, day, month, year);

}
else
{
    printf("No DeviceTimeAns received.\r\n");
   
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

    ESP_LOGI(
        TAG,
        "Downlink received in RX%d, FPort=%u, length=%u",
        state,
        eventDown.fPort,
        (unsigned int)lenDown
    );

    printf(
        "Received text: %s\r\n",
        reinterpret_cast<char*>(dataDown)
    );
}
else if(state == RADIOLIB_ERR_NONE)
{
    ESP_LOGI(
        TAG,
        "Uplink sent successfully, no downlink received"
    );
}
else
{
    ESP_LOGE(
        TAG,
        "LoRaWAN sendReceive failed, code: %d",
        state
    );
}
//////////////////////////////////////////

    if(state >= RADIOLIB_ERR_NONE)
    {
        ESP_LOGI(TAG,"LoRaWAN uplink sent, result: %d",state);
        sendsuccessful++;
        while(1)
        {
         printf("\r WAITING FOR SENDING NEXT MESSAGE(count: %d)\r",start);
            gpio_set_level(GPIO_NUM_47,1);
            hal->delay(50);
            gpio_set_level(GPIO_NUM_47,0);
            hal->delay(950);
            start++;
                if(start>299)
            {
                start=0;
                break;
            }
        }
    }
    else
    {
        ESP_LOGE(TAG,"LoRaWAN uplink failed, code: %d",state);
        senderror++;
        while(1){
         gpio_set_level(GPIO_NUM_47,1);
         hal->delay(500);
         gpio_set_level(GPIO_NUM_47,0);
         hal->delay(500); 
         start++;
         printf("\r wait until next try,count: %d\r",start);
         if(start>60)
         {
            start=0;
            break;
         }  
        }
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

    }
    
}