/**
  ******************************************************************************
  * @file 	 app.h
  * @brief 	 The header file of the APP library.
  * @author  [FangJian]Holtek Semiconductor Inc.
  * @version V1.0.0
  * @date 	 2020-03-27
  ******************************************************************************
  * @attention
  *
  * Firmware Disclaimer Information
  *
  * 1. The customer hereby acknowledges and agrees that the program technical documentation, including the
  *    code, which is supplied by Holtek Semiconductor Inc., (hereinafter referred to as "HOLTEK") is the
  *    proprietary and confidential intellectual property of HOLTEK, and is protected by copyright law and
  *    other intellectual property laws.
  *
  * 2. The customer hereby acknowledges and agrees that the program technical documentation, including the
  *    code, is confidential information belonging to HOLTEK, and must not be disclosed to any third parties
  *    other than HOLTEK and the customer.
  *
  * 3. The program technical documentation, including the code, is provided "as is" and for customer reference
  *    only. After delivery by HOLTEK, the customer shall use the program technical documentation, including
  *    the code, at their own risk. HOLTEK disclaims any expressed, implied or statutory warranties, including
  *    the warranties of merchantability, satisfactory quality and fitness for a particular purpose.
  *
  * <h2><center>Copyright (C) Holtek Semiconductor Inc. All rights reserved</center></h2>
  ************************************************************************************************************/

/* Define to prevent recursive inclusion -------------------------------------*/

#ifndef _APP_H_
#define	_APP_H_



#define _BS66F350	(1)   // 1: BS66F350;  0: BS66F350C
//#define BS66F350C (1)

#if (_BS66F350)
#include  "BS66F350.H" 
#define AD_REG_VAL  	((u16)_adrh<<8) | _adrl;
#define AD_CHANNEL(ANx)	 _adcr0 &= 0xF0; _adcr0 |= ANx;
#else
#include <BS66F350C.H>
#define AD_REG_VAL  	((u16)_sad0h<<8) | _sad0l;
#define AD_CHANNEL(ANx)	 _sadc0 &= 0xF0; _sadc0 |= ANx;
#endif
 
typedef unsigned long  	u32;
typedef unsigned short 	u16;
typedef unsigned char  	u8;

#ifdef bit
#include <stdio.h>
#include <stdlib.h>
#define bit u8
#endif


extern volatile u16 Timestamp;
extern volatile u16 stopDownCount;  // 工作停止倒计时
#define STOP_DOWN_TIME  	(40*60) // 40分钟
#define FAN_DOWN_TIME  		(3*60)	// 3分钟
#define DOWN_COUNT_TIME		(STOP_DOWN_TIME + FAN_DOWN_TIME)		// 40分钟倒计时+ 3分钟结束风扇散热
//=============================================================
typedef enum
{
  FALSE = 0,
  TRUE = 1,
}boolType;

typedef enum 
{
  RESET = 0,
  SET = 1,
}FlagStatus, ITStatus;

typedef enum 
{
  DISABLE = 0,
  ENABLE = 1,
}FunctionalState;

typedef enum 
{
  ERROR = 0,
  SUCCESS = 1,
}ErrorStatus;

//=============================================================
// LED PIN
#define LED9	_pd7
#define LED10	_pd5
#define LED11	_pa4
#define LED12	_pa6
#define LED13	_pb0
#define LED14	_pb6
#define LED15	_pd6
#define LED16	_pe0

#define LED_ON	 0
#define LED_OFF  1

//====================
#define RLY		_pb4
#define FAN		_pb5
#define SEG5	_pe1
#define SEG6	_pe2

#define RLY_ON	 0
#define RLY_OFF  1
#define FAN_ON	 0
#define FAN_OFF  1

//====================
#define NTCC     _pbc3 
#define NTCC_EN	 NTCC = 0, _pb3 = 1      // 高温需要使能；进行NTC分压采集
#define NTCC_DIS NTCC = 1, _pbpu3 = 0    // 低温可以不使能

// 数码管引脚
#define IOA		_pa5
#define IOB		_pe7
#define IOC		_pe5
#define IOD		_pa2
#define IOE		_pa1
#define IOF		_pa7
#define	IOG		_pe4
#define IODP	_pa0
#define COM1	_pe3
#define COM2	_pe6
#define COM3	_pb7
#define COM4	_pa3

#define COM_TYPE 1	// 1: 共阳； 0: 共阴
#define COM_ON	 0	// COM ON
#define COM_OFF	 1	// COM OFF
#define IOX_ON	 0
#define IOX_OFF	 1

//=============================================================
/* 蜂鸣器 */
#define BUZZ_POWER_ON		  1
#define BUZZ_PREHEAT		  2
#define BUZZ_PREHEAT_OVER	3
#define BUZZ_TIME_OVER		4
#define BUZZ_POWER_OFF		5
#define BUZZ_4K1S			    6

extern volatile u16 buzzONTime;
extern volatile u8  buzzType;
void BUZZ_ActionType(void);

