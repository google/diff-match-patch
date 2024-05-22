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

/*
 * To compile with Mono:
 *   mcs DiffMatchPatchTest.cs ../DiffMatchPatch.cs
 * To run with Mono:
 *   mono DiffMatchPatchTest.exe
*/

using DiffMatchPatch;
using System.Collections.Generic;
using System;
using System.Text;

public class diff_match_patchTest : diff_match_patch {
  public void diff_commonPrefixTest() {
    // Detect any common suffix.
    assertEquals("diff_commonPrefix: Null case.", 0, this.diff_commonPrefix("abc", "xyz"));

    assertEquals("diff_commonPrefix: Non-null case.", 4, this.diff_commonPrefix("1234abcdef", "1234xyz"));

    assertEquals("diff_commonPrefix: Whole case.", 4, this.diff_commonPrefix("1234", "1234xyz"));
  }

  public void diff_commonSuffixTest() {
    // Detect any common suffix.
    assertEquals("diff_commonSuffix: Null case.", 0, this.diff_commonSuffix("abc", "xyz"));

    assertEquals("diff_commonSuffix: Non-null case.", 4, this.diff_commonSuffix("abcdef1234", "xyz1234"));

    assertEquals("diff_commonSuffix: Whole case.", 4, this.diff_commonSuffix("1234", "xyz1234"));
  }

  public void diff_commonOverlapTest() {
    // Detect any suffix/prefix overlap.
    assertEquals("diff_commonOverlap: Null case.", 0, this.diff_commonOverlap("", "abcd"));

    assertEquals("diff_commonOverlap: Whole case.", 3, this.diff_commonOverlap("abc", "abcd"));

    assertEquals("diff_commonOverlap: No overlap.", 0, this.diff_commonOverlap("123456", "abcd"));

    assertEquals("diff_commonOverlap: Overlap.", 3, this.diff_commonOverlap("123456xxx", "xxxabcd"));

    // Some overly clever languages (C#) may treat ligatures as equal to their
    // component letters.  E.g. U+FB01 == 'fi'
    assertEquals("diff_commonOverlap: Unicode.", 0, this.diff_commonOverlap("fi", "\ufb01i"));
  }

  public void diff_halfmatchTest() {
    this.Diff_Timeout = 1;
    assertNull("diff_halfMatch: No match #1.", this.diff_halfMatch("1234567890", "abcdef"));

    assertNull("diff_halfMatch: No match #2.", this.diff_halfMatch("12345", "23"));

    assertEquals("diff_halfMatch: Single Match #1.", new string[] { "12", "90", "a", "z", "345678" }, this.diff_halfMatch("1234567890", "a345678z"));

    assertEquals("diff_halfMatch: Single Match #2.", new string[] { "a", "z", "12", "90", "345678" }, this.diff_halfMatch("a345678z", "1234567890"));

    assertEquals("diff_halfMatch: Single Match #3.", new string[] { "abc", "z", "1234", "0", "56789" }, this.diff_halfMatch("abc56789z", "1234567890"));

    assertEquals("diff_halfMatch: Single Match #4.", new string[] { "a", "xyz", "1", "7890", "23456" }, this.diff_halfMatch("a23456xyz", "1234567890"));

    assertEquals("diff_halfMatch: Multiple Matches #1.", new string[] { "12123", "123121", "a", "z", "1234123451234" }, this.diff_halfMatch("121231234123451234123121", "a1234123451234z"));

    assertEquals("diff_halfMatch: Multiple Matches #2.", new string[] { "", "-=-=-=-=-=", "x", "", "x-=-=-=-=-=-=-=" }, this.diff_halfMatch("x-=-=-=-=-=-=-=-=-=-=-=-=", "xx-=-=-=-=-=-=-="));

    assertEquals("diff_halfMatch: Multiple Matches #3.", new string[] { "-=-=-=-=-=", "", "", "y", "-=-=-=-=-=-=-=y" }, this.diff_halfMatch("-=-=-=-=-=-=-=-=-=-=-=-=y", "-=-=-=-=-=-=-=yy"));

    // Optimal diff would be -q+x=H-i+e=lloHe+Hu=llo-Hew+y not -qHillo+x=HelloHe-w+Hulloy
    assertEquals("diff_halfMatch: Non-optimal halfmatch.", new string[] { "qHillo", "w", "x", "Hulloy", "HelloHe" }, this.diff_halfMatch("qHilloHelloHew", "xHelloHeHulloy"));

    this.Diff_Timeout = 0;
    assertNull("diff_halfMatch: Optimal no halfmatch.", this.diff_halfMatch("qHilloHelloHew", "xHelloHeHulloy"));
  }

  public void diff_linesToCharsTest() {
    // Convert lines down to characters.
    List<string> tmpVector = new List<string>();
    tmpVector.Add("");
    tmpVector.Add("alpha\n");
    tmpVector.Add("beta\n");
    Object[] result = this.diff_linesToChars("alpha\nbeta\nalpha\n", "beta\nalpha\nbeta\n");
    assertEquals("diff_linesToChars: Shared lines #1.", "\u0001\u0002\u0001", (string)result[0]);
    assertEquals("diff_linesToChars: Shared lines #2.", "\u0002\u0001\u0002", (string)result[1]);
    assertEquals("diff_linesToChars: Shared lines #3.", tmpVector, (List<string>)result[2]);

    tmpVector.Clear();
    tmpVector.Add("");
    tmpVector.Add("alpha\r\n");
    tmpVector.Add("beta\r\n");
    tmpVector.Add("\r\n");
    result = this.diff_linesToChars("", "alpha\r\nbeta\r\n\r\n\r\n");
    assertEquals("diff_linesToChars: Empty string and blank lines #1.", "", (string)result[0]);
    assertEquals("diff_linesToChars: Empty string and blank lines #2.", "\u0001\u0002\u0003\u0003", (string)result[1]);
    assertEquals("diff_linesToChars: Empty string and blank lines #3.", tmpVector, (List<string>)result[2]);

    tmpVector.Clear();
    tmpVector.Add("");
    tmpVector.Add("a");
    tmpVector.Add("b");
    result = this.diff_linesToChars("a", "b");
    assertEquals("diff_linesToChars: No linebreaks #1.", "\u0001", (string)result[0]);
    assertEquals("diff_linesToChars: No linebreaks #2.", "\u0002", (string)result[1]);
    assertEquals("diff_linesToChars: No linebreaks #3.", tmpVector, (List<string>)result[2]);

    // More than 256 to reveal any 8-bit limitations.
    int n = 300;
    tmpVector.Clear();
    StringBuilder lineList = new StringBuilder();
    StringBuilder charList = new StringBuilder();
    for (int i = 1; i < n + 1; i++) {
      tmpVector.Add(i + "\n");
      lineList.Append(i + "\n");
      charList.Append(Convert.ToChar(i));
    }
    assertEquals("Test initialization fail #1.", n, tmpVector.Count);
    string lines = lineList.ToString();
    string chars = charList.ToString();
    assertEquals("Test initialization fail #2.", n, chars.Length);
    tmpVector.Insert(0, "");
    result = this.diff_linesToChars(lines, "");
    assertEquals("diff_linesToChars: More than 256 #1.", chars, (string)result[0]);
    assertEquals("diff_linesToChars: More than 256 #2.", "", (string)result[1]);
    assertEquals("diff_linesToChars: More than 256 #3.", tmpVector, (List<string>)result[2]);
  }

  public void diff_charsToLinesTest() {
    // First check that Diff equality works.
    assertTrue("diff_charsToLines: Equality #1.", new Diff(Operation.EQUAL, "a").Equals(new Diff(Operation.EQUAL, "a")));

    assertEquals("diff_charsToLines: Equality #2.", new Diff(Operation.EQUAL, "a"), new Diff(Operation.EQUAL, "a"));

    // Convert chars up to lines.
    List<Diff> diffs = new List<Diff> {
        new Diff(Operation.EQUAL, "\u0001\u0002\u0001"),
        new Diff(Operation.INSERT, "\u0002\u0001\u0002")};
    List<string> tmpVector = new List<string>();
    tmpVector.Add("");
    tmpVector.Add("alpha\n");
    tmpVector.Add("beta\n");
    this.diff_charsToLines(diffs, tmpVector);
    assertEquals("diff_charsToLines: Shared lines.", new List<Diff> {
        new Diff(Operation.EQUAL, "alpha\nbeta\nalpha\n"),
        new Diff(Operation.INSERT, "beta\nalpha\nbeta\n")}, diffs);

    // More than 256 to reveal any 8-bit limitations.
    int n = 300;
    tmpVector.Clear();
    StringBuilder lineList = new StringBuilder();
    StringBuilder charList = new StringBuilder();
    for (int i = 1; i < n + 1; i++) {
      tmpVector.Add(i + "\n");
      lineList.Append(i + "\n");
      charList.Append(Convert.ToChar (i));
    }
    assertEquals("Test initialization fail #3.", n, tmpVector.Count);
    string lines = lineList.ToString();
    string chars = charList.ToString();
    assertEquals("Test initialization fail #4.", n, chars.Length);
    tmpVector.Insert(0, "");
    diffs = new List<Diff> {new Diff(Operation.DELETE, chars)};
    this.diff_charsToLines(diffs, tmpVector);
    assertEquals("diff_charsToLines: More than 256.", new List<Diff>
        {new Diff(Operation.DELETE, lines)}, diffs);

    // More than 65536 to verify any 16-bit limitation.
    lineList = new StringBuilder();
    for (int i = 0; i < 66000; i++) {
      lineList.Append(i + "\n");
    }
    chars = lineList.ToString();
    Object[] result = this.diff_linesToChars(chars, "");
    diffs = new List<Diff> {new Diff(Operation.INSERT, (string)result[0])};
    this.diff_charsToLines(diffs, (List<string>)result[2]);
    assertEquals("diff_charsToLines: More than 65536.", chars, diffs[0].text);
  }

