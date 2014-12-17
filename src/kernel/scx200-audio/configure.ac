AC_INIT(configure.ac)

AC_PROG_CC
AC_CHECK_TOOL(LD, ld, :)
AC_ARG_WITH(kernel-path, [  --with-kernel-path    Specify the kernel path],
KERNEL_SRC=$withval, KERNEL_SRC="/usr/src/linux")

AC_ARG_WITH(module-path, [  --with-module-path    Specify where to install the modules], INSTALL_MOD_PATH=$withval, INSTALL_MOD_PATH=/lib/modules)

CFLAGS="$CFLAGS -Wall"

AC_SUBST(KERNEL_SRC)
AC_SUBST(INSTALL_MOD_PATH)

AC_OUTPUT([
./Makefile
])

