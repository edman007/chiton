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
 #   Copyright 2020-2022 Ed Martin <edman007@edman007.com>
 #
 ##########################################################################

define([BUILDVER], [0.5.0])
AC_INIT([Chiton Security], BUILDVER, [edman007@edman007.com], [chiton], [https://edman007.com/])
AC_CONFIG_SRCDIR([main.cpp])
AC_CONFIG_MACRO_DIR([m4])
AC_CANONICAL_TARGET
AM_INIT_AUTOMAKE([subdir-objects tar-pax dist-xz])
LT_INIT

#pull the correct git revesion
git_version=$(echo $(git log --oneline --no-walk || echo Unknown) | head -1 | cut -d ' ' -f 1 )
AS_IF([test "x$git_version" == "xUnknown"],
            #assume we are in a dist build, pull the version from that
            AC_CHECK_FILE([$srcdir/dist-version],
                   git_version=$(cat "$srcdir/dist-version"),
                   []),
       [])

git_version_str=$(echo '"'git_$git_version'"')
define([GITVERSION], $git_version_str)
AC_SUBST([DIST_VERSION], [$git_version])

AC_DEFINE_UNQUOTED([GIT_VER], GITVERSION, [Git version])
AS_IF([echo BUILDVER | grep -q pre],
  AC_DEFINE_UNQUOTED([BUILD_VERSION], ["]BUILDVER[_$git_version"], [Build Version]),
  AC_DEFINE([BUILD_VERSION], ["]BUILDVER["], [Build Version])
)

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
PKG_CHECK_MODULES([FFMPEG], [libavformat libavcodec libavutil libavfilter],
                             [AS_VAR_APPEND([[CPPFLAGS]], [" $FFMPEG_CFLAGS"])
                             AS_VAR_APPEND([[LIBS]], [" $FFMPEG_LIBS "])])

#pthread (actually not required, but we do have a dep for it..)
AX_PTHREAD([AS_VAR_APPEND([[CPPFLAGS]], [" $PTHREAD_CFLAGS"])
                             AS_VAR_APPEND([[LIBS]], [" $PTHREAD_LIBS "])
                             AC_DEFINE([HAVE_PTHREAD], [1], [Have pthread])])

#On 32-bit we need libatomic to give us 64-bit atomics
CHECK_ATOMIC()

AC_ARG_ENABLE([vaapi],
  [AS_HELP_STRING([--enable-vaapi], [Enable VA-API, not strictly required to use vaapi])],
  [],
  [enable_vaapi=auto])
AS_IF([test "x$enable_vaapi" != "xno"],
            [PKG_CHECK_MODULES([VA], [libva],
             [have_vaapi=1 AS_VAR_APPEND([[CPPFLAGS]], [" $VA_CFLAGS "])
             AS_VAR_APPEND([[LIBS]], [" $VA_LIBS "])])
             AC_DEFINE([HAVE_VAAPI], [1], [Have VAAPI])])
AS_IF([test "x$enable_vaapi" == "xyes" && test "x$have_vaapi" != "x1"], [AC_MSG_ERROR([Could not find vaapi])])

AC_ARG_ENABLE([vdpau],
  [AS_HELP_STRING([--enable-vdpau], [Enable VA-API, not strictly required to use vdpau])],
  [],
  [enable_vdpau=auto])
AS_IF([test "x$enable_vdpau" != "xno"],
            [PKG_CHECK_MODULES([VDPAU], [vdpau],
             [have_vdpau=1 AS_VAR_APPEND([[CPPFLAGS]], [" $VDPAU_CFLAGS "])
             AS_VAR_APPEND([[LIBS]], [" $VDPAU_LIBS "])])
             AC_DEFINE([HAVE_VDPAU], [1], [Have VDPAU])])
AS_IF([test "x$enable_vdpau" == "xyes" && test "x$have_vdpau" != "x1"], [AC_MSG_ERROR([Could not find vdpau])])

AC_ARG_ENABLE([opencv],
  [AS_HELP_STRING([--enable-opencv], [Enable OpenCV and associated components])],
  [],
  [enable_opencv=auto])
AS_IF([test "x$enable_opencv" != "xno"],
            [PKG_CHECK_MODULES([OPENCV], [opencv4],
             [have_opencv=1 AS_VAR_APPEND([[CPPFLAGS]], [" $OPENCV_CFLAGS "])
             AS_VAR_APPEND([[LIBS]], [" $OPENCV_LIBS "])])
             AC_DEFINE([HAVE_OPENCV], [1], [Have OpenCV])])
AS_IF([test "x$enable_opencv" == "xyes" && test "x$have_opencv" != "x1"], [AC_MSG_ERROR([Could not find OpenCV])])

AC_ARG_ENABLE([opencl],
  [AS_HELP_STRING([--enable-opencl], [Enable OpenCL and associated components])],
  [],
  [enable_opencl=auto])
AS_IF([test "x$enable_opencl" != "xno"],[AX_CHECK_CL])
AS_IF([test "x$enable_opencl" == "xyes" && test "X$no_cl" == "Xyes"], [AC_MSG_ERROR([Could not find OpenCL])])
AS_IF([test "X$no_cl" == "Xyes" || test "x$enable_opencl" == "xno"], [AC_MSG_NOTICE([OpenCL Support Disabled])], [AS_VAR_APPEND([[CPPFLAGS]], [" $CL_CFLAGS "])
             AS_VAR_APPEND([[LIBS]], [" $CL_LIBS "])
             AC_DEFINE([HAVE_OPENCL], [1], [Have OpenCL])])

AC_ARG_ENABLE([v4l2],
  [AS_HELP_STRING([--enable-v4l2], [Enable V4L2 and associated components])],
  [],
  [enable_v4l2=auto])
AS_IF([test "x$enable_v4l2" != "xno"],[AC_CHECK_HEADER([libv4l2.h],
            [AC_CHECK_LIB([v4l2], [v4l2_open], [no_v4l2=no], [no_v4l2=yes])],
            [no_v4l2=yes])])
AC_CHECK_HEADER([linux/videodev2.h], [], [no_v4l2=yes])
AS_IF([test "x$enable_v4l2" == "xyes" && test "X$no_v4l2" == "Xyes"], [AC_MSG_ERROR([Could not find V4L2])])
AS_IF([test "X$no_v4l2" == "Xyes" || test "x$enable_v4l2" == "xno"], [AC_MSG_NOTICE([V4L2 Support Disabled])], [AS_VAR_APPEND([[CPPFLAGS]], [" $V4L2_CFLAGS "])
             AS_VAR_APPEND([[LIBS]], [" -lv4l2 "])
             AC_DEFINE([HAVE_V4L2], [1], [Have V4L2])])



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


AC_ARG_VAR([RSVG_CONVERT], [The path to rsvg-convert])
AC_CHECK_PROG([RSVG_CONVERT], [rsvg-convert], [rsvg-convert], [false])
AS_IF([test x"$RSVG_CONVERT" = x"false"], [AC_MSG_ERROR([Could not find rsvg-convert, required for building])])

AC_CONFIG_FILES(Makefile)
AC_CONFIG_HEADERS([config_build.hpp])
AC_OUTPUT
