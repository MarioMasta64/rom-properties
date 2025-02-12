/***************************************************************************
 * ROM Properties Page shell extension. (librpfile)                        *
 * RpFile_windres.hpp: Windows resource wrapper for IRpFile. (Win32)       *
 *                                                                         *
 * Copyright (c) 2016-2023 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#pragma once

#ifndef _WIN32
#  error RpFile_Resource.hpp is Windows only.
#endif

#include "../MemFile.hpp"
#include "libwin32common/RpWin32_sdk.h"

namespace LibRpFile {

class RpFile_windres final : public MemFile
{
	public:
		/**
		 * Open an IRpFile backed by a Win32 resource.
		 * The resulting IRpFile is read-only.
		 *
		 * @param hModule Module handle.
		 * @param lpName Resource name.
		 * @param lpType Resource type.
		 */
		RP_LIBROMDATA_PUBLIC
		RpFile_windres(HMODULE hModule, LPCTSTR lpName, LPCTSTR lpType);
	protected:
		~RpFile_windres() final;	// call unref() instead

	private:
		typedef MemFile super;
		RP_DISABLE_COPY(RpFile_windres)

	public:
		/**
		 * Close the file.
		 */
		void close(void) final;

	protected:
		HGLOBAL m_hGlobal;	// Resource handle.
};

}
