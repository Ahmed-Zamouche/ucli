/**
 * @file ringbuffer.h
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
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ringbuffer_s {
  uint8_t *buffer;
  size_t wr_pos;
  size_t rd_pos;
  size_t capacity;
} ringbuffer_t;

bool ringbuffer_is_empty(const ringbuffer_t *const rb);

bool ringbuffer_is_full(const ringbuffer_t *const rb);

size_t ringbuffer_size(const ringbuffer_t *const rb);

size_t ringbuffer_capacity(const ringbuffer_t *const rb);

int ringbuffer_put(ringbuffer_t *rb, uint8_t data);

int ringbuffer_get(ringbuffer_t *rb, uint8_t *data);

int ringbuffer_peek(ringbuffer_t *rb, uint8_t *data);

void ringbuffer_wrap(ringbuffer_t *rb, uint8_t *buffer, size_t capacity);

#ifdef __cplusplus
}
#endif
