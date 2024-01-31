/*
 * hdtout.c -- AMD HDTOUT uart driver
 *
 * (C) Copyright 2019, Advanced Micro Device Inc.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/console.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/serial.h>
#include <linux/serial_core.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/io.h>

#define DRV_NAME "hdtout"
#define SERIAL_HDTOUT_MAJOR 100
#define SERIAL_HDTOUT_MINOR 100

#define HDTOUT_UART_SIZE		32

#define HDTOUT_UART_BASE		0x800000
#define HDTOUT_UART_RXDATA_REG		0x828
#define HDTOUT_UART_TXDATA_REG		0x824

#define HDT_START_CODE			0x5f535452
#define HDT_END_CODE			0x5f454e44

#define PORT_HDTOUT_UART		92

static u32 hdtout_count = 0;
static u32 hdtout_data = 0;

struct hdtout_uart_platform_uart {
	unsigned long mapbase;	/* Physical address base */
	unsigned int irq;	/* Interrupt vector */
	unsigned int uartclk;	/* UART clock rate */
	unsigned int bus_shift;	/* Bus shift (address stride) */
};

/*
 * Local per-uart structure.
 */
struct hdtout_uart {
	struct uart_port port;
	struct timer_list tmr;
	unsigned int sigs;	/* Local copy of line sigs */
	unsigned short imr;	/* Local IMR mirror */
};

static void hdtout_uart_begin(struct uart_port *port)
{
	writel(HDT_START_CODE, port->membase + HDTOUT_UART_TXDATA_REG);
	barrier();
	hdtout_data = 0;
	hdtout_count = 0;
}

static void hdtout_uart_end(struct uart_port *port)
{
	if (hdtout_count > 0)
		writel(hdtout_data, port->membase + HDTOUT_UART_TXDATA_REG);

	writel(HDT_END_CODE, port->membase + HDTOUT_UART_TXDATA_REG);
	barrier();
}

static void hdtout_uart_writel(struct uart_port *port, u32 dat, int reg)
{
	hdtout_data = hdtout_data | ((dat & 0xff) << (hdtout_count * 8));
	hdtout_count++;
	if (hdtout_count > 3) {
		writel(hdtout_data, port->membase + reg);
		hdtout_data = 0;
		hdtout_count = 0;
		barrier();
	}
}

static unsigned int hdtout_uart_tx_empty(struct uart_port *port)
{
	return 1;
}

static unsigned int hdtout_uart_get_mctrl(struct uart_port *port)
{
	return 0;
}

static void hdtout_uart_set_mctrl(struct uart_port *port, unsigned int sigs)
{
}

static void hdtout_uart_start_tx(struct uart_port *port)
{
}

static void hdtout_uart_stop_tx(struct uart_port *port)
{
}

static void hdtout_uart_stop_rx(struct uart_port *port)
{
}

static void hdtout_uart_break_ctl(struct uart_port *port, int break_state)
{
}

static void hdtout_uart_set_termios(struct uart_port *port,
				    struct ktermios *termios,
				    struct ktermios *old)
{
}

static void hdtout_uart_config_port(struct uart_port *port, int flags)
{
}

static int hdtout_uart_startup(struct uart_port *port)
{
	return 0;
}

static void hdtout_uart_shutdown(struct uart_port *port)
{
}

static const char *hdtout_uart_type(struct uart_port *port)
{
	return (port->type == PORT_HDTOUT_UART) ? "HDTOUT UART" : NULL;
}

static int hdtout_uart_request_port(struct uart_port *port)
{
	/* UARTs always present */
	return 0;
}

static void hdtout_uart_release_port(struct uart_port *port)
{
	/* Nothing to release... */
}

static int hdtout_uart_verify_port(struct uart_port *port,
				   struct serial_struct *ser)
{
	if ((ser->type != PORT_UNKNOWN) && (ser->type != PORT_HDTOUT_UART))
		return -EINVAL;
	return 0;
}

#ifdef CONFIG_CONSOLE_POLL
static int hdtout_uart_poll_get_char(struct uart_port *port)
{
	return 0;
}

static void hdtout_uart_poll_put_char(struct uart_port *port, unsigned char c)
{
	hdtout_uart_writel(port, c, HDTOUT_UART_TXDATA_REG);
}
#endif

/*
 *	Define the basic serial functions we support.
 */
