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
//  Description : optional internal tracing
// ***************************************************************************

#ifndef BOOST_RT_TRACE_HPP_062604GER
#define BOOST_RT_TRACE_HPP_062604GER

// Boost.Runtime.Parameter
#include <boost/test/utils/runtime/config.hpp>

#ifdef BOOST_RT_PARAM_DEBUG

#include <iostream>

#  define BOOST_RT_PARAM_TRACE( str ) std::cerr << str << std::endl
#else
#  define BOOST_RT_PARAM_TRACE( str )
#endif

// ************************************************************************** //
//   Revision History:
//
//   $Log$
//   Revision 1.1.1.1  2006/05/01 14:12:16  tjensen
//   - import of Boost-1.33.1 release
//
//   Revision 1.1  2005/04/12 06:42:42  rogeeff
//   Runtime.Param library initial commit
//
// ************************************************************************** //

#endif // BOOST_RT_TRACE_HPP_062604GER
