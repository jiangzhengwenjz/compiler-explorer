/* as.c - GAS main program.
   Copyright (C) 1987-2018 Free Software Foundation, Inc.

   This file is part of GAS, the GNU Assembler.

   GAS is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GAS is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with GAS; see the file COPYING.  If not, write to the Free
   Software Foundation, 51 Franklin Street - Fifth Floor, Boston, MA
   02110-1301, USA.  */

/* Main program for AS; a 32-bit assembler of GNU.
   Understands command arguments.
   Has a few routines that don't fit in other modules because they
   are shared.

                        bugs

   : initialisers
        Since no-one else says they will support them in future: I
   don't support them now.  */

#define COMMON

/* Disable code to set FAKE_LABEL_NAME in obj-multi.h, to avoid circular
   reference.  */
#define INITIALIZING_EMULS

#include "as.h"
#include "subsegs.h"
#include "output-file.h"
#include "sb.h"
#include "macro.h"
#include "dwarf2dbg.h"
#include "dw2gencfi.h"
#include "bfdver.h"
#include "write.h"
#include "struc-symbol.h"

static void agbasm_debug_init(void);

/* We build a list of defsyms as we read the options, and then define
   them after we have initialized everything.  */
struct defsym_list {
    struct defsym_list *next;
    char *name;
    valueT value;
};


/* True if a listing is wanted.  */
int listing;

/* Type of debugging to generate.  */
enum debug_info_type debug_type = DEBUG_UNSPECIFIED;
int use_gnu_debug_info_extensions = 0;

#ifndef MD_DEBUG_FORMAT_SELECTOR
#define MD_DEBUG_FORMAT_SELECTOR NULL
#endif
static enum debug_info_type (*md_debug_format_selector)(int *) = MD_DEBUG_FORMAT_SELECTOR;

/* Maximum level of macro nesting.  */
int max_macro_nest = 100;

/* argv[0]  */
static char * myname;

/* The default obstack chunk size.  If we set this to zero, the
   obstack code will use whatever will fit in a 4096 byte block.  */
int chunksize = 0;

/* To monitor memory allocation more effectively, make this non-zero.
   Then the chunk sizes for gas and bfd will be reduced.  */
int debug_memory = 0;

/* Enable verbose mode.  */
int verbose = 0;

int flag_use_elf_stt_common = FALSE;
bfd_boolean flag_generate_build_notes = FALSE;

/* Keep the output file.  */
static int keep_it = 0;

segT reg_section;
segT expr_section;
segT text_section;
segT data_section;
segT bss_section;

/* Name of listing file.  */
static char *listing_filename = NULL;

static struct defsym_list *defsyms;

static long start_time;

static int flag_macro_alternate;

char * agbasm_debug_filename;

void print_version_id(void)
{
    static int printed;

    if (printed) {
        return;
    }
    printed = 1;

    fprintf(stderr, _("GNU assembler version %s (armv4tl-none-eabi) using BFD version %s\n"),
            VERSION, BFD_VERSION_STRING);
}