static const struct uart_ops hdtout_uart_ops = {
	.tx_empty	= hdtout_uart_tx_empty,
	.get_mctrl	= hdtout_uart_get_mctrl,
	.set_mctrl	= hdtout_uart_set_mctrl,
	.start_tx	= hdtout_uart_start_tx,
	.stop_tx	= hdtout_uart_stop_tx,
	.stop_rx	= hdtout_uart_stop_rx,
	.break_ctl	= hdtout_uart_break_ctl,
	.startup	= hdtout_uart_startup,
	.shutdown	= hdtout_uart_shutdown,
	.set_termios	= hdtout_uart_set_termios,
	.type		= hdtout_uart_type,
	.request_port	= hdtout_uart_request_port,
	.release_port	= hdtout_uart_release_port,
	.config_port	= hdtout_uart_config_port,
	.verify_port	= hdtout_uart_verify_port,
#ifdef CONFIG_CONSOLE_POLL
	.poll_get_char	= hdtout_uart_poll_get_char,
	.poll_put_char	= hdtout_uart_poll_put_char,
#endif
};

static struct hdtout_uart hdtout_uart_ports[CONFIG_SERIAL_HDTOUT_UART_MAXPORTS];

#if defined(CONFIG_SERIAL_HDTOUT_UART_CONSOLE)

static void hdtout_uart_console_putc(struct uart_port *port, int c)
{
	hdtout_uart_writel(port, c, HDTOUT_UART_TXDATA_REG);
}

static void hdtout_uart_console_write(struct console *co, const char *s,
				      unsigned int count)
{
	struct uart_port *port = &(hdtout_uart_ports + co->index)->port;

	/* bypass console write if readback 0 value indicating write disable */
	if (!readl(port->membase + HDTOUT_UART_RXDATA_REG))
		return;

	hdtout_uart_begin(port);
	uart_console_write(port, s, count, hdtout_uart_console_putc);
	hdtout_uart_end(port);
}

static int __init hdtout_uart_console_setup(struct console *co, char *options)
{
	struct uart_port *port;
	int baud = CONFIG_SERIAL_HDTOUT_UART_BAUDRATE;
	int bits = 8;
	int parity = 'n';
	int flow = 'n';

	if (co->index < 0 || co->index >= CONFIG_SERIAL_HDTOUT_UART_MAXPORTS)
		return -EINVAL;
	port = &hdtout_uart_ports[co->index].port;
	if (!port->membase) {
		port->mapbase = HDTOUT_UART_BASE;
		port->membase = ioremap(port->mapbase, HDTOUT_UART_SIZE);

		if (!port->membase)
			return -ENOMEM;
	}

	if (options)
		uart_parse_options(options, &baud, &parity, &bits, &flow);

	port->line = co->index;
	port->type = PORT_HDTOUT_UART;
	port->iotype = SERIAL_IO_MEM;
	port->ops = &hdtout_uart_ops;
	port->flags = UPF_BOOT_AUTOCONF;
	port->cons = co;

	return uart_set_options(port, co, baud, parity, bits, flow);
}

static struct uart_driver hdtout_uart_driver;

static struct console hdtout_uart_console = {
	.name	= "ttyHOUT",
	.write	= hdtout_uart_console_write,
	.device	= uart_console_device,
	.setup	= hdtout_uart_console_setup,
	.flags	= CON_PRINTBUFFER,
	.index	= -1,
	.data	= &hdtout_uart_driver,
};

static int __init hdtout_uart_console_init(void)
{
	register_console(&hdtout_uart_console);
	return 0;
}

console_initcall(hdtout_uart_console_init);

#define	HDTOUT_UART_CONSOLE	(&hdtout_uart_console)

static void hdtout_uart_earlycon_write(struct console *co, const char *s,
				       unsigned int count)
{
	struct earlycon_device *dev = co->data;

	/* bypass console write if readback 0 value indicating write disable */
	if (!readl(dev->port.membase + HDTOUT_UART_RXDATA_REG))
		return;

	hdtout_uart_begin(&dev->port);
	uart_console_write(&dev->port, s, count, hdtout_uart_console_putc);
	hdtout_uart_end(&dev->port);
}

static int __init hdtout_uart_earlycon_setup(struct earlycon_device *dev,
					     const char *options)
{
	struct uart_port *port = &dev->port;

	if (!port->membase)
		return -ENODEV;

	dev->con->write = hdtout_uart_earlycon_write;
	return 0;
}

