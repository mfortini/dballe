/*
 * Copyright (C) 2005--2012  ARPA-SIM <urpsim@smr.arpa.emr.it>
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
#include "db/v6/data.h"
#include "db/v6/lev_tr.h"
#include "db/v5/station.h"
#include "db/internals.h"

using namespace dballe;
using namespace std;

namespace tut {

struct db_v6_data_shar : public dballe::tests::db_test
{
    db::v6::Data* da;

    db_v6_data_shar() : dballe::tests::db_test(db::V6)
    {
        if (!has_db()) return;
        db::v6::DB& v6db = v6();
        da = &v6db.data();

        db::v5::Station& st = v6db.station();
        db::v6::LevTr& lt = v6db.lev_tr();

		// Insert a mobile station
		st.lat = 4500000;
		st.lon = 1100000;
		st.set_ident("ciao");
        ensure_equals(st.insert(), 1);

		// Insert a fixed station
		st.lat = 4600000;
		st.lon = 1200000;
		st.set_ident(NULL);
		ensure_equals(st.insert(), 2);

        // Insert a lev_tr
        lt.ltype1 = 1;
        lt.l1 = 2;
        lt.ltype2 = 0;
        lt.l2 = 3;
        lt.pind = 4;
        lt.p1 = 5;
        lt.p2 = 6;
        ensure_equals(lt.insert(), 1);

        // Insert another lev_tr
        lt.ltype1 = 2;
        lt.l1 = 3;
        lt.ltype2 = 1;
        lt.l2 = 4;
        lt.pind = 5;
        lt.p1 = 6;
        lt.p2 = 7;
        ensure_equals(lt.insert(), 2);
    }

    void set_sample1()
    {
        da->id_station = 1;
        da->id_report = 1;
        da->id_lev_tr = 1;
        da->date = dballe::tests::mkts(2001, 2, 3, 4, 5, 6);
        da->id_var = WR_VAR(0, 1, 2);
        da->set_value("123");
    }

    void set_sample2()
    {
        da->id_station = 2;
        da->id_report = 2;
        da->id_lev_tr = 2;
        da->date = dballe::tests::mkts(2002, 3, 4, 5, 6, 7);
        da->id_var = WR_VAR(0, 1, 2);
        da->set_value("234");
    }
};
TESTGRP(db_v6_data);

/* Test Data::set * */
template<> template<>
void to::test<1>()
{
	use_db();

    // Test set
    da->set(var(WR_VAR(0, 1, 2), 123));
    ensure_varcode_equals(da->id_var, WR_VAR(0, 1, 2));
	ensure_equals(da->value, string("123"));
	ensure_equals(da->value_ind, 3);

    // Test set_value
    da->set_value("32");
    ensure_varcode_equals(da->id_var, WR_VAR(0, 1, 2));
	ensure_equals(da->value, string("32"));
	ensure_equals(da->value_ind, 2);
}


/* Insert some values and try to read them again */
template<> template<>
void to::test<2>()
{
    use_db();

    // Insert a datum
    set_sample1();
    da->insert_or_fail();

    // Insert another datum
    set_sample2();
    da->insert_or_fail();

    // Reinsert a datum: it should fail
    set_sample1();
    try {
        da->insert_or_fail();
        ensure(false);
    } catch (db::error_odbc& e) {
        //ensure_contains(e.what(), "uplicate");
    }

    // Reinsert the other datum: it should fail
    set_sample2();
    try {
        da->insert_or_fail();
        ensure(false);
    } catch (db::error_odbc& e) {
        //ensure_contains(e.what(), "uplicate");
    }

    // Reinsert a datum with overwrite: it should work
    set_sample1();
    da->insert_or_overwrite();

    // Reinsert the other datum with overwrite: it should work
    set_sample2();
    da->insert_or_overwrite();

	// Insert a new datum with ignore: it should insert
	da->id_lev_tr = 3;
	da->id_var = WR_VAR(0, 1, 2);
	da->set_value("234");
    ensure_equals(da->insert_or_ignore(), true);

	// Reinsert the same datum with ignore: it should ignore
	da->id_lev_tr = 3;
	da->id_var = WR_VAR(0, 1, 2);
	da->set_value("234");
    ensure_equals(da->insert_or_ignore(), false);

#if 0
	// Get the ID of the first data
	lt->id = 0;
	lt->id_ana = 1;
	lt->id_report = 1;
	lt->date_ind = snprintf(lt->date, 25, "%04d-%02d-%02d %02d:%02d:%02d", 2001, 2, 3, 4, 5, 6);
	lt->ltype = 1;
	lt->l1 = 2;
	lt->l2 = 3;
	lt->pind = 4;
	lt->p1 = 5;
	lt->p2 = 6;
	CHECKED(dba_db_data_get_id(lt, &id));
	ensure_equals(id, 1);

	// Get the ID of the second data
	lt->id = 0;
	lt->id_ana = 2;
	lt->id_report = 2;
	lt->date_ind = snprintf(lt->date, 25, "%04d-%02d-%02d %02d:%02d:%02d", 2002, 3, 4, 5, 6, 7);
	lt->ltype = 2;
	lt->l1 = 3;
	lt->l2 = 4;
	lt->pind = 5;
	lt->p1 = 6;
	lt->p2 = 7;
	CHECKED(dba_db_data_get_id(lt, &id));
	ensure_equals(id, 2);

	// Get info on the first data
	CHECKED(dba_db_data_get_data(lt, 1));
	ensure_equals(lt->id_ana, 1);
	ensure_equals(lt->id_report, 1);
	ensure_equals(lt->date, string("2001-02-03 04:05:06"));
	ensure_equals(lt->date_ind, 19);
	ensure_equals(lt->ltype, 1);
	ensure_equals(lt->l1, 2);
	ensure_equals(lt->l2, 3);
	ensure_equals(lt->pind, 4);
	ensure_equals(lt->p1, 5);
	ensure_equals(lt->p2, 6);

	// Get info on the second data
	CHECKED(dba_db_data_get_data(lt, 2));
	ensure_equals(lt->id_ana, 2);
	ensure_equals(lt->id_report, 2);
	ensure_equals(lt->date, string("2002-03-04 05:06:07"));
	ensure_equals(lt->date_ind, 19);
	ensure_equals(lt->ltype, 2);
	ensure_equals(lt->l1, 3);
	ensure_equals(lt->l2, 4);
	ensure_equals(lt->pind, 5);
	ensure_equals(lt->p1, 6);
	ensure_equals(lt->p2, 7);
#endif

#if 0
	// Update the second data
	lt->id = 2;
	lt->id_ana = 2;
	lt->id_report = 2;
	lt->date_ind = snprintf(lt->date, 25, "%04d-%02d-%02d %02d:%02d:%02d", 2003, 4, 5, 6, 7, 8);
	lt->ltype = 3;
	lt->l1 = 4;
	lt->l2 = 5;
	lt->pind = 6;
	lt->p1 = 7;
	lt->p2 = 8;
	CHECKED(dba_db_data_update(lt));

	// Get info on the first station: it should be unchanged
	CHECKED(dba_db_data_get_data(pa, 1));
	ensure_equals(pa->lat, 4500000);
	ensure_equals(pa->lon, 1100000);
	ensure_equals(pa->ident, string("ciao"));
	ensure_equals(pa->ident_ind, 4);

	// Get info on the second station: it should be updated
	CHECKED(dba_db_data_get_data(pa, 2));
	ensure_equals(pa->lat, 4700000);
	ensure_equals(pa->lon, 1300000);
	ensure_equals(pa->ident[0], 0);
#endif
}

}

/* vim:set ts=4 sw=4: */