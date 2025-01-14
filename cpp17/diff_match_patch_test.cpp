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

#include "diff_match_patch.h"

#include "diff_match_patch_test.h"
#include "diff_match_patch_utils.h"

#ifdef USE_GTEST
    #include "gtest/gtest.h"
#endif

#include <chrono>
#include <iostream>
#include <codecvt>
#include <locale>

int main( int argc, char **argv )
{
#ifdef USE_GTEST
    ::testing::InitGoogleTest( &argc, argv );
    int retVal = RUN_ALL_TESTS();
#else
    diff_match_patch_test dmp_test;
    std::cerr << "Starting diff_match_patch unit tests.\n";
    int retVal = dmp_test.run_all_tests();
    std::cerr << "Done.\n";
#endif
    return retVal;
}

static wchar_t kZero{ 0 };
static wchar_t kOne{ 1 };
static wchar_t kTwo{ 2 };
namespace NDiffMatchPatch
{
    void PrintTo( const Diff &value, ::std::ostream *os )
    {
        std::wostringstream oss;
        oss << value.toString( EStringType::eUnitTest );
        std::wstring_convert< std::codecvt_utf8< wchar_t > > utf8_conv;
        *os << utf8_conv.to_bytes( oss.str() );
    }

    diff_match_patch_test::diff_match_patch_test()
    {
    }

#ifndef USE_GTEST
    bool diff_match_patch_test::runTest( std::function< void() > test )
    {
        bool retVal = false;
        try
        {
            test();
            numPassedTests++;
            retVal = true;
        }
        catch ( std::string msg )
        {
            std::cerr << "Test failed: " << msg << "\n";
            numFailedTests++;
            retVal = false;
        }
        return retVal;
    }

    int diff_match_patch_test::run_all_tests()
    {
        auto startTime = std::chrono::high_resolution_clock::now();

        runTest( std::bind( &diff_match_patch_test::testDiffCommonPrefix, this ) );
        runTest( std::bind( &diff_match_patch_test::testDiffCommonSuffix, this ) );
        runTest( std::bind( &diff_match_patch_test::testDiffCommonOverlap, this ) );
        runTest( std::bind( &diff_match_patch_test::testDiffHalfmatch, this ) );
        runTest( std::bind( &diff_match_patch_test::testDiffLinesToChars, this ) );
        runTest( std::bind( &diff_match_patch_test::testDiffCharsToLines, this ) );
        runTest( std::bind( &diff_match_patch_test::testDiffCleanupMerge, this ) );
        runTest( std::bind( &diff_match_patch_test::testDiffCleanupSemanticLossless, this ) );
        runTest( std::bind( &diff_match_patch_test::testDiffCleanupSemantic, this ) );
        runTest( std::bind( &diff_match_patch_test::testDiffCleanupEfficiency, this ) );
        runTest( std::bind( &diff_match_patch_test::testDiffPrettyHtml, this ) );
        runTest( std::bind( &diff_match_patch_test::testDiffPrettyConsole, this ) );
        runTest( std::bind( &diff_match_patch_test::testDiffText, this ) );
        runTest( std::bind( &diff_match_patch_test::testDiffDelta, this ) );
        runTest( std::bind( &diff_match_patch_test::testDiffXIndex, this ) );
        runTest( std::bind( &diff_match_patch_test::testDiffLevenshtein, this ) );
        runTest( std::bind( &diff_match_patch_test::testDiffBisect, this ) );
        runTest( std::bind( &diff_match_patch_test::testDiffMain, this ) );

        runTest( std::bind( &diff_match_patch_test::testMatchAlphabet, this ) );
        runTest( std::bind( &diff_match_patch_test::testMatchBitap, this ) );
        runTest( std::bind( &diff_match_patch_test::testMatchMain, this ) );

        runTest( std::bind( &diff_match_patch_test::testPatchObj, this ) );
        runTest( std::bind( &diff_match_patch_test::testPatchFromText, this ) );
        runTest( std::bind( &diff_match_patch_test::testPatchToText, this ) );
        runTest( std::bind( &diff_match_patch_test::testPatchAddContext, this ) );
        runTest( std::bind( &diff_match_patch_test::testPatchMake, this ) );
        runTest( std::bind( &diff_match_patch_test::testPatchSplitMax, this ) );
        runTest( std::bind( &diff_match_patch_test::testPatchAddPadding, this ) );
        runTest( std::bind( &diff_match_patch_test::testPatchApply, this ) );
        if ( numFailedTests == 0 )
            std::cout << numPassedTests << " Tests Passed\n" << numFailedTests << " Tests Failed\n";
        else
            std::cerr << numPassedTests << " Tests Passed\n" << numFailedTests << " Tests Failed\n";
        auto endTime = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast< std::chrono::milliseconds >( endTime - startTime ).count();
        std::wcout << "Total time: " << elapsed << " ms\n";
        return ( numFailedTests == 0 ) ? 0 : 1;
    }
#endif

    //  DIFF TEST FUNCTIONS

    TEST_F( diff_match_patch_test, testDiffCommonPrefix )
    {
        // Detect any common prefix.
        assertEquals( "diff_commonPrefix: nullptr case.", 0, dmp.diff_commonPrefix( "abc", "xyz" ) );

        assertEquals( "diff_commonPrefix: Non-nullptr case.", 4, dmp.diff_commonPrefix( "1234abcdef", "1234xyz" ) );

        assertEquals( "diff_commonPrefix: Whole case.", 4, dmp.diff_commonPrefix( "1234", "1234xyz" ) );
    }

    TEST_F( diff_match_patch_test, testDiffCommonSuffix )
    {
        // Detect any common suffix.
        assertEquals( "diff_commonSuffix: nullptr case.", 0, dmp.diff_commonSuffix( "abc", "xyz" ) );

        assertEquals( "diff_commonSuffix: Non-nullptr case.", 4, dmp.diff_commonSuffix( "abcdef1234", "xyz1234" ) );

        assertEquals( "diff_commonSuffix: Whole case.", 4, dmp.diff_commonSuffix( "1234", "xyz1234" ) );
    }

    TEST_F( diff_match_patch_test, testDiffCommonOverlap )
    {
        // Detect any suffix/prefix overlap.
        assertEquals( "diff_commonOverlap: nullptr case.", 0, dmp.diff_commonOverlap( "", "abcd" ) );

        assertEquals( "diff_commonOverlap: Whole case.", 3, dmp.diff_commonOverlap( "abc", "abcd" ) );

        assertEquals( "diff_commonOverlap: No overlap.", 0, dmp.diff_commonOverlap( "123456", "abcd" ) );

        assertEquals( "diff_commonOverlap: Overlap.", 3, dmp.diff_commonOverlap( "123456xxx", "xxxabcd" ) );

        // Some overly clever languages (C#) may treat ligatures as equal to their
        // component letters.  E.g. U+FB01 == 'fi'
        assertEquals( "diff_commonOverlap: Unicode.", 0, dmp.diff_commonOverlap( L"fi", std::wstring( L"\ufb01i" ) ) );
    }

    TEST_F( diff_match_patch_test, testDiffHalfmatch )
    {
        // Detect a halfmatch.
        dmp.Diff_Timeout = 1;
        assertEmpty( "diff_halfMatch: No match #1.", dmp.diff_halfMatch( "1234567890", "abcdef" ) );

        assertEmpty( "diff_halfMatch: No match #2.", dmp.diff_halfMatch( "12345", "23" ) );

        assertEquals( "diff_halfMatch: Single Match #1.", TStringVector( { L"12", L"90", L"a", L"z", L"345678" } ), dmp.diff_halfMatch( "1234567890", "a345678z" ) );

        assertEquals( "diff_halfMatch: Single Match #2.", TStringVector( { L"a", L"z", L"12", L"90", L"345678" } ), dmp.diff_halfMatch( "a345678z", "1234567890" ) );

        assertEquals( "diff_halfMatch: Single Match #3.", TStringVector( { L"abc", L"z", L"1234", L"0", L"56789" } ), dmp.diff_halfMatch( "abc56789z", "1234567890" ) );

        assertEquals( "diff_halfMatch: Single Match #4.", TStringVector( { L"a", L"xyz", L"1", L"7890", L"23456" } ), dmp.diff_halfMatch( "a23456xyz", "1234567890" ) );

        assertEquals( "diff_halfMatch: Multiple Matches #1.", TStringVector( { L"12123", L"123121", L"a", L"z", L"1234123451234" } ), dmp.diff_halfMatch( "121231234123451234123121", "a1234123451234z" ) );

        assertEquals( "diff_halfMatch: Multiple Matches #2.", TStringVector( { L"", L"-=-=-=-=-=", L"x", L"", L"x-=-=-=-=-=-=-=" } ), dmp.diff_halfMatch( "x-=-=-=-=-=-=-=-=-=-=-=-=", "xx-=-=-=-=-=-=-=" ) );

        assertEquals( "diff_halfMatch: Multiple Matches #3.", TStringVector( { L"-=-=-=-=-=", L"", L"", L"y", L"-=-=-=-=-=-=-=y" } ), dmp.diff_halfMatch( "-=-=-=-=-=-=-=-=-=-=-=-=y", "-=-=-=-=-=-=-=yy" ) );

        // Optimal diff would be -q+x=H-i+e=lloHe+Hu=llo-Hew+y not
        // -qHillo+x=HelloHe-w+Hulloy
        assertEquals( "diff_halfMatch: Non-optimal halfmatch.", TStringVector( { L"qHillo", L"w", L"x", L"Hulloy", L"HelloHe" } ), dmp.diff_halfMatch( "qHilloHelloHew", "xHelloHeHulloy" ) );

        dmp.Diff_Timeout = 0;
        assertEmpty( "diff_halfMatch: Optimal no halfmatch.", dmp.diff_halfMatch( L"qHilloHelloHew", L"xHelloHeHulloy" ) );
    }

