#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.65])
AC_INIT([instantsend], [0.8], [martin.habovstiak@gmail.com])
AM_INIT_AUTOMAKE(instantsend, 0.8)
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADERS([config.h])

# Determine OS
operating_system=linux # Default value
AS_IF([test "x$host_alias" '=' "xi686-w64-mingw32"], [operating_system=windows], [])
AS_IF([test "x$host_alias" '=' "xx86_64-w64-mingw32"], [operating_system=windows], [])
AS_IF([test "x$host_alias" '=' "xi686-apple-darwin10"], [operating_system=macosx], [])

# Default package settings
AS_IF([test "x$operating_system" '=' xlinux], [
       default_widget_gtk=yes
       default_plugin_ip6tcp=yes
       default_plugin_ip4tcps=yes
       default_plugin_dbus=yes
       default_plugin_notify=yes
       default_plugin_avahi=yes
       default_scanners=yes
       default_scanner_avahi=yes
       default_filemanager_caja=auto
       default_filemanager_nautilus=auto
       default_bash_completion=yes
       ], [])

AS_IF([test "x$operating_system" '=' xmacosx], [
       default_widget_gtk=no
       default_plugin_ip6tcp=yes
       default_plugin_ip4tcps=yes
       default_plugin_dbus=no
       default_plugin_notify=no
       default_plugin_avahi=no
       default_scanners=yes
       default_scanner_avahi=no
       default_filemanager_caja=no
       default_filemanager_nautilus=no
       default_bash_completion=yes
       ], [])

AS_IF([test "x$operating_system" '=' xwindows], [
       default_widget_gtk=no
       default_plugin_ip6tcp=no
       default_plugin_ip4tcps=no
       default_plugin_dbus=no
       default_plugin_notify=no
       default_plugin_avahi=no
       default_scanners=no
       default_scanner_avahi=no
       default_filemanager_caja=no
       default_filemanager_nautilus=no
       default_bash_completion=no
       ], [])

# Options

AC_ARG_ENABLE([automatic],
	      [AS_HELP_STRING([--enable-automatic=@<:@yes|no@:>@],
				[if enabled, all features found in system will be used; default package will be built if set to no @<:@default=no@:>@]
	      ),
				[],
				[enable_automatic=no]
	      ])

AC_ARG_ENABLE([server],
		[AS_HELP_STRING([--enable-server@<:@=yes|no|auto@:>@],
				[define whether server plugin should be compiled @<:@default=yes@:>@]
		)],
		[],
		[AS_IF([test "x$enable_automatic" = xyes],
		       [enable_server=auto],
		       [enable_server=yes])
		]
		)

AC_ARG_ENABLE([client],
		[AS_HELP_STRING([--enable-client@<:@=yes|no|auto@:>@],
				[define whether client plugin should be compiled @<:@default=yes@:>@]
		)],
		[],
		[AS_IF([test "x$enable_automatic" = xyes],
		       [enable_client=auto],
		       [enable_client=yes])
		]
		)

AC_ARG_ENABLE([configtool],
		[AS_HELP_STRING([--enable-configtool@<:@=yes|no|auto@:>@],
				[define whether configtool plugin should be compiled @<:@default=yes@:>@]
		)],
		[],
		[AS_IF([test "x$enable_automatic" = xyes],
		       [enable_configtool=auto],
		       [enable_configtool=yes])
		]
		)

AC_ARG_ENABLE([widget-gtk],
		[AS_HELP_STRING([--enable-widget-gtk@<:@=yes|no|auto@:>@],
				[define whether widget-gtk should be compiled @<:@default=$default_widget_gtk@:>@]
		)],
		[],
		[
		 AC_MSG_NOTICE([Assingning default value to enable_widget_gtk])
		 AS_IF([test "x$enable_automatic" = xyes],
		       [enable_widget_gtk=auto],
		       [enable_widget_gtk=$default_widget_gtk])
		]
		)

# Enable/disable plugins
AC_ARG_ENABLE([plugin-ip4tcp],
		[AS_HELP_STRING([--enable-plugin-ip4tcp@<:@=yes|no|auto@:>@],
				[define whether ip4tcp plugin should be compiled @<:@default=yes@:>@]
		)],
		[],
		[AS_IF([test "x$enable_automatic" = xyes],
		       [enable_plugin_ip4tcp=auto],
		       [enable_plugin_ip4tcp=yes])
		]
		)

AC_ARG_ENABLE([plugin-ip6tcp],
		[AS_HELP_STRING([--enable-plugin-ip6tcp@<:@=yes|no|auto@:>@],
				[define whether ip6tcp plugin should be compiled @<:@default=$default_plugin_ip6tcp@:>@]
		)],
		[],
		[AS_IF([test "x$enable_automatic" = xyes],
		       [enable_plugin_ip6tcp=auto],
		       [enable_plugin_ip6tcp=$default_plugin_ip6tcp])
		]
		)

