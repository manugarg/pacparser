// Copyright (C) 2007 Manu Garg.
// Author: Manu Garg <manugarg@gmail.com>
//
// pacparser is a library that provides methods to parse proxy auto-config
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

#if PY_MAJOR_VERSION >= 3
  #define MOD_ERROR_VAL NULL
  #define MOD_SUCCESS_VAL(val) val
  #define MOD_INIT(name) PyMODINIT_FUNC PyInit_##name(void)
  #define MOD_DEF(ob, name, doc, methods) \
              static struct PyModuleDef moduledef = { \
                              PyModuleDef_HEAD_INIT, name, doc, -1, methods, }; \
                  ob = PyModule_Create(&moduledef);
#else
  #define MOD_ERROR_VAL
  #define MOD_SUCCESS_VAL(val)
  #define MOD_INIT(name) void init##name(void)
  #define MOD_DEF(ob, name, doc, methods) \
              ob = Py_InitModule3(name, methods, doc);
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

// Parses the PAC script string.
//
// Evaluates the PAC script string in the JavaScript context created by
// pacparser_init.
static PyObject *                          // 0 (=Failure) or 1 (=Success)
py_pacparser_parse_pac_string(PyObject *self, PyObject *args)
{
  const char *pac_script;
  if (!PyArg_ParseTuple(args, "s", &pac_script))
    return NULL;
  if (pacparser_parse_pac_string(pac_script))
    Py_RETURN_NONE;
  else
  {
    PyErr_SetString(PacparserError, "Could not parse pac script string");
    return NULL;
  }
}

// Finds proxy for the given URL and Host.
//
// Evaluates FindProxyForURL(url,host) in the JavaScript context and returns
// the result.
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

// Return pacparser version.
static PyObject *                            // Version string.
py_pacparser_version(PyObject *self, PyObject *args)
{
  return Py_BuildValue("s", pacparser_version());
}


// Destroys JavaSctipt Engine.
static PyObject *
py_pacparser_cleanup(PyObject *self, PyObject *args)
{
  pacparser_cleanup();
  Py_RETURN_NONE;
}

// Sets local ip to the given argument.
static PyObject *
py_pacparser_setmyip(PyObject *self, PyObject *args)
{
  const char *ip;
  if (!PyArg_ParseTuple(args, "s", &ip))
    return NULL;
  pacparser_setmyip(ip);
  Py_RETURN_NONE;
}

// Enables Microsoft extensions.
static PyObject *
py_pacparser_enable_microsoft_extensions(PyObject *self, PyObject *args)
{
  pacparser_enable_microsoft_extensions();
  Py_RETURN_NONE;
}

static PyMethodDef  PpMethods[] = {
  {"init", py_pacparser_init, METH_VARARGS, "initialize pacparser"},
  {"parse_pac_string", py_pacparser_parse_pac_string, METH_VARARGS,
    "parses pac script string"},
  {"find_proxy", py_pacparser_find_proxy, METH_VARARGS, "returns proxy string"},
  {"version", py_pacparser_version, METH_VARARGS, "returns pacparser version"},
  {"cleanup", py_pacparser_cleanup, METH_VARARGS, "destroy pacparser engine"},
  {"setmyip", py_pacparser_setmyip, METH_VARARGS, "set my ip address"},
  {"enable_microsoft_extensions", py_pacparser_enable_microsoft_extensions,
    METH_VARARGS, "enable Microsoft extensions"},
  {NULL, NULL, 0, NULL}
};

MOD_INIT(_pacparser)
{
  PyObject *m;
  MOD_DEF(m, "_pacparser", NULL, PpMethods)
  if(m == NULL)
      return MOD_ERROR_VAL;
  PacparserError = PyErr_NewException("pacparser.error", NULL, NULL);
  Py_INCREF(PacparserError);
  PyModule_AddObject(m, "error", PacparserError);
  return MOD_SUCCESS_VAL(m);
}
