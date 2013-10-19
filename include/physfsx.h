/* $Id: physfsx.h,v 1.1.1.1 2006/03/17 20:01:28 zicodxx Exp $ */

/*
 *
 * Some simple physfs extensions
 *
 */

#ifndef PHYSFSX_H
#define PHYSFSX_H

#include <string.h>
#include <stdarg.h>
#include <stdio.h>

// When PhysicsFS can *easily* be built as a framework on Mac OS X,
// the framework form will be supported again -kreatordxx
#if 1	//!(defined(__APPLE__) && defined(__MACH__))
#include <physfs.h>
#else
#include <physfs/physfs.h>
#endif

#ifdef DEBUG_PHYSFSX_CHATTY
#include <ctype.h>

#define PHYSFSX_CHATTY_PRE_TELL(A)	PHYSFS_sint64 t = g_physfsx_chatty ? PHYSFS_tell((A)) : 0
#define PHYSFSX_CHATTY_PRINTF(F,...)	if (g_physfsx_chatty) fprintf(stderr, "%s:%u:" F, func, line, __VA_ARGS__)
extern unsigned char g_physfsx_chatty;

static inline PHYSFS_sint64 PHYSFSX_read_chatty(const char *func,unsigned line,PHYSFS_File *handle, void *buffer, PHYSFS_uint32 objSize, PHYSFS_uint32 objCount)
{
	PHYSFSX_CHATTY_PRE_TELL(handle);
	PHYSFS_sint64 r = (PHYSFS_read)(handle, buffer, objSize, objCount);
	PHYSFSX_CHATTY_PRINTF("PHYSFS_read(%p [o=%#4lx -> %#4lx], %p, %u, %u) = %li\n", handle, (unsigned long) t, (unsigned long) PHYSFS_tell(handle), buffer, objSize, objCount, (unsigned long) r);
	return r;
}
#define PHYSFS_read(A,B,C,D)	((PHYSFSX_read_chatty)(__func__, __LINE__,(A),(B),(C),(D)))
#else
#define PHYSFSX_CHATTY_PRE_TELL(A)
#define PHYSFSX_CHATTY_PRINTF(F,...)
#endif

#include "pstypes.h"
#include "strutil.h"
#include "u_mem.h"
#include "dxxerror.h"
#include "vecmat.h"
#include "args.h"
#include "ignorecase.h"
#include "byteswap.h"
#include "compiler.h"

typedef int PHYSFSX_UNSAFE_TRUNCATE_TO_32BIT_INT;
typedef long PHYSFSX_UNSAFE_TRUNCATE_TO_LONG;
typedef unsigned PHYSFSX_UNSAFE_TRUNCATE_TO_UNSIGNED;
typedef unsigned PHYSFSX_UNSAFE_TRUNCATE_TO_32BIT_UINT;

#ifdef __cplusplus

extern void PHYSFSX_init(int argc, char *argv[]);

static inline int PHYSFSX_readSXE16(PHYSFS_file *file, int swap)
{
	PHYSFS_sint16 val;

	PHYSFS_read(file, &val, sizeof(val), 1);

	return swap ? SWAPSHORT(val) : val;
}

static inline int PHYSFSX_readSXE32(PHYSFS_file *file, int swap)
{
	PHYSFS_sint32 val;

	PHYSFS_read(file, &val, sizeof(val), 1);

	return swap ? SWAPINT(val) : val;
}

static inline void PHYSFSX_readVectorX(PHYSFS_file *file, vms_vector *v, int swap)
{
	v->x = PHYSFSX_readSXE32(file, swap);
	v->y = PHYSFSX_readSXE32(file, swap);
	v->z = PHYSFSX_readSXE32(file, swap);
}

static inline void PHYSFSX_readAngleVecX(PHYSFS_file *file, vms_angvec *v, int swap)
{
	v->p = PHYSFSX_readSXE16(file, swap);
	v->b = PHYSFSX_readSXE16(file, swap);
	v->h = PHYSFSX_readSXE16(file, swap);
}

