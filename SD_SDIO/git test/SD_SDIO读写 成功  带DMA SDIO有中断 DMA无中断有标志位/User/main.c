/**
  ******************************************************************************
  * @file    Project/STM32F10x_StdPeriph_Template/main.c 
  * @author  MCD Application Team
  * @version V3.5.0
  * @date    08-April-2011
  * @brief   Main program body
  ******************************************************************************
  * @attention
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2011 STMicroelectronics</center></h2>
  ******************************************************************************
  */  

/* Includes ------------------------------------------------------------------*/
#include "stm32f10x.h"

#include <stdio.h>
#include <bsp_sdio_sdcard.h>
#include <bsp_usart1.h>
#define  u8	uint8_t  
#define  u16	uint16_t
#define uchar unsigned char
#define uint unsigned int


#define BLOCK_SIZE            512 /* Block Size in Bytes */

#define NUMBER_OF_BLOCKS      32  /* For Multi Blocks operation (Read/Write) */


#define NUMBER_OF_BLOCKS      32  /* For Multi Blocks operation (Read/Write) */
#define MULTI_BUFFER_SIZE    (BLOCK_SIZE * NUMBER_OF_BLOCKS)   //缓冲区大小	 


typedef enum {FAILED = 0, PASSED = !FAILED} TestStatus;
extern SD_CardInfo SDCardInfo;	

SD_Error Status = SD_OK;
//uchar Buffercmp(u8 *pBuffer1,u8 *pBuffer2,u16 BufferLength);
uint8_t Buffer_MultiBlock_Tx[MULTI_BUFFER_SIZE], Buffer_MultiBlock_Rx[MULTI_BUFFER_SIZE];

uint8_t Buffer_Block_Tx[BLOCK_SIZE], Buffer_Block_Rx[BLOCK_SIZE];


volatile TestStatus EraseStatus = FAILED, TransferStatus1 = FAILED, TransferStatus2 = FAILED;
uchar send_data[1536],receive_data[1536];			//未进行初始化，在程序内进行初始化

/**
  * @brief  主函数
  * @param  无
  * @retval 无
  */

/*
 * 函数名：eBuffercmp
 * 描述  ：检查缓冲区的数据是否为0
 * 输入  ：-pBuffer 要比较的缓冲区
 *         -BufferLength 缓冲区长度        
 * 输出  ：PASSED 缓冲区的数据全为0
 *         FAILED 缓冲区的数据至少有一个不为0 
 */
TestStatus eBuffercmp(uint8_t* pBuffer, uint32_t BufferLength)
{
  while (BufferLength--)
  {
    /* In some SD Cards the erased state is 0xFF, in others it's 0x00 */
    if ((*pBuffer != 0xFF) && (*pBuffer != 0x00))//擦除后是0xff或0x00
    {
      return FAILED;
    }

    pBuffer++;
  }

  return PASSED;
}


/*
 * 函数名：Buffercmp
 * 描述  ：比较两个缓冲区中的数据是否相等
 * 输入  ：-pBuffer1, -pBuffer2 : 要比较的缓冲区的指针
 *         -BufferLength 缓冲区长度
 * 输出  ：-PASSED 相等
 *         -FAILED 不等
 */
TestStatus Buffercmp(uint8_t* pBuffer1, uint8_t* pBuffer2, uint32_t BufferLength)
{
  while (BufferLength--)
  {
    if (*pBuffer1 != *pBuffer2)
    {
      return FAILED;
    }

    pBuffer1++;
    pBuffer2++;
  }

  return PASSED;
}

/*
 * 函数名：Fill_Buffer
 * 描述  ：填充数组
 * 输入  ：无
 * 输出  ：一个填充好的数组
 */
//定义一个数组的首地址  ，在定义该数组的长度，在定义该数组的起始物理地址
void Fill_Buffer(uint8_t *pBuffer, uint32_t BufferLength, uint32_t Offset)	
{
  uint16_t index = 0;

  /* Put in global buffer same values */
  for (index = 0; index < BufferLength; index++ )
  {
    pBuffer[index] = index + Offset;
  }
}
/*
 * 函数名：SD_EraseTest
 * 描述  ：擦除数据测试
 * 输入  ：无
 * 输出  ：无
 */
void SD_EraseTest(void)
{
  if (Status == SD_OK)
  {    
		/* 第一个参数为擦除起始地址，第二个参数为擦除结束地址 */
    Status = SD_Erase(0x00, (BLOCK_SIZE * NUMBER_OF_BLOCKS));
  }
	
	printf("\r\n 进入擦除函数 " );
  if (Status == SD_OK)
  {	/* 读取刚刚擦除的区域 */
    Status = SD_ReadMultiBlocks(Buffer_MultiBlock_Rx, 0x00, BLOCK_SIZE, NUMBER_OF_BLOCKS);

    /* 查询传输是否结束 */
    Status = SD_WaitReadOperation(); 		//因为没有配置SDIO中断 ，所以在这里会被卡死
    while(SD_GetStatus() != SD_TRANSFER_OK);
  }
 
  if (Status == SD_OK)
  {	/* 把擦除区域读出来对比 */
		printf("\r\n 正在进行对比！ " );
    EraseStatus = eBuffercmp(Buffer_MultiBlock_Rx, MULTI_BUFFER_SIZE);
  }
  
  if(EraseStatus == PASSED)
  	printf("\r\n 擦除测试成功！ " );
 
  else	  
  	printf("\r\n 擦除测试失败！ " );  
}

