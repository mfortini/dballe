#include <Python.h>
#include <dballe/core/record.h>
#include <dballe/core/defs.h>
#include "record.h"
#include "common.h"
#include "var.h"
#include <vector>

#if PY_MAJOR_VERSION >= 3
    #define PyInt_FromLong PyLong_FromLong
    #define PyInt_AsLong PyLong_AsLong
    #define PyInt_Check PyLong_Check
    #define Py_TPFLAGS_HAVE_ITER 0
#endif

using namespace std;
using namespace dballe;
using namespace dballe::python;
using namespace wreport;

extern "C" {

static const char* level_keys[4] = { "leveltype1", "l1", "leveltype2", "l2" };
static const char* trange_keys[3] = { "pindicator", "p1", "p2" };

/*
 * Record iterator
 */

typedef struct {
    PyObject_HEAD
    // Pointer to the container record, to hold a reference to it
    dpy_Record* rec;
    std::vector<wreport::Var*>::const_iterator iter;
} dpy_RecordIter;

static void dpy_RecordIter_dealloc(dpy_RecordIter* self)
{
    Py_DECREF(self->rec);
}

static PyObject* dpy_RecordIter_iter(dpy_RecordIter* self)
{
    Py_INCREF(self);
    return (PyObject*)self;
}

static PyObject* dpy_RecordIter_iternext(dpy_RecordIter* self)
{
    if (self->iter == core::Record::downcast(*self->rec->rec).vars().end())
    {
        PyErr_SetNone(PyExc_StopIteration);
        return NULL;
    }
    wreport::Varcode result = (*self->iter)->code();
    ++(self->iter);
    return format_varcode(result);
}

PyTypeObject dpy_RecordIter_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "dballe.RecordIter",           // tp_name
    sizeof(dpy_RecordIter),        // tp_basicsize
    0,                         // tp_itemsize
    (destructor)dpy_RecordIter_dealloc, // tp_dealloc
    0,                         // tp_print
    0,                         // tp_getattr
    0,                         // tp_setattr
    0,                         // tp_compare
    0,                         // tp_repr
    0,                         // tp_as_number
    0,                         // tp_as_sequence
    0,                         // tp_as_mapping
    0,                         // tp_hash
    0,                         // tp_call
    0,                         // tp_str
    0,                         // tp_getattro
    0,                         // tp_setattro
    0,                         // tp_as_buffer
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_ITER, // tp_flags
    "DB-All.e Record Iterator", // tp_doc
    0,                         // tp_traverse
    0,                         // tp_clear
    0,                         // tp_richcompare
    0,                         // tp_weaklistoffset
    (getiterfunc)dpy_RecordIter_iter,      // tp_iter
    (iternextfunc)dpy_RecordIter_iternext, // tp_iternext
    0,                         // tp_methods
    0,                         // tp_members
    0,                         // tp_getset
    0,                         // tp_base
    0,                         // tp_dict
    0,                         // tp_descr_get
    0,                         // tp_descr_set
    0,                         // tp_dictoffset
    0,                         // tp_init
    0,                         // tp_alloc
    0,                         // tp_new
};


/*
 * Record
 */

static int dpy_Record_setitem(dpy_Record* self, PyObject *key, PyObject *val);
static int dpy_Record_contains(dpy_Record* self, PyObject *value);
static PyObject* dpy_Record_getitem(dpy_Record* self, PyObject* key);

/*
static PyObject* dpy_Var_code(dpy_Var* self, void* closure) { return format_varcode(self->var.code()); }
static PyObject* dpy_Var_isset(dpy_Var* self, void* closure) {
    if (self->var.isset())
        return Py_True;
    else
        return Py_False;
}
*/

static PyGetSetDef dpy_Record_getsetters[] = {
    //{"code", (getter)dpy_Var_code, NULL, "variable code", NULL },
    //{"isset", (getter)dpy_Var_isset, NULL, "true if the value is set", NULL },
    {NULL}
};

static PyObject* dpy_Record_copy(dpy_Record* self)
{
    dpy_Record* result = PyObject_New(dpy_Record, &dpy_Record_Type);
    if (!result) return NULL;
    result = (dpy_Record*)PyObject_Init((PyObject*)result, &dpy_Record_Type);
    result->rec = self->rec->clone().release();
    result->station_context = self->station_context;
    return (PyObject*)result;
}

