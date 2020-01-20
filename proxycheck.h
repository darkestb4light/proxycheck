/*
    Purpose:
      Header file for "proxycheck".
    Developer:
      Ray Daley (https://github.com/darkestb4light)
   Note:
      - License: GNU General Public License Version 3
      | This file is part of proxycheck.
      |
      | proxycheck is free software: you can redistribute it and/or modify
      | it under the terms of the GNU General Public License as published by
      | the Free Software Foundation, either version 3 of the License, or
      | (at your option) any later version.
      |
      | proxycheck is distributed in the hope that it will be useful,
      | but WITHOUT ANY WARRANTY; without even the implied warranty of
      | MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
      | GNU General Public License for more details.
      |
      | You should have received a copy of the GNU General Public License
      - Used with:
      |__ proxycheck_main.c, proxycheck.c
*/

#ifndef PROXYCHECK_H
#if defined(__FreeBSD__) || defined(__gnu_hurd__)
#define _WITH_GETLINE
#elif defined(__GNU__) || defined(__linux__)
#define _GNU_SOURCE                             /* Don't modify */
#include <strings.h>                            /* Don't modify */
#elif defined(__APPLE__) || defined(__MACH__)
  /* intentionally empty */
#elif defined(__CYGWIN__) && !defined(_WIN32)
#define _GNU_SOURCE                             /* Don't modify */
#include <strings.h>
#include <sys/select.h>
#else
#error Target OS unsupported. Aborting.
#error If this is unexpected, submit an issue to the project:
#error https://github.com/darkestb4light/proxycheck
#endif           

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>

#ifdef AVOID_MALLOC_FREE                        /* Set if getting errors 
                                                   relating to malloc() or
                                                   free() and submit a bug 
                                                   to the project  
                                                */
#define AVOID_MALLOC_FREE           1
#else
#define AVOID_MALLOC_FREE           0
#endif

#ifdef SOCK_PROTO_IP                            /* Request socket */
#define SOCK_PROTO                  0
#else
#define SOCK_PROTO                  6
#endif

#ifndef NO_LIMIT_MAX_REDIRECTS                  /* Follow redirects "infinitely"
                                                   or set a maximum before 
                                                   giving up
                                                 */
#define MAX_REDIRECTS               3
#else
#define MAX_REDIRECTS               0
#endif


/* Debug definitions */

#ifdef DEBUG_BUILDREQ                           /* Show built requests */
#define DEBUG_BUILDREQ              1
#else
#define DEBUG_BUILDREQ              0
#endif

#ifdef DEBUG_PROCRESP                           /* Show processed responses */
#define DEBUG_PROCRESP              1
#else
#define DEBUG_PROCRESP              0
#endif

#ifdef DEBUG_READREQ                            /* Show responses received */
#define DEBUG_READREQ               1
#else
#define DEBUG_READREQ               0
#endif

#ifdef DEBUG_WRITEREQ                           /* Show requests sent */
#define DEBUG_WRITEREQ              1
#else
#define DEBUG_WRITEREQ              0
#endif

#ifdef DEBUG_FORCE_EXIT                         /* Force exit with each debug */
#define DEBUG_FORCE_EXIT            1
#else
#define DEBUG_FORCE_EXIT            0
#endif

#ifdef EXIT_CONN_WRITEREQ_ERR                   /* CONNECT request fail */
#define EXIT_CONN_WRITEREQ_ERR      1
#else
#define EXIT_CONN_WRITEREQ_ERR      0
#endif

#ifdef EXIT_CONN_READREQ_ERR                    /* CONNECT response fail */
#define EXIT_CONN_READREQ_ERR       1
#else
#define EXIT_CONN_READREQ_ERR       0
#endif

#ifdef EXIT_CONN_PROCRESP_ERR                   /* CONNECT parse fail */
#define EXIT_CONN_PROCRESP_ERR      1
#else
#define EXIT_CONN_PROCRESP_ERR      0
#endif

#ifdef EXIT_WRITEREQ_ERR                        /* STANDARD request fail */
#define EXIT_WRITEREQ_ERR           1
#else
#define EXIT_WRITEREQ_ERR           0
#endif     

#ifdef EXIT_READREQ_ERR                         /* STANDARD response fail */
#define EXIT_READREQ_ERR            1
#else
#define EXIT_READREQ_ERR            0
#endif 

#ifdef EXIT_PROCRESP_400_ERR                    /* STANDARD HTTP 400 fail */
#define EXIT_PROCRESP_400_ERR       1
#else
#define EXIT_PROCRESP_400_ERR       0
#endif 

#ifndef NO_EXIT_PROCRESP_407_ERR                /* STANDARD HTTP 407 fail */
#define EXIT_PROCRESP_407_ERR       1
#else
#define EXIT_PROCRESP_407_ERR       0
#endif
         
