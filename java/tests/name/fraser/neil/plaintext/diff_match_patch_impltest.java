/*
 * Diff Match and Patch -- Test harness
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

package name.fraser.neil.plaintext;

import java.util.Arrays;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;

import name.fraser.neil.plaintext.diff_match_patch.Diff;
import name.fraser.neil.plaintext.diff_match_patch.Patch;

/**
 * Tests the internal implementation methods.
 *
 * Compile from diff-match-patch/java with:
 * javac -d classes src/name/fraser/neil/plaintext/diff_match_patch.java tests/name/fraser/neil/plaintext/diff_match_patch_impltest.java
 * Execute with:
 * java -classpath classes name/fraser/neil/plaintext/diff_match_patch_impltest
 */
public class diff_match_patch_impltest {

  private static diff_match_patch dmp;
  private static diff_match_patch.Operation DELETE = diff_match_patch.Operation.DELETE;
  private static diff_match_patch.Operation EQUAL = diff_match_patch.Operation.EQUAL;
  private static diff_match_patch.Operation INSERT = diff_match_patch.Operation.INSERT;


  //  DIFF TEST FUNCTIONS


  public static void testDiffCommonPrefix() {
    // Detect any common prefix.
    assertEquals("diff_commonPrefix: Null case.", 0, dmp.diff_commonPrefix("abc", "xyz"));

    assertEquals("diff_commonPrefix: Non-null case.", 4, dmp.diff_commonPrefix("1234abcdef", "1234xyz"));

    assertEquals("diff_commonPrefix: Whole case.", 4, dmp.diff_commonPrefix("1234", "1234xyz"));
  }

  public static void testDiffCommonSuffix() {
    // Detect any common suffix.
    assertEquals("diff_commonSuffix: Null case.", 0, dmp.diff_commonSuffix("abc", "xyz"));

    assertEquals("diff_commonSuffix: Non-null case.", 4, dmp.diff_commonSuffix("abcdef1234", "xyz1234"));

    assertEquals("diff_commonSuffix: Whole case.", 4, dmp.diff_commonSuffix("1234", "xyz1234"));
  }

  public static void testDiffCommonOverlap() {
    // Detect any suffix/prefix overlap.
    assertEquals("diff_commonOverlap: Null case.", 0, dmp.diff_commonOverlap("", "abcd"));

    assertEquals("diff_commonOverlap: Whole case.", 3, dmp.diff_commonOverlap("abc", "abcd"));

    assertEquals("diff_commonOverlap: No overlap.", 0, dmp.diff_commonOverlap("123456", "abcd"));

    assertEquals("diff_commonOverlap: Overlap.", 3, dmp.diff_commonOverlap("123456xxx", "xxxabcd"));

    // Some overly clever languages (C#) may treat ligatures as equal to their
    // component letters.  E.g. U+FB01 == 'fi'
    assertEquals("diff_commonOverlap: Unicode.", 0, dmp.diff_commonOverlap("fi", "\ufb01i"));
  }

