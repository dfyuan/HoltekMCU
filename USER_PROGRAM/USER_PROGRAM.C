/**
  ******************************************************************************
  * @file 		main.c
  * @brief 		"代号5380"
  * @author 	[FangJian]Holtek Semiconductor Inc.
  * @version 	V1.0.0
  * @date 		2020-04-03
  ******************************************************************************
  * MCU 				: BS66F350C
  * Operating Voltage 	: 5.0v
  * System Clock 		: 16MHz
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
    
#include "USER_PROGRAM.H"   
#include "APP.h"
#include "PID.h"

//--------------------------------------------
volatile u16 Timestamp = 0;


static volatile u8  tksPresValue = 0;
static volatile u16 tksPresTimeCnt = 0;	   
static volatile u16 tksSetTimeCnt = 0;
static volatile bit secUpdate = 0;


typedef enum 
{
	TKS_STST 	= 15,	// START/STOP按鍵
	TKS_MODE 	= 14,
	TKS_UP  	= 16,
	TKS_DOWN 	= 17,
}TKS_ValueFun;	    
 
//**********************************************

void Touch_Scan();
void PowerOFF(void);

/**
  * @brief Interrupt a Interruption routine.
  * @par Parameters:
  * None
  * @retval
  * None
  */
DEFINE_ISR (Interrupt_Extemal, 0x04)
{
	GCC_NOP();		
}
static volatile u16 secTimeCnt = 1;
static volatile u8  time5sCnt = 0;
static volatile u8  time5sDelayCnt = 0;  // 延时5s

//void SysWork_Event()
//{
//
//	
//}

void __attribute((interrupt(0x18))) STM_ISR(void)    // 1ms
{
	/*!< 触摸长按判断. */	
	if (tksPresValue && tksPresTimeCnt < 2000) {  // 2s
		if (++tksPresTimeCnt == 2000) {
			tksPresValue |= 0x80;	 /*!< 触摸长按标识. */		
		}		
	}

	/*!< 蜂鸣器. */	
	BUZZ_ActionType();

#if 1
	if (sysWork.State == WORK_STATE_PRESET) {  // 预设模式
		if (!(Timestamp%400)) {
			ledUnion.Bit.bit7 = !ledUnion.Bit.bit7;	
		}
		if (++secTimeCnt >= 1000) {	// 1s
			tksSetTimeCnt++;
			secTimeCnt = 0;
		}
	} else if (sysWork.State == WORK_STATE_PREHEAT || stopDownCount) {  // 预热模式||倒计时工作
		/************* 秒 定 时 **************/
		if (++secTimeCnt >= 1000) {	// 1s
			if (!stopDownCount) { // 还没进入工作结束倒计时 
				if (sysWork.Time) {
					if (++time5sCnt > interDistime) {  // 温湿度间隔交替时间
						nixTubeDisType = !nixTubeDisType;  // 5s 交替显示
						ledUnion.Bit.bit11 = FALSE;	 // LED12
						if (interDistime == 3) {   // 预热完成了
							ledUnion.Bit.bit10 = TRUE;   // LED11
						} else {
							ledUnion.Bit.bit10 = FALSE;	 // LED11
						}	
					 	ledUnion.Bit.bit15 = !ledUnion.Bit.bit15;	 // LED16	
					 	ledUnion.Bit.bit13 = !ledUnion.Bit.bit13;	 // LED15
						time5sCnt = 0;	
					}
					if (--sysWork.Time == 0) {  // 數碼管閃爍顯示“00：00”，5S后只顯示溫度
						time5sDelayCnt = 5;
						secUpdate = 1;
						ledUnion.Bit.bit11 = FALSE;	 // LED12
						ledUnion.Bit.bit15 = FALSE;	 // LED16
						ledUnion.Bit.bit10 = TRUE;	 // LED11
						ledUnion.Bit.bit13 = TRUE;	 // LED15
						nixTubeDisType = NIXIE_TYPE_TIME;
						buzzType = BUZZ_TIME_OVER;	// 	蜂鳴器報警
					} else {
						secUpdate = !secUpdate;	 // 秒更新	
					}		
				} else {
					if (time5sDelayCnt) {  // 延时5s 數碼管閃爍顯示“00：00”
						if (--time5sDelayCnt == 0) {
							nixTubeDisType = NIXIE_TYPE_TEMP;
							if (interDistime == 3) {   // 预热完成了
								stopDownCount = DOWN_COUNT_TIME;  // 工作结束倒计时时间	
							}		
						}	
					} 
				}
				if (nixTubeDisType == NIXIE_TYPE_TEMP || time5sDelayCnt) {  // TEMP
					if (interDistime == 3) {   // 预热完成了
						ledUnion.Bit.bit10 = TRUE;
					} else {
						ledUnion.Bit.bit10 = !ledUnion.Bit.bit10;	 // LED11
					}	
				} else {  // TIME
					ledUnion.Bit.bit11 = !ledUnion.Bit.bit11;	 // LED12	
					
				}
			} else {  // 工作结束倒计时时间开始	
				if (--stopDownCount == FAN_DOWN_TIME) {  // 关机时间到
					 buzzType = BUZZ_POWER_OFF;    	// 蜂鸣器声
					 PowerOFF();
					 FAN = FAN_ON;	
				}
				if (stopDownCount <= 1) {  // 风扇停止；
					FAN = FAN_OFF;	
					stopDownCount = 0;	// 倒计时结束
				}	
				//if (stopDownCount)  stopDownCount--;
			}
			secTimeCnt = 0;
		} else if (secTimeCnt == 500) {
			if (sysWork.Time) {
				secUpdate = !secUpdate;	 // 秒更新	
			}
		}
	}
#endif	 

	Timestamp++;
	
#if PID_ENABLE	// 使能PID算法
 	PID_InteTimeCount();  // PID 积分时间计数
#endif

	_stmaf = 0;
	_mf1f = 0;
}
 

