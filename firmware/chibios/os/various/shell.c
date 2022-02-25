/*
    ChibiOS/RT - Copyright (C) 2006-2013 Giovanni Di Sirio

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

/**
 * @file    shell.c
 * @brief   Simple CLI shell code.
 *
 * @addtogroup SHELL
 * @{
 */

#include <string.h>

#include "ch.h"
#include "hal.h"
#include "shell.h"
#include "chprintf.h"

/**
 * @brief   Shell termination event source.
 */
EventSource shell_terminated;

static char *_strtok(char *str, const char *delim, char **saveptr) {
  char *token;
  if (str)
    *saveptr = str;
  token = *saveptr;

  if (!token)
    return NULL;

  token += strspn(token, delim);
  *saveptr = strpbrk(token, delim);
  if (*saveptr)
    *(*saveptr)++ = '\0';

  return *token ? token : NULL;
}

static void usage(BaseSequentialStream *chp, char *p) {

  chprintf(chp, "Usage: %s\r\n", p);
}

static void list_commands(BaseSequentialStream *chp, const ShellCommand *scp) {

  while (scp->sc_name != NULL) {
    chprintf(chp, "%s ", scp->sc_name);
    scp++;
  }
}

static void cmd_info(BaseSequentialStream *chp, int argc, char *argv[]) {

  (void)argv;
  if (argc > 0) {
    usage(chp, "info");
    return;
  }

  chprintf(chp, "Kernel:       %s\r\n", CH_KERNEL_VERSION);
#ifdef CH_COMPILER_NAME
  chprintf(chp, "Compiler:     %s\r\n", CH_COMPILER_NAME);
#endif
  chprintf(chp, "Architecture: %s\r\n", CH_ARCHITECTURE_NAME);
#ifdef CH_CORE_VARIANT_NAME
  chprintf(chp, "Core Variant: %s\r\n", CH_CORE_VARIANT_NAME);
#endif
#ifdef CH_PORT_INFO
  chprintf(chp, "Port Info:    %s\r\n", CH_PORT_INFO);
#endif
#ifdef PLATFORM_NAME
  chprintf(chp, "Platform:     %s\r\n", PLATFORM_NAME);
#endif
#ifdef BOARD_NAME
  chprintf(chp, "Board:        %s\r\n", BOARD_NAME);
#endif
#ifdef __DATE__
#ifdef __TIME__
  chprintf(chp, "Build time:   %s%s%s\r\n", __DATE__, " - ", __TIME__);
#endif
#endif
}

static void cmd_systime(BaseSequentialStream *chp, int argc, char *argv[]) {

  (void)argv;
  if (argc > 0) {
    usage(chp, "systime");
    return;
  }
  chprintf(chp, "%lu\r\n", (unsigned long)chTimeNow());
}

/**
 * @brief   Array of the default commands.
 */
static ShellCommand local_commands[] = {
  {"info", cmd_info},
  {"systime", cmd_systime},
  {NULL, NULL}
};

static bool_t cmdexec(const ShellCommand *scp, BaseSequentialStream *chp,
                      char *name, int argc, char *argv[]) {

  while (scp->sc_name != NULL) {
    if (strcasecmp(scp->sc_name, name) == 0) {
      scp->sc_function(chp, argc, argv);
      return FALSE;
    }
    scp++;
  }
  return TRUE;
}

/**
 * @brief   Shell thread function.
 *
 * @param[in] p         pointer to a @p BaseSequentialStream object
 * @return              Termination reason.
 * @retval RDY_OK       terminated by command.
 * @retval RDY_RESET    terminated by reset condition on the I/O channel.
 *
 * @notapi
 */
