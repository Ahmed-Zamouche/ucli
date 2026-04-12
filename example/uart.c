/**
 * @file uart.c
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
#include "uart.h"

#include <pthread.h>
// cppcheck-suppress misra-c2012-21.6
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

static struct termios termios_orig;
static void (*uart_rx_callback)(char ch);
static pthread_spinlock_t spinlock;

static void termios_enable_raw_mode(void) {
  tcgetattr(STDIN_FILENO, &termios_orig);
  struct termios raw = termios_orig;
  cfmakeraw(&raw);
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

static void termios_disable_raw_mode(void) {
  tcsetattr(STDIN_FILENO, TCSANOW, &termios_orig);
}

static void uart_default_rx_callback(char ch) { (void)ch; }

static void *uart_rx_thread(void *arg) {

  (void)arg;

  int ch = uart_getchar();

  while (ch != EOF) {
    pthread_spin_lock(&spinlock);
    void (*cb)(char) = uart_rx_callback;
    pthread_spin_unlock(&spinlock);
    cb(ch);
    ch = uart_getchar();
  }

  pthread_exit(&ch);

  return NULL;
}

int uart_putchar(int ch) {
  size_t n = fwrite(&ch, 1, 1, stdout);
  return (n != 1U) ? EOF : ch;
}

int uart_getchar(void) {
  char ch;
  size_t n = fread(&ch, 1, 1, stdin);
  return (n != 1U) ? EOF : ch;
}

int uart_flush(void) { return fflush(stdout); }

void uart_register_rx_callback(void (*cb)(char)) {

  pthread_spin_lock(&spinlock);
  uart_rx_callback = cb ? cb : uart_default_rx_callback;
  pthread_spin_unlock(&spinlock);
}

void uart_init(void) {

  termios_enable_raw_mode();
  (void)atexit(termios_disable_raw_mode);

  pthread_spin_init(&spinlock, 0);

  uart_rx_callback = uart_default_rx_callback;

  pthread_t thread_id;
  pthread_create(&thread_id, NULL, uart_rx_thread, NULL);
}
