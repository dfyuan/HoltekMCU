/**
  ******************************************************************************
  * @file    PID.c
  * @brief   This file PID functions.
  * @author  Holtek Semiconductor Inc.
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

#include "PID.h"

#if PID_ENABLE

PID pid; 
pidu16_t inteTimeCnt;	// 积分时间计数


void PID_Init(PID *pid)
{
	pid->proportion = Kp;
	pid->integral   = Ki;
	pid->differential = Kd;
	pid->errorCurrent = 0;
	pid->errorLast = 0;
	pid->errorPrev = 0;	
	pid->errorSum = 0;
	pid->setPoint = 0;	
	
}
//#if PID_ENABLE	// 使能PID算法
void PID_InteTimeCount(void)  // PID 积分时间计数
{
	inteTimeCnt++; // 积分时间计数++
}

/**
  * @brief:  PID获取积分时间
  * @param:  None.
  * @retval: 1: 时间到; 0: 时间未到;
  */
int PID_GetInteTime(void)
{
	if (inteTimeCnt >= Ts) { // 判断积分时间是否到了
		inteTimeCnt = 0;
		return 0;	
	} 
	return inteTimeCnt; 
}

#if PID_INCREASE 	// 1: 增量式；0: 位置式

/**
  * @brief: 增量式PID
  * @param: *pid: PID参数;	realValue: 实际值; setValue: 设定值
  * @retval: pidInc: pid out value.
  */
pidOut_t PID_Increase(PID *pid, pidIn_t realValue, pidIn_t setValue)
{
	static volatile pidOut_t pidInc = 0; 
	
	pid->setPoint = setValue;	// 设定值
	
	pid->errorCurrent = pid->setPoint - realValue;  // 当前误差值
	// 输出的是误差增量
	pidInc += pid->proportion * (pid->errorCurrent - pid->errorLast) 	// 比例
			+ pid->integral * pid->errorCurrent						// 积分
			+ pid->differential * (pid->errorCurrent - 2*pid->errorLast + pid->errorPrev);  // 微积分
	 
	pid->errorPrev = pid->errorLast;
	
	pid->errorLast = pid->errorCurrent;
	
	return pidInc;	
}

#else	// 位置式

/**
  * @brief: 位置式PID
  * @param: *pid: PID参数;	realValue: 实际值; setValue: 设定值
  * @retval: pidInc: pid out value.
  */
pidOut_t PID_Location(PID *pid, pidIn_t realValue, pidIn_t setValue)
{
	volatile pidOut_t pidLoc = 0;	
	
	pid->setPoint = setValue;	// 设定值
	
	pid->errorCurrent = pid->setPoint - realValue;  // 当前误差值
	
	pid->errorSum += pid->errorCurrent;	 // 累计误差
	
	pidLoc =  pid->proportion * pid->errorCurrent							// 比例
			+ pid->integral * pid->errorSum									// 积分
			+ pid->differential * (pid->errorCurrent - pid->errorLast); 	// 微积分
			
	pid->errorLast = pid->errorCurrent;
	
	return pidLoc;
}

#endif	/* PID_INCREASE */

#endif	/* PID_ENABLE */










