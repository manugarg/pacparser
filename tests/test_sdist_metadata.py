# Copyright (C) 2007-2026 Manu Garg.
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
Regression test for the python sdist (see issue #253).

pip prepares an sdist's metadata by re-running setup.py inside the
extracted archive, where there is no git metadata. The sdist must
therefore ship everything setup.py needs to recover the right version
(a version.mk pinning it) and to build the extension (the C sources).

This test builds the sdist, checks the shipped files, then re-runs
egg_info in a git-free copy of the archive and verifies that the
regenerated metadata version matches the archive name.

Usage: python tests/test_sdist_metadata.py
"""
import glob
import os
import re
import shutil
import subprocess
import sys
import tarfile
import tempfile


def run(cmd, cwd):
    print("+ %s (in %s)" % (" ".join(cmd), cwd))
    subprocess.check_call(cmd, cwd=cwd)


def extract_sdist(sdist, dest):
    with tarfile.open(sdist) as tar:
        try:
            tar.extractall(dest, filter="data")
        except TypeError:
            # extractall() has no filter argument before Python 3.12.
            tar.extractall(dest)
    entries = glob.glob(os.path.join(dest, "*"))
    assert len(entries) == 1, "unexpected sdist layout: %r" % entries
    return entries[0]


def main():
    tests_dir = os.path.dirname(os.path.abspath(__file__))
    pymod_dir = os.path.join(os.path.dirname(tests_dir), "src", "pymod")
    py = sys.executable

    tmp_dir = tempfile.mkdtemp(prefix="pysdist-test-")
    try:
        dist_dir = os.path.join(tmp_dir, "dist")
        run([py, "setup.py", "-q", "sdist", "--dist-dir", dist_dir],
            cwd=pymod_dir)
        sdists = glob.glob(os.path.join(dist_dir, "pacparser-*.tar.gz"))
        assert len(sdists) == 1, "expected one sdist, found %r" % sdists
        sdist = sdists[0]

        archive = os.path.basename(sdist)
        m = re.match(r"pacparser-(.+)\.tar\.gz$", archive)
        assert m, "unexpected sdist name %r" % archive
        archive_version = m.group(1)
        base = "pacparser-%s" % archive_version

        with tarfile.open(sdist) as tar:
            names = set(tar.getnames())
        # The sdist must be self-contained: C sources to build the
        # extension and a version.mk pinning the version.
        for required in (
            "version.mk",
            "pacparser.c",
            "pacparser.h",
            "pac_utils.h",
            "quickjs/quickjs.c",
            "quickjs/quickjs.h",
        ):
            assert "%s/%s" % (base, required) in names, \
                "missing %s in sdist" % required

        # Re-run egg_info in a git-free copy of the sdist, as pip does
        # when preparing the metadata for installation.
        extract_dir = os.path.join(tmp_dir, "extract")
        os.mkdir(extract_dir)
        sdist_dir = extract_sdist(sdist, extract_dir)
        run([py, "setup.py", "-q", "egg_info"], cwd=sdist_dir)
        with open(os.path.join(sdist_dir, "pacparser.egg-info", "PKG-INFO")) \
                as f:
            versions = [
                line.split(":", 1)[1].strip()
                for line in f
                if line.startswith("Version:")
            ]
        assert len(versions) == 1, "no Version: line in PKG-INFO"
        metadata_version = versions[0]
        assert metadata_version == archive_version, (
            "sdist metadata version %r does not match archive version %r"
            % (metadata_version, archive_version)
        )
        print("OK: %s metadata version is %s" % (archive, metadata_version))
    finally:
        # The sdist build copies C sources into src/pymod; remove them so
        # the source tree is left as found.
        subprocess.call([py, "setup.py", "-q", "clean", "--all"],
                        cwd=pymod_dir)
        shutil.rmtree(tmp_dir, ignore_errors=True)


if __name__ == "__main__":
    main()
