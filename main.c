//*****************************************************************************
//
// Copyright (C) 2014 Texas Instruments Incorporated - http://www.ti.com/ 
// 
// 
//  Redistribution and use in source and binary forms, with or without 
//  modification, are permitted provided that the following conditions 
//  are met:
//
//    Redistributions of source code must retain the above copyright 
//    notice, this list of conditions and the following disclaimer.
//
//    Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the 
//    documentation and/or other materials provided with the   
//    distribution.
//
//    Neither the name of Texas Instruments Incorporated nor the names of
//    its contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
//  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
//  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
//  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
//  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
//  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
//  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
//  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
//  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
//  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
//  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//*****************************************************************************

//*****************************************************************************
//
// Application Name     -   HTTP Server
// Application Overview -   This is a sample application demonstrating
//                          interaction between HTTP Client(Browser) and 
//                          SimpleLink Device.The SimpleLink device runs an 
//                          HTTP Server and user can interact using web browser.
// Application Details  -
// http://processors.wiki.ti.com/index.php/CC32xx_HTTP_Server
// or
// doc\examples\CC32xx_HTTP_Server.pdf
//
//*****************************************************************************

//****************************************************************************
//
//! \addtogroup httpserver
//! @{
//
//****************************************************************************

// Standard includes
#include <string.h>
#include <stdlib.h>

// Simplelink includes
#include "simplelink.h"
#include "netcfg.h"
#include "wlan.h"

//driverlib includes
#include "hw_ints.h"
#include "hw_types.h"
#include "hw_memmap.h"
#include "interrupt.h"
#include "utils.h"
#include "pin.h"
#include "uart.h"
#include "rom.h"
#include "rom_map.h"
#include "prcm.h"

#include "spi_module.h"
#include "inc/hw_mcspi.h"
//Free_rtos/ti-rtos includes
#include "osi.h"

// Free-RTOS includes
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "portmacro.h"

// common interface includes
#include "gpio_if.h"
#include "uart_if.h"
#include "common.h"

#include "smartconfig.h"
#include "pinmux.h"

#include "cmd_define.h"

//#include "file_operation.h"
//HttpServer MACRO definition
#define APPLICATION_NAME        "HTTP Server"
#define APPLICATION_VERSION     "1.1.0"
#define AP_SSID_LEN_MAX         (33)
#define ROLE_INVALID            (-5)

#define IP_ADDR             0xc0a80102/* 192.168.1.2 */
#define PORT_NUM            5432
#define TR_BUFF_SIZE         1024
#define CMD_LEN_TX          64
#define TCP_PACKET_COUNT    10

#define LED_STRING              "LED"
#define LED1_STRING             "LED1_"
#define LED2_STRING             ",LED2_"
#define LED_ON_STRING           "ON"
#define LED_OFF_STRING          "OFF"

#define OOB_TASK_PRIORITY               (2)
#define OSI_STACK_SIZE                  (2048)
#define SH_GPIO_3                       (3)  /* P58 - Device Mode */
#define ROLE_INVALID                    (-5)
#define AUTO_CONNECTION_TIMEOUT_COUNT   (50)   /* 5 Sec */

#define RECV_TASK_PRIORITY               (3)
#define RECV_STACK_SIZE                  (2048)

#define MAIN_TASK_PRIORITY               (2)
#define MAIN_STACK_SIZE                  (2048)



// Application specific status/error codes
typedef enum{
    // Choosing -0x7D0 to avoid overlap w/ host-driver's error codes
    LAN_CONNECTION_FAILED = -0x7D0,       
    INTERNET_CONNECTION_FAILED = LAN_CONNECTION_FAILED - 1,
    DEVICE_NOT_IN_STATION_MODE = INTERNET_CONNECTION_FAILED - 1,

    TCP_CLIENT_FAILED = DEVICE_NOT_IN_STATION_MODE -1,
    TCP_SERVER_FAILED = TCP_CLIENT_FAILED - 1,
    
    STATUS_CODE_MAX = -0xBB8
}e_AppStatusCodes;


//*****************************************************************************
//                 GLOBAL VARIABLES -- Start
//*****************************************************************************
unsigned long  g_ulStatus = 0;//SimpleLink Status
unsigned long  g_ulPingPacketsRecv = 0; //Number of Ping Packets received 
unsigned long  g_ulGatewayIP = 0; //Network Gateway IP address
unsigned char  g_ucConnectionSSID[SSID_LEN_MAX+1]; //Connection SSID
unsigned char  g_ucConnectionBSSID[BSSID_LEN_MAX]; //Connection BSSID
unsigned char POST_token[] = "__SL_P_ULD";
unsigned char GET_token[]  = "__SL_G_ULD";
int g_iSimplelinkRole = ROLE_INVALID;
signed int g_uiIpAddress = 0;
unsigned char g_ucSSID[AP_SSID_LEN_MAX];

unsigned char  g_ucConnectionSSID[SSID_LEN_MAX+1]; //Connection SSID
unsigned char  g_ucConnectionBSSID[BSSID_LEN_MAX]; //Connection BSSID
unsigned long  g_ulDestinationIp = IP_ADDR;
unsigned int   g_uiPortNum = PORT_NUM;
unsigned long  g_ulPacketCount = TCP_PACKET_COUNT;
unsigned char  g_ucConnectionStatus = 0;
unsigned char  g_ucSimplelinkstarted = 0;
unsigned long  g_ulIpAddr = 0;
char SocketCreated=1;
char g_cBsdBuf[TR_BUFF_SIZE];
char Recvdata[CMD_LEN_TX];
short sTestBufLen;
unsigned int number=0;
unsigned char cmdReceived=0;
unsigned char Recv_Len=0;

extern unsigned char *sindata_ptr;
extern unsigned char cmd_flag;
extern unsigned char CMDBuffer[CMD_LEN_RX];
extern unsigned int *pDataCount;
extern unsigned int messagecount;
extern unsigned char Buffer1_full;
extern unsigned char Buffer2_full;
extern unsigned int DataCount;

SlSockAddrIn_t  sAddr;
SlSockAddrIn_t  sLocalAddr;
int iAddrSize;
int iSockID;
int iNewSockID;
int iStatus;
     
OsiSyncObj_t* pSyncObj_SPI;     //数据相关的信号量
OsiSyncObj_t* pSyncObj_CMD;     //命令相关的信号量
OsiSyncObj_t  SyncObj_SPI;
OsiSyncObj_t  SyncObj_CMD;



unsigned char p2data[1024];

unsigned char datasend=0;
#if defined(ccs)
extern void (* const g_pfnVectors[])(void);
#endif
#if defined(ewarm)
extern uVectorEntry __vector_table;
#endif
//*****************************************************************************
//                 GLOBAL VARIABLES -- End
//*****************************************************************************


//*****************************************************************************
// Variable related to Connection status
//*****************************************************************************
volatile unsigned short g_usMCNetworkUstate = 0;

int g_uiSimplelinkRole = ROLE_INVALID;
unsigned int g_uiDeviceModeConfig = ROLE_STA; //default is STA mode
unsigned char g_ucConnectTimeout =0;
unsigned long ulStatus;

unsigned char Reply_Handshake[]="I am here!";
unsigned char Reply_APFailed[]="Creat AP Failed!";
unsigned char Reply_APOK[]="AP is OK!";
unsigned char Reply_SocketOK[]="Socket is OK!";
unsigned char ConfigInformation[24];
unsigned char TX_Buffer[CMD_LEN_TX];


