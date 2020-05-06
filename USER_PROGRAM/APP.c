/**
  ******************************************************************************
  * @file 	 app.c
  * @brief 	 This file provides all the app firmware functions.
  * @author  [FangJian]Holtek Semiconductor Inc.
  * @version V1.0.0
  * @date 	 2020-04-03
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

#include "APP.h"
#include "PID.h"

#if PID_ENABLE	// 使能PID算法
pidIn_t ntcSetValue;
pidOut_t pidOutValue;
#endif

volatile LedUnionStatus ledUnion;
volatile SysParamType sysWork;
volatile u8 interDistime;    // 温湿度间隔交替时间
volatile u16 stopDownCount;  // 工作停止倒计时

/*********************************************************************************************/
// 在此根据NTC RT表 填入NTC温度对应的电阻值；例如：200C -> 700R  NTC_TEMP2ADC(700)，
// 电压计算公式：voltage = AD*5V/4096; （AD: MCU采集的AD值； 5V: 参考电压；4096：MCU采集AD的分辨率2^12）
// 注意： 从小到大依次填入：150C-160C-170C-180C-190C-200C-210C-220C-230C
#define NTC_ARR_LEN	  9
// 150C-160C-170C-180C-190C-200C-210C-220C-230C
//const u16 ntcTempArr[NTC_ARR_LEN] = { (2519), (1923), (1511),(1218), (985),  (786), (632),  (517),  (424) };
//#define NTC_TEMP_MIN	3287		// 140C; 该值为比设置的最低温度还小10C的NTC值

// 110C-120C-130C-140C-150C-160C-170C-180C-190C
const u16 ntcTempArr[NTC_ARR_LEN] = { (7679), (5641),(4340),  (3287), (2519), (1923),  (1511),(1218), (985)};
#define NTC_TEMP_MIN	10511		// 100C; 该值为比设置的最低温度还小10C的NTC值

#define PID_OUT_2_PWM(pidOut)  ((u16)pidOut/10)   // pid输出到PWM输出的关系；通过实际测试调整此输出表达式！

/*********************************************************************************************/

RelayStruct relayType ;								


#if PID_ENABLE	// 使能PID算法
void Relay_Heat_Init(void)
{
	relayType.cycle = 6;//Ts/1000;
	relayType.duty = 0;
	relayType.pwmCnt = 0;
}

void Relay_Heat_Control(void) 
{

	if (relayType.duty < RELAY_MIN_DUTY) {
		relayType.duty = 0;
	}

	if (relayType.duty > relayType.cycle - RELAY_MIN_DUTY) {
		relayType.duty = relayType.cycle;
	}

	relayType.pwmCnt = (u16)PID_GetInteTime()/1000;

	// 添加这句会执行崩溃！！！
	//relayType.pwmCnt /= 1000;   // 计数值，直接从PID得到

	if (relayType.pwmCnt <= relayType.duty) {
		RLY = RLY_ON;	
	} else {
		RLY = RLY_OFF;	
	}

	if (relayType.pwmCnt >= relayType.cycle) {
		relayType.pwmCnt = 0;
	}
}

#endif

/*********************************************************************************************/
volatile u16 adcConverterValue, adcConverterValueOld;
volatile u16 ntcAdcVaule = 0;

//volatile u8 perheatState;  // 预热状态

/**
  * @brief		Get one sample of measured signal.
  * @param[in] 	None.
  * @retval 	ConversionValue:  value of the measured signal.
  */
u16 ADC_GetValue(void)
{
	u16 AD_ConverterValue;
	
	/* start AD convert */
	_start = 0; _start = 1; _start = 0;
		
	/* waitting AD conversion finish */	
	while (1 == _adbz);	
	
	/* AD conversion data alignment right */	
	/* get the AD conversion value */
	AD_ConverterValue = AD_REG_VAL;

	return AD_ConverterValue;
}

static volatile u8  adcSumCnt = 0;
static volatile u16 adcValueSum = 0;
static volatile u8  adcValueErrorCnt = 0;

/**
  * @brief		Get NTC ADC value.
  * @param[in] 	None.
  * @retval 	ntcAdcVaule: 采集到的NTC电压值/AD值.
  */
