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

// Code known to compile and run with Qt 4.3 through Qt 4.7.
#include "diff_match_patch.h"
#include "diff_match_patch_test.h"

#include <iostream>
#include <chrono>

int main( int /*argc*/, char ** /*argv*/ )
{
    diff_match_patch_test dmp_test;
    std::cerr << "Starting diff_match_patch unit tests.\n";
    dmp_test.run_all_tests();
    std::cerr << "Done.\n";
    return 0;
}

static wchar_t kZero{ 0 };
static wchar_t kOne{ 1 };
static wchar_t kTwo{ 2 };

diff_match_patch_test::diff_match_patch_test()
{
}

void diff_match_patch_test::runTest( std::function< void() > test )
{
    try
    {
        test();
        numPassedTests++;
    }
    //catch ( const char *msg )
    //{
    //    std::cerr << "Test failed: " << msg << "\n";
    //}
    catch ( std::string msg )
    {
        std::cerr << "Test failed: " << msg << "\n";
        numFailedTests++;
    }
}

void diff_match_patch_test::run_all_tests()
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
}

//  DIFF TEST FUNCTIONS

void diff_match_patch_test::testDiffCommonPrefix()
{
    // Detect any common prefix.
    assertEquals( "diff_commonPrefix: nullptr case.", 0, dmp.diff_commonPrefix( "abc", "xyz" ) );

    assertEquals( "diff_commonPrefix: Non-nullptr case.", 4, dmp.diff_commonPrefix( "1234abcdef", "1234xyz" ) );

    assertEquals( "diff_commonPrefix: Whole case.", 4, dmp.diff_commonPrefix( "1234", "1234xyz" ) );
}

void diff_match_patch_test::testDiffCommonSuffix()
{
    // Detect any common suffix.
    assertEquals( "diff_commonSuffix: nullptr case.", 0, dmp.diff_commonSuffix( "abc", "xyz" ) );

    assertEquals( "diff_commonSuffix: Non-nullptr case.", 4, dmp.diff_commonSuffix( "abcdef1234", "xyz1234" ) );

    assertEquals( "diff_commonSuffix: Whole case.", 4, dmp.diff_commonSuffix( "1234", "xyz1234" ) );
}

void diff_match_patch_test::testDiffCommonOverlap()
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

void diff_match_patch_test::testDiffHalfmatch()
{
    // Detect a halfmatch.
    dmp.Diff_Timeout = 1;
    assertEmpty( "diff_halfMatch: No match #1.", dmp.diff_halfMatch( "1234567890", "abcdef" ) );

    assertEmpty( "diff_halfMatch: No match #2.", dmp.diff_halfMatch( "12345", "23" ) );

    assertEquals( "diff_halfMatch: Single Match #1.", { L"12", L"90", L"a", L"z", L"345678" }, dmp.diff_halfMatch( "1234567890", "a345678z" ) );

    assertEquals( "diff_halfMatch: Single Match #2.", { L"a", L"z", L"12", L"90", L"345678" }, dmp.diff_halfMatch( "a345678z", "1234567890" ) );

    assertEquals( "diff_halfMatch: Single Match #3.", { L"abc", L"z", L"1234", L"0", L"56789" }, dmp.diff_halfMatch( "abc56789z", "1234567890" ) );

    assertEquals( "diff_halfMatch: Single Match #4.", { L"a", L"xyz", L"1", L"7890", L"23456" }, dmp.diff_halfMatch( "a23456xyz", "1234567890" ) );

    assertEquals( "diff_halfMatch: Multiple Matches #1.", { L"12123", L"123121", L"a", L"z", L"1234123451234" }, dmp.diff_halfMatch( "121231234123451234123121", "a1234123451234z" ) );

    assertEquals( "diff_halfMatch: Multiple Matches #2.", { L"", L"-=-=-=-=-=", L"x", L"", L"x-=-=-=-=-=-=-=" }, dmp.diff_halfMatch( "x-=-=-=-=-=-=-=-=-=-=-=-=", "xx-=-=-=-=-=-=-=" ) );

    assertEquals( "diff_halfMatch: Multiple Matches #3.", { L"-=-=-=-=-=", L"", L"", L"y", L"-=-=-=-=-=-=-=y" }, dmp.diff_halfMatch( "-=-=-=-=-=-=-=-=-=-=-=-=y", "-=-=-=-=-=-=-=yy" ) );

    // Optimal diff would be -q+x=H-i+e=lloHe+Hu=llo-Hew+y not -qHillo+x=HelloHe-w+Hulloy
    assertEquals( "diff_halfMatch: Non-optimal halfmatch.", { L"qHillo", L"w", L"x", L"Hulloy", L"HelloHe" }, dmp.diff_halfMatch( "qHilloHelloHew", "xHelloHeHulloy" ) );

    dmp.Diff_Timeout = 0;
    assertEmpty( "diff_halfMatch: Optimal no halfmatch.", dmp.diff_halfMatch( L"qHilloHelloHew", L"xHelloHeHulloy" ) );
}

void diff_match_patch_test::testDiffLinesToChars()
{
    // Convert lines down to characters.
    TStringVector tmpVector = TStringVector( { L"", L"alpha\n", L"beta\n" } );
    TVariantVector tmpVarList;
    tmpVarList.emplace_back( to_wstring( { 1, 2, 1 } ) );   //(("\u0001\u0002\u0001"));
    tmpVarList.emplace_back( to_wstring( { 2, 1, 2 } ) );   // (("\u0002\u0001\u0002"));
    tmpVarList.emplace_back( tmpVector );
    assertEquals( "diff_linesToChars:", tmpVarList, dmp.diff_linesToChars( "alpha\nbeta\nalpha\n", "beta\nalpha\nbeta\n" ) );

    tmpVector.clear();
    tmpVarList.clear();
    tmpVector.emplace_back( L"" );
    tmpVector.emplace_back( L"alpha\r\n" );
    tmpVector.emplace_back( L"beta\r\n" );
    tmpVector.emplace_back( L"\r\n" );
    tmpVarList.emplace_back( std::wstring() );
    tmpVarList.emplace_back( to_wstring( { 1, 2, 3, 3 } ) );   // (("\u0001\u0002\u0003\u0003"));
    tmpVarList.emplace_back( tmpVector );
    assertEquals( "diff_linesToChars:", tmpVarList, dmp.diff_linesToChars( "", "alpha\r\nbeta\r\n\r\n\r\n" ) );

    tmpVector.clear();
    tmpVarList.clear();
    tmpVector.emplace_back( L"" );
    tmpVector.emplace_back( L"a" );
    tmpVector.emplace_back( L"b" );
    tmpVarList.emplace_back( to_wstring( 1 ) );   // (("\u0001"));
    tmpVarList.emplace_back( to_wstring( 2 ) );   // (("\u0002"));
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
        chars += to_wstring( x );
    }
    assertEquals( "diff_linesToChars: More than 256 (setup).", n, tmpVector.size() );
    assertEquals( "diff_linesToChars: More than 256 (setup).", n, chars.length() );
    tmpVector.emplace( tmpVector.begin(), L"" );
    tmpVarList.emplace_back( chars );
    tmpVarList.emplace_back( std::wstring() );
    tmpVarList.emplace_back( tmpVector );
    assertEquals( "diff_linesToChars: More than 256.", tmpVarList, dmp.diff_linesToChars( lines, {} ) );
}

