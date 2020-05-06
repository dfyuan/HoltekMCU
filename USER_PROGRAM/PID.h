/**
  ******************************************************************************
  * @file    PID.h
  * @brief   This file PID functions.
  * @author  [FangJian]Holtek Semiconductor Inc.
  * @version V1.0.0
  * @date 	 2020-04-15
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
#ifndef _PID_H_
#define _PID_H_

#define PID_ENABLE	  0		// PID使能

#if PID_ENABLE

#define PID_INCREASE  1		// 1: 增量式；0: 位置式

typedef unsigned long  	pidu32_t;
typedef unsigned int 	  pidu16_t;
typedef unsigned char  	pidu8_t;

typedef pidu16_t  	pidIn_t;		// PID输入参数类型
typedef int  		    pidOut_t;		// PID输出参数类型(注意：不能是unsigned 类型)

/* Exported constants --------------------------------------------------------*/
typedef struct {
	int setPoint;		// 设定目标
	int proportion;	// 比例常数
	int integral;		// 积分常数
	int differential;	// 微分常数	
	int errorCurrent;	// 当前误差
	int errorLast;		// 上一次误差
	int errorPrev;		// 上上次误差
	int errorSum;		// 累计误差
}PID;

extern pidu16_t inteTimeCnt;	// 积分时间计数
extern PID pid;
/* Exported macro ------------------------------------------------------------*/

/*

// 继电器参数
#define RELAY_CYCLE			(10)	 // 继电器周期/s
#define RELAY_MIN_DUTY		(2)		 // 最小占空比精度/s

typedef struct {
	u8 cycle;
	u8 duty;
	u8 pwmCnt;	
}RelayStruct;
*/


/* ------------------------------------------------------- */
/*------------------------------------------------------------
PID 参数整定方法:
	1、采样周期设定主要根据被控对象的特性决定。
	2、比例作用是依据偏差的大小来动作，比例参数设定还要考虑被控制值的性质。
	3、在调节时可以先设定一个较大的积分时间常数Ti的初值，然后逐渐减小Ti，
	   直至系统出现振荡之后在反过来，逐渐加大Ti，直至系统振荡消失，
	   记录此时的Ti，设定PID的积分时间常数Ti为当前值的150％到180％。
	4、微分与确定Ti的方法相同，取不振荡时的30％。
	5、死区的设置应该在其他参数的设置基础上进行，否则会导致系统失去控制。
------------------------------------------------------------*/

// PID 参数整定(根据系统精度定义参数类型以节约内存)
// 注意：因为MCU硬件问题；不能使用float类型；所以注意 Ki  Kd  在做运行时最好是整数；比如： Ts与Ti、Td 需要是整数倍的关系; 
#define Ts  (int)(6000)	// 采样周期常数/ms(根据控制对象特性设置); 同时也是控制周期
#define Ti 	(int)(8000)	// 积分时间常数(不能为0)
#define Td  (int)(8000)	// 微分时间常数
#define Kp  (int)(4)		// 比例系数
#define Ki	Kp*((Ts/Ti))	// 积分系数(注意: Ti不能等于0)	
#define Kd	Kp*((Td/Ts))	// 微分系数

//#define Ki  0		// 积分系数
//#define Kd  0		// 微分系数


/* Exported functions ------------------------------------------------------- */
void PID_Init(PID *pid);
void PID_InteTimeCount(void);
int PID_GetInteTime(void);
#if PID_INCREASE	// 增量式
pidOut_t PID_Increase(PID *pid, pidIn_t realValue, pidIn_t setValue);
#else
pidOut_t PID_Location(PID *pid, pidIn_t realValue, pidIn_t setValue);
#endif	/* PID_INCREASE */

#endif	/* PID_ENABLE */

#endif	/* _PID_H_*/

/******************* (C) COPYRIGHT 2019 Holtek Semiconductor Inc *****END OF FILE****/