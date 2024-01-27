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
    assertEquals( "diff_commonPrefix: nullptr case.", 0, dmp.diff_commonPrefix( L"abc", L"xyz" ) );

    assertEquals( "diff_commonPrefix: Non-nullptr case.", 4, dmp.diff_commonPrefix( L"1234abcdef", L"1234xyz" ) );

    assertEquals( "diff_commonPrefix: Whole case.", 4, dmp.diff_commonPrefix( L"1234", L"1234xyz" ) );
}

void diff_match_patch_test::testDiffCommonSuffix()
{
    // Detect any common suffix.
    assertEquals( "diff_commonSuffix: nullptr case.", 0, dmp.diff_commonSuffix( L"abc", L"xyz" ) );

    assertEquals( "diff_commonSuffix: Non-nullptr case.", 4, dmp.diff_commonSuffix( L"abcdef1234", L"xyz1234" ) );

    assertEquals( "diff_commonSuffix: Whole case.", 4, dmp.diff_commonSuffix( L"1234", L"xyz1234" ) );
}

void diff_match_patch_test::testDiffCommonOverlap()
{
    // Detect any suffix/prefix overlap.
    assertEquals( "diff_commonOverlap: nullptr case.", 0, dmp.diff_commonOverlap( L"", L"abcd" ) );

    assertEquals( "diff_commonOverlap: Whole case.", 3, dmp.diff_commonOverlap( L"abc", L"abcd" ) );

    assertEquals( "diff_commonOverlap: No overlap.", 0, dmp.diff_commonOverlap( L"123456", L"abcd" ) );

    assertEquals( "diff_commonOverlap: Overlap.", 3, dmp.diff_commonOverlap( L"123456xxx", L"xxxabcd" ) );

    // Some overly clever languages (C#) may treat ligatures as equal to their
    // component letters.  E.g. U+FB01 == 'fi'
    assertEquals( "diff_commonOverlap: Unicode.", 0, dmp.diff_commonOverlap( L"fi", std::wstring( L"\ufb01i" ) ) );
}

void diff_match_patch_test::testDiffHalfmatch()
{
    // Detect a halfmatch.
    dmp.Diff_Timeout = 1;
    assertEmpty( "diff_halfMatch: No match #1.", dmp.diff_halfMatch( L"1234567890", L"abcdef" ) );

    assertEmpty( "diff_halfMatch: No match #2.", dmp.diff_halfMatch( L"12345", L"23" ) );

    assertEquals( "diff_halfMatch: Single Match #1.", { L"12", L"90", L"a", L"z", L"345678" }, dmp.diff_halfMatch( L"1234567890", L"a345678z" ) );

    assertEquals( "diff_halfMatch: Single Match #2.", { L"a", L"z", L"12", L"90", L"345678" }, dmp.diff_halfMatch( L"a345678z", L"1234567890" ) );

    assertEquals( "diff_halfMatch: Single Match #3.", { L"abc", L"z", L"1234", L"0", L"56789" }, dmp.diff_halfMatch( L"abc56789z", L"1234567890" ) );

    assertEquals( "diff_halfMatch: Single Match #4.", { L"a", L"xyz", L"1", L"7890", L"23456" }, dmp.diff_halfMatch( L"a23456xyz", L"1234567890" ) );

    assertEquals( "diff_halfMatch: Multiple Matches #1.", { L"12123", L"123121", L"a", L"z", L"1234123451234" }, dmp.diff_halfMatch( L"121231234123451234123121", L"a1234123451234z" ) );

    assertEquals( "diff_halfMatch: Multiple Matches #2.", { L"", L"-=-=-=-=-=", L"x", L"", L"x-=-=-=-=-=-=-=" }, dmp.diff_halfMatch( L"x-=-=-=-=-=-=-=-=-=-=-=-=", L"xx-=-=-=-=-=-=-=" ) );

    assertEquals( "diff_halfMatch: Multiple Matches #3.", { L"-=-=-=-=-=", L"", L"", L"y", L"-=-=-=-=-=-=-=y" }, dmp.diff_halfMatch( L"-=-=-=-=-=-=-=-=-=-=-=-=y", L"-=-=-=-=-=-=-=yy" ) );

    // Optimal diff would be -q+x=H-i+e=lloHe+Hu=llo-Hew+y not -qHillo+x=HelloHe-w+Hulloy
    assertEquals( "diff_halfMatch: Non-optimal halfmatch.", { L"qHillo", L"w", L"x", L"Hulloy", L"HelloHe" }, dmp.diff_halfMatch( L"qHilloHelloHew", L"xHelloHeHulloy" ) );

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
    assertEquals( "diff_linesToChars:", tmpVarList, dmp.diff_linesToChars( L"alpha\nbeta\nalpha\n", L"beta\nalpha\nbeta\n" ) );

    tmpVector.clear();
    tmpVarList.clear();
    tmpVector.emplace_back( L"" );
    tmpVector.emplace_back( L"alpha\r\n" );
    tmpVector.emplace_back( L"beta\r\n" );
    tmpVector.emplace_back( L"\r\n" );
    tmpVarList.emplace_back( std::wstring() );
    tmpVarList.emplace_back( to_wstring( { 1, 2, 3, 3 } ) );   // (("\u0001\u0002\u0003\u0003"));
    tmpVarList.emplace_back( tmpVector );
    assertEquals( "diff_linesToChars:", tmpVarList, dmp.diff_linesToChars( L"", L"alpha\r\nbeta\r\n\r\n\r\n" ) );

    tmpVector.clear();
    tmpVarList.clear();
    tmpVector.emplace_back( L"" );
    tmpVector.emplace_back( L"a" );
    tmpVector.emplace_back( L"b" );
    tmpVarList.emplace_back( to_wstring( 1 ) );   // (("\u0001"));
    tmpVarList.emplace_back( to_wstring( 2 ) );   // (("\u0002"));
    tmpVarList.emplace_back( tmpVector );
    assertEquals( "diff_linesToChars:", tmpVarList, dmp.diff_linesToChars( L"a", L"b" ) );

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
    assertEquals( "diff_linesToChars: More than 256.", tmpVarList, dmp.diff_linesToChars( lines, L"" ) );
}