AC_ARG_ENABLE([plugin-ip4tcps],
		[AS_HELP_STRING([--enable-plugin-ip4tcps@<:@=yes|no|auto@:>@],
				[define whether ip4tcps plugin should be compiled @<:@default=yes@:>@]
		)],
		[],
		[AS_IF([test "x$enable_automatic" = xyes],
		       [enable_plugin_ip4tcps=auto],
		       [enable_plugin_ip4tcps=$default_plugin_ip4tcps])
		]
		)

AC_ARG_ENABLE([plugin-dbus],
		[AS_HELP_STRING([--enable-plugin-dbus@<:@=yes|no|auto@:>@],
				[define whether dbus plugin should be compiled @<:@default=$default_plugin_dbus@:>@]
		)],
		[],
		[AS_IF([test "x$enable_automatic" = xyes],
		       [enable_plugin_dbus=auto],
		       [enable_plugin_dbus=$default_plugin_dbus])
		]
		)

AC_ARG_ENABLE([plugin-avahi],
	      [AS_HELP_STRING([--enable-plugin-avahi@<:@=yes|no|auto@:>@],
			      [define whether avahi plugin should be compiled @<:@default=$default_plugin_avahi@:>@]
		)],
		[],
		[AS_IF([test "x$enable_automatic" = xyes],
		       [enable_plugin_avahi=auto],
		       [enable_plugin_avahi=$default_plugin_avahi])
		]
		)

AC_ARG_ENABLE([scanners],
	      [AS_HELP_STRING([--enable-scanners@<:@=yes|no@:>@],
			      [define whether scanners should be installed @<:@default=yes@:>@]
	      )],
	      [have_scan="$enable_scanners"],
	      [have_scan=$default_scanners]
	      )

AC_ARG_ENABLE([scanner-avahi],
	      [AS_HELP_STRING([--enable-scanner-avahi@<:@=yes|no|auto@:>@],
			      [define whether avahi scanner should be installed @<:@default=scanners@:>@]
	      )],
	      [],
	      [AS_IF([test "x$have_scan" = xyes],
		     [enable_scanner_avahi=$default_scanner_avahi],
		     [enable_scanner_avahi=no])
	      ]
	      )

AC_ARG_ENABLE([plugin-notify],
	      [AS_HELP_STRING([--enable-plugin-notify@<:@=yes|no|auto@:>@],
			      [define whether notify plugin should be compiled @<:@default=$default_plugin_notify@:>@] 
		)],
		[],
		[AS_IF([test "x$enable_automatic" = xyes],
		       [enable_plugin_notify=auto],
		       [enable_plugin_notify=$default_plugin_notify])
		]
		)

AC_ARG_ENABLE([filemanager-caja],
	      [AS_HELP_STRING([--enable-filemanager-caja@<:@=yes|no|auto@:>@],
			      [define whether extension for caja file manager should be compiled @<:@default=$default_filemanager_caja@:>@] 
		)],
		[],
		[AS_IF([test "x$enable_automatic" = xyes],
		       [enable_filemanager_caja=auto],
		       [enable_filemanager_caja=$default_filemanager_caja])
		]
		)

AC_ARG_ENABLE([filemanager-nautilus],
	      [AS_HELP_STRING([--enable-filemanager-nautilus@<:@=yes|no|auto@:>@],
			      [define whether extension for nautilus file manager should be compiled @<:@default=$default_filemanager_nautilus@:>@] 
		)],
		[],
		[AS_IF([test "x$enable_automatic" = xyes],
		       [enable_filemanager_nautilus=auto],
		       [enable_filemanager_nautilus=$default_filemanager_nautilus])
		]
		)

AC_ARG_ENABLE([bash-completion],
	      [AS_HELP_STRING([--enable-bash-completion@<:@=yes|no|auto@:>@],
			      [define whether bash-completion should be installed @<:@default=$default_bash_completion@:>@] 
		)],
		[],
		[AS_IF([test "x$enable_automatic" = xyes],
		       [enable_bash_completion=auto],
		       [enable_bash_completion=$default_bash_completion])
		]
		)

AC_ARG_ENABLE([doc-html],
	      [AS_HELP_STRING([--enable-doc-html@<:@=yes|no|auto@:>@],
			      [define whether HTML documentation should be built @<:@default=yes@:>@]
			      )],
			      [],
			      [enable_doc_html=yes])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_INSTALL # Do we need this?
AS_IF([test "x$enable_doc_html" '!=' xno], [
	AC_CHECK_PROGS([DOXYGEN], [doxygen])
	])

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h malloc.h netdb.h netinet/in.h stdint.h string.h sys/socket.h sys/time.h unistd.h])

