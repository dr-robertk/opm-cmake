/*
  Copyright 2018 Statoil ASA.

  This file is part of the Open Porous Media project (OPM).

  OPM is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  OPM is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with OPM.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdexcept>
#include <iostream>
#include <boost/filesystem.hpp>

#define BOOST_TEST_MODULE ACTIONX

#include <boost/test/unit_test.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <ert/util/util.h>

#include <opm/parser/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/SummaryState.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Action/ActionAST.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Action/ActionContext.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Action/Actions.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Action/ActionX.hpp>
#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/Parser/ParseContext.hpp>
#include <opm/parser/eclipse/Parser/ErrorGuard.hpp>

using namespace Opm;




BOOST_AUTO_TEST_CASE(Create) {
    const auto action_kw = std::string{ R"(
ACTIONX
   'ACTION' /
   WWCT OPX  > 0.75 /
/
)"};
    ActionX action1("NAME", 10, 100, 0);
    BOOST_CHECK_EQUAL(action1.name(), "NAME");

    const auto deck = Parser{}.parseString( action_kw );
    const auto& kw = deck.getKeyword("ACTIONX");

    ActionX action2(kw, 0);
    BOOST_CHECK_EQUAL(action2.name(), "ACTION");
}


BOOST_AUTO_TEST_CASE(SCAN) {
    const auto MISSING_END= std::string{ R"(
SCHEDULE

ACTIONX
   'ACTION' /
   WWCT OPX  > 0.75 /
/

TSTEP
   10 /
)"};

    const auto WITH_WELSPECS = std::string{ R"(
SCHEDULE

WELSPECS
  'W2'  'OP'  1 1 3.33  'OIL' 7*/
/

ACTIONX
   'ACTION' /
   WWCT OPX  > 0.75 /
/

WELSPECS
  'W1'  'OP'  1 1 3.33  'OIL' 7*/
/

ENDACTIO

TSTEP
   10 /
)"};

    const auto WITH_GRID = std::string{ R"(
SCHEDULE

WELSPECS
  'W2'  'OP'  1 1 3.33  'OIL' 7*/
/

ACTIONX
   'ACTION' /
   WWCT OPX  > 0.75 /
/

PORO
  100*0.78 /

ENDACTIO

TSTEP
   10 /
)"};
    Opm::Parser parser;
    auto deck1 = parser.parseString(MISSING_END);
    auto deck2 = parser.parseString(WITH_WELSPECS);
    auto deck3 = parser.parseString(WITH_GRID);
    EclipseGrid grid1(10,10,10);
    TableManager table ( deck1 );
    Eclipse3DProperties eclipseProperties ( deck1 , table, grid1);
    Runspec runspec (deck1);

    // The ACTIONX keyword has no matching 'ENDACTIO' -> exception
    BOOST_CHECK_THROW(Schedule(deck1, grid1, eclipseProperties, runspec ), std::invalid_argument);

    Schedule sched(deck2, grid1, eclipseProperties, runspec);
    BOOST_CHECK( !sched.hasWell("W1") );
    BOOST_CHECK( sched.hasWell("W2"));

    // The deck3 contains the 'GRID' keyword in the ACTIONX block - that is not a whitelisted keyword.
    ParseContext parseContext( {{ParseContext::ACTIONX_ILLEGAL_KEYWORD, InputError::THROW_EXCEPTION}} );
    ErrorGuard errors;
    BOOST_CHECK_THROW(Schedule(deck3, grid1, eclipseProperties, runspec, parseContext, errors), std::invalid_argument);
}


BOOST_AUTO_TEST_CASE(TestActions) {
    Opm::SummaryState st;
    Opm::ActionContext context(st);
    Opm::Actions config;
    std::vector<std::string> matching_wells;
    BOOST_CHECK_EQUAL(config.size(), 0);
    BOOST_CHECK(config.empty());

    Opm::ActionX action1("NAME", 10, 100, 0);
    config.add(action1);
    BOOST_CHECK_EQUAL(config.size(), 1);
    BOOST_CHECK(!config.empty());

    double min_wait = 86400;
    size_t max_eval = 3;
    {
        Opm::ActionX action("NAME", max_eval, min_wait, util_make_date_utc(1, 7, 2000));
        config.add(action);
        BOOST_CHECK_EQUAL(config.size(), 1);


        Opm::ActionX action3("NAME3", 1000000, 0, util_make_date_utc(1,7,2000));
        config.add(action3);
    }
    Opm::ActionX& action2 = config.at("NAME");
    // The action2 instance has an empty condition, so it will never evaluate to true.
    BOOST_CHECK(action2.ready(util_make_date_utc(1,7,2000)));
    BOOST_CHECK(!action2.ready(util_make_date_utc(1,6,2000)));
    BOOST_CHECK(!action2.eval(util_make_date_utc(1,6,2000), context, matching_wells));

    auto pending = config.pending( util_make_date_utc(7,8,2000));
    BOOST_CHECK_EQUAL( pending.size(), 2);
    for (auto& ptr : pending) {
        BOOST_CHECK( ptr->ready(util_make_date_utc(7,8,2000)));
        BOOST_CHECK( !ptr->eval(util_make_date_utc(7,8,2000), context, matching_wells));
    }

    BOOST_CHECK(!action2.eval(util_make_date_utc(7,8,2000), context, matching_wells ));
}



