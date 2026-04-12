/**
 * @file main.c
 * @author Ahmed Zamouche (ahmed.zamouche@gmail.com)
 * @brief
 * @version 0.1
 * @date 2019-12-01
 *
 *  @copyright Copyright (c) 2019
 *
 * MIT License
 *
 * Copyright (c) 2019 Ahmed Zamouche
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "cmd_list.h"
#include "lib/cli.h"

#include "uart.h"

// cppcheck-suppress misra-c2012-21.6
#include <stdio.h>
#include <string.h>

#ifdef WIN32
#include <windows.h>
#elif _POSIX_C_SOURCE >= 199309L
// cppcheck-suppress misra-c2012-21.10
#include <time.h> // for nanosleep
#else
#include <unistd.h> // for usleep
#endif

static cli_t cli;

static void sleep_ms(int milliseconds) // cross-platform sleep function
{
#ifdef WIN32
  Sleep(milliseconds);
#elif _POSIX_C_SOURCE >= 199309L
  struct timespec ts;
  ts.tv_sec = milliseconds / 1000;
  ts.tv_nsec = (milliseconds % 1000) * 1000000;
  nanosleep(&ts, NULL);
#else
  usleep(milliseconds * 1000);
#endif
}

static void uart_rx_callback(char ch) {
  // cppcheck-suppress misra-c2012-17.3
  if (cli_putchar(&cli, ch) != ch) {
    ; // could not write. buffer full
  }
}

// Wrapper function to adapt uart_putchar to cli.write signature
static size_t uart_write_wrapper(const void *ptr, size_t size) {
  // cppcheck-suppress misra-c2012-11.5 ; Generic buffer treated as bytes
  const uint8_t *const p = (const uint8_t *)ptr;
  size_t written = 0U;
  for (size_t i = 0U; (i < size) && (written == i); i++) {
    if (uart_putchar(p[i]) != EOF) {
      written++;
    } else {
      ;
    }
  }
  return written;
}

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;

  uart_init();

  uart_register_rx_callback(uart_rx_callback);

  cli_init(&cli, &cli_cmd_list);

  // Set write function to use our wrapper
  cli.write = uart_write_wrapper;
  cli.flush = uart_flush;

  cli_print_prompt(&cli);
  while (1) {

    cli_mainloop(&cli);

    sleep_ms(10);
  }
  return 0;
}