static PyObject* dpy_Record_keys(dpy_Record* self)
{
    const std::vector<wreport::Var*>& vars = core::Record::downcast(*self->rec).vars();

    PyObject* result = PyTuple_New(vars.size());
    if (!result) return NULL;

    for (size_t i = 0; i < vars.size(); ++i)
    {
        PyObject* v = format_varcode(vars[i]->code());
        if (!v)
        {
            Py_DECREF(result);
            return NULL;
        }

        if (!PyTuple_SetItem(result, i, v) == -1)
        {
            // No need to Py_DECREF v, because PyTuple_SetItem steals its
            // reference
            Py_DECREF(result);
            return NULL;
        }
    }
    return result;
}

static PyObject* dpy_Record_var(dpy_Record* self, PyObject* args, PyObject* kw)
{
    static char* kwlist[] = { "code", NULL };
    const char* name = NULL;

    if (!PyArg_ParseTupleAndKeywords(args, kw, "|s", kwlist, &name))
        return NULL;

    try {
        if (name == NULL)
            name = self->rec->enq("var", "");

        return (PyObject*)var_create((*self->rec)[name]);
    } catch (wreport::error& e) {
        return raise_wreport_exception(e);
    } catch (std::exception& se) {
        return raise_std_exception(se);
    }
}

static PyObject* dpy_Record_vars(dpy_Record* self)
{
    const std::vector<wreport::Var*>& vars = core::Record::downcast(*self->rec).vars();

    PyObject* result = PyTuple_New(vars.size());
    if (!result) return NULL;

    try {
        for (size_t i = 0; i < vars.size(); ++i)
        {
            PyObject* v = (PyObject*)var_create(*vars[i]);
            if (!v)
            {
                Py_DECREF(result);
                return NULL;
            }

            if (!PyTuple_SetItem(result, i, v) == -1)
            {
                Py_DECREF(result);
                return NULL;
            }
        }
        return result;
    } catch (wreport::error& e) {
        return raise_wreport_exception(e);
    } catch (std::exception& se) {
        return raise_std_exception(se);
    }
}

static PyObject* dpy_Record_key(dpy_Record* self, PyObject* args)
{
    const char* name = NULL;

    if (!PyArg_ParseTuple(args, "s", &name))
        return NULL;

    try {
        return (PyObject*)var_create((*self->rec)[name]);
    } catch (wreport::error& e) {
        return raise_wreport_exception(e);
    } catch (std::exception& se) {
        return raise_std_exception(se);
    }
}

static PyObject* dpy_Record_get(dpy_Record* self, PyObject *args, PyObject* kw)
{
    static char* kwlist[] = { "key", "default", NULL };
    PyObject* key;
    PyObject* def = Py_None;

    if (!PyArg_ParseTupleAndKeywords(args, kw, "O|O", kwlist, &key, &def))
        return NULL;

    int has = dpy_Record_contains(self, key);
    if (has < 0) return NULL;
    if (!has)
    {
        Py_INCREF(def);
        return def;
    }

    return dpy_Record_getitem(self, key);
}

static PyObject* dpy_Record_update(dpy_Record* self, PyObject *args, PyObject *kw)
{
    if (kw)
    {
        PyObject *key, *value;
        Py_ssize_t pos = 0;

        while (PyDict_Next(kw, &pos, &key, &value))
            if (dpy_Record_setitem(self, key, value) < 0)
                return NULL;
    }

    Py_RETURN_NONE;
}

static PyObject* dpy_Record_date_extremes(dpy_Record* self)
{
    if (PyErr_WarnEx(PyExc_DeprecationWarning, "date_extremes may disappear in a future version of DB-All.e, and no replacement is planned", 1))
        return NULL;

    DatetimeRange dtr = core::Record::downcast(*self->rec).get_datetimerange();

    PyObject* dt_min = datetime_to_python(dtr.min);
    PyObject* dt_max = datetime_to_python(dtr.max);

    if (!dt_min || !dt_max)
    {
        Py_XDECREF(dt_min);
        Py_XDECREF(dt_max);
        return NULL;
    }

    return Py_BuildValue("(NN)", dt_min, dt_max);
}