void diff_match_patch_test::testDiffCharsToLines()
{
    // First check that Diff equality works.
    assertTrue( "diff_charsToLines:", Diff( EQUAL, "a" ) == Diff( EQUAL, "a" ) );

    assertEquals( "diff_charsToLines:", Diff( EQUAL, "a" ), Diff( EQUAL, "a" ) );

    // Convert chars up to lines.
    TDiffVector diffs;
    diffs.emplace_back( EQUAL, to_wstring( { 1, 2, 1 } ) );   // ("\u0001\u0002\u0001");
    diffs.emplace_back( INSERT, to_wstring( { 2, 1, 2 } ) );   // ("\u0002\u0001\u0002");
    TStringVector tmpVector;
    tmpVector.emplace_back( L"" );
    tmpVector.emplace_back( L"alpha\n" );
    tmpVector.emplace_back( L"beta\n" );
    dmp.diff_charsToLines( diffs, tmpVector );
    assertEquals( "diff_charsToLines:", { Diff( EQUAL, "alpha\nbeta\nalpha\n" ), Diff( INSERT, "beta\nalpha\nbeta\n" ) }, diffs );

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
        chars += to_wstring( x );
    }
    assertEquals( "diff_linesToChars: More than 256 (setup).", n, tmpVector.size() );
    assertEquals( "diff_linesToChars: More than 256 (setup).", n, chars.length() );
    tmpVector.emplace( tmpVector.begin(), L"" );
    diffs = { Diff( DELETE, chars ) };
    dmp.diff_charsToLines( diffs, tmpVector );
    assertEquals( "diff_charsToLines: More than 256.", { Diff( DELETE, lines ) }, diffs );
}

void diff_match_patch_test::testDiffCleanupMerge()
{
    // Cleanup a messy diff.
    TDiffVector diffs;
    dmp.diff_cleanupMerge( diffs );
    assertEquals( "diff_cleanupMerge: nullptr case.", {}, diffs );

    diffs = { Diff( EQUAL, "a" ), Diff( DELETE, "b" ), Diff( INSERT, "c" ) };
    dmp.diff_cleanupMerge( diffs );
    assertEquals( "diff_cleanupMerge: No change case.", { Diff( EQUAL, "a" ), Diff( DELETE, "b" ), Diff( INSERT, "c" ) }, diffs );

    diffs = { Diff( EQUAL, "a" ), Diff( EQUAL, "b" ), Diff( EQUAL, "c" ) };
    dmp.diff_cleanupMerge( diffs );
    assertEquals( "diff_cleanupMerge: Merge equalities.", { Diff( EQUAL, "abc" ) }, diffs );

    diffs = { Diff( DELETE, "a" ), Diff( DELETE, "b" ), Diff( DELETE, "c" ) };
    dmp.diff_cleanupMerge( diffs );
    assertEquals( "diff_cleanupMerge: Merge deletions.", { Diff( DELETE, "abc" ) }, diffs );

    diffs = { Diff( INSERT, "a" ), Diff( INSERT, "b" ), Diff( INSERT, "c" ) };
    dmp.diff_cleanupMerge( diffs );
    assertEquals( "diff_cleanupMerge: Merge insertions.", { Diff( INSERT, "abc" ) }, diffs );

    diffs = { Diff( DELETE, "a" ), Diff( INSERT, "b" ), Diff( DELETE, "c" ), Diff( INSERT, "d" ), Diff( EQUAL, "e" ), Diff( EQUAL, "f" ) };
    dmp.diff_cleanupMerge( diffs );
    assertEquals( "diff_cleanupMerge: Merge interweave.", { Diff( DELETE, "ac" ), Diff( INSERT, "bd" ), Diff( EQUAL, "ef" ) }, diffs );

    diffs = { Diff( DELETE, "a" ), Diff( INSERT, "abc" ), Diff( DELETE, "dc" ) };
    dmp.diff_cleanupMerge( diffs );
    assertEquals( "diff_cleanupMerge: Prefix and suffix detection.", { Diff( EQUAL, "a" ), Diff( DELETE, "d" ), Diff( INSERT, "b" ), Diff( EQUAL, "c" ) }, diffs );

    diffs = { Diff( EQUAL, "x" ), Diff( DELETE, "a" ), Diff( INSERT, "abc" ), Diff( DELETE, "dc" ), Diff( EQUAL, "y" ) };
    dmp.diff_cleanupMerge( diffs );
    assertEquals( "diff_cleanupMerge: Prefix and suffix detection with equalities.", { Diff( EQUAL, "xa" ), Diff( DELETE, "d" ), Diff( INSERT, "b" ), Diff( EQUAL, "cy" ) }, diffs );

    diffs = { Diff( EQUAL, "a" ), Diff( INSERT, "ba" ), Diff( EQUAL, "c" ) };
    dmp.diff_cleanupMerge( diffs );
    assertEquals( "diff_cleanupMerge: Slide edit left.", { Diff( INSERT, "ab" ), Diff( EQUAL, "ac" ) }, diffs );

    diffs = { Diff( EQUAL, "c" ), Diff( INSERT, "ab" ), Diff( EQUAL, "a" ) };
    dmp.diff_cleanupMerge( diffs );
    assertEquals( "diff_cleanupMerge: Slide edit right.", { Diff( EQUAL, "ca" ), Diff( INSERT, "ba" ) }, diffs );

    diffs = { Diff( EQUAL, "a" ), Diff( DELETE, "b" ), Diff( EQUAL, "c" ), Diff( DELETE, "ac" ), Diff( EQUAL, "x" ) };
    dmp.diff_cleanupMerge( diffs );
    assertEquals( "diff_cleanupMerge: Slide edit left recursive.", { Diff( DELETE, "abc" ), Diff( EQUAL, "acx" ) }, diffs );

    diffs = { Diff( EQUAL, "x" ), Diff( DELETE, "ca" ), Diff( EQUAL, "c" ), Diff( DELETE, "b" ), Diff( EQUAL, "a" ) };
    dmp.diff_cleanupMerge( diffs );
    assertEquals( "diff_cleanupMerge: Slide edit right recursive.", { Diff( EQUAL, "xca" ), Diff( DELETE, "cba" ) }, diffs );
}

