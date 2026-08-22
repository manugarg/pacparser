# Copyright (C) 2007-2023 Manu Garg.
# Author: Manu Garg <manugarg@gmail.com>
#
# pacparser is a library that provides methods to parse proxy auto-config
# (PAC) files. Please read README file included with this package for more
# information about this library.
#
# pacparser is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.

# pacparser is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.

# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
# USA.

"""
Wrapper script around python module Makefiles. This script take care of
identifying python setup and setting up some environment variables needed by
Makefiles.
"""
import glob
import os
import platform
import re
import shlex
import setuptools
import setuptools.command.sdist
import shutil
import subprocess
import sys
import sysconfig

from distutils.command.clean import clean as _clean_cmd
from unittest.mock import patch


def setup_dir():
    return os.path.dirname(os.path.join(os.getcwd(), sys.argv[0]))


def module_path():
    py_ver = "*".join([str(x) for x in sys.version_info[0:2]])
    print("Python version: %s" % py_ver)

    builddir = os.path.join(setup_dir(), "build")
    print("Build dir: %s" % builddir)
    print(os.listdir(builddir))

    return glob.glob(os.path.join(builddir, "lib*%s*" % py_ver))[0]


def sanitize_version(ver):
    ver = ver.strip()
    # Strip first 'v' and last part from git provided versions.
    # For example, v1.3.8-12-g231 becomes v1.3.8-12.
    ver = re.sub(r"^v?([\d]{1,3}\.[\d]{1,3}\.[\d]{1,3}(-[\d]{1,3})).*$", "\\1", ver)
    # 1.3.8-12 becomes 1.3.8.dev12
    return ver.replace("-", ".dev")


def git_version():
    return sanitize_version(
        subprocess.check_output(
            "git describe --always --tags --candidate=100".split(" "), text=True
        )
    )


def pacparser_version():
    if (
        subprocess.call("git rev-parse --git-dir".split(" "), stderr=subprocess.DEVNULL)
        == 0
    ):
        return git_version()

    # Check if we have version.mk. It's added in the manual release tarball
    # (next to src/) and in the python sdist (at its root).
    for version_file in (
        os.path.join(setup_dir(), "..", "version.mk"),
        os.path.join(setup_dir(), "version.mk"),
    ):
        if os.path.exists(version_file):
            with open(version_file) as f:
                return sanitize_version(f.read().replace("VERSION=", ""))

    return sanitize_version(os.environ.get("PACPARSER_VERSION", "1.0.0"))


class DistCmd(setuptools.Command):
    """Build pacparser python distribution."""

    description = "Build pacparser python distribution."
    user_options = []

    def initialize_options(self):
        # Override to do nothing.
        pass

    def finalize_options(self):
        # Override to do nothing.
        pass

    def run(self):
        py_ver = ".".join([str(x) for x in sys.version_info[0:2]])
        pp_ver = pacparser_version()

        mach = platform.machine().lower()
        if mach == "x86_64":
            mach = "amd64"
        dist = "pacparser-python%s-%s-%s-%s" % (
            py_ver.replace(".", ""),
            pp_ver,
            platform.system().lower(),
            mach,
        )

        if os.path.exists(dist):
            shutil.rmtree(dist)
        os.mkdir(dist)
        shutil.copytree(
            os.path.join(module_path(), "pacparser"),
            dist + "/pacparser",
            ignore=shutil.ignore_patterns("*pycache*"),
        )


class SDistCmd(setuptools.command.sdist.sdist):
    """Build pacparser python source distribution."""

    description = "Build pacparser python source distribution."

    def run(self):
        # The sdist must be buildable on its own, so pull in the C sources
        # that pacparser_py.c needs to compile and link, along with a
        # version.mk pinning the version (there is no git metadata in the
        # sdist).
        for name in ("pacparser.h", "pacparser.c", "pac_utils.h"):
            shutil.copy(os.path.join("..", name), name)
        if not os.path.isdir("quickjs"):
            os.mkdir("quickjs")
            for name in ("quickjs.c", "quickjs.h"):
                shutil.copy(
                    os.path.join("..", "quickjs", name), os.path.join("quickjs", name)
                )
        with open("version.mk", "w") as f:
            f.write("VERSION=%s\n" % pacparser_version())
        setuptools.command.sdist.sdist.run(self)


