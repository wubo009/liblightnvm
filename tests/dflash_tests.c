/*
 * dflash - user-space append-only file system for flash memories
 *
 * Copyright (C) 2015 Javier Gonzalez <javier@cnexlabs.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <liblightnvm.h>
#include <unistd.h>

#include "CuTest/CuTest.h"
#include "../src/ioctl.h"

static CuSuite *per_test_suite = NULL;
uint64_t file_id;

static void init_lib(CuTest *ct)
{
	int ret = nvm_init();
	CuAssertIntEquals(ct, 0, ret);
}

static void create_tgt(CuTest *ct)
{
	struct nvm_ioctl_create c;
	int ret;

	sprintf(c.dev, "nvme0n1");
	sprintf(c.tgttype, "dflash");
	sprintf(c.tgtname, "test1");
	c.flags = 0;
	c.conf.type = 0;
	c.conf.s.lun_begin = 0;
	c.conf.s.lun_end = 0;

	ret = nvm_create_target(&c);

	CuAssertIntEquals(ct, 0, ret);
}

static void remove_tgt(CuTest *ct)
{
	struct nvm_ioctl_remove r;
	int ret;

	sprintf(r.tgtname, "test1");
	r.flags = 0;

	ret = nvm_remove_target(&r);

	CuAssertIntEquals(ct, 0, ret);
}

static void create_file(CuTest *ct)
{
	int tgt_id, fd;
	int i;

	create_tgt(ct);
	tgt_id = nvm_target_open("test1", 0);
	CuAssertTrue(ct, tgt_id > 0);

	file_id = nvm_file_create(tgt_id, 0, 0);
	CuAssertTrue(ct, file_id > 0);

	fd = nvm_file_open(file_id, 0);
	CuAssertTrue(ct, fd > 0);

	nvm_file_close(fd, 0);
	nvm_file_delete(file_id, 0);
	nvm_target_close(tgt_id);

	remove_tgt(ct);
}

static void file_close_ungrac(CuTest *ct)
{
	int tgt_id, fd;

	/* Not close fd; delete file -> Blocks are returned to MM */
	create_tgt(ct);
	tgt_id = nvm_target_open("test1", 0);
	CuAssertTrue(ct, tgt_id > 0);

	file_id = nvm_file_create(tgt_id, 0, 0);
	CuAssertTrue(ct, file_id > 0);

	fd = nvm_file_open(file_id, 0);
	CuAssertTrue(ct, fd > 0);

	nvm_file_delete(file_id, 0);
	nvm_target_close(tgt_id);

	remove_tgt(ct);

	/* Not close fd; not delete file -> Blocks are not returned to MM */
	create_tgt(ct);
	tgt_id = nvm_target_open("test1", 0);
	CuAssertTrue(ct, tgt_id > 0);

	file_id = nvm_file_create(tgt_id, 0, 0);
	CuAssertTrue(ct, file_id > 0);

	fd = nvm_file_open(file_id, 0);
	CuAssertTrue(ct, fd > 0);

	nvm_target_close(tgt_id);

	remove_tgt(ct);
}

static void init_data_test(char *buf, int len)
{
	int i;
	char payload = 'a';

	for (i = 0; i < len; i++) {
		buf[i] = (payload + i) % 28;
	}
}

/*
 * Append and read back from file.
 *	- Append: payload < PAGE_SIZE
 *	- Read: All
 *	- open - append - close - open - read - close
 */
