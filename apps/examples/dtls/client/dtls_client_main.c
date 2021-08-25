/****************************************************************************
 *
 * Copyright 2016 Samsung Electronics All Rights Reserved.
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
/*
 *  Simple DTLS client demonstration program
 *
 *  Copyright (C) 2006-2015, ARM Limited, All Rights Reserved
 *  SPDX-License-Identifier: Apache-2.0
 *
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may
 *  not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  This file is part of mbed TLS (https://tls.mbed.org)
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <sys/types.h>
#include <stdio.h>
#include <string.h>

#include "mbedtls/config.h"

#if !defined(MBEDTLS_SSL_CLI_C) || !defined(MBEDTLS_SSL_PROTO_DTLS) ||	\
	!defined(MBEDTLS_NET_C) || !defined(MBEDTLS_TIMING_C) ||			\
	!defined(MBEDTLS_ENTROPY_C) || !defined(MBEDTLS_CTR_DRBG_C) ||		\
	!defined(MBEDTLS_X509_CRT_PARSE_C) || !defined(MBEDTLS_RSA_C) ||	\
	!defined(MBEDTLS_CERTS_C)
int dtls_client_main(int argc, char **argv)
{
	printf("MBEDTLS_SSL_CLI_C and/or MBEDTLS_SSL_PROTO_DTLS and/or\n");
	printf("MBEDTLS_NET_C and/or MBEDTLS_TIMING_C and/or\n");
	printf("MBEDTLS_ENTROPY_C and/or MBEDTLS_CTR_DRBG_C and/or\n");
	printf("MBEDTLS_X509_CRT_PARSE_C and/or MBEDTLS_RSA_C and/or\n");
	printf("MBEDTLS_CERTS_C and/or MBEDTLS_PEM_PARSE_C not defined.\n");
	return 0;
}
#else

#define mbedtls_printf     printf
#define mbedtls_fprintf    fprintf

#include "mbedtls/net.h"
#include "mbedtls/debug.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#include "mbedtls/certs.h"
#include "mbedtls/timing.h"

#define SERVER_PORT "4433"
#define SERVER_NAME "localhost"
#define SERVER_ADDR "127.0.0.1"	/* forces IPv4 */
#define MESSAGE     "TinyARA test echo packet"

#define READ_TIMEOUT_MS 1000
#define MAX_RETRY       5

#define USAGE_DTLS \
"\n--------------------------USAGE----------------------------\n"	\
"dtlsc\n"															\
"dtlsc server_addr=xxx.xxx.xxx.xxx\n"								\
"-----------------------------------------------------------\n"

#define DEBUG_LEVEL 0

/*
 * Definition for handling pthread
 */
#define DTLS_CLIENT_PRIORITY     100
#define DTLS_CLIENT_STACK_SIZE   51200
#define DTLS_CLIENT_SCHED_POLICY SCHED_RR

struct pthread_arg {
	int argc;
	char **argv;
};

static void my_debug(void *ctx, int level, const char *file, int line, const char *str)
{
	((void)level);

	mbedtls_fprintf((FILE *)ctx, "%s:%04d: %s", file, line, str);
	fflush((FILE *)ctx);
}

/****************************************************************************
 * dtls_client_main
 ****************************************************************************/

