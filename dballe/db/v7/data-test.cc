#include "db/tests.h"
#include "sql/sql.h"
#include "db/v7/db.h"
#include "db/v7/transaction.h"
#include "db/v7/driver.h"
#include "db/v7/repinfo.h"
#include "db/v7/station.h"
#include "db/v7/levtr.h"
#include "db/v7/data.h"
#include "config.h"

using namespace dballe;
using namespace dballe::tests;
using namespace wreport;
using namespace std;

namespace {

struct Fixture : V7DBFixture
{
    using V7DBFixture::V7DBFixture;

    std::unique_ptr<dballe::db::v7::Transaction> t;
    db::v7::StationDesc sde1;
    db::v7::StationDesc sde2;
    db::v7::levtrs_t::iterator lt1;
    db::v7::levtrs_t::iterator lt2;

    Fixture(const char* backend)
        : V7DBFixture(backend)
    {
        sde1.rep = 1;
        sde1.coords = Coords(4500000, 1100000);
        sde1.ident = "ciao";

        sde2.rep = 1;
        sde2.coords = Coords(4600000, 1200000);
        sde2.ident = nullptr;
    }

    void reset_data()
    {
        db->disappear();
        db->reset();

        db::v7::stations_t::iterator si;
        db::v7::levtrs_t::iterator li;

        auto transaction = db->transaction();
        auto tp = dynamic_cast<dballe::db::v7::Transaction*>(transaction.get());
        wassert_true(tp);
        t.reset(tp);
        transaction.release();

        // Insert a mobile station
        si = db->station().obtain_id(t->state, sde1);

        // Insert a fixed station
        si = db->station().obtain_id(t->state, sde2);

        // Insert a lev_tr
        lt1 = db->levtr().obtain_id(t->state, db::v7::LevTrDesc(Level(1, 2, 0, 3), Trange(4, 5, 6)));

        // Insert another lev_tr
        lt2 = db->levtr().obtain_id(t->state, db::v7::LevTrDesc(Level(2, 3, 1, 4), Trange(5, 6, 7)));
    }

    void test_setup()
    {
        V7DBFixture::test_setup();
        reset_data();
    }

    void test_teardown()
    {
        V7DBFixture::test_teardown();
        t.reset();
    }
};


class Tests : public DBFixtureTestCase<Fixture>
{
    using DBFixtureTestCase::DBFixtureTestCase;