void diff_match_patch_test::testDiffCleanupSemanticLossless()
{
    // Slide diffs to match logical boundaries.
    auto diffs = TDiffVector();
    dmp.diff_cleanupSemanticLossless( diffs );
    assertEquals( "diff_cleanupSemantic: nullptr case.", {}, diffs );

    diffs = { Diff( EQUAL, "AAA\r\n\r\nBBB" ), Diff( INSERT, "\r\nDDD\r\n\r\nBBB" ), Diff( EQUAL, "\r\nEEE" ) };
    dmp.diff_cleanupSemanticLossless( diffs );
    assertEquals( "diff_cleanupSemanticLossless: Blank lines.", { Diff( EQUAL, "AAA\r\n\r\n" ), Diff( INSERT, "BBB\r\nDDD\r\n\r\n" ), Diff( EQUAL, "BBB\r\nEEE" ) }, diffs );

    diffs = { Diff( EQUAL, "AAA\r\nBBB" ), Diff( INSERT, " DDD\r\nBBB" ), Diff( EQUAL, " EEE" ) };
    dmp.diff_cleanupSemanticLossless( diffs );
    assertEquals( "diff_cleanupSemanticLossless: Line boundaries.", { Diff( EQUAL, "AAA\r\n" ), Diff( INSERT, "BBB DDD\r\n" ), Diff( EQUAL, "BBB EEE" ) }, diffs );

    diffs = { Diff( EQUAL, "The c" ), Diff( INSERT, "ow and the c" ), Diff( EQUAL, "at." ) };
    dmp.diff_cleanupSemanticLossless( diffs );
    assertEquals( "diff_cleanupSemantic: Word boundaries.", { Diff( EQUAL, "The " ), Diff( INSERT, "cow and the " ), Diff( EQUAL, "cat." ) }, diffs );

    diffs = { Diff( EQUAL, "The-c" ), Diff( INSERT, "ow-and-the-c" ), Diff( EQUAL, "at." ) };
    dmp.diff_cleanupSemanticLossless( diffs );
    assertEquals( "diff_cleanupSemantic: Alphanumeric boundaries.", { Diff( EQUAL, "The-" ), Diff( INSERT, "cow-and-the-" ), Diff( EQUAL, "cat." ) }, diffs );

    diffs = { Diff( EQUAL, "a" ), Diff( DELETE, "a" ), Diff( EQUAL, "ax" ) };
    dmp.diff_cleanupSemanticLossless( diffs );
    assertEquals( "diff_cleanupSemantic: Hitting the start.", { Diff( DELETE, "a" ), Diff( EQUAL, "aax" ) }, diffs );

    diffs = { Diff( EQUAL, "xa" ), Diff( DELETE, "a" ), Diff( EQUAL, "a" ) };
    dmp.diff_cleanupSemanticLossless( diffs );
    assertEquals( "diff_cleanupSemantic: Hitting the end.", { Diff( EQUAL, "xaa" ), Diff( DELETE, "a" ) }, diffs );

    diffs = { Diff( EQUAL, "The xxx. The " ), Diff( INSERT, "zzz. The " ), Diff( EQUAL, "yyy." ) };
    dmp.diff_cleanupSemanticLossless( diffs );
    assertEquals( "diff_cleanupSemantic: Sentence boundaries.", { Diff( EQUAL, "The xxx." ), Diff( INSERT, " The zzz." ), Diff( EQUAL, " The yyy." ) }, diffs );
}

void diff_match_patch_test::testDiffCleanupSemantic()
{
    // Cleanup semantically trivial equalities.
    auto diffs = TDiffVector();
    dmp.diff_cleanupSemantic( diffs );
    assertEquals( "diff_cleanupSemantic: nullptr case.", {}, diffs );

    diffs = { Diff( DELETE, "ab" ), Diff( INSERT, "cd" ), Diff( EQUAL, "12" ), Diff( DELETE, "e" ) };
    dmp.diff_cleanupSemantic( diffs );
    assertEquals( "diff_cleanupSemantic: No elimination #1.", { Diff( DELETE, "ab" ), Diff( INSERT, "cd" ), Diff( EQUAL, "12" ), Diff( DELETE, "e" ) }, diffs );

    diffs = { Diff( DELETE, "abc" ), Diff( INSERT, "ABC" ), Diff( EQUAL, "1234" ), Diff( DELETE, "wxyz" ) };
    dmp.diff_cleanupSemantic( diffs );
    assertEquals( "diff_cleanupSemantic: No elimination #2.", { Diff( DELETE, "abc" ), Diff( INSERT, "ABC" ), Diff( EQUAL, "1234" ), Diff( DELETE, "wxyz" ) }, diffs );

    diffs = { Diff( DELETE, "a" ), Diff( EQUAL, "b" ), Diff( DELETE, "c" ) };
    dmp.diff_cleanupSemantic( diffs );
    assertEquals( "diff_cleanupSemantic: Simple elimination.", { Diff( DELETE, "abc" ), Diff( INSERT, "b" ) }, diffs );

    diffs = { Diff( DELETE, "ab" ), Diff( EQUAL, "cd" ), Diff( DELETE, "e" ), Diff( EQUAL, "f" ), Diff( INSERT, "g" ) };
    dmp.diff_cleanupSemantic( diffs );
    assertEquals( "diff_cleanupSemantic: Backpass elimination.", { Diff( DELETE, "abcdef" ), Diff( INSERT, "cdfg" ) }, diffs );

    diffs = { Diff( INSERT, "1" ), Diff( EQUAL, "a" ), Diff( DELETE, "b" ), Diff( INSERT, "2" ), Diff( EQUAL, "_" ), Diff( INSERT, "1" ), Diff( EQUAL, "a" ), Diff( DELETE, "b" ), Diff( INSERT, "2" ) };
    dmp.diff_cleanupSemantic( diffs );
    assertEquals( "diff_cleanupSemantic: Multiple elimination.", { Diff( DELETE, "AB_AB" ), Diff( INSERT, "1A2_1A2" ) }, diffs );

    diffs = { Diff( EQUAL, "The c" ), Diff( DELETE, "ow and the c" ), Diff( EQUAL, "at." ) };
    dmp.diff_cleanupSemantic( diffs );
    assertEquals( "diff_cleanupSemantic: Word boundaries.", { Diff( EQUAL, "The " ), Diff( DELETE, "cow and the " ), Diff( EQUAL, "cat." ) }, diffs );

    diffs = { Diff( DELETE, "abcxx" ), Diff( INSERT, "xxdef" ) };
    dmp.diff_cleanupSemantic( diffs );
    assertEquals( "diff_cleanupSemantic: No overlap elimination.", { Diff( DELETE, "abcxx" ), Diff( INSERT, "xxdef" ) }, diffs );

    diffs = { Diff( DELETE, "abcxxx" ), Diff( INSERT, "xxxdef" ) };
    dmp.diff_cleanupSemantic( diffs );
    assertEquals( "diff_cleanupSemantic: Overlap elimination.", { Diff( DELETE, "abc" ), Diff( EQUAL, "xxx" ), Diff( INSERT, "def" ) }, diffs );

    diffs = { Diff( DELETE, "xxxabc" ), Diff( INSERT, "defxxx" ) };
    dmp.diff_cleanupSemantic( diffs );
    assertEquals( "diff_cleanupSemantic: Reverse overlap elimination.", { Diff( INSERT, "def" ), Diff( EQUAL, "xxx" ), Diff( DELETE, "abc" ) }, diffs );

    diffs = { Diff( DELETE, "abcd1212" ), Diff( INSERT, "1212efghi" ), Diff( EQUAL, "----" ), Diff( DELETE, "A3" ), Diff( INSERT, "3BC" ) };
    dmp.diff_cleanupSemantic( diffs );
    assertEquals( "diff_cleanupSemantic: Two overlap eliminations.", { Diff( DELETE, "abcd" ), Diff( EQUAL, "1212" ), Diff( INSERT, "efghi" ), Diff( EQUAL, "----" ), Diff( DELETE, "a" ), Diff( EQUAL, "3" ), Diff( INSERT, "BC" ) }, diffs );
}

