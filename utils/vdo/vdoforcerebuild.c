/*
 * Copyright Red Hat
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA. 
 */

#include <err.h>
#include <getopt.h>
#include <stdio.h>
#include <unistd.h>

#include "errors.h"
#include "logger.h"

#include "constants.h"
#include "status-codes.h"
#include "types.h"
#include "vdoConfig.h"

#include "fileLayer.h"

static const char usageString[] =
  " [--help] filename";

static const char helpString[] =
  "vdoforcerebuild - prepare a VDO device to exit read-only mode\n"
  "\n"
  "SYNOPSIS\n"
  "  vdoforcerebuild filename\n"
  "\n"
  "DESCRIPTION\n"
  "  vdoforcerebuild forces an existing VDO device to exit read-only\n"
  "  mode and to attempt to regenerate as much metadata as possible.\n"
  "\n"
  "OPTIONS\n"
  "    --help\n"
  "       Print this help message and exit.\n"
  "\n"
  "    --version\n"
  "       Show the version of vdoforcerebuild.\n"
  "\n";

// N.B. the option array must be in sync with the option string.
static struct option options[] = {
  { "help",                  no_argument,       NULL, 'h' },
  { "version",               no_argument,       NULL, 'V' },
  { NULL,                    0,                 NULL,  0  },
};
static char optionString[] = "h";

static void usage(const char *progname, const char *usageOptionsString)
{
  errx(1, "Usage: %s%s\n", progname, usageOptionsString);
}

int main(int argc, char *argv[])
{
  static char errBuf[UDS_MAX_ERROR_MESSAGE_SIZE];

  int result = vdo_register_status_codes();
  if (result != VDO_SUCCESS) {
    errx(1, "Could not register status codes: %s",
         uds_string_error(result, errBuf, UDS_MAX_ERROR_MESSAGE_SIZE));
  }

  int c;
  while ((c = getopt_long(argc, argv, optionString, options, NULL)) != -1) {
    switch (c) {
    case 'h':
      printf("%s", helpString);
      exit(0);
      break;

    case 'V':
      fprintf(stdout, "vdoforcerebuild version is: %s\n", CURRENT_VERSION);
      exit(0);
      break;

    default:
      usage(argv[0], usageString);
      break;
    };
  }

  if (optind != (argc - 1)) {
    usage(argv[0], usageString);
  }

  char *filename = argv[optind];
  PhysicalLayer *layer;
  // Passing 0 physical blocks will make a filelayer to fit the file.
  result = makeFileLayer(filename, 0, &layer);
  if (result != VDO_SUCCESS) {
    errx(result, "makeFileLayer failed on '%s'", filename);
  }

  result = forceVDORebuild(layer);
  if (result != VDO_SUCCESS) {
    char buf[UDS_MAX_ERROR_MESSAGE_SIZE];
    errx(result, "forceRebuild failed on '%s': %s",
         filename, uds_string_error(result, buf, sizeof(buf)));
  }

  layer->destroy(&layer);
}