    TEST_F( diff_match_patch_test, testDiffLinesToChars )
    {
        // Convert lines down to characters.
        TStringVector tmpVector = TStringVector( { L"", L"alpha\n", L"beta\n" } );
        TVariantVector tmpVarList;
        tmpVarList.emplace_back( NUtils::to_wstring( { 1, 2, 1 }, false ) );   //(("\u0001\u0002\u0001"));
        tmpVarList.emplace_back( NUtils::to_wstring( { 2, 1, 2 }, false ) );   // (("\u0002\u0001\u0002"));
        tmpVarList.emplace_back( tmpVector );
        assertEquals( "diff_linesToChars:", tmpVarList, dmp.diff_linesToChars( "alpha\nbeta\nalpha\n", "beta\nalpha\nbeta\n" ) );

        tmpVector.clear();
        tmpVarList.clear();
        tmpVector.emplace_back( L"" );
        tmpVector.emplace_back( L"alpha\r\n" );
        tmpVector.emplace_back( L"beta\r\n" );
        tmpVector.emplace_back( L"\r\n" );
        tmpVarList.emplace_back( std::wstring() );
        tmpVarList.emplace_back( NUtils::to_wstring( { 1, 2, 3, 3 }, false ) );   // (("\u0001\u0002\u0003\u0003"));
        tmpVarList.emplace_back( tmpVector );
        assertEquals( "diff_linesToChars:", tmpVarList, dmp.diff_linesToChars( "", "alpha\r\nbeta\r\n\r\n\r\n" ) );

        tmpVector.clear();
        tmpVarList.clear();
        tmpVector.emplace_back( L"" );
        tmpVector.emplace_back( L"a" );
        tmpVector.emplace_back( L"b" );
        tmpVarList.emplace_back( NUtils::to_wstring( kOne ) );   // (("\u0001"));
        tmpVarList.emplace_back( NUtils::to_wstring( kTwo ) );   // (("\u0002"));
        tmpVarList.emplace_back( tmpVector );
        assertEquals( "diff_linesToChars:", tmpVarList, dmp.diff_linesToChars( "a", "b" ) );

        // More than 256 to reveal any 8-bit limitations.
        int n = 300;
        tmpVector.clear();
        tmpVarList.clear();
        std::wstring lines;
        std::wstring chars;
        for ( int x = 1; x < n + 1; x++ )
        {
            tmpVector.emplace_back( std::to_wstring( x ) + L"\n" );
            lines += std::to_wstring( x ) + L"\n";
            chars += NUtils::to_wstring( (wchar_t)x );
        }
        assertEquals( "diff_linesToChars: More than 256 (setup).", n, tmpVector.size() );
        assertEquals( "diff_linesToChars: More than 256 (setup).", n, chars.length() );
        tmpVector.emplace( tmpVector.begin(), L"" );
        tmpVarList.emplace_back( chars );
        tmpVarList.emplace_back( std::wstring() );
        tmpVarList.emplace_back( tmpVector );
        assertEquals( "diff_linesToChars: More than 256.", tmpVarList, dmp.diff_linesToChars( lines, {} ) );
    }

    TEST_F( diff_match_patch_test, testDiffCharsToLines )
    {
        // First check that Diff equality works.
        assertTrue( "diff_charsToLines:", Diff( EOperation::eEQUAL, "a" ) == Diff( EOperation::eEQUAL, "a" ) );

        assertEquals( "diff_charsToLines:", Diff( EOperation::eEQUAL, "a" ), Diff( EOperation::eEQUAL, "a" ) );

        // Convert chars up to lines.
        TDiffVector diffs;
        diffs.emplace_back( EOperation::eEQUAL, NUtils::to_wstring( { 1, 2, 1 }, false ) );   // ("\u0001\u0002\u0001");
        diffs.emplace_back( EOperation::eINSERT, NUtils::to_wstring( { 2, 1, 2 }, false ) );   // ("\u0002\u0001\u0002");
        TStringVector tmpVector;
        tmpVector.emplace_back( L"" );
        tmpVector.emplace_back( L"alpha\n" );
        tmpVector.emplace_back( L"beta\n" );
        dmp.diff_charsToLines( diffs, tmpVector );
        assertEquals( "diff_charsToLines:", TDiffVector( { Diff( EOperation::eEQUAL, "alpha\nbeta\nalpha\n" ), Diff( EOperation::eINSERT, "beta\nalpha\nbeta\n" ) } ), diffs );

        // More than 256 to reveal any 8-bit limitations.
        int n = 300;
        tmpVector.clear();
        std::vector< TVariant > tmpVarList;
        std::wstring lines;
        std::wstring chars;
        for ( int x = 1; x < n + 1; x++ )
        {
            tmpVector.emplace_back( std::to_wstring( x ) + L"\n" );
            lines += std::to_wstring( x ) + L"\n";
            chars += NUtils::to_wstring( (wchar_t)x );
        }
        assertEquals( "diff_linesToChars: More than 256 (setup).", n, tmpVector.size() );
        assertEquals( "diff_linesToChars: More than 256 (setup).", n, chars.length() );
        tmpVector.emplace( tmpVector.begin(), L"" );
        diffs = { Diff( EOperation::eDELETE, chars ) };
        dmp.diff_charsToLines( diffs, tmpVector );
        assertEquals( "diff_charsToLines: More than 256.", TDiffVector( { Diff( EOperation::eDELETE, lines ) } ), diffs );
    }

    TEST_F( diff_match_patch_test, testDiffCleanupMerge )
    {
        // Cleanup a messy diff.
        TDiffVector diffs;
        dmp.diff_cleanupMerge( diffs );
        assertEquals( "diff_cleanupMerge: nullptr case.", TDiffVector(), diffs );

        diffs = { Diff( EOperation::eEQUAL, "a" ), Diff( EOperation::eDELETE, "b" ), Diff( EOperation::eINSERT, "c" ) };
        dmp.diff_cleanupMerge( diffs );
        assertEquals( "diff_cleanupMerge: No change case.", TDiffVector( { Diff( EOperation::eEQUAL, "a" ), Diff( EOperation::eDELETE, "b" ), Diff( EOperation::eINSERT, "c" ) } ), diffs );

        diffs = { Diff( EOperation::eEQUAL, "a" ), Diff( EOperation::eEQUAL, "b" ), Diff( EOperation::eEQUAL, "c" ) };
        dmp.diff_cleanupMerge( diffs );
        assertEquals( "diff_cleanupMerge: Merge equalities.", TDiffVector( { Diff( EOperation::eEQUAL, "abc" ) } ), diffs );

        diffs = { Diff( EOperation::eDELETE, "a" ), Diff( EOperation::eDELETE, "b" ), Diff( EOperation::eDELETE, "c" ) };
        dmp.diff_cleanupMerge( diffs );
        assertEquals( "diff_cleanupMerge: Merge deletions.", TDiffVector( { Diff( EOperation::eDELETE, "abc" ) } ), diffs );

        diffs = { Diff( EOperation::eINSERT, "a" ), Diff( EOperation::eINSERT, "b" ), Diff( EOperation::eINSERT, "c" ) };
        dmp.diff_cleanupMerge( diffs );
        assertEquals( "diff_cleanupMerge: Merge insertions.", TDiffVector( { Diff( EOperation::eINSERT, "abc" ) } ), diffs );

        diffs = { Diff( EOperation::eDELETE, "a" ), Diff( EOperation::eINSERT, "b" ), Diff( EOperation::eDELETE, "c" ), Diff( EOperation::eINSERT, "d" ), Diff( EOperation::eEQUAL, "e" ), Diff( EOperation::eEQUAL, "f" ) };
        dmp.diff_cleanupMerge( diffs );
        assertEquals( "diff_cleanupMerge: Merge interweave.", TDiffVector( { Diff( EOperation::eDELETE, "ac" ), Diff( EOperation::eINSERT, "bd" ), Diff( EOperation::eEQUAL, "ef" ) } ), diffs );

        diffs = { Diff( EOperation::eDELETE, "a" ), Diff( EOperation::eINSERT, "abc" ), Diff( EOperation::eDELETE, "dc" ) };
        dmp.diff_cleanupMerge( diffs );
        assertEquals( "diff_cleanupMerge: Prefix and suffix detection.", TDiffVector( { Diff( EOperation::eEQUAL, "a" ), Diff( EOperation::eDELETE, "d" ), Diff( EOperation::eINSERT, "b" ), Diff( EOperation::eEQUAL, "c" ) } ), diffs );

        diffs = { Diff( EOperation::eEQUAL, "x" ), Diff( EOperation::eDELETE, "a" ), Diff( EOperation::eINSERT, "abc" ), Diff( EOperation::eDELETE, "dc" ), Diff( EOperation::eEQUAL, "y" ) };
        dmp.diff_cleanupMerge( diffs );
        assertEquals( "diff_cleanupMerge: Prefix and suffix detection with equalities.", TDiffVector( { Diff( EOperation::eEQUAL, "xa" ), Diff( EOperation::eDELETE, "d" ), Diff( EOperation::eINSERT, "b" ), Diff( EOperation::eEQUAL, "cy" ) } ), diffs );

        diffs = { Diff( EOperation::eEQUAL, "a" ), Diff( EOperation::eINSERT, "ba" ), Diff( EOperation::eEQUAL, "c" ) };
        dmp.diff_cleanupMerge( diffs );
        assertEquals( "diff_cleanupMerge: Slide edit left.", TDiffVector( { Diff( EOperation::eINSERT, "ab" ), Diff( EOperation::eEQUAL, "ac" ) } ), diffs );

        diffs = { Diff( EOperation::eEQUAL, "c" ), Diff( EOperation::eINSERT, "ab" ), Diff( EOperation::eEQUAL, "a" ) };
        dmp.diff_cleanupMerge( diffs );
        assertEquals( "diff_cleanupMerge: Slide edit right.", TDiffVector( { Diff( EOperation::eEQUAL, "ca" ), Diff( EOperation::eINSERT, "ba" ) } ), diffs );

        diffs = { Diff( EOperation::eEQUAL, "a" ), Diff( EOperation::eDELETE, "b" ), Diff( EOperation::eEQUAL, "c" ), Diff( EOperation::eDELETE, "ac" ), Diff( EOperation::eEQUAL, "x" ) };
        dmp.diff_cleanupMerge( diffs );
        assertEquals( "diff_cleanupMerge: Slide edit left recursive.", TDiffVector( { Diff( EOperation::eDELETE, "abc" ), Diff( EOperation::eEQUAL, "acx" ) } ), diffs );

        diffs = { Diff( EOperation::eEQUAL, "x" ), Diff( EOperation::eDELETE, "ca" ), Diff( EOperation::eEQUAL, "c" ), Diff( EOperation::eDELETE, "b" ), Diff( EOperation::eEQUAL, "a" ) };
        dmp.diff_cleanupMerge( diffs );
        assertEquals( "diff_cleanupMerge: Slide edit right recursive.", TDiffVector( { Diff( EOperation::eEQUAL, "xca" ), Diff( EOperation::eDELETE, "cba" ) } ), diffs );
    }