static void show_agbasm_help(FILE * stream)
{
    fprintf(stream, _("\n\
  --agbasm          enable all agbasm features except --agbasm-debug\n"));
    fprintf(stream, _("\n\
  --agbasm-debug FILE\n\
                    enable agbasm debug info. Outputs miscellaneous debugging print\n\
                    statements to the specified file. (\"printf debugging\")\n"));
    fprintf(stream, _("\n\
  --agbasm-colonless-labels\n\
                    enable agbasm colonless labels. This allows defining labels\n\
                    without a colon at the end if the label is in column\n\
                    zero and ends with a newline (after optional whitespace).\n\
                    If the label does not end with a newline, then an error is\n\
                    thrown and the label is assumed to be a statement.\n"));
    fprintf(stream, _("\n\
  --agbasm-colon-defined-global-labels\n\
                    enable agbasm colon defined global labels. This allows setting\n\
                    a label as global on definition by following the label name\n\
                    with two colons, as opposed to one (e.g. `label::').\n"));
    fprintf(stream, _("\n\
  --agbasm-local-labels\n\
                    enable agbasm local labels. These are like dollar local\n\
                    labels (as in they go out of scope when a non-local label\n\
                    is defined), but are not limited to a number as the label\n\
                    name. An agbasm local label is prefixed (and thus defined)\n\
                    with `%c'. Internally, an agbasm local label is actually\n\
                    just a concatenation of the most recently defined\n\
                    non-local label and the local label (including the prefix).\n\
                    This gives us a safe way to canonicalize local label names\n\
                    so that they can be exported for debug information. This\n\
                    also means that local labels can be referenced outside\n\
                    of their scope by using the canonicalized label name.\n\
                    Note that agbasm local labels are NOT local symbols by\n\
                    default.\n"), AGBASM_LOCAL_LABEL_PREFIX);
    fprintf(stream, _("\n\
  --agbasm-multiline-macros\n\
                    enable agbasm multiline macros. This allows the use of\n\
                    a macro to span across multiple lines, by placing a `%c'\n\
                    after the macro name, and then placing a `%c' once all\n\
                    macro arguments have been defined, e.g.\n\
                    \n\
                    my_macro %c\n\
                        arg_1=FOO,\n\
                        arg_2=BAR\n\
                    %c\n\
                    \n\
                    In a multiline macro, the equal sign used in assigning\n\
                    keyword arguments can substituted with a colon (`:'). Note\n\
                    that there cannot be whitespace before the colon, but\n\
                    there can be whitespace after the colon (this behavior also\n\
                    exists in unmodified gas with the equal sign).\n\
                    \n\
                    The opening character (`%c') must be defined before any\n\
                    macro arguments are specified. Arguments can be defined\n\
                    on the same line as the opening character with optional\n\
                    whitespace in-between the opening character and the starting\n\
                    argument, e.g.:\n\
                    \n\
                    my_macro %carg_1=FOO,\n\
                      arg_2=BAR\n\
                    %c\n\
                    \n\
                    The closing character (`%c') can be defined in one of\n\
                    two ways:\n\
                    - After the last argument, a comma is placed (to indicate\n\
                      the end of the argument), followed by optional whitespace\n\
                      and then the closing character, e.g.:\n\
                      \n\
                      my_macro %c\n\
                          FOO, %c\n\
                      \n\
                    - On a single line by itself (supposedly after the last\n\
                      argument has been defined) with no non-whitespace characters\n\
                      before or after it, e.g.:\n\
                      \n\
                      my_macro %c\n\
                          FOO\n\
                      %c\n\
                      \n\
                    Note that the first method **requires** a comma before the\n\
                    closing character, while the second method does not require\n\
                    the closing character. This is due to the inherent design\n\
                    of how macro arguments are parsed, which may be explained\n\
                    here in the future.\n\
                    \n\
                    A comma should be inserted after the last argument for each\n\
                    line (except as mentioned above in the second closing character\n\
                    method), otherwise a warning is generated. It is\n\
                    recommended to not ignore these warnings as they can\n\
                    be an indicator of a missing closing character, as most\n\
                    directives do not end with a comma.\n"), AGBASM_MULTILINE_MACRO_OPENING, AGBASM_MULTILINE_MACRO_CLOSING, AGBASM_MULTILINE_MACRO_OPENING, AGBASM_MULTILINE_MACRO_CLOSING, AGBASM_MULTILINE_MACRO_OPENING, AGBASM_MULTILINE_MACRO_OPENING, AGBASM_MULTILINE_MACRO_CLOSING, AGBASM_MULTILINE_MACRO_CLOSING, AGBASM_MULTILINE_MACRO_OPENING, AGBASM_MULTILINE_MACRO_CLOSING, AGBASM_MULTILINE_MACRO_OPENING, AGBASM_MULTILINE_MACRO_CLOSING);
    fprintf(stream, _("\n\
  --agbasm-charmap\n\
                    enable agbasm charmap.\n"));
    fprintf(stream, _("\n\
  --agbasm-help     show this message and exit\n"));
    fputc('\n', stream);

    /*if (REPORT_BUGS_TO[0] && stream == stdout) {
        fprintf(stream, _("Report bugs to %s\n"), REPORT_BUGS_TO);
    }*/
      
}
static void show_usage(FILE * stream)
{
    fprintf(stream, _("Usage: %s [option...] [asmfile...]\n"), myname);

    fprintf(stream, _("\
Options:\n\
  -a[sub-option...]	  turn on listings\n\
                          Sub-options [default hls]:\n\
                          c      omit false conditionals\n\
                          d      omit debugging directives\n\
                          g      include general info\n\
                          h      include high-level source\n\
                          l      include assembly\n\
                          m      include macro expansions\n\
                          n      omit forms processing\n\
                          s      include symbols\n\
                          =FILE  list to FILE (must be last sub-option)\n"));

    fprintf(stream, _("\
  --agbasm                enable all agbasm features except --agbasm-debug\n"));
    fprintf(stream, _("\
  --agbasm-debug FILE     enable agbasm debug info\n"));
    fprintf(stream, _("\n\
  --agbasm-charmap        enable agbasm charmap. Detailed explanation TODO.\n"));
    fprintf(stream, _("\
  --agbasm-colonless-labels\n\
                          enable agbasm colonless labels.\n"));
    fprintf(stream, _("\
  --agbasm-colon-defined-global-labels\n\
                          enable agbasm colon defined global labels.\n"));
    fprintf(stream, _("\
  --agbasm-local-labels   enable agbasm local labels.\n"));
    fprintf(stream, _("\
  --agbasm-multiline-macros\n\
                          enable agbasm multiline macros.\n"));
    fprintf(stream, _("\
  --agbasm-help           display detailed documentation on agbasm features and exit\n"));
    fprintf(stream, _("\
  --alternate             initially turn on alternate macro syntax\n"));
    fprintf(stream, _("\
  --compress-debug-sections[={none|zlib|zlib-gnu|zlib-gabi}]\n\
                          compress DWARF debug sections using zlib\n"));
    fprintf(stream, _("\
  --nocompress-debug-sections\n\
                          don't compress DWARF debug sections [default]\n"));
    fprintf(stream, _("\
  -D                      produce assembler debugging messages\n"));
    fprintf(stream, _("\
  --debug-prefix-map OLD=NEW\n\
                          map OLD to NEW in debug information\n"));
    fprintf(stream, _("\
  --defsym SYM=VAL        define symbol SYM to given value\n"));


    fprintf(stream, _("\
  --execstack             require executable stack for this object\n"));
    fprintf(stream, _("\
  --noexecstack           don't require executable stack for this object\n"));
    fprintf(stream, _("\
  --size-check=[error|warning]\n\
			  ELF .size directive check (default --size-check=error)\n"));
    fprintf(stream, _("\
  --elf-stt-common=[no|yes]\n\
                          generate ELF common symbols with STT_COMMON type\n"));
    fprintf(stream, _("\
  --sectname-subst        enable section name substitution sequences\n"));

    fprintf(stream, _("\
  --generate-missing-build-notes=[no|yes] "));
    fprintf(stream, _("(default: no)\n"));
    fprintf(stream, _("\
                          generate GNU Build notes if none are present in the input\n"));

    fprintf(stream, _("\
  -f                      skip whitespace and comment preprocessing\n"));
    fprintf(stream, _("\
  -g --gen-debug          generate debugging information\n"));
    fprintf(stream, _("\
  --gdwarf-2              generate DWARF2 debugging information\n"));
    fprintf(stream, _("\
  --gdwarf-sections       generate per-function section names for DWARF line information\n"));
    fprintf(stream, _("\
  --hash-size=<value>     set the hash table size close to <value>\n"));
    fprintf(stream, _("\
  --help                  show this message and exit\n"));
    fprintf(stream, _("\
  --target-help           show target specific options\n"));
    fprintf(stream, _("\
  -I DIR                  add DIR to search list for .include directives\n"));
    fprintf(stream, _("\
  -J                      don't warn about signed overflow\n"));
    fprintf(stream, _("\
  -K                      warn when differences altered for long displacements\n"));
    fprintf(stream, _("\
  -L,--keep-locals        keep local symbols (e.g. starting with `L')\n"));
    fprintf(stream, _("\
  -M,--mri                assemble in MRI compatibility mode\n"));
    fprintf(stream, _("\
  --MD FILE               write dependency information in FILE (default none)\n"));
    fprintf(stream, _("\
  -nocpp                  ignored\n"));
    fprintf(stream, _("\
  -no-pad-sections        do not pad the end of sections to alignment boundaries\n"));
    fprintf(stream, _("\
  -o OBJFILE              name the object-file output OBJFILE (default a.out)\n"));
    fprintf(stream, _("\
  -R                      fold data section into text section\n"));
    fprintf(stream, _("\
  --reduce-memory-overheads \n\
                          prefer smaller memory use at the cost of longer\n\
                          assembly times\n"));
    fprintf(stream, _("\
  --statistics            print various measured statistics from execution\n"));
    fprintf(stream, _("\
  --strip-local-absolute  strip local absolute symbols\n"));
    fprintf(stream, _("\
  --traditional-format    Use same format as native assembler when possible\n"));
    fprintf(stream, _("\
  --version               print assembler version number and exit\n"));
    fprintf(stream, _("\
  -W  --no-warn           suppress warnings\n"));
    fprintf(stream, _("\
  --warn                  don't suppress warnings\n"));
    fprintf(stream, _("\
  --fatal-warnings        treat warnings as errors\n"));
    fprintf(stream, _("\
  -w                      ignored\n"));
    fprintf(stream, _("\
  -X                      ignored\n"));
    fprintf(stream, _("\
  -Z                      generate object file even after errors\n"));
    fprintf(stream, _("\
  --listing-lhs-width     set the width in words of the output data column of\n\
                          the listing\n"));
    fprintf(stream, _("\
  --listing-lhs-width2    set the width in words of the continuation lines\n\
                          of the output data column; ignored if smaller than\n\
                          the width of the first line\n"));
    fprintf(stream, _("\
  --listing-rhs-width     set the max width in characters of the lines from\n\
                          the source file\n"));
    fprintf(stream, _("\
  --listing-cont-lines    set the maximum number of continuation lines used\n\
                          for the output data column of the listing\n"));
    fprintf(stream, _("\
  @FILE                   read options from FILE\n"));

    md_show_usage(stream);

    fputc('\n', stream);

    if (REPORT_BUGS_TO[0] && stream == stdout) {
        fprintf(stream, _("Report bugs to %s\n"), REPORT_BUGS_TO);
    }
}

