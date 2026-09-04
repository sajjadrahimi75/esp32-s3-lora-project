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





void gsm_init(const uint8_t *test_image,size_t test_image_len)
{
gpio_reset_pin(GPIO_NUM_4);
gpio_set_direction(GPIO_NUM_4,GPIO_MODE_OUTPUT);
gpio_set_level(GPIO_NUM_4, 0);       // GSM OFF
gpio_reset_pin(GPIO_NUM_7);
gpio_set_direction(GPIO_NUM_7,GPIO_MODE_OUTPUT);
gpio_set_level(GPIO_NUM_7, 1);
vTaskDelay(pdMS_TO_TICKS(1000));


char command[1000]="";
char received_message_gsm[200]="";
char received_command[200]="";
char condition_letter[5]="OK";
int condition_len=5;
int command_type=0;
int wait_to_send=2000;

    uart_config_t uart_config = {
        .baud_rate = 115200,
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
    int step=-1;
    int let=0;
    

size_t command_len = 0;
    while(step<90)
    {
    
         if(step==-1)
         {
         gpio_set_level(GPIO_NUM_4,1);//GSM ON
         printf("\nGSM TURNED ON");
         vTaskDelay(pdMS_TO_TICKS(1500));
         gpio_set_level(GPIO_NUM_7,0);//GSM RESET
         printf("\nRESETING GSM");
         vTaskDelay(pdMS_TO_TICKS(1200));
         gpio_set_level(GPIO_NUM_7, 1);     // release RESET
         printf("\nRELEASE RESET");
         vTaskDelay(pdMS_TO_TICKS(4000)); 
         step=0;  
         }
         if(step==0 && let==0)
         {
         printf("\nSTEP 1");
         strcpy(command,"ATE0\r\n");
         step=1;
         let=1;
         command_len = strlen(command);
         }
         if(let==0 && step==1)  
         {
          printf("\nSTEP 2");
          //vTaskDelay(pdMS_TO_TICKS(100));
          strcpy(command,"AT+CMEE=2\r\n");  
          //memset(received_command, 0, sizeof(received_command));
          step=2;
          let=1;
          strcpy(condition_letter,"OK");
          condition_len=2;
          command_len = strlen(command);
         }
         if(step==2 && let==0 )  
         {
          printf("\nSTEP 3");
          //vTaskDelay(pdMS_TO_TICKS(100));
          strcpy(command,"AT+CSQ\r\n");  
          memset(received_command, 0, sizeof(received_command));
          step=3;
          let=1;
          strcpy(condition_letter,"SQ");
          condition_len=6;
          command_len = strlen(command);
         }
         if(step==3 && let==0)  
         {
          printf("\nSTEP 4");
         // vTaskDelay(pdMS_TO_TICKS(100));
          strcpy(command,"AT+CPIN?\r\n"); //check simcard in 
          step=4;
          let=1;
          strcpy(condition_letter,"CP");
          condition_len=11;
          command_len = strlen(command);
         }
         if( step==4 && let==0)  
         {
          printf("\nSTEP 5");
         // vTaskDelay(pdMS_TO_TICKS(100));
          strcpy(command,"AT+CREG?\r\n"); //check whether the modem is registered on the GSM network 
          step=40;
          let=1;
          strcpy(condition_letter,"CR");
          condition_len=9;
          command_len = strlen(command);
         }
         /////////////////////////////////////////////SMS send and recived/////////////////////////////////////////
         if( step==20 && let==0)  
         {
          printf("\nSTEP 21");
         // vTaskDelay(pdMS_TO_TICKS(100));
          strcpy(command,"AT+CMGF=1\r\n");  
          step=21;
          let=1;
          command_len = strlen(command);
         }
         if( step==21 && let==0)  
         {
          printf("\nSTEP 22");
         // vTaskDelay(pdMS_TO_TICKS(100));
          strcpy(command,"AT+CSCA?\r\n");  
          step=22;
          let=1;
          command_len = strlen(command);
         }
         if( step==22 && let==0)  
         {
          printf("\nSTEP 23");
         // vTaskDelay(pdMS_TO_TICKS(100));
          strcpy(command,"AT+CMGS=\"+393518693525\"\r\n");  
          step=23;
          let=1;
          command_len = strlen(command);
         }
         if( step==23 && let==0)  
         {
          printf("\nSTEP 24");
         // vTaskDelay(pdMS_TO_TICKS(100));
          strcpy(command,"HELLO\x1A");  
          step=24;
          let=1;
          command_len = strlen(command);
         }
         if( step==24 && let==0)  
         {
          printf("\nSTEP 25");
         // vTaskDelay(pdMS_TO_TICKS(100));
          strcpy(command,"AT+CMGF=1\r\n");  
          step=25;
          let=1;
          command_len = strlen(command);
         }
         if( step==25 && let==0)  
         {
          printf("\nSTEP 26");
         // vTaskDelay(pdMS_TO_TICKS(100));
          strcpy(command,"AT+CNMI=2,2,0,0,0\r\n");  
          step=40;
          let=1;
          command_len = strlen(command);
         }

         ////////////////////////////////////////////////////////////////////////////////////////////
         if( step==40 && let==0)  
         {
          printf("\nSTEP 41");
         // vTaskDelay(pdMS_TO_TICKS(100));
          strcpy(command,"AT+CGATT?\r\n");  
          step=41;
          let=1;
          strcpy(condition_letter,"CG");
          condition_len=8;
          command_len = strlen(command);
         }
            if(let==0 && step==41)  
         { 
          printf("\nSTEP 42");
         // vTaskDelay(pdMS_TO_TICKS(100));
          strcpy(command,"AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"\r\n"); 
          step=42; 
          let=1;
          strcpy(condition_letter,"OK");
          condition_len=2;
          command_len = strlen(command);
         }
         if(let==0 && step==42 )  
         { 
          printf("\nSTEP 43");
         // vTaskDelay(pdMS_TO_TICKS(100));
          strcpy(command,"AT+COPS?\r\n"); 
          memset(received_command, 0, sizeof(received_command));
          step=43; 
          let=1;
          strcpy(condition_letter,"CO");
          condition_len=30;
          command_len = strlen(command);
         }  
         if(let==0 && step==43 )  
         { 
          printf("\nSTEP 44");
         // vTaskDelay(pdMS_TO_TICKS(100));
          strcpy(command,"AT+CGDCONT=1,\"IP\",\"apn.fastweb.it\"\r\n"); //AT+SAPBR=3,1,\"APN\",\"mobile.vodafone.it\"
          memset(received_command, 0, sizeof(received_command));
          step=44;
          let=1;
          strcpy(condition_letter,"OK");
          condition_len=2;
          command_len = strlen(command); 
         }  
         if(let==0 && step==44)  
         { 
          printf("\nSTEP 50");
         // vTaskDelay(pdMS_TO_TICKS(100));
          strcpy(command,"AT+SAPBR=3,1,\"APN\",\"apn.fastweb.it\"\r\n"); 
          memset(received_command, 0, sizeof(received_command));  
          step=50;
          let=1;
          strcpy(condition_letter,"OK");
          condition_len=2;
          command_len = strlen(command);
         } 
         //////////////////////////////////
         if(let==0 && step==50)  
         { 
          printf("\nSTEP 51");
         // vTaskDelay(pdMS_TO_TICKS(100));
          strcpy(command,"AT+CGREG?\r\n"); 
          memset(received_command, 0, sizeof(received_command)); 
          step=58;
          let=1;
          strcpy(condition_letter,"CG");
          condition_len=10;
          command_len = strlen(command);
         }  
         /*if(let==0 && step==51)  
         { 
          printf("\nSTEP 52");
          vTaskDelay(pdMS_TO_TICKS(100));
          strcpy(command,"AT+SAPBR=4,1\r\n"); 
          step=53; 
          let=1;
          command_len = strlen(command);
         }  
         if(let==0 && step==53)  
         { 
          printf("\nSTEP 54");
          vTaskDelay(pdMS_TO_TICKS(100));
          strcpy(command,"AT+CIPSHUT\r\n"); 
          memset(received_command, 0, sizeof(received_command));
          step=54; 
          let=1;
          command_len = strlen(command);
         } 
         if(let==0 && step==54)  
         { 
          printf("\nSTEP 55");
          vTaskDelay(pdMS_TO_TICKS(100));
          strcpy(command,"AT+CGDCONT=1,\"IP\",\"apn.fastweb.it\"\r\n"); 
          memset(received_command, 0, sizeof(received_command));
          step=55;
          let=1; 
          command_len = strlen(command);
         }  
         if(let==0 && step==55)  
         { 
          printf("\nSTEP 56");
          vTaskDelay(pdMS_TO_TICKS(100));
          strcpy(command,"AT+CSTT=\"apn.fastweb.it\",\"\",\"\"\r\n"); 
          memset(received_command, 0, sizeof(received_command));
          step=56;
          let=1; 
          command_len = strlen(command);
         } 
         if(let==0 && step==56)  
         { 
          printf("\nSTEP 57");
          vTaskDelay(pdMS_TO_TICKS(100));
          strcpy(command,"AT+CIICR\r\n"); 
          memset(received_command, 0, sizeof(received_command));
          step=57;
          let=1; 
          command_len = strlen(command);
         }        
         if(let==0 && step==57)  
         { 
          printf("\nSTEP 58");
          vTaskDelay(pdMS_TO_TICKS(100));
          strcpy(command,"AT+CIFSR\r\n"); 
          memset(received_command, 0, sizeof(received_command));
          step=58;
          let=1; 
          condition_letter[5]="SQ";
          condition_len=6;
          command_len = strlen(command);
         }  */
         if(step==58 && let==0)  
         { 
          printf("\nSTEP 59");
         // vTaskDelay(pdMS_TO_TICKS(100));
          strcpy(command,"AT+SAPBR=1,1\r\n"); 
          memset(received_command, 0, sizeof(received_command));
          step=59; 
          let=1;
          wait_to_send = 30000;
          strcpy(condition_letter,"OK");
          condition_len=2;
          command_len = strlen(command);
         }     
         if(step==59 && let==0)  
         { 
          printf("\nSTEP 60");
         // vTaskDelay(pdMS_TO_TICKS(100));
          strcpy(command,"AT+SAPBR=2,1\r\n"); 
          memset(received_command, 0, sizeof(received_command));
          step=70; 
          let=1;
          strcpy(condition_letter,"SA");
          condition_len=26;
          command_len = strlen(command);
         } 
         
         /////////////////////////////////////////////////////////////////////
         ///////////////sending text////////////////////////////////// 
         if(step==70 && let==0)  
         { 
          printf("\nSTEP 71");
         // vTaskDelay(pdMS_TO_TICKS(100));
          strcpy(command,"AT+HTTPINIT\r\n"); 
          memset(received_command, 0, sizeof(received_command));
          step=71; 
          strcpy(condition_letter,"OK");
          condition_len=2;
          command_len = strlen(command);
         } 
         ////////////////////////timeout
         if(step==71 && strcmp(received_command,"OK")==0)  
         { 
          printf("\nSTEP 72");
         // vTaskDelay(pdMS_TO_TICKS(100));
          strcpy(command,"AT+HTTPPARA=\"TIMEOUT\",300\r\n");
          memset(received_command, 0, sizeof(received_command));
          step=72; 
          let=1;
          strcpy(condition_letter,"OK");
          condition_len=2;
          command_len = strlen(command);
         } 
         /////////////////////////////////////////
         if(let==0 && step==72 && strcmp(received_command,"OK")==0)  
         { 
          printf("\nSTEP 73");
         // vTaskDelay(pdMS_TO_TICKS(100));
          strcpy(command,"AT+HTTPPARA=\"CID\",1\r\n"); 
          memset(received_command, 0, sizeof(received_command));
          step=73; 
          strcpy(condition_letter,"OK");
          condition_len=2;
          command_len = strlen(command);
         }
         if(let==0 && step==73 && strcmp(received_command,"OK")==0)  
         { 
          printf("\nSTEP 74");
         // vTaskDelay(pdMS_TO_TICKS(100));
          strcpy(command,"AT+HTTPPARA=\"URL\",\"http://ntfy.sh/sim800_myproject_alert_789/json?poll=1&since=latest\"\r\n"); 
          memset(received_command, 0, sizeof(received_command));
          step=74;
          let=1; 
          wait_to_send = 30000;
          command_len = strlen(command);
         } 
         if(let==0 && step==74)  
         { 
          printf("\nSTEP 75");
         // vTaskDelay(pdMS_TO_TICKS(100));
          strcpy(command,"AT+HTTPACTION=0\r\n"); //receive message
          memset(received_command, 0, sizeof(received_command));
          step=75;
          let=1; 
          wait_to_send = 30000;
          command_len = strlen(command);
         } 
         if(let==0 && step==75)  
         { 
          printf("\nSTEP 76");
         // vTaskDelay(pdMS_TO_TICKS(100));
          strcpy(command,"AT+HTTPREAD\r\n"); //read message
          memset(received_command, 0, sizeof(received_command));
          step=76;
          let=1; 
          wait_to_send = 30000;
          strcpy(condition_letter,"OK");
          condition_len=2;
          command_len = strlen(command);
         }   
         ///////////////////////////////////////////////
         if(let==0 && step==76 && strcmp(received_command,"OK")==0)  
         { 
          printf("\nSTEP 77");
         // vTaskDelay(pdMS_TO_TICKS(100));
          strcpy(command,"AT+HTTPPARA=\"URL\",\"http://ntfy.sh/sim800_myproject_alert_789\"\r\n"); 
          memset(received_command, 0, sizeof(received_command));
          step=77; 
          strcpy(condition_letter,"OK");
          condition_len=2;
          command_len = strlen(command);
         }    
         if(strcmp(received_command,"OK")==0 && step==77)  
         { 
          printf("\nSTEP 78");
         // vTaskDelay(pdMS_TO_TICKS(100));
          snprintf(command,sizeof(command),"AT+HTTPDATA=100,10000\r\n");
          memset(received_command, 0, sizeof(received_command));
          step=78;
          strcpy(condition_letter,"DO");
          condition_len=8;
          command_len = strlen(command); 
         }   
         if(strcmp(received_command,"DOWNLOAD")==0 && step==78)  
         { 
          printf("\nSTEP 79");
         // vTaskDelay(pdMS_TO_TICKS(100));
          strcpy(command,"IMAGE IS UPLOADING..."); 
          command_len = strlen(command);
          memset(received_command, 0, sizeof(received_command));
          step=79; 
          strcpy(condition_letter,"OK");
          condition_len=2;
         } 
         if(strcmp(received_command,"OK")==0 && step==79)  
         { 
          printf("\nSTEP 80");
         // vTaskDelay(pdMS_TO_TICKS(100));
          strcpy(command,"AT+HTTPACTION=1\r\n"); //send message
          memset(received_command, 0, sizeof(received_command));
          step=80;
          let=1; 
          wait_to_send = 30000;
          command_len = strlen(command);
         } 
         /////////////////////sending image/////////////////////////
         /*if(step==25 && let==0)  
         { 
          printf("\nSTEP 26");
          vTaskDelay(pdMS_TO_TICKS(100));
          strcpy(command,"AT+HTTPINIT\r\n"); 
          memset(received_command, 0, sizeof(received_command));
          step=26; 
          command_len = strlen(command);
         } 
         if(strcmp(received_command,"OK")==0 && step==26)  
         { 
          printf("\nSTEP 27");
          vTaskDelay(pdMS_TO_TICKS(100));
          strcpy(command,"AT+HTTPPARA=\"CID\",1\r\n"); 
          memset(received_command, 0, sizeof(received_command));
          step=27; 
          command_len = strlen(command);
         } 
         if(let==0 && step==27)  
         { 
          printf("\nSTEP 28");
          vTaskDelay(pdMS_TO_TICKS(100));
          strcpy(command,"AT+HTTPPARA=\"URL\",\"http://ntfy.sh/sim800_myproject_alert_789\"\r\n"); 
          memset(received_command, 0, sizeof(received_command));
          step=28; 
          let==1;
          command_len = strlen(command);
         } */
         if(let==0 && step==80)  
         { 
          printf("\nSTEP 81");
         // vTaskDelay(pdMS_TO_TICKS(100));
          snprintf(command,sizeof(command),"AT+HTTPDATA=%u,120000\r\n",test_image_len);
          memset(received_command, 0, sizeof(received_command));
          step=81;
          strcpy(condition_letter,"DO");
          condition_len=8;
          command_len = strlen(command); 
         }   
         if(strcmp(received_command,"DOWNLOAD")==0 && step==81)  //PUT IMAGE INTO BUFFER OF SIM800
         { 
          printf("\nSTEP 82");
         // vTaskDelay(pdMS_TO_TICKS(100));
          command_type=1;
          command_len = test_image_len;
          memset(received_command, 0, sizeof(received_command));
          wait_to_send=2000;
          strcpy(condition_letter,"OK");
          condition_len=2;
          step=82; 
         } 
         if(strcmp(received_command,"OK")==0 && step==82)  
         { 
          printf("\nSTEP 83");
         // vTaskDelay(pdMS_TO_TICKS(100));
          strcpy(command,"AT+HTTPACTION=1\r\n"); 
          memset(received_command, 0, sizeof(received_command));
          step=83; 
          command_len = strlen(command);
          wait_to_send=60000;
          strcpy(condition_letter,"HT");
          condition_len=17;
          let=1;
         } 
         if( step==83 && let==0 && strcmp(received_command,"HTTPACTION: 1,200")==0 )  //+HTTPACTION: 1,408,0
         { 
          printf("\nSTEP 84");
          gpio_set_level(GPIO_NUM_4,0);//GSM OFF
          printf("\nGSM TURNED OFF");
          step=90; 
          command_len = strlen(command);
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
    
    
    if(command_type==0 && command[0] !='\0')
    {
    printf("\nGSM SEND: %s", command);
    uart_write_bytes(GSM_UART,command,command_len);
    }
    else 
    {
     uart_write_bytes(GSM_UART,(const char *)test_image,command_len); 
     command_type=0; 
    }
    /*uart_write_bytes(
        GSM_UART,
        "\r\n",
        2
    );*/
    memset(command, 0, sizeof(command));
    command_len = 0;
    
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
        pdMS_TO_TICKS(wait_to_send)//maximum wait for receiving
    );
    wait_to_send=2000;
        if(len > 0)
        {
        received_message_gsm[len] = '\0';

        printf("\nGSM RECEIVED:%s", received_message_gsm);
        let=0;
        
          ////////////////////////////////////finding new line
          int i=0;
          int c=0;
          int select=0;
          
            for(i = 0; i < 200; i++)
            {
            
                if(received_message_gsm[i] == condition_letter[0] && received_message_gsm[i+1] == condition_letter[1])
                {  
                  break;
                }
                
            }
    
          ///////////////////////////////////////////////
          int w=0;
          memset(received_command, 0, sizeof(received_command));
          for(int j=0 ;j<200 ;j++)
          {
            
            if(received_message_gsm[i]!='\0'&& received_message_gsm[i]!='\n'&& received_message_gsm[i]!='\r')
            {
            received_command[w]=received_message_gsm[i];
            w++;
            }
            i++;
            if(i==200 || condition_len==w)
            {
              break;
            }
          }

        printf("\nmessage after+ is:%s", received_command);
        
        }
       /* else
        {
            printf("\nGSM: NO RESPONSE");
        } */  
        
        

        //vTaskDelay(pdMS_TO_TICKS(2000));
    
    }


}