/*
 * Diff Match and Patch -- Test Harness
 * Copyright 2018 The diff-match-patch Authors.
 * https://github.com/google/diff-match-patch
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <iostream>

#include "diff_match_patch.h"
#include "diff_match_patch_test.h"
#include "diff_match_patch_utils.h"
namespace NDiffMatchPatch
{
#ifndef USE_GTEST
    void diff_match_patch_test::reportFailure( const std::string &strCase, const std::wstring &expected, const std::wstring &actual )
    {
        std::cout << "FAILED : " + strCase + "\n";
        std::wcerr << "    Expected: " << expected << "\n      Actual: " << actual << "\n";
        numFailedTests++;
        // throw strCase;
    }

    void diff_match_patch_test::reportPassed( const std::string &strCase )
    {
        std::cout << "PASSED: " + strCase + "\n";
    }

    void diff_match_patch_test::assertEquals( const std::string &strCase, std::size_t n1, std::size_t n2 )
    {
        if ( n1 != n2 )
        {
            reportFailure( strCase, std::to_wstring( n1 ), std::to_wstring( n2 ) );
        }
        reportPassed( strCase );
    }

    void diff_match_patch_test::assertEquals( const std::string &strCase, const std::wstring &s1, const std::wstring &s2 )
    {
        if ( s1 != s2 )
        {
            reportFailure( strCase, s1, s2 );
        }
        reportPassed( strCase );
    }

    void diff_match_patch_test::assertEquals( const std::string &strCase, const std::string &s1, const std::string &s2 )
    {
        return assertEquals( strCase, NUtils::to_wstring( s1 ), NUtils::to_wstring( s2 ) );
    }

    void diff_match_patch_test::assertEquals( const std::string &strCase, const Diff &d1, const Diff &d2 )
    {
        if ( d1 != d2 )
        {
            reportFailure( strCase, d1.toString(), d2.toString() );
        }
        reportPassed( strCase );
    }

    void diff_match_patch_test::assertEquals( const std::string &strCase, const TVariant &var1, const TVariant &var2 )
    {
        if ( var1 != var2 )
        {
            reportFailure( strCase, NUtils::to_wstring( var1 ), NUtils::to_wstring( var2 ) );
        }
        reportPassed( strCase );
    }

    void diff_match_patch_test::assertEquals( const std::string &strCase, const TCharPosMap &m1, const TCharPosMap &m2 )
    {
        for ( auto &&ii : m1 )
        {
            auto rhs = m2.find( ii.first );
            if ( rhs == m2.end() )
            {
                reportFailure( strCase, L"(" + NUtils::to_wstring( ii.first ) + L"," + std::to_wstring( ii.second ) + L")", L"<NOT FOUND>" );
            }
        }

        for ( auto &&ii : m2 )
        {
            auto rhs = m1.find( ii.first );
            if ( rhs == m1.end() )
            {
                reportFailure( strCase, L"(" + NUtils::to_wstring( ii.first ) + L"," + std::to_wstring( ii.second ) + L")", L"<NOT FOUND>" );
            }
        }

        reportPassed( strCase );
    }

    void diff_match_patch_test::assertEquals( const std::string &strCase, bool lhs, bool rhs )
    {
        if ( lhs != rhs )
        {
            reportFailure( strCase, NUtils::to_wstring( lhs, false ), NUtils::to_wstring( rhs, false ) );
        }
        reportPassed( strCase );
    }

    void diff_match_patch_test::assertTrue( const std::string &strCase, bool value )
    {
        if ( !value )
        {
            reportFailure( strCase, NUtils::to_wstring( true, false ), NUtils::to_wstring( false, false ) );
        }
        reportPassed( strCase );
    }

    void diff_match_patch_test::assertFalse( const std::string &strCase, bool value )
    {
        if ( value )
        {
            reportFailure( strCase, NUtils::to_wstring( false, false ), NUtils::to_wstring( true, false ) );
        }
        reportPassed( strCase );
    }

    void diff_match_patch_test::assertEmpty( const std::string &strCase, const TStringVector &list )
    {
        if ( !list.empty() )
        {
            throw strCase;
        }
    }
#endif

    // Construct the two texts which made up the diff originally.
    diff_match_patch_test::TStringVector diff_match_patch_test::diff_rebuildtexts( const TDiffVector &diffs )
    {
        TStringVector text( 2, std::wstring() );
        for ( auto &&myDiff : diffs )
        {
            if ( !myDiff.isInsert() )
            {
                text[ 0 ] += myDiff.text();
            }

            if ( !myDiff.isDelete() )
            {
                text[ 1 ] += myDiff.text();
            }
        }
        return text;
    }
}
