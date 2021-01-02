 ##########################################################################
 #
 #     This file is part of Chiton.
 #
 #   Chiton is free software: you can redistribute it and/or modify
 #   it under the terms of the GNU General Public License as published by
 #   the Free Software Foundation, either version 3 of the License, or
 #   (at your option) any later version.
 #
 #   Chiton is distributed in the hope that it will be useful,
 #   but WITHOUT ANY WARRANTY; without even the implied warranty of
 #   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 #   GNU General Public License for more details.
 #
 #   You should have received a copy of the GNU General Public License
 #   along with Chiton.  If not, see <https://www.gnu.org/licenses/>.
 #
 #   Copyright 2020 Ed Martin <edman007@edman007.com>
 #
 ##########################################################################

define([BUILDVER], [0.3.0pre])
AC_INIT([Chiton Security], BUILDVER, [edman007@edman007.com], [chiton], [https://edman007.com/])
AC_CONFIG_SRCDIR([main.cpp])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([subdir-objects tar-pax dist-xz])
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
AC_SUBST([DIST_VERSION], [$git_version])

AC_DEFINE_UNQUOTED([GIT_VER], GITVERSION, [Git version])

AS_IF([test "x$git_version" != "xUnknown"],
            AC_SUBST([VER_GITVER], [_$git_version]),
            AC_SUBST([VER_GITVER], [])
            AC_MSG_WARN([Unable to detect the git revision of this build]))
define([CONFDATE], [$(echo \"$(date -R)\")])
AC_DEFINE_UNQUOTED([BUILD_DATE], CONFDATE, [Build Date])

AC_PROG_CC
AC_PROG_CXX

#Build adjustements
AC_ARG_ENABLE([debug], [AS_HELP_STRING([--enable-debug], [Enable Debugging])], [], [enable_debug=no])
AC_ARG_ENABLE([profiling], [AS_HELP_STRING([--enable-profiling], [Enable Profiling])], [], [enable_profiling=no])


AS_IF([test "x$enable_debug" != "xno"],
    [AS_VAR_APPEND([[CXXFLAGS]], [[" -g -O0 -rdynamic "]])
    AC_DEFINE([DEBUG], [1], [Debugging enabled])
    ],
    [AS_VAR_APPEND([[CXXFLAGS]], [[" -O3 "]])
    AC_DEFINE([NDEBUG], [1], [Debugging Disabled])
    ]
)

#CPU specific debug flags...
case $host in
       arm*-*-*)   AS_IF([test "x$enable_debug" != "xno"], [AS_VAR_APPEND([[CXXFLAGS]], [[" -funwind-tables "]])]) ;;
       i?86-*-*)   AS_IF([test "x$enable_debug" != "xno"], [], [AS_VAR_APPEND([[CXXFLAGS]], [["  "]])]) ;;
esac

AS_IF([test "x$enable_profiling" != "xno"],
    [AS_VAR_APPEND([[CXXFLAGS]], [[" -pg -fprofile-arcs -ftest-coverage "]])
    ],
    []
)


#our required libs:
PKG_PROG_PKG_CONFIG

#untested with mysql...
PKG_CHECK_MODULES([MARIADB], [mariadb],
                             [AS_VAR_APPEND([[CPPFLAGS]], [" $MARIADB_CFLAGS"])
                             AS_VAR_APPEND([[LIBS]], [" $MARIADB_LIBS "])],
                             [PKG_CHECK_MODULES([MYSQL], [mysqlclient],
                             [AS_VAR_APPEND([[CPPFLAGS]], [" $MYSQL_CFLAGS"])
                             AS_VAR_APPEND([[LIBS]], [" $MYSQL_LIBS "])]
                             )])


##ffmpeg
PKG_CHECK_MODULES([FFMPEG], [libavformat libavcodec libavutil],
                             [AS_VAR_APPEND([[CPPFLAGS]], [" $FFMPEG_CFLAGS"])
                             AS_VAR_APPEND([[LIBS]], [" $FFMPEG_LIBS "])])

AC_ARG_WITH([apacheconfigdir],
  [AS_HELP_STRING([--with-apacheconfigdir=DIR], [Directory to install apache config])],
  [apacheconfdir=$withval],
  [apacheconfdir="\${sysconfdir}/apache2/sites-available/"])
AC_SUBST([apacheconfdir])

AC_ARG_WITH([runuser],
  [AS_HELP_STRING([--with-runuser=USER], [Username to run the daemon as [$PACKAGE_TARNAME]])],
  [runuser=$withval],
  [runuser=$PACKAGE_TARNAME])
AC_SUBST([runuser])

AC_ARG_WITH([smartydir],
  [AS_HELP_STRING([--with-smartydir=DIR], [Directory of system Smarty 3.x installation  [no]])],
  [SMARTYDIR=$withval],
  [SMARTYDIR=no])
AC_SUBST([SMARTYDIR])
AM_CONDITIONAL([INSTALLSMARTY], [test "x$SMARTYDIR" = "xno"])

AC_ARG_WITH([initdir],
  [AS_HELP_STRING([--with-initdir=DIR], [Directory of init script [SYSCONFDIR/systemd/system]])],
  [initdir=$withval],
  [initdir=${sysconfdir}/systemd/system])
AC_SUBST([initdir])

AC_ARG_ENABLE([sysv],
  [AS_HELP_STRING([--with-sysv], [Install a SysV compatible init script [disabled]])],
  [sysv=true],
  [sysv=false])
AM_CONDITIONAL([ENABLESYSV], [test "x$sysv" = "xtrue"])

AC_ARG_WITH([webuser],
  [AS_HELP_STRING([--with-webuser=user], [User that the webserver will run under [apache]])],
  [webuser=$withval],
  [webuser=apache])
AC_SUBST([webuser])

AC_ARG_WITH([apachesymlinkdir],
  [AS_HELP_STRING([--with-apachesymlinkdir=dir], [Directory to create a symlink to the conf [no]])],
  [apachesymlinkdir=$withval],
  [apachesymlinkdir=no])
AC_SUBST([apachesymlinkdir])

AC_ARG_WITH([apacheappendconf],
  [AS_HELP_STRING([--with-apacheappendconf=file], [File to include the configuration from [no]])],
  [apacheappendconf=$withval],
  [apacheappendconf=apache])
AC_SUBST([apacheappendconf])


AC_ARG_VAR([IMAGEMAGICK_CONVERT], [The path to imagemagick's convert])
AC_CHECK_PROG([IMAGEMAGICK_CONVERT], [convert], [convert], [false])
AS_IF([test x"$IMAGEMAGICK_CONVERT" = x"false"], [AC_MSG_ERROR([Could not find imagemagick, required for building])])

AC_CONFIG_FILES(Makefile)
AC_CONFIG_HEADERS([config_build.hpp])
AC_OUTPUT