int dtls_client_cb(void *args)
{
	int argc;
	char **argv;
	char *p;
	char *q;

	int ret = 0;
	int len;
	mbedtls_net_context server_fd;
	uint32_t flags;
	unsigned char buf[1024];
	const char *pers = "dtls_client";
	int retry_left = MAX_RETRY;

	mbedtls_entropy_context entropy;
	mbedtls_ctr_drbg_context ctr_drbg;
	mbedtls_ssl_context ssl;
	mbedtls_ssl_config conf;
	mbedtls_x509_crt cacert;
	mbedtls_timing_delay_context timer;

#if defined(MBEDTLS_DEBUG_C)
	mbedtls_debug_set_threshold(DEBUG_LEVEL);
#endif

	char *server_addr = SERVER_ADDR;

	argc = ((struct pthread_arg *)args)->argc;
	argv = ((struct pthread_arg *)args)->argv;

	if (argc == 0) {
usage:
		printf(USAGE_DTLS);
		goto exit;
	} else {
		int i;
		for (i = 1; i < argc; i++) {
			p = argv[i];
			if ((q = strchr(p, '=')) == NULL) {
				goto usage;
			}
			*q++ = '\0';
			if (strcmp(p, "server_addr") == 0) {
				server_addr = q;
			} else {
				goto usage;
			}
		}
	}

	/*
	 * 0. Initialize the RNG and the session data
	 */
	mbedtls_net_init(&server_fd);
	mbedtls_ssl_init(&ssl);
	mbedtls_ssl_config_init(&conf);
	mbedtls_x509_crt_init(&cacert);
	mbedtls_ctr_drbg_init(&ctr_drbg);

	mbedtls_printf("\n  . Seeding the random number generator...");
	fflush(stdout);

	mbedtls_entropy_init(&entropy);
	if ((ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, (const unsigned char *)pers, strlen(pers))) != 0) {
		mbedtls_printf(" failed\n  ! mbedtls_ctr_drbg_seed returned %d\n", ret);
		goto exit;
	}

	mbedtls_printf(" ok\n");

	/*
	 * 0. Load certificates
	 */
	mbedtls_printf("  . Loading the CA root certificate ...");
	fflush(stdout);

	ret = mbedtls_x509_crt_parse(&cacert, (const unsigned char *)mbedtls_test_ca_crt_rsa, mbedtls_test_ca_crt_rsa_len);
	if (ret < 0) {
		mbedtls_printf(" failed\n  !  mbedtls_x509_crt_parse returned -0x%x\n\n", -ret);
		goto exit;
	}

	mbedtls_printf(" ok (%d skipped)\n", ret);

	/*
	 * 1. Start the connection
	 */
	mbedtls_printf("  . Connecting to udp/%s/%s...", server_addr, SERVER_PORT);
	fflush(stdout);

	if ((ret = mbedtls_net_connect(&server_fd, server_addr, SERVER_PORT, MBEDTLS_NET_PROTO_UDP)) != 0) {
		mbedtls_printf(" failed\n  ! mbedtls_net_connect returned %d\n\n", ret);
		goto exit;
	}

	mbedtls_printf(" ok\n");

	/*
	 * 2. Setup stuff
	 */
	mbedtls_printf("  . Setting up the DTLS structure...");
	fflush(stdout);

	if ((ret = mbedtls_ssl_config_defaults(&conf, MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_DATAGRAM, MBEDTLS_SSL_PRESET_DEFAULT)) != 0) {
		mbedtls_printf(" failed\n  ! mbedtls_ssl_config_defaults returned %d\n\n", ret);
		goto exit;
	}

	/*
	 * OPTIONAL is usually a bad choice for security, but makes interop easier
	 * in this simplified example, in which the ca chain is hardcoded.
	 * Production code should set a proper ca chain and use REQUIRED.
	 */
	mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_OPTIONAL);
	mbedtls_ssl_conf_ca_chain(&conf, &cacert, NULL);
	mbedtls_ssl_conf_rng(&conf, mbedtls_ctr_drbg_random, &ctr_drbg);
	mbedtls_ssl_conf_dbg(&conf, my_debug, stdout);

	if ((ret = mbedtls_ssl_setup(&ssl, &conf)) != 0) {
		mbedtls_printf(" failed\n  ! mbedtls_ssl_setup returned %d\n\n", ret);
		goto exit;
	}

	if ((ret = mbedtls_ssl_set_hostname(&ssl, SERVER_NAME)) != 0) {
		mbedtls_printf(" failed\n  ! mbedtls_ssl_set_hostname returned %d\n\n", ret);
		goto exit;
	}

	mbedtls_ssl_set_bio(&ssl, &server_fd, mbedtls_net_send, mbedtls_net_recv, mbedtls_net_recv_timeout);
	mbedtls_ssl_set_timer_cb(&ssl, &timer, mbedtls_timing_set_delay, mbedtls_timing_get_delay);

	mbedtls_printf(" ok\n");

	/*
	 * 4. Handshake
	 */
	mbedtls_printf("  . Performing the SSL/TLS handshake...");
	fflush(stdout);

	do {
		ret = mbedtls_ssl_handshake(&ssl);
	} while (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE);

	if (ret != 0) {
		mbedtls_printf(" failed\n  ! mbedtls_ssl_handshake returned -0x%x\n\n", -ret);
		goto exit;
	}

	mbedtls_printf(" ok\n");

	/*
	 * 5. Verify the server certificate
	 */
	mbedtls_printf("  . Verifying peer X.509 certificate...");

	/* In real life, we would have used MBEDTLS_SSL_VERIFY_REQUIRED so that the
	 * handshake would not succeed if the peer's cert is bad.  Even if we used
	 * MBEDTLS_SSL_VERIFY_OPTIONAL, we would bail out here if ret != 0 */
	if ((flags = mbedtls_ssl_get_verify_result(&ssl)) != 0) {
		char vrfy_buf[512];

		mbedtls_printf(" failed\n");

		mbedtls_x509_crt_verify_info(vrfy_buf, sizeof(vrfy_buf), "  ! ", flags);

		mbedtls_printf("%s\n", vrfy_buf);
	} else {
		mbedtls_printf(" ok\n");
	}

	/*
	 * 6. Write the echo request
	 */
