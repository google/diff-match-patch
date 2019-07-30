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
#include <string>
#include <list>
#include <unordered_map>
#include <iostream>
#include "diff_match_patch.h"
#include "diff_match_patch_test.h"

#define qPrintable(x) (url_encode(x, " !~*'();/?:@&=+$,#-_.~").c_str())

int main(int argc, char **argv) {
    diff_match_patch_test dmp_test;
    std::debug_print(L"Starting diff_match_patch unit tests.");
    dmp_test.run_all_tests();
    std::debug_print(L"Done.");
    return 0;
    (void)argc;
    (void)argv;
}


diff_match_patch_test::diff_match_patch_test() {
}

void diff_match_patch_test::run_all_tests() {
    try {
        testDiffCommonPrefix();
        testDiffCommonSuffix();
        testDiffCommonOverlap();
        testDiffHalfmatch();
        testDiffLinesToChars();
        testDiffCharsToLines();
        testDiffCleanupMerge();
        testDiffCleanupSemanticLossless();
        testDiffCleanupSemantic();
        testDiffCleanupEfficiency();
        testDiffPrettyHtml();
        testDiffText();
        testDiffDelta();
        testDiffXIndex();
        testDiffLevenshtein();
        testDiffBisect();
        testDiffMain();

        testMatchAlphabet();
        testMatchBitap();
        testMatchMain();

        testPatchObj();
        testPatchFromText();
        testPatchToText();
        testPatchAddContext();
        testPatchMake();
        testPatchSplitMax();
        testPatchAddPadding();
        testPatchApply();
        std::debug_print(L"All tests passed.");
    } catch (std::wstring strCase) {
        std::debug_print(L"Test failed: %ls", qPrintable(strCase));
    } catch (char const *strCase) {
        std::cout << "Test failed:" << strCase << std::endl;
    }

}

//  DIFF TEST FUNCTIONS

void diff_match_patch_test::testDiffCommonPrefix() {
    // Detect any common prefix.
    assertEquals(L"diff_commonPrefix: Null case.", 0, dmp.diff_commonPrefix(L"abc", L"xyz"));

    assertEquals(L"diff_commonPrefix: Non-null case.", 4, dmp.diff_commonPrefix(L"1234abcdef", L"1234xyz"));

    assertEquals(L"diff_commonPrefix: Whole case.", 4, dmp.diff_commonPrefix(L"1234", L"1234xyz"));
}

void diff_match_patch_test::testDiffCommonSuffix() {
    // Detect any common suffix.
    assertEquals(L"diff_commonSuffix: Null case.", 0, dmp.diff_commonSuffix(L"abc", L"xyz"));

    assertEquals(L"diff_commonSuffix: Non-null case.", 4, dmp.diff_commonSuffix(L"abcdef1234", L"xyz1234"));

    assertEquals(L"diff_commonSuffix: Whole case.", 4, dmp.diff_commonSuffix(L"1234", L"xyz1234"));
}

void diff_match_patch_test::testDiffCommonOverlap() {
    // Detect any suffix/prefix overlap.
    assertEquals(L"diff_commonOverlap: Null case.", 0, dmp.diff_commonOverlap(L"", L"abcd"));

    assertEquals(L"diff_commonOverlap: Whole case.", 3, dmp.diff_commonOverlap(L"abc", L"abcd"));

    assertEquals(L"diff_commonOverlap: No overlap.", 0, dmp.diff_commonOverlap(L"123456", L"abcd"));

    assertEquals(L"diff_commonOverlap: Overlap.", 3, dmp.diff_commonOverlap(L"123456xxx", L"xxxabcd"));

    // Some overly clever languages (C#) may treat ligatures as equal to their
    // component letters.  E.g. U+FB01 == 'fi'
    assertEquals(L"diff_commonOverlap: Unicode.", 0, dmp.diff_commonOverlap(L"fi", std::wstring(L"\ufb01i", 2)));
}

void diff_match_patch_test::testDiffHalfmatch() {
    // Detect a halfmatch.
    dmp.Diff_Timeout = 1;
    assertEmpty(L"diff_halfMatch: No match #1.", dmp.diff_halfMatch(L"1234567890", L"abcdef"));

    assertEmpty(L"diff_halfMatch: No match #2.", dmp.diff_halfMatch(L"12345", L"23"));

    assertEquals(L"diff_halfMatch: Single Match #1.", std::split(L"12,90,a,z,345678", ','), dmp.diff_halfMatch(L"1234567890", L"a345678z"));

    assertEquals(L"diff_halfMatch: Single Match #2.", std::split(L"a,z,12,90,345678", ','), dmp.diff_halfMatch(L"a345678z", L"1234567890"));

    assertEquals(L"diff_halfMatch: Single Match #3.", std::split(L"abc,z,1234,0,56789", ','), dmp.diff_halfMatch(L"abc56789z", L"1234567890"));

    assertEquals(L"diff_halfMatch: Single Match #4.", std::split(L"a,xyz,1,7890,23456", ','), dmp.diff_halfMatch(L"a23456xyz", L"1234567890"));

    assertEquals(L"diff_halfMatch: Multiple Matches #1.", std::split(L"12123,123121,a,z,1234123451234", ','), dmp.diff_halfMatch(L"121231234123451234123121", L"a1234123451234z"));

    assertEquals(L"diff_halfMatch: Multiple Matches #2.", std::split(L",-=-=-=-=-=,x,,x-=-=-=-=-=-=-=", ','), dmp.diff_halfMatch(L"x-=-=-=-=-=-=-=-=-=-=-=-=", L"xx-=-=-=-=-=-=-="));

    assertEquals(L"diff_halfMatch: Multiple Matches #3.", std::split(L"-=-=-=-=-=,,,y,-=-=-=-=-=-=-=y", ','), dmp.diff_halfMatch(L"-=-=-=-=-=-=-=-=-=-=-=-=y", L"-=-=-=-=-=-=-=yy"));

    // Optimal diff would be -q+x=H-i+e=lloHe+Hu=llo-Hew+y not -qHillo+x=HelloHe-w+Hulloy
    assertEquals(L"diff_halfMatch: Non-optimal halfmatch.", std::split(L"qHillo,w,x,Hulloy,HelloHe", ','), dmp.diff_halfMatch(L"qHilloHelloHew", L"xHelloHeHulloy"));

    dmp.Diff_Timeout = 0;
    assertEmpty(L"diff_halfMatch: Optimal no halfmatch.", dmp.diff_halfMatch(L"qHilloHelloHew", L"xHelloHeHulloy"));
}

void diff_match_patch_test::testDiffLinesToChars() {
    // Convert lines down to characters.
    std::wstring_list tmpList;
    std::list<std::dmp_variant> tmpVarList;
    tmpList.push_back(L"");
    tmpList.push_back(L"alpha\n");
    tmpList.push_back(L"beta\n");
    tmpVarList.push_back(std::dmp_variant(L"\u0001\u0002\u0001"));  //((L"\u0001\u0002\u0001"));
    tmpVarList.push_back(std::dmp_variant(L"\u0002\u0001\u0002"));  // ((L"\u0002\u0001\u0002"));
    tmpVarList.push_back(std::dmp_variant(tmpList));
    assertEquals(L"diff_linesToChars:", tmpVarList, dmp.diff_linesToChars(L"alpha\nbeta\nalpha\n", L"beta\nalpha\nbeta\n"));

    tmpList.clear();
    tmpVarList.clear();
    tmpList.push_back(L"");
    tmpList.push_back(L"alpha\r\n");
    tmpList.push_back(L"beta\r\n");
    tmpList.push_back(L"\r\n");
    tmpVarList.push_back(std::dmp_variant(std::wstring(L"")));
    tmpVarList.push_back(std::dmp_variant(L"\u0001\u0002\u0003\u0003"));  // ((L"\u0001\u0002\u0003\u0003"));
    tmpVarList.push_back(std::dmp_variant(tmpList));
    assertEquals(L"diff_linesToChars:", tmpVarList, dmp.diff_linesToChars(L"", L"alpha\r\nbeta\r\n\r\n\r\n"));

    tmpList.clear();
    tmpVarList.clear();
    tmpList.push_back(L"");
    tmpList.push_back(L"a");
    tmpList.push_back(L"b");
    tmpVarList.push_back(std::dmp_variant(L"\u0001"));  // ((L"\u0001")));
    tmpVarList.push_back(std::dmp_variant(L"\u0002"));  // ((L"\u0002"));
    tmpVarList.push_back(std::dmp_variant(tmpList));
    assertEquals(L"diff_linesToChars:", tmpVarList, dmp.diff_linesToChars(L"a", L"b"));

    // More than 256 to reveal any 8-bit limitations.
    int n = 300;
    tmpList.clear();
    tmpVarList.clear();
    std::wstring lines;
    std::wstring chars;
    for (int x = 1; x < n + 1; x++) {
        tmpList.push_back(std::to_wstring(x) + L"\n");
        lines += std::to_wstring(x) + L"\n";
        chars += wchar_t(x);
    }
    assertEquals(L"diff_linesToChars: More than 256 (setup).", n, tmpList.size());
    assertEquals(L"diff_linesToChars: More than 256 (setup).", n, chars.length());
    tmpList.insert(tmpList.begin(), L"");
    tmpVarList.push_back(std::dmp_variant(chars));
    tmpVarList.push_back(std::dmp_variant(std::wstring(L"")));
    tmpVarList.push_back(std::dmp_variant(tmpList));
    assertEquals(L"diff_linesToChars: More than 256.", tmpVarList, dmp.diff_linesToChars(lines, L""));
}

