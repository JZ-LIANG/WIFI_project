#include <stdlib.h>
#include <string.h>

// Simplelink includes
#include "simplelink.h"

//Driverlib includes
#include "hw_types.h"
#include "hw_ints.h"
#include "rom.h"
#include "rom_map.h"
#include "interrupt.h"
#include "prcm.h"

//Common interface includes
#include "gpio_if.h"
#include "common.h"
#ifndef NOTERM
#include "uart_if.h"
#endif
#include "pinmux.h"

#define SL_MAX_FILE_SIZE        220*409//64L*1024L       /* 64KB file */
#define BUF_SIZE                2048
#define USER_FILE_NAME          "www/image/1.bmp"
#define EXIST_FILE              "www/image/2.bmp"

SlFsFileInfo_t *pFsFileInfo; 
SlFsFileInfo_t File_info;
struct tagBITMAPFILEHEADER *bmp_p;  //定义bmp文件头结构体指针
unsigned char gaucCmpBuf[BUF_SIZE];

extern unsigned char sindata[409];
unsigned char picdata[220];

typedef unsigned char byte;
typedef unsigned short dbyte;
typedef unsigned int dword;
typedef unsigned short word;

struct tagBITMAPFILEHEADER {
    //bmp file header
    dbyte bfType;        //文件类型
    word bfSizelow;            //文件大小，字节为单位
    word bfSizehigh;
    word bfReserved1;   //保留，必须为0
    word bfReserved2;   //保留，必须为0
    word bfOffBitslow;         //从文件头开始的偏移量
    word bfOffBitshigh;  

    //bmp info head
    word  biSizelow;            //该结构的大小
    word  biSizehigh;
    word  biWidthlow;           //图像的宽度，以像素为单位
    word  biWidthhigh;
    word  biHeightlow;          //图像的高度，以像素为单位
    word  biHeighthigh;
    word biPlanes;          //为目标设备说明位面数，其值总是设为1
    word biBitCount;        //说明比特数/像素
    word biCompressionlow;     //图像数据压缩类型
    word biCompressionhigh;
    word biSizeImagelow;       //图像大小，以字节为单位
    word biSizeImagehigh;
    word biXPelsPerMeterlow;   //水平分辨率，像素/米
    word biXPelsPerMeterhigh;
    word biYPelsPerMeterlow;   //垂直分辨率，同上
    word biYPelsPerMeterhigh;
    word biClrUsedlow;         //位图实际使用的彩色表中的颜色索引数
    word biClrUsedhigh;
    word biClrImportantlow;    //对图像显示有重要影响的颜色索引的数目
    word biClrImportanthigh;  
    //bmp rgb quad
     //对于16位，24位，32位的位图不需要色彩表
    unsigned char rgbBlue0;    //指定蓝色强度
    unsigned char rgbGreen0;   //指定绿色强度
    unsigned char rgbRed0;     //指定红色强度
    unsigned char rgbReserved0; //保留，设置为0 
    
    unsigned char rgbBlue1;    //指定蓝色强度
    unsigned char rgbGreen1;   //指定绿色强度
    unsigned char rgbRed1;     //指定红色强度
    unsigned char rgbReserved1; //保留，设置为0 
    
    unsigned char rgbBlue2;    //指定蓝色强度
    unsigned char rgbGreen2;   //指定绿色强度
    unsigned char rgbRed2;     //指定红色强度
    unsigned char rgbReserved2; //保留，设置为0 
    
    unsigned char rgbBlue3;    //指定蓝色强度
    unsigned char rgbGreen3;   //指定绿色强度
    unsigned char rgbRed3;     //指定红色强度
    unsigned char rgbReserved3; //保留，设置为0 
    
    unsigned char rgbBlue4;    //指定蓝色强度
    unsigned char rgbGreen4;   //指定绿色强度
    unsigned char rgbRed4;     //指定红色强度
    unsigned char rgbReserved4; //保留，设置为0 
    
    unsigned char rgbBlue5;    //指定蓝色强度
    unsigned char rgbGreen5;   //指定绿色强度
    unsigned char rgbRed5;     //指定红色强度
    unsigned char rgbReserved5; //保留，设置为0 
    