    TEST_F( diff_match_patch_test, testDiffCleanupSemanticLossless )
    {
        // Slide diffs to match logical boundaries.
        auto diffs = TDiffVector();
        dmp.diff_cleanupSemanticLossless( diffs );
        assertEquals( "diff_cleanupSemantic: nullptr case.", TDiffVector(), diffs );

        diffs = { Diff( EOperation::eEQUAL, "AAA\r\n\r\nBBB" ), Diff( EOperation::eINSERT, "\r\nDDD\r\n\r\nBBB" ), Diff( EOperation::eEQUAL, "\r\nEEE" ) };
        dmp.diff_cleanupSemanticLossless( diffs );
        assertEquals( "diff_cleanupSemanticLossless: Blank lines.", TDiffVector( { Diff( EOperation::eEQUAL, "AAA\r\n\r\n" ), Diff( EOperation::eINSERT, "BBB\r\nDDD\r\n\r\n" ), Diff( EOperation::eEQUAL, "BBB\r\nEEE" ) } ), diffs );

        diffs = { Diff( EOperation::eEQUAL, "AAA\r\nBBB" ), Diff( EOperation::eINSERT, " DDD\r\nBBB" ), Diff( EOperation::eEQUAL, " EEE" ) };
        dmp.diff_cleanupSemanticLossless( diffs );
        assertEquals( "diff_cleanupSemanticLossless: Line boundaries.", TDiffVector( { Diff( EOperation::eEQUAL, "AAA\r\n" ), Diff( EOperation::eINSERT, "BBB DDD\r\n" ), Diff( EOperation::eEQUAL, "BBB EEE" ) } ), diffs );

        diffs = { Diff( EOperation::eEQUAL, "The c" ), Diff( EOperation::eINSERT, "ow and the c" ), Diff( EOperation::eEQUAL, "at." ) };
        dmp.diff_cleanupSemanticLossless( diffs );
        assertEquals( "diff_cleanupSemantic: Word boundaries.", TDiffVector( { Diff( EOperation::eEQUAL, "The " ), Diff( EOperation::eINSERT, "cow and the " ), Diff( EOperation::eEQUAL, "cat." ) } ), diffs );

        diffs = { Diff( EOperation::eEQUAL, "The-c" ), Diff( EOperation::eINSERT, "ow-and-the-c" ), Diff( EOperation::eEQUAL, "at." ) };
        dmp.diff_cleanupSemanticLossless( diffs );
        assertEquals( "diff_cleanupSemantic: Alphanumeric boundaries.", TDiffVector( { Diff( EOperation::eEQUAL, "The-" ), Diff( EOperation::eINSERT, "cow-and-the-" ), Diff( EOperation::eEQUAL, "cat." ) } ), diffs );

        diffs = { Diff( EOperation::eEQUAL, "a" ), Diff( EOperation::eDELETE, "a" ), Diff( EOperation::eEQUAL, "ax" ) };
        dmp.diff_cleanupSemanticLossless( diffs );
        assertEquals( "diff_cleanupSemantic: Hitting the start.", TDiffVector( { Diff( EOperation::eDELETE, "a" ), Diff( EOperation::eEQUAL, "aax" ) } ), diffs );

        diffs = { Diff( EOperation::eEQUAL, "xa" ), Diff( EOperation::eDELETE, "a" ), Diff( EOperation::eEQUAL, "a" ) };
        dmp.diff_cleanupSemanticLossless( diffs );
        assertEquals( "diff_cleanupSemantic: Hitting the end.", TDiffVector( { Diff( EOperation::eEQUAL, "xaa" ), Diff( EOperation::eDELETE, "a" ) } ), diffs );

        diffs = { Diff( EOperation::eEQUAL, "The xxx. The " ), Diff( EOperation::eINSERT, "zzz. The " ), Diff( EOperation::eEQUAL, "yyy." ) };
        dmp.diff_cleanupSemanticLossless( diffs );
        assertEquals( "diff_cleanupSemantic: Sentence boundaries.", TDiffVector( { Diff( EOperation::eEQUAL, "The xxx." ), Diff( EOperation::eINSERT, " The zzz." ), Diff( EOperation::eEQUAL, " The yyy." ) } ), diffs );
    }

    TEST_F( diff_match_patch_test, testDiffCleanupSemantic )
    {
        // Cleanup semantically trivial equalities.
        auto diffs = TDiffVector();
        dmp.diff_cleanupSemantic( diffs );
        assertEquals( "diff_cleanupSemantic: nullptr case.", TDiffVector(), diffs );

        diffs = { Diff( EOperation::eDELETE, "ab" ), Diff( EOperation::eINSERT, "cd" ), Diff( EOperation::eEQUAL, "12" ), Diff( EOperation::eDELETE, "e" ) };
        dmp.diff_cleanupSemantic( diffs );
        assertEquals( "diff_cleanupSemantic: No elimination #1.", TDiffVector( { Diff( EOperation::eDELETE, "ab" ), Diff( EOperation::eINSERT, "cd" ), Diff( EOperation::eEQUAL, "12" ), Diff( EOperation::eDELETE, "e" ) } ), diffs );

        diffs = { Diff( EOperation::eDELETE, "abc" ), Diff( EOperation::eINSERT, "ABC" ), Diff( EOperation::eEQUAL, "1234" ), Diff( EOperation::eDELETE, "wxyz" ) };
        dmp.diff_cleanupSemantic( diffs );
        assertEquals( "diff_cleanupSemantic: No elimination #2.", TDiffVector( { Diff( EOperation::eDELETE, "abc" ), Diff( EOperation::eINSERT, "ABC" ), Diff( EOperation::eEQUAL, "1234" ), Diff( EOperation::eDELETE, "wxyz" ) } ), diffs );

        diffs = { Diff( EOperation::eDELETE, "a" ), Diff( EOperation::eEQUAL, "b" ), Diff( EOperation::eDELETE, "c" ) };
        dmp.diff_cleanupSemantic( diffs );
        assertEquals( "diff_cleanupSemantic: Simple elimination.", TDiffVector( { Diff( EOperation::eDELETE, "abc" ), Diff( EOperation::eINSERT, "b" ) } ), diffs );

        diffs = { Diff( EOperation::eDELETE, "ab" ), Diff( EOperation::eEQUAL, "cd" ), Diff( EOperation::eDELETE, "e" ), Diff( EOperation::eEQUAL, "f" ), Diff( EOperation::eINSERT, "g" ) };
        dmp.diff_cleanupSemantic( diffs );
        assertEquals( "diff_cleanupSemantic: Backpass elimination.", TDiffVector( { Diff( EOperation::eDELETE, "abcdef" ), Diff( EOperation::eINSERT, "cdfg" ) } ), diffs );

        diffs = { Diff( EOperation::eINSERT, "1" ), Diff( EOperation::eEQUAL, "A" ), Diff( EOperation::eDELETE, "B" ), Diff( EOperation::eINSERT, "2" ), Diff( EOperation::eEQUAL, "_" ), Diff( EOperation::eINSERT, "1" ), Diff( EOperation::eEQUAL, "A" ), Diff( EOperation::eDELETE, "B" ), Diff( EOperation::eINSERT, "2" ) };
        dmp.diff_cleanupSemantic( diffs );
        assertEquals( "diff_cleanupSemantic: Multiple elimination.", TDiffVector( { Diff( EOperation::eDELETE, "AB_AB" ), Diff( EOperation::eINSERT, "1A2_1A2" ) } ), diffs );

        diffs = { Diff( EOperation::eEQUAL, "The c" ), Diff( EOperation::eDELETE, "ow and the c" ), Diff( EOperation::eEQUAL, "at." ) };
        dmp.diff_cleanupSemantic( diffs );
        assertEquals( "diff_cleanupSemantic: Word boundaries.", TDiffVector( { Diff( EOperation::eEQUAL, "The " ), Diff( EOperation::eDELETE, "cow and the " ), Diff( EOperation::eEQUAL, "cat." ) } ), diffs );

        diffs = { Diff( EOperation::eDELETE, "abcxx" ), Diff( EOperation::eINSERT, "xxdef" ) };
        dmp.diff_cleanupSemantic( diffs );
        assertEquals( "diff_cleanupSemantic: No overlap elimination.", TDiffVector( { Diff( EOperation::eDELETE, "abcxx" ), Diff( EOperation::eINSERT, "xxdef" ) } ), diffs );

        diffs = { Diff( EOperation::eDELETE, "abcxxx" ), Diff( EOperation::eINSERT, "xxxdef" ) };
        dmp.diff_cleanupSemantic( diffs );
        assertEquals( "diff_cleanupSemantic: Overlap elimination.", TDiffVector( { Diff( EOperation::eDELETE, "abc" ), Diff( EOperation::eEQUAL, "xxx" ), Diff( EOperation::eINSERT, "def" ) } ), diffs );

        diffs = { Diff( EOperation::eDELETE, "xxxabc" ), Diff( EOperation::eINSERT, "defxxx" ) };
        dmp.diff_cleanupSemantic( diffs );
        assertEquals( "diff_cleanupSemantic: Reverse overlap elimination.", TDiffVector( { Diff( EOperation::eINSERT, "def" ), Diff( EOperation::eEQUAL, "xxx" ), Diff( EOperation::eDELETE, "abc" ) } ), diffs );

        diffs = { Diff( EOperation::eDELETE, "abcd1212" ), Diff( EOperation::eINSERT, "1212efghi" ), Diff( EOperation::eEQUAL, "----" ), Diff( EOperation::eDELETE, "A3" ), Diff( EOperation::eINSERT, "3BC" ) };
        dmp.diff_cleanupSemantic( diffs );
        assertEquals( "diff_cleanupSemantic: Two overlap eliminations.", TDiffVector( { Diff( EOperation::eDELETE, "abcd" ), Diff( EOperation::eEQUAL, "1212" ), Diff( EOperation::eINSERT, "efghi" ), Diff( EOperation::eEQUAL, "----" ), Diff( EOperation::eDELETE, "A" ), Diff( EOperation::eEQUAL, "3" ), Diff( EOperation::eINSERT, "BC" ) } ), diffs );
    }