/* Since it is easy to do here we interpret the special arg "-"
   to mean "use stdin" and we set that argv[] pointing to "".
   After we have munged argv[], the only things left are source file
   name(s) and ""(s) denoting stdin. These file names are used
   (perhaps more than once) later.

   check for new machine-dep cmdline options in
   md_parse_option definitions in config/tc-*.c.  */

static void parse_args(int * pargc, char *** pargv)
{
    int old_argc;
    int new_argc;
    char ** old_argv;
    char ** new_argv;
    /* Starting the short option string with '-' is for programs that
       expect options and other ARGV-elements in any order and that care about
       the ordering of the two.  We describe each non-option ARGV-element
       as if it were the argument of an option with character code 1.  */
    char *shortopts;
    extern const char *md_shortopts;
    static const char std_shortopts[] =
    {
        '-', 'J',
#ifndef WORKING_DOT_WORD
        /* -K is not meaningful if .word is not being hacked.  */
        'K',
#endif
        'L', 'M', 'R', 'W', 'Z', 'a', ':', ':', 'D', 'f', 'g', ':', ':', 'I', ':', 'o', ':',
#ifndef VMS
        /* -v takes an argument on VMS, so we don't make it a generic
           option.  */
        'v',
#endif
        'w', 'X',
        '\0'
    };
    struct option *longopts;
    extern struct option md_longopts[];
    extern size_t md_longopts_size;
    /* Codes used for the long options with no short synonyms.  */
    enum option_values {
        OPTION_HELP = OPTION_STD_BASE,
        OPTION_NOCPP,
        OPTION_STATISTICS,
        OPTION_VERSION,
        OPTION_DUMPCONFIG,
        OPTION_VERBOSE,
        OPTION_EMULATION,
        OPTION_DEBUG_PREFIX_MAP,
        OPTION_DEFSYM,
        OPTION_LISTING_LHS_WIDTH,
        OPTION_LISTING_LHS_WIDTH2,
        OPTION_LISTING_RHS_WIDTH,
        OPTION_LISTING_CONT_LINES,
        OPTION_DEPFILE,
        OPTION_GDWARF2,
        OPTION_GDWARF_SECTIONS,
        OPTION_STRIP_LOCAL_ABSOLUTE,
        OPTION_TRADITIONAL_FORMAT,
        OPTION_WARN,
        OPTION_TARGET_HELP,
        OPTION_EXECSTACK,
        OPTION_NOEXECSTACK,
        OPTION_SIZE_CHECK,
        OPTION_ELF_STT_COMMON,
        OPTION_ELF_BUILD_NOTES,
        OPTION_SECTNAME_SUBST,
        OPTION_ALTERNATE,
        OPTION_AL,
        OPTION_HASH_TABLE_SIZE,
        OPTION_REDUCE_MEMORY_OVERHEADS,
        OPTION_WARN_FATAL,
        OPTION_COMPRESS_DEBUG,
        OPTION_NOCOMPRESS_DEBUG,
        OPTION_NO_PAD_SECTIONS, /* = STD_BASE + 40 */
        OPTION_AGBASM,
        OPTION_AGBASM_DEBUG,
        OPTION_AGBASM_HELP,
        OPTION_AGBASM_LOCAL_LABELS,
        OPTION_AGBASM_COLONLESS_LABELS,
        OPTION_AGBASM_COLON_DEFINED_GLOBAL_LABELS,
        OPTION_AGBASM_MULTILINE_MACROS,
        OPTION_AGBASM_CHARMAP

        /* When you add options here, check that they do
           not collide with OPTION_MD_BASE.  See as.h.  */
    };

    static const struct option std_longopts[] =
    {
        /* Note: commas are placed at the start of the line rather than
           the end of the preceding line so that it is simpler to
           selectively add and remove lines from this list.  */
        { "alternate", no_argument, NULL, OPTION_ALTERNATE }
        /* The entry for "a" is here to prevent getopt_long_only() from
           considering that -a is an abbreviation for --alternate.  This is
           necessary because -a=<FILE> is a valid switch but getopt would
           normally reject it since --alternate does not take an argument.  */
        , { "agbasm", no_argument, NULL, OPTION_AGBASM}
        , { "agbasm-local-labels", no_argument, NULL, OPTION_AGBASM_LOCAL_LABELS}
        , { "agbasm-colonless-labels", no_argument, NULL, OPTION_AGBASM_COLONLESS_LABELS}
        , { "agbasm-help", no_argument, NULL, OPTION_AGBASM_HELP}
        , { "agbasm-debug", required_argument, NULL, OPTION_AGBASM_DEBUG}
        , { "agbasm-colon-defined-global-labels", no_argument, NULL, OPTION_AGBASM_COLON_DEFINED_GLOBAL_LABELS}
        , { "agbasm-multiline-macros", no_argument, NULL, OPTION_AGBASM_MULTILINE_MACROS}
        , { "agbasm-charmap", no_argument, NULL, OPTION_AGBASM_CHARMAP}
        , { "a", optional_argument, NULL, 'a' }
        /* Handle -al=<FILE>.  */
        , { "al", optional_argument, NULL, OPTION_AL }
        , { "compress-debug-sections", optional_argument, NULL, OPTION_COMPRESS_DEBUG }
        , { "nocompress-debug-sections", no_argument, NULL, OPTION_NOCOMPRESS_DEBUG }
        , { "debug-prefix-map", required_argument, NULL, OPTION_DEBUG_PREFIX_MAP }
        , { "defsym", required_argument, NULL, OPTION_DEFSYM }
        , { "dump-config", no_argument, NULL, OPTION_DUMPCONFIG }
        , { "emulation", required_argument, NULL, OPTION_EMULATION }
        , { "execstack", no_argument, NULL, OPTION_EXECSTACK }
        , { "noexecstack", no_argument, NULL, OPTION_NOEXECSTACK }
        , { "size-check", required_argument, NULL, OPTION_SIZE_CHECK }
        , { "elf-stt-common", required_argument, NULL, OPTION_ELF_STT_COMMON }
        , { "sectname-subst", no_argument, NULL, OPTION_SECTNAME_SUBST }
        , { "generate-missing-build-notes", required_argument, NULL, OPTION_ELF_BUILD_NOTES }
        , { "fatal-warnings", no_argument, NULL, OPTION_WARN_FATAL }
        , { "gdwarf-2", no_argument, NULL, OPTION_GDWARF2 }
        /* GCC uses --gdwarf-2 but GAS uses to use --gdwarf2,
           so we keep it here for backwards compatibility.  */
        , { "gdwarf2", no_argument, NULL, OPTION_GDWARF2 }
        , { "gdwarf-sections", no_argument, NULL, OPTION_GDWARF_SECTIONS }
        , { "gen-debug", no_argument, NULL, 'g' }
        , { "hash-size", required_argument, NULL, OPTION_HASH_TABLE_SIZE }
        , { "help", no_argument, NULL, OPTION_HELP }
        /* getopt allows abbreviations, so we do this to stop it from
           treating -k as an abbreviation for --keep-locals.  Some
           ports use -k to enable PIC assembly.  */
        , { "keep-locals", no_argument, NULL, 'L' }
        , { "keep-locals", no_argument, NULL, 'L' }
        , { "listing-lhs-width", required_argument, NULL, OPTION_LISTING_LHS_WIDTH }
        , { "listing-lhs-width2", required_argument, NULL, OPTION_LISTING_LHS_WIDTH2 }
        , { "listing-rhs-width", required_argument, NULL, OPTION_LISTING_RHS_WIDTH }
        , { "listing-cont-lines", required_argument, NULL, OPTION_LISTING_CONT_LINES }
        , { "MD", required_argument, NULL, OPTION_DEPFILE }
        , { "mri", no_argument, NULL, 'M' }
        , { "nocpp", no_argument, NULL, OPTION_NOCPP }
        , { "no-pad-sections", no_argument, NULL, OPTION_NO_PAD_SECTIONS }
        , { "no-warn", no_argument, NULL, 'W' }
        , { "reduce-memory-overheads", no_argument, NULL, OPTION_REDUCE_MEMORY_OVERHEADS }
        , { "statistics", no_argument, NULL, OPTION_STATISTICS }
        , { "strip-local-absolute", no_argument, NULL, OPTION_STRIP_LOCAL_ABSOLUTE }
        , { "version", no_argument, NULL, OPTION_VERSION }
        , { "verbose", no_argument, NULL, OPTION_VERBOSE }
        , { "target-help", no_argument, NULL, OPTION_TARGET_HELP }
        , { "traditional-format", no_argument, NULL, OPTION_TRADITIONAL_FORMAT }
        , { "warn", no_argument, NULL, OPTION_WARN }
    };

    /* Construct the option lists from the standard list and the target
       dependent list.  Include space for an extra NULL option and
       always NULL terminate.  */
    shortopts = concat(std_shortopts, md_shortopts, (char*)NULL);
    longopts = (struct option *)malloc(sizeof(std_longopts)
                                       + md_longopts_size + sizeof(struct option));
    memcpy(longopts, std_longopts, sizeof(std_longopts));
    memcpy(((char*)longopts) + sizeof(std_longopts), md_longopts, md_longopts_size);
    memset(((char*)longopts) + sizeof(std_longopts) + md_longopts_size,
           0, sizeof(struct option));

    /* Make a local copy of the old argv.  */
    old_argc = *pargc;
    old_argv = *pargv;

    /* Initialize a new argv that contains no options.  */
    new_argv = XNEWVEC(char *, old_argc + 1);
    new_argv[0] = old_argv[0];
    new_argc = 1;
    new_argv[new_argc] = NULL;

    while (1) {
        /* getopt_long_only is like getopt_long, but '-' as well as '--' can
           indicate a long option.  */
        int longind;
        int optc = getopt_long_only(old_argc, old_argv, shortopts, longopts,
                                    &longind);

        if (optc == -1) {
            break;
        }

        switch (optc) {
        default:
            /* md_parse_option should return 1 if it recognizes optc,
               0 if not.  */
            if (md_parse_option(optc, optarg) != 0) {
                break;
            }
            /* `-v' isn't included in the general short_opts list, so check for
               it explicitly here before deciding we've gotten a bad argument.  */
            if (optc == 'v') {
#ifdef VMS
                /* Telling getopt to treat -v's value as optional can result
                   in it picking up a following filename argument here.  The
                   VMS code in md_parse_option can return 0 in that case,
                   but it has no way of pushing the filename argument back.  */
                if (optarg && *optarg) {
                    new_argv[new_argc++] = optarg, new_argv[new_argc] = NULL;
                } else
#else
            case 'v':
#endif
                { case OPTION_VERBOSE:
                  print_version_id(); }
                verbose = 1;
                break;
            } else {
                as_bad(_("unrecognized option -%c%s"), optc, optarg ? optarg : "");
            }
        /* Fall through.  */

        case '?':
            exit(EXIT_FAILURE);

        case 1:         /* File name.  */
            if (!strcmp(optarg, "-")) {
                optarg = (char*)"";
            }
            new_argv[new_argc++] = optarg;
            new_argv[new_argc] = NULL;
            break;

        case OPTION_TARGET_HELP:
            md_show_usage(stdout);
            exit(EXIT_SUCCESS);

        case OPTION_HELP:
            show_usage(stdout);
            exit(EXIT_SUCCESS);

        case OPTION_NOCPP:
            break;

        case OPTION_NO_PAD_SECTIONS:
            do_not_pad_sections_to_alignment = 1;
            break;

        case OPTION_STATISTICS:
            flag_print_statistics = 1;
            break;

        case OPTION_STRIP_LOCAL_ABSOLUTE:
            flag_strip_local_absolute = 1;
            break;

        case OPTION_TRADITIONAL_FORMAT:
            flag_traditional_format = 1;
            break;

        case OPTION_VERSION:
            /* This output is intended to follow the GNU standards document.  */
            printf(_("GNU assembler %s\n"), BFD_VERSION_STRING);
            printf(_("Copyright (C) 2018 Free Software Foundation, Inc.\n"));
            printf(_("\
This program is free software; you may redistribute it under the terms of\n\
the GNU General Public License version 3 or later.\n\
This program has absolutely no warranty.\n"));
            printf(_("This assembler was configured for a target of `armv4tl-none-eabi'.\n"));
            exit(EXIT_SUCCESS);

        case OPTION_EMULATION:
            as_fatal(_("emulations not handled in this configuration"));
            break;

        case OPTION_DUMPCONFIG:
            fprintf(stderr, _("alias = armv4tl-none-eabi\n"));
            fprintf(stderr, _("canonical = armv4tl-none-eabi\n"));
            fprintf(stderr, _("cpu-type = %s\n"), TARGET_CPU);
#ifdef TARGET_OBJ_FORMAT
            fprintf(stderr, _("format = %s\n"), TARGET_OBJ_FORMAT);
#endif
#ifdef TARGET_FORMAT
            fprintf(stderr, _("bfd-target = %s\n"), TARGET_FORMAT);
#endif
            exit(EXIT_SUCCESS);

        case OPTION_COMPRESS_DEBUG:
            if (optarg) {
                if (strcasecmp(optarg, "none") == 0) {
                    flag_compress_debug = COMPRESS_DEBUG_NONE;
                } else if (strcasecmp(optarg, "zlib") == 0) {
                    flag_compress_debug = COMPRESS_DEBUG_GABI_ZLIB;
                } else if (strcasecmp(optarg, "zlib-gnu") == 0) {
                    flag_compress_debug = COMPRESS_DEBUG_GNU_ZLIB;
                } else if (strcasecmp(optarg, "zlib-gabi") == 0) {
                    flag_compress_debug = COMPRESS_DEBUG_GABI_ZLIB;
                } else {
                    as_fatal(_("Invalid --compress-debug-sections option: `%s'"),
                             optarg);
                }
            } else {
                flag_compress_debug = COMPRESS_DEBUG_GABI_ZLIB;
            }
            break;

        case OPTION_NOCOMPRESS_DEBUG:
            flag_compress_debug = COMPRESS_DEBUG_NONE;
            break;

        case OPTION_AGBASM:
            flag_agbasm |= (AGBASM_LOCAL_LABELS | AGBASM_COLONLESS_LABELS | AGBASM_COLON_DEFINED_GLOBAL_LABELS | AGBASM_MULTILINE_MACROS | AGBASM_CHARMAP);
            break;

        case OPTION_AGBASM_DEBUG:
            flag_agbasm |= AGBASM_DEBUG;
            agbasm_debug_filename = optarg;
            agbasm_debug_init();
            break;

        case OPTION_AGBASM_LOCAL_LABELS:
            flag_agbasm |= AGBASM_LOCAL_LABELS;
            break;

        case OPTION_AGBASM_COLONLESS_LABELS:
            flag_agbasm |= AGBASM_COLONLESS_LABELS;
            break;

        case OPTION_AGBASM_HELP:
            show_agbasm_help(stdout);
            exit(EXIT_SUCCESS);

        case OPTION_AGBASM_COLON_DEFINED_GLOBAL_LABELS:
            flag_agbasm |= AGBASM_COLON_DEFINED_GLOBAL_LABELS;
            break;

        case OPTION_AGBASM_MULTILINE_MACROS:
            flag_agbasm |= AGBASM_MULTILINE_MACROS;
            break;

        case OPTION_AGBASM_CHARMAP:
            flag_agbasm |= AGBASM_CHARMAP;
            break;

        case OPTION_DEBUG_PREFIX_MAP:
            add_debug_prefix_map(optarg);
            break;

        case OPTION_DEFSYM:
        {
            char *s;
            valueT i;
            struct defsym_list *n;

            for (s = optarg; *s != '\0' && *s != '='; s++) {
                ;
            }
            if (*s == '\0') {
                as_fatal(_("bad defsym; format is --defsym name=value"));
            }
            *s++ = '\0';
            i = bfd_scan_vma(s, (const char**)NULL, 0);
            n = XNEW(struct defsym_list);
            n->next = defsyms;
            n->name = optarg;
            n->value = i;
            defsyms = n;
        }
        break;

        case OPTION_DEPFILE:
            start_dependencies(optarg);
            break;

        case 'g':
            /* Some backends, eg Alpha and Mips, use the -g switch for their
               own purposes.  So we check here for an explicit -g and allow
               the backend to decide if it wants to process it.  */
            if (old_argv[optind - 1][1] == 'g'
                && md_parse_option(optc, optarg)) {
                continue;
            }

            if (md_debug_format_selector) {
                debug_type = md_debug_format_selector(&use_gnu_debug_info_extensions);
            } else {
                debug_type = DEBUG_DWARF2;
            }
            break;

        case OPTION_GDWARF2:
            debug_type = DEBUG_DWARF2;
            break;

        case OPTION_GDWARF_SECTIONS:
            flag_dwarf_sections = TRUE;
            break;

        case 'J':
            flag_signed_overflow_ok = 1;
            break;

#ifndef WORKING_DOT_WORD
        case 'K':
            flag_warn_displacement = 1;
            break;
#endif
        case 'L':
            flag_keep_locals = 1;
            break;

        case OPTION_LISTING_LHS_WIDTH:
            listing_lhs_width = atoi(optarg);
            if (listing_lhs_width_second < listing_lhs_width) {
                listing_lhs_width_second = listing_lhs_width;
            }
            break;
        case OPTION_LISTING_LHS_WIDTH2:
        {
            int tmp = atoi(optarg);

            if (tmp > listing_lhs_width) {
                listing_lhs_width_second = tmp;
            }
        }
        break;
        case OPTION_LISTING_RHS_WIDTH:
            listing_rhs_width = atoi(optarg);
            break;
        case OPTION_LISTING_CONT_LINES:
            listing_lhs_cont_lines = atoi(optarg);
            break;

        case 'M':
            flag_mri = 1;
            break;

        case 'R':
            flag_readonly_data_in_text = 1;
            break;

        case 'W':
            flag_no_warnings = 1;
            break;

        case OPTION_WARN:
            flag_no_warnings = 0;
            flag_fatal_warnings = 0;
            break;

        case OPTION_WARN_FATAL:
            flag_no_warnings = 0;
            flag_fatal_warnings = 1;
            break;

#if defined OBJ_ELF || defined OBJ_MAYBE_ELF
        case OPTION_EXECSTACK:
            flag_execstack = 1;
            flag_noexecstack = 0;
            break;

        case OPTION_NOEXECSTACK:
            flag_noexecstack = 1;
            flag_execstack = 0;
            break;

        case OPTION_SIZE_CHECK:
            if (strcasecmp(optarg, "error") == 0) {
                flag_allow_nonconst_size = FALSE;
            } else if (strcasecmp(optarg, "warning") == 0) {
                flag_allow_nonconst_size = TRUE;
            } else {
                as_fatal(_("Invalid --size-check= option: `%s'"), optarg);
            }
            break;

        case OPTION_ELF_STT_COMMON:
            if (strcasecmp(optarg, "no") == 0) {
                flag_use_elf_stt_common = 0;
            } else if (strcasecmp(optarg, "yes") == 0) {
                flag_use_elf_stt_common = 1;
            } else {
                as_fatal(_("Invalid --elf-stt-common= option: `%s'"),
                         optarg);
            }
            break;

        case OPTION_SECTNAME_SUBST:
            flag_sectname_subst = 1;
            break;

        case OPTION_ELF_BUILD_NOTES:
            if (strcasecmp(optarg, "no") == 0) {
                flag_generate_build_notes = FALSE;
            } else if (strcasecmp(optarg, "yes") == 0) {
                flag_generate_build_notes = TRUE;
            } else {
                as_fatal(_("Invalid --generate-missing-build-notes option: `%s'"),
                         optarg);
            }
            break;
#endif /* OBJ_ELF */

        case 'Z':
            flag_always_generate_output = 1;
            break;

        case OPTION_AL:
            listing |= LISTING_LISTING;
            if (optarg) {
                listing_filename = strdup(optarg);
            }
            break;

        case OPTION_ALTERNATE:
            optarg = old_argv [optind - 1];
            while (*optarg == '-') {
                optarg++;
            }

            if (strcmp(optarg, "alternate") == 0) {
                flag_macro_alternate = 1;
                break;
            }
            optarg++;
        /* Fall through.  */

        case 'a':
            if (optarg) {
                if (optarg != old_argv[optind] && optarg[-1] == '=') {
                    --optarg;
                }

                if (md_parse_option(optc, optarg) != 0) {
                    break;
                }

                while (*optarg) {
                    switch (*optarg) {
                    case 'c':
                        listing |= LISTING_NOCOND;
                        break;
                    case 'd':
                        listing |= LISTING_NODEBUG;
                        break;
                    case 'g':
                        listing |= LISTING_GENERAL;
                        break;
                    case 'h':
                        listing |= LISTING_HLL;
                        break;
                    case 'l':
                        listing |= LISTING_LISTING;
                        break;
                    case 'm':
                        listing |= LISTING_MACEXP;
                        break;
                    case 'n':
                        listing |= LISTING_NOFORM;
                        break;
                    case 's':
                        listing |= LISTING_SYMBOLS;
                        break;
                    case '=':
                        listing_filename = strdup(optarg + 1);
                        optarg += strlen(listing_filename);
                        break;
                    default:
                        as_fatal(_("invalid listing option `%c'"), *optarg);
                        break;
                    }
                    optarg++;
                }
            }
            if (!listing) {
                listing = LISTING_DEFAULT;
            }
            break;

        case 'D':
            /* DEBUG is implemented: it debugs different
               things from other people's assemblers.  */
            flag_debug = 1;
            break;

        case 'f':
            flag_no_comments = 1;
            break;

        case 'I':
        {               /* Include file directory.  */
            char *temp = strdup(optarg);

            add_include_dir(temp);
            break;
        }

        case 'o':
            out_file_name = strdup(optarg);
            break;

        case 'w':
            break;

        case 'X':
            /* -X means treat warnings as errors.  */
            break;

        case OPTION_REDUCE_MEMORY_OVERHEADS:
            /* The only change we make at the moment is to reduce
               the size of the hash tables that we use.  */
            set_gas_hash_table_size(4051);
            break;

        case OPTION_HASH_TABLE_SIZE:
        {
            unsigned long new_size;

            new_size = strtoul(optarg, NULL, 0);
            if (new_size) {
                set_gas_hash_table_size(new_size);
            } else {
                as_fatal(_("--hash-size needs a numeric argument"));
            }
            break;
        }
        }
    }

    free(shortopts);
    free(longopts);

    *pargc = new_argc;
    *pargv = new_argv;

#ifdef md_after_parse_args
    md_after_parse_args();
#endif
}