void diff_match_patch_test::testDiffCharsToLines() {
    // First check that Diff equality works.
    assertTrue(L"diff_charsToLines:", Diff(EQUAL, L"a") == Diff(EQUAL, L"a"));

    assertEquals(L"diff_charsToLines:", Diff(EQUAL, L"a"), Diff(EQUAL, L"a"));

    // Convert chars up to lines.
    std::list<Diff> diffs;
    diffs.push_back(Diff(EQUAL, L"\u0001\u0002\u0001"));  // (L"\u0001\u0002\u0001");
    diffs.push_back(Diff(INSERT, L"\u0002\u0001\u0002"));  // (L"\u0002\u0001\u0002");
    std::wstring_list tmpList;
    tmpList.push_back(L"");
    tmpList.push_back(L"alpha\n");
    tmpList.push_back(L"beta\n");
    dmp.diff_charsToLines(diffs, tmpList);
    assertEquals(L"diff_charsToLines:", diffList(Diff(EQUAL, L"alpha\nbeta\nalpha\n"), Diff(INSERT, L"beta\nalpha\nbeta\n")), diffs);

    // More than 256 to reveal any 8-bit limitations.
    int n = 300;
    tmpList.clear();
    std::list<std::dmp_variant> tmpVarList;
    std::wstring lines;
    std::wstring chars;
    for (int x = 1; x < n + 1; x++) {
        tmpList.push_back(std::to_wstring(x) + L"\n");
        lines += std::to_wstring(x) + L"\n";
        chars += wchar_t(x);
    }
    assertEquals(L"diff_linesToChars: More than 256 (setup).", n, tmpList.size());
    assertEquals(L"diff_linesToChars: More than 256 (setup).", n, chars.length());
    tmpList.insert(tmpList.begin(), L"");
    diffs = diffList(Diff(DELETE, chars));
    dmp.diff_charsToLines(diffs, tmpList);
    assertEquals(L"diff_charsToLines: More than 256.", diffList(Diff(DELETE, lines)), diffs);
}

void diff_match_patch_test::testDiffCleanupMerge() {
    // Cleanup a messy diff.
    std::list<Diff> diffs;
    dmp.diff_cleanupMerge(diffs);
    assertEquals(L"diff_cleanupMerge: Null case.", diffList(), diffs);

    diffs = diffList(Diff(EQUAL, L"a"), Diff(DELETE, L"b"), Diff(INSERT, L"c"));
    dmp.diff_cleanupMerge(diffs);
    assertEquals(L"diff_cleanupMerge: No change case.", diffList(Diff(EQUAL, L"a"), Diff(DELETE, L"b"), Diff(INSERT, L"c")), diffs);

    diffs = diffList(Diff(EQUAL, L"a"), Diff(EQUAL, L"b"), Diff(EQUAL, L"c"));
    dmp.diff_cleanupMerge(diffs);
    assertEquals(L"diff_cleanupMerge: Merge equalities.", diffList(Diff(EQUAL, L"abc")), diffs);

    diffs = diffList(Diff(DELETE, L"a"), Diff(DELETE, L"b"), Diff(DELETE, L"c"));
    dmp.diff_cleanupMerge(diffs);
    assertEquals(L"diff_cleanupMerge: Merge deletions.", diffList(Diff(DELETE, L"abc")), diffs);

    diffs = diffList(Diff(INSERT, L"a"), Diff(INSERT, L"b"), Diff(INSERT, L"c"));
    dmp.diff_cleanupMerge(diffs);
    assertEquals(L"diff_cleanupMerge: Merge insertions.", diffList(Diff(INSERT, L"abc")), diffs);

    diffs = diffList(Diff(DELETE, L"a"), Diff(INSERT, L"b"), Diff(DELETE, L"c"), Diff(INSERT, L"d"), Diff(EQUAL, L"e"), Diff(EQUAL, L"f"));
    dmp.diff_cleanupMerge(diffs);
    assertEquals(L"diff_cleanupMerge: Merge interweave.", diffList(Diff(DELETE, L"ac"), Diff(INSERT, L"bd"), Diff(EQUAL, L"ef")), diffs);

    diffs = diffList(Diff(DELETE, L"a"), Diff(INSERT, L"abc"), Diff(DELETE, L"dc"));
    dmp.diff_cleanupMerge(diffs);
    assertEquals(L"diff_cleanupMerge: Prefix and suffix detection.", diffList(Diff(EQUAL, L"a"), Diff(DELETE, L"d"), Diff(INSERT, L"b"), Diff(EQUAL, L"c")), diffs);

    diffs = diffList(Diff(EQUAL, L"x"), Diff(DELETE, L"a"), Diff(INSERT, L"abc"), Diff(DELETE, L"dc"), Diff(EQUAL, L"y"));
    dmp.diff_cleanupMerge(diffs);
    assertEquals(L"diff_cleanupMerge: Prefix and suffix detection with equalities.", diffList(Diff(EQUAL, L"xa"), Diff(DELETE, L"d"), Diff(INSERT, L"b"), Diff(EQUAL, L"cy")), diffs);

    diffs = diffList(Diff(EQUAL, L"a"), Diff(INSERT, L"ba"), Diff(EQUAL, L"c"));
    dmp.diff_cleanupMerge(diffs);
    assertEquals(L"diff_cleanupMerge: Slide edit left.", diffList(Diff(INSERT, L"ab"), Diff(EQUAL, L"ac")), diffs);

    diffs = diffList(Diff(EQUAL, L"c"), Diff(INSERT, L"ab"), Diff(EQUAL, L"a"));
    dmp.diff_cleanupMerge(diffs);
    assertEquals(L"diff_cleanupMerge: Slide edit right.", diffList(Diff(EQUAL, L"ca"), Diff(INSERT, L"ba")), diffs);

    diffs = diffList(Diff(EQUAL, L"a"), Diff(DELETE, L"b"), Diff(EQUAL, L"c"), Diff(DELETE, L"ac"), Diff(EQUAL, L"x"));
    dmp.diff_cleanupMerge(diffs);
    assertEquals(L"diff_cleanupMerge: Slide edit left recursive.", diffList(Diff(DELETE, L"abc"), Diff(EQUAL, L"acx")), diffs);

    diffs = diffList(Diff(EQUAL, L"x"), Diff(DELETE, L"ca"), Diff(EQUAL, L"c"), Diff(DELETE, L"b"), Diff(EQUAL, L"a"));
    dmp.diff_cleanupMerge(diffs);
    assertEquals(L"diff_cleanupMerge: Slide edit right recursive.", diffList(Diff(EQUAL, L"xca"), Diff(DELETE, L"cba")), diffs);
}

void diff_match_patch_test::testDiffCleanupSemanticLossless() {
    // Slide diffs to match logical boundaries.
    std::list<Diff> diffs = diffList();
    dmp.diff_cleanupSemanticLossless(diffs);
    assertEquals(L"diff_cleanupSemantic: Null case.", diffList(), diffs);

    diffs = diffList(Diff(EQUAL, L"AAA\r\n\r\nBBB"), Diff(INSERT, L"\r\nDDD\r\n\r\nBBB"), Diff(EQUAL, L"\r\nEEE"));
    dmp.diff_cleanupSemanticLossless(diffs);
    assertEquals(L"diff_cleanupSemanticLossless: Blank lines.", diffList(Diff(EQUAL, L"AAA\r\n\r\n"), Diff(INSERT, L"BBB\r\nDDD\r\n\r\n"), Diff(EQUAL, L"BBB\r\nEEE")), diffs);

    diffs = diffList(Diff(EQUAL, L"AAA\r\nBBB"), Diff(INSERT, L" DDD\r\nBBB"), Diff(EQUAL, L" EEE"));
    dmp.diff_cleanupSemanticLossless(diffs);
    assertEquals(L"diff_cleanupSemanticLossless: Line boundaries.", diffList(Diff(EQUAL, L"AAA\r\n"), Diff(INSERT, L"BBB DDD\r\n"), Diff(EQUAL, L"BBB EEE")), diffs);

    diffs = diffList(Diff(EQUAL, L"The c"), Diff(INSERT, L"ow and the c"), Diff(EQUAL, L"at."));
    dmp.diff_cleanupSemanticLossless(diffs);
    assertEquals(L"diff_cleanupSemantic: Word boundaries.", diffList(Diff(EQUAL, L"The "), Diff(INSERT, L"cow and the "), Diff(EQUAL, L"cat.")), diffs);

    diffs = diffList(Diff(EQUAL, L"The-c"), Diff(INSERT, L"ow-and-the-c"), Diff(EQUAL, L"at."));
    dmp.diff_cleanupSemanticLossless(diffs);
    assertEquals(L"diff_cleanupSemantic: Alphanumeric boundaries.", diffList(Diff(EQUAL, L"The-"), Diff(INSERT, L"cow-and-the-"), Diff(EQUAL, L"cat.")), diffs);

    diffs = diffList(Diff(EQUAL, L"a"), Diff(DELETE, L"a"), Diff(EQUAL, L"ax"));
    dmp.diff_cleanupSemanticLossless(diffs);
    assertEquals(L"diff_cleanupSemantic: Hitting the start.", diffList(Diff(DELETE, L"a"), Diff(EQUAL, L"aax")), diffs);

    diffs = diffList(Diff(EQUAL, L"xa"), Diff(DELETE, L"a"), Diff(EQUAL, L"a"));
    dmp.diff_cleanupSemanticLossless(diffs);
    assertEquals(L"diff_cleanupSemantic: Hitting the end.", diffList(Diff(EQUAL, L"xaa"), Diff(DELETE, L"a")), diffs);

    diffs = diffList(Diff(EQUAL, L"The xxx. The "), Diff(INSERT, L"zzz. The "), Diff(EQUAL, L"yyy."));
    dmp.diff_cleanupSemanticLossless(diffs);
    assertEquals(L"diff_cleanupSemantic: Sentence boundaries.", diffList(Diff(EQUAL, L"The xxx."), Diff(INSERT, L" The zzz."), Diff(EQUAL, L" The yyy.")), diffs);
}

