/*
 * liblightnvm - Linux Open-Channel I/O interface
 *
 * Copyright (C) 2015 Javier González <javier@cnexlabs.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  - Redistributions of source code must retain the above copyright notice,
 *  this list of conditions and the following disclaimer.
 *  - Redistributions in binary form must reproduce the above copyright notice,
 *  this list of conditions and the following disclaimer in the documentation
 *  and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef __LIBLIGHTNVM_H
#define __LIBLIGHTNVM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#ifndef DISK_NAME_LEN
#define DISK_NAME_LEN 32
#endif
#ifndef NVM_DISK_NAME_LEN
#define NVM_DISK_NAME_LEN 32
#endif
#ifndef NVM_TTYPE_NAME_MAX
#define NVM_TTYPE_NAME_MAX 48
#endif
#ifndef NVM_TTYPE_MAX
#define NVM_TTYPE_MAX 63
#endif

#ifndef NVM_TGT_NAME_MAX
#define NVM_TGT_NAME_MAX (DISK_NAME_LEN + 5)	/* 5 = strlen(/dev/) */
#endif

/* BITS ALLOCATED FOR THE GENERAL ADDRESS FORMAT */
#define NVM_BLK_BITS (16)
#define NVM_PG_BITS  (16)
#define NVM_SEC_BITS (8)
#define NVM_PL_BITS  (8)
#define NVM_LUN_BITS (8)
#define NVM_CH_BITS  (7)

typedef struct NVM_ADDR {
	union {
		/* General address format */
		struct {
			uint64_t blk		: NVM_BLK_BITS;
			uint64_t pg		: NVM_PG_BITS;
			uint64_t sec		: NVM_SEC_BITS;
			uint64_t pl		: NVM_PL_BITS;
			uint64_t lun		: NVM_LUN_BITS;
			uint64_t ch		: NVM_CH_BITS;
			uint64_t reserved	: 1;
		} g;

		uint64_t ppa;
	};
} NVM_ADDR;

typedef struct nvm_geo {
	/* Values queried from device */
	size_t nchannels;	// # of channels on device
	size_t nluns;		// # of luns per channel
	size_t nplanes;		// # of planes for lun
	size_t nblocks;		// # of blocks per plane
	size_t npages;		// # of pages per block
	size_t nsectors;	// # of sectors per page
	size_t nbytes;		// # of bytes per sector

	/* Values derived from above */
	size_t tbytes;		// Total # of bytes on device
	size_t vblock_nbytes;	// # of bytes per vblock
	size_t io_nbytes_max;	// # upper bound on _nvm_vblock_[read|write]
} NVM_GEO;

typedef struct nvm_dev *NVM_DEV;
typedef struct nvm_tgt *NVM_TGT;
typedef struct nvm_vblock *NVM_VBLOCK;

void nvm_addr_pr(NVM_ADDR addr);
void nvm_geo_pr(NVM_GEO geo);

NVM_DEV nvm_dev_open(const char *dev_name);
void nvm_dev_close(NVM_DEV dev);
void nvm_dev_pr(NVM_DEV dev);

/**
 * @return Number of channels on given device
 */
int nvm_dev_get_nchannels(NVM_DEV dev);

/**
 * @return Number of luns (NAND dies) on given device
 */
int nvm_dev_get_nluns(NVM_DEV dev);

/**
 * @return Number of planes in a lun on given device
 */
int nvm_dev_get_nplanes(NVM_DEV dev);

/**
 * @return Number of blocks per plane on given device

 */
int nvm_dev_get_nblocks(NVM_DEV dev);

/**
 * @return Number of pages per block on given device

 */
int nvm_dev_get_npages(NVM_DEV dev);

/**
 * @return Number of sectors per page on given device

 */
int nvm_dev_get_nsectors(NVM_DEV dev);

/**
 * @return Number of bytes per sector on given device

 */
int nvm_dev_get_nbytes(NVM_DEV dev);

/**
 * Returns of the geometry related device information including derived
 * information such as total number of bytes etc. See NVM_GEO for the specifics.
 *
 * @return NVM_GEO of given dev
 */
NVM_GEO nvm_dev_get_geo(NVM_DEV dev);

/* TODO: Deprecate these */
int nvm_dev_get_pln_pg_size(NVM_DEV dev);
int nvm_dev_get_sec_size(NVM_DEV dev);

/**
 * Open a target file descriptor for the target named tgt.
 *
 * Descriptor is passed to the nvm_vblock_get / nvm_vblock_put interface.
 * Descriptor is passed to the nvm_vblock_read / nvm_vblock_write interface.
 *
 * By using nvm_tgt_open instead of directly opening a file descriptor to the
 * target path, liblightnvm can keep track of which targets are being used and
 * manage memory accordingly.
 *
 * Returns: A file descriptor to the target named tgt. On error, -1 is returned,
 * in which case errno is set to indicate the error.
 */
