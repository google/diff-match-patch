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

#ifndef DIFF_MATCH_PATCH_TEST_H
#define DIFF_MATCH_PATCH_TEST_H

#include <functional>

class diff_match_patch_test
{
public:
    using TStringVector = diff_match_patch::TStringVector;
    using TCharPosMap = diff_match_patch::TCharPosMap;
    using TVariant = diff_match_patch::TVariant;
    using TVariantVector = diff_match_patch::TVariantVector;

    diff_match_patch_test();
    void run_all_tests();
    void runTest( std::function< void() > test );

    //  DIFF TEST FUNCTIONS
    void testDiffCommonPrefix();
    void testDiffCommonSuffix();
    void testDiffCommonOverlap();
    void testDiffHalfmatch();
    void testDiffLinesToChars();
    void testDiffCharsToLines();
    void testDiffCleanupMerge();
    void testDiffCleanupSemanticLossless();
    void testDiffCleanupSemantic();
    void testDiffCleanupEfficiency();
    void testDiffPrettyHtml();
    void testDiffText();
    void testDiffDelta();
    void testDiffXIndex();
    void testDiffLevenshtein();
    void testDiffBisect();
    void testDiffMain();

    //  MATCH TEST FUNCTIONS
    void testMatchAlphabet();
    void testMatchBitap();
    void testMatchMain();

    //  PATCH TEST FUNCTIONS
    void testPatchObj();
    void testPatchFromText();
    void testPatchToText();
    void testPatchAddContext();
    void testPatchMake();
    void testPatchSplitMax();
    void testPatchAddPadding();
    void testPatchApply();

private:
    std::size_t numPassedTests{ 0 };
    std::size_t numFailedTests{ 0 };
    diff_match_patch dmp;

    template< typename T >
    std::wstring to_wstring( const T & /*value*/, bool /*doubleQuoteEmpty*/ = false )
    {
        assert( false );
        return {};
    }

    template<>
    std::wstring to_wstring( const bool &value, bool /*doubleQuoteOnEmpty*/ )
    {
        std::wstring retVal = std::wstring( value ? L"true" : L"false" );
        return retVal;
    }

    template<>
    std::wstring to_wstring( const std::vector< bool >::reference &value, bool /*doubleQuoteOnEmpty*/ )
    {
        std::wstring retVal = std::wstring( value ? L"true" : L"false" );
        return retVal;
    }

    template<>
    std::wstring to_wstring( const std::string &string, bool doubleQuoteEmpty )
    {
        if ( doubleQuoteEmpty && string.empty() )
            return LR"("")";

        std::wstring wstring( string.size(), L' ' );   // Overestimate number of code points.
        wstring.resize( std::mbstowcs( &wstring[ 0 ], string.c_str(), string.size() ) );   // Shrink to fit.
        return wstring;
    }

    template<>
    std::wstring to_wstring( const TVariant &variant, bool doubleQuoteEmpty )
    {
        std::wstring retVal;
        if ( std::holds_alternative< std::wstring >( variant ) )
            retVal = std::get< std::wstring >( variant );

        if ( doubleQuoteEmpty && retVal.empty() )
            return LR"("")";

        return retVal;
    }

    template<>
    std::wstring to_wstring( const Diff &diff, bool doubleQuoteEmpty )
    {
        auto retVal = diff.toString();
        if ( doubleQuoteEmpty && retVal.empty() )
            return LR"("")";
        return retVal;
    }

    template<>
    std::wstring to_wstring( const Patch &patch, bool doubleQuoteEmpty )
    {
        auto retVal = patch.toString();
        if ( doubleQuoteEmpty && retVal.empty() )
            return LR"("")";
        return retVal;
    }

    template<>
    std::wstring to_wstring( const wchar_t &value, bool doubleQuoteEmpty )
    {
        if ( doubleQuoteEmpty && ( value == 0 ) )
            return LR"("")";

        return std::wstring( 1, value );
    }

    template<>
    std::wstring to_wstring( const int &value, bool doubleQuoteEmpty )
    {
        return to_wstring( static_cast< wchar_t >( value ), doubleQuoteEmpty );
    }

