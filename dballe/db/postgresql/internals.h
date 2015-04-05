/*
 * db/sqlite/internals - Implementation infrastructure for the PostgreSQL DB connection
 *
 * Copyright (C) 2015  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#ifndef DBALLE_DB_POSTGRESQL_INTERNALS_H
#define DBALLE_DB_POSTGRESQL_INTERNALS_H

#include <dballe/db/db.h>
#include <dballe/db/sql.h>
#include <libpq-fe.h>
#include <arpa/inet.h>

namespace dballe {
namespace db {

/**
 * Report an PostgreSQL error
 */
struct error_postgresql : public db::error
{
    std::string msg;

    /**
     * Copy informations from the ODBC diagnostic record to the dba error
     * report
     */
    error_postgresql(PGconn* db, const std::string& msg);
    error_postgresql(PGresult* db, const std::string& msg);
    error_postgresql(const std::string& dbmsg, const std::string& msg);
    ~error_postgresql() throw () {}

    wreport::ErrorCode code() const throw () { return wreport::WR_ERR_ODBC; }

    virtual const char* what() const throw () { return msg.c_str(); }

    static void throwf(PGconn* db, const char* fmt, ...) WREPORT_THROWF_ATTRS(2, 3);
    static void throwf(PGresult* db, const char* fmt, ...) WREPORT_THROWF_ATTRS(2, 3);
};

namespace postgresql {

/// Argument list for PQexecParams built at compile time
template<typename... ARGS> struct Params
{
    int count = sizeof...(ARGS);
    const char* args[sizeof...(ARGS)];
    int lengths[sizeof...(ARGS)];
    int formats[sizeof...(ARGS)];
    void* local[sizeof...(ARGS)];

    Params(ARGS... args)
    {
        _add(0, args...);
    }
    ~Params()
    {
        for (auto i: local)
            free(i);
    }

    Params(const Params&) = delete;
    Params(const Params&&) = delete;
    Params& operator=(const Params&) = delete;
    Params& operator=(const Params&&) = delete;

protected:
    /// Terminating condition for compile-time arg expansion
    void _add(unsigned pos)
    {
    }

#if 0
    /// Fill in the argument structures
    template<typename... REST>
    void _add(unsigned pos, uint32_t arg, REST... rest)
    {
        local[pos] = malloc(sizeof(uint32_t));
        *(uint32_t*)local[pos] = htonl(arg);
        args[pos] = (const char*)local[pos];
        lengths[pos] = sizeof(uint32_t);
        formats[pos] = 1;
        _add(pos + 1, rest...);
    }

    /// Fill in the argument structures
    template<typename... REST>
    void _add(unsigned pos, uint64_t arg, REST... rest)
    {
        local[pos] = malloc(sizeof(uint64_t));
        *(uint64_t*)local[pos] = htobe64(arg);
        args[pos] = (const char*)local[pos];
        lengths[pos] = sizeof(uint64_t);
        formats[pos] = 1;
        _add(pos + 1, rest...);
    }
#endif

    /// Fill in the argument structures
    template<typename... REST>
    void _add(unsigned pos, const std::string& arg, REST... rest)
    {
        local[pos] = nullptr;
        args[pos] = arg.data();
        lengths[pos] = arg.size();
        formats[pos] = 0;
        _add(pos + 1, rest...);
    }
};

/// Wrap a PGresult, taking care of its memory management
struct Result
{
    PGresult* res;

    Result(PGresult* res) : res(res) {}
    ~Result() { PQclear(res); }

    /// Implement move
    Result(Result&& o) : res(o.res) { o.res = nullptr; }
    Result& operator=(Result&& o)
    {
        if (this == &o) return *this;
        PQclear(res);
        res = o.res;
        o.res = nullptr;
        return *this;
    }

    operator PGresult*() { return res; }
    operator const PGresult*() const { return res; }

    /// Check that the result successfully returned no data
    void expect_no_data(const std::string& query);

    /// Check that the result successfully returned some (possibly empty) data
    void expect_result(const std::string& query);

    /// Check that the result successfully returned one row of data
    void expect_one_row(const std::string& query);

    /// Check that the result was successful
    void expect_success(const std::string& query);

    /// Get the number of rows in the result
    unsigned rowcount() const { return PQntuples(res); }

    /// Check if a result value is null
    bool is_null(unsigned row, unsigned col) const
    {
        return PQgetisnull(res, row, col);
    }

    /// Return a result value, transmitted in binary as a byte (?)
    bool get_bool(unsigned row, unsigned col) const
    {
        char* val = PQgetvalue(res, row, col);
        return *val;
    }