void diff_match_patch_test::testDiffCleanupSemantic() {
    // Cleanup semantically trivial equalities.
    std::list<Diff> diffs = diffList();
    dmp.diff_cleanupSemantic(diffs);
    assertEquals(L"diff_cleanupSemantic: Null case.", diffList(), diffs);

    diffs = diffList(Diff(DELETE, L"ab"), Diff(INSERT, L"cd"), Diff(EQUAL, L"12"), Diff(DELETE, L"e"));
    dmp.diff_cleanupSemantic(diffs);
    assertEquals(L"diff_cleanupSemantic: No elimination #1.", diffList(Diff(DELETE, L"ab"), Diff(INSERT, L"cd"), Diff(EQUAL, L"12"), Diff(DELETE, L"e")), diffs);

    diffs = diffList(Diff(DELETE, L"abc"), Diff(INSERT, L"ABC"), Diff(EQUAL, L"1234"), Diff(DELETE, L"wxyz"));
    dmp.diff_cleanupSemantic(diffs);
    assertEquals(L"diff_cleanupSemantic: No elimination #2.", diffList(Diff(DELETE, L"abc"), Diff(INSERT, L"ABC"), Diff(EQUAL, L"1234"), Diff(DELETE, L"wxyz")), diffs);

    diffs = diffList(Diff(DELETE, L"a"), Diff(EQUAL, L"b"), Diff(DELETE, L"c"));
    dmp.diff_cleanupSemantic(diffs);
    assertEquals(L"diff_cleanupSemantic: Simple elimination.", diffList(Diff(DELETE, L"abc"), Diff(INSERT, L"b")), diffs);

    diffs = diffList(Diff(DELETE, L"ab"), Diff(EQUAL, L"cd"), Diff(DELETE, L"e"), Diff(EQUAL, L"f"), Diff(INSERT, L"g"));
    dmp.diff_cleanupSemantic(diffs);
    assertEquals(L"diff_cleanupSemantic: Backpass elimination.", diffList(Diff(DELETE, L"abcdef"), Diff(INSERT, L"cdfg")), diffs);

    diffs = diffList(Diff(INSERT, L"1"), Diff(EQUAL, L"A"), Diff(DELETE, L"B"), Diff(INSERT, L"2"), Diff(EQUAL, L"_"), Diff(INSERT, L"1"), Diff(EQUAL, L"A"), Diff(DELETE, L"B"), Diff(INSERT, L"2"));
    dmp.diff_cleanupSemantic(diffs);
    assertEquals(L"diff_cleanupSemantic: Multiple elimination.", diffList(Diff(DELETE, L"AB_AB"), Diff(INSERT, L"1A2_1A2")), diffs);

    diffs = diffList(Diff(EQUAL, L"The c"), Diff(DELETE, L"ow and the c"), Diff(EQUAL, L"at."));
    dmp.diff_cleanupSemantic(diffs);
    assertEquals(L"diff_cleanupSemantic: Word boundaries.", diffList(Diff(EQUAL, L"The "), Diff(DELETE, L"cow and the "), Diff(EQUAL, L"cat.")), diffs);

    diffs = diffList(Diff(DELETE, L"abcxx"), Diff(INSERT, L"xxdef"));
    dmp.diff_cleanupSemantic(diffs);
    assertEquals(L"diff_cleanupSemantic: No overlap elimination.", diffList(Diff(DELETE, L"abcxx"), Diff(INSERT, L"xxdef")), diffs);

    diffs = diffList(Diff(DELETE, L"abcxxx"), Diff(INSERT, L"xxxdef"));
    dmp.diff_cleanupSemantic(diffs);
    assertEquals(L"diff_cleanupSemantic: Overlap elimination.", diffList(Diff(DELETE, L"abc"), Diff(EQUAL, L"xxx"), Diff(INSERT, L"def")), diffs);

    diffs = diffList(Diff(DELETE, L"xxxabc"), Diff(INSERT, L"defxxx"));
    dmp.diff_cleanupSemantic(diffs);
    assertEquals(L"diff_cleanupSemantic: Reverse overlap elimination.", diffList(Diff(INSERT, L"def"), Diff(EQUAL, L"xxx"), Diff(DELETE, L"abc")), diffs);

    diffs = diffList(Diff(DELETE, L"abcd1212"), Diff(INSERT, L"1212efghi"), Diff(EQUAL, L"----"), Diff(DELETE, L"A3"), Diff(INSERT, L"3BC"));
    dmp.diff_cleanupSemantic(diffs);
    assertEquals(L"diff_cleanupSemantic: Two overlap eliminations.", diffList(Diff(DELETE, L"abcd"), Diff(EQUAL, L"1212"), Diff(INSERT, L"efghi"), Diff(EQUAL, L"----"), Diff(DELETE, L"A"), Diff(EQUAL, L"3"), Diff(INSERT, L"BC")), diffs);
}

void diff_match_patch_test::testDiffCleanupEfficiency() {
    // Cleanup operationally trivial equalities.
    dmp.Diff_EditCost = 4;
    std::list<Diff> diffs = diffList();
    dmp.diff_cleanupEfficiency(diffs);
    assertEquals(L"diff_cleanupEfficiency: Null case.", diffList(), diffs);

    diffs = diffList(Diff(DELETE, L"ab"), Diff(INSERT, L"12"), Diff(EQUAL, L"wxyz"), Diff(DELETE, L"cd"), Diff(INSERT, L"34"));
    dmp.diff_cleanupEfficiency(diffs);
    assertEquals(L"diff_cleanupEfficiency: No elimination.", diffList(Diff(DELETE, L"ab"), Diff(INSERT, L"12"), Diff(EQUAL, L"wxyz"), Diff(DELETE, L"cd"), Diff(INSERT, L"34")), diffs);

    diffs = diffList(Diff(DELETE, L"ab"), Diff(INSERT, L"12"), Diff(EQUAL, L"xyz"), Diff(DELETE, L"cd"), Diff(INSERT, L"34"));
    dmp.diff_cleanupEfficiency(diffs);
    assertEquals(L"diff_cleanupEfficiency: Four-edit elimination.", diffList(Diff(DELETE, L"abxyzcd"), Diff(INSERT, L"12xyz34")), diffs);

    diffs = diffList(Diff(INSERT, L"12"), Diff(EQUAL, L"x"), Diff(DELETE, L"cd"), Diff(INSERT, L"34"));
    dmp.diff_cleanupEfficiency(diffs);
    assertEquals(L"diff_cleanupEfficiency: Three-edit elimination.", diffList(Diff(DELETE, L"xcd"), Diff(INSERT, L"12x34")), diffs);

    diffs = diffList(Diff(DELETE, L"ab"), Diff(INSERT, L"12"), Diff(EQUAL, L"xy"), Diff(INSERT, L"34"), Diff(EQUAL, L"z"), Diff(DELETE, L"cd"), Diff(INSERT, L"56"));
    dmp.diff_cleanupEfficiency(diffs);
    assertEquals(L"diff_cleanupEfficiency: Backpass elimination.", diffList(Diff(DELETE, L"abxyzcd"), Diff(INSERT, L"12xy34z56")), diffs);

    dmp.Diff_EditCost = 5;
    diffs = diffList(Diff(DELETE, L"ab"), Diff(INSERT, L"12"), Diff(EQUAL, L"wxyz"), Diff(DELETE, L"cd"), Diff(INSERT, L"34"));
    dmp.diff_cleanupEfficiency(diffs);
    assertEquals(L"diff_cleanupEfficiency: High cost elimination.", diffList(Diff(DELETE, L"abwxyzcd"), Diff(INSERT, L"12wxyz34")), diffs);
    dmp.Diff_EditCost = 4;
}

void diff_match_patch_test::testDiffPrettyHtml() {
    // Pretty print.
    std::list<Diff> diffs = diffList(Diff(EQUAL, L"a\n"), Diff(DELETE, L"<B>b</B>"), Diff(INSERT, L"c&d"));
    assertEquals(L"diff_prettyHtml:", L"<span>a&para;<br></span><del style=\"background:#ffe6e6;\">&lt;B&gt;b&lt;/B&gt;</del><ins style=\"background:#e6ffe6;\">c&amp;d</ins>", dmp.diff_prettyHtml(diffs));
}