static void dump_statistics(void)
{
    long run_time = get_run_time() - start_time;

    fprintf(stderr, _("%s: total time in assembly: %ld.%06ld\n"),
            myname, run_time / 1000000, run_time % 1000000);

    subsegs_print_statistics(stderr);
    write_print_statistics(stderr);
    symbol_print_statistics(stderr);
    read_print_statistics(stderr);

#ifdef tc_print_statistics
    tc_print_statistics(stderr);
#endif

#ifdef obj_print_statistics
    obj_print_statistics(stderr);
#endif
}

static void close_output_file(void)
{
    output_file_close(out_file_name);
    if (!keep_it) {
        unlink_if_ordinary(out_file_name);
    }
}

/* The interface between the macro code and gas expression handling.  */

static size_t macro_expr(const char *emsg, size_t idx, sb *in, offsetT *val)
{
    char *hold;
    expressionS ex;

    sb_terminate(in);

    hold = input_line_pointer;
    input_line_pointer = in->ptr + idx;
    expression_and_evaluate(&ex);
    idx = input_line_pointer - in->ptr;
    input_line_pointer = hold;

    if (ex.X_op != O_constant) {
        as_bad("%s (X_op: %u, name: %s)", emsg, ex.X_op, ex.X_add_symbol->bsym->name);
    }

    *val = ex.X_add_number;

    return idx;
}

