/*
 * db/internals - Internal support infrastructure for the DB
 *
 * Copyright (C) 2005--2009  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#include "internals.h"

#include <sql.h>
#include <sqlext.h>

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#if 0

#define _GNU_SOURCE
#include <dballe/db/repinfo.h>
#include <dballe/db/pseudoana.h>
#include <dballe/db/context.h>
#include <dballe/db/data.h>
#include <dballe/db/attr.h>
#include <dballe/core/verbose.h>

#include <config.h>

#include <assert.h>
#endif

using namespace std;
using namespace wreport;

namespace dballe {
namespace db {

error_odbc::error_odbc(SQLSMALLINT handleType, SQLHANDLE handle, const std::string& msg)
{
	static const int strsize = 200;
	char stat[10], sqlmsg[strsize];
	SQLINTEGER err;
	SQLSMALLINT mlen;

	SQLGetDiagRec(handleType, handle, 1, (unsigned char*)stat, &err, (unsigned char*)sqlmsg, strsize, &mlen);
	if (mlen > strsize) mlen = strsize;

	this->msg = msg;
	this->msg += ": ";
	this->msg += sqlmsg;
}

void error_odbc::throwf(SQLSMALLINT handleType, SQLHANDLE handle, const char* fmt, ...)
{
	// Format the arguments
	va_list ap;
	va_start(ap, fmt);
	char* cmsg;
	vasprintf(&cmsg, fmt, ap);
	va_end(ap);

	// Convert to string
	std::string msg(cmsg);
	free(cmsg);
	throw error_odbc(handleType, handle, msg);
}

Environment::Environment()
{
	// Allocate ODBC environment handle and register version 
	int res = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &od_env);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
		throw error_odbc(SQL_HANDLE_ENV, od_env, "Allocating main environment handle");

	res = SQLSetEnvAttr(od_env, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0); 
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
	{
		error_odbc e(SQL_HANDLE_ENV, od_env, "Asking for ODBC version 3");
		SQLFreeHandle(SQL_HANDLE_ENV, od_env);
		throw e;
	}
}

Environment::~Environment()
{
	SQLFreeHandle(SQL_HANDLE_ENV, od_env);
}

Environment& Environment::get()
{
	static Environment* env = NULL;
	if (!env) env = new Environment;
	return *env;
}

Connection::Connection()
	: connected(false)
{
	/* Allocate the ODBC connection handle */
	Environment& env = Environment::get();
	int sqlres = SQLAllocHandle(SQL_HANDLE_DBC, env.od_env, &od_conn);
	if ((sqlres != SQL_SUCCESS) && (sqlres != SQL_SUCCESS_WITH_INFO))
		throw error_odbc(SQL_HANDLE_DBC, od_conn, "Allocating new connection handle");
}

Connection::~Connection()
{
	if (connected)
	{
		// FIXME: It was commit with no reason, setting it to rollback,
		// needs checking it doesn't cause trouble
		SQLEndTran(SQL_HANDLE_DBC, od_conn, SQL_ROLLBACK);
		SQLDisconnect(od_conn);
	}
	SQLFreeHandle(SQL_HANDLE_DBC, od_conn);
}

void Connection::connect(const char* dsn, const char* user, const char* password)
{
	/* Connect to the DSN */
	int sqlres = SQLConnect(od_conn,
				(SQLCHAR*)dsn, SQL_NTS,
				(SQLCHAR*)user, SQL_NTS,
				(SQLCHAR*)(password == NULL ? "" : password), SQL_NTS);
	if ((sqlres != SQL_SUCCESS) && (sqlres != SQL_SUCCESS_WITH_INFO))
		error_odbc::throwf(SQL_HANDLE_DBC, od_conn, "Connecting to DSN %s as user %s", dsn, user);
	connected = true;
	init_after_connect();
}

void Connection::driver_connect(const char* config)
{
	/* Connect to the DSN */
	char sdcout[1024];
	SQLSMALLINT outlen;
	int sqlres = SQLDriverConnect(od_conn, NULL,
					(SQLCHAR*)config, SQL_NTS,
					(SQLCHAR*)sdcout, 1024, &outlen,
					SQL_DRIVER_NOPROMPT);

	if ((sqlres != SQL_SUCCESS) && (sqlres != SQL_SUCCESS_WITH_INFO))
		error_odbc::throwf(SQL_HANDLE_DBC, od_conn, "Connecting to DB using configuration %s", config);
	connected = true;
	init_after_connect();
}

