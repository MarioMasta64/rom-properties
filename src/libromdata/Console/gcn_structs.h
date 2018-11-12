/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * gcn_structs.h: Nintendo GameCube data structures.                       *
 *                                                                         *
 * Copyright (c) 2016-2018 by David Korth.                                 *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU General Public License as published by the   *
 * Free Software Foundation; either version 2 of the License, or (at your  *
 * option) any later version.                                              *
 *                                                                         *
 * This program is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License       *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_LIBROMDATA_GCN_STRUCTS_H__
#define __ROMPROPERTIES_LIBROMDATA_GCN_STRUCTS_H__

#include "librpbase/common.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(1)

/**
 * GameCube/Wii disc image header.
 * This matches the disc image format exactly.
 *
 * NOTE: Strings are NOT null-terminated!
 */
#define GCN_MAGIC 0xC2339F3D
#define WII_MAGIC 0x5D1C9EA3
typedef struct PACKED _GCN_DiscHeader {
	union {
		char id6[6];	// [0x000] Game code. (ID6)
		struct {
			char id4[4];		// [0x000] Game code. (ID4)
			char company[2];	// [0x004] Company code.
		};
	};

	uint8_t disc_number;		// [0x006] Disc number.
	uint8_t revision;		// [0x007] Revision.
	uint8_t audio_streaming;	// [0x008] Audio streaming flag.
	uint8_t stream_buffer_size;	// [0x009] Streaming buffer size.

	uint8_t reserved1[14];		// [0x00A]

	uint32_t magic_wii;		// [0x018] Wii magic. (0x5D1C9EA3)
	uint32_t magic_gcn;		// [0x01C] GameCube magic. (0xC2339F3D)

	char game_title[64];		// [0x020] Game title.

	// Wii: Disc encryption status.
	// Normally 0 on retail and RVT-R (indicating the disc is encrypted).
	uint8_t hash_verify;		// [0x060] If non-zero, disable hash verification.
	uint8_t disc_noCrypto;		// [0x061] If non-zero, disable disc encryption.
} GCN_DiscHeader;
ASSERT_STRUCT(GCN_DiscHeader, 98);

/**
 * GameCube region codes.
 * Used in bi2.bin (GameCube) and RVL_RegionSetting.
 */
typedef enum {
	GCN_REGION_JPN = 0,	// Japan / Taiwan
	GCN_REGION_USA = 1,	// USA
	GCN_REGION_EUR = 2,	// Europe / Australia
	GCN_REGION_ALL = 3,	// Region-Free
	GCN_REGION_KOR = 4,	// South Korea
} GCN_Region_Code;

/**
 * DVD Boot Block.
 * References:
 * - http://wiibrew.org/wiki/Wii_Disc#Decrypted
 * - http://hitmen.c02.at/files/yagcd/yagcd/chap13.html
 * - http://www.gc-forever.com/wiki/index.php?title=Apploader
 *
 * All fields are big-endian.
 */
#define GCN_Boot_Block_ADDRESS 0x420
typedef struct PACKED _GCN_Boot_Block {
	uint32_t dol_offset;	// NOTE: 34-bit RSH2 on Wii.
	uint32_t fst_offset;	// NOTE: 34-bit RSH2 on Wii.
	uint32_t fst_size;	// FST size. (NOTE: 34-bit RSH2 on Wii.)
	uint32_t fst_max_size;	// Size of biggest additional FST.

	uint32_t fst_mem_addr;	// FST address in RAM.
	uint32_t user_pos;	// Data area start. (Might be wrong; use FST.)
	uint32_t user_len;	// Data area length. (Might be wrong; use FST.)
	uint32_t reserved;
} GCN_Boot_Block;
ASSERT_STRUCT(GCN_Boot_Block, 32);

/**
 * DVD Boot Info. (bi2.bin)
 * Reference: http://www.gc-forever.com/wiki/index.php?title=Apploader
 *
 * All fields are big-endian.
 */
#define GCN_Boot_Info_ADDRESS 0x440
typedef struct PACKED _GCN_Boot_Info {
	uint32_t debug_mon_size;	// Debug monitor size. [FIXME: Listed as signed?]
	uint32_t sim_mem_size;		// Simulated memory size. (bytes) [FIXME: Listed as signed?]
	uint32_t arg_offset;		// Command line arguments.
	uint32_t debug_flag;		// Debug flag. (set to 3 if using CodeWarrior on GDEV)
	uint32_t trk_location;		// Target resident kernel location.
	uint32_t trk_size;		// Size of TRK. [FIXME: Listed as signed?]
	uint32_t region_code;		// Region code. (See GCN_Region_Code.)
	uint32_t reserved1[3];
	uint32_t dol_limit;		// Maximum total size of DOL text/data sections. (0 == unlimited)
	uint32_t reserved2;
} GCN_Boot_Info;
ASSERT_STRUCT(GCN_Boot_Info, 48);

/**
 * FST entry.
 * All fields are big-endian.
 *
 * Reference: http://hitmen.c02.at/files/yagcd/yagcd/index.html#idx13.4
 */
typedef struct PACKED _GCN_FST_Entry {
	uint32_t file_type_name_offset;	// MSB = type; low 24 bits = name offset
	union {
		struct {
			uint32_t unused;		// Unused.
			uint32_t file_count;		// File count.
		} root_dir;
		struct {
			uint32_t parent_dir_idx;	// Parent directory index.
			uint32_t next_offset;		// Index of the next entry in the current directory.
		} dir;
		struct {
			uint32_t offset;		// File offset. (<< 2 for Wii)
			uint32_t size;			// File size.
		} file;
	};
} GCN_FST_Entry;
ASSERT_STRUCT(GCN_FST_Entry, 12);

/**
 * TGC header.
 * Used on some GameCube demo discs.
 * Reference: http://hitmen.c02.at/files/yagcd/yagcd/index.html#idx14.8
 * TODO: Check Dolphin?
 *
 * All fields are big-endian.
 */
#define TGC_MAGIC 0xAE0F38A2
typedef struct PACKED _GCN_TGC_Header {
	uint32_t tgc_magic;	// TGC magic.
	uint32_t reserved1;	// Unknown (usually 0x00000000)
	uint32_t header_size;	// Header size. (usually 0x8000)
	uint32_t reserved2;	// Unknown (usually 0x00100000)
	uint32_t fst_offset;	// Offset to FST inside the embedded GCM.
	uint32_t fst_size;	// FST size.
	uint32_t fst_max_size;	// Size of biggest additional FST.
	uint32_t dol_offset;	// Offset to main.dol inside the embedded GCM.
	uint32_t dol_size;	// main.dol size.
	uint32_t reserved3[2];
	uint32_t banner_offset;	// Offset to opening.bnr inside the embedded GCM.
	uint32_t banner_size;	// opening.bnr size.
	uint32_t reserved4[3];
} GCN_TGC_Header;
ASSERT_STRUCT(GCN_TGC_Header, 64);

#pragma pack()

#ifdef __cplusplus
}
#endif

#endif /* __ROMPROPERTIES_LIBROMDATA_GCN_STRUCTS_H__ */
