#!/usr/bin/python
# coding: utf-8
from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals
import dballe
from dballe import dbacsv
import datetime
import unittest
import io

class TestCSV(unittest.TestCase):
    def setUp(self):
        from testlib import fill_volnd
        self.db = dballe.DB.connect_test()
        fill_volnd(self.db)

    def testExport(self):
        query = dballe.Record()
        #query["rep_memo"] = "synop"
        out = io.StringIO()
        dbacsv.export(self.db, query, out)

        lines = out.getvalue().splitlines()
        self.assertEquals(lines[0],
                          "Station B01001: 12; Station B01002: 123; Station B01019: Test of long station name,,,,,,,,,,,,,,,")
        self.assertEquals(lines[1],
                          "Station,Latitude,Longitude,Network,Datetime,Level1,L1,Level2,L2,Time range,P1,P2,Variable,Value,Attr B33007,Attr B33040")
        self.assertEquals(lines[2],
                          "1,10.0,15.0,synop,2007-01-01 00:00:00,1,-,-,-,0,-,-,B10004,73810,80,")

    def testAttrs(self):
        self.db.reset()
        data = dballe.Record()
        data.update(
            rep_memo="synop",
            lat=0.0, lon=0.0,
            ident="#000000",
            level=(103, 2000),
            trange=(254, 0, 0),
            date=datetime.datetime(1005, 1, 1, 1, 1, 0),
            B12101=270.96)
        self.db.insert(data, False, True)
        attrs = dballe.Record()
        attrs["B33209"] = 98
        self.db.attr_insert("B12101", attrs)

        data.update(
            rep_memo="synop",
            lat=0.0, lon=0.0,
            ident="#000000",
            level=(103, 2000),
            trange=(254, 0, 0),
            date=datetime.datetime(1005, 1, 1, 1, 1, 1),
            B12101=271.96)
        self.db.insert(data, False, True)
        attrs = dballe.Record()
        attrs["B33209"] = 100
        self.db.attr_insert("B12101", attrs)

        query = dballe.Record()
        out = io.StringIO()
        dbacsv.export(self.db, query, out)

        lines = out.getvalue().splitlines()
        self.assertEquals(lines[2], "1005-01-01 01:01:00,270.96,98")
        self.assertEquals(lines[3], "1005-01-01 01:01:01,271.96,100")


if __name__ == "__main__":
    from testlib import main
    main("test_csv")
