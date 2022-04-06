This file is about the instructions to build (and use) pacparser on Windows.
For general information on pacparser, please have a look at README in the same
directory.

Building pacparser on Windows:
-----------------------------
*  Install MinGW64 tools through [msys2](
    https://github.com/msys2/msys2-installer/releases).

*  Rename mingw32-make.exe to make.exe.
   ```
   rename C:\msys64\mingw64\bin\mingw32-make.exe C:\msys64\mingw64\bin\make.exe
   ```

*  Add your MinGW directory's bin (`C:\msys64\mingw64\bin`) to your system path variable.

*  Install latest [python](http://www.python.org/). You'll need it to build the python module.
   

*  Git-clone the pacparser source code, or download the source code tarball from the [releases](
    https://github.com/manugarg/pacparser/releases) page, and extract it somewhere,
    say `C:\workspace`.

*  Compile pacparser and create ditribution.
   ```
   cd C:\workspace\pacparser-*
   make -C src -f Makefile.win32 dist
   ```

* Compile pacparser python module and copy required files to a directory (dist).
  ```
  make -C src -f Makefile.win32 pymod-3.9
  ```

* Compile pacparser python module and copy required files to a directory (dist).
  ```  
  make -C src -f Makefile.win32 pymod-dist-3.9
  ```

## Using pacparser on Windows:

### In C programs:

Make sure that you have pacparser.dll in the sytem path somewhere
(current directory would do just fine for testing purpose).
```
  Change to your program's directory:
  =>  cd c:\workspace\pacparser-1.3.8\examples
  
  Copy pacparser.dll here and compile your program:
  => gcc -o pactest pactest.c -lpacparser -L.
  
  Run your program:
  => pactest wpad.dat http://www.google.com www.google.com
  'PROXY proxy1.manugarg.com:3128; PROXY proxy2.manugarg.com:3128; DIRECT'
```

### In python programs:

Install pacparser python module by running install.py in the distribution
folder. You'll then be able to use pacparser python module in the following
manner:

```python
  => python
  >>> import pacparser
  >>> pacparser.init()
  >>> pacparser.parse_pac('examples/wpad.dat')
  >>> pacparser.find_proxy('http://www.google.com', 'www.google.com')
  'PROXY proxy1.manugarg.com:3128; PROXY proxy2.manugarg.com:3128; DIRECT'
  >>> pacparser.cleanup()
```