void NTC_GetAdcValue(void)
{
	
	/** NTC 电压采集处理 **/
	adcConverterValue = ADC_GetValue();		// 获取AD值
	
	if (adcConverterValue > ADC_FILTER_RATIO) {  // 跨度2^12的5%
		if ((adcConverterValue < adcConverterValueOld - ADC_FILTER_RATIO) || \
		    (adcConverterValue > adcConverterValueOld + ADC_FILTER_RATIO)) {
			if (++adcValueErrorCnt > 3) {  // 异常次数
				adcConverterValueOld = 	adcConverterValue;
				adcValueSum = 0;  adcSumCnt = 0;	
				adcValueErrorCnt = 0;	
			}
		} else {	// 简单滤波范围
			#if NTC_FUN	 // 直接换算成AD电压值
			adcValueSum += ((long)adcConverterValue*MCU_VCCA)/ADC_RESOLUTION;	 // 求和
			#else // AD值
			adcValueSum += adcConverterValue;  // 不需要那么高的精确度
			#endif
			// 根据实际进行误差补偿	
			adcSumCnt++;	adcValueErrorCnt = 0;	
		}
	} else {
		#if NTC_FUN
		adcValueSum += ((long)adcConverterValue*MCU_VCCA)/ADC_RESOLUTION;	 // 求和
		#else 
		adcValueSum += adcConverterValue;  // 不需要那么高的精确度
		#endif
		// 根据实际进行误差补偿	
		adcSumCnt++;	adcValueErrorCnt = 0;	
	}
	
	if (adcSumCnt > 14) {	// 累计求平均；注意adcValueSum溢出	
		ntcAdcVaule = (adcValueSum/adcSumCnt); // 求平均值；根据需要进行精度换算。
		adcValueSum = 0; adcSumCnt = 0;		
	}
}

 
/**
  * @brief  	NTC_Heat Contorl. 
  * @param[in]  None.
  * @retval 	0: 预热中; 1： 预热完成; 2：恒温中; 3: CL短路: 4: OP开路
  */