BOOST_AUTO_TEST_CASE(TestContext) {
    Opm::SummaryState st;
    st.update_well_var("OP1", "WOPR", 100);
    Opm::ActionContext context(st);

    BOOST_REQUIRE_THROW(context.get("func", "arg"), std::out_of_range);

    context.add("FUNC", "ARG", 100);
    BOOST_CHECK_EQUAL(context.get("FUNC", "ARG"), 100);

    const auto& wopr_wells = context.wells("WOPR");
    BOOST_CHECK_EQUAL(wopr_wells.size(), 1);
    BOOST_CHECK_EQUAL(wopr_wells[0], "OP1");

    const auto& wwct_wells = context.wells("WWCT");
    BOOST_CHECK_EQUAL(wwct_wells.size(), 0);
}



Opm::Schedule make_action(const std::string& action_string) {
    std::string start = std::string{ R"(
SCHEDULE
)"};
    std::string end = std::string{ R"(
ENDACTIO

TSTEP
   10 /
)"};

    std::string deck_string = start + action_string + end;
    Opm::Parser parser;
    auto deck = parser.parseString(deck_string);
    EclipseGrid grid1(10,10,10);
    TableManager table ( deck );
    Eclipse3DProperties eclipseProperties ( deck , table, grid1);
    Runspec runspec(deck);

    return Schedule(deck, grid1, eclipseProperties, runspec);
}


BOOST_AUTO_TEST_CASE(TestActionAST_BASIC) {
    // Missing comparator
    BOOST_REQUIRE_THROW( ActionAST( {"WWCT", "OPX", "0.75"} ), std::invalid_argument);

    // Left hand side must be function expression
    BOOST_REQUIRE_THROW( ActionAST({"0.75", "<", "1.0"}), std::invalid_argument);

    //Extra data
    BOOST_REQUIRE_THROW(ActionAST({"0.75", "<", "1.0", "EXTRA"}), std::invalid_argument);

    ActionAST ast1({"WWCT", "OPX", ">", "0.75"});
    ActionAST ast2({"WWCT", "OPX", "=", "WWCT", "OPX"});
    ActionAST ast3({"WWCT", "OPY", ">", "0.75"});
    SummaryState st;
    ActionContext context(st);
    std::vector<std::string> matching_wells;

    context.add("WWCT", "OPX", 100);
    BOOST_CHECK(ast1.eval(context, matching_wells));

    context.add("WWCT", "OPX", -100);
    BOOST_CHECK(!ast1.eval(context, matching_wells));

    BOOST_CHECK(ast2.eval(context, matching_wells));
    BOOST_REQUIRE_THROW(ast3.eval(context, matching_wells), std::out_of_range);
}