void diff_match_patch_test::testDiffText() {
    // Compute the source and destination texts.
    std::list<Diff> diffs = diffList(Diff(EQUAL, L"jump"), Diff(DELETE, L"s"), Diff(INSERT, L"ed"), Diff(EQUAL, L" over "), Diff(DELETE, L"the"), Diff(INSERT, L"a"), Diff(EQUAL, L" lazy"));
    assertEquals(L"diff_text1:", L"jumps over the lazy", dmp.diff_text1(diffs));
    assertEquals(L"diff_text2:", L"jumped over a lazy", dmp.diff_text2(diffs));
}

void diff_match_patch_test::testDiffDelta() {
    // Convert a diff into delta string.
    std::list<Diff> diffs = diffList(Diff(EQUAL, L"jump"), Diff(DELETE, L"s"), Diff(INSERT, L"ed"), Diff(EQUAL, L" over "), Diff(DELETE, L"the"), Diff(INSERT, L"a"), Diff(EQUAL, L" lazy"), Diff(INSERT, L"old dog"));
    std::wstring text1 = dmp.diff_text1(diffs);
    assertEquals(L"diff_text1: Base text.", L"jumps over the lazy", text1);

    std::wstring delta = dmp.diff_toDelta(diffs);
    assertEquals(L"diff_toDelta:", L"=4\t-1\t+ed\t=6\t-3\t+a\t=5\t+old dog", delta);

    // Convert delta string into a diff.
    assertEquals(L"diff_fromDelta: Normal.", diffs, dmp.diff_fromDelta(text1, delta));

    // Generates error (19 < 20).
    try {
        dmp.diff_fromDelta(text1 + std::wstring(L"x"), delta);
        assertFalse(L"diff_fromDelta: Too long.", true);
    } catch (std::wstring ex) {
        // Exception expected.
    }

    // Generates error (19 > 18).
    try {
        dmp.diff_fromDelta(text1.substr(1), delta);
        assertFalse(L"diff_fromDelta: Too short.", true);
    } catch (std::wstring ex) {
        // Exception expected.
    }

    // Generates error (%c3%xy invalid Unicode).
    /* This test does not work because QUrl::fromPercentEncoding(L"%xy") -> "?"
       try {
       dmp.diff_fromDelta(L"", L"+%c3%xy");
       assertFalse(L"diff_fromDelta: Invalid character.", true);
       } catch (std::wstring ex) {
       // Exception expected.
       }
    */

    // Test deltas with special characters.
    diffs = diffList(Diff(EQUAL, std::wstring(L"\u0680 \000 \t %", 7)), Diff(DELETE, std::wstring(L"\u0681 \001 \n ^", 7)), Diff(INSERT, std::wstring(L"\u0682 \002 \\ |", 7)));
    text1 = dmp.diff_text1(diffs);
    assertEquals(L"diff_text1: Unicode text.", std::wstring(L"\u0680 \000 \t %\u0681 \001 \n ^", 14), text1);

    delta = dmp.diff_toDelta(diffs);
    assertEquals(L"diff_toDelta: Unicode.", L"=7\t-7\t+%DA%82 %02 %5C %7C", delta);

    assertEquals(L"diff_fromDelta: Unicode.", diffs, dmp.diff_fromDelta(text1, delta));

    // Verify pool of unchanged characters.
    diffs = diffList(Diff(INSERT, L"A-Z a-z 0-9 - _ . ! ~ * ' ( ) ; / ? : @ & = + $ , # "));
    std::wstring text2 = dmp.diff_text2(diffs);
    assertEquals(L"diff_text2: Unchanged characters.", L"A-Z a-z 0-9 - _ . ! ~ * \' ( ) ; / ? : @ & = + $ , # ", text2);

    delta = dmp.diff_toDelta(diffs);
    assertEquals(L"diff_toDelta: Unchanged characters.", L"+A-Z a-z 0-9 - _ . ! ~ * \' ( ) ; / ? : @ & = + $ , # ", delta);

    // Convert delta string into a diff.
    assertEquals(L"diff_fromDelta: Unchanged characters.", diffs, dmp.diff_fromDelta(L"", delta));
}

void diff_match_patch_test::testDiffXIndex() {
    // Translate a location in text1 to text2.
    std::list<Diff> diffs = diffList(Diff(DELETE, L"a"), Diff(INSERT, L"1234"), Diff(EQUAL, L"xyz"));
    assertEquals(L"diff_xIndex: Translation on equality.", 5, dmp.diff_xIndex(diffs, 2));

    diffs = diffList(Diff(EQUAL, L"a"), Diff(DELETE, L"1234"), Diff(EQUAL, L"xyz"));
    assertEquals(L"diff_xIndex: Translation on deletion.", 1, dmp.diff_xIndex(diffs, 3));
}

void diff_match_patch_test::testDiffLevenshtein() {
    std::list<Diff> diffs = diffList(Diff(DELETE, L"abc"), Diff(INSERT, L"1234"), Diff(EQUAL, L"xyz"));
    assertEquals(L"diff_levenshtein: Trailing equality.", 4, dmp.diff_levenshtein(diffs));

    diffs = diffList(Diff(EQUAL, L"xyz"), Diff(DELETE, L"abc"), Diff(INSERT, L"1234"));
    assertEquals(L"diff_levenshtein: Leading equality.", 4, dmp.diff_levenshtein(diffs));

    diffs = diffList(Diff(DELETE, L"abc"), Diff(EQUAL, L"xyz"), Diff(INSERT, L"1234"));
    assertEquals(L"diff_levenshtein: Middle equality.", 7, dmp.diff_levenshtein(diffs));
}

void diff_match_patch_test::testDiffBisect() {
    // Normal.
    std::wstring a = L"cat";
    std::wstring b = L"map";
    // Since the resulting diff hasn't been normalized, it would be ok if
    // the insertion and deletion pairs are swapped.
    // If the order changes, tweak this test as required.
    std::list<Diff> diffs = diffList(Diff(DELETE, L"c"), Diff(INSERT, L"m"), Diff(EQUAL, L"a"), Diff(DELETE, L"t"), Diff(INSERT, L"p"));
    assertEquals(L"diff_bisect: Normal.", diffs, dmp.diff_bisect(a, b, std::numeric_limits<clock_t>::max()));

    // Timeout.
    diffs = diffList(Diff(DELETE, L"cat"), Diff(INSERT, L"map"));
    assertEquals(L"diff_bisect: Timeout.", diffs, dmp.diff_bisect(a, b, 0));
}

