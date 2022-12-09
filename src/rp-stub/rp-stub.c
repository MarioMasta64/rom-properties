/***************************************************************************
 * ROM Properties Page shell extension. (rp-stub)                          *
 * rp-stub.c: Stub program to invoke the rom-properties library.           *
 *                                                                         *
 * Copyright (c) 2016-2021 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

/**
 * rp-stub.c is a wrapper program for the Linux plugins.
 * It parses the command line and then searches for installed
 * rom-properties libraries. If found, it runs the requested
 * function from the library.
 */
#include "config.version.h"
#include "git.h"

#include "libunixcommon/dll-search.h"
#include "libi18n/i18n.h"

// OS-specific security options.
#include "rp-stub_secure.h"
#include "stdboolx.h"

// C includes.
#include <dlfcn.h>
#include <errno.h>
#include <getopt.h>
#include <locale.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef _WIN32
#  define DIR_SEP_CHR '\\'
#else
#  define DIR_SEP_CHR '/'
#endif

/**
 * rp_create_thumbnail2() flags
 */
typedef enum {
	RPCT_FLAG_NO_XDG_THUMBNAIL_METADATA	= (1U << 0),	/*< Don't add XDG thumbnail metadata */
} RpCreateThumbnailFlags;

/**
 * rp_create_thumbnail2() function pointer. (v2)
 * @param source_file Source file (UTF-8)
 * @param output_file Output file (UTF-8)
 * @param maximum_size Maximum size
 * @param flags Flags (see RpCreateThumbnailFlags)
 * @return 0 on success; non-zero on error.
 */
typedef int (RP_C_API *PFN_RP_CREATE_THUMBNAIL2)(const char *source_file, const char *output_file, int maximum_size, unsigned int flags);

/**
 * rp_show_config_dialog() function pointer. (Unix/Linux version)
 * @param argc
 * @param argv
 * @return 0 on success; non-zero on error.
 */
typedef int (RP_C_API *PFN_RP_SHOW_CONFIG_DIALOG)(int argc, char *argv[]);

// Are we running as rp-config?
static bool is_rp_config = false;
// Is debug logging enabled?
static bool is_debug = false;

static void show_version(void)
{
	puts(RP_DESCRIPTION);
	puts(C_("rp_stub",
		"Shared library stub program.\n"
		"Copyright (c) 2016-2021 by David Korth."));
	putchar('\n');
	printf(C_("rp-stub", "rom-properties version: %s"), RP_VERSION_STRING);
	putchar('\n');
#ifdef RP_GIT_VERSION
	puts(RP_GIT_VERSION);
# ifdef RP_GIT_DESCRIBE
	puts(RP_GIT_DESCRIBE);
# endif /* RP_GIT_DESCRIBE */
#endif /* RP_GIT_VERSION */
	putchar('\n');
	puts(C_("rp-stub",
		"This program is licensed under the GNU GPL v2.\n"
		"See https://www.gnu.org/licenses/old-licenses/gpl-2.0.html for more information."));
}

static void show_help(const char *argv0)
{
	show_version();
	putchar('\n');
	if (!is_rp_config) {
		printf(C_("rp-stub", "Usage: %s [-s size] source_file output_file"), argv0);
		putchar('\n');
		putchar('\n');
		puts(C_("rp-stub",
			"If source_file is a supported ROM image, a thumbnail is\n"
			"extracted and saved as output_file.\n"
			"\n"
			"Thumbnailing options:\n"
			"  -s, --size\t\tMaximum thumbnail size. (default is 256px) [0 for full image]\n"
			"  -a, --autoext\t\tGenerate the output filename based on the source filename.\n"
			"               \t\t(WARNING: May overwrite an existing file without prompting.)\n"
			"  -n, --noxdg\t\tDon't include XDG thumbnail metadata.\n"
			"\n"
			"Other options:\n"
			"  -c, --config\t\tShow the configuration dialog instead of thumbnailing.\n"
			"  -d, --debug\t\tShow debug output when searching for rom-properties.\n"
			"  -h, --help\t\tDisplay this help and exit.\n"
			"  -V, --version\t\tOutput version information and exit."));
	} else {
		printf(C_("rp-stub", "Usage: %s"), argv0);
		putchar('\n');
		putchar('\n');
		puts(C_("rp-stub",
			"When invoked as rp-config, this program will open the configuration dialog\n"
			"using an installed plugin that most closely matches the currently running\n"
			"desktop environment."));
	}
}