#ifndef NO_EXIT_PROCRESP_DEF_ERR                /* STANDARD unknown fail */
#define EXIT_PROCRESP_DEF_ERR       1
#else
#define EXIT_PROCRESP_DEF_ERR       0
#endif

#ifdef OUTPUT_REQ                               /* See request payload; 
                                                   REQ_BYTE_MAX might need 
                                                   to be modified to see 
                                                   desired results
                                                */
#define OUTPUT_REQ                  1
#else
#define OUTPUT_REQ                  0
#endif

#ifdef OUTPUT_RESP                              /* See request payload; 
                                                   RESP_SZ_MAX might need to 
                                                   be modified to see desired 
                                                   results
                                                */	
#define OUTPUT_RESP                 1
#else
#define OUTPUT_RESP                 0
#endif

#ifdef NO_CONN_FORCE_GET_DEF_VERB               /* Avoid CONNECT forcing GET
                                                   for default method;
                                                   Ignored when CONNECT is 
                                                   passed as an option
                                                */
#define NO_CONN_FORCE_GET_DEF_VERB  1
#else
#define NO_CONN_FORCE_GET_DEF_VERB  0
#endif

/* General definitions */

#define NAME                    "proxycheck"
#define VERSION                 "0.0.2"         /* Current version */
#define VER_BUF                 15              /* Version buffer */      
#define NAME_SZ                 11              /* Should match NAME len */
#define MAX_ARGS                17              /* Total args/opts */
#define REQ_ARG_OFFSET          13              /* Offset for required args */
#define DAEMON_DIR_DEF          "/"             /* Default path; If running
                                                   native on Windows, this 
                                                   path will be:
                                                   <drive>:\Users\<user>\ 
                                                */
#define DAEMON_SLEEP_MIN        1               /* Minimum seconds to sleep */
#define DAEMON_SLEEP_MAX        4294967295      /* Maximum seconds to sleep */
#define DAEMON_SLEEP_DEF        3600            /* Default seconds to sleep */
#define DAEMON_LOG_DEF          NAME ".log"     /* Default log file */
#define REQ_DEF_VERB            "GET"           /* Default method/verb */
#define FILE_BUF                2048            /* File buffer */
#define LOG_LEVEL_MIN           1               /* Minimum verbosity level */
#define LOG_LEVEL_MAX           3               /* Maximum verbosity level */
#define REQ_DEF_SCHEME          "http://"       /* Default scheme */
#define LOG_IONBF               1               /* Unbuffered (default) */
#define LOG_IOLBF               2               /* Line buffered */
#define LOG_IOFBF               3               /* Fully buffered */
#define LOG_STREAM_STDOUT       stdout          /* STDOUT log stream */
#define LOG_STREAM_STDERR       stderr          /* STDERR log stream */		
#define REQ_BYTE_MAX            100000000       /* Request buffer; This can 
                                                   be adjusted based on how 
                                                   large to allocate for the 
                                                   request buffer; Note: 
                                                   setting too low can hinder 
                                                   the amount of requests that 
                                                   can be processed, while 
                                                   setting too high may affect 
                                                   available memory or possibly 
                                                   performance; Use caution. 
                                                   Default is 10MB; 
                                                   1MB == 1000000 
                                                */
#define REQ_TOKENS              " ,;|"          /* Request delimiters */
#define	TIME_SZ                 26              /* Timestamp buffer; 
                                                   Don't modify 
                                                */
#define MAX_VERBS               4               /* HTTP methods; Don't modify */
#define MAX_SCHEMES             2               /* HTTP schemes; Don't modify */
#define MAX_REDIRECT_TYPES      4               /* HTTP redirects; 
                                                   Don't modify 
                                                */
#define HOST_SZ                 256             /* Host buffer; Don't modify */
#define PORT_MIN                0               /* Port minimum */
#define PORT_MAX                65535           /* Port maximum */
#define WRITE_TM_MIN            0               /* Min seconds - don't modify */
#define READ_TM_MIN             0               /* Min seconds - don't modify */ 
#define WRITE_TM_DEF            10              /* Request seconds (default) */
#define READ_TM_DEF             10              /* Response seconds (default) */
#define WRITE_TM_MAX            120             /* Max seconds - don't modify */
#define READ_TM_MAX             120             /* Max seconds - don't modify */

/* Request definitions */

#define REQ_VERB_SZ             8               /* Request method buffer */
#define REQ_SCHEME_SZ           9               /* Request scheme buffer */
#define REQ_PORT_SZ             6               /* Request port buffer */
#define REQ_HTTP_PORT_DEF       80              /* Request def. HTTP port */
#define REQ_HTTPS_PORT_DEF      443             /* Request def. HTTPS port */
#define REQ_PATH_DEF            "/"             /* Request URI default */
#define REQ_PATH_SZ             (2080+HOST_SZ)  /* URL buffer per request */
#ifdef REQ_HTTP_VER1_0
#define REQ_HTTP_VER_DEF        "HTTP/1.0"      /* Request version 1.0 */
#else
#define REQ_HTTP_VER_DEF        "HTTP/1.1"      /* Request version 1.1 */
#endif
#define REQ_HTTP_VER_SZ         9               /* HTTP version buffer */
#define REQ_ACCEPT_HDR_DEF      "*/*"           /* Accept header default */

                                                /* Request header buffers */