void diff_match_patch_test::testDiffMain() {
    // Perform a trivial diff.
    std::list<Diff> diffs = diffList();
    assertEquals(L"diff_main: Null case.", diffs, dmp.diff_main(L"", L"", false));

    diffs = diffList(Diff(EQUAL, L"abc"));
    assertEquals(L"diff_main: Equality.", diffs, dmp.diff_main(L"abc", L"abc", false));

    diffs = diffList(Diff(EQUAL, L"ab"), Diff(INSERT, L"123"), Diff(EQUAL, L"c"));
    assertEquals(L"diff_main: Simple insertion.", diffs, dmp.diff_main(L"abc", L"ab123c", false));

    diffs = diffList(Diff(EQUAL, L"a"), Diff(DELETE, L"123"), Diff(EQUAL, L"bc"));
    assertEquals(L"diff_main: Simple deletion.", diffs, dmp.diff_main(L"a123bc", L"abc", false));

    diffs = diffList(Diff(EQUAL, L"a"), Diff(INSERT, L"123"), Diff(EQUAL, L"b"), Diff(INSERT, L"456"), Diff(EQUAL, L"c"));
    assertEquals(L"diff_main: Two insertions.", diffs, dmp.diff_main(L"abc", L"a123b456c", false));

    diffs = diffList(Diff(EQUAL, L"a"), Diff(DELETE, L"123"), Diff(EQUAL, L"b"), Diff(DELETE, L"456"), Diff(EQUAL, L"c"));
    assertEquals(L"diff_main: Two deletions.", diffs, dmp.diff_main(L"a123b456c", L"abc", false));

    // Perform a real diff.
    // Switch off the timeout.
    dmp.Diff_Timeout = 0;
    diffs = diffList(Diff(DELETE, L"a"), Diff(INSERT, L"b"));
    assertEquals(L"diff_main: Simple case #1.", diffs, dmp.diff_main(L"a", L"b", false));

    diffs = diffList(Diff(DELETE, L"Apple"), Diff(INSERT, L"Banana"), Diff(EQUAL, L"s are a"), Diff(INSERT, L"lso"), Diff(EQUAL, L" fruit."));
    assertEquals(L"diff_main: Simple case #2.", diffs, dmp.diff_main(L"Apples are a fruit.", L"Bananas are also fruit.", false));

    diffs = diffList(Diff(DELETE, L"a"), Diff(INSERT, std::wstring(L"\u0680", 1)), Diff(EQUAL, L"x"), Diff(DELETE, L"\t"), Diff(INSERT, std::wstring(L"\000", 1)));
    assertEquals(L"diff_main: Simple case #3.", diffs, dmp.diff_main(L"ax\t", std::wstring(L"\u0680x\000", 3), false));

    diffs = diffList(Diff(DELETE, L"1"), Diff(EQUAL, L"a"), Diff(DELETE, L"y"), Diff(EQUAL, L"b"), Diff(DELETE, L"2"), Diff(INSERT, L"xab"));
    assertEquals(L"diff_main: Overlap #1.", diffs, dmp.diff_main(L"1ayb2", L"abxab", false));

    diffs = diffList(Diff(INSERT, L"xaxcx"), Diff(EQUAL, L"abc"), Diff(DELETE, L"y"));
    assertEquals(L"diff_main: Overlap #2.", diffs, dmp.diff_main(L"abcy", L"xaxcxabc", false));

    diffs = diffList(Diff(DELETE, L"ABCD"), Diff(EQUAL, L"a"), Diff(DELETE, L"="), Diff(INSERT, L"-"), Diff(EQUAL, L"bcd"), Diff(DELETE, L"="), Diff(INSERT, L"-"), Diff(EQUAL, L"efghijklmnopqrs"), Diff(DELETE, L"EFGHIJKLMNOefg"));
    assertEquals(L"diff_main: Overlap #3.", diffs, dmp.diff_main(L"ABCDa=bcd=efghijklmnopqrsEFGHIJKLMNOefg", L"a-bcd-efghijklmnopqrs", false));

    diffs = diffList(Diff(INSERT, L" "), Diff(EQUAL, L"a"), Diff(INSERT, L"nd"), Diff(EQUAL, L" [[Pennsylvania]]"), Diff(DELETE, L" and [[New"));
    assertEquals(L"diff_main: Large equality.", diffs, dmp.diff_main(L"a [[Pennsylvania]] and [[New", L" and [[Pennsylvania]]", false));

    dmp.Diff_Timeout = 0.1f;  // 100ms
    // This test may 'fail' on extremely fast computers.  If so, just increase the text lengths.
    std::wstring a = L"`Twas brillig, and the slithy toves\nDid gyre and gimble in the wabe:\nAll mimsy were the borogoves,\nAnd the mome raths outgrabe.\n";
    std::wstring b = L"I am the very model of a modern major general,\nI've information vegetable, animal, and mineral,\nI know the kings of England, and I quote the fights historical,\nFrom Marathon to Waterloo, in order categorical.\n";
    // Increase the text lengths by 1024 times to ensure a timeout.
    for (int x = 0; x < 10; x++) {
        a = a + a;
        b = b + b;
    }
    clock_t startTime = clock();
    dmp.diff_main(a, b);
    clock_t endTime = clock();
    // Test that we took at least the timeout period.
    assertTrue(L"diff_main: Timeout min.", dmp.Diff_Timeout * CLOCKS_PER_SEC <= endTime - startTime);
    // Test that we didn't take forever (be forgiving).
    // Theoretically this test could fail very occasionally if the
    // OS task swaps or locks up for a second at the wrong moment.
    // Java seems to overrun by ~80% (compared with 10% for other languages).
    // Therefore use an upper limit of 0.5s instead of 0.2s.
    assertTrue(L"diff_main: Timeout max.", dmp.Diff_Timeout * CLOCKS_PER_SEC * 2 > endTime - startTime);
    dmp.Diff_Timeout = 0;

    // Test the linemode speedup.
    // Must be long to pass the 100 char cutoff.
    a = L"1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n";
    b = L"abcdefghij\nabcdefghij\nabcdefghij\nabcdefghij\nabcdefghij\nabcdefghij\nabcdefghij\nabcdefghij\nabcdefghij\nabcdefghij\nabcdefghij\nabcdefghij\nabcdefghij\n";
    assertEquals(L"diff_main: Simple line-mode.", dmp.diff_main(a, b, true), dmp.diff_main(a, b, false));

    a = L"1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890";
    b = L"abcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghij";
    assertEquals(L"diff_main: Single line-mode.", dmp.diff_main(a, b, true), dmp.diff_main(a, b, false));

    a = L"1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n";
    b = L"abcdefghij\n1234567890\n1234567890\n1234567890\nabcdefghij\n1234567890\n1234567890\n1234567890\nabcdefghij\n1234567890\n1234567890\n1234567890\nabcdefghij\n";
    std::wstring_list texts_linemode = diff_rebuildtexts(dmp.diff_main(a, b, true));
    std::wstring_list texts_textmode = diff_rebuildtexts(dmp.diff_main(a, b, false));
    assertEquals(L"diff_main: Overlap line-mode.", texts_textmode, texts_linemode);

    /*invalid case
    // Test null inputs.
    try {
        dmp.diff_main(NULL, NULL);
        assertFalse(L"diff_main: Null inputs.", true);
    } catch (const char* ex) {
        // Exception expected.
    }
    */
}


//  MATCH TEST FUNCTIONS


void diff_match_patch_test::testMatchAlphabet() {
    // Initialise the bitmasks for Bitap.
    std::unordered_map<wchar_t, int> bitmask;
    bitmask.emplace('a', 4);
    bitmask.emplace('b', 2);
    bitmask.emplace('c', 1);
    assertEquals(L"match_alphabet: Unique.", bitmask, dmp.match_alphabet(L"abc"));

    bitmask.clear();
    bitmask.emplace('a', 37);
    bitmask.emplace('b', 18);
    bitmask.emplace('c', 8);
    assertEquals(L"match_alphabet: Duplicates.", bitmask, dmp.match_alphabet(L"abcaba"));
}

void diff_match_patch_test::testMatchBitap() {
    // Bitap algorithm.
    dmp.Match_Distance = 100;
    dmp.Match_Threshold = 0.5f;
    assertEquals(L"match_bitap: Exact match #1.", 5, dmp.match_bitap(L"abcdefghijk", L"fgh", 5));

    assertEquals(L"match_bitap: Exact match #2.", 5, dmp.match_bitap(L"abcdefghijk", L"fgh", 0));

    assertEquals(L"match_bitap: Fuzzy match #1.", 4, dmp.match_bitap(L"abcdefghijk", L"efxhi", 0));

    assertEquals(L"match_bitap: Fuzzy match #2.", 2, dmp.match_bitap(L"abcdefghijk", L"cdefxyhijk", 5));

    assertEquals(L"match_bitap: Fuzzy match #3.", -1, dmp.match_bitap(L"abcdefghijk", L"bxy", 1));

    assertEquals(L"match_bitap: Overflow.", 2, dmp.match_bitap(L"123456789xx0", L"3456789x0", 2));

    assertEquals(L"match_bitap: Before start match.", 0, dmp.match_bitap(L"abcdef", L"xxabc", 4));

    assertEquals(L"match_bitap: Beyond end match.", 3, dmp.match_bitap(L"abcdef", L"defyy", 4));

    assertEquals(L"match_bitap: Oversized pattern.", 0, dmp.match_bitap(L"abcdef", L"xabcdefy", 0));

    dmp.Match_Threshold = 0.4f;
    assertEquals(L"match_bitap: Threshold #1.", 4, dmp.match_bitap(L"abcdefghijk", L"efxyhi", 1));

    dmp.Match_Threshold = 0.3f;
    assertEquals(L"match_bitap: Threshold #2.", -1, dmp.match_bitap(L"abcdefghijk", L"efxyhi", 1));

    dmp.Match_Threshold = 0.0f;
    assertEquals(L"match_bitap: Threshold #3.", 1, dmp.match_bitap(L"abcdefghijk", L"bcdef", 1));

    dmp.Match_Threshold = 0.5f;
    assertEquals(L"match_bitap: Multiple select #1.", 0, dmp.match_bitap(L"abcdexyzabcde", L"abccde", 3));

    assertEquals(L"match_bitap: Multiple select #2.", 8, dmp.match_bitap(L"abcdexyzabcde", L"abccde", 5));

    dmp.Match_Distance = 10;  // Strict location.
    assertEquals(L"match_bitap: Distance test #1.", -1, dmp.match_bitap(L"abcdefghijklmnopqrstuvwxyz", L"abcdefg", 24));

    assertEquals(L"match_bitap: Distance test #2.", 0, dmp.match_bitap(L"abcdefghijklmnopqrstuvwxyz", L"abcdxxefg", 1));

    dmp.Match_Distance = 1000;  // Loose location.
    assertEquals(L"match_bitap: Distance test #3.", 0, dmp.match_bitap(L"abcdefghijklmnopqrstuvwxyz", L"abcdefg", 24));
}

void diff_match_patch_test::testMatchMain() {
    // Full match.
    assertEquals(L"match_main: Equality.", 0, dmp.match_main(L"abcdef", L"abcdef", 1000));

    assertEquals(L"match_main: Null text.", -1, dmp.match_main(L"", L"abcdef", 1));

    assertEquals(L"match_main: Null pattern.", 3, dmp.match_main(L"abcdef", L"", 3));

    assertEquals(L"match_main: Exact match.", 3, dmp.match_main(L"abcdef", L"de", 3));

    dmp.Match_Threshold = 0.7f;
    assertEquals(L"match_main: Complex match.", 4, dmp.match_main(L"I am the very model of a modern major general.", L" that berry ", 5));
    dmp.Match_Threshold = 0.5f;

    /* invalid case for std::wstring
    // Test null inputs.
    try {
        dmp.match_main(NULL, NULL, 0);
        assertFalse(L"match_main: Null inputs.", true);
    } catch (const char* ex) {
        // Exception expected.
    }
    */
}