/* Here to attempt 1 pass over each input file.
   We scan argv[*] looking for filenames or exactly "" which is
   shorthand for stdin. Any argv that is NULL is not a file-name.
   We set need_pass_2 TRUE if, after this, we still have unresolved
   expressions of the form (unknown value)+-(unknown value).

   Note the un*x semantics: there is only 1 logical input file, but it
   may be a catenation of many 'physical' input files.  */

static void perform_an_assembly_pass(int argc, char ** argv)
{
    int saw_a_file = 0;
#ifndef OBJ_MACH_O
    flagword applicable;
#endif

    need_pass_2 = 0;

#ifndef OBJ_MACH_O
    /* Create the standard sections, and those the assembler uses
       internally.  */
    text_section = subseg_new(TEXT_SECTION_NAME, 0);
    data_section = subseg_new(DATA_SECTION_NAME, 0);
    bss_section = subseg_new(BSS_SECTION_NAME, 0);
    /* @@ FIXME -- we're setting the RELOC flag so that sections are assumed
       to have relocs, otherwise we don't find out in time.  */
    applicable = bfd_applicable_section_flags(stdoutput);
    bfd_set_section_flags(stdoutput, text_section,
                          applicable & (SEC_ALLOC | SEC_LOAD | SEC_RELOC
                                        | SEC_CODE | SEC_READONLY));
    bfd_set_section_flags(stdoutput, data_section,
                          applicable & (SEC_ALLOC | SEC_LOAD | SEC_RELOC
                                        | SEC_DATA));
    bfd_set_section_flags(stdoutput, bss_section, applicable & SEC_ALLOC);
    seg_info(bss_section)->bss = 1;
#endif
    subseg_new(BFD_ABS_SECTION_NAME, 0);
    subseg_new(BFD_UND_SECTION_NAME, 0);
    reg_section = subseg_new("*GAS `reg' section*", 0);
    expr_section = subseg_new("*GAS `expr' section*", 0);

    subseg_set(text_section, 0);

    /* This may add symbol table entries, which requires having an open BFD,
       and sections already created.  */
    md_begin();

#ifdef obj_begin
    obj_begin();
#endif

    /* Skip argv[0].  */
    argv++;
    argc--;

    while (argc--) {
        if (*argv) {    /* Is it a file-name argument?  */
            saw_a_file++;
            /* argv->"" if stdin desired, else->filename.  */
            read_a_source_file(*argv);
        }
        argv++;         /* Completed that argv.  */
    }
    if (!saw_a_file) {
        read_a_source_file("");
    }
}