    TEST_F( diff_match_patch_test, testDiffCleanupEfficiency )
    {
        // Cleanup operationally trivial equalities.
        dmp.Diff_EditCost = 4;
        auto diffs = TDiffVector();
        dmp.diff_cleanupEfficiency( diffs );
        assertEquals( "diff_cleanupEfficiency: nullptr case.", TDiffVector(), diffs );

        diffs = { Diff( EOperation::eDELETE, "ab" ), Diff( EOperation::eINSERT, "12" ), Diff( EOperation::eEQUAL, "wxyz" ), Diff( EOperation::eDELETE, "cd" ), Diff( EOperation::eINSERT, "34" ) };
        dmp.diff_cleanupEfficiency( diffs );
        assertEquals( "diff_cleanupEfficiency: No elimination.", TDiffVector( { Diff( EOperation::eDELETE, "ab" ), Diff( EOperation::eINSERT, "12" ), Diff( EOperation::eEQUAL, "wxyz" ), Diff( EOperation::eDELETE, "cd" ), Diff( EOperation::eINSERT, "34" ) } ), diffs );

        diffs = { Diff( EOperation::eDELETE, "ab" ), Diff( EOperation::eINSERT, "12" ), Diff( EOperation::eEQUAL, "xyz" ), Diff( EOperation::eDELETE, "cd" ), Diff( EOperation::eINSERT, "34" ) };
        dmp.diff_cleanupEfficiency( diffs );
        assertEquals( "diff_cleanupEfficiency: Four-edit elimination.", TDiffVector( { Diff( EOperation::eDELETE, "abxyzcd" ), Diff( EOperation::eINSERT, "12xyz34" ) } ), diffs );

        diffs = { Diff( EOperation::eINSERT, "12" ), Diff( EOperation::eEQUAL, "x" ), Diff( EOperation::eDELETE, "cd" ), Diff( EOperation::eINSERT, "34" ) };
        dmp.diff_cleanupEfficiency( diffs );
        assertEquals( "diff_cleanupEfficiency: Three-edit elimination.", TDiffVector( { Diff( EOperation::eDELETE, "xcd" ), Diff( EOperation::eINSERT, "12x34" ) } ), diffs );

        diffs = { Diff( EOperation::eDELETE, "ab" ), Diff( EOperation::eINSERT, "12" ), Diff( EOperation::eEQUAL, "xy" ), Diff( EOperation::eINSERT, "34" ), Diff( EOperation::eEQUAL, "z" ), Diff( EOperation::eDELETE, "cd" ), Diff( EOperation::eINSERT, "56" ) };
        dmp.diff_cleanupEfficiency( diffs );
        assertEquals( "diff_cleanupEfficiency: Backpass elimination.", TDiffVector( { Diff( EOperation::eDELETE, "abxyzcd" ), Diff( EOperation::eINSERT, "12xy34z56" ) } ), diffs );

        dmp.Diff_EditCost = 5;
        diffs = { Diff( EOperation::eDELETE, "ab" ), Diff( EOperation::eINSERT, "12" ), Diff( EOperation::eEQUAL, "wxyz" ), Diff( EOperation::eDELETE, "cd" ), Diff( EOperation::eINSERT, "34" ) };
        dmp.diff_cleanupEfficiency( diffs );
        assertEquals( "diff_cleanupEfficiency: High cost elimination.", TDiffVector( { Diff( EOperation::eDELETE, "abwxyzcd" ), Diff( EOperation::eINSERT, "12wxyz34" ) } ), diffs );
        dmp.Diff_EditCost = 4;
    }

    TEST_F( diff_match_patch_test, testDiffPrettyHtml )
    {
        // Pretty print.
        auto diffs = TDiffVector( { Diff( EOperation::eEQUAL, "a\n" ), Diff( EOperation::eDELETE, "<B>b</B>" ), Diff( EOperation::eINSERT, "c&d" ) } );
        assertEquals(
            "diff_prettyHtml:",
            L"<span>a&para;<br></span><del "
            L"style=\"background:#ffe6e6;\">&lt;B&gt;b&lt;/B&gt;</del><ins "
            L"style=\"background:#e6ffe6;\">c&amp;d</ins>",
            dmp.diff_prettyHtml( diffs ) );
    }

    TEST_F( diff_match_patch_test, testDiffPrettyConsole )
    {
        // Pretty print.
        static std::wstring kRed{ L"\033[0;31m" };
        static std::wstring kGreen{ L"\033[0;32m" };
        static std::wstring kYellow{ L"\033[0;33m" };
        static std::wstring kReset{ L"\033[m" };
        static std::wstring kEOL{ NUtils::fromPercentEncoding( L"%C2%B6" ) + L"\n" };

        auto diffs = TDiffVector( { Diff( EOperation::eEQUAL, "a\n" ), Diff( EOperation::eDELETE, "<B>b</B>" ), Diff( EOperation::eINSERT, "c&d" ) } );
        auto results = dmp.diff_prettyConsole( diffs );
        assertEquals( "diff_prettyConsole:", L"a" + kEOL + kRed + L"<B>b</B>" + kReset + kGreen + L"c&d" + kReset, results );
    }

    TEST_F( diff_match_patch_test, testDiffText )
    {
        // Compute the source and destination texts.
        auto diffs = { Diff( EOperation::eEQUAL, "jump" ), Diff( EOperation::eDELETE, "s" ), Diff( EOperation::eINSERT, "ed" ), Diff( EOperation::eEQUAL, " over " ), Diff( EOperation::eDELETE, "the" ), Diff( EOperation::eINSERT, "a" ), Diff( EOperation::eEQUAL, " lazy" ) };
        assertEquals( "diff_text1:", L"jumps over the lazy", dmp.diff_text1( diffs ) );
        assertEquals( "diff_text2:", L"jumped over a lazy", dmp.diff_text2( diffs ) );
    }

    TEST_F( diff_match_patch_test, testDiffDelta )
    {
        // Convert a diff into delta string.
        auto diffs = TDiffVector( { Diff( EOperation::eEQUAL, "jump" ), Diff( EOperation::eDELETE, "s" ), Diff( EOperation::eINSERT, "ed" ), Diff( EOperation::eEQUAL, " over " ), Diff( EOperation::eDELETE, "the" ), Diff( EOperation::eINSERT, "a" ), Diff( EOperation::eEQUAL, " lazy" ), Diff( EOperation::eINSERT, "old dog" ) } );
        std::wstring text1 = dmp.diff_text1( diffs );
        assertEquals( "diff_text1: Base text.", L"jumps over the lazy", text1 );

        std::wstring delta = dmp.diff_toDelta( diffs );
        std::wstring golden = L"=4\t-1\t+ed\t=6\t-3\t+a\t=5\t+old dog";
        assertEquals( "diff_toDelta:", L"=4\t-1\t+ed\t=6\t-3\t+a\t=5\t+old dog", delta );

        // Convert delta string into a diff.
        assertEquals( "diff_fromDelta: Normal.", diffs, dmp.diff_fromDelta( text1, delta ) );

        // Generates error (19 < 20).
        assertThrow( "diff_fromDelta: Too long.", dmp.diff_fromDelta( text1 + L"x", delta ), std::wstring );

        // Generates error (19 > 18).
        assertThrow( "diff_fromDelta: Too short.", dmp.diff_fromDelta( text1.substr( 1 ), delta ), std::wstring );

        // Generates error (%c3%xy invalid Unicode).
        assertThrow( "diff_fromDelta: Invalid character.", dmp.diff_fromDelta( "", "+%c3%xy" ), std::wstring );

        // Test deltas with special characters.
        diffs = { Diff( EOperation::eEQUAL, std::wstring( L"\u0680 " ) + kZero + std::wstring( L" \t %" ) ), Diff( EOperation::eDELETE, std::wstring( L"\u0681 " ) + kOne + std::wstring( L" \n ^" ) ), Diff( EOperation::eINSERT, std::wstring( L"\u0682 " ) + kTwo + std::wstring( L" \\ |" ) ) };

        text1 = dmp.diff_text1( diffs );
        golden = std::wstring( L"\u0680 " ) + kZero + std::wstring( L" \t %\u0681 " ) + kOne + std::wstring( L" \n ^" );
        assertEquals( "diff_text1: Unicode text", golden, text1 );

        delta = dmp.diff_toDelta( diffs );
        assertEquals( "diff_toDelta: Unicode", L"=7\t-7\t+%DA%82 %02 %5C %7C", delta );

        assertEquals( "diff_fromDelta: Unicode", diffs, dmp.diff_fromDelta( text1, delta ) );

        // Verify pool of unchanged characters.
        diffs = { Diff( EOperation::eINSERT, "A-Z a-z 0-9 - _ . ! ~ * ' ( ) ; / ? : @ & = + $ , # " ) };
        std::wstring text2 = dmp.diff_text2( diffs );
        assertEquals( "diff_text2: Unchanged characters.", L"A-Z a-z 0-9 - _ . ! ~ * \' ( ) ; / ? : @ & = + $ , # ", text2 );

        delta = dmp.diff_toDelta( diffs );
        assertEquals( "diff_toDelta: Unchanged characters.", L"+A-Z a-z 0-9 - _ . ! ~ * \' ( ) ; / ? : @ & = + $ , # ", delta );

        // Convert delta string into a diff.
        assertEquals( "diff_fromDelta: Unchanged characters.", diffs, dmp.diff_fromDelta( {}, delta ) );
    }