EARLYCON_DECLARE(hdtout_uart, hdtout_uart_earlycon_setup);
OF_EARLYCON_DECLARE(uart, "amd,hdtout-1.0", hdtout_uart_earlycon_setup);

#else

#define	HDTOUT_UART_CONSOLE	NULL

#endif /* CONFIG_SERIAL_HDTOUT_UART_CONSOLE */

/*
 *	Define the hdtout_uart UART driver structure.
 */
static struct uart_driver hdtout_uart_driver = {
	.owner		= THIS_MODULE,
	.driver_name	= DRV_NAME,
	.dev_name	= "ttyHOUT",
	.major		= SERIAL_HDTOUT_MAJOR,
	.minor		= SERIAL_HDTOUT_MINOR,
	.nr		= CONFIG_SERIAL_HDTOUT_UART_MAXPORTS,
	.cons		= HDTOUT_UART_CONSOLE,
};

static int hdtout_uart_probe(struct platform_device *pdev)
{
	struct hdtout_uart_platform_uart *platp = dev_get_platdata(&pdev->dev);
	struct uart_port *port;
	struct resource *res_mem;
	struct resource *res_irq;
	int i = pdev->id;

	/* if id is -1 then use the existing setup from port0 */
	if (i == -1)
		i = 0;

	if (i < 0 || i >= CONFIG_SERIAL_HDTOUT_UART_MAXPORTS)
		return -EINVAL;

	port = &hdtout_uart_ports[i].port;

	if (!port->membase) {
		res_mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
		if (res_mem)
			port->mapbase = res_mem->start;
		else if (platp)
			port->mapbase = platp->mapbase;
		else
			return -EINVAL;
	}

	res_irq = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (res_irq)
		port->irq = res_irq->start;
	else if (platp)
		port->irq = platp->irq;

	if (!port->mapbase) {
		port->membase = ioremap(port->mapbase, HDTOUT_UART_SIZE);
		if (!port->membase)
			return -ENOMEM;
	}

	if (platp)
		port->regshift = platp->bus_shift;
	else
		port->regshift = 0;

	port->line = i;
	port->type = PORT_HDTOUT_UART;
	port->iotype = SERIAL_IO_MEM;
	port->ops = &hdtout_uart_ops;
	port->flags = UPF_BOOT_AUTOCONF;
	port->dev = &pdev->dev;

	if (!hdtout_uart_driver.state) {
		int ret;
		ret = uart_register_driver(&hdtout_uart_driver);
		if (ret < 0) {
			pr_err("failed to register HDTOUT UART driver\n");
			return ret;
		}
	}

	platform_set_drvdata(pdev, port);

	uart_add_one_port(&hdtout_uart_driver, port);

	return 0;
}

static int hdtout_uart_remove(struct platform_device *pdev)
{
	struct uart_port *port = platform_get_drvdata(pdev);

	if (port) {
		uart_remove_one_port(&hdtout_uart_driver, port);
		port->mapbase = 0;
		iounmap(port->membase);
	}

	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id hdtout_uart_match[] = {
	{ .compatible = "amd,hdtout-1.0", },
	{},
};
MODULE_DEVICE_TABLE(of, hdtout_uart_match);
#endif /* CONFIG_OF */

static struct platform_driver hdtout_uart_platform_driver = {
	.probe	= hdtout_uart_probe,
	.remove	= hdtout_uart_remove,
	.driver	= {
		.name		= DRV_NAME,
		.of_match_table	= of_match_ptr(hdtout_uart_match),
	},
};

#if 0
static int __init hdtout_uart_init(void)
{
	int rc;

	rc = uart_register_driver(&hdtout_uart_driver);
	if (rc)
		return rc;
	rc = platform_driver_register(&hdtout_uart_platform_driver);
	if (rc)
		uart_unregister_driver(&hdtout_uart_driver);
	return rc;
}

static void __exit hdtout_uart_exit(void)
{
	platform_driver_unregister(&hdtout_uart_platform_driver);
	uart_unregister_driver(&hdtout_uart_driver);
}
#endif

module_platform_driver(hdtout_uart_platform_driver);
//module_init(hdtout_uart_init);
//module_exit(hdtout_uart_exit);

MODULE_DESCRIPTION("HDTOUT UART driver");
MODULE_ALIAS("platform:" DRV_NAME);
MODULE_ALIAS_CHARDEV_MAJOR(SERIAL_HDTOUT_MAJOR);
