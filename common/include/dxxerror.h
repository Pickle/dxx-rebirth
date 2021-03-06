/*
THE COMPUTER CODE CONTAINED HEREIN IS THE SOLE PROPERTY OF PARALLAX
SOFTWARE CORPORATION ("PARALLAX").  PARALLAX, IN DISTRIBUTING THE CODE TO
END-USERS, AND SUBJECT TO ALL OF THE TERMS AND CONDITIONS HEREIN, GRANTS A
ROYALTY-FREE, PERPETUAL LICENSE TO SUCH END-USERS FOR USE BY SUCH END-USERS
IN USING, DISPLAYING,  AND CREATING DERIVATIVE WORKS THEREOF, SO LONG AS
SUCH USE, DISPLAY OR CREATION IS FOR NON-COMMERCIAL, ROYALTY OR REVENUE
FREE PURPOSES.  IN NO EVENT SHALL THE END-USER USE THE COMPUTER CODE
CONTAINED HEREIN FOR REVENUE-BEARING PURPOSES.  THE END-USER UNDERSTANDS
AND AGREES TO THE TERMS HEREIN AND ACCEPTS THE SAME BY USE OF THIS FILE.
COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

/*
 *
 * Header for error handling/printing/exiting code
 *
 */

#ifndef _ERROR_H
#define _ERROR_H

#include <stdio.h>
#include "dxxsconf.h"
#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __GNUC__
#define __noreturn __attribute__ ((noreturn))
#else
#define __noreturn
#endif

void warn_printf(const char *s);
int error_init(void (*func)(const char *));    //init error system, returns 0=ok
void Warning(const char *fmt,...) __attribute_format_printf(1, 2);				//print out warning message to user
void set_warn_func(void (*f)(const char *s));//specifies the function to call with warning messages
void clear_warn_func();//say this function no longer valid
void Error(const char *func, unsigned line, const char *fmt,...) __noreturn __attribute_format_printf(3, 4);				//exit with error code=1, print message
#define Error(F,...)	((Error)(__func__, __LINE__, (F), ## __VA_ARGS__))
#define Assert assert
#ifndef NDEBUG		//macros for debugging

# if defined(__APPLE__) || defined(macintosh)
extern void Debugger(void);	// Avoids some name clashes
#  define Int3 Debugger
# else
#  define Int3() ((void)0)
# endif // Macintosh

#else					//macros for real game

//Changed Assert and Int3 because I couldn't get the macros to compile -KRB
#define Int3() ((void)0)
#endif

#ifdef __cplusplus
}
#endif

#endif /* _ERROR_H */
