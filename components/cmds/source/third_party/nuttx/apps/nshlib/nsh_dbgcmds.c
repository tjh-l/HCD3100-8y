/****************************************************************************
 * apps/nshlib/dbg_dbgcmds.c
 *
 *   Copyright (C) 2008-2009, 2011-2012, 2015, 2017-2018 Gregory Nutt. All
 *     rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <generated/br2_autoconf.h>

#include <sys/types.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#include "nsh.h"
#include "nsh_console.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#undef NSH_HAVE_MEMCMDS
#if !defined(CONFIG_NSH_DISABLE_MB) || !defined(CONFIG_NSH_DISABLE_MH) || \
    !defined(CONFIG_NSH_DISABLE_MW)
#  define NSH_HAVE_MEMCMDS 1
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

#ifdef NSH_HAVE_MEMCMDS
struct dbgmem_s
{
  bool         dm_write;  /* true: perform write operation */
  FAR void    *dm_addr;   /* Address to access */
  uint32_t     dm_value;  /* Value to write */
  unsigned int dm_count;  /* The number of bytes to access */
};
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: mem_parse
 ****************************************************************************/

#ifdef NSH_HAVE_MEMCMDS
static int mem_parse(FAR struct nsh_vtbl_s *vtbl, int argc, FAR char **argv,
                     FAR struct dbgmem_s *mem)
{
  FAR char *pcvalue = strchr(argv[1], '=');
  unsigned long lvalue = 0;

  /* Check if we are writing a value */

  if (pcvalue)
    {
      *pcvalue = '\0';
      pcvalue++;

      lvalue = strtoul(pcvalue, NULL, 16);
      if (lvalue > 0xffffffffl)
        {
          return -EINVAL;
        }

      mem->dm_write = true;
      mem->dm_value = (uint32_t)lvalue;
    }
  else
    {
      mem->dm_write = false;
      mem->dm_value = 0;
    }

  /* Get the address to be accessed */

  mem->dm_addr = (FAR void *)((uintptr_t)strtoul(argv[1], NULL, 16));

  /* Get the number of bytes to access */

  if (argc > 2)
    {
      mem->dm_count = (unsigned int)strtoul(argv[2], NULL, 16);
    }
  else
    {
      mem->dm_count = 1;
    }

  return OK;
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: cmd_mb
 ****************************************************************************/

#ifndef CONFIG_NSH_DISABLE_MB
int _cmd_mb(int argc, char **argv)
{
	cmd_mb(&g_vtbl, argc, argv);
}
NSH_CONSOLE_CMD(mb, "nsh", _cmd_mb, CONSOLE_CMD_MODE_SELF, "<hex-address>[=<hex-value>] [<hex-byte-count>]")
int cmd_mb(FAR struct nsh_vtbl_s *vtbl, int argc, FAR char **argv)
{
  struct dbgmem_s mem;
  FAR volatile uint8_t *ptr;
  unsigned int i;
  int ret;

  ret = mem_parse(vtbl, argc, argv, &mem);
  if (ret == 0)
    {
      /* Loop for the number of requested bytes */

      for (i = 0, ptr = (volatile uint8_t *)mem.dm_addr; i < mem.dm_count;
           i++, ptr++)
        {
          /* Print the value at the address */

          nsh_output(vtbl, "  %p = 0x%02x", ptr, *ptr);

          /* Are we supposed to write a value to this address? */

          if (mem.dm_write)
            {
              /* Yes, was the supplied value within range? */

              if (mem.dm_value > 0x000000ff)
                {
                  nsh_error(vtbl, g_fmtargrange, argv[0]);
                  return ERROR;
                }

              /* Write the value and re-read the address so that we print its
               * current value (if the address is a process address, then the
               * value read might not necessarily be the value written).
               */

              *ptr = (uint8_t)mem.dm_value;
              nsh_output(vtbl, " -> 0x%02x", *ptr);
            }

          /* Make sure we end it with a newline */

          nsh_output(vtbl, "\n", *ptr);
        }
    }

  return ret;
}
#endif

/****************************************************************************
 * Name: cmd_mh
 ****************************************************************************/

#ifndef CONFIG_NSH_DISABLE_MH
int _cmd_mh(int argc, char **argv)
{
	cmd_mh(&g_vtbl, argc, argv);
}
NSH_CONSOLE_CMD(mh, "nsh", _cmd_mh, CONSOLE_CMD_MODE_SELF, "<hex-address>[=<hex-value>] [<hex-byte-count>]")
int cmd_mh(FAR struct nsh_vtbl_s *vtbl, int argc, FAR char **argv)
{
  struct dbgmem_s mem;
  FAR volatile uint16_t *ptr;
  unsigned int i;
  int ret;

  ret = mem_parse(vtbl, argc, argv, &mem);
  if (ret == 0)
    {
      /* Loop for the number of requested bytes */

      for (i = 0, ptr = (volatile uint16_t *)mem.dm_addr;
           i < mem.dm_count;
           i += 2, ptr++)
        {
          /* Print the value at the address */

          nsh_output(vtbl, "  %p = 0x%04x", ptr, *ptr);

          /* Are we supposed to write a value to this address? */

          if (mem.dm_write)
            {
              /* Yes, was the supplied value within range? */

              if (mem.dm_value > 0x0000ffff)
                {
                  nsh_error(vtbl, g_fmtargrange, argv[0]);
                  return ERROR;
                }

              /* Write the value and re-read the address so that we print its
               * current value (if the address is a process address, then the
               * value read might not necessarily be the value written).
               */

              *ptr = (uint16_t)mem.dm_value;
              nsh_output(vtbl, " -> 0x%04x", *ptr);
            }

          /* Make sure we end it with a newline */

          nsh_output(vtbl, "\n", *ptr);
        }
    }

  return ret;
}
#endif

/****************************************************************************
 * Name: cmd_mw
 ****************************************************************************/

#ifndef CONFIG_NSH_DISABLE_MW
int _cmd_mw(int argc, char **argv)
{
	cmd_mw(&g_vtbl, argc, argv);
}
NSH_CONSOLE_CMD(mw, "nsh", _cmd_mw, CONSOLE_CMD_MODE_SELF, "<hex-address>[=<hex-value>] [<hex-byte-count>]")
int cmd_mw(FAR struct nsh_vtbl_s *vtbl, int argc, FAR char **argv)
{
  struct dbgmem_s mem;
  FAR volatile uint32_t *ptr;
  unsigned int i;
  int ret;

  ret = mem_parse(vtbl, argc, argv, &mem);
  if (ret == 0)
    {
      /* Loop for the number of requested bytes */

      for (i = 0, ptr = (volatile uint32_t *)mem.dm_addr; i < mem.dm_count;
           i += 4, ptr++)
        {
          /* Print the value at the address */

          nsh_output(vtbl, "  %p = 0x%08x", ptr, *ptr);

          /* Are we supposed to write a value to this address? */

          if (mem.dm_write)
            {
              /* Write the value and re-read the address so that we print its
               * current value (if the address is a process address, then the
               * value read might not necessarily be the value written).
               */

              *ptr = mem.dm_value;
              nsh_output(vtbl, " -> 0x%08x", *ptr);
            }

          /* Make sure we end it with a newline */

          nsh_output(vtbl, "\n", *ptr);
        }
    }

  return ret;
}

int cmd_mw2(FAR struct nsh_vtbl_s *vtbl, int argc, FAR char **argv)
{
  struct dbgmem_s mem;
  FAR volatile uint32_t *ptr;
  unsigned int i;
  int ret;

  ret = mem_parse(vtbl, argc, argv, &mem);
  if (ret == 0)
    {
      /* Loop for the number of requested bytes */

      for (i = 0, ptr = (volatile uint32_t *)mem.dm_addr; i < mem.dm_count;
           i += 4, ptr++)
        {
          /* Are we supposed to write a value to this address? */

          if (mem.dm_write)
            {
              /* Write the value and re-read the address so that we print its
               * current value (if the address is a process address, then the
               * value read might not necessarily be the value written).
               */

              *ptr = mem.dm_value;
            }
        }
    }

  return ret;
}
int _cmd_mw2(int argc, char **argv)
{
	cmd_mw2(&g_vtbl, argc, argv);
}
NSH_CONSOLE_CMD(mw2, "nsh", _cmd_mw2, CONSOLE_CMD_MODE_SELF, "<hex-address>[=<hex-value>] [<hex-byte-count>]")
#endif

/****************************************************************************
 * Name: nsh_dumpbuffer
 ****************************************************************************/

void nsh_dumpbuffer(FAR struct nsh_vtbl_s *vtbl, FAR const char *msg,
                    FAR const uint8_t *buffer, ssize_t nbytes)
{
  char line[128];
  int ch;
  int i;
  int j;

  nsh_output(vtbl, "%s:\n", msg);
  for (i = 0; i < nbytes; i += 16)
    {
      sprintf(line, "%04x: ", i);

      for (j = 0; j < 16; j++)
        {
          if (i + j < nbytes)
            {
              sprintf(&line[strlen(line)], "%02x ", buffer[i + j]);
            }
          else
            {
              strcpy(&line[strlen(line)], "   ");
            }
        }

      for (j = 0; j < 16; j++)
        {
          if (i + j < nbytes)
            {
              ch = buffer[i + j];
              sprintf(&line[strlen(line)], "%c",
                      ch >= 0x20 && ch <= 0x7e ? ch : '.');
            }
        }

      nsh_output(vtbl, "%s\n", line);
    }
}

void nsh_dumpbuffer2(FAR struct nsh_vtbl_s *vtbl, FAR const char *msg,
                    FAR const uint8_t *buffer, ssize_t nbytes, int width)
{
  char line[128];
  int ch;
  int i;
  int j;

  nsh_output(vtbl, "%s:\n", msg);
  for (i = 0; i < nbytes; i += 16)
    {
      sprintf(line, "%08x: ", (unsigned int)(buffer) + i);

      if (width == 1)
        {
          for (j = 0; j < 16; j++)
            {
              if (i + j < nbytes)
                {
                  sprintf(&line[strlen(line)], "%02x ", buffer[i + j]);
                }
              else
                {
                  strcpy(&line[strlen(line)], "   ");
                }
            }
        }
      if (width == 2)
        {
          for (j = 0; j < 16; j += 2)
            {
              if (i + j < nbytes)
                {
                  sprintf(&line[strlen(line)], "%04x ", *(unsigned short *)(buffer + i + j));
                }
              else
                {
                  strcpy(&line[strlen(line)], "     ");
                }
            }
        }
      if (width == 4)
        {
          for (j = 0; j < 16; j += 4)
            {
              if (i + j < nbytes)
                {
                  sprintf(&line[strlen(line)], "%08x ", *(unsigned int *)(buffer + i + j));
                }
              else
                {
                  strcpy(&line[strlen(line)], "         ");
                }
            }
        }

      for (j = 0; j < 16; j++)
        {
          if (i + j < nbytes)
            {
              ch = buffer[i + j];
              sprintf(&line[strlen(line)], "%c",
                      ch >= 0x20 && ch <= 0x7e ? ch : '.');
            }
        }

      nsh_output(vtbl, "%s\n", line);
    }
}

/****************************************************************************
 * Name: cmd_xd, hex dump of memory
 ****************************************************************************/

#ifndef CONFIG_NSH_DISABLE_XD
int _cmd_xd(int argc, char **argv)
{
	cmd_xd(&g_vtbl, argc, argv);
}
NSH_CONSOLE_CMD(xd, "nsh", _cmd_xd, CONSOLE_CMD_MODE_SELF, "<hex-address> <byte-count>")
int cmd_xd(FAR struct nsh_vtbl_s *vtbl, int argc, FAR char **argv)
{
  FAR char *addr;
  FAR char *endptr;
  int       nbytes = 4;
  int       width = 4;

  addr = (FAR char *)((uintptr_t)strtoul(argv[1], &endptr, 16));
  if (argv[0][0] == '\0' || *endptr != '\0')
    {
      return ERROR;
    }
  if (!((unsigned)addr & 0x80000000))
    {
      addr = (char *)((unsigned)addr | 0xA0000000);
    }

  if (argc >= 3)
    {
      nbytes = (int)strtol(argv[2], &endptr, 0);
      if (argv[0][0] == '\0' || *endptr != '\0' || nbytes < 0)
        {
          return ERROR;
        }
    }
  if (argc == 4)
    {
      width = (int)strtol(argv[3], &endptr, 0);
      if (argv[0][0] == '\0' || *endptr != '\0' || width < 0)
        {
          return ERROR;
        }
      if (width != 1 && width != 2 && width != 4)
        {
          width = 4;
        }
    }

  if ((unsigned)addr & (width - 1))
    {
      nbytes += (unsigned)addr & (width - 1);
      addr -= (unsigned)addr & (width - 1);
    }
  if (nbytes & (width - 1))
    {
      nbytes += width - (nbytes & (width - 1));
    }

  nsh_dumpbuffer2(vtbl, "Hex dump", (uint8_t *)addr, nbytes, width);
  return OK;
}
#endif

/****************************************************************************
 * Name: cmd_hexdump, hex dump of files
 ****************************************************************************/

#ifndef CONFIG_NSH_DISABLE_HEXDUMP
int _cmd_hexdump(int argc, char **argv)
{
	cmd_hexdump(&g_vtbl, argc, argv);
}
NSH_CONSOLE_CMD(hexdump, "nsh", _cmd_hexdump, CONSOLE_CMD_MODE_SELF, "<file or device> [skip=<bytes>] [count=<bytes>]")
int cmd_hexdump(FAR struct nsh_vtbl_s *vtbl, int argc, FAR char **argv)
{
  FAR uint8_t *buffer;
  char msg[32];
  off_t position;
  int fd;
  int ret = OK;
#ifdef CONFIG_NSH_CMDOPT_HEXDUMP
  off_t skip = 0;
  off_t count = 0xfffffff;
  off_t dumpbytes;
  int x;
#endif

  /* Open the file for reading */

  fd = open(argv[1], O_RDONLY);
  if (fd < 0)
    {
      nsh_error(vtbl, g_fmtcmdfailed, "hexdump", "open", NSH_ERRNO);
      return ERROR;
    }

  buffer = (FAR uint8_t *)malloc(IOBUFFERSIZE);
  if (buffer == NULL)
    {
      close(fd);
      nsh_error(vtbl, g_fmtcmdfailed, "hexdump", "malloc", NSH_ERRNO);
      return ERROR;
    }

#ifdef CONFIG_NSH_CMDOPT_HEXDUMP
  for (x = 2; x < argc; x++)
    {
      if (strncmp(argv[x], "skip=", 5) == 0)
        {
          skip = atoi(&argv[x][5]);
        }
      else if (strncmp(argv[x], "count=", 6) == 0)
        {
          count = atoi(&argv[x][6]);
        }
    }
#endif

  position = 0;
  for (; ; )
    {
      int nbytesread = read(fd, buffer, IOBUFFERSIZE);

      /* Check for read errors */

      if (nbytesread < 0)
        {
          int errval = errno;
          nsh_error(vtbl, g_fmtcmdfailed, "hexdump", "read",
                     NSH_ERRNO_OF(errval));
          ret = ERROR;
          break;
        }
      else if (nbytesread > 0)
        {
#ifdef CONFIG_NSH_CMDOPT_HEXDUMP
          if (position < skip)
            {
              /* Skip bytes until we reach the skip point */

              position += nbytesread;
              if (position > skip)
                {
                  dumpbytes = position - skip;
                  if (dumpbytes > count)
                    {
                      dumpbytes = count;
                    }

                  snprintf(msg, sizeof(msg), "%s at %08llx", argv[1],
                           (uintmax_t)skip);
                  nsh_dumpbuffer(vtbl, msg,
                                 &buffer[nbytesread - (position - skip)],
                                 dumpbytes);

                  if (count > dumpbytes)
                    {
                      count -= dumpbytes;
                    }
                  else
                    {
                      break;
                    }
                }

              /* Don't print if we are in skip mode */

              continue;
            }

          /* Limit dumpbuffer to count if less than a full buffer needed */

          if (nbytesread > count)
            {
              nbytesread = count;
            }
#endif

          snprintf(msg, sizeof(msg), "%s at %08llx", argv[1],
                   (uintmax_t)position);
          nsh_dumpbuffer(vtbl, msg, buffer, nbytesread);
          position += nbytesread;

#ifdef CONFIG_NSH_CMDOPT_HEXDUMP
          if (count > nbytesread)
            {
              count -= nbytesread;
            }
          else
            {
              break;
            }
#endif
        }
      else
        {
          break; /* EOF */
        }
    }

  close(fd);
  free(buffer);
  return ret;
}
#endif

/****************************************************************************
 * Name: cmd_irqinfo
 ****************************************************************************/

#ifdef HAVE_IRQINFO
int cmd_irqinfo(FAR struct nsh_vtbl_s *vtbl, int argc, FAR char **argv)
{
  return nsh_catfile(vtbl, argv[0], CONFIG_NSH_PROC_MOUNTPOINT "/irqs");
}
#endif
