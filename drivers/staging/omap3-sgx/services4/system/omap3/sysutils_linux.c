/**********************************************************************
 *
 * Copyright(c) 2008 Imagination Technologies Ltd. All rights reserved.
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope it will be useful but, except 
 * as otherwise stated in writing, without any warranty; without even the 
 * implied warranty of merchantability or fitness for a particular purpose. 
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 * 
 * The full GNU General Public License is included in this distribution in
 * the file called "COPYING".
 *
 * Contact Information:
 * Imagination Technologies Ltd. <gpl-support@imgtec.com>
 * Home Park Estate, Kings Langley, Herts, WD4 8LZ, UK 
 *
 ******************************************************************************/

#include <linux/version.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/hardirq.h>
#include <linux/spinlock.h>
#include <asm/bug.h>

#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,26))
#include <linux/semaphore.h>
#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,31))
#include <plat/resource.h>
#else 
#include <mach/resource.h>
#endif 
#else 
#include <asm/semaphore.h>
#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,22))
#include <asm/arch/resource.h>
#endif 
#endif 

#if	(LINUX_VERSION_CODE >  KERNEL_VERSION(2,6,22)) && \
	(LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,27))
#define CONSTRAINT_NOTIFICATIONS
#endif 
#include "sgxdefs.h"
#include "services_headers.h"
#include "sysinfo.h"
#include "sgxapi_km.h"
#include "sysconfig.h"
#include "sgxinfokm.h"
#include "syslocal.h"

#define	ONE_MHZ	1000000
#define	HZ_TO_MHZ(m) ((m) / ONE_MHZ)

#if defined(SUPPORT_OMAP3430_SGXFCLK_96M)
#define SGX_PARENT_CLOCK "cm_96m_fck"
#else
#define SGX_PARENT_CLOCK "core_ck"
#endif

#if !defined(PDUMP) && !defined(NO_HARDWARE)
static IMG_BOOL PowerLockWrappedOnCPU(SYS_SPECIFIC_DATA *psSysSpecData)
{
	IMG_INT iCPU;
	IMG_BOOL bLocked = IMG_FALSE;

	if (!in_interrupt())
	{
		iCPU = get_cpu();
		bLocked = (iCPU == atomic_read(&psSysSpecData->sPowerLockCPU));

		put_cpu();
	}

	return bLocked;
}

static IMG_VOID PowerLockWrap(SYS_SPECIFIC_DATA *psSysSpecData)
{
	IMG_INT iCPU;

	if (!in_interrupt())
	{
		
		iCPU = get_cpu();

		
		PVR_ASSERT(iCPU != -1);

		PVR_ASSERT(!PowerLockWrappedOnCPU(psSysSpecData));

		spin_lock(&psSysSpecData->sPowerLock);

		atomic_set(&psSysSpecData->sPowerLockCPU, iCPU);
	}
}

static IMG_VOID PowerLockUnwrap(SYS_SPECIFIC_DATA *psSysSpecData)
{
	if (!in_interrupt())
	{
		PVR_ASSERT(PowerLockWrappedOnCPU(psSysSpecData));

		atomic_set(&psSysSpecData->sPowerLockCPU, -1);

		spin_unlock(&psSysSpecData->sPowerLock);

		put_cpu();
	}
}

PVRSRV_ERROR SysPowerLockWrap(SYS_DATA *psSysData)
{
	SYS_SPECIFIC_DATA *psSysSpecData = (SYS_SPECIFIC_DATA *) psSysData->pvSysSpecificData;

	PowerLockWrap(psSysSpecData);

	return PVRSRV_OK;
}

IMG_VOID SysPowerLockUnwrap(SYS_DATA *psSysData)
{
	SYS_SPECIFIC_DATA *psSysSpecData = (SYS_SPECIFIC_DATA *) psSysData->pvSysSpecificData;

	PowerLockUnwrap(psSysSpecData);
}
#else	
static IMG_BOOL PowerLockWrappedOnCPU(SYS_SPECIFIC_DATA unref__ *psSysSpecData)
{
	return IMG_FALSE;
}

static IMG_VOID PowerLockWrap(SYS_SPECIFIC_DATA unref__ *psSysSpecData)
{
}

static IMG_VOID PowerLockUnwrap(SYS_SPECIFIC_DATA unref__ *psSysSpecData)
{
}