BOOST_AUTO_TEST_CASE(TestActionAST_OR_AND) {
    ActionAST ast_or({"WWCT", "OPX", ">", "0.75", "OR", "WWCT", "OPY", ">", "0.75"});
    ActionAST ast_and({"WWCT", "OPX", ">", "0.75", "AND", "WWCT", "OPY", ">", "0.75"});
    ActionAST par({"WWCT", "OPX", ">", "0.75", "AND", "(", "WWCT", "OPY", ">", "0.75", "OR", "WWCT", "OPZ", ">", "0.75", ")"});
    SummaryState st;
    ActionContext context(st);
    std::vector<std::string> matching_wells;

    context.add("WWCT", "OPX", 100);
    context.add("WWCT", "OPY", -100);
    context.add("WWCT", "OPZ", 100);
    BOOST_CHECK( ast_or.eval(context, matching_wells) );
    BOOST_CHECK( !ast_and.eval(context, matching_wells) );
    BOOST_CHECK( par.eval(context, matching_wells));


    context.add("WWCT", "OPX", -100);
    context.add("WWCT", "OPY", 100);
    context.add("WWCT", "OPZ", 100);
    BOOST_CHECK( ast_or.eval(context, matching_wells) );
    BOOST_CHECK( !ast_and.eval(context, matching_wells) );
    BOOST_CHECK( !par.eval(context, matching_wells));


    context.add("WWCT", "OPX", 100);
    context.add("WWCT", "OPY", 100);
    context.add("WWCT", "OPZ", -100);
    BOOST_CHECK( ast_or.eval(context, matching_wells) );
    BOOST_CHECK( ast_and.eval(context, matching_wells) );
    BOOST_CHECK( par.eval(context, matching_wells));

    context.add("WWCT", "OPX", -100);
    context.add("WWCT", "OPY", -100);
    context.add("WWCT", "OPZ", -100);
    BOOST_CHECK( !ast_or.eval(context, matching_wells) );
    BOOST_CHECK( !ast_and.eval(context, matching_wells) );
    BOOST_CHECK( !par.eval(context, matching_wells));
}

BOOST_AUTO_TEST_CASE(DATE) {
    ActionAST ast({"MNTH", ">=", "JUN"});
    SummaryState st;
    ActionContext context(st);
    std::vector<std::string> matching_wells;

    context.add("MNTH", 6);
    BOOST_CHECK( ast.eval(context, matching_wells) );

    context.add("MNTH", 8);
    BOOST_CHECK( ast.eval(context, matching_wells) );

    context.add("MNTH", 5);
    BOOST_CHECK( !ast.eval(context, matching_wells) );
}


BOOST_AUTO_TEST_CASE(MANUAL1) {
    ActionAST ast({"GGPR", "FIELD", ">", "50000", "AND", "WGOR", "PR", ">" ,"GGOR", "FIELD"});
    SummaryState st;
    ActionContext context(st);
    std::vector<std::string> matching_wells;

    context.add("GGPR", "FIELD", 60000 );
    context.add("WGOR", "PR" , 300 );
    context.add("GGOR", "FIELD", 200);
    BOOST_CHECK( ast.eval(context, matching_wells) );

    context.add("GGPR", "FIELD", 0 );
    context.add("WGOR", "PR" , 300 );
    context.add("GGOR", "FIELD", 200);
    BOOST_CHECK( !ast.eval(context, matching_wells) );

    context.add("GGPR", "FIELD", 60000 );
    context.add("WGOR", "PR" , 100 );
    context.add("GGOR", "FIELD", 200);
    BOOST_CHECK( !ast.eval(context, matching_wells) );
}

BOOST_AUTO_TEST_CASE(MANUAL2) {
    ActionAST ast({"GWCT", "LIST1", ">", "0.70", "AND", "(", "GWPR", "LIST1", ">", "GWPR", "LIST2", "OR", "GWPR", "LIST1", ">", "GWPR", "LIST3", ")"});
    SummaryState st;
    ActionContext context(st);
    std::vector<std::string> matching_wells;

    context.add("GWCT", "LIST1", 1.0);
    context.add("GWPR", "LIST1", 1 );
    context.add("GWPR", "LIST2", 2 );
    context.add("GWPR", "LIST3", 3 );
    BOOST_CHECK( !ast.eval(context, matching_wells));

    context.add("GWCT", "LIST1", 1.0);
    context.add("GWPR", "LIST1", 1 );
    context.add("GWPR", "LIST2", 2 );
    context.add("GWPR", "LIST3", 0 );
    BOOST_CHECK( ast.eval(context, matching_wells));

    context.add("GWCT", "LIST1", 1.0);
    context.add("GWPR", "LIST1", 1 );
    context.add("GWPR", "LIST2", 0 );
    context.add("GWPR", "LIST3", 3 );
    BOOST_CHECK( ast.eval(context, matching_wells));

    context.add("GWCT", "LIST1", 1.0);
    context.add("GWPR", "LIST1", 1 );
    context.add("GWPR", "LIST2", 0 );
    context.add("GWPR", "LIST3", 0 );
    BOOST_CHECK( ast.eval(context, matching_wells));

    context.add("GWCT", "LIST1", 0.0);
    context.add("GWPR", "LIST1", 1 );
    context.add("GWPR", "LIST2", 0 );
    context.add("GWPR", "LIST3", 3 );
    BOOST_CHECK( !ast.eval(context, matching_wells));
}

