#define _DBALLE_TEST_CODE
#include "dballe/db/tests.h"
#include "dballe/db/v7/db.h"
#include "dballe/db/v7/transaction.h"
#include "explorer.h"
#include "config.h"

using namespace dballe;
using namespace dballe::db;
using namespace dballe::tests;
using namespace wreport;
using namespace std;

namespace {

template<typename DB>
class Tests : public FixtureTestCase<EmptyTransactionFixture<DB>>
{
    typedef EmptyTransactionFixture<DB> Fixture;
    using FixtureTestCase<Fixture>::FixtureTestCase;

    void register_tests() override;
};

Tests<V7DB> tg6("db_explorer_v7_sqlite", "SQLITE");
#ifdef HAVE_LIBPQ
Tests<V7DB> tg7("db_explorer_v7_postgresql", "POSTGRESQL");
#endif
#ifdef HAVE_MYSQL
Tests<V7DB> tg8("db_explorer_v7_mysql", "MYSQL");
#endif

template<typename DB>
void Tests<DB>::register_tests()
{

this->add_method("filter_rep_memo", [](Fixture& f) {
    // Test building a summary and checking if it supports queries
    OldDballeTestDataSet test_data;
    wassert(f.populate(test_data));

    Explorer explorer;
    explorer.revalidate(*f.tr);

    core::Query query;
    query.set_from_test_string("rep_memo=metar");
    explorer.set_filter(query);

    vector<Station> stations;

    stations.clear();
    for (const auto& s: explorer.global_summary().stations())
        stations.push_back(s.station);
    wassert(actual(stations.size()) == 2);
    wassert(actual(stations[0].report) == "metar");
    wassert(actual(stations[1].report) == "synop");

    stations.clear();
    for (const auto& s: explorer.active_summary().stations())
        stations.push_back(s.station);
    wassert(actual(stations.size()) == 1);
    wassert(actual(stations[0].report) == "metar");

    vector<string> reports;

    reports.clear();
    for (const auto& v: explorer.global_summary().reports())
        reports.push_back(v);
    wassert(actual(reports.size()) == 2);
    wassert(actual(reports[0]) == "metar");
    wassert(actual(reports[1]) == "synop");

    reports.clear();
    for (const auto& v: explorer.active_summary().reports())
        reports.push_back(v);
    wassert(actual(reports.size()) == 1);
    wassert(actual(reports[0]) == "metar");

    vector<Level> levels;

    levels.clear();
    for (const auto& v: explorer.global_summary().levels())
        levels.push_back(v);
    wassert(actual(levels.size()) == 1);
    wassert(actual(levels[0]) == Level(10, 11, 15, 22));

    levels.clear();
    for (const auto& v: explorer.active_summary().levels())
        levels.push_back(v);
    wassert(actual(levels.size()) == 1);
    wassert(actual(levels[0]) == Level(10, 11, 15, 22));

    vector<Trange> tranges;

    tranges.clear();
    for (const auto& v: explorer.global_summary().tranges())
        tranges.push_back(v);
    wassert(actual(tranges.size()) == 2);
    wassert(actual(tranges[0]) == Trange(20, 111, 122));
    wassert(actual(tranges[1]) == Trange(20, 111, 123));

    tranges.clear();
    for (const auto& v: explorer.active_summary().tranges())
        tranges.push_back(v);
    wassert(actual(tranges.size()) == 1);
    wassert(actual(tranges[0]) == Trange(20, 111, 123));

    vector<wreport::Varcode> vars;

    vars.clear();
    for (const auto& v: explorer.global_summary().varcodes())
        vars.push_back(v);
    wassert(actual(vars.size()) == 2);
    wassert(actual(vars[0]) == WR_VAR(0, 1, 11));
    wassert(actual(vars[1]) == WR_VAR(0, 1, 12));

    vars.clear();
    for (const auto& v: explorer.active_summary().varcodes())
        vars.push_back(v);
    wassert(actual(vars.size()) == 2);
    wassert(actual(vars[0]) == WR_VAR(0, 1, 11));
    wassert(actual(vars[1]) == WR_VAR(0, 1, 12));
});

#if 0
// tests for supports() implementation (currently not used)
    // Check what it can support

    // An existing station is ok: we know we have it
    wassert(actual(s.supports(*query_from_string("ana_id=1"))) == summary::Support::EXACT);

    // A non-existing station is also ok: we know we don't have it
    wassert(actual(s.supports(*query_from_string("ana_id=2"))) == summary::Support::EXACT);

    wassert(actual(s.supports(*query_from_string("ana_id=1, leveltype1=10"))) == summary::Support::EXACT);

    wassert(actual(s.supports(*query_from_string("ana_id=1, leveltype1=10, pindicator=20"))) == summary::Support::EXACT);

    wassert(actual(s.supports(*query_from_string("ana_id=1, leveltype1=10, pindicator=20"))) == summary::Support::EXACT);

    // Still exact, because the query matches the entire summary
    wassert(actual(s.supports(*query_from_string("yearmin=1945"))) == summary::Support::EXACT);

    // Still exact, because although the query partially matches the summary,
    // each summary entry is entier included completely or excluded completely
    wassert(actual(s.supports(*query_from_string("yearmin=1945, monthmin=4, daymin=25, hourmin=8, yearmax=1945, monthmax=4, daymax=25, hourmax=8, minumax=10"))) == summary::Support::EXACT);
#endif

}

}