  public static void testDiffHalfmatch() {
    // Detect a halfmatch.
    dmp.Diff_Timeout = 1;
    assertNull("diff_halfMatch: No match #1.", dmp.diff_halfMatch("1234567890", "abcdef"));

    assertNull("diff_halfMatch: No match #2.", dmp.diff_halfMatch("12345", "23"));

    assertArrayEquals("diff_halfMatch: Single Match #1.", new String[]{"12", "90", "a", "z", "345678"}, dmp.diff_halfMatch("1234567890", "a345678z"));

    assertArrayEquals("diff_halfMatch: Single Match #2.", new String[]{"a", "z", "12", "90", "345678"}, dmp.diff_halfMatch("a345678z", "1234567890"));

    assertArrayEquals("diff_halfMatch: Single Match #3.", new String[]{"abc", "z", "1234", "0", "56789"}, dmp.diff_halfMatch("abc56789z", "1234567890"));

    assertArrayEquals("diff_halfMatch: Single Match #4.", new String[]{"a", "xyz", "1", "7890", "23456"}, dmp.diff_halfMatch("a23456xyz", "1234567890"));

    assertArrayEquals("diff_halfMatch: Multiple Matches #1.", new String[]{"12123", "123121", "a", "z", "1234123451234"}, dmp.diff_halfMatch("121231234123451234123121", "a1234123451234z"));

    assertArrayEquals("diff_halfMatch: Multiple Matches #2.", new String[]{"", "-=-=-=-=-=", "x", "", "x-=-=-=-=-=-=-="}, dmp.diff_halfMatch("x-=-=-=-=-=-=-=-=-=-=-=-=", "xx-=-=-=-=-=-=-="));

    assertArrayEquals("diff_halfMatch: Multiple Matches #3.", new String[]{"-=-=-=-=-=", "", "", "y", "-=-=-=-=-=-=-=y"}, dmp.diff_halfMatch("-=-=-=-=-=-=-=-=-=-=-=-=y", "-=-=-=-=-=-=-=yy"));

    // Optimal diff would be -q+x=H-i+e=lloHe+Hu=llo-Hew+y not -qHillo+x=HelloHe-w+Hulloy
    assertArrayEquals("diff_halfMatch: Non-optimal halfmatch.", new String[]{"qHillo", "w", "x", "Hulloy", "HelloHe"}, dmp.diff_halfMatch("qHilloHelloHew", "xHelloHeHulloy"));

    dmp.Diff_Timeout = 0;
    assertNull("diff_halfMatch: Optimal no halfmatch.", dmp.diff_halfMatch("qHilloHelloHew", "xHelloHeHulloy"));
  }

  public static void testDiffBisect() {
    // Normal.
    String a = "cat";
    String b = "map";
    // Since the resulting diff hasn't been normalized, it would be ok if
    // the insertion and deletion pairs are swapped.
    // If the order changes, tweak this test as required.
    LinkedList<Diff> diffs = diffList(new Diff(DELETE, "c"), new Diff(INSERT, "m"), new Diff(EQUAL, "a"), new Diff(DELETE, "t"), new Diff(INSERT, "p"));
    assertEquals("diff_bisect: Normal.", diffs, dmp.diff_bisect(a, b, Long.MAX_VALUE));

    // Timeout.
    diffs = diffList(new Diff(DELETE, "cat"), new Diff(INSERT, "map"));
    assertEquals("diff_bisect: Timeout.", diffs, dmp.diff_bisect(a, b, 0));
  }

  
  //  MATCH TEST FUNCTIONS


  public static void testMatchAlphabet() {
    // Initialise the bitmasks for Bitap.
    Map<Character, Integer> bitmask;
    bitmask = new HashMap<Character, Integer>();
    bitmask.put('a', 4); bitmask.put('b', 2); bitmask.put('c', 1);
    assertEquals("match_alphabet: Unique.", bitmask, dmp.match_alphabet("abc"));

    bitmask = new HashMap<Character, Integer>();
    bitmask.put('a', 37); bitmask.put('b', 18); bitmask.put('c', 8);
    assertEquals("match_alphabet: Duplicates.", bitmask, dmp.match_alphabet("abcaba"));
  }

