// Standard includes
#include <string.h>

// Driverlib includes
#include "hw_types.h"
#include "hw_memmap.h"
#include "hw_common_reg.h"
#include "hw_ints.h"
#include "spi.h"
#include "rom.h"
#include "rom_map.h"
#include "utils.h"
#include "prcm.h"
#include "uart.h"
#include "interrupt.h"
#include "gpio_if.h"
// Common interface includes
#include "uart_if.h"

#define APPLICATION_VERSION     "1.1.0"
//*****************************************************************************
//
// Application Master/Slave mode selector macro
//
// MASTER_MODE = 1 : Application in master mode
// MASTER_MODE = 0 : Application in slave mode
//
//*****************************************************************************
#define MASTER_MODE      0

#define SPI_IF_BIT_RATE  800000 //100000
#define TR_BUFF_SIZE     1024
#define CMD_LEN_RX       64

//#define MASTER_MSG       "This is CC3200 SPI Master Application\n\r"
//#define SLAVE_MSG        "This is CC3200 SPI Slave Application\n\r"

//*****************************************************************************
//                 GLOBAL VARIABLES -- Start
//*****************************************************************************
//static unsigned char g_ucTxBuff[TR_BUFF_SIZE];
//static unsigned char g_ucRxBuff[TR_BUFF_SIZE];
//static unsigned char ucTxBuffNdx;
//static unsigned char ucRxBuffNdx;

#if defined(ccs)
extern void (* const g_pfnVectors[])(void);
#endif
#if defined(ewarm)
extern uVectorEntry __vector_table;
#endif

extern OsiSyncObj_t* pSyncObj_SPI;
extern OsiSyncObj_t* pSyncObj_CMD;
extern unsigned char datasend;
//*****************************************************************************
//
//! SPI Slave Interrupt handler
//!
//! This function is invoked when SPI slave has its receive register full or
//! transmit register empty.
//!
//! \return None.
//
//*****************************************************************************
unsigned int messagecount=0;             //用于记录接收到的数据的长度
unsigned int DataCountBuffer[2]={0,0};   //用于存储两个数据缓冲区中实际数据的数量
unsigned int *pDataCount=DataCountBuffer;//用于指向当前缓冲区长度变量
unsigned int DataCount=0;
unsigned char sindata[TR_BUFF_SIZE];    //乒乓缓存区
//unsigned char sindata1[TR_BUFF_SIZE];   //乒乓缓存区
unsigned char *sindata_ptr=sindata;     //用于指向当前存满的缓存区,另一个缓存区在继续存数
unsigned char sindata_flag=0;           //用于标识当前数据缓存在哪个区
unsigned char cmdcount=0;               //用于记录接收到的数据的长度
unsigned char CMDBuffer[CMD_LEN_RX];    //用于接收PLAB发送过来的命令
unsigned char cmd_flag=0;               //用于指示主程序当前数据是命令还是数据
unsigned char Buffer1_full=0;
unsigned char Buffer2_full=0;
extern unsigned char cmdReceived;
static void SlaveIntHandler()
{
    unsigned long ulRecvData;
    unsigned long ulStatus;
    long lRetVal = -1;
    //ulStatus = MAP_SPIIntStatus(GSPI_BASE,true);

    MAP_SPIIntClear(GSPI_BASE,SPI_INT_RX_FULL);//|SPI_INT_TX_EMPTY
//TX empty中断处理，拉低64号管脚
 //  if(ulStatus & SPI_INT_TX_EMPTY)
 //   {
//     GPIO_IF_LedOff(MCU_RED_LED_GPIO); //64号管脚拉低，表明slave无数据发送
 //    datasend=0;
//    }
//RX full中断处理
//    if(ulStatus & SPI_INT_RX_FULL)
 //   {
      if(GPIOPinRead(GPIOA1_BASE,0x4)){//this is a cmd
//        cmd_flag=1;//说明有命令来了
        MAP_SPIDataGet(GSPI_BASE,&ulRecvData);
        CMDBuffer[cmdcount]=ulRecvData;
        cmdcount++;
        cmdcount=cmdcount&0x3F;
        if((ulRecvData&0xff)==0x0D){//因为ulRecvData是一个32位寄存器，接收到的数据不断往高位移动
          cmdcount=0;//首先计数清零，保证下一次数据的接受顺序
//          lRetVal = osi_SyncObjSignal(pSyncObj_CMD);//产生信号量
          if(lRetVal <0 )Message("Fail to creat sync Signal\n\r");
          cmdReceived=1;
        }
        if(cmdcount==CMD_LEN_RX)cmdcount=0;
      }else{//this is data
      MAP_SPIDataGet(GSPI_BASE,&ulRecvData);//从RX buffer取数
        sindata[messagecount]=ulRecvData;
        messagecount++;
        DataCount=messagecount;
        if(messagecount==TR_BUFF_SIZE)messagecount=0;
 /*     if(sindata_flag==0){//查看标志位，确定数据放在哪个缓冲区
        sindata[messagecount]=ulRecvData;
        DataCountBuffer[0]=messagecount+1;//记录当前缓冲区数据的数量
      }
      else {
        sindata1[messagecount]=ulRecvData;
        DataCountBuffer[1]=messagecount+1;//记录当前缓冲区数据的数量
      }
        messagecount++;//接收数据长度加1
        if(messagecount==TR_BUFF_SIZE){
          messagecount=0;
          if(sindata_flag==0){
            sindata_flag=1;
            sindata_ptr=sindata;
   //         pDataCount=DataCountBuffer;
            Buffer1_full=1;
          }
          else{
            sindata_flag=0;
            sindata_ptr=sindata1;
 //           pDataCount=DataCountBuffer+1;
            Buffer2_full=1;
          }
//        lRetVal = osi_SyncObjSignal(pSyncObj_SPI);
 //       if(lRetVal <0 ){
  //        Message("Fail to creat sync Signal\n\r");
 //         }
        }*/
      }
 //   }
}