void __attribute((interrupt(0x28))) TimeBase1_ISR(void)
{
	LED_Control(ledUnion.Status);  // LED 控制
	
	if (sysWork.State != WORK_STATE_STOP) {  // 不是关机状态
		switch (nixTubeDisType) {
		case NIXIE_TYPE_TEMP:
			NixieTube_TempDisplay(sysWork.Temp);	  // 数码管显示温度
			break;
		case NIXIE_TYPE_TIME:
			NixieTube_TimeDisplay(sysWork.Time, secUpdate);	// 数码管显示时间	
			break;
		default:
			NixieTube_ErrCodeDisplay(nixTubeDisType);	// 数码管显示异常
			break;	
		}	 
	}
	_tb1f = 0;	
}

/**
  * @brief 		SystemInit.
  * @param[in]  None.
  * @retval 	None.
  */
void SystemInit(void)
{
	// "compilerPath": "C:\\MinGW\\bin\\gcc.exe",
	// "compilerPath": "C:\\Program Files\\Holtek MCU Development Tools\\HT-IDE3000V8.x\\BIN\\HT-IDE3000.EXE",
	// "miDebuggerPath": "\\usr\\bin\\gdb.exe",
	//"miDebuggerPath": "C:\\Program Files\\Holtek MCU Development Tools\\HT-IDE3000V8.x\\BIN\\Hbuild32.EXE",
	_emi = 0;
	
	_lvden = 1;
	_vlvd2 = 1;	// 3.0V
	_lvdo = 0;	// 清LVD标识
	
	_pac = 0;
	_pbc = 0;
	_pcc = 0;
	_pdc = 0;
	_pec = 0;
	
	_pa = 0XFF;
	_pb = 0XFF;
	_pc = 0XFF;
	_pd = 0XFF;
	_pe = 0XFF;
	
#if 0	
	/*!< PTM - Timer. */
	_ptmc0 = 0b00101000;	// fh/16; PTON;
	_ptmc1 = 0b11000001;    // Timer mode ：PTM 比较器 A 匹配 
	// 16MHz/16 = 1000kHz => 1000K/500 = 2K
	_ptmal = 500&0xFF;	// 实测 1ms中断一次。理论应该为1000
	_ptmah = 500>>8;
	/* 使能中断 */
	_ptmaf = 0;	_ptmae = 1;
	_mf2f = 0;	_mf2e = 1;	
#endif

#if 1  // BUZZ
	/*!< PTM - PWM(PB2). */
	_ptmc0 = 0b00111000;	// fH/64; PTON;
	_ptmc1 = 0b10101000;	// PWM模式-高有效-同向-CCRP-周期；CCRA-占空比；0： CTMn 比较器 P 匹配
	_ptmrpl = 31&0xFF; 
	_ptmrph = 31 >> 8;
#define PTM_DUTY  31/2
	_ptmal = PTM_DUTY&0xFF;	
	_ptmah = PTM_DUTY>>8;
#endif	
	_pbs05 = 1; _pbs04 = 0;  // PB2->PTP

#if (_BS66F350)
	/* ADC...*/
	_adcr0 = 0b00110001;	// ADCEN; A/D数据格式SADOL=D[7:0]; AN1
	_adcr1 = 0b00100100;	// IDLE_CONV;参考电压AVDD; 时钟fSYS/16
	_pbs03 = 1;	_pbs02 = 1;	// PB1->AN1 
	//_pbs10 = 1;	_pbs11 = 1;	// PB4->AN4 
#else
	/* ADC...*/
	_sadc0 = 0b00110100;	// ADCEN; A/D数据格式SADOL=D[7:0]; AN4
	_sadc1 = 0b00001100;	// IDLE_CONV;参考电压AVDD; 时钟fSYS/16
	_pbs11 = 1;	_pbs10 = 1;	// PB4->AN4 
  
#endif

#if 0
	/* CTM1 -> PWM(CTP1->PE6) (f~=4KHz) */
	_ctm1c0 = 0b00001000;	// fSYS/4; CT1ON;  1024 个 CTMn 时钟周期
	_ctm1c1 = 0b10101000;	// PWM模式-高有效-同向-CCRP-周期；CCRA-占空比；0： CTMn 比较器 P 匹配
	_ctm1al = 512&0xFF; 
	_ctm1ah = 512 >> 8;
	_pes15 = 0;	_pes14 = 1;	// PE6->CTP1
	//_pes17 = 0;	_pes16 = 1;	// PE7->CTP1B
#endif
	
	/* STM - Timer */
	_stmc0 = 0b00101000;	// fh/16; STON;
	_stmc1 = 0b11000001;	// Timer mode ：PTM 比较器 A 匹配 
	// 16MHz/16 = 1000kHz => 1000K/500 = 2K
	_stmal = 1000&0xFF;	// 实测 1ms中断一次。理论应该为1000
	_stmah = 1000>>8;
	/* 使能中断 */
	_stmaf = 0;	_stmae = 1;
	_mf1f = 0;	_mf1e = 1;	
	
	/* Timebase1... */
#if (_BS66F350)
	_pscr1 = 0x01;	// fSYS/4
#else
	_psc1r = 0x01;	// fSYS/4
#endif
	_tb1c = 0x86;	// TB10N; 2^10/fPSC1;
	_tb1on = 1;	
	_tb1f = 0;	_tb1e = 1;
	
 	/* 其他IO... */

	_sledc = 0XFF;

}



