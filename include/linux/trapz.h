/*
 * trapz.h
 *
 * TRAPZ (TRAcing and Profiling for Zpeed) Log Driver header file
 *
 * Copyright (C) Amazon Technologies Inc. All rights reserved.
 * Andy Prunicki (prunicki@lab126.com)
 * Martin Unsal (munsal@lab126.com)
 * TODO: Add additional contributor's names.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*
 * This file must be #included anywhere that TRAPZ tracing calls are made from C
 * or C++ code.  Currently that includes the Linux kernel build and the Android
 * build.
 *
 * You must also make sure that your source code is scanned by the TRAPZ tool
 * chain. This is accomplished by editing labscripts/trapz/trapz.xml and adding
 * a <component> tag for your project with a <scan> tag pointing at your source
 * code.
 */

#ifndef _LINUX_TRAPZ_H
#define _LINUX_TRAPZ_H

/**
 * CONFIG_TRAPZ is used to enable the compilation of the TRAPZ kernel driver.
 * CONFIG_TRAPZ_TP is used to turn TRAPZ logging on and off.
 */

#include <linux/types.h>
#include <linux/time.h>
#ifndef __KERNEL__
#include <sys/syscall.h>
#endif

#ifdef CONFIG_TRAPZ_TP
#ifdef __KERNEL__
#include <generated/trapz_generated_kernel.h>
#else
#include <linux/trapz_generated.h>
#endif /* __KERNEL__ */
#endif /* CONFIG_TRAPZ_TP */

#ifdef CONFIG_TRAPZ_PVA
#define TRAPZ_PVA
#endif

typedef struct trapz_info {
	struct timespec ts;
	int counter;
} trapz_info_t;

#ifdef __KERNEL__
/* Internal kernel API to register events */
long systrapz(unsigned int ctrl, unsigned int extra1, unsigned int extra2,
	unsigned int extra3, unsigned int extra4, trapz_info_t __user *ti);
/* Checks if a trapz component is enabled for a given loglevel */
int trapz_check_loglevel(const int level, const int cat_id,
	const int component_id);
/* actual system call interface (see kernel/sys_ni.c) */
long sys_trapz(unsigned int ctrl, unsigned int extra1, unsigned int extra2,
	unsigned int extra3, unsigned int extra4, struct trapz_info __user *ti);
#else /* __KERNEL */
#define __NR_trapz 98
inline int systrapz(unsigned int ctrl, unsigned int extra1, unsigned int extra2,
	unsigned int extra3, unsigned int extra4, trapz_info_t *ti)
{
	return syscall(__NR_trapz, ctrl, extra1, extra2, extra3, extra4, ti);
}
#endif

/* ctrl field masks */
#define TRAPZ_LEVEL_MASK        0x3
#define TRAPZ_FLAGS_MASK        0x7
#define TRAPZ_CAT_ID_MASK       0x3
#define TRAPZ_COMP_ID_MASK      0xFFF
#define TRAPZ_TRACE_ID_MASK     0xFFF

/* ctrl field sizes in bits */
#define TRAPZ_LEVEL_SIZE        2
#define TRAPZ_FLAGS_SIZE        3
#define TRAPZ_CAT_ID_SIZE       2
#define TRAPZ_COMP_ID_SIZE     12
#define TRAPZ_TRACE_ID_SIZE    12

/* ctrl field offsets (from low order bit) */
#define TRAPZ_TRACE_ID_OFFSET   0
#define TRAPZ_COMP_ID_OFFSET    (TRAPZ_TRACE_ID_OFFSET + TRAPZ_TRACE_ID_SIZE)
#define TRAPZ_CAT_ID_OFFSET     (TRAPZ_COMP_ID_OFFSET + TRAPZ_COMP_ID_SIZE)
#define TRAPZ_FLAGS_OFFSET      (TRAPZ_CAT_ID_OFFSET + TRAPZ_CAT_ID_SIZE)
#define TRAPZ_LEVEL_OFFSET      (TRAPZ_FLAGS_OFFSET + TRAPZ_FLAGS_SIZE)

