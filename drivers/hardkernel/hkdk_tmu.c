#include "hkdk_tmu.h"

static ssize_t hkdk_tmu_sysfs_curr_temp(struct device *dev,  struct device_attribute *attr, char *buf) {

	struct s_hkdk_tmu *hkdk_tmu = dev_get_drvdata(dev);
	return sprintf(buf, "%d\n", hkdk_tmu->curr_temp);	
}
static DEVICE_ATTR(curr_temp, S_IRUGO, hkdk_tmu_sysfs_curr_temp, NULL);

static void get_cur_temp(struct s_hkdk_tmu *hkdk_tmu) {

	unsigned char current_temperature_uncompensated;
	unsigned char current_temperature;

	current_temperature_uncompensated = __raw_readl(hkdk_tmu->tmu_base + E_CURRENT_TEMP) & 0xff;

	current_temperature = current_temperature_uncompensated - hkdk_tmu->te1 + TMU_DC_VALUE;

	hkdk_tmu->curr_temp = (int) current_temperature;
	
}

static void tmu_work_out(struct work_struct *work) {
	struct delayed_work *dw = to_delayed_work(work);

	struct s_hkdk_tmu *hkdk_tmu = container_of(dw, struct s_hkdk_tmu, tmu_work);

	enable_irq(hkdk_tmu->irq);
	get_cur_temp(hkdk_tmu);
	if (hkdk_tmu->TMU_TEMP1 == false && hkdk_tmu->curr_temp >= TMU_TEMP1_START) {
		pr_info("HKDK Stage 1 Kick In\n");
		hkdk_tmu->TMU_TEMP1 = true;
		hack_setcpu_frequency(TMU_TEMP1_CPU_SPEED);
		goto out;
	} 
	if (hkdk_tmu->TMU_TEMP1 == true && hkdk_tmu->curr_temp <= TMU_TEMP1_STOP) {
		pr_info("HKDK Stage 1 Kick out\n");
		hkdk_tmu->TMU_TEMP1 = false;
		hack_setcpu_frequency(TMU_MAX_FREQ);
		goto out;
	}
	if (hkdk_tmu->TMU_TEMP2 == false && hkdk_tmu->curr_temp >= TMU_TEMP2_START) {
		pr_info("HKDK Stage 2 Kick in\n");
		hkdk_tmu->TMU_TEMP2 = true;
		hack_setcpu_frequency(TMU_TEMP2_CPU_SPEED);
		goto out;
	}
	if (hkdk_tmu->TMU_TEMP2 == true && hkdk_tmu->curr_temp <= TMU_TEMP2_STOP) {
		pr_info("HKDK Stage 2 Kick out\n");
		hkdk_tmu->TMU_TEMP2 = false;
		hack_setcpu_frequency(TMU_TEMP1_CPU_SPEED);
		goto out;
	} 
		
	goto out;
	out:
	disable_irq_nosync(hkdk_tmu->irq);     
	queue_delayed_work_on(0, tmu_wq, &hkdk_tmu->tmu_work, usecs_to_jiffies(1500 * 1000));

}

static irqreturn_t tmu_irq(int irq, void *id) {
	struct s_hkdk_tmu *info = id;

	unsigned int status;

	disable_irq_nosync(irq);

	status = __raw_readl(info->tmu_base + INTSTAT);
	pr_emerg("HKDK TMU: IRQ STATUS -> %d", status);

	if (status & INTSTAT_RISE0) {
		pr_emerg("Throttling interrupt occured!!!!\n");
		__raw_writel(INTCLEAR_RISE0, info->tmu_base + INTCLEAR);
		info->tmu_state = TMU_STATUS_THROTTLED;
		queue_delayed_work_on(0, tmu_wq, &info->tmu_work, usecs_to_jiffies(1500 * 1000));
	} else if (status & INTSTAT_RISE1) {
		pr_emerg("Warning interrupt occured!!!!\n");
		__raw_writel(INTCLEAR_RISE1, info->tmu_base + INTCLEAR);
		info->tmu_state = TMU_STATUS_WARNING;
		queue_delayed_work_on(0, tmu_wq, &info->tmu_work, usecs_to_jiffies(1500 * 1000));
	} else if (status & INTSTAT_RISE2) {
		pr_emerg("Tripping interrupt occured!!!!\n");
		info->tmu_state = TMU_STATUS_TRIPPED;
		__raw_writel(INTCLEAR_RISE2, info->tmu_base + INTCLEAR);
	} else {
		pr_err("%s: TMU interrupt error\n", __func__);
		return -ENODEV;
	}

	return IRQ_HANDLED;
};

