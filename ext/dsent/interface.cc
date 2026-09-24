/* Modernized for Python 3 compatibility and proper varargs */
#include <Python.h>
#include <cstdio>
#include <map>
#include <string>
#include <vector>
#include <cassert>

#include "DSENT.h"
#include "libutil/String.h"
#include "model/Model.h"

using namespace std;
using namespace LibUtil;

static PyObject *DSENTError;
static PyObject* dsent_initialize(PyObject*, PyObject*);
static PyObject* dsent_finalize(PyObject*, PyObject*);
static PyObject* dsent_computeRouterPowerAndArea(PyObject*, PyObject*);
static PyObject* dsent_computeLinkPower(PyObject*, PyObject*);

map<String, String> params;
DSENT::Model *ms_model;

static PyMethodDef DSENTMethods[] = {
    {"initialize", dsent_initialize, METH_VARARGS, "initialize dsent"},
    {"finalize", dsent_finalize, METH_NOARGS, "finalize dsent"},
    {"computeRouterPowerAndArea", dsent_computeRouterPowerAndArea, METH_VARARGS, "compute router power"},
    {"computeLinkPower", dsent_computeLinkPower, METH_VARARGS, "compute link power"},
    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef dsentmodule = {
    PyModuleDef_HEAD_INIT, "dsent", NULL, -1, DSENTMethods
};

PyMODINIT_FUNC PyInit_dsent(void) {
    PyObject *m = PyModule_Create(&dsentmodule);
    if (m == NULL) return NULL;
    DSENTError = PyErr_NewException("dsent.error", NULL, NULL);
    Py_INCREF(DSENTError);
    PyModule_AddObject(m, "error", DSENTError);
    ms_model = nullptr;
    return m;
}

static PyObject *dsent_initialize(PyObject *self, PyObject *args) {
    const char *config_file;
    if (!PyArg_ParseTuple(args, "s", &config_file)) return NULL;
    ms_model = DSENT::initialize(config_file, params);
    Py_RETURN_NONE;
}

static PyObject *dsent_finalize(PyObject *self, PyObject *args) {
    DSENT::finalize(params, ms_model);
    ms_model = nullptr;
    Py_RETURN_NONE;
}

static PyObject *dsent_computeRouterPowerAndArea(PyObject *self, PyObject *args) {
    double freq_in;
    int in_p, out_p, vclass, vchannel, buf_data, buf_ctrl;

    if (!PyArg_ParseTuple(args, "diiiiii", &freq_in, &in_p, &out_p, &vclass, &vchannel, &buf_data, &buf_ctrl)) {
        return NULL;
    }

    params["Frequency"] = String((uint64_t)freq_in);
    params["NumberInputPorts"] = String(in_p);
    params["NumberOutputPorts"] = String(out_p);
    params["NumberVirtualNetworks"] = String(vclass);
    params["NumberVirtualChannelsPerVirtualNetwork"] = String(vchannel);
    params["NumberBuffersPerDataVC"] = String(buf_data);
    params["NumberBuffersPerCtrlVC"] = String(buf_ctrl);

    map<string, double> outputs;
    DSENT::run(params, ms_model, outputs);

    PyObject *r = PyDict_New();
    for (const auto &it : outputs) {
        PyDict_SetItemString(r, it.first.c_str(), PyFloat_FromDouble(it.second));
    }
    return r;
}

static PyObject *dsent_computeLinkPower(PyObject *self, PyObject *args) {
    double freq_in;
    int vclass, vchannel, buf_data, buf_ctrl;

    if (!PyArg_ParseTuple(args, "diiii", &freq_in, &vclass, &vchannel, &buf_data, &buf_ctrl)) {
        return NULL;
    }

    params["Frequency"] = String((uint64_t)freq_in);
    params["NumberVirtualNetworks"] = String(vclass);
    params["NumberVirtualChannelsPerVirtualNetwork"] = String(vchannel);
    params["NumberBuffersPerDataVC"] = String(buf_data);
    params["NumberBuffersPerCtrlVC"] = String(buf_ctrl);

    map<string, double> outputs;
    DSENT::run(params, ms_model, outputs);

    PyObject *r = PyDict_New();
    for (const auto &it : outputs) {
        PyDict_SetItemString(r, it.first.c_str(), PyFloat_FromDouble(it.second));
    }
    return r;
}
