# proxycheck

## TODO

This is a work in progress -- All options/arguments and functionality are subject to change. That said, everything noted in this README is up-to-date and reflects the current capability (especially the "Usage" and "Examples" sections). 

## Language

Written in C

## Purpose:

A proof-of-concept tool for testing various domains/URLs via a proxy

## Overview

The idea is to provide a way to audit if a domain/URL is accessible, 
given a request being sent. This tool is designed to provide a 
way to (hopefully) quickly get results, knowing there could be a 
huge variety of domains/URLs and spot checking may not be a good 
way to proceed.

This tool is not designed to care about a successful request 
versus a failed request as it relates to an HTTP status result.

At a high level, the tool:

- parses a list of one or more requested domains/URLs
- builds the request into a list of one or more request objects
- establishes a TCP connection to a listening server and port 
(ideally this is a proxy)
- writes the built request through the socket
- reads the response
- processes the response (most notably to derive a status)
- evaluates and outputs the results

The tool evaluates an HTTP status response in a binary fasion 
from the perspective of the request either made it past the 
proxy or it didn't.

There are two high level flows, given a request:
1. CONNECT
2. STANDARD

With a CONNECT flow, the request is conducted by establishing a 
TCP connection with the server (most likely the proxy), sending 
the request through the socket, and retrieving the response. Upon 
obtaining the response, it is evaluated to determine success 
or failure. If the response is anything other than an HTTP 200, 
an error is generated and no further processing on that request 
is attempted. If the CONNECT results in an HTTP 200, then another
request is sent to the upstream server following the STANDARD
flow.

With the STANDARD flow, the request is sent via the previously 
created socket as long as it previously followed a CONNECT flow. 
Otherwise, a new socket is created with the server (most likely 
the proxy). Next, the request is sent through the socket before 
retrieving the response. The response is evaluated against one 
of several different status codes.

It is up to the user to determine what success versus failure is 
from an audit perspective. After all, it can vary on the use case:

Example: You may want to audit whether a list of domains/URLs are 
reachable. If this results in the contrary, it could mean there is 
an issue that needs to be investigated (network, proxy, logical error 
in policy, etc.). In this case, anything other than a success is most 
likely not a good thing.

Example: You may want to audit whether a list of domains/URLs are NOT 
reachable. In this case, if a request is successful, it most likely 
means an issue with the defined proxy policy ACLs. Again, most likely 
not a good thing.

In the end, the user can take the results and determine the best course 
of action, given their use case(s). The results can be fed into a tool 
designed for ingesting events. Further metrics, alerts, etc. can be created. 
Finally, action can be taken from these results if necessary or desired.

## Supported Platforms

Compiles and runs on:

- Unix (such as FreeBSD or variants like OS X)
- Linux (such as CentOS)
- Windows (requires cygwin1.dll for native use via a command prompt) or it can operate via a Cygwin terminal if preferred

## Compile:

```
gcc -o proxycheck proxycheck_main.c proxycheck.c
```
OR
```
gcc -std=<standard> -o proxycheck proxycheck_main.c proxycheck.c

Where <standard> is: c99, c11, etc
```

## Usage

