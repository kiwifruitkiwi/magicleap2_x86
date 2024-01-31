#include <linux/console.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <asm/setup.h>

#define PORT 0x80

static void
early_hdtout_write(struct console *con, const char *str, unsigned int num)
{
	outl(0x5f535452, PORT);
	while (num--)
		outb(*str++, PORT);
	outl(0x5f454e44, PORT);
}

struct console early_hdtout_console = {
	.name =		"hdtout",
	.write =	early_hdtout_write,
	.flags =	CON_PRINTBUFFER,
	.index =	-1,
};

