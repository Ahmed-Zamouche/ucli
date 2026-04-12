/**
 * @file cli.c
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
#include "cli.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

// Only include stdio for desktop/hosted environments
#if defined(__unix__) || defined(__linux__) || defined(_POSIX_VERSION) ||      \
    (defined(__APPLE__) && defined(__MACH__)) || defined(_WIN32)
#include <stdio.h>
#endif
// strings.h may not be available on all embedded toolchains
// We'll provide our own strcasecmp if needed

#define CLI_CMD_LIST_TRV_NEXT (0)
#define CLI_CMD_LIST_TRV_SKIP (1)
#define CLI_CMD_LIST_TRV_END (2)

static int cli_cmd_echo(cli_t *cli, int argc, char **argv);
static int cli_cmd_help(cli_t *cli, int argc, char **argv);
static int cli_cmd_quit(cli_t *cli, int argc, char **argv);
static int cli_cmd_clear(cli_t *cli, int argc, char **argv);
#ifdef CLI_USE_HISTORY
static int cli_cmd_history(cli_t *cli, int argc, char **argv);
#endif

static const char *const CLI_MSG_CMD_OK = "Ok\r\n";
static const char *const CLI_MSG_CMD_ERROR = "Error\r\n";

/**
 * @brief default command line interpreter write function
 *
 * @param ptr pinter to the buffer to be written
 * @param size number of bytes to write
 * @return size_t On success, return the number of items read or written.
 * Otherwise, less than size or 0 is returned
 */
static size_t cli_default_write(const void *ptr, size_t size) {
  // For embedded systems without stdio, this should be overridden
  // by the user. Here we provide a minimal implementation that
  // works for both embedded and desktop systems.
#if defined(__unix__) || defined(__linux__) || defined(_POSIX_VERSION) ||      \
    (defined(__APPLE__) && defined(__MACH__)) || defined(_WIN32)
  // On POSIX/Windows systems with stdio, write to stdout
  return fwrite(ptr, size, 1, stdout);
#else
  // For embedded systems without stdio, this should be overridden
  // Return success without actually writing
  (void)ptr;
  (void)size;
  return size;
#endif
}

/**
 * @brief default flush callback function if none is registered
 *
 * @return int  Upon successful completion 0 is returned.
 */
static int cli_default_flush(void) {
#if defined(__unix__) || defined(__linux__) || defined(_POSIX_VERSION) ||      \
    (defined(__APPLE__) && defined(__MACH__)) || defined(_WIN32)
  return fflush(stdout);
#else
  return 0; // No-op for embedded
#endif
}

/**
 * @brief default quit callback function if none is registered
 *
 */
static void cli_cmd_quit_default_cb(void) {
  // For embedded systems, quitting might mean:
  // - Returning to main menu
  // - Entering low-power mode
  // - Restarting the CLI
  // User should override this with their own handler
#if defined(__unix__) || defined(__linux__) || defined(_POSIX_VERSION) ||      \
    (defined(__APPLE__) && defined(__MACH__)) || defined(_WIN32)
  // On desktop systems, exit the program
  exit(0);
#else
  // For embedded systems, safer to loop
  while (1) {
  }
#endif
}

/**
 * @brief build-in default command list
 *
 */
static const cli_cmd_t cli_default_cmd_list[] = {
    {.name = "help", .desc = "Print this help", .handler = cli_cmd_help},
    {.name = "echo",
     .desc = "(on|off). Turn echoing On or Off",
     .handler = cli_cmd_echo},
    {.name = "clear", .desc = "Clear screen", .handler = cli_cmd_clear},
#ifdef CLI_USE_HISTORY
    {.name = "history",
     .desc = "(|clear). Print or clear past commands",
     .handler = cli_cmd_history},
#endif /* CLI_USE_HISTORY */
    {.name = "quit",
     .desc = "Quit command line interpreter",
     .handler = cli_cmd_quit},
};

/**
 * @brief default command handler if none is attached
 *
 * @param cli the command line interpreter struct
 * @param argc arguments count
 * @param argv arguments vector
 * @return int On success 0 is return. Otherwise non zero value
 */
static int cli_cmd_default_handler(cli_t *cli, int argc, char **argv) {
  (void)cli;
  (void)argc;
  (void)argv;
  return 0;
}

/**
 * @brief build-in clear command handler
 *
 * @param cli the command line interpreter struct
 * @param argc arguments count
 * @param argv arguments vector
 * @return int On success 0 is return. Otherwise non zero value
 */