send_request:

	len = sizeof(MESSAGE) - 1;

	do {
		ret = mbedtls_ssl_write(&ssl, (unsigned char *)MESSAGE, len);
	} while (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE);

	if (ret < 0) {
		mbedtls_printf(" failed\n  ! mbedtls_ssl_write returned %d\n", ret);
		goto exit;
	}

	len = ret;
	mbedtls_printf(" > WRITE TO SERVER  %d bytes written : %s\n", len, MESSAGE);

	/*
	 * 7. Read the echo response
	 */
	len = sizeof(buf) - 1;
	memset(buf, 0, sizeof(buf));

	do {
		ret = mbedtls_ssl_read(&ssl, buf, len);
	} while (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE);

	if (ret <= 0) {
		switch (ret) {
		case MBEDTLS_ERR_SSL_TIMEOUT:
			mbedtls_printf(" timeout\n\n");
			if (retry_left-- > 0) {
				goto send_request;
			}
			goto exit;

		case MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY:
			mbedtls_printf(" connection was closed gracefully\n");
			ret = 0;
			goto close_notify;

		default:
			mbedtls_printf(" mbedtls_ssl_read returned -0x%x\n", -ret);
			goto exit;
		}
	}

	len = ret;
	mbedtls_printf(" < READ FROM SERVER %d bytes read : %s\n", len, buf);

	/*
	 * 8. Done, cleanly close the connection
	 */
close_notify:
	mbedtls_printf("  . Closing the connection...");

	/* No error checking, the connection might be closed already */
	do {
		ret = mbedtls_ssl_close_notify(&ssl);
	} while (ret == MBEDTLS_ERR_SSL_WANT_WRITE);
	ret = 0;

	mbedtls_printf(" done\n");

	/*
	 * 9. Final clean-ups and exit
	 */
exit:

#ifdef MBEDTLS_ERROR_C
	if (ret != 0) {
		char error_buf[100];
		mbedtls_strerror(ret, error_buf, 100);
		mbedtls_printf("Last error was: %d - %s\n\n", ret, error_buf);
	}
#endif

	mbedtls_net_free(&server_fd);

	mbedtls_x509_crt_free(&cacert);
	mbedtls_ssl_free(&ssl);
	mbedtls_ssl_config_free(&conf);
	mbedtls_ctr_drbg_free(&ctr_drbg);
	mbedtls_entropy_free(&entropy);

#if defined(_WIN32)
	mbedtls_printf("  + Press Enter to exit this program.\n");
	fflush(stdout);
	getchar();
#endif

	/* Shell can not handle large exit numbers -> 1 for errors */
	if (ret < 0) {
		ret = 1;
	}

	return 0;
}

int dtls_client_main(int argc, char **argv)
{
	pthread_t tid;
	pthread_attr_t attr;
	struct sched_param sparam;
	int r;

	struct pthread_arg args;
	args.argc = argc;
	args.argv = argv;

	/* Initialize the attribute variable */
	if ((r = pthread_attr_init(&attr)) != 0) {
		printf("%s: pthread_attr_init failed, status=%d\n", __func__, r);
	}

	/* 1. set a priority */
	sparam.sched_priority = DTLS_CLIENT_PRIORITY;
	if ((r = pthread_attr_setschedparam(&attr, &sparam)) != 0) {
		printf("%s: pthread_attr_setschedparam failed, status=%d\n", __func__, r);
	}

	if ((r = pthread_attr_setschedpolicy(&attr, DTLS_CLIENT_SCHED_POLICY)) != 0) {
		printf("%s: pthread_attr_setschedpolicy failed, status=%d\n", __func__, r);
	}

	/* 2. set a stacksize */
	if ((r = pthread_attr_setstacksize(&attr, DTLS_CLIENT_STACK_SIZE)) != 0) {
		printf("%s: pthread_attr_setstacksize failed, status=%d\n", __func__, r);
	}

	/* 3. create pthread with entry function */
	if ((r = pthread_create(&tid, &attr, (pthread_startroutine_t)dtls_client_cb, (void *)&args)) != 0) {
		printf("%s: pthread_create failed, status=%d\n", __func__, r);
	}

	/* Wait for the threads to stop */
	pthread_join(tid, NULL);

	return 0;
}
#endif