int main(int argc, char ** argv)
{
    char ** argv_orig = argv;
    struct stat sob;

    int macro_strip_at;

    start_time = get_run_time();
    signal_init();

    bindtextdomain(PACKAGE, LOCALEDIR);
    textdomain(PACKAGE);

    if (debug_memory) {
        chunksize = 64;
    }

#ifdef HOST_SPECIAL_INIT
    HOST_SPECIAL_INIT(argc, argv);
#endif

    myname = argv[0];

    expandargv(&argc, &argv);

#ifndef OBJ_DEFAULT_OUTPUT_FILE_NAME
#define OBJ_DEFAULT_OUTPUT_FILE_NAME "a.out"
#endif

    out_file_name = OBJ_DEFAULT_OUTPUT_FILE_NAME;

    hex_init();
    bfd_init();
    bfd_set_error_program_name(myname);

    /* Call parse_args before any of the init/begin functions
       so that switches like --hash-size can be honored.  */
    parse_args(&argc, &argv);

    if (argc > 1 && stat(out_file_name, &sob) == 0) {
        int i;

        for (i = 1; i < argc; ++i) {
            struct stat sib;

            if (stat(argv[i], &sib) == 0) {
                if (sib.st_ino == sob.st_ino && sib.st_ino != 0) {
                    /* Don't let as_fatal remove the output file!  */
                    out_file_name = NULL;
                    as_fatal(_("The input and output files must be distinct"));
                }
            }
        }
    }

    symbol_begin();
    frag_init();
    subsegs_begin();
    read_begin();
    input_scrub_begin();
    expr_begin();

    /* It has to be called after dump_statistics ().  */
    xatexit(close_output_file);

    if (flag_print_statistics) {
        xatexit(dump_statistics);
    }

    macro_strip_at = 0;
#ifdef TC_I960
    macro_strip_at = flag_mri;
#endif

    macro_init(flag_macro_alternate, flag_mri, macro_strip_at, macro_expr);

    output_file_create(out_file_name);
    gas_assert(stdoutput != 0);

    /* Tell bfd whether local labels can start with only a dot */
    bfd_set_agbasm_local_label_syntax((flag_agbasm & AGBASM_LOCAL_LABELS) != 0);

    /* not related to above line */
    dot_symbol_init();

#ifdef tc_init_after_args
    tc_init_after_args();
#endif

    dwarf2_init();

    local_symbol_make(".gasversion.", absolute_section,
                      BFD_VERSION / 10000UL, &predefined_address_frag);

    /* Now that we have fully initialized, and have created the output
       file, define any symbols requested by --defsym command line
       arguments.  */
    while (defsyms != NULL) {
        symbolS *sym;
        struct defsym_list *next;

        sym = symbol_new(defsyms->name, absolute_section, defsyms->value,
                         &zero_address_frag);
        /* Make symbols defined on the command line volatile, so that they
           can be redefined inside a source file.  This makes this assembler's
           behaviour compatible with earlier versions, but it may not be
           completely intuitive.  */
        S_SET_VOLATILE(sym);
        symbol_table_insert(sym);
        next = defsyms->next;
        free(defsyms);
        defsyms = next;
    }

    /* Assemble it.  */
    perform_an_assembly_pass(argc, argv);

    cond_finish_check(-1);

#ifdef md_end
    md_end();
#endif

#if defined OBJ_ELF || defined OBJ_MAYBE_ELF
    if ((flag_execstack || flag_noexecstack)
        && OUTPUT_FLAVOR == bfd_target_elf_flavour) {
        segT gnustack;

        gnustack = subseg_new(".note.GNU-stack", 0);
        bfd_set_section_flags(stdoutput, gnustack,
                              SEC_READONLY | (flag_execstack ? SEC_CODE : 0));
    }
#endif

    /* If we've been collecting dwarf2 .debug_line info, either for
       assembly debugging or on behalf of the compiler, emit it now.  */
    dwarf2_finish();

    /* If we constructed dwarf2 .eh_frame info, either via .cfi
       directives from the user or by the backend, emit it now.  */
    cfi_finish();

    keep_it = 0;
    if (seen_at_least_1_file()) {
        int n_warns, n_errs;
        char warn_msg[50];
        char err_msg[50];

        write_object_file();

        n_warns = had_warnings();
        n_errs = had_errors();

        sprintf(warn_msg,
                ngettext("%d warning", "%d warnings", n_warns), n_warns);
        sprintf(err_msg,
                ngettext("%d error", "%d errors", n_errs), n_errs);
        if (flag_fatal_warnings && n_warns != 0) {
            if (n_errs == 0) {
                as_bad(_("%s, treating warnings as errors"), warn_msg);
            }
            n_errs += n_warns;
        }

        if (n_errs == 0) {
            keep_it = 1;
        } else if (flag_always_generate_output) {
            /* The -Z flag indicates that an object file should be generated,
               regardless of warnings and errors.  */
            keep_it = 1;
            fprintf(stderr, _("%s, %s, generating bad object file\n"),
                    err_msg, warn_msg);
        }
    }

    fflush(stderr);

#ifndef NO_LISTING
    listing_print(listing_filename, argv_orig);
#endif

    input_scrub_end();

    /* Use xexit instead of return, because under VMS environments they
       may not place the same interpretation on the value given.  */
    if (had_errors() != 0) {
        xexit(EXIT_FAILURE);
    }

    /* Only generate dependency file if assembler was successful.  */
    print_dependencies();

    xexit(EXIT_SUCCESS);
}