BOOST_AUTO_TEST_CASE(MANUAL3) {
    ActionAST ast({"MNTH", ".GE.", "MAR", "AND", "MNTH", ".LE.", "OCT", "AND", "GMWL", "HIGH", ".GE.", "4"});
    SummaryState st;
    ActionContext context(st);
    std::vector<std::string> matching_wells;

    context.add("MNTH", 4);
    context.add("GMWL", "HIGH", 4);
    BOOST_CHECK( ast.eval(context, matching_wells));

    context.add("MNTH", 3);
    context.add("GMWL", "HIGH", 4);
    BOOST_CHECK( ast.eval(context, matching_wells));

    context.add("MNTH", 11);
    context.add("GMWL", "HIGH", 4);
    BOOST_CHECK( !ast.eval(context, matching_wells));

    context.add("MNTH", 3);
    context.add("GMWL", "HIGH", 3);
    BOOST_CHECK( !ast.eval(context, matching_wells));
}


BOOST_AUTO_TEST_CASE(MANUAL4) {
    ActionAST ast({"GWCT", "FIELD", ">", "0.8", "AND", "DAY", ">", "1", "AND", "MNTH", ">", "JUN", "AND", "YEAR", ">=", "2021"});
    SummaryState st;
    ActionContext context(st);
    std::vector<std::string> matching_wells;


    context.add("MNTH", 7);
    context.add("DAY", 2);
    context.add("YEAR", 2030);
    context.add("GWCT", "FIELD", 1.0);
    BOOST_CHECK( ast.eval(context, matching_wells) );

    context.add("MNTH", 7);
    context.add("DAY", 2);
    context.add("YEAR", 2019);
    context.add("GWCT", "FIELD", 1.0);
    BOOST_CHECK( !ast.eval(context, matching_wells) );
}



BOOST_AUTO_TEST_CASE(MANUAL5) {
    ActionAST ast({"WCG2", "PROD1", ">", "WCG5", "PROD2", "AND", "GCG3", "G1", ">", "GCG7", "G2", "OR", "FCG1", ">", "FCG7"});
    SummaryState st;
    ActionContext context(st);
    std::vector<std::string> matching_wells;

    context.add("WCG2", "PROD1", 100);
    context.add("WCG5", "PROD2",  50);
    context.add("GCG3", "G1", 200);
    context.add("GCG7", "G2", 100);
    context.add("FCG1", 100);
    context.add("FCG7",  50);
    BOOST_CHECK(ast.eval(context, matching_wells));

    context.add("WCG2", "PROD1", 100);
    context.add("WCG5", "PROD2",  50);
    context.add("GCG3", "G1", 200);
    context.add("GCG7", "G2", 100);
    context.add("FCG1", 100);
    context.add("FCG7", 150);
    BOOST_CHECK(ast.eval(context, matching_wells));

    context.add("WCG2", "PROD1", 100);
    context.add("WCG5", "PROD2",  50);
    context.add("GCG3", "G1", 20);
    context.add("GCG7", "G2", 100);
    context.add("FCG1", 100);
    context.add("FCG7", 150);
    BOOST_CHECK(!ast.eval(context, matching_wells));

    context.add("WCG2", "PROD1", 100);
    context.add("WCG5", "PROD2",  50);
    context.add("GCG3", "G1", 20);
    context.add("GCG7", "G2", 100);
    context.add("FCG1", 200);
    context.add("FCG7", 150);
    BOOST_CHECK(ast.eval(context, matching_wells));
}



BOOST_AUTO_TEST_CASE(LGR) {
    ActionAST ast({"LWCC" , "OPX", "LOCAL", "1", "2", "3", ">", "100"});
    SummaryState st;
    ActionContext context(st);
    std::vector<std::string> matching_wells;

    context.add("LWCC", "OPX:LOCAL:1:2:3", 200);
    BOOST_CHECK(ast.eval(context, matching_wells));

    context.add("LWCC", "OPX:LOCAL:1:2:3", 20);
    BOOST_CHECK(!ast.eval(context, matching_wells));
}


BOOST_AUTO_TEST_CASE(ActionContextTest) {
    SummaryState st;
    st.update("WWCT:OP1", 100);
    ActionContext context(st);


    BOOST_CHECK_EQUAL(context.get("WWCT", "OP1"), 100);
    BOOST_REQUIRE_THROW(context.get("WGOR", "B37"), std::out_of_range);
    context.add("WWCT", "OP1", 200);

    BOOST_CHECK_EQUAL(context.get("WWCT", "OP1"), 200);
    BOOST_REQUIRE_THROW(context.get("WGOR", "B37"), std::out_of_range);
}



