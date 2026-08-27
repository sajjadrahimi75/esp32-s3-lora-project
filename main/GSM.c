#include "gsm.h"

#include <stdio.h>
#include <string.h>

#include "driver/gpio.h"
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define GSM_UART       UART_NUM_1

#define GSM_TX_PIN     17     // CHANGE TO YOUR GSM TX PIN
#define GSM_RX_PIN     18     // CHANGE TO YOUR GSM RX PIN





void gsm_init(void)
{
gpio_reset_pin(GPIO_NUM_4);
gpio_set_direction(GPIO_NUM_4,GPIO_MODE_OUTPUT);
gpio_reset_pin(GPIO_NUM_7);
gpio_set_direction(GPIO_NUM_7,GPIO_MODE_OUTPUT);

gpio_set_level(GPIO_NUM_4,1);//GSM ON
printf("\nGSM TURNED ON");
vTaskDelay(pdMS_TO_TICKS(200));
gpio_set_level(GPIO_NUM_7,0);//GSM RESET
printf("\nRESETING GSM");
vTaskDelay(pdMS_TO_TICKS(1200));
gpio_set_level(GPIO_NUM_7, 1);     // release RESET
printf("\nRELEASE RESET");
vTaskDelay(pdMS_TO_TICKS(2000));

char command[1000]="";
char received_message_gsm[200]="";
char received_command[200]="";


    uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT
    };

    uart_driver_install(
        GSM_UART,
        2048,
        2048,
        0,
        NULL,
        0
    );

    uart_param_config(
        GSM_UART,
        &uart_config
    );

    uart_set_pin(
    GSM_UART,
    GPIO_NUM_17,       // ESP32 TX
    GPIO_NUM_18,       // ESP32 RX
    UART_PIN_NO_CHANGE,
    UART_PIN_NO_CHANGE
    );
    printf("\nGSM UART initialized");
    printf("\nsending:AT");
    int step=0;
    int let=0;
    while(step<100)
    {
    

         if(step==0 && let==0)
         {
         printf("\nSTEP 1");
         strcpy(command,"ATE0\r\n");
         step=1;
         }
         if(strcmp(received_command, "OK+CPIN: READY") == 0 && step==1)  
         {
          printf("\nSTEP 2");
          vTaskDelay(pdMS_TO_TICKS(100));
          strcpy(command,"AT+CMEE=2\r\n");  
          memset(received_command, 0, sizeof(received_command));
          step=2;
         }
         if(strcmp(received_command, "OKCall ReadySMS Ready") == 0 && step==2)  
         {
          printf("\nSTEP 3");
          vTaskDelay(pdMS_TO_TICKS(100));
          strcpy(command,"AT+CSQ\r\n");  
          memset(received_command, 0, sizeof(received_command));
          step=3;
          let=1;
         }
         if(step==3 && let==0)  
         {
          printf("\nSTEP 4");
          vTaskDelay(pdMS_TO_TICKS(100));
          strcpy(command,"AT+CPIN?\r\n"); //check simcard in 
          step=4;
         }
         if(strcmp(received_command, "+CPIN: READYOK") == 0 && step==4)  
         {
          printf("\nSTEP 5");
          vTaskDelay(pdMS_TO_TICKS(100));
          strcpy(command,"AT+CREG?\r\n"); //check whether the modem is registered on the GSM network 
          step=5;
         }
         if(strcmp(received_command,"+CREG: 0,1OK")==0 && step==5)  
         {
          printf("\nSTEP 6");
          vTaskDelay(pdMS_TO_TICKS(100));
          strcpy(command,"AT+CGATT?\r\n");  
          step=6;
         }
            if(strcmp(received_command,"+CGATT: 1OK")==0 && step==6)  
         { 
          printf("\nSTEP 7");
          vTaskDelay(pdMS_TO_TICKS(100));
          strcpy(command,"AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"\r\n"); 
          step=7; 
         }
         if(strcmp(received_command,"OK")==0 && step==7 )  
         { 
          printf("\nSTEP 8");
          vTaskDelay(pdMS_TO_TICKS(100));
          strcpy(command,"AT+COPS?\r\n"); 
          memset(received_command, 0, sizeof(received_command));
          step=8; 
         }  
         if(strcmp(received_command,"+COPS: 0,0,\"vodafone\"OK")==0 && step==8 )  
         { 
          printf("\nSTEP 9");
          vTaskDelay(pdMS_TO_TICKS(100));
          strcpy(command,"AT+CGDCONT=1,\"IP\",\"apn.fastweb.it\"\r\n"); //AT+SAPBR=3,1,\"APN\",\"mobile.vodafone.it\"
          memset(received_command, 0, sizeof(received_command));
          step=9; 
         }  
         if(strcmp(received_command,"OK")==0 && step==9)  
         { 
          printf("\nSTEP 10");
          vTaskDelay(pdMS_TO_TICKS(100));
          strcpy(command,"AT+SAPBR=3,1,\"APN\",\"apn.fastweb.it\"\r\n"); 
          memset(received_command, 0, sizeof(received_command));  
          step=10;
         } 
         if(strcmp(received_command,"OK")==0 && step==10)  
         { 
          printf("\nSTEP 11");
          vTaskDelay(pdMS_TO_TICKS(100));
          strcpy(command,"AT+CGREG?\r\n"); 
          memset(received_command, 0, sizeof(received_command)); 
          step=11;
         }  
         if(strcmp(received_command,"+CGREG: 0,1OK")==0 && step==11)  
         { 
          printf("\nSTEP 12");
          vTaskDelay(pdMS_TO_TICKS(100));
          strcpy(command,"AT+SAPBR=4,1\r\n"); 
          step=12; 
         }  
         if(strcmp(received_command,"+SAPBR:CONTYPE: GPRSAPN: apn.fastweb.itPHONENUM: USER: PWD: RATE: 2OK")==0 && step==12)  
         { 
          printf("\nSTEP 13");
          vTaskDelay(pdMS_TO_TICKS(100));
          strcpy(command,"AT+CIPSHUT\r\n"); 
          memset(received_command, 0, sizeof(received_command));
          step=13; 
         } 
         if(strcmp(received_command,"SHUT OK")==0 && step==13)  
         { 
          printf("\nSTEP 14");
          vTaskDelay(pdMS_TO_TICKS(100));
          strcpy(command,"AT+CGDCONT=1,\"IP\",\"apn.fastweb.it\"\r\n"); 
          memset(received_command, 0, sizeof(received_command));
          step=14; 
         }  
         if(strcmp(received_command,"OK")==0 && step==14)  
         { 
          printf("\nSTEP 15");
          vTaskDelay(pdMS_TO_TICKS(100));
          strcpy(command,"AT+CSTT=\"apn.fastweb.it\",\"\",\"\"\r\n"); 
          memset(received_command, 0, sizeof(received_command));
          step=15; 
         } 
         if(strcmp(received_command,"OK")==0 && step==15)  
         { 
          printf("\nSTEP 16");
          vTaskDelay(pdMS_TO_TICKS(100));
          strcpy(command,"AT+CIICR\r\n"); 
          memset(received_command, 0, sizeof(received_command));
          step=16; 
         }         
         if(strcmp(received_command,"OK")==0 && step==16)  
         { 
          printf("\nSTEP 17");
          vTaskDelay(pdMS_TO_TICKS(100));
          strcpy(command,"AT+CIFSR\r\n"); 
          memset(received_command, 0, sizeof(received_command));
          step=17;
          let=1; 
         }   
         if(step==17 && let==0)  
         { 
          printf("\nSTEP 18");
          vTaskDelay(pdMS_TO_TICKS(100));
          strcpy(command,"AT+SAPBR=1,1\r\n"); 
          memset(received_command, 0, sizeof(received_command));
          step=18; 
          let=1;
         }     
         if(step==18 && let==0)  
         { 
          printf("\nSTEP 19");
          vTaskDelay(pdMS_TO_TICKS(100));
          strcpy(command,"AT+SAPBR=2,1\r\n"); 
          memset(received_command, 0, sizeof(received_command));
          step=19; 
          let=1;
         } 
         /////////////////////////////////////////////////////////////////////
         if(step==19 && let==0)  
         { 
          printf("\nSTEP 20");
          vTaskDelay(pdMS_TO_TICKS(100));
          strcpy(command,"AT+HTTPINIT\r\n"); 
          memset(received_command, 0, sizeof(received_command));
          step=20; 
         } 
         if(strcmp(received_command,"OK")==0 && step==20)  
         { 
          printf("\nSTEP 21");
          vTaskDelay(pdMS_TO_TICKS(100));
          strcpy(command,"AT+HTTPPARA=\"CID\",1\r\n"); 
          memset(received_command, 0, sizeof(received_command));
          step=21; 
         }  
         if(strcmp(received_command,"OK")==0 && step==21)  
         { 
          printf("\nSTEP 22");
          vTaskDelay(pdMS_TO_TICKS(100));
          strcpy(command,"AT+HTTPPARA=\"URL\",\"http://ntfy.sh/sim800_myproject_alert_789\"\r\n"); 
          memset(received_command, 0, sizeof(received_command));
          step=22; 
         }     
         if(strcmp(received_command,"OK")==0 && step==22)  
         { 
          printf("\nSTEP 23");
          vTaskDelay(pdMS_TO_TICKS(100));
          strcpy(command,"AT+HTTPDATA=200,10000\r\n"); 
          memset(received_command, 0, sizeof(received_command));
          step=23; 
         }   
         if(strcmp(received_command,"DOWNLOAD")==0 && step==23)  
         { 
          printf("\nSTEP 24");
          vTaskDelay(pdMS_TO_TICKS(100));
          strcpy(command,"HI MY NAME IS SAJJAD RAHIMI AND THIS IS A TEST OF MY PROJECT"); 
          memset(received_command, 0, sizeof(received_command));
          step=24; 
         } 
         if(strcmp(received_command,"OK")==0 && step==24)  
         { 
          printf("\nSTEP 25");
          vTaskDelay(pdMS_TO_TICKS(100));
          strcpy(command,"AT+HTTPACTION=1\r\n"); 
          memset(received_command, 0, sizeof(received_command));
          step=25; 
         } 
         if(strcmp(received_command,"OK")==0 && step==25)  
         { 
          printf("\nSTEP 26");
          printf("\nWAIT FOR SENDING NEXT MESSAGE");
          vTaskDelay(pdMS_TO_TICKS(10000));
          step=19; //26
         } 
         ///////////////////////////////////////////////////   
         if(strcmp(received_command,"OK")==0 && step==26)  
         { 
          printf("\nSTEP 27");
          vTaskDelay(pdMS_TO_TICKS(100));
          strcpy(command,"AT+SMTPRCPT=0,0,\"sajjadrahimi939@gmail.com\",\"Receiver\""); 
          memset(received_command, 0, sizeof(received_command));
          step=27; 
         }  
         if(strcmp(received_command,"OK")==0 && step==27)  
         { 
          printf("\nSTEP 28");
          vTaskDelay(pdMS_TO_TICKS(100));
          strcpy(command,"AT+SMTPSUB=\"ESP32 Test\""); 
          memset(received_command, 0, sizeof(received_command));
          step=28; 
         }    
         if(strcmp(received_command,"OK")==0 && step==28)  
         { 
          printf("\nSTEP 29");
          vTaskDelay(pdMS_TO_TICKS(100));
          strcpy(command,"AT+SMTPBODY=18"); 
          memset(received_command, 0, sizeof(received_command));
          step=29; 
         }                             
         if(strcmp(received_command,"DOWNLOAD")==0 && step==29)  
         { 
          printf("\nSTEP 30");
          vTaskDelay(pdMS_TO_TICKS(100));
          strcpy(command,"Hello from ESP32"); 
          memset(received_command, 0, sizeof(received_command));
          step=30; 
         }  
         if(strcmp(received_command,"OK")==0 && step==30)  
         { 
          printf("\nSTEP 31");
          vTaskDelay(pdMS_TO_TICKS(100));
          strcpy(command,"AT+SMTPSEND"); 
          memset(received_command, 0, sizeof(received_command));
          step=31; 
         }    
           

         /*if(send>25)  7p55d51772393
         { 
          printf("\nSTEP 6");
          vTaskDelay(pdMS_TO_TICKS(100));
          strcpy(command,"hello sajjad");  //send message
          strcat(command, "\x1A");         //add ctrl+z
         }  */            
      
          /*if(received_command=="CPIN: READY")  
         {
          vTaskDelay(pdMS_TO_TICKS(100));
          strcpy(command,"AT+CMGF=1");  //put SMS into text mode
         } 
         
         if(received_command=="CPIN: READY")  
         {
          vTaskDelay(pdMS_TO_TICKS(100));
          strcpy(command,"AT+CMGS=\"+393515601139\"");  //tell the modem the destination number
         }    
      
         if(received_command=="CPIN: READY")  
         {
          vTaskDelay(pdMS_TO_TICKS(100));
          strcpy(command,"hello sajjad");  //send message
         }    
      
         if(received_command=="CPIN: READY")  
         {
          vTaskDelay(pdMS_TO_TICKS(100));
          
         }*/
         
    
    
    
    //strcpy(command,"AT");
    if(command[0] !='\0')
    {
    printf("\nGSM SEND: %s", command);
    uart_write_bytes(
        GSM_UART,
        command,
        strlen(command)
    );
    /*uart_write_bytes(
        GSM_UART,
        "\r\n",
        2
    );*/
    memset(command, 0, sizeof(command));
    }
     /*if(send==5)
     {
     uart_write_bytes(
        GSM_UART,
        "\x1A",
    1
    );   
    printf("\nCtrl+Z has been sent");
     }
     else
     {*/
        
     //}

    


    // Wait for SIM800C response
  
    int len =0;
    memset(received_message_gsm, 0, sizeof(received_message_gsm));

    len = uart_read_bytes(
        GSM_UART,
        (uint8_t *)received_message_gsm,
        sizeof(received_message_gsm) - 1,
        pdMS_TO_TICKS(2000)//maximum wait for receiving
    );
        if(len > 0)
        {
        received_message_gsm[len] = '\0';

        printf("\nGSM RECEIVED:%s", received_message_gsm);
        let=0;
        
          ////////////////////////////////////finding new line
          int i=0;
          for(i = 0; i < 200; i++)
          {
              if(received_message_gsm[i] == '\n')
              {   
              break;
              }
          }
          ///////////////////////////////////////////////
          int w=0;
          memset(received_command, 0, sizeof(received_command));
          for(int j=0 ;j<=len ;j++)
          {
            if(received_message_gsm[i]!='\0'&& received_message_gsm[i]!='\n'&& received_message_gsm[i]!='\r')
            {
            received_command[w]=received_message_gsm[i];
            w++;
            }

            if(i==200)
            {
              break;
            }
            i++;
          }

        printf("\nmessage after+ is:%s", received_command);
        
        }
        else
        {
            printf("\nGSM: NO RESPONSE");
        }   
        
        

        vTaskDelay(pdMS_TO_TICKS(2000));
    
    }


}