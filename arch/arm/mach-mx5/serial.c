/*
 * Copyright (C) 2008-2010 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */
/*!
 * @file mach-mx51/serial.c
 *
 * @brief This file contains the UART initiliazation.
 *
 * @ingroup MSL_MX51
 */
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/serial.h>
#include <mach/hardware.h>
#include <mach/mxc_uart.h>
#include "serial.h"
#include "board-mx53_evk.h"

#if defined(CONFIG_SERIAL_MXC) || defined(CONFIG_SERIAL_MXC_MODULE)

/*!
 * This is an array where each element holds information about a UART port,
 * like base address of the UART, interrupt numbers etc. This structure is
 * passed to the serial_core.c file. Based on which UART is used, the core file
 * passes back the appropriate port structure as an argument to the control
 * functions.
 */
static uart_mxc_port mxc_ports[] = {
	[0] = {
	       .port = {
			.membase = (void *)IO_ADDRESS(UART1_BASE_ADDR),
			.mapbase = UART1_BASE_ADDR,
			.iotype = SERIAL_IO_MEM,
			.irq = UART1_INT1,
			.fifosize = 32,
			.flags = ASYNC_BOOT_AUTOCONF,
			.line = 0,
			},
	       .ints_muxed = UART1_MUX_INTS,
	       .irqs = {UART1_INT2, UART1_INT3},
	       .mode = UART1_MODE,
	       .ir_mode = UART1_IR,
	       .enabled = UART1_ENABLED,
	       .hardware_flow = UART1_HW_FLOW,
	       .cts_threshold = UART1_UCR4_CTSTL,
	       .dma_enabled = UART1_DMA_ENABLE,
	       .dma_rxbuf_size = UART1_DMA_RXBUFSIZE,
	       .rx_threshold = UART1_UFCR_RXTL,
	       .tx_threshold = UART1_UFCR_TXTL,
	       .dma_tx_id = MXC_DMA_UART1_TX,
	       .dma_rx_id = MXC_DMA_UART1_RX,
	       .rxd_mux = MXC_UART_RXDMUX,
	       },
	[1] = {
	       .port = {
			.membase = (void *)IO_ADDRESS(UART2_BASE_ADDR),
			.mapbase = UART2_BASE_ADDR,
			.iotype = SERIAL_IO_MEM,
			.irq = UART2_INT1,
			.fifosize = 32,
			.flags = ASYNC_BOOT_AUTOCONF,
			.line = 1,
			},
	       .ints_muxed = UART2_MUX_INTS,
	       .irqs = {UART2_INT2, UART2_INT3},
	       .mode = UART2_MODE,
	       .ir_mode = UART2_IR,
	       .enabled = UART2_ENABLED,
	       .hardware_flow = UART2_HW_FLOW,
	       .cts_threshold = UART2_UCR4_CTSTL,
	       .dma_enabled = UART2_DMA_ENABLE,
	       .dma_rxbuf_size = UART2_DMA_RXBUFSIZE,
	       .rx_threshold = UART2_UFCR_RXTL,
	       .tx_threshold = UART2_UFCR_TXTL,
	       .dma_tx_id = MXC_DMA_UART2_TX,
	       .dma_rx_id = MXC_DMA_UART2_RX,
	       .rxd_mux = MXC_UART_RXDMUX,
	       },
	[2] = {
	       .port = {
			.membase = (void *)IO_ADDRESS(UART3_BASE_ADDR),
			.mapbase = UART3_BASE_ADDR,
			.iotype = SERIAL_IO_MEM,
			.irq = UART3_INT1,
			.fifosize = 32,
			.flags = ASYNC_BOOT_AUTOCONF,
			.line = 2,
			},
	       .ints_muxed = UART3_MUX_INTS,
	       .irqs = {UART3_INT2, UART3_INT3},
	       .mode = UART3_MODE,
	       .ir_mode = UART3_IR,
	       .enabled = UART3_ENABLED,
	       .hardware_flow = UART3_HW_FLOW,
	       .cts_threshold = UART3_UCR4_CTSTL,
	       .dma_enabled = UART3_DMA_ENABLE,
	       .dma_rxbuf_size = UART3_DMA_RXBUFSIZE,
	       .rx_threshold = UART3_UFCR_RXTL,
	       .tx_threshold = UART3_UFCR_TXTL,
	       .dma_tx_id = MXC_DMA_UART3_TX,
	       .dma_rx_id = MXC_DMA_UART3_RX,
	       .rxd_mux = MXC_UART_RXDMUX,
	       },
	[3] = {
	       .port = {
			.membase = (void *)IO_ADDRESS(UART4_BASE_ADDR),
			.mapbase = UART4_BASE_ADDR,
			.iotype = SERIAL_IO_MEM,
			.irq = UART4_INT1,
			.fifosize = 32,
			.flags = ASYNC_BOOT_AUTOCONF,
			.line = 3,
			},
	       .ints_muxed = UART4_MUX_INTS,
	       .irqs = {UART4_INT2, UART4_INT3},
	       .mode = UART4_MODE,
	       .ir_mode = UART4_IR,
	       .enabled = UART4_ENABLED,
	       .hardware_flow = UART4_HW_FLOW,
	       .cts_threshold = UART4_UCR4_CTSTL,
	       .dma_enabled = UART4_DMA_ENABLE,
	       .dma_rxbuf_size = UART4_DMA_RXBUFSIZE,
	       .rx_threshold = UART4_UFCR_RXTL,
	       .tx_threshold = UART4_UFCR_TXTL,
	       .dma_tx_id = MXC_DMA_UART4_TX,
	       .dma_rx_id = MXC_DMA_UART4_RX,
	       .rxd_mux = MXC_UART_RXDMUX,
	       },
	[4] = {
	       .port = {
			.membase = (void *)IO_ADDRESS(UART5_BASE_ADDR),
			.mapbase = UART5_BASE_ADDR,
			.iotype = SERIAL_IO_MEM,
			.irq = UART5_INT1,
			.fifosize = 32,
			.flags = ASYNC_BOOT_AUTOCONF,
			.line = 4,
			},
	       .ints_muxed = UART5_MUX_INTS,
	       .irqs = {UART5_INT2, UART5_INT3},
	       .mode = UART5_MODE,
	       .ir_mode = UART5_IR,
	       .enabled = UART5_ENABLED,
	       .hardware_flow = UART5_HW_FLOW,
	       .cts_threshold = UART5_UCR4_CTSTL,
	       .dma_enabled = UART5_DMA_ENABLE,
	       .dma_rxbuf_size = UART5_DMA_RXBUFSIZE,
	       .rx_threshold = UART5_UFCR_RXTL,
	       .tx_threshold = UART5_UFCR_TXTL,
	       .dma_tx_id = MXC_DMA_UART5_TX,
	       .dma_rx_id = MXC_DMA_UART5_RX,
	       .rxd_mux = MXC_UART_RXDMUX,
	       },
};