static msg_t shell_thread(void *p) {
  int n;
  BaseSequentialStream *chp = ((ShellConfig *)p)->sc_channel;
  const ShellCommand *scp = ((ShellConfig *)p)->sc_commands;
  char *lp, *cmd, *tokp, line[SHELL_MAX_LINE_LENGTH];
  char *args[SHELL_MAX_ARGUMENTS + 1];

  chRegSetThreadName("shell");
  chprintf(chp, "\r\nChibiOS/RT Shell\r\n");
  while (TRUE) {
    chprintf(chp, "ch> ");
    if (shellGetLine(chp, line, sizeof(line))) {
      chprintf(chp, "\r\nlogout");
      break;
    }
    lp = _strtok(line, " \t", &tokp);
    cmd = lp;
    n = 0;
    while ((lp = _strtok(NULL, " \t", &tokp)) != NULL) {
      if (n >= SHELL_MAX_ARGUMENTS) {
        chprintf(chp, "too many arguments\r\n");
        cmd = NULL;
        break;
      }
      args[n++] = lp;
    }
    args[n] = NULL;
    if (cmd != NULL) {
      if (strcasecmp(cmd, "exit") == 0) {
        if (n > 0) {
          usage(chp, "exit");
          continue;
        }
        break;
      }
      else if (strcasecmp(cmd, "help") == 0) {
        if (n > 0) {
          usage(chp, "help");
          continue;
        }
        chprintf(chp, "Commands: help exit ");
        list_commands(chp, local_commands);
        if (scp != NULL)
          list_commands(chp, scp);
        chprintf(chp, "\r\n");
      }
      else if (cmdexec(local_commands, chp, cmd, n, args) &&
          ((scp == NULL) || cmdexec(scp, chp, cmd, n, args))) {
        chprintf(chp, "%s", cmd);
        chprintf(chp, " ?\r\n");
      }
    }
  }
  shellExit(RDY_OK);
  /* Never executed, silencing a warning.*/
  return 0;
}

/**
 * @brief   Shell manager initialization.
 *
 * @api
 */
void shellInit(void) {

  chEvtInit(&shell_terminated);
}

/**
 * @brief   Terminates the shell.
 * @note    Must be invoked from the command handlers.
 * @note    Does not return.
 *
 * @param[in] msg       shell exit code
 *
 * @api
 */
void shellExit(msg_t msg) {

  /* Atomically broadcasting the event source and terminating the thread,
     there is not a chSysUnlock() because the thread terminates upon return.*/
  chSysLock();
  chEvtBroadcastI(&shell_terminated);
  chThdExitS(msg);
}

/**
 * @brief   Spawns a new shell.
 * @pre     @p CH_USE_HEAP and @p CH_USE_DYNAMIC must be enabled.
 *
 * @param[in] scp       pointer to a @p ShellConfig object
 * @param[in] size      size of the shell working area to be allocated
 * @param[in] prio      priority level for the new shell
 * @return              A pointer to the shell thread.
 * @retval NULL         thread creation failed because memory allocation.
 *
 * @api
 */
#if CH_USE_HEAP && CH_USE_DYNAMIC
Thread *shellCreate(const ShellConfig *scp, size_t size, tprio_t prio) {

  return chThdCreateFromHeap(NULL, size, prio, shell_thread, (void *)scp);
}
#endif

/**
 * @brief   Create statically allocated shell thread.
 *
 * @param[in] scp       pointer to a @p ShellConfig object
 * @param[in] wsp       pointer to a working area dedicated to the shell thread stack
 * @param[in] size      size of the shell working area
 * @param[in] prio      priority level for the new shell
 * @return              A pointer to the shell thread.
 *
 * @api
 */
Thread *shellCreateStatic(const ShellConfig *scp, void *wsp,
                          size_t size, tprio_t prio) {

  return chThdCreateStatic(wsp, size, prio, shell_thread, (void *)scp);
}

/**
 * @brief   Reads a whole line from the input channel.
 *
 * @param[in] chp       pointer to a @p BaseSequentialStream object
 * @param[in] line      pointer to the line buffer
 * @param[in] size      buffer maximum length
 * @return              The operation status.
 * @retval TRUE         the channel was reset or CTRL-D pressed.
 * @retval FALSE        operation successful.
 *
 * @api
 */
bool_t shellGetLine(BaseSequentialStream *chp, char *line, unsigned size) {
  char *p = line;

  while (TRUE) {
    char c;

    if (chSequentialStreamRead(chp, (uint8_t *)&c, 1) == 0)
      return TRUE;
    if (c == 4) {
      chprintf(chp, "^D");
      return TRUE;
    }
    if ((c == 8) || (c == 127)) {
      if (p != line) {
        chSequentialStreamPut(chp, c);
        chSequentialStreamPut(chp, 0x20);
        chSequentialStreamPut(chp, c);
        p--;
      }
      continue;
    }
    if (c == '\r') {
      chprintf(chp, "\r\n");
      *p = 0;
      return FALSE;
    }
    if (c < 0x20)
      continue;
    if (p < line + size - 1) {
      chSequentialStreamPut(chp, c);
      *p++ = (char)c;
    }
  }
}

/** @} */
