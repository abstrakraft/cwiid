AC_INIT(CWiid,0.5.00,wiimote@abstrakraft.org)

AC_PROG_CC
AC_PROG_AWK
AC_PROG_LEX
if test "$LEX" != "flex"; then
	AC_MSG_ERROR([flex not found])
fi
AC_PROG_YACC
if test "$YACC" != "bison -y"; then
	AC_MSG_ERROR([bison not found])
fi

AC_CHECK_LIB([pthread], [pthread_create],,
	AC_MSG_ERROR([pthread library not found]))
AC_CHECK_LIB([bluetooth], [hci_devid],,
	AC_MSG_ERROR([bluetooth library not found]))
AC_CHECK_LIB([dl], [dlopen],,
	AC_MSG_ERROR([dl lib not found]))
#AC_CHECK_LIB([rt], [clock_gettime],,
#	AC_MSG_ERROR([rt lib not found]))

AC_HEADER_STDC
AC_CHECK_HEADER([bluetooth/bluetooth.h],,
	AC_MSG_ERROR([bluetooth/bluetooth.h not found]))
AC_CHECK_HEADERS([bluetooth/l2cap.h bluetooth/hci.h],,
	AC_MSG_ERROR([bluetooth headers not found]),
	[#include <bluetooth/bluetooth.h>])
AC_CHECK_HEADER([linux/input.h],,
	AC_MSG_ERROR([linux/input.h not found]))
AC_CHECK_HEADER([linux/uinput.h],,
	AC_MSG_ERROR([linux/uinput.h]),
	[#include <linux/input.h>])

AC_ISC_POSIX

pkg_modules="gtk+-2.0 >= 2.0.0 gthread-2.0"
PKG_CHECK_MODULES(PACKAGE, [$pkg_modules])
AC_SUBST(PACKAGE_CFLAGS)
AC_SUBST(PACKAGE_LIBS)

AC_OUTPUT(
	[Makefile]
	[common/include/defs.mak]
	[wiimote/Makefile]
	[wmdemo/Makefile]
	[wmgui/Makefile]
	[wminput/Makefile]
	[wminput/plugins/Makefile]
	[wminput/plugins/ir_ptr/Makefile]
	[wminput/plugins/acc/Makefile]
	[wminput/plugins/nunchuk_acc/Makefile]
	)