//#define BUZZ_ON() 	{ _ctm1al = 512&0xFF; _ctm1ah = 512 >> 8; buzzONTime = 100;}	
//#define BUZZ_OFF() 	{ _ctm1al = 0; _ctm1ah = 0; buzzONTime = 0;}
//=============================================================
#define NTC_ANOMALY 	1  	// NCT 异常1：开启		
/* 换算公式：ADC = VCC*NTC(R) / 10K+NTC(R) */ 
#define MCU_VCCA		    	(int)500				// MCU VCCA 5.00V (放大100倍)
#define ADC_RESOLUTION  	(int)4096				// ADC 采样分辨率 2^12
#define ADC_FILTER_RATIO	(int)200				// ADC 采样消抖滤波比(ADC_RESOLUTION*5%)；为了省内存自己换算吧！
#define NTC_DIV_RESH 		  (long)2152			// NTC 高温分压电阻/R  (1000k与2.2K分压)
#define NTC_DIV_RESL 		  (long)100000		// NTC 低温分压电阻/R
#define NTC_FUN				    (0)						  // NTC 温度测量方式

#if NTC_FUN	// 计算出NTC理论计数值 与 AD电压测量值进行比较(优点：直观得出温度值对应的AD电压值； 缺点：会被MCU_VCCA波动干扰；会存在误差)
#define NTC_TEMP2ADC(Rt)	 ((long)Rt*MCU_VCCA)/((long)NTC_DIV_RES+Rt)	// NTC 温度转电压值; Rt： NTC温度下的电阻值/R 
#else   // NTC温度只与 ADC_RESOLUTION 有关; 与MCU_VCCA无关！（优点：误差小；计算量小）
#define NTC_TEMP2ADC(DivRes, Rt)	 ((long)Rt*ADC_RESOLUTION)/((long)DivRes+Rt)	// NTC 温度转AD值；Rt： NTC温度下的电阻值/R  
#endif

//#define NTC_TEMP_1	NTC_TEMP2ADC(703)	// 190C
//#define TEMP_ADC_MAX	NTC_TEMP2ADC(574)	// 200C

void NTC_GetAdcValue(void);
u8 NTC_HeatControl(void);


// 继电器参数
#define RELAY_CYCLE		  	(6)	 // 继电器周期/s
#define RELAY_MIN_DUTY		(2)	 // 最小占空比精度/s

typedef struct {
	u8 cycle;
	u8 duty;
	u8 pwmCnt;	
}RelayStruct;

extern RelayStruct relayType;

void Relay_Heat_Init(void);

void Relay_Heat_Control(void);


//=============================================================================
// 触摸功能标识
typedef struct {
	u8 timeLevel;
	u8 tempLevel;
	u8 modeLevel;
	u8 onoffState;		
}TKS_Fun;

extern volatile TKS_Fun tksFun;

// EEPROM 存储地址

// EEPROM_Read(EEPROM_IRS_ADDR); EEPROM_Write(EEPROM_AUTO_ADDR, );
//=============================================================================
typedef struct {
	u16 Time;
	u16 Temp;
	u8  State;	
	u8  LastState;		
}SysParamType;

extern volatile SysParamType sysWork;

#define WORK_STATE_STOP  	0x00
#define WORK_STATE_PRESET  	0x01	// 预设
#define WORK_STATE_PREHEAT  0x02	// 预热
#define WORK_STATE_HEATOVER	0x04	// 预热完成
#define WORK_STATE_TIMEOVER	0x08	// 定时完成
#define WORK_STATE_RUN   	0X10
#define WORK_STATE_ERR  	0x80 	// 异常



//=============================================================
// 状态标识位
typedef struct {
	u8 bit0   :  1;
	u8 bit1   :  1;
	u8 bit2   :  1;	
	u8 bit3   :  1;
	u8 bit4   :  1;
	u8 bit5   :  1;
	u8 bit6   :  1;
	u8 bit7   :  1;
	u8 bit8   :  1;
	u8 bit9   :  1;
	u8 bit10  :  1;	
	u8 bit11  :  1;
	u8 bit12  :  1;
	u8 bit13  :  1;
	u8 bit14  :  1;
	u8 bit15  :  1;
}BitStatus;

typedef union {
	BitStatus Bit;
	u16 Status;	
}LedUnionStatus;

extern volatile LedUnionStatus ledUnion;

void LED_Control(u16 ledState);

//=============================================================


//=============================================================
// 数码管

#define NIXIE_TYPE_TEMP		0
#define NIXIE_TYPE_TIME		1
#define NIXIE_TYPE_EER_OP	2
#define NIXIE_TYPE_EER_CL	3

extern volatile u8 nixTubeDisType;  // 数码管显示类型
extern volatile u8 interDistime;    // 温湿度间隔交替时间

void NixieTube_SegmentCode(u8 code);
void NixieTube_TimeDisplay(u16 Time, u8 secUpdate);
void NixieTube_TempDisplay(u16 Temp);
void NixieTube_ErrCodeDisplay(u8 errType);
void NixieTube_OFF(void);
//=============================================================
// 按键功能
void Mode_Contorl(void);
void Up_Contorl(void);
void Down_Contorl(void);
void Start_Contorl(u8 startType);
void Stop_Contorl(void); 
// stopType:1:正常工作结束； 0： STOP结束； 2： 异常结束
void Work_Stop(u8 stopType);

// 记录倒计时的剩余时间。
u8 EEPROM_TimeRemaining(u8 timeRemain);

u8 EEPROM_Read(u8 addr);
void EEPROM_Write(u8 addr, u8 data);

#endif

/******************* (C) COPYRIGHT 2020 Holtek Semiconductor Inc *****END OF FILE****/