static inline PHYSFSX_UNSAFE_TRUNCATE_TO_32BIT_INT PHYSFSX_writeU8(PHYSFS_file *file, PHYSFS_uint8 val)
{
	return (PHYSFSX_UNSAFE_TRUNCATE_TO_32BIT_INT)PHYSFS_write(file, &val, 1, 1);
}

static inline PHYSFSX_UNSAFE_TRUNCATE_TO_32BIT_INT PHYSFSX_writeString(PHYSFS_file *file, const char *s)
{
	return (PHYSFSX_UNSAFE_TRUNCATE_TO_32BIT_INT)PHYSFS_write(file, s, 1, strlen(s) + 1);
}

static inline PHYSFSX_UNSAFE_TRUNCATE_TO_32BIT_INT PHYSFSX_puts(PHYSFS_file *file, const char *s)
{
	return (PHYSFSX_UNSAFE_TRUNCATE_TO_32BIT_INT)PHYSFS_write(file, s, 1, strlen(s));
}

static inline int PHYSFSX_putc(PHYSFS_file *file, int c)
{
	unsigned char ch = (unsigned char)c;

	if (PHYSFS_write(file, &ch, 1, 1) < 1)
		return -1;
	else
		return (int)c;
}

#ifdef DEBUG_PHYSFSX_CHATTY
static inline int PHYSFSX_fgetc(const char *func,unsigned line,PHYSFS_file *const fp)
#define PHYSFSX_fgetc(A)	((PHYSFSX_fgetc)(__func__, __LINE__, (A)))
#else
static inline int PHYSFSX_fgetc(PHYSFS_file *const fp)
#endif
{
	unsigned char c;
	PHYSFSX_CHATTY_PRE_TELL(fp);

	if ((PHYSFS_read)(fp, &c, 1, 1) != 1)
	{
		PHYSFSX_CHATTY_PRINTF("PHYSFS_fgetc(%p [o=%#4lx -> %#4lx]) = EOF\n", fp, (unsigned long) t, (unsigned long) PHYSFS_tell(fp));
		return EOF;
	}
	PHYSFSX_CHATTY_PRINTF("PHYSFS_fgetc(%p [o=%#4lx -> %#4lx]) = %#2x '%c'\n", fp, (unsigned long) t, (unsigned long) PHYSFS_tell(fp), c, isprint((unsigned)c) ? c : '.');
	return c;
}

#ifdef DEBUG_PHYSFSX_CHATTY
static inline int PHYSFSX_fseek(const char *func,unsigned line,PHYSFS_file *fp, long int offset, int where)
#define PHYSFSX_fseek(A,B,C)	((PHYSFSX_fseek)(__func__,__LINE__,(A),(B),(C)))
#else
static inline int PHYSFSX_fseek(PHYSFS_file *fp, long int offset, int where)
#endif
{
	PHYSFS_sint64 c, goal_position;
	PHYSFSX_CHATTY_PRE_TELL(fp);

	switch(where)
	{
	case SEEK_SET:
		goal_position = offset;
		break;
	case SEEK_CUR:
		goal_position = PHYSFS_tell(fp) + offset;
		break;
	case SEEK_END:
		goal_position = PHYSFS_fileLength(fp) + offset;
		break;
	default:
		return 1;
	}
	c = PHYSFS_seek(fp, goal_position);
	PHYSFSX_CHATTY_PRINTF("PHYSFSX_fseek(%p [o=%#4lx -> %#4lx], %li, %i) = %i\n", fp, (unsigned long) t, (unsigned long) PHYSFS_tell(fp), offset, where, !c);
	return !c;
}

#if defined(__GNUC__) && defined(__OPTIMIZE__) && defined(__INLINE__)
void __trap_PHYSFSX_fgets_wrong_size() __attribute__((__error__("wrong size passed to PHYSFSX_fgets")));
void __trap_PHYSFSX_fgets_unknown_size() __attribute__((__error__("unknown size passed to PHYSFSX_fgets")));
#define PHYSFSX_fgets(B,N,F)	({	\
	if (__bos((B)) != (size_t)-1) {	\
		if (!__builtin_constant_p((N)))	\
			__trap_PHYSFSX_fgets_unknown_size();	\
		else if ((N) != __bos((B)))	\
			__trap_PHYSFSX_fgets_wrong_size();	\
	}	\
	(PHYSFSX_fgets)((B),(N),(F));	\
	})