```
Usage:
	proxycheck [options] <arguments>
	proxycheck <arguments> [options]

[Options]

-B [resp-buf]		# The response buffer to allocate for each request. By
			# default, a sufficient buffer is allocated to hold
			# HTTP response status only. If the option is provided
			# and [resp-buf] is omitted, then the buffer will be
			# increased to 512 bytes. Setting the buffer can be
			# useful when using -V to view responses or following
			# redirects (see: -F).
			# [NOTE]: When [resp-buf] is provided, the range is:
			# 10 bytes - 65535 bytes.
			#
-D [interval]		# The amount of time (in seconds) to sleep between
			# processing requests via <proxy-server>. Should
			# [interval] be omitted, the amount of time sleeping
			# is 3600 seconds.
			# [NOTE]: Tool will run as a background daemon process.
			# [NOTE]: If -L is specified, events are written to
			# <request-log>. Otherwise, the events are written to
			# the default log: "proxycheck.log".
			#
-F <resp-status ...>	# Attempt to follow one or more HTTP status codes. Must
			# consist of one or more of the following:
			#	- 301
			#	- 302
			#	- 303
			#	- 307
			# [NOTE]: Valid delimiters are the same as <request>.
			# [NOTE]: Due to where the Location header might be
			# located among the HTTP response headers, using the
			# -B option can be useful to ensure the response buffer
			# is adequate to contain the Location header.
			#
-H <header...>		# One or more headers to include in the request. If
			# using more than one, they should be appended.
			# Supported headers:
			#	- A or a (adds Accept header)
			#	- C or c (adds Connection header)
			#	- H or h (adds Host header)
			#	- U or u (adds User-Agent header)
			# [NOTE]: Some servers expect certain headers (such
			# as a Host header) and might respond with an error
			# (e.g. HTTP 400). In such cases, adding a header is
			# advised to minimize false results.
			#
-I <sec>		# The amount of time (in seconds) to wait for input
			# from a request to be retrieved via <proxy-server>.
			# This timer is reset each time data is read via the
			# socket. Valid range is between: 0 - 120 seconds.
			# When omitted, it defaults to: 10 seconds.
			#
-L <request-log>	# Send stdout and stderr streams to <request-log>. If
			# <request-log> does not exist, it will be created
			# or an error is returned. If <request-log> does
			# exist, it is appended to.
			#
-M <method>		# HTTP method to use for <request>. Must be one of:
			#  - CONNECT
			#  - GET
			#  - HEAD
			#  - OPTIONS
			# If omitted, an HTTP GET will be used. If CONNECT
			# is specified, then HTTP GET will be used for the
			# subsequent request. When CONNECT is specified and
			# a scheme exists in <request>, then HTTP CONNECT
			# will be implied and an HTTP GET will be used for
			# the follow-up request. When <scheme> is HTTPS://
			# and not already present in <request>, CONNECT will
			# be used, followed by <method>. Otherwise, <method>
			# will be used for <request>.
			# [NOTE]: <method> is not case-sensitive.
			#
-O <sec>		# The amount of time (in seconds) to wait for output
			# to be sent via <proxy-server>. The timer is reset
			# each time data is sent via the socket. If specified,
			# the range is between: 0 - 120 seconds.
			# When omitted, defaults to: 10 seconds.
			#
-P <request-port>	# Forces <request-port> to override the default port
			# used where a scheme does not exist in <request>.
			# A request will default to:
			#	- 80 (HTTP:// scheme)
			#	- 443 (HTTPS:// scheme)
			#
-R <request-file>	# Proceses <request-file> containing domain(s)/URL(s)
			# to send via -s and -p arguments. If this option is
			# specified, it overrides -r argument as long as the
			# option appears before -r in the argument list. The
			# requests can be entered one per line and/or many
			# per line. In the latter case, they should follow
			# legal delimiters (see: -r argument).
			#
-S <scheme>		# The scheme to use for <request>. It must be one of
			# HTTP:// or HTTPS://. If a scheme is present within
			# <request>, then <scheme> is ignored. Otherwise,
			# <scheme> is applied to domain/URL in <request>.
			# [NOTE]: <scheme> is not case-sensitive.
			#
-V <verbosity-level>	# The verbosity level for output. It must be one of
			#	- 1 (normal output and request payload only)
			#	- 2 (normal output and response payload only)
			#	- 3 (combines #1 and #2)
			# [NOTE]: Requests/Responses are sent to stderr
			# [NOTE]: When -L is used, they will be directed to
			# <request-log>; redirect stderr if needed.
			#

[Arguments]

-h			# Output this menu and exit.
			#
-p <proxy-port>		# The proxy server's listening TCP port.
			#
-r [<request ...>]	# Domain(s)/URL(s) to send via -s and -p arguments. If
			# you decide to send multiple requests, they can be
			# enclosed in quotes and delimted by:
			#	- commas
			#	- pipes
			#	- semi-colons
			#	- spaces
			# When <request> is omitted, requests are retrieved
			# via standard input. This can be achieved with two
			# main approaches:
			#	1. Manually entering via this program's stdin
			#	2. Piping stdout into this program's stdin
			# For approach #1, requests can be manually entered:
			#	A. Several on one line (following delimiters)
			#	B. One per line
			# The above can be combined as well. Sending an EOF
			# (end-of-file) will complete the input and allow
			# for further processing of requests.
			# For approach #2, requests can be sent via stdout of
			# one process to the stdin of this tool. For example:
			#	A. echo "request1...requestN" | proxycheck ...
			#	B. cat request.txt | proxycheck ...
			# [Note]: For approach 2B, requests can be processed,
			# following the same rules as approach #1A and/or #1B.
			# [Note]: If <request> exists and either approach #1 or
			# #2 is attempted, <request> takes precedence and other
			# request processing methods are ignored.
			# [Note]: If -R option is used and it appears before
			# this argument, then -R takes priority.
			#
-s <proxy-server>	# The proxy server to send requests through.
			#
-v			# Output current program version and exit.
			#
```
## Examples