static void agbasm_debug_init(void)
{
    FILE *f;
    if (agbasm_debug_filename == NULL) {
        as_bad(_("agbasm debug filename is NULL"));
    }

    f = fopen(agbasm_debug_filename, FOPEN_WT);
    if (f == NULL) {
        as_bad(_("can't open `%s' for writing"), agbasm_debug_filename);
    }

    if (fclose(f)) {
        as_bad(_("agbasm debug: can't close `%s'"), agbasm_debug_filename);
    }
}

/* agbasm debug routines are here for now to avoid touching the Makefile */
void agbasm_debug_write(const char * format, ...)
{
    FILE *f;
    va_list args;

    if (!(flag_agbasm & AGBASM_DEBUG)) {
        return;
    }

    if (agbasm_debug_filename == NULL) {
        as_bad(_("agbasm debug filename is NULL"));
    }

    f = fopen(agbasm_debug_filename, FOPEN_AT);
    if (f == NULL) {
        as_bad(_("can't open `%s' for writing"), agbasm_debug_filename);
    }

    va_start(args, format);
    vfprintf(f, format, args);
    va_end(args);

    putc('\n', f);

    if (fclose(f)) {
        as_bad(_("agbasm debug: can't close `%s'"), agbasm_debug_filename);
    }
}