#ifdef USE_FREERTOS
//*****************************************************************************
// FreeRTOS User Hook Functions enabled in FreeRTOSConfig.h
//*****************************************************************************

//*****************************************************************************
//
//! \brief Application defined hook (or callback) function - assert
//!
//! \param[in]  pcFile - Pointer to the File Name
//! \param[in]  ulLine - Line Number
//!
//! \return none
//!
//*****************************************************************************
void
vAssertCalled( const char *pcFile, unsigned long ulLine )
{
    //Handle Assert here
    while(1)
    {
    }
}

//*****************************************************************************
//
//! \brief Application defined idle task hook
//!
//! \param  none
//!
//! \return none
//!
//*****************************************************************************
void
vApplicationIdleHook( void)
{
    //Handle Idle Hook for Profiling, Power Management etc
}

//*****************************************************************************
//
//! \brief Application defined malloc failed hook
//!
//! \param  none
//!
//! \return none
//!
//*****************************************************************************
void vApplicationMallocFailedHook()
{
    //Handle Memory Allocation Errors
    while(1)
    {
    }
}

//*****************************************************************************
//
//! \brief Application defined stack overflow hook
//!
//! \param  none
//!
//! \return none
//!
//*****************************************************************************
void vApplicationStackOverflowHook( OsiTaskHandle *pxTask,
                                   signed char *pcTaskName)
{
    //Handle FreeRTOS Stack Overflow
    while(1)
    {
    }
}
#endif //USE_FREERTOS
//*****************************************************************************
// 该函数用于命令识别
//*****************************************************************************
void choosecmd(){
  
}



void SendByte(unsigned char data){
 // ulStatus = MAP_SPIIntStatus(GSPI_BASE,false);//true
 MAP_SPIDataPut(GSPI_BASE,(unsigned long)data);//先把数据放到TX的寄存器，等待时钟信号
 ulStatus=HWREG(GSPI_BASE + 0x00000138);
 datasend=1;
  GPIO_IF_LedOn(MCU_RED_LED_GPIO);//64号管脚拉高，表明slave有数据要发送
}
//*****************************************************************************
// 该函数用于利用SPI发送数据
//*****************************************************************************
void SendData(unsigned char *data,unsigned char len){
  unsigned char i=0; 
 // unsigned long ulStatus;
  SlaveMain1();////??? 作用
   // SendByte(len);
  MAP_SPIDataPut(GSPI_BASE,(unsigned long)len);//先把长度放到TX的寄存器，等待时钟信号///？？？先放len
    //while(datasend);
  GPIO_IF_LedOn(MCU_RED_LED_GPIO);//64号管脚拉高，表明slave有数据要发送
  for(i=0;i<(len+1);i++){//由于第一个字节为0，所以要多发一次
    MAP_SPIDataPut(GSPI_BASE,(unsigned long)(*(data+i)));
    // SendByte(*(data+i));
   // while(datasend);
  }
  while(!(HWREG(GSPI_BASE + MCSPI_O_CH0STAT)&MCSPI_CH0STAT_TXS));//等待最后一个字节发送完
  GPIO_IF_LedOff(MCU_RED_LED_GPIO); //64号管脚拉低，表明slave无数据发送
  SlaveMain();////??? 作用  
}


//*****************************************************************************
//
//! \brief The Function Handles WLAN Events
//!
//! \param[in]  pWlanEvent - Pointer to WLAN Event Info
//!
//! \return None
//!
//*****************************************************************************
void SimpleLinkWlanEventHandler(SlWlanEvent_t *pWlanEvent)
{
    if(!pWlanEvent)
    {
        return;
    }

    switch(pWlanEvent->Event)
    {
        case SL_WLAN_CONNECT_EVENT:
        {
            SET_STATUS_BIT(g_ulStatus, STATUS_BIT_CONNECTION);
            
            //
            // Information about the connected AP (like name, MAC etc) will be
            // available in 'slWlanConnectAsyncResponse_t'-Applications
            // can use it if required
            //
            //  slWlanConnectAsyncResponse_t *pEventData = NULL;
            // pEventData = &pWlanEvent->EventData.STAandP2PModeWlanConnected;
            //
            
            // Copy new connection SSID and BSSID to global parameters
            memcpy(g_ucConnectionSSID,pWlanEvent->EventData.
                   STAandP2PModeWlanConnected.ssid_name,
                   pWlanEvent->EventData.STAandP2PModeWlanConnected.ssid_len);
            memcpy(g_ucConnectionBSSID,
                   pWlanEvent->EventData.STAandP2PModeWlanConnected.bssid,
                   SL_BSSID_LENGTH);

            UART_PRINT("[WLAN EVENT] STA Connected to the AP: %s ,"
                        "BSSID: %x:%x:%x:%x:%x:%x\n\r",
                      g_ucConnectionSSID,g_ucConnectionBSSID[0],
                      g_ucConnectionBSSID[1],g_ucConnectionBSSID[2],
                      g_ucConnectionBSSID[3],g_ucConnectionBSSID[4],
                      g_ucConnectionBSSID[5]);
        }
        break;

        case SL_WLAN_DISCONNECT_EVENT:
        {
            slWlanConnectAsyncResponse_t*  pEventData = NULL;

            CLR_STATUS_BIT(g_ulStatus, STATUS_BIT_CONNECTION);
            CLR_STATUS_BIT(g_ulStatus, STATUS_BIT_IP_AQUIRED);

            pEventData = &pWlanEvent->EventData.STAandP2PModeDisconnected;

            // If the user has initiated 'Disconnect' request, 
            //'reason_code' is SL_USER_INITIATED_DISCONNECTION 
            if(SL_USER_INITIATED_DISCONNECTION == pEventData->reason_code)
            {
                UART_PRINT("[WLAN EVENT]Device disconnected from the AP: %s,"
                "BSSID: %x:%x:%x:%x:%x:%x on application's request \n\r",
                           g_ucConnectionSSID,g_ucConnectionBSSID[0],
                           g_ucConnectionBSSID[1],g_ucConnectionBSSID[2],
                           g_ucConnectionBSSID[3],g_ucConnectionBSSID[4],
                           g_ucConnectionBSSID[5]);
            }
            else
            {
                UART_PRINT("[WLAN ERROR]Device disconnected from the AP AP: %s,"
                "BSSID: %x:%x:%x:%x:%x:%x on an ERROR..!! \n\r",
                           g_ucConnectionSSID,g_ucConnectionBSSID[0],
                           g_ucConnectionBSSID[1],g_ucConnectionBSSID[2],
                           g_ucConnectionBSSID[3],g_ucConnectionBSSID[4],
                           g_ucConnectionBSSID[5]);
            }
            memset(g_ucConnectionSSID,0,sizeof(g_ucConnectionSSID));
            memset(g_ucConnectionBSSID,0,sizeof(g_ucConnectionBSSID));
        }
        break;

        case SL_WLAN_STA_CONNECTED_EVENT:
        {
            // when device is in AP mode and any client connects to device cc3xxx
            SET_STATUS_BIT(g_ulStatus, STATUS_BIT_CONNECTION);
            CLR_STATUS_BIT(g_ulStatus, STATUS_BIT_CONNECTION_FAILED);

            //
            // Information about the connected client (like SSID, MAC etc) will
            // be available in 'slPeerInfoAsyncResponse_t' - Applications
            // can use it if required
            //
            // slPeerInfoAsyncResponse_t *pEventData = NULL;
            // pEventData = &pSlWlanEvent->EventData.APModeStaConnected;
            //

        }
        break;

        case SL_WLAN_STA_DISCONNECTED_EVENT:
        {
            // when client disconnects from device (AP)
            CLR_STATUS_BIT(g_ulStatus, STATUS_BIT_CONNECTION);
            CLR_STATUS_BIT(g_ulStatus, STATUS_BIT_IP_LEASED);

            //
            // Information about the connected client (like SSID, MAC etc) will
            // be available in 'slPeerInfoAsyncResponse_t' - Applications
            // can use it if required
            //
            // slPeerInfoAsyncResponse_t *pEventData = NULL;
            // pEventData = &pSlWlanEvent->EventData.APModestaDisconnected;
            //
        }
        break;

        case SL_WLAN_SMART_CONFIG_COMPLETE_EVENT:
        {
            SET_STATUS_BIT(g_ulStatus, STATUS_BIT_SMARTCONFIG_START);

            //
            // Information about the SmartConfig details (like Status, SSID,
            // Token etc) will be available in 'slSmartConfigStartAsyncResponse_t'
            // - Applications can use it if required
            //
            //  slSmartConfigStartAsyncResponse_t *pEventData = NULL;
            //  pEventData = &pSlWlanEvent->EventData.smartConfigStartResponse;
            //

        }
        break;

        case SL_WLAN_SMART_CONFIG_STOP_EVENT:
        {
            // SmartConfig operation finished
            CLR_STATUS_BIT(g_ulStatus, STATUS_BIT_SMARTCONFIG_START);

            //
            // Information about the SmartConfig details (like Status, padding
            // etc) will be available in 'slSmartConfigStopAsyncResponse_t' -
            // Applications can use it if required
            //
            // slSmartConfigStopAsyncResponse_t *pEventData = NULL;
            // pEventData = &pSlWlanEvent->EventData.smartConfigStopResponse;
            //
        }
        break;

        case SL_WLAN_P2P_DEV_FOUND_EVENT:
        {
            SET_STATUS_BIT(g_ulStatus, STATUS_BIT_P2P_DEV_FOUND);

            //
            // Information about P2P config details (like Peer device name, own
            // SSID etc) will be available in 'slPeerInfoAsyncResponse_t' -
            // Applications can use it if required
            //
            // slPeerInfoAsyncResponse_t *pEventData = NULL;
            // pEventData = &pSlWlanEvent->EventData.P2PModeDevFound;
            //
        }
        break;

        case SL_WLAN_P2P_NEG_REQ_RECEIVED_EVENT:
        {
            SET_STATUS_BIT(g_ulStatus, STATUS_BIT_P2P_REQ_RECEIVED);

            //
            // Information about P2P Negotiation req details (like Peer device
            // name, own SSID etc) will be available in 'slPeerInfoAsyncResponse_t'
            //  - Applications can use it if required
            //
            // slPeerInfoAsyncResponse_t *pEventData = NULL;
            // pEventData = &pSlWlanEvent->EventData.P2PModeNegReqReceived;
            //
        }
        break;

        case SL_WLAN_CONNECTION_FAILED_EVENT:
        {
            // If device gets any connection failed event
            SET_STATUS_BIT(g_ulStatus, STATUS_BIT_CONNECTION_FAILED);
        }
        break;

        default:
        {
            UART_PRINT("[WLAN EVENT] Unexpected event [0x%x]\n\r",
                       pWlanEvent->Event);
        }
        break;
    }
}