#endif
static inline char * (PHYSFSX_fgets)(char *buf, size_t n, PHYSFS_file *const fp)
{
	size_t i;
	int c;

	for (i = 0; i < n - 1; i++)
	{
		do
		{
			c = PHYSFSX_fgetc(fp);
			if (c == EOF)
			{
				*buf = 0;

				return NULL;
			}
			if (c == 0 || c == 10)  // Unix line ending
				break;
			if (c == 13)            // Mac or DOS line ending
			{
				int c1;

				c1 = PHYSFSX_fgetc(fp);
				if (c1 != EOF)  // The file could end with a Mac line ending
					PHYSFSX_fseek(fp, -1, SEEK_CUR);
				if (c1 == 10) // DOS line ending
					continue;
				else            // Mac line ending
					break;
			}
		} while (c == 13);
		if (c == 13)    // because cr-lf is a bad thing on the mac
			c = '\n';   // and anyway -- 0xod is CR on mac, not 0x0a
		if (c == '\n')
			break;
		*buf++ = c;
	}
	*buf = 0;

	return buf;
}

static inline PHYSFSX_UNSAFE_TRUNCATE_TO_32BIT_INT PHYSFSX_printf(PHYSFS_file *file, const char *format, ...) __attribute_gcc_format((printf, 2, 3));
static inline PHYSFSX_UNSAFE_TRUNCATE_TO_32BIT_INT PHYSFSX_printf(PHYSFS_file *file, const char *format, ...)
{
	char buffer[1024];
	va_list args;

	va_start(args, format);
#if defined(_WIN32) && defined(_MSC_VER)
	/* MinGW does not define StringCbVPrintfA */
	StringCbVPrintfA(buffer, sizeof(buffer), format, args);
#else
	vsnprintf(buffer, sizeof(buffer), format, args);
#endif

	return (PHYSFSX_UNSAFE_TRUNCATE_TO_32BIT_INT)PHYSFSX_puts(file, buffer);
}

#define PHYSFSX_writeFix	PHYSFS_writeSLE32
#define PHYSFSX_writeFixAng	PHYSFS_writeSLE16

static inline int PHYSFSX_writeVector(PHYSFS_file *file, vms_vector *v)
{
	if (PHYSFSX_writeFix(file, v->x) < 1 ||
	 PHYSFSX_writeFix(file, v->y) < 1 ||
	 PHYSFSX_writeFix(file, v->z) < 1)
		return 0;

	return 1;
}

static inline int PHYSFSX_writeAngleVec(PHYSFS_file *file, vms_angvec *v)
{
	if (PHYSFSX_writeFixAng(file, v->p) < 1 ||
	 PHYSFSX_writeFixAng(file, v->b) < 1 ||
	 PHYSFSX_writeFixAng(file, v->h) < 1)
		return 0;

	return 1;
}

static inline int PHYSFSX_writeMatrix(PHYSFS_file *file, vms_matrix *m)
{
	if (PHYSFSX_writeVector(file, &m->rvec) < 1 ||
	 PHYSFSX_writeVector(file, &m->uvec) < 1 ||
	 PHYSFSX_writeVector(file, &m->fvec) < 1)
		return 0;

	return 1;
}