static PyObject* dpy_Record_set_station_context(dpy_Record* self)
{
    if (PyErr_WarnEx(PyExc_DeprecationWarning, "Record.set_station_context is deprecated in favour of using DB.query_station_data", 1))
        return NULL;
    self->rec->set_datetime(Datetime());
    self->rec->set_level(Level());
    self->rec->set_trange(Trange());
    self->station_context = true;
    Py_RETURN_NONE;
}

static PyObject* dpy_Record_clear(dpy_Record* self)
{
    self->rec->clear();
    self->station_context = false;
    Py_RETURN_NONE;
}

static PyObject* dpy_Record_clear_vars(dpy_Record* self)
{
    core::Record::downcast(*self->rec).clear_vars();
    Py_RETURN_NONE;
}

static PyObject* dpy_Record_set_from_string(dpy_Record* self, PyObject *args)
{
    const char* str = NULL;
    if (!PyArg_ParseTuple(args, "s", &str))
        return NULL;

    try {
        core::Record::downcast(*self->rec).set_from_string(str);
        self->station_context = false;
        Py_RETURN_NONE;
    } catch (wreport::error& e) {
        return raise_wreport_exception(e);
    } catch (std::exception& se) {
        return raise_std_exception(se);
    }
}

static PyMethodDef dpy_Record_methods[] = {
    {"clear", (PyCFunction)dpy_Record_clear, METH_NOARGS, "remove all data from the record" },
    {"clear_vars", (PyCFunction)dpy_Record_clear_vars, METH_NOARGS, "remove all variables from the record, leaving the keywords intact" },
    {"key", (PyCFunction)dpy_Record_key, METH_VARARGS, "return a var key from the record"},
    {"get", (PyCFunction)dpy_Record_get, METH_VARARGS | METH_KEYWORDS, "lookup a value, returning a fallback value (None by default) if unset" },
    {"copy", (PyCFunction)dpy_Record_copy, METH_NOARGS, "return a copy of the Record" },
    {"var", (PyCFunction)dpy_Record_var, METH_VARARGS | METH_KEYWORDS, "return a variable from the record. If no varcode is given, use record['var']" },
    {"keys", (PyCFunction)dpy_Record_keys, METH_NOARGS, "return a sequence with all the varcodes of the variables set on the Record. Note that this does not include keys." },
    {"vars", (PyCFunction)dpy_Record_vars, METH_NOARGS, "return a sequence with all the variables set on the Record. Note that this does not include keys." },
    {"update", (PyCFunction)dpy_Record_update, METH_VARARGS | METH_KEYWORDS, "set many record keys/vars in a single shot, via kwargs" },
    {"date_extremes", (PyCFunction)dpy_Record_date_extremes, METH_NOARGS, "get two datetime objects with the lower and upper bounds of the datetime period in this record" },
    {"set_station_context", (PyCFunction)dpy_Record_set_station_context, METH_NOARGS, "set the date, level and time range values to match the station data context" },
    {"set_from_string", (PyCFunction)dpy_Record_set_from_string, METH_VARARGS, "set values from a 'key=val' string" },
    {NULL}
};

static int dpy_Record_init(dpy_Record* self, PyObject* args, PyObject* kw)
{
    // Construct on preallocated memory
    self->rec = Record::create().release();
    self->station_context = false;

    if (kw)
    {
        PyObject *key, *value;
        Py_ssize_t pos = 0;

        while (PyDict_Next(kw, &pos, &key, &value))
            if (dpy_Record_setitem(self, key, value) < 0)
                return -1;
    }

    return 0;
}

static void dpy_Record_dealloc(dpy_Record* self)
{
    delete self->rec;
    self->rec = 0;
}

static PyObject* dpy_Record_str(dpy_Record* self)
{
    /*
    std::string f = self->var.format("None");
    return PyString_FromString(f.c_str());
    */
    return PyUnicode_FromString("Record");
}

