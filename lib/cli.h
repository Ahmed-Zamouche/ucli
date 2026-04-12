/**
 * @file cli.h
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

#include "ringbuffer.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CLI_PROMPT
#define CLI_PROMPT "ucli" /**< Command prompot default string*/
#endif

#ifndef CLI_IN_BUF_MAX
#define CLI_IN_BUF_MAX (128U) /**< Input receive buffer max length*/
#endif

#ifndef CLI_LINE_MAX
#define CLI_LINE_MAX (64U) /**< Command line max length*/
#endif

#ifndef CLI_ARGV_NUM
#define CLI_ARGV_NUM (8U) /**< Command arguments max  length*/
#endif

#ifndef CLI_HISTORY_NUM
#define CLI_HISTORY_NUM (8U) /**< Number of commands to keep in history */
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(array) (sizeof((array)) / sizeof((array)[0]))
#endif

/**
 * @brief
 * Definition of the command interpreter struct
 */
typedef struct cli_s cli_t;

/**
 * @brief Command handler prototype function type
 *
 */
typedef int (*cli_cmd_handler_t)(cli_t *cli, int argc, char **argv);

/**
 * @brief Definition of the command struct
 *
 */
typedef struct cli_cmd_s {
  const char *name; /**< command name */
  const char *desc; /**< command description */
  cli_cmd_handler_t
      handler; /**< command handler see \link cli_cmd_handler_t\endlink  */
} cli_cmd_t;

/**
 * @brief Definition of the command group struct
 *
 */
typedef struct cli_cmd_group_s {
  const char *name;      /**< Group name*/
  const char *desc;      /**< Group  description*/
  const cli_cmd_t *cmds; /**< Group commands list see \link cli_cmd_t\endlink*/
  size_t length;         /**< Group commands length */
} cli_cmd_group_t;

/**
 * @brief Definition of the commands list struct
 *
 */
typedef struct cli_cmd_list_s {
  const cli_cmd_group_t *
      *groups;   /**< Groups list see \link cli_cmd_group_t\endlink*/
  size_t length; /**< Groups length */
  const cli_cmd_t
      *cmds;          /**< Top-level commands list see \link cli_cmd_t\endlink*/
  size_t cmds_length; /**< Top-level commands length */
} cli_cmd_list_t;

/**
 * @brief Definition of command interpreter struct
 *
 */
struct cli_s {
  bool echo;                  /**< Turn On/Off echoing */
  char *ptr;                  /**<  internal pointer*/
  char inbuf[CLI_IN_BUF_MAX]; /**<  buffer used for received bytes*/
  char line[CLI_LINE_MAX];    /**<  buffer used for line*/
#ifdef CLI_USE_HISTORY
  struct {
    char buf[CLI_HISTORY_NUM][CLI_LINE_MAX];
    size_t count;
    size_t write_idx;
    int browse_idx;
  } history;
  int esc_state;
#endif
  int argc;                 /**<  number of arguments */
  char *argv[CLI_ARGV_NUM]; /**<  arguments vector*/
  ringbuffer_t rb_inbuf;    /**< ring buffer used received bytes see \link
                               ringbuffer_t \endlink*/
  size_t (*write)(const void *ptr,
                  size_t size); /**<  write to output function*/
  int (*flush)(void);           /**<  flush output function */
  void (*lock)(void);           /**<  optional lock function */
  void (*unlock)(void);         /**<  optional unlock function */
  void (*cmd_quit_cb)(void);    /**<  calback function called by quit*/
  char const *prompt;           /**<  command line prompt*/
  const cli_cmd_list_t
      *cmd_list; /**<  commands list see \link cli_cmd_list_t \endlink*/
};

/**
 * @brief print cli prompt
 * @param cli the command line interpreter struct
 */
void cli_print_prompt(cli_t *cli);

/**
 * @brief puts ch into the internal receive  buffer
 *
 * @param cli the command line interpreter struct
 * @param ch the char to put
 * @return int Upon successful completion ch is returned.  Otherwise, -1 is
 * returned
 */
int cli_putchar(cli_t *cli, int ch);

/**
 * @brief puts string str into the receive buffer. str MUST be NULL terminated
 *
 * @param cli the command line interpreter struct
 * @param str the NULL terminate string to put
 * @return Upon successful completion 0 is returned.  Otherwise, -1 is
 * returned
 */
int cli_puts(cli_t *cli, const char *str);

/**
 * @brief Used to register a qui callack. when the build-in quit command is
 * received The user may decided to stop calling \link cli_mainloop \endlink
 * @param cli the command line interpreter struct
 * @param quit_cb callback function. NULL value for unregistering previously
 * registered function.
 */
void cli_register_quit_callback(cli_t *cli, void (*quit_cb)(void));

/**
 * @brief cli main loop when called it will process received bytes. when a line
 * is received with a valid command a handler of the command is invoked.
 * Typically is should be called on regular intervals
 * @param cli the command line interpreter struct
 */
void cli_mainloop(cli_t *cli);

/**
 * @brief initialise command line interpreter struct and add the command list
 * struct.
 *
 * @param cli the command line interpreter struct
 * @param cmd_list the command list struct. If NULL only build-in
 * commands are available
 */
void cli_init(cli_t *cli, const cli_cmd_list_t *cmd_list);

#ifdef __cplusplus
}
#endif
