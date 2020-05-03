define([BUILDVER], [git])
AC_INIT([Chiton Security], BUILDVER, [edman007@edman007.com], [chiton], [http://edman007.com/])
AC_CONFIG_SRCDIR([main.cpp])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([subdir-objects])
LT_INIT

#pull the correct git revesion
git_version=$(echo $(git log --oneline --no-walk || echo Unknown) | head -1 | cut -d ' ' -f 1 )
AS_IF([test "x$git_version" == "xUnknown"],
            #assume we are in a dist build, pull the version from that
            AC_CHECK_FILE(["$srcdir/dist-version"],
                   git_version=$(cat "$srcdir/dist-version"),
                   []),
       [])

git_version_str=$(echo '"'git_$git_version'"')
define([GITVERSION], [$git_version_str])
AC_DEFINE_UNQUOTED([GIT_VER], GITVERSION, [Git version])

AS_IF([test "x$git_version" != "xUnknown"],
            AC_SUBST([VER_GITVER], [_$git_version]),
            AC_SUBST([VER_GITVER], [])
            AC_MSG_WARN([Unable to detect the git revision of this build]))
define([CONFDATE], [$(echo '"'$(date -R)'"')])
AC_DEFINE_UNQUOTED([BUILD_DATE], CONFDATE, [Build Date])


AC_PROG_CC
AC_PROG_CXX

#Build adjustements
AC_ARG_ENABLE([debug], [AS_HELP_STRING([--enable-debug], [Enable Debugging])], [], [enable_debug=no])
AC_ARG_ENABLE([profiling], [AS_HELP_STRING([--enable-profiling], [Enable Profiling])], [], [enable_profiling=no])


AS_IF([test "x$enable_debug" != "xno"],
    [AS_VAR_APPEND([[CFLAGS]], [[" -g -O0 -rdynamic "]])
    AC_DEFINE([DEBUG], [1], [Debugging enabled])
    ],
    [AS_VAR_APPEND([[CFLAGS]], [[" -O3 "]])
    AC_DEFINE([NDEBUG], [1], [Debugging Disabled])
    ]
)

#CPU specific debug flags...
case $host in
       arm*-*-*)   AS_IF([test "x$enable_debug" != "xno"], [AS_VAR_APPEND([[CFLAGS]], [[" -funwind-tables "]])]) ;;
       i?86-*-*)   AS_IF([test "x$enable_debug" != "xno"], [], [AS_VAR_APPEND([[CFLAGS]], [["  "]])]) ;;
esac

AS_IF([test "x$enable_profiling" != "xno"],
    [AS_VAR_APPEND([[CFLAGS]], [[" -pg -fprofile-arcs -ftest-coverage "]])
    ],
    []
)

AC_CONFIG_FILES(Makefile)
AC_OUTPUT