void diff_match_patch_test::testDiffCharsToLines()
{
    // First check that Diff equality works.
    assertTrue( "diff_charsToLines:", Diff( EQUAL, L"a" ) == Diff( EQUAL, L"a" ) );

    assertEquals( "diff_charsToLines:", Diff( EQUAL, L"a" ), Diff( EQUAL, L"a" ) );

    // Convert chars up to lines.
    TDiffVector diffs;
    diffs.emplace_back( EQUAL, to_wstring( { 1, 2, 1 } ) );   // ("\u0001\u0002\u0001");
    diffs.emplace_back( INSERT, to_wstring( { 2, 1, 2 } ) );   // ("\u0002\u0001\u0002");
    TStringVector tmpVector;
    tmpVector.emplace_back( L"" );
    tmpVector.emplace_back( L"alpha\n" );
    tmpVector.emplace_back( L"beta\n" );
    dmp.diff_charsToLines( diffs, tmpVector );
    assertEquals( "diff_charsToLines:", { Diff( EQUAL, L"alpha\nbeta\nalpha\n" ), Diff( INSERT, L"beta\nalpha\nbeta\n" ) }, diffs );

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

    diffs = { Diff( EQUAL, L"a" ), Diff( DELETE, L"b" ), Diff( INSERT, L"c" ) };
    dmp.diff_cleanupMerge( diffs );
    assertEquals( "diff_cleanupMerge: No change case.", { Diff( EQUAL, L"a" ), Diff( DELETE, L"b" ), Diff( INSERT, L"c" ) }, diffs );

    diffs = { Diff( EQUAL, L"a" ), Diff( EQUAL, L"b" ), Diff( EQUAL, L"c" ) };
    dmp.diff_cleanupMerge( diffs );
    assertEquals( "diff_cleanupMerge: Merge equalities.", { Diff( EQUAL, L"abc" ) }, diffs );

    diffs = { Diff( DELETE, L"a" ), Diff( DELETE, L"b" ), Diff( DELETE, L"c" ) };
    dmp.diff_cleanupMerge( diffs );
    assertEquals( "diff_cleanupMerge: Merge deletions.", { Diff( DELETE, L"abc" ) }, diffs );

    diffs = { Diff( INSERT, L"a" ), Diff( INSERT, L"b" ), Diff( INSERT, L"c" ) };
    dmp.diff_cleanupMerge( diffs );
    assertEquals( "diff_cleanupMerge: Merge insertions.", { Diff( INSERT, L"abc" ) }, diffs );

    diffs = { Diff( DELETE, L"a" ), Diff( INSERT, L"b" ), Diff( DELETE, L"c" ), Diff( INSERT, L"d" ), Diff( EQUAL, L"e" ), Diff( EQUAL, L"f" ) };
    dmp.diff_cleanupMerge( diffs );
    assertEquals( "diff_cleanupMerge: Merge interweave.", { Diff( DELETE, L"ac" ), Diff( INSERT, L"bd" ), Diff( EQUAL, L"ef" ) }, diffs );

    diffs = { Diff( DELETE, L"a" ), Diff( INSERT, L"abc" ), Diff( DELETE, L"dc" ) };
    dmp.diff_cleanupMerge( diffs );
    assertEquals( "diff_cleanupMerge: Prefix and suffix detection.", { Diff( EQUAL, L"a" ), Diff( DELETE, L"d" ), Diff( INSERT, L"b" ), Diff( EQUAL, L"c" ) }, diffs );

    diffs = { Diff( EQUAL, L"x" ), Diff( DELETE, L"a" ), Diff( INSERT, L"abc" ), Diff( DELETE, L"dc" ), Diff( EQUAL, L"y" ) };
    dmp.diff_cleanupMerge( diffs );
    assertEquals( "diff_cleanupMerge: Prefix and suffix detection with equalities.", { Diff( EQUAL, L"xa" ), Diff( DELETE, L"d" ), Diff( INSERT, L"b" ), Diff( EQUAL, L"cy" ) }, diffs );

    diffs = { Diff( EQUAL, L"a" ), Diff( INSERT, L"ba" ), Diff( EQUAL, L"c" ) };
    dmp.diff_cleanupMerge( diffs );
    assertEquals( "diff_cleanupMerge: Slide edit left.", { Diff( INSERT, L"ab" ), Diff( EQUAL, L"ac" ) }, diffs );

    diffs = { Diff( EQUAL, L"c" ), Diff( INSERT, L"ab" ), Diff( EQUAL, L"a" ) };
    dmp.diff_cleanupMerge( diffs );
    assertEquals( "diff_cleanupMerge: Slide edit right.", { Diff( EQUAL, L"ca" ), Diff( INSERT, L"ba" ) }, diffs );

    diffs = { Diff( EQUAL, L"a" ), Diff( DELETE, L"b" ), Diff( EQUAL, L"c" ), Diff( DELETE, L"ac" ), Diff( EQUAL, L"x" ) };
    dmp.diff_cleanupMerge( diffs );
    assertEquals( "diff_cleanupMerge: Slide edit left recursive.", { Diff( DELETE, L"abc" ), Diff( EQUAL, L"acx" ) }, diffs );

    diffs = { Diff( EQUAL, L"x" ), Diff( DELETE, L"ca" ), Diff( EQUAL, L"c" ), Diff( DELETE, L"b" ), Diff( EQUAL, L"a" ) };
    dmp.diff_cleanupMerge( diffs );
    assertEquals( "diff_cleanupMerge: Slide edit right recursive.", { Diff( EQUAL, L"xca" ), Diff( DELETE, L"cba" ) }, diffs );
}

void diff_match_patch_test::testDiffCleanupSemanticLossless()
{
    // Slide diffs to match logical boundaries.
    auto diffs = TDiffVector();
    dmp.diff_cleanupSemanticLossless( diffs );
    assertEquals( "diff_cleanupSemantic: nullptr case.", {}, diffs );

    diffs = { Diff( EQUAL, L"AAA\r\n\r\nBBB" ), Diff( INSERT, L"\r\nDDD\r\n\r\nBBB" ), Diff( EQUAL, L"\r\nEEE" ) };
    dmp.diff_cleanupSemanticLossless( diffs );
    assertEquals( "diff_cleanupSemanticLossless: Blank lines.", { Diff( EQUAL, L"AAA\r\n\r\n" ), Diff( INSERT, L"BBB\r\nDDD\r\n\r\n" ), Diff( EQUAL, L"BBB\r\nEEE" ) }, diffs );

    diffs = { Diff( EQUAL, L"AAA\r\nBBB" ), Diff( INSERT, L" DDD\r\nBBB" ), Diff( EQUAL, L" EEE" ) };
    dmp.diff_cleanupSemanticLossless( diffs );
    assertEquals( "diff_cleanupSemanticLossless: Line boundaries.", { Diff( EQUAL, L"AAA\r\n" ), Diff( INSERT, L"BBB DDD\r\n" ), Diff( EQUAL, L"BBB EEE" ) }, diffs );

    diffs = { Diff( EQUAL, L"The c" ), Diff( INSERT, L"ow and the c" ), Diff( EQUAL, L"at." ) };
    dmp.diff_cleanupSemanticLossless( diffs );
    assertEquals( "diff_cleanupSemantic: Word boundaries.", { Diff( EQUAL, L"The " ), Diff( INSERT, L"cow and the " ), Diff( EQUAL, L"cat." ) }, diffs );

    diffs = { Diff( EQUAL, L"The-c" ), Diff( INSERT, L"ow-and-the-c" ), Diff( EQUAL, L"at." ) };
    dmp.diff_cleanupSemanticLossless( diffs );
    assertEquals( "diff_cleanupSemantic: Alphanumeric boundaries.", { Diff( EQUAL, L"The-" ), Diff( INSERT, L"cow-and-the-" ), Diff( EQUAL, L"cat." ) }, diffs );

    diffs = { Diff( EQUAL, L"a" ), Diff( DELETE, L"a" ), Diff( EQUAL, L"ax" ) };
    dmp.diff_cleanupSemanticLossless( diffs );
    assertEquals( "diff_cleanupSemantic: Hitting the start.", { Diff( DELETE, L"a" ), Diff( EQUAL, L"aax" ) }, diffs );

    diffs = { Diff( EQUAL, L"xa" ), Diff( DELETE, L"a" ), Diff( EQUAL, L"a" ) };
    dmp.diff_cleanupSemanticLossless( diffs );
    assertEquals( "diff_cleanupSemantic: Hitting the end.", { Diff( EQUAL, L"xaa" ), Diff( DELETE, L"a" ) }, diffs );

    diffs = { Diff( EQUAL, L"The xxx. The " ), Diff( INSERT, L"zzz. The " ), Diff( EQUAL, L"yyy." ) };
    dmp.diff_cleanupSemanticLossless( diffs );
    assertEquals( "diff_cleanupSemantic: Sentence boundaries.", { Diff( EQUAL, L"The xxx." ), Diff( INSERT, L" The zzz." ), Diff( EQUAL, L" The yyy." ) }, diffs );
}