//*****************************************************************************
//
//! \brief This function handles network events such as IP acquisition, IP
//!           leased, IP released etc.
//!
//! \param[in]  pNetAppEvent - Pointer to NetApp Event Info 
//!
//! \return None
//!
//*****************************************************************************
void SimpleLinkNetAppEventHandler(SlNetAppEvent_t *pNetAppEvent)
{
    if(!pNetAppEvent)
    {
        return;
    }

    switch(pNetAppEvent->Event)
    {
        case SL_NETAPP_IPV4_IPACQUIRED_EVENT:
        {
            SlIpV4AcquiredAsync_t *pEventData = NULL;

            SET_STATUS_BIT(g_ulStatus, STATUS_BIT_IP_AQUIRED);
            
            //Ip Acquired Event Data
            pEventData = &pNetAppEvent->EventData.ipAcquiredV4;
            g_uiIpAddress = pEventData->ip;

            //Gateway IP address
            g_ulGatewayIP = pEventData->gateway;
            
            /*UART_PRINT("[NETAPP EVENT] IP Acquired: IP=%d.%d.%d.%d , "
            "Gateway=%d.%d.%d.%d\n\r", 
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.ip,3),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.ip,2),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.ip,1),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.ip,0),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.gateway,3),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.gateway,2),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.gateway,1),
            SL_IPV4_BYTE(pNetAppEvent->EventData.ipAcquiredV4.gateway,0));
            */
        }
        break;

        case SL_NETAPP_IP_LEASED_EVENT:
        {
            SET_STATUS_BIT(g_ulStatus, STATUS_BIT_IP_LEASED);

            //
            // Information about the IP-Leased details(like IP-Leased,lease-time,
            // mac etc) will be available in 'SlIpLeasedAsync_t' - Applications
            // can use it if required
            //
            // SlIpLeasedAsync_t *pEventData = NULL;
            // pEventData = &pNetAppEvent->EventData.ipLeased;
            //

        }
        break;

        case SL_NETAPP_IP_RELEASED_EVENT:
        {
            CLR_STATUS_BIT(g_ulStatus, STATUS_BIT_IP_LEASED);

            //
            // Information about the IP-Released details (like IP-address, mac
            // etc) will be available in 'SlIpReleasedAsync_t' - Applications
            // can use it if required
            //
            // SlIpReleasedAsync_t *pEventData = NULL;
            // pEventData = &pNetAppEvent->EventData.ipReleased;
            //
        }
		break;

        default:
        {
            UART_PRINT("[NETAPP EVENT] Unexpected event [0x%x] \n\r",
                       pNetAppEvent->Event);
        }
        break;
    }
}

//*****************************************************************************
//
//! This function handles socket events indication
//!
//! \param[in]      pSock - Pointer to Socket Event Info
//!
//! \return None
//!
//*****************************************************************************
void SimpleLinkSockEventHandler(SlSockEvent_t *pSock)
{
    //
    // This application doesn't work w/ socket - Events are not expected
    //
     if(!pSock)
    {
        return;
    }

    //
    // This application doesn't work w/ socket - Events are not expected
    //
    switch( pSock->Event )
    {
        case SL_SOCKET_TX_FAILED_EVENT:
            switch( pSock->EventData.status )
            {
                case SL_ECLOSE:
                    UART_PRINT("[SOCK ERROR] - close socket (%d) operation "
                    "failed to transmit all queued packets\n\n",
                           pSock->EventData.sd);
                    break;
                default:
                    UART_PRINT("[SOCK ERROR] - TX FAILED : socket %d , reason"
                        "(%d) \n\n",
                           pSock->EventData.sd, pSock->EventData.status);
            }
            break;

        default:
            UART_PRINT("[SOCK EVENT] - Unexpected Event [%x0x]\n\n",pSock->Event);
    }
 
}


