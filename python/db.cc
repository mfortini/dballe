/*
 * python/db - DB-All.e DB python bindings
 *
 * Copyright (C) 2013  ARPA-SIM <urpsim@smr.arpa.emr.it>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 *
 * Author: Enrico Zini <enrico@enricozini.com>
 */
#include <Python.h>
#include <datetime.h>
#include "db.h"
#include "record.h"
#include "cursor.h"
#include "common.h"
#include "dballe/core/defs.h"
#include "dballe/core/file.h"
#include "dballe/msg/msgs.h"
#include "dballe/msg/codec.h"

using namespace std;
using namespace dballe;
using namespace dballe::python;
using namespace wreport;

extern "C" {

static PyGetSetDef dpy_DB_getsetters[] = {
    //{"code", (getter)dpy_Var_code, NULL, "variable code", NULL },
    //{"isset", (getter)dpy_Var_isset, NULL, "true if the value is set", NULL },
    {NULL}
};

static PyObject* dpy_DB_connect(PyTypeObject *type, PyObject *args, PyObject* kw)
{
    static char* kwlist[] = { "dsn", "user", "password", NULL };
    const char* dsn;
    const char* user = "";
    const char* pass = "";
    if (!PyArg_ParseTupleAndKeywords(args, kw, "s|ss", kwlist, &dsn, &user, &pass))
        return NULL;

    auto_ptr<DB> db;
    try {
        db = DB::connect(dsn, user, pass);
    } catch (wreport::error& e) {
        return raise_wreport_exception(e);
    } catch (std::exception& se) {
        return raise_std_exception(se);
    }
    return (PyObject*)db_create(db);
}

static PyObject* dpy_DB_connect_from_file(PyTypeObject *type, PyObject *args)
{
    const char* fname;
    if (!PyArg_ParseTuple(args, "s", &fname))
        return NULL;

    auto_ptr<DB> db;
    try {
        db = DB::connect_from_file(fname);
    } catch (wreport::error& e) {
        return raise_wreport_exception(e);
    } catch (std::exception& se) {
        return raise_std_exception(se);
    }
    return (PyObject*)db_create(db);
}

static PyObject* dpy_DB_connect_from_url(PyTypeObject *type, PyObject *args)
{
    const char* url;
    if (!PyArg_ParseTuple(args, "s", &url))
        return NULL;

    auto_ptr<DB> db;
    try {
        db = DB::connect_from_url(url);
    } catch (wreport::error& e) {
        return raise_wreport_exception(e);
    } catch (std::exception& se) {
        return raise_std_exception(se);
    }
    return (PyObject*)db_create(db);
}

static PyObject* dpy_DB_connect_test(PyTypeObject *type)
{
    auto_ptr<DB> db;
    try {
        db = DB::connect_test();
    } catch (wreport::error& e) {
        return raise_wreport_exception(e);
    } catch (std::exception& se) {
        return raise_std_exception(se);
    }
    return (PyObject*)db_create(db);
}

static PyObject* dpy_DB_is_url(PyTypeObject *type, PyObject *args)
{
    const char* url;
    if (!PyArg_ParseTuple(args, "s", &url))
        return NULL;

    if (DB::is_url(url))
        Py_RETURN_TRUE;
    else
        Py_RETURN_TRUE;
}

static PyObject* dpy_DB_reset(dpy_DB* self, PyObject *args)
{
    const char* repinfo_file = 0;
    if (!PyArg_ParseTuple(args, "|s", &repinfo_file))
        return NULL;

    try {
        self->db->reset(repinfo_file);
    } catch (wreport::error& e) {
        return raise_wreport_exception(e);
    } catch (std::exception& se) {
        return raise_std_exception(se);
    }

    Py_RETURN_NONE;
}

/*
virtual void update_repinfo(const char* repinfo_file, int* added, int* deleted, int* updated) = 0;
virtual const std::string& rep_memo_from_cod(int rep_cod) = 0;
*/

static PyObject* dpy_DB_insert(dpy_DB* self, PyObject* args, PyObject* kw)
{
    static char* kwlist[] = { "record", "can_replace", "can_add_stations", NULL };
    dpy_Record* record;
    int can_replace = 0;
    int station_can_add = 0;
    if (!PyArg_ParseTupleAndKeywords(args, kw, "O!|ii", kwlist, &dpy_Record_Type, &record, &can_replace, &station_can_add))
        return NULL;

    try {
        self->db->insert(record->rec, can_replace, station_can_add);
        Py_RETURN_NONE;
    } catch (wreport::error& e) {
        return raise_wreport_exception(e);
    } catch (std::exception& se) {
        return raise_std_exception(se);
    }
}

static PyObject* dpy_DB_remove(dpy_DB* self, PyObject* args)
{
    dpy_Record* record;
    if (!PyArg_ParseTuple(args, "O!", &dpy_Record_Type, &record))
        return NULL;

    try {
        self->db->remove(record->rec);
    } catch (wreport::error& e) {
        return raise_wreport_exception(e);
    } catch (std::exception& se) {
        return raise_std_exception(se);
    }

    Py_RETURN_NONE;
}

static PyObject* dpy_DB_disappear(dpy_DB* self)
{
    try {
        self->db->disappear();
    } catch (wreport::error& e) {
        return raise_wreport_exception(e);
    } catch (std::exception& se) {
        return raise_std_exception(se);
    }

    Py_RETURN_NONE;
}

static PyObject* dpy_DB_vacuum(dpy_DB* self)
{
    try {
        self->db->vacuum();
    } catch (wreport::error& e) {
        return raise_wreport_exception(e);
    } catch (std::exception& se) {
        return raise_std_exception(se);
    }

    Py_RETURN_NONE;
}

static PyObject* dpy_DB_query_reports(dpy_DB* self, PyObject* args)
{
    dpy_Record* record;
    if (!PyArg_ParseTuple(args, "O!", &dpy_Record_Type, &record))
        return NULL;

    try {
        std::auto_ptr<db::Cursor> res = self->db->query_reports(record->rec);
        return (PyObject*)cursor_create(self, res);
    } catch (wreport::error& e) {
        return raise_wreport_exception(e);
    } catch (std::exception& se) {
        return raise_std_exception(se);
    }
}

static PyObject* dpy_DB_query_stations(dpy_DB* self, PyObject* args)
{
    dpy_Record* record;
    if (!PyArg_ParseTuple(args, "O!", &dpy_Record_Type, &record))
        return NULL;

    try {
        std::auto_ptr<db::Cursor> res = self->db->query_stations(record->rec);
        return (PyObject*)cursor_create(self, res);
    } catch (wreport::error& e) {
        return raise_wreport_exception(e);
    } catch (std::exception& se) {
        return raise_std_exception(se);
    }
}

static PyObject* dpy_DB_query_levels(dpy_DB* self, PyObject* args)
{
    dpy_Record* record;
    if (!PyArg_ParseTuple(args, "O!", &dpy_Record_Type, &record))
        return NULL;

    try {
        std::auto_ptr<db::Cursor> res = self->db->query_levels(record->rec);
        return (PyObject*)cursor_create(self, res);
    } catch (wreport::error& e) {
        return raise_wreport_exception(e);
    } catch (std::exception& se) {
        return raise_std_exception(se);
    }
}

static PyObject* dpy_DB_query_tranges(dpy_DB* self, PyObject* args)
{
    dpy_Record* record;
    if (!PyArg_ParseTuple(args, "O!", &dpy_Record_Type, &record))
        return NULL;

    try {
        std::auto_ptr<db::Cursor> res = self->db->query_tranges(record->rec);
        return (PyObject*)cursor_create(self, res);
    } catch (wreport::error& e) {
        return raise_wreport_exception(e);
    } catch (std::exception& se) {
        return raise_std_exception(se);
    }
}

static PyObject* dpy_DB_query_variable_types(dpy_DB* self, PyObject* args)
{
    dpy_Record* record;
    if (!PyArg_ParseTuple(args, "O!", &dpy_Record_Type, &record))
        return NULL;

    try {
        std::auto_ptr<db::Cursor> res = self->db->query_variable_types(record->rec);
        return (PyObject*)cursor_create(self, res);
    } catch (wreport::error& e) {
        return raise_wreport_exception(e);
    } catch (std::exception& se) {
        return raise_std_exception(se);
    }
}

static PyObject* dpy_DB_query_data(dpy_DB* self, PyObject* args)
{
    dpy_Record* record;
    if (!PyArg_ParseTuple(args, "O!", &dpy_Record_Type, &record))
        return NULL;

    try {
        std::auto_ptr<db::Cursor> res = self->db->query_data(record->rec);
        return (PyObject*)cursor_create(self, res);
    } catch (wreport::error& e) {
        return raise_wreport_exception(e);
    } catch (std::exception& se) {
        return raise_std_exception(se);
    }
}

static PyObject* dpy_DB_query_datetime_extremes(dpy_DB* self, PyObject* args)
{
    dpy_Record* record;
    if (!PyArg_ParseTuple(args, "O!", &dpy_Record_Type, &record))
        return NULL;

    try {
        Record res;
        self->db->query_datetime_extremes(record->rec, res);
        if (res.key_peek_value(DBA_KEY_YEARMIN))
        {
            OwnedPyObject dt_min(PyDateTime_FromDateAndTime(
                        res.get(DBA_KEY_YEARMIN).enqi(),
                        res.get(DBA_KEY_MONTHMIN).enqi(),
                        res.get(DBA_KEY_DAYMIN).enqi(),
                        res.get(DBA_KEY_HOURMIN).enqi(),
                        res.get(DBA_KEY_MINUMIN).enqi(),
                        res.get(DBA_KEY_SECMIN).enqi(), 0));
            OwnedPyObject dt_max(PyDateTime_FromDateAndTime(
                        res.get(DBA_KEY_YEARMAX).enqi(),
                        res.get(DBA_KEY_MONTHMAX).enqi(),
                        res.get(DBA_KEY_DAYMAX).enqi(),
                        res.get(DBA_KEY_HOURMAX).enqi(),
                        res.get(DBA_KEY_MINUMAX).enqi(),
                        res.get(DBA_KEY_SECMAX).enqi(), 0));
            return Py_BuildValue("OO", dt_min.get(), dt_max.get());
        } else {
            return Py_BuildValue("OO", Py_None, Py_None);
        }
    } catch (wreport::error& e) {
        return raise_wreport_exception(e);
    } catch (std::exception& se) {
        return raise_std_exception(se);
    }
}

static PyObject* dpy_DB_query_attrs(dpy_DB* self, PyObject* args, PyObject* kw)
{
    static char* kwlist[] = { "varcode", "reference_id", "attrs", NULL };
    int reference_id;
    const char* varname;
    PyObject* attrs = 0;
    if (!PyArg_ParseTupleAndKeywords(args, kw, "si|O", kwlist, &varname, &reference_id, &attrs))
        return NULL;

    wreport::Varcode varcode = resolve_varcode(varname);

    // Read the attribute list, if provided
    db::AttrList codes;
    if (!db_read_attrlist(attrs, codes))
        return NULL;

    try {
        self->db->query_attrs(reference_id, varcode, codes, self->attr_rec->rec);
        Py_INCREF(self->attr_rec);
        return (PyObject*)self->attr_rec;
    } catch (wreport::error& e) {
        return raise_wreport_exception(e);
    } catch (std::exception& se) {
        return raise_std_exception(se);
    }
}

static PyObject* dpy_DB_attr_insert(dpy_DB* self, PyObject* args, PyObject* kw)
{
    static char* kwlist[] = { "varcode", "attrs", "reference_id", "replace", NULL };
    int reference_id = -1;
    const char* varname;
    dpy_Record* record;
    int can_replace = 1;
    if (!PyArg_ParseTupleAndKeywords(args, kw, "sO!|ii", kwlist,
                &varname,
                &dpy_Record_Type, &record,
                &reference_id,
                &can_replace))
        return NULL;

    try {
        if (reference_id == -1)
            self->db->attr_insert(resolve_varcode(varname), record->rec, can_replace);
        else
            self->db->attr_insert(reference_id, resolve_varcode(varname), record->rec, can_replace);
        Py_RETURN_NONE;
    } catch (wreport::error& e) {
        return raise_wreport_exception(e);
    } catch (std::exception& se) {
        return raise_std_exception(se);
    }
}

static PyObject* dpy_DB_attr_remove(dpy_DB* self, PyObject* args, PyObject* kw)
{
    static char* kwlist[] = { "varcode", "reference_id", "attrs", NULL };
    int reference_id;
    const char* varname;
    PyObject* attrs = 0;
    if (!PyArg_ParseTupleAndKeywords(args, kw, "si|O", kwlist, &varname, &reference_id, &attrs))
        return NULL;

    wreport::Varcode varcode = resolve_varcode(varname);

    // Read the attribute list, if provided
    db::AttrList codes;
    if (!db_read_attrlist(attrs, codes))
        return NULL;

    try {
        self->db->attr_remove(reference_id, varcode, codes);
        Py_RETURN_NONE;
    } catch (wreport::error& e) {
        return raise_wreport_exception(e);
    } catch (std::exception& se) {
        return raise_std_exception(se);
    }
}

    /*
    virtual void import_msg(const Msg& msg, const char* repmemo, int flags) = 0;
    virtual void import_msgs(const Msgs& msgs, const char* repmemo, int flags) = 0;
    virtual void export_msgs(const Record& query, MsgConsumer& cons) = 0;
    virtual void dump(FILE* out) = 0;
    */

namespace {
struct ExportConsumer : public MsgConsumer
{
    File& out;
    msg::Exporter* exporter;
    ExportConsumer(File& out, const char* template_name=NULL)
        : out(out), exporter(0)
    {
        if (template_name == NULL)
            exporter = msg::Exporter::create(out.type()).release();
        else
        {
            msg::Exporter::Options opts;
            opts.template_name = "generic";
            exporter = msg::Exporter::create(out.type(), opts).release();
        }
    }
    ~ExportConsumer()
    {
        if (exporter) delete exporter;
    }
    void operator()(std::auto_ptr<Msg> msg)
    {
        Rawmsg raw;
        Msgs msgs;
        msgs.acquire(msg);
        exporter->to_rawmsg(msgs, raw);
        out.write(raw);
    }
};
}

static PyObject* dpy_DB_export_to_file(dpy_DB* self, PyObject* args, PyObject* kw)
{
    static char* kwlist[] = { "query", "format", "filename", "generic", NULL };
    dpy_Record* query;
    const char* format;
    const char* filename;
    int as_generic = 0;
    if (!PyArg_ParseTupleAndKeywords(args, kw, "O!ss|i", kwlist, &dpy_Record_Type, &query, &format, &filename, &as_generic))
        return NULL;

    Encoding encoding = BUFR;
    if (strcmp(format, "BUFR") == 0)
        encoding = BUFR;
    else if (strcmp(format, "CREX") == 0)
        encoding = CREX;
    else
    {
        PyErr_SetString(PyExc_ValueError, "encoding must be one of BUFR or CREX");
        return NULL;
    }

    try {
        std::auto_ptr<File> out = File::create(encoding, filename, "wb");
        ExportConsumer msg_writer(*out, as_generic ? "generic" : NULL);
        self->db->export_msgs(query->rec, msg_writer);
        Py_RETURN_NONE;
    } catch (wreport::error& e) {
        return raise_wreport_exception(e);
    } catch (std::exception& se) {
        return raise_std_exception(se);
    }
}

static PyMethodDef dpy_DB_methods[] = {
    {"connect",           (PyCFunction)dpy_DB_connect, METH_VARARGS | METH_CLASS,
        "Create a DB connecting to an ODBC source" },
    {"connect_from_file", (PyCFunction)dpy_DB_connect_from_file, METH_VARARGS | METH_CLASS,
        "Create a DB connecting to a SQLite file" },
    {"connect_from_url",  (PyCFunction)dpy_DB_connect_from_url, METH_VARARGS | METH_CLASS,
        "Create a DB as defined in an URL-like string" },
    {"connect_test",      (PyCFunction)dpy_DB_connect_test, METH_NOARGS | METH_CLASS,
        "Create a DB for running the test suite, as configured in the test environment" },
    {"is_url",            (PyCFunction)dpy_DB_is_url, METH_VARARGS | METH_CLASS,
        "Checks if a string looks like a DB url" },
    {"disappear",         (PyCFunction)dpy_DB_disappear, METH_NOARGS,
        "Remove all our traces from the database, if applicable." },
    {"reset",             (PyCFunction)dpy_DB_reset, METH_VARARGS,
        "Reset the database, removing all existing Db-All.e tables and re-creating them empty." },
    {"insert",            (PyCFunction)dpy_DB_insert, METH_VARARGS | METH_KEYWORDS,
        "Insert a record in the database" },
    {"remove",            (PyCFunction)dpy_DB_remove, METH_VARARGS,
        "Remove records from the database" },
    {"vacuum",            (PyCFunction)dpy_DB_vacuum, METH_NOARGS,
        "Perform database cleanup operations" },
    {"query_reports",    (PyCFunction)dpy_DB_query_reports, METH_VARARGS,
        "Query the report information archive in the database; returns a Cursor" },
    {"query_stations",    (PyCFunction)dpy_DB_query_stations, METH_VARARGS,
        "Query the station archive in the database; returns a Cursor" },
    {"query_levels",      (PyCFunction)dpy_DB_query_levels, METH_VARARGS,
        "Query the level archive in the database; returns a Cursor" },
    {"query_tranges",     (PyCFunction)dpy_DB_query_tranges, METH_VARARGS,
        "Query the time range archive in the database; returns a Cursor" },
    {"query_variable_types", (PyCFunction)dpy_DB_query_variable_types, METH_VARARGS,
        "Query the variable types in the database; returns a Cursor" },
    {"query_data",        (PyCFunction)dpy_DB_query_data, METH_VARARGS,
        "Query the variables in the database; returns a Cursor" },
    {"query_datetime_extremes", (PyCFunction)dpy_DB_query_datetime_extremes, METH_VARARGS,
        "Compute the earliest and lastest datetimes for the results of the given query."
        " Returns two datetime objects, or two None if the query has no results" },
    {"attr_insert",       (PyCFunction)dpy_DB_attr_insert, METH_VARARGS | METH_KEYWORDS,
        "Insert new attributes into the database" },
    {"attr_remove",       (PyCFunction)dpy_DB_attr_remove, METH_VARARGS | METH_KEYWORDS,
        "Remove attributes" },
    {"query_attrs",       (PyCFunction)dpy_DB_query_attrs, METH_VARARGS | METH_KEYWORDS,
        "Query attributes" },
    {"export_to_file",    (PyCFunction)dpy_DB_export_to_file, METH_VARARGS | METH_KEYWORDS,
        "Export data matching a query as bulletins to a named file" },
    {NULL}
};

static int dpy_DB_init(dpy_DB* self, PyObject* args, PyObject* kw)
{
    // People should not invoke DB() as a constructor, but if they do,
    // this is better than a segfault later on
    PyErr_SetString(PyExc_NotImplementedError, "DB objects cannot be constructed explicitly");
    return -1;
}

static void dpy_DB_dealloc(dpy_DB* self)
{
    if (self->db)
        delete self->db;
}

static PyObject* dpy_DB_str(dpy_DB* self)
{
    /*
    std::string f = self->var.format("None");
    return PyString_FromString(f.c_str());
    */
    return PyString_FromString("DB");
}

static PyObject* dpy_DB_repr(dpy_DB* self)
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
    return PyString_FromString("DB object");
}