    void register_tests() override;
};

Tests tg1("db_v7_data_sqlite", "SQLITE");
#ifdef HAVE_LIBPQ
Tests tg3("db_v7_data_postgresql", "POSTGRESQL");
#endif
#ifdef HAVE_MYSQL
Tests tg4("db_v7_data_mysql", "MYSQL");
#endif


void Tests::register_tests()
{

add_method("insert", [](Fixture& f) {
    using namespace dballe::db::v7;
    auto& da = f.db->data();

    Var var(varinfo(WR_VAR(0, 1, 2)));

    auto insert_sample1 = [&](bulk::InsertVars& vars, int value, bulk::UpdateMode update) {
        vars.shared_context.station = f.t->state.stations.find(f.sde1);
        vars.shared_context.datetime = Datetime(2001, 2, 3, 4, 5, 6);
        var.seti(value);
        vars.add(&var, f.lt1->second);
        wassert(da.insert(*f.t, vars, update));
    };

    // Insert a datum
    {
        bulk::InsertVars vars(f.t->state);
        wassert(insert_sample1(vars, 123, bulk::ERROR));
        wassert(actual(vars[0].cur->second.id) == 1);
        wassert(actual(vars[0].needs_insert()).isfalse());
        wassert(actual(vars[0].inserted()).istrue());
        wassert(actual(vars[0].needs_update()).isfalse());
        wassert(actual(vars[0].updated()).isfalse());
    }

    // Insert another datum
    {
        bulk::InsertVars vars(f.t->state);
        vars.shared_context.station = f.t->state.stations.find(f.sde2);
        vars.shared_context.datetime = Datetime(2002, 3, 4, 5, 6, 7);
        Var var(varinfo(WR_VAR(0, 1, 2)), 234);
        vars.add(&var, f.lt2->second);
        wassert(da.insert(*f.t, vars, bulk::ERROR));
        wassert(actual(vars[0].cur->second.id) == 2);
        wassert(actual(vars[0].needs_insert()).isfalse());
        wassert(actual(vars[0].inserted()).istrue());
        wassert(actual(vars[0].needs_update()).isfalse());
        wassert(actual(vars[0].updated()).isfalse());
    }

    // Reinsert the first datum: it should give an error, since V7 does
    // not check if old and new values are the same
    {
        bulk::InsertVars vars(f.t->state);
        bool successful = false;
        try {
            insert_sample1(vars, 123, bulk::ERROR);
            successful = true;
        } catch (std::exception& e) {
            wassert(actual(e.what()).matches("refusing to overwrite existing data"));
        }
        wassert(actual(successful).isfalse());
    }

    // Reinsert the first datum, with a different value and ignore
    // overwrite: it should find its ID and do nothing
    {
        bulk::InsertVars vars(f.t->state);
        wassert(insert_sample1(vars, 125, bulk::IGNORE));
        wassert(actual(vars[0].cur->second.id) == 1);
        wassert(actual(vars[0].needs_insert()).isfalse());
        wassert(actual(vars[0].inserted()).isfalse());
        wassert(actual(vars[0].needs_update()).istrue());
        wassert(actual(vars[0].updated()).isfalse());
    }

    // Reinsert the first datum, with a different value and overwrite:
    // it should find its ID and update it
    {
        bulk::InsertVars vars(f.t->state);
        wassert(insert_sample1(vars, 125, bulk::UPDATE));
        wassert(actual(vars[0].cur->second.id) == 1);
        wassert(actual(vars[0].needs_insert()).isfalse());
        wassert(actual(vars[0].inserted()).isfalse());
        wassert(actual(vars[0].needs_update()).isfalse());
        wassert(actual(vars[0].updated()).istrue());
    }

    // Reinsert the first datum, with the same value and error on
    // overwrite: it should find its ID and do nothing, because the value
    // does not change.
    {
        bulk::InsertVars vars(f.t->state);
        bool successful = false;
        try {
            insert_sample1(vars, 125, bulk::ERROR);
            successful = true;
        } catch (std::exception& e) {
            wassert(actual(e.what()).matches("refusing to overwrite existing data"));
        }
        wassert(actual(successful).isfalse());
    }

    // Reinsert the first datum, with a different value and error on
    // overwrite: it should find the ID and skip the update
    {
        bulk::InsertVars vars(f.t->state);
        wassert(insert_sample1(vars, 126, bulk::IGNORE));
        wassert(actual(vars[0].cur->second.id) == 1);
        wassert(actual(vars[0].needs_insert()).isfalse());
        wassert(actual(vars[0].inserted()).isfalse());
        wassert(actual(vars[0].needs_update()).istrue());
        wassert(actual(vars[0].updated()).isfalse());
    }
});

add_method("attrs", [](Fixture& f) {
    using namespace dballe::db::v7;
    auto& da = f.db->data();

    Var var(varinfo(WR_VAR(0, 1, 2)), 123);

    // Insert a datum with attributes
    bulk::InsertVars vars(f.t->state);
    vars.shared_context.station = f.t->state.stations.find(f.sde1);
    vars.shared_context.datetime = Datetime(2001, 2, 3, 4, 5, 6);
    var.seta(newvar(WR_VAR(0, 33, 7), 50));
    vars.add(&var, f.lt1->second);
    wassert(da.insert(*f.t, vars, bulk::ERROR, true));
    int id = vars[0].cur->second.id;

    vector<wreport::Var> attrs;
    da.read_attrs(id, [&](std::unique_ptr<wreport::Var> a) {
        attrs.emplace_back(*a);
    });

    wassert(actual(attrs.size()) == 1);
    wassert(actual_varcode(attrs[0].code()) == WR_VAR(0, 33, 7));
    wassert(actual(attrs[0]) == 50);
});

}

}