NVM_TGT nvm_tgt_open(const char *tgt_name, int flags);
void nvm_tgt_close(NVM_TGT tgt);
void nvm_tgt_pr(NVM_TGT tgt);

int nvm_tgt_get_fd(NVM_TGT tgt);
NVM_DEV nvm_tgt_get_dev(NVM_TGT tgt);

NVM_VBLOCK nvm_vblock_new(void);
void nvm_vblock_free(NVM_VBLOCK *vblock);
void nvm_vblock_pr(NVM_VBLOCK vblock);

uint64_t nvm_vblock_get_ppa(NVM_VBLOCK vblock);
uint16_t nvm_vblock_get_flags(NVM_VBLOCK vblock);

/**
 * Get ownership of an arbitrary flash block from via the given target.
 *
 * Returns: On success, a flash block is allocated in LightNVM's media manager
 * and vblock is filled up accordingly. On error, -1 is returned, in which case
 * errno is set to indicate the error.
 */
int nvm_vblock_get(NVM_VBLOCK vblock, NVM_TGT tgt);

/**
 * Reserves a block on given target using a specific lun.
 *
 * @param vblock Block created with nvm_vblock_new
 * @param tgt Handle obtained with nvm_tgt_open
 * @param ch Channel from which to reserve via
 * @param lun Lun from which to reserve via
 * @return -1 on error and *errno* set, zero otherwise.
 */
int nvm_vblock_gets(NVM_VBLOCK vblock, NVM_TGT tgt, uint32_t ch, uint32_t lun);

/**
 * Put flash block(s) represented by vblock back to tgt.
 *
 * This action implies that the owner of the flash block previous to this
 * function call no longer owns the flash block, and therefor can no longer
 * submit I/Os to it, or expect that data on it is persisted. The flash block
 * cannot be reclaimed by the previous owner.
 *
 * @return -1 on error and *errno* set, zero otherwise.
 */
int nvm_vblock_put(NVM_VBLOCK vblock);

/**
 * Read count pages starting at ppa_off from tgt into buf using flags
 *
 * fpage_size is the flash page *read* size, which might be smaller than the
 * flash page write size; some controllers support reading at sector granularity
 * (typically 4KB), instead of reading the whole virtual flash page (across
 * planes).
 *
 * XXX(1): For now, we assume that the device supports reading at a sector
 * granurality; we will take this information from the device in the future.
 */
ssize_t nvm_vblock_pread(NVM_VBLOCK vblock, void *buf, size_t count,
			 size_t ppa_off);

/**
 * Write count flash pages of size fpage_size.
 *
 * fpage_size is the flash page write size. That is, the size of a virtual flash
 * pages, i.e., spanning flash planes.
 */
ssize_t nvm_vblock_pwrite(NVM_VBLOCK vblock, const void *buf, size_t count,
                          size_t ppa_off);

/**
 * Erase a block
 *
 * @returns 0 on success, some error code otherwise.
 */
int nvm_vblock_erase(NVM_VBLOCK vblock);

/**
 * Instantiates a target with a given target type on top of an nvme device
 * reserving a range of luns.
 *
 * Equivalent of cli: nvme lnvm create  -d <dev_name>
 *                                      -n <tgt_name>
 *                                      -t <tgt_type_name>
 *                                      -O<lun_begin>:<lun_end>
 *
 * @param dev_name Name of the nvme device e.g. nvme0n1
 * @param tgt_name Name of target e.g. nvm0
 * @param tgt_type_name Name of target e.g. dflash, pblk, rrpc
 * @param lun_begin First lun in range
 * @param lun_end Last lun in range
 * @return Error code ?
 */
int nvm_mgmt_tgt_create(const char *tgt_name, const char *tgt_type_name,
			const char *dev_name, int lun_begin, int lun_end);

/**
 * Removes an instantiated target.
 *
 * Equivalent of cli: nvme lnvm remove -n tgt-name
 *
 * @param tgt_name Name of target instance to remove
 * @return ?
 */
int nvm_mgmt_tgt_remove(const char *tgt_name);

/**
 * Initialize structures for beams
 */
int nvm_beam_init(void);

/*
 * Tear down structures for beams.
 */
void nvm_beam_exit(void);

/**
 *
 * @param tgt_name
 * @param lun
 * @param flags
 * @return
 */
int nvm_beam_create(const char *tgt_name, int lun, int flags);

/**
 *
 * @param beam
 * @param flags
 */
void nvm_beam_destroy(int beam, int flags);

/**
 *
 * @param beam
 * @param buf
 * @param count
 * @return
 */
ssize_t nvm_beam_append(int beam, const void *buf, size_t count);

/**
 *
 * @param beam
 * @param buf
 * @param count
 * @param offset
 * @param flags
 * @return
 */
ssize_t nvm_beam_read(int beam, void *buf, size_t count, off_t offset,
		      int flags);

/**
 *
 * @param beam
 * @param flags
 * @return
 */
int nvm_beam_sync(int beam, int flags);

#ifdef __cplusplus
}
#endif

#endif /* __LIBLIGHTNVM.H */