PVRSRV_ERROR SysPowerLockWrap(SYS_DATA unref__ *psSysData)
{
	return PVRSRV_OK;
}

IMG_VOID SysPowerLockUnwrap(SYS_DATA unref__ *psSysData)
{
}
#endif	

IMG_BOOL WrapSystemPowerChange(SYS_SPECIFIC_DATA *psSysSpecData)
{
	IMG_BOOL bPowerLock = PowerLockWrappedOnCPU(psSysSpecData);

	if (bPowerLock)
	{
		PowerLockUnwrap(psSysSpecData);
	}

	return bPowerLock;
}

IMG_VOID UnwrapSystemPowerChange(SYS_SPECIFIC_DATA *psSysSpecData)
{
	PowerLockWrap(psSysSpecData);
}

static inline IMG_UINT32 scale_by_rate(IMG_UINT32 val, IMG_UINT32 rate1, IMG_UINT32 rate2)
{
	if (rate1 >= rate2)
	{
		return val * (rate1 / rate2);
	}

	return val / (rate2 / rate1);
}

static inline IMG_UINT32 scale_prop_to_SGX_clock(IMG_UINT32 val, IMG_UINT32 rate)
{
	return scale_by_rate(val, rate, SYS_SGX_CLOCK_SPEED);
}

static inline IMG_UINT32 scale_inv_prop_to_SGX_clock(IMG_UINT32 val, IMG_UINT32 rate)
{
	return scale_by_rate(val, SYS_SGX_CLOCK_SPEED, rate);
}

IMG_VOID SysGetSGXTimingInformation(SGX_TIMING_INFORMATION *psTimingInfo)
{
	IMG_UINT32 rate;

#if defined(NO_HARDWARE)
	rate = SYS_SGX_CLOCK_SPEED;
#else
	PVR_ASSERT(atomic_read(&gpsSysSpecificData->sSGXClocksEnabled) != 0);

	rate = clk_get_rate(gpsSysSpecificData->psSGX_FCK);
	PVR_ASSERT(rate != 0);
#endif
	psTimingInfo->ui32CoreClockSpeed = rate;
	psTimingInfo->ui32HWRecoveryFreq = scale_prop_to_SGX_clock(SYS_SGX_HWRECOVERY_TIMEOUT_FREQ, rate);
	psTimingInfo->ui32uKernelFreq = scale_prop_to_SGX_clock(SYS_SGX_PDS_TIMER_FREQ, rate);
#if defined(SUPPORT_ACTIVE_POWER_MANAGEMENT)
	psTimingInfo->bEnableActivePM = IMG_TRUE;
#else
	psTimingInfo->bEnableActivePM = IMG_FALSE;
#endif 
	psTimingInfo->ui32ActivePowManLatencyms = SYS_SGX_ACTIVE_POWER_LATENCY_MS;
}

#if defined(CONSTRAINT_NOTIFICATIONS)
#if !defined(SGX_DYNAMIC_TIMING_INFO)
#error "SGX_DYNAMIC_TIMING_INFO must be defined for this platform"
#endif

static struct constraint_id cnstr_id_vdd2 = {
	.type = RES_OPP_CO,
	.data = (IMG_VOID *)"vdd2_opp"
};

#if !defined(PDUMP) && !defined(NO_HARDWARE)
static inline IMG_BOOL ConstraintNotificationsEnabled(SYS_SPECIFIC_DATA *psSysSpecData)
{
	return (atomic_read(&psSysSpecData->sSGXClocksEnabled) != 0) && psSysSpecData->bSGXInitComplete && psSysSpecData->bConstraintNotificationsEnabled;

}

static IMG_BOOL NotifyLockedOnCPU(SYS_SPECIFIC_DATA *psSysSpecData)
{
	IMG_INT iCPU = get_cpu();
	IMG_BOOL bLocked = (iCPU == atomic_read(&psSysSpecData->sNotifyLockCPU));

	put_cpu();

	return bLocked;
}

static IMG_VOID NotifyLock(SYS_SPECIFIC_DATA *psSysSpecData)
{
	IMG_INT iCPU;

	BUG_ON(in_interrupt());

	
	iCPU = get_cpu();

	
	PVR_ASSERT(iCPU != -1);

	PVR_ASSERT(!NotifyLockedOnCPU(psSysSpecData));

	spin_lock(&psSysSpecData->sNotifyLock);

	atomic_set(&psSysSpecData->sNotifyLockCPU, iCPU);
}

