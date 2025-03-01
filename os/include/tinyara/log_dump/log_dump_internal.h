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

#ifndef __LOG_DUMP_INTERNAL_H
#define __LOG_DUMP_INTERNAL_H

/****************************************************************************
 * Included Files
 ****************************************************************************/
/****************************************************************************
 * Pre-Processor Definitions
 ****************************************************************************/

/* Log Dump Thread information */
#define LOG_DUMP_NAME        "log_dump"		/* Log dump thread name */

/****************************************************************************
 * Public Types
 ****************************************************************************/
/****************************************************************************
 * Public Data
 ****************************************************************************/
/****************************************************************************
 * Public Functions
 ****************************************************************************/

/* Functions contained in log_dump_save.c ***********************************/
int log_dump_save(char ch);

/* Functions contained in log_dump_manager.c ***********************************/
int log_dump_manager(int argc, char *argv[]);

#endif		/* __LOG_DUMP_INTERNAL_H */
