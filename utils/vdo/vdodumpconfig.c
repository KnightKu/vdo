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
 *
 * $Id: //eng/linux-vdo/src/c++/vdo/user/vdoDumpConfig.c#21 $
 */

#include <err.h>
#include <getopt.h>
#include <uuid/uuid.h>
#include <stdio.h>

#include "errors.h"
#include "logger.h"
#include "memoryAlloc.h"

#include "constants.h"
#include "types.h"
#include "status-codes.h"
#include "volume-geometry.h"

#include "userVDO.h"
#include "vdoVolumeUtils.h"

static const char usageString[] = "[--help] vdoBacking";

static const char helpString[] =
  "vdodumpconfig - dump the configuration of a VDO volume from its backing\n"
  "                store.\n"
  "\n"
  "SYNOPSIS\n"
  "  vdodumpconfig <vdoBacking>\n"
  "\n"
  "DESCRIPTION\n"
  "  vdodumpconfig dumps the configuration of a VDO volume, whether or not\n"
  "  the VDO is running.\n"
  "OPTIONS\n"
  "    --help\n"
  "       Print this help message and exit.\n"
  "\n"
  "    --version\n"
  "       Show the version of vdodmeventd.\n"
  "\n";

static struct option options[] = {
  { "help",            no_argument,       NULL, 'h' },
  { "version",         no_argument,       NULL, 'V' },
  { NULL,              0,                 NULL,  0  },
};

/**
 * Explain how this command-line tool is used.
 *
 * @param progname           Name of this program
 * @param usageOptionString  Multi-line explanation
 **/
static void usage(const char *progname)
{
  errx(1, "Usage: %s %s\n", progname, usageString);
}

/**
 * Parse the arguments passed; print command usage if arguments are wrong.
 *
 * @param argc  Number of input arguments
 * @param argv  Array of input arguments
 *
 * @return The backing store of the VDO
 **/
static const char *processArgs(int argc, char *argv[])
{
  int   c;
  char *optionString = "h";
  while ((c = getopt_long(argc, argv, optionString, options, NULL)) != -1) {
    switch (c) {
    case 'h':
      printf("%s", helpString);
      exit(0);

    case 'V':
      fprintf(stdout, "vdodumpconfig version is: %s\n", CURRENT_VERSION);
      exit(0);
      break;

    default:
      usage(argv[0]);
      break;
    }
  }

  // Explain usage and exit
  if (optind != (argc - 1)) {
    usage(argv[0]);
  }

  return argv[optind++];
}

/**********************************************************************/
static void readVDOConfig(const char             *vdoBacking,
                          struct vdo_config      *configPtr,
                          struct volume_geometry *geometryPtr)
{
  UserVDO *vdo;
  int result = makeVDOFromFile(vdoBacking, true, &vdo);
  if (result != VDO_SUCCESS) {
    errx(1, "Could not load VDO from '%s'", vdoBacking);
  }

  *configPtr = vdo->states.vdo.config;

  result = vdo_load_volume_geometry(vdo->layer, geometryPtr);
  if (result != VDO_SUCCESS) {
    errx(1, "Could not read VDO geometry from '%s'", vdoBacking);
  }

  freeVDOFromFile(&vdo);
}

/**********************************************************************/
int main(int argc, char *argv[])
{
  static char errBuf[ERRBUF_SIZE];

  int result = register_vdo_status_codes();
  if (result != VDO_SUCCESS) {
    errx(1, "Could not register status codes: %s",
         uds_string_error(result, errBuf, ERRBUF_SIZE));
  }

  const char *vdoBacking = processArgs(argc, argv);

  struct vdo_config config;
  struct volume_geometry geometry;
  readVDOConfig(vdoBacking, &config, &geometry);

  char uuid[UUID_STR_LEN];
  uuid_unparse(geometry.uuid, uuid);

  // This output must be valid YAML.
  printf("VDOConfig:\n");
  printf("  blockSize: %d\n", VDO_BLOCK_SIZE);
  printf("  logicalBlocks: %llu\n",
         (unsigned long long) config.logical_blocks);
  printf("  physicalBlocks: %llu\n",
         (unsigned long long) config.physical_blocks);
  printf("  slabSize: %llu\n", (unsigned long long) config.slab_size);
  printf("  recoveryJournalSize: %llu\n",
         (unsigned long long) config.recovery_journal_size);
  printf("  slabJournalBlocks: %llu\n",
         (unsigned long long) config.slab_journal_blocks);
  printf("UUID: %s\n", uuid);
  printf("ReleaseVersion: %u\n", geometry.release_version);
  printf("Nonce: %llu\n", (unsigned long long) geometry.nonce);
  printf("IndexRegion: %llu\n",
         (unsigned long long) geometry.regions[VDO_INDEX_REGION].start_block);
  printf("DataRegion: %llu\n",
         (unsigned long long) geometry.regions[VDO_DATA_REGION].start_block);
  printf("IndexConfig:\n");
  printf("  memory: %u\n", geometry.index_config.mem);
  printf("  sparse: %s\n", geometry.index_config.sparse ? "true" : "false");
  exit(0);
}