static IMG_VOID NotifyUnlock(SYS_SPECIFIC_DATA *psSysSpecData)
{
	PVR_ASSERT(NotifyLockedOnCPU(psSysSpecData));

	atomic_set(&psSysSpecData->sNotifyLockCPU, -1);

	spin_unlock(&psSysSpecData->sNotifyLock);

	put_cpu();
}

static int VDD2PostFunc(struct notifier_block *n, unsigned long event, IMG_VOID *ptr)
{
	PVR_UNREFERENCED_PARAMETER(n);
	PVR_UNREFERENCED_PARAMETER(event);
	PVR_UNREFERENCED_PARAMETER(ptr);

	if (in_interrupt())
	{
		PVR_DPF((PVR_DBG_ERROR, "%s Called in interrupt context.  Ignoring.", __FUNCTION__));
		return 0;
	}

	
	if (!NotifyLockedOnCPU(gpsSysSpecificData))
	{
		return 0;
	}

#if defined(DEBUG)
	if (ConstraintNotificationsEnabled(gpsSysSpecificData))
	{
		IMG_UINT32 rate;

		rate = clk_get_rate(gpsSysSpecificData->psSGX_FCK);

		PVR_ASSERT(rate != 0);

		PVR_DPF((PVR_DBG_MESSAGE, "%s: SGX clock rate: %dMHz", __FUNCTION__, HZ_TO_MHZ(rate)));
	}
#endif
	if (gpsSysSpecificData->bCallVDD2PostFunc)
	{
		PVRSRVDevicePostClockSpeedChange(gpsSysSpecificData->psSGXDevNode->sDevId.ui32DeviceIndex, IMG_TRUE, IMG_NULL);

		gpsSysSpecificData->bCallVDD2PostFunc = IMG_FALSE;
	}
	else
	{
		if (ConstraintNotificationsEnabled(gpsSysSpecificData))
		{
			PVR_TRACE(("%s: Not calling PVR clock speed notification functions", __FUNCTION__));
		}
	}

	NotifyUnlock(gpsSysSpecificData);

	return 0;
}

static int VDD2PreFunc(struct notifier_block *n, unsigned long event, IMG_VOID *ptr)
{
	PVR_UNREFERENCED_PARAMETER(n);
	PVR_UNREFERENCED_PARAMETER(event);
	PVR_UNREFERENCED_PARAMETER(ptr);

	if (in_interrupt())
	{
		PVR_DPF((PVR_DBG_WARNING, "%s Called in interrupt context.  Ignoring.", __FUNCTION__));
		return 0;
	}

	if (PowerLockWrappedOnCPU(gpsSysSpecificData))
	{
		PVR_DPF((PVR_DBG_WARNING, "%s Called from within a power transition.  Ignoring.", __FUNCTION__));
		return 0;
	}

	NotifyLock(gpsSysSpecificData);

	PVR_ASSERT(!gpsSysSpecificData->bCallVDD2PostFunc);

	if (ConstraintNotificationsEnabled(gpsSysSpecificData))
	{
		PVRSRV_ERROR eError;

		eError = PVRSRVDevicePreClockSpeedChange(gpsSysSpecificData->psSGXDevNode->sDevId.ui32DeviceIndex, IMG_TRUE, IMG_NULL);

		gpsSysSpecificData->bCallVDD2PostFunc = (eError == PVRSRV_OK);

	}

	return 0;
}

static struct notifier_block sVDD2Pre = {
	VDD2PreFunc,
	 NULL
};

static struct notifier_block sVDD2Post = {
	VDD2PostFunc,
	 NULL
};

static IMG_VOID RegisterConstraintNotifications(IMG_VOID)
{
	PVR_TRACE(("Registering constraint notifications"));

	PVR_ASSERT(!gpsSysSpecificData->bConstraintNotificationsEnabled);

	constraint_register_pre_notification(gpsSysSpecificData->pVdd2Handle, &sVDD2Pre,
						max_vdd2_opp+1);

	constraint_register_post_notification(gpsSysSpecificData->pVdd2Handle, &sVDD2Post,
						max_vdd2_opp+1);

	
	NotifyLock(gpsSysSpecificData);
	gpsSysSpecificData->bConstraintNotificationsEnabled = IMG_TRUE;
	NotifyUnlock(gpsSysSpecificData);

	PVR_TRACE(("VDD2 constraint notifications registered"));
}