#define define_read_helper(T,N,F)	\
	static inline T N(const char *func, const unsigned line, PHYSFS_file *file)	\
	{	\
		T i;	\
		PHYSFSX_CHATTY_PRE_TELL(file);	\
		if (!((F)(file, &i)))	\
		{	\
			(Error)(func, line, "reading " #T " in " #N "() at %#lx", (unsigned long)((PHYSFS_tell)(file)));	\
		}	\
		PHYSFSX_CHATTY_PRINTF("%s(%p [o=%#4lx -> %#4lx], %p): i=%#lx\n", (#N), (file), (unsigned long) t, (unsigned long) PHYSFS_tell((file)), (&i), (unsigned long) (i));	\
		return i;	\
	}

static inline bool PHYSFSX_readS8(PHYSFS_file *file, sbyte *b)
{
	return (PHYSFS_read(file, b, sizeof(*b), 1) == 1);
}

define_read_helper(sbyte, PHYSFSX_readByte, PHYSFSX_readS8);
#define PHYSFSX_readByte(F)	((PHYSFSX_readByte)(__func__, __LINE__, (F)))

define_read_helper(int, PHYSFSX_readInt, PHYSFS_readSLE32);
#define PHYSFSX_readInt(F)	((PHYSFSX_readInt)(__func__, __LINE__, (F)))

define_read_helper(int16_t, PHYSFSX_readShort, PHYSFS_readSLE16);
#define PHYSFSX_readShort(F)	((PHYSFSX_readShort)(__func__, __LINE__, (F)))

define_read_helper(fix, PHYSFSX_readFix, PHYSFS_readSLE32);
#define PHYSFSX_readFix(F)	((PHYSFSX_readFix)(__func__, __LINE__, (F)))

define_read_helper(fixang, PHYSFSX_readFixAng, PHYSFS_readSLE16);
#define PHYSFSX_readFixAng(F)	((PHYSFSX_readFixAng)(__func__, __LINE__, (F)))

static inline void PHYSFSX_readVector(const char *func, const unsigned line, vms_vector *v, PHYSFS_file *file)
{
	v->x = (PHYSFSX_readFix)(func, line, file);
	v->y = (PHYSFSX_readFix)(func, line, file);
	v->z = (PHYSFSX_readFix)(func, line, file);
}
#define PHYSFSX_readVector(V,F)	((PHYSFSX_readVector(__func__, __LINE__, (V), (F))))

static inline void PHYSFSX_readAngleVec(const char *func, const unsigned line, vms_angvec *v, PHYSFS_file *file)
{
	v->p = (PHYSFSX_readFixAng)(func, line, file);
	v->b = (PHYSFSX_readFixAng)(func, line, file);
	v->h = (PHYSFSX_readFixAng)(func, line, file);
}
#define PHYSFSX_readAngleVec(V,F)	((PHYSFSX_readAngleVec(__func__, __LINE__, (V), (F))))

static inline void PHYSFSX_readMatrix(const char *func, const unsigned line, vms_matrix *m,PHYSFS_file *file)
{
	(PHYSFSX_readVector)(func, line, &m->rvec,file);
	(PHYSFSX_readVector)(func, line, &m->uvec,file);
	(PHYSFSX_readVector)(func, line, &m->fvec,file);
}

#define PHYSFSX_readMatrix(M,F)	((PHYSFSX_readMatrix)(__func__, __LINE__, (M), (F)))

#define PHYSFSX_contfile_init PHYSFSX_addRelToSearchPath
#define PHYSFSX_contfile_close PHYSFSX_removeRelFromSearchPath

extern int PHYSFSX_addRelToSearchPath(const char *relname, int add_to_end);
extern int PHYSFSX_removeRelFromSearchPath(const char *relname);
extern int PHYSFSX_fsize(const char *hogname);
extern void PHYSFSX_listSearchPathContent();
extern int PHYSFSX_checkSupportedArchiveTypes();
extern int PHYSFSX_getRealPath(const char *stdPath, char *realPath);
extern int PHYSFSX_isNewPath(const char *path);
extern int PHYSFSX_rename(const char *oldpath, const char *newpath);
extern char **PHYSFSX_findFiles(const char *path, const char *const *exts);
extern char **PHYSFSX_findabsoluteFiles(const char *path, const char *realpath, const char *const *exts);
extern PHYSFS_sint64 PHYSFSX_getFreeDiskSpace();
extern int PHYSFSX_exists(const char *filename, int ignorecase);
extern PHYSFS_file *PHYSFSX_openReadBuffered(const char *filename);
extern PHYSFS_file *PHYSFSX_openWriteBuffered(const char *filename);
extern void PHYSFSX_addArchiveContent();
extern void PHYSFSX_removeArchiveContent();

#endif

#endif /* PHYSFSX_H */
