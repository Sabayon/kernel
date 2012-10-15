#include <linux/init.h>
#include <linux/module.h>
#include <linux/kmod.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/workqueue.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/crc32.h>
#include <linux/interrupt.h>
#include <linux/gfp.h>
#include <linux/io.h>
#include <linux/types.h>
#include <linux/stat.h>
#include <linux/string.h>

#include <stdbool.h>
#include "samsung_info.h"


#define TMU_TEMP1_CPU_SPEED "800000"
#define TMU_TEMP2_CPU_SPEED "200000"

#define TMU_TEMP1_START  80
#define TMU_TEMP1_STOP   70

#define TMU_TEMP2_START  100
#define TMU_TEMP2_STOP 	 90

#define ONLINE_FILE_PATH "/sys/devices/system/cpu/cpu0/online"
#define CPUFREQ_FILE_PATH "/sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq"

#ifdef CONFIG_EXYNOS4X12_1800MHZ_SUPPORT
	#define TMU_MAX_FREQ "1800000"
#elif defined(CONFIG_EXYNOS4X12_1704MHZ_SUPPORT)
	#define TMU_MAX_FREQ "1704000"
#elif defined(CONFIG_EXYNOS4X12_1600MHZ_SUPPORT)
	#define TMU_MAX_FREQ "1600000"
#else
	#define TMU_MAX_FREQ "1400000"
#endif


struct workqueue_struct *tmu_wq;


char *hkdk_change_cpu(char *str, const char *cpuid) {
  static char buffer[50];
  strcpy(buffer, str);
  buffer[27] = *cpuid;
  return buffer;
}

int hkdk_write_to_file(char *aa_text, const char *file_name) {

	struct file *fp = NULL;
	char *buffer[10] = { 0 };
	mm_segment_t oldfs = { 0 };

	memset(buffer, 0x00, sizeof(buffer));

	sprintf(buffer, "%s", aa_text);

	fp = filp_open(file_name, O_RDWR, 0777);
	if(IS_ERR(fp)) {
		pr_emerg("Error while opening the file");
		return 0;
	}

	oldfs = get_fs();
	set_fs(get_ds());

	if (fp->f_mode & FMODE_WRITE ) {
		int fret = fp->f_op->write(fp, (const char *)buffer, sizeof(buffer), &fp->f_pos);
		return fret;
	}

	set_fs(oldfs);

	if (fp) {
		filp_close(fp, NULL );
	}
	return 0;
}

void hack_setcpu_frequency(char *frequency) {
	/* first run, cpu0 */
	hkdk_write_to_file(frequency, (const char*) CPUFREQ_FILE_PATH);

	/* run for cpu1 */
	hkdk_write_to_file("1", hkdk_change_cpu(ONLINE_FILE_PATH, "1"));
	hkdk_write_to_file(frequency, hkdk_change_cpu(CPUFREQ_FILE_PATH, "1"));

	/* run for cpu2 */
	hkdk_write_to_file("1", hkdk_change_cpu(ONLINE_FILE_PATH, "2"));
	hkdk_write_to_file(frequency, hkdk_change_cpu(CPUFREQ_FILE_PATH, "2"));

	/* run for cpu3 */
	hkdk_write_to_file("1", hkdk_change_cpu(ONLINE_FILE_PATH, "3"));
	hkdk_write_to_file(frequency, hkdk_change_cpu(CPUFREQ_FILE_PATH, "3"));

}