//*****************************************************************************
//
//! This function gets triggered when HTTP Server receives Application
//! defined GET and POST HTTP Tokens.
//!
//! \param pHttpServerEvent Pointer indicating http server event
//! \param pHttpServerResponse Pointer indicating http server response
//!
//! \return None
//!
//*****************************************************************************
void SimpleLinkHttpServerCallback(SlHttpServerEvent_t *pSlHttpServerEvent, 
                               SlHttpServerResponse_t *pSlHttpServerResponse)
{
    unsigned char strLenVal = 0;

    if(!pSlHttpServerEvent || !pSlHttpServerResponse)
    {
        return;
    }
    
    switch (pSlHttpServerEvent->Event)
    {
        case SL_NETAPP_HTTPGETTOKENVALUE_EVENT:
        {
          unsigned char status, *ptr;

          ptr = pSlHttpServerResponse->ResponseData.token_value.data;
          pSlHttpServerResponse->ResponseData.token_value.len = 0;
          if(memcmp(pSlHttpServerEvent->EventData.httpTokenName.data, GET_token,
                    strlen((const char *)GET_token)) == 0)
          {
            status = GPIO_IF_LedStatus(MCU_RED_LED_GPIO);
            strLenVal = strlen(LED1_STRING);
            memcpy(ptr, LED1_STRING, strLenVal);
            ptr += strLenVal;
            pSlHttpServerResponse->ResponseData.token_value.len += strLenVal;
            if(status & 0x01)
            {
              strLenVal = strlen(LED_ON_STRING);
              memcpy(ptr, LED_ON_STRING, strLenVal);
              ptr += strLenVal;
              pSlHttpServerResponse->ResponseData.token_value.len += strLenVal;
            }
            else
            {
              strLenVal = strlen(LED_OFF_STRING);
              memcpy(ptr, LED_OFF_STRING, strLenVal);
              ptr += strLenVal;
              pSlHttpServerResponse->ResponseData.token_value.len += strLenVal;
            }
            status = GPIO_IF_LedStatus(MCU_GREEN_LED_GPIO);
            strLenVal = strlen(LED2_STRING);
            memcpy(ptr, LED2_STRING, strLenVal);
            ptr += strLenVal;
            pSlHttpServerResponse->ResponseData.token_value.len += strLenVal;
            if(status & 0x01)
            {
              strLenVal = strlen(LED_ON_STRING);
              memcpy(ptr, LED_ON_STRING, strLenVal);
              ptr += strLenVal;
              pSlHttpServerResponse->ResponseData.token_value.len += strLenVal;
            }
            else
            {
              strLenVal = strlen(LED_OFF_STRING);
              memcpy(ptr, LED_OFF_STRING, strLenVal);
              ptr += strLenVal;
              pSlHttpServerResponse->ResponseData.token_value.len += strLenVal;
            }
            *ptr = '\0';
          }

        }
        break;

        case SL_NETAPP_HTTPPOSTTOKENVALUE_EVENT:
        {
          unsigned char led;
          unsigned char *ptr = pSlHttpServerEvent->EventData.httpPostData.token_name.data;

          if(memcmp(ptr, POST_token, strlen((const char *)POST_token)) == 0)
          {
            ptr = pSlHttpServerEvent->EventData.httpPostData.token_value.data;
            strLenVal = strlen(LED_STRING);
            if(memcmp(ptr, LED_STRING, strLenVal) != 0)
              break;
            ptr += strLenVal;
            led = *ptr;
            strLenVal = strlen(LED_ON_STRING);
            ptr += strLenVal;
            if(led == '1')
            {
              if(memcmp(ptr, LED_ON_STRING, strLenVal) == 0)
              {
                      GPIO_IF_LedOn(MCU_RED_LED_GPIO);
              }
              else
              {
                      GPIO_IF_LedOff(MCU_RED_LED_GPIO);
              }
            }
            else if(led == '2')
            {
              if(memcmp(ptr, LED_ON_STRING, strLenVal) == 0)
              {
                      GPIO_IF_LedOn(MCU_GREEN_LED_GPIO);
              }
              else
              {
                      GPIO_IF_LedOff(MCU_GREEN_LED_GPIO);
              }
            }

          }
        }
          break;
        default:
          break;
    }
}

//*****************************************************************************
//
//! \brief This function initializes the application variables
//!
//! \param    None
//!
//! \return None
//!
//*****************************************************************************
static void InitializeAppVariables()
{
    g_ulStatus = 0;
    g_uiIpAddress = 0;
 
    memset(g_ucConnectionSSID,0,sizeof(g_ucConnectionSSID));
    memset(g_ucConnectionBSSID,0,sizeof(g_ucConnectionBSSID));
    g_ulDestinationIp = IP_ADDR;
    g_uiPortNum = PORT_NUM;
    g_ulPacketCount = TCP_PACKET_COUNT;
    
}