static int cli_cmd_clear(cli_t *cli, int argc, char **argv) {
  int status = 0;
  (void)argv;

  if (argc != 1) {
    status = -1;
  } else {

    cli->write("\x1b" /* ANSI escape character */
               "[2J"  /* Clear entire screen */
               "\x1b" /* Move cursor to home position (top-left corner) */
               "[H",  /* Move cursor to home position (top-left corner) */
               7);
  }
  return status;
}

#ifdef CLI_USE_HISTORY
static void cli_history_push(cli_t *cli, const char *line) {
  bool should_push = true;

  /* 1. Don't add if empty */
  if (strlen(line) == 0U) {
    should_push = false;
  }

  /* 2. Don't add if same as last */
  if (should_push && (cli->history.count > 0U)) {
    size_t last_idx = (cli->history.write_idx + (size_t)CLI_HISTORY_NUM - 1U) %
                      (size_t)CLI_HISTORY_NUM;
    if (strcmp(cli->history.buf[last_idx], line) == 0) {
      should_push = false;
    }
  }

  /* 3. Perform the push if criteria met */
  if (should_push) {
    (void)strncpy(cli->history.buf[cli->history.write_idx], line,
                  (size_t)CLI_LINE_MAX - 1U);
    cli->history.buf[cli->history.write_idx][CLI_LINE_MAX - 1U] = '\0';

    cli->history.write_idx =
        (cli->history.write_idx + 1U) % (size_t)CLI_HISTORY_NUM;

    if (cli->history.count < (size_t)CLI_HISTORY_NUM) {
      cli->history.count++;
    }
  }
}

static int cli_cmd_history(cli_t *cli, int argc, char **argv) {
  int status = 0;

  if ((argc == 2) && (strcmp(argv[1], "clear") == 0)) {
    cli->history.count = 0U;
    cli->history.write_idx = 0U;
    cli->history.browse_idx = -1;
  } else if (argc > 1) {
    status = -1;
  } else {
    for (size_t i = 0U; i < cli->history.count; i++) {
      /* Calculate index with explicit size_t casting to avoid sign-conversion
       * issues */
      size_t idx = (cli->history.write_idx + (size_t)CLI_HISTORY_NUM -
                    cli->history.count + i) %
                   (size_t)CLI_HISTORY_NUM;

      char num[24];
      (void)snprintf(num, sizeof(num), "%2zu ", i + 1U);

      cli->write(num, strlen(num));
      cli->write(cli->history.buf[idx], strlen(cli->history.buf[idx]));
      cli->write("\r\n", 2);
    }
  }

  return status;
}
#endif

/**
 * @brief Traverse the commands list and callback the caller for every group and
 * command using cb. cb function takes 3 parameters: Pointer to the command
 * line interpreter struct, pointer to command group struct and pointer to the
 * command struct. for every new group found the command struct argument is set
 * to NULL. cb return \link CLI_CMD_LIST_TRV_NEXT \endlink to indicated to
 * traverser to continue traversing,  \link CLI_CMD_LIST_TRV_SKIP \endlink to
 * skip the current group \link CLI_CMD_LIST_TRV_END \endlink to end traversing.
 * @param cli the command line interpreter struct
 * @param cb callback function
 * @return int 0 if the whole list was traversed, 1 if the traversing ended at a
 * group or 2  if the traversing ended at a command
 */
static int cli_cmd_list_traverser(cli_t *cli,
                                  int (*cb)(cli_t *, const cli_cmd_group_t *,
                                            const cli_cmd_t *)) {
  int status = 0;
  bool keep_running = true;

  if (cli->cmd_list == NULL) {
    keep_running = false;
  }

  /* 1. Traverse top-level commands */
  if (keep_running && (cli->cmd_list->cmds != NULL)) {
    for (size_t i = 0; (i < cli->cmd_list->cmds_length) && keep_running; i++) {
      int ret = cb(cli, NULL, &cli->cmd_list->cmds[i]);
      if (ret == CLI_CMD_LIST_TRV_END) {
        status = 2;
        keep_running = false;
      }
    }
  }

  /* 2. Traverse groups */
  if (keep_running && (cli->cmd_list->groups == NULL)) {
    keep_running = false;
  }

  if (keep_running) {
    for (size_t i = 0; (i < cli->cmd_list->length) && keep_running; i++) {
      const cli_cmd_group_t *group = cli->cmd_list->groups[i];
      int ret = cb(cli, group, NULL);

      if (ret == CLI_CMD_LIST_TRV_SKIP) {
        /* Skip this group and move to next i */
        continue;
      } else if (ret == CLI_CMD_LIST_TRV_END) {
        status = 1;
        keep_running = false;
      } else {
        /* Standard flow: check group commands */
        if (group->cmds != NULL) {
          for (size_t j = 0; (j < group->length) && keep_running; j++) {
            const cli_cmd_t *cmd = &group->cmds[j];
            ret = cb(cli, group, cmd);

            if (ret == CLI_CMD_LIST_TRV_SKIP) {
              continue;
            } else if (ret == CLI_CMD_LIST_TRV_END) {
              status = 2;
              keep_running = false;
            } else {
              /* Carry on */
            }
          }
        }
      }
    }
  }

  return status;
}

