/*
 * nvm - internal header for liblightnvm
 *
 * Copyright (C) 2015 Javier González <javier@cnexlabs.com>
 * Copyright (C) 2015 Matias Bjørling <matias@cnexlabs.com>
 * Copyright (C) 2016 Simon A. F. Lund <slund@cnexlabs.com>
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
#ifndef __NVM_H
#define __NVM_H

#include <liblightnvm.h>

/**
 * NVMe command opcodes as defined by LigthNVM specification 1.2
 */
enum spec12_opcodes {
	S12_OPC_SET_BBT = 0xF1,
	S12_OPC_GET_BBT = 0xF2,
	S12_OPC_ERASE = 0x90,
	S12_OPC_WRITE = 0x91,
	S12_OPC_READ = 0x92
};

/**
 * Representation of widths in LBA <-> Physical sector mapping
 *
 * Think of it as a multi-dimensional array in row-major ordered as:
 *
 * channels[]luns[]blocks[]pages[]planes[]sectors[] = sector_nbytes
 *
 * Contains the stride in bytes for each dimension of the geometry.
 */
typedef struct nvm_lba_map {
	size_t channel_nbytes;	///< Number of bytes in a channel
	size_t lun_nbytes;	///< Number of bytes in a LUN
	size_t plane_nbytes;	///< Number of bytes in plane
	size_t block_nbytes;	///< Number of bytes in a block
	size_t page_nbytes;	///< Number of bytes in a page
	size_t sector_nbytes;	///< Number of bytes in a sector
} NVM_LBA_MAP;

struct nvm_dev {
	char name[NVM_DEV_NAME_LEN];	///< Device name e.g. "nvme0n1"
	char path[NVM_DEV_PATH_LEN];	///< Device path e.g. "/dev/nvme0n1"
	struct nvm_addr_fmt fmt;	///< Device address format
	struct nvm_geo geo;		///< Device geometry
	struct nvm_lba_map lba_map;	///< Mapping for LBA format
    	int pmode;			///< Default plane-mode I/O
	int fd;				///< Device fd / IOCTL handle
};

struct nvm_vblk {
	struct nvm_dev *dev;
	struct nvm_addr addr;
	size_t pos_write;
	size_t pos_read;
};

struct nvm_sblk {
	struct nvm_dev *dev;
	struct nvm_addr bgn;
	struct nvm_addr end;
	struct nvm_geo geo;
	size_t pos_write;
	size_t pos_read;
};

void nvm_lba_map_pr(struct nvm_lba_map* map);

#endif /* __NVM_H */