//*****************************************************************************
//! \brief This function puts the device in its default state. It:
//!           - Set the mode to STATION
//!           - Configures connection policy to Auto and AutoSmartConfig
//!           - Deletes all the stored profiles
//!           - Enables DHCP
//!           - Disables Scan policy
//!           - Sets Tx power to maximum
//!           - Sets power policy to normal
//!           - Unregister mDNS services
//!           - Remove all filters
//!
//! \param   none
//! \return  On success, zero is returned. On error, negative is returned
//*****************************************************************************
static long ConfigureSimpleLinkToDefaultState()
{
    SlVersionFull   ver = {0};
    _WlanRxFilterOperationCommandBuff_t  RxFilterIdMask = {0};

    unsigned char ucVal = 1;
    unsigned char ucConfigOpt = 0;
    unsigned char ucConfigLen = 0;
    unsigned char ucPower = 0;

    long lRetVal = -1;
    long lMode = -1;

    lMode = sl_Start(0, 0, 0);
    ASSERT_ON_ERROR(lMode);

    // If the device is not in station-mode, try configuring it in station-mode 
    if (ROLE_STA != lMode)
    {
        if (ROLE_AP == lMode)
        {
            // If the device is in AP mode, we need to wait for this event 
            // before doing anything 
            while(!IS_IP_ACQUIRED(g_ulStatus))
            {
#ifndef SL_PLATFORM_MULTI_THREADED
              _SlNonOsMainLoopTask(); 
#endif
            }
        }

        // Switch to STA role and restart 
        lRetVal = sl_WlanSetMode(ROLE_STA);//ROLE_STA
        ASSERT_ON_ERROR(lRetVal);

        lRetVal = sl_Stop(0xFF);
        ASSERT_ON_ERROR(lRetVal);

        lRetVal = sl_Start(0, 0, 0);
        ASSERT_ON_ERROR(lRetVal);

        // Check if the device is in station again 
        if (ROLE_STA != lRetVal)//ROLE_STA
        {
            // We don't want to proceed if the device is not coming up in STA-mode 
            return DEVICE_NOT_IN_STATION_MODE;
        }
    }
    
    // Get the device's version-information
    ucConfigOpt = SL_DEVICE_GENERAL_VERSION;
    ucConfigLen = sizeof(ver);
    lRetVal = sl_DevGet(SL_DEVICE_GENERAL_CONFIGURATION, &ucConfigOpt, 
                                &ucConfigLen, (unsigned char *)(&ver));
    ASSERT_ON_ERROR(lRetVal);
    
    UART_PRINT("Host Driver Version: %s\n\r",SL_DRIVER_VERSION);
    UART_PRINT("Build Version %d.%d.%d.%d.31.%d.%d.%d.%d.%d.%d.%d.%d\n\r",
    ver.NwpVersion[0],ver.NwpVersion[1],ver.NwpVersion[2],ver.NwpVersion[3],
    ver.ChipFwAndPhyVersion.FwVersion[0],ver.ChipFwAndPhyVersion.FwVersion[1],
    ver.ChipFwAndPhyVersion.FwVersion[2],ver.ChipFwAndPhyVersion.FwVersion[3],
    ver.ChipFwAndPhyVersion.PhyVersion[0],ver.ChipFwAndPhyVersion.PhyVersion[1],
    ver.ChipFwAndPhyVersion.PhyVersion[2],ver.ChipFwAndPhyVersion.PhyVersion[3]);

    // Set connection policy to Auto + SmartConfig 
    //      (Device's default connection policy)
    lRetVal = sl_WlanPolicySet(SL_POLICY_CONNECTION, 
                                SL_CONNECTION_POLICY(1, 0, 0, 0, 1), NULL, 0);
    ASSERT_ON_ERROR(lRetVal);

    // Remove all profiles
    lRetVal = sl_WlanProfileDel(0xFF);
    ASSERT_ON_ERROR(lRetVal);

    

    //
    // Device in station-mode. Disconnect previous connection if any
    // The function returns 0 if 'Disconnected done', negative number if already
    // disconnected Wait for 'disconnection' event if 0 is returned, Ignore 
    // other return-codes
    //
    lRetVal = sl_WlanDisconnect();
    if(0 == lRetVal)
    {
        // Wait
        while(IS_CONNECTED(g_ulStatus))
        {
#ifndef SL_PLATFORM_MULTI_THREADED
              _SlNonOsMainLoopTask(); 
#endif
        }
    }

    // Enable DHCP client
    lRetVal = sl_NetCfgSet(SL_IPV4_STA_P2P_CL_DHCP_ENABLE,1,1,&ucVal);
    ASSERT_ON_ERROR(lRetVal);

    // Disable scan
    ucConfigOpt = SL_SCAN_POLICY(0);
    lRetVal = sl_WlanPolicySet(SL_POLICY_SCAN , ucConfigOpt, NULL, 0);
    ASSERT_ON_ERROR(lRetVal);

    // Set Tx power level for station mode
    // Number between 0-15, as dB offset from max power - 0 will set max power
    ucPower = 15;  
    lRetVal = sl_WlanSet(SL_WLAN_CFG_GENERAL_PARAM_ID, 
            WLAN_GENERAL_PARAM_OPT_AP_TX_POWER, 1, (unsigned char *)&ucPower);//WLAN_GENERAL_PARAM_OPT_STA_TX_POWER
    ASSERT_ON_ERROR(lRetVal);
    //Set the SSID
    unsigned char *PLABSSID=ConfigInformation+0;
    lRetVal = sl_WlanSet(SL_WLAN_CFG_AP_ID, WLAN_AP_OPT_SSID, 8,
                                              (unsigned char*)PLABSSID );
    ASSERT_ON_ERROR(lRetVal);  
    //Set the Securuty Type
    unsigned char SecurityType= SL_SEC_TYPE_WPA_WPA2;
     lRetVal = sl_WlanSet(SL_WLAN_CFG_AP_ID, WLAN_AP_OPT_SECURITY_TYPE , 1,
                                              &SecurityType);
    ASSERT_ON_ERROR(lRetVal);  
    //Set the passward
    unsigned char *PASSWARD=ConfigInformation+8;
    lRetVal = sl_WlanSet(SL_WLAN_CFG_AP_ID, WLAN_AP_OPT_PASSWORD  , 8,
                                              (unsigned char*)PASSWARD);
    ASSERT_ON_ERROR(lRetVal); 
    // Set PM policy to normal
    lRetVal = sl_WlanPolicySet(SL_POLICY_PM , SL_NORMAL_POLICY, NULL, 0);
    ASSERT_ON_ERROR(lRetVal);

    // Unregister mDNS services
    lRetVal = sl_NetAppMDNSUnRegisterService(0, 0);
    ASSERT_ON_ERROR(lRetVal);

    // Remove  all 64 filters (8*8)
    memset(RxFilterIdMask.FilterIdMask, 0xFF, 8);
    lRetVal = sl_WlanRxFilterSet(SL_REMOVE_RX_FILTER, (_u8 *)&RxFilterIdMask,
                       sizeof(_WlanRxFilterOperationCommandBuff_t));
    ASSERT_ON_ERROR(lRetVal);

    lRetVal = sl_Stop(SL_STOP_TIMEOUT);
    ASSERT_ON_ERROR(lRetVal);

    InitializeAppVariables();
    
    return lRetVal; // Success
}



//****************************************************************************
//
//!    \brief Connects to the Network in AP or STA Mode - If ForceAP Jumper is
//!                                             Placed, Force it to AP mode
//!
//! \return                        0 on success else error code
//
//****************************************************************************
long ConnectToNetwork()
{
    char ucAPSSID[32];
    unsigned short len, config_opt;
    long lRetVal = -1;

    // staring simplelink
    g_uiSimplelinkRole =  sl_Start(NULL,NULL,NULL);

    // Device is not in STA mode and Force AP Jumper is not Connected 
    //- Switch to STA mode
    if(g_uiSimplelinkRole != ROLE_STA && g_uiDeviceModeConfig == ROLE_STA )
    {
        //Switch to STA Mode
        lRetVal = sl_WlanSetMode(ROLE_STA);
        ASSERT_ON_ERROR(lRetVal);
        
        lRetVal = sl_Stop(SL_STOP_TIMEOUT);
        
        g_usMCNetworkUstate = 0;
        g_uiSimplelinkRole =  sl_Start(NULL,NULL,NULL);
    }

    //Device is not in AP mode and Force AP Jumper is Connected - 
    //Switch to AP mode
    if(g_uiSimplelinkRole != ROLE_AP && g_uiDeviceModeConfig == ROLE_AP )
    {
         //Switch to AP Mode
        lRetVal = sl_WlanSetMode(ROLE_AP);
        ASSERT_ON_ERROR(lRetVal);
        
        lRetVal = sl_Stop(SL_STOP_TIMEOUT);
        
        g_usMCNetworkUstate = 0;
        g_uiSimplelinkRole =  sl_Start(NULL,NULL,NULL);
    }

    //No Mode Change Required
    if(g_uiSimplelinkRole == ROLE_AP)
    {
       //waiting for the AP to acquire IP address from Internal DHCP Server
       while(!IS_IP_ACQUIRED(g_ulStatus))
       {

       }
       char iCount=0;
       //Read the AP SSID
       memset(ucAPSSID,'\0',AP_SSID_LEN_MAX);
       len = AP_SSID_LEN_MAX;
       config_opt = WLAN_AP_OPT_SSID;
       lRetVal = sl_WlanGet(SL_WLAN_CFG_AP_ID, &config_opt , &len,
                                              (unsigned char*) ucAPSSID);

        ASSERT_ON_ERROR(lRetVal);
        
       Report("\n\rDevice is in AP Mode, Please Connect to AP [%s] and"
          "type [mysimplelink.net] in the browser \n\r",ucAPSSID);
    }
    else
    {
        //Stop Internal HTTP Server
        lRetVal = sl_NetAppStop(SL_NET_APP_HTTP_SERVER_ID);
        ASSERT_ON_ERROR( lRetVal);

        //Start Internal HTTP Server
        lRetVal = sl_NetAppStart(SL_NET_APP_HTTP_SERVER_ID);
        ASSERT_ON_ERROR( lRetVal);
        
		//waiting for the device to Auto Connect
		while ( (!IS_IP_ACQUIRED(g_ulStatus))&&
			   g_ucConnectTimeout < AUTO_CONNECTION_TIMEOUT_COUNT)
		{
			//Turn RED LED On
			//GPIO_IF_LedOn(MCU_RED_LED_GPIO);
			osi_Sleep(50);

			//Turn RED LED Off
			//GPIO_IF_LedOff(MCU_RED_LED_GPIO);
			osi_Sleep(50);

			g_ucConnectTimeout++;
		}
		//Couldn't connect Using Auto Profile
		if(g_ucConnectTimeout == AUTO_CONNECTION_TIMEOUT_COUNT)
		{
			//Blink Red LED to Indicate Connection Error
			//GPIO_IF_LedOn(MCU_RED_LED_GPIO);

			CLR_STATUS_BIT_ALL(g_ulStatus);

			Report("Use Smart Config Application to configure the device.\n\r");
			//Connect Using Smart Config
			lRetVal = SmartConfigConnect();
			ASSERT_ON_ERROR(lRetVal);

			//Waiting for the device to Auto Connect
			while(!IS_IP_ACQUIRED(g_ulStatus))
			{
				MAP_UtilsDelay(500);
			}

		}
    //Turn RED LED Off
    GPIO_IF_LedOff(MCU_RED_LED_GPIO);
    UART_PRINT("\n\rDevice is in STA Mode, Connect to the AP[%s] and type"
          "IP address [%d.%d.%d.%d] in the browser \n\r",g_ucConnectionSSID,
          SL_IPV4_BYTE(g_uiIpAddress,3),SL_IPV4_BYTE(g_uiIpAddress,2),
          SL_IPV4_BYTE(g_uiIpAddress,1),SL_IPV4_BYTE(g_uiIpAddress,0));

    }
    return SUCCESS;
}