static int cli_cmd_help_traverser_cb(cli_t *cli, const cli_cmd_group_t *group,
                                     const cli_cmd_t *cmd) {
  const char *name;
  const char *desc;
  if (group && !cmd) {
    name = group->name;
    desc = group->desc;
    cli->write("\r\n", 2);
  } else {
    if (group != NULL) {
      cli->write(" ", 1);
    }
    name = cmd->name;
    desc = cmd->desc;
  }

  cli->write(name, strlen(name));
  cli->write("\t", 1);
  cli->write(desc, strlen(desc));
  cli->write("\r\n", 2);

  return CLI_CMD_LIST_TRV_NEXT;
}

/**
 * @brief build-in help command handler
 *
 * @param cli the command line interpreter struct
 * @param argc arguments count
 * @param argv arguments vector
 * @return int On success 0 is return. Otherwise non zero value
 */
static int cli_cmd_help(cli_t *cli, int argc, char **argv) {
  int status = 0;
  (void)argv;

  if (argc > 3) {
    status = -1;
  } else {

    for (size_t i = 0; i < ARRAY_SIZE(cli_default_cmd_list); i++) {
      cli->write(cli_default_cmd_list[i].name,
                 strlen(cli_default_cmd_list[i].name));
      cli->write("\t", 1);
      cli->write(cli_default_cmd_list[i].desc,
                 strlen(cli_default_cmd_list[i].desc));
      cli->write("\r\n", 2);
    }

    (void)cli_cmd_list_traverser(cli, cli_cmd_help_traverser_cb);
  }
  return status;
}

/**
 * @brief build-in echo command handler
 *
 * @param cli the command line interpreter struct
 * @param argc arguments count
 * @param argv arguments vector
 * @return int On success 0 is return. Otherwise non zero value
 */
static int cli_cmd_echo(cli_t *cli, int argc, char **argv) {

  int status = 0;
  if (argc != 2) {
    status = -1;
  } else {

    if (!strcmp("on", argv[1])) {
      cli->echo = true;
    } else if (!strcmp("off", argv[1])) {
      cli->echo = false;
    } else {
      status = -2;
    }
  }
  return status;
}

/**
 * @brief build-in quit command handler
 *
 * @param cli the command line interpreter struct
 * @param argc arguments count
 * @param argv arguments vector
 * @return int On success 0 is return. Otherwise non zero value
 */
static int cli_cmd_quit(cli_t *cli, int argc, char **argv) {

  int status = 0;
  (void)cli;
  (void)argv;

  if (argc != 1) {
    status = -1;
  } else {
    cli->cmd_quit_cb();
  }

  return status;
}

/**
 * @brief write buffer pointed to by ptr of size size to the command line
 * interpreter write to output function. If the echoing is not enable nothing is
 * written
 * @param cli the command line interpreter struct
 * @param ptr pointer to buffer to be written
 * @param size number of byte to be written
 */
static void cli_echo(cli_t *cli, const void *ptr, size_t size) {
  if (cli->echo) {
    cli->write(ptr, size);
    cli->flush();
  }
}

/**
 * @brief tokenise the command line interpreter line using space and tab as
 * token delimiter
 *
 * @param cli the command line interpreter struct
 * @return int number of tokens found. -1 if number of token exceeded  \link
 * CLI_ARGV_NUM \endlink
 */
static int cli_tokenize(cli_t *cli) {
  int status = 0;
  char *token;
  char *saveptr = cli->line;

  cli->argc = 0;

  for (token = strtok_r(cli->line, " \t", &saveptr); token != NULL;
       token = strtok_r(NULL, " \t", &saveptr)) {

    if ((size_t)cli->argc >= ARRAY_SIZE(cli->argv)) {
      status = -1;
      break;
    }

    cli->argv[cli->argc] = token;
    cli->argc++;
  }
  return (status == 0) ? cli->argc : -1;
}

