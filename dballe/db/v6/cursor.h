/*
 * db/v6/cursor - manage select queries
 *
 * Copyright (C) 2005--2014  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

/** @file
 * @ingroup db
 *
 * Functions used to manage a general DB-ALLe query
 */

#ifndef DBA_DB_V6_CURSOR_H
#define DBA_DB_V6_CURSOR_H

#include <dballe/db/db.h>
#include <dballe/db/v6/internals.h>
#include <dballe/core/structbuf.h>
#include <wreport/varinfo.h>
#include <sqltypes.h>
#include <cstddef>
#include <vector>

namespace dballe {
struct DB;
struct Record;

namespace db {
struct Connection;
struct Statement;

namespace v6 {
struct DB;
struct QueryBuilder;

/**
 * Simple typedef to make typing easier, and also to help some versions of swig
 * match this complex type
 */
typedef std::vector<wreport::Varcode> AttrList;

/**
 * Structure used to build and execute a query, and to iterate through the
 * results
 */
struct Cursor : public db::Cursor
{
    /// Database to operate on
    v6::DB& db;

    /** Modifier flags to enable special query behaviours */
    const unsigned int modifiers;

    /// Results from the query
    Structbuf<SQLRecord> results;

    /// Current result element being iterated
    int cur = -1;

    virtual ~Cursor();

    dballe::DB& get_db() const override;

    /**
     * Get the number of rows still to be fetched
     *
     * @return
     *   The number of rows still to be queried.  The value is undefined if no
     *   query has been successfully peformed yet using this cursor.
     */
    int remaining() const override;

    /**
     * Get a new item from the results of a query
     *
     * @returns
     *   true if a new record has been read, false if there is no more data to read
     */
    bool next() override;

    /// Discard the results that have not been read yet
    void discard_rest() override;

    /**
     * Query attributes for the current variable
     */
    void query_attrs(std::function<void(std::unique_ptr<wreport::Var>)> dest) override;
    void attr_insert(const Record& attrs) override;
    void attr_remove(const AttrList& qcs) override;

    virtual int get_station_id() const;
    virtual double get_lat() const;
    virtual double get_lon() const;
    virtual const char* get_ident(const char* def=0) const;
    virtual const char* get_rep_memo() const;
    virtual Level get_level() const;
    virtual Trange get_trange() const;
    virtual void get_datetime(int (&dt)[6]) const;
    virtual wreport::Varcode get_varcode() const;
    virtual wreport::Var get_var() const;

    virtual int attr_reference_id() const;

    /**
     * Iterate the cursor until the end, returning the number of items.
     *
     * If dump is a FILE pointer, also dump the cursor values to it
     */
    virtual unsigned test_iterate(FILE* dump=0) = 0;

    static std::unique_ptr<Cursor> run_station_query(DB& db, const Query& query);
    static std::unique_ptr<Cursor> run_data_query(DB& db, const Query& query);
    static std::unique_ptr<Cursor> run_summary_query(DB& db, const Query& query);
    static void run_delete_query(DB& db, const Query& query);

protected:
    /// Run the query in qb and fill results with its output
    virtual void load(const QueryBuilder& qb);

    /**
     * Create a query cursor
     *
     * @param wanted
     *   The values wanted in output
     * @param modifiers
     *   Optional modifiers to ask for special query behaviours
     */
    Cursor(v6::DB& db, unsigned int modifiers);

    void to_record_pseudoana(Record& rec);
    void to_record_repinfo(Record& rec);
    void to_record_ltr(Record& rec);
    void to_record_datetime(Record& rec);
    void to_record_varcode(Record& rec);

    /// Query extra station info and add it to \a rec
    void add_station_info(Record& rec);
};

struct CursorStations : public Cursor
{
    ~CursorStations();
    /// Query station info
    virtual void to_record(Record& rec);
    virtual unsigned test_iterate(FILE* dump=0);

protected:
    CursorStations(DB& db, unsigned int modifiers)
        : Cursor(db, modifiers) {}

    friend class Cursor;
};

struct CursorData : public Cursor
{
    ~CursorData();
    /// Query data
    virtual void to_record(Record& rec);
    virtual unsigned test_iterate(FILE* dump=0);

protected:
    CursorData(DB& db, unsigned int modifiers)
        : Cursor(db, modifiers) {}

    friend class Cursor;
};

struct CursorSummary : public Cursor
{
    ~CursorSummary();
    virtual void to_record(Record& rec);
    virtual unsigned test_iterate(FILE* dump=0);

protected:
    CursorSummary(DB& db, unsigned int modifiers)
        : Cursor(db, modifiers) {}

    friend class Cursor;
};

struct CursorBest : public Cursor
{
    ~CursorBest();

    virtual void to_record(Record& rec);
    virtual unsigned test_iterate(FILE* dump=0);

protected:
    CursorBest(DB& db, unsigned int modifiers)
        : Cursor(db, modifiers) {}

    /// Run the query in qb and fill results with its output
    virtual void load(const QueryBuilder& qb);

    friend class Cursor;
};

} // namespace v6
} // namespace db
} // namespace dballe

/* vim:set ts=4 sw=4: */
#endif