    /// Return a result value, transmitted in binary as a 2 bit integer
    uint16_t get_int2(unsigned row, unsigned col) const
    {
        char* val = PQgetvalue(res, row, col);
        return ntohs(*(uint16_t*)val);
    }

    /// Return a result value, transmitted in binary as a 4 bit integer
    uint32_t get_int4(unsigned row, unsigned col) const
    {
        char* val = PQgetvalue(res, row, col);
        return ntohl(*(uint32_t*)val);
    }

    /// Return a result value, transmitted in binary as an 8 bit integer
    uint64_t get_int8(unsigned row, unsigned col) const;

    /// Return a result value, transmitted as a string
    const char* get_string(unsigned row, unsigned col) const
    {
        return PQgetvalue(res, row, col);
    }

    // Prevent copy
    Result(const Result&) = delete;
    Result& operator=(const Result&) = delete;
};

}


/// Database connection
class PostgreSQLConnection : public Connection
{
protected:
    /// Database connection
    PGconn* db = nullptr;

protected:
    void impl_exec_void(const std::string& query) override;
    void impl_exec_void_string(const std::string& query, const std::string& arg1) override;
    void impl_exec_void_string_string(const std::string& query, const std::string& arg1, const std::string& arg2) override;
    void init_after_connect();

public:
    PostgreSQLConnection();
    PostgreSQLConnection(const PostgreSQLConnection&) = delete;
    PostgreSQLConnection(const PostgreSQLConnection&&) = delete;
    ~PostgreSQLConnection();

    PostgreSQLConnection& operator=(const PostgreSQLConnection&) = delete;

    operator PGconn*() { return db; }

    void open(const std::string& connection_string);

    std::unique_ptr<Transaction> transaction() override;
    //std::unique_ptr<PostgreSQLStatement> pqstatement(const std::string& query);

    void exec_no_data(const std::string& query)
    {
        postgresql::Result res(PQexecParams(db, query.c_str(), 0, nullptr, nullptr, nullptr, nullptr, 1));
        res.expect_no_data(query);
    }

    postgresql::Result exec(const std::string& query)
    {
        postgresql::Result res(PQexecParams(db, query.c_str(), 0, nullptr, nullptr, nullptr, nullptr, 1));
        res.expect_result(query);
        return res;
    }

    postgresql::Result exec_one_row(const std::string& query)
    {
        postgresql::Result res(PQexecParams(db, query.c_str(), 0, nullptr, nullptr, nullptr, nullptr, 1));
        res.expect_one_row(query);
        return res;
    }

    template<typename ...ARGS>
    void exec_no_data(const std::string& query, ARGS... args)
    {
        postgresql::Params<ARGS...> params(args...);
        postgresql::Result res(PQexecParams(db, query.c_str(), params.count, nullptr, params.args, params.lengths, params.formats, 1));
        res.expect_no_data(query);
    }

    template<typename ...ARGS>
    postgresql::Result exec(const std::string& query, ARGS... args)
    {
        postgresql::Params<ARGS...> params(args...);
        postgresql::Result res(PQexecParams(db, query.c_str(), params.count, nullptr, params.args, params.lengths, params.formats, 1));
        res.expect_result(query);
        return res;
    }

    template<typename ...ARGS>
    postgresql::Result exec_one_row(const std::string& query, ARGS... args)
    {
        postgresql::Params<ARGS...> params(args...);
        postgresql::Result res(PQexecParams(db, query.c_str(), params.count, nullptr, params.args, params.lengths, params.formats, 1));
        res.expect_one_row(query);
        return res;
    }

    /// Check if the database contains a table
    bool has_table(const std::string& name) override;

    /**
     * Get a value from the settings table.
     *
     * Returns the empty string if the table does not exist.
     */
    std::string get_setting(const std::string& key) override;

    /**
     * Set a value in the settings table.
     *
     * The table is created if it does not exist.
     */
    void set_setting(const std::string& key, const std::string& value) override;

    /// Drop the settings table
    void drop_settings() override;

    /**
     * Delete a table in the database if it exists, otherwise do nothing.
     */
    void drop_table_if_exists(const char* name) override;

    /**
     * Delete a sequence in the database if it exists, otherwise do nothing.
     */
    void drop_sequence_if_exists(const char* name) override;

    /**
     * Return LAST_INSERT_ID or LAST_INSER_ROWID or whatever is appropriate for
     * the current database, if supported.
     *
     * If not supported, an exception is thrown.
     */
    int get_last_insert_id() override;

