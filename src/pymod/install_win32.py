import shutil
import sys
from distutils import sysconfig

def main():
  if sys.platform == 'win32':
    shutil.rmtree('%s\\pacparser' % sysconfig.get_python_lib(),
                  ignore_errors=True)
    shutil.copytree('pacparser', '%s\\pacparser' % sysconfig.get_python_lib())
  else:
    print 'This script should be used only on Win32 systems.'


if __name__ == '__main__':
  main()
