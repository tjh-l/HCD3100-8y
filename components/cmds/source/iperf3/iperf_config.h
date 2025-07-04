/* src/iperf_config.h.in.  Generated from configure.ac by autoheader.  */


#ifndef __TR_SW__
/* Define to 1 if you have the `clock_gettime' function. */
#undef HAVE_CLOCK_GETTIME

/* Define to 1 if you have the `cpuset_setaffinity' function. */
#undef HAVE_CPUSET_SETAFFINITY

/* Have CPU affinity support. */
#undef HAVE_CPU_AFFINITY

/* Define to 1 if you have the `daemon' function. */
#undef HAVE_DAEMON

/* Define to 1 if you have the <dlfcn.h> header file. */
#undef HAVE_DLFCN_H

/* Define to 1 if you have the <endian.h> header file. */
#undef HAVE_ENDIAN_H

/* Have IPv6 flowlabel support. */
#undef HAVE_FLOWLABEL

/* Define to 1 if you have the `getline' function. */
#undef HAVE_GETLINE

/* Define to 1 if you have the <inttypes.h> header file. */
#undef HAVE_INTTYPES_H

/* Define to 1 if you have the <memory.h> header file. */
#undef HAVE_MEMORY_H

/* Define to 1 if you have the <netinet/sctp.h> header file. */
#undef HAVE_NETINET_SCTP_H

/* Define to 1 if you have the <poll.h> header file. */
#undef HAVE_POLL_H

/* Define to 1 if you have the `sched_setaffinity' function. */
#undef HAVE_SCHED_SETAFFINITY

/* Have SCTP support. */
#undef HAVE_SCTP

/* Define to 1 if you have the `sendfile' function. */
#undef HAVE_SENDFILE

/* Define to 1 if you have the `SetProcessAffinityMask' function. */
#undef HAVE_SETPROCESSAFFINITYMASK

/* Have SO_MAX_PACING_RATE sockopt. */
#undef HAVE_SO_MAX_PACING_RATE

/* OpenSSL Is Available */
#undef HAVE_SSL

/* Define to 1 if you have the <types.h> header file. */
#undef HAVE_STDINT_H

/* Define to 1 if you have the <stdlib.h> header file. */
#undef HAVE_STDLIB_H

/* Define to 1 if you have the <strings.h> header file. */
#undef HAVE_STRINGS_H

/* Define to 1 if you have the <string.h> header file. */
#undef HAVE_STRING_H

/* Define to 1 if the system has the type `struct sctp_assoc_value'. */
#undef HAVE_STRUCT_SCTP_ASSOC_VALUE

/* Define to 1 if you have the <sys/endian.h> header file. */
#undef HAVE_SYS_ENDIAN_H

/* Define to 1 if you have the <sys/socket.h> header file. */
#undef HAVE_SYS_SOCKET_H

/* Define to 1 if you have the <sys/stat.h> header file. */
#undef HAVE_SYS_STAT_H

/* Define to 1 if you have the <sys/types.h> header file. */
#undef HAVE_SYS_TYPES_H

/* Have TCP_CONGESTION sockopt. */
#undef HAVE_TCP_CONGESTION

/* Define to 1 if you have the <unistd.h> header file. */
#undef HAVE_UNISTD_H

/* Define to the sub-directory where libtool stores uninstalled libraries. */
#undef LT_OBJDIR

/* Name of package */
#undef PACKAGE

/* Define to the address where bug reports for this package should be sent. */
#undef PACKAGE_BUGREPORT

/* Define to the full name of this package. */
#undef PACKAGE_NAME

/* Define to the full name and version of this package. */
#undef PACKAGE_STRING

/* Define to the one symbol short name of this package. */
#undef PACKAGE_TARNAME

/* Define to the home page for this package. */
#undef PACKAGE_URL

/* Define to the version of this package. */
#undef PACKAGE_VERSION

/* Define to 1 if you have the ANSI C header files. */
#undef STDC_HEADERS

/* Version number of package */
#undef VERSION

/* Define to empty if `const' does not conform to ANSI C. */
#undef const
#else
/* Define to 1 if you have the `clock_gettime' function. */
#undef HAVE_CLOCK_GETTIME

/* Define to 1 if you have the `cpuset_setaffinity' function. */
#undef HAVE_CPUSET_SETAFFINITY

/* Have CPU affinity support. */
#undef HAVE_CPU_AFFINITY

/* Define to 1 if you have the `daemon' function. */
#undef HAVE_DAEMON

/* Define to 1 if you have the <dlfcn.h> header file. */
#define HAVE_DLFCN_H 1

/* Define to 1 if you have the <endian.h> header file. */
#undef HAVE_ENDIAN_H

/* Have IPv6 flowlabel support. */
#undef HAVE_FLOWLABEL

/* Define to 1 if you have the `getline' function. */
#define HAVE_GETLINE 1

/* Define to 1 if you have the <inttypes.h> header file. */
#undef HAVE_INTTYPES_H

/* Define to 1 if you have the <memory.h> header file. */
#undef HAVE_MEMORY_H

/* Define to 1 if you have the <netinet/sctp.h> header file. */
#undef HAVE_NETINET_SCTP_H

/* Define to 1 if you have the <poll.h> header file. */
#undef HAVE_POLL_H

/* Define to 1 if you have the `sched_setaffinity' function. */
#undef HAVE_SCHED_SETAFFINITY

/* Have SCTP support. */
#undef HAVE_SCTP

/* Define to 1 if you have the `sendfile' function. */
#undef HAVE_SENDFILE

/* Define to 1 if you have the `SetProcessAffinityMask' function. */
#undef HAVE_SETPROCESSAFFINITYMASK

/* Have SO_MAX_PACING_RATE sockopt. */
#undef HAVE_SO_MAX_PACING_RATE

/* OpenSSL Is Available */
#undef HAVE_SSL

/* Define to 1 if you have the <types.h> header file. */
#undef HAVE_STDINT_H

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if the system has the type `struct sctp_assoc_value'. */
#undef HAVE_STRUCT_SCTP_ASSOC_VALUE

/* Define to 1 if you have the <sys/endian.h> header file. */
#undef HAVE_SYS_ENDIAN_H

/* Define to 1 if you have the <sys/socket.h> header file. */
#undef HAVE_SYS_SOCKET_H

/* Define to 1 if you have the <sys/stat.h> header file. */
#undef HAVE_SYS_STAT_H

/* Define to 1 if you have the <sys/types.h> header file. */
#undef HAVE_SYS_TYPES_H

/* Have TCP_CONGESTION sockopt. */
#undef HAVE_TCP_CONGESTION

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to the sub-directory where libtool stores uninstalled libraries. */
#undef LT_OBJDIR

/* Name of package */
#define PACKAGE "iperf"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "https://github.com/esnet/iperf"

/* Define to the full name of this package. */
#define PACKAGE_NAME "iperf"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "iperf 3.6"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "iperf"

/* Define to the home page for this package. */
#define PACKAGE_URL "http://software.es.net/iperf/"

/* Define to the version of this package. */
#define PACKAGE_VERSION "3.6"

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Version number of package */
#define VERSION "3.6"

#endif