PyTypeObject dpy_DB_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                         // ob_size
    "dballe.DB",               // tp_name
    sizeof(dpy_DB),            // tp_basicsize
    0,                         // tp_itemsize
    (destructor)dpy_DB_dealloc, // tp_dealloc
    0,                         // tp_print
    0,                         // tp_getattr
    0,                         // tp_setattr
    0,                         // tp_compare
    (reprfunc)dpy_DB_repr,     // tp_repr
    0,                         // tp_as_number
    0,                         // tp_as_sequence
    0,                         // tp_as_mapping
    0,                         // tp_hash
    0,                         // tp_call
    (reprfunc)dpy_DB_str,      // tp_str
    0,                         // tp_getattro
    0,                         // tp_setattro
    0,                         // tp_as_buffer
    Py_TPFLAGS_DEFAULT,        // tp_flags
    "DB-All.e DB",             // tp_doc
    0,                         // tp_traverse
    0,                         // tp_clear
    0,                         // tp_richcompare
    0,                         // tp_weaklistoffset
    0,                         // tp_iter
    0,                         // tp_iternext
    dpy_DB_methods,            // tp_methods
    0,                         // tp_members
    dpy_DB_getsetters,         // tp_getset
    0,                         // tp_base
    0,                         // tp_dict
    0,                         // tp_descr_get
    0,                         // tp_descr_set
    0,                         // tp_dictoffset
    (initproc)dpy_DB_init,     // tp_init
    0,                         // tp_alloc
    0,                         // tp_new
};

}