  public static void testMatchBitap() {
    // Bitap algorithm.
    dmp.Match_Distance = 100;
    dmp.Match_Threshold = 0.5f;
    assertEquals("match_bitap: Exact match #1.", 5, dmp.match_bitap("abcdefghijk", "fgh", 5));

    assertEquals("match_bitap: Exact match #2.", 5, dmp.match_bitap("abcdefghijk", "fgh", 0));

    assertEquals("match_bitap: Fuzzy match #1.", 4, dmp.match_bitap("abcdefghijk", "efxhi", 0));

    assertEquals("match_bitap: Fuzzy match #2.", 2, dmp.match_bitap("abcdefghijk", "cdefxyhijk", 5));

    assertEquals("match_bitap: Fuzzy match #3.", -1, dmp.match_bitap("abcdefghijk", "bxy", 1));

    assertEquals("match_bitap: Overflow.", 2, dmp.match_bitap("123456789xx0", "3456789x0", 2));

    assertEquals("match_bitap: Before start match.", 0, dmp.match_bitap("abcdef", "xxabc", 4));

    assertEquals("match_bitap: Beyond end match.", 3, dmp.match_bitap("abcdef", "defyy", 4));

    assertEquals("match_bitap: Oversized pattern.", 0, dmp.match_bitap("abcdef", "xabcdefy", 0));

    dmp.Match_Threshold = 0.4f;
    assertEquals("match_bitap: Threshold #1.", 4, dmp.match_bitap("abcdefghijk", "efxyhi", 1));

    dmp.Match_Threshold = 0.3f;
    assertEquals("match_bitap: Threshold #2.", -1, dmp.match_bitap("abcdefghijk", "efxyhi", 1));

    dmp.Match_Threshold = 0.0f;
    assertEquals("match_bitap: Threshold #3.", 1, dmp.match_bitap("abcdefghijk", "bcdef", 1));

    dmp.Match_Threshold = 0.5f;
    assertEquals("match_bitap: Multiple select #1.", 0, dmp.match_bitap("abcdexyzabcde", "abccde", 3));

    assertEquals("match_bitap: Multiple select #2.", 8, dmp.match_bitap("abcdexyzabcde", "abccde", 5));

    dmp.Match_Distance = 10;  // Strict location.
    assertEquals("match_bitap: Distance test #1.", -1, dmp.match_bitap("abcdefghijklmnopqrstuvwxyz", "abcdefg", 24));

    assertEquals("match_bitap: Distance test #2.", 0, dmp.match_bitap("abcdefghijklmnopqrstuvwxyz", "abcdxxefg", 1));

    dmp.Match_Distance = 1000;  // Loose location.
    assertEquals("match_bitap: Distance test #3.", 0, dmp.match_bitap("abcdefghijklmnopqrstuvwxyz", "abcdefg", 24));
  }


  //  PATCH TEST FUNCTIONS


  public static void testPatchAddContext() {
    dmp.Patch_Margin = 4;
    Patch p;
    p = dmp.patch_fromText("@@ -21,4 +21,10 @@\n-jump\n+somersault\n").get(0);
    dmp.patch_addContext(p, "The quick brown fox jumps over the lazy dog.");
    assertEquals("patch_addContext: Simple case.", "@@ -17,12 +17,18 @@\n fox \n-jump\n+somersault\n s ov\n", p.toString());

    p = dmp.patch_fromText("@@ -21,4 +21,10 @@\n-jump\n+somersault\n").get(0);
    dmp.patch_addContext(p, "The quick brown fox jumps.");
    assertEquals("patch_addContext: Not enough trailing context.", "@@ -17,10 +17,16 @@\n fox \n-jump\n+somersault\n s.\n", p.toString());

    p = dmp.patch_fromText("@@ -3 +3,2 @@\n-e\n+at\n").get(0);
    dmp.patch_addContext(p, "The quick brown fox jumps.");
    assertEquals("patch_addContext: Not enough leading context.", "@@ -1,7 +1,8 @@\n Th\n-e\n+at\n  qui\n", p.toString());

    p = dmp.patch_fromText("@@ -3 +3,2 @@\n-e\n+at\n").get(0);
    dmp.patch_addContext(p, "The quick brown fox jumps.  The quick brown fox crashes.");
    assertEquals("patch_addContext: Ambiguity.", "@@ -1,27 +1,28 @@\n Th\n-e\n+at\n  quick brown fox jumps. \n", p.toString());
  }

  private static void assertEquals(String error_msg, Object a, Object b) {
    if (!a.toString().equals(b.toString())) {
      throw new Error("assertEquals fail:\n Expected: " + a + "\n Actual: " + b
                      + "\n" + error_msg);
    }
  }

  private static void assertArrayEquals(String error_msg, Object[] a, Object[] b) {
    List<Object> list_a = Arrays.asList(a);
    List<Object> list_b = Arrays.asList(b);
    assertEquals(error_msg, list_a, list_b);
  }

  private static void assertNull(String error_msg, Object n) {
    if (n != null) {
      throw new Error("assertNull fail: " + error_msg);
    }
  }

  // Private function for quickly building lists of diffs.
  private static LinkedList<Diff> diffList(Diff... diffs) {
      return new LinkedList<Diff>(Arrays.asList(diffs));
  }

  public static void main(String args[]) {
    dmp = new diff_match_patch();

    testDiffCommonPrefix();
    testDiffCommonSuffix();
    testDiffCommonOverlap();
    testDiffHalfmatch();
    testDiffBisect();

    testMatchAlphabet();
    testMatchBitap();

    testPatchAddContext();

    System.out.println("All tests passed.");
  }
}