static PyObject* dpy_Record_repr(dpy_Record* self)
{
    /*
    string res = "Var('";
    res += varcode_format(self->var.code());
    if (self->var.info()->is_string())
    {
        res += "', '";
        res += self->var.format();
        res += "')";
    } else {
        res += "', ";
        res += self->var.format("None");
        res += ")";
    }
    return PyString_FromString(res.c_str());
    */
    return PyUnicode_FromString("Record object");
}

static PyObject* dpy_Record_getitem(dpy_Record* self, PyObject* key)
{
    string varname;
    if (string_from_python(key, varname))
        return NULL;

    // Just look at the first character to see if we need to check for python
    // API specific keys
    switch (varname[0])
    {
        case 'd':
            if (varname == "date")
            {
                if (PyErr_WarnEx(PyExc_DeprecationWarning, "date, datemin, datemax, level, trange, and timerange  may disappear as record keys in a future version of DB-All.e; no replacement is planned", 1))
                    return NULL;
                auto dt = core::Record::downcast(*self->rec).get_datetime();
                if (!dt.is_missing()) return datetime_to_python(dt);
                PyErr_SetString(PyExc_KeyError, varname.c_str());
                return NULL;
            } else if (varname == "datemin") {
                if (PyErr_WarnEx(PyExc_DeprecationWarning, "date, datemin, datemax, level, trange, and timerange  may disappear as record keys in a future version of DB-All.e; no replacement is planned", 1))
                    return NULL;
                auto dt = core::Record::downcast(*self->rec).get_datetimerange().min;
                if (!dt.is_missing()) return datetime_to_python(dt);
                PyErr_SetString(PyExc_KeyError, varname.c_str());
                return NULL;
            } else if (varname == "datemax") {
                auto dt = core::Record::downcast(*self->rec).get_datetimerange().max;
                if (!dt.is_missing()) return datetime_to_python(dt);
                PyErr_SetString(PyExc_KeyError, varname.c_str());
                return NULL;
            }
            break;
        case 'l':
            if (varname == "level")
            {
                if (PyErr_WarnEx(PyExc_DeprecationWarning, "date, datemin, datemax, level, trange, and timerange  may disappear as record keys in a future version of DB-All.e; no replacement is planned", 1))
                    return NULL;
                auto lev = core::Record::downcast(*self->rec).get_level();
                if (!lev.is_missing()) return level_to_python(lev);
                PyErr_SetString(PyExc_KeyError, varname.c_str());
                return NULL;
            }
            break;
        case 't':
            if (varname == "trange" || varname == "timerange")
            {
                if (PyErr_WarnEx(PyExc_DeprecationWarning, "date, datemin, datemax, level, trange, and timerange  may disappear as record keys in a future version of DB-All.e; no replacement is planned", 1))
                    return NULL;
                auto tr = core::Record::downcast(*self->rec).get_trange();
                if (!tr.is_missing()) return trange_to_python(tr);
                PyErr_SetString(PyExc_KeyError, varname.c_str());
                return NULL;
            }
            break;
    }

    const Var* var = self->rec->get(varname.c_str());
    if (var == NULL)
    {
        PyErr_SetString(PyExc_KeyError, varname.c_str());
        return NULL;
    }

    if (!var->isset())
    {
        PyErr_SetString(PyExc_KeyError, varname.c_str());
        return NULL;
    }

    return var_value_to_python(*var);
}

