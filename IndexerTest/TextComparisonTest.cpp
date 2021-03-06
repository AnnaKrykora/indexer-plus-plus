// This file is the part of the Indexer++ project.
// Copyright (C) 2016 Anna Krykora <krykoraanna@gmail.com>. All rights reserved.
// Use of this source code is governed by a MIT-style license that can be found in the LICENSE file.

#include "gtest/gtest.h"

#include <map>

#include "TextComparison.h"

#include "Helpers.h"

namespace indexer {

    TEST(TextComparison, WcharLessComparatorWorks) {

        indexer_common::WcharLessComparator cmp;

        auto* s1 = __L__(L".ex");

        EXPECT_FALSE(cmp(s1, s1));

        auto* s2 = __L__(L".ex_");

        EXPECT_TRUE(cmp(s1, s2));
        EXPECT_FALSE(cmp(s2, s1));
    }

    TEST(TextComparison, MapUsesWcharLessComparator) {

		std::map<const char16_t*, int, indexer_common::WcharLessComparator> wchar_to_int_map;

        auto* s1 = __L__(L".ex");
        auto* s2 = __L__(L".ex_");

        wchar_to_int_map.emplace(s1, 0);
        EXPECT_FALSE(wchar_to_int_map.find(s1) == wchar_to_int_map.end());

        wchar_to_int_map.emplace(s1, 0);
        EXPECT_TRUE(wchar_to_int_map.size() == 1);

        wchar_to_int_map.emplace(s2, 0);
        EXPECT_FALSE(wchar_to_int_map.find(s2) == wchar_to_int_map.end());
        EXPECT_TRUE(wchar_to_int_map.size() == 2);

        wchar_to_int_map.emplace(s2, 0);
        EXPECT_TRUE(wchar_to_int_map.size() == 2);
    }

} // namespace indexer