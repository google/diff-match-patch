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

#ifdef USE_GTEST
    #include "gtest/gtest.h"
    #define assertEquals( msg, GOLDEN, COMPUTED ) EXPECT_EQ( GOLDEN, COMPUTED ) << msg
    #define assertEmpty( msg, COMPUTED )          EXPECT_TRUE( COMPUTED.empty() ) << msg
    #define assertTrue( msg, COMPUTED )           EXPECT_TRUE( COMPUTED ) << msg
    #define assertFalse( msg, COMPUTED )          EXPECT_FALSE( COMPUTED ) << msg
    #define PUBLIC_TESTING : public testing::Test
    #define assertThrow( msg, STATEMENT, EXCEPTION_TYPE ) EXPECT_THROW( STATEMENT, EXCEPTION_TYPE ) << msg
#else
    #include <functional>
    #define PUBLIC_TESTING
    #define TEST_F( className, funcName ) void diff_match_patch_test::funcName()
#endif

#include "diff_match_patch_utils.h"

namespace NDiffMatchPatch
{
    class diff_match_patch_test PUBLIC_TESTING
    {
    public:
        using TStringVector = NDiffMatchPatch::diff_match_patch::TStringVector;
        using TCharPosMap = NDiffMatchPatch::diff_match_patch::TCharPosMap;
        using TVariant = NDiffMatchPatch::diff_match_patch::TVariant;
        using TVariantVector = NDiffMatchPatch::diff_match_patch::TVariantVector;

        diff_match_patch_test();

#ifndef USE_GTEST
    public:
        int run_all_tests();

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
        void testDiffPrettyConsole();
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

        void fromGitHubExamples();

    private:
        bool runTest( std::function< void() > test );
        std::size_t numPassedTests{ 0 };
        std::size_t numFailedTests{ 0 };

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

            if ( failed )
            {
                // Build human readable description of both lists.
                auto lhsString = NUtils::to_wstring( lhs, true );
                auto rhsString = NUtils::to_wstring( rhs, true );
                reportFailure( strCase, lhsString, rhsString );
                return;
            }
            reportPassed( strCase );
        }

        void assertEquals( const std::string &strCase, bool lhs, bool rhs );
        void assertEquals( const std::string &strCase, std::size_t n1, std::size_t n2 );
        void assertEquals( const std::string &strCase, const std::wstring &s1, const std::wstring &s2 );
        void assertEquals( const std::string &strCase, const std::string &s1, const std::string &s2 );
        void assertEquals( const std::string &strCase, const Diff &d1, const Diff &d2 );
        void assertEquals( const std::string &strCase, const TVariant &var1, const TVariant &var2 );
        void assertEquals( const std::string &strCase, const TCharPosMap &m1, const TCharPosMap &m2 );

        void assertTrue( const std::string &strCase, bool value );
        void assertFalse( const std::string &strCase, bool value );
        void assertEmpty( const std::string &strCase, const TStringVector &list );

        void reportFailure( const std::string &strCase, const std::wstring &expected, const std::wstring &actual );
        void reportPassed( const std::string &strCase );

    #define assertThrow( msg, COMMAND, EXCEPTION_TYPE ) \
        { \
            bool exceptionTriggered = false; \
            try \
            { \
                COMMAND; \
                assertFalse( msg, true ); \
            } \
            catch ( const EXCEPTION_TYPE &ex ) \
            { \
                exceptionTriggered = true; \
            } \
            assertTrue( std::string( msg ) + std::string( " - Exception triggered" ), exceptionTriggered ); \
        }

#endif
    public:
        bool equals( const TVariant &var1, const TVariant &var2 );

        template< typename T >
        bool equals( const T &lhs, const T &rhs )
        {
            bool equal = ( lhs.size() == rhs.size() );
            for ( auto ii = 0ULL; equal && ( ii < lhs.size() ); ++ii )
            {
                auto &&t1 = lhs[ ii ];
                auto &&t2 = rhs[ ii ];
                equal = t1 == t2;
            }
            return equal;
        }
        NDiffMatchPatch::diff_match_patch dmp;

        // Construct the two texts which made up the diff originally.
        TStringVector diff_rebuildtexts( const NDiffMatchPatch::TDiffVector &diffs );
    };
}

#endif   // DIFF_MATCH_PATCH_TEST_H