    unsigned char rgbBlue6;    //指定蓝色强度
    unsigned char rgbGreen6;   //指定绿色强度
    unsigned char rgbRed6;     //指定红色强度
    unsigned char rgbReserved6; //保留，设置为0 
    
    unsigned char rgbBlue7;    //指定蓝色强度
    unsigned char rgbGreen7;   //指定绿色强度
    unsigned char rgbRed7;     //指定红色强度
    unsigned char rgbReserved7; //保留，设置为0 
    
    unsigned char rgbBlue8;    //指定蓝色强度
    unsigned char rgbGreen8;   //指定绿色强度
    unsigned char rgbRed8;     //指定红色强度
    unsigned char rgbReserved8; //保留，设置为0 
    
    unsigned char rgbBlue9;    //指定蓝色强度
    unsigned char rgbGreen9;   //指定绿色强度
    unsigned char rgbRed9;     //指定红色强度
    unsigned char rgbReserved9; //保留，设置为0 
    
    unsigned char rgbBlue10;    //指定蓝色强度
    unsigned char rgbGreen10;   //指定绿色强度
    unsigned char rgbRed10;     //指定红色强度
    unsigned char rgbReserved10; //保留，设置为0 
    
    unsigned char rgbBlue11;    //指定蓝色强度
    unsigned char rgbGreen11;   //指定绿色强度
    unsigned char rgbRed11;     //指定红色强度
    unsigned char rgbReserved11; //保留，设置为0 
    
    unsigned char rgbBlue12;    //指定蓝色强度
    unsigned char rgbGreen12;   //指定绿色强度
    unsigned char rgbRed12;     //指定红色强度
    unsigned char rgbReserved12; //保留，设置为0 
    
    unsigned char rgbBlue13;    //指定蓝色强度
    unsigned char rgbGreen13;   //指定绿色强度
    unsigned char rgbRed13;     //指定红色强度
    unsigned char rgbReserved13; //保留，设置为0 
    
    unsigned char rgbBlue14;    //指定蓝色强度
    unsigned char rgbGreen14;  //指定绿色强度
    unsigned char rgbRed14;     //指定红色强度
    unsigned char rgbReserved14; //保留，设置为0 
    
    unsigned char rgbBlue15;    //指定蓝色强度
    unsigned char rgbGreen15;   //指定绿色强度
    unsigned char rgbRed15;     //指定红色强度
    unsigned char rgbReserved15; //保留，设置为0 
}BMPFILEHEADER;

typedef enum{
    // Choosing this number to avoid overlap w/ host-driver's error codes
    FILE_ALREADY_EXIST = -0x7D0,
    FILE_CLOSE_ERROR = FILE_ALREADY_EXIST - 1,
    FILE_NOT_MATCHED = FILE_CLOSE_ERROR - 1,
    FILE_OPEN_READ_FAILED = FILE_NOT_MATCHED - 1,
    FILE_OPEN_WRITE_FAILED = FILE_OPEN_READ_FAILED -1,
    FILE_READ_FAILED = FILE_OPEN_WRITE_FAILED - 1,
    FILE_WRITE_FAILED = FILE_READ_FAILED - 1,

    //STATUS_CODE_MAX = -0xBB8
}e_AppStatusCodes_1;