//****************************************************************************
//
//!    \brief Read Force AP GPIO and Configure Mode - 1(Access Point Mode)
//!                                                  - 0 (Station Mode)
//!
//! \return                        None
//
//****************************************************************************
static void ReadDeviceConfiguration()
{
    unsigned int uiGPIOPort;
    unsigned char pucGPIOPin;
    unsigned char ucPinValue;
        
        //Read GPIO
    GPIO_IF_GetPortNPin(SH_GPIO_3,&uiGPIOPort,&pucGPIOPin);
    ucPinValue = GPIO_IF_Get(SH_GPIO_3,uiGPIOPort,pucGPIOPin);
        
        //If Connected to VCC, Mode is AP
    if(ucPinValue == 1)
    {
            //AP Mode
            g_uiDeviceModeConfig = ROLE_AP;
    }
    else
    {
            //STA Mode
            //g_uiDeviceModeConfig = ROLE_STA;
            g_uiDeviceModeConfig = ROLE_AP;
    }

}
//****************************************************************************
//
//!    \brief Creat AP
//!                                                  
//!
//! \return                        None
//
//****************************************************************************
long Creat_AP()
{
  long lRetVal = -1;
      lRetVal = ConfigureSimpleLinkToDefaultState();//将网络配置到初始状态,此在配置网络的过程中,会根据之前接收到的SSID和PASSWARD信息进行配置
    if(lRetVal < 0)
    {

        if (DEVICE_NOT_IN_STATION_MODE == lRetVal)
            UART_PRINT("Failed to configure the device in its default state\n\r");

        LOOP_FOREVER();
    }

    UART_PRINT("Device is configured in default state \n\r");  
    
    memset(g_ucSSID,'\0',AP_SSID_LEN_MAX);
    
    //Read Device Mode Configuration
    ReadDeviceConfiguration();

    //Connect to Network
    lRetVal = ConnectToNetwork();
    if(lRetVal<0)UART_PRINT("Creat AP Failed!\n\r");
    return lRetVal;
}
//****************************************************************************
//
//!    \brief Creat Socket
//!                                                  
//!
//! \return                        None
//
//****************************************************************************
long Creat_Socket(){
    unsigned int g_Status=-1;
    unsigned char g_NonBlocking=1;
    SlTimeval_t sltimeval;
    sltimeval.tv_sec = 0;
    sltimeval.tv_usec = 300;
    //filling the TCP server socket address
    sLocalAddr.sin_family = SL_AF_INET;
    sLocalAddr.sin_port = sl_Htons((unsigned short)PORT_NUM);
    sLocalAddr.sin_addr.s_addr = 0;

    // creating a TCP socket
    iSockID = sl_Socket(SL_AF_INET,SL_SOCK_STREAM, 0);
    if( iSockID < 0 )
    {
        // error
        UART_PRINT("TCP_SERVER_FAILED\n\r");
    }
   // g_Status = sl_SetSockOpt(iSockID, SL_SOL_SOCKET, SL_SO_NONBLOCKING, &g_NonBlocking, sizeof(g_NonBlocking));
    iStatus = sl_SetSockOpt(iSockID, SL_SOL_SOCKET, SL_SO_RCVTIMEO , &sltimeval, sizeof(sltimeval));
   // if(g_Status<0)UART_PRINT("TCP_SERVER_NONBLOCKING_FAILED\n\r");
    iAddrSize = sizeof(SlSockAddrIn_t);

    // binding the TCP socket to the TCP server address
    iStatus = sl_Bind(iSockID, (SlSockAddr_t *)&sLocalAddr, iAddrSize);
    if( iStatus < 0 )
    {
        // error
      sl_Close(iSockID);
      UART_PRINT("TCP_SERVER_FAILED\n\r");
      return -1;
    }

    // putting the socket for listening to the incoming TCP connection
    iStatus = sl_Listen(iSockID, 0);
    if( iStatus < 0 )
    {
      sl_Close(iSockID);
      UART_PRINT("TCP_SERVER_FAILED\n\r");
      return -1;
    }

    // setting socket option to make the socket as non blocking
    //iStatus = sl_SetSockOpt(iSockID, SL_SOL_SOCKET, SL_SO_NONBLOCKING, 
                           // &lNonBlocking, sizeof(lNonBlocking));
    UART_PRINT("Waiting for Connection......\n\r");
     if(GPIOPinRead(GPIOA0_BASE,0x8))GPIO_IF_LedOn(MCU_GREEN_LED_GPIO);
    iNewSockID = sl_Accept(iSockID, ( struct SlSockAddr_t *)&sAddr, 
                                (SlSocklen_t*)&iAddrSize);
   // g_Status = sl_SetSockOpt(iSockID, SL_SOL_SOCKET, SL_SO_NONBLOCKING, &g_NonBlocking, sizeof(g_NonBlocking));
    //if(g_Status<0)UART_PRINT("TCP_SERVER_NONBLOCKING_FAILED\n\r");
    UART_PRINT("Connection Completed\n\r");
    return SUCCESS;
}