void cli_print_prompt(cli_t *cli) {
  cli->write(cli->prompt, strlen(cli->prompt));
  cli->write("> ", 2);
  cli->flush();
}

#ifdef CLI_USE_HISTORY
static void cli_history_navigate(cli_t *cli, bool up) {
  if (cli->history.count > 0U) {

    if (up) { // UP or CTRL-P
      if (cli->history.browse_idx == -1) {
        cli->history.browse_idx = (int)cli->history.count - 1;
      } else if (cli->history.browse_idx > 0) {
        cli->history.browse_idx--;
      } else {
        /* Already at the oldest command, do nothing*/
      }
    } else { // DOWN or CTRL-N
      if (cli->history.browse_idx != -1) {
        if (cli->history.browse_idx < (int)cli->history.count - 1) {
          cli->history.browse_idx++;
        } else {
          cli->history.browse_idx = -1;
        }
      }
    }

    // Clear current line
    while (cli->ptr > cli->line) {
      cli_echo(cli, "\b \b", 3);
      cli->ptr--;
    }

    if (cli->history.browse_idx != -1) {
      size_t idx = (cli->history.write_idx + CLI_HISTORY_NUM -
                    cli->history.count + (size_t)cli->history.browse_idx) %
                   CLI_HISTORY_NUM;
      (void)strncpy(cli->line, cli->history.buf[idx], CLI_LINE_MAX - 1U);
      cli->line[CLI_LINE_MAX - 1U] = '\0';
      cli->ptr = &cli->line[strlen(cli->line)];
      cli_echo(cli, cli->line, strlen(cli->line));
    } else {
      *cli->line = '\0';
      cli->ptr = cli->line;
    }
  } else {
    /* No history available: do nothing */
  }
}
#endif /* CLI_USE_HISTORY */

/**
 * @brief read bytes from the receive buffer and add them to the line buffer.
 * Only printing character are added, If newline delimiter is found the the
 * function return the strlen of line
 * @param cli the command line interpreter struct
 * @return size_t strlen of the line. 0 if the receive buffer was emptied before
 * detecting a new line
 */