static IMG_VOID UnRegisterConstraintNotifications(IMG_VOID)
{
	PVR_TRACE(("Unregistering constraint notifications"));

	
	NotifyLock(gpsSysSpecificData);
	gpsSysSpecificData->bConstraintNotificationsEnabled = IMG_FALSE;
	NotifyUnlock(gpsSysSpecificData);

	
	constraint_unregister_pre_notification(gpsSysSpecificData->pVdd2Handle, &sVDD2Pre,
						max_vdd2_opp+1);

	constraint_unregister_post_notification(gpsSysSpecificData->pVdd2Handle, &sVDD2Post,
						max_vdd2_opp+1);
}
#else
static IMG_VOID RegisterConstraintNotifications(IMG_VOID)
{
}

static IMG_VOID UnRegisterConstraintNotifications(IMG_VOID)
{
}
#endif 
#endif 

PVRSRV_ERROR EnableSGXClocks(SYS_DATA *psSysData)
{
#if !defined(NO_HARDWARE)
	SYS_SPECIFIC_DATA *psSysSpecData = (SYS_SPECIFIC_DATA *) psSysData->pvSysSpecificData;
	long lNewRate;
	long lRate;
	IMG_INT res;

	
	if (atomic_read(&psSysSpecData->sSGXClocksEnabled) != 0)
	{
		return PVRSRV_OK;
	}

	PVR_DPF((PVR_DBG_MESSAGE, "EnableSGXClocks: Enabling SGX Clocks"));

#if defined(DEBUG)
	{

		IMG_UINT32 rate = clk_get_rate(psSysSpecData->psMPU_CK);
		PVR_DPF((PVR_DBG_MESSAGE, "EnableSGXClocks: CPU Clock is %dMhz", HZ_TO_MHZ(rate)));
	}
#endif

	res = clk_enable(psSysSpecData->psSGX_FCK);
	if (res < 0)
	{
		PVR_DPF((PVR_DBG_ERROR, "EnableSGXClocks: Couldn't enable SGX functional clock (%d)", res));
		return PVRSRV_ERROR_UNABLE_TO_ENABLE_CLOCK;
	}

	res = clk_enable(psSysSpecData->psSGX_ICK);
	if (res < 0)
	{
		PVR_DPF((PVR_DBG_ERROR, "EnableSGXClocks: Couldn't enable SGX interface clock (%d)", res));

		clk_disable(psSysSpecData->psSGX_FCK);
		return PVRSRV_ERROR_UNABLE_TO_ENABLE_CLOCK;
	}

	lNewRate = clk_round_rate(psSysSpecData->psSGX_FCK, SYS_SGX_CLOCK_SPEED + ONE_MHZ);
	if (lNewRate <= 0)
	{
		PVR_DPF((PVR_DBG_ERROR, "EnableSGXClocks: Couldn't round SGX functional clock rate"));
		return PVRSRV_ERROR_UNABLE_TO_ROUND_CLOCK_RATE;
	}

	
	lRate = clk_get_rate(psSysSpecData->psSGX_FCK);
	if (lRate != lNewRate)
	{
		res = clk_set_rate(psSysSpecData->psSGX_FCK, lNewRate);
		if (res < 0)
		{
			PVR_DPF((PVR_DBG_WARNING, "EnableSGXClocks: Couldn't set SGX functional clock rate (%d)", res));
		}
	}

#if defined(DEBUG)
	{
		IMG_UINT32 rate = clk_get_rate(psSysSpecData->psSGX_FCK);
		PVR_DPF((PVR_DBG_MESSAGE, "EnableSGXClocks: SGX Functional Clock is %dMhz", HZ_TO_MHZ(rate)));
	}
#endif

	
	atomic_set(&psSysSpecData->sSGXClocksEnabled, 1);

#else	
	PVR_UNREFERENCED_PARAMETER(psSysData);
#endif	
	return PVRSRV_OK;
}