void diff_match_patch_test::testDiffCleanupSemantic()
{
    // Cleanup semantically trivial equalities.
    auto diffs = TDiffVector();
    dmp.diff_cleanupSemantic( diffs );
    assertEquals( "diff_cleanupSemantic: nullptr case.", {}, diffs );

    diffs = { Diff( DELETE, L"ab" ), Diff( INSERT, L"cd" ), Diff( EQUAL, L"12" ), Diff( DELETE, L"e" ) };
    dmp.diff_cleanupSemantic( diffs );
    assertEquals( "diff_cleanupSemantic: No elimination #1.", { Diff( DELETE, L"ab" ), Diff( INSERT, L"cd" ), Diff( EQUAL, L"12" ), Diff( DELETE, L"e" ) }, diffs );

    diffs = { Diff( DELETE, L"abc" ), Diff( INSERT, L"ABC" ), Diff( EQUAL, L"1234" ), Diff( DELETE, L"wxyz" ) };
    dmp.diff_cleanupSemantic( diffs );
    assertEquals( "diff_cleanupSemantic: No elimination #2.", { Diff( DELETE, L"abc" ), Diff( INSERT, L"ABC" ), Diff( EQUAL, L"1234" ), Diff( DELETE, L"wxyz" ) }, diffs );

    diffs = { Diff( DELETE, L"a" ), Diff( EQUAL, L"b" ), Diff( DELETE, L"c" ) };
    dmp.diff_cleanupSemantic( diffs );
    assertEquals( "diff_cleanupSemantic: Simple elimination.", { Diff( DELETE, L"abc" ), Diff( INSERT, L"b" ) }, diffs );

    diffs = { Diff( DELETE, L"ab" ), Diff( EQUAL, L"cd" ), Diff( DELETE, L"e" ), Diff( EQUAL, L"f" ), Diff( INSERT, L"g" ) };
    dmp.diff_cleanupSemantic( diffs );
    assertEquals( "diff_cleanupSemantic: Backpass elimination.", { Diff( DELETE, L"abcdef" ), Diff( INSERT, L"cdfg" ) }, diffs );

    diffs = { Diff( INSERT, L"1" ), Diff( EQUAL, L"a" ), Diff( DELETE, L"b" ), Diff( INSERT, L"2" ), Diff( EQUAL, L"_" ), Diff( INSERT, L"1" ), Diff( EQUAL, L"a" ), Diff( DELETE, L"b" ), Diff( INSERT, L"2" ) };
    dmp.diff_cleanupSemantic( diffs );
    assertEquals( "diff_cleanupSemantic: Multiple elimination.", { Diff( DELETE, L"AB_AB" ), Diff( INSERT, L"1A2_1A2" ) }, diffs );

    diffs = { Diff( EQUAL, L"The c" ), Diff( DELETE, L"ow and the c" ), Diff( EQUAL, L"at." ) };
    dmp.diff_cleanupSemantic( diffs );
    assertEquals( "diff_cleanupSemantic: Word boundaries.", { Diff( EQUAL, L"The " ), Diff( DELETE, L"cow and the " ), Diff( EQUAL, L"cat." ) }, diffs );

    diffs = { Diff( DELETE, L"abcxx" ), Diff( INSERT, L"xxdef" ) };
    dmp.diff_cleanupSemantic( diffs );
    assertEquals( "diff_cleanupSemantic: No overlap elimination.", { Diff( DELETE, L"abcxx" ), Diff( INSERT, L"xxdef" ) }, diffs );

    diffs = { Diff( DELETE, L"abcxxx" ), Diff( INSERT, L"xxxdef" ) };
    dmp.diff_cleanupSemantic( diffs );
    assertEquals( "diff_cleanupSemantic: Overlap elimination.", { Diff( DELETE, L"abc" ), Diff( EQUAL, L"xxx" ), Diff( INSERT, L"def" ) }, diffs );

    diffs = { Diff( DELETE, L"xxxabc" ), Diff( INSERT, L"defxxx" ) };
    dmp.diff_cleanupSemantic( diffs );
    assertEquals( "diff_cleanupSemantic: Reverse overlap elimination.", { Diff( INSERT, L"def" ), Diff( EQUAL, L"xxx" ), Diff( DELETE, L"abc" ) }, diffs );

    diffs = { Diff( DELETE, L"abcd1212" ), Diff( INSERT, L"1212efghi" ), Diff( EQUAL, L"----" ), Diff( DELETE, L"A3" ), Diff( INSERT, L"3BC" ) };
    dmp.diff_cleanupSemantic( diffs );
    assertEquals( "diff_cleanupSemantic: Two overlap eliminations.", { Diff( DELETE, L"abcd" ), Diff( EQUAL, L"1212" ), Diff( INSERT, L"efghi" ), Diff( EQUAL, L"----" ), Diff( DELETE, L"a" ), Diff( EQUAL, L"3" ), Diff( INSERT, L"BC" ) }, diffs );
}