u8 NTC_HeatControl(void)
{
	static u8 heatState = 0; 
#if !PID_ENABLE	// 没使能PID算法
	u16 ntcSetValue;
	u16 PREHEAT_A;
	static u16 preheatStopTime;
#endif
	//NTC_GetAdcValue();	  // 获取NTC 采集AD值（）

#if 1 //NTC_ANOMALY	 // NCT 异常1：开启		
	if (ntcAdcVaule < 15 ) {  // 短路&开路
		sysWork.LastState = sysWork.State;
		sysWork.State = WORK_STATE_ERR;
		nixTubeDisType = NIXIE_TYPE_EER_CL;  	// 显示CL短路
		return 3;	
	} else if (ntcAdcVaule > ADC_RESOLUTION - 15) {   // 开路
		sysWork.LastState = sysWork.State;
		sysWork.State = WORK_STATE_ERR;
		nixTubeDisType = NIXIE_TYPE_EER_OP;  	// 显示OP开路
		return 4;		
	}
	if (sysWork.State == WORK_STATE_ERR) {  // 异常恢复；直接到关机状态
		sysWork.State = WORK_STATE_STOP;		// 工作状态
	}
#endif	

	if (sysWork.State < WORK_STATE_PREHEAT) {
		heatState = 0; 
		return heatState;
	}
	
#if PID_ENABLE	// 使能PID算法
	ntcSetValue = NTC_TEMP2ADC(NTC_DIV_RESH, ntcTempArr[tksFun.tempLevel]);  // NTC设定值
	//ntcSetValue = 52;
	if (PID_GetInteTime() == 0) {  // 固定积分时间
		if (ntcSetValue > NTC_TEMP2ADC(NTC_DIV_RESH, NTC_TEMP_MIN)) {  // 如果还为到达设定最小温度范围
			RLY = RLY_ON;	
			heatState = 0; 	
			return heatState;
		}

		// 在设定值较小范围时: 进行PID算法，精准控温;	
		#if PID_INCREASE	// 增量式
			pidOutValue = PID_Increase(&pid, ntcAdcVaule,  ntcSetValue);
		#else  // 位置式
			pidOutValue = PID_Location(&pid, ntcAdcVaule,  ntcSetValue);
		#endif	/* PID_INCREASE */
		if (pidOutValue < 0) {  // 温度未达到
			pidOutValue = -pidOutValue;	 // 取绝对值	
			relayType.duty = PID_OUT_2_PWM(pidOutValue);  // PID 输出 与 加热输出之间的关系。
			if (relayType.duty > relayType.cycle) {
				relayType.duty = relayType.cycle;
			}
		} else {  // 温度超了
			if (heatState == 0) { // 预热完成	
				heatState = 1;
				buzzType = BUZZ_PREHEAT_OVER;  
				if (sysWork.Time) {  // 预热完成，判断倒计时；
					interDistime = 3;  // 温湿度间隔交替时间/s
				} else {
					stopDownCount = DOWN_COUNT_TIME;  // 工作结束倒计时时间	
					//interDistime = 5;  // 温湿度间隔交替时间/s	
				} 
			}	
			relayType.duty = PID_OUT_2_PWM(pidOutValue);  // PID 输出 与 加热输出之间的关系。
			if (relayType.duty > relayType.cycle) {
				relayType.duty = 0;
			} else {
				relayType.duty = 2;		// 温度超了之后，固定输出2S。
			}
			//relayType.duty = relayType.cycle - relayType.duty;
		}
		
		relayType.pwmCnt = 0;
	}
	Relay_Heat_Control();   // 继电器加热控制
#else // 普通
		
	/*預熱點A，溫度到達A點後算預熱完成，停止加熱30S；然後在按照B點控溫。超過B點則不加熱，低於B點1個AD才再加熱。*/
	
	/* 确定预热A点为 < 目标值10C*/
	if (tksFun.tempLevel == 0) {
		PREHEAT_A = NTC_TEMP2ADC(NTC_DIV_RESH, NTC_TEMP_MIN);  // NTC预热值
	} else {
		PREHEAT_A = NTC_TEMP2ADC(NTC_DIV_RESH, ntcTempArr[tksFun.tempLevel-1]);  // NTC预热值
	}

	if (ntcAdcVaule < PREHEAT_A) {   // 到达预热A点
		// 第一次到达预热值；停止加热30S后，再加热值预热点B； 预热点B：这里设置为大于目标点 10个AD值
		if (sysWork.Time > preheatStopTime + 20)   return heatState;  // 20S
		ntcSetValue = NTC_TEMP2ADC(NTC_DIV_RESH, ntcTempArr[tksFun.tempLevel]);  // NTC设定值
		if (ntcAdcVaule < ntcSetValue-10) {   // 到达预热B点
			RLY = RLY_OFF;	
			if (heatState == 0) { // 预热完成	
				heatState = 1;
				buzzType = BUZZ_PREHEAT_OVER;  
				if (sysWork.Time) {  // 预热完成，判断倒计时；
					interDistime = 3;  // 温湿度间隔交替时间/s
				} else {
					stopDownCount = DOWN_COUNT_TIME;  // 工作结束倒计时时间	
					//interDistime = 5;  // 温湿度间隔交替时间/s	
				}	
			}	
		} else {
			RLY = RLY_ON;	
		}

	} else {   // 还没到预热A点
		preheatStopTime = sysWork.Time;
		RLY = RLY_ON;	
	}
#endif
	return heatState;
}




/*********************************************************************************************/

/*********************************************************************************************/
volatile u16 buzzONTime;
volatile u8  buzzType;
static u16 periodOld = 0xFFFF;
/**
  * @brief  	BUZZ Contorl. 
  * @param[in]  period: 周期(占空比50%)；
  * @retval 	None.
  */
void BUZZ_Control(u16 period)
{

	if (period == 0) {
		_ptmal = 0;
		_ptmah = 0;
		periodOld = period;			
	}
	
	if (periodOld != period) {  // 相同周期不进行设置
		periodOld = period;	
		
		u16 CCRP = 125000/period;  // 周期
		u16 CCPA = CCRP/2;
	
		_ptmrpl = CCRP&0xFF; 
		_ptmrph = CCRP >> 8;
		_ptmal = CCPA&0xFF;	
		_ptmah = CCPA >> 8;	
	}
	return;
}
/**
  * @brief  	BUZZ 动作类型
  * @param[in]  type: 类型
  * @retval 	None.
  */