#define REQ_HOST_HDR_SZ         (6+HOST_SZ+4)   /* |__ Host */
#define REQ_ACCEPT_HDR_SZ       16              /* |__ Accept */
#define REQ_USER_HDR_SZ         (12+NAME_SZ+4)  /* |__ User-Agent */
#define REQ_CONN_HDR_SZ         (18+HOST_SZ+4)  /* |__ Host */

                                                /* Request CONNECT buffer */
#define REQ_CONN_SZ             (REQ_VERB_SZ+REQ_SCHEME_SZ+HOST_SZ+\
                                REQ_PATH_SZ+REQ_HTTP_VER_SZ)
                                
                                                /* Request STANDARD buffer */
#define REQ_SZ                  (REQ_CONN_SZ+REQ_HOST_HDR_SZ+\
                                REQ_CONN_HDR_SZ)

/* Response definitions */

#define RESP_MSG_403            "Forbidden/Proxy denial"
#define RESP_MSG_407            "Proxy Authentication Required"
#define RESP_LOC_BUF            HOST_SZ         /* Response Location buffer */
#define RESP_PROC_SZ            32              /* Response field buffer */	
#define RESP_SZ_MIN             10              /* Minimum response buffer */	
#define RESP_SZ_DEF             512             /* Default response buffer */
#define RESP_SZ_MAX             65535           /* Maximum response buffer */

struct Sockhost{                                /* Hold client/server info */
    char c_name[HOST_SZ];
    char s_name[HOST_SZ];
    char s_ip[HOST_SZ];	
    int s_port;
};

struct requestopt {                             /* Request options */
    char path[REQ_PATH_SZ];
    char scheme[REQ_SCHEME_SZ];
    char tmpurlport[REQ_PORT_SZ];
    char ver[REQ_HTTP_VER_SZ];
    char verb[REQ_VERB_SZ];
    char *tmpurl;
    const char *hdr;
    int port;
    unsigned int do_accept_hdr;
    unsigned int do_conn_hdr;
    unsigned int do_host_hdr;
    unsigned int do_user_hdr;
    unsigned int opt_scheme;
};

struct request {                                /* Request object(s) */
    char accept_hdr[REQ_ACCEPT_HDR_SZ];
    char conn_hdr[REQ_CONN_HDR_SZ];
    char connreq[REQ_CONN_SZ];
    char connverb[REQ_VERB_SZ];
    char domain[HOST_SZ];
    char host_hdr[REQ_HOST_HDR_SZ];
    char httpreq[REQ_SZ];
    char path[REQ_PATH_SZ];
    char port[REQ_PORT_SZ];
    char scheme[REQ_SCHEME_SZ];
    char user_hdr[REQ_USER_HDR_SZ];
    char ver[REQ_HTTP_VER_SZ];
    char verb[REQ_VERB_SZ];
    unsigned int valid_request;
    struct request *next;
};

/* Prototypes */
  
void buildreq(struct request *, struct requestopt *);
int daemonize(pid_t *, char *);
int getaddr(const char *, char *, const int);
int getlocation(char *, unsigned int, char *, unsigned int, char *);
int getreqcount(struct request *);
void getreqline(char *, size_t, FILE *, char *, long);
int gettime(char *);
int initsock(int, int, int, struct sockaddr_in, socklen_t);
void logevents(char *, int, FILE *);
long optdaemon(const char **, unsigned int *);
int *optfollow(const char **, unsigned int *, int *);
void optheader(const char **, unsigned int *, struct requestopt *);
int optinput(const char **, unsigned int *);
void optlog(const char **, unsigned int *, char *);
int optoutput(const char **, unsigned int *);
int optport(const char **, unsigned int *);
int optproxyport(const char **, unsigned int *);
int optproxyserver(const char **, unsigned int *, char *);
unsigned int optrespbuffer(const char **, unsigned int *);
void optreq(const char **, unsigned int *, char *, long);
void optreqfile(const char **, unsigned int *, char *, long);
void optscheme(const char **, unsigned int *, char *);
void optverb(const char **, unsigned int *, char *);
int optverbosity(const char **, unsigned int *);
void optversion(char *);
int procresp(const char *, int);
ssize_t readreq(int, char *, size_t, unsigned int, unsigned int);
void stripline(char *);
void usage(void);
size_t writereq(int, char *, size_t, unsigned int);

#endif