    TEST_F( diff_match_patch_test, testDiffXIndex )
    {
        // Translate a location in text1 to text2.
        auto diffs = TDiffVector( { Diff( EOperation::eDELETE, "a" ), Diff( EOperation::eINSERT, "1234" ), Diff( EOperation::eEQUAL, "xyz" ) } );
        assertEquals( "diff_xIndex: Translation on equality.", 5, dmp.diff_xIndex( diffs, 2 ) );

        diffs = { Diff( EOperation::eEQUAL, "a" ), Diff( EOperation::eDELETE, "1234" ), Diff( EOperation::eEQUAL, "xyz" ) };
        assertEquals( "diff_xIndex: Translation on deletion.", 1, dmp.diff_xIndex( diffs, 3 ) );
    }

    TEST_F( diff_match_patch_test, testDiffLevenshtein )
    {
        auto diffs = TDiffVector( { Diff( EOperation::eDELETE, "abc" ), Diff( EOperation::eINSERT, "1234" ), Diff( EOperation::eEQUAL, "xyz" ) } );
        assertEquals( "diff_levenshtein: Trailing equality.", 4, dmp.diff_levenshtein( diffs ) );

        diffs = { Diff( EOperation::eEQUAL, "xyz" ), Diff( EOperation::eDELETE, "abc" ), Diff( EOperation::eINSERT, "1234" ) };
        assertEquals( "diff_levenshtein: Leading equality.", 4, dmp.diff_levenshtein( diffs ) );

        diffs = { Diff( EOperation::eDELETE, "abc" ), Diff( EOperation::eEQUAL, "xyz" ), Diff( EOperation::eINSERT, "1234" ) };
        assertEquals( "diff_levenshtein: Middle equality.", 7, dmp.diff_levenshtein( diffs ) );
    }

    TEST_F( diff_match_patch_test, testDiffBisect )
    {
        // Normal.
        std::wstring a = L"cat";
        std::wstring b = L"map";
        // Since the resulting diff hasn't been normalized, it would be ok if
        // the insertion and deletion pairs are swapped.
        // If the order changes, tweak this test as required.
        auto diffs = TDiffVector( { Diff( EOperation::eDELETE, "c" ), Diff( EOperation::eINSERT, "m" ), Diff( EOperation::eEQUAL, "a" ), Diff( EOperation::eDELETE, "t" ), Diff( EOperation::eINSERT, "p" ) } );
        auto results = dmp.diff_bisect( a, b, std::numeric_limits< clock_t >::max() );
        assertEquals( "diff_bisect: Normal.", diffs, results );

        // Timeout.
        diffs = { Diff( EOperation::eDELETE, "cat" ), Diff( EOperation::eINSERT, "map" ) };
        results = dmp.diff_bisect( a, b, 0 );
        assertEquals( "diff_bisect: Timeout.", diffs, results );
    }

    TEST_F( diff_match_patch_test, testDiffMain )
    {
        // Perform a trivial diff.
        auto diffs = TDiffVector();
        assertEquals( "diff_main: nullptr case.", diffs, dmp.diff_main( "", "", false ) );

        diffs = { Diff( EOperation::eDELETE, "abc" ) };
        assertEquals( "diff_main: RHS side nullptr case.", diffs, dmp.diff_main( "abc", "", false ) );

        diffs = { Diff( EOperation::eINSERT, "abc" ) };
        assertEquals( "diff_main: LHS side nullptr case.", diffs, dmp.diff_main( "", "abc", false ) );

        diffs = { Diff( EOperation::eEQUAL, "abc" ) };
        assertEquals( "diff_main: Equality.", diffs, dmp.diff_main( "abc", "abc", false ) );

        diffs = { Diff( EOperation::eEQUAL, "ab" ), Diff( EOperation::eINSERT, "123" ), Diff( EOperation::eEQUAL, "c" ) };
        assertEquals( "diff_main: Simple insertion.", diffs, dmp.diff_main( "abc", "ab123c", false ) );

        diffs = { Diff( EOperation::eEQUAL, "a" ), Diff( EOperation::eDELETE, "123" ), Diff( EOperation::eEQUAL, "bc" ) };
        assertEquals( "diff_main: Simple deletion.", diffs, dmp.diff_main( "a123bc", "abc", false ) );

        diffs = { Diff( EOperation::eEQUAL, "a" ), Diff( EOperation::eINSERT, "123" ), Diff( EOperation::eEQUAL, "b" ), Diff( EOperation::eINSERT, "456" ), Diff( EOperation::eEQUAL, "c" ) };
        assertEquals( "diff_main: Two insertions.", diffs, dmp.diff_main( "abc", "a123b456c", false ) );

        diffs = { Diff( EOperation::eEQUAL, "a" ), Diff( EOperation::eDELETE, "123" ), Diff( EOperation::eEQUAL, "b" ), Diff( EOperation::eDELETE, "456" ), Diff( EOperation::eEQUAL, "c" ) };
        assertEquals( "diff_main: Two deletions.", diffs, dmp.diff_main( "a123b456c", "abc", false ) );

        // Perform a real diff.
        // Switch off the timeout.
        dmp.Diff_Timeout = 0;
        diffs = { Diff( EOperation::eDELETE, "a" ), Diff( EOperation::eINSERT, "b" ) };
        assertEquals( "diff_main: Simple case #1.", diffs, dmp.diff_main( "a", "b", false ) );

        diffs = { Diff( EOperation::eDELETE, "Apple" ), Diff( EOperation::eINSERT, "Banana" ), Diff( EOperation::eEQUAL, "s are a" ), Diff( EOperation::eINSERT, "lso" ), Diff( EOperation::eEQUAL, " fruit." ) };
        assertEquals( "diff_main: Simple case #2.", diffs, dmp.diff_main( "Apples are a fruit.", "Bananas are also fruit.", false ) );

        diffs = { Diff( EOperation::eDELETE, "a" ), Diff( EOperation::eINSERT, L"\u0680" ), Diff( EOperation::eEQUAL, "x" ), Diff( EOperation::eDELETE, "\t" ), Diff( EOperation::eINSERT, NUtils::to_wstring( kZero ) ) };
        assertEquals( "diff_main: Simple case #3.", diffs, dmp.diff_main( L"ax\t", std::wstring( L"\u0680x" ) + kZero, false ) );

        diffs = { Diff( EOperation::eDELETE, "1" ), Diff( EOperation::eEQUAL, "a" ), Diff( EOperation::eDELETE, "y" ), Diff( EOperation::eEQUAL, "b" ), Diff( EOperation::eDELETE, "2" ), Diff( EOperation::eINSERT, "xab" ) };
        assertEquals( "diff_main: Overlap #1.", diffs, dmp.diff_main( "1ayb2", "abxab", false ) );

        diffs = { Diff( EOperation::eINSERT, "xaxcx" ), Diff( EOperation::eEQUAL, "abc" ), Diff( EOperation::eDELETE, "y" ) };
        assertEquals( "diff_main: Overlap #2.", diffs, dmp.diff_main( "abcy", "xaxcxabc", false ) );

        diffs = { Diff( EOperation::eDELETE, "ABCD" ), Diff( EOperation::eEQUAL, "a" ), Diff( EOperation::eDELETE, "=" ), Diff( EOperation::eINSERT, "-" ), Diff( EOperation::eEQUAL, "bcd" ), Diff( EOperation::eDELETE, "=" ), Diff( EOperation::eINSERT, "-" ), Diff( EOperation::eEQUAL, "efghijklmnopqrs" ), Diff( EOperation::eDELETE, "EFGHIJKLMNOefg" ) };
        assertEquals( "diff_main: Overlap #3.", diffs, dmp.diff_main( "ABCDa=bcd=efghijklmnopqrsEFGHIJKLMNOefg", "a-bcd-efghijklmnopqrs", false ) );

        diffs = { Diff( EOperation::eINSERT, " " ), Diff( EOperation::eEQUAL, "a" ), Diff( EOperation::eINSERT, "nd" ), Diff( EOperation::eEQUAL, " [[Pennsylvania]]" ), Diff( EOperation::eDELETE, " and [[New" ) };
        assertEquals( "diff_main: Large equality.", diffs, dmp.diff_main( "a [[Pennsylvania]] and [[New", " and [[Pennsylvania]]", false ) );

        dmp.Diff_Timeout = 0.1f;   // 100ms
        // This test may 'fail' on extremely fast computers.  If so, just increase the
        // text lengths.
        std::wstring a = L"`Twas brillig, and the slithy toves\nDid gyre and gimble in the "
                         L"wabe:\nAll mimsy were the borogoves,\nAnd the mome raths outgrabe.\n";
        std::wstring b = L"I am the very model of a modern major general,\nI've information "
                         L"vegetable, animal, and mineral,\nI know the kings of England, and I "
                         L"quote the fights historical,\nFrom Marathon to Waterloo, in order "
                         L"categorical.\n";
        // Increase the text lengths by 1024 times to ensure a timeout.
        for ( int x = 0; x < 10; x++ )
        {
            a = a + a;
            b = b + b;
        }
        clock_t startTime = clock();
        dmp.diff_main( a, b );
        clock_t endTime = clock();
        // Test that we took at least the timeout period.
        assertTrue( "diff_main: Timeout min.", ( dmp.Diff_Timeout * CLOCKS_PER_SEC ) <= ( endTime - startTime ) );
        // Test that we didn't take forever (be forgiving).
        // Theoretically this test could fail very occasionally if the
        // OS task swaps or locks up for a second at the wrong moment.
        // Java seems to overrun by ~80% (compared with 10% for other languages).
        // Therefore use an upper limit of 0.5s instead of 0.2s.
        assertTrue( "diff_main: Timeout max.", ( dmp.Diff_Timeout * CLOCKS_PER_SEC * 2 ) > ( endTime - startTime ) );
        dmp.Diff_Timeout = 0;

        // Test the linemode speedup.
        // Must be long to pass the 100 char cutoff.
        a = L"1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890"
            L"\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n123456789"
            L"0\n1234567890\n";
        b = L"abcdefghij\nabcdefghij\nabcdefghij\nabcdefghij\nabcdefghij\nabcdefghij"
            L"\nabcdefghij\nabcdefghij\nabcdefghij\nabcdefghij\nabcdefghij\nabcdefghi"
            L"j\nabcdefghij\n";
        assertEquals( "diff_main: Simple line-mode.", dmp.diff_main( a, b, true ), dmp.diff_main( a, b, false ) );

        a = L"12345678901234567890123456789012345678901234567890123456789012345678901"
            L"23456789012345678901234567890123456789012345678901234567890";
        b = L"abcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghija"
            L"bcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghij";
        assertEquals( "diff_main: Single line-mode.", dmp.diff_main( a, b, true ), dmp.diff_main( a, b, false ) );

        a = L"1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890"
            L"\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n123456789"
            L"0\n1234567890\n";
        b = L"abcdefghij\n1234567890\n1234567890\n1234567890\nabcdefghij\n1234567890"
            L"\n1234567890\n1234567890\nabcdefghij\n1234567890\n1234567890\n123456789"
            L"0\nabcdefghij\n";
        TStringVector texts_linemode = diff_rebuildtexts( dmp.diff_main( a, b, true ) );
        TStringVector texts_textmode = diff_rebuildtexts( dmp.diff_main( a, b, false ) );
        assertEquals( "diff_main: Overlap line-mode.", texts_textmode, texts_linemode );
    }

