/****************************************************************************
 *
 * Copyright 2022 Samsung Electronics All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <tinyara/config.h>

#include <sys/types.h>
#include <sys/statfs.h>
#include <sys/stat.h>

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>
#include <errno.h>
#include <debug.h>

#include <tinyara/clock.h>
#include <tinyara/kmalloc.h>
#include <tinyara/fs/fs.h>
#include <tinyara/fs/procfs.h>
#include <tinyara/log_dump/log_dump_internal.h>

#if !defined(CONFIG_DISABLE_MOUNTPOINT) && defined(CONFIG_FS_PROCFS)
#if defined(CONFIG_LOG_DUMP)

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
/* Determines the size of an intermediate buffer that must be large enough
 * to handle the longest line generated by this logic.
 */

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* This structure describes one open "file" */

struct logsave_file_s {
	struct procfs_file_s base;		/* Base open file structure */
	unsigned int linesize;			/* Number of valid characters in line[] */
	char line[10];				/* Pre-allocated buffer for formatted lines */
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/* File system methods */

static int logsave_open(FAR struct file *filep, FAR const char *relpath, int oflags, mode_t mode);
static int logsave_close(FAR struct file *filep);
static ssize_t logsave_read(FAR struct file *filep, FAR char *buffer, size_t buflen);
static ssize_t logsave_write(FAR struct file *filep, FAR const char *buffer, size_t buflen);
static int logsave_dup(FAR const struct file *oldp, FAR struct file *newp);
static int logsave_stat(FAR const char *relpath, FAR struct stat *buf);

/****************************************************************************
 * Private Variables
 ****************************************************************************/

/****************************************************************************
 * Public Variables
 ****************************************************************************/

/* See fs_mount.c -- this structure is explicitly externed there.
 * We use the old-fashioned kind of initializers so that this will compile
 * with any compiler.
 */

const struct procfs_operations logsave_operations = {
	logsave_open,				/* open */
	logsave_close,				/* close */
	logsave_read,				/* read */
	logsave_write,				/* write */

	logsave_dup,				/* dup */

	NULL,					/* opendir */
	NULL,					/* closedir */
	NULL,					/* readdir */
	NULL,					/* rewinddir */

	logsave_stat				/* stat */
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: logsave_open
 ****************************************************************************/

static int logsave_open(FAR struct file *filep, FAR const char *relpath, int oflags, mode_t mode)
{
	FAR struct logsave_file_s *attr = (struct logsave_file_s *)kmm_zalloc(sizeof(struct logsave_file_s));
	filep->f_priv = (FAR void *)attr;
	fvdbg("Open '%s'\n", relpath);
	return OK;
}

/****************************************************************************
 * Name: logsave_close
 ****************************************************************************/

static int logsave_close(FAR struct file *filep)
{
	return OK;
}

/****************************************************************************
 * Name: logsave_read
 ****************************************************************************/

static ssize_t logsave_read(FAR struct file *filep, FAR char *buffer, size_t buflen)
{
	return log_dump_read(buffer, buflen);
}

/****************************************************************************
 * Name: logsave_write
 ****************************************************************************/

static ssize_t logsave_write(FAR struct file *filep, FAR const char *buffer, size_t buflen)
{
	return log_dump_set_mode(buffer, buflen);
}

/****************************************************************************
 * Name: logsave_dup
 ****************************************************************************/

static int logsave_dup(FAR const struct file *oldp, FAR struct file *newp)
{
	return OK;
}

/****************************************************************************
 * Name: logsave_stat
 *
 * Description: Return information about a file or directory
 *
 ****************************************************************************/

static int logsave_stat(const char *relpath, struct stat *buf)
{
	/* "logsave" is the only acceptable value for the relpath */

	if (strcmp(relpath, "logsave") != 0) {
		fdbg("ERROR: relpath is '%s'\n", relpath);
		return -ENOENT;
	}

	/* "logsave" is the name for a read-only file */

	buf->st_mode = S_IFREG | S_IROTH | S_IRGRP | S_IRUSR;
	buf->st_size = 0;
	buf->st_blksize = 0;
	buf->st_blocks = 0;
	return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

#endif
#endif							/* !CONFIG_DISABLE_MOUNTPOINT && CONFIG_FS_PROCFS */
