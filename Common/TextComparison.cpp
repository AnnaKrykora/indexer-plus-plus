// This file is the part of the Indexer++ project.
// Copyright (C) 2016 Anna Krykora <krykoraanna@gmail.com>. All rights reserved.
// Use of this source code is governed by a MIT-style license that can be found in the LICENSE file.

#include "TextComparison.h"

#include "AsyncLog.h"
#include "Log.h"
#include "OneThreadLog.h"

#include "Helpers.h"
#include "LetterCaseMatching.h"

namespace indexer_common {

	// TODO: I believe it's possible to speed up this function.
    char16_t* search(const char16_t* text, const char16_t* pattern, const ushort* case_match_table) {
        // Log* logger_;
        // GET_LOGGER

		int m = helpers::Str16Len(pattern), n = helpers::Str16Len(text), diff = n - m;

        for (int i = 0, j; i <= diff; ++i) {

            for (j = 0; j < m; ++j) {
                if (case_match_table[pattern[j]] != case_match_table[text[i + j]]) break;
            }

            if (j == m) {
                auto res = text + i;
                return const_cast<char16_t*>(res);
            }
        }

        return nullptr;
    }

    int compare(const char16_t* lhs, const char16_t* rhs, ushort* case_match_table) {
        while (*lhs != 0 && case_match_table[*lhs] == case_match_table[*rhs]) {
            ++lhs;
            ++rhs;
        }

        if (case_match_table[*lhs] > case_match_table[*rhs]) return 1;

        if (case_match_table[*lhs] < case_match_table[*rhs]) return -1;

        return 0;
    }

    bool WcharLessComparator::operator()(const char16_t* lhs, const char16_t* rhs) const {
        return compare(lhs, rhs, GetLowerMatchTable()) < 0;
    }

} // namespace indexer_common