class CleanCmd(_clean_cmd):
    """Clean pacparser python build artifacts."""

    def run(self):
        _clean_cmd.run(self)
        # Remove the C sources copied in by the sdist and wheel builds.
        for name in ("pacparser.h", "pacparser.c", "pac_utils.h", "version.mk"):
            if os.path.exists(name):
                os.remove(name)
        if os.path.isdir("quickjs"):
            shutil.rmtree("quickjs")


def build_c_objects():
    """Compile pacparser.o and quickjs/libquickjs.a from the C sources.

    The sdist ships the C sources but no prebuilt objects, so they have to
    be built before the _pacparser extension can be linked.
    """
    cc = shlex.split(sysconfig.get_config_var("CC") or "cc")
    ar = shlex.split(sysconfig.get_config_var("AR") or "ar")
    if not os.path.exists(os.path.join("quickjs", "libquickjs.a")):
        quickjs_obj = os.path.join("quickjs", "quickjs.o")
        if not os.path.exists(quickjs_obj):
            subprocess.check_call(
                cc + ["-fPIC", "-c", os.path.join("quickjs", "quickjs.c"),
                      "-o", quickjs_obj]
            )
        subprocess.check_call(
            ar + ["rcs", os.path.join("quickjs", "libquickjs.a"), quickjs_obj]
        )
    if not os.path.exists("pacparser.o"):
        subprocess.check_call(
            cc + ["-g", "-Wall", "-DVERSION=%s" % pacparser_version(),
                  "-Iquickjs", "-fPIC", "-c", "pacparser.c",
                  "-o", "pacparser.o"]
        )


@patch("setuptools._distutils.cygwinccompiler.get_msvcr")
def main(patched_func):
    python_home = os.path.dirname(sys.executable)

    obj_search_path = {
        "pacparser.o": ["..", "."],
        "libquickjs.a": ["../quickjs", "quickjs", "."],
    }
    found_objects = {}
    for obj, paths in obj_search_path.items():
        for path in paths:
            if os.path.exists(os.path.join(path, obj)):
                found_objects[obj] = os.path.join(path, obj)
                break

    # When building from the sdist, the C sources are present but the
    # prebuilt objects are not, so compile them.
    if len(found_objects) < len(obj_search_path) and sys.platform != "win32":
        build_c_objects()
        found_objects["pacparser.o"] = found_objects.get(
            "pacparser.o", "pacparser.o")
        found_objects["libquickjs.a"] = found_objects.get(
            "libquickjs.a", os.path.join("quickjs", "libquickjs.a"))

    extra_objects = list(found_objects.values())

    libraries = []
    extra_link_args = []

    if sys.platform == "win32":
        import distutils.cygwinccompiler

        distutils.cygwinccompiler.get_msvcr = lambda: ["vcruntime140"]
        extra_objects = ["../pacparser.o", "../quickjs/libquickjs.a"]
        libraries = ["ws2_32"]
        extra_link_args = ["-static-libgcc", "-L" + python_home]

    pacparser_module = setuptools.Extension(
        "_pacparser",
        include_dirs=["..", "."],
        sources=["pacparser_py.c"],
        libraries=libraries,
        extra_link_args=extra_link_args,
        extra_objects=extra_objects,
    )
    setuptools.setup(
        cmdclass={
            "clean": CleanCmd,
            "dist": DistCmd,
            "sdist": SDistCmd,
        },
        name="pacparser",
        version=pacparser_version(),
        description="Pacparser package",
        author="Manu Garg",
        author_email="manugarg@gmail.com",
        url="https://github.com/manugarg/pacparser",
        long_description=("python library to parse proxy auto-config (PAC) files."),
        license="LGPL",
        ext_package="pacparser",
        ext_modules=[pacparser_module],
        py_modules=["pacparser.__init__"],
    )


if __name__ == "__main__":
    main()
