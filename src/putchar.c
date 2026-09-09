#include "include/KernelUtils.h"
#include "include/kernel.h"
#include "include/paging/paging_bootstrap.h"
#include "include/printf.h"
#include "include/registers.h"
#include "include/serial.h"
#include "include/terminal/term.h"
#include <stdint.h>

#define LOGBUF_SIZE 256
static char   logbuf[LOGBUF_SIZE];
static size_t loglen = 0;

/* Calibrated once at boot */
static uint64_t tsc_hz = 2000000000ULL; // TODO: replace with real calibration

/* ------------------------------------------------------------------ */
/* Conversion helpers                                                 */
/* ------------------------------------------------------------------ */
static uint64_t ticks_to_ms(uint64_t ticks)
{
    return (ticks * 1000ULL) / tsc_hz;
}

static void u64_to_str(uint64_t val, char *buf)
{
    char tmp[21];
    int  i = 0;
    do
    {
        tmp[i++] = '0' + (val % 10);
        val /= 10;
    } while (val > 0);
    buf[i] = '\0';
    for (int j = 0; j < i; j++)
    {
        buf[j] = tmp[i - j - 1];
    }
    buf[i] = '\0';
}

/* ------------------------------------------------------------------ */
/* Timestamped line flush                                             */
/* ------------------------------------------------------------------ */
static void flush_serial_line(void)
{
    if (loglen == 0)
        return;

    logbuf[loglen] = '\0';

    // convert ticks → ms
    uint64_t ms = ticks_to_ms(get_ts());
    char     tsbuf[32];
    u64_to_str(ms, tsbuf);

    // print [xxx ms] prefix
    serial_debug('[');
    for (char *p = tsbuf; *p; p++)
        serial_debug(*p);
    serial_debug('m');
    serial_debug('s');
    serial_debug(']');
    serial_debug(' ');

    // print buffered line
    for (size_t i = 0; i < loglen; i++)
    {
        serial_debug(logbuf[i]);
    }
    serial_debug('\n');

    loglen = 0;
}

/* ------------------------------------------------------------------ */
/* Buffer helper                                                      */
/* ------------------------------------------------------------------ */
static void buffer_or_flush(char c)
{
    if (c == '\n' || loglen == LOGBUF_SIZE - 1)
    {
        flush_serial_line();
    }
    else
    {
        logbuf[loglen++] = c;
    }
}

static inline bool voyager_page_tables_active(void)
{
    uint64_t voyager_cr3 = paging_pml4_phys();
    if (voyager_cr3 == 0)
        return false;

    return (readCR3() & ~0xfffull) == (voyager_cr3 & ~0xfffull);
}

/* ------------------------------------------------------------------ */
/* putchar                                                            */
/* ------------------------------------------------------------------ */
void _putchar(char character)
{
    // Terminal / framebuffer routing (unchanged from your version)
    if (kerror_mode == 1)
    {
        term_write(term_context, &character, sizeof(char));
        if (k_mode.timestamp == 0)
            serial_debug(character);
        else
            buffer_or_flush(character);
    }
    else if (kerror_mode == 2)
    {
        if (k_mode.timestamp == 0)
            serial_debug(character);
        else
            buffer_or_flush(character);
    }
    else if (bootspace == 1)
    {
        if (k_mode.timestamp == 0)
            serial_debug(character);
        else
            buffer_or_flush(character);
    }
    else if (bootspace == 2)
    {
        /* Limine's terminal write callback belongs to the bootloader's
         * execution environment. Once Voyager installs its own CR3, do not
         * execute that callback; the replacement HHDM is deliberately NX.
         * Stay on COM1 until Voyager's framebuffer terminal is initialized. */
        if (!voyager_page_tables_active())
        {
            early_term.response->write(
                early_term.response->terminals[0], &character, sizeof(char));
        }

        if (k_mode.timestamp == 0)
            serial_debug(character);
        else
            buffer_or_flush(character);
    }
    else if (bootspace == 3)
    {
        if (!voyager_page_tables_active())
        {
            early_term.response->write(
                early_term.response->terminals[0], &character, sizeof(char));
        }
        else if (k_mode.timestamp == 0)
        {
            serial_debug(character);
        }
        else
        {
            buffer_or_flush(character);
        }
    }
    else
    {
        if (term_context)
        {
            term_write(term_context, &character, sizeof(char));
            if (k_mode.timestamp == 0)
                serial_debug(character);
            else
                buffer_or_flush(character);
        }
        else
        {
            if (k_mode.timestamp == 0)
                serial_debug(character);
            else
                buffer_or_flush(character);

            printf_("%s\n", "ERROR: TERMINAL WRITE FAILURE!");
            printf_("%s\n", "Defaulting to serial");
            return;
        }
    }
}