//  PATCH TEST FUNCTIONS


void diff_match_patch_test::testPatchObj() {
    // Patch Object.
    Patch p;
    p.start1 = 20;
    p.start2 = 21;
    p.length1 = 18;
    p.length2 = 17;
    p.diffs = diffList(Diff(EQUAL, L"jump"), Diff(DELETE, L"s"), Diff(INSERT, L"ed"), Diff(EQUAL, L" over "), Diff(DELETE, L"the"), Diff(INSERT, L"a"), Diff(EQUAL, L"\nlaz"));
    std::wstring strp = L"@@ -21,18 +22,17 @@\n jump\n-s\n+ed\n  over \n-the\n+a\n %0Alaz\n";
    assertEquals(L"Patch: toString.", strp, p.toString());
}

void diff_match_patch_test::testPatchFromText() {
    assertTrue(L"patch_fromText: #0.", dmp.patch_fromText(L"").size() == 0);

    std::wstring strp = L"@@ -21,18 +22,17 @@\n jump\n-s\n+ed\n  over \n-the\n+a\n %0Alaz\n";
    assertEquals(L"patch_fromText: #1.", strp, dmp.patch_fromText(strp).begin()->toString());

    assertEquals(L"patch_fromText: #2.", L"@@ -1 +1 @@\n-a\n+b\n", dmp.patch_fromText(L"@@ -1 +1 @@\n-a\n+b\n").begin()->toString());

    assertEquals(L"patch_fromText: #3.", L"@@ -1,3 +0,0 @@\n-abc\n", dmp.patch_fromText(L"@@ -1,3 +0,0 @@\n-abc\n").begin()->toString());

    assertEquals(L"patch_fromText: #4.", L"@@ -0,0 +1,3 @@\n+abc\n", dmp.patch_fromText(L"@@ -0,0 +1,3 @@\n+abc\n").begin()->toString());

    assertEquals(L"patch_fromText: #5.", L"@@ -21,4 +21,10 @@\n-jump\n+somersault\n", dmp.patch_fromText(L"@@ -21,4 +21,10 @@\n-jump\n+somersault\n").front().toString());

    // Generates error.
    try {
        dmp.patch_fromText(L"Bad\nPatch\n");
        assertFalse(L"patch_fromText: #5.", true);
    } catch (std::wstring ex) {
        // Exception expected.
    }
}

void diff_match_patch_test::testPatchToText() {
    std::wstring strp = L"@@ -21,18 +22,17 @@\n jump\n-s\n+ed\n  over \n-the\n+a\n  laz\n";
    std::list<Patch> patches;
    patches = dmp.patch_fromText(strp);
    assertEquals(L"patch_toText: Single", strp, dmp.patch_toText(patches));

    strp = L"@@ -1,9 +1,9 @@\n-f\n+F\n oo+fooba\n@@ -7,9 +7,9 @@\n obar\n-,\n+.\n  tes\n";
    patches = dmp.patch_fromText(strp);
    assertEquals(L"patch_toText: Dual", strp, dmp.patch_toText(patches));
}

void diff_match_patch_test::testPatchAddContext() {
    dmp.Patch_Margin = 4;
    Patch p = dmp.patch_fromText(L"@@ -21,4 +21,10 @@\n-jump\n+somersault\n").front();
    dmp.patch_addContext(p, L"The quick brown fox jumps over the lazy dog.");
    assertEquals(L"patch_addContext: Simple case.", L"@@ -17,12 +17,18 @@\n fox \n-jump\n+somersault\n s ov\n", p.toString());

    p = dmp.patch_fromText(L"@@ -21,4 +21,10 @@\n-jump\n+somersault\n").front();
    dmp.patch_addContext(p, L"The quick brown fox jumps.");
    assertEquals(L"patch_addContext: Not enough trailing context.", L"@@ -17,10 +17,16 @@\n fox \n-jump\n+somersault\n s.\n", p.toString());

    p = dmp.patch_fromText(L"@@ -3 +3,2 @@\n-e\n+at\n").front();
    dmp.patch_addContext(p, L"The quick brown fox jumps.");
    assertEquals(L"patch_addContext: Not enough leading context.", L"@@ -1,7 +1,8 @@\n Th\n-e\n+at\n  qui\n", p.toString());

    p = dmp.patch_fromText(L"@@ -3 +3,2 @@\n-e\n+at\n").front();
    dmp.patch_addContext(p, L"The quick brown fox jumps.  The quick brown fox crashes.");
    assertEquals(L"patch_addContext: Ambiguity.", L"@@ -1,27 +1,28 @@\n Th\n-e\n+at\n  quick brown fox jumps. \n", p.toString());
}

void diff_match_patch_test::testPatchMake() {
    std::list<Patch> patches;
    patches = dmp.patch_make(L"", L"");
    assertEquals(L"patch_make: Null case", L"", dmp.patch_toText(patches));

    std::wstring text1 = L"The quick brown fox jumps over the lazy dog.";
    std::wstring text2 = L"That quick brown fox jumped over a lazy dog.";
    std::wstring expectedPatch = L"@@ -1,8 +1,7 @@\n Th\n-at\n+e\n  qui\n@@ -21,17 +21,18 @@\n jump\n-ed\n+s\n  over \n-a\n+the\n  laz\n";
    // The second patch must be "-21,17 +21,18", not "-22,17 +21,18" due to rolling context.
    patches = dmp.patch_make(text2, text1);
    assertEquals(L"patch_make: Text2+Text1 inputs", expectedPatch, dmp.patch_toText(patches));

    expectedPatch = L"@@ -1,11 +1,12 @@\n Th\n-e\n+at\n  quick b\n@@ -22,18 +22,17 @@\n jump\n-s\n+ed\n  over \n-the\n+a\n  laz\n";
    patches = dmp.patch_make(text1, text2);
    assertEquals(L"patch_make: Text1+Text2 inputs", expectedPatch, dmp.patch_toText(patches));

    std::list<Diff> diffs = dmp.diff_main(text1, text2, false);
    patches = dmp.patch_make(diffs);
    assertEquals(L"patch_make: Diff input", expectedPatch, dmp.patch_toText(patches));

    patches = dmp.patch_make(text1, diffs);
    assertEquals(L"patch_make: Text1+Diff inputs", expectedPatch, dmp.patch_toText(patches));

    patches = dmp.patch_make(text1, text2, diffs);
    assertEquals(L"patch_make: Text1+Text2+Diff inputs (deprecated)", expectedPatch, dmp.patch_toText(patches));

    patches = dmp.patch_make(L"`1234567890-=[]\\;',./", L"~!@#$%^&*()_+{}|:\"<>?");
    assertEquals(L"patch_toText: Character encoding.", L"@@ -1,21 +1,21 @@\n-%601234567890-=%5B%5D%5C;',./\n+~!@#$%25%5E&*()_+%7B%7D%7C:%22%3C%3E?\n", dmp.patch_toText(patches));

    diffs = diffList(Diff(DELETE, L"`1234567890-=[]\\;',./"), Diff(INSERT, L"~!@#$%^&*()_+{}|:\"<>?"));
    assertEquals(L"patch_fromText: Character decoding.", diffs, dmp.patch_fromText(L"@@ -1,21 +1,21 @@\n-%601234567890-=%5B%5D%5C;',./\n+~!@#$%25%5E&*()_+%7B%7D%7C:%22%3C%3E?\n").front().diffs);

    text1 = L"";
    for (int x = 0; x < 100; x++) {
        text1 += L"abcdef";
    }
    text2 = text1 + L"123";
    expectedPatch = L"@@ -573,28 +573,31 @@\n cdefabcdefabcdefabcdefabcdef\n+123\n";
    patches = dmp.patch_make(text1, text2);
    assertEquals(L"patch_make: Long string with repeats.", expectedPatch, dmp.patch_toText(patches));

    // Test null inputs.
    /* invalid case
    try {
        dmp.patch_make(NULL, NULL);
        assertFalse(L"patch_make: Null inputs.", true);
    } catch (const char* ex) {
        // Exception expected.
    }
    */
}