void Connection::init_after_connect()
{
	/* Find out what kind of database we are working with */
	string name = driver_name();

	if (name.substr(0, 9) == "libmyodbc" || name.substr(0, 6) == "myodbc")
		server_type = MYSQL;
	else if (name.substr(0, 6) == "sqlite")
		server_type = SQLITE;
	else if (name.substr(0, 5) == "SQORA")
		server_type = ORACLE;
	else if (name.substr(0, 11) == "libpsqlodbc")
		server_type = POSTGRES;
	else
	{
		fprintf(stderr, "ODBC driver %s is unsupported: assuming it's similar to Postgres", name.c_str());
		server_type = POSTGRES;
	}
}

std::string Connection::driver_name()
{
	char drivername[50];
	SQLSMALLINT len;
	int sqlres = SQLGetInfo(od_conn, SQL_DRIVER_NAME, (SQLPOINTER)drivername, 50, &len);
	if ((sqlres != SQL_SUCCESS) && (sqlres != SQL_SUCCESS_WITH_INFO))
		throw error_odbc(SQL_HANDLE_DBC, od_conn, "Getting ODBC driver name");
	return string(drivername, len);
}

void Connection::set_autocommit(bool val)
{
	int sqlres = SQLSetConnectAttr(od_conn, SQL_ATTR_AUTOCOMMIT, (void*)(val ? SQL_AUTOCOMMIT_ON : SQL_AUTOCOMMIT_OFF), 0);
	if ((sqlres != SQL_SUCCESS) && (sqlres != SQL_SUCCESS_WITH_INFO))
		error_odbc::throwf(SQL_HANDLE_DBC, od_conn, "%s ODBC autocommit", val ? "Enabling" : "Disabling");
}

#ifdef DBA_USE_TRANSACTIONS
void Connection::commit()
{
	int sqlres = SQLEndTran(SQL_HANDLE_DBC, od_conn, SQL_COMMIT);
	if ((sqlres != SQL_SUCCESS) && (sqlres != SQL_SUCCESS_WITH_INFO))
		throw error_odbc(SQL_HANDLE_DBC, od_conn, "Committing a transaction");
}

void Connection::rollback()
{
	int sqlres = SQLEndTran(SQL_HANDLE_DBC, od_conn, SQL_ROLLBACK);
	if ((sqlres != SQL_SUCCESS) && (sqlres != SQL_SUCCESS_WITH_INFO))
		throw error_odbc(SQL_HANDLE_DBC, od_conn, "Rolling back a transaction");
}
#else
// TODO: lock and unlock tables instead
void Connection::commit() {}
void Connection::rollback() {}
#endif

Statement::Statement(Connection& conn)
	: stm(NULL), ignore_error(NULL)
{
	int sqlres = SQLAllocHandle(SQL_HANDLE_STMT, conn.od_conn, &stm);
	if ((sqlres != SQL_SUCCESS) && (sqlres != SQL_SUCCESS_WITH_INFO))
		throw error_odbc(SQL_HANDLE_STMT, stm, "Allocating new statement handle");
}

Statement::~Statement()
{
	SQLFreeHandle(SQL_HANDLE_STMT, stm);
}

bool Statement::error_is_ignored()
{
	if (!ignore_error) return false;

	// Retrieve the current error code
	char stat[10];
	SQLINTEGER err;
	SQLSMALLINT mlen;
	SQLGetDiagRec(SQL_HANDLE_STMT, stm, 1, (unsigned char*)stat, &err, NULL, 0, &mlen);

	// Ignore the given SQL error
	return memcmp(stat, ignore_error, 5) == 0;
}

bool Statement::is_error(int sqlres)
{
	return (sqlres != SQL_SUCCESS) && (sqlres != SQL_SUCCESS_WITH_INFO) && !error_is_ignored();
}

void Statement::bind(int idx, DBALLE_SQL_C_SINT_TYPE& val)
{
	SQLBindCol(stm, idx, DBALLE_SQL_C_SINT, &val, sizeof(val), 0);
}