void diff_match_patch_test::testDiffCleanupEfficiency()
{
    // Cleanup operationally trivial equalities.
    dmp.Diff_EditCost = 4;
    auto diffs = TDiffVector();
    dmp.diff_cleanupEfficiency( diffs );
    assertEquals( "diff_cleanupEfficiency: nullptr case.", {}, diffs );

    diffs = { Diff( DELETE, L"ab" ), Diff( INSERT, L"12" ), Diff( EQUAL, L"wxyz" ), Diff( DELETE, L"cd" ), Diff( INSERT, L"34" ) };
    dmp.diff_cleanupEfficiency( diffs );
    assertEquals( "diff_cleanupEfficiency: No elimination.", { Diff( DELETE, L"ab" ), Diff( INSERT, L"12" ), Diff( EQUAL, L"wxyz" ), Diff( DELETE, L"cd" ), Diff( INSERT, L"34" ) }, diffs );

    diffs = { Diff( DELETE, L"ab" ), Diff( INSERT, L"12" ), Diff( EQUAL, L"xyz" ), Diff( DELETE, L"cd" ), Diff( INSERT, L"34" ) };
    dmp.diff_cleanupEfficiency( diffs );
    assertEquals( "diff_cleanupEfficiency: Four-edit elimination.", { Diff( DELETE, L"abxyzcd" ), Diff( INSERT, L"12xyz34" ) }, diffs );

    diffs = { Diff( INSERT, L"12" ), Diff( EQUAL, L"x" ), Diff( DELETE, L"cd" ), Diff( INSERT, L"34" ) };
    dmp.diff_cleanupEfficiency( diffs );
    assertEquals( "diff_cleanupEfficiency: Three-edit elimination.", { Diff( DELETE, L"xcd" ), Diff( INSERT, L"12x34" ) }, diffs );

    diffs = { Diff( DELETE, L"ab" ), Diff( INSERT, L"12" ), Diff( EQUAL, L"xy" ), Diff( INSERT, L"34" ), Diff( EQUAL, L"z" ), Diff( DELETE, L"cd" ), Diff( INSERT, L"56" ) };
    dmp.diff_cleanupEfficiency( diffs );
    assertEquals( "diff_cleanupEfficiency: Backpass elimination.", { Diff( DELETE, L"abxyzcd" ), Diff( INSERT, L"12xy34z56" ) }, diffs );

    dmp.Diff_EditCost = 5;
    diffs = { Diff( DELETE, L"ab" ), Diff( INSERT, L"12" ), Diff( EQUAL, L"wxyz" ), Diff( DELETE, L"cd" ), Diff( INSERT, L"34" ) };
    dmp.diff_cleanupEfficiency( diffs );
    assertEquals( "diff_cleanupEfficiency: High cost elimination.", { Diff( DELETE, L"abwxyzcd" ), Diff( INSERT, L"12wxyz34" ) }, diffs );
    dmp.Diff_EditCost = 4;
}

void diff_match_patch_test::testDiffPrettyHtml()
{
    // Pretty print.
    auto diffs = TDiffVector( { Diff( EQUAL, L"a\n" ), Diff( DELETE, L"<B>b</B>" ), Diff( INSERT, L"c&d" ) } );
    assertEquals( "diff_prettyHtml:", L"<span>a&para;<br></span><del style=\"background:#ffe6e6;\">&lt;B&gt;b&lt;/B&gt;</del><ins style=\"background:#e6ffe6;\">c&amp;d</ins>", dmp.diff_prettyHtml( diffs ) );
}

void diff_match_patch_test::testDiffText()
{
    // Compute the source and destination texts.
    auto diffs = TDiffVector( { Diff( EQUAL, L"jump" ), Diff( DELETE, L"s" ), Diff( INSERT, L"ed" ), Diff( EQUAL, L" over " ), Diff( DELETE, L"the" ), Diff( INSERT, L"a" ), Diff( EQUAL, L" lazy" ) } );
    assertEquals( "diff_text1:", L"jumps over the lazy", dmp.diff_text1( diffs ) );
    assertEquals( "diff_text2:", L"jumped over a lazy", dmp.diff_text2( diffs ) );
}

void diff_match_patch_test::testDiffDelta()
{
    // Convert a diff into delta string.
    auto diffs = TDiffVector( { Diff( EQUAL, L"jump" ), Diff( DELETE, L"s" ), Diff( INSERT, L"ed" ), Diff( EQUAL, L" over " ), Diff( DELETE, L"the" ), Diff( INSERT, L"a" ), Diff( EQUAL, L" lazy" ), Diff( INSERT, L"old dog" ) } );
    std::wstring text1 = dmp.diff_text1( diffs );
    assertEquals( "diff_text1: Base text.", L"jumps over the lazy", text1 );

    std::wstring delta = dmp.diff_toDelta( diffs );
    std::wstring golden = L"=4\t-1\t+ed\t=6\t-3\t+a\t=5\t+old dog";
    assertEquals( "diff_toDelta:", L"=4\t-1\t+ed\t=6\t-3\t+a\t=5\t+old dog", delta );

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
        dmp.diff_fromDelta( L"", L"+%c3%xy" );
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
    assertEquals( "diff_toDelta: Unicode", L"=7\t-7\t+%DA%82 %02 %5C %7C", delta );

    assertEquals( "diff_fromDelta: Unicode", diffs, dmp.diff_fromDelta( text1, delta ) );

    // Verify pool of unchanged characters.
    diffs = { Diff( INSERT, L"A-Z a-z 0-9 - _ . ! ~ * ' ( ) ; / ? : @ & = + $ , # " ) };
    std::wstring text2 = dmp.diff_text2( diffs );
    assertEquals( "diff_text2: Unchanged characters.", L"A-Z a-z 0-9 - _ . ! ~ * \' ( ) ; / ? : @ & = + $ , # ", text2 );

    delta = dmp.diff_toDelta( diffs );
    assertEquals( "diff_toDelta: Unchanged characters.", L"+A-Z a-z 0-9 - _ . ! ~ * \' ( ) ; / ? : @ & = + $ , # ", delta );

    // Convert delta string into a diff.
    assertEquals( "diff_fromDelta: Unchanged characters.", diffs, dmp.diff_fromDelta( L"", delta ) );
}

void diff_match_patch_test::testDiffXIndex()
{
    // Translate a location in text1 to text2.
    auto diffs = TDiffVector( { Diff( DELETE, L"a" ), Diff( INSERT, L"1234" ), Diff( EQUAL, L"xyz" ) } );
    assertEquals( "diff_xIndex: Translation on equality.", 5, dmp.diff_xIndex( diffs, 2 ) );

    diffs = { Diff( EQUAL, L"a" ), Diff( DELETE, L"1234" ), Diff( EQUAL, L"xyz" ) };
    assertEquals( "diff_xIndex: Translation on deletion.", 1, dmp.diff_xIndex( diffs, 3 ) );
}