static int hkdk_tmu_start(struct s_hkdk_tmu *info) {

	int te_temp, throttle_temp, warning_temp, trip_temp, cooling_temp = 0, rising_value, reg_info, con;

	// Reload efuse falue
	__raw_writel(TRIMINFO_RELOAD, info->tmu_base + TRIMINFO_CON);

	// get the compensation parameter
	te_temp = __raw_readl(info->tmu_base + TRIMINFO);
	pr_emerg("HKDK TMU: Compensation Parameter is: %d", te_temp);

	info->te1 = te_temp & TRIM_INFO_MASK;
	info->te2 = ((te_temp >> 8) & TRIM_INFO_MASK);

	if ((EFUSE_MIN_VALUE > info->te1) || (info->te1 > EFUSE_MAX_VALUE) || (info->te2 != 0))
		info->te1 = info->efuse_value;

	throttle_temp = TMU_TEMP1_START + info->te1 - TMU_DC_VALUE;
	warning_temp = TMU_TEMP2_STOP + info->te1 - TMU_DC_VALUE;
	trip_temp = TMU_TEMP2_START + info->te1 - TMU_DC_VALUE;

	cooling_temp = 0;

	rising_value = (throttle_temp | (warning_temp << 8) | (trip_temp << 16));

	__raw_writel(rising_value, info->tmu_base + THD_TEMP_RISE);
	__raw_writel(cooling_temp, info->tmu_base + THD_TEMP_FALL);

	/* Say Hello to Exynos */
	__raw_writel(info->slope, info->tmu_base + TMU_CON);

	pr_emerg("HKDK TMU: CPU Registers Initialization Complete");

	reg_info = __raw_readl(info->tmu_base + THD_TEMP_RISE);
	pr_emerg("HKDK TMU: Rising Threshold: %x", reg_info);

	__raw_writel(INTCLEARALL, info->tmu_base + INTCLEAR);

	con = __raw_readl(info->tmu_base + TMU_CON);
	con |= (MUX_ADDR_VALUE << 20 | CORE_EN);

	__raw_writel(con, info->tmu_base + TMU_CON);

	// Enable Interrupts
	__raw_writel(INTEN_RISE0 | INTEN_RISE1 | INTEN_RISE2,
			info->tmu_base + INTEN);

	return 0;
}

static int hkdk_tmu_initialize(struct platform_device *pdev) {

	struct s_hkdk_tmu *info = platform_get_drvdata(pdev);
	int en;

	// pr_emerg("HKDK TMU: TMU Base: %d     TMU_STATUS: %d\n", info->tmu_base, TMU_STATUS);

	en = (__raw_readl(info->tmu_base + TMU_STATUS) & 0x1);

	if (!en) {
		pr_emerg("HKDK TMU: Failed to start the TMU driver\n");
		return -ENOENT;
	}

	return hkdk_tmu_start(info);
}