void Statement::prepare(const char* query)
{
	// Casting out 'const' because ODBC API is not const-conscious
	if (is_error(SQLPrepare(stm, (unsigned char*)query, SQL_NTS)))
		error_odbc::throwf(SQL_HANDLE_STMT, stm, "compiling query \"%s\"", query);
}

void Statement::prepare(const char* query, int qlen)
{
	// Casting out 'const' because ODBC API is not const-conscious
	if (is_error(SQLPrepare(stm, (unsigned char*)query, qlen)))
		error_odbc::throwf(SQL_HANDLE_STMT, stm, "compiling query \"%.*s\"", qlen, query);
}

void Statement::exec_direct(const char* query)
{
	// Casting out 'const' because ODBC API is not const-conscious
	if (is_error(SQLExecDirect(stm, (SQLCHAR*)query, SQL_NTS)))
		error_odbc::throwf(SQL_HANDLE_STMT, stm, "executing query \"%s\"", query);
}

void Statement::exec_direct(const char* query, int qlen)
{
	// Casting out 'const' because ODBC API is not const-conscious
	if (is_error(SQLExecDirect(stm, (SQLCHAR*)query, qlen)))
		error_odbc::throwf(SQL_HANDLE_STMT, stm, "executing query \"%.*s\"", qlen, query);
}

Sequence::Sequence(Connection& conn, const char* name)
	: Statement(conn)
{
	char qbuf[100];
	int qlen;

	bind(1, out);
	if (conn.server_type == ORACLE)
		qlen = snprintf(qbuf, 100, "SELECT %s.CurrVal FROM dual", name);	
	else
		qlen = snprintf(qbuf, 100, "SELECT last_value FROM %s", name);	
	prepare(qbuf, qlen);
}

Sequence::~Sequence() {}

const DBALLE_SQL_C_SINT_TYPE& Sequence::read()
{
	if (is_error(SQLExecute(stm)))
		throw error_odbc(SQL_HANDLE_STMT, stm, "reading sequence value");
	/* Get the result */
	if (SQLFetch(stm) == SQL_NO_DATA)
		throw error_notfound("fetching results of sequence value reads");
	if (is_error(SQLCloseCursor(stm)))
		throw error_odbc(SQL_HANDLE_STMT, stm, "closing sequence read cursor");
	return out;
}

#if 0
/*
 * Define to true to enable the use of transactions during writes
 */
#define DBA_USE_TRANSACTIONS


dba_err dba_db_last_insert_id(dba_db db, int* id)
{
	int res;
	*id = -1;

	res = SQLExecute(db->stm_last_insert_id);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
		return dba_db_error_odbc(SQL_HANDLE_STMT, db->stm_last_insert_id, "querying last inserted ID");

	if (SQLFetch(db->stm_last_insert_id) == SQL_NO_DATA)
		return dba_db_error_odbc(SQL_HANDLE_STMT, db->stm_last_insert_id, "retrieving results of query for last inserted ID");

	res = SQLCloseCursor(db->stm_last_insert_id);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
		return dba_db_error_odbc(SQL_HANDLE_STMT, db->stm_last_insert_id, "closing dba_db_last_insert_id cursor");

	*id = db->last_insert_id;

	return dba_error_ok();
}



dba_err dba_db_need_repinfo(dba_db db)
{
	if (db->repinfo == NULL)
		return dba_db_repinfo_create(db, &(db->repinfo));
	return dba_error_ok();
}
dba_err dba_db_need_pseudoana(dba_db db)
{
	if (db->pseudoana == NULL)
		return dba_db_pseudoana_create(db, &(db->pseudoana));
	return dba_error_ok();
}
dba_err dba_db_need_context(dba_db db)
{
	if (db->context == NULL)
		return dba_db_context_create(db, &(db->context));
	return dba_error_ok();
}
dba_err dba_db_need_data(dba_db db)
{
	if (db->data == NULL)
		return dba_db_data_create(db, &(db->data));
	return dba_error_ok();
}
dba_err dba_db_need_attr(dba_db db)
{
	if (db->attr == NULL)
		return dba_db_attr_create(db, &(db->attr));
	return dba_error_ok();
}

#endif

} // namespace db
} // namespace dballe

/* vim:set ts=4 sw=4: */