IMG_VOID DisableSGXClocks(SYS_DATA *psSysData)
{
#if !defined(NO_HARDWARE)
	SYS_SPECIFIC_DATA *psSysSpecData = (SYS_SPECIFIC_DATA *) psSysData->pvSysSpecificData;

	
	if (atomic_read(&psSysSpecData->sSGXClocksEnabled) == 0)
	{
		return;
	}

	PVR_DPF((PVR_DBG_MESSAGE, "DisableSGXClocks: Disabling SGX Clocks"));

	if (psSysSpecData->psSGX_ICK)
	{
		clk_disable(psSysSpecData->psSGX_ICK);
	}

	if (psSysSpecData->psSGX_FCK)
	{
		clk_disable(psSysSpecData->psSGX_FCK);
	}

	
	atomic_set(&psSysSpecData->sSGXClocksEnabled, 0);

#else	
	PVR_UNREFERENCED_PARAMETER(psSysData);
#endif	
}

PVRSRV_ERROR EnableSystemClocks(SYS_DATA *psSysData)
{
	SYS_SPECIFIC_DATA *psSysSpecData = (SYS_SPECIFIC_DATA *) psSysData->pvSysSpecificData;
	struct clk *psCLK;
	IMG_INT res;
	PVRSRV_ERROR eError;
	IMG_BOOL bPowerLock;

#if defined(DEBUG) || defined(TIMING)
	IMG_INT rate;
	struct clk *sys_ck;
	IMG_CPU_PHYADDR     TimerRegPhysBase;
	IMG_HANDLE hTimerEnable;
	IMG_UINT32 *pui32TimerEnable;

#endif	

	PVR_TRACE(("EnableSystemClocks: Enabling System Clocks"));

	if (!psSysSpecData->bSysClocksOneTimeInit)
	{
		bPowerLock = IMG_FALSE;

		spin_lock_init(&psSysSpecData->sPowerLock);
		atomic_set(&psSysSpecData->sPowerLockCPU, -1);
		spin_lock_init(&psSysSpecData->sNotifyLock);
		atomic_set(&psSysSpecData->sNotifyLockCPU, -1);

		atomic_set(&psSysSpecData->sSGXClocksEnabled, 0);

		psCLK = clk_get(NULL, SGX_PARENT_CLOCK);
		if (IS_ERR(psCLK))
		{
			PVR_DPF((PVR_DBG_ERROR, "EnableSsystemClocks: Couldn't get Core Clock"));
			goto ExitError;
		}
		psSysSpecData->psCORE_CK = psCLK;

		psCLK = clk_get(NULL, "sgx_fck");
		if (IS_ERR(psCLK))
		{
			PVR_DPF((PVR_DBG_ERROR, "EnableSsystemClocks: Couldn't get SGX Functional Clock"));
			goto ExitError;
		}
		psSysSpecData->psSGX_FCK = psCLK;

		psCLK = clk_get(NULL, "sgx_ick");
		if (IS_ERR(psCLK))
		{
			PVR_DPF((PVR_DBG_ERROR, "EnableSystemClocks: Couldn't get SGX Interface Clock"));
			goto ExitError;
		}
		psSysSpecData->psSGX_ICK = psCLK;

#if defined(DEBUG)
		psCLK = clk_get(NULL, "mpu_ck");
		if (IS_ERR(psCLK))
		{
			PVR_DPF((PVR_DBG_ERROR, "EnableSystemClocks: Couldn't get MPU Clock"));
			goto ExitError;
		}
		psSysSpecData->psMPU_CK = psCLK;
#endif
		res = clk_set_parent(psSysSpecData->psSGX_FCK, psSysSpecData->psCORE_CK);
		if (res < 0)
		{
			PVR_DPF((PVR_DBG_ERROR, "EnableSystemClocks: Couldn't set SGX parent clock (%d)", res));
			goto ExitError;
		}

		psSysSpecData->bSysClocksOneTimeInit = IMG_TRUE;
	}
	else
	{
		
		bPowerLock = PowerLockWrappedOnCPU(psSysSpecData);
		if (bPowerLock)
		{
			PowerLockUnwrap(psSysSpecData);
		}
	}

#if defined(CONSTRAINT_NOTIFICATIONS)
	psSysSpecData->pVdd2Handle = constraint_get(PVRSRV_MODNAME, &cnstr_id_vdd2);
	if (IS_ERR(psSysSpecData->pVdd2Handle))
	{
		PVR_DPF((PVR_DBG_ERROR, "EnableSystemClocks: Couldn't get VDD2 constraint handle"));
		goto ExitError;
	}

	RegisterConstraintNotifications();
#endif

#if defined(DEBUG) || defined(TIMING)
	
	psCLK = clk_get(NULL, "gpt11_fck");
	if (IS_ERR(psCLK))
	{
		PVR_DPF((PVR_DBG_ERROR, "EnableSystemClocks: Couldn't get GPTIMER11 functional clock"));
		goto ExitUnRegisterConstraintNotifications;
	}
	psSysSpecData->psGPT11_FCK = psCLK;

	psCLK = clk_get(NULL, "gpt11_ick");
	if (IS_ERR(psCLK))
	{
		PVR_DPF((PVR_DBG_ERROR, "EnableSystemClocks: Couldn't get GPTIMER11 interface clock"));
		goto ExitUnRegisterConstraintNotifications;
	}
	psSysSpecData->psGPT11_ICK = psCLK;

	sys_ck = clk_get(NULL, "sys_ck");
	if (IS_ERR(sys_ck))
	{
		PVR_DPF((PVR_DBG_ERROR, "EnableSystemClocks: Couldn't get System clock"));
		goto ExitUnRegisterConstraintNotifications;
	}

	if(clk_get_parent(psSysSpecData->psGPT11_FCK) != sys_ck)
	{
		PVR_TRACE(("Setting GPTIMER11 parent to System Clock"));
		res = clk_set_parent(psSysSpecData->psGPT11_FCK, sys_ck);
		if (res < 0)
		{
			PVR_DPF((PVR_DBG_ERROR, "EnableSystemClocks: Couldn't set GPTIMER11 parent clock (%d)", res));
		goto ExitUnRegisterConstraintNotifications;
		}
	}

	rate = clk_get_rate(psSysSpecData->psGPT11_FCK);
	PVR_TRACE(("GPTIMER11 clock is %dMHz", HZ_TO_MHZ(rate)));

	res = clk_enable(psSysSpecData->psGPT11_FCK);
	if (res < 0)
	{
		PVR_DPF((PVR_DBG_ERROR, "EnableSystemClocks: Couldn't enable GPTIMER11 functional clock (%d)", res));
		goto ExitUnRegisterConstraintNotifications;
	}

	res = clk_enable(psSysSpecData->psGPT11_ICK);
	if (res < 0)
	{
		PVR_DPF((PVR_DBG_ERROR, "EnableSystemClocks: Couldn't enable GPTIMER11 interface clock (%d)", res));
		goto ExitDisableGPT11FCK;
	}

	
	TimerRegPhysBase.uiAddr = SYS_OMAP3430_GP11TIMER_TSICR_SYS_PHYS_BASE;
	pui32TimerEnable = OSMapPhysToLin(TimerRegPhysBase,
                  4,
                  PVRSRV_HAP_KERNEL_ONLY|PVRSRV_HAP_UNCACHED,
                  &hTimerEnable);

	if (pui32TimerEnable == IMG_NULL)
	{
		PVR_DPF((PVR_DBG_ERROR, "EnableSystemClocks: OSMapPhysToLin failed"));
		goto ExitDisableGPT11ICK;
	}

	rate = *pui32TimerEnable;
	if(!(rate & 4))
	{
		PVR_TRACE(("Setting GPTIMER11 mode to posted (currently is non-posted)"));

		
		*pui32TimerEnable = rate | 4;
	}

	OSUnMapPhysToLin(pui32TimerEnable,
		    4,
		    PVRSRV_HAP_KERNEL_ONLY|PVRSRV_HAP_UNCACHED,
		    hTimerEnable);

	
	TimerRegPhysBase.uiAddr = SYS_OMAP3430_GP11TIMER_ENABLE_SYS_PHYS_BASE;
	pui32TimerEnable = OSMapPhysToLin(TimerRegPhysBase,
                  4,
                  PVRSRV_HAP_KERNEL_ONLY|PVRSRV_HAP_UNCACHED,
                  &hTimerEnable);

	if (pui32TimerEnable == IMG_NULL)
	{
		PVR_DPF((PVR_DBG_ERROR, "EnableSystemClocks: OSMapPhysToLin failed"));
		goto ExitDisableGPT11ICK;
	}

	
	*pui32TimerEnable = 3;

	OSUnMapPhysToLin(pui32TimerEnable,
		    4,
		    PVRSRV_HAP_KERNEL_ONLY|PVRSRV_HAP_UNCACHED,
		    hTimerEnable);

#endif 

#if defined(PDUMP) && !defined(NO_HARDWARE) && defined(CONSTRAINT_NOTIFICATIONS)
	PVR_TRACE(("EnableSystemClocks: Setting SGX OPP constraint"));

	
	res = constraint_set(psSysSpecData->pVdd2Handle, max_vdd2_opp);
	if (res != 0)
	{
		PVR_DPF((PVR_DBG_ERROR, "EnableSystemClocks: constraint_set failed (%d)", res));
		goto ExitConstraintSetFailed;
	}
#endif
	eError = PVRSRV_OK;
	goto Exit;

#if defined(PDUMP) && !defined(NO_HARDWARE) && defined(CONSTRAINT_NOTIFICATIONS)
ExitConstraintSetFailed:
#endif
#if defined(DEBUG) || defined(TIMING)
ExitDisableGPT11ICK:
	clk_disable(psSysSpecData->psGPT11_ICK);
ExitDisableGPT11FCK:
	clk_disable(psSysSpecData->psGPT11_FCK);
ExitUnRegisterConstraintNotifications:
#endif	
#if defined(CONSTRAINT_NOTIFICATIONS)
	UnRegisterConstraintNotifications();
	constraint_put(psSysSpecData->pVdd2Handle);
#endif
ExitError:
	eError = PVRSRV_ERROR_DISABLE_CLOCK_FAILURE;
Exit:
	if (bPowerLock)
	{
		PowerLockWrap(psSysSpecData);
	}

	return eError;
}

