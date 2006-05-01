//  (C) Copyright Gennadiy Rozental 2005.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at 
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
//  File        : $RCSfile$
//
//  Version     : $Revision$
//
//  Description : contains forward eclarations for Boost.Test data types
// ***************************************************************************

#ifndef BOOST_TEST_FWD_DECL_HPP_011605GER
#define BOOST_TEST_FWD_DECL_HPP_011605GER

namespace boost {

class  execution_monitor;
class  execution_exception;

namespace unit_test {

class  test_unit;
class  test_case;
class  test_suite;

class  test_tree_visitor;
class  test_observer;

// singletons
class  unit_test_monitor_t;
class  unit_test_log_t;

class  unit_test_log_formatter;
struct log_entry_data;
struct log_checkpoint_data;


} // namespace unit_test

} // namespace boost

// ***************************************************************************
//  Revision History :
//  
//  $Log$
//  Revision 1.1.1.1  2006/05/01 14:12:16  tjensen
//  - import of Boost-1.33.1 release
//
//  Revision 1.1  2005/02/20 08:27:06  rogeeff
//  This a major update for Boost.Test framework. See release docs for complete list of fixes/updates
//
// ***************************************************************************

#endif // BOOST_TEST_FWD_DECL_HPP_011605GER

