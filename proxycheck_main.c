/*
    Purpose:
        C file for "proxycheck".
      
        This is a proof-of-concept for testing various domains/URLs via 
        a proxy. The idea is to provide a way to audit if a domain/URL is 
        accessible, given a request being sent. This tool is designed 
        to provide a way to (hopefully) quickly get results, knowing 
        there could be a huge variety of domains/URLs and spot checking 
        may not be a good way to proceed.

        This tool is not designed to care about a successful request 
        versus a failed request as it relates to an HTTP status result. 
        It is up to the user to determine what success versus failure 
        is from an audit perspective. After all, it can vary, on the 
        use case:

        Example: You may want audit whether a list of domains/URLs are 
        reachable. If this results in the contrary, it could mean there 
        is an issue that needs to be investigated (network, proxy, logical 
        error in policy, etc.). In this case anything other than a success 
        is most likely not a good thing.

        Example: You may want audit whether a list of domains/URLs are NOT 
        reachable. In this example, if a request is successful, it most 
        likely means an issue with the defined proxy policy ACLs. Again, 
        most likely not a good thing.

        The user can take the results and determine the best course of 
        action, given their use case(s). 
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
        | along with proxycheck. If not, see <https://www.gnu.org/licenses/>.
        - Used with:
        |__ proxycheck.c, proxycheck.h
        - Should run on:
        |__ Linux (such as CentOS)
        |__ Unix (such as FreeBSD or variants like OS X)
        |__ Windows (requires cygwin1.dll for native use) or Cygwin terminal
        - Compile:
        |__ $ gcc -o proxycheck proxycheck_main.c proxycheck.c
        |__ OR
        |__ $ gcc -std=c99 -o proxycheck proxycheck_main.c proxycheck.c
*/

#include "proxycheck.h"