//****************************************************************************
//
//!  \brief                     Handles TCP Socket Task
//!                                              
//! \param[in]                  pvParameters is the data passed to the Task
//!
//! \return                        None
//
//****************************************************************************
static void RecvTask(void *pvParameters){
       UART_PRINT("Enter Recv Task\n\r");
 //      while(SocketCreated);
  while(1){
      // UART_PRINT("Enter Recv Task1\n\r");
       Recv_Len=sl_Recv(iNewSockID,Recvdata,CMD_LEN_TX,0);//一旦连接断开，这个sockID就毫无意义了，所以这里也不再是挂起状态
//      SendData(Recvdata,65);       
       strncpy(TX_Buffer,Recvdata,CMD_LEN_TX);
       memset(Recvdata,0,CMD_LEN_TX);//将TCPIP接收到的数据用SPI发送之后，要清空接收缓存
       cmdReceived=1;
//       UART_PRINT(Recvdata);
//       UART_PRINT("\n\rReceive Data Complete\n\r");

  }
  
}
//****************************************************************************
//
//!  \brief                     Handles TCP Socket Task
//!                                              
//! \param[in]                  pvParameters is the data passed to the Task
//!
//! \return                        None
//
//****************************************************************************
static void TCPSocketTask(void *pvParameters)
{
     long lRetVal = -1;
 
    sTestBufLen  = TR_BUFF_SIZE;

    //filling the TCP server socket address
    sLocalAddr.sin_family = SL_AF_INET;
    sLocalAddr.sin_port = sl_Htons((unsigned short)PORT_NUM);
    sLocalAddr.sin_addr.s_addr = 0;

    // creating a TCP socket
    iSockID = sl_Socket(SL_AF_INET,SL_SOCK_STREAM, 0);
    if( iSockID < 0 )
    {
        // error
        UART_PRINT("TCP_SERVER_FAILED\n\r");
    }

    iAddrSize = sizeof(SlSockAddrIn_t);

    // binding the TCP socket to the TCP server address
    iStatus = sl_Bind(iSockID, (SlSockAddr_t *)&sLocalAddr, iAddrSize);
    if( iStatus < 0 )
    {
        // error
      sl_Close(iSockID);
      UART_PRINT("TCP_SERVER_FAILED\n\r");
    }

    // putting the socket for listening to the incoming TCP connection
    iStatus = sl_Listen(iSockID, 0);
    if( iStatus < 0 )
    {
      sl_Close(iSockID);
      UART_PRINT("TCP_SERVER_FAILED\n\r");
    }

    // setting socket option to make the socket as non blocking
    //iStatus = sl_SetSockOpt(iSockID, SL_SOL_SOCKET, SL_SO_NONBLOCKING, 
                           // &lNonBlocking, sizeof(lNonBlocking));
    UART_PRINT("Waiting for Connection......\n\r");
     if(GPIOPinRead(GPIOA0_BASE,0x8))GPIO_IF_LedOn(MCU_GREEN_LED_GPIO);
    iNewSockID = sl_Accept(iSockID, ( struct SlSockAddr_t *)&sAddr, 
                                (SlSocklen_t*)&iAddrSize);
    UART_PRINT("Connection Completed\n\r");
    SocketCreated=0;
   // SPI_Init();
        //
    // Create Recv Data Task
    //
 //   lRetVal = osi_TaskCreate(RecvTask, (signed char*)"RecvTask",
                     //    OSI_STACK_SIZE, NULL, RECV_TASK_PRIORITY, NULL );    
  //  if(lRetVal < 0)
  // {
 //       UART_PRINT("Unable to create Recv task\n\r");
        //LOOP_FOREVER();
 //   }
//   strncpy(TX_Buffer,Reply_SocketOK,13);
//    SendData(TX_Buffer,65);
   // GPIO_IF_LedOn(MCU_ORANGE_LED_GPIO);
    cmd_flag=0;
    while(1)
    {
      osi_SyncObjWait(pSyncObj_SPI,OSI_WAIT_FOREVER);//等待信号量
       if(cmd_flag==1){//如果是命令
         choosecmd();//调用处理命令函数
         cmd_flag=0;//命令标识符归零
         memset(CMDBuffer,0,CMD_LEN_RX);//cmdbuffer清零
       }else{//否则当成数据发送
        iStatus = sl_Send(iNewSockID, sindata_ptr,TR_BUFF_SIZE, 0 );// sTestBufLen
        if( iStatus <= 0 )
        {
            // error
          sl_Close(iSockID);
          UART_PRINT("TCP_CLIENT_FAILED\n\r");
        }
       // memset(sindata_ptr,0,TR_BUFF_SIZE);
       }
    
   //  UART_PRINT("Send Data Complete%d\n\r",number);
    }
}