    //  MATCH TEST FUNCTIONS

    TEST_F( diff_match_patch_test, testMatchAlphabet )
    {
        // Initialise the bitmasks for Bitap.
        TCharPosMap bitmask;
        bitmask[ 'a' ] = 4;
        bitmask[ 'b' ] = 2;
        bitmask[ 'c' ] = 1;
        assertEquals( "match_alphabet: Unique.", bitmask, dmp.match_alphabet( "abc" ) );

        bitmask = TCharPosMap();
        bitmask[ 'a' ] = 37;
        bitmask[ 'b' ] = 18;
        bitmask[ 'c' ] = 8;
        assertEquals( "match_alphabet: Duplicates.", bitmask, dmp.match_alphabet( "abcaba" ) );
    }

    TEST_F( diff_match_patch_test, testMatchBitap )
    {
        // Bitap algorithm.
        dmp.Match_Distance = 100;
        dmp.Match_Threshold = 0.5f;
        assertEquals( "match_bitap: Exact match #1.", 5, dmp.match_bitap( "abcdefghijk", "fgh", 5 ) );

        assertEquals( "match_bitap: Exact match #2.", 5, dmp.match_bitap( "abcdefghijk", "fgh", 0 ) );

        assertEquals( "match_bitap: Fuzzy match #1.", 4, dmp.match_bitap( "abcdefghijk", "efxhi", 0 ) );

        assertEquals( "match_bitap: Fuzzy match #2.", 2, dmp.match_bitap( "abcdefghijk", "cdefxyhijk", 5 ) );

        assertEquals( "match_bitap: Fuzzy match #3.", -1, dmp.match_bitap( "abcdefghijk", "bxy", 1 ) );

        assertEquals( "match_bitap: Overflow.", 2, dmp.match_bitap( "123456789xx0", "3456789x0", 2 ) );

        assertEquals( "match_bitap: Before start match.", 0, dmp.match_bitap( "abcdef", "xxabc", 4 ) );

        assertEquals( "match_bitap: Beyond end match.", 3, dmp.match_bitap( "abcdef", "defyy", 4 ) );

        assertEquals( "match_bitap: Oversized pattern.", 0, dmp.match_bitap( "abcdef", "xabcdefy", 0 ) );

        dmp.Match_Threshold = 0.4f;
        assertEquals( "match_bitap: Threshold #1.", 4, dmp.match_bitap( "abcdefghijk", "efxyhi", 1 ) );

        dmp.Match_Threshold = 0.3f;
        assertEquals( "match_bitap: Threshold #2.", -1, dmp.match_bitap( "abcdefghijk", "efxyhi", 1 ) );

        dmp.Match_Threshold = 0.0f;
        assertEquals( "match_bitap: Threshold #3.", 1, dmp.match_bitap( "abcdefghijk", "bcdef", 1 ) );

        dmp.Match_Threshold = 0.5f;
        assertEquals( "match_bitap: Multiple select #1.", 0, dmp.match_bitap( "abcdexyzabcde", "abccde", 3 ) );

        assertEquals( "match_bitap: Multiple select #2.", 8, dmp.match_bitap( "abcdexyzabcde", "abccde", 5 ) );

        dmp.Match_Distance = 10;   // Strict location.
        assertEquals( "match_bitap: Distance test #1.", -1, dmp.match_bitap( "abcdefghijklmnopqrstuvwxyz", "abcdefg", 24 ) );

        assertEquals( "match_bitap: Distance test #2.", 0, dmp.match_bitap( "abcdefghijklmnopqrstuvwxyz", "abcdxxefg", 1 ) );

        dmp.Match_Distance = 1000;   // Loose location.
        assertEquals( "match_bitap: Distance test #3.", 0, dmp.match_bitap( "abcdefghijklmnopqrstuvwxyz", "abcdefg", 24 ) );
    }

    TEST_F( diff_match_patch_test, testMatchMain )
    {
        // Full match.
        assertEquals( "match_main: Equality.", 0, dmp.match_main( "abcdef", "abcdef", 1000 ) );

        assertEquals( "match_main: nullptr text.", -1, dmp.match_main( "", "abcdef", 1 ) );

        assertEquals( "match_main: nullptr pattern.", 3, dmp.match_main( "abcdef", "", 3 ) );

        assertEquals( "match_main: Exact match.", 3, dmp.match_main( "abcdef", "de", 3 ) );

        dmp.Match_Threshold = 0.7f;
        assertEquals( "match_main: Complex match.", 4, dmp.match_main( "I am the very model of a modern major general.", " that berry ", 5 ) );
        dmp.Match_Threshold = 0.5f;
    }

    //  PATCH TEST FUNCTIONS

    TEST_F( diff_match_patch_test, testPatchObj )
    {
        // Patch Object.
        Patch p;
        p.start1 = 20;
        p.start2 = 21;
        p.length1 = 18;
        p.length2 = 17;
        p.diffs = { Diff( EOperation::eEQUAL, "jump" ), Diff( EOperation::eDELETE, "s" ), Diff( EOperation::eINSERT, "ed" ), Diff( EOperation::eEQUAL, " over " ), Diff( EOperation::eDELETE, "the" ), Diff( EOperation::eINSERT, "a" ), Diff( EOperation::eEQUAL, "\nlaz" ) };
        std::wstring strp = L"@@ -21,18 +22,17 @@\n jump\n-s\n+ed\n  over \n-the\n+a\n %0Alaz\n";
        assertEquals( "patch: toString.", strp, p.toString() );
    }

    TEST_F( diff_match_patch_test, testPatchFromText )
    {
        assertTrue( "patch_fromText: #0.", dmp.patch_fromText( "" ).empty() );

        std::wstring strp = L"@@ -21,18 +22,17 @@\n jump\n-s\n+ed\n  over \n-the\n+a\n %0Alaz\n";
        assertEquals( "patch_fromText: #1.", strp, dmp.patch_fromText( strp )[ 0 ].toString() );

        assertEquals( "patch_fromText: #2.", L"@@ -1 +1 @@\n-a\n+b\n", dmp.patch_fromText( "@@ -1 +1 @@\n-a\n+b\n" )[ 0 ].toString() );

        assertEquals( "patch_fromText: #3.", L"@@ -1,3 +0,0 @@\n-abc\n", dmp.patch_fromText( "@@ -1,3 +0,0 @@\n-abc\n" )[ 0 ].toString() );

        assertEquals( "patch_fromText: #4.", L"@@ -0,0 +1,3 @@\n+abc\n", dmp.patch_fromText( "@@ -0,0 +1,3 @@\n+abc\n" )[ 0 ].toString() );

        // Generates error.
        assertThrow( "patch_fromText: #5.", dmp.patch_fromText( "Bad\nPatch\n" ), std::wstring );
    }