/* ctrl field shift in */
#define TRAPZ_LEVEL_IN(x) ((x & TRAPZ_LEVEL_MASK) << TRAPZ_LEVEL_OFFSET)
#define TRAPZ_FLAGS_IN(x) ((x & TRAPZ_FLAGS_MASK) << TRAPZ_FLAGS_OFFSET)
#define TRAPZ_CAT_ID_IN(x) ((x & TRAPZ_CAT_ID_MASK) << TRAPZ_CAT_ID_OFFSET)
#define TRAPZ_COMP_ID_IN(x) ((x & TRAPZ_COMP_ID_MASK) << TRAPZ_COMP_ID_OFFSET)
#define TRAPZ_TRACE_ID_IN(x) \
((x & TRAPZ_TRACE_ID_MASK) << TRAPZ_TRACE_ID_OFFSET)

/* Log Macro definitions
 */
/* A simplified macro which uses the component category defined in
  trapz_generated.h */
#define TRAPZ_ILOG(level, flags, component, trace_id, extra1, extra2, extra3, \
	extra4) \
	trapz_ilog(level, flags, component##__CAT, component##__ID, trace_id, \
	extra1, extra2, extra3, extra4)

/* The following macros depend on a pre-compilation step to generate
 * numeric trace IDs from human-readable identifiers.
 *
 * Each macro takes an argument "trace" which is an identifier that
 * describes the purpose of the log. This identifier doesn't need to
 * be valid in the current scope.  For example:
 *
 * TRAPZ_LOG(TRAPZ_LOG_INFO, 0, TRAPZ_KERN, descriptive_phrase_here, 0, 0)
 *
 * After adding any new log statement, you must run 'make trapz' at
 * top level before building your code. The toolchain will parse your
 * TRAPZ_LOG call and #define the symbol you have used to an
 * autogenerated unique ID. In this case it would generate something
 * like this:
 *
 * #define TRAPZ_KERN___descriptive_phrase_here 3517
 *
 * XXXmunsal Be aware the parser is for the moment extremely crude and
 * not aware of C syntax. For example, if you have a TRAPZ_LOG
 * call that has been commented out, trapztool will still try to parse
 * it. Also the parser will be confused by extra commas contained
 * within nested expressions, such as here:
 *
 * TRAPZ_LOG(calculateLogLevel(foo,bar,baz), 0, TRAPZ_KERN, whatever, 0, 0)
 *
 * As far as my shitty parser is concerned, the first argument to
 * TRAPZ_LOG is 'calculateLogLevel(foo'. Epic fail.
 *
 * XXXmunsal Be aware that 'trace' must currently be a valid identifier string
 * in both C and Verilog (it ends up going in VCD file). I need to investigate
 * whether there are valid C identifiers that aren't valid Verilog identifiers
 * and either document that or convert them in toolchain.
 */
#define TRAPZ_LOG(level, flags, component, trace, extra1, extra2, extra3, \
	extra4) \
	TRAPZ_ILOG(level, flags, component, component##___##trace, \
		extra1, extra2, extra3, extra4)

/* Macro for logging with a printf string. The string is ignored in
 * compilation. It is used by code analysis tools to make the TRAPZ
 * log more readable.
 *
 * TRAPZ_LOG_PRINTF(TRAPZ_LOG_INFO, 0, TRAPZ_KERN_TOUCH, touch,
 *                  "Touch event in function Foo at %d %d", x, y)
 *
 * XXXmunsal I will warn again... due to my shitty parser you MUST
 * NOT have any commas or parentheses in your format string!
 *
 * XXXmunsal Be aware that format string will actually be formatted
 * by Python % operator, not C printf(). They are broadly compatible
 * but you're best off keeping it simple. I need to investigate
 * differences and document them here.
 */
#define TRAPZ_LOG_PRINTF(level, flags, component, trace, format, extra1, \
	extra2, extra3, extra4) \
	TRAPZ_ILOG(level, flags, component, component##___##trace, extra1, \
	extra2, extra3, extra4)

/* Macros for logging an interval. Similar to TRAPZ_LOG but will
 * be handled intelligently by postprocessing tools.
 */