  public void diff_cleanupMergeTest() {
    // Cleanup a messy diff.
    // Null case.
    List<Diff> diffs = new List<Diff>();
    assertEquals("diff_cleanupMerge: Null case.", new List<Diff>(), diffs);

    diffs = new List<Diff> {new Diff(Operation.EQUAL, "a"), new Diff(Operation.DELETE, "b"), new Diff(Operation.INSERT, "c")};
    this.diff_cleanupMerge(diffs);
    assertEquals("diff_cleanupMerge: No change case.", new List<Diff> {new Diff(Operation.EQUAL, "a"), new Diff(Operation.DELETE, "b"), new Diff(Operation.INSERT, "c")}, diffs);

    diffs = new List<Diff> {new Diff(Operation.EQUAL, "a"), new Diff(Operation.EQUAL, "b"), new Diff(Operation.EQUAL, "c")};
    this.diff_cleanupMerge(diffs);
    assertEquals("diff_cleanupMerge: Merge equalities.", new List<Diff> {new Diff(Operation.EQUAL, "abc")}, diffs);

    diffs = new List<Diff> {new Diff(Operation.DELETE, "a"), new Diff(Operation.DELETE, "b"), new Diff(Operation.DELETE, "c")};
    this.diff_cleanupMerge(diffs);
    assertEquals("diff_cleanupMerge: Merge deletions.", new List<Diff> {new Diff(Operation.DELETE, "abc")}, diffs);

    diffs = new List<Diff> {new Diff(Operation.INSERT, "a"), new Diff(Operation.INSERT, "b"), new Diff(Operation.INSERT, "c")};
    this.diff_cleanupMerge(diffs);
    assertEquals("diff_cleanupMerge: Merge insertions.", new List<Diff> {new Diff(Operation.INSERT, "abc")}, diffs);

    diffs = new List<Diff> {new Diff(Operation.DELETE, "a"), new Diff(Operation.INSERT, "b"), new Diff(Operation.DELETE, "c"), new Diff(Operation.INSERT, "d"), new Diff(Operation.EQUAL, "e"), new Diff(Operation.EQUAL, "f")};
    this.diff_cleanupMerge(diffs);
    assertEquals("diff_cleanupMerge: Merge interweave.", new List<Diff> {new Diff(Operation.DELETE, "ac"), new Diff(Operation.INSERT, "bd"), new Diff(Operation.EQUAL, "ef")}, diffs);

    diffs = new List<Diff> {new Diff(Operation.DELETE, "a"), new Diff(Operation.INSERT, "abc"), new Diff(Operation.DELETE, "dc")};
    this.diff_cleanupMerge(diffs);
    assertEquals("diff_cleanupMerge: Prefix and suffix detection.", new List<Diff> {new Diff(Operation.EQUAL, "a"), new Diff(Operation.DELETE, "d"), new Diff(Operation.INSERT, "b"), new Diff(Operation.EQUAL, "c")}, diffs);

    diffs = new List<Diff> {new Diff(Operation.EQUAL, "x"), new Diff(Operation.DELETE, "a"), new Diff(Operation.INSERT, "abc"), new Diff(Operation.DELETE, "dc"), new Diff(Operation.EQUAL, "y")};
    this.diff_cleanupMerge(diffs);
    assertEquals("diff_cleanupMerge: Prefix and suffix detection with equalities.", new List<Diff> {new Diff(Operation.EQUAL, "xa"), new Diff(Operation.DELETE, "d"), new Diff(Operation.INSERT, "b"), new Diff(Operation.EQUAL, "cy")}, diffs);

    diffs = new List<Diff> {new Diff(Operation.EQUAL, "a"), new Diff(Operation.INSERT, "ba"), new Diff(Operation.EQUAL, "c")};
    this.diff_cleanupMerge(diffs);
    assertEquals("diff_cleanupMerge: Slide edit left.", new List<Diff> {new Diff(Operation.INSERT, "ab"), new Diff(Operation.EQUAL, "ac")}, diffs);

    diffs = new List<Diff> {new Diff(Operation.EQUAL, "c"), new Diff(Operation.INSERT, "ab"), new Diff(Operation.EQUAL, "a")};
    this.diff_cleanupMerge(diffs);
    assertEquals("diff_cleanupMerge: Slide edit right.", new List<Diff> {new Diff(Operation.EQUAL, "ca"), new Diff(Operation.INSERT, "ba")}, diffs);

    diffs = new List<Diff> {new Diff(Operation.EQUAL, "a"), new Diff(Operation.DELETE, "b"), new Diff(Operation.EQUAL, "c"), new Diff(Operation.DELETE, "ac"), new Diff(Operation.EQUAL, "x")};
    this.diff_cleanupMerge(diffs);
    assertEquals("diff_cleanupMerge: Slide edit left recursive.", new List<Diff> {new Diff(Operation.DELETE, "abc"), new Diff(Operation.EQUAL, "acx")}, diffs);

    diffs = new List<Diff> {new Diff(Operation.EQUAL, "x"), new Diff(Operation.DELETE, "ca"), new Diff(Operation.EQUAL, "c"), new Diff(Operation.DELETE, "b"), new Diff(Operation.EQUAL, "a")};
    this.diff_cleanupMerge(diffs);
    assertEquals("diff_cleanupMerge: Slide edit right recursive.", new List<Diff> {new Diff(Operation.EQUAL, "xca"), new Diff(Operation.DELETE, "cba")}, diffs);

    diffs = new List<Diff> {new Diff(Operation.DELETE, "b"), new Diff(Operation.INSERT, "ab"), new Diff(Operation.EQUAL, "c")};
    this.diff_cleanupMerge(diffs);
    assertEquals("diff_cleanupMerge: Empty merge.", new List<Diff> {new Diff(Operation.INSERT, "a"), new Diff(Operation.EQUAL, "bc")}, diffs);

    diffs = new List<Diff> {new Diff(Operation.EQUAL, ""), new Diff(Operation.INSERT, "a"), new Diff(Operation.EQUAL, "b")};
    this.diff_cleanupMerge(diffs);
    assertEquals("diff_cleanupMerge: Empty equality.", new List<Diff> {new Diff(Operation.INSERT, "a"), new Diff(Operation.EQUAL, "b")}, diffs);
  }