void diff_match_patch_test::testDiffCleanupEfficiency()
{
    // Cleanup operationally trivial equalities.
    dmp.Diff_EditCost = 4;
    auto diffs = TDiffVector();
    dmp.diff_cleanupEfficiency( diffs );
    assertEquals( "diff_cleanupEfficiency: nullptr case.", {}, diffs );

    diffs = { Diff( DELETE, "ab" ), Diff( INSERT, "12" ), Diff( EQUAL, "wxyz" ), Diff( DELETE, "cd" ), Diff( INSERT, "34" ) };
    dmp.diff_cleanupEfficiency( diffs );
    assertEquals( "diff_cleanupEfficiency: No elimination.", { Diff( DELETE, "ab" ), Diff( INSERT, "12" ), Diff( EQUAL, "wxyz" ), Diff( DELETE, "cd" ), Diff( INSERT, "34" ) }, diffs );

    diffs = { Diff( DELETE, "ab" ), Diff( INSERT, "12" ), Diff( EQUAL, "xyz" ), Diff( DELETE, "cd" ), Diff( INSERT, "34" ) };
    dmp.diff_cleanupEfficiency( diffs );
    assertEquals( "diff_cleanupEfficiency: Four-edit elimination.", { Diff( DELETE, "abxyzcd" ), Diff( INSERT, "12xyz34" ) }, diffs );

    diffs = { Diff( INSERT, "12" ), Diff( EQUAL, "x" ), Diff( DELETE, "cd" ), Diff( INSERT, "34" ) };
    dmp.diff_cleanupEfficiency( diffs );
    assertEquals( "diff_cleanupEfficiency: Three-edit elimination.", { Diff( DELETE, "xcd" ), Diff( INSERT, "12x34" ) }, diffs );

    diffs = { Diff( DELETE, "ab" ), Diff( INSERT, "12" ), Diff( EQUAL, "xy" ), Diff( INSERT, "34" ), Diff( EQUAL, "z" ), Diff( DELETE, "cd" ), Diff( INSERT, "56" ) };
    dmp.diff_cleanupEfficiency( diffs );
    assertEquals( "diff_cleanupEfficiency: Backpass elimination.", { Diff( DELETE, "abxyzcd" ), Diff( INSERT, "12xy34z56" ) }, diffs );

    dmp.Diff_EditCost = 5;
    diffs = { Diff( DELETE, "ab" ), Diff( INSERT, "12" ), Diff( EQUAL, "wxyz" ), Diff( DELETE, "cd" ), Diff( INSERT, "34" ) };
    dmp.diff_cleanupEfficiency( diffs );
    assertEquals( "diff_cleanupEfficiency: High cost elimination.", { Diff( DELETE, "abwxyzcd" ), Diff( INSERT, "12wxyz34" ) }, diffs );
    dmp.Diff_EditCost = 4;
}

void diff_match_patch_test::testDiffPrettyHtml()
{
    // Pretty print.
    auto diffs = TDiffVector( { Diff( EQUAL, "a\n" ), Diff( DELETE, "<B>b</B>" ), Diff( INSERT, "c&d" ) } );
    assertEquals( "diff_prettyHtml:", "<span>a&para;<br></span><del style=\"background:#ffe6e6;\">&lt;B&gt;b&lt;/B&gt;</del><ins style=\"background:#e6ffe6;\">c&amp;d</ins>", dmp.diff_prettyHtml( diffs ) );
}

void diff_match_patch_test::testDiffText()
{
    // Compute the source and destination texts.
    auto diffs = TDiffVector( { Diff( EQUAL, "jump" ), Diff( DELETE, "s" ), Diff( INSERT, "ed" ), Diff( EQUAL, " over " ), Diff( DELETE, "the" ), Diff( INSERT, "a" ), Diff( EQUAL, " lazy" ) } );
    assertEquals( "diff_text1:", "jumps over the lazy", dmp.diff_text1( diffs ) );
    assertEquals( "diff_text2:", "jumped over a lazy", dmp.diff_text2( diffs ) );
}

void diff_match_patch_test::testDiffDelta()
{
    // Convert a diff into delta string.
    auto diffs = TDiffVector( { Diff( EQUAL, "jump" ), Diff( DELETE, "s" ), Diff( INSERT, "ed" ), Diff( EQUAL, " over " ), Diff( DELETE, "the" ), Diff( INSERT, "a" ), Diff( EQUAL, " lazy" ), Diff( INSERT, "old dog" ) } );
    std::wstring text1 = dmp.diff_text1( diffs );
    assertEquals( "diff_text1: Base text.", "jumps over the lazy", text1 );

    std::wstring delta = dmp.diff_toDelta( diffs );
    std::wstring golden = L"=4\t-1\t+ed\t=6\t-3\t+a\t=5\t+old dog";
    assertEquals( "diff_toDelta:", "=4\t-1\t+ed\t=6\t-3\t+a\t=5\t+old dog", delta );

    // Convert delta string into a diff.
    assertEquals( "diff_fromDelta: Normal.", diffs, dmp.diff_fromDelta( text1, delta ) );

    // Generates error (19 < 20).
    bool exceptionTriggered = false;
    try
    {
        dmp.diff_fromDelta( text1 + L"x", delta );
        assertFalse( "diff_fromDelta: Too long.", true );
    }
    catch ( std::wstring ex )
    {
        exceptionTriggered = true;
        // Exception expected.
    }
    assertEquals( "diff_fromDelta: Too long - Exception triggered", true, exceptionTriggered );
    // Generates error (19 > 18).

    exceptionTriggered = false;
    try
    {
        dmp.diff_fromDelta( text1.substr( 1 ), delta );
        assertFalse( "diff_fromDelta: Too short.", true );
    }
    catch ( std::wstring ex )
    {
        exceptionTriggered = true;
        // Exception expected.
    }
    assertEquals( "diff_fromDelta: Too short - Exception triggered", true, exceptionTriggered );
    // Generates error (%c3%xy invalid Unicode).
    // This test does not work because QUrl::fromPercentEncoding("%xy") ->"?"
    exceptionTriggered = false;
    try
    {
        dmp.diff_fromDelta( "", "+%c3%xy" );
        assertFalse( "diff_fromDelta: Invalid character.", true );
    }
    catch ( std::wstring ex )
    {
        exceptionTriggered = true;
        // Exception expected.
    }
    assertEquals( "diff_fromDelta: Invalid character - Exception triggered", true, exceptionTriggered );

    // Test deltas with special characters.
    diffs = { Diff( EQUAL, std::wstring( L"\u0680 " ) + kZero + std::wstring( L" \t %" ) ), Diff( DELETE, std::wstring( L"\u0681 " ) + kOne + std::wstring( L" \n ^" ) ), Diff( INSERT, std::wstring( L"\u0682 " ) + kTwo + std::wstring( L" \\ |" ) ) };

    text1 = dmp.diff_text1( diffs );
    golden = std::wstring( L"\u0680 " ) + kZero + std::wstring( L" \t %\u0681 " ) + kOne + std::wstring( L" \n ^" );
    assertEquals( "diff_text1: Unicode text", golden, text1 );

    delta = dmp.diff_toDelta( diffs );
    assertEquals( "diff_toDelta: Unicode", "=7\t-7\t+%DA%82 %02 %5C %7C", delta );

    assertEquals( "diff_fromDelta: Unicode", diffs, dmp.diff_fromDelta( text1, delta ) );

    // Verify pool of unchanged characters.
    diffs = { Diff( INSERT, "A-Z a-z 0-9 - _ . ! ~ * ' ( ) ; / ? : @ & = + $ , # " ) };
    std::wstring text2 = dmp.diff_text2( diffs );
    assertEquals( "diff_text2: Unchanged characters.", "A-Z a-z 0-9 - _ . ! ~ * \' ( ) ; / ? : @ & = + $ , # ", text2 );

    delta = dmp.diff_toDelta( diffs );
    assertEquals( "diff_toDelta: Unchanged characters.", "+A-Z a-z 0-9 - _ . ! ~ * \' ( ) ; / ? : @ & = + $ , # ", delta );

    // Convert delta string into a diff.
    assertEquals( "diff_fromDelta: Unchanged characters.", diffs, dmp.diff_fromDelta( {}, delta ) );
}