# Check for modules

# Pre-define variables
have_notify=no
have_dbus=no
have_avahi=no
have_gtkmm=no
have_widget_gtk=no
have_caja_extension=no
have_nautilus_extension=no

have_bash_completion=$enable_bash_completion #no checks

# Headers has been checked before
# TODO: Make it better

AS_IF([test "x$enable_server" '!=' xno], [have_server=yes], [have_server=no])
AS_IF([test "x$enable_client" '!=' xno], [have_client=yes], [have_client=no])
AS_IF([test "x$enable_configtool" '!=' xno], [have_configtool=yes], [have_configtool=no])

AS_IF([test "x$enable_plugin_ip4tcp" '!=' xno], [have_ip4tcp=yes], [have_ip4tcp=no])
AS_IF([test "x$enable_plugin_ip6tcp" '!=' xno], [have_ip6tcp=$default_plugin_ip6tcp], [have_ip6tcp=no])

AS_IF([test "x$enable_plugin_ip4tcps" '!=' xno], [
	PKG_CHECK_MODULES([OPENSSL], [openssl],
			  [AC_DEFINE([HAVE_OPENSSL], [1], [Use openssl])
			  have_openssl=yes],
			  [have_openssl=no]
	)
])

# Test libnotify version
AS_IF([test "x$enable_plugin_notify" '!=' xno], [
	PKG_CHECK_MODULES([LIBNOTIFY4], [libnotify >= 0.7.5],
			  [AC_DEFINE([HAVE_LIBNOTIFY4], [1], [Use libnotify4])
			  have_notify=yes
			  libnotify_version=4],
			  [PKG_CHECK_MODULES([LIBNOTIFY1], [libnotify >= 0.4.5],
					     [AC_DEFINE([HAVE_LIBNOTIFY1], [1], [Use libnotify1])
						     have_notify=yes
						     libnotify_version=1
					     ],
					     [
					      have_notify=no
					     ]
					     )
			])

])

AS_IF([test "x$enable_plugin_dbus" '!=' xno || test "x$enable_widget_gtk" '!=' xno], [
	PKG_CHECK_MODULES([DBUS], [dbus-1],
			  [AC_DEFINE([HAVE_DBUS], [1], [Use dbus])
			  have_dbus=yes],
			  [have_dbus=no])
])

AS_IF([test "x$enable_plugin_avahi" '!=' xno], [
	PKG_CHECK_MODULES([AVAHI], [avahi-client],
			  [AC_DEFINE([HAVE_AVAHI], [1], [Use avahi])
			  have_avahi=yes],
			  [have_avahi=no])
])

AS_IF([test "x$enable_widget_gtk" '!=' xno], [
	PKG_CHECK_MODULES([GTKMM], [gtkmm-2.4],
			  [AC_DEFINE([HAVE_GTKMM], [1], [Use gtkmm])
			  have_gtkmm=yes],
			  [have_gtkmm=no])

	# Setup libraries correctly
	PKG_CHECK_MODULES([WIDGET_GTK], [gtkmm-2.4 dbus-1],
			  [],
			  [])
])


# Caja and Nautilus are always automatic or disabled
AS_IF([test "x$enable_filemanager_caja" '!=' xno], [
	PKG_CHECK_MODULES([CAJA_EXTENSION], [libcaja-extension],
			  [AC_DEFINE([HAVE_CAJA], [1], [Use caja])
			  have_caja_extension=yes],
			  [have_caja_extension=no])
])

AS_IF([test "x$enable_filemanager_nautilus" '!=' xno], [
	PKG_CHECK_MODULES([NAUTILUS_EXTENSION3], [libnautilus-extension >= 3.0],
			  [AC_DEFINE([HAVE_NAUTILUS], [1], [Use nautilus])
			  have_nautilus_extension=yes
			  nautilus_extension_version=3.0],
			  [
			  	PKG_CHECK_MODULES([NAUTILUS_EXTENSION2], [libnautilus-extension >= 2.0],
						  [AC_DEFINE([HAVE_NAUTILUS], [1], [Use nautilus])
						  have_nautilus_extension=yes
						  nautilus_extension_version=2.0],
			  			  [have_nautilus_extension=no])
			])
])

AS_IF([test "x$enable_scanner_avahi" '!=' xno], [
       AC_MSG_CHECKING([for avahi-browse])
       AS_IF([which avahi-browse > /dev/null],
	     [AC_MSG_RESULT([yes])
	      have_scanner_avahi=yes],
	     [
	      AC_MSG_RESULT([no])
	      AS_IF([test "x$enable_scanner_avahi" = xyes],
	      	[have_scanner_avahi=yes
		 AC_MSG_WARN([avahi-browse not found. This will cause run-time problem if not solved. Suggestion: install it or disable avahi scanner])
		 ],
	     	[have_scanner_avahi=no],
	      )
	     ])
       ],
       [have_scanner_avahi=no])

