[![Build Status](https://travis-ci.org/manugarg/pacparser.svg?branch=master)](https://travis-ci.org/manugarg/pacparser)
[![Build status](https://ci.appveyor.com/api/projects/status/uvct988e0jx991x3?svg=true)](https://ci.appveyor.com/project/manugarg/pacparser)

# [Pacparser](http://pacparser.github.io)
***[pacparser.github.io](http://pacparser.github.io)***

pacparser is a library to parse proxy auto-config (PAC) files. Proxy auto-config
files are a vastly used proxy configuration method these days. Web browsers can
use a PAC file to determine which proxy server to use or whether to go direct
for a given URL. PAC files are written in JavaScript and can be programmed to
return different proxy methods (e.g., `"PROXY proxy1:port; DIRECT"`) depending
upon URL, source IP address, protocol, time of the day etc. PAC files introduce
a lot of possibilities. Please look at the wikipedia entry for Proxy auto-config
(<http://en.wikipedia.org/wiki/Proxy_auto-config>) for more information.

Needless to say, PAC files are now a widely accepted method for proxy
configuration management and companies all over are using them in corporate
environment. Almost all popular web browsers support PAC files. The idea behind
pacparser is to make it easy to add this PAC file parsing capability to any
program (C and python supported right now). It comes as a shared C library and
a python module which can be used to make any C or python program PAC scripts
intelligent. Some very useful targets could be popular web software like wget,
curl and python-urllib.

### Implementation
pacparser makes use of Mozilla's JavaScript interpreter SpiderMonkey to parse
PAC files (which are nothing but javascripts). Apart from that, proxy
auto-config standard assumes availability of some functions which are not
part of standard JavaScript. pacparser uses Mozilla's PAC implementation to
define all these functions except couple of dns functions which are defined by
pacparser itself. As a result, pacparser is as close to standard as it gets :)

### Install
Please see 'INSTALL' in the root directory of the package.

### How to use it?
Pacparser comes as a shared library (libpacparser.so on Unix-like systems
and pacparser.dll on windows) as well as a python module. Using it is as easy
compiling your C programs against it or importing pacparser module in your
python programs.

### Usage Examples

#### Python:
```python
>>> import pacparser
>>> pacparser.init()
>>> pacparser.parse_pac('examples/wpad.dat')
>>> pacparser.find_proxy('http://www.google.com', 'www.google.com')
'DIRECT'
>>> pacparser.setmyip("192.168.1.134")
>>> pacparser.find_proxy('http://www.google.com', 'www.google.com')
'PROXY proxy1.manugarg.com:3128; PROXY proxy2.manugarg.com:3128; DIRECT'
>>> pacparser.find_proxy('http://www2.manugarg.com', 'www2.manugarg.com')
'DIRECT'
>>> pacparser.cleanup()
>>>
```

#### C
```C
#include <stdio.h>

int pacparser_init();
int pacparser_parse_pac(char* pacfile);
char *pacparser_find_proxy(char *url, char *host);
void pacparser_cleanup();

int main(int argc, char* argv[])
{
  char *proxy;
  pacparser_init();
  pacparser_parse_pac(argv[1]);
  proxy = pacparser_find_proxy(argv[2], argv[3]);
  printf("%s\n", proxy);
  pacparser_cleanup();
}
```
```
manugarg@hobbiton:~$ gcc -o pactest pactest.c -lpacparser
manugarg@hobbiton:~$ ./pactest wpad.dat http://www.google.com www.google.com
PROXY proxy1.manugarg.com:3128; PROXY proxy2.manugarg.com:3128; DIRECT
```

#### Platforms
pacparser has been tested to work on Linux (all architectures supported by
Debian), Mac OS X and Win32 systems.

#### Homepage
<http://pacparser.github.io>

Author: Manu Garg <manugarg@gmail.com>  
Copyright (C) 2007 Manu Garg.
Copyright (C) 2015 Google, Inc.