/**
 * Debug print function for rp_dll_search().
 * @param level Debug level.
 * @param format Format string.
 * @param ... Format arguments.
 * @return vfprintf() return value.
 */
static int ATTR_PRINTF(2, 3) fnDebug(int level, const char *format, ...)
{
	if (level < LEVEL_ERROR && !is_debug)
		return 0;

	va_list args;
	va_start(args, format);
	int ret = vfprintf(stderr, format, args);
	fputc('\n', stderr);
	va_end(args);

	return ret;
}

int main(int argc, char *argv[])
{
	/**
	 * Command line syntax:
	 * - Thumbnail: rp-stub [-s size] path output
	 * - Config:    rp-stub -c
	 *
	 * If invoked as 'rp-config', the configuration dialog
	 * will be shown instead of thumbnailing.
	 *
	 * TODO: Support URIs in addition to paths?
	 */

	if (getuid() == 0 || geteuid() == 0) {
		fprintf(stderr, "*** %s does not support running as root.\n", argv[0]);
		return EXIT_FAILURE;
	}

	// Set the C locale.
	// TODO: Stub may need to set the C++ locale.
	setlocale(LC_ALL, "");

	// Initialize i18n.
	rp_i18n_init();

	// Check if we were invoked as 'rp-config'.
	const char *argv0 = argv[0];
	const char *slash = strrchr(argv0, '/');
	if (slash) {
		// Ignore the subdirectories.
		argv0 = slash + 1;
	}
	if (!strcmp(argv0, "rp-config")) {
		// Invoked as rp-config.
		is_rp_config = true;
	}

	static const struct option long_options[] = {
		{"size",	required_argument,	NULL, 's'},
		{"autoext",	no_argument,		NULL, 'a'},
		{"noxdg",	no_argument,		NULL, 'n'},
		{"config",	no_argument,		NULL, 'c'},
		{"debug",	no_argument,		NULL, 'd'},
		{"help",	no_argument,		NULL, 'h'},
		{"version",	no_argument,		NULL, 'V'},
		// TODO: Option to scan for installed plugins.

		{NULL, 0, NULL, 0}
	};

	// "More Information" string.
	const char *const str_help_more_info = C_("rp-stub", "Try '%s --help' for more information.");

	// Default to 256x256.
	uint8_t config = is_rp_config;
	int maximum_size = 256;
	unsigned int flags = 0;
	bool autoext = false;
	int c, option_index;
	while ((c = getopt_long(argc, argv, "s:acdnhV", long_options, &option_index)) != -1) {
		switch (c) {
			case 's': {
				char *endptr = NULL;
				errno = 0;
				long lTmp = (int)strtol(optarg, &endptr, 10);
				if (errno == ERANGE || *endptr != 0) {
					// tr: %1$s == program name, %2%s == invalid size
					fprintf_p(stderr, C_("rp-stub", "%1$s: invalid size '%2$s'"), argv[0], optarg);
					putc('\n', stderr);
					// tr: %s == program name
					fprintf(stderr, str_help_more_info, argv[0]);
					putc('\n', stderr);
					return EXIT_FAILURE;
				} else if (lTmp < 0 || lTmp > 32768) {
					// tr: %1$s == program name, %2%s == invalid size
					fprintf_p(stderr, C_("rp-stub", "%1$s: size '%2$s' is out of range"), argv[0], optarg);
					putc('\n', stderr);
					// tr: %s == program name
					fprintf(stderr, str_help_more_info, argv[0]);
					putc('\n', stderr);
					return EXIT_FAILURE;
				}
				maximum_size = (int)lTmp;
				break;
			}

			case 'a':
				// Automatic output filename creation.
				autoext = true;
				break;

			case 'c':
				// Show the configuration dialog.
				config = true;
				break;

			case 'd':
				// Enable debug output.
				is_debug = true;
				break;

			case 'h':
				show_help(argv[0]);
				return EXIT_SUCCESS;

			case 'n':
				// Don't add XDG thumbnail metadata.
				flags |= RPCT_FLAG_NO_XDG_THUMBNAIL_METADATA;
				break;

			case 'V':
				show_version();
				return EXIT_SUCCESS;

			case '?':
			default:
				// Unrecognized option.
				fprintf(stderr, str_help_more_info, argv[0]);
				putc('\n', stderr);
				return EXIT_FAILURE;
		}
	}

	// Enable security options.
	// TODO: Check for '-c' first, then enable options
	// and reparse?
	rp_stub_do_security_options(config);

	if (!config) {
		// Thumbnailing mode.
		// We must have 2 filenames specified.
		if (optind == argc) {
			// tr: %s == program name
			fprintf(stderr, C_("rp-stub", "%s: missing source and output file parameters"), argv[0]);
			putc('\n', stderr);
			// tr: %s == program name
			fprintf(stderr, str_help_more_info, argv[0]);
			putc('\n', stderr);
			return EXIT_FAILURE;
		} else if (optind+1 == argc && !autoext) {
			// tr: %s == program name
			fprintf(stderr, C_("rp-stub", "%s: missing output file parameter"), argv[0]);
			putc('\n', stderr);
			// tr: %s == program name
			fprintf(stderr, str_help_more_info, argv[0]);
			putc('\n', stderr);
			return EXIT_FAILURE;
		} else if (optind+2 == argc && autoext) {
			// tr: %s == program name
			fprintf(stderr, C_("rp-stub", "%s: --autoext and output file specified"), argv[0]);
			putc('\n', stderr);
			// tr: %s == program name
			fprintf(stderr, str_help_more_info, argv[0]);
			putc('\n', stderr);
			return EXIT_FAILURE;
		} else if (optind+3 < argc) {
			// tr: %s == program name
			fprintf(stderr, C_("rp-stub", "%s: too many parameters specified"), argv[0]);
			putc('\n', stderr);
			// tr: %s == program name
			fprintf(stderr, str_help_more_info, argv[0]);
			putc('\n', stderr);
			return EXIT_FAILURE;
		}
	}

	// Search for a usable rom-properties library.
	// TODO: Desktop override option?
	const char *const symname = (config ? "rp_show_config_dialog" : "rp_create_thumbnail2");
	void *pDll = NULL, *pfn = NULL;
	int ret = rp_dll_search(symname, &pDll, &pfn, fnDebug);
	if (ret != 0) {
		return ret;
	}

	if (!config) {
		// Create the thumbnail.
		const char *const source_file = argv[optind];
		char *output_file;
		if (autoext) {
			// Create the output filename based on the input filename.
			size_t len = strlen(source_file);
			output_file = malloc(len + 16);
			strcpy(output_file, source_file);

			// Find the current extension and replace it.
			char *dotpos = strrchr(output_file, '.');
			if (!dotpos) {
				// No file extension. Add it.
				strcat(output_file, ".png");
			} else {
				// If the dot is after the last slash, we already have a file extension.
				// Otherwise, we don't have one, and need to add it.
				char *slashpos = strrchr(output_file, DIR_SEP_CHR);
				if (slashpos < dotpos) {
					// We already have a file extension.
					strcpy(dotpos, ".png");
				} else {
					// No file extension.
					strcat(output_file, ".png");
				}
			}
		} else {
			// Use the specified output filename.
			output_file = argv[optind+1];
		}

		if (is_debug) {
			// tr: NOTE: Not positional. Don't change argument positions!
			// tr: Only localize "Calling function:".
			fprintf(stderr, C_("rp-stub", "Calling function: %s(\"%s\", \"%s\", %d, %u);"),
				symname, source_file, output_file, maximum_size, flags);
			putc('\n', stderr);
		}
		ret = ((PFN_RP_CREATE_THUMBNAIL2)pfn)(source_file, output_file, maximum_size, flags);

		if (autoext) {
			free(output_file);
		}
	} else {
		// Show the configuration dialog.
		if (is_debug) {
			fprintf(stderr, C_("rp-stub", "Calling function: %s();"), symname);
			putc('\n', stderr);
		}
		// NOTE: argc/argv may be manipulated by getopt().
		ret = ((PFN_RP_SHOW_CONFIG_DIALOG)pfn)(argc, argv);
	}

	dlclose(pDll);
	if (ret == 0) {
		if (is_debug) {
			// tr: %1$s == function name, %2$d == return value
			fprintf_p(stderr, C_("rp-stub", "%1$s() returned %2$d."), symname, ret);
			putc('\n', stderr);
		}
	} else {
		// tr: %1$s == function name, %2$d == return value
		fprintf_p(stderr, C_("rp-stub", "*** ERROR: %1$s() returned %2$d."), symname, ret);
		putc('\n', stderr);
	}
	return ret;
}