/**
  * @brief 		Power OFF.
  * @param[in]  None.
  * @retval 	None.
  */
void PowerOFF(void)
{
	sysWork.State = WORK_STATE_STOP;		// 工作状态
	NTCC_EN;
	RLY = RLY_OFF;
	FAN = FAN_OFF;
	ledUnion.Status = 0x0;   	// LED 状态清零
	ledUnion.Bit.bit12 = TRUE;	// LED13&LED14恆亮
	NixieTube_OFF();
}

/**
  * @brief 		USER_PROGRAM_INITIAL.
  * @param[in]  None.
  * @retval 	None.
  */
void USER_PROGRAM_INITIAL()
{
	SystemInit();	// 系统初始化
	
	buzzType = BUZZ_4K1S;	// 	蜂鳴器報警1聲。
	
	PowerOFF();
#if PID_ENABLE	// 使能PID算法	
	PID_Init(&pid); 
#endif
	tksPresTimeCnt = 0;
 	tksPresValue = 0;
 	
 	stopDownCount = 0;
 	
 	_emi = 1;
}

//==============================================
//**********************************************
//==============================================
void USER_PROGRAM()
{
    GET_KEY_BITMAP();
    
	GCC_CLRWDT();
	
	if (sysWork.State != WORK_STATE_ERR) {  // 异常不执行触摸
		Touch_Scan();	// 功能按键扫描
	}
	
	// 5S內沒有按鍵動作 || 在預設模式，無按鍵操作10分鐘后關機。
	if (sysWork.State == WORK_STATE_PRESET) {  // 如果状态是预设；
	   	if (sysWork.LastState == WORK_STATE_PREHEAT) {  // 如果上一次状态是预热；
		   	if (tksSetTimeCnt >= 5) {     // 5S自动恢复为预热状态
		   		tksPresValue = TKS_STST;  // 利用START进入预热状态	
		   	}
	   	}else if (sysWork.LastState == WORK_STATE_STOP) {  // 如果上一次状态是STOP；进入关机 
	   		if (tksSetTimeCnt == 10*60) {       // 10分钟
				buzzType = BUZZ_POWER_OFF;    	// 蜂鸣器声
				PowerOFF();
	   		}
		}
	}  
	
	if (!(Timestamp%10)) {
		NTC_GetAdcValue();	  // 获取NTC 采集AD值（）
		NTC_HeatControl();
	}

}