IMG_VOID DisableSystemClocks(SYS_DATA *psSysData)
{
	SYS_SPECIFIC_DATA *psSysSpecData = (SYS_SPECIFIC_DATA *) psSysData->pvSysSpecificData;
	IMG_BOOL bPowerLock;
#if defined(DEBUG) || defined(TIMING)
	IMG_CPU_PHYADDR TimerRegPhysBase;
	IMG_HANDLE hTimerDisable;
	IMG_UINT32 *pui32TimerDisable;
#endif	

	PVR_TRACE(("DisableSystemClocks: Disabling System Clocks"));

	
	DisableSGXClocks(psSysData);

	bPowerLock = PowerLockWrappedOnCPU(psSysSpecData);
	if (bPowerLock)
	{
		
		PowerLockUnwrap(psSysSpecData);
	}

#if defined(PDUMP) && !defined(NO_HARDWARE) && defined(CONSTRAINT_NOTIFICATIONS)
	{
		int res;

		PVR_TRACE(("DisableSystemClocks: Removing SGX OPP constraint"));

		
		res = constraint_remove(psSysSpecData->pVdd2Handle);
		if (res != 0)
		{
			PVR_DPF((PVR_DBG_WARNING, "DisableSystemClocks: constraint_remove failed (%d)", res));
		}
	}
#endif

#if defined(CONSTRAINT_NOTIFICATIONS)
	UnRegisterConstraintNotifications();
#endif

#if defined(DEBUG) || defined(TIMING)
	
	TimerRegPhysBase.uiAddr = SYS_OMAP3430_GP11TIMER_ENABLE_SYS_PHYS_BASE;
	pui32TimerDisable = OSMapPhysToLin(TimerRegPhysBase,
				4,
				PVRSRV_HAP_KERNEL_ONLY|PVRSRV_HAP_UNCACHED,
				&hTimerDisable);

	if (pui32TimerDisable == IMG_NULL)
	{
		PVR_DPF((PVR_DBG_ERROR, "DisableSystemClocks: OSMapPhysToLin failed"));
	}
	else
	{
		*pui32TimerDisable = 0;

		OSUnMapPhysToLin(pui32TimerDisable,
				4,
				PVRSRV_HAP_KERNEL_ONLY|PVRSRV_HAP_UNCACHED,
				hTimerDisable);
	}

	clk_disable(psSysSpecData->psGPT11_ICK);

	clk_disable(psSysSpecData->psGPT11_FCK);

#endif 
#if defined(CONSTRAINT_NOTIFICATIONS)
	constraint_put(psSysSpecData->pVdd2Handle);
#endif
	if (bPowerLock)
	{
		PowerLockWrap(psSysSpecData);
	}
}