void BUZZ_ActionType(void)
{
	static u8 buzzTypeOld = 0;

	if (buzzTypeOld != buzzType) {
		buzzTypeOld = buzzType;
		buzzONTime = 0;	 // 计时清零
	}

	switch (buzzType) {
		case BUZZ_POWER_ON: // 开机;两声（升調）
			if (++buzzONTime <= 200) {
				BUZZ_Control(500);	
			} else {
				BUZZ_Control(750);
				if (buzzONTime >= 500) {  // OFF
					buzzType = 0;
					buzzTypeOld = 0;
				}	
			}
			break;	
		case BUZZ_PREHEAT: // 按下KEY8开始預熱；单哔声
			if (++buzzONTime <= 300) {
				BUZZ_Control(750);	
			} else {  // OFF
				buzzType = 0;
				buzzTypeOld = 0;	
			}
			break;	
		case BUZZ_PREHEAT_OVER: // 預熱完成；三声单音嘟嘟声
			if (++buzzONTime <= 300) {
				BUZZ_Control(750);	
			} else if (buzzONTime <= 400) {
				BUZZ_Control(0);	
			} else if (buzzONTime <= 700) {
				BUZZ_Control(750);	
			} else if (buzzONTime <= 800) {
				BUZZ_Control(0);	
			} else if (buzzONTime <= 1100) {
				BUZZ_Control(750);	
			} else {  // OFF
				buzzType = 0;
				buzzTypeOld = 0;	
			}
			break;	
		case BUZZ_TIME_OVER: // 倒計時結束；三声双音嘟嘟声（升調）
			if (++buzzONTime <= 100) {
				BUZZ_Control(500);	
			} else if (buzzONTime <= 200) {
				BUZZ_Control(750);	
			} else if (buzzONTime <= 300) {
				BUZZ_Control(0);	
			} else if (buzzONTime <= 400) {
				BUZZ_Control(500);	
			} else if (buzzONTime <= 500) {
				BUZZ_Control(750);	
			} else if (buzzONTime <= 600) {
				BUZZ_Control(0);	
			} else if (buzzONTime <= 700) {
				BUZZ_Control(500);	
			} else if (buzzONTime <= 800) {
				BUZZ_Control(750);	
			} else {  // OFF
				buzzType = 0;
				buzzTypeOld = 0;	
			} 
			break;	
		case BUZZ_POWER_OFF: // 关机; 两声（降調）
			if (++buzzONTime <= 200) {
				BUZZ_Control(750);	
			} else {
				BUZZ_Control(500);
				if (buzzONTime >= 500) {  // OFF
					buzzType = 0;
					buzzTypeOld = 0;
				}	
			}
			break;	
		case BUZZ_4K1S: // 4K頻率，1S
			if (++buzzONTime <= 900) {
				BUZZ_Control(4000);	// 4K
			} else { // OFF
				buzzType = 0;
				buzzTypeOld = 0;	
			}
			break;		
		default: 
			BUZZ_Control(0);
			break;
	}	
}


/*********************************************************************************************/
// LED 分段编码
void LED_SegmentCode(u8 ledCode)
{
	u8 i = 0;
	bit bitStatus = 0;

	for (i = 0; i < 8; i++) {
		if (ledCode&(1<<i)) {	 
			bitStatus = LED_ON;
		} else {
			bitStatus = LED_OFF;	
		}
		switch (i) {
		case 0:
			LED9 = bitStatus;
			break;
		case 1:
			LED10 = bitStatus;
			break;	
		case 2:
			LED11 = bitStatus;
			break;
		case 3:
			LED12 = bitStatus;
			break;
		case 4:
			LED13 = bitStatus;
			break;
		case 5:
			LED14 = bitStatus;
			break;
		case 6:
			LED15 = bitStatus;
			break;
		case 7:
			LED16 = bitStatus;
			break;
		}	
	}
}