void diff_match_patch_test::testPatchSplitMax() {
    // Assumes that Match_MaxBits is 32.
    std::list<Patch> patches;
    patches = dmp.patch_make(L"abcdefghijklmnopqrstuvwxyz01234567890", L"XabXcdXefXghXijXklXmnXopXqrXstXuvXwxXyzX01X23X45X67X89X0");
    dmp.patch_splitMax(patches);
    assertEquals(L"patch_splitMax: #1.", L"@@ -1,32 +1,46 @@\n+X\n ab\n+X\n cd\n+X\n ef\n+X\n gh\n+X\n ij\n+X\n kl\n+X\n mn\n+X\n op\n+X\n qr\n+X\n st\n+X\n uv\n+X\n wx\n+X\n yz\n+X\n 012345\n@@ -25,13 +39,18 @@\n zX01\n+X\n 23\n+X\n 45\n+X\n 67\n+X\n 89\n+X\n 0\n", dmp.patch_toText(patches));

    patches = dmp.patch_make(L"abcdef1234567890123456789012345678901234567890123456789012345678901234567890uvwxyz", L"abcdefuvwxyz");
    std::wstring oldToText = dmp.patch_toText(patches);
    dmp.patch_splitMax(patches);
    assertEquals(L"patch_splitMax: #2.", oldToText, dmp.patch_toText(patches));

    patches = dmp.patch_make(L"1234567890123456789012345678901234567890123456789012345678901234567890", L"abc");
    dmp.patch_splitMax(patches);
    assertEquals(L"patch_splitMax: #3.", L"@@ -1,32 +1,4 @@\n-1234567890123456789012345678\n 9012\n@@ -29,32 +1,4 @@\n-9012345678901234567890123456\n 7890\n@@ -57,14 +1,3 @@\n-78901234567890\n+abc\n", dmp.patch_toText(patches));

    patches = dmp.patch_make(L"abcdefghij , h : 0 , t : 1 abcdefghij , h : 0 , t : 1 abcdefghij , h : 0 , t : 1", L"abcdefghij , h : 1 , t : 1 abcdefghij , h : 1 , t : 1 abcdefghij , h : 0 , t : 1");
    dmp.patch_splitMax(patches);
    assertEquals(L"patch_splitMax: #4.", L"@@ -2,32 +2,32 @@\n bcdefghij , h : \n-0\n+1\n  , t : 1 abcdef\n@@ -29,32 +29,32 @@\n bcdefghij , h : \n-0\n+1\n  , t : 1 abcdef\n", dmp.patch_toText(patches));
}

void diff_match_patch_test::testPatchAddPadding() {
    std::list<Patch> patches;
    patches = dmp.patch_make(L"", L"test");
    assertEquals(L"patch_addPadding: Both edges full.", L"@@ -0,0 +1,4 @@\n+test\n", dmp.patch_toText(patches));
    dmp.patch_addPadding(patches);
    assertEquals(L"patch_addPadding: Both edges full.", L"@@ -1,8 +1,12 @@\n %01%02%03%04\n+test\n %01%02%03%04\n", dmp.patch_toText(patches));

    patches = dmp.patch_make(L"XY", L"XtestY");
    assertEquals(L"patch_addPadding: Both edges partial.", L"@@ -1,2 +1,6 @@\n X\n+test\n Y\n", dmp.patch_toText(patches));
    dmp.patch_addPadding(patches);
    assertEquals(L"patch_addPadding: Both edges partial.", L"@@ -2,8 +2,12 @@\n %02%03%04X\n+test\n Y%01%02%03\n", dmp.patch_toText(patches));

    patches = dmp.patch_make(L"XXXXYYYY", L"XXXXtestYYYY");
    assertEquals(L"patch_addPadding: Both edges none.", L"@@ -1,8 +1,12 @@\n XXXX\n+test\n YYYY\n", dmp.patch_toText(patches));
    dmp.patch_addPadding(patches);
    assertEquals(L"patch_addPadding: Both edges none.", L"@@ -5,8 +5,12 @@\n XXXX\n+test\n YYYY\n", dmp.patch_toText(patches));
}

void diff_match_patch_test::testPatchApply() {
    dmp.Match_Distance = 1000;
    dmp.Match_Threshold = 0.5f;
    dmp.Patch_DeleteThreshold = 0.5f;
    std::list<Patch> patches;
    patches = dmp.patch_make(L"", L"");
    std::pair<std::wstring, std::vector<bool> > results = dmp.patch_apply(patches, L"Hello world.");
    std::vector<bool> boolArray = results.second;

    std::wstring resultStr = std::format(L"%ls\t%d", std::get<0>(results).c_str(), boolArray.size());
    assertEquals(L"patch_apply: Null case.", L"Hello world.\t0", resultStr);

    patches = dmp.patch_make(L"The quick brown fox jumps over the lazy dog.", L"That quick brown fox jumped over a lazy dog.");
    results = dmp.patch_apply(patches, L"The quick brown fox jumps over the lazy dog.");
    boolArray = results.second;
    resultStr = results.first + L"\t" + (boolArray[0] ? L"true" : L"false") + L"\t" + (boolArray[1] ? L"true" : L"false");
    assertEquals(L"patch_apply: Exact match.", L"That quick brown fox jumped over a lazy dog.\ttrue\ttrue", resultStr);

    results = dmp.patch_apply(patches, L"The quick red rabbit jumps over the tired tiger.");
    boolArray = results.second;
    resultStr = results.first + L"\t" + (boolArray[0] ? L"true" : L"false") + L"\t" + (boolArray[1] ? L"true" : L"false");
    assertEquals(L"patch_apply: Partial match.", L"That quick red rabbit jumped over a tired tiger.\ttrue\ttrue", resultStr);

    results = dmp.patch_apply(patches, L"I am the very model of a modern major general.");
    boolArray = results.second;
    resultStr = results.first + L"\t" + (boolArray[0] ? L"true" : L"false") + L"\t" + (boolArray[1] ? L"true" : L"false");
    assertEquals(L"patch_apply: Failed match.", L"I am the very model of a modern major general.\tfalse\tfalse", resultStr);

    patches = dmp.patch_make(L"x1234567890123456789012345678901234567890123456789012345678901234567890y", L"xabcy");
    results = dmp.patch_apply(patches, L"x123456789012345678901234567890-----++++++++++-----123456789012345678901234567890y");
    boolArray = results.second;
    resultStr = results.first + L"\t" + (boolArray[0] ? L"true" : L"false") + L"\t" + (boolArray[1] ? L"true" : L"false");
    assertEquals(L"patch_apply: Big delete, small change.", L"xabcy\ttrue\ttrue", resultStr);

    patches = dmp.patch_make(L"x1234567890123456789012345678901234567890123456789012345678901234567890y", L"xabcy");
    results = dmp.patch_apply(patches, L"x12345678901234567890---------------++++++++++---------------12345678901234567890y");
    boolArray = results.second;
    resultStr = results.first + L"\t" + (boolArray[0] ? L"true" : L"false") + L"\t" + (boolArray[1] ? L"true" : L"false");
    assertEquals(L"patch_apply: Big delete, large change 1.", L"xabc12345678901234567890---------------++++++++++---------------12345678901234567890y\tfalse\ttrue", resultStr);

    dmp.Patch_DeleteThreshold = 0.6f;
    patches = dmp.patch_make(L"x1234567890123456789012345678901234567890123456789012345678901234567890y", L"xabcy");
    results = dmp.patch_apply(patches, L"x12345678901234567890---------------++++++++++---------------12345678901234567890y");
    boolArray = results.second;
    resultStr = results.first + L"\t" + (boolArray[0] ? L"true" : L"false") + L"\t" + (boolArray[1] ? L"true" : L"false");
    assertEquals(L"patch_apply: Big delete, large change 2.", L"xabcy\ttrue\ttrue", resultStr);
    dmp.Patch_DeleteThreshold = 0.5f;

    dmp.Match_Threshold = 0.0f;
    dmp.Match_Distance = 0;
    patches = dmp.patch_make(L"abcdefghijklmnopqrstuvwxyz--------------------1234567890", L"abcXXXXXXXXXXdefghijklmnopqrstuvwxyz--------------------1234567YYYYYYYYYY890");
    results = dmp.patch_apply(patches, L"ABCDEFGHIJKLMNOPQRSTUVWXYZ--------------------1234567890");
    boolArray = results.second;
    resultStr = results.first + L"\t" + (boolArray[0] ? L"true" : L"false") + L"\t" + (boolArray[1] ? L"true" : L"false");
    assertEquals(L"patch_apply: Compensate for failed patch.", L"ABCDEFGHIJKLMNOPQRSTUVWXYZ--------------------1234567YYYYYYYYYY890\tfalse\ttrue", resultStr);
    dmp.Match_Threshold = 0.5f;
    dmp.Match_Distance = 1000;

    patches = dmp.patch_make(L"", L"test");
    std::wstring patchStr = dmp.patch_toText(patches);
    dmp.patch_apply(patches, L"");
    assertEquals(L"patch_apply: No side effects.", patchStr, dmp.patch_toText(patches));

    patches = dmp.patch_make(L"The quick brown fox jumps over the lazy dog.", L"Woof");
    patchStr = dmp.patch_toText(patches);
    dmp.patch_apply(patches, L"The quick brown fox jumps over the lazy dog.");
    assertEquals(L"patch_apply: No side effects with major delete.", patchStr, dmp.patch_toText(patches));

    patches = dmp.patch_make(L"", L"test");
    results = dmp.patch_apply(patches, L"");
    boolArray = results.second;
    resultStr = results.first + L"\t" + (boolArray[0] ? L"true" : L"false");
    assertEquals(L"patch_apply: Edge exact match.", L"test\ttrue", resultStr);

    patches = dmp.patch_make(L"XY", L"XtestY");
    results = dmp.patch_apply(patches, L"XY");
    boolArray = results.second;
    resultStr = results.first + L"\t" + (boolArray[0] ? L"true" : L"false");
    assertEquals(L"patch_apply: Near edge exact match.", L"XtestY\ttrue", resultStr);

    patches = dmp.patch_make(L"y", L"y123");
    results = dmp.patch_apply(patches, L"x");
    boolArray = results.second;
    resultStr = results.first + L"\t" + (boolArray[0] ? L"true" : L"false");
    assertEquals(L"patch_apply: Edge partial match.", L"x123\ttrue", resultStr);
}


