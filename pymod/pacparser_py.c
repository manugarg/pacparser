// Copyright (C) 2007 Manu Garg.
// Author: Manu Garg <manugarg@gmail.com>
//
// pacparser is a C library that provides methods to parse proxy auto-config
// (PAC) files. Please read README file included with this package for more
// information about this library.
//
// pacparser is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.

// pacparser is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.

// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
#include <Python.h>
#include "pacparser.h"

// PyMODINIT_FUNC macro is not defined on python < 2.3. Take care of that.
#ifndef PyMODINIT_FUNC        /* declarations for DLL import/export */
#define PyMODINIT_FUNC void
#endif

// Py_RETURN_NONE macro is not defined on python < 2.4. Take care of that.
#ifndef Py_RETURN_NONE
#define Py_RETURN_NONE return Py_INCREF(Py_None), Py_None
#endif

static PyObject *PacparserError;
// Initialize PAC parser.
//
// - Initializes JavaScript engine,
// - Exports dns_functions (defined above) to JavaScript context.
// - Sets error reporting function to print_error,
// - Evaluates JavaScript code in pacUtils variable defined in pac_utils.h.
static PyObject *                          // 0 (=Failure) or 1 (=Success)
py_pacparser_init(PyObject *self, PyObject *args)
{
  if(pacparser_init())
    Py_RETURN_NONE;
  else
  {
    PyErr_SetString(PacparserError, "Could not initialize pacparser");
    return NULL;
  }
}

// Parses PAC file
//
// parses and evaulates PAC file in the JavaScript context created by
// pacparser_init.
static PyObject *                          // 0 (=Failure) or 1 (=Success)
py_pacparser_parse_pac(PyObject *self, PyObject *args)
{
  const char *pacfile;
  if (!PyArg_ParseTuple(args, "s", &pacfile))
    return NULL;
  if (pacparser_parse_pac(pacfile))
    Py_RETURN_NONE;
  else
  {
    PyErr_SetString(PacparserError, "Could not parse pac file");
    return NULL;
  }
}

// Finds proxy for a given URL and Host.
//
// If JavaScript engine is intialized and FindProxyForURL function is defined,
// it evaluates code FindProxyForURL(url,host) in JavaScript context and
// returns the result.
static PyObject *                            // Proxy string or NULL if failed.
py_pacparser_find_proxy(PyObject *self, PyObject *args)
{
  char *proxy;
  const char *url;
  const char *host;
  if (!PyArg_ParseTuple(args, "ss", &url, &host))
    return NULL;
  if(!(proxy = pacparser_find_proxy(url, host)))
  {
    PyErr_SetString(PacparserError, "Could not find proxy");
    return NULL;
  }
  return Py_BuildValue("s", proxy);
}

// Destroys JavaSctipt Engine.
static PyObject * 
py_pacparser_cleanup()
{
  pacparser_cleanup();
  Py_RETURN_NONE;
}

static PyMethodDef  PpMethods[] = {
  {"init", py_pacparser_init, METH_VARARGS, "initialize pacparser"},
  {"parse_pac", py_pacparser_parse_pac, METH_VARARGS, "parse pacfile"},
  {"find_proxy", py_pacparser_find_proxy, METH_VARARGS, "returns proxy string"},
  {"cleanup", py_pacparser_cleanup, METH_VARARGS, "destroy pacparser engine"},
  {NULL, NULL, 0, NULL}
};

PyMODINIT_FUNC
init_pacparser(void)
{
  PyObject *m;
  m = Py_InitModule("_pacparser", PpMethods);
  PacparserError = PyErr_NewException("pacparser.error", NULL, NULL);
  Py_INCREF(PacparserError);
  PyModule_AddObject(m, "error", PacparserError);
}