    TEST_F( diff_match_patch_test, testPatchToText )
    {
        std::wstring strp = L"@@ -21,18 +22,17 @@\n jump\n-s\n+ed\n  over \n-the\n+a\n  laz\n";
        auto patches = dmp.patch_fromText( strp );
        assertEquals( "patch_toText: Single", strp, dmp.patch_toText( patches ) );

        strp = L"@@ -1,9 +1,9 @@\n-f\n+F\n oo+fooba\n@@ -7,9 +7,9 @@\n obar\n-,\n+.\n  "
               L"tes\n";
        patches = dmp.patch_fromText( strp );
        assertEquals( "patch_toText: Dua", strp, dmp.patch_toText( patches ) );
    }

    TEST_F( diff_match_patch_test, testPatchAddContext )
    {
        dmp.Patch_Margin = 4;
        auto p = dmp.patch_fromText( "@@ -21,4 +21,10 @@\n-jump\n+somersault\n" )[ 0 ];
        dmp.patch_addContext( p, "The quick brown fox jumps over the lazy dog." );
        assertEquals( "patch_addContext: Simple case.", L"@@ -17,12 +17,18 @@\n fox \n-jump\n+somersault\n s ov\n", p.toString() );

        p = dmp.patch_fromText( "@@ -21,4 +21,10 @@\n-jump\n+somersault\n" )[ 0 ];
        dmp.patch_addContext( p, "The quick brown fox jumps." );
        assertEquals( "patch_addContext: Not enough trailing context.", L"@@ -17,10 +17,16 @@\n fox \n-jump\n+somersault\n s.\n", p.toString() );

        p = dmp.patch_fromText( "@@ -3 +3,2 @@\n-e\n+at\n" )[ 0 ];
        dmp.patch_addContext( p, "The quick brown fox jumps." );
        assertEquals( "patch_addContext: Not enough leading context.", L"@@ -1,7 +1,8 @@\n Th\n-e\n+at\n  qui\n", p.toString() );

        p = dmp.patch_fromText( "@@ -3 +3,2 @@\n-e\n+at\n" )[ 0 ];
        dmp.patch_addContext( p, "The quick brown fox jumps.  The quick brown fox crashes." );
        assertEquals( "patch_addContext: Ambiguity.", L"@@ -1,27 +1,28 @@\n Th\n-e\n+at\n  quick brown fox jumps. \n", p.toString() );
    }

    TEST_F( diff_match_patch_test, testPatchMake )
    {
        TPatchVector patches;
        patches = dmp.patch_make( "", "" );
        assertEquals( "patch_make: nullptr case", L"", dmp.patch_toText( patches ) );

        std::wstring text1 = L"The quick brown fox jumps over the lazy dog.";
        std::wstring text2 = L"That quick brown fox jumped over a lazy dog.";
        std::wstring expectedPatch = L"@@ -1,8 +1,7 @@\n Th\n-at\n+e\n  qui\n@@ -21,17 +21,18 @@\n "
                                     L"jump\n-ed\n+s\n  over \n-a\n+the\n  laz\n";
        // The second patch must be "-21,17 +21,18", not "-22,17 +21,18" due to
        // rolling context.
        patches = dmp.patch_make( text2, text1 );
        assertEquals( "patch_make: Text2+Text1 inputs", expectedPatch, dmp.patch_toText( patches ) );

        expectedPatch = L"@@ -1,11 +1,12 @@\n Th\n-e\n+at\n  quick b\n@@ -22,18 +22,17 @@\n "
                        L"jump\n-s\n+ed\n  over \n-the\n+a\n  laz\n";
        patches = dmp.patch_make( text1, text2 );
        assertEquals( "patch_make: Text1+Text2 inputs", expectedPatch, dmp.patch_toText( patches ) );

        auto diffs = dmp.diff_main( text1, text2, false );
        patches = dmp.patch_make( diffs );
        assertEquals( "patch_make: Diff input", expectedPatch, dmp.patch_toText( patches ) );

        patches = dmp.patch_make( text1, diffs );
        assertEquals( "patch_make: Text1+Diff inputs", expectedPatch, dmp.patch_toText( patches ) );

        patches = dmp.patch_make( text1, text2, diffs );
        assertEquals( "patch_make: Text1+Text2+Diff inputs (deprecated)", expectedPatch, dmp.patch_toText( patches ) );

        patches = dmp.patch_make( "`1234567890-=[]\\;',./", "~!@#$%^&*()_+{}|:\"<>?" );
        assertEquals(
            "patch_toText: Character encoding.",
            L"@@ -1,21 +1,21 "
            L"@@\n-%601234567890-=%5B%5D%5C;',./"
            L"\n+~!@#$%25%5E&*()_+%7B%7D%7C:%22%3C%3E?\n",
            dmp.patch_toText( patches ) );

        diffs = { Diff( EOperation::eDELETE, "`1234567890-=[]\\;',./" ), Diff( EOperation::eINSERT, "~!@#$%^&*()_+{}|:\"<>?" ) };
        assertEquals(
            "patch_fromText: Character decoding.", diffs,
            dmp.patch_fromText( "@@ -1,21 +1,21 "
                                "@@\n-%601234567890-=%5B%5D%5C;',./"
                                "\n+~!@#$%25%5E&*()_+%7B%7D%7C:%22%3C%3E?\n" )[ 0 ]
                .diffs );

        text1 = {};
        for ( int x = 0; x < 100; x++ )
        {
            text1 += L"abcdef";
        }
        text2 = text1 + L"123";
        expectedPatch = L"@@ -573,28 +573,31 @@\n cdefabcdefabcdefabcdefabcdef\n+123\n";
        patches = dmp.patch_make( text1, text2 );
        assertEquals( "patch_make: Long string with repeats.", expectedPatch, dmp.patch_toText( patches ) );
    }

    TEST_F( diff_match_patch_test, testPatchSplitMax )
    {
        dmp.Diff_Timeout = 0;
        // Confirm Match_MaxBits is 32.
        TPatchVector patches;
        patches = dmp.patch_make( "abcdefghijklmnopqrstuvwxyz01234567890", "XabXcdXefXghXijXklXmnXopXqrXstXuvXwxXyzX01X23X45X67X89X0" );
        dmp.patch_splitMax( patches );
        assertEquals(
            "patch_splitMax: #1.",
            L"@@ -1,32 +1,46 @@\n+X\n ab\n+X\n cd\n+X\n ef\n+X\n gh\n+X\n "
            L"ij\n+X\n kl\n+X\n mn\n+X\n op\n+X\n qr\n+X\n st\n+X\n "
            L"uv\n+X\n wx\n+X\n yz\n+X\n 012345\n@@ -25,13 +39,18 @@\n "
            L"zX01\n+X\n 23\n+X\n 45\n+X\n 67\n+X\n 89\n+X\n 0\n",
            dmp.patch_toText( patches ) );

        patches = dmp.patch_make(
            "abcdef123456789012345678901234567890123456789012345678901234567890123456"
            "7890uvwxyz",
            "abcdefuvwxyz" );
        std::wstring oldToText = dmp.patch_toText( patches );
        dmp.patch_splitMax( patches );
        assertEquals( "patch_splitMax: #2.", oldToText, dmp.patch_toText( patches ) );

        patches = dmp.patch_make( "1234567890123456789012345678901234567890123456789012345678901234567890", "abc" );
        dmp.patch_splitMax( patches );
        auto golden = L"@@ -1,32 +1,4 @@\n-1234567890123456789012345678\n 9012\n@@ "
                      L"-29,32 +1,4 @@\n-9012345678901234567890123456\n 7890\n@@ "
                      L"-57,14 +1,3 @@\n-78901234567890\n+abc\n";
        auto results = dmp.patch_toText( patches );
        assertEquals( "patch_splitMax: #3.", golden, results );

        patches = dmp.patch_make(
            "abcdefghij , h : 0 , t : 1 abcdefghij , h : 0 , t : 1 abcdefghij , h : "
            "0 , t : 1",
            "abcdefghij , h : 1 , t : 1 abcdefghij , h : 1 , t : 1 abcdefghij , h : "
            "0 , t : 1" );
        dmp.patch_splitMax( patches );
        assertEquals(
            "patch_splitMax: #4.",
            L"@@ -2,32 +2,32 @@\n bcdefghij , h : \n-0\n+1\n  , t : 1 abcdef\n@@ "
            L"-29,32 +29,32 @@\n bcdefghij , h : \n-0\n+1\n  , t : 1 abcdef\n",
            dmp.patch_toText( patches ) );
    }

    TEST_F( diff_match_patch_test, testPatchAddPadding )
    {
        TPatchVector patches;
        patches = dmp.patch_make( "", "test" );
        assertEquals( "patch_addPadding: Both edges ful", L"@@ -0,0 +1,4 @@\n+test\n", dmp.patch_toText( patches ) );
        dmp.patch_addPadding( patches );
        assertEquals( "patch_addPadding: Both edges full.", L"@@ -1,8 +1,12 @@\n %01%02%03%04\n+test\n %01%02%03%04\n", dmp.patch_toText( patches ) );

        patches = dmp.patch_make( "XY", "XtestY" );
        assertEquals( "patch_addPadding: Both edges partial.", L"@@ -1,2 +1,6 @@\n X\n+test\n Y\n", dmp.patch_toText( patches ) );
        dmp.patch_addPadding( patches );
        assertEquals( "patch_addPadding: Both edges partial.", L"@@ -2,8 +2,12 @@\n %02%03%04X\n+test\n Y%01%02%03\n", dmp.patch_toText( patches ) );

        patches = dmp.patch_make( "XXXXYYYY", "XXXXtestYYYY" );
        assertEquals( "patch_addPadding: Both edges none.", L"@@ -1,8 +1,12 @@\n XXXX\n+test\n YYYY\n", dmp.patch_toText( patches ) );
        dmp.patch_addPadding( patches );
        assertEquals( "patch_addPadding: Both edges none.", L"@@ -5,8 +5,12 @@\n XXXX\n+test\n YYYY\n", dmp.patch_toText( patches ) );
    }