void diff_match_patch_test::testDiffXIndex()
{
    // Translate a location in text1 to text2.
    auto diffs = TDiffVector( { Diff( DELETE, "a" ), Diff( INSERT, "1234" ), Diff( EQUAL, "xyz" ) } );
    assertEquals( "diff_xIndex: Translation on equality.", 5, dmp.diff_xIndex( diffs, 2 ) );

    diffs = { Diff( EQUAL, "a" ), Diff( DELETE, "1234" ), Diff( EQUAL, "xyz" ) };
    assertEquals( "diff_xIndex: Translation on deletion.", 1, dmp.diff_xIndex( diffs, 3 ) );
}

void diff_match_patch_test::testDiffLevenshtein()
{
    auto diffs = TDiffVector( { Diff( DELETE, "abc" ), Diff( INSERT, "1234" ), Diff( EQUAL, "xyz" ) } );
    assertEquals( "diff_levenshtein: Trailing equality.", 4, dmp.diff_levenshtein( diffs ) );

    diffs = { Diff( EQUAL, "xyz" ), Diff( DELETE, "abc" ), Diff( INSERT, "1234" ) };
    assertEquals( "diff_levenshtein: Leading equality.", 4, dmp.diff_levenshtein( diffs ) );

    diffs = { Diff( DELETE, "abc" ), Diff( EQUAL, "xyz" ), Diff( INSERT, "1234" ) };
    assertEquals( "diff_levenshtein: Middle equality.", 7, dmp.diff_levenshtein( diffs ) );
}

void diff_match_patch_test::testDiffBisect()
{
    // Normal.
    std::wstring a = L"cat";
    std::wstring b = L"map";
    // Since the resulting diff hasn't been normalized, it would be ok if
    // the insertion and deletion pairs are swapped.
    // If the order changes, tweak this test as required.
    auto diffs = TDiffVector( { Diff( DELETE, "c" ), Diff( INSERT, "m" ), Diff( EQUAL, "a" ), Diff( DELETE, "t" ), Diff( INSERT, "p" ) } );
    auto results = dmp.diff_bisect( a, b, std::numeric_limits< clock_t >::max() );
    assertEquals( "diff_bisect: Normal.", diffs, results );

    // Timeout.
    diffs = { Diff( DELETE, "cat" ), Diff( INSERT, "map" ) };
    assertEquals( "diff_bisect: Timeout.", diffs, dmp.diff_bisect( a, b, 0 ) );
}

