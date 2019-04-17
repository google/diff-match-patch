/**
 * Diff Match and Patch -- Test Harness
 * Copyright 2018 The diff-match-patch Authors.
 * https://github.com/google/diff-match-patch
 *
 * Licensed under the Apache License, Version 2.0 (the 'License');
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an 'AS IS' BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

import '../DiffMatchPatch.dart';

// Expect class disappeared from Dart unexpectedly.  Here's a minimal shim.
class Expect {
  static void equals(var expected, var actual, String msg) {
    if (expected == actual) return;
    throw new Exception(
        'Expect.equals(expected: <$expected>, actual: <$actual> $msg) fails.');
  }

  static void isNull(actual, String msg) {
    if (null == actual) return;
    throw new Exception('Expect.isNull(actual: <$actual>$msg) fails.');
  }

  static void isTrue(var actual, String msg) {
    if (identical(actual, true)) return;
    throw new Exception('Expect.isTrue($actual, $msg) fails.');
  }

  static void throws(void f(), String msg) {
    try {
      f();
    } catch (e) {
      return;
    }
    throw new Exception('Expect.throws($msg) fails');
  }

  static void listEquals(List expected, List actual, String msg) {
    int n = (expected.length < actual.length) ? expected.length : actual.length;
    for (int i = 0; i < n; i++) {
      if (expected[i] != actual[i]) {
        throw new Exception('Expect.listEquals(at index $i, '
            'expected: <${expected[i]}>, actual: <${actual[i]}> $msg) fails');
      }
    }
    // We check on length at the end in order to provide better error
    // messages when an unexpected item is inserted in a list.
    if (expected.length != actual.length) {
      throw new Exception('Expect.listEquals(list length, '
        'expected: <${expected.length}>, actual: <${actual.length}> $msg) '
        'fails: Next element <'
        '${expected.length > n ? expected[n] : actual[n]}>');
    }
  }

  static void mapEquals(Map expected, Map actual, String msg) {
    for (var k in actual.keys) {
      if (!expected.containsKey(k)) {
        throw new Exception('Expect.mapEquals(unexpected key <$k> found '
            'expected: <$expected>, actual: <$actual> $msg) fails');
      }
    }
    for (var k in expected.keys) {
      if (!actual.containsKey(k)) {
        throw new Exception('Expect.mapEquals(key <$k> not found '
            'expected: <$expected>, actual: <$actual> $msg) fails');
      }
      Expect.equals(actual[k], expected[k], "$msg [Key: $k]");
    }
  }
}

List<String> _diff_rebuildtexts(diffs) {
  // Construct the two texts which made up the diff originally.
  final text1 = new StringBuffer();
  final text2 = new StringBuffer();
  for (int x = 0; x < diffs.length; x++) {
    if (diffs[x].operation != Operation.insert) {
      text1.write(diffs[x].text);
    }
    if (diffs[x].operation != Operation.delete) {
      text2.write(diffs[x].text);
    }
  }
  return [text1.toString(), text2.toString()];
}

DiffMatchPatch dmp;

// DIFF TEST FUNCTIONS


void testDiffCommonPrefix() {
  // Detect any common prefix.
  Expect.equals(0, dmp.diff_commonPrefix('abc', 'xyz'), 'diff_commonPrefix: Null case.');

  Expect.equals(4, dmp.diff_commonPrefix('1234abcdef', '1234xyz'), 'diff_commonPrefix: Non-null case.');

  Expect.equals(4, dmp.diff_commonPrefix('1234', '1234xyz'), 'diff_commonPrefix: Whole case.');
}

void testDiffCommonSuffix() {
  // Detect any common suffix.
  Expect.equals(0, dmp.diff_commonSuffix('abc', 'xyz'), 'diff_commonSuffix: Null case.');

  Expect.equals(4, dmp.diff_commonSuffix('abcdef1234', 'xyz1234'), 'diff_commonSuffix: Non-null case.');

  Expect.equals(4, dmp.diff_commonSuffix('1234', 'xyz1234'), 'diff_commonSuffix: Whole case.');
}

void testDiffCommonOverlap() {
  // Detect any suffix/prefix overlap.
  Expect.equals(0, dmp.test_diff_commonOverlap('', 'abcd'), 'diff_commonOverlap: Null case.');

  Expect.equals(3, dmp.test_diff_commonOverlap('abc', 'abcd'), 'diff_commonOverlap: Whole case.');

  Expect.equals(0, dmp.test_diff_commonOverlap('123456', 'abcd'), 'diff_commonOverlap: No overlap.');

  Expect.equals(3, dmp.test_diff_commonOverlap('123456xxx', 'xxxabcd'), 'diff_commonOverlap: Overlap.');

  // Some overly clever languages (C#) may treat ligatures as equal to their
  // component letters.  E.g. U+FB01 == 'fi'
  Expect.equals(0, dmp.test_diff_commonOverlap('fi', '\ufb01i'), 'diff_commonOverlap: Unicode.');
}

void testDiffHalfmatch() {
  // Detect a halfmatch.
  dmp.Diff_Timeout = 1.0;
  Expect.isNull(dmp.test_diff_halfMatch('1234567890', 'abcdef'), 'diff_halfMatch: No match #1.');

  Expect.isNull(dmp.test_diff_halfMatch('12345', '23'), 'diff_halfMatch: No match #2.');

  Expect.listEquals(['12', '90', 'a', 'z', '345678'], dmp.test_diff_halfMatch('1234567890', 'a345678z'), 'diff_halfMatch: Single Match #1.');

  Expect.listEquals(['a', 'z', '12', '90', '345678'], dmp.test_diff_halfMatch('a345678z', '1234567890'), 'diff_halfMatch: Single Match #2.');

  Expect.listEquals(['abc', 'z', '1234', '0', '56789'], dmp.test_diff_halfMatch('abc56789z', '1234567890'), 'diff_halfMatch: Single Match #3.');

  Expect.listEquals(['a', 'xyz', '1', '7890', '23456'], dmp.test_diff_halfMatch('a23456xyz', '1234567890'), 'diff_halfMatch: Single Match #4.');

  Expect.listEquals(['12123', '123121', 'a', 'z', '1234123451234'], dmp.test_diff_halfMatch('121231234123451234123121', 'a1234123451234z'), 'diff_halfMatch: Multiple Matches #1.');

  Expect.listEquals(['', '-=-=-=-=-=', 'x', '', 'x-=-=-=-=-=-=-='], dmp.test_diff_halfMatch('x-=-=-=-=-=-=-=-=-=-=-=-=', 'xx-=-=-=-=-=-=-='), 'diff_halfMatch: Multiple Matches #2.');

  Expect.listEquals(['-=-=-=-=-=', '', '', 'y', '-=-=-=-=-=-=-=y'], dmp.test_diff_halfMatch('-=-=-=-=-=-=-=-=-=-=-=-=y', '-=-=-=-=-=-=-=yy'), 'diff_halfMatch: Multiple Matches #3.');

  // Optimal diff would be -q+x=H-i+e=lloHe+Hu=llo-Hew+y not -qHillo+x=HelloHe-w+Hulloy
  Expect.listEquals(['qHillo', 'w', 'x', 'Hulloy', 'HelloHe'], dmp.test_diff_halfMatch('qHilloHelloHew', 'xHelloHeHulloy'), 'diff_halfMatch: Non-optimal halfmatch.');

  dmp.Diff_Timeout = 0.0;
  Expect.isNull(dmp.test_diff_halfMatch('qHilloHelloHew', 'xHelloHeHulloy'), 'diff_halfMatch: Optimal no halfmatch.');
}

void testDiffLinesToChars() {
  void assertLinesToCharsResultEquals(Map<String, dynamic> a, Map<String, dynamic> b, String error_msg) {
    Expect.equals(a['chars1'], b['chars1'], error_msg);
    Expect.equals(a['chars2'], b['chars2'], error_msg);
    Expect.listEquals(a['lineArray'], b['lineArray'], error_msg);
  }

  // Convert lines down to characters.
  assertLinesToCharsResultEquals({'chars1': '\u0001\u0002\u0001', 'chars2': '\u0002\u0001\u0002', 'lineArray': ['', 'alpha\n', 'beta\n']}, dmp.test_diff_linesToChars('alpha\nbeta\nalpha\n', 'beta\nalpha\nbeta\n'), 'diff_linesToChars: Shared lines.');

  assertLinesToCharsResultEquals({'chars1': '', 'chars2': '\u0001\u0002\u0003\u0003', 'lineArray': ['', 'alpha\r\n', 'beta\r\n', '\r\n']}, dmp.test_diff_linesToChars('', 'alpha\r\nbeta\r\n\r\n\r\n'), 'diff_linesToChars: Empty string and blank lines.');

  assertLinesToCharsResultEquals({'chars1': '\u0001', 'chars2': '\u0002', 'lineArray': ['', 'a', 'b']}, dmp.test_diff_linesToChars('a', 'b'), 'diff_linesToChars: No linebreaks.');

  // More than 256 to reveal any 8-bit limitations.
  int n = 300;
  List<String> lineList = [];
  StringBuffer charList = new StringBuffer();
  for (int i = 1; i < n + 1; i++) {
    lineList.add('$i\n');
    charList.writeCharCode(i);
  }
  Expect.equals(n, lineList.length, 'Test initialization fail #1.');
  String lines = lineList.join();
  String chars = charList.toString();
  Expect.equals(n, chars.length, 'Test initialization fail #2.');
  lineList.insert(0, '');
  assertLinesToCharsResultEquals({'chars1': chars, 'chars2': '', 'lineArray': lineList}, dmp.test_diff_linesToChars(lines, ''), 'diff_linesToChars: More than 256.');
}

void testDiffCharsToLines() {
  // First check that Diff equality works.
  Expect.isTrue(new Diff(Operation.equal, 'a') == new Diff(Operation.equal, 'a'), 'diff_charsToLines: Equality #1.');

  Expect.equals(new Diff(Operation.equal, 'a'), new Diff(Operation.equal, 'a'), 'diff_charsToLines: Equality #2.');

  // Convert chars up to lines.
  List<Diff> diffs = [new Diff(Operation.equal, '\u0001\u0002\u0001'), new Diff(Operation.insert, '\u0002\u0001\u0002')];
  dmp.test_diff_charsToLines(diffs, ['', 'alpha\n', 'beta\n']);
  Expect.listEquals([new Diff(Operation.equal, 'alpha\nbeta\nalpha\n'), new Diff(Operation.insert, 'beta\nalpha\nbeta\n')], diffs, 'diff_charsToLines: Shared lines.');

  // More than 256 to reveal any 8-bit limitations.
  int n = 300;
  List<String> lineList = [];
  StringBuffer charList = new StringBuffer();
  for (int i = 1; i < n + 1; i++) {
    lineList.add('$i\n');
    charList.writeCharCode(i);
  }
  Expect.equals(n, lineList.length, 'Test initialization fail #3.');
  String lines = lineList.join();
  String chars = charList.toString();
  Expect.equals(n, chars.length, 'Test initialization fail #4.');
  lineList.insert(0, '');
  diffs = [new Diff(Operation.delete, chars)];
  dmp.test_diff_charsToLines(diffs, lineList);
  Expect.listEquals([new Diff(Operation.delete, lines)], diffs, 'diff_charsToLines: More than 256.');

  // More than 65536 to verify any 16-bit limitation.
  lineList = [];
  for (int i = 0; i < 66000; i++) {
    lineList.add('$i\n');
  }
  chars = lineList.join();
  final results = dmp.test_diff_linesToChars(chars, '');
  diffs = [new Diff(Operation.insert, results['chars1'])];
  dmp.test_diff_charsToLines(diffs, results['lineArray']);
  Expect.equals(chars, diffs[0].text, 'diff_charsToLines: More than 65536.');
}

void testDiffCleanupMerge() {
  // Cleanup a messy diff.
  List<Diff> diffs = [];
  dmp.diff_cleanupMerge(diffs);
  Expect.listEquals([], diffs, 'diff_cleanupMerge: Null case.');

  diffs = [new Diff(Operation.equal, 'a'), new Diff(Operation.delete, 'b'), new Diff(Operation.insert, 'c')];
  dmp.diff_cleanupMerge(diffs);
  Expect.listEquals([new Diff(Operation.equal, 'a'), new Diff(Operation.delete, 'b'), new Diff(Operation.insert, 'c')], diffs, 'diff_cleanupMerge: No change case.');

  diffs = [new Diff(Operation.equal, 'a'), new Diff(Operation.equal, 'b'), new Diff(Operation.equal, 'c')];
  dmp.diff_cleanupMerge(diffs);
  Expect.listEquals([new Diff(Operation.equal, 'abc')], diffs, 'diff_cleanupMerge: Merge equalities.');

  diffs = [new Diff(Operation.delete, 'a'), new Diff(Operation.delete, 'b'), new Diff(Operation.delete, 'c')];
  dmp.diff_cleanupMerge(diffs);
  Expect.listEquals([new Diff(Operation.delete, 'abc')], diffs, 'diff_cleanupMerge: Merge deletions.');

  diffs = [new Diff(Operation.insert, 'a'), new Diff(Operation.insert, 'b'), new Diff(Operation.insert, 'c')];
  dmp.diff_cleanupMerge(diffs);
  Expect.listEquals([new Diff(Operation.insert, 'abc')], diffs, 'diff_cleanupMerge: Merge insertions.');

  diffs = [new Diff(Operation.delete, 'a'), new Diff(Operation.insert, 'b'), new Diff(Operation.delete, 'c'), new Diff(Operation.insert, 'd'), new Diff(Operation.equal, 'e'), new Diff(Operation.equal, 'f')];
  dmp.diff_cleanupMerge(diffs);
  Expect.listEquals([new Diff(Operation.delete, 'ac'), new Diff(Operation.insert, 'bd'), new Diff(Operation.equal, 'ef')], diffs, 'diff_cleanupMerge: Merge interweave.');

  diffs = [new Diff(Operation.delete, 'a'), new Diff(Operation.insert, 'abc'), new Diff(Operation.delete, 'dc')];
  dmp.diff_cleanupMerge(diffs);
  Expect.listEquals([new Diff(Operation.equal, 'a'), new Diff(Operation.delete, 'd'), new Diff(Operation.insert, 'b'), new Diff(Operation.equal, 'c')], diffs, 'diff_cleanupMerge: Prefix and suffix detection.');

  diffs = [new Diff(Operation.equal, 'x'), new Diff(Operation.delete, 'a'), new Diff(Operation.insert, 'abc'), new Diff(Operation.delete, 'dc'), new Diff(Operation.equal, 'y')];
  dmp.diff_cleanupMerge(diffs);
  Expect.listEquals([new Diff(Operation.equal, 'xa'), new Diff(Operation.delete, 'd'), new Diff(Operation.insert, 'b'), new Diff(Operation.equal, 'cy')], diffs, 'diff_cleanupMerge: Prefix and suffix detection with equalities.');

  diffs = [new Diff(Operation.equal, 'a'), new Diff(Operation.insert, 'ba'), new Diff(Operation.equal, 'c')];
  dmp.diff_cleanupMerge(diffs);
  Expect.listEquals([new Diff(Operation.insert, 'ab'), new Diff(Operation.equal, 'ac')], diffs, 'diff_cleanupMerge: Slide edit left.');

  diffs = [new Diff(Operation.equal, 'c'), new Diff(Operation.insert, 'ab'), new Diff(Operation.equal, 'a')];
  dmp.diff_cleanupMerge(diffs);
  Expect.listEquals([new Diff(Operation.equal, 'ca'), new Diff(Operation.insert, 'ba')], diffs, 'diff_cleanupMerge: Slide edit right.');

  diffs = [new Diff(Operation.equal, 'a'), new Diff(Operation.delete, 'b'), new Diff(Operation.equal, 'c'), new Diff(Operation.delete, 'ac'), new Diff(Operation.equal, 'x')];
  dmp.diff_cleanupMerge(diffs);
  Expect.listEquals([new Diff(Operation.delete, 'abc'), new Diff(Operation.equal, 'acx')], diffs, 'diff_cleanupMerge: Slide edit left recursive.');

  diffs = [new Diff(Operation.equal, 'x'), new Diff(Operation.delete, 'ca'), new Diff(Operation.equal, 'c'), new Diff(Operation.delete, 'b'), new Diff(Operation.equal, 'a')];
  dmp.diff_cleanupMerge(diffs);
  Expect.listEquals([new Diff(Operation.equal, 'xca'), new Diff(Operation.delete, 'cba')], diffs, 'diff_cleanupMerge: Slide edit right recursive.');

  diffs = [new Diff(Operation.delete, 'b'), new Diff(Operation.insert, 'ab'), new Diff(Operation.equal, 'c')];
  dmp.diff_cleanupMerge(diffs);
  Expect.listEquals([new Diff(Operation.insert, 'a'), new Diff(Operation.equal, 'bc')], diffs, 'diff_cleanupMerge: Empty merge.');

  diffs = [new Diff(Operation.equal, ''), new Diff(Operation.insert, 'a'), new Diff(Operation.equal, 'b')];
  dmp.diff_cleanupMerge(diffs);
  Expect.listEquals([new Diff(Operation.insert, 'a'), new Diff(Operation.equal, 'b')], diffs, 'diff_cleanupMerge: Empty equality.');
}

void testDiffCleanupSemanticLossless() {
  // Slide diffs to match logical boundaries.
  List<Diff> diffs = [];
  dmp.test_diff_cleanupSemanticLossless(diffs);
  Expect.listEquals([], diffs, 'diff_cleanupSemanticLossless: Null case.');

  diffs = [new Diff(Operation.equal, 'AAA\r\n\r\nBBB'), new Diff(Operation.insert, '\r\nDDD\r\n\r\nBBB'), new Diff(Operation.equal, '\r\nEEE')];
  dmp.test_diff_cleanupSemanticLossless(diffs);
  Expect.listEquals([new Diff(Operation.equal, 'AAA\r\n\r\n'), new Diff(Operation.insert, 'BBB\r\nDDD\r\n\r\n'), new Diff(Operation.equal, 'BBB\r\nEEE')], diffs, 'diff_cleanupSemanticLossless: Blank lines.');

  diffs = [new Diff(Operation.equal, 'AAA\r\nBBB'), new Diff(Operation.insert, ' DDD\r\nBBB'), new Diff(Operation.equal, ' EEE')];
  dmp.test_diff_cleanupSemanticLossless(diffs);
  Expect.listEquals([new Diff(Operation.equal, 'AAA\r\n'), new Diff(Operation.insert, 'BBB DDD\r\n'), new Diff(Operation.equal, 'BBB EEE')], diffs, 'diff_cleanupSemanticLossless: Line boundaries.');

  diffs = [new Diff(Operation.equal, 'The c'), new Diff(Operation.insert, 'ow and the c'), new Diff(Operation.equal, 'at.')];
  dmp.test_diff_cleanupSemanticLossless(diffs);
  Expect.listEquals([new Diff(Operation.equal, 'The '), new Diff(Operation.insert, 'cow and the '), new Diff(Operation.equal, 'cat.')], diffs, 'diff_cleanupSemanticLossless: Word boundaries.');

  diffs = [new Diff(Operation.equal, 'The-c'), new Diff(Operation.insert, 'ow-and-the-c'), new Diff(Operation.equal, 'at.')];
  dmp.test_diff_cleanupSemanticLossless(diffs);
  Expect.listEquals([new Diff(Operation.equal, 'The-'), new Diff(Operation.insert, 'cow-and-the-'), new Diff(Operation.equal, 'cat.')], diffs, 'diff_cleanupSemanticLossless: Alphanumeric boundaries.');

  diffs = [new Diff(Operation.equal, 'a'), new Diff(Operation.delete, 'a'), new Diff(Operation.equal, 'ax')];
  dmp.test_diff_cleanupSemanticLossless(diffs);
  Expect.listEquals([new Diff(Operation.delete, 'a'), new Diff(Operation.equal, 'aax')], diffs, 'diff_cleanupSemanticLossless: Hitting the start.');

  diffs = [new Diff(Operation.equal, 'xa'), new Diff(Operation.delete, 'a'), new Diff(Operation.equal, 'a')];
  dmp.test_diff_cleanupSemanticLossless(diffs);
  Expect.listEquals([new Diff(Operation.equal, 'xaa'), new Diff(Operation.delete, 'a')], diffs, 'diff_cleanupSemanticLossless: Hitting the end.');

  diffs = [new Diff(Operation.equal, 'The xxx. The '), new Diff(Operation.insert, 'zzz. The '), new Diff(Operation.equal, 'yyy.')];
  dmp.test_diff_cleanupSemanticLossless(diffs);
  Expect.listEquals([new Diff(Operation.equal, 'The xxx.'), new Diff(Operation.insert, ' The zzz.'), new Diff(Operation.equal, ' The yyy.')], diffs, 'diff_cleanupSemanticLossless: Sentence boundaries.');
}

void testDiffCleanupSemantic() {
  // Cleanup semantically trivial equalities.
  List<Diff> diffs = [];
  dmp.diff_cleanupSemantic(diffs);
  Expect.listEquals([], diffs, 'diff_cleanupSemantic: Null case.');

  diffs = [new Diff(Operation.delete, 'ab'), new Diff(Operation.insert, 'cd'), new Diff(Operation.equal, '12'), new Diff(Operation.delete, 'e')];
  dmp.diff_cleanupSemantic(diffs);
  Expect.listEquals([new Diff(Operation.delete, 'ab'), new Diff(Operation.insert, 'cd'), new Diff(Operation.equal, '12'), new Diff(Operation.delete, 'e')], diffs, 'diff_cleanupSemantic: No elimination #1.');

  diffs = [new Diff(Operation.delete, 'abc'), new Diff(Operation.insert, 'ABC'), new Diff(Operation.equal, '1234'), new Diff(Operation.delete, 'wxyz')];
  dmp.diff_cleanupSemantic(diffs);
  Expect.listEquals([new Diff(Operation.delete, 'abc'), new Diff(Operation.insert, 'ABC'), new Diff(Operation.equal, '1234'), new Diff(Operation.delete, 'wxyz')], diffs, 'diff_cleanupSemantic: No elimination #2.');

  diffs = [new Diff(Operation.delete, 'a'), new Diff(Operation.equal, 'b'), new Diff(Operation.delete, 'c')];
  dmp.diff_cleanupSemantic(diffs);
  Expect.listEquals([new Diff(Operation.delete, 'abc'), new Diff(Operation.insert, 'b')], diffs, 'diff_cleanupSemantic: Simple elimination.');

  diffs = [new Diff(Operation.delete, 'ab'), new Diff(Operation.equal, 'cd'), new Diff(Operation.delete, 'e'), new Diff(Operation.equal, 'f'), new Diff(Operation.insert, 'g')];
  dmp.diff_cleanupSemantic(diffs);
  Expect.listEquals([new Diff(Operation.delete, 'abcdef'), new Diff(Operation.insert, 'cdfg')], diffs, 'diff_cleanupSemantic: Backpass elimination.');

  diffs = [new Diff(Operation.insert, '1'), new Diff(Operation.equal, 'A'), new Diff(Operation.delete, 'B'), new Diff(Operation.insert, '2'), new Diff(Operation.equal, '_'), new Diff(Operation.insert, '1'), new Diff(Operation.equal, 'A'), new Diff(Operation.delete, 'B'), new Diff(Operation.insert, '2')];
  dmp.diff_cleanupSemantic(diffs);
  Expect.listEquals([new Diff(Operation.delete, 'AB_AB'), new Diff(Operation.insert, '1A2_1A2')], diffs, 'diff_cleanupSemantic: Multiple elimination.');

  diffs = [new Diff(Operation.equal, 'The c'), new Diff(Operation.delete, 'ow and the c'), new Diff(Operation.equal, 'at.')];
  dmp.diff_cleanupSemantic(diffs);
  Expect.listEquals([new Diff(Operation.equal, 'The '), new Diff(Operation.delete, 'cow and the '), new Diff(Operation.equal, 'cat.')], diffs, 'diff_cleanupSemantic: Word boundaries.');

  diffs = [new Diff(Operation.delete, 'abcxx'), new Diff(Operation.insert, 'xxdef')];
  dmp.diff_cleanupSemantic(diffs);
  Expect.listEquals([new Diff(Operation.delete, 'abcxx'), new Diff(Operation.insert, 'xxdef')], diffs, 'diff_cleanupSemantic: No overlap elimination.');

  diffs = [new Diff(Operation.delete, 'abcxxx'), new Diff(Operation.insert, 'xxxdef')];
  dmp.diff_cleanupSemantic(diffs);
  Expect.listEquals([new Diff(Operation.delete, 'abc'), new Diff(Operation.equal, 'xxx'), new Diff(Operation.insert, 'def')], diffs, 'diff_cleanupSemantic: Overlap elimination.');

  diffs = [new Diff(Operation.delete, 'xxxabc'), new Diff(Operation.insert, 'defxxx')];
  dmp.diff_cleanupSemantic(diffs);
  Expect.listEquals([new Diff(Operation.insert, 'def'), new Diff(Operation.equal, 'xxx'), new Diff(Operation.delete, 'abc')], diffs, 'diff_cleanupSemantic: Reverse overlap elimination.');

  diffs = [new Diff(Operation.delete, 'abcd1212'), new Diff(Operation.insert, '1212efghi'), new Diff(Operation.equal, '----'), new Diff(Operation.delete, 'A3'), new Diff(Operation.insert, '3BC')];
  dmp.diff_cleanupSemantic(diffs);
  Expect.listEquals([new Diff(Operation.delete, 'abcd'), new Diff(Operation.equal, '1212'), new Diff(Operation.insert, 'efghi'), new Diff(Operation.equal, '----'), new Diff(Operation.delete, 'A'), new Diff(Operation.equal, '3'), new Diff(Operation.insert, 'BC')], diffs, 'diff_cleanupSemantic: Two overlap eliminations.');
}

void testDiffCleanupEfficiency() {
  // Cleanup operationally trivial equalities.
  dmp.Diff_EditCost = 4;
  List<Diff> diffs = [];
  dmp.diff_cleanupEfficiency(diffs);
  Expect.listEquals([], diffs, 'diff_cleanupEfficiency: Null case.');

  diffs = [new Diff(Operation.delete, 'ab'), new Diff(Operation.insert, '12'), new Diff(Operation.equal, 'wxyz'), new Diff(Operation.delete, 'cd'), new Diff(Operation.insert, '34')];
  dmp.diff_cleanupEfficiency(diffs);
  Expect.listEquals([new Diff(Operation.delete, 'ab'), new Diff(Operation.insert, '12'), new Diff(Operation.equal, 'wxyz'), new Diff(Operation.delete, 'cd'), new Diff(Operation.insert, '34')], diffs, 'diff_cleanupEfficiency: No elimination.');

  diffs = [new Diff(Operation.delete, 'ab'), new Diff(Operation.insert, '12'), new Diff(Operation.equal, 'xyz'), new Diff(Operation.delete, 'cd'), new Diff(Operation.insert, '34')];
  dmp.diff_cleanupEfficiency(diffs);
  Expect.listEquals([new Diff(Operation.delete, 'abxyzcd'), new Diff(Operation.insert, '12xyz34')], diffs, 'diff_cleanupEfficiency: Four-edit elimination.');

  diffs = [new Diff(Operation.insert, '12'), new Diff(Operation.equal, 'x'), new Diff(Operation.delete, 'cd'), new Diff(Operation.insert, '34')];
  dmp.diff_cleanupEfficiency(diffs);
  Expect.listEquals([new Diff(Operation.delete, 'xcd'), new Diff(Operation.insert, '12x34')], diffs, 'diff_cleanupEfficiency: Three-edit elimination.');

  diffs = [new Diff(Operation.delete, 'ab'), new Diff(Operation.insert, '12'), new Diff(Operation.equal, 'xy'), new Diff(Operation.insert, '34'), new Diff(Operation.equal, 'z'), new Diff(Operation.delete, 'cd'), new Diff(Operation.insert, '56')];
  dmp.diff_cleanupEfficiency(diffs);
  Expect.listEquals([new Diff(Operation.delete, 'abxyzcd'), new Diff(Operation.insert, '12xy34z56')], diffs, 'diff_cleanupEfficiency: Backpass elimination.');

  dmp.Diff_EditCost = 5;
  diffs = [new Diff(Operation.delete, 'ab'), new Diff(Operation.insert, '12'), new Diff(Operation.equal, 'wxyz'), new Diff(Operation.delete, 'cd'), new Diff(Operation.insert, '34')];
  dmp.diff_cleanupEfficiency(diffs);
  Expect.listEquals([new Diff(Operation.delete, 'abwxyzcd'), new Diff(Operation.insert, '12wxyz34')], diffs, 'diff_cleanupEfficiency: High cost elimination.');
  dmp.Diff_EditCost = 4;
}

void testDiffPrettyHtml() {
  // Pretty print.
  List<Diff> diffs = [new Diff(Operation.equal, 'a\n'), new Diff(Operation.delete, '<B>b</B>'), new Diff(Operation.insert, 'c&d')];
  Expect.equals('<span>a&para;<br></span><del style="background:#ffe6e6;">&lt;B&gt;b&lt;/B&gt;</del><ins style="background:#e6ffe6;">c&amp;d</ins>', dmp.diff_prettyHtml(diffs), 'diff_prettyHtml:');
}

void testDiffText() {
  // Compute the source and destination texts.
  List<Diff> diffs = [new Diff(Operation.equal, 'jump'), new Diff(Operation.delete, 's'), new Diff(Operation.insert, 'ed'), new Diff(Operation.equal, ' over '), new Diff(Operation.delete, 'the'), new Diff(Operation.insert, 'a'), new Diff(Operation.equal, ' lazy')];
  Expect.equals('jumps over the lazy', dmp.diff_text1(diffs), 'diff_text1:');
  Expect.equals('jumped over a lazy', dmp.diff_text2(diffs), 'diff_text2:');
}

void testDiffDelta() {
  // Convert a diff into delta string.
  List<Diff> diffs = [new Diff(Operation.equal, 'jump'), new Diff(Operation.delete, 's'), new Diff(Operation.insert, 'ed'), new Diff(Operation.equal, ' over '), new Diff(Operation.delete, 'the'), new Diff(Operation.insert, 'a'), new Diff(Operation.equal, ' lazy'), new Diff(Operation.insert, 'old dog')];
  String text1 = dmp.diff_text1(diffs);
  Expect.equals('jumps over the lazy', text1, 'diff_text1: Base text.');

  String delta = dmp.diff_toDelta(diffs);
  Expect.equals('=4\t-1\t+ed\t=6\t-3\t+a\t=5\t+old dog', delta, 'diff_toDelta:');

  // Convert delta string into a diff.
  Expect.listEquals(diffs, dmp.diff_fromDelta(text1, delta), 'diff_fromDelta: Normal.');

  // Generates error (19 < 20).
  Expect.throws(() => dmp.diff_fromDelta(text1 + 'x', delta), 'diff_fromDelta: Too long.');

  // Generates error (19 > 18).
  Expect.throws(() => dmp.diff_fromDelta(text1.substring(1), delta), 'diff_fromDelta: Too short.');

  // Generates error (%c3%xy invalid Unicode).
  Expect.throws(() => dmp.diff_fromDelta('', '+%c3%xy'), 'diff_fromDelta: Invalid character.');

  // Test deltas with special characters.
  diffs = [new Diff(Operation.equal, '\u0680 \x00 \t %'), new Diff(Operation.delete, '\u0681 \x01 \n ^'), new Diff(Operation.insert, '\u0682 \x02 \\ |')];
  text1 = dmp.diff_text1(diffs);
  Expect.equals('\u0680 \x00 \t %\u0681 \x01 \n ^', text1, 'diff_text1: Unicode text.');

  delta = dmp.diff_toDelta(diffs);
  Expect.equals('=7\t-7\t+%DA%82 %02 %5C %7C', delta, 'diff_toDelta: Unicode.');

  Expect.listEquals(diffs, dmp.diff_fromDelta(text1, delta), 'diff_fromDelta: Unicode.');

  // Verify pool of unchanged characters.
  diffs = [new Diff(Operation.insert, 'A-Z a-z 0-9 - _ . ! ~ * \' ( ) ; / ? : @ & = + \$ , # ')];
  String text2 = dmp.diff_text2(diffs);
  Expect.equals('A-Z a-z 0-9 - _ . ! ~ * \' ( ) ; / ? : @ & = + \$ , # ', text2, 'diff_text2: Unchanged characters.');

  delta = dmp.diff_toDelta(diffs);
  Expect.equals('+A-Z a-z 0-9 - _ . ! ~ * \' ( ) ; / ? : @ & = + \$ , # ', delta, 'diff_toDelta: Unchanged characters.');

  // Convert delta string into a diff.
  Expect.listEquals(diffs, dmp.diff_fromDelta('', delta), 'diff_fromDelta: Unchanged characters.');

  // 160 kb string.
  var a = 'abcdefghij';
  for (var i = 0; i < 14; i++) {
    a += a;
  }
  diffs = [new Diff(Operation.insert, a)];
  delta = dmp.diff_toDelta(diffs);
  Expect.equals('+' + a, delta, 'diff_toDelta: 160kb string.');

  // Convert delta string into a diff.
  Expect.listEquals(diffs, dmp.diff_fromDelta('', delta), 'diff_fromDelta: 160kb string.');
}

void testDiffXIndex() {
  // Translate a location in text1 to text2.
  List<Diff> diffs = [new Diff(Operation.delete, 'a'), new Diff(Operation.insert, '1234'), new Diff(Operation.equal, 'xyz')];
  Expect.equals(5, dmp.diff_xIndex(diffs, 2), 'diff_xIndex: Translation on equality.');

  diffs = [new Diff(Operation.equal, 'a'), new Diff(Operation.delete, '1234'), new Diff(Operation.equal, 'xyz')];
  Expect.equals(1, dmp.diff_xIndex(diffs, 3), 'diff_xIndex: Translation on deletion.');
}

void testDiffLevenshtein() {
  List<Diff> diffs = [new Diff(Operation.delete, 'abc'), new Diff(Operation.insert, '1234'), new Diff(Operation.equal, 'xyz')];
  Expect.equals(4, dmp.diff_levenshtein(diffs), 'Levenshtein with trailing equality.');

  diffs = [new Diff(Operation.equal, 'xyz'), new Diff(Operation.delete, 'abc'), new Diff(Operation.insert, '1234')];
  Expect.equals(4, dmp.diff_levenshtein(diffs), 'Levenshtein with leading equality.');

  diffs = [new Diff(Operation.delete, 'abc'), new Diff(Operation.equal, 'xyz'), new Diff(Operation.insert, '1234')];
  Expect.equals(7, dmp.diff_levenshtein(diffs), 'Levenshtein with middle equality.');
}

void testDiffBisect() {
  // Normal.
  String a = 'cat';
  String b = 'map';
  // Since the resulting diff hasn't been normalized, it would be ok if
  // the insertion and deletion pairs are swapped.
  // If the order changes, tweak this test as required.
  List<Diff> diffs = [new Diff(Operation.delete, 'c'), new Diff(Operation.insert, 'm'), new Diff(Operation.equal, 'a'), new Diff(Operation.delete, 't'), new Diff(Operation.insert, 'p')];
  // One year should be sufficient.
  DateTime deadline = new DateTime.now().add(new Duration(days : 365));
  Expect.listEquals(diffs, dmp.test_diff_bisect(a, b, deadline), 'diff_bisect: Normal.');

  // Timeout.
  diffs = [new Diff(Operation.delete, 'cat'), new Diff(Operation.insert, 'map')];
  // Set deadline to one year ago.
  deadline = new DateTime.now().subtract(new Duration(days : 365));
  Expect.listEquals(diffs, dmp.test_diff_bisect(a, b, deadline), 'diff_bisect: Timeout.');
}

void testDiffMain() {
  // Perform a trivial diff.
  List<Diff> diffs = [];
  Expect.listEquals(diffs, dmp.diff_main('', '', false), 'diff_main: Null case.');

  diffs = [new Diff(Operation.equal, 'abc')];
  Expect.listEquals(diffs, dmp.diff_main('abc', 'abc', false), 'diff_main: Equality.');

  diffs = [new Diff(Operation.equal, 'ab'), new Diff(Operation.insert, '123'), new Diff(Operation.equal, 'c')];
  Expect.listEquals(diffs, dmp.diff_main('abc', 'ab123c', false), 'diff_main: Simple insertion.');

  diffs = [new Diff(Operation.equal, 'a'), new Diff(Operation.delete, '123'), new Diff(Operation.equal, 'bc')];
  Expect.listEquals(diffs, dmp.diff_main('a123bc', 'abc', false), 'diff_main: Simple deletion.');

  diffs = [new Diff(Operation.equal, 'a'), new Diff(Operation.insert, '123'), new Diff(Operation.equal, 'b'), new Diff(Operation.insert, '456'), new Diff(Operation.equal, 'c')];
  Expect.listEquals(diffs, dmp.diff_main('abc', 'a123b456c', false), 'diff_main: Two insertions.');

  diffs = [new Diff(Operation.equal, 'a'), new Diff(Operation.delete, '123'), new Diff(Operation.equal, 'b'), new Diff(Operation.delete, '456'), new Diff(Operation.equal, 'c')];
  Expect.listEquals(diffs, dmp.diff_main('a123b456c', 'abc', false), 'diff_main: Two deletions.');

  // Perform a real diff.
  // Switch off the timeout.
  dmp.Diff_Timeout = 0.0;
  diffs = [new Diff(Operation.delete, 'a'), new Diff(Operation.insert, 'b')];
  Expect.listEquals(diffs, dmp.diff_main('a', 'b', false), 'diff_main: Simple case #1.');

  diffs = [new Diff(Operation.delete, 'Apple'), new Diff(Operation.insert, 'Banana'), new Diff(Operation.equal, 's are a'), new Diff(Operation.insert, 'lso'), new Diff(Operation.equal, ' fruit.')];
  Expect.listEquals(diffs, dmp.diff_main('Apples are a fruit.', 'Bananas are also fruit.', false), 'diff_main: Simple case #2.');

  diffs = [new Diff(Operation.delete, 'a'), new Diff(Operation.insert, '\u0680'), new Diff(Operation.equal, 'x'), new Diff(Operation.delete, '\t'), new Diff(Operation.insert, '\000')];
  Expect.listEquals(diffs, dmp.diff_main('ax\t', '\u0680x\000', false), 'diff_main: Simple case #3.');

  diffs = [new Diff(Operation.delete, '1'), new Diff(Operation.equal, 'a'), new Diff(Operation.delete, 'y'), new Diff(Operation.equal, 'b'), new Diff(Operation.delete, '2'), new Diff(Operation.insert, 'xab')];
  Expect.listEquals(diffs, dmp.diff_main('1ayb2', 'abxab', false), 'diff_main: Overlap #1.');

  diffs = [new Diff(Operation.insert, 'xaxcx'), new Diff(Operation.equal, 'abc'), new Diff(Operation.delete, 'y')];
  Expect.listEquals(diffs, dmp.diff_main('abcy', 'xaxcxabc', false), 'diff_main: Overlap #2.');

  diffs = [new Diff(Operation.delete, 'ABCD'), new Diff(Operation.equal, 'a'), new Diff(Operation.delete, '='), new Diff(Operation.insert, '-'), new Diff(Operation.equal, 'bcd'), new Diff(Operation.delete, '='), new Diff(Operation.insert, '-'), new Diff(Operation.equal, 'efghijklmnopqrs'), new Diff(Operation.delete, 'EFGHIJKLMNOefg')];
  Expect.listEquals(diffs, dmp.diff_main('ABCDa=bcd=efghijklmnopqrsEFGHIJKLMNOefg', 'a-bcd-efghijklmnopqrs', false), 'diff_main: Overlap #3.');

  diffs = [new Diff(Operation.insert, ' '), new Diff(Operation.equal, 'a'), new Diff(Operation.insert, 'nd'), new Diff(Operation.equal, ' [[Pennsylvania]]'), new Diff(Operation.delete, ' and [[New')];
  Expect.listEquals(diffs, dmp.diff_main('a [[Pennsylvania]] and [[New', ' and [[Pennsylvania]]', false), 'diff_main: Large equality.');

  dmp.Diff_Timeout = 0.1;  // 100ms
  String a = '`Twas brillig, and the slithy toves\nDid gyre and gimble in the wabe:\nAll mimsy were the borogoves,\nAnd the mome raths outgrabe.\n';
  String b = 'I am the very model of a modern major general,\nI\'ve information vegetable, animal, and mineral,\nI know the kings of England, and I quote the fights historical,\nFrom Marathon to Waterloo, in order categorical.\n';
  // Increase the text lengths by 1024 times to ensure a timeout.
  for (int i = 0; i < 10; i++) {
    a += a;
    b += b;
  }
  DateTime startTime = new DateTime.now();
  dmp.diff_main(a, b);
  DateTime endTime = new DateTime.now();
  double elapsedSeconds = endTime.difference(startTime).inMilliseconds / 1000;
  // Test that we took at least the timeout period.
  Expect.isTrue(dmp.Diff_Timeout <= elapsedSeconds, 'diff_main: Timeout min.');
  // Test that we didn't take forever (be forgiving).
  // Theoretically this test could fail very occasionally if the
  // OS task swaps or locks up for a second at the wrong moment.
  Expect.isTrue(dmp.Diff_Timeout * 2 > elapsedSeconds, 'diff_main: Timeout max.');
  dmp.Diff_Timeout = 0.0;

  // Test the linemode speedup.
  // Must be long to pass the 100 char cutoff.
  a = '1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n';
  b = 'abcdefghij\nabcdefghij\nabcdefghij\nabcdefghij\nabcdefghij\nabcdefghij\nabcdefghij\nabcdefghij\nabcdefghij\nabcdefghij\nabcdefghij\nabcdefghij\nabcdefghij\n';
  Expect.listEquals(dmp.diff_main(a, b, true), dmp.diff_main(a, b, false), 'diff_main: Simple line-mode.');

  a = '1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890';
  b = 'abcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghij';
  Expect.listEquals(dmp.diff_main(a, b, true), dmp.diff_main(a, b, false), 'diff_main: Single line-mode.');

  a = '1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n';
  b = 'abcdefghij\n1234567890\n1234567890\n1234567890\nabcdefghij\n1234567890\n1234567890\n1234567890\nabcdefghij\n1234567890\n1234567890\n1234567890\nabcdefghij\n';
  List<String> texts_linemode = _diff_rebuildtexts(dmp.diff_main(a, b, true));
  List<String> texts_textmode = _diff_rebuildtexts(dmp.diff_main(a, b, false));
  Expect.listEquals(texts_textmode, texts_linemode, 'diff_main: Overlap line-mode.');

  // Test null inputs.
  Expect.throws(() => dmp.diff_main(null, null), 'diff_main: Null inputs.');
}


//  MATCH TEST FUNCTIONS

void testMatchAlphabet() {
  // Initialise the bitmasks for Bitap.
  Map<String, int> bitmask = {'a': 4, 'b': 2, 'c': 1};
  Expect.mapEquals(bitmask, dmp.test_match_alphabet('abc'), 'match_alphabet: Unique.');

  bitmask = {'a': 37, 'b': 18, 'c': 8};
  Expect.mapEquals(bitmask, dmp.test_match_alphabet('abcaba'), 'match_alphabet: Duplicates.');
}

void testMatchBitap() {
  // Bitap algorithm.
  dmp.Match_Distance = 100;
  dmp.Match_Threshold = 0.5;
  Expect.equals(5, dmp.test_match_bitap('abcdefghijk', 'fgh', 5), 'match_bitap: Exact match #1.');

  Expect.equals(5, dmp.test_match_bitap('abcdefghijk', 'fgh', 0), 'match_bitap: Exact match #2.');

  Expect.equals(4, dmp.test_match_bitap('abcdefghijk', 'efxhi', 0), 'match_bitap: Fuzzy match #1.');

  Expect.equals(2, dmp.test_match_bitap('abcdefghijk', 'cdefxyhijk', 5), 'match_bitap: Fuzzy match #2.');

  Expect.equals(-1, dmp.test_match_bitap('abcdefghijk', 'bxy', 1), 'match_bitap: Fuzzy match #3.');

  Expect.equals(2, dmp.test_match_bitap('123456789xx0', '3456789x0', 2), 'match_bitap: Overflow.');

  Expect.equals(0, dmp.test_match_bitap('abcdef', 'xxabc', 4), 'match_bitap: Before start match.');

  Expect.equals(3, dmp.test_match_bitap('abcdef', 'defyy', 4), 'match_bitap: Beyond end match.');

  Expect.equals(0, dmp.test_match_bitap('abcdef', 'xabcdefy', 0), 'match_bitap: Oversized pattern.');

  dmp.Match_Threshold = 0.4;
  Expect.equals(4, dmp.test_match_bitap('abcdefghijk', 'efxyhi', 1), 'match_bitap: Threshold #1.');

  dmp.Match_Threshold = 0.3;
  Expect.equals(-1, dmp.test_match_bitap('abcdefghijk', 'efxyhi', 1), 'match_bitap: Threshold #2.');

  dmp.Match_Threshold = 0.0;
  Expect.equals(1, dmp.test_match_bitap('abcdefghijk', 'bcdef', 1), 'match_bitap: Threshold #3.');

  dmp.Match_Threshold = 0.5;
  Expect.equals(0, dmp.test_match_bitap('abcdexyzabcde', 'abccde', 3), 'match_bitap: Multiple select #1.');

  Expect.equals(8, dmp.test_match_bitap('abcdexyzabcde', 'abccde', 5), 'match_bitap: Multiple select #2.');

  dmp.Match_Distance = 10;  // Strict location.
  Expect.equals(-1, dmp.test_match_bitap('abcdefghijklmnopqrstuvwxyz', 'abcdefg', 24), 'match_bitap: Distance test #1.');

  Expect.equals(0, dmp.test_match_bitap('abcdefghijklmnopqrstuvwxyz', 'abcdxxefg', 1), 'match_bitap: Distance test #2.');

  dmp.Match_Distance = 1000;  // Loose location.
  Expect.equals(0, dmp.test_match_bitap('abcdefghijklmnopqrstuvwxyz', 'abcdefg', 24), 'match_bitap: Distance test #3.');
}

void testMatchMain() {
  // Full match.
  Expect.equals(0, dmp.match_main('abcdef', 'abcdef', 1000), 'match_main: Equality.');

  Expect.equals(-1, dmp.match_main('', 'abcdef', 1), 'match_main: Null text.');

  Expect.equals(3, dmp.match_main('abcdef', '', 3), 'match_main: Null pattern.');

  Expect.equals(3, dmp.match_main('abcdef', 'de', 3), 'match_main: Exact match.');

  Expect.equals(3, dmp.match_main('abcdef', 'defy', 4), 'match_main: Beyond end match.');

  Expect.equals(0, dmp.match_main('abcdef', 'abcdefy', 0), 'match_main: Oversized pattern.');

  dmp.Match_Threshold = 0.7;
  Expect.equals(4, dmp.match_main('I am the very model of a modern major general.', ' that berry ', 5), 'match_main: Complex match.');
  dmp.Match_Threshold = 0.5;

  // Test null inputs.
  Expect.throws(() => dmp.match_main(null, null, 0), 'match_main: Null inputs.');
}


//  PATCH TEST FUNCTIONS


void testPatchObj() {
  // Patch Object.
  Patch p = new Patch();
  p.start1 = 20;
  p.start2 = 21;
  p.length1 = 18;
  p.length2 = 17;
  p.diffs = [new Diff(Operation.equal, 'jump'), new Diff(Operation.delete, 's'), new Diff(Operation.insert, 'ed'), new Diff(Operation.equal, ' over '), new Diff(Operation.delete, 'the'), new Diff(Operation.insert, 'a'), new Diff(Operation.equal, '\nlaz')];
  String strp = '@@ -21,18 +22,17 @@\n jump\n-s\n+ed\n  over \n-the\n+a\n %0Alaz\n';
  Expect.equals(strp, p.toString(), 'Patch: toString.');
}

void testPatchFromText() {
  Expect.isTrue(dmp.patch_fromText('').isEmpty, 'patch_fromText: #0.');

  String strp = '@@ -21,18 +22,17 @@\n jump\n-s\n+ed\n  over \n-the\n+a\n %0Alaz\n';
  Expect.equals(strp, dmp.patch_fromText(strp)[0].toString(), 'patch_fromText: #1.');

  Expect.equals('@@ -1 +1 @@\n-a\n+b\n', dmp.patch_fromText('@@ -1 +1 @@\n-a\n+b\n')[0].toString(), 'patch_fromText: #2.');

  Expect.equals('@@ -1,3 +0,0 @@\n-abc\n', dmp.patch_fromText('@@ -1,3 +0,0 @@\n-abc\n')[0].toString(), 'patch_fromText: #3.');

  Expect.equals('@@ -0,0 +1,3 @@\n+abc\n', dmp.patch_fromText('@@ -0,0 +1,3 @@\n+abc\n')[0].toString(), 'patch_fromText: #4.');

  // Generates error.
  Expect.throws(() => dmp.patch_fromText('Bad\nPatch\n'), 'patch_fromText: #5.');
}

void testPatchToText() {
  String strp = '@@ -21,18 +22,17 @@\n jump\n-s\n+ed\n  over \n-the\n+a\n  laz\n';
  List<Patch> patches;
  patches = dmp.patch_fromText(strp);
  Expect.equals(strp, dmp.patch_toText(patches), 'patch_toText: Single.');

  strp = '@@ -1,9 +1,9 @@\n-f\n+F\n oo+fooba\n@@ -7,9 +7,9 @@\n obar\n-,\n+.\n  tes\n';
  patches = dmp.patch_fromText(strp);
  Expect.equals(strp, dmp.patch_toText(patches), 'patch_toText: Dual.');
}

void testPatchAddContext() {
  dmp.Patch_Margin = 4;
  Patch p;
  p = dmp.patch_fromText('@@ -21,4 +21,10 @@\n-jump\n+somersault\n')[0];
  dmp.test_patch_addContext(p, 'The quick brown fox jumps over the lazy dog.');
  Expect.equals('@@ -17,12 +17,18 @@\n fox \n-jump\n+somersault\n s ov\n', p.toString(), 'patch_addContext: Simple case.');

  p = dmp.patch_fromText('@@ -21,4 +21,10 @@\n-jump\n+somersault\n')[0];
  dmp.test_patch_addContext(p, 'The quick brown fox jumps.');
  Expect.equals('@@ -17,10 +17,16 @@\n fox \n-jump\n+somersault\n s.\n', p.toString(), 'patch_addContext: Not enough trailing context.');

  p = dmp.patch_fromText('@@ -3 +3,2 @@\n-e\n+at\n')[0];
  dmp.test_patch_addContext(p, 'The quick brown fox jumps.');
  Expect.equals('@@ -1,7 +1,8 @@\n Th\n-e\n+at\n  qui\n', p.toString(), 'patch_addContext: Not enough leading context.');

  p = dmp.patch_fromText('@@ -3 +3,2 @@\n-e\n+at\n')[0];
  dmp.test_patch_addContext(p, 'The quick brown fox jumps.  The quick brown fox crashes.');
  Expect.equals('@@ -1,27 +1,28 @@\n Th\n-e\n+at\n  quick brown fox jumps. \n', p.toString(), 'patch_addContext: Ambiguity.');
}

void testPatchMake() {
  List<Patch> patches;
  patches = dmp.patch_make('', '');
  Expect.equals('', dmp.patch_toText(patches), 'patch_make: Null case.');

  String text1 = 'The quick brown fox jumps over the lazy dog.';
  String text2 = 'That quick brown fox jumped over a lazy dog.';
  String expectedPatch = '@@ -1,8 +1,7 @@\n Th\n-at\n+e\n  qui\n@@ -21,17 +21,18 @@\n jump\n-ed\n+s\n  over \n-a\n+the\n  laz\n';
  // The second patch must be '-21,17 +21,18', not '-22,17 +21,18' due to rolling context.
  patches = dmp.patch_make(text2, text1);
  Expect.equals(expectedPatch, dmp.patch_toText(patches), 'patch_make: Text2+Text1 inputs.');

  expectedPatch = '@@ -1,11 +1,12 @@\n Th\n-e\n+at\n  quick b\n@@ -22,18 +22,17 @@\n jump\n-s\n+ed\n  over \n-the\n+a\n  laz\n';
  patches = dmp.patch_make(text1, text2);
  Expect.equals(expectedPatch, dmp.patch_toText(patches), 'patch_make: Text1+Text2 inputs.');

  List<Diff> diffs = dmp.diff_main(text1, text2, false);
  patches = dmp.patch_make(diffs);
  Expect.equals(expectedPatch, dmp.patch_toText(patches), 'patch_make: Diff input.');

  patches = dmp.patch_make(text1, diffs);
  Expect.equals(expectedPatch, dmp.patch_toText(patches), 'patch_make: Text1+Diff inputs.');

  patches = dmp.patch_make(text1, text2, diffs);
  Expect.equals(expectedPatch, dmp.patch_toText(patches), 'patch_make: Text1+Text2+Diff inputs (deprecated).');

  patches = dmp.patch_make('`1234567890-=[]\\;\',./', '~!@#\$%^&*()_+{}|:"<>?');
  Expect.equals('@@ -1,21 +1,21 @@\n-%601234567890-=%5B%5D%5C;\',./\n+~!@#\$%25%5E&*()_+%7B%7D%7C:%22%3C%3E?\n', dmp.patch_toText(patches), 'patch_toText: Character encoding.');

  diffs = [new Diff(Operation.delete, '`1234567890-=[]\\;\',./'), new Diff(Operation.insert, '~!@#\$%^&*()_+{}|:"<>?')];
  Expect.listEquals(diffs, dmp.patch_fromText('@@ -1,21 +1,21 @@\n-%601234567890-=%5B%5D%5C;\',./\n+~!@#\$%25%5E&*()_+%7B%7D%7C:%22%3C%3E?\n')[0].diffs, 'patch_fromText: Character decoding.');

  final sb = new StringBuffer();
  for (int x = 0; x < 100; x++) {
    sb.write('abcdef');
  }
  text1 = sb.toString();
  text2 = text1 + '123';
  expectedPatch = '@@ -573,28 +573,31 @@\n cdefabcdefabcdefabcdefabcdef\n+123\n';
  patches = dmp.patch_make(text1, text2);
  Expect.equals(expectedPatch, dmp.patch_toText(patches), 'patch_make: Long string with repeats.');

  // Test null inputs.
  Expect.throws(() => dmp.patch_make(null), 'patch_make: Null inputs.');
}

void testPatchSplitMax() {
  // Assumes that Match_MaxBits is 32.
  List<Patch> patches;
  patches = dmp.patch_make('abcdefghijklmnopqrstuvwxyz01234567890', 'XabXcdXefXghXijXklXmnXopXqrXstXuvXwxXyzX01X23X45X67X89X0');
  dmp.patch_splitMax(patches);
  Expect.equals('@@ -1,32 +1,46 @@\n+X\n ab\n+X\n cd\n+X\n ef\n+X\n gh\n+X\n ij\n+X\n kl\n+X\n mn\n+X\n op\n+X\n qr\n+X\n st\n+X\n uv\n+X\n wx\n+X\n yz\n+X\n 012345\n@@ -25,13 +39,18 @@\n zX01\n+X\n 23\n+X\n 45\n+X\n 67\n+X\n 89\n+X\n 0\n', dmp.patch_toText(patches), 'patch_splitMax: #1.');

  patches = dmp.patch_make('abcdef1234567890123456789012345678901234567890123456789012345678901234567890uvwxyz', 'abcdefuvwxyz');
  String oldToText = dmp.patch_toText(patches);
  dmp.patch_splitMax(patches);
  Expect.equals(oldToText, dmp.patch_toText(patches), 'patch_splitMax: #2.');

  patches = dmp.patch_make('1234567890123456789012345678901234567890123456789012345678901234567890', 'abc');
  dmp.patch_splitMax(patches);
  Expect.equals('@@ -1,32 +1,4 @@\n-1234567890123456789012345678\n 9012\n@@ -29,32 +1,4 @@\n-9012345678901234567890123456\n 7890\n@@ -57,14 +1,3 @@\n-78901234567890\n+abc\n', dmp.patch_toText(patches), 'patch_splitMax: #3.');

  patches = dmp.patch_make('abcdefghij , h : 0 , t : 1 abcdefghij , h : 0 , t : 1 abcdefghij , h : 0 , t : 1', 'abcdefghij , h : 1 , t : 1 abcdefghij , h : 1 , t : 1 abcdefghij , h : 0 , t : 1');
  dmp.patch_splitMax(patches);
  Expect.equals('@@ -2,32 +2,32 @@\n bcdefghij , h : \n-0\n+1\n  , t : 1 abcdef\n@@ -29,32 +29,32 @@\n bcdefghij , h : \n-0\n+1\n  , t : 1 abcdef\n', dmp.patch_toText(patches), 'patch_splitMax: #4.');
}

void testPatchAddPadding() {
  List<Patch> patches;
  patches = dmp.patch_make('', 'test');
  Expect.equals('@@ -0,0 +1,4 @@\n+test\n', dmp.patch_toText(patches), 'patch_addPadding: Both edges full.');
  dmp.patch_addPadding(patches);
  Expect.equals('@@ -1,8 +1,12 @@\n %01%02%03%04\n+test\n %01%02%03%04\n', dmp.patch_toText(patches), 'patch_addPadding: Both edges full.');

  patches = dmp.patch_make('XY', 'XtestY');
  Expect.equals('@@ -1,2 +1,6 @@\n X\n+test\n Y\n', dmp.patch_toText(patches), 'patch_addPadding: Both edges partial.');
  dmp.patch_addPadding(patches);
  Expect.equals('@@ -2,8 +2,12 @@\n %02%03%04X\n+test\n Y%01%02%03\n', dmp.patch_toText(patches), 'patch_addPadding: Both edges partial.');

  patches = dmp.patch_make('XXXXYYYY', 'XXXXtestYYYY');
  Expect.equals('@@ -1,8 +1,12 @@\n XXXX\n+test\n YYYY\n', dmp.patch_toText(patches), 'patch_addPadding: Both edges none.');
  dmp.patch_addPadding(patches);
  Expect.equals('@@ -5,8 +5,12 @@\n XXXX\n+test\n YYYY\n', dmp.patch_toText(patches), 'patch_addPadding: Both edges none.');
}

void testPatchApply() {
  dmp.Match_Distance = 1000;
  dmp.Match_Threshold = 0.5;
  dmp.Patch_DeleteThreshold = 0.5;
  List<Patch> patches;
  patches = dmp.patch_make('', '');
  List results = dmp.patch_apply(patches, 'Hello world.');
  List boolArray = results[1];
  String resultStr = '${results[0]}\t${boolArray.length}';
  Expect.equals('Hello world.\t0', resultStr, 'patch_apply: Null case.');

  patches = dmp.patch_make('The quick brown fox jumps over the lazy dog.', 'That quick brown fox jumped over a lazy dog.');
  results = dmp.patch_apply(patches, 'The quick brown fox jumps over the lazy dog.');
  boolArray = results[1];
  resultStr = '${results[0]}\t${boolArray[0]}\t${boolArray[1]}';
  Expect.equals('That quick brown fox jumped over a lazy dog.\ttrue\ttrue', resultStr, 'patch_apply: Exact match.');

  results = dmp.patch_apply(patches, 'The quick red rabbit jumps over the tired tiger.');
  boolArray = results[1];
  resultStr = '${results[0]}\t${boolArray[0]}\t${boolArray[1]}';
  Expect.equals('That quick red rabbit jumped over a tired tiger.\ttrue\ttrue', resultStr, 'patch_apply: Partial match.');

  results = dmp.patch_apply(patches, 'I am the very model of a modern major general.');
  boolArray = results[1];
  resultStr = '${results[0]}\t${boolArray[0]}\t${boolArray[1]}';
  Expect.equals('I am the very model of a modern major general.\tfalse\tfalse', resultStr, 'patch_apply: Failed match.');

  patches = dmp.patch_make('x1234567890123456789012345678901234567890123456789012345678901234567890y', 'xabcy');
  results = dmp.patch_apply(patches, 'x123456789012345678901234567890-----++++++++++-----123456789012345678901234567890y');
  boolArray = results[1];
  resultStr = '${results[0]}\t${boolArray[0]}\t${boolArray[1]}';
  Expect.equals('xabcy\ttrue\ttrue', resultStr, 'patch_apply: Big delete, small change.');

  patches = dmp.patch_make('x1234567890123456789012345678901234567890123456789012345678901234567890y', 'xabcy');
  results = dmp.patch_apply(patches, 'x12345678901234567890---------------++++++++++---------------12345678901234567890y');
  boolArray = results[1];
  resultStr = '${results[0]}\t${boolArray[0]}\t${boolArray[1]}';
  Expect.equals('xabc12345678901234567890---------------++++++++++---------------12345678901234567890y\tfalse\ttrue', resultStr, 'patch_apply: Big delete, big change 1.');

  dmp.Patch_DeleteThreshold = 0.6;
  patches = dmp.patch_make('x1234567890123456789012345678901234567890123456789012345678901234567890y', 'xabcy');
  results = dmp.patch_apply(patches, 'x12345678901234567890---------------++++++++++---------------12345678901234567890y');
  boolArray = results[1];
  resultStr = '${results[0]}\t${boolArray[0]}\t${boolArray[1]}';
  Expect.equals('xabcy\ttrue\ttrue', resultStr, 'patch_apply: Big delete, big change 2.');
  dmp.Patch_DeleteThreshold = 0.5;

  // Compensate for failed patch.
  dmp.Match_Threshold = 0.0;
  dmp.Match_Distance = 0;
  patches = dmp.patch_make('abcdefghijklmnopqrstuvwxyz--------------------1234567890', 'abcXXXXXXXXXXdefghijklmnopqrstuvwxyz--------------------1234567YYYYYYYYYY890');
  results = dmp.patch_apply(patches, 'ABCDEFGHIJKLMNOPQRSTUVWXYZ--------------------1234567890');
  boolArray = results[1];
  resultStr = '${results[0]}\t${boolArray[0]}\t${boolArray[1]}';
  Expect.equals('ABCDEFGHIJKLMNOPQRSTUVWXYZ--------------------1234567YYYYYYYYYY890\tfalse\ttrue', resultStr, 'patch_apply: Compensate for failed patch.');
  dmp.Match_Threshold = 0.5;
  dmp.Match_Distance = 1000;

  patches = dmp.patch_make('', 'test');
  String patchStr = dmp.patch_toText(patches);
  dmp.patch_apply(patches, '');
  Expect.equals(patchStr, dmp.patch_toText(patches), 'patch_apply: No side effects.');

  patches = dmp.patch_make('The quick brown fox jumps over the lazy dog.', 'Woof');
  patchStr = dmp.patch_toText(patches);
  dmp.patch_apply(patches, 'The quick brown fox jumps over the lazy dog.');
  Expect.equals(patchStr, dmp.patch_toText(patches), 'patch_apply: No side effects with major delete.');

  patches = dmp.patch_make('', 'test');
  results = dmp.patch_apply(patches, '');
  boolArray = results[1];
  resultStr = '${results[0]}\t${boolArray[0]}';
  Expect.equals('test\ttrue', resultStr, 'patch_apply: Edge exact match.');

  patches = dmp.patch_make('XY', 'XtestY');
  results = dmp.patch_apply(patches, 'XY');
  boolArray = results[1];
  resultStr = '${results[0]}\t${boolArray[0]}';
  Expect.equals('XtestY\ttrue', resultStr, 'patch_apply: Near edge exact match.');

  patches = dmp.patch_make('y', 'y123');
  results = dmp.patch_apply(patches, 'x');
  boolArray = results[1];
  resultStr = '${results[0]}\t${boolArray[0]}';
  Expect.equals('x123\ttrue', resultStr, 'patch_apply: Edge partial match.');
}

// Run each test.
main() {
  dmp = new DiffMatchPatch();

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

  print('All tests passed.');
}