Connecting to a proxy (localhost), listing on port 8080/TCP and checking "www.foobar.com": 
```
proxycheck -s localhost -p 8080 -r www.foobar.com

...
Sun Jan 20 19:40:56 2019: proxycheck: WARN: STANDARD: dst=localhost: dport=8080: action=procresp: verb=GET: domain=www.foobar.com: port=80: path=/: ver=HTTP/1.1: status:400
...
```
Notice the result was a status of HTTP 400. This can happen when a Host header is required by the server, such as when using HTTP/1.1 for example. By adding the Host header, we can see a different result:
```
proxycheck -s localhost -p 8080 -r www.foobar.com -H h

...
Sun Jan 20 19:45:25 2019: proxycheck: STANDARD: dst=localhost: dport=8080: action=procresp: verb=GET: domain=www.foobar.com: port=80: path=/: ver=HTTP/1.1: status:200
...
```
Another example showing multiple requests. This example is using the HEAD method, with various delimiters. We also add the Accept, Connection, User-Agent, and Host headers:
```
proxycheck -H acuh -s localhost -p 8080 -r "www.foobar.com; http://www.example.com/some/page.html, www.google.com|www.bing.com;github.com"

...
Sun Jan 20 19:54:12 2019: proxycheck: WARN: STANDARD: dst=localhost: dport=8080: action=procresp: verb=head: domain=www.foobar.com: port=80: path=/: ver=HTTP/1.1: status:400
Sun Jan 20 19:54:12 2019: proxycheck: WARN: STANDARD: dst=localhost: dport=8080: action=procresp: verb=head: domain=www.example.com: port=80: path=/some/page.html: ver=HTTP/1.1: status:501
Sun Jan 20 19:54:12 2019: proxycheck: WARN: STANDARD: dst=localhost: dport=8080: action=procresp: verb=head: domain=www.google.com: port=80: path=/: ver=HTTP/1.1: status:405
Sun Jan 20 19:54:12 2019: proxycheck: STANDARD: dst=localhost: dport=8080: action=procresp: verb=head: domain=www.bing.com: port=80: path=/: ver=HTTP/1.1: status:200
Sun Jan 20 19:54:12 2019: proxycheck: STANDARD: dst=localhost: dport=8080: action=procresp: verb=head: domain=github.com: port=80: path=/: ver=HTTP/1.1: status:301
...
```
We received some status warnings such as HTTP 405 and even a HTTP 501 - This example shows the previous execution, but we turn on verbosity level 3 so we can see the requests and the responses:
```
...
Sun Jan 20 19:57:05 2019: proxycheck: STANDARD: dst=localhost: dport=8080: request:
head http://www.foobar.com/ HTTP/1.1
Host: www.foobar.com
User-Agent: proxycheck
Accept: */*
Connection: close



Sun Jan 20 19:57:05 2019: proxycheck: STANDARD: dst=localhost: dport=8080: response:
HTTP/1.1 400 Bad Request
Server: nginx/1.14.1
Date: Mon, 21 Jan 2019 02:57:05 GMT
Content-Type: text/html
Content-Length: 173
Connection: close
...
Sun Jan 20 19:57:05 2019: proxycheck: WARN: STANDARD: dst=localhost: dport=8080: action=procresp: verb=head: domain=www.foobar.com: port=80: path=/: ver=HTTP/1.1: status:400
Sun Jan 20 19:57:05 2019: proxycheck: STANDARD: dst=localhost: dport=8080: request:
head http://www.example.com/some/page.html HTTP/1.1
Host: www.example.com
User-Agent: proxycheck
Accept: */*
Connection: close



Sun Jan 20 19:57:05 2019: proxycheck: STANDARD: dst=localhost: dport=8080: response:
HTTP/1.0 501 Not Implemented
Content-Type: text/html
Content-Length: 357
Connection: close
Date: Mon, 21 Jan 2019 02:57:05 GMT
Server: ECSF (oxr/83C5)
...
Sun Jan 20 19:57:05 2019: proxycheck: WARN: STANDARD: dst=localhost: dport=8080: action=procresp: verb=head: domain=www.example.com: port=80: path=/some/page.html: ver=HTTP/1.1: status:501
Sun Jan 20 19:57:05 2019: proxycheck: STANDARD: dst=localhost: dport=8080: request:
head http://www.google.com/ HTTP/1.1
Host: www.google.com
User-Agent: proxycheck
Accept: */*
Connection: close



Sun Jan 20 19:57:05 2019: proxycheck: STANDARD: dst=localhost: dport=8080: response:
HTTP/1.1 405 Method Not Allowed
Content-Type: text/html; charset=UTF-8
Referrer-Policy: no-referrer
Content-Length: 1589
Date: Mon, 21 Jan 2019 02:57:05 GMT
Connection: close
...
```
We can see that either the specififed method (HEAD) is not implemented or it is not allowed. We issue the check again but leverage the HTTP GET (which is the current default) instead of the HEAD method. Note: The verbosity level is omitted since we no longer need the level of detail:
```
proxycheck -H acuh -s localhost -p 8080 -r "www.foobar.com; http://www.example.com/some/page.html, www.google.com|www.bing.com;github.com"

...
Sun Jan 20 20:00:55 2019: proxycheck: STANDARD: dst=localhost: dport=8080: action=procresp: verb=GET: domain=www.foobar.com: port=80: path=/: ver=HTTP/1.1: status:200
Sun Jan 20 20:00:55 2019: proxycheck: WARN: STANDARD: dst=localhost: dport=8080: action=procresp: verb=GET: domain=www.example.com: port=80: path=/some/page.html: ver=HTTP/1.1: status:404
Sun Jan 20 20:00:56 2019: proxycheck: STANDARD: dst=localhost: dport=8080: action=procresp: verb=GET: domain=www.google.com: port=80: path=/: ver=HTTP/1.1: status:200
Sun Jan 20 20:00:56 2019: proxycheck: STANDARD: dst=localhost: dport=8080: action=procresp: verb=GET: domain=www.bing.com: port=80: path=/: ver=HTTP/1.1: status:200
Sun Jan 20 20:00:56 2019: proxycheck: STANDARD: dst=localhost: dport=8080: action=procresp: verb=GET: domain=github.com: port=80: path=/: ver=HTTP/1.1: status:301
...
```
In this example, there is a slower response from the server. The default timeout per response is 10 seconds, but this can still be undesireable if we have a lot of URLs/domains to process:
```
proxycheck -s localhost -p 8080 -H huac -R test-urls.txt

...
Sun Feb  3 13:25:16 2019: proxycheck: validation complete - starting
Sun Feb  3 13:25:16 2019: proxycheck: client [neo] > server [localhost/8080]
Sun Feb  3 13:25:16 2019: proxycheck: socket proto: TCP
Sun Feb  3 13:25:16 2019: proxycheck: include header=Accept
Sun Feb  3 13:25:16 2019: proxycheck: include header=Connection
Sun Feb  3 13:25:16 2019: proxycheck: include header=Host
Sun Feb  3 13:25:16 2019: proxycheck: include header=User-Agent
Sun Feb  3 13:25:16 2019: proxycheck: building request(s)
Sun Feb  3 13:25:16 2019: proxycheck: requests built: 27
Sun Feb  3 13:25:16 2019: proxycheck: request: processing
Sun Feb  3 13:25:26 2019: proxycheck: WARN: STANDARD: read=Resource temporarily unavailable: dst=localhost: dport=8080: action=readreq: verb=GET: domain=www.foobar.com: port=80: path=/: ver=HTTP/1.1
Sun Feb  3 13:25:36 2019: proxycheck: WARN: STANDARD: read=Resource temporarily unavailable: dst=localhost: dport=8080: action=readreq: verb=GET: domain=www.bar.com: port=80: path=/sample.html: ver=HTTP/1.1
Sun Feb  3 13:25:46 2019: proxycheck: WARN: STANDARD: read=Resource temporarily unavailable: dst=localhost: dport=8080: action=readreq: verb=GET: domain=baz.com: port=80: path=/: ver=HTTP/1.1
...
```
We can choose to speed processing up a bit and specify a shorter duration with "-I" - Here, we choose to wait for 2 seconds before moving on:
```
proxycheck -s localhost -p 8080 -H huac -R test-urls.txt -I 2

...
Sun Feb  3 13:30:02 2019: proxycheck: validation complete - starting
Sun Feb  3 13:30:02 2019: proxycheck: client [neo] > server [localhost/8080]
Sun Feb  3 13:30:02 2019: proxycheck: socket proto: TCP
Sun Feb  3 13:30:02 2019: proxycheck: include header=Accept
Sun Feb  3 13:30:02 2019: proxycheck: include header=Connection
Sun Feb  3 13:30:02 2019: proxycheck: include header=Host
Sun Feb  3 13:30:02 2019: proxycheck: include header=User-Agent
Sun Feb  3 13:30:02 2019: proxycheck: building request(s)
Sun Feb  3 13:30:02 2019: proxycheck: requests built: 27
Sun Feb  3 13:30:02 2019: proxycheck: request: processing
Sun Feb  3 13:30:04 2019: proxycheck: WARN: STANDARD: read=Resource temporarily unavailable: dst=localhost: dport=8080: action=readreq: verb=GET: domain=www.foobar.com: port=80: path=/: ver=HTTP/1.1
Sun Feb  3 13:30:06 2019: proxycheck: WARN: STANDARD: read=Resource temporarily unavailable: dst=localhost: dport=8080: action=readreq: verb=GET: domain=www.bar.com: port=80: path=/sample.html: ver=HTTP/1.1
Sun Feb  3 13:30:08 2019: proxycheck: WARN: STANDARD: read=Resource temporarily unavailable: dst=localhost: dport=8080: action=readreq: verb=GET: domain=baz.com: port=80: path=/: ver=HTTP/1.1
...
```
In this example, we are testing two sites entered manually. Whem we are done, we send an EOF - Take note of the HTTP status responses returned:
```
proxycheck -s localhost -p 8080 -H huac -I 2 -r
bar.com
baz.com
<EOF Given Here>
Sun Feb  3 13:33:57 2019: proxycheck: validation complete - starting
Sun Feb  3 13:33:57 2019: proxycheck: client [neo] > server [localhost/8080]
Sun Feb  3 13:33:57 2019: proxycheck: socket proto: TCP
Sun Feb  3 13:33:57 2019: proxycheck: include header=Accept
Sun Feb  3 13:33:57 2019: proxycheck: include header=Connection
Sun Feb  3 13:33:57 2019: proxycheck: include header=Host
Sun Feb  3 13:33:57 2019: proxycheck: include header=User-Agent
Sun Feb  3 13:33:57 2019: proxycheck: building request(s)
Sun Feb  3 13:33:57 2019: proxycheck: requests built: 2
Sun Feb  3 13:33:57 2019: proxycheck: request: processing
Sun Feb  3 13:33:58 2019: proxycheck: STANDARD: dst=localhost: dport=8080: action=procresp: verb=GET: domain=bar.com: port=80: path=/: ver=HTTP/1.1: status:301
Sun Feb  3 13:33:58 2019: proxycheck: STANDARD: dst=localhost: dport=8080: action=procresp: verb=GET: domain=baz.com: port=80: path=/: ver=HTTP/1.1: status:302
Sun Feb  3 13:33:58 2019: proxycheck: processing complete
Sun Feb  3 13:33:58 2019: proxycheck: cleaning up
Sun Feb  3 13:33:58 2019: proxycheck: finished
```
If we decided that we want to follow these redirects, we can use "-F" and specify the status codes we want to follow. Note: the result of "unable to follow" in each of the responses:
```
proxycheck -s localhost -p 8080 -H huac -I 2 -r -F 301,302
bar.com
baz.com
<EOF Given Here>
Sun Feb  3 13:38:04 2019: proxycheck: validation complete - starting
Sun Feb  3 13:38:04 2019: proxycheck: client [neo] > server [localhost/8080]
Sun Feb  3 13:38:04 2019: proxycheck: socket proto: TCP
Sun Feb  3 13:38:04 2019: proxycheck: include header=Accept
Sun Feb  3 13:38:04 2019: proxycheck: include header=Connection
Sun Feb  3 13:38:04 2019: proxycheck: include header=Host
Sun Feb  3 13:38:04 2019: proxycheck: include header=User-Agent
Sun Feb  3 13:38:04 2019: proxycheck: building request(s)
Sun Feb  3 13:38:04 2019: proxycheck: requests built: 2
Sun Feb  3 13:38:04 2019: proxycheck: request: processing
Sun Feb  3 13:38:06 2019: proxycheck: STANDARD: dst=localhost: dport=8080: action=procresp: verb=GET: domain=bar.com: port=80: path=/: ver=HTTP/1.1: status:301
Sun Feb  3 13:38:06 2019: proxycheck: STANDARD: dst=localhost: dport=8080: action=getlocation: location=None: unable to follow
Sun Feb  3 13:38:06 2019: proxycheck: STANDARD: dst=localhost: dport=8080: action=procresp: verb=GET: domain=baz.com: port=80: path=/: ver=HTTP/1.1: status:302
Sun Feb  3 13:38:06 2019: proxycheck: STANDARD: dst=localhost: dport=8080: action=getlocation: location=None: unable to follow
...
```
In the previous example, we tried to follow HTTP 301 and HTTP 302 redirects. However, we recieved a response of "unable to follow" - This is because the HTTP Location header was not found in the orginal response. This could be because it truly did not exist or it could be that our response buffer is not large enough to contain the Location header. To be sure, we can use "-V 2" to see what is returned in each response:
```
proxycheck -s localhost -p 8080 -H huac -I 2 -r -F "301, 302" -V 2
bar.com
baz.com
<EOF Given Here>
...
Sun Feb  3 13:42:52 2019: proxycheck: STANDARD: dst=localhost: dport=8080: response:
HTTP/1.1 301 Moved Permanently
Sun Feb  3 13:42:52 2019: proxycheck: STANDARD: dst=localhost: dport=8080: action=procresp: verb=GET: domain=bar.com: port=80: path=/: ver=HTTP/1.1: status:301
Sun Feb  3 13:42:52 2019: proxycheck: STANDARD: dst=localhost: dport=8080: action=getlocation: location=None: unable to follow
Sun Feb  3 13:42:52 2019: proxycheck: STANDARD: dst=localhost: dport=8080: response:
HTTP/1.1 302 Found
Sun Feb  3 13:42:52 2019: proxycheck: STANDARD: dst=localhost: dport=8080: action=procresp: verb=GET: domain=baz.com: port=80: path=/: ver=HTTP/1.1: status:302
Sun Feb  3 13:42:52 2019: proxycheck: STANDARD: dst=localhost: dport=8080: action=getlocation: location=None: unable to follow
...
```
Based on the previous example, we can see that it is possible our response buffer is not large enough. Let's try the requests again but specify a larger buffer. Note: We only use "-B" with its default buffer. In MOST cases, this is sufficient (we will see later why we state "MOST"). Take note that we were still unable to follow "bar.com", but we successfully followed "baz.com". We will begin to look at why "bar.com" was not successful in the next example.
```
proxycheck -s localhost -p 8080 -H huac -I 2 -r -F 301,302 -B
bar.com
baz.com
<EOF Given Here>
...
Sun Feb  3 13:50:00 2019: proxycheck: STANDARD: dst=localhost: dport=8080: action=procresp: verb=GET: domain=bar.com: port=80: path=/: ver=HTTP/1.1: status:301
Sun Feb  3 13:50:00 2019: proxycheck: STANDARD: dst=localhost: dport=8080: action=getlocation: location=None: unable to follow
Sun Feb  3 13:50:00 2019: proxycheck: STANDARD: dst=localhost: dport=8080: action=procresp: verb=GET: domain=baz.com: port=80: path=/: ver=HTTP/1.1: status:302
Sun Feb  3 13:50:00 2019: proxycheck: STANDARD: dst=localhost: dport=8080: action=getlocation: location=http://baz.com/quark/: attempting to follow previous 302
Sun Feb  3 13:50:00 2019: proxycheck: STANDARD: dst=localhost: dport=8080: action=procresp: verb=GET: domain=baz.com: port=80: path=/quark/: ver=HTTP/1.1: status:200
...
```
Recall that if we us e "-F" and we get an "unable to follow" message, it means the Location header is not found in the response. Again, this could be because the Location header is not truly provided or our buffer is not sufficiently large enough to include it. Let's just look at "bar.com" and see if we can narrow it down. To do this, let's use our verbosity option again and look at the response - IMPORTANT: Notice, how we are using the default buffer with "-B" (This is to illustrate why the default response buffer may not always be sufficient, when trying to follow redirects):
```
echo bar.com | ./proxycheck -s localhost -p 8080 -H huac -I 2 -r -F 301 -B -V 2

...
Sun Feb  3 13:56:11 2019: proxycheck: STANDARD: dst=localhost: dport=8080: response:
HTTP/1.1 301 Moved Permanently
Date: Sun, 03 Feb 2019 20:56:11 GMT
Content-Type: text/html; charset=UTF-8
Connection: close
Set-Cookie: __cfduid=d69c23a485ece9dd691d7e48737c650f61549227370; expires=Mon, 03-Feb-20 20:56:10 GMT; path=/; domain=.bar.com; HttpOnly
X-Powered-By: PHP/5.5.38
Referrer-Policy: unsafe-url
x-frame-options: DENY
X-XSS-Protection: 1; mode=block
X-Content-Type-Options: nosniff
Set-Cookie: icwp-wpsf=8afc970823e84ddd202086bc8dd16766; expires=Sat, 07-Apr-2068 17:52:20 GMT; Max-Ag
Sun Feb  3 13:56:11 2019: proxycheck: STANDARD: dst=localhost: dport=8080: action=procresp: verb=GET: domain=bar.com: port=80: path=/: ver=HTTP/1.1: status:301
Sun Feb  3 13:56:11 2019: proxycheck: STANDARD: dst=localhost: dport=8080: action=getlocation: location=None: unable to follow
...
```
In the previous example, we examined the response using just "-B" and we did not see a Location header. This explains why we could not orignally follow it. We could call it a day or we could check to see if, given a more sizeable buffer, we still do not see a Location header. This time, we increase the size of the buffer to 4096 bytes (this is an arbitrary value but we wanted something that was reasonable to hold the headers) - Note: We can't control where the server side decides to place the Location header or even that it includes it. This example demonstrates that although "-B" alone will be enough in MOST situations, it can be useful to specify a response buffer. We can see with the increased buffer that proxycheck is now able to follow the redirect as desired:
```
echo bar.com | ./proxycheck -s localhost -p 8080 -H huac -I 2 -r -F 301 -B 4096 -V 2

...
Sun Feb  3 13:57:01 2019: proxycheck: STANDARD: dst=localhost: dport=8080: response:
HTTP/1.1 301 Moved Permanently
Date: Sun, 03 Feb 2019 20:57:01 GMT
Content-Type: text/html; charset=UTF-8
Connection: close
Set-Cookie: __cfduid=da0365dd8f2dddcad3831230635239b311549227420; expires=Mon, 03-Feb-20 20:57:00 GMT; path=/; domain=.bar.com; HttpOnly
X-Powered-By: PHP/5.5.38
Referrer-Policy: unsafe-url
x-frame-options: DENY
X-XSS-Protection: 1; mode=block
X-Content-Type-Options: nosniff
Set-Cookie: icwp-wpsf=655ed0e27f3e41a7a11511fcaa0cd709; expires=Sat, 07-Apr-2068 17:54:02 GMT; Max-Age=1551819421; path=/
Location: https://bar.com/
Server: cloudflare
CF-RAY: 4a37ceb218cf3982-PHX
Content-Length: 0


Sun Feb  3 13:57:01 2019: proxycheck: STANDARD: dst=localhost: dport=8080: action=procresp: verb=GET: domain=bar.com: port=80: path=/: ver=HTTP/1.1: status:301
Sun Feb  3 13:57:01 2019: proxycheck: STANDARD: dst=localhost: dport=8080: action=getlocation: location=https://bar.com/: attempting to follow previous 301
...
```