#define TRAPZ_LOG_BEGIN(level, flags, component, trace) \
	TRAPZ_ILOG(level, flags, component, component##___##trace, 0, 1, 0, 0)
#define TRAPZ_LOG_END(level, flags, component, trace) \
	TRAPZ_ILOG(level, flags, component, component##___##trace, 0, 0, 0, 0)

/* Macros for logging function scope. These are slightly different, in
 * that the identifier used to generate trace IDs MUST be a valid C
 * identifier; it should be the name of the enclosing function. This
 * is used to embed the function pointer in the log. Example:
 *
 * void myFunction(void) {
 *   TRAPZ_LOG_ENTER(TRAPZ_LOG_INFO, 0, TRAPZ_KERN, myFunction);
 *   do_stuff();
 *   if (stuff_failed()) {
 *     TRAPZ_LOG_FAIL(TRAPZ_LOG_INFO, 0, TRAPZ_KERN, myFunction);
 *     return;
 *   }
 *   TRAPZ_LOG_EXIT(TRAPZ_LOG_INFO, 0, TRAPZ_KERN, myFunction);
 * }
 *
 * XXXmunsal The utility of these macros is unclear. Unless we have
 * postprocessing tools that use symbol tables, the function pointr is pretty
 * useless to us. I expect these will eventually move into TRAPZ_LOG_BEGIN/END.
 */
#define TRAPZ_LOG_ENTER(level, flags, component, fn_ptr) \
	TRAPZ_ILOG(level, flags, component, component##___##fn_ptr, \
	(int)fn_ptr, 1, 0, 0)
#define TRAPZ_LOG_EXIT(level, flags, component, fn_ptr) \
	TRAPZ_ILOG(level, flags, component, component##___##fn_ptr, \
	(int)fn_ptr, 0, 0, 0)
#define TRAPZ_LOG_FAIL(level, flags, component, fn_ptr) \
	TRAPZ_ILOG(level, flags, component, component##___##fn_ptr, \
	(int)fn_ptr, -1, 0, 0)

/* Macro for generating documentation of a Trapz trace
 *
 * E.g.:
 *
 * TRAPZ_DESCRIBE(TRAPZ_KERN, my_log,
 * "Here is a multi line string literal talking about"
 * "just what this log point means."
 * "You can even use <strong>HTML TAGS!</strong>")
 * TRAPZ_LOG(TRAPZ_LOG_INFO, 0, TRAPZ_KERN, my_log, 0, 0)
 */
#define TRAPZ_DESCRIBE(component, trace_or_fn_ptr, description)

/* Lowest level macros. This is where CONFIG_TRAPZ_TP takes effect. */
#ifdef CONFIG_TRAPZ_TP

#define trapz_ilog(level, flags, cat_id, comp_id, trace_id, extra1, extra2, \
	extra3, extra4)  \
	trapz_ilog_info(level, flags, cat_id, comp_id, trace_id, \
		extra1, extra2, extra3, extra4, NULL)

#define trapz_ilog_info(level, flags, cat_id, comp_id, trace_id, extra1, \
	extra2, extra3, extra4, trapzinfo)  \
	systrapz(                             \
		TRAPZ_LEVEL_IN(level) |        \
		TRAPZ_FLAGS_IN(flags) |        \
		TRAPZ_CAT_ID_IN(cat_id) |       \
		TRAPZ_COMP_ID_IN(comp_id) |     \
		TRAPZ_TRACE_ID_IN(trace_id),    \
		extra1, extra2, extra3, extra4, trapzinfo)

#else /* CONFIG_TRAPZ_TP */

#define trapz_ilog(level, flags, cat_id, comp_id, trace_id, extra1, extra2, \
	extra3, extra4)
#define trapz_ilog_info(level, flags, cat_id, comp_id, trace_id, extra1, \
	extra2, extra3, extra4, trapzinfo)

#endif /* CONFIG_TRAPZ_TP */

/* Category definitions
 */
#define TRAPZ_CAT_KERNEL         0
#define TRAPZ_CAT_PLATFORM       1
#define TRAPZ_CAT_APPS           2

/* Level definitions
 */
#define TRAPZ_LOG_OFF   3
#define TRAPZ_LOG_INFO    2
#define TRAPZ_LOG_DEBUG   1
#define TRAPZ_LOG_VERBOSE 0

#define TRAPZ_MAX_COMP_ID         0xfff         /* 4095 */

#endif  /* _LINUX_TRAPZ_H */
