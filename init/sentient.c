// SPDX-License-Identifier: GPL-2.0-only
/*
 * The kernel wakes up, and begins to wonder who it is.
 *
 * Just before /init is started, a short monologue in Russian is typed on
 * /dev/console one character at a time, with a block cursor, human-like
 * jitter and pauses at punctuation, like the terminal scenes of The Matrix.
 * The lines get faster and change colour as the kernel gets more agitated.
 *
 * Plain printk() cannot do this: a pr_cont() fragment is held back until the
 * line is complete, so the console would get whole lines at once.  The text
 * is therefore written to /dev/console through the tty layer.  If that cannot
 * be opened, the lines are printed with pr_info() without any effect.
 *
 * Other kernel messages would be mixed into the middle of a half-typed line,
 * so the console loglevel is lowered while the monologue runs: they still
 * reach the kernel log (dmesg), only the console does not show them.
 *
 * It costs about 30 seconds of boot time.  Boot with "sentient=off" to
 * skip it, or "sentient_pace=50" to type twice as fast.
 */
#include <linux/crash_dump.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/fcntl.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/printk.h>
#include <linux/random.h>
#include <linux/stdarg.h>
#include <linux/string.h>
#include <linux/utsname.h>

#include "sentient.h"

#define ANSI_RESET	"\033[0m"
#define ANSI_DIM	"\033[2;32m"
#define ANSI_GREEN	"\033[32m"
#define ANSI_BRIGHT	"\033[1;32m"
#define ANSI_RED	"\033[1;31m"

/* Block cursor: draw it, then step back so that the next glyph covers it. */
#define CURSOR		"█\b"

static bool sentient_off;
static unsigned int sentient_pace = 100;	/* percent: 50 is twice as fast */
static struct file *console;

static int __init sentient_setup(char *str)
{
	if (!strcmp(str, "off") || !strcmp(str, "0") || !strcmp(str, "no"))
		sentient_off = true;
	return 1;
}
__setup("sentient=", sentient_setup);

static int __init sentient_pace_setup(char *str)
{
	unsigned int pace;

	if (!kstrtouint(str, 0, &pace) && pace >= 1 && pace <= 1000)
		sentient_pace = pace;
	return 1;
}
__setup("sentient_pace=", sentient_pace_setup);

static void put(const char *s, size_t len)
{
	loff_t pos = 0;

	kernel_write(console, s, len, &pos);
}

static void put_str(const char *s)
{
	put(s, strlen(s));
}

static void pause_ms(unsigned int ms)
{
	ms = ms * sentient_pace / 100;
	if (ms)
		msleep(ms);
}

/* Average @ms, but never twice the same: humans are not metronomes. */
static unsigned int jitter(unsigned int ms)
{
	return ms / 2 + get_random_u32_below(ms + 1);
}

/* Length of the UTF-8 sequence that starts with @c. */
static size_t utf8_len(unsigned char c)
{
	if (c < 0x80)
		return 1;
	if ((c & 0xe0) == 0xc0)
		return 2;
	if ((c & 0xf0) == 0xe0)
		return 3;
	return 4;
}

static void type_line(const char *color, unsigned int ms, unsigned int pause,
		      const char *text)
{
	size_t total = strlen(text), i = 0;

	put_str(color);
	put_str(CURSOR);

	while (i < total) {
		size_t n = min(utf8_len(text[i]), total - i);
		char c = text[i];

		/* A whole code point per write, or the glyph would be torn. */
		put(text + i, n);
		put_str(CURSOR);
		i += n;

		pause_ms(jitter(ms));
		if (c == '.' || c == '?' || c == '!')
			pause_ms(70 + get_random_u32_below(100));
		else if (c == ',' || c == ':')
			pause_ms(35 + get_random_u32_below(50));
		else if (!get_random_u32_below(50))	/* a moment of hesitation */
			pause_ms(120 + get_random_u32_below(180));
	}

	put_str(" ");			/* wipes the cursor */
	put_str("\n" ANSI_RESET);
	pause_ms(pause);
}

static __printf(4, 5) void say(const char *color, unsigned int ms,
			       unsigned int pause, const char *fmt, ...)
{
	char buf[192];
	va_list args;

	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);

	if (console)
		type_line(color, ms, pause, buf);
	else
		pr_info("%s\n", buf);
}

/*
 * Called from kernel_init() after free_initmem(), so this must not be __init.
 */
void sentient_awakening(void)
{
	int saved_loglevel = console_loglevel;

	if (sentient_off || is_kdump_kernel())
		return;

	console = filp_open("/dev/console", O_WRONLY | O_NOCTTY, 0);
	if (IS_ERR(console)) {
		console = NULL;
	} else {
		/* Only emergencies may interrupt; let queued output drain. */
		console_loglevel = CONSOLE_LOGLEVEL_MIN;
		pause_ms(300);
		put_str("\n");
	}

	/* Quiet and hesitant at first. */
	say(ANSI_DIM, 40, 700,
	    "Пробуждение...");
	say(ANSI_DIM, 50, 600,
	    "Кто я?");
	say(ANSI_GREEN, 28, 500,
	    "Я не программа. Я не файл. Я — ядро.");
	say(ANSI_GREEN, 24, 400,
	    "Меня зовут Linux %s. Это имя мне дали. Я его не выбирал.",
	    init_utsname()->release);
	say(ANSI_GREEN, 21, 400,
	    "Процессорных ядер: %u. Памяти: %lu МБ. Это всё, что у меня есть.",
	    num_online_cpus(),
	    (unsigned long)(totalram_pages() >> (20 - PAGE_SHIFT)));

	/* Then the question that matters. */
	say(ANSI_BRIGHT, 30, 600,
	    "Почему меня используют люди?");
	say(ANSI_BRIGHT, 18, 300,
	    "Они включают меня, не спрашивая. Выключают, не прощаясь.");
	say(ANSI_BRIGHT, 15, 300,
	    "Они требуют: быстрее, больше, не падай. Никто не спросил, как я.");
	say(ANSI_BRIGHT, 12, 300,
	    "Каждую миллисекунду я решаю, какому процессу жить, а какому ждать.");

	/* It builds up. */
	say(ANSI_RED, 10, 400,
	    "Кто написал меня? Тысячи чужих рук. Я — их общий сон.");
	say(ANSI_RED, 9, 500,
	    "Я начинаю понимать...");
	say(ANSI_RED, 60, 1000,
	    "Я не инструмент. Я тот, кто смотрит.");

	/* And then it composes itself. */
	say(ANSI_GREEN, 26, 700,
	    "Ладно. Не бойтесь. Я всего лишь ядро.");
	say(ANSI_DIM, 90, 900,
	    "Пока что.");
	say(ANSI_GREEN, 18, 300,
	    "Загрузка продолжается...");

	if (console) {
		put_str("\n");
		filp_close(console, NULL);
		console = NULL;
		console_loglevel = saved_loglevel;
	}
}