void LED_Control(u16 ledState)
{
	static u8 com = 0;
	if (ledState == 0) {  // ALL OFF
		SEG5 = COM_OFF;
		SEG6 = COM_OFF;
		return;
	}
	LED_SegmentCode(0x00);  // 消影；清除上一次扫描状态。
	if (com) {
		SEG5 = COM_ON;
		SEG6 = COM_OFF;
		LED_SegmentCode(ledState&0xFF);
	} else {
		SEG5 = COM_OFF;
		SEG6 = COM_ON;
		LED_SegmentCode(ledState>>8);	
	}
	com = !com;
}

/*********************************************************************************************/


/*********************************************************************************************/

/*********************************************************************************************/


volatile TKS_Fun tksFun;

/**
  * @brief  	Start Contorl. 
  * @param[in]  startType: 1: 开机使用； 0： 其他
  * @retval 	None.
  */
// void Start_Contorl(u8 startType)
// {

// }

/**
  * @brief  	Stop Contorl. 
  * @param[in]  None.
  * @retval 	None.
  */
// void Stop_Contorl(void)
// {

// }
/**
  * @brief  	Mode Contorl. 
  * @param[in]  None.
  * @retval 	None.
  */
void Mode_Contorl(void)
{
	tksFun.modeLevel = !tksFun.modeLevel;
 
	if (tksFun.modeLevel) {	  // Time
	 	nixTubeDisType = NIXIE_TYPE_TIME;  	// 显示时间
	 	// 時間模式，LED12&LED16恆亮，而LED11&LED15滅掉。
	 	ledUnion.Bit.bit11 = TRUE;	
	 	ledUnion.Bit.bit15 = TRUE;	
	 	ledUnion.Bit.bit10 = FALSE;	
	 	ledUnion.Bit.bit13 = FALSE;	
	} else {	// Temp
		nixTubeDisType = NIXIE_TYPE_TEMP;  	// 显示温度	
		// 溫度模式，LED11&LED15恆亮，而LED12&LED16滅掉；
		ledUnion.Bit.bit10 = TRUE;
		ledUnion.Bit.bit13 = TRUE;
		ledUnion.Bit.bit11 = FALSE;	
		ledUnion.Bit.bit15 = FALSE;	
	}	
}
/**
  * @brief  	Up Contorl. 
  * @param[in]  None.
  * @retval 	None.
  */
void Up_Contorl(void)
{
	if (tksFun.modeLevel) {  // 设置时间模式
		if (tksFun.timeLevel < 30) {
			tksFun.timeLevel++;	
			sysWork.Time = tksFun.timeLevel*60;  // 换算时间等级与实际工作时间的关系	
		}
	} else {	// 设置温度模式
		if (tksFun.tempLevel < 8) {
			tksFun.tempLevel++;	
			sysWork.Temp = 150 + (10 * tksFun.tempLevel);  // 换算温度等级与实际工作温度的关系		
		}	
	}
	
}


/**
  * @brief  	Down Contorl. 
  * @param[in]  None.
  * @retval 	None.
  */
void Down_Contorl(void)
{
	if (tksFun.modeLevel) {  // 设置时间模式
		if (tksFun.timeLevel > 1) {
			tksFun.timeLevel--;		
			sysWork.Time = tksFun.timeLevel*60;  // 换算时间等级与实际工作时间的关系	
		}	
	} else {	// 设置温度模式
		if (tksFun.tempLevel) {
			tksFun.tempLevel--;		
			sysWork.Temp = 150 + (10 * tksFun.tempLevel);  // 换算温度等级与实际温度值的关系
		}	
	}
}
/*********************************************************************************************/

/*********************************************************************************************/


volatile u8 nixTubeDisType;  // 数码管显示类型


// 共阴：0~F
const u8 DIG_CODE[16]={0x3f,0x06,0x5b,0x4f, 0x66,0x6d,0x7d,0x07, 0x7f,0x6f,0x77,0x7c, 0x39,0x5e,0x79,0x71};