namespace dballe {
namespace python {

bool db_read_attrlist(PyObject* attrs, db::AttrList& codes)
{
    if (!attrs) return true;

    OwnedPyObject iter(PyObject_GetIter(attrs));
    if (iter == NULL) return false;

    try {
        while (PyObject* iter_item = PyIter_Next(iter)) {
            OwnedPyObject item(iter_item);
            const char* name = PyString_AsString(item);
            if (!name) return false;
            codes.push_back(resolve_varcode(name));
        }
        return true;
    } catch (wreport::error& e) {
        raise_wreport_exception(e);
        return false;
    } catch (std::exception& se) {
        raise_std_exception(se);
        return false;
    }
}

dpy_DB* db_create(std::auto_ptr<DB> db)
{
    dpy_Record* attr_rec = record_create();
    if (!attr_rec) return NULL;

    dpy_DB* result = PyObject_New(dpy_DB, &dpy_DB_Type);
    if (!result)
    {
        Py_DECREF(attr_rec);
        return NULL;
    }

    result = (dpy_DB*)PyObject_Init((PyObject*)result, &dpy_DB_Type);
    result->db = db.release();
    result->attr_rec = attr_rec;
    return result;
}

void register_db(PyObject* m)
{
    PyDateTime_IMPORT;

    dpy_DB_Type.tp_new = PyType_GenericNew;
    if (PyType_Ready(&dpy_DB_Type) < 0)
        return;

    Py_INCREF(&dpy_DB_Type);
    PyModule_AddObject(m, "DB", (PyObject*)&dpy_DB_Type);
}

}
}
