/*
 * Copyright (C) 2005--2015  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#include "db/test-utils-db.h"
#include "db/v6/db.h"
#include "db/sql/station.h"
#include "db/sql/levtr.h"
#include "db/sql/datav6.h"
#include "db/sql/attrv6.h"

using namespace dballe;
using namespace dballe::tests;
using namespace wreport;
using namespace wibble::tests;
using namespace std;

namespace {

struct db_sql_attrv6 : public dballe::tests::db_test
{
    db_sql_attrv6()
    {
        db::v6::DB& v6db = v6();
        db::sql::Station& st = v6db.station();
        db::sql::LevTr& lt = v6db.lev_tr();
        db::sql::DataV6& da = v6().data();

        // Insert a mobile station
        wassert(actual(st.obtain_id(4500000, 1100000, "ciao")) == 1);

        // Insert a fixed station
        wassert(actual(st.obtain_id(4600000, 1200000)) == 2);

        // Insert a lev_tr
        wassert(actual(lt.obtain_id(Level(1, 2, 0, 3), Trange(4, 5, 6))) == 1);

        // Insert another lev_tr
        wassert(actual(lt.obtain_id(Level(2, 3, 1, 4), Trange(5, 6, 7))) == 2);

        // Insert a datum
        da.set_context(1, 1, 1);
        da.set_date(2001, 2, 3, 4, 5, 6);
        da.insert_or_fail(Var(varinfo(WR_VAR(0, 1, 2)), 123));

        // Insert another datum
        da.set_context(2, 2, 2);
        da.set_date(2002, 3, 4, 5, 6, 7);
        da.insert_or_fail(Var(varinfo(WR_VAR(0, 1, 2)), 234));
    }

    db::sql::AttrV6& attr()
    {
        if (db::v6::DB* db6 = dynamic_cast<db::v6::DB*>(db.get()))
            return db6->attr();
        throw error_consistency("cannot test attrv6 on the current DB");
    }

    Var query(int id_data, unsigned expected_attr_count)
    {
        Var res(varinfo(WR_VAR(0, 12, 101)));
        unsigned count = 0;
        attr().read(id_data, [&](unique_ptr<Var> attr) { res.seta(auto_ptr<Var>(attr.release())); ++count; });
        wassert(actual(count) == expected_attr_count);
        return res;
    }
};

}

namespace tut {

typedef db_tg<db_sql_attrv6> tg;
typedef tg::object to;

// Insert some values and try to read them again
template<> template<> void to::test<1>()
{
    use_db();
    auto& at = attr();

    Var var1(varinfo(WR_VAR(0, 12, 101)), 280.0);
    var1.seta(ap_newvar(WR_VAR(0, 33, 7), 50));

    Var var2(varinfo(WR_VAR(0, 12, 101)), 280.0);
    var2.seta(ap_newvar(WR_VAR(0, 33, 7), 75));

    // Insert a datum
    at.add(1, var1);

    // Insert another datum
    at.add(2, var2);

    // Reinsert the same datum: it should work, doing no insert/update queries
    at.add(1, var1);

    // Reinsert the other same datum: it should work, doing no insert/update queries
    at.add(2, var2);

    // Load the attributes for the first variable
    {
        Var var(query(1, 1));
        wassert(actual(var.next_attr()->code()) == WR_VAR(0, 33, 7));
        wassert(actual(var.next_attr()->value()) == "50");
    }

    // Load the attributes for the second variable
    {
        Var var(query(2, 1));
        wassert(actual(var.next_attr()->code()) == WR_VAR(0, 33, 7));
        wassert(actual(var.next_attr()->value()) == "75");
    }

    // Update both values
    at.add(1, var2);
    at.add(2, var1);
    {
        Var var(query(1, 1));
        wassert(actual(var.next_attr()->code()) == WR_VAR(0, 33, 7));
        wassert(actual(var.next_attr()->value()) == "75");
    }
    {
        Var var(query(2, 1));
        wassert(actual(var.next_attr()->code()) == WR_VAR(0, 33, 7));
        wassert(actual(var.next_attr()->value()) == "50");
    }

    // TODO: test a mix of update and insert
}

}

namespace {

tut::tg db_test_sql_attrv6_tg("db_sql_attrv6", db::V6);

}