static int dpy_Record_setitem(dpy_Record* self, PyObject *key, PyObject *val)
{
    string varname;
    if (string_from_python(key, varname))
        return -1;

    // Just look at the first character to see if we need to check for python
    // API specific keys
    switch (varname[0])
    {
        case 'd':
            if (varname == "date")
            {
                if (int res = PyErr_WarnEx(PyExc_DeprecationWarning, "date, datemin, datemax, level, trange, and timerange  may disappear as record keys in a future version of DB-All.e; no replacement is planned", 1))
                    return res;
                Datetime dt;
                if (int res = datetime_from_python(val, dt)) return res;
                self->rec->set(dt);
                self->station_context = false;
                return 0;
            } else if (varname == "datemin") {
                if (int res = PyErr_WarnEx(PyExc_DeprecationWarning, "date, datemin, datemax, level, trange, and timerange  may disappear as record keys in a future version of DB-All.e; no replacement is planned", 1))
                    return res;
                DatetimeRange dtr = core::Record::downcast(*self->rec).get_datetimerange();
                if (int res = datetime_from_python(val, dtr.min)) return res;
                self->rec->set(dtr);
                self->station_context = false;
                return 0;
            } else if (varname == "datemax") {
                if (int res = PyErr_WarnEx(PyExc_DeprecationWarning, "date, datemin, datemax, level, trange, and timerange  may disappear as record keys in a future version of DB-All.e; no replacement is planned", 1))
                    return res;
                DatetimeRange dtr = core::Record::downcast(*self->rec).get_datetimerange();
                if (int res = datetime_from_python(val, dtr.max)) return res;
                self->rec->set(dtr);
                self->station_context = false;
                return 0;
            }
            break;
        case 'l':
            if (varname == "level")
            {
                if (int res = PyErr_WarnEx(PyExc_DeprecationWarning, "date, datemin, datemax, level, trange, and timerange  may disappear as record keys in a future version of DB-All.e; no replacement is planned", 1))
                    return res;
                Level lev;
                if (int res = level_from_python(val, lev))
                    return res;
                self->station_context = false;
                self->rec->set(lev);
                return 0;
            }
        case 't':
            if (varname == "trange" || varname == "timerange")
            {
                if (int res = PyErr_WarnEx(PyExc_DeprecationWarning, "date, datemin, datemax, level, trange, and timerange  may disappear as record keys in a future version of DB-All.e; no replacement is planned", 1))
                    return res;
                Trange tr;
                if (int res = trange_from_python(val, tr))
                    return res;
                self->station_context = false;
                self->rec->set(tr);
                return 0;
            }
    }

    if (val == NULL)
    {
        // del rec[val]
        self->rec->unset(varname.c_str());
        return 0;
    }

    if (PyFloat_Check(val))
    {
        double v = PyFloat_AsDouble(val);
        if (v == -1.0 && PyErr_Occurred())
            return -1;
        self->rec->set(varname.c_str(), v);
    } else if (PyInt_Check(val)) {
        long v = PyInt_AsLong(val);
        if (v == -1 && PyErr_Occurred())
            return -1;
        self->rec->set(varname.c_str(), (int)v);
    } else if (
            PyUnicode_Check(val)
#if PY_MAJOR_VERSION >= 3
            || PyBytes_Check(val)
#else
            || PyString_Check(val)
#endif
            ) {
        string value;
        if (string_from_python(val, value))
            return -1;
        self->rec->set(varname.c_str(), value.c_str());
    } else if (val == Py_None) {
        self->rec->unset(varname.c_str());
    } else {
        PyErr_SetString(PyExc_TypeError, "Expected int, float, str, unicode, or None");
        return -1;
    }
    return 0;
}

static int any_key_set(const Record& rec, const char** keys, unsigned len)
{
    for (unsigned i = 0; i < len; ++i)
        if (rec.isset(keys[i]))
            return 1;
    return 0;
}

static int dpy_Record_len(dpy_Record* self)
{
    return core::Record::downcast(*self->rec).vars().size();
}
static int dpy_Record_contains(dpy_Record* self, PyObject *value)
{
    string varname;
    if (string_from_python(value, varname))
        return -1;

    switch (varname[0])
    {
        case 'd':
            // We don't bother checking the seconds, since they default to 0 if
            // missing
            if (varname == "date")
                return !core::Record::downcast(*self->rec).get_datetime().is_missing();
            else if (varname == "datemin")
                return !core::Record::downcast(*self->rec).get_datetimerange().min.is_missing();
            else if (varname == "datemax")
                return !core::Record::downcast(*self->rec).get_datetimerange().max.is_missing();
            break;
        case 'l':
            if (varname == "level")
                return any_key_set(*self->rec, level_keys, 4);
            break;
        case 't':
            if (varname == "trange" || varname == "timerange")
                return any_key_set(*self->rec, trange_keys, 3);
            break;
    }

    return self->rec->isset(varname.c_str()) ? 1 : 0;
}