static void file_ar1(CuTest *ct)
{
	size_t written, read;
	char test[100] = "Hello World\n";
	char test2[100];
	int tgt_id, fd;

	create_tgt(ct);
	tgt_id = nvm_target_open("test1", 0);
	CuAssertTrue(ct, tgt_id > 0);

	file_id = nvm_file_create(tgt_id, 0, 0);
	CuAssertTrue(ct, file_id > 0);

	fd = nvm_file_open(file_id, 0);
	CuAssertTrue(ct, fd > 0);

	written = nvm_file_append(fd, test, 12);
	CuAssertIntEquals(ct, 12, written);

	nvm_file_close(fd, 0);

	fd = nvm_file_open(file_id, 0);
	CuAssertTrue(ct, fd > 0);

	read = nvm_file_read(fd, test2, written, 0, 0);
	CuAssertIntEquals(ct, written, read);

	CuAssertByteArrayEquals(ct, test, test2, 12, 12);

	nvm_file_close(fd, 0);
	nvm_file_delete(file_id, 0);
	nvm_target_close(tgt_id);

	remove_tgt(ct);
}

/*
 * Append and read back from file.
 *	- Append: PAGE_SIZE < payload < BLOCK_SIZE
 *	- Read: All, 1000 byte chunks, 1 byte chunks
 *	- open - append - close - open - read - close
 */
static void file_ar2(CuTest *ct)
{
	size_t written, read;
	char test[5000];
	char test2[5000];
	int tgt_id, fd;
	int i;

	init_data_test(test, 5000);

	create_tgt(ct);
	tgt_id = nvm_target_open("test1", 0);
	CuAssertTrue(ct, tgt_id > 0);

	file_id = nvm_file_create(tgt_id, 0, 0);
	CuAssertTrue(ct, file_id > 0);

	fd = nvm_file_open(file_id, 0);
	CuAssertTrue(ct, fd > 0);

	written = nvm_file_append(fd, test, 5000);
	CuAssertIntEquals(ct, 5000, written);

	nvm_file_close(fd, 0);

	fd = nvm_file_open(file_id, 0);
	CuAssertTrue(ct, fd > 0);

	read = nvm_file_read(fd, test2, written, 0, 0);
	CuAssertIntEquals(ct, written, read);

	CuAssertByteArrayEquals(ct, test, test2, 5000, 5000);

	/* read in chunks */
	memset(test2, 0, 5000);

	for (i = 0; i < 5000; i++) {
		read = nvm_file_read(fd, test2 + i, 1, i, 0);
		CuAssertIntEquals(ct, 1, read);
	}

	CuAssertByteArrayEquals(ct, test, test2, 5000, 5000);

	/* read in chunks */
	memset(test2, 0, 5000);

	for (i = 0; i < 5; i++) {
		int offset = i * 1000;
		read = nvm_file_read(fd, test2 + offset, 1000, offset, 0);
		CuAssertIntEquals(ct, 1000, read);
	}

	CuAssertByteArrayEquals(ct, test, test2, 5000, 5000);

	nvm_file_close(fd, 0);
	nvm_file_delete(file_id, 0);
	nvm_target_close(tgt_id);

	remove_tgt(ct);
}

static void fini_lib(CuTest *ct)
{
	nvm_fini();
}

CuSuite *dflash_GetSuite()
{
	per_test_suite = CuSuiteNew();

	SUITE_ADD_TEST(per_test_suite, init_lib);
	SUITE_ADD_TEST(per_test_suite, create_tgt);
	SUITE_ADD_TEST(per_test_suite, remove_tgt);
	SUITE_ADD_TEST(per_test_suite, create_file);
	SUITE_ADD_TEST(per_test_suite, file_close_ungrac);
	SUITE_ADD_TEST(per_test_suite, file_ar1);
	SUITE_ADD_TEST(per_test_suite, file_ar2);
	SUITE_ADD_TEST(per_test_suite, fini_lib);
}

void run_all_test(void)
{
	CuString *output = CuStringNew();
	CuSuite *suite = CuSuiteNew();

	CuSuiteAddSuite(suite, (CuSuite*) dflash_GetSuite());

	CuSuiteRun(suite);
	CuSuiteSummary(suite, output);
	CuSuiteDetails(suite, output);
	printf("%s\n", output->buffer);

	CuStringDelete(output);
	CuSuiteDelete(suite);

	free(per_test_suite);
}

int main(void)
{
	run_all_test();
	return 0;
}
