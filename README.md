# proxycheck

## TODO

This is a work in progress and all options/argumennts and functionality are subject to change. Code will be uploaded soon, once I get to a good stopping point to upload it. That said, everything noted in this README is up-to-date and reflects the current capability

## Language

Written in C

## Purpose:

A simple PoC for testing various domains/URLs via a proxy

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
from the perspective of the request either mdae it past the 
proxy or it didn't.

There are two high level flows, given a request:
1. CONNECT
2. STANDARD

With a CONNECT flow, the request is conducted by establishing a 
TCP connection with the server (most likely the proxy), sending 
the request through the socket, and retrieving the response. Upon 
obtaining the response, it is next evaluated to determine success 
or failure. If response is anything other than an HTTP 200, an 
error is generated and no further processing on that request is 
attempted. If, however, the CONNECT results in an HTTP 200, then 
another request is sent to the upstream server, following the 
STANDARD flow.

With the STANDARD flow, the request is sent via the previously 
created socket as long as it follows a CONNECT flow. Otherwise, 
a new socket the request is conducted by establishing a TCP 
connection with the server (most likely the proxy). Next, the 
request is sent through the socket, before retrieving the response.

It is up to the user to determine what success versus failure is 
from an audit perspective. After all, it can vary, on the use case:

Example: You may want audit whether a list of domains/URLs are 
reachable. If this results in the contrary, it could mean there is 
an issue that needs to be investigated (network, proxy, logical error 
in policy, etc.). In this case anything other than a success is most 
likely not a good thing.

Example: You may want audit whether a list of domains/URLs are NOT 
reachable. In this example, if a request is successful, it most likely 
means an issue with the defined proxy policy ACLs. Again, most likely 
not a good thing.

The user can take the results and determine the best course of action, 
given their use case(s).

## Supported Platforms

Compiles and runs on:

- Unix (such as FreeBSD or variants like OS X)
- Linux (such as CentOS)
- Windows (requires cygwin1.dll for native use) or Cygwin terminal **

** There is some funtionality that odes not (yet) work on Windows - this has to do with:
-I and -O options. The reason is how Windows implements sockets. So, it does not respect 
how I am manipulating the socket timeout thresholds... I will work on this.

## Compile:

gcc -o proxycheck proxycheck.c


## Usage

```
Usage:
	proxycheck [options] <arguments>
	proxycheck <arguments> [options]

[Options]

-H <header...>		# One or more headers to include in the request. If
		    	# using more than one, they should be appended.
		    	# Supported headers:
		    	#	- A or a (adds Accept header)
		    	#	- C or c (adds Host header)
		    	#	- H or h (adds Connection header)
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
-L <request-log>	# Send stdout and stderr strams to <request-log>. If
			# <request-log> does not exist, it will be created
			# or an error is returned. If <request-log> does
			# exist, it is appended to.
			#
-M <method>		# HTTP method to use for <request>. Must be one of:
			#   	- CONNECT
			#   	- GET
			#   	- HEAD
			#   	- OPTIONS
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
			#	  - 80 (HTTP:// scheme)
			#	  - 443 (HTTPS:// scheme)
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
-V <verbosity-level>	# The verbosity level for output. It must be one of
			#	  - 1 (normal output and request payload only)
			#	  - 2 (normal output and response payload only)
			#	  - 3 (combines #1 and #2)
			# [NOTE]: Requests/Responses are sent to standard
			# error (stderr)
			# [NOTE]: When -L is used, they will be directed to
			# <request-log>; redirect stderr if needed.
		   	#

[Arguments]

-p <proxy-port>		 # The proxy server's listening TCP port.
			 #
-r [<request ...>]	 # Domain(s)/URL(s) to send via -s and -p arguments. If
			 # you decide to send multiple requests, they can be
			 # enclosed in quotes and delimted by:
			 #	  - commas
			 #	  - pipes
			 #	  - semi-colons
			 #	  - spaces
			 # When <request> is omitted, requests are retrieved
			 # via standard input (stdin). This can be achieved
			 # with two main approaches:
			 #	  1. Manually via standard input (stdin)
			 #	  2. Piping standard output (stdout)
			 # For approach #1, requests can be manually entered:
			 #	  A. Several on one line (following delimiters)
			 #	  B. One per line
			 # The above can be combined as well. Sending an EOF
			 # (end-of-file) will complete the input and allow
			 # for further processing of requests.
			 # For approach #2, requests can be sent via stdout of
			 # one process to the stdin of this tool. For example:
			 #	  A. echo "request1...requestN" | proxycheck ...
			 #	  B. cat request.txt | proxycheck ...
			 # [Note]: For approach 2B, requests can be processed,
			 # following the same rules as approach #1A and/or #1B.
			 # [Note]: If <request> exists and either approach #1 or
			 # #2 is attempted, <request> takes precedence and other
			 # request processing methods are ignored.
			 # [Note]: If -R option is used and it appears before
			 # this argument, then -R takes priority.
			 #
-s <proxy-server>	 # The proxy server to send requests through.
```
## Examples

Coming...