static size_t cli_getline(cli_t *cli) {
  size_t result = 0U;
  bool done = false;

  if (cli->ptr == NULL) {
    cli->ptr = cli->line;
    *cli->ptr = '\0';
  }

  char ch;
  /* Keep getting chars until buffer is empty OR we have a terminal condition */
  while (!done && (ringbuffer_get(&cli->rb_inbuf, (uint8_t *)&ch) == 0)) {

    switch (ch) {
    case '\r':
    case '\n': {
      char next_ch;
      /* Consume extra CRLF characters */
      while (ringbuffer_peek(&cli->rb_inbuf, (uint8_t *)&next_ch) == 0) {
        if ((next_ch != '\r') && (next_ch != '\n')) {
          break;
        }
        (void)ringbuffer_get(&cli->rb_inbuf, (uint8_t *)&next_ch);
      }

      cli->write("\r\n", 2);
      cli->flush();
      cli->ptr = NULL;
      result = strlen(cli->line);

      if (result == 0U) {
        cli_print_prompt(cli);
      }
      done = true; /* Exit loop and return result */
      break;
    }

    case 0x15: // CTRL-U
      while (cli->ptr != cli->line) {
        cli_echo(cli, "\b \b", 3);
        --cli->ptr;
      }
      *cli->ptr = '\0';
      break;

    case 0x03: // CTRL-C
      cli->write("^C\r\n", 4);
      cli->ptr = cli->line;
      *cli->ptr = '\0';
      cli_print_prompt(cli);
      break;

    case 0x0C:          // CTRL-L
      cli->write("\x1b" /* ANSI escape character */
                 "[2J"  /* Clear entire screen */
                 "\x1b" /* Move cursor to home position (top-left corner) */
                 "[H",  /* Move cursor to home position (top-left corner) */
                 7);
      cli_print_prompt(cli);
      cli_echo(cli, cli->line, strlen(cli->line));
      break;

    case 0x17: // CTRL-W
      while ((cli->ptr > cli->line) && isspace((unsigned char)cli->ptr[-1])) {
        cli_echo(cli, "\b \b", 3);
        --cli->ptr;
      }
      while ((cli->ptr > cli->line) && !isspace((unsigned char)cli->ptr[-1])) {
        cli_echo(cli, "\b \b", 3);
        --cli->ptr;
      }
      *cli->ptr = '\0';
      break;

    case 0x10: // CTRL-P
#ifdef CLI_USE_HISTORY
      cli_history_navigate(cli, true);
#endif /* CLI_USE_HISTORY */
      break;

    case 0x0E: // CTRL-N
#ifdef CLI_USE_HISTORY
      cli_history_navigate(cli, false);
#endif
      break;

    case '\e': // ESC
#ifdef CLI_USE_HISTORY
      cli->esc_state = 1;
#else
      cli->write("\r\n", 2);
      cli->ptr = cli->line;
      *cli->ptr = '\0';
      cli_print_prompt(cli);
#endif /* CLI_USE_HISTORY */
      break;

    case '\b': // Backspace
    case 0x7f:
      if (cli->ptr > cli->line) {
        cli->ptr--;
        *(cli->ptr) = '\0';
        cli_echo(cli, "\b \b", 3);
      }
      break;

    default:
#ifdef CLI_USE_HISTORY
      if (cli->esc_state == 1) {
        cli->esc_state = (ch == '[') ? (int)2 : (int)0;
        break;
      } else if (cli->esc_state == 2) {
        cli->esc_state = 0;
        if ((ch == 'A') /* UP */ || (ch == 'B') /* DOWN */) {
          cli_history_navigate(cli, (ch == 'A'));
        }
        break;
      } else {
        ;
      }
#endif
      if (isprint((int)ch) != 0) {
        /* Calculate the maximum allowed address for a new character.
         * We subtract 1U from sizeof to account for the null terminator. */
        const char *const limit = &cli->line[sizeof(cli->line) - 1U];

        /* Rule 18.3 allows comparison of pointers into the same array.
         * This avoids pointer subtraction (Rule 18.4) and the resulting cast
         * (Rule 10.8). */
        if (cli->ptr < limit) {
          *(cli->ptr) = ch;
          cli->ptr++;
          *cli->ptr = '\0';
          cli_echo(cli, &ch, 1U);
        } else {
          static const char *const MSG_LEN_ERR =
              "Error: The line length exceeds maximum of CLI_LINE_MAX\r\n";
          cli->write("\r\n", 2);
          cli->write(MSG_LEN_ERR, strlen(MSG_LEN_ERR));
          cli->ptr = NULL;
          cli_print_prompt(cli);
          result = 0U;
          done = true; /* Line too long, stop processing this batch */
        }
      }
      break;
    }
  }

  return result;
}

int cli_putchar(cli_t *cli, int ch) {
  int ret;
  if (cli->lock != NULL) {
    cli->lock();
  }
  ret = ringbuffer_put(&cli->rb_inbuf, ch) ? -1 : ch;
  if (cli->unlock != NULL) {
    cli->unlock();
  }
  return ret;
}

int cli_puts(cli_t *cli, const char *str) {
  int status = 0;
  const char *p = str;

  if (cli->lock != NULL) {
    cli->lock();
  }

  while (*p != '\0') {
    if (ringbuffer_put(&cli->rb_inbuf, *p) < 0) {
      status = -1;
      break;
    }
    p++;
  }

  if (cli->unlock != NULL) {
    cli->unlock();
  }

  return status;
}

// Embedded-friendly case-insensitive string comparison
// Use this instead of strcasecmp() which may not be available
static int cli_strcasecmp(const char *s1, const char *s2) {
  int status = 0;
  const char *p1 = s1;
  const char *p2 = s2;

  while ((*p1 != '\0') && (*p2 != '\0')) {
    unsigned char c1 = (unsigned char)*p1;
    unsigned char c2 = (unsigned char)*p2;
    int diff = tolower(c1) - tolower(c2);
    if (diff != 0) {
      status = diff;
      break;
    }
    p1++;
    p2++;
  }
  // Both must be at end to be equal
  return (status != 0)
             ? status
             : (tolower((unsigned char)*p1) - tolower((unsigned char)*p2));
}