    TEST_F( diff_match_patch_test, testPatchApply )
    {
        dmp.Match_Distance = 1000;
        dmp.Match_Threshold = 0.5f;
        dmp.Patch_DeleteThreshold = 0.5f;
        TPatchVector patches;
        patches = dmp.patch_make( "", "" );
        auto results = dmp.patch_apply( patches, "Hello world." );
        auto &&boolArray = results.second;

        std::wstring resultStr = results.first + L"\t" + std::to_wstring( boolArray.size() );
        assertEquals( "patch_apply: nullptr case.", L"Hello world.\t0", resultStr );

        patches = dmp.patch_make( "The quick brown fox jumps over the lazy dog.", "That quick brown fox jumped over a lazy dog." );
        assertEquals(
            "patch_apply: Exact match.",
            L"@@ -1,11 +1,12 @@\n Th\n-e\n+at\n  quick b\n@@ -22,18 +22,17 "
            L"@@\n jump\n-s\n+ed\n  over \n-the\n+a\n  laz\n",
            dmp.patch_toText( patches ) );

        results = dmp.patch_apply( patches, "The quick brown fox jumps over the lazy dog." );
        boolArray = results.second;
        resultStr = results.first + NUtils::to_wstring( boolArray, false );

        assertEquals( "patch_apply: Exact match.", L"That quick brown fox jumped over a lazy dog.(true, true)", resultStr );

        results = dmp.patch_apply( patches, "The quick red rabbit jumps over the tired tiger." );
        boolArray = results.second;
        resultStr = results.first + NUtils::to_wstring( boolArray, false );
        assertEquals( "patch_apply: Partial match.", L"That quick red rabbit jumped over a tired tiger.(true, true)", resultStr );

        results = dmp.patch_apply( patches, "I am the very model of a modern major general." );
        boolArray = results.second;
        resultStr = results.first + NUtils::to_wstring( boolArray, false );
        assertEquals( "patch_apply: Failed match.", L"I am the very model of a modern major general.(false, false)", resultStr );

        patches = dmp.patch_make(
            "x1234567890123456789012345678901234567890123456789012345678901234567890"
            "y",
            "xabcy" );
        results = dmp.patch_apply(
            patches, "x123456789012345678901234567890-----++++++++++----"
                     "-123456789012345678901234567890y" );
        boolArray = results.second;
        resultStr = results.first + NUtils::to_wstring( boolArray, false );
        assertEquals( "patch_apply: Big delete, small change.", L"xabcy(true, true)", resultStr );

        patches = dmp.patch_make(
            "x1234567890123456789012345678901234567890123456789012345678901234567890"
            "y",
            "xabcy" );
        results = dmp.patch_apply(
            patches, "x12345678901234567890---------------++++++++++----"
                     "-----------12345678901234567890y" );
        boolArray = results.second;
        resultStr = results.first + NUtils::to_wstring( boolArray, false );
        assertEquals(
            "patch_apply: Big delete, large change 1.",
            L"xabc12345678901234567890---------------++++++++++-------------"
            L"--12345678901234567890y(false, true)",
            resultStr );

        dmp.Patch_DeleteThreshold = 0.6f;
        patches = dmp.patch_make(
            "x1234567890123456789012345678901234567890123456789012345678901234567890"
            "y",
            "xabcy" );
        results = dmp.patch_apply(
            patches, "x12345678901234567890---------------++++++++++----"
                     "-----------12345678901234567890y" );
        boolArray = results.second;
        resultStr = results.first + NUtils::to_wstring( boolArray, false );
        assertEquals( "patch_apply: Big delete, large change 2.", L"xabcy(true, true)", resultStr );
        dmp.Patch_DeleteThreshold = 0.5f;

        dmp.Match_Threshold = 0.0f;
        dmp.Match_Distance = 0;
        patches = dmp.patch_make(
            "abcdefghijklmnopqrstuvwxyz--------------------1234567890", "abcXXXXXXXXXXdefghijklmnopqrstuvwxyz--------------------"
                                                                        "1234567YYYYYYYYYY890" );
        results = dmp.patch_apply( patches, "ABCDEFGHIJKLMNOPQRSTUVWXYZ--------------------1234567890" );
        boolArray = results.second;
        resultStr = results.first + NUtils::to_wstring( boolArray, false );
        assertEquals(
            "patch_apply: Compensate for failed patch.",
            L"ABCDEFGHIJKLMNOPQRSTUVWXYZ--------------------"
            L"1234567YYYYYYYYYY890(false, true)",
            resultStr );
        dmp.Match_Threshold = 0.5f;
        dmp.Match_Distance = 1000;

        patches = dmp.patch_make( "", "test" );
        std::wstring patchStr = dmp.patch_toText( patches );
        dmp.patch_apply( patches, "" );
        assertEquals( "patch_apply: No side effects.", patchStr, dmp.patch_toText( patches ) );

        patches = dmp.patch_make( "The quick brown fox jumps over the lazy dog.", "Woof" );
        patchStr = dmp.patch_toText( patches );
        dmp.patch_apply( patches, "The quick brown fox jumps over the lazy dog." );
        assertEquals( "patch_apply: No side effects with major delete.", patchStr, dmp.patch_toText( patches ) );

        patches = dmp.patch_make( "", "test" );
        results = dmp.patch_apply( patches, "" );
        boolArray = results.second;
        resultStr = results.first + L"\t" + NUtils::to_wstring( boolArray[ 0 ], false );
        assertEquals( "patch_apply: Edge exact match.", L"test\ttrue", resultStr );

        patches = dmp.patch_make( "XY", "XtestY" );
        results = dmp.patch_apply( patches, "XY" );
        boolArray = results.second;
        resultStr = results.first + L"\t" + NUtils::to_wstring( boolArray[ 0 ], false );
        assertEquals( "patch_apply: Near edge exact match.", L"XtestY\ttrue", resultStr );

        patches = dmp.patch_make( "y", "y123" );
        results = dmp.patch_apply( patches, "x" );
        boolArray = results.second;
        resultStr = results.first + L"\t" + NUtils::to_wstring( boolArray[ 0 ], false );
        assertEquals( "patch_apply: Edge partial match.", L"x123\ttrue", resultStr );
    }

    TEST_F( diff_match_patch_test, fromGitHubExamples )
    {
        auto lhs = L"I am the very model of a modern Major-General, I've information "
                   L"vegetable, animal, and mineral, I know the kings of England, and I "
                   L"quote the fights historical, From Marathon to Waterloo, in order "
                   L"categorical.";
        auto rhs = L"I am the very model of a cartoon individual, My animation's comical, "
                   L"unusual, and whimsical, I'm quite adept at funny gags, comedic theory "
                   L"I have read, From wicked puns and stupid jokes to anvils that drop on "
                   L"your head.";
        auto diffs = dmp.diff_main( lhs, rhs );
        dmp.diff_cleanupSemantic( diffs );
        auto console = dmp.diff_prettyConsole( diffs );
        auto html = dmp.diff_prettyHtml( diffs );
        auto delta = dmp.diff_toDelta( diffs );

        auto consoleGolden = L"I am the very model of a \x1B[0;31mmodern Major-General, I've "
                             L"information vegetable, animal, and mineral, I know the kings of "
                             L"England, and I quote the fights historical, From Marathon to Waterloo, "
                             L"in order categorical\x1B[m\x1B[0;32mcartoon individual, My animation's "
                             L"comical, unusual, and whimsical, I'm quite adept at funny gags, "
                             L"comedic theory I have read, From wicked puns and stupid jokes to "
                             L"anvils that drop on your head\x1B[m.";
        assertEquals( "gitHubDemos", consoleGolden, console );

        auto htmlGolden = LR"(<span>I am the very model of a </span><del style="background:#ffe6e6;">modern Major-General, I've information vegetable, animal, and mineral, I know the kings of England, and I quote the fights historical, From Marathon to Waterloo, in order categorical</del><ins style="background:#e6ffe6;">cartoon individual, My animation's comical, unusual, and whimsical, I'm quite adept at funny gags, comedic theory I have read, From wicked puns and stupid jokes to anvils that drop on your head</ins><span>.</span>)";
        assertEquals( "gitHubDemos", htmlGolden, html );

        auto deltaGolden = L"=25\t-182\t+cartoon individual, My animation's comical, unusual, and "
                           L"whimsical, I'm quite adept at funny gags, comedic theory I have read, "
                           L"From wicked puns and stupid jokes to anvils that drop on your head\t=1";
        assertEquals( "gitHubDemos", deltaGolden, delta );

        auto patches = dmp.patch_make( lhs, rhs );
        auto patch = dmp.patch_toText( patches );
        auto patchGolden = L"@@ -22,187 +22,198 @@\n f a \n-modern Major-General, I've information "
                           L"vegetable, animal, and mineral, I know the kings of England, and I "
                           L"quote the fights historical, From Marathon to Waterloo, in order "
                           L"categorical\n+cartoon individual, My animation's comical, unusual, and "
                           L"whimsical, I'm quite adept at funny gags, comedic theory I have read, "
                           L"From wicked puns and stupid jokes to anvils that drop on your head\n "
                           L".\n";
        assertEquals( "gitHubDemos", patchGolden, patch );
    }
}
