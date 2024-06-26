[![Build Status](https://github.com/manugarg/pacparser/actions/workflows/build.yml/badge.svg)](https://github.com/manugarg/pacparser/actions/workflows/build.yml)
[![PyPI version](https://badge.fury.io/py/pacparser.svg)](https://badge.fury.io/py/pacparser)
[![PyPI Downloads](https://static.pepy.tech/badge/pacparser)](https://pepy.tech/project/pacparser)
[![Packaging status](https://repology.org/badge/tiny-repos/pacparser.svg)](https://repology.org/project/pacparser/versions)

# [Pacparser](http://pacparser.manugarg.com)
***[pacparser.manugarg.com](http://pacparser.manugarg.com)***

Pacparser is a library to parse proxy auto-config (PAC) files. Proxy auto-config
files are a vastly used proxy configuration method these days. Web browsers can
use a PAC file to determine which proxy server to use or whether to go direct
for a given URL. PAC files are written in JavaScript and can be programmed to
return different proxy methods (e.g., `"PROXY proxy1:port; DIRECT"`) depending
upon URL, source IP address, protocol, time of the day etc. PAC files introduce
a lot of possibilities. Please look at the wikipedia entry for Proxy auto-config
(<http://en.wikipedia.org/wiki/Proxy_auto-config>) for more information.

The idea behind pacparser is to make it easy to add PAC-file parsing capability
to any program (C and python supported right now). It comes as a shared C library
and a python module which can be used to make any C or python program PAC scripts
aware.

### Implementation
Pacparser makes use of the Mozilla's JavaScript interpreter SpiderMonkey to parse
PAC files (which are nothing but javascripts). Apart from that, proxy
auto-config standard assumes availability of some functions which are not
part of the standard JavaScript. Pacparser uses Mozilla's PAC implementation to
define all these functions except for a couple of dns functions which are defined by
pacparser itself. As a result, pacparser is as close to standard as it gets :)

### Install

For Python module, you can use pip. Pre-built module is available for `64-bit Linux,
Windows, MacOS-Intel, and MacOS-ARM`, for Python `3.7, 3.8, 3.9, 3.10 and 3.11`.
```
python -m pip install pacparser
python -m pip install pacparser==1.3.8.dev15 (specific version)
```

For other pre-built binaries, download them from the project's [releases](
  https://github.com/manugarg/pacparser/releases) page.
  
You can also download the latest binaries from the [Github actions](
  https://github.com/manugarg/pacparser/actions) artifcacts.

See [INSTALL](https://github.com/manugarg/pacparser/blob/master/INSTALL) for how
to compile pacparser from the source.

### How to use it?
Pacparser comes as a shared library (`libpacparser.so` on Linux, `libpacparser.dylib`
on MacOS, and pacparser.dll on windows) as well as a python module. Using it is as
easy compiling your C programs against it or importing pacparser module in your
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
Debian), FreeBSD, Mac OS X and Win32 systems.

#### Homepage
http://pacparser.manugarg.com

Author: [Manu Garg](http://github.com/manugarg)