//****************************************************************************
//
//!  \brief                     Handles cmd Task
//!                                              
//! \param[in]                  pvParameters is the data passed to the Task
//!
//! \return                        None
//
//****************************************************************************
static void main_task(void *pvParameters){
     long lRetVal = -1;
     unsigned char fn_state=0;//用于标识状态机当前的状态
     SPI_Init();//初始化SPI
     while(1){
   //   osi_SyncObjWait(pSyncObj_CMD, OSI_WAIT_FOREVER);
       if(cmdReceived){  
     if(CMDBuffer[0]==HELLO_WIFI){//收到握手命令
         //memset(CMDBuffer,0,CMD_LEN_RX);//清空命令缓冲区
         TX_Buffer[0]=ACK_HELLO;
         SendData(TX_Buffer,1);//发送回复内容
         memset(CMDBuffer,0,CMD_LEN_RX);//清空命令缓冲区
         memset(TX_Buffer,0,CMD_LEN_TX);//清空发送缓冲区
         if(fn_state==0)fn_state=1;//r如果在状态0，表明下面进入状态1
     }
     if((CMDBuffer[0]==CONFIG)&&(fn_state==1)){//收到CONFIG命令，利用SSID和PASSWARD信息创建AP
         strncpy(ConfigInformation,CMDBuffer+1,16);//将配置信息复制到ConfigInformation数组中，格式为CONFIG_SSID_PASSWEARD
          memset(CMDBuffer,0,CMD_LEN_RX);//清空命令缓冲区
         lRetVal=Creat_AP();//建立AP
         if(lRetVal<0){
 //          strncpy(TX_Buffer,Reply_APFailed,16);//如果建立不成功，回复不成功信息
           TX_Buffer[0]=ACK_CONFIG_FAILED;
           SendData(TX_Buffer,1);
           memset(TX_Buffer,0,CMD_LEN_TX);//清空发送缓冲区
         }
         else{
           //strncpy(TX_Buffer,Reply_APOK,9);//如果建立成功，回复成功信息
           TX_Buffer[0]=ACK_CONFIG_SUCCESS;
           SendData(TX_Buffer,1);
           memset(TX_Buffer,0,CMD_LEN_TX);//清空发送缓冲区
           fn_state=2;//表明进入状态2
         }
     }
     if((CMDBuffer[0]==SOCKET)&&(fn_state==2)){//收到SOCKET命令，创建SOCKET套接字
          //memset(CMDBuffer,0,CMD_LEN_RX);//清空命令缓冲区
          lRetVal=Creat_Socket();
          memset(CMDBuffer,0,CMD_LEN_RX);
          if(lRetVal<0){
           // strncpy(TX_Buffer,Reply_APFailed,16);//如果socket创建不成功，回复不成功信息
            TX_Buffer[0]= ACK_SOCKET_FAILED;
            SendData(TX_Buffer,1);
            memset(TX_Buffer,0,CMD_LEN_TX);//清空发送缓冲区
          }
          else{
       //    lRetVal = osi_TaskCreate(RecvTask, (signed char*)"RecvTask",//如果创建成功，建立等待任务
       //                  OSI_STACK_SIZE, NULL, RECV_TASK_PRIORITY, NULL );    
       //   if(lRetVal < 0)UART_PRINT("Unable to create Recv task\n\r");
          //strncpy(TX_Buffer,Reply_SocketOK,13);//并且回复创建成功
          TX_Buffer[0]=ACK_SOCKET_SUCCESS;
          SendData(TX_Buffer,1);
          memset(TX_Buffer,0,CMD_LEN_TX);//清空发送缓冲区
          fn_state=3;//表明进入状态3
          }
     }
     if((CMDBuffer[0]==SENDDATA)&&(fn_state==3)){//收到SENDDATA命令，发送数据
         memset(CMDBuffer,0,CMD_LEN_RX);//清空命令缓冲区
         iStatus = sl_Send(iNewSockID, sindata_ptr,DataCount, 0 );
         //Recv_Len =sl_Recv(iNewSockID,Recvdata,CMD_LEN_TX,0);
         //Recv_Len=strlen(Recvdata);
         //TX_Buffer[0]=Recv_Len+4;//这里将要传数据的长度放到了TX_Buffer中，如果不这样，似乎会丢一个字节
         //TX_Buffer[0]=4;//这里将要传数据的长度放到了TX_Buffer中，如果不这样，似乎会丢一个字节
         //strncpy(TX_Buffer+5,Recvdata,Recv_Len);
         //TX_Buffer[0]=(unsigned char)(DataCount);//将接收到的数据的长度的低字节放到发送缓冲区的第1个
         //TX_Buffer[1]=(unsigned char)((DataCount)>>8);//将接收到的数据长度的高字节放到发送缓冲区的第2个
         //TX_Buffer[2]=(unsigned char)(iStatus);
         //TX_Buffer[3]=(unsigned char)((iStatus)>>8);
         
         Recv_Len =sl_Recv(iNewSockID,Recvdata,CMD_LEN_TX,0);
         Recv_Len=strlen(Recvdata);
         TX_Buffer[0]=Recv_Len+4;//这里将要传数据的长度放到了TX_Buffer中，如果不这样，似乎会丢一个字节
         strncpy(TX_Buffer+1,Recvdata,Recv_Len);
         TX_Buffer[Recv_Len+1]=(unsigned char)(DataCount);//将接收到的数据的长度的低字节放到发送缓冲区的第1个
         TX_Buffer[Recv_Len+2]=(unsigned char)((DataCount)>>8);//将接收到的数据长度的高字节放到发送缓冲区的第2个
         TX_Buffer[Recv_Len+3]=(unsigned char)(iStatus);
         TX_Buffer[Recv_Len+4]=(unsigned char)((iStatus)>>8);
         
         if(DataCount!=1024)messagecount=0;
         SendData(TX_Buffer,Recv_Len+5);
         //SendData(TX_Buffer,4);
         memset(TX_Buffer,0,CMD_LEN_TX);
         memset(Recvdata,0,CMD_LEN_TX);
     }
     if((CMDBuffer[0]==GETDATA)&&(fn_state==3)){
         Recv_Len=sl_Recv(iNewSockID,Recvdata,CMD_LEN_TX,0);
         Recv_Len=strlen(Recvdata);
         //TX_Buffer[0]=Recv_Len;
         strncpy(TX_Buffer,Recvdata,Recv_Len);
         SendData(TX_Buffer,Recv_Len);
         memset(TX_Buffer,0,CMD_LEN_RX);  
         memset(Recvdata,0,CMD_LEN_TX);
     }
      cmdReceived=0;
       }
    
  }
}
//*****************************************************************************
//
//! Board Initialization & Configuration
//!
//! \param  None
//!
//! \return None
//
//*****************************************************************************
static void
BoardInit(void)
{
/* In case of TI-RTOS vector table is initialize by OS itself */
#ifndef USE_TIRTOS
  //
  // Set vector table base
  //
#if defined(ccs)
    MAP_IntVTableBaseSet((unsigned long)&g_pfnVectors[0]);
#endif
#if defined(ewarm)
    MAP_IntVTableBaseSet((unsigned long)&__vector_table);
#endif
#endif
    //
    // Enable Processor
    //
    MAP_IntMasterEnable();
    MAP_IntEnable(FAULT_SYSTICK);

    PRCMCC3200MCUInit();
}

//****************************************************************************
//                            MAIN FUNCTION
//****************************************************************************
  void main()
{
    long lRetVal = -1;
    pSyncObj_SPI = &SyncObj_SPI;
    pSyncObj_CMD = &SyncObj_CMD;

    //Board Initialization
    BoardInit();
    
    //Pin Configuration
    PinMuxConfig();
    
    //Change Pin 58 Configuration from Default to Pull Down
    MAP_PinConfigSet(PIN_58,PIN_STRENGTH_2MA|PIN_STRENGTH_4MA,PIN_TYPE_STD_PD);
    MAP_UtilsDelay(80000000);
    //
    // Initialize GREEN and ORANGE LED
    //
    GPIO_IF_LedConfigure(LED1|LED2|LED3);
    //Turn Off the LEDs
    GPIO_IF_LedOff(MCU_ALL_LED_IND);

    //UART Initialization
    MAP_PRCMPeripheralReset(PRCM_UARTA0);

    MAP_UARTConfigSetExpClk(CONSOLE,MAP_PRCMPeripheralClockGet(CONSOLE_PERIPH),
                            UART_BAUD_RATE,(UART_CONFIG_WLEN_8 | 
                              UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));

    //Display Application Banner on UART Terminal
    //DisplayBanner(APPLICATION_NAME);
    //uart initialization
    Message("UART module success\n\r");
    //spi initialization success
    //SPI_Init();
    //
    // Simplelinkspawntask
    //
    lRetVal = VStartSimpleLinkSpawnTask(SPAWN_TASK_PRIORITY);    
    if(lRetVal < 0)
    {
        UART_PRINT("Unable to start simpelink spawn task\n\r");
        LOOP_FOREVER();
    }
    //
    //Creat a sync object
    //
    lRetVal = osi_SyncObjCreate(pSyncObj_SPI);
    if(lRetVal < 0)
    {
        UART_PRINT("Unable to creat sync object SPI\n\r");
        LOOP_FOREVER();
    }
      lRetVal = osi_SyncObjCreate(pSyncObj_CMD);
    if(lRetVal < 0)
    {
        UART_PRINT("Unable to creat sync object CMD\n\r");
        LOOP_FOREVER();
    }
    //
    // Create TCP Socket Task
    //
   /* lRetVal = osi_TaskCreate(TCPSocketTask, (signed char*)"TCPSocketTask",
                         OSI_STACK_SIZE, NULL, OOB_TASK_PRIORITY, NULL );    
    if(lRetVal < 0)
   {
        UART_PRINT("Unable to create task\n\r");
        LOOP_FOREVER();
    }*/
    //
    // Create Recv Data Task
    //
   /* lRetVal = osi_TaskCreate(RecvTask, (signed char*)"RecvTask",
                         OSI_STACK_SIZE, NULL, RECV_TASK_PRIORITY, NULL );    
    if(lRetVal < 0)
   {
        UART_PRINT("Unable to create task\n\r");
        LOOP_FOREVER();
    }*/
        //
    // Create Recv Data Task
    //
   lRetVal = osi_TaskCreate(main_task, (signed char*)"main_task",
                         MAIN_STACK_SIZE, NULL, MAIN_TASK_PRIORITY, NULL );    
    if(lRetVal < 0)
   {
        UART_PRINT("Unable to create task\n\r");
        LOOP_FOREVER();
    }
    //
    // Start OS Scheduler
    //
    osi_start();

    while (1)
    {

    }

}