//*****************************************************************************
//
//!  This funtion includes the following steps:
//!  -open a user file for writing
//!  -write "Old MacDonalds" child song 37 times to get just below a 64KB file
//!  -close the user file
//!
//!  /param[out] ulToken : file token
//!  /param[out] lFileHandle : file handle
//!
//!  /return  0:Success, -ve: failure
//
//*****************************************************************************  
long WriteFileToDevice(unsigned char *p2data)
{
    bmp_p = &BMPFILEHEADER;//给指针赋值
    long lRetVal = -1;
    unsigned long int iLoopCnt = 0,sincount=0;
    unsigned long int *ulToken,Token;
    long int *lFileHandle,FileHandle;
    ulToken=&Token;
    lFileHandle=&FileHandle;
    pFsFileInfo=&File_info;
   // UART_PRINT("sl_start will start in writeDevice\n\r");
  //  sl_Start(0, 0, 0);
   // UART_PRINT("sl_Start in writeDevice\n\r");
    //打开预存的bmp文件，读模式
    lRetVal = sl_FsOpen((unsigned char *)EXIST_FILE,
                       FS_MODE_OPEN_READ,
                        ulToken,
                        lFileHandle);
      if(lRetVal < 0)
    {
        lRetVal = sl_FsClose(*lFileHandle, 0, 0, 0);
    //    ASSERT_ON_ERROR(FILE_OPEN_READ_FAILED);
    }
    //读出预存bmp文件头，存在bmp_p中
    lRetVal = sl_FsRead(*lFileHandle,0,(unsigned char *)bmp_p, 118);
        if ((lRetVal < 0))
        {
            lRetVal = sl_FsClose(*lFileHandle, 0, 0, 0);
         //   ASSERT_ON_ERROR(FILE_READ_FAILED);
            
        }  
    //关闭预存文件    
    lRetVal = sl_FsClose(*lFileHandle, 0, 0, 0);
    if (SL_RET_CODE_OK != lRetVal)
    {
     //   ASSERT_ON_ERROR(FILE_CLOSE_ERROR);
    }
    //打开需要修改的bmp文件,写模式
    sl_FsGetInfo("www/image/1.bmp",0,pFsFileInfo);
    lRetVal = sl_FsOpen((unsigned char *)USER_FILE_NAME,
                        FS_MODE_OPEN_WRITE, 
                        ulToken,
                        lFileHandle);
    if(lRetVal < 0)
    {
        lRetVal = sl_FsClose(*lFileHandle, 0, 0, 0);
     //   ASSERT_ON_ERROR(FILE_OPEN_WRITE_FAILED);
    }
    //写入bmp文件头
    lRetVal = sl_FsWrite(*lFileHandle,
                    (unsigned int)(iLoopCnt ),// * sizeof(gaucOldMacDonald)
                    (unsigned char *)bmp_p, 118);
        if (lRetVal < 0)
        {
            lRetVal = sl_FsClose(*lFileHandle, 0, 0, 0);
       //     ASSERT_ON_ERROR(FILE_WRITE_FAILED);
        }
    //写入bmp文件内容
        for(iLoopCnt = 0;iLoopCnt<409;iLoopCnt++){
          for(sincount=0;sincount<220;sincount++)picdata[sincount]=0xff;
         picdata[(sindata[iLoopCnt]>>1)+50]=0x00;
          lRetVal = sl_FsWrite(*lFileHandle,
                    (unsigned int)(iLoopCnt*220+118 ),// * sizeof(gaucOldMacDonald)
                   picdata , 220);//p2data
            if (lRetVal < 0)
        {
            lRetVal = sl_FsClose(*lFileHandle, 0, 0, 0);
      //      ASSERT_ON_ERROR(FILE_WRITE_FAILED);
        }
        }
     //关闭文件
    lRetVal = sl_FsClose(*lFileHandle, 0, 0, 0);
    if (SL_RET_CODE_OK != lRetVal)
    {
    //    ASSERT_ON_ERROR(FILE_CLOSE_ERROR);
    }   
    //打开文件，读模式
        lRetVal = sl_FsOpen((unsigned char *)USER_FILE_NAME,
                       FS_MODE_OPEN_READ,
                        ulToken,
                        lFileHandle);
    if(lRetVal < 0)
    {
        lRetVal = sl_FsClose(*lFileHandle, 0, 0, 0);
      //  ASSERT_ON_ERROR(FILE_OPEN_READ_FAILED);
    }
    //读文件
    lRetVal = sl_FsRead(*lFileHandle,0,gaucCmpBuf, 2048);
     if ((lRetVal < 0))
     {
      lRetVal = sl_FsClose(*lFileHandle, 0, 0, 0);
   //   ASSERT_ON_ERROR(FILE_READ_FAILED);
            
      }
    //关闭文件    
    lRetVal = sl_FsClose(*lFileHandle, 0, 0, 0);
    if (SL_RET_CODE_OK != lRetVal)
    {
     //   ASSERT_ON_ERROR(FILE_CLOSE_ERROR);
    }
   // sl_Stop(200);
   // UART_PRINT("sl_Stop in writeDevice\n\r");
    return SUCCESS;
}