  public void diff_cleanupSemanticLosslessTest() {
    // Slide diffs to match logical boundaries.
    List<Diff> diffs = new List<Diff>();
    this.diff_cleanupSemanticLossless(diffs);
    assertEquals("diff_cleanupSemanticLossless: Null case.", new List<Diff>(), diffs);

    diffs = new List<Diff> {
        new Diff(Operation.EQUAL, "AAA\r\n\r\nBBB"),
        new Diff(Operation.INSERT, "\r\nDDD\r\n\r\nBBB"),
        new Diff(Operation.EQUAL, "\r\nEEE")
    };
    this.diff_cleanupSemanticLossless(diffs);
    assertEquals("diff_cleanupSemanticLossless: Blank lines.", new List<Diff> {
        new Diff(Operation.EQUAL, "AAA\r\n\r\n"),
        new Diff(Operation.INSERT, "BBB\r\nDDD\r\n\r\n"),
        new Diff(Operation.EQUAL, "BBB\r\nEEE")}, diffs);

    diffs = new List<Diff> {
        new Diff(Operation.EQUAL, "AAA\r\nBBB"),
        new Diff(Operation.INSERT, " DDD\r\nBBB"),
        new Diff(Operation.EQUAL, " EEE")};
    this.diff_cleanupSemanticLossless(diffs);
    assertEquals("diff_cleanupSemanticLossless: Line boundaries.", new List<Diff> {
        new Diff(Operation.EQUAL, "AAA\r\n"),
        new Diff(Operation.INSERT, "BBB DDD\r\n"),
        new Diff(Operation.EQUAL, "BBB EEE")}, diffs);

    diffs = new List<Diff> {
        new Diff(Operation.EQUAL, "The c"),
        new Diff(Operation.INSERT, "ow and the c"),
        new Diff(Operation.EQUAL, "at.")};
    this.diff_cleanupSemanticLossless(diffs);
    assertEquals("diff_cleanupSemanticLossless: Word boundaries.", new List<Diff> {
        new Diff(Operation.EQUAL, "The "),
        new Diff(Operation.INSERT, "cow and the "),
        new Diff(Operation.EQUAL, "cat.")}, diffs);

    diffs = new List<Diff> {
        new Diff(Operation.EQUAL, "The-c"),
        new Diff(Operation.INSERT, "ow-and-the-c"),
        new Diff(Operation.EQUAL, "at.")};
    this.diff_cleanupSemanticLossless(diffs);
    assertEquals("diff_cleanupSemanticLossless: Alphanumeric boundaries.", new List<Diff> {
        new Diff(Operation.EQUAL, "The-"),
        new Diff(Operation.INSERT, "cow-and-the-"),
        new Diff(Operation.EQUAL, "cat.")}, diffs);

    diffs = new List<Diff> {
        new Diff(Operation.EQUAL, "a"),
        new Diff(Operation.DELETE, "a"),
        new Diff(Operation.EQUAL, "ax")};
    this.diff_cleanupSemanticLossless(diffs);
    assertEquals("diff_cleanupSemanticLossless: Hitting the start.", new List<Diff> {
        new Diff(Operation.DELETE, "a"),
        new Diff(Operation.EQUAL, "aax")}, diffs);

    diffs = new List<Diff> {
        new Diff(Operation.EQUAL, "xa"),
        new Diff(Operation.DELETE, "a"),
        new Diff(Operation.EQUAL, "a")};
    this.diff_cleanupSemanticLossless(diffs);
    assertEquals("diff_cleanupSemanticLossless: Hitting the end.", new List<Diff> {
        new Diff(Operation.EQUAL, "xaa"),
        new Diff(Operation.DELETE, "a")}, diffs);

    diffs = new List<Diff> {
        new Diff(Operation.EQUAL, "The xxx. The "),
        new Diff(Operation.INSERT, "zzz. The "),
        new Diff(Operation.EQUAL, "yyy.")};
    this.diff_cleanupSemanticLossless(diffs);
    assertEquals("diff_cleanupSemanticLossless: Sentence boundaries.", new List<Diff> {
        new Diff(Operation.EQUAL, "The xxx."),
        new Diff(Operation.INSERT, " The zzz."),
        new Diff(Operation.EQUAL, " The yyy.")}, diffs);
  }

  public void diff_cleanupSemanticTest() {
    // Cleanup semantically trivial equalities.
    // Null case.
    List<Diff> diffs = new List<Diff>();
    this.diff_cleanupSemantic(diffs);
    assertEquals("diff_cleanupSemantic: Null case.", new List<Diff>(), diffs);

    diffs = new List<Diff> {
        new Diff(Operation.DELETE, "ab"),
        new Diff(Operation.INSERT, "cd"),
        new Diff(Operation.EQUAL, "12"),
        new Diff(Operation.DELETE, "e")};
    this.diff_cleanupSemantic(diffs);
    assertEquals("diff_cleanupSemantic: No elimination #1.", new List<Diff> {
        new Diff(Operation.DELETE, "ab"),
        new Diff(Operation.INSERT, "cd"),
        new Diff(Operation.EQUAL, "12"),
        new Diff(Operation.DELETE, "e")}, diffs);

    diffs = new List<Diff> {
        new Diff(Operation.DELETE, "abc"),
        new Diff(Operation.INSERT, "ABC"),
        new Diff(Operation.EQUAL, "1234"),
        new Diff(Operation.DELETE, "wxyz")};
    this.diff_cleanupSemantic(diffs);
    assertEquals("diff_cleanupSemantic: No elimination #2.", new List<Diff> {
        new Diff(Operation.DELETE, "abc"),
        new Diff(Operation.INSERT, "ABC"),
        new Diff(Operation.EQUAL, "1234"),
        new Diff(Operation.DELETE, "wxyz")}, diffs);

    diffs = new List<Diff> {
        new Diff(Operation.DELETE, "a"),
        new Diff(Operation.EQUAL, "b"),
        new Diff(Operation.DELETE, "c")};
    this.diff_cleanupSemantic(diffs);
    assertEquals("diff_cleanupSemantic: Simple elimination.", new List<Diff> {
        new Diff(Operation.DELETE, "abc"),
        new Diff(Operation.INSERT, "b")}, diffs);

    diffs = new List<Diff> {
        new Diff(Operation.DELETE, "ab"),
        new Diff(Operation.EQUAL, "cd"),
        new Diff(Operation.DELETE, "e"),
        new Diff(Operation.EQUAL, "f"),
        new Diff(Operation.INSERT, "g")};
    this.diff_cleanupSemantic(diffs);
    assertEquals("diff_cleanupSemantic: Backpass elimination.", new List<Diff> {
        new Diff(Operation.DELETE, "abcdef"),
        new Diff(Operation.INSERT, "cdfg")}, diffs);

    diffs = new List<Diff> {
        new Diff(Operation.INSERT, "1"),
        new Diff(Operation.EQUAL, "A"),
        new Diff(Operation.DELETE, "B"),
        new Diff(Operation.INSERT, "2"),
        new Diff(Operation.EQUAL, "_"),
        new Diff(Operation.INSERT, "1"),
        new Diff(Operation.EQUAL, "A"),
        new Diff(Operation.DELETE, "B"),
        new Diff(Operation.INSERT, "2")};
    this.diff_cleanupSemantic(diffs);
    assertEquals("diff_cleanupSemantic: Multiple elimination.", new List<Diff> {
        new Diff(Operation.DELETE, "AB_AB"),
        new Diff(Operation.INSERT, "1A2_1A2")}, diffs);

    diffs = new List<Diff> {
        new Diff(Operation.EQUAL, "The c"),
        new Diff(Operation.DELETE, "ow and the c"),
        new Diff(Operation.EQUAL, "at.")};
    this.diff_cleanupSemantic(diffs);
    assertEquals("diff_cleanupSemantic: Word boundaries.", new List<Diff> {
        new Diff(Operation.EQUAL, "The "),
        new Diff(Operation.DELETE, "cow and the "),
        new Diff(Operation.EQUAL, "cat.")}, diffs);

    diffs = new List<Diff> {
        new Diff(Operation.DELETE, "abcxx"),
        new Diff(Operation.INSERT, "xxdef")};
    this.diff_cleanupSemantic(diffs);
    assertEquals("diff_cleanupSemantic: No overlap elimination.", new List<Diff> {
        new Diff(Operation.DELETE, "abcxx"),
        new Diff(Operation.INSERT, "xxdef")}, diffs);

    diffs = new List<Diff> {
        new Diff(Operation.DELETE, "abcxxx"),
        new Diff(Operation.INSERT, "xxxdef")};
    this.diff_cleanupSemantic(diffs);
    assertEquals("diff_cleanupSemantic: Overlap elimination.", new List<Diff> {
        new Diff(Operation.DELETE, "abc"),
        new Diff(Operation.EQUAL, "xxx"),
        new Diff(Operation.INSERT, "def")}, diffs);

    diffs = new List<Diff> {
        new Diff(Operation.DELETE, "xxxabc"),
        new Diff(Operation.INSERT, "defxxx")};
    this.diff_cleanupSemantic(diffs);
    assertEquals("diff_cleanupSemantic: Reverse overlap elimination.", new List<Diff> {
        new Diff(Operation.INSERT, "def"),
        new Diff(Operation.EQUAL, "xxx"),
        new Diff(Operation.DELETE, "abc")}, diffs);

    diffs = new List<Diff> {
        new Diff(Operation.DELETE, "abcd1212"),
        new Diff(Operation.INSERT, "1212efghi"),
        new Diff(Operation.EQUAL, "----"),
        new Diff(Operation.DELETE, "A3"),
        new Diff(Operation.INSERT, "3BC")};
    this.diff_cleanupSemantic(diffs);
    assertEquals("diff_cleanupSemantic: Two overlap eliminations.", new List<Diff> {
        new Diff(Operation.DELETE, "abcd"),
        new Diff(Operation.EQUAL, "1212"),
        new Diff(Operation.INSERT, "efghi"),
        new Diff(Operation.EQUAL, "----"),
        new Diff(Operation.DELETE, "A"),
        new Diff(Operation.EQUAL, "3"),
        new Diff(Operation.INSERT, "BC")}, diffs);
  }

  public void diff_cleanupEfficiencyTest() {
    // Cleanup operationally trivial equalities.
    this.Diff_EditCost = 4;
    List<Diff> diffs = new List<Diff> ();
    this.diff_cleanupEfficiency(diffs);
    assertEquals("diff_cleanupEfficiency: Null case.", new List<Diff>(), diffs);

    diffs = new List<Diff> {
        new Diff(Operation.DELETE, "ab"),
        new Diff(Operation.INSERT, "12"),
        new Diff(Operation.EQUAL, "wxyz"),
        new Diff(Operation.DELETE, "cd"),
        new Diff(Operation.INSERT, "34")};
    this.diff_cleanupEfficiency(diffs);
    assertEquals("diff_cleanupEfficiency: No elimination.", new List<Diff> {
        new Diff(Operation.DELETE, "ab"),
        new Diff(Operation.INSERT, "12"),
        new Diff(Operation.EQUAL, "wxyz"),
        new Diff(Operation.DELETE, "cd"),
        new Diff(Operation.INSERT, "34")}, diffs);

    diffs = new List<Diff> {
        new Diff(Operation.DELETE, "ab"),
        new Diff(Operation.INSERT, "12"),
        new Diff(Operation.EQUAL, "xyz"),
        new Diff(Operation.DELETE, "cd"),
        new Diff(Operation.INSERT, "34")};
    this.diff_cleanupEfficiency(diffs);
    assertEquals("diff_cleanupEfficiency: Four-edit elimination.", new List<Diff> {
        new Diff(Operation.DELETE, "abxyzcd"),
        new Diff(Operation.INSERT, "12xyz34")}, diffs);

    diffs = new List<Diff> {
        new Diff(Operation.INSERT, "12"),
        new Diff(Operation.EQUAL, "x"),
        new Diff(Operation.DELETE, "cd"),
        new Diff(Operation.INSERT, "34")};
    this.diff_cleanupEfficiency(diffs);
    assertEquals("diff_cleanupEfficiency: Three-edit elimination.", new List<Diff> {
        new Diff(Operation.DELETE, "xcd"),
        new Diff(Operation.INSERT, "12x34")}, diffs);

    diffs = new List<Diff> {
        new Diff(Operation.DELETE, "ab"),
        new Diff(Operation.INSERT, "12"),
        new Diff(Operation.EQUAL, "xy"),
        new Diff(Operation.INSERT, "34"),
        new Diff(Operation.EQUAL, "z"),
        new Diff(Operation.DELETE, "cd"),
        new Diff(Operation.INSERT, "56")};
    this.diff_cleanupEfficiency(diffs);
    assertEquals("diff_cleanupEfficiency: Backpass elimination.", new List<Diff> {
        new Diff(Operation.DELETE, "abxyzcd"),
        new Diff(Operation.INSERT, "12xy34z56")}, diffs);

    this.Diff_EditCost = 5;
    diffs = new List<Diff> {
        new Diff(Operation.DELETE, "ab"),
        new Diff(Operation.INSERT, "12"),
        new Diff(Operation.EQUAL, "wxyz"),
        new Diff(Operation.DELETE, "cd"),
        new Diff(Operation.INSERT, "34")};
    this.diff_cleanupEfficiency(diffs);
    assertEquals("diff_cleanupEfficiency: High cost elimination.", new List<Diff> {
        new Diff(Operation.DELETE, "abwxyzcd"),
        new Diff(Operation.INSERT, "12wxyz34")}, diffs);
    this.Diff_EditCost = 4;
  }

  public void diff_prettyHtmlTest() {
    // Pretty print.
    List<Diff> diffs = new List<Diff> {
        new Diff(Operation.EQUAL, "a\n"),
        new Diff(Operation.DELETE, "<B>b</B>"),
        new Diff(Operation.INSERT, "c&d")};
    assertEquals("diff_prettyHtml:", "<span>a&para;<br></span><del style=\"background:#ffe6e6;\">&lt;B&gt;b&lt;/B&gt;</del><ins style=\"background:#e6ffe6;\">c&amp;d</ins>",
        this.diff_prettyHtml(diffs));
  }

  public void diff_textTest() {
    // Compute the source and destination texts.
    List<Diff> diffs = new List<Diff> {
        new Diff(Operation.EQUAL, "jump"),
        new Diff(Operation.DELETE, "s"),
        new Diff(Operation.INSERT, "ed"),
        new Diff(Operation.EQUAL, " over "),
        new Diff(Operation.DELETE, "the"),
        new Diff(Operation.INSERT, "a"),
        new Diff(Operation.EQUAL, " lazy")};
    assertEquals("diff_text1:", "jumps over the lazy", this.diff_text1(diffs));

    assertEquals("diff_text2:", "jumped over a lazy", this.diff_text2(diffs));
  }

  public void diff_deltaTest() {
    // Convert a diff into delta string.
    List<Diff> diffs = new List<Diff> {
        new Diff(Operation.EQUAL, "jump"),
        new Diff(Operation.DELETE, "s"),
        new Diff(Operation.INSERT, "ed"),
        new Diff(Operation.EQUAL, " over "),
        new Diff(Operation.DELETE, "the"),
        new Diff(Operation.INSERT, "a"),
        new Diff(Operation.EQUAL, " lazy"),
        new Diff(Operation.INSERT, "old dog")};
    string text1 = this.diff_text1(diffs);
    assertEquals("diff_text1: Base text.", "jumps over the lazy", text1);

    string delta = this.diff_toDelta(diffs);
    assertEquals("diff_toDelta:", "=4\t-1\t+ed\t=6\t-3\t+a\t=5\t+old dog", delta);

    // Convert delta string into a diff.
    assertEquals("diff_fromDelta: Normal.", diffs, this.diff_fromDelta(text1, delta));

    // Generates error (19 < 20).
    try {
      this.diff_fromDelta(text1 + "x", delta);
      assertFail("diff_fromDelta: Too long.");
    } catch (ArgumentException) {
      // Exception expected.
    }

    // Generates error (19 > 18).
    try {
      this.diff_fromDelta(text1.Substring(1), delta);
      assertFail("diff_fromDelta: Too short.");
    } catch (ArgumentException) {
      // Exception expected.
    }

    // Generates error (%c3%xy invalid Unicode).
    try {
      this.diff_fromDelta("", "+%c3%xy");
      assertFail("diff_fromDelta: Invalid character.");
    } catch (ArgumentException) {
      // Exception expected.
    }

    // Test deltas with special characters.
    char zero = (char)0;
    char one = (char)1;
    char two = (char)2;
    diffs = new List<Diff> {
        new Diff(Operation.EQUAL, "\u0680 " + zero + " \t %"),
        new Diff(Operation.DELETE, "\u0681 " + one + " \n ^"),
        new Diff(Operation.INSERT, "\u0682 " + two + " \\ |")};
    text1 = this.diff_text1(diffs);
    assertEquals("diff_text1: Unicode text.", "\u0680 " + zero + " \t %\u0681 " + one + " \n ^", text1);

    delta = this.diff_toDelta(diffs);
    // Lowercase, due to UrlEncode uses lower.
    assertEquals("diff_toDelta: Unicode.", "=7\t-7\t+%da%82 %02 %5c %7c", delta);

    assertEquals("diff_fromDelta: Unicode.", diffs, this.diff_fromDelta(text1, delta));

    // Verify pool of unchanged characters.
    diffs = new List<Diff> {
        new Diff(Operation.INSERT, "A-Z a-z 0-9 - _ . ! ~ * ' ( ) ; / ? : @ & = + $ , # ")};
    string text2 = this.diff_text2(diffs);
    assertEquals("diff_text2: Unchanged characters.", "A-Z a-z 0-9 - _ . ! ~ * \' ( ) ; / ? : @ & = + $ , # ", text2);

    delta = this.diff_toDelta(diffs);
    assertEquals("diff_toDelta: Unchanged characters.", "+A-Z a-z 0-9 - _ . ! ~ * \' ( ) ; / ? : @ & = + $ , # ", delta);

    // Convert delta string into a diff.
    assertEquals("diff_fromDelta: Unchanged characters.", diffs, this.diff_fromDelta("", delta));

    // 160 kb string.
    string a = "abcdefghij";
    for (int i = 0; i < 14; i++) {
      a += a;
    }
    diffs = new List<Diff> {new Diff(Operation.INSERT, a)};
    delta = this.diff_toDelta(diffs);
    assertEquals("diff_toDelta: 160kb string.", "+" + a, delta);

    // Convert delta string into a diff.
    assertEquals("diff_fromDelta: 160kb string.", diffs, this.diff_fromDelta("", delta));
  }

  public void diff_xIndexTest() {
    // Translate a location in text1 to text2.
    List<Diff> diffs = new List<Diff> {
        new Diff(Operation.DELETE, "a"),
        new Diff(Operation.INSERT, "1234"),
        new Diff(Operation.EQUAL, "xyz")};
    assertEquals("diff_xIndex: Translation on equality.", 5, this.diff_xIndex(diffs, 2));

    diffs = new List<Diff> {
        new Diff(Operation.EQUAL, "a"),
        new Diff(Operation.DELETE, "1234"),
        new Diff(Operation.EQUAL, "xyz")};
    assertEquals("diff_xIndex: Translation on deletion.", 1, this.diff_xIndex(diffs, 3));
  }

  public void diff_levenshteinTest() {
    List<Diff> diffs = new List<Diff> {
        new Diff(Operation.DELETE, "abc"),
        new Diff(Operation.INSERT, "1234"),
        new Diff(Operation.EQUAL, "xyz")};
    assertEquals("diff_levenshtein: Levenshtein with trailing equality.", 4, this.diff_levenshtein(diffs));

    diffs = new List<Diff> {
        new Diff(Operation.EQUAL, "xyz"),
        new Diff(Operation.DELETE, "abc"),
        new Diff(Operation.INSERT, "1234")};
    assertEquals("diff_levenshtein: Levenshtein with leading equality.", 4, this.diff_levenshtein(diffs));

    diffs = new List<Diff> {
        new Diff(Operation.DELETE, "abc"),
        new Diff(Operation.EQUAL, "xyz"),
        new Diff(Operation.INSERT, "1234")};
    assertEquals("diff_levenshtein: Levenshtein with middle equality.", 7, this.diff_levenshtein(diffs));
  }

  public void diff_bisectTest() {
    // Normal.
    string a = "cat";
    string b = "map";
    // Since the resulting diff hasn't been normalized, it would be ok if
    // the insertion and deletion pairs are swapped.
    // If the order changes, tweak this test as required.
    List<Diff> diffs = new List<Diff> {new Diff(Operation.DELETE, "c"), new Diff(Operation.INSERT, "m"), new Diff(Operation.EQUAL, "a"), new Diff(Operation.DELETE, "t"), new Diff(Operation.INSERT, "p")};
    assertEquals("diff_bisect: Normal.", diffs, this.diff_bisect(a, b, DateTime.MaxValue));

    // Timeout.
    diffs = new List<Diff> {new Diff(Operation.DELETE, "cat"), new Diff(Operation.INSERT, "map")};
    assertEquals("diff_bisect: Timeout.", diffs, this.diff_bisect(a, b, DateTime.MinValue));
  }

  public void diff_mainTest() {
    // Perform a trivial diff.
    List<Diff> diffs = new List<Diff> {};
    assertEquals("diff_main: Null case.", diffs, this.diff_main("", "", false));

    diffs = new List<Diff> {new Diff(Operation.EQUAL, "abc")};
    assertEquals("diff_main: Equality.", diffs, this.diff_main("abc", "abc", false));

    diffs = new List<Diff> {new Diff(Operation.EQUAL, "ab"), new Diff(Operation.INSERT, "123"), new Diff(Operation.EQUAL, "c")};
    assertEquals("diff_main: Simple insertion.", diffs, this.diff_main("abc", "ab123c", false));

    diffs = new List<Diff> {new Diff(Operation.EQUAL, "a"), new Diff(Operation.DELETE, "123"), new Diff(Operation.EQUAL, "bc")};
    assertEquals("diff_main: Simple deletion.", diffs, this.diff_main("a123bc", "abc", false));

    diffs = new List<Diff> {new Diff(Operation.EQUAL, "a"), new Diff(Operation.INSERT, "123"), new Diff(Operation.EQUAL, "b"), new Diff(Operation.INSERT, "456"), new Diff(Operation.EQUAL, "c")};
    assertEquals("diff_main: Two insertions.", diffs, this.diff_main("abc", "a123b456c", false));

    diffs = new List<Diff> {new Diff(Operation.EQUAL, "a"), new Diff(Operation.DELETE, "123"), new Diff(Operation.EQUAL, "b"), new Diff(Operation.DELETE, "456"), new Diff(Operation.EQUAL, "c")};
    assertEquals("diff_main: Two deletions.", diffs, this.diff_main("a123b456c", "abc", false));

    // Perform a real diff.
    // Switch off the timeout.
    this.Diff_Timeout = 0;
    diffs = new List<Diff> {new Diff(Operation.DELETE, "a"), new Diff(Operation.INSERT, "b")};
    assertEquals("diff_main: Simple case #1.", diffs, this.diff_main("a", "b", false));

    diffs = new List<Diff> {new Diff(Operation.DELETE, "Apple"), new Diff(Operation.INSERT, "Banana"), new Diff(Operation.EQUAL, "s are a"), new Diff(Operation.INSERT, "lso"), new Diff(Operation.EQUAL, " fruit.")};
    assertEquals("diff_main: Simple case #2.", diffs, this.diff_main("Apples are a fruit.", "Bananas are also fruit.", false));

    diffs = new List<Diff> {new Diff(Operation.DELETE, "a"), new Diff(Operation.INSERT, "\u0680"), new Diff(Operation.EQUAL, "x"), new Diff(Operation.DELETE, "\t"), new Diff(Operation.INSERT, new string (new char[]{(char)0}))};
    assertEquals("diff_main: Simple case #3.", diffs, this.diff_main("ax\t", "\u0680x" + (char)0, false));

    diffs = new List<Diff> {new Diff(Operation.DELETE, "1"), new Diff(Operation.EQUAL, "a"), new Diff(Operation.DELETE, "y"), new Diff(Operation.EQUAL, "b"), new Diff(Operation.DELETE, "2"), new Diff(Operation.INSERT, "xab")};
    assertEquals("diff_main: Overlap #1.", diffs, this.diff_main("1ayb2", "abxab", false));

    diffs = new List<Diff> {new Diff(Operation.INSERT, "xaxcx"), new Diff(Operation.EQUAL, "abc"), new Diff(Operation.DELETE, "y")};
    assertEquals("diff_main: Overlap #2.", diffs, this.diff_main("abcy", "xaxcxabc", false));

    diffs = new List<Diff> {new Diff(Operation.DELETE, "ABCD"), new Diff(Operation.EQUAL, "a"), new Diff(Operation.DELETE, "="), new Diff(Operation.INSERT, "-"), new Diff(Operation.EQUAL, "bcd"), new Diff(Operation.DELETE, "="), new Diff(Operation.INSERT, "-"), new Diff(Operation.EQUAL, "efghijklmnopqrs"), new Diff(Operation.DELETE, "EFGHIJKLMNOefg")};
    assertEquals("diff_main: Overlap #3.", diffs, this.diff_main("ABCDa=bcd=efghijklmnopqrsEFGHIJKLMNOefg", "a-bcd-efghijklmnopqrs", false));

    diffs = new List<Diff> {new Diff(Operation.INSERT, " "), new Diff(Operation.EQUAL, "a"), new Diff(Operation.INSERT, "nd"), new Diff(Operation.EQUAL, " [[Pennsylvania]]"), new Diff(Operation.DELETE, " and [[New")};
    assertEquals("diff_main: Large equality.", diffs, this.diff_main("a [[Pennsylvania]] and [[New", " and [[Pennsylvania]]", false));

    this.Diff_Timeout = 0.1f;  // 100ms
    string a = "`Twas brillig, and the slithy toves\nDid gyre and gimble in the wabe:\nAll mimsy were the borogoves,\nAnd the mome raths outgrabe.\n";
    string b = "I am the very model of a modern major general,\nI've information vegetable, animal, and mineral,\nI know the kings of England, and I quote the fights historical,\nFrom Marathon to Waterloo, in order categorical.\n";
    // Increase the text lengths by 1024 times to ensure a timeout.
    for (int i = 0; i < 10; i++) {
      a += a;
      b += b;
    }
    DateTime startTime = DateTime.Now;
    this.diff_main(a, b);
    DateTime endTime = DateTime.Now;
    // Test that we took at least the timeout period.
    assertTrue("diff_main: Timeout min.", new TimeSpan(((long)(this.Diff_Timeout * 1000)) * 10000) <= endTime - startTime);
    // Test that we didn't take forever (be forgiving).
    // Theoretically this test could fail very occasionally if the
    // OS task swaps or locks up for a second at the wrong moment.
    assertTrue("diff_main: Timeout max.", new TimeSpan(((long)(this.Diff_Timeout * 1000)) * 10000 * 2) > endTime - startTime);
    this.Diff_Timeout = 0;

    // Test the linemode speedup.
    // Must be long to pass the 100 char cutoff.
    a = "1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n";
    b = "abcdefghij\nabcdefghij\nabcdefghij\nabcdefghij\nabcdefghij\nabcdefghij\nabcdefghij\nabcdefghij\nabcdefghij\nabcdefghij\nabcdefghij\nabcdefghij\nabcdefghij\n";
    assertEquals("diff_main: Simple line-mode.", this.diff_main(a, b, true), this.diff_main(a, b, false));

    a = "1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890";
    b = "abcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghij";
    assertEquals("diff_main: Single line-mode.", this.diff_main(a, b, true), this.diff_main(a, b, false));

    a = "1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n";
    b = "abcdefghij\n1234567890\n1234567890\n1234567890\nabcdefghij\n1234567890\n1234567890\n1234567890\nabcdefghij\n1234567890\n1234567890\n1234567890\nabcdefghij\n";
    string[] texts_linemode = diff_rebuildtexts(this.diff_main(a, b, true));
    string[] texts_textmode = diff_rebuildtexts(this.diff_main(a, b, false));
    assertEquals("diff_main: Overlap line-mode.", texts_textmode, texts_linemode);

    // Test null inputs -- not needed because nulls can't be passed in C#.
  }

  public void match_alphabetTest() {
    // Initialise the bitmasks for Bitap.
    Dictionary<char, int> bitmask = new Dictionary<char, int>();
    bitmask.Add('a', 4); bitmask.Add('b', 2); bitmask.Add('c', 1);
    assertEquals("match_alphabet: Unique.", bitmask, this.match_alphabet("abc"));

    bitmask.Clear();
    bitmask.Add('a', 37); bitmask.Add('b', 18); bitmask.Add('c', 8);
    assertEquals("match_alphabet: Duplicates.", bitmask, this.match_alphabet("abcaba"));
  }

  public void match_bitapTest() {
    // Bitap algorithm.
    this.Match_Distance = 100;
    this.Match_Threshold = 0.5f;
    assertEquals("match_bitap: Exact match #1.", 5, this.match_bitap("abcdefghijk", "fgh", 5));

    assertEquals("match_bitap: Exact match #2.", 5, this.match_bitap("abcdefghijk", "fgh", 0));

    assertEquals("match_bitap: Fuzzy match #1.", 4, this.match_bitap("abcdefghijk", "efxhi", 0));

    assertEquals("match_bitap: Fuzzy match #2.", 2, this.match_bitap("abcdefghijk", "cdefxyhijk", 5));

    assertEquals("match_bitap: Fuzzy match #3.", -1, this.match_bitap("abcdefghijk", "bxy", 1));

    assertEquals("match_bitap: Overflow.", 2, this.match_bitap("123456789xx0", "3456789x0", 2));

    assertEquals("match_bitap: Before start match.", 0, this.match_bitap("abcdef", "xxabc", 4));

    assertEquals("match_bitap: Beyond end match.", 3, this.match_bitap("abcdef", "defyy", 4));

    assertEquals("match_bitap: Oversized pattern.", 0, this.match_bitap("abcdef", "xabcdefy", 0));

    this.Match_Threshold = 0.4f;
    assertEquals("match_bitap: Threshold #1.", 4, this.match_bitap("abcdefghijk", "efxyhi", 1));

    this.Match_Threshold = 0.3f;
    assertEquals("match_bitap: Threshold #2.", -1, this.match_bitap("abcdefghijk", "efxyhi", 1));

    this.Match_Threshold = 0.0f;
    assertEquals("match_bitap: Threshold #3.", 1, this.match_bitap("abcdefghijk", "bcdef", 1));

    this.Match_Threshold = 0.5f;
    assertEquals("match_bitap: Multiple select #1.", 0, this.match_bitap("abcdexyzabcde", "abccde", 3));

    assertEquals("match_bitap: Multiple select #2.", 8, this.match_bitap("abcdexyzabcde", "abccde", 5));

    this.Match_Distance = 10;  // Strict location.
    assertEquals("match_bitap: Distance test #1.", -1, this.match_bitap("abcdefghijklmnopqrstuvwxyz", "abcdefg", 24));

    assertEquals("match_bitap: Distance test #2.", 0, this.match_bitap("abcdefghijklmnopqrstuvwxyz", "abcdxxefg", 1));

    this.Match_Distance = 1000;  // Loose location.
    assertEquals("match_bitap: Distance test #3.", 0, this.match_bitap("abcdefghijklmnopqrstuvwxyz", "abcdefg", 24));
  }

  public void match_mainTest() {
    // Full match.
    assertEquals("match_main: Equality.", 0, this.match_main("abcdef", "abcdef", 1000));

    assertEquals("match_main: Null text.", -1, this.match_main("", "abcdef", 1));

    assertEquals("match_main: Null pattern.", 3, this.match_main("abcdef", "", 3));

    assertEquals("match_main: Exact match.", 3, this.match_main("abcdef", "de", 3));

    assertEquals("match_main: Beyond end match.", 3, this.match_main("abcdef", "defy", 4));

    assertEquals("match_main: Oversized pattern.", 0, this.match_main("abcdef", "abcdefy", 0));

    this.Match_Threshold = 0.7f;
    assertEquals("match_main: Complex match.", 4, this.match_main("I am the very model of a modern major general.", " that berry ", 5));
    this.Match_Threshold = 0.5f;

    // Test null inputs -- not needed because nulls can't be passed in C#.
  }

  public void patch_patchObjTest() {
    // Patch Object.
    Patch p = new Patch();
    p.start1 = 20;
    p.start2 = 21;
    p.length1 = 18;
    p.length2 = 17;
    p.diffs = new List<Diff> {
        new Diff(Operation.EQUAL, "jump"),
        new Diff(Operation.DELETE, "s"),
        new Diff(Operation.INSERT, "ed"),
        new Diff(Operation.EQUAL, " over "),
        new Diff(Operation.DELETE, "the"),
        new Diff(Operation.INSERT, "a"),
        new Diff(Operation.EQUAL, "\nlaz")};
    string strp = "@@ -21,18 +22,17 @@\n jump\n-s\n+ed\n  over \n-the\n+a\n %0alaz\n";
    assertEquals("Patch: toString.", strp, p.ToString());
  }

  public void patch_fromTextTest() {
    assertTrue("patch_fromText: #0.", this.patch_fromText("").Count == 0);

    string strp = "@@ -21,18 +22,17 @@\n jump\n-s\n+ed\n  over \n-the\n+a\n %0alaz\n";
    assertEquals("patch_fromText: #1.", strp, this.patch_fromText(strp)[0].ToString());

    assertEquals("patch_fromText: #2.", "@@ -1 +1 @@\n-a\n+b\n", this.patch_fromText("@@ -1 +1 @@\n-a\n+b\n")[0].ToString());

    assertEquals("patch_fromText: #3.", "@@ -1,3 +0,0 @@\n-abc\n", this.patch_fromText("@@ -1,3 +0,0 @@\n-abc\n") [0].ToString());

    assertEquals("patch_fromText: #4.", "@@ -0,0 +1,3 @@\n+abc\n", this.patch_fromText("@@ -0,0 +1,3 @@\n+abc\n") [0].ToString());

    // Generates error.
    try {
      this.patch_fromText("Bad\nPatch\n");
      assertFail("patch_fromText: #5.");
    } catch (ArgumentException) {
      // Exception expected.
    }
  }

  public void patch_toTextTest() {
    string strp = "@@ -21,18 +22,17 @@\n jump\n-s\n+ed\n  over \n-the\n+a\n  laz\n";
    List<Patch> patches;
    patches = this.patch_fromText(strp);
    string result = this.patch_toText(patches);
    assertEquals("patch_toText: Single.", strp, result);

    strp = "@@ -1,9 +1,9 @@\n-f\n+F\n oo+fooba\n@@ -7,9 +7,9 @@\n obar\n-,\n+.\n  tes\n";
    patches = this.patch_fromText(strp);
    result = this.patch_toText(patches);
    assertEquals("patch_toText: Dual.", strp, result);
  }

  public void patch_addContextTest() {
    this.Patch_Margin = 4;
    Patch p;
    p = this.patch_fromText("@@ -21,4 +21,10 @@\n-jump\n+somersault\n") [0];
    this.patch_addContext(p, "The quick brown fox jumps over the lazy dog.");
    assertEquals("patch_addContext: Simple case.", "@@ -17,12 +17,18 @@\n fox \n-jump\n+somersault\n s ov\n", p.ToString());

    p = this.patch_fromText("@@ -21,4 +21,10 @@\n-jump\n+somersault\n")[0];
    this.patch_addContext(p, "The quick brown fox jumps.");
    assertEquals("patch_addContext: Not enough trailing context.", "@@ -17,10 +17,16 @@\n fox \n-jump\n+somersault\n s.\n", p.ToString());

    p = this.patch_fromText("@@ -3 +3,2 @@\n-e\n+at\n")[0];
    this.patch_addContext(p, "The quick brown fox jumps.");
    assertEquals("patch_addContext: Not enough leading context.", "@@ -1,7 +1,8 @@\n Th\n-e\n+at\n  qui\n", p.ToString());

    p = this.patch_fromText("@@ -3 +3,2 @@\n-e\n+at\n")[0];
    this.patch_addContext(p, "The quick brown fox jumps.  The quick brown fox crashes.");
    assertEquals("patch_addContext: Ambiguity.", "@@ -1,27 +1,28 @@\n Th\n-e\n+at\n  quick brown fox jumps. \n", p.ToString());
  }

  public void patch_makeTest() {
    List<Patch> patches;
    patches = this.patch_make("", "");
    assertEquals("patch_make: Null case.", "", this.patch_toText(patches));

    string text1 = "The quick brown fox jumps over the lazy dog.";
    string text2 = "That quick brown fox jumped over a lazy dog.";
    string expectedPatch = "@@ -1,8 +1,7 @@\n Th\n-at\n+e\n  qui\n@@ -21,17 +21,18 @@\n jump\n-ed\n+s\n  over \n-a\n+the\n  laz\n";
    // The second patch must be "-21,17 +21,18", not "-22,17 +21,18" due to rolling context.
    patches = this.patch_make(text2, text1);
    assertEquals("patch_make: Text2+Text1 inputs.", expectedPatch, this.patch_toText(patches));

    expectedPatch = "@@ -1,11 +1,12 @@\n Th\n-e\n+at\n  quick b\n@@ -22,18 +22,17 @@\n jump\n-s\n+ed\n  over \n-the\n+a\n  laz\n";
    patches = this.patch_make(text1, text2);
    assertEquals("patch_make: Text1+Text2 inputs.", expectedPatch, this.patch_toText(patches));

    List<Diff> diffs = this.diff_main(text1, text2, false);
    patches = this.patch_make(diffs);
    assertEquals("patch_make: Diff input.", expectedPatch, this.patch_toText(patches));

    patches = this.patch_make(text1, diffs);
    assertEquals("patch_make: Text1+Diff inputs.", expectedPatch, this.patch_toText(patches));

    patches = this.patch_make(text1, text2, diffs);
    assertEquals("patch_make: Text1+Text2+Diff inputs (deprecated).", expectedPatch, this.patch_toText(patches));

    patches = this.patch_make("`1234567890-=[]\\;',./", "~!@#$%^&*()_+{}|:\"<>?");
    assertEquals("patch_toText: Character encoding.",
        "@@ -1,21 +1,21 @@\n-%601234567890-=%5b%5d%5c;',./\n+~!@#$%25%5e&*()_+%7b%7d%7c:%22%3c%3e?\n",
        this.patch_toText(patches));

    diffs = new List<Diff> {
        new Diff(Operation.DELETE, "`1234567890-=[]\\;',./"),
        new Diff(Operation.INSERT, "~!@#$%^&*()_+{}|:\"<>?")};
    assertEquals("patch_fromText: Character decoding.",
        diffs,
        this.patch_fromText("@@ -1,21 +1,21 @@\n-%601234567890-=%5B%5D%5C;',./\n+~!@#$%25%5E&*()_+%7B%7D%7C:%22%3C%3E?\n") [0].diffs);

    text1 = "";
    for (int x = 0; x < 100; x++) {
      text1 += "abcdef";
    }
    text2 = text1 + "123";
    expectedPatch = "@@ -573,28 +573,31 @@\n cdefabcdefabcdefabcdefabcdef\n+123\n";
    patches = this.patch_make(text1, text2);
    assertEquals("patch_make: Long string with repeats.", expectedPatch, this.patch_toText(patches));

    // Test null inputs -- not needed because nulls can't be passed in C#.
  }

  public void patch_splitMaxTest() {
    // Assumes that Match_MaxBits is 32.
    List<Patch> patches;

    patches = this.patch_make("abcdefghijklmnopqrstuvwxyz01234567890", "XabXcdXefXghXijXklXmnXopXqrXstXuvXwxXyzX01X23X45X67X89X0");
    this.patch_splitMax(patches);
    assertEquals("patch_splitMax: #1.", "@@ -1,32 +1,46 @@\n+X\n ab\n+X\n cd\n+X\n ef\n+X\n gh\n+X\n ij\n+X\n kl\n+X\n mn\n+X\n op\n+X\n qr\n+X\n st\n+X\n uv\n+X\n wx\n+X\n yz\n+X\n 012345\n@@ -25,13 +39,18 @@\n zX01\n+X\n 23\n+X\n 45\n+X\n 67\n+X\n 89\n+X\n 0\n", this.patch_toText(patches));

    patches = this.patch_make("abcdef1234567890123456789012345678901234567890123456789012345678901234567890uvwxyz", "abcdefuvwxyz");
    string oldToText = this.patch_toText(patches);
    this.patch_splitMax(patches);
    assertEquals("patch_splitMax: #2.", oldToText, this.patch_toText(patches));

    patches = this.patch_make("1234567890123456789012345678901234567890123456789012345678901234567890", "abc");
    this.patch_splitMax(patches);
    assertEquals("patch_splitMax: #3.", "@@ -1,32 +1,4 @@\n-1234567890123456789012345678\n 9012\n@@ -29,32 +1,4 @@\n-9012345678901234567890123456\n 7890\n@@ -57,14 +1,3 @@\n-78901234567890\n+abc\n", this.patch_toText(patches));

    patches = this.patch_make("abcdefghij , h : 0 , t : 1 abcdefghij , h : 0 , t : 1 abcdefghij , h : 0 , t : 1", "abcdefghij , h : 1 , t : 1 abcdefghij , h : 1 , t : 1 abcdefghij , h : 0 , t : 1");
    this.patch_splitMax(patches);
    assertEquals("patch_splitMax: #4.", "@@ -2,32 +2,32 @@\n bcdefghij , h : \n-0\n+1\n  , t : 1 abcdef\n@@ -29,32 +29,32 @@\n bcdefghij , h : \n-0\n+1\n  , t : 1 abcdef\n", this.patch_toText(patches));
  }

  public void patch_addPaddingTest() {
    List<Patch> patches;
    patches = this.patch_make("", "test");
    assertEquals("patch_addPadding: Both edges full.",
        "@@ -0,0 +1,4 @@\n+test\n",
        this.patch_toText(patches));
    this.patch_addPadding(patches);
    assertEquals("patch_addPadding: Both edges full.",
        "@@ -1,8 +1,12 @@\n %01%02%03%04\n+test\n %01%02%03%04\n",
        this.patch_toText(patches));

    patches = this.patch_make("XY", "XtestY");
    assertEquals("patch_addPadding: Both edges partial.",
        "@@ -1,2 +1,6 @@\n X\n+test\n Y\n",
        this.patch_toText(patches));
    this.patch_addPadding(patches);
    assertEquals("patch_addPadding: Both edges partial.",
        "@@ -2,8 +2,12 @@\n %02%03%04X\n+test\n Y%01%02%03\n",
        this.patch_toText(patches));

    patches = this.patch_make("XXXXYYYY", "XXXXtestYYYY");
    assertEquals("patch_addPadding: Both edges none.",
        "@@ -1,8 +1,12 @@\n XXXX\n+test\n YYYY\n",
        this.patch_toText(patches));
    this.patch_addPadding(patches);
    assertEquals("patch_addPadding: Both edges none.",
        "@@ -5,8 +5,12 @@\n XXXX\n+test\n YYYY\n",
        this.patch_toText(patches));
  }