AS_IF([test "x$enable_plugin_ip4tcps" = xyes],
      [
       AS_IF([test "x$have_openssl" '!=' xyes],
	     [AC_MSG_ERROR([You have missing module openssl! Install it or disable.])]
	     )
       ]
      )

AS_IF([test "x$enable_plugin_dbus" = xyes],
	[
	 AS_IF([test "x$have_dbus" '!=' xyes],
	       [AC_MSG_ERROR([You have missing module dbus! Install it or disable.])]
	       )
	 ]
      )

AS_IF([test "x$enable_plugin_avahi" = xyes],
	[
	 AS_IF([test "x$have_avahi" '!=' xyes],
	       [AC_MSG_ERROR([You have missing module avahi-client! Install it or disable.])]
	       )
	 ]
      )

AS_IF([test "x$enable_plugin_notify" = xyes],
	[
	 AS_IF([test "x$have_notify" '!=' xyes],
	       [AC_MSG_ERROR([You have missing modules libnotify1 and libnotify4! Install one of them or disable.])]
	       )
	 ]
      )

AS_IF([test "x$enable_widget_gtk" = xyes],
	[
	 AS_IF([test "x$have_gtkmm" '!=' xyes],
	       [AC_MSG_ERROR([You have missing gtkmm! Install it or disable widget-gtk.])]
	       )
	 ]

	AS_IF([test "x$have_dbus" '!=' xyes],
	      [AC_MSG_ERROR([You have missing dbus! Install it or disable widget-gtk.])]
	     )

	have_widget_gtk=yes
      )

AS_IF([test "x$enable_widget_gtk" = xauto],
      [
      AS_IF([test "x$have_gtkmm" = xyes && test "x$have_dbus" != xyes],
	    [have_widget_gtk=yes],
	    [have_widget_gtk=no]
	    )
	])

AS_IF([test "x$DOXYGEN" = "x"],
      [have_doc_html=no],
      [have_doc_html=yes])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_INT64_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_FORK
AC_CHECK_FUNCS([dup2 gethostbyname gettimeofday inet_ntoa socket strerror])

AC_SUBST([have_ip4tcp])
AC_SUBST([have_ip6tcp])
AC_SUBST([have_notify])
AC_SUBST([libnotify_version])
AC_SUBST([have_dbus])
AC_SUBST([have_avahi])
AC_SUBST([have_gtkmm])
AC_SUBST([have_client])
AC_SUBST([have_server])
AC_SUBST([have_configtool])
AC_SUBST([have_widget_gtk])
AC_SUBST([have_scan])
AC_SUBST([have_scanner_avahi])
AC_SUBST([have_caja_extension])
AC_SUBST([have_nautilus_extension])
AC_SUBST([nautilus_extension_version])
AC_SUBST([have_doc_html])
AC_SUBST([have_bash_completion])

AC_CONFIG_FILES([Makefile
		 src/Makefile
		 scripts/Makefile
		 data/Makefile
                 plugins/Makefile
		 filemanagers/caja/Makefile
		 filemanagers/nautilus/Makefile
		 po/Makefile
		 po/widget-gtk/Makefile
		 include/Makefile
		 doc/doxygen.conf
		 doc/Makefile])

AC_OUTPUT

info_msg="successful

------------------------------------Programs------------------------------------
Server:                                                                      $have_server
Client:                                                                      $have_client
Configtool:                                                                  $have_configtool
Widget-gtk:                                                                  $have_widget_gtk

------------------------------------Plugins-------------------------------------
(Communication)
ip4tcp:                                                                      $have_ip4tcp
ip4tcps:                                                                     $have_openssl
ip6tcp:                                                                      $have_ip6tcp
(Event handlers)
dbus:                                                                        $have_dbus
notify:                                                                      $have_notify
"
AS_IF([test "x$have_notify" = xyes], [
       info_msg="$info_msg - libnotify version: $libnotify_version
"
])
info_msg="${info_msg}avahi:                                                                       $have_avahi

------------------------------------Scanners------------------------------------
Scanners enabled:                                                            $have_scan
avahi:                                                                       $have_scanner_avahi
----------------------------------File managers---------------------------------
Caja:                                                                        $have_caja_extension
Nautilus:                                                                    $have_nautilus_extension"

AS_IF([test "x$have_nautilus_extension" = xyes], [
       info_msg="$info_msg
 - libnautilus-extension version: $nautilus_extension_version"
])

info_msg="$info_msg
-------------------------------------Scripts------------------------------------
Bash completion                                                              $have_bash_completion
----------------------------------Documentation---------------------------------
HTML (Doxygen):                                                              $have_doc_html"

AC_MSG_NOTICE([$info_msg])