void diff_match_patch_test::testDiffLevenshtein()
{
    auto diffs = TDiffVector( { Diff( DELETE, L"abc" ), Diff( INSERT, L"1234" ), Diff( EQUAL, L"xyz" ) } );
    assertEquals( "diff_levenshtein: Trailing equality.", 4, dmp.diff_levenshtein( diffs ) );

    diffs = { Diff( EQUAL, L"xyz" ), Diff( DELETE, L"abc" ), Diff( INSERT, L"1234" ) };
    assertEquals( "diff_levenshtein: Leading equality.", 4, dmp.diff_levenshtein( diffs ) );

    diffs = { Diff( DELETE, L"abc" ), Diff( EQUAL, L"xyz" ), Diff( INSERT, L"1234" ) };
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
    auto diffs = TDiffVector( { Diff( DELETE, L"c" ), Diff( INSERT, L"m" ), Diff( EQUAL, L"a" ), Diff( DELETE, L"t" ), Diff( INSERT, L"p" ) } );
    auto results = dmp.diff_bisect( a, b, std::numeric_limits< clock_t >::max() );
    assertEquals( "diff_bisect: Normal.", diffs, results );

    // Timeout.
    diffs = { Diff( DELETE, L"cat" ), Diff( INSERT, L"map" ) };
    assertEquals( "diff_bisect: Timeout.", diffs, dmp.diff_bisect( a, b, 0 ) );
}

void diff_match_patch_test::testDiffMain()
{
    // Perform a trivial diff.
    auto diffs = TDiffVector();
    assertEquals( "diff_main: nullptr case.", diffs, dmp.diff_main( L"", L"", false ) );

    diffs = { Diff( DELETE, L"abc" ) };
    assertEquals( "diff_main: RHS side nullptr case.", diffs, dmp.diff_main( L"abc", L"", false ) );

    diffs = { Diff( INSERT, L"abc" ) };
    assertEquals( "diff_main: LHS side nullptr case.", diffs, dmp.diff_main( L"", L"abc", false ) );

    diffs = { Diff( EQUAL, L"abc" ) };
    assertEquals( "diff_main: Equality.", diffs, dmp.diff_main( L"abc", L"abc", false ) );

    diffs = { Diff( EQUAL, L"ab" ), Diff( INSERT, L"123" ), Diff( EQUAL, L"c" ) };
    assertEquals( "diff_main: Simple insertion.", diffs, dmp.diff_main( L"abc", L"ab123c", false ) );

    diffs = { Diff( EQUAL, L"a" ), Diff( DELETE, L"123" ), Diff( EQUAL, L"bc" ) };
    assertEquals( "diff_main: Simple deletion.", diffs, dmp.diff_main( L"a123bc", L"abc", false ) );

    diffs = { Diff( EQUAL, L"a" ), Diff( INSERT, L"123" ), Diff( EQUAL, L"b" ), Diff( INSERT, L"456" ), Diff( EQUAL, L"c" ) };
    assertEquals( "diff_main: Two insertions.", diffs, dmp.diff_main( L"abc", L"a123b456c", false ) );

    diffs = { Diff( EQUAL, L"a" ), Diff( DELETE, L"123" ), Diff( EQUAL, L"b" ), Diff( DELETE, L"456" ), Diff( EQUAL, L"c" ) };
    assertEquals( "diff_main: Two deletions.", diffs, dmp.diff_main( L"a123b456c", L"abc", false ) );

    // Perform a real diff.
    // Switch off the timeout.
    dmp.Diff_Timeout = 0;
    diffs = { Diff( DELETE, L"a" ), Diff( INSERT, L"b" ) };
    assertEquals( "diff_main: Simple case #1.", diffs, dmp.diff_main( L"a", L"b", false ) );

    diffs = { Diff( DELETE, L"Apple" ), Diff( INSERT, L"Banana" ), Diff( EQUAL, L"s are a" ), Diff( INSERT, L"lso" ), Diff( EQUAL, L" fruit." ) };
    assertEquals( "diff_main: Simple case #2.", diffs, dmp.diff_main( L"Apples are a fruit.", L"Bananas are also fruit.", false ) );

    diffs = { Diff( DELETE, L"a" ), Diff( INSERT, L"\u0680" ), Diff( EQUAL, L"x" ), Diff( DELETE, L"\t" ), Diff( INSERT, to_wstring( kZero ) ) };
    assertEquals( "diff_main: Simple case #3.", diffs, dmp.diff_main( L"ax\t", std::wstring( L"\u0680x" ) + kZero, false ) );

    diffs = { Diff( DELETE, L"1" ), Diff( EQUAL, L"a" ), Diff( DELETE, L"y" ), Diff( EQUAL, L"b" ), Diff( DELETE, L"2" ), Diff( INSERT, L"xab" ) };
    assertEquals( "diff_main: Overlap #1.", diffs, dmp.diff_main( L"1ayb2", L"abxab", false ) );

    diffs = { Diff( INSERT, L"xaxcx" ), Diff( EQUAL, L"abc" ), Diff( DELETE, L"y" ) };
    assertEquals( "diff_main: Overlap #2.", diffs, dmp.diff_main( L"abcy", L"xaxcxabc", false ) );

    diffs = { Diff( DELETE, L"ABCD" ), Diff( EQUAL, L"a" ), Diff( DELETE, L"=" ), Diff( INSERT, L"-" ), Diff( EQUAL, L"bcd" ), Diff( DELETE, L"=" ), Diff( INSERT, L"-" ), Diff( EQUAL, L"efghijklmnopqrs" ), Diff( DELETE, L"EFGHIJKLMNOefg" ) };
    assertEquals( "diff_main: Overlap #3.", diffs, dmp.diff_main( L"ABCDa=bcd=efghijklmnopqrsEFGHIJKLMNOefg", L"a-bcd-efghijklmnopqrs", false ) );

    diffs = { Diff( INSERT, L" " ), Diff( EQUAL, L"a" ), Diff( INSERT, L"nd" ), Diff( EQUAL, L" [[Pennsylvania]]" ), Diff( DELETE, L" and [[New" ) };
    assertEquals( "diff_main: Large equality.", diffs, dmp.diff_main( L"a [[Pennsylvania]] and [[New", L" and [[Pennsylvania]]", false ) );

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
    assertEquals( "match_alphabet: Unique.", bitmask, dmp.match_alphabet( L"abc" ) );

    bitmask = TCharPosMap();
    bitmask[ 'a' ] = 37;
    bitmask[ 'b' ] = 18;
    bitmask[ 'c' ] = 8;
    assertEquals( "match_alphabet: Duplicates.", bitmask, dmp.match_alphabet( L"abcaba" ) );
}