  public void patch_applyTest() {
    this.Match_Distance = 1000;
    this.Match_Threshold = 0.5f;
    this.Patch_DeleteThreshold = 0.5f;
    List<Patch> patches;
    patches = this.patch_make("", "");
    Object[] results = this.patch_apply(patches, "Hello world.");
    bool[] boolArray = (bool[])results[1];
    string resultStr = results[0] + "\t" + boolArray.Length;
    assertEquals("patch_apply: Null case.", "Hello world.\t0", resultStr);

    patches = this.patch_make("The quick brown fox jumps over the lazy dog.", "That quick brown fox jumped over a lazy dog.");
    results = this.patch_apply(patches, "The quick brown fox jumps over the lazy dog.");
    boolArray = (bool[])results[1];
    resultStr = results[0] + "\t" + boolArray[0] + "\t" + boolArray[1];
    assertEquals("patch_apply: Exact match.", "That quick brown fox jumped over a lazy dog.\tTrue\tTrue", resultStr);

    results = this.patch_apply(patches, "The quick red rabbit jumps over the tired tiger.");
    boolArray = (bool[])results[1];
    resultStr = results[0] + "\t" + boolArray[0] + "\t" + boolArray[1];
    assertEquals("patch_apply: Partial match.", "That quick red rabbit jumped over a tired tiger.\tTrue\tTrue", resultStr);

    results = this.patch_apply(patches, "I am the very model of a modern major general.");
    boolArray = (bool[])results[1];
    resultStr = results[0] + "\t" + boolArray[0] + "\t" + boolArray[1];
    assertEquals("patch_apply: Failed match.", "I am the very model of a modern major general.\tFalse\tFalse", resultStr);

    patches = this.patch_make("x1234567890123456789012345678901234567890123456789012345678901234567890y", "xabcy");
    results = this.patch_apply(patches, "x123456789012345678901234567890-----++++++++++-----123456789012345678901234567890y");
    boolArray = (bool[])results[1];
    resultStr = results[0] + "\t" + boolArray[0] + "\t" + boolArray[1];
    assertEquals("patch_apply: Big delete, small change.", "xabcy\tTrue\tTrue", resultStr);

    patches = this.patch_make("x1234567890123456789012345678901234567890123456789012345678901234567890y", "xabcy");
    results = this.patch_apply(patches, "x12345678901234567890---------------++++++++++---------------12345678901234567890y");
    boolArray = (bool[])results[1];
    resultStr = results[0] + "\t" + boolArray[0] + "\t" + boolArray[1];
    assertEquals("patch_apply: Big delete, big change 1.", "xabc12345678901234567890---------------++++++++++---------------12345678901234567890y\tFalse\tTrue", resultStr);

    this.Patch_DeleteThreshold = 0.6f;
    patches = this.patch_make("x1234567890123456789012345678901234567890123456789012345678901234567890y", "xabcy");
    results = this.patch_apply(patches, "x12345678901234567890---------------++++++++++---------------12345678901234567890y");
    boolArray = (bool[])results[1];
    resultStr = results[0] + "\t" + boolArray[0] + "\t" + boolArray[1];
    assertEquals("patch_apply: Big delete, big change 2.", "xabcy\tTrue\tTrue", resultStr);
    this.Patch_DeleteThreshold = 0.5f;

    this.Match_Threshold = 0.0f;
    this.Match_Distance = 0;
    patches = this.patch_make("abcdefghijklmnopqrstuvwxyz--------------------1234567890", "abcXXXXXXXXXXdefghijklmnopqrstuvwxyz--------------------1234567YYYYYYYYYY890");
    results = this.patch_apply(patches, "ABCDEFGHIJKLMNOPQRSTUVWXYZ--------------------1234567890");
    boolArray = (bool[])results[1];
    resultStr = results[0] + "\t" + boolArray[0] + "\t" + boolArray[1];
    assertEquals("patch_apply: Compensate for failed patch.", "ABCDEFGHIJKLMNOPQRSTUVWXYZ--------------------1234567YYYYYYYYYY890\tFalse\tTrue", resultStr);
    this.Match_Threshold = 0.5f;
    this.Match_Distance = 1000;

    patches = this.patch_make("", "test");
    string patchStr = this.patch_toText(patches);
    this.patch_apply(patches, "");
    assertEquals("patch_apply: No side effects.", patchStr, this.patch_toText(patches));

    patches = this.patch_make("The quick brown fox jumps over the lazy dog.", "Woof");
    patchStr = this.patch_toText(patches);
    this.patch_apply(patches, "The quick brown fox jumps over the lazy dog.");
    assertEquals("patch_apply: No side effects with major delete.", patchStr, this.patch_toText(patches));

    patches = this.patch_make("", "test");
    results = this.patch_apply(patches, "");
    boolArray = (bool[])results[1];
    resultStr = results[0] + "\t" + boolArray[0];
    assertEquals("patch_apply: Edge exact match.", "test\tTrue", resultStr);

    patches = this.patch_make("XY", "XtestY");
    results = this.patch_apply(patches, "XY");
    boolArray = (bool[])results[1];
    resultStr = results[0] + "\t" + boolArray[0];
    assertEquals("patch_apply: Near edge exact match.", "XtestY\tTrue", resultStr);

    patches = this.patch_make("y", "y123");
    results = this.patch_apply(patches, "x");
    boolArray = (bool[])results[1];
    resultStr = results[0] + "\t" + boolArray[0];
    assertEquals("patch_apply: Edge partial match.", "x123\tTrue", resultStr);
  }

  private string[] diff_rebuildtexts(List<Diff> diffs) {
    string[] text = { "", "" };
    foreach (Diff myDiff in diffs) {
      if (myDiff.operation != Operation.INSERT) {
        text[0] += myDiff.text;
      }
      if (myDiff.operation != Operation.DELETE) {
        text[1] += myDiff.text;
      }
    }
    return text;
  }

  private static void assertEquals(string error_msg, string expected, string actual) {
    if (expected != actual) {
      throw new ArgumentException(String.Format("assertEquals (string, string) fail:\n Expected: {0}\n Actual: {1}\n{2}", expected, actual, error_msg));
    }
  }

  private static void assertEquals(string error_msg, string[] expected, string[] actual) {
    if (expected.Length != actual.Length) {
      throw new ArgumentException(String.Format("assertEquals (string[], string[]) length fail:\n Expected: {0}\n Actual: {1}\n{2}", expected, actual, error_msg));
    }
    for (int i = 0; i < expected.Length; i++) {
      if (expected[i] != actual[i]) {
        throw new ArgumentException(String.Format("assertEquals (string[], string[]) index {0} fail:\n Expected: {1}\n Actual: {2}\n{3}", i, expected, actual, error_msg));
      }
    }
  }

  private static void assertEquals(string error_msg, List<string> expected, List<string> actual) {
    if (expected.Count != actual.Count) {
      throw new ArgumentException(String.Format("assertEquals (List<string>, List<string>) length fail:\n Expected: {0}\n Actual: {1}\n{2}", expected, actual, error_msg));
    }
    for (int i = 0; i < expected.Count; i++) {
      if (expected[i] != actual[i]) {
        throw new ArgumentException(String.Format("assertEquals (List<string>, List<string>) index {0} fail:\n Expected: {1}\n Actual: {2}\n{3}", i, expected, actual, error_msg));
      }
    }
  }

  private static void assertEquals(string error_msg, List<Diff> expected, List<Diff> actual) {
    if (expected.Count != actual.Count) {
      throw new ArgumentException(String.Format("assertEquals (List<Diff>, List<Diff>) length fail:\n Expected: {0}\n Actual: {1}\n{2}", expected, actual, error_msg));
    }
    for (int i = 0; i < expected.Count; i++) {
      if (!expected[i].Equals(actual[i])) {
      throw new ArgumentException(String.Format("assertEquals (List<Diff>, List<Diff>) index {0} fail:\n Expected: {1}\n Actual: {2}\n{3}", i, expected, actual, error_msg));
      }
    }
  }

  private static void assertEquals(string error_msg, Diff expected, Diff actual) {
    if (!expected.Equals(actual)) {
      throw new ArgumentException(String.Format("assertEquals (Diff, Diff) fail:\n Expected: {0}\n Actual: {1}\n{2}", expected, actual, error_msg));
    }
  }

  private static void assertEquals(string error_msg, Dictionary<char, int> expected, Dictionary<char, int> actual) {
    foreach(char k in actual.Keys) {
      if (!expected.ContainsKey(k)) {
        throw new ArgumentException(String.Format("assertEquals (Dictionary<char, int>, Dictionary<char, int>) key {0} fail:\n Expected: {1}\n Actual: {2}\n{3}", k, expected, actual, error_msg));
      }
    }
    foreach(char k in expected.Keys) {
      if (!actual.ContainsKey(k)) {
        throw new ArgumentException(String.Format("assertEquals (Dictionary<char, int>, Dictionary<char, int>) key {0} fail:\n Expected: {1}\n Actual: {2}\n{3}", k, expected, actual, error_msg));
      }
      if (actual[k] != expected[k]) {
        throw new ArgumentException(String.Format("assertEquals (Dictionary<char, int>, Dictionary<char, int>) key {0} fail:\n Expected: {1}\n Actual: {2}\n{3}", k, expected, actual, error_msg));
      }
    }
  }

  private static void assertEquals(string error_msg, int expected, int actual) {
    if (expected != actual) {
      throw new ArgumentException(String.Format("assertEquals (int, int) fail:\n Expected: {0}\n Actual: {1}\n{2}", expected, actual, error_msg));
    }
  }

  private static void assertTrue(string error_msg, bool expected) {
    if (!expected) {
      throw new ArgumentException(String.Format("assertTrue fail:\n{0}", error_msg));
    }
  }

  private static void assertNull(string error_msg, object value) {
    if (value != null) {
      throw new ArgumentException(String.Format("assertNull fail:\n{0}", error_msg));
    }
  }

  private static void assertFail(string error_msg) {
    throw new ArgumentException(String.Format("assertFail fail:\n{0}", error_msg));
  }

  public static void Main(string[] args) {
    diff_match_patchTest dmp = new diff_match_patchTest();

    dmp.diff_commonPrefixTest();
    dmp.diff_commonSuffixTest();
    dmp.diff_commonOverlapTest();
    dmp.diff_halfmatchTest();
    dmp.diff_linesToCharsTest();
    dmp.diff_charsToLinesTest();
    dmp.diff_cleanupMergeTest();
    dmp.diff_cleanupSemanticLosslessTest();
    dmp.diff_cleanupSemanticTest();
    dmp.diff_cleanupEfficiencyTest();
    dmp.diff_prettyHtmlTest();
    dmp.diff_textTest();
    dmp.diff_deltaTest();
    dmp.diff_xIndexTest();
    dmp.diff_levenshteinTest();
    dmp.diff_bisectTest();
    dmp.diff_mainTest();

    dmp.match_alphabetTest();
    dmp.match_bitapTest();
    dmp.match_mainTest();

    dmp.patch_patchObjTest();
    dmp.patch_fromTextTest();
    dmp.patch_toTextTest();
    dmp.patch_addContextTest();
    dmp.patch_makeTest();
    dmp.patch_splitMaxTest();
    dmp.patch_addPaddingTest();
    dmp.patch_applyTest();

    Console.WriteLine("All tests passed.");
  }
}