void diff_match_patch_test::testDiffMain()
{
    // Perform a trivial diff.
    auto diffs = TDiffVector();
    assertEquals( "diff_main: nullptr case.", diffs, dmp.diff_main( "", "", false ) );

    diffs = { Diff( DELETE, "abc" ) };
    assertEquals( "diff_main: RHS side nullptr case.", diffs, dmp.diff_main( "abc", "", false ) );

    diffs = { Diff( INSERT, "abc" ) };
    assertEquals( "diff_main: LHS side nullptr case.", diffs, dmp.diff_main( "", "abc", false ) );

    diffs = { Diff( EQUAL, "abc" ) };
    assertEquals( "diff_main: Equality.", diffs, dmp.diff_main( "abc", "abc", false ) );

    diffs = { Diff( EQUAL, "ab" ), Diff( INSERT, "123" ), Diff( EQUAL, "c" ) };
    assertEquals( "diff_main: Simple insertion.", diffs, dmp.diff_main( "abc", "ab123c", false ) );

    diffs = { Diff( EQUAL, "a" ), Diff( DELETE, "123" ), Diff( EQUAL, "bc" ) };
    assertEquals( "diff_main: Simple deletion.", diffs, dmp.diff_main( "a123bc", "abc", false ) );

    diffs = { Diff( EQUAL, "a" ), Diff( INSERT, "123" ), Diff( EQUAL, "b" ), Diff( INSERT, "456" ), Diff( EQUAL, "c" ) };
    assertEquals( "diff_main: Two insertions.", diffs, dmp.diff_main( "abc", "a123b456c", false ) );

    diffs = { Diff( EQUAL, "a" ), Diff( DELETE, "123" ), Diff( EQUAL, "b" ), Diff( DELETE, "456" ), Diff( EQUAL, "c" ) };
    assertEquals( "diff_main: Two deletions.", diffs, dmp.diff_main( "a123b456c", "abc", false ) );

    // Perform a real diff.
    // Switch off the timeout.
    dmp.Diff_Timeout = 0;
    diffs = { Diff( DELETE, "a" ), Diff( INSERT, "b" ) };
    assertEquals( "diff_main: Simple case #1.", diffs, dmp.diff_main( "a", "b", false ) );

    diffs = { Diff( DELETE, "Apple" ), Diff( INSERT, "Banana" ), Diff( EQUAL, "s are a" ), Diff( INSERT, "lso" ), Diff( EQUAL, " fruit." ) };
    assertEquals( "diff_main: Simple case #2.", diffs, dmp.diff_main( "Apples are a fruit.", "Bananas are also fruit.", false ) );

    diffs = { Diff( DELETE, "a" ), Diff( INSERT, L"\u0680" ), Diff( EQUAL, "x" ), Diff( DELETE, "\t" ), Diff( INSERT, to_wstring( kZero ) ) };
    assertEquals( "diff_main: Simple case #3.", diffs, dmp.diff_main( L"ax\t", std::wstring( L"\u0680x" ) + kZero, false ) );

    diffs = { Diff( DELETE, "1" ), Diff( EQUAL, "a" ), Diff( DELETE, "y" ), Diff( EQUAL, "b" ), Diff( DELETE, "2" ), Diff( INSERT, "xab" ) };
    assertEquals( "diff_main: Overlap #1.", diffs, dmp.diff_main( "1ayb2", "abxab", false ) );

    diffs = { Diff( INSERT, "xaxcx" ), Diff( EQUAL, "abc" ), Diff( DELETE, "y" ) };
    assertEquals( "diff_main: Overlap #2.", diffs, dmp.diff_main( "abcy", "xaxcxabc", false ) );

    diffs = { Diff( DELETE, "ABCD" ), Diff( EQUAL, "a" ), Diff( DELETE, "=" ), Diff( INSERT, "-" ), Diff( EQUAL, "bcd" ), Diff( DELETE, "=" ), Diff( INSERT, "-" ), Diff( EQUAL, "efghijklmnopqrs" ), Diff( DELETE, "EFGHIJKLMNOefg" ) };
    assertEquals( "diff_main: Overlap #3.", diffs, dmp.diff_main( "ABCDa=bcd=efghijklmnopqrsEFGHIJKLMNOefg", "a-bcd-efghijklmnopqrs", false ) );

    diffs = { Diff( INSERT, " " ), Diff( EQUAL, "a" ), Diff( INSERT, "nd" ), Diff( EQUAL, " [[Pennsylvania]]" ), Diff( DELETE, " and [[New" ) };
    assertEquals( "diff_main: Large equality.", diffs, dmp.diff_main( "a [[Pennsylvania]] and [[New", " and [[Pennsylvania]]", false ) );

    dmp.Diff_Timeout = 0.1f;   // 100ms
    // This test may 'fail' on extremely fast computers.  If so, just increase the text lengths.
    std::wstring a = L"`Twas brillig, and the slithy toves\nDid gyre and gimble in the wabe:\nAll mimsy were the borogoves,\nAnd the mome raths outgrabe.\n";
    std::wstring b = L"I am the very model of a modern major general,\nI've information vegetable, animal, and mineral,\nI know the kings of England, and I quote the fights historical,\nFrom Marathon to Waterloo, in order categorical.\n";
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
    a = L"1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n";
    b = L"abcdefghij\nabcdefghij\nabcdefghij\nabcdefghij\nabcdefghij\nabcdefghij\nabcdefghij\nabcdefghij\nabcdefghij\nabcdefghij\nabcdefghij\nabcdefghij\nabcdefghij\n";
    assertEquals( "diff_main: Simple line-mode.", dmp.diff_main( a, b, true ), dmp.diff_main( a, b, false ) );

    a = L"1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890";
    b = L"abcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghij";
    assertEquals( "diff_main: Single line-mode.", dmp.diff_main( a, b, true ), dmp.diff_main( a, b, false ) );

    a = L"1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n";
    b = L"abcdefghij\n1234567890\n1234567890\n1234567890\nabcdefghij\n1234567890\n1234567890\n1234567890\nabcdefghij\n1234567890\n1234567890\n1234567890\nabcdefghij\n";
    TStringVector texts_linemode = diff_rebuildtexts( dmp.diff_main( a, b, true ) );
    TStringVector texts_textmode = diff_rebuildtexts( dmp.diff_main( a, b, false ) );
    assertEquals( "diff_main: Overlap line-mode.", texts_textmode, texts_linemode );
}

//  MATCH TEST FUNCTIONS

void diff_match_patch_test::testMatchAlphabet()
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

void diff_match_patch_test::testMatchBitap()
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

void diff_match_patch_test::testMatchMain()
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

void diff_match_patch_test::testPatchObj()
{
    // Patch Object.
    Patch p;
    p.start1 = 20;
    p.start2 = 21;
    p.length1 = 18;
    p.length2 = 17;
    p.diffs = { Diff( EQUAL, "jump" ), Diff( DELETE, "s" ), Diff( INSERT, "ed" ), Diff( EQUAL, " over " ), Diff( DELETE, "the" ), Diff( INSERT, "a" ), Diff( EQUAL, "\nlaz" ) };
    std::wstring strp = L"@@ -21,18 +22,17 @@\n jump\n-s\n+ed\n  over \n-the\n+a\n %0Alaz\n";
    assertEquals( "patch: toString.", strp, p.toString() );
}

void diff_match_patch_test::testPatchFromText()
{
    assertTrue( "patch_fromText: #0.", dmp.patch_fromText( "" ).empty() );

    std::wstring strp = L"@@ -21,18 +22,17 @@\n jump\n-s\n+ed\n  over \n-the\n+a\n %0Alaz\n";
    assertEquals( "patch_fromText: #1.", strp, dmp.patch_fromText( strp )[ 0 ].toString() );

    assertEquals( "patch_fromText: #2.", "@@ -1 +1 @@\n-a\n+b\n", dmp.patch_fromText( "@@ -1 +1 @@\n-a\n+b\n" )[ 0 ].toString() );

    assertEquals( "patch_fromText: #3.", "@@ -1,3 +0,0 @@\n-abc\n", dmp.patch_fromText( "@@ -1,3 +0,0 @@\n-abc\n" )[ 0 ].toString() );

    assertEquals( "patch_fromText: #4.", "@@ -0,0 +1,3 @@\n+abc\n", dmp.patch_fromText( "@@ -0,0 +1,3 @@\n+abc\n" )[ 0 ].toString() );

    // Generates error.
    bool exceptionTriggered = false;
    try
    {
        dmp.patch_fromText( "Bad\nPatch\n" );
        assertFalse( "patch_fromText: #5.", true );
    }
    catch ( std::wstring ex )
    {
        exceptionTriggered = true;
        // Exception expected.
    }
    assertEquals( "patch_fromText: #5 - Exception triggered", true, exceptionTriggered );
}

void diff_match_patch_test::testPatchToText()
{
    std::wstring strp = L"@@ -21,18 +22,17 @@\n jump\n-s\n+ed\n  over \n-the\n+a\n  laz\n";
    auto patches = dmp.patch_fromText( strp );
    assertEquals( "patch_toText: Single", strp, dmp.patch_toText( patches ) );

    strp = L"@@ -1,9 +1,9 @@\n-f\n+F\n oo+fooba\n@@ -7,9 +7,9 @@\n obar\n-,\n+.\n  tes\n";
    patches = dmp.patch_fromText( strp );
    assertEquals( "patch_toText: Dua", strp, dmp.patch_toText( patches ) );
}