static int cli_cmd_run_traverser_cb(cli_t *cli, const cli_cmd_group_t *group,
                                    const cli_cmd_t *cmd) {
  int result = CLI_CMD_LIST_TRV_NEXT;

  if ((group != NULL) && (cmd == NULL)) {
    if (cli_strcasecmp(cli->argv[0], group->name) != 0) {
      result = CLI_CMD_LIST_TRV_SKIP;
    }
  } else if (cmd != NULL) {
    const char *cmd_name_to_match = NULL;
    bool match_found = true;

    if (group != NULL) {
      if (cli->argc < 2) {
        result = CLI_CMD_LIST_TRV_SKIP;
        match_found = false;
      } else {
        cmd_name_to_match = cli->argv[1];
      }
    } else {
      cmd_name_to_match = cli->argv[0];
    }

    if (match_found) {
      if (cli_strcasecmp(cmd_name_to_match, cmd->name) != 0) {
        result = CLI_CMD_LIST_TRV_SKIP;
      } else {
        cli_cmd_handler_t handler =
            (cmd->handler != NULL) ? cmd->handler : cli_cmd_default_handler;

        if (handler(cli, cli->argc, cli->argv) == 0) {
          cli->write(CLI_MSG_CMD_OK, strlen(CLI_MSG_CMD_OK));
        } else {
          cli->write(CLI_MSG_CMD_ERROR, strlen(CLI_MSG_CMD_ERROR));
        }

        result = CLI_CMD_LIST_TRV_END;
      }
    }
  } else {
    /* MISRA Rule 15.7: All if...else if constructs shall be terminated with an
     * else statement */
  }

  return result;
}

void cli_register_quit_callback(cli_t *cli, void (*cmd_quit_cb)(void)) {
  cli->cmd_quit_cb = cmd_quit_cb ? cmd_quit_cb : cli_cmd_quit_default_cb;
}

void cli_mainloop(cli_t *cli) {
  size_t len;
#ifdef CLI_USE_HISTORY
  char line_copy[CLI_LINE_MAX];
#endif

  if (cli->lock != NULL) {
    cli->lock();
  }
  len = cli_getline(cli);
  if (cli->unlock != NULL) {
    cli->unlock();
  }

  if (len != 0U) {

#ifdef CLI_USE_HISTORY
    (void)strncpy(line_copy, cli->line, sizeof(line_copy) - 1U);
    line_copy[sizeof(line_copy) - 1U] = '\0';
    cli->history.browse_idx = -1;
#endif

    if (cli_tokenize(cli) < 0) {
      static const char *const CLI_MSG_NUM_ARG_ERR =
          "Error: The number of arguments exceeds maximum of CLI_ARGV_NUM\r\n";
      cli->write(CLI_MSG_NUM_ARG_ERR, strlen(CLI_MSG_NUM_ARG_ERR));
    } else if (cli->argc > 0) {
      bool found = false;

      for (size_t i = 0; i < ARRAY_SIZE(cli_default_cmd_list); i++) {

        if (!cli_strcasecmp(cli->argv[0], cli_default_cmd_list[i].name)) {
#ifdef CLI_USE_HISTORY
          cli_history_push(cli, line_copy);
#endif
          if (cli_default_cmd_list[i].handler(cli, cli->argc, cli->argv) == 0) {
            cli->write(CLI_MSG_CMD_OK, strlen(CLI_MSG_CMD_OK));
          } else {
            cli->write(CLI_MSG_CMD_ERROR, strlen(CLI_MSG_CMD_ERROR));
          }
          found = true;
          break;
        }
      }

      if (!found) {
        if (cli_cmd_list_traverser(cli, cli_cmd_run_traverser_cb) != 0) {
#ifdef CLI_USE_HISTORY
          cli_history_push(cli, line_copy);
#endif
        } else {
          static const char *const CLI_MSG_CMD_UNKNOWN = "Unknown command\r\n";
          cli->write(CLI_MSG_CMD_UNKNOWN, strlen(CLI_MSG_CMD_UNKNOWN));
        }
      }
    } else {
      /* Empty command line */
    }

    cli_print_prompt(cli);
  } else {
    /* MISRA Rule 15.7: All if...else if constructs shall be terminated with an
     * else statement */
  }
}

void cli_init(cli_t *cli, const cli_cmd_list_t *cmd_list) {

  ringbuffer_wrap(&cli->rb_inbuf, (uint8_t *)cli->inbuf, sizeof(cli->inbuf));

  cli->echo = true;
  cli->ptr = NULL;

  cli->prompt = CLI_PROMPT;
  cli->write = cli_default_write;
  cli->flush = cli_default_flush;

  cli->lock = NULL;
  cli->unlock = NULL;

#ifdef CLI_USE_HISTORY
  cli->history.count = 0;
  cli->history.write_idx = 0;
  cli->history.browse_idx = -1;
  cli->esc_state = 0;
#endif

  cli->cmd_quit_cb = cli_cmd_quit_default_cb;

  cli->cmd_list = cmd_list;
}