static int hkdk_tmu_probe(struct platform_device *pdev) {

	int ret, ret2;
	struct resource *res;
	struct s_hkdk_tmu *hkdk_tmu;

	pr_emerg("HKDK TMU: Loaded\n");

	hkdk_tmu = kzalloc(sizeof(struct s_hkdk_tmu), GFP_KERNEL);

	if (!hkdk_tmu) {
		pr_emerg("HKDK TMU: Failed to alloc memory to the structure\n");
		return -ENOMEM;
	}

	hkdk_tmu->dev = &pdev->dev;

	pr_emerg("HKDK TMU: Pdev\n");

	hkdk_tmu->efuse_value = 55;
	hkdk_tmu->slope = 0x10008802;
	hkdk_tmu->mode = 0;
	hkdk_tmu->te1 = 55;
	hkdk_tmu->TMU_TEMP1 = false;
	hkdk_tmu->TMU_TEMP2 = false;
	hkdk_tmu->curr_temp = 10;
	hkdk_tmu->tmu_state = 0;

	pr_emerg("HKDK TMU: MAX CPU Frequency: %s", TMU_MAX_FREQ);
	pr_emerg("HKDK TMU: Set TMU Status\n");

	hkdk_tmu->irq = platform_get_irq(pdev, 0);
	pr_emerg("HKDK TMU: Got IRQ");

	if (hkdk_tmu->irq < 0) {
		pr_emerg("HKDK TMU: No IRQ for TMU -> tmu irq: %d\n", hkdk_tmu->irq);
		return -ENOMEM;
	}

	ret = request_irq(hkdk_tmu->irq, tmu_irq, IRQF_DISABLED, "tmu interrupt", hkdk_tmu);
	pr_emerg("HKDK TMU: Request IRQ\n");

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (res == NULL ) {
		pr_emerg("HKDK TMU: Failed to get memory resources\n");
		return -ENOENT;
	}

	hkdk_tmu->ioarea = request_mem_region(res->start, res->end-res->start+1, pdev->name);
	pr_emerg("HKDK TMU: Request IO Memory\n");
	if (!(hkdk_tmu->ioarea))
		pr_emerg("HKDK TMU: Something wrong at requesting the memory region\n");

	hkdk_tmu->tmu_base = ioremap(res->start, (res->end - res->start) + 1);
	pr_emerg("HKDK TMU: Remap IO Memory\n");
	if (!(hkdk_tmu->tmu_base))
		pr_emerg("HKDK TMU: Something wrong at the io remap\n");

	platform_set_drvdata(pdev, hkdk_tmu);

	ret2 = device_create_file(&pdev->dev, &dev_attr_curr_temp);

	hkdk_tmu_initialize(pdev);
	pr_emerg("HKDK TMU: Initialized\n");

	tmu_wq = create_freezable_workqueue("hkdk_tmu_wq");
	pr_emerg("HKDK TMU: Created Queue\n");

	INIT_DELAYED_WORK_DEFERRABLE(&hkdk_tmu->tmu_work, tmu_work_out);
	pr_emerg("HKDK TMU: INIT Queue\n");

	queue_delayed_work_on(0, tmu_wq, &hkdk_tmu->tmu_work, usecs_to_jiffies(1500 * 1000));
	pr_emerg("HKDK TMU: Delay Work created on the queue\n");

	return 0;
}

static void hkdk_tmu_remove(void) {
	pr_emerg("HKDK TMU Unloaded\n");
	destroy_workqueue(tmu_wq);
}

static struct platform_driver hkdk_tmu_driver = {
		.probe = hkdk_tmu_probe,
		.remove = hkdk_tmu_remove,
		.driver = {
				.name = "hkdk_tmu",
		},
};

static struct resource hkdk_tmu_resource[] = {
		[0] = {
				.start = EXYNOS4_PA_TMU,
				.end = EXYNOS4_PA_TMU + 0xFFFF -1,
				.flags = IORESOURCE_MEM,
		},
		[1] = {
				.start = IRQ_TMU,
				.end = IRQ_TMU,
				.flags = IORESOURCE_IRQ,
		},
};

struct platform_device hkdk_device_tmu = {
		.name = "hkdk_tmu",
		.id = -1,
		.num_resources = ARRAY_SIZE(hkdk_tmu_resource),
		.resource = hkdk_tmu_resource,
};


static int __init tmu_driver_init(void) {
	pr_emerg("HKDK TMU: Platform Device Init\n");
	platform_device_register(&hkdk_device_tmu);
	pr_emerg("HKDK TMU: Platform Driver Init\n");
	return platform_driver_register(&hkdk_tmu_driver);
}

static void __exit tmu_driver_exit(void) {
	platform_driver_unregister(&hkdk_tmu_driver);
}

module_init(tmu_driver_init);
module_exit(tmu_driver_exit);

MODULE_DESCRIPTION("TMU hack for ODROID-X");
MODULE_LICENSE("GPL");
