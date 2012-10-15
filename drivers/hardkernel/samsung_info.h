/*
 * samsung_info.h
 *
 *  Created on: Oct 2, 2012
 *      Author: mdrjr
 */


#define EXYNOS_IRQ_OFFSET		(32)
#define HKDK_IRQ_Q(x)			((x) + EXYNOS_IRQ_OFFSET)
#define SPI_IRQ(x)				HKDK_IRQ_Q(x+32)
#define GROUP_COMBINER(x)		((x) * 8 + SPI_IRQ(128))
#define IRQ_COMBINER(x, y)		(GROUP_COMBINER(x) + y)

#define E_CURRENT_TEMP			(0x40)
#define TMU_SAVE_NUM 			10
#define TMU_DC_VALUE 			25
#define EFUSE_MIN_VALUE 		40
#define EFUSE_MAX_VALUE 		100
#define TRIMINFO_RELOAD			(1)
#define TRIMINFO_CON			(0x14)
#define TRIM_INFO_MASK			(0xFF)
#define INTSTAT                 (0x74)
#define INTSTAT_RISE0           (1)
#define INTSTAT_RISE1           (1<<4)
#define INTSTAT_RISE2           (1<<8)
#define INTSTAT_FALL0           (1<<16)
#define INTSTAT_FALL1           (1<<20)
#define INTSTAT_FALL2           (1<<24)
#define INTCLEAR_RISE0          (1)
#define INTCLEAR_RISE1          (1<<4)
#define INTCLEAR_RISE2          (1<<8)
#define INTCLEAR_FALL0          (1<<16)
#define INTCLEAR_FALL1          (1<<20)
#define INTCLEAR_FALL2          (1<<24)
#define INTCLEARALL             (INTCLEAR_RISE0 | INTCLEAR_RISE1 | INTCLEAR_RISE2 | INTCLEAR_FALL0 | INTCLEAR_FALL1 | INTCLEAR_FALL2)

#define INTCLEAR                (0x78)
#define TMU_STATUS				(0x28)
#define TRIMINFO				(0x0)
#define THD_TEMP_RISE			(0x50)
#define THD_TEMP_FALL			(0x54)
#define TMU_CON					(0x20)
#define MUX_ADDR_VALUE           6
#define CORE_EN					(1)
#define INTEN_RISE0				(1)
#define INTEN_RISE1				(1<<4)
#define INTEN_RISE2				(1<<8)
#define INTEN					(0x70)
#define IRQ_TMU					IRQ_COMBINER(2, 4)
#define EXYNOS4_PA_TMU			0x100C0000

enum tmu_status_t {
        TMU_STATUS_INIT = 0,
        TMU_STATUS_NORMAL,
        TMU_STATUS_THROTTLED,
        TMU_STATUS_WARNING,
        TMU_STATUS_TRIPPED,
};

struct s_hkdk_tmu {
	int id;
	void __iomem *tmu_base;
	struct device *dev;
	struct resource *ioarea;
	int irq;
	unsigned int efuse_value;
	unsigned int slope;
	int mode;
	unsigned int te1;
	unsigned int te2;
	int tmu_state;

	struct delayed_work tmu_work;

	bool TMU_TEMP1;
	bool TMU_TEMP2;
	
	int curr_temp;
};