void diff_match_patch_test::testMatchBitap()
{
    // Bitap algorithm.
    dmp.Match_Distance = 100;
    dmp.Match_Threshold = 0.5f;
    assertEquals( "match_bitap: Exact match #1.", 5, dmp.match_bitap( L"abcdefghijk", L"fgh", 5 ) );

    assertEquals( "match_bitap: Exact match #2.", 5, dmp.match_bitap( L"abcdefghijk", L"fgh", 0 ) );

    assertEquals( "match_bitap: Fuzzy match #1.", 4, dmp.match_bitap( L"abcdefghijk", L"efxhi", 0 ) );

    assertEquals( "match_bitap: Fuzzy match #2.", 2, dmp.match_bitap( L"abcdefghijk", L"cdefxyhijk", 5 ) );

    assertEquals( "match_bitap: Fuzzy match #3.", -1, dmp.match_bitap( L"abcdefghijk", L"bxy", 1 ) );

    assertEquals( "match_bitap: Overflow.", 2, dmp.match_bitap( L"123456789xx0", L"3456789x0", 2 ) );

    assertEquals( "match_bitap: Before start match.", 0, dmp.match_bitap( L"abcdef", L"xxabc", 4 ) );

    assertEquals( "match_bitap: Beyond end match.", 3, dmp.match_bitap( L"abcdef", L"defyy", 4 ) );

    assertEquals( "match_bitap: Oversized pattern.", 0, dmp.match_bitap( L"abcdef", L"xabcdefy", 0 ) );

    dmp.Match_Threshold = 0.4f;
    assertEquals( "match_bitap: Threshold #1.", 4, dmp.match_bitap( L"abcdefghijk", L"efxyhi", 1 ) );

    dmp.Match_Threshold = 0.3f;
    assertEquals( "match_bitap: Threshold #2.", -1, dmp.match_bitap( L"abcdefghijk", L"efxyhi", 1 ) );

    dmp.Match_Threshold = 0.0f;
    assertEquals( "match_bitap: Threshold #3.", 1, dmp.match_bitap( L"abcdefghijk", L"bcdef", 1 ) );

    dmp.Match_Threshold = 0.5f;
    assertEquals( "match_bitap: Multiple select #1.", 0, dmp.match_bitap( L"abcdexyzabcde", L"abccde", 3 ) );

    assertEquals( "match_bitap: Multiple select #2.", 8, dmp.match_bitap( L"abcdexyzabcde", L"abccde", 5 ) );

    dmp.Match_Distance = 10;   // Strict location.
    assertEquals( "match_bitap: Distance test #1.", -1, dmp.match_bitap( L"abcdefghijklmnopqrstuvwxyz", L"abcdefg", 24 ) );

    assertEquals( "match_bitap: Distance test #2.", 0, dmp.match_bitap( L"abcdefghijklmnopqrstuvwxyz", L"abcdxxefg", 1 ) );

    dmp.Match_Distance = 1000;   // Loose location.
    assertEquals( "match_bitap: Distance test #3.", 0, dmp.match_bitap( L"abcdefghijklmnopqrstuvwxyz", L"abcdefg", 24 ) );
}

void diff_match_patch_test::testMatchMain()
{
    // Full match.
    assertEquals( "match_main: Equality.", 0, dmp.match_main( L"abcdef", L"abcdef", 1000 ) );

    assertEquals( "match_main: nullptr text.", -1, dmp.match_main( L"", L"abcdef", 1 ) );

    assertEquals( "match_main: nullptr pattern.", 3, dmp.match_main( L"abcdef", L"", 3 ) );

    assertEquals( "match_main: Exact match.", 3, dmp.match_main( L"abcdef", L"de", 3 ) );

    dmp.Match_Threshold = 0.7f;
    assertEquals( "match_main: Complex match.", 4, dmp.match_main( L"I am the very model of a modern major general.", L" that berry ", 5 ) );
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
    p.diffs = { Diff( EQUAL, L"jump" ), Diff( DELETE, L"s" ), Diff( INSERT, L"ed" ), Diff( EQUAL, L" over " ), Diff( DELETE, L"the" ), Diff( INSERT, L"a" ), Diff( EQUAL, L"\nlaz" ) };
    std::wstring strp = L"@@ -21,18 +22,17 @@\n jump\n-s\n+ed\n  over \n-the\n+a\n %0Alaz\n";
    assertEquals( "Patch: toString.", strp, p.toString() );
}

