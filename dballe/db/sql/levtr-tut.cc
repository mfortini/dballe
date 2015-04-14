/*
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

#include "db/test-utils-db.h"
#include "db/v6/db.h"
#include "db/sql/levtr.h"

using namespace dballe;
using namespace dballe::tests;
using namespace wibble::tests;
using namespace wreport;
using namespace std;

namespace {

struct db_sql_levtr : public dballe::tests::db_test
{
    db::sql::LevTr& levtr()
    {
        if (db::v6::DB* db6 = dynamic_cast<db::v6::DB*>(db.get()))
            return db6->lev_tr();
        throw error_consistency("cannot test repinfo on the current DB");
    }
};

}

namespace tut {

typedef db_tg<db_sql_levtr> tg;
typedef tg::object to;

// Insert some values and try to read them again
template<> template<> void to::test<1>()
{
    use_db();
    auto& lt = levtr();

    // Insert a lev_tr
    wassert(actual(lt.obtain_id(Level(1, 2, 0, 3), Trange(4, 5, 6))) == 1);

    // Insert another lev_tr
    wassert(actual(lt.obtain_id(Level(2, 3, 1, 4), Trange(5, 6, 7))) == 2);

#if 0
	// Get the ID of the first lev_tr
	co->id = 0;
	co->ltype1 = 1;
	co->l1 = 2;
	co->ltype2 = 0;
	co->l2 = 3;
	co->pind = 4;
	co->p1 = 5;
	co->p2 = 6;
	ensure_equals(co->get_id(), 1);

	// Get the ID of the second lev_tr
	co->id = 0;
	co->ltype1 = 2;
	co->l1 = 3;
	co->ltype2 = 1;
	co->l2 = 4;
	co->pind = 5;
	co->p1 = 6;
	co->p2 = 7;
	ensure_equals(co->get_id(), 2);

	// Get info on the first lev_tr
	co->get_data(1);
	ensure_equals(co->ltype1, 1);
	ensure_equals(co->l1, 2);
	ensure_equals(co->ltype2, 0);
	ensure_equals(co->l2, 3);
	ensure_equals(co->pind, 4);
	ensure_equals(co->p1, 5);
	ensure_equals(co->p2, 6);

	// Get info on the second lev_tr
	co->get_data(2);
	ensure_equals(co->ltype1, 2);
	ensure_equals(co->l1, 3);
	ensure_equals(co->ltype2, 1);
	ensure_equals(co->l2, 4);
	ensure_equals(co->pind, 5);
	ensure_equals(co->p1, 6);
	ensure_equals(co->p2, 7);
#endif

#if 0
	// Update the second lev_tr
	co->id = 2;
	co->id_station = 2;
	co->id_report = 2;
	co->date_ind = snprintf(co->date, 25, "%04d-%02d-%02d %02d:%02d:%02d", 2003, 4, 5, 6, 7, 8);
	co->ltype = 3;
	co->l1 = 4;
	co->l2 = 5;
	co->pind = 6;
	co->p1 = 7;
	co->p2 = 8;
	CHECKED(dba_db_lev_tr_update(co));

	// Get info on the first station: it should be unchanged
	CHECKED(dba_db_lev_tr_get_data(pa, 1));
	ensure_equals(pa->lat, 4500000);
	ensure_equals(pa->lon, 1100000);
	ensure_equals(pa->ident, string("ciao"));
	ensure_equals(pa->ident_ind, 4);

	// Get info on the second station: it should be updated
	CHECKED(dba_db_lev_tr_get_data(pa, 2));
	ensure_equals(pa->lat, 4700000);
	ensure_equals(pa->lon, 1300000);
	ensure_equals(pa->ident[0], 0);
#endif
}

}

namespace {

tut::tg v6_tg("db_sql_levtr_v6", db::V6);

}