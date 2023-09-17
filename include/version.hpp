//
// Copyright (c) 2023-present DeepGrace (complex dot invoke at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/deepgrace/unp
//

#ifndef UNP_VERSION_HPP
#define UNP_VERSION_HPP

#define UNP_STRINGIZE(T) #T

/*
 *   UNP_VERSION_NUMBER
 *
 *   Identifies the API version of unp.
 *   This is a simple integer that is incremented by one every
 *   time a set of code changes is merged to the master branch.
 */

#define UNP_VERSION_NUMBER 2
#define UNP_VERSION_STRING "unp/" UNP_STRINGIZE(UNP_VERSION_NUMBER)

#endif
