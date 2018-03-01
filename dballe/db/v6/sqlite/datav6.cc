#include "datav6.h"
#include "dballe/db/v6/qbuilder.h"
#include "dballe/sql/sqlite.h"
#include "dballe/sql/querybuf.h"
#include "dballe/record.h"
#include <algorithm>
#include <cstring>

using namespace wreport;
using namespace std;
using dballe::sql::SQLiteConnection;
using dballe::sql::SQLiteStatement;
using dballe::sql::Querybuf;

namespace dballe {
namespace db {
namespace v6 {
namespace sqlite {

SQLiteDataV6::SQLiteDataV6(SQLiteConnection& conn)
    : conn(conn)
{
    // Create the statement for select id
    sstm = conn.sqlitestatement(R"(
        SELECT id, id_lev_tr, id_var, value
          FROM data
         WHERE id_station=? AND id_report=? AND datetime=?
         ORDER BY id_lev_tr, id_var
    )").release();
}

SQLiteDataV6::~SQLiteDataV6()
{
    if (sstm) delete sstm;
}

void SQLiteDataV6::insert(dballe::db::Transaction& t, v6::bulk::InsertV6& vars, UpdateMode update_mode)
{
    // Get the current status of variables for this context
    sstm->bind_val(1, vars.id_station);
    sstm->bind_val(2, vars.id_report);
    sstm->bind_val(3, vars.datetime);

    // Scan the result in parallel with the variable list, annotating changed
    // items with their data ID
    v6::bulk::AnnotateVarsV6 todo(vars);
    sstm->execute([&]() {
        todo.annotate(
                sstm->column_int(0),
                sstm->column_int(1),
                sstm->column_int(2),
                sstm->column_string(3));
    });
    todo.annotate_end();

    // We now have a todo-list

    switch (update_mode)
    {
        case UPDATE:
            if (todo.do_update)
            {
                auto update_stm = conn.sqlitestatement("UPDATE data SET value=? WHERE id=?");
                for (auto& v: vars)
                {
                    if (!v.needs_update()) continue;
                    // Warning: we do not know if v.var is a string, so the result of
                    // enqc is only valid until another enqc is called
                    update_stm->bind(v.var->enqc(), v.id_data);
                    update_stm->execute();
                    v.set_updated();
                }
            }
            break;
        case IGNORE:
            break;
        case ERROR:
            if (todo.do_update)
                throw error_consistency("refusing to overwrite existing data");
    }

    if (todo.do_insert)
    {
        Querybuf dq(512);
        dq.appendf(R"(
            INSERT INTO data (id_station, id_report, id_lev_tr, datetime, id_var, value)
                 VALUES (%d, %d, ?, '%04d-%02d-%02d %02d:%02d:%02d', ?, ?)
        )", vars.id_station, vars.id_report,
            vars.datetime.year, vars.datetime.month, vars.datetime.day,
            vars.datetime.hour, vars.datetime.minute, vars.datetime.second);
        auto insert = conn.sqlitestatement(dq);
        for (auto& v: vars)
        {
            if (!v.needs_insert()) continue;
            // Warning: we do not know if v.var is a string, so the result of
            // enqc is only valid until another enqc is called
            insert->bind(v.id_levtr, v.var->code(), v.var->enqc());
            insert->execute();
            v.id_data = conn.get_last_insert_id();
            v.set_inserted();
        }
    }
}

void SQLiteDataV6::remove(const v6::QueryBuilder& qb)
{
    auto stmd = conn.sqlitestatement("DELETE FROM data WHERE id=?");
    auto stm = conn.sqlitestatement(qb.sql_query);
    if (qb.bind_in_ident) stm->bind_val(1, qb.bind_in_ident);

    // Iterate all the data_id results, deleting the related data and attributes
    stm->execute([&]() {
        // Compile the DELETE query for the data
        stmd->bind_val(1, stm->column_int(0));
        stmd->execute();
    });
}

void SQLiteDataV6::dump(FILE* out)
{
    int count = 0;
    fprintf(out, "dump of table data:\n");
    fprintf(out, " id   st   rep ltr  datetime              var\n");
    auto stm = conn.sqlitestatement("SELECT id, id_station, id_report, id_lev_tr, datetime, id_var, value FROM data");
    stm->execute([&]() {
        int id_lev_tr = stm->column_int(3);
        const char* datetime = stm->column_string(4);
        Varcode code = stm->column_int(5);

        char ltr[20];
        if (id_lev_tr == -1)
            strcpy(ltr, "----");
        else
            snprintf(ltr, 20, "%04d", id_lev_tr);

        fprintf(out, " %4d %4d %3d %s %s %01d%02d%03d",
                stm->column_int(0),
                stm->column_int(1),
                stm->column_int(2),
                ltr,
                datetime,
                WR_VAR_F(code), WR_VAR_X(code), WR_VAR_Y(code));
        if (stm->column_isnull(6))
            fprintf(out, "\n");
        else
            fprintf(out, " %s\n", stm->column_string(6));

        ++count;
    });
    fprintf(out, "%d element%s in table data\n", count, count != 1 ? "s" : "");
}

}
}
}
}