int main(int argc, const char **argv)
{
    char            c, *opt[MAX_ARGS][3] = {{"-B", "0", "0"}, {"-D", "0", "0"},
                                            {"-F", "0", "0"}, {"-H", "0", "0"}, 
                                            {"-I", "0", "0"}, {"-L", "0", "0"}, 
                                            {"-M", "0", "0"}, {"-O", "0", "0"}, 
                                            {"-P", "0", "0"}, {"-R", "0", "0"},
                                            {"-S", "0", "0"}, {"-V", "0", "0"},
                                            {"-h", "0", "0"}, {"-p", "0", "1"}, 
                                            {"-r", "0", "1"}, {"-s", "0", "1"}, 
                                            {"-v", "0", "0"}},
                    version[VER_BUF], lfile[FILE_BUF], *request = NULL, 
                    *cp, ts[TIME_SZ], *response = NULL, resploc[RESP_LOC_BUF];
    int             r_tmout = READ_TM_DEF, w_tmout = WRITE_TM_DEF, 
                    loglevel = 0, res, lmode = LOG_IONBF, redirstream = 0,
                    sd, status, dorespbuf = 0, *redirect = NULL, 
                    redirect_types = 0;
    unsigned int    optvalid = 0, d_sleep = 0, respbuf = RESP_SZ_DEF, 
                    redirect_type_found = 0, thisredirect = 0;
    pid_t           pid;
    size_t          reqbytes;
    ssize_t         respbytes;
    FILE            *lstream = LOG_STREAM_STDOUT;
    struct          requestopt reqopt;
    struct          request *req = NULL, *thisreq = NULL;
    struct          Sockhost skthost;
    struct          sockaddr_in serveraddr;
    
    reqopt.hdr = NULL;
    reqopt.do_host_hdr = 0;
    reqopt.do_user_hdr = 0;
    reqopt.do_accept_hdr = 0;
    reqopt.do_conn_hdr = 0;
    reqopt.port = -1;
    reqopt.tmpurl = NULL;
    strncpy(reqopt.tmpurlport, "-1", REQ_PORT_SZ);
    strncpy(reqopt.verb, REQ_DEF_VERB, REQ_VERB_SZ);
    strncpy(reqopt.path, REQ_PATH_DEF, REQ_PATH_SZ);
    strncpy(reqopt.scheme, REQ_DEF_SCHEME, REQ_SCHEME_SZ);
    strncpy(reqopt.ver, REQ_HTTP_VER_DEF, REQ_HTTP_VER_SZ);
    
    /* Process args/opts */
    if(argc == 1)
        usage();
    
    for(unsigned int i = 1; i != argc; ++i)
    {
        for(unsigned int j = 0; j != MAX_ARGS; ++j)
        {
            if(strncmp(argv[i], *opt[j], 3) == 0){
                if(strncmp("1", opt[j][1], 2) != 0) 
                    opt[j][1] = "1"; /* found flag */
                c = *(*(opt[j])+1);
                switch(c){
                case 'B':
                    respbuf = optrespbuffer(argv, &i);
                    dorespbuf = 1;
                    break;
                case 'D':
                    d_sleep = optdaemon(argv, &i);
                    break;
                case 'F':
                    redirect = optfollow(argv, &i, &redirect_types);
                    break;
                case 'H':
                    optheader(argv, &i, &reqopt);
                    break;
                case 'I':
                    r_tmout = optinput(argv, &i);
                    break;
                case 'L':
                    optlog(argv, &i, lfile);
                    redirstream = 1;
                    break;
                case 'M':
                    optverb(argv, &i, reqopt.verb);
                    break;
                case 'O':
                    w_tmout = optoutput(argv, &i);
                    break;
                case 'P':
                    reqopt.port = optport(argv, &i);
                    break;
                case 'R':
                    if(strncmp("1", opt[14][1], 1) == 0) 
                        break; /* -r already exists */
                    if((request = (char *) malloc(REQ_BYTE_MAX)) == NULL){
                        fprintf(stderr, "%s: ERR: %s: %s: %s\n", NAME, "request", 
                                "malloc", strerror(errno));
                        exit(1);
                    }
                    optreqfile(argv, &i, request, REQ_BYTE_MAX);
                    reqopt.tmpurl = strtok_r(request, REQ_TOKENS, &cp);
                    req = (struct request *) malloc(sizeof(struct request));
                    if(req == NULL){
                        fprintf(stderr, "%s: ERR: %s: %s: %s: %s\n", NAME, "request",
                                "object allocation", "malloc", strerror(errno));
                        exit(1);
                    }        
                    opt[14][1] = "1"; /* override -r */
                    break;
                case 'S':
                    optscheme(argv, &i, reqopt.scheme);
                    break;
                case 'V':
                    loglevel = optverbosity(argv, &i);
                    break;
                case 'h':
                    fprintf(stderr, "%s: Help menu\n\n", NAME);
                    usage();
                case 'p':
                    skthost.s_port = optproxyport(argv, &i);
                    if(skthost.s_port == -1){
                        opt[j][1] = "0";
                        break; 
                    }
                    break;
                case 'r':
                    if(strncmp("1", opt[9][1], 1) == 0) 
                        break; /* -R already exists */       
                    if((request = (char *) malloc(REQ_BYTE_MAX)) == NULL){
                        fprintf(stderr, "%s: ERR: %s: %s: %s\n", NAME, "request", 
                                "malloc", strerror(errno));
                        exit(1);
                    }
                    optreq(argv, &i, request, REQ_BYTE_MAX);
                    reqopt.tmpurl = strtok_r(request, REQ_TOKENS, &cp);
                    req = (struct request *) malloc(sizeof(struct request));
                    if(req == NULL){
                        fprintf(stderr, "%s: ERR: %s: %s: %s: %s\n", NAME, "request",
                                "object allocation", "malloc", strerror(errno));
                        exit(1);
                    }
                    opt[9][1] = "1"; /* override -R */
                    break;
                case 's':
                    if(optproxyserver(argv, &i, skthost.s_name) == -1){
                        opt[j][1] = "0";
                        break; 
                    }
                    if((res = getaddr(skthost.s_name, skthost.s_ip, HOST_SZ)) != 0){ 
                        fprintf(stderr, "%s: ERR: %s: %s: %s\n", NAME, "server", 
                                "getaddrinfo", gai_strerror(res));
                        exit(1);
                    }
                    if(gethostname(skthost.c_name, HOST_SZ) != 0){
                        fprintf(stderr, "%s: ERR: %s: %s: %s\n", NAME, "client", 
                            "gethostname", strerror(errno));
                        exit(1);
                    }
                    break;
                case 'v':
                    optversion(version);
                    fprintf(stderr, "%s: %s: %s\n", NAME, "version", version);
                    exit(1);
                default: /* should never get here */
                    fprintf(stderr, "%s: ERR: %s: %s (%c)\n", NAME, 
                            "argument/option found but unaccounted for", opt[j][0], c);
                    exit(1);
                }
                optvalid = 1;
            }
        }
        if(!optvalid){
            fprintf(stderr, "%s: ERR: %s: %s\n", NAME, "invalid argument or option", 
                    argv[i]);
            usage();
        }
        optvalid = 0;
    }
    
    /* Further validate required args */
    
    for(unsigned int j = REQ_ARG_OFFSET; j != MAX_ARGS; ++j)
    {
        if(strncmp("1", opt[j][2], 1) == 0){
            if(strncmp("1", opt[j][1], 1) != 0){
                fprintf(stderr, "%s: ERR: %s: %s\n", NAME, 
                        "argument (or parameter to it) required", *opt[j]);
                usage();
            }
        }
    }
        
    /* Potentially become a daemon; Redirect stdout and stderr streams */

    if(d_sleep){
        if(redirstream)
            logevents(lfile, lmode, lstream);
        else
            logevents(DAEMON_LOG_DEF, lmode, lstream);
        daemonize(&pid, ts);
    }else{
        if(redirstream)
            logevents(lfile, lmode, lstream);
    }
    
    /* Able to start & build server address */
    
    gettime(ts);
    printf("%s: %s: %s\n", ts, NAME, "validation complete - starting");
    
    memset((char *) &serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = inet_addr(skthost.s_ip);
    serveraddr.sin_port = htons(skthost.s_port);
    
    gettime(ts);
    printf("%s: %s: %s [%s] > %s [%s/%d]\n", ts, NAME, "client", 
            skthost.c_name, "server", skthost.s_name, skthost.s_port);
    printf("%s: %s: socket proto: %s\n", ts, NAME, SOCK_PROTO ? "TCP":"IP");

    if((response = (char *) malloc(sizeof(char) * respbuf)) == NULL){
        gettime(ts);
        fprintf(stderr, "%s: %s: ERR: %s: %s: %s\n", ts, NAME, "response", 
                "malloc", strerror(errno));
        exit(1);
    }

    /* Request headers */ 

    gettime(ts);
    if(reqopt.do_accept_hdr)
        printf("%s: %s: include header=Accept\n", ts, NAME);
    
    if(reqopt.do_conn_hdr)
        printf("%s: %s: include header=Connection\n", ts, NAME);
    
    if(reqopt.do_host_hdr)
        printf("%s: %s: include header=Host\n", ts, NAME);
        
    if(reqopt.do_user_hdr)
        printf("%s: %s: include header=User-Agent\n", ts, NAME);

    /* Build request(s) */

    gettime(ts);
    printf("%s: %s: %s\n", ts, NAME, "building request(s)");

    buildreq(req, &reqopt);
    req->next = NULL;

    while((reqopt.tmpurl = strtok_r(NULL, REQ_TOKENS, &cp)) != NULL)
    {
        thisreq = req;
        while(thisreq->next != NULL) 
            thisreq = thisreq->next;
        thisreq->next = (struct request *) malloc(sizeof(struct request));
        if(thisreq->next == NULL){
            gettime(ts);
            fprintf(stderr, "%s: %s: ERR: %s: %s: %s\n", ts, NAME, "request", 
                    "malloc", strerror(errno));
            exit(1);
        }
        buildreq(thisreq->next, &reqopt);
        thisreq->next->next = NULL;
    }

    gettime(ts);
    printf("%s: %s: requests built: %d\n", ts, NAME, getreqcount(req));

    /* Process request(s) */
    
    gettime(ts);
    printf("%s: %s: %s: %s\n", ts, NAME, "request", "processing");
    
    thisreq = req;
    
    for(;;)
    {
        if(! thisreq->valid_request){
            gettime(ts);
            fprintf(stderr, "%s: %s: %s: %s: %s: %s%s: %s%s: %s%s: %s%s: %s%s: %s%s\n", 
                ts, NAME, "WARN", "Invalid request - skipping", "action=buildreq", 
                "verb=", thisreq->verb, "scheme=", thisreq->scheme, "domain=", 
                thisreq->domain, "port=", thisreq->port, "path=", thisreq->path, 
                "ver=", thisreq->ver);
            thisreq = thisreq->next;
            if(thisreq == NULL)
                break;
            else
                continue;
        }
        sd = initsock(serveraddr.sin_family, SOCK_STREAM, SOCK_PROTO, serveraddr, 
                        sizeof(serveraddr));
        memset(response, 0, respbuf);
        /* 
            CONNECT processing 
        */
        if(thisreq->connverb[0] != 0){
            reqbytes = writereq(sd, thisreq->connreq, strnlen(thisreq->connreq, 
                                REQ_CONN_SZ), w_tmout);
            gettime(ts);
            if(reqbytes == -1){
#if EXIT_CONN_WRITEREQ_ERR
                fprintf(stderr, "%s: %s: %s: %s: %s%s: %s%s: %s%d: ", ts, NAME, 
                        "ERR", "CONNECT", "write=", strerror(errno), "dst=", 
                        skthost.s_name, "dport=", skthost.s_port);
                fprintf(stderr, "%s: %s%s: %s%s: %s%s: %s%s: %s%s: %s%s\n", 
                        "action=writereq", "connverb=", thisreq->connverb, "scheme=", 
                        thisreq->scheme, "domain=", thisreq->domain, "port=", 
                        thisreq->port, "path=", thisreq->path, "ver=", thisreq->ver);
#if OUTPUT_REQ
                fprintf(stderr, "%s: %s: %s: %s%s: %s%d: ", ts, NAME, "CONNECT", 
                        "dst=", skthost.s_name, "dport=", skthost.s_port);
                fprintf(stderr, "%s:\n%s\n", "request", thisreq->connreq);
#endif
                exit(1);
#else
                fprintf(stderr, "%s: %s: %s: %s: %s%s: %s%s: %s%d: ", ts, NAME, 
                        "WARN", "CONNECT", "write=", strerror(errno), "dst=",
                        skthost.s_name, "dport=", skthost.s_port);
                fprintf(stderr, "%s: %s%s: %s%s: %s%s: %s%s: %s%s: %s%s\n", 
                        "action=writereq", "connverb=", thisreq->connverb, "scheme=", 
                        thisreq->scheme, "domain=", thisreq->domain, "port=", 
                        thisreq->port, "path=", thisreq->path, "ver=", thisreq->ver);
#if OUTPUT_REQ

                fprintf(stderr, "%s: %s: %s: %s%s: %s%d: ", ts, NAME, "CONNECT", 
                        "dst=", skthost.s_name, "dport=", skthost.s_port);
                fprintf(stderr, "%s:\n%s\n", "request", thisreq->connreq);
                
#endif
                thisreq = thisreq->next;
                close(sd);
                if(thisreq == NULL)
                    break;
                else
                    continue;
#endif
            }
            if(loglevel == LOG_LEVEL_MIN || loglevel == LOG_LEVEL_MAX){
                fprintf(stderr, "%s: %s: %s: %s%s: %s%d: ", ts, NAME, "CONNECT",
                        "dst=", skthost.s_name, "dport=", skthost.s_port);
                fprintf(stderr, "%s:\n%s\n", "request", thisreq->connreq);
            }
            respbytes = readreq(sd, response, respbuf, r_tmout, dorespbuf);
            gettime(ts);
            if(respbytes == -1){
#if EXIT_CONN_READREQ_ERR
                fprintf(stderr, "%s: %s: %s: %s: %s%s: %s%s: %s%d: ", ts, NAME, 
                        "ERR", "CONNECT", "read=", strerror(errno), "dst=",
                        skthost.s_name, "dport=", skthost.s_port);
                fprintf(stderr, "%s: %s%s: %s%s: %s%s: %s%s: %s%s: %s%s\n", 
                        "action=readreq", "connverb=", thisreq->connverb, "scheme=", 
                        thisreq->scheme, "domain=", thisreq->domain, "port=", 
                        thisreq->port, "path=", thisreq->path, "ver=", thisreq->ver);
#if OUTPUT_RESP
                fprintf(stderr, "%s: %s: %s: %s%s: %s%d: ", ts, NAME, "CONNECT", 
                        "dst=", skthost.s_name, "dport=", skthost.s_port);
                fprintf(stderr, "%s:\n%s\n", "response", response);
#endif
                exit(1);
#else
                fprintf(stderr, "%s: %s: %s: %s: %s%s: %s%s: %s%d: ", ts, NAME, 
                        "WARN", "CONNECT", "read=", strerror(errno), "dst=",
                        skthost.s_name, "dport=", skthost.s_port);
                fprintf(stderr, "%s: %s%s: %s%s: %s%s: %s%s: %s%s: %s%s\n", 
                        "action=readreq", "connverb=", thisreq->connverb, "scheme=", 
                        thisreq->scheme, "domain=", thisreq->domain, "port=", 
                        thisreq->port, "path=", thisreq->path, "ver=", thisreq->ver);
#if OUTPUT_RESP
                fprintf(stderr, "%s: %s: %s: %s%s: %s%d: ", ts, NAME, "CONNECT",
                        "dst=", skthost.s_name, "dport=", skthost.s_port);
                fprintf(stderr, "%s:\n%s\n", "response", response);
 #endif
                thisreq = thisreq->next;
                close(sd);
                if(thisreq == NULL)
                    break;
                else
                    continue;
#endif
            }
            if(loglevel == 2 || loglevel == LOG_LEVEL_MAX){
                fprintf(stderr, "%s: %s: %s: %s%s: %s%d: ", ts, NAME, "CONNECT", 
                        "dst=", skthost.s_name, "dport=", skthost.s_port);
                fprintf(stderr, "%s:\n%s\n", "response", response);
            }
            status = procresp(response, 1);
            gettime(ts);
            if(status != 200){
#if EXIT_CONN_PROCRESP_ERR
                fprintf(stderr, "%s: %s: %s: %s: %s%s: %s%d: ", ts, NAME, "ERR", 
                        "CONNECT", "dst=", skthost.s_name, "dport=", skthost.s_port);
                fprintf(stderr, "%s: %s%s: %s%s: %s%s: %s%s: %s%s: %s%s: %s:%d\n", 
                        "action=procresp", "connverb=", thisreq->connverb, 
                        "scheme=", thisreq->scheme, "domain=", thisreq->domain, 
                        "port=", thisreq->port, "path=", thisreq->path, "ver=", 
                        thisreq->ver, "status", status);
                exit(1);
#else
                fprintf(stderr, "%s: %s: %s: %s: %s%s: %s%d: ", ts, NAME, "WARN", 
                        "CONNECT", "dst=", skthost.s_name, "dport=", skthost.s_port);
                fprintf(stderr, "%s: %s%s: %s%s: %s%s: %s%s: %s%s: %s%s: %s:%d\n", 
                        "action=procresp", "connverb=", thisreq->connverb, 
                        "scheme=", thisreq->scheme, "domain=", thisreq->domain, 
                        "port=", thisreq->port, "path=", thisreq->path, "ver=", 
                        thisreq->ver, "status", status);
                thisreq = thisreq->next;
                close(sd);
                if(thisreq == NULL)
                    break;
                else
                    continue;
#endif
            }else{
                printf("%s: %s: %s: %s%s: %s%d: ", ts, NAME, "CONNECT", "dst=",       
                    skthost.s_name, "dport=", skthost.s_port);
                printf("%s: %s%s: %s%s: %s%s: %s%s: %s%s: %s%s: %s:%d\n", "action=procresp",
                    "connverb=", thisreq->connverb, "scheme=", thisreq->scheme, "domain=", 
                    thisreq->domain, "port=", thisreq->port, "path=", thisreq->path, "ver=",
                    thisreq->ver, "status", status);
            }
            memset(response, 0, respbuf);
        }
        /*
            STANDARD processing 
        */
        reqbytes = writereq(sd, thisreq->httpreq, strnlen(thisreq->httpreq, 
                            REQ_SZ), w_tmout);
        gettime(ts);
        if(reqbytes == -1){
#if EXIT_WRITEREQ_ERR
            fprintf(stderr, "%s: %s: %s: %s: %s%s: %s%s: %s%d: ", ts, NAME, "ERR",   
                    "STANDARD", "write=", strerror(errno), "dst=", skthost.s_name, 
                    "dport=", skthost.s_port);
            fprintf(stderr, "%s: %s%s: %s%s: %s%s: %s%s: %s%s: %s%s\n", "action=writereq",     
                    "verb=", thisreq->verb, "scheme=", thisreq->scheme, "domain=", 
                    thisreq->domain, "port=", thisreq->port, "path=", thisreq->path, 
                    "ver=", thisreq->ver);
#if OUTPUT_REQ
            fprintf(stderr, "%s: %s: %s: %s%s: %s%d: ", ts, NAME, "STANDARD", 
                    "dst=", skthost.s_name, "dport=", skthost.s_port);
            fprintf(stderr, "%s:\n%s\n", "request", thisreq->httpreq);
#endif
            exit(1);
#else
            fprintf(stderr, "%s: %s: %s: %s: %s%s: %s%s: %s%d: ", ts, NAME, "WARN", 
                    "STANDARD", "write=", strerror(errno), "dst=", skthost.s_name, 
                    "dport=", skthost.s_port);
            fprintf(stderr, "%s: %s%s: %s%s: %s%s: %s%s: %s%s: %s%s\n", "action=writereq",     
                    "verb=", thisreq->verb, "scheme=", thisreq->scheme, "domain=", 
                    thisreq->domain, "port=", thisreq->port, "path=", thisreq->path, 
                    "ver=", thisreq->ver);
#if OUTPUT_REQ
            fprintf(stderr, "%s: %s: %s: %s%s: %s%d: ", ts, NAME, "STANDARD", "dst=", 
                    skthost.s_name, "dport=", skthost.s_port);
            fprintf(stderr, "%s:\n%s\n", "request", thisreq->httpreq);
#endif
            thisreq = thisreq->next;
            close(sd);
            if(thisreq == NULL)
                break;
            else
                continue;	
#endif
        }
        if(loglevel == LOG_LEVEL_MIN || loglevel == LOG_LEVEL_MAX){
            fprintf(stderr, "%s: %s: %s: %s%s: %s%d: ", ts, NAME, "STANDARD", 
                    "dst=", skthost.s_name, "dport=", skthost.s_port);
            fprintf(stderr, "%s:\n%s\n", "request", thisreq->httpreq);
        }
        respbytes = readreq(sd, response, respbuf, r_tmout, dorespbuf);
        gettime(ts);
        if(respbytes == -1){
#if EXIT_READREQ_ERR
            fprintf(stderr, "%s: %s: %s: %s: %s%s: %s%s: %s%d: ", ts, NAME, 
                    "ERR", "STANDARD", "read=", strerror(errno), "dst=", 
                    skthost.s_name, "dport=", skthost.s_port);
            fprintf(stderr, "%s: %s%s: %s%s %s%s: %s%s: %s%s: %s%s\n", "action=readreq", 
                    "verb=", thisreq->verb, "scheme=", thisreq->scheme, "domain=", 
                    thisreq->domain, "port=", thisreq->port, "path=", thisreq->path, 
                    "ver=", thisreq->ver);
#if OUTPUT_RESP
            fprintf(stderr, "%s: %s: %s: %s%s: %s%d: ", ts, NAME, "STANDARD", 
                    "dst=", skthost.s_name, "dport=", skthost.s_port);
            fprintf(stderr, "%s:\n%s\n", "response", response);
#endif
            exit(1);
#else
            fprintf(stderr, "%s: %s: %s: %s: %s%s: %s%s: %s%d: ", ts, NAME, 
                    "WARN", "STANDARD", "read=", strerror(errno), "dst=",     
                    skthost.s_name, "dport=", skthost.s_port);
            fprintf(stderr, "%s: %s%s: %s%s %s%s: %s%s: %s%s: %s%s\n", "action=readreq", 
                    "verb=", thisreq->verb, "scheme=", thisreq->scheme, "domain=", 
                    thisreq->domain, "port=", thisreq->port, "path=", thisreq->path, 
                    "ver=", thisreq->ver);
#if OUTPUT_RESP
            fprintf(stderr, "%s: %s: %s: %s%s: %s%d: ", ts, NAME, "STANDARD", 
                    "dst=", skthost.s_name, "dport=", skthost.s_port);
            fprintf(stderr, "%s:\n%s\n", "response", response);
#endif
            thisreq = thisreq->next;
            close(sd);
            if(thisreq == NULL)
                break;
            else
                continue;
#endif
        }
        if(loglevel == 2 || loglevel == LOG_LEVEL_MAX){
            fprintf(stderr, "%s: %s: %s: %s%s: %s%d: ", ts, NAME, "STANDARD", 
                    "dst=", skthost.s_name, "dport=", skthost.s_port);
            fprintf(stderr, "%s:\n%s\n", "response", response);
        }
        status = procresp(response, 0);
        gettime(ts);
        switch(status){
        case 100 ... 199:
        case 200 ... 299:
            printf("%s: %s: %s: %s%s: %s%d: ", ts, NAME, "STANDARD", "dst=", 
                    skthost.s_name, "dport=", skthost.s_port);
            printf("%s: %s%s: %s%s: %s%s: %s%s: %s%s: %s%s: %s:%d\n", "action=procresp", 
                    "verb=", thisreq->verb, "scheme=", thisreq->scheme, "domain=", 
                    thisreq->domain, "port=", thisreq->port, "path=", thisreq->path, 
                    "ver=", thisreq->ver, "status", status);
            break;
        case 300 ... 399:
            printf("%s: %s: %s: %s%s: %s%d: ", ts, NAME, "STANDARD", "dst=", 
                    skthost.s_name, "dport=", skthost.s_port);
            printf("%s: %s%s: %s%s: %s%s: %s%s: %s%s: %s%s: %s:%d\n", "action=procresp", 
                    "verb=", thisreq->verb, "scheme=", thisreq->scheme, "domain=", 
                    thisreq->domain, "port=", thisreq->port, "path=", thisreq->path, 
                    "ver=", thisreq->ver, "status", status);
            if(redirect_types){
                for(int i = 0; i != redirect_types; ++i)
                {
                    if(status == *(redirect+i)){
                        redirect_type_found = 1;
                        break;
                    }
                }
                if(redirect_type_found){
                    printf("%s: %s: %s: %s%s: %s%d: ", ts, NAME, "STANDARD", "dst=",
                            skthost.s_name, "dport=", skthost.s_port);
                    if(getlocation(response, respbuf, resploc, RESP_LOC_BUF, thisreq->domain)){
                        printf("%s: %s%s: %s%d\n", "action=getlocation", "location=", 
                                resploc, "attempting to follow previous ", status);
                        if(MAX_REDIRECTS){
                            if(thisredirect < MAX_REDIRECTS){
                                thisredirect++;
                                printf("%s: %s: %s: %s%s: %s%d: ", ts, NAME, "STANDARD", "dst=",
                                        skthost.s_name, "dport=", skthost.s_port);
                                printf("%s: %s%d/%d\n", "action=following redirect", 
                                        "redirect count=", thisredirect, MAX_REDIRECTS);
                            }else{
                                printf("%s: %s: %s: %s%s: %s%d: ", ts, NAME, "STANDARD", "dst=",
                                        skthost.s_name, "dport=", skthost.s_port);
                                printf("%s: %s%d/%d\n", "action=redirects exceeded - skipping", 
                                        "redirect count=", thisredirect, MAX_REDIRECTS);
                                break;
                            }
                        }
                        reqopt.tmpurl = resploc;
                        buildreq(thisreq, &reqopt);
                        close(sd);
                        continue;
                    }else{
                        printf("%s: %s: %s\n", "action=getlocation", "location=None", 
                                "unable to follow");
                    }
                    redirect_type_found = 0;
                }
            }
            break;
        case 400:
#if EXIT_PROCRESP_400_ERR
            fprintf(stderr, "%s: %s: %s: %s: %s%s: %s%d: ", ts, NAME, "ERR", 
                    "STANDARD", "dst=", skthost.s_name, "dport=", skthost.s_port);
            fprintf(stderr, "%s: %s%s: %s%s: %s%s: %s%s: %s%s: %s%s: %s:%d\n", "action=procresp", 
                    "verb=", thisreq->verb, "scheme=", thisreq->scheme, "domain=", 
                    thisreq->domain, "port=", thisreq->port, "path=", thisreq->path, 
                    "ver=", thisreq->ver, "status", status);
            exit(1);
#else
            printf("%s: %s: %s: %s: %s%s: %s%d: ", ts, NAME, "WARN", 		
                    "STANDARD", "dst=", skthost.s_name, "dport=", skthost.s_port);
            printf("%s: %s%s: %s%s: %s%s: %s%s: %s%s: %s%s: %s:%d\n", "action=procresp", 
                    "verb=", thisreq->verb, "scheme=", thisreq->scheme, "domain=", 
                    thisreq->domain, "port=", thisreq->port, "path=", thisreq->path, 
                    "ver=", thisreq->ver, "status", status);
            break;
#endif
        case 401 ... 402:
            printf("%s: %s: %s: %s: %s%s: %s%d: ", ts, NAME, "WARN", 
                    "STANDARD", "dst=", skthost.s_name, "dport=", skthost.s_port);
            printf("%s: %s%s: %s%s: %s%s: %s%s: %s%s: %s%s: %s:%d\n", "action=procresp", 
                    "verb=", thisreq->verb, "scheme=", thisreq->scheme, "domain=", 
                    thisreq->domain, "port=", thisreq->port, "path=", thisreq->path, 
                    "ver=", thisreq->ver, "status", status);
            break;
        case 403: /* possible proxy ACL; keep separate from other HTTP 4XX */
            printf("%s: %s: %s: %s: %s%s: %s%d: ", ts, NAME, "WARN", 
                    "STANDARD", "dst=", skthost.s_name, "dport=", skthost.s_port);
            printf("%s: %s%s: %s%s %s%s: %s%s: %s%s: %s%s: %s:%d: %s\n", 
                    "action=procresp", "verb=", thisreq->verb, "scheme=", thisreq->scheme, 
                    "domain=", thisreq->domain, "port=", thisreq->port, "path=", thisreq->path, 
                    "ver=", thisreq->ver, "status", status, RESP_MSG_403);
            break;
        case 404 ... 406:
            printf("%s: %s: %s: %s: %s%s: %s%d: ", ts, NAME, "WARN", 
                    "STANDARD", "dst=", skthost.s_name, "dport=", skthost.s_port);
            printf("%s: %s%s: %s%s: %s%s: %s%s: %s%s: %s%s: %s:%d\n", "action=procresp", 
                    "verb=", thisreq->verb, "scheme=", thisreq->scheme, "domain=", 
                    thisreq->domain, "port=", thisreq->port, "path=", thisreq->path, 
                    "ver=", thisreq->ver, "status", status);
            break;
        case 407:
#if EXIT_PROCRESP_407_ERR
            fprintf(stderr, "%s: %s: %s: %s: %s%s: %s%d: ", ts, NAME, "ERR", 
                    "STANDARD", "dst=", skthost.s_name, "dport=", skthost.s_port);
            fprintf(stderr, "%s: %s%s: %s%s: %s%s: %s%s: %s%s: %s%s: %s:%d: %s\n", 
                    "action=procresp", "verb=", thisreq->verb, "scheme=", thisreq->scheme, 
                    "domain=", thisreq->domain, "port=", thisreq->port, "path=", 
                    thisreq->path, "ver=", thisreq->ver, "status", status,RESP_MSG_407);
            fprintf(stderr, "%s: %s: %s: %s\n", ts, NAME, "ERR",
                    "Exiting - Avoid this with macro: NO_EXIT_PROCRESP_407_ERR");
            exit(1);
#else
            printf("%s: %s: %s: %s: %s%s: %s%d: ", ts, NAME, "WARN", 
                    "STANDARD", "dst=", skthost.s_name, "dport=", skthost.s_port);
            printf("%s: %s%s: %s%s: %s%s: %s%s: %s%s: %s%s: %s:%d: %s\n", 
                    "action=procresp", "verb=", thisreq->verb, "scheme=", thisreq->scheme, 
                    "domain=", thisreq->domain, "port=", thisreq->port, "path=", thisreq->path, 
                    "ver=", thisreq->ver, "status", status, RESP_MSG_407);
            break;
#endif
        case 408 ... 499:
        case 500 ... 599:
            printf("%s: %s: %s: %s: %s%s: %s%d: ", ts, NAME, "WARN", 
                    "STANDARD", "dst=", skthost.s_name, "dport=", skthost.s_port);
            printf("%s: %s%s: %s%s: %s%s: %s%s: %s%s: %s%s: %s:%d\n", "action=procresp", 
                    "verb=", thisreq->verb, "scheme=", thisreq->scheme, "domain=", 
                    thisreq->domain, "port=", thisreq->port, "path=", thisreq->path, 
                    "ver=", thisreq->ver, "status", status);
            break;
        default:
#if EXIT_PROCRESP_DEF_ERR
            fprintf(stderr, "%s: %s: %s: %s: %s%s: %s%d: ", ts, NAME, "ERR", 
                    "STANDARD", "dst=", skthost.s_name, "dport=", skthost.s_port);
            fprintf(stderr, "%s: %s%s: %s%s: %s%s: %s%s: %s%s: %s%s: %s:%d\n", 
                    "action=procresp", "verb=", thisreq->verb, "scheme=", thisreq->scheme, 
                    "domain=", thisreq->domain, "port=", thisreq->port, "path=", 
                    thisreq->path, "ver=", thisreq->ver, "status", status);
            fprintf(stderr, "%s: %s: %s: %s\n", ts, NAME, "ERR",
                    "Exiting - Avoid this with macro: NO_EXIT_PROCRESP_DEF_ERR");
            exit(1);
#else
            printf("%s: %s: %s: %s: %s%s: %s%d: ", ts, NAME, "WARN", 			
                    "STANDARD", "dst=", skthost.s_name, "dport=", skthost.s_port);
            printf("%s: %s%s: %s%s: %s%s: %s%s: %s%s: %s%s: %s:%d\n", "action=procresp", 
                    "verb=", thisreq->verb, "scheme=", thisreq->scheme, "domain=", 
                    thisreq->domain, "port=", thisreq->port, "path=", thisreq->path, 
                    "ver=", thisreq->ver, "status", status);
#endif
        }
        thisreq = thisreq->next;
        close(sd);
        memset(response, 0, respbuf);
        thisredirect = 0;
        if(!d_sleep){
            if(thisreq == NULL)
                break;
        }else{
            if(thisreq == NULL){
                printf("%s: %s: %s: %s: %u %s\n", ts, NAME, "daemon", "sleeping", 
                        d_sleep, (d_sleep == 1) ? "second" : "seconds");
                sleep(d_sleep);
                gettime(ts);
                printf("%s: %s: %s: %s\n", ts, NAME, "request", "processing");
                thisreq = req;
            }
        }
    }

    gettime(ts);
    printf("%s: %s: %s\n", ts, NAME, "processing complete");

    /* Clean up */

    gettime(ts);
    printf("%s: %s: %s\n", ts, NAME, "cleaning up");
        
#ifndef AVOID_MALLOC_FREE
    if(redirect != NULL) free(redirect);
    if(response != NULL) free(response);
    if(request != NULL) free(request);  
    
    while(req != NULL)
    {
        thisreq = req;
        req = req->next;
        if(thisreq != NULL) free(thisreq);
    }
#endif

    /* All done */

    gettime(ts);
    printf("%s: %s: %s\n", ts, NAME, "finished");

    return 0;
}