    template<>
    std::wstring to_wstring( const std::wstring &value, bool doubleQuoteEmpty )
    {
        if ( doubleQuoteEmpty && value.empty() )
            return LR"("")";

        return value;
    }

    template< typename T >
    std::wstring to_wstring( const std::vector< T > &values, bool doubleQuoteEmpty = false )
    {
        std::wstring retVal = L"(";
        bool first = true;
        for ( auto &&curr : values )
        {
            if ( !first )
            {
                retVal += L", ";
            }
            retVal += to_wstring( curr, doubleQuoteEmpty );
            first = false;
        }
        retVal += L")";
        return retVal;
    }

    template<>
    std::wstring to_wstring( const std::vector< bool > &boolArray, bool doubleQuoteOnEmpty )
    {
        std::wstring retVal;
        for ( auto &&curr : boolArray )
        {
            retVal += L"\t" + to_wstring( curr, doubleQuoteOnEmpty );
        }
        return retVal;
    }


    template< typename T >
    typename std::enable_if_t< std::is_integral_v< T >, std::wstring > to_wstring( const std::initializer_list< T > &values, bool doubleQuoteEmpty = false )
    {
        if ( doubleQuoteEmpty && ( values.size() == 0 ) )
            return LR"(\"\")";

        std::wstring retVal;
        for ( auto &&curr : values )
        {
            retVal += to_wstring( curr, false );
        }
        return retVal;
    }

    template< typename T >
    typename std::enable_if_t< !std::is_integral_v< T >, std::wstring > to_wstring( const std::initializer_list< T > &values, bool doubleQuoteEmpty = false )
    {
        std::wstring retVal = L"(";
        bool first = true;
        for ( auto &&curr : values )
        {
            if ( !first )
            {
                retVal += L", ";
            }
            retVal += to_wstring( curr, doubleQuoteEmpty );
            first = false;
        }
        retVal += L")";
        return retVal;
    }

    // Define equality.
    template< typename T >
    void assertEquals( const std::string &strCase, const T &lhs, const T &rhs )
    {
        bool failed = ( lhs.size() != rhs.size() );
        if ( !failed )
        {
            for ( auto ii = 0ULL; !failed && ( ii < lhs.size() ); ++ii )
            {
                auto &&t1 = lhs[ ii ];
                auto &&t2 = rhs[ ii ];
                failed = t1 != t2;
            }
        }
        else
        {
            // Build human readable description of both lists.
            auto lhsString = to_wstring( lhs, true );
            auto rhsString = to_wstring( rhs, true );
            reportFailure( strCase, lhsString, rhsString );
            return;
        }
        reportPassed( strCase );
    }
    void assertEquals( const std::string &strCase, bool lhs, bool rhs );
    void assertEquals( const std::string &strCase, std::size_t n1, std::size_t n2 );
    void assertEquals( const std::string &strCase, const std::wstring &s1, const std::wstring &s2 );
    void assertEquals( const std::string &strCase, const std::string &s1, const std::string &s2 ) { return assertEquals( strCase, ::to_wstring( s1 ), ::to_wstring( s2 ) ); }
    void assertEquals( const std::string &strCase, const std::wstring &s1, const std::string &s2 ) { return assertEquals( strCase,  s1, ::to_wstring( s2 ) ); }
    void assertEquals( const std::string &strCase, const std::string &s1, const std::wstring &s2 ) { return assertEquals( strCase, ::to_wstring( s1 ), s2 ); }
    void assertEquals( const std::string &strCase, const Diff &d1, const Diff &d2 );
    void assertEquals( const std::string &strCase, const TVariant &var1, const TVariant &var2 );
    void assertEquals( const std::string &strCase, const TCharPosMap &m1, const TCharPosMap &m2 );

    void assertTrue( const std::string &strCase, bool value );
    void assertFalse( const std::string &strCase, bool value );
    void assertEmpty( const std::string &strCase, const TStringVector &list );

    void reportFailure( const std::string &strCase, const std::wstring &expected, const std::wstring &actual );
    void reportPassed( const std::string &strCase );

    // Construct the two texts which made up the diff originally.
    TStringVector diff_rebuildtexts( const TDiffVector &diffs );
};

#endif   // DIFF_MATCH_PATCH_TEST_H
