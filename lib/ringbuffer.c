/**
 * @file ringbuffer.c
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
#include "ringbuffer.h"

#define ringbuffer_lock(rb)
#define ringbuffer_unlock(rb)

static bool _ringbuffer_is_empty(const ringbuffer_t *const rb) {
  return (rb->wr_pos == rb->rd_pos);
}

bool ringbuffer_is_empty(const ringbuffer_t *const rb) {
  ringbuffer_lock(rb);
  bool is_empty = _ringbuffer_is_empty(rb);
  ringbuffer_unlock(rb);
  return is_empty;
}

static bool _ringbuffer_is_full(const ringbuffer_t *const rb) {
  return (((rb->wr_pos + 1U) % rb->capacity) == rb->rd_pos);
}

bool ringbuffer_is_full(const ringbuffer_t *const rb) {
  ringbuffer_lock(rb);
  bool is_full = _ringbuffer_is_full(rb);
  ringbuffer_unlock(rb);
  return is_full;
}

size_t ringbuffer_size(const ringbuffer_t *const rb) {
  size_t size = 0;
  if (rb->wr_pos >= rb->rd_pos) {
    size = rb->wr_pos - rb->rd_pos;
  } else {
    size = rb->capacity - (rb->rd_pos - rb->wr_pos);
  }
  return size;
}

size_t ringbuffer_capacity(const ringbuffer_t *const rb) {
  return rb->capacity - 1U; // 1 byte lost to distinguish empty/full
}

int ringbuffer_put(ringbuffer_t *rb, uint8_t u8) {

  int res = -1;

  ringbuffer_lock(rb);

  if (!_ringbuffer_is_full(rb)) {
    rb->buffer[rb->wr_pos] = u8;
    rb->wr_pos = (rb->wr_pos + 1U) % rb->capacity;
    res = 0;
  }

  ringbuffer_unlock(rb);

  return res;
}

int ringbuffer_get(ringbuffer_t *rb, uint8_t *pU8) {
  int res = -1;

  ringbuffer_lock(rb);

  if (!_ringbuffer_is_empty(rb)) {
    *pU8 = rb->buffer[rb->rd_pos];
    rb->rd_pos = (rb->rd_pos + 1U) % rb->capacity;
    res = 0;
  }

  ringbuffer_unlock(rb);

  return res;
}

int ringbuffer_peek(ringbuffer_t *rb, uint8_t *pU8) {
  int res = -1;

  ringbuffer_lock(rb);

  if (!_ringbuffer_is_empty(rb)) {
    *pU8 = rb->buffer[rb->rd_pos];
    res = 0;
  }

  ringbuffer_unlock(rb);

  return res;
}

void ringbuffer_wrap(ringbuffer_t *rb, uint8_t *buffer, size_t capacity) {
  rb->buffer = buffer;
  rb->capacity = capacity;
  rb->wr_pos = 0;
  rb->rd_pos = 0;
}