//===============================================================

	

/**
  * @brief  	TouchScan. 
  * @param[in]  None. 
  * @retval 	None.
  */
void Touch_Scan(void)
{
	u8 tksChannel = 0;

	if (tksPresValue) {
		if (!(DATA_BUF[1]|DATA_BUF[2])) {	// 松手判断
			if (tksPresTimeCnt > 1200) {	// 单击超时时间
				tksPresValue = FALSE;	
				return;
			} else {  
				if (tksPresValue&0xC0) {  // 如果是有效键值了
					tksPresValue = FALSE;	
					return;
				} else {
					tksPresValue |= 0x40;		// 单击	
				}
			}	
		}
		
		if (!(tksPresValue&0xC0))  return;		// 单击/长按按键事件
		//if (!(tksPresValue&0x40))  return;	// 单击事件
		
		tksChannel = (tksPresValue&0x3F);
		/************** 根据条件过滤功能键 ***************/
		
		if ((tksPresValue&0x80) ) { // 长按过滤;且是设定温度模式
			if (tksFun.modeLevel && (tksChannel == TKS_UP || tksChannel == TKS_DOWN)) {
				// 长按通过	
			} else {
				return;
			}	
		}
		
		if (sysWork.State == WORK_STATE_ERR) {  // 异常状态只有START
			if (tksChannel == TKS_STST) {   // 	Start/Stop & Mode 有效
				// 预热时；进行关机
				buzzType = BUZZ_POWER_OFF;    	// 蜂鸣器声
				PowerOFF();
				FAN = FAN_ON;	
				stopDownCount = FAN_DOWN_TIME;  // 风扇倒计时时间	
				tksPresValue = 0;
				return;	
			} else {
				tksPresValue = 0;
				return;		
			}	
		}
		
		// 开机处理
		if (sysWork.State == WORK_STATE_STOP) { // 工作状态是关机；只有START键值有效
			if (tksChannel == TKS_STST) { 		// Start/Stop
				buzzType = BUZZ_POWER_ON;    	// 蜂鸣器声
				// ledUnion.Status |= 0x103F;	// LED1~LED6 & LED13&LED14; 常亮。
				ledUnion.Status = ~0x8800;		// 除了	ED12&LED16不亮；其他都亮起来。
				tksFun.modeLevel = 0;			// 默认为温度模式
				tksFun.timeLevel = 5;			// 默认时间对应5分钟的等级
				tksFun.tempLevel = 5;			// 默认温度对应200C的等级
				sysWork.Temp = 150 + (10 * tksFun.tempLevel);  // 數碼管顯示”200C”；
				sysWork.Time = tksFun.timeLevel*60;	// 工作时间5分钟(单位/S);
				nixTubeDisType = NIXIE_TYPE_TEMP;  	// 显示温度
				sysWork.LastState = sysWork.State;  // 记录上一次状态
				sysWork.State = WORK_STATE_PRESET;   // 进入预设状态
				tksSetTimeCnt = 0;	// 预设时间清零/s
			}
			tksPresValue = 0;
			return;
		} else if (sysWork.State == WORK_STATE_PREHEAT) { // 预热状态；只有START&MODE键值有效
			if (tksChannel == TKS_STST) {   // 	Start/Stop & Mode 有效
				// 预热时；进行关机
				buzzType = BUZZ_POWER_OFF;    	// 蜂鸣器声
				PowerOFF();
				FAN = FAN_ON;	
				stopDownCount = FAN_DOWN_TIME;  // 风扇倒计时时间	
				tksPresValue = 0;
				return;	
			} else if (tksChannel == TKS_MODE) {
				buzzType = BUZZ_PREHEAT;    // 蜂鸣器声
				ledUnion.Bit.bit6 = TRUE;
				ledUnion.Bit.bit9 = TRUE;
				tksFun.modeLevel = 1;	
				sysWork.LastState = sysWork.State;  // 记录上一次状态
				sysWork.State = WORK_STATE_PRESET;  // 进入预设状态
			} else {
				tksPresValue = 0;
				return;	
			}	
		}
		
		tksSetTimeCnt = 0;	// 预设时间清零/s

		//buzzType = BUZZ_PREHEAT;    // 蜂鸣器声

		/*!< 以下执行对应的按键通道功能代码 */
		switch ((tksPresValue&0x3F)) {
			case TKS_STST:	// Start/Stop
				if (tksPresValue&0x80) {  // 长按; 
					tksPresValue = 0; 
				} else { // 短按
					if (sysWork.State == WORK_STATE_PRESET) {  // 预设状态
						buzzType = BUZZ_PREHEAT;    // 蜂鸣器声
						ledUnion.Bit.bit7 = TRUE;
						ledUnion.Bit.bit6 = FALSE;
						ledUnion.Bit.bit9 = FALSE;
						RLY = RLY_ON;  FAN = FAN_ON;	
						interDistime = 5;  // 温湿度间隔交替时间/s
						stopDownCount = 0;	// 倒计时清零
						sysWork.LastState = sysWork.State;  // 记录上一次状态
						sysWork.State = WORK_STATE_PREHEAT;  // 进入预热状态	
					} 
				}
				break;	
			case TKS_MODE:	// Mode
				if (tksPresValue&0x80) {  // 长按; 
					tksPresValue = 0; 
				} else { // 短按
					buzzType = BUZZ_PREHEAT;    // 蜂鸣器声
					Mode_Contorl();	
				}
				break;	
			case TKS_UP:	// Up '+'
				Up_Contorl();
				break;	
			case TKS_DOWN: 	// Down '-'
				Down_Contorl();
				break;	
			default:	
					break;	
		}
		
		if (tksPresValue&0x80) {  // 长按;只有TKS_UP&TKS_DOWN 有效
			tksPresValue &= 0x3F;  // 清长按标识；保留键值	
			tksPresTimeCnt -= 100;  
		} else {  // 短按
			tksPresValue = 0;	
		}
		return;	
	}

	// 判断触摸键值 
	if (DATA_BUF[1]) {
		for (tksChannel = 0; tksChannel < 8; tksChannel++) { // 轮询读值
			if (DATA_BUF[1]&(1<<tksChannel)) {
				tksPresValue = tksChannel+9;
				tksPresTimeCnt = 0;	
				return;
			}	
		}
	} else if (DATA_BUF[2]) {
		for (tksChannel = 0; tksChannel < 8; tksChannel++) { // 轮询读值
			if (DATA_BUF[2]&(1<<tksChannel)) {
				tksPresValue = tksChannel+17;
				tksPresTimeCnt = 0;	
				return;
			}	
		}
	}
	return;
} 