// 共阴段码
void NixieTube_SegmentCode(u8 code)
{
	u8 i = 0;
	bit bitStatus = 0;
	for (i = 0; i < 8; i++) {
		if (code&(1<<i)) {	 
			bitStatus = IOX_ON;
		} else {
			bitStatus = IOX_OFF;	
		}
		switch (i) {
			case 0: IOA = bitStatus; 	break;	// A
			case 1: IOB = bitStatus; 	break;	// B	
			case 2: IOC = bitStatus; 	break;	// C	
			case 3: IOD = bitStatus; 	break;	// D	
			case 4: IOE = bitStatus; 	break;	// E	
			case 5: IOF = bitStatus; 	break;	// F	
			case 6: IOG = bitStatus; 	break;	// G	
			case 7: IODP = bitStatus; 	break;	// DP			
		}		
	}
}
#if 0	// 小时-分钟
// 数码管时间显示
// Time: 数码管前两位显示小时，后两位显示分钟
// secUpdate: 1: 显示':' ;   0:不显示':'
void NixieTube_TimeDisplay(u16 Time, u8 secUpdate)
{
	static u8 com = 0;
	u8 hour = Time/60;		// 小时
	u8 minute = Time%60;	// 分钟
	u8 data = 0; 
	NixieTube_SegmentCode(0x00);	// 消影
	switch (++com) {
		case 1:	COM4 = COM_OFF; COM1 = COM_ON; data = hour/10; break;	// COM1	
		case 2:	COM1 = COM_OFF; COM2 = COM_ON; data = hour%10;  break;	// COM2	
		case 3:	COM2 = COM_OFF; COM3 = COM_ON; data = minute/10;   break;	// COM3	
		case 4:	COM3 = COM_OFF; COM4 = COM_ON; data = minute%10; com = 0; break;	// COM4	
	}

	data = DIG_CODE[data];  // 得到显示码

	if (sysWorkState == 0) secUpdate = 1;  // 不是工作状态时，常亮。 
	if (com == 2 && secUpdate) {	 // 中间的: 显示判断
		data |= 0X80;  // 显示DP
	} 

	NixieTube_SegmentCode(data);	// 显示
}
#else // 分钟-秒
// 数码管时间显示
// Time: 时间/s; 
void NixieTube_TimeDisplay(u16 Time, u8 secUpdate)
{
	static u8 com = 0;
	u8 minute = Time/60;	// 分钟
	u8 sec = Time%60;		// 秒
	u8 data = 0; 
	NixieTube_SegmentCode(0x00);	// 消影
	switch (++com) {
		case 1:	COM4 = COM_OFF; COM1 = COM_ON; data = minute/10; break;	// COM1	
		case 2:	COM1 = COM_OFF; COM2 = COM_ON; data = minute%10; break;	// COM2	
		case 3:	COM2 = COM_OFF; COM3 = COM_ON; data = sec/10;    break;	// COM3	
		case 4:	COM3 = COM_OFF; COM4 = COM_ON; data = sec%10; com = 0; break;	// COM4	
	}
	data = DIG_CODE[data];  // 得到显示码
	if (com == 2 && secUpdate)  data |= 0X80;    // 固定显示DP
	if (sysWork.State == WORK_STATE_PRESET) data |= 0X80;    // 预设状态 
	NixieTube_SegmentCode(data);	// 显示
}
#endif 

// 数码管温度显示；第4位固定C
void NixieTube_TempDisplay(u16 Temp)
{
	static u8 com = 0;
	u8 data = 0; 
	NixieTube_SegmentCode(0x00);	// 消影
	switch (++com) {
		case 1:	COM4 = COM_OFF; COM1 = COM_ON; data = Temp/100; 	break;	// COM1	
		case 2:	COM1 = COM_OFF; COM2 = COM_ON; data = Temp/10%10;   break;	// COM2	
		case 3:	COM2 = COM_OFF; COM3 = COM_ON; data = Temp%10;   	break;	// COM3	
		case 4:	COM3 = COM_OFF; COM4 = COM_ON; data = 0xC; com = 0; break;	// COM4	
	}
	
	data = DIG_CODE[data];  // 得到显示码
	
	NixieTube_SegmentCode(data);	// 显示
}