void diff_match_patch_test::testPatchFromText()
{
    assertTrue( "patch_fromText: #0.", dmp.patch_fromText( L"" ).empty() );

    std::wstring strp = L"@@ -21,18 +22,17 @@\n jump\n-s\n+ed\n  over \n-the\n+a\n %0Alaz\n";
    assertEquals( "patch_fromText: #1.", strp, dmp.patch_fromText( strp )[ 0 ].toString() );

    assertEquals( "patch_fromText: #2.", L"@@ -1 +1 @@\n-a\n+b\n", dmp.patch_fromText( L"@@ -1 +1 @@\n-a\n+b\n" )[ 0 ].toString() );

    assertEquals( "patch_fromText: #3.", L"@@ -1,3 +0,0 @@\n-abc\n", dmp.patch_fromText( L"@@ -1,3 +0,0 @@\n-abc\n" )[ 0 ].toString() );

    assertEquals( "patch_fromText: #4.", L"@@ -0,0 +1,3 @@\n+abc\n", dmp.patch_fromText( L"@@ -0,0 +1,3 @@\n+abc\n" )[ 0 ].toString() );

    // Generates error.
    bool exceptionTriggered = false;
    try
    {
        dmp.patch_fromText( L"Bad\nPatch\n" );
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
    assertEquals( "patch_toText: Dual", strp, dmp.patch_toText( patches ) );
}

void diff_match_patch_test::testPatchAddContext()
{
    dmp.Patch_Margin = 4;
    auto p = dmp.patch_fromText( L"@@ -21,4 +21,10 @@\n-jump\n+somersault\n" )[ 0 ];
    dmp.patch_addContext( p, L"The quick brown fox jumps over the lazy dog." );
    assertEquals( "patch_addContext: Simple case.", L"@@ -17,12 +17,18 @@\n fox \n-jump\n+somersault\n s ov\n", p.toString() );

    p = dmp.patch_fromText( L"@@ -21,4 +21,10 @@\n-jump\n+somersault\n" )[ 0 ];
    dmp.patch_addContext( p, L"The quick brown fox jumps." );
    assertEquals( "patch_addContext: Not enough trailing context.", L"@@ -17,10 +17,16 @@\n fox \n-jump\n+somersault\n s.\n", p.toString() );

    p = dmp.patch_fromText( L"@@ -3 +3,2 @@\n-e\n+at\n" )[ 0 ];
    dmp.patch_addContext( p, L"The quick brown fox jumps." );
    assertEquals( "patch_addContext: Not enough leading context.", L"@@ -1,7 +1,8 @@\n Th\n-e\n+at\n  qui\n", p.toString() );

    p = dmp.patch_fromText( L"@@ -3 +3,2 @@\n-e\n+at\n" )[ 0 ];
    dmp.patch_addContext( p, L"The quick brown fox jumps.  The quick brown fox crashes." );
    assertEquals( "patch_addContext: Ambiguity.", L"@@ -1,27 +1,28 @@\n Th\n-e\n+at\n  quick brown fox jumps. \n", p.toString() );
}

void diff_match_patch_test::testPatchMake()
{
    TPatchVector patches;
    patches = dmp.patch_make( L"", L"" );
    assertEquals( "patch_make: nullptr case", L"", dmp.patch_toText( patches ) );

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

    patches = dmp.patch_make( L"`1234567890-=[]\\;',./", L"~!@#$%^&*()_+{}|:\"<>?" );
    assertEquals( "patch_toText: Character encoding.", L"@@ -1,21 +1,21 @@\n-%601234567890-=%5B%5D%5C;',./\n+~!@#$%25%5E&*()_+%7B%7D%7C:%22%3C%3E?\n", dmp.patch_toText( patches ) );

    diffs = { Diff( DELETE, L"`1234567890-=[]\\;',./" ), Diff( INSERT, L"~!@#$%^&*()_+{}|:\"<>?" ) };
    assertEquals( "patch_fromText: Character decoding.", diffs, dmp.patch_fromText( L"@@ -1,21 +1,21 @@\n-%601234567890-=%5B%5D%5C;',./\n+~!@#$%25%5E&*()_+%7B%7D%7C:%22%3C%3E?\n" )[ 0 ].diffs );

    text1 = L"";
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
    patches = dmp.patch_make( L"abcdefghijklmnopqrstuvwxyz01234567890", L"XabXcdXefXghXijXklXmnXopXqrXstXuvXwxXyzX01X23X45X67X89X0" );
    dmp.patch_splitMax( patches );
    assertEquals( "patch_splitMax: #1.", L"@@ -1,32 +1,46 @@\n+X\n ab\n+X\n cd\n+X\n ef\n+X\n gh\n+X\n ij\n+X\n kl\n+X\n mn\n+X\n op\n+X\n qr\n+X\n st\n+X\n uv\n+X\n wx\n+X\n yz\n+X\n 012345\n@@ -25,13 +39,18 @@\n zX01\n+X\n 23\n+X\n 45\n+X\n 67\n+X\n 89\n+X\n 0\n", dmp.patch_toText( patches ) );

    patches = dmp.patch_make( L"abcdef1234567890123456789012345678901234567890123456789012345678901234567890uvwxyz", L"abcdefuvwxyz" );
    std::wstring oldToText = dmp.patch_toText( patches );
    dmp.patch_splitMax( patches );
    assertEquals( "patch_splitMax: #2.", oldToText, dmp.patch_toText( patches ) );

    patches = dmp.patch_make( L"1234567890123456789012345678901234567890123456789012345678901234567890", L"abc" );
    dmp.patch_splitMax( patches );
    assertEquals( "patch_splitMax: #3.", L"@@ -1,32 +1,4 @@\n-1234567890123456789012345678\n 9012\n@@ -29,32 +1,4 @@\n-9012345678901234567890123456\n 7890\n@@ -57,14 +1,3 @@\n-78901234567890\n+abc\n", dmp.patch_toText( patches ) );

    patches = dmp.patch_make( L"abcdefghij , h : 0 , t : 1 abcdefghij , h : 0 , t : 1 abcdefghij , h : 0 , t : 1", L"abcdefghij , h : 1 , t : 1 abcdefghij , h : 1 , t : 1 abcdefghij , h : 0 , t : 1" );
    dmp.patch_splitMax( patches );
    assertEquals( "patch_splitMax: #4.", L"@@ -2,32 +2,32 @@\n bcdefghij , h : \n-0\n+1\n  , t : 1 abcdef\n@@ -29,32 +29,32 @@\n bcdefghij , h : \n-0\n+1\n  , t : 1 abcdef\n", dmp.patch_toText( patches ) );
}

void diff_match_patch_test::testPatchAddPadding()
{
    TPatchVector patches;
    patches = dmp.patch_make( L"", L"test" );
    assertEquals( "patch_addPadding: Both edges full", L"@@ -0,0 +1,4 @@\n+test\n", dmp.patch_toText( patches ) );
    dmp.patch_addPadding( patches );
    assertEquals( "patch_addPadding: Both edges full.", L"@@ -1,8 +1,12 @@\n %01%02%03%04\n+test\n %01%02%03%04\n", dmp.patch_toText( patches ) );

    patches = dmp.patch_make( L"XY", L"XtestY" );
    assertEquals( "patch_addPadding: Both edges partial.", L"@@ -1,2 +1,6 @@\n X\n+test\n Y\n", dmp.patch_toText( patches ) );
    dmp.patch_addPadding( patches );
    assertEquals( "patch_addPadding: Both edges partial.", L"@@ -2,8 +2,12 @@\n %02%03%04X\n+test\n Y%01%02%03\n", dmp.patch_toText( patches ) );

    patches = dmp.patch_make( L"XXXXYYYY", L"XXXXtestYYYY" );
    assertEquals( "patch_addPadding: Both edges none.", L"@@ -1,8 +1,12 @@\n XXXX\n+test\n YYYY\n", dmp.patch_toText( patches ) );
    dmp.patch_addPadding( patches );
    assertEquals( "patch_addPadding: Both edges none.", L"@@ -5,8 +5,12 @@\n XXXX\n+test\n YYYY\n", dmp.patch_toText( patches ) );
}

void diff_match_patch_test::testPatchApply()
{
    dmp.Match_Distance = 1000;
    dmp.Match_Threshold = 0.5f;
    dmp.Patch_DeleteThreshold = 0.5f;
    TPatchVector patches;
    patches = dmp.patch_make( L"", L"" );
    auto results = dmp.patch_apply( patches, L"Hello world." );
    auto &&boolArray = results.second;

    std::wstring resultStr = results.first + L"\t" + std::to_wstring( boolArray.size() );
    assertEquals( "patch_apply: nullptr case.", L"Hello world.\t0", resultStr );

    patches = dmp.patch_make( L"The quick brown fox jumps over the lazy dog.", L"That quick brown fox jumped over a lazy dog." );
    assertEquals( "patch_apply: Exact match.", L"@@ -1,11 +1,12 @@\n Th\n-e\n+at\n  quick b\n@@ -22,18 +22,17 @@\n jump\n-s\n+ed\n  over \n-the\n+a\n  laz\n", dmp.patch_toText( patches ) );

    results = dmp.patch_apply( patches, L"The quick brown fox jumps over the lazy dog." );
    boolArray = results.second;
    resultStr = results.first + L"\t" + ( boolArray[ 0 ] ? L"true" : L"false" ) + L"\t" + ( boolArray[ 1 ] ? L"true" : L"false" );
    assertEquals( "patch_apply: Exact match.", L"That quick brown fox jumped over a lazy dog.\ttrue\ttrue", resultStr );

    results = dmp.patch_apply( patches, L"The quick red rabbit jumps over the tired tiger." );
    boolArray = results.second;
    resultStr = results.first + L"\t" + ( boolArray[ 0 ] ? L"true" : L"false" ) + L"\t" + ( boolArray[ 1 ] ? L"true" : L"false" );
    assertEquals( "patch_apply: Partial match.", L"That quick red rabbit jumped over a tired tiger.\ttrue\ttrue", resultStr );

    results = dmp.patch_apply( patches, L"I am the very model of a modern major general." );
    boolArray = results.second;
    resultStr = results.first + L"\t" + ( boolArray[ 0 ] ? L"true" : L"false" ) + L"\t" + ( boolArray[ 1 ] ? L"true" : L"false" );
    assertEquals( "patch_apply: Failed match.", L"I am the very model of a modern major general.\tfalse\tfalse", resultStr );

    patches = dmp.patch_make( L"x1234567890123456789012345678901234567890123456789012345678901234567890y", L"xabcy" );
    results = dmp.patch_apply( patches, L"x123456789012345678901234567890-----++++++++++-----123456789012345678901234567890y" );
    boolArray = results.second;
    resultStr = results.first + L"\t" + ( boolArray[ 0 ] ? L"true" : L"false" ) + L"\t" + ( boolArray[ 1 ] ? L"true" : L"false" );
    assertEquals( "patch_apply: Big delete, small change.", L"xabcy\ttrue\ttrue", resultStr );

    patches = dmp.patch_make( L"x1234567890123456789012345678901234567890123456789012345678901234567890y", L"xabcy" );
    results = dmp.patch_apply( patches, L"x12345678901234567890---------------++++++++++---------------12345678901234567890y" );
    boolArray = results.second;
    resultStr = results.first + L"\t" + ( boolArray[ 0 ] ? L"true" : L"false" ) + L"\t" + ( boolArray[ 1 ] ? L"true" : L"false" );
    assertEquals( "patch_apply: Big delete, large change 1.", L"xabc12345678901234567890---------------++++++++++---------------12345678901234567890y\tfalse\ttrue", resultStr );

    dmp.Patch_DeleteThreshold = 0.6f;
    patches = dmp.patch_make( L"x1234567890123456789012345678901234567890123456789012345678901234567890y", L"xabcy" );
    results = dmp.patch_apply( patches, L"x12345678901234567890---------------++++++++++---------------12345678901234567890y" );
    boolArray = results.second;
    resultStr = results.first + L"\t" + ( boolArray[ 0 ] ? L"true" : L"false" ) + L"\t" + ( boolArray[ 1 ] ? L"true" : L"false" );
    assertEquals( "patch_apply: Big delete, large change 2.", L"xabcy\ttrue\ttrue", resultStr );
    dmp.Patch_DeleteThreshold = 0.5f;

    dmp.Match_Threshold = 0.0f;
    dmp.Match_Distance = 0;
    patches = dmp.patch_make( L"abcdefghijklmnopqrstuvwxyz--------------------1234567890", L"abcXXXXXXXXXXdefghijklmnopqrstuvwxyz--------------------1234567YYYYYYYYYY890" );
    results = dmp.patch_apply( patches, L"ABCDEFGHIJKLMNOPQRSTUVWXYZ--------------------1234567890" );
    boolArray = results.second;
    resultStr = results.first + L"\t" + ( boolArray[ 0 ] ? L"true" : L"false" ) + L"\t" + ( boolArray[ 1 ] ? L"true" : L"false" );
    assertEquals( "patch_apply: Compensate for failed patch.", L"ABCDEFGHIJKLMNOPQRSTUVWXYZ--------------------1234567YYYYYYYYYY890\tfalse\ttrue", resultStr );
    dmp.Match_Threshold = 0.5f;
    dmp.Match_Distance = 1000;

    patches = dmp.patch_make( L"", L"test" );
    std::wstring patchStr = dmp.patch_toText( patches );
    dmp.patch_apply( patches, L"" );
    assertEquals( "patch_apply: No side effects.", patchStr, dmp.patch_toText( patches ) );

    patches = dmp.patch_make( L"The quick brown fox jumps over the lazy dog.", L"Woof" );
    patchStr = dmp.patch_toText( patches );
    dmp.patch_apply( patches, L"The quick brown fox jumps over the lazy dog." );
    assertEquals( "patch_apply: No side effects with major delete.", patchStr, dmp.patch_toText( patches ) );

    patches = dmp.patch_make( L"", L"test" );
    results = dmp.patch_apply( patches, L"" );
    boolArray = results.second;
    resultStr = results.first + L"\t" + ( boolArray[ 0 ] ? L"true" : L"false" );
    assertEquals( "patch_apply: Edge exact match.", L"test\ttrue", resultStr );

    patches = dmp.patch_make( L"XY", L"XtestY" );
    results = dmp.patch_apply( patches, L"XY" );
    boolArray = results.second;
    resultStr = results.first + L"\t" + ( boolArray[ 0 ] ? L"true" : L"false" );
    assertEquals( "patch_apply: Near edge exact match.", L"XtestY\ttrue", resultStr );

    patches = dmp.patch_make( L"y", L"y123" );
    results = dmp.patch_apply( patches, L"x" );
    boolArray = results.second;
    resultStr = results.first + L"\t" + ( boolArray[ 0 ] ? L"true" : L"false" );
    assertEquals( "patch_apply: Edge partial match.", L"x123\ttrue", resultStr );
}

void diff_match_patch_test::reportFailure( const std::string &strCase, const std::wstring &expected, const std::wstring &actual )
{
    std::cout << "FAILED : " + strCase + "\n";
    std::wcerr << "    Expected: " << expected << L"\n      Actual: " << actual << "\n";
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

void diff_match_patch_test::assertEquals( const std::string &strCase, const std::string &s1, const std::string &s2 )
{
    if ( s1 != s2 )
    {
        reportFailure( strCase, to_wstring( s1 ), to_wstring( s2 ) );
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
        reportFailure( strCase, lhs ? L"true" : L"false", rhs ? L"true" : L"false" );
    }
    reportPassed( strCase );
}

void diff_match_patch_test::assertTrue( const std::string &strCase, bool value )
{
    if ( !value )
    {
        reportFailure( strCase, L"true", L"false" );
    }
    reportPassed( strCase );
}

void diff_match_patch_test::assertFalse( const std::string &strCase, bool value )
{
    if ( value )
    {
        reportFailure( strCase, L"false", L"true" );
    }
    reportPassed( strCase );
}

// Construct the two texts which made up the diff originally.
TStringVector diff_match_patch_test::diff_rebuildtexts( const TDiffVector &diffs )
{
    TStringVector text( { L"", L"" } );
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
