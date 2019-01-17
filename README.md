# proxycheck

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

## Compile:

gcc -o proxycheck proxycheck.c


## Usage

Coming...

## Examples

Coming...