void diff_match_patch_test::testPatchAddContext()
{
    dmp.Patch_Margin = 4;
    auto p = dmp.patch_fromText( "@@ -21,4 +21,10 @@\n-jump\n+somersault\n" )[ 0 ];
    dmp.patch_addContext( p, "The quick brown fox jumps over the lazy dog." );
    assertEquals( "patch_addContext: Simple case.", "@@ -17,12 +17,18 @@\n fox \n-jump\n+somersault\n s ov\n", p.toString() );

    p = dmp.patch_fromText( "@@ -21,4 +21,10 @@\n-jump\n+somersault\n" )[ 0 ];
    dmp.patch_addContext( p, "The quick brown fox jumps." );
    assertEquals( "patch_addContext: Not enough trailing context.", "@@ -17,10 +17,16 @@\n fox \n-jump\n+somersault\n s.\n", p.toString() );

    p = dmp.patch_fromText( "@@ -3 +3,2 @@\n-e\n+at\n" )[ 0 ];
    dmp.patch_addContext( p, "The quick brown fox jumps." );
    assertEquals( "patch_addContext: Not enough leading context.", "@@ -1,7 +1,8 @@\n Th\n-e\n+at\n  qui\n", p.toString() );

    p = dmp.patch_fromText( "@@ -3 +3,2 @@\n-e\n+at\n" )[ 0 ];
    dmp.patch_addContext( p, "The quick brown fox jumps.  The quick brown fox crashes." );
    assertEquals( "patch_addContext: Ambiguity.", "@@ -1,27 +1,28 @@\n Th\n-e\n+at\n  quick brown fox jumps. \n", p.toString() );
}

void diff_match_patch_test::testPatchMake()
{
    TPatchVector patches;
    patches = dmp.patch_make( "", "" );
    assertEquals( "patch_make: nullptr case", "", dmp.patch_toText( patches ) );

    std::wstring text1 = L"The quick brown fox jumps over the lazy dog.";
    std::wstring text2 = L"That quick brown fox jumped over a lazy dog.";
    std::wstring expectedPatch = L"@@ -1,8 +1,7 @@\n Th\n-at\n+e\n  qui\n@@ -21,17 +21,18 @@\n jump\n-ed\n+s\n  over \n-a\n+the\n  laz\n";
    // The second patch must be "-21,17 +21,18", not "-22,17 +21,18" due to rolling context.
    patches = dmp.patch_make( text2, text1 );
    assertEquals( "patch_make: Text2+Text1 inputs", expectedPatch, dmp.patch_toText( patches ) );

    expectedPatch = L"@@ -1,11 +1,12 @@\n Th\n-e\n+at\n  quick b\n@@ -22,18 +22,17 @@\n jump\n-s\n+ed\n  over \n-the\n+a\n  laz\n";
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
    assertEquals( "patch_toText: Character encoding.", "@@ -1,21 +1,21 @@\n-%601234567890-=%5B%5D%5C;',./\n+~!@#$%25%5E&*()_+%7B%7D%7C:%22%3C%3E?\n", dmp.patch_toText( patches ) );

    diffs = { Diff( DELETE, "`1234567890-=[]\\;',./" ), Diff( INSERT, "~!@#$%^&*()_+{}|:\"<>?" ) };
    assertEquals( "patch_fromText: Character decoding.", diffs, dmp.patch_fromText( "@@ -1,21 +1,21 @@\n-%601234567890-=%5B%5D%5C;',./\n+~!@#$%25%5E&*()_+%7B%7D%7C:%22%3C%3E?\n" )[ 0 ].diffs );

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

void diff_match_patch_test::testPatchSplitMax()
{
    // Confirm Match_MaxBits is 32.
    TPatchVector patches;
    patches = dmp.patch_make( "abcdefghijklmnopqrstuvwxyz01234567890", "XabXcdXefXghXijXklXmnXopXqrXstXuvXwxXyzX01X23X45X67X89X0" );
    dmp.patch_splitMax( patches );
    assertEquals( "patch_splitMax: #1.", "@@ -1,32 +1,46 @@\n+X\n ab\n+X\n cd\n+X\n ef\n+X\n gh\n+X\n ij\n+X\n kl\n+X\n mn\n+X\n op\n+X\n qr\n+X\n st\n+X\n uv\n+X\n wx\n+X\n yz\n+X\n 012345\n@@ -25,13 +39,18 @@\n zX01\n+X\n 23\n+X\n 45\n+X\n 67\n+X\n 89\n+X\n 0\n", dmp.patch_toText( patches ) );

    patches = dmp.patch_make( "abcdef1234567890123456789012345678901234567890123456789012345678901234567890uvwxyz", "abcdefuvwxyz" );
    std::wstring oldToText = dmp.patch_toText( patches );
    dmp.patch_splitMax( patches );
    assertEquals( "patch_splitMax: #2.", oldToText, dmp.patch_toText( patches ) );

    patches = dmp.patch_make( "1234567890123456789012345678901234567890123456789012345678901234567890", "abc" );
    dmp.patch_splitMax( patches );
    assertEquals( "patch_splitMax: #3.", "@@ -1,32 +1,4 @@\n-1234567890123456789012345678\n 9012\n@@ -29,32 +1,4 @@\n-9012345678901234567890123456\n 7890\n@@ -57,14 +1,3 @@\n-78901234567890\n+abc\n", dmp.patch_toText( patches ) );

    patches = dmp.patch_make( "abcdefghij , h : 0 , t : 1 abcdefghij , h : 0 , t : 1 abcdefghij , h : 0 , t : 1", "abcdefghij , h : 1 , t : 1 abcdefghij , h : 1 , t : 1 abcdefghij , h : 0 , t : 1" );
    dmp.patch_splitMax( patches );
    assertEquals( "patch_splitMax: #4.", "@@ -2,32 +2,32 @@\n bcdefghij , h : \n-0\n+1\n  , t : 1 abcdef\n@@ -29,32 +29,32 @@\n bcdefghij , h : \n-0\n+1\n  , t : 1 abcdef\n", dmp.patch_toText( patches ) );
}

void diff_match_patch_test::testPatchAddPadding()
{
    TPatchVector patches;
    patches = dmp.patch_make( "", "test" );
    assertEquals( "patch_addPadding: Both edges ful", "@@ -0,0 +1,4 @@\n+test\n", dmp.patch_toText( patches ) );
    dmp.patch_addPadding( patches );
    assertEquals( "patch_addPadding: Both edges full.", "@@ -1,8 +1,12 @@\n %01%02%03%04\n+test\n %01%02%03%04\n", dmp.patch_toText( patches ) );

    patches = dmp.patch_make( "XY", "XtestY" );
    assertEquals( "patch_addPadding: Both edges partial.", "@@ -1,2 +1,6 @@\n X\n+test\n Y\n", dmp.patch_toText( patches ) );
    dmp.patch_addPadding( patches );
    assertEquals( "patch_addPadding: Both edges partial.", "@@ -2,8 +2,12 @@\n %02%03%04X\n+test\n Y%01%02%03\n", dmp.patch_toText( patches ) );

    patches = dmp.patch_make( "XXXXYYYY", "XXXXtestYYYY" );
    assertEquals( "patch_addPadding: Both edges none.", "@@ -1,8 +1,12 @@\n XXXX\n+test\n YYYY\n", dmp.patch_toText( patches ) );
    dmp.patch_addPadding( patches );
    assertEquals( "patch_addPadding: Both edges none.", "@@ -5,8 +5,12 @@\n XXXX\n+test\n YYYY\n", dmp.patch_toText( patches ) );
}

void diff_match_patch_test::testPatchApply()
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
    assertEquals( "patch_apply: Exact match.", "@@ -1,11 +1,12 @@\n Th\n-e\n+at\n  quick b\n@@ -22,18 +22,17 @@\n jump\n-s\n+ed\n  over \n-the\n+a\n  laz\n", dmp.patch_toText( patches ) );

    results = dmp.patch_apply( patches, "The quick brown fox jumps over the lazy dog." );
    boolArray = results.second;
    resultStr = results.first + to_wstring( boolArray );
    
    assertEquals( "patch_apply: Exact match.", "That quick brown fox jumped over a lazy dog.\ttrue\ttrue", resultStr );

    results = dmp.patch_apply( patches, "The quick red rabbit jumps over the tired tiger." );
    boolArray = results.second;
    resultStr = results.first + to_wstring( boolArray );
    assertEquals( "patch_apply: Partial match.", "That quick red rabbit jumped over a tired tiger.\ttrue\ttrue", resultStr );

    results = dmp.patch_apply( patches, "I am the very model of a modern major general." );
    boolArray = results.second;
    resultStr = results.first + to_wstring( boolArray );
    assertEquals( "patch_apply: Failed match.", "I am the very model of a modern major general.\tfalse\tfalse", resultStr );

    patches = dmp.patch_make( "x1234567890123456789012345678901234567890123456789012345678901234567890y", "xabcy" );
    results = dmp.patch_apply( patches, "x123456789012345678901234567890-----++++++++++-----123456789012345678901234567890y" );
    boolArray = results.second;
    resultStr = results.first + to_wstring( boolArray );
    assertEquals( "patch_apply: Big delete, small change.", "xabcy\ttrue\ttrue", resultStr );

    patches = dmp.patch_make( "x1234567890123456789012345678901234567890123456789012345678901234567890y", "xabcy" );
    results = dmp.patch_apply( patches, "x12345678901234567890---------------++++++++++---------------12345678901234567890y" );
    boolArray = results.second;
    resultStr = results.first + to_wstring( boolArray );
    assertEquals( "patch_apply: Big delete, large change 1.", "xabc12345678901234567890---------------++++++++++---------------12345678901234567890y\tfalse\ttrue", resultStr );

    dmp.Patch_DeleteThreshold = 0.6f;
    patches = dmp.patch_make( "x1234567890123456789012345678901234567890123456789012345678901234567890y", "xabcy" );
    results = dmp.patch_apply( patches, "x12345678901234567890---------------++++++++++---------------12345678901234567890y" );
    boolArray = results.second;
    resultStr = results.first + to_wstring( boolArray );
    assertEquals( "patch_apply: Big delete, large change 2.", "xabcy\ttrue\ttrue", resultStr );
    dmp.Patch_DeleteThreshold = 0.5f;

    dmp.Match_Threshold = 0.0f;
    dmp.Match_Distance = 0;
    patches = dmp.patch_make( "abcdefghijklmnopqrstuvwxyz--------------------1234567890", "abcXXXXXXXXXXdefghijklmnopqrstuvwxyz--------------------1234567YYYYYYYYYY890" );
    results = dmp.patch_apply( patches, "ABCDEFGHIJKLMNOPQRSTUVWXYZ--------------------1234567890" );
    boolArray = results.second;
    resultStr = results.first + to_wstring( boolArray );
    assertEquals( "patch_apply: Compensate for failed patch.", "ABCDEFGHIJKLMNOPQRSTUVWXYZ--------------------1234567YYYYYYYYYY890\tfalse\ttrue", resultStr );
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
    resultStr = results.first + L"\t" + to_wstring( boolArray[ 0 ], false );
    assertEquals( "patch_apply: Edge exact match.", "test\ttrue", resultStr );

    patches = dmp.patch_make( "XY", "XtestY" );
    results = dmp.patch_apply( patches, "XY" );
    boolArray = results.second;
    resultStr = results.first + L"\t" + to_wstring( boolArray[ 0 ], false );
    assertEquals( "patch_apply: Near edge exact match.", "XtestY\ttrue", resultStr );

    patches = dmp.patch_make( "y", "y123" );
    results = dmp.patch_apply( patches, "x" );
    boolArray = results.second;
    resultStr = results.first + L"\t" + to_wstring( boolArray[ 0 ] );
    assertEquals( "patch_apply: Edge partial match.", "x123\ttrue", resultStr );
}