/*
 * 函数名：SD_SingleBlockTest
 * 描述  ：	单个数据块读写测试
 * 输入  ：无
 * 输出  ：无
 */
void SD_SingleBlockTest(void)
{  
  /* Fill the buffer to send */
  Fill_Buffer(Buffer_Block_Tx, BLOCK_SIZE, 0x320F);

  if (Status == SD_OK)
  {
    /* Write block of 512 bytes on address 0 */
    Status = SD_WriteBlock(Buffer_Block_Tx, 0x00, BLOCK_SIZE);
		
    /* Check if the Transfer is finished */
    Status = SD_WaitWriteOperation();	  
    while(SD_GetStatus() != SD_TRANSFER_OK); 
  }

  if (Status == SD_OK)
  {
    /* Read block of 512 bytes from address 0 */
    Status = SD_ReadBlock(Buffer_Block_Rx, 0x00, BLOCK_SIZE);//读取数据
    /* Check if the Transfer is finished */
    Status = SD_WaitReadOperation();
    while(SD_GetStatus() != SD_TRANSFER_OK);
  }

  /* Check the correctness of written data */
  if (Status == SD_OK)
  {
    TransferStatus1 = Buffercmp(Buffer_Block_Tx, Buffer_Block_Rx, BLOCK_SIZE);	//比较
  }
  
  if(TransferStatus1 == PASSED)
    printf("\r\n 单块读写测试成功！" );
 
  else  
  	printf("\r\n 单块读写测试失败！ " );  
}

/*
 * 函数名：SD_MultiBlockTest
 * 描述  ：	多数据块读写测试
 * 输入  ：无
 * 输出  ：无
 */
void SD_MultiBlockTest(void)
{
  /* Fill the buffer to send */
	u16 index=0;
  Fill_Buffer(Buffer_MultiBlock_Tx, MULTI_BUFFER_SIZE, 0x0);

  if (Status == SD_OK)
  {
    /* Write multiple block of many bytes on address 0 */
    Status = SD_WriteMultiBlocks(Buffer_MultiBlock_Tx, 0x00, BLOCK_SIZE, NUMBER_OF_BLOCKS);
    /* Check if the Transfer is finished */
    Status = SD_WaitWriteOperation();
    while(SD_GetStatus() != SD_TRANSFER_OK);
		
		for(index=0;index< BLOCK_SIZE;index++)
		{
			printf("\r\n 发送缓冲区内的数据为 %d",Buffer_MultiBlock_Tx[index] );
		}
		printf("\r\n");
		
  }

  if (Status == SD_OK)
  {
    /* Read block of many bytes from address 0 */
    Status = SD_ReadMultiBlocks(Buffer_MultiBlock_Rx, 0x00, BLOCK_SIZE, NUMBER_OF_BLOCKS);
    /* Check if the Transfer is finished */
    Status = SD_WaitReadOperation();
    while(SD_GetStatus() != SD_TRANSFER_OK);
		
		for(index=0;index< BLOCK_SIZE;index++)
		{
			printf("\r\n 接收缓冲区内的数据为 %d",Buffer_MultiBlock_Rx[index] );
		}
		printf("\r\n");
  }

  /* Check the correctness of written data */
  if (Status == SD_OK)
  {
    TransferStatus2 = Buffercmp(Buffer_MultiBlock_Tx, Buffer_MultiBlock_Rx, MULTI_BUFFER_SIZE);
  }
  
  if(TransferStatus2 == PASSED)	  
  	printf("\r\n 多块读写测试成功！ " );

  else 
  	printf("\r\n 多块读写测试失败！ " );  

}



void Delay_Ms( u16 dly)
{
	u16 i,j;
	for(i=0;i<dly;i++)
	for(j=1000;j>0;j--);
}

	u8 ID_num[12];	
	u8 Tab[]="hello every body";
	
	/*-------------USART重映射---------*/
int main(void)
{								
	NVIC_Configuration();
	USART1_Config();
	switch(SD_Init())
	{
	case 0:
		printf(" SD Card Init Successfully\r\n");
		break;
	
	case 1:
				printf("Init process is going wrong...");
		break;
	default:
			printf("  请重试");
	break;
	}
	
	printf( " \r\n CardType is ：%d ", SDCardInfo.CardType );
	printf( " \r\n CardCapacity is ：%d ", SDCardInfo.CardCapacity );
	printf( " \r\n CardBlockSize is ：%d ", SDCardInfo.CardBlockSize );
	printf( " \r\n RCA is ：%d ", SDCardInfo.RCA);
	printf( " \r\n ManufacturerID is ：%d \r\n", SDCardInfo.SD_cid.ManufacturerID );
	/* 擦除测试 */
	printf("  \r\n即将进行擦除扇区测试\r\n");
	SD_EraseTest();
	
  /* 单块读写测试 */
		printf("  \r\n 即将进行单块读写测试r\n");
	SD_SingleBlockTest(); 
	
	/* 多块读写测试 */
	printf("  \r\n 即将进行多块读写测试r\n");	
	SD_MultiBlockTest();  
}

/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/