    /// Count the number of rows modified by the last query that was run
    int changes();

    void add_datetime(Querybuf& qb, const int* dt) const override;

    /// Wrap PQexec
    void pqexec(const std::string& query);
    /**
     * Wrap PQexec but do not throw an exception in case of errors.
     *
     * This is useful to be called in destructors. Errors will be printed to
     * stderr.
     */
    void pqexec_nothrow(const std::string& query) noexcept;
};

#if 0
/// PostgreSQL statement
struct PostgreSQLStatement
{
    PostgreSQLConnection& conn;
    std::string t65
    sqlite3_stmt *stm = nullptr;

    PostgreSQLStatement(PostgreSQLConnection& conn, const std::string& query);
    PostgreSQLStatement(const PostgreSQLStatement&) = delete;
    PostgreSQLStatement(const PostgreSQLStatement&&) = delete;
    ~PostgreSQLStatement();
    PostgreSQLStatement& operator=(const PostgreSQLStatement&) = delete;

    /**
     * Bind all the arguments in a single invocation.
     *
     * Note that the parameter positions are used as bind column numbers, so
     * calling this function twice will re-bind columns instead of adding new
     * ones.
     */
    template<typename... Args> void bind(const Args& ...args)
    {
        bindn<sizeof...(args)>(args...);
    }

    void bind_null_val(int idx);
    void bind_val(int idx, int val);
    void bind_val(int idx, unsigned val);
    void bind_val(int idx, unsigned short val);
    void bind_val(int idx, const Datetime& val);
    void bind_val(int idx, const char* val); // Warning: SQLITE_STATIC is used
    void bind_val(int idx, const std::string& val); // Warning: SQLITE_STATIC is used

    /// Run the query, ignoring all results
    void execute();

    /**
     * Run the query, calling on_row for every row in the result.
     *
     * At the end of the function, the statement is reset, even in case an
     * exception is thrown.
     */
    void execute(std::function<void()> on_row);

    /**
     * Run the query, raising an error if there is more than one row in the
     * result
     */
    void execute_one(std::function<void()> on_row);

    /// Read the int value of a column in the result set (0-based)
    int column_int(int col) { return sqlite3_column_int(stm, col); }

    /// Read the int value of a column in the result set (0-based)
    sqlite3_int64 column_int64(int col) { return sqlite3_column_int64(stm, col); }

    /// Read the double value of a column in the result set (0-based)
    double column_double(int col) { return sqlite3_column_double(stm, col); }

    /// Read the string value of a column in the result set (0-based)
    std::string column_string(int col)
    {
        const char* res = (const char*)sqlite3_column_text(stm, col);
        if (res == NULL)
            return std::string();
        else
            return res;
    }

    /// Read the string value of a column and parse it as a Datetime
    Datetime column_datetime(int col);

    /// Check if a column has a NULL value (0-based)
    bool column_isnull(int col) { return sqlite3_column_type(stm, col) == SQLITE_NULL; }

    void wrap_sqlite3_reset();
    void wrap_sqlite3_reset_nothrow() noexcept;
    /**
     * Get the current error message, reset the statement and throw
     * error_sqlite
     */
    [[noreturn]] void reset_and_throw(const std::string& errmsg);

    operator sqlite3_stmt*() { return stm; }
#if 0
    /// @return SQLExecute's result
    int execute();
    /// @return SQLExecute's result
    int exec_direct(const char* query);
    /// @return SQLExecute's result
    int exec_direct(const char* query, int qlen);

    /// @return SQLExecute's result
    int execute_and_close();
    /// @return SQLExecute's result
    int exec_direct_and_close(const char* query);
    /// @return SQLExecute's result
    int exec_direct_and_close(const char* query, int qlen);

    /**
     * @return the number of columns in the result set (or 0 if the statement
     * did not return columns)
     */
    int columns_count();
    bool fetch();
    bool fetch_expecting_one();
    void close_cursor();
    void close_cursor_if_needed();
    /// Row count for select operations
    size_t select_rowcount();
    /// Row count for insert, delete and other non-select operations
    size_t rowcount();
#endif

private:
    // Implementation of variadic bind: terminating condition
    template<size_t total> void bindn() {}
    // Implementation of variadic bind: recursive iteration over the parameter pack
    template<size_t total, typename ...Args, typename T> void bindn(const T& first, const Args& ...args)
    {
        bind_val(total - sizeof...(args), first);
        bindn<total>(args...);
    }
};
#endif

}
}
#endif