BOOST_AUTO_TEST_CASE(TestMatchingWells) {
    ActionAST ast({"WOPR", "*", ">", "1.0"});
    SummaryState st;
    std::vector<std::string> matching_wells;

    st.update_well_var("OPX", "WOPR", 0);
    st.update_well_var("OPY", "WOPR", 0.50);
    st.update_well_var("OPZ", "WOPR", 2.0);

    ActionContext context(st);
    BOOST_CHECK( ast.eval(context, matching_wells) );

    BOOST_CHECK_EQUAL( matching_wells.size(), 1);
    BOOST_CHECK_EQUAL( matching_wells[0], "OPZ" );
}

BOOST_AUTO_TEST_CASE(TestMatchingWells2) {
  ActionAST ast1({"WOPR", "P*", ">", "1.0"});
  ActionAST ast2({"WOPR", "*", ">", "1.0"});
  SummaryState st;
  std::vector<std::string> matching_wells1;
  std::vector<std::string> matching_wells2;

  st.update_well_var("PX", "WOPR", 0);
  st.update_well_var("PY", "WOPR", 0.50);
  st.update_well_var("PZ", "WOPR", 2.0);

  st.update_well_var("IX", "WOPR", 0);
  st.update_well_var("IY", "WOPR", 0.50);
  st.update_well_var("IZ", "WOPR", 2.0);

  ActionContext context(st);
  BOOST_CHECK( ast1.eval(context, matching_wells1) );
  BOOST_CHECK_EQUAL( matching_wells1.size(), 1);
  BOOST_CHECK_EQUAL( matching_wells1[0], "PZ" );

  BOOST_CHECK( ast2.eval(context, matching_wells2) );
  BOOST_CHECK_EQUAL( matching_wells2.size(), 2);
  BOOST_CHECK_EQUAL( std::count(matching_wells2.begin(), matching_wells2.end(), "PZ") , 1);
  BOOST_CHECK_EQUAL( std::count(matching_wells2.begin(), matching_wells2.end(), "IZ") , 1);
}



BOOST_AUTO_TEST_CASE(TestMatchingWells_AND) {
    ActionAST ast({"WOPR", "*", ">", "1.0", "AND", "WWCT", "*", "<", "0.50"});
    SummaryState st;
    std::vector<std::string> matching_wells;

    st.update_well_var("OPX", "WOPR", 0);
    st.update_well_var("OPY", "WOPR", 0.50);
    st.update_well_var("OPZ", "WOPR", 2.0);      // The WOPR check matches this well.

    st.update_well_var("OPX", "WWCT", 1.0);
    st.update_well_var("OPY", "WWCT", 0.0);     // The WWCT check matches this well.
    st.update_well_var("OPZ", "WWCT", 1.0);

    ActionContext context(st);
    BOOST_CHECK( ast.eval(context, matching_wells) );

    // Even though condition as a whole matches, there is no finite set of wells
    // which mathes both conditions when combined with AND - i.e. the matching_wells
    // variable should be empty.
    BOOST_CHECK( matching_wells.empty() );
}

BOOST_AUTO_TEST_CASE(TestMatchingWells_OR) {
    ActionAST ast({"WOPR", "*", ">", "1.0", "OR", "WWCT", "*", "<", "0.50"});
    SummaryState st;
    std::vector<std::string> matching_wells;

    st.update_well_var("OPX", "WOPR", 0);
    st.update_well_var("OPY", "WOPR", 0.50);
    st.update_well_var("OPZ", "WOPR", 2.0);      // The WOPR check matches this well.

    st.update_well_var("OPX", "WWCT", 1.0);
    st.update_well_var("OPY", "WWCT", 0.0);     // The WWCT check matches this well.
    st.update_well_var("OPZ", "WWCT", 1.0);

    ActionContext context(st);
    BOOST_CHECK( ast.eval(context, matching_wells) );

    // The well 'OPZ' matches the first condition and the well 'OPY' matches the
    // second condition, since the two conditions are combined with || the
    // resulting mathcing_wells variable should contain both these wells.
    BOOST_CHECK_EQUAL( matching_wells.size(), 2);
    BOOST_CHECK( std::find(matching_wells.begin(), matching_wells.end(), "OPZ") != matching_wells.end());
    BOOST_CHECK( std::find(matching_wells.begin(), matching_wells.end(), "OPY") != matching_wells.end());
}
