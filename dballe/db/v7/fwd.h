#ifndef DBALLE_DB_V7_FWD_H
#define DBALLE_DB_V7_FWD_H

namespace dballe {
namespace db {
namespace v7 {

struct Transaction;
struct QueryBuilder;
struct StationQueryBuilder;
struct DataQueryBuilder;
struct SummaryQueryBuilder;
struct IdQueryBuilder;
struct DB;
struct Repinfo;
struct Station;
struct LevTr;
struct SQLTrace;
struct Driver;

namespace batch {
struct Station;
struct StationDatum;
struct MeasuredDatum;
}

namespace trace {
struct Step;
}

template<typename Step=trace::Step>
class Tracer;

}
}
}

#endif
