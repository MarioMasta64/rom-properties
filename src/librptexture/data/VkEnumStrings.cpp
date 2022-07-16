/***************************************************************************
 * ROM Properties Page shell extension. (librptexture)                     *
 * VkEnumStrings.cpp: Vulkan string tables.                                *
 *                                                                         *
 * Copyright (c) 2016-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "VkEnumStrings.hpp"
#include "fileformat/vk_defs.h"

namespace LibRpTexture { namespace VkEnumStrings {

#include "vkEnum_base_data.h"
#include "vkEnum_1000156xxx_data.h"
#include "vkEnum_PVRTC_data.h"
#include "vkEnum_ASTC_data.h"

/** Public functions **/

/**
 * Look up a Vulkan VkFormat enum string.
 * @param vkFormat	[in] vkFormat enum
 * @return String, or nullptr if not found.
 */
const char *lookup_vkFormat(unsigned int vkFormat)
{
	const char *tbl = nullptr;
	uint16_t offset = 0;
	if (/*vkFormat >= VK_FORMAT_UNDEFINED &&*/
	      vkFormat <= VK_FORMAT_ASTC_12x12_SRGB_BLOCK)
	{
		tbl = vkEnum_base_strtbl;
		offset = vkEnum_base_offtbl[vkFormat];
	}
	else if (vkFormat >= VK_FORMAT_G8B8G8R8_422_UNORM &&
	         vkFormat <= VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM)
	{
		tbl = vkEnum_1000156xxx_strtbl;
		offset = vkEnum_1000156xxx_offtbl[vkFormat - VK_FORMAT_G8B8G8R8_422_UNORM];
	}
	else if (vkFormat >= VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG &&
	         vkFormat <= VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG)
	{
		tbl = vkEnum_PVRTC_strtbl;
		offset = vkEnum_PVRTC_offtbl[vkFormat - VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG];
	}
	else if (vkFormat >= VK_FORMAT_ASTC_4x4_SFLOAT_BLOCK_EXT &&
	         vkFormat <= VK_FORMAT_ASTC_12x12_SFLOAT_BLOCK_EXT)
	{
		tbl = vkEnum_ASTC_strtbl;
		offset = vkEnum_ASTC_offtbl[vkFormat - VK_FORMAT_ASTC_4x4_SFLOAT_BLOCK_EXT];
	}

	if (!tbl || offset == 0) {
		return nullptr;
	}
	return &tbl[offset];
}

} }