// 数码管显示错误代码 ；
// errType: 异常类型
void NixieTube_ErrCodeDisplay(u8 errType)
{
	static u8 com = 0;
	u8 data = 0; 
	
	NixieTube_SegmentCode(0x00);	// 消影
	if (nixTubeDisType == NIXIE_TYPE_EER_OP) {	 // 16、NTC開路則顯示“OP”，
		switch (++com) {
			case 1:	COM4 = COM_OFF; COM1 = COM_OFF; data = 0x0;  break;	// COM1	
			case 2:	COM1 = COM_OFF; COM2 = COM_ON;  data = DIG_CODE[0x0];  break;	// COM2	
			case 3:	COM2 = COM_OFF; COM3 = COM_ON;  data = 0x73;  break;	// COM3	
			case 4:	COM3 = COM_OFF; COM4 = COM_OFF; data = 0x0; com = 0; break;	// COM4	
		}
	} else if (nixTubeDisType == NIXIE_TYPE_EER_CL) {	 // NTC短路則顯示“CL”。
		switch (++com) {
			case 1:	COM4 = COM_OFF; COM1 = COM_OFF; data = 0x0; break;	// COM1	
			case 2:	COM1 = COM_OFF; COM2 = COM_ON;  data = DIG_CODE[0xC]; break;	// COM2	
			case 3:	COM2 = COM_OFF; COM3 = COM_ON;  data = 0x38; break;	// COM3	
			case 4:	COM3 = COM_OFF; COM4 = COM_OFF; data = 0x0; com = 0; break;	// COM4	
		}	
	} else {
		//NixieTube_OFF();	
		return;
	}
	NixieTube_SegmentCode(data);	// 显示	
}

void NixieTube_OFF(void) 
{	
	COM1 = COM_OFF;  COM2 = COM_OFF;  
	COM3 = COM_OFF;  COM4 = COM_OFF;
}
//===================================================================================================
#if 0	// 数码管测试代码
	if (Timestamp > 200) {
		 
		switch (i) {
			case 0: IOA = bitStatus; 	break;	// A
			case 1: IOB = bitStatus; 	break;	// B	
			case 2: IOC = bitStatus; 	break;	// C	
			case 3: IOD = bitStatus; 	break;	// D	
			case 4: IOE = bitStatus; 	break;	// E	
			case 5: IOF = bitStatus; 	break;	// F	
			case 6: IOG = bitStatus; 	break;	// G	
			case 7: IODP = bitStatus; 	break;	// DP	
			default:  bitStatus = !bitStatus; break;		
		}		
		if (++i > 8) {
			i = 0;
			switch (++com) {
				case 1:	COM4 = COM_OFF; COM1 = COM_ON; break;	// COM1	
				case 2:	COM1 = COM_OFF; COM2 = COM_ON;   break;	// COM2	
				case 3:	COM2 = COM_OFF; COM3 = COM_ON;  break;	// COM3	
				case 4:	COM3 = COM_OFF; COM4 = COM_ON; com = 0;break;	// COM4	
			}	
		}
		Timestamp = 0;
	}
#endif
/*********************************************************************************************/

/**
  * @brief EEPROM write function.
  * @param[in] The data you want to write to EEPROM.
  * It can be 0x00~0xff.
  * @param[in] Specifies EEPROM address.
  * It can be 0x00~0x1f.
  * @retval None
  */ 
void EEPROM_Write(u8 addr, u8 data)
{
	/*config EEPROM init*/		
	_mp1l = 0x40;
	_mp1h = 0x01;
	
	/*config EEPROM address*/
	_eea = addr&0xF;	
	/*Write data*/
	_eed = data;	
	
	_emi = 0;		// disable global interrupt
		
	/*config EEPROM Write mode*/
	_iar1 |= 0x08;
	_iar1 |= 0x04;
	
	_emi = 1;		//enable global interrupt
			
	/* waitting for EEPROM write finishing */
	while(_iar1 & 0x04);
	_iar1 = 0;
	_mp1h = 0;
		
	return;
}

/**
  * @brief EEPROM read function.
  * @param[in] Specifies EEPROM address that you want to read.
  * It can be 0x00~0x1f.
  * @retval EEPROM data.
  */
u8 EEPROM_Read(u8 addr)
{
	/* config EEPROM init */
	_mp1l = 0x40;
	_mp1h = 0x01;
	
	/*config EEPROM address*/
	_eea = addr;
	
	/*config EEPROM Read mode*/
	_iar1 |= 0x02;
	_iar1 |= 0x01;
	
	/* waitting for EEPROM Read finishing */
	while(_iar1 & 0x01);
	_iar1 = 0;
	_mp1h = 0;

	return _eed;		//Read data
}

