static PySequenceMethods dpy_Record_sequence = {
    (lenfunc)dpy_Record_len,         // sq_length
    0,                               // sq_concat
    0,                               // sq_repeat
    0,                               // sq_item
    0,                               // sq_slice
    0,                               // sq_ass_item
    0,                               // sq_ass_slice
    (objobjproc)dpy_Record_contains, // sq_contains
};
static PyMappingMethods dpy_Record_mapping = {
    (lenfunc)dpy_Record_len,           // __len__
    (binaryfunc)dpy_Record_getitem,    // __getitem__
    (objobjargproc)dpy_Record_setitem, // __setitem__
};

static PyObject* dpy_Record_iter(dpy_Record* self)
{
    dpy_RecordIter* result = PyObject_New(dpy_RecordIter, &dpy_RecordIter_Type);
    if (!result) return NULL;

    result = (dpy_RecordIter*)PyObject_Init((PyObject*)result, &dpy_RecordIter_Type);
    Py_INCREF(self);
    result->rec = self;
    result->iter = core::Record::downcast(*self->rec).vars().begin();
    return (PyObject*)result;
}

static PyObject* dpy_Record_richcompare(dpy_Record* a, dpy_Record* b, int op)
{
    PyObject *result;

    // Make sure both arguments are Records.
    if (!(dpy_Record_Check(a) && dpy_Record_Check(b))) {
        result = Py_NotImplemented;
        goto out;
    }

    int cmp;
    switch (op) {
        case Py_EQ: cmp = *a->rec == *b->rec; break;
        case Py_NE: cmp = *a->rec != *b->rec; break;
        default:
            // https://www.python.org/dev/peps/pep-0207/
            // If the function cannot compare the particular combination of objects, it
            // should return a new reference to Py_NotImplemented.
            result = Py_NotImplemented;
            goto out;
    }
    result = cmp ? Py_True : Py_False;

out:
    Py_INCREF(result);
    return result;
}

PyTypeObject dpy_Record_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "dballe.Record",           // tp_name
    sizeof(dpy_Record),        // tp_basicsize
    0,                         // tp_itemsize
    (destructor)dpy_Record_dealloc, // tp_dealloc
    0,                         // tp_print
    0,                         // tp_getattr
    0,                         // tp_setattr
    0,                         // tp_compare
    (reprfunc)dpy_Record_repr, // tp_repr
    0,                         // tp_as_number
    &dpy_Record_sequence,      // tp_as_sequence
    &dpy_Record_mapping,       // tp_as_mapping
    0,                         // tp_hash
    0,                         // tp_call
    (reprfunc)dpy_Record_str,  // tp_str
    0,                         // tp_getattro
    0,                         // tp_setattro
    0,                         // tp_as_buffer
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_ITER, // tp_flags
    "DB-All.e Record",         // tp_doc
    0,                         // tp_traverse
    0,                         // tp_clear
    (richcmpfunc)dpy_Record_richcompare, // tp_richcompare
    0,                         // tp_weaklistoffset
    (getiterfunc)dpy_Record_iter, // tp_iter
    0,                         // tp_iternext
    dpy_Record_methods,        // tp_methods
    0,                         // tp_members
    dpy_Record_getsetters,     // tp_getset
    0,                         // tp_base
    0,                         // tp_dict
    0,                         // tp_descr_get
    0,                         // tp_descr_set
    0,                         // tp_dictoffset
    (initproc)dpy_Record_init, // tp_init
    0,                         // tp_alloc
    0,                         // tp_new
};

}

namespace dballe {
namespace python {

dpy_Record* record_create()
{
    return (dpy_Record*)PyObject_CallObject((PyObject*)&dpy_Record_Type, NULL);
}

void register_record(PyObject* m)
{
    common_init();

    dpy_Record_Type.tp_new = PyType_GenericNew;
    if (PyType_Ready(&dpy_Record_Type) < 0)
        return;
    Py_INCREF(&dpy_Record_Type);

    dpy_RecordIter_Type.tp_new = PyType_GenericNew;
    if (PyType_Ready(&dpy_RecordIter_Type) < 0)
        return;
    Py_INCREF(&dpy_Record_Type);

    PyModule_AddObject(m, "Record", (PyObject*)&dpy_Record_Type);
}

}
}