void diff_match_patch_test::assertEquals(const std::wstring &strCase, int n1, int n2) {
    if (n1 != n2) {
        std::debug_print(L"%ls FAIL\nExpected: %d\nActual: %d", qPrintable(strCase), n1, n2);
        throw strCase;
    }
    std::debug_print(L"%ls OK", qPrintable(strCase));
}

void diff_match_patch_test::assertEquals(const std::wstring &strCase, const std::wstring &s1, const std::wstring &s2) {
    if (s1 != s2) {
        std::debug_print(L"%ls FAIL\nExpected: %ls\nActual: %ls",
                         qPrintable(strCase), qPrintable(s1), qPrintable(s2));
        throw strCase;
    }
    std::debug_print(L"%ls OK", qPrintable(strCase));
}

void diff_match_patch_test::assertEquals(const std::wstring &strCase, const Diff &d1, const Diff &d2) {
    if (d1 != d2) {
        std::debug_print(L"%ls FAIL\nExpected: %ls\nActual: %ls", qPrintable(strCase),
                         qPrintable(d1.toString()), qPrintable(d2.toString()));
        throw strCase;
    }
    std::debug_print(L"%ls OK", qPrintable(strCase));
}

void diff_match_patch_test::assertEquals(const std::wstring &strCase, const std::list<Diff> &list1, const std::list<Diff> &list2) {
    bool fail = false;
    if (list1.size() == list2.size()) {
        int i = 0;
        for(const Diff & d1: list1) {
            auto it = list2.begin();
            std::advance(it, i);
            const Diff & d2 = *it;
            if (d1 != d2) {
                fail = true;
                break;
            }
            i++;
        }
    } else {
        fail = true;
    }

    if (fail) {
        // Build human readable description of both lists.
        std::wstring listString1 = L"(";
        bool first = true;
        for(Diff d1: list1) {
            if (!first) {
                listString1 += L", ";
            }
            listString1 += d1.toString();
            first = false;
        }
        listString1 += L")";
        std::wstring listString2 = L"(";
        first = true;
        for(Diff d2: list2) {
            if (!first) {
                listString2 += L", ";
            }
            listString2 += d2.toString();
            first = false;
        }
        listString2 += L")";
        std::debug_print(L"%ls FAIL\nExpected: %ls\nActual: %ls",
                         qPrintable(strCase), qPrintable(listString1), qPrintable(listString2));
        throw strCase;
    }
    std::debug_print(L"%ls OK", qPrintable(strCase));
}

void diff_match_patch_test::assertEquals(const std::wstring &strCase, const std::list<std::dmp_variant> &list1, const std::list<std::dmp_variant> &list2) {
    bool fail = false;
    if (list1.size() == list2.size()) {
        int i = 0;
        for(const auto & q1: list1) {
            auto it = list2.begin();
            std::advance(it, i);

            const auto & q2 = *it;
            if (q1 != q2) {
                std::debug_print(L"variable %d not equal, (%ls)_____________ (%ls)", i, qPrintable(var_to_string(q1)), qPrintable(var_to_string(q2)));
                fail = true;
                break;
            }
            i++;
        }
    } else {
        fail = true;
    }

    if (fail) {
        // Build human readable description of both lists.
        std::wstring listString1 = L"(";
        bool first = true;
        for(std::dmp_variant q1: list1) {
            if (!first) {
                listString1 += L", ";
            }
            listString1 += var_to_string(q1);
            first = false;
        }
        listString1 += L")";
        std::wstring listString2 = L"(";
        first = true;
        for(std::dmp_variant q2: list2) {
            if (!first) {
                listString2 += L", ";
            }
            listString2 += var_to_string(q2);
            first = false;
        }
        listString2 += L")";
        std::debug_print(L"%ls FAIL\nExpected: %ls\nActual: %ls",
                         qPrintable(strCase), qPrintable(listString1), qPrintable(listString2));
        throw strCase;
    }
    std::debug_print(L"%ls OK", qPrintable(strCase));
}

void diff_match_patch_test::assertEquals(const std::wstring &strCase, const std::dmp_variant &var1, const std::dmp_variant &var2) {
    if (var1 != var2) {
        std::debug_print(L"%ls FAIL\nExpected: %ls\nActual: %ls", qPrintable(strCase),
                         qPrintable(var_to_string(var1)), qPrintable(var_to_string(var2)));
        throw strCase;
    }
    std::debug_print(L"%ls OK", qPrintable(strCase));
}

void diff_match_patch_test::assertEquals(const std::wstring &strCase, const std::unordered_map<wchar_t, int> &m1, const std::unordered_map<wchar_t, int> &m2) {
    auto i1 = m1.begin(), i2 = m2.begin();

    while (i1 != m1.end() && i2 != m2.end()) {
        if (i1->first != i2->first || i1->second != i2->second) {
            std::debug_print(L"%ls FAIL\nExpected: (%c, %d)\nActual: (%c, %d)", qPrintable(strCase),
                             i1->first, i1->second, i2->first, i2->second);
            throw strCase;
        }

        i1++;
        i2++;
    }

    if (i1 != m1.end()) {
        std::debug_print(L"%ls FAIL\nExpected: (%c, %d)\nActual: none",
                         qPrintable(strCase), i1->first, i1->second);
        throw strCase;
    }
    if (i2 != m2.end()) {
        std::debug_print(L"%ls FAIL\nExpected: none\nActual: (%c, %d)",
                         qPrintable(strCase), i2->first, i2->second);
        throw strCase;
    }
    std::debug_print(L"%ls OK", qPrintable(strCase));
}

void diff_match_patch_test::assertEquals(const std::wstring &strCase, const std::wstring_list &list1, const std::wstring_list &list2) {
    if (list1 != list2) {
        std::debug_print(L"%ls FAIL\nExpected: %ls\nActual: %ls", qPrintable(strCase),
                         qPrintable(std::join(list1, L",")), qPrintable(std::join(list2, L",")));
        throw strCase;
    }
    std::debug_print(L"%ls OK", qPrintable(strCase));
}

void diff_match_patch_test::assertTrue(const std::wstring &strCase, bool value) {
    if (!value) {
        std::debug_print(L"%ls FAIL\nExpected: true\nActual: false", qPrintable(strCase));
        throw strCase;
    }
    std::debug_print(L"%ls OK", qPrintable(strCase));
}

void diff_match_patch_test::assertFalse(const std::wstring &strCase, bool value) {
    if (value) {
        std::debug_print(L"%ls FAIL\nExpected: false\nActual: true", qPrintable(strCase));
        throw strCase;
    }
    std::debug_print(L"%ls OK", qPrintable(strCase));
}


// Construct the two texts which made up the diff originally.
std::wstring_list diff_match_patch_test::diff_rebuildtexts(std::list<Diff> diffs) {
    std::wstring_list text;
    text.push_back(std::wstring(L""));
    text.push_back(std::wstring(L""));
    for (Diff myDiff: diffs) {
        if (myDiff.operation != INSERT) {
            text[0] += myDiff.text;
        }
        if (myDiff.operation != DELETE) {
            text[1] += myDiff.text;
        }
    }
    return text;
}

void diff_match_patch_test::assertEmpty(const std::wstring &strCase, const std::wstring_list &list) {
    if (list.size() != 0) {
        throw strCase;
    }
}


// Private function for quickly building lists of diffs.
std::list<Diff> diff_match_patch_test::diffList(Diff d1, Diff d2, Diff d3, Diff d4, Diff d5,
                                                  Diff d6, Diff d7, Diff d8, Diff d9, Diff d10) {
    // Diff(INSERT, NULL) is invalid and thus is used as the default argument.
    std::list<Diff> listRet;
    if (d1.operation == INSERT && d1.invalid) {
        return listRet;
    }
    listRet.push_back(d1);

    if (d2.operation == INSERT && d2.invalid) {
        return listRet;
    }
    listRet.push_back(d2);

    if (d3.operation == INSERT && d3.invalid) {
        return listRet;
    }
    listRet.push_back(d3);

    if (d4.operation == INSERT && d4.invalid) {
        return listRet;
    }
    listRet.push_back(d4);

    if (d5.operation == INSERT && d5.invalid) {
        return listRet;
    }
    listRet.push_back(d5);

    if (d6.operation == INSERT && d6.invalid) {
        return listRet;
    }
    listRet.push_back(d6);

    if (d7.operation == INSERT && d7.invalid) {
        return listRet;
    }
    listRet.push_back(d7);

    if (d8.operation == INSERT && d8.invalid) {
        return listRet;
    }
    listRet.push_back(d8);

    if (d9.operation == INSERT && d9.invalid) {
        return listRet;
    }
    listRet.push_back(d9);

    if (d10.operation == INSERT && d10.invalid) {
        return listRet;
    }
    listRet.push_back(d10);

    return listRet;
}


/*
  Compile instructions for MinGW and QT4 on Windows:
  qmake -project
  qmake
  mingw32-make
  g++ -o diff_match_patch_test debug\diff_match_patch_test.o debug\diff_match_patch.o \qt4\lib\libQtCore4.a
  diff_match_patch_test.exe

  Compile insructions for OS X:
  qmake -spec macx-g++
  make
  ./diff_match_patch
*/