void diff_match_patch_test::reportFailure( const std::string &strCase, const std::wstring &expected, const std::wstring &actual )
{
    std::cout << "FAILED : " + strCase + "\n";
    std::wcerr << "    Expected: " << expected << "\n      Actual: " << actual << "\n";
    numFailedTests++;
    //throw strCase;
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
        reportFailure( strCase, to_wstring( var1 ), to_wstring( var2 ) );
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
            reportFailure( strCase, L"(" + to_wstring( ii.first ) + L"," + std::to_wstring( ii.second ) + L")", L"<NOT FOUND>" );
        }
    }

    for ( auto &&ii : m2 )
    {
        auto rhs = m1.find( ii.first );
        if ( rhs == m1.end() )
        {
            reportFailure( strCase, L"(" + to_wstring( ii.first ) + L"," + std::to_wstring( ii.second ) + L")", L"<NOT FOUND>" );
        }
    }

    reportPassed( strCase );
}

void diff_match_patch_test::assertEquals( const std::string &strCase, bool lhs, bool rhs )
{
    if ( lhs != rhs )
    {
        reportFailure( strCase, to_wstring( lhs, false ), to_wstring( rhs, false ) );
    }
    reportPassed( strCase );
}

void diff_match_patch_test::assertTrue( const std::string &strCase, bool value )
{
    if ( !value )
    {
        reportFailure( strCase, to_wstring( true, false ), to_wstring( false, false ) );
    }
    reportPassed( strCase );
}

void diff_match_patch_test::assertFalse( const std::string &strCase, bool value )
{
    if ( value )
    {
        reportFailure( strCase, to_wstring( false, false ), to_wstring( true, false ) );
    }
    reportPassed( strCase );
}

// Construct the two texts which made up the diff originally.
diff_match_patch_test::TStringVector diff_match_patch_test::diff_rebuildtexts( const TDiffVector &diffs )
{
    TStringVector text( 2, std::wstring() );
    for ( auto &&myDiff : diffs )
    {
        if ( myDiff.operation != INSERT )
        {
            text[ 0 ] += myDiff.text;
        }
        if ( myDiff.operation != DELETE )
        {
            text[ 1 ] += myDiff.text;
        }
    }
    return text;
}

void diff_match_patch_test::assertEmpty( const std::string &strCase, const TStringVector &list )
{
    if ( !list.empty() )
    {
        throw strCase;
    }
}

/*
Compile instructions for cmake on Windows:
mkdir build
cd build
cmake ..
make
diff_match_patch_test.exe

Compile insructions for OS X:
qmake -spec macx-g++
make
./diff_match_patch
*/
