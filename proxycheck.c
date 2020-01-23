/*
    Purpose:
        C file for "proxycheck".
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
        |__ proxycheck_main.c, proxycheck.h
*/

#include "proxycheck.h"

void buildreq(struct request *req, struct requestopt *reqopt)
{
    char        tmpurl[REQ_PATH_SZ]= {0}, *scheme = tmpurl, *domain = tmpurl,
                *tmpurlport = NULL, tmpreqport[REQ_PORT_SZ], *path = tmpurl;
    const char  *hdr = reqopt->hdr;
    int 	    urlportoverride = 0, len = 0;

    if(hdr){
        while(*hdr != '\0')
        {
            switch(*hdr){
            case 'A':
            case 'a':
                reqopt->do_accept_hdr = 1;
                break;
            case 'C':
            case 'c':
                reqopt->do_conn_hdr = 1;
                break;
            case 'H':
            case 'h':
                reqopt->do_host_hdr = 1;
                break;
            case 'U':
            case 'u':
                reqopt->do_user_hdr = 1;
                break;
            }
            hdr++;
        }
    }
  
    memset(req->connverb, 0, REQ_VERB_SZ);
    memset(req->verb, 0, REQ_VERB_SZ);
    memset(req->scheme, 0, REQ_SCHEME_SZ);
    memset(req->domain, 0, HOST_SZ);
    memset(req->path, 0, REQ_PATH_SZ);
    memset(req->port , 0, REQ_PORT_SZ);
    memset(req->ver, 0, REQ_HTTP_VER_SZ);
    memset(req->connreq, 0, REQ_CONN_SZ);
    memset(req->httpreq, 0, REQ_SZ);
    memset(req->host_hdr, 0, REQ_HOST_HDR_SZ);
    memset(req->user_hdr, 0, REQ_USER_HDR_SZ);
    memset(req->accept_hdr, 0, REQ_ACCEPT_HDR_SZ);
    memset(req->conn_hdr, 0, REQ_CONN_HDR_SZ);
    req->valid_request = 0;

    /* Prep for parsing */
    for(int i = 0; reqopt->tmpurl[i] != '\0'; ++i) 
        reqopt->tmpurl[i] = tolower(reqopt->tmpurl[i]);

    strncpy(tmpurl, reqopt->tmpurl, REQ_PATH_SZ);
    len = strlen(tmpurl);

    if(len >= REQ_PATH_SZ)
        tmpurl[REQ_PATH_SZ-1] = '\0';
    else
        tmpurl[len] = '\0';

    /* Parse for method */
    if(strstr(tmpurl, "http://") != NULL){
        if(strncasecmp(reqopt->verb, "CONNECT", strlen(req->verb)+1) == 0){
            strncpy(req->connverb, "CONNECT", REQ_VERB_SZ);
            strncpy(req->verb, "GET", REQ_VERB_SZ);
        }else{
            strncpy(req->verb, reqopt->verb, REQ_VERB_SZ);
            len = strlen(req->verb);
            if(len >= REQ_VERB_SZ)
                req->verb[REQ_VERB_SZ-1] = '\0';
            else
                req->verb[len] = '\0';  
        }
    }else if(strstr(tmpurl, "https://") != NULL){
        if(strncasecmp(reqopt->verb, "CONNECT", strlen(req->verb)+1) == 0){
            strncpy(req->connverb, "CONNECT", REQ_VERB_SZ);
            strncpy(req->verb, "GET", REQ_VERB_SZ);
        }else{
            strncpy(req->connverb, "CONNECT", REQ_VERB_SZ);
            strncpy(req->verb, reqopt->verb, REQ_VERB_SZ);
            len = strlen(req->verb);
            if(len >= REQ_VERB_SZ)
                req->verb[REQ_VERB_SZ-1] = '\0';
            else
                req->verb[len] = '\0';
        }
    }else{ /* No scheme in URL */
        if(strstr(reqopt->scheme, "https://") != NULL){
            strncpy(req->connverb, "CONNECT", REQ_VERB_SZ);
            strncpy(req->verb, "GET", REQ_VERB_SZ);
            len = strlen(req->verb);
            if(len >= REQ_VERB_SZ)
                req->verb[REQ_VERB_SZ-1] = '\0';
            else
                req->verb[len] = '\0';
        }else{
            if(strncasecmp(reqopt->verb, "CONNECT", strlen(req->verb)+1) == 0){
                strncpy(req->connverb, "CONNECT", REQ_VERB_SZ);
                strncpy(req->verb, "GET", REQ_VERB_SZ);
            }else{
                strncpy(req->verb, reqopt->verb, REQ_VERB_SZ);
                len = strlen(req->verb);
                if(len >= REQ_VERB_SZ)
                    req->verb[REQ_VERB_SZ-1] = '\0';
                else
                    req->verb[len] = '\0';
            }
        }
    }

    /* Parse for scheme */
    if((scheme = strstr(tmpurl, "http://")) != NULL){
        strncpy(req->scheme, "http://", REQ_SCHEME_SZ);
        scheme += REQ_SCHEME_SZ - 3; 
        *scheme = '\0';
        domain = scheme + 1;
        path = domain;
        if(reqopt->port != -1){
            sprintf(tmpreqport, "%d", reqopt->port);
            urlportoverride = 1;
        }else{
            sprintf(tmpreqport, "%d", REQ_HTTP_PORT_DEF);
        }
        strncpy(req->port, tmpreqport, REQ_PORT_SZ);
        len = strlen(req->port);
        if(len >= REQ_PORT_SZ)
            req->port[REQ_PORT_SZ-1] = '\0';
        else
            req->port[len] = '\0';    
    }else if((scheme = strstr(tmpurl, "https://")) != NULL){
        strncpy(req->scheme, "https://", REQ_SCHEME_SZ);
        scheme += REQ_SCHEME_SZ - 2; 
        *scheme = '\0';
        domain = scheme + 1;
        path = domain;
        if(reqopt->port != -1){
            sprintf(tmpreqport, "%d", reqopt->port);
            urlportoverride = 1;
        }else{
            sprintf(tmpreqport, "%d", REQ_HTTPS_PORT_DEF);
        }
        strncpy(req->port, tmpreqport, REQ_PORT_SZ);
        len = strlen(req->port);
        if(len >= REQ_PORT_SZ)
            req->port[REQ_PORT_SZ-1] = '\0';
        else
            req->port[len] = '\0';
    }else{ /* invalid / missing scheme in URL */
        for(int i = 0; reqopt->scheme[i] != '\0'; ++i)
            reqopt->scheme[i] = tolower(reqopt->scheme[i]);
        strncpy(req->scheme, reqopt->scheme, REQ_SCHEME_SZ);
        if(strstr(req->scheme, "https://") != NULL){
            sprintf(tmpreqport, "%d", REQ_HTTPS_PORT_DEF);
            if(reqopt->port != -1){
                sprintf(tmpreqport, "%d", reqopt->port);
                urlportoverride = 1;
            }else{
                sprintf(tmpreqport, "%d", REQ_HTTPS_PORT_DEF);
            }
        }else{
            if(reqopt->port != -1){
                sprintf(tmpreqport, "%d", reqopt->port);
                urlportoverride = 1;
            }else{
                sprintf(tmpreqport, "%d", REQ_HTTP_PORT_DEF);
            }
        }
        strncpy(req->port, tmpreqport, REQ_PORT_SZ);
        len = strlen(req->port);
        if(len >= REQ_PORT_SZ)
            req->port[REQ_PORT_SZ-1] = '\0';
        else
            req->port[len] = '\0';
    }

    /* Parse for domain and port */

     if((tmpurlport = strchr(domain, ':')) != NULL){
        *tmpurlport = '\0';
        if(tmpurlport + 1 != NULL){
            tmpurlport++;
            len = strnlen(tmpurlport, REQ_PORT_SZ-1);
            strncpy(reqopt->tmpurlport, tmpurlport, len);
            for(int i = 0; reqopt->tmpurlport[i] != '\0'; ++i)
                if (! isdigit(reqopt->tmpurlport[i]))
                    reqopt->tmpurlport[i] = '\0';
            strncpy(req->port, reqopt->tmpurlport, REQ_PORT_SZ);
            len = strlen(req->port);
            if(len >= REQ_PORT_SZ)
                req->port[REQ_PORT_SZ-1] = '\0';
            else
                req->port[len] = '\0';
            urlportoverride = 1;
        }
    }else{ /* Override req port */
        if(reqopt->port != -1){
            memset(tmpreqport, 0, REQ_PORT_SZ);
            sprintf(tmpreqport, "%d", reqopt->port);
            strncpy(req->port, tmpreqport, REQ_PORT_SZ);
            len = strlen(req->port);
            if(len >= REQ_PORT_SZ)
                req->port[REQ_PORT_SZ-1] = '\0';
            else
                req->port[len] = '\0';
        }
    }

    /* Parse for path */
    if((path = strchr(domain, '/')) != NULL){
        strncpy(req->path, path, REQ_PATH_SZ);
        *path = '\0';
    }else{
        strcpy(req->path, REQ_PATH_DEF);
    }

    len = strlen(req->path);
    if(len >= REQ_PATH_SZ)
        req->path[REQ_PATH_SZ-1] = '\0';
    else
        req->path[len] = '\0';

    strncpy(req->domain, domain, HOST_SZ);
    len = strlen(req->domain);

    if(len >= HOST_SZ)
        req->domain[HOST_SZ-1] = '\0';
    else
        req->domain[len] = '\0';

    /* HTTP version */
    strncpy(req->ver, reqopt->ver, REQ_HTTP_VER_SZ);

    /* HTTP headers */
    if(reqopt->do_host_hdr){
        strncpy(req->host_hdr, "Host: ", REQ_HOST_HDR_SZ-1);
        req->host_hdr[REQ_HOST_HDR_SZ-1] = '\0';
        len = strlen(req->host_hdr);
        strncat(req->host_hdr, req->domain, (REQ_HOST_HDR_SZ-len)-1);
    }

    if(reqopt->do_user_hdr){
        strncpy(req->user_hdr, "User-Agent: ", REQ_USER_HDR_SZ-1);
        req->user_hdr[REQ_USER_HDR_SZ-1] = '\0';
        len = strlen(req->user_hdr);
        strncat(req->user_hdr, NAME, (REQ_USER_HDR_SZ-len)-1);
        len = strlen(req->user_hdr);
        strncat(req->user_hdr, "\r\n", (REQ_USER_HDR_SZ-len)-1);
    }

    if(reqopt->do_accept_hdr){
        strncpy(req->accept_hdr, "Accept: ", REQ_ACCEPT_HDR_SZ-1);
        req->accept_hdr[REQ_ACCEPT_HDR_SZ-1] = '\0';
        len = strlen(req->accept_hdr);
        strncat(req->accept_hdr, REQ_ACCEPT_HDR_DEF, (REQ_ACCEPT_HDR_SZ-len)-1);
        len = strlen(req->accept_hdr);
        strncat(req->accept_hdr, "\r\n", (REQ_ACCEPT_HDR_SZ-len)-1);
    }

    if(reqopt->do_conn_hdr){
        strncpy(req->conn_hdr, "Connection: close", REQ_CONN_HDR_SZ-1);
        req->conn_hdr[REQ_CONN_HDR_SZ-1] = '\0';
        len = strlen(req->conn_hdr);
        strncat(req->conn_hdr, "\r\n", (REQ_CONN_HDR_SZ-len)-1);
    }

    /* Build request */
    if(strlen(req->port) > 0 )
        req->valid_request = 1;
    else
       strncpy(req->port, "NULL", REQ_PORT_SZ);

    if(req->connverb[0] != 0){ /* CONNECT */
#if NO_CONN_FORCE_GET_DEF_VERB
        if(strncasecmp(reqopt->verb, "CONNECT", strlen(req->verb)+1) == 0){
            strncpy(req->connverb, "CONNECT", REQ_VERB_SZ);
            strncpy(req->verb, "GET", REQ_VERB_SZ);
        }else{
            strncpy(req->verb, reqopt->verb, REQ_VERB_SZ);
            len = strlen(req->verb);
            if(len >= REQ_VERB_SZ)
                req->verb[REQ_VERB_SZ-1] = '\0';
            else
                req->verb[len] = '\0';
        }
#endif
        strncpy(req->connreq, req->connverb, REQ_CONN_SZ-1);
        req->connreq[REQ_CONN_SZ-1] = '\0';
        len = strlen(req->connreq);
        strncat(req->connreq, " ", (REQ_CONN_SZ-len)-1);
        len = strlen(req->connreq);
        strncat(req->connreq, req->domain, (REQ_CONN_SZ-len)-1);
        len = strlen(req->connreq);
        strncat(req->connreq, ":", (REQ_CONN_SZ-len)-1);
        len = strlen(req->connreq);
        strncat(req->connreq, req->port, (REQ_CONN_SZ-len)-1);
        len = strlen(req->connreq);
        strncat(req->connreq, " ", (REQ_CONN_SZ-len)-1);
        len = strlen(req->connreq);
        strncat(req->connreq, req->ver, (REQ_CONN_SZ-len)-1);
        len = strlen(req->connreq);
        strncat(req->connreq, "\r\n", (REQ_CONN_SZ-len)-1);
        if(reqopt->do_host_hdr){
            len = strlen(req->connreq);
            strncat(req->connreq, req->host_hdr, (REQ_CONN_SZ-len)-1);
            len = strlen(req->connreq);
            strncat(req->connreq, ":", (REQ_CONN_SZ-len)-1);
            len = strlen(req->connreq);
            strncat(req->connreq, req->port, (REQ_CONN_SZ-len)-1);
            len = strlen(req->connreq);
            strncat(req->connreq, "\r\n", (REQ_CONN_SZ-len)-1);
        }
        len = strlen(req->connreq);
        strncat(req->connreq, "\r\n\r\n", (REQ_CONN_SZ-len)-1);
        strncpy(req->httpreq, req->verb, REQ_SZ-1);
        req->httpreq[REQ_SZ-1] = '\0';
        len = strlen(req->httpreq);
        strncat(req->httpreq, " ", (REQ_SZ-len)-1);
        len = strlen(req->httpreq);
        strncat(req->httpreq, req->path, (REQ_SZ-len)-1);
        len = strlen(req->httpreq);
        strncat(req->httpreq, " ", (REQ_SZ-len)-1);
        len = strlen(req->httpreq);
        strncat(req->httpreq, req->ver, (REQ_SZ-len)-1);
        len = strlen(req->httpreq);
        strncat(req->httpreq, "\r\n", (REQ_SZ-len)-1);
    }else{
        strncpy(req->httpreq, req->verb, REQ_SZ-1);
        req->httpreq[REQ_SZ-1] = '\0';
        len = strlen(req->httpreq);
        strncat(req->httpreq, " ", (REQ_SZ-len)-1);
        len = strlen(req->httpreq);
        strncat(req->httpreq, req->scheme, (REQ_SZ-len)-1);
        len = strlen(req->httpreq);
        strncat(req->httpreq, req->domain, (REQ_SZ-len)-1);
        if(urlportoverride){
            strncat(req->httpreq, ":", REQ_SZ-1);
            req->httpreq[REQ_SZ-1] = '\0';
            len = strlen(req->httpreq);
            strncat(req->httpreq, req->port, (REQ_SZ-len)-1);
        }
        len = strlen(req->httpreq);
        strncat(req->httpreq, req->path, (REQ_SZ-len)-1);
        len = strlen(req->httpreq);
        strncat(req->httpreq, " ", (REQ_SZ-len)-1);
        len = strlen(req->httpreq);
        strncat(req->httpreq, req->ver, (REQ_SZ-len)-1);
        len = strlen(req->httpreq);
        strncat(req->httpreq, "\r\n", (REQ_SZ-len)-1);
    }

    if(reqopt->do_host_hdr){
        len = strlen(req->httpreq);
        strncat(req->httpreq, req->host_hdr, (REQ_SZ-len)-1);
        len = strlen(req->httpreq);
        strncat(req->httpreq, "\r\n", (REQ_SZ-len)-1);
    }

    if(reqopt->do_user_hdr){
        len = strlen(req->httpreq);
        strncat(req->httpreq, req->user_hdr, (REQ_SZ-len)-1);
    }

    if(reqopt->do_accept_hdr){
        len = strlen(req->httpreq);
        strncat(req->httpreq, req->accept_hdr, (REQ_SZ-len)-1);
    }

    if(reqopt->do_conn_hdr){
        len = strlen(req->httpreq);
        strncat(req->httpreq, req->conn_hdr, (REQ_SZ-len)-1);
    }

    len = strlen(req->httpreq);
    strncat(req->httpreq, "\r\n\r\n", (REQ_SZ-len)-1);
    
    len = strlen(req->connverb);  
    if(len >= REQ_VERB_SZ)
        req->connverb[REQ_VERB_SZ-1] = '\0';
    else
        req->connverb[len] = '\0';
        
    len = strlen(req->verb);  
    if(len >= REQ_VERB_SZ)
        req->verb[REQ_VERB_SZ-1] = '\0';
    else
        req->verb[len] = '\0';
    
    len = strlen(req->scheme);
    if(len >= REQ_SCHEME_SZ)
        req->scheme[REQ_SCHEME_SZ-1] = '\0';
    else
        req->scheme[len] = '\0';
    
    len = strlen(req->domain);  
    if(len >= HOST_SZ)
        req->domain[HOST_SZ-1] = '\0';
    else
        req->domain[len] = '\0';
        
    len = strlen(req->path);
    if(len >= REQ_PATH_SZ)
        req->path[REQ_PATH_SZ-1] = '\0';
    else
        req->path[len] = '\0';
        
    len = strlen(req->ver); 
    if(len >= REQ_HTTP_VER_SZ)
        req->ver[REQ_HTTP_VER_SZ-1] = '\0';
    else
        req->ver[len] = '\0';
        
    len = strlen(req->connreq);
    if(len >= REQ_CONN_SZ)
        req->connreq[REQ_CONN_SZ-1] = '\0';
    else
        req->connreq[len] = '\0';
    
    len = strlen(req->httpreq); 
    if(len >= REQ_SZ)
        req->httpreq[REQ_SZ-1] = '\0';
    else
        req->httpreq[len] = '\0';
    
    len = strlen(req->host_hdr);  
    if(len >= REQ_HOST_HDR_SZ)
        req->host_hdr[REQ_HOST_HDR_SZ-1] = '\0';
    else
        req->host_hdr[len] = '\0';
        
    len = strlen(req->user_hdr);   
    if(len >= REQ_USER_HDR_SZ)
        req->user_hdr[REQ_USER_HDR_SZ-1] = '\0';
    else
        req->user_hdr[len] = '\0';
        
    len = strlen(req->accept_hdr);   
    if(len >= REQ_ACCEPT_HDR_SZ)
        req->accept_hdr[REQ_ACCEPT_HDR_SZ-1] = '\0';
    else
        req->accept_hdr[len] = '\0';
        
    len = strlen(req->conn_hdr);
    if(len >= REQ_CONN_HDR_SZ)
        req->conn_hdr[REQ_CONN_HDR_SZ-1] = '\0';
    else
        req->conn_hdr[len] = '\0';

#if DEBUG_BUILDREQ
    char ts[TIME_SZ];
    /* options passed */
    gettime(ts);
    fprintf(stderr, "%s: %s: %s: %s:\n", ts, NAME, "[DEBUG]", "request options");
    fprintf(stderr, "%s%d\n%s%d\n%s%d\n%s%d\n%s%s\n%s%d\n", 
            "[*] do_accept_hdr=", reqopt->do_accept_hdr, "[*] do_conn_hdr=", 
            reqopt->do_conn_hdr, "[*] do_host_hdr=", reqopt->do_host_hdr, 
            "[*] do_user_hdr=", reqopt->do_user_hdr, "[*] hdr=", reqopt->hdr, 
            "[*] port=", reqopt->port);
    fprintf(stderr, "%s%s\n%s%d\n%s%s\n%s%s\n%s%s\n%s%s\n%s%s\n", "[*] verb=", 
            reqopt->verb, "[*] opt_scheme=", reqopt->opt_scheme, "[*] scheme", 
            reqopt->scheme, "[*] tmpurl=", reqopt->tmpurl, "[*] tmpurlport=", 
            reqopt->tmpurlport, "[*] path=", reqopt->path, "[*] ver=", reqopt->ver);

    /* request object */
    gettime(ts);
    fprintf(stderr, "%s: %s: %s: %s:\n", ts, NAME, "[DEBUG]", "request object");
    fprintf(stderr, "%s%d\n%s%s%s%s%s%s\n%s%s",  "[*] valid_request=", req->valid_request, 
            "[*] accept_hdr=", req->accept_hdr, "[*] conn_hdr=", req->conn_hdr, "[*] host_hdr=", 
            req->host_hdr, "[*] user_hdr=", req->user_hdr); 
    fprintf(stderr, "%s%s\n%s%s\n%s%s\n%s%s\n%s%s\n%s%s\n%s%s\n",
            "[*] connverb=", req->connverb, "[*] verb=", req->verb, "[*] scheme=", req->scheme, 
            "[*] domain=", req->domain, "[*] path=", req->path, "[*] port=", req->port, 
            "[*] ver=", req->ver);
    fprintf(stderr, "%s:\n%s%s:\n%s", "[*] Built CONNECT request (connreq)", 
            strlen(req->connreq) ? req->connreq : "|__ N/A\n", "[*] Built STANDARD request (httpreq)", 
            strlen(req->httpreq) ? req->httpreq : "|__ N/A\n");
#if DEBUG_FORCE_EXIT
    exit(1);
#endif
#endif
}
int daemonize(pid_t *pid, char *ts)
{
    pid_t sid;

    gettime(ts);
    printf("%s: %s: %s: %s\n", ts, NAME, "daemon", "initializing");

    *pid = fork();

    switch(*pid){
    case -1:
        gettime(ts);
        fprintf(stderr, "%s: %s: ERR: %s: %s\n", ts, NAME, "fork", 
                strerror(errno));
        exit(1);
    case 0:
        gettime(ts);
        printf("%s: %s: %s: %s: %s\n", ts, NAME, "daemon", "child", 
                "setting home directory");
        if(chdir(DAEMON_DIR_DEF) == -1){
            fprintf(stderr, "%s: ERR: %s: %s\n", NAME, "chdir", 
                    strerror(errno));
            exit(1);
        }
        gettime(ts);
        printf("%s: %s: %s: %s: %s: %s\n", ts, NAME, "daemon", "child", 
                "home directory", DAEMON_DIR_DEF);
        umask(0);
        gettime(ts);
        printf("%s: %s: %s: %s: %s\n", ts, NAME, "daemon", "child", 
                "file creation mode mask set");
        gettime(ts);
        printf("%s: %s: %s: %s: %s\n", ts, NAME, "daemon", "child", 
                "creating session and setting process group ID");
        if((sid = setsid()) == -1){
            fprintf(stderr, "%s: %s: ERR: %s: %s\n", ts, NAME, "setsid", 
                    strerror(errno));
            exit(1);
        }
        gettime(ts);
        printf("%s: %s: %s: %s: %s\n", ts, NAME, "daemon", "child", 
            "session and process group ID set");
        gettime(ts);
        printf("%s: %s: %s: %s: %s\n", ts, NAME, "daemon", "child", 
            "closing streams and fading to background");
#if 0 /* defer to logevents() to close streams for now */
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
#endif
        break;
    default:
        gettime(ts);
        printf("%s: %s: %s: %s: %s: %d\n", ts, NAME, "daemon", "parent",
                "child created", *pid);
        printf("%s: %s: %s: %s: %s\n", ts, NAME, "daemon", "parent",
                "exiting");
        exit(0);
    }
  
    return 0;
}
int getaddr(const char *hostname, char *ip, const int buf)
{
    int             result;
    struct addrinfo *ai;
    void            *ptr;

    memset(&ai, 0, sizeof (ai));
    
    if((result = getaddrinfo(hostname, NULL, NULL, &ai)) != 0)
        return 1;	

    while(ai)
    {
        inet_ntop(ai->ai_family, ai->ai_addr->sa_data, ip, buf);
        if(ai->ai_family == AF_INET){
            ptr = &((struct sockaddr_in *) ai->ai_addr)->sin_addr;
            inet_ntop(ai->ai_family, ptr, ip, buf);
            break; /* grab first address */
        }
        ai = ai->ai_next;
    }
  
    freeaddrinfo(ai);

    return 0;
}
int getlocation(char *response, unsigned int respbuf, char *location, 
                unsigned int locbuf, char *lastreqdomain)
{
    char    *loc = NULL, *loc_start = NULL, *loc_end = NULL; 
    int     len = 0; 

    loc = strcasestr(response, "Location:");

    if(loc == NULL) return 0;
    
    loc_end = strchr(loc, '\r');
    *loc_end = '\0';
    loc_start = strchr(loc, ':');
    loc_start += 2; /* bypass colon & space */

    if(*loc_start == '/')           /* domain not present in header (e.g. URI) */
        loc_start = lastreqdomain;  /* use last request domain */

    loc = loc_start;
    len = strlen(loc);

    memset(location, 0, locbuf);
    strcpy(location, loc);
    
    if(len >= locbuf)
        location[locbuf-1] = '\0';
    else
        location[len] = '\0'; 

    return 1;
}
int getreqcount(struct request *node) 
{ 
    if(node == NULL)
        return 0; 

    return 1 + getreqcount(node->next);
}
void getreqline(char *line, size_t linelen, FILE *stream, 
                char *requrl, long urlbuf)
{
    char    ts[TIME_SZ], *tmprequrl;
    int     len;
    
    if((tmprequrl = (char *) malloc(sizeof(char)*urlbuf)) == NULL){
        gettime(ts);
        fprintf(stderr, "%s: %s: ERR: %s: %s: %s\n", ts, NAME, "getreqline", 
                "malloc", strerror(errno));
        exit(1);   
    }

    memset(tmprequrl, 0, urlbuf);
    memset(requrl, 0, urlbuf);

  while(getline(&line, &linelen, stream) != -1)
  {
        stripline(line);
        if(!tmprequrl){
            strncpy(tmprequrl, line, urlbuf-1);
            tmprequrl[urlbuf-1] = '\0';
        }else{
            len = strlen(tmprequrl);
            strncat(tmprequrl, line, (urlbuf-len)-1);
        }
        line = NULL;
  }
  
    strncpy(requrl, tmprequrl, urlbuf-1);
    requrl[urlbuf-1] = '\0';
    free(tmprequrl);
}
int gettime(char *ts)
{
    struct timeval tv;

    if(gettimeofday(&tv, NULL) == -1){
        fprintf(stderr, "%s: gettimeofday (%d): %s\n", NAME, errno,
                strerror(errno));
        return -1;
    }

    memset(ts, 0, TIME_SZ);	
    stpncpy(ts, ctime(&tv.tv_sec), TIME_SZ);
    ts[TIME_SZ-2] = '\0'; /* account for \n */

    return 0;
}
int initsock(int sa_fam, int type, int proto, struct sockaddr_in sa_addr,
            socklen_t sa_len)
{
    char ts[TIME_SZ];
    int sd;

    if((sd = socket(sa_fam, type, proto)) == -1){
        gettime(ts);
        fprintf(stderr, "%s: %s: ERR: %s: %s\n", ts, NAME, "socket", strerror(errno));
        exit(1);
    }

    if(connect(sd, (struct sockaddr *) &sa_addr, sa_len) == -1){
        gettime(ts);
        fprintf(stderr, "%s, %s: ERR: %s: %s\n", ts, NAME, "connect", strerror(errno));
        exit(1);
    }

    return sd;
}
void logevents(char *logfile, int lmode, FILE *lstream)
{
    char ts[TIME_SZ];
    int fd; 

    if((fd = open(logfile, O_RDWR|O_CREAT|O_APPEND, 0600)) == -1){
        fprintf(stderr, "%s: %s: ERR: %s: %s\n", ts, NAME, "open", strerror(errno));
        exit(1);
    }

    switch(lmode){
    case LOG_IONBF: /* recommended */
        if(setvbuf(lstream, NULL, _IONBF, 0) != 0){
            gettime(ts);
            fprintf(stdout, "%s: %s: ERR: %s: %s: %s\n", ts, NAME, "setvbuf", "_IONBF", 
                    strerror(errno));
            exit(1);
        }
        break;
    case LOG_IOLBF:
        if(setvbuf(lstream, NULL, _IOLBF, 0) != 0){
            gettime(ts);
            fprintf(stderr, "%s: %s: ERR: %s: %s: %s\n", ts, NAME, "setvbuf", "_IOLBF",
                    strerror(errno));
            exit(1);
        }
        break;
    case LOG_IOFBF:
        if(setvbuf(lstream, NULL, _IOFBF, 0) != 0){
            gettime(ts);
            fprintf(stderr, "%s: %s: ERR: %s: %s: %s\n", ts, NAME, "setvbuf", "_IOFBF", 
                    strerror(errno));
            exit(1);
        }
        break;
    default:
        gettime(ts);
        fprintf(stderr, "%s: %s: ERR: %s: %d\n", ts, NAME, "log mode unknown", lmode);
        exit(1);
    }

#if 0 /* defer to dup2(2) to deal with streams */
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
#else
    dup2(fd, STDOUT_FILENO);
    dup2(fd, STDERR_FILENO);
    close(fd);
#endif
}
long optdaemon(const char **arg, unsigned int *index)
{
    long interval = 0;

    if(arg[(*index)+1] == NULL || *(arg[(*index)+1]) == '-'){
        interval = DAEMON_SLEEP_DEF;
    }else{  
        interval = atoi(arg[++(*index)]);
        if(interval < DAEMON_SLEEP_MIN || interval > DAEMON_SLEEP_MAX){
            fprintf(stderr, "%s: ERR: %s %ld %s: %d - %ld\n", NAME, 
                    "daemon interval (", interval, ") not in range", 
                    DAEMON_SLEEP_MIN, DAEMON_SLEEP_MAX);
            exit(1);
        }
    }

    return interval;
}
int *optfollow(const char **arg, unsigned int *index, int *total)
{
    int     i = 1, http_redirect[MAX_REDIRECT_TYPES] = {301, 302, 303, 307},
            *redirect = NULL, found = 0;
    char    *optredirects, *tmpredirect, *cp;
    
    if(arg[(*index)+1] == NULL || *(arg[(*index)+1]) == '-'){
        fprintf(stderr, "%s: ERR: %s: %s\n", NAME, 
                "missing HTTP status to follow", arg[*index]);
        exit(1);
    }
    
    if((redirect = (int *) malloc(sizeof(int))) == NULL){
        fprintf(stderr, "%s: ERR: %s: %s: %s\n", NAME, "follow", 
                "malloc", strerror(errno));
        exit(1);   
    }

    optredirects = (char *) arg[++(*index)];
    
    tmpredirect = strtok_r(optredirects, REQ_TOKENS, &cp);
    *redirect = atoi(tmpredirect);
    
    while((tmpredirect = strtok_r(NULL, REQ_TOKENS, &cp)) != NULL)
    {
        if((redirect = (int *) realloc(redirect, sizeof(int))) == NULL){
            fprintf(stderr, "%s: ERR: %s: %s: %s\n", NAME, "follow", 
                    "malloc", strerror(errno));
            if(redirect != NULL) 
                free(redirect);
            exit(1);   
        }
        *(redirect + i) = atoi(tmpredirect);
        ++i;
    }

    *total = i;
   
    for(int j = 0; j != *total; ++j)
    {
        found = MAX_REDIRECT_TYPES;
        for(int k = 0; k != MAX_REDIRECT_TYPES; ++k)
        {
            if(*(redirect + j) == http_redirect[k])
                break;
            else
                --found;
        }
        if(!found){
            fprintf(stderr, "%s: ERR: %s: %d\n", NAME, "invalid redirect",
                    *(redirect+j));
            free(redirect);
            exit(1);
        }
    }
    
    return redirect;
}
void optheader(const char **arg, unsigned int *index, struct requestopt *reqopt)
{
    const char *hdr;
    
    if(arg[(*index)+1] == NULL || *(arg[(*index)+1]) == '-'){
        fprintf(stderr, "%s: ERR: %s: %s\n", NAME, "missing header", 
                arg[*index]);
        exit(1);
    }
  
    reqopt->hdr = arg[++(*index)];
    hdr = reqopt->hdr;
  
    while(*hdr != '\0')
    {
        switch(*hdr){
        case 'A':
        case 'a':
            if(reqopt->do_accept_hdr) break;
            reqopt->do_accept_hdr = 1;
            break;
        case 'C':
        case 'c':
            if(reqopt->do_conn_hdr) break;
            reqopt->do_conn_hdr = 1;
            break;
        case 'H':
        case 'h':
            if(reqopt->do_host_hdr) break;
            reqopt->do_host_hdr = 1;
            break;
        case 'U':
        case 'u':
            if(reqopt->do_user_hdr) break;
            reqopt->do_user_hdr = 1;
            break;
        default:
            fprintf(stderr, "%s: ERR: %s: %c\n", "invalid header", NAME, *hdr);
            usage();
        }
        hdr++;
    }
}
int optinput(const char **arg, unsigned int *index)
{
    int r_tmout;
    
    if(arg[(*index)+1] == NULL || *(arg[(*index)+1]) == '-'){
        fprintf(stderr, "%s: ERR: %s: %s\n", NAME, "missing input time",
                arg[*index]);
        usage();
    }
    
    r_tmout = atoi(arg[++(*index)]);
    
    if(r_tmout < READ_TM_MIN || r_tmout > READ_TM_MAX){
        fprintf(stderr, "%s: ERR: %s %d %s: %d - %d\n", NAME, 
                "input time (", r_tmout, ") not in range", 
                READ_TM_MIN, READ_TM_MAX);
        exit(1);
    }
    
    return r_tmout;
}
void optlog(const char **arg, unsigned int *index, char *logfile)
{
    if(arg[(*index)+1] == NULL || *(arg[(*index)+1]) == '-'){
        fprintf(stderr, "%s: ERR: %s: %s\n", NAME, "missing log file", arg[*index]);
        usage();
    }
    
    memset((char *)logfile, 0, FILE_BUF);
    strncpy(logfile, arg[++(*index)], FILE_BUF);
    logfile[FILE_BUF-1] = '\0';
}
int optoutput(const char **arg, unsigned int *index)
{
    int w_tmout;
    
    if(arg[(*index)+1] == NULL || *(arg[(*index)+1]) == '-'){
        fprintf(stderr, "%s: ERR: %s: %s\n", NAME, "missing output time",
                arg[*index]);
        usage();
    }
    
    w_tmout = atoi(arg[++(*index)]);
    
    if(w_tmout < WRITE_TM_MIN || w_tmout > WRITE_TM_MAX){
        fprintf(stderr, "%s: ERR: %s %d %s: %d - %d\n", NAME, 
                "output time (", w_tmout, ") not in range", 
                WRITE_TM_MIN, WRITE_TM_MAX);
        exit(1);
    }
    
    return w_tmout;
}
int optport(const char **arg, unsigned int *index)
{
    int port;
    
    if(arg[(*index)+1] == NULL || *(arg[(*index)+1]) == '-'){
        fprintf(stderr, "%s: ERR: %s: %s\n", NAME, "missing port", arg[*index]);
        usage();
    }
    
    port = atoi(arg[++(*index)]);
        
    if(port < PORT_MIN || port > PORT_MAX){
        fprintf(stderr, "%s: ERR: %s %d %s: %d - %d\n", NAME, 
                "request port (", port, ") not in range", 
                PORT_MIN, PORT_MAX);
        exit(1);
    }
    
    return port;
}
int optproxyport(const char **arg, unsigned int *index)
{
    int proxyport;
    
    if(arg[(*index)+1] == NULL || *(arg[(*index)+1]) == '-') return -1;

    proxyport = atoi(arg[++(*index)]);
        
    if(proxyport < PORT_MIN || proxyport > PORT_MAX){
        fprintf(stderr, "%s: ERR: %s %d %s: %d - %d\n", NAME, 
                "proxy port (", proxyport, ") not in range", 
                PORT_MIN, PORT_MAX);
        exit(1);
    }

    return proxyport;
}
unsigned int optrespbuffer(const char **arg, unsigned int *index)
{
    unsigned int respbuf;

    if(arg[(*index)+1] == NULL || *(arg[(*index)+1]) == '-'){
        respbuf = RESP_SZ_DEF;
    }else{
        respbuf = atoi(arg[++(*index)]);
        if(respbuf < RESP_SZ_MIN || respbuf > RESP_SZ_MAX){
            fprintf(stderr, "%s: ERR: %s %d %s: %d - %d\n", NAME, 
                    "response buffer (", respbuf, ") not in range", 
                    RESP_SZ_MIN, RESP_SZ_MAX);
            exit(1);
        }
    }

    return respbuf;
}
int optproxyserver(const char **arg, unsigned int *index, char *server)
{
    int len;
    
    if(arg[(*index)+1] == NULL || *(arg[(*index)+1]) == '-') return -1;
    
    memset(server, 0, HOST_SZ);
    strncpy(server, arg[++(*index)], HOST_SZ);

    len = strlen(server);
    
    if(len >= HOST_SZ)
        server[HOST_SZ-1] = '\0';
    else
        server[len] = '\0';
    
    return 0;
}
void optreq(const char **arg, unsigned int *index, char *requrl, long urlbuf)
{
    char    *c, *line = NULL;
    int     len;
    size_t  pos, linelen = 0;

    memset(requrl, 0, urlbuf);

    if(arg[(*index)+1] != NULL){
        if((c = strchr((arg[(*index)+1]), '-')) == NULL){
            strncpy(requrl, arg[++(*index)], urlbuf);
        }else{
            pos = (size_t)(c - arg[(*index)+1]); /* double-check for hyphen */
            if(pos > 0){ /* likely hyphen in URL, not argument/option */
                strncpy(requrl, arg[++(*index)], urlbuf);
            }else{
                getreqline(line, linelen, stdin, requrl, urlbuf);
                len = strlen(requrl);
                if(len >= urlbuf)
                    requrl[urlbuf-1] = '\0';
                else
                    requrl[len] = '\0';
            }
        }
    }else{
        getreqline(line, linelen, stdin, requrl, urlbuf);
        len = strlen(requrl);
        if(len >= urlbuf)
            requrl[urlbuf-1] = '\0';
        else
            requrl[len] = '\0';
    }
}
void optreqfile(const char **arg, unsigned int *index, char *requrl, 
                long urlbuf)
{
    char    rfile[FILE_BUF];
    char    *line = NULL;
    FILE    *reqfile;
    size_t  linelen = 0;
    
    if(arg[(*index)+1] == NULL || *(arg[(*index)+1]) == '-'){
        fprintf(stderr, "%s: ERR: %s: %s\n", NAME, "missing request file", 
                arg[*index]);
        usage();
    }
    
    memset(rfile, 0, FILE_BUF);
    strncpy(rfile, arg[++(*index)], FILE_BUF);
    rfile[FILE_BUF-1] = '\0';
    
    if((reqfile = fopen(rfile, "r")) == NULL){
        fprintf(stderr, "%s: ERR: %s: %s: %s: %s\n", NAME, "request file", 
                "fopen", rfile, strerror(errno));
        exit(1);
    }
  
    getreqline(line, linelen, reqfile, requrl, urlbuf);
    fclose(reqfile);
}
void optscheme(const char **arg, unsigned int *index, char *scheme)
{
    int i, flag = 0;
    char httpscheme[MAX_SCHEMES][REQ_SCHEME_SZ] = {"http://", "https://"};

    if(arg[(*index)+1] == NULL || *(arg[(*index)+1]) == '-'){
        fprintf(stderr, "%s: ERR: %s: missing scheme\n", NAME, arg[*index]);
        usage();
    }
    
    if(!scheme) return;
    
    strncpy(scheme, arg[++(*index)], REQ_SCHEME_SZ);
    
    for(i = 0; i != MAX_SCHEMES; ++i)
    {
        if(strncasecmp(scheme, httpscheme[i], strlen(scheme)+1) == 0){
            flag = 1;
            break;
        }
    }

    if(!flag){
        fprintf(stderr, "%s: ERR: %s: invalid scheme\n", NAME, 
                arg[(*index)-1]);
        exit(1);
    }
}
void optverb(const char **arg, unsigned int *index, char *verb)
{
    int   i, flag = 0;
    char  httpverb[MAX_VERBS][REQ_VERB_SZ] = {"CONNECT", "GET", "HEAD", "OPTIONS"};

    if(arg[(*index)+1] == NULL || *(arg[(*index)+1]) == '-'){
        fprintf(stderr, "%s: ERR: %s: missing verb\n", NAME, arg[*index]);
        usage();
    }

    strncpy(verb, arg[++(*index)], REQ_VERB_SZ);
    
    for(i = 0; i != MAX_VERBS; ++i)
    {
        if(strncasecmp(verb, httpverb[i], strlen(verb)+1) == 0){
            flag = 1;
            break;
        }
    }

    if(!flag){
        fprintf(stderr, "%s: ERR: %s: invalid verb\n", NAME, 
                arg[(*index)-1]);
        exit(1);
    }
}
int optverbosity(const char **arg, unsigned int *index)
{
    int level; 
  
    if(arg[(*index)+1] == NULL || *(arg[(*index)+1]) == '-'){
        fprintf(stderr, "%s: ERR: %s: %s\n", NAME, "missing verbosity level",
                arg[*index]);
        usage();
    }
    
    level = atoi(arg[++(*index)]);
    
    if(level < LOG_LEVEL_MIN || level > LOG_LEVEL_MAX){
        fprintf(stderr, "%s: ERR: %s %d %s\n", NAME, "verbosity level (", 
                level, ") not in range.");
        exit(1);
    }
    
    return level;
}
void optversion(char *version)
{
    memset(version, 0, VER_BUF);
    
    strncpy(version, VERSION, VER_BUF);
    
    if(strnlen(version, VER_BUF) >= VER_BUF)
        version[VER_BUF-1] = '\0';
    else
        version[strlen(version)] = '\0';
}
int procresp(const char *response, int check_httpconn)
{
    char    httpver[RESP_PROC_SZ], httpres[RESP_PROC_SZ];
    int     status = 0;

#if DEBUG_PROCRESP
    char ts[TIME_SZ];
    gettime(ts);
    fprintf(stderr, "%s: %s: [DEBUG]: response: processing:\n%s\n", ts, NAME, response);
#if DEBUG_FORCE_EXIT
    exit(1);
#endif
#endif

    if(check_httpconn == 1){
        sscanf(response, "%s %d %s\r\n", httpver, &status, httpres);
    }else{
        sscanf(response, "%s %d %s\r\n", httpver, &status, httpres);
    }

    return status;
}
ssize_t readreq(int sd, char *response, size_t buf, unsigned int r_tmout, 
                unsigned int dorespbuf)
{
    size_t          nbytes;
    struct timeval  timeout;

    timeout.tv_sec = r_tmout;
    timeout.tv_usec = 0;

#if defined(__CYGWIN__)
    char    ts[TIME_SZ];
    int     rd;
    fd_set  readfds;
    FD_ZERO(&readfds);
    FD_SET(sd, &readfds);

    rd = select(sd+1, &readfds, NULL, NULL, &timeout);

    if(rd == -1){
        gettime(ts);
        fprintf(stderr, "%s: %s: ERR: %s: %s: %s\n", ts, NAME, "select", 
                strerror(errno));
        exit(1);
    }else if(rd == 0){
        nbytes = -1;
        errno = 11; /* Resource temporarily unavailable */
        return nbytes;
    }
#else
    nbytes = setsockopt(sd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout,
                        sizeof(timeout));
#endif

    if(nbytes == -1) return nbytes;

    memset(response, 0, buf);
    nbytes = read(sd, response, buf);

    if(nbytes){
        if(nbytes >= buf)
            response[buf-1] = '\0';
        else
            response[strlen(response)] = '\0';
    }

    if(!dorespbuf){
        char *c; 
        if((c = strchr(response, '\r')) != NULL) *c = '\0';
    }

#if DEBUG_READREQ
    char ts[TIME_SZ];

    gettime(ts);
    fprintf(stderr, "%s: %s: [DEBUG]: request: reading:\n%s\n", ts, NAME, response);
#if DEBUG_FORCE_EXIT
    exit(1);
#endif
#endif

    return nbytes;
}
void stripline(char *line)
{
    char    delim = REQ_TOKENS[0]; /* match a value in REQ_TOKENS */
    int     len;

    if(line == NULL) 
        return;
    
    len = strlen(line);
    
    if(line[len-1] == '\n' || line[len - 1] == '\r') 
        line[len-1] = delim;
}
void usage(void)
{
    fprintf(stderr, "Usage:\n\t%s %s\n\t%s %s", NAME, 
            "[options] <arguments>", NAME, 
            "<arguments> [options]\n");

    /* Begin options */

    fputs("\n[Options]\n\n", stderr);
    fprintf(stderr, "%s%s%s%s%s%s%d%s%s%s%s%s%s%d%s%d%s%s",
            "-B [resp-buf]",
            "\t\t# The response buffer to allocate for each request. By\n",
            "\t\t\t# default, a sufficient buffer is allocated to hold\n",
            "\t\t\t# HTTP response status only. If the option is provided\n",
            "\t\t\t# and [resp-buf] is omitted, then the buffer will be\n",
            "\t\t\t# increased to ", RESP_SZ_DEF, " bytes. Setting the ",
            "buffer can be\n", 
            "\t\t\t# useful when using -V to view responses or following\n",
            "\t\t\t# redirects (see: -F).\n",
            "\t\t\t# [NOTE]: When [resp-buf] is provided, the range is:\n",
            "\t\t\t# ", RESP_SZ_MIN, " bytes - ", RESP_SZ_MAX, " bytes.\n",
            "\t\t\t#\n");
    fprintf(stderr, "%s%s%s%s%s%d%s%s%s%s%s%s%s%s",
            "-D [interval]", 
            "\t\t# The amount of time (in seconds) to sleep between\n",
            "\t\t\t# processing requests via <proxy-server>. Should\n",
            "\t\t\t# [interval] be omitted, the amount of time sleeping\n",
            "\t\t\t# is ", DAEMON_SLEEP_DEF, " seconds.\n",
            "\t\t\t# [NOTE]: Tool will run as a background daemon process.\n",
            "\t\t\t# [NOTE]: If -L is specified, events are written to\n",
            "\t\t\t# <request-log>. Otherwise, the events are written to\n",
            "\t\t\t# the default log: \"", DAEMON_LOG_DEF, "\".\n",
            "\t\t\t#\n");
    fprintf(stderr, "%s%s%s%s%s%s%s%s%s%s%s%s%s",
            "-F <resp-status ...>", 
            "\t# Attempt to follow one or more HTTP status codes. Must\n",
            "\t\t\t# consist of one or more of the following:\n",
            "\t\t\t#\t- 301\n",
            "\t\t\t#\t- 302\n",
            "\t\t\t#\t- 303\n",
            "\t\t\t#\t- 307\n",
            "\t\t\t# [NOTE]: Valid delimiters are the same as <request>.\n",
            "\t\t\t# [NOTE]: Due to where the Location header might be\n",
            "\t\t\t# located among the HTTP response headers, using the\n",
            "\t\t\t# -B option can be useful to ensure the response buffer\n",
            "\t\t\t# is adequate to contain the Location header.\n",
            "\t\t\t#\n");
    fprintf(stderr, "%s%s%s%s%s%s%s%s%s%s%s%s%s",
            "-H <header...>", 
            "\t\t# One or more headers to include in the request. If\n",
            "\t\t\t# using more than one, they should be appended.\n",
            "\t\t\t# Supported headers:\n",
            "\t\t\t#\t- A or a (adds Accept header)\n",
            "\t\t\t#\t- C or c (adds Connection header)\n",
            "\t\t\t#\t- H or h (adds Host header)\n",
            "\t\t\t#\t- U or u (adds User-Agent header)\n",
            "\t\t\t# [NOTE]: Some servers expect certain headers (such\n",
            "\t\t\t# as a Host header) and might respond with an error\n",
            "\t\t\t# (e.g. HTTP 400). In such cases, adding a header is\n",
            "\t\t\t# advised to minimize false results.\n",
            "\t\t\t#\n");
    fprintf(stderr, "%s%s%s%s%s%d%s%d%s%s%d%s%s",
            "-I <sec>", 
            "\t\t# The amount of time (in seconds) to wait for input\n",
            "\t\t\t# from a request to be retrieved via <proxy-server>.\n",
            "\t\t\t# This timer is reset each time data is read via the\n",
            "\t\t\t# socket. Valid range is between: ", 
            READ_TM_MIN, " - ", READ_TM_MAX, " seconds.\n",
            "\t\t\t# When omitted, it defaults to: ", READ_TM_DEF, 
            " seconds.\n", "\t\t\t#\n");
    fprintf(stderr, "%s%s%s%s%s%s",
            "-L <request-log>", 
            "\t# Send stdout and stderr streams to <request-log>. If\n",
            "\t\t\t# <request-log> does not exist, it will be created\n",
            "\t\t\t# or an error is returned. If <request-log> does\n",
            "\t\t\t# exist, it is appended to.\n",
            "\t\t\t#\n");
    fprintf(stderr, "%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s",
            "-M <method>", 
            "\t\t# HTTP method to use for <request>. Must be one of:\n",
            "\t\t\t#  - CONNECT\n", 
            "\t\t\t#  - GET\n", 
            "\t\t\t#  - HEAD\n", 
            "\t\t\t#  - OPTIONS\n", 
            "\t\t\t# If omitted, an HTTP GET will be used. If CONNECT\n",
            "\t\t\t# is specified, then HTTP GET will be used for the\n",
            "\t\t\t# subsequent request. When CONNECT is specified and\n",
            "\t\t\t# a scheme exists in <request>, then HTTP CONNECT\n", 
            "\t\t\t# will be implied and an HTTP GET will be used for\n",
            "\t\t\t# the follow-up request. When <scheme> is HTTPS://\n",
            "\t\t\t# and not already present in <request>, CONNECT will\n",
            "\t\t\t# be used, followed by <method>. Otherwise, <method>\n",
            "\t\t\t# will be used for <request>.\n",
            "\t\t\t# [NOTE]: <method> is not case-sensitive.\n",
            "\t\t\t#\n");
    fprintf(stderr, "%s%s%s%s%s%d%s%d%s%s%d%s%s",
            "-O <sec>", 
            "\t\t# The amount of time (in seconds) to wait for output\n",
            "\t\t\t# to be sent via <proxy-server>. The timer is reset\n",
            "\t\t\t# each time data is sent via the socket. If specified,\n",
            "\t\t\t# the range is between: ", 
            WRITE_TM_MIN, " - ", WRITE_TM_MAX, " seconds.\n", 
            "\t\t\t# When omitted, defaults to: ", WRITE_TM_DEF, " seconds.\n",
            "\t\t\t#\n");
    fprintf(stderr, "%s%s%s%s%s%d%s%s%d%s%s", 
            "-P <request-port>",
            "\t# Forces <request-port> to override the default port\n",
            "\t\t\t# used where a port does not exist in <request>.\n",
            "\t\t\t# A request will default to:\n", 
            "\t\t\t#\t- ", REQ_HTTP_PORT_DEF, " (HTTP:// scheme)\n",
            "\t\t\t#\t- ", REQ_HTTPS_PORT_DEF, " (HTTPS:// scheme)\n",
            "\t\t\t#\n");
    fprintf(stderr, "%s%s%s%s%s%s%s%s%s", 
            "-R <request-file>",
            "\t# Proceses <request-file> containing domain(s)/URL(s)\n", 
            "\t\t\t# to send via -s and -p arguments. If this option is\n",
            "\t\t\t# specified, it overrides -r argument as long as the\n",
            "\t\t\t# option appears before -r in the argument list. The\n",
            "\t\t\t# requests can be entered one per line and/or many\n",
            "\t\t\t# per line. In the latter case, they should follow\n",
            "\t\t\t# legal delimiters (see: -r argument).\n",
            "\t\t\t#\n");
    fprintf(stderr, "%s%s%s%s%s%s%s",
            "-S <scheme>",
            "\t\t# The scheme to use for <request>. It must be one of\n",
            "\t\t\t# HTTP:// or HTTPS://. If a scheme is present within\n",
            "\t\t\t# <request>, then <scheme> is ignored. Otherwise,\n",
            "\t\t\t# <scheme> is applied to domain/URL in <request>.\n",
            "\t\t\t# [NOTE]: <scheme> is not case-sensitive.\n",
            "\t\t\t#\n");
    fprintf(stderr, "%s%s%s%s%s%s%s%s%s",
            "-V <verbosity-level>",
            "\t# The verbosity level for output. It must be one of\n",
            "\t\t\t#\t- 1 (normal output and request payload only)\n",
            "\t\t\t#\t- 2 (normal output and response payload only)\n",
            "\t\t\t#\t- 3 (combines #1 and #2)\n",
            "\t\t\t# [NOTE]: Requests/Responses are sent to stderr\n",
            "\t\t\t# [NOTE]: When -L is used, they will be directed to\n",
            "\t\t\t# <request-log>; redirect stderr if needed.\n",
            "\t\t\t#\n");

    /* Begin arguments */

    fputs("\n[Arguments]\n\n", stderr);
    fprintf(stderr, "%s%s%s",
            "-h",
            "\t\t\t# Output this menu and exit.\n",
            "\t\t\t#\n");
    fprintf(stderr, "%s%s%s",
            "-p <proxy-port>", 
            "\t\t# The proxy server's listening TCP port.\n",
            "\t\t\t#\n");
    fprintf(stderr, 
            "%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s",
            "-r [<request ...>]", 
            "\t# Domain(s)/URL(s) to send via -s and -p arguments. If\n",
            "\t\t\t# you decide to send multiple requests, they can be\n",
            "\t\t\t# enclosed in quotes and delimted by:\n",
            "\t\t\t#\t- commas\n",
            "\t\t\t#\t- pipes\n",
            "\t\t\t#\t- semi-colons\n",
            "\t\t\t#\t- spaces\n",
            "\t\t\t# When <request> is omitted, requests are retrieved\n",
            "\t\t\t# via standard input. This can be achieved with two\n",
            "\t\t\t# main approaches:\n",
            "\t\t\t#\t1. Manually entering via this program's stdin\n",
            "\t\t\t#\t2. Piping stdout into this program's stdin\n"
            "\t\t\t# For approach #1, requests can be manually entered:\n",
            "\t\t\t#\tA. Several on one line (following delimiters)\n",
            "\t\t\t#\tB. One per line\n"
            "\t\t\t# The above can be combined as well. Sending an EOF\n",
            "\t\t\t# (end-of-file) will complete the input and allow\n",
            "\t\t\t# for further processing of requests.\n",
            "\t\t\t# For approach #2, requests can be sent via stdout of\n",
            "\t\t\t# one process to the stdin of this tool. For example:\n", 
            "\t\t\t#\tA. echo \"request1...requestN\" | ", NAME, " ...\n",
            "\t\t\t#\tB. cat request.txt | ", NAME, " ...\n",
            "\t\t\t# [Note]: For approach 2B, requests can be processed,\n",
            "\t\t\t# following the same rules as approach #1A and/or #1B.\n",
            "\t\t\t# [Note]: If <request> exists and either approach #1 or\n",
            "\t\t\t# #2 is attempted, <request> takes precedence and other\n",
            "\t\t\t# request processing methods are ignored.\n",
            "\t\t\t# [Note]: If -R option is used and it appears before\n",
            "\t\t\t# this argument, then -R takes priority.\n",
            "\t\t\t#\n");
    fprintf(stderr, "%s%s%s",
            "-s <proxy-server>",
            "\t# The proxy server to send requests through.\n",
            "\t\t\t#\n");
    fprintf(stderr, "%s%s%s",
            "-v",
            "\t\t\t# Output current program version and exit.\n",
            "\t\t\t#\n");

    exit(1);
}
size_t writereq(int sd, char *request, size_t len, unsigned int w_tmout)
{
    size_t          nbytes;
    struct timeval  timeout;

#if DEBUG_WRITEREQ
    char ts[TIME_SZ];

    gettime(ts);
    fprintf(stderr, "%s: %s: [DEBUG]: request: writing:\n%s\n", ts, NAME, request);
#if DEBUG_FORCE_EXIT
    exit(1);
#endif
#endif

    timeout.tv_sec = w_tmout;
    timeout.tv_usec = 0;

    nbytes = setsockopt(sd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout,
                        sizeof(timeout));

    if(nbytes == -1)
        return nbytes;

    nbytes = write(sd, request, len);

    return nbytes;
}