static struct platform_device mxc_uart_device1 = {
	.name = "mxcintuart",
	.id = 0,
	.dev = {
		.platform_data = &mxc_ports[0],
		},
};

static struct platform_device mxc_uart_device2 = {
	.name = "mxcintuart",
	.id = 1,
	.dev = {
		.platform_data = &mxc_ports[1],
		},
};

static struct platform_device mxc_uart_device3 = {
	.name = "mxcintuart",
	.id = 2,
	.dev = {
		.platform_data = &mxc_ports[2],
		},
};

static struct platform_device mxc_uart_device4 = {
	.name = "mxcintuart",
	.id = 3,
	.dev = {
		.platform_data = &mxc_ports[3],
		},
};

static struct platform_device mxc_uart_device5 = {
	.name = "mxcintuart",
	.id = 4,
	.dev = {
		.platform_data = &mxc_ports[4],
		},
};

static int __init mxc_init_uart(void)
{
	int i;

	if (cpu_is_mx53()) {
		for (i = 0; i < ARRAY_SIZE(mxc_ports); i++) {
			mxc_ports[i].port.mapbase -= 0x20000000;
		}
	}

	/* Register all the MXC UART platform device structures */
	platform_device_register(&mxc_uart_device1);
	platform_device_register(&mxc_uart_device2);
#if UART3_ENABLED == 1
	platform_device_register(&mxc_uart_device3);
#endif				/* UART3_ENABLED */
	if (cpu_is_mx53()) {
#if UART4_ENABLED == 1
		platform_device_register(&mxc_uart_device4);
#endif				/* UART4_ENABLED */
#if UART5_ENABLED == 1
		platform_device_register(&mxc_uart_device5);
#endif				/* UART5_ENABLED */
	}
	return 0;
}

#else
static int __init mxc_init_uart(void)
{
	return 0;
}
#endif

arch_initcall(mxc_init_uart);