static void SlaveIntHandler1()
{
    MAP_SPIIntClear(GSPI_BASE,SPI_INT_TX_EMPTY);//
//TX empty中断处理，拉低64号管脚
 //    GPIO_IF_LedOff(MCU_RED_LED_GPIO); //64号管脚拉低，表明slave无数据发送
     datasend=0;

}
//*****************************************************************************
//
//! SPI Slave mode main loop
//!
//! This function configures SPI modelue as slave and enables the channel for
//! communication
//!
//! \return None.
//
//*****************************************************************************
void SlaveMain()
{
    //
    // Initialize the message
    //
   // memcpy(g_ucTxBuff,SLAVE_MSG,sizeof(SLAVE_MSG));

    //
    // Set Tx buffer index
    //
  //  ucTxBuffNdx = 0;
  //  ucRxBuffNdx = 0;

    //
    // Reset SPI
    //
    MAP_SPIReset(GSPI_BASE);

    //
    // Configure SPI interface
    //SPI_CS_ACTIVEHIGH
    MAP_SPIConfigSetExpClk(GSPI_BASE,MAP_PRCMPeripheralClockGet(PRCM_GSPI),
                     SPI_IF_BIT_RATE,SPI_MODE_SLAVE,SPI_SUB_MODE_0,
                     (SPI_HW_CTRL_CS |
                     SPI_4PIN_MODE |
                     SPI_TURBO_OFF |
                     SPI_CS_ACTIVEHIGH |
                     SPI_WL_8));

    //
    // Register Interrupt Handler
    //
    MAP_SPIIntRegister(GSPI_BASE,SlaveIntHandler);

    //
    // Enable Interrupts
    //
    MAP_SPIIntEnable(GSPI_BASE,SPI_INT_RX_FULL);//

    //
    // Enable SPI for communication
    //
    MAP_SPIEnable(GSPI_BASE);

    //
    // Print mode on uart
    //
    Message("SPI module success\n\r");
}

void SlaveMain1()
{
    //
    // Initialize the message
    //
   // memcpy(g_ucTxBuff,SLAVE_MSG,sizeof(SLAVE_MSG));

    //
    // Set Tx buffer index
    //
   // ucTxBuffNdx = 0;
   // ucRxBuffNdx = 0;

    //
    // Reset SPI
    //
    MAP_SPIReset(GSPI_BASE);

    //
    // Configure SPI interface
    //SPI_CS_ACTIVEHIGH
    MAP_SPIConfigSetExpClk(GSPI_BASE,MAP_PRCMPeripheralClockGet(PRCM_GSPI),
                     SPI_IF_BIT_RATE,SPI_MODE_SLAVE,SPI_SUB_MODE_0,
                     (SPI_HW_CTRL_CS |
                     SPI_4PIN_MODE |
                     SPI_TURBO_OFF |
                     SPI_CS_ACTIVEHIGH |
                     SPI_WL_8));

    //
    // Register Interrupt Handler
    //
    MAP_SPIIntRegister(GSPI_BASE,SlaveIntHandler1);

    //
    // Enable Interrupts
    //
    MAP_SPIIntEnable(GSPI_BASE,SPI_INT_TX_EMPTY);//SPI_INT_RX_FULL|

    //
    // Enable SPI for communication
    //
    MAP_SPIEnable(GSPI_BASE);

    //
    // Print mode on uart
    //
    Message("SPI module success\n\r");
}

//*****************************************************************************
//SPI Init
//*****************************************************************************
void SPI_Init(){
     //
    // Enable Peripheral Clocks 
    //
    MAP_PRCMPeripheralClkEnable(PRCM_GSPI, PRCM_RUN_MODE_CLK);
    //
    // Configure PIN_55 for UART0 UART0_TX
    //
    MAP_PinTypeUART(PIN_55, PIN_MODE_3);

    //
    // Configure PIN_57 for UART0 UART0_RX
    //
    MAP_PinTypeUART(PIN_57, PIN_MODE_3);

    //
    // Configure PIN_05 for SPI0 GSPI_CLK
    //
    MAP_PinTypeSPI(PIN_05, PIN_MODE_7);

    //
    // Configure PIN_06 for SPI0 GSPI_MISO
    //
    MAP_PinTypeSPI(PIN_06, PIN_MODE_7);

    //
    // Configure PIN_07 for SPI0 GSPI_MOSI
    //
    MAP_PinTypeSPI(PIN_07, PIN_MODE_7);

    //
    // Configure PIN_08 for SPI0 GSPI_CS
    //
    MAP_PinTypeSPI(PIN_08, PIN_MODE_7);
      // Enable the SPI module clock
    //
    MAP_PRCMPeripheralClkEnable(PRCM_GSPI,PRCM_RUN_MODE_CLK);
//    Message("SPI module success");
    MAP_PRCMPeripheralReset(PRCM_GSPI);
    SlaveMain();
}

