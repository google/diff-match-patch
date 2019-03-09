/**
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

declare var assertEquals;
declare var assertTrue;
declare var assertFalse;

// If expected and actual are the equivalent, pass the test.
function assertEquivalent(msg, expected, actual?) {
  if (typeof actual == 'undefined') {
    // msg is optional.
    actual = expected;
    expected = msg;
    msg = 'Expected: \'' + expected + '\' Actual: \'' + actual + '\'';
  }
  if (_equivalent(expected, actual)) {
    return assertEquals(msg, String(expected), String(actual));
  } else {
    return assertEquals(msg, expected, actual);
  }
}


// Are a and b the equivalent? -- Recursive.
function _equivalent(a, b) {
  if (a == b) {
    return true;
  }
  if (typeof a == 'object' && typeof b == 'object' && a !== null && b !== null) {
    if (a.toString() != b.toString()) {
      return false;
    }
    for (var p in a) {
      if (a.hasOwnProperty(p) && !_equivalent(a[p], b[p])) {
        return false;
      }
    }
    for (var p in b) {
      if (a.hasOwnProperty(p) && !_equivalent(a[p], b[p])) {
        return false;
      }
    }
    return true;
  }
  return false;
}


function newDiff(op: number, text: string) {
  return new diff_match_patch.Diff(op, text)
}

function diff_rebuildtexts(diffs) {
  // Construct the two texts which made up the diff originally.
  var text1 = '';
  var text2 = '';
  for (var x = 0; x < diffs.length; x++) {
    if (diffs[x].op != DIFF_INSERT) {
      text1 += diffs[x].text;
    }
    if (diffs[x].op != DIFF_DELETE) {
      text2 += diffs[x].text;
    }
  }
  return [text1, text2];
}

var dmp = new diff_match_patch();


// DIFF TEST FUNCTIONS


function testDiffCommonPrefix() {
  // Detect any common prefix.
  // Null case.
  assertEquals(0, dmp.diff_commonPrefix('abc', 'xyz'));

  // Non-null case.
  assertEquals(4, dmp.diff_commonPrefix('1234abcdef', '1234xyz'));

  // Whole case.
  assertEquals(4, dmp.diff_commonPrefix('1234', '1234xyz'));
}

function testDiffCommonSuffix() {
  // Detect any common suffix.
  // Null case.
  assertEquals(0, dmp.diff_commonSuffix('abc', 'xyz'));

  // Non-null case.
  assertEquals(4, dmp.diff_commonSuffix('abcdef1234', 'xyz1234'));

  // Whole case.
  assertEquals(4, dmp.diff_commonSuffix('1234', 'xyz1234'));
}

function testDiffCommonOverlap() {
  // Detect any suffix/prefix overlap.
  // Null case.
  assertEquals(0, dmp.diff_commonOverlap_('', 'abcd'));

  // Whole case.
  assertEquals(3, dmp.diff_commonOverlap_('abc', 'abcd'));

  // No overlap.
  assertEquals(0, dmp.diff_commonOverlap_('123456', 'abcd'));

  // Overlap.
  assertEquals(3, dmp.diff_commonOverlap_('123456xxx', 'xxxabcd'));

  // Unicode.
  // Some overly clever languages (C#) may treat ligatures as equal to their
  // component letters.  E.g. U+FB01 == 'fi'
  assertEquals(0, dmp.diff_commonOverlap_('fi', '\ufb01i'));
}

function testDiffHalfMatch() {
  // Detect a halfmatch.
  dmp.Diff_Timeout = 1;
  // No match.
  assertEquals(null, dmp.diff_halfMatch_('1234567890', 'abcdef'));

  assertEquals(null, dmp.diff_halfMatch_('12345', '23'));

  // Single Match.
  assertEquivalent(['12', '90', 'a', 'z', '345678'], dmp.diff_halfMatch_('1234567890', 'a345678z'));

  assertEquivalent(['a', 'z', '12', '90', '345678'], dmp.diff_halfMatch_('a345678z', '1234567890'));

  assertEquivalent(['abc', 'z', '1234', '0', '56789'], dmp.diff_halfMatch_('abc56789z', '1234567890'));

  assertEquivalent(['a', 'xyz', '1', '7890', '23456'], dmp.diff_halfMatch_('a23456xyz', '1234567890'));

  // Multiple Matches.
  assertEquivalent(['12123', '123121', 'a', 'z', '1234123451234'], dmp.diff_halfMatch_('121231234123451234123121', 'a1234123451234z'));

  assertEquivalent(['', '-=-=-=-=-=', 'x', '', 'x-=-=-=-=-=-=-='], dmp.diff_halfMatch_('x-=-=-=-=-=-=-=-=-=-=-=-=', 'xx-=-=-=-=-=-=-='));

  assertEquivalent(['-=-=-=-=-=', '', '', 'y', '-=-=-=-=-=-=-=y'], dmp.diff_halfMatch_('-=-=-=-=-=-=-=-=-=-=-=-=y', '-=-=-=-=-=-=-=yy'));

  // Non-optimal halfmatch.
  // Optimal diff would be -q+x=H-i+e=lloHe+Hu=llo-Hew+y not -qHillo+x=HelloHe-w+Hulloy
  assertEquivalent(['qHillo', 'w', 'x', 'Hulloy', 'HelloHe'], dmp.diff_halfMatch_('qHilloHelloHew', 'xHelloHeHulloy'));

  // Optimal no halfmatch.
  dmp.Diff_Timeout = 0;
  assertEquals(null, dmp.diff_halfMatch_('qHilloHelloHew', 'xHelloHeHulloy'));
}

function testDiffLinesToChars() {
  function assertLinesToCharsResultEquals(a, b) {
    assertEquals(a.chars1, b.chars1);
    assertEquals(a.chars2, b.chars2);
    assertEquivalent(a.lineArray, b.lineArray);
  }

  // Convert lines down to characters.
  assertLinesToCharsResultEquals({chars1: '\x01\x02\x01', chars2: '\x02\x01\x02', lineArray: ['', 'alpha\n', 'beta\n']}, dmp.diff_linesToChars_('alpha\nbeta\nalpha\n', 'beta\nalpha\nbeta\n'));

  assertLinesToCharsResultEquals({chars1: '', chars2: '\x01\x02\x03\x03', lineArray: ['', 'alpha\r\n', 'beta\r\n', '\r\n']}, dmp.diff_linesToChars_('', 'alpha\r\nbeta\r\n\r\n\r\n'));

  assertLinesToCharsResultEquals({chars1: '\x01', chars2: '\x02', lineArray: ['', 'a', 'b']}, dmp.diff_linesToChars_('a', 'b'));

  // More than 256 to reveal any 8-bit limitations.
  var n = 300;
  var lineList = [];
  var charList = [];
  for (var i = 1; i < n + 1; i++) {
    lineList[i - 1] = i + '\n';
    charList[i - 1] = String.fromCharCode(i);
  }
  assertEquals(n, lineList.length);
  var lines = lineList.join('');
  var chars = charList.join('');
  assertEquals(n, chars.length);
  lineList.unshift('');
  assertLinesToCharsResultEquals({chars1: chars, chars2: '', lineArray: lineList}, dmp.diff_linesToChars_(lines, ''));
}

function testDiffCharsToLines() {
  // Convert chars up to lines.
  var diffs = [newDiff(DIFF_EQUAL, '\x01\x02\x01'), newDiff(DIFF_INSERT, '\x02\x01\x02')];
  dmp.diff_charsToLines_(diffs, ['', 'alpha\n', 'beta\n']);
  assertEquivalent([newDiff(DIFF_EQUAL, 'alpha\nbeta\nalpha\n'), newDiff(DIFF_INSERT, 'beta\nalpha\nbeta\n')], diffs);

  // More than 256 to reveal any 8-bit limitations.
  var n = 300;
  var lineList = [];
  var charList = [];
  for (var i = 1; i < n + 1; i++) {
    lineList[i - 1] = i + '\n';
    charList[i - 1] = String.fromCharCode(i);
  }
  assertEquals(n, lineList.length);
  var lines = lineList.join('');
  var chars = charList.join('');
  assertEquals(n, chars.length);
  lineList.unshift('');
  var diffs = [newDiff(DIFF_DELETE, chars)];
  dmp.diff_charsToLines_(diffs, lineList);
  assertEquivalent([newDiff(DIFF_DELETE, lines)], diffs);

  // More than 65536 to verify any 16-bit limitation.
  lineList = [];
  for (var i = 0; i < 66000; i++) {
    lineList[i] = i + '\n';
  }
  chars = lineList.join('');
  var results = dmp.diff_linesToChars_(chars, '');
  diffs = [newDiff(DIFF_INSERT, results.chars1)];
  dmp.diff_charsToLines_(diffs, results.lineArray);
  assertEquals(chars, diffs[0].text);
}

function testDiffCleanupMerge() {
  // Cleanup a messy diff.
  // Null case.
  var diffs = [];
  dmp.diff_cleanupMerge(diffs);
  assertEquivalent([], diffs);

  // No change case.
  diffs = [newDiff(DIFF_EQUAL, 'a'), newDiff(DIFF_DELETE, 'b'), newDiff(DIFF_INSERT, 'c')];
  dmp.diff_cleanupMerge(diffs);
  assertEquivalent([newDiff(DIFF_EQUAL, 'a'), newDiff(DIFF_DELETE, 'b'), newDiff(DIFF_INSERT, 'c')], diffs);

  // Merge equalities.
  diffs = [newDiff(DIFF_EQUAL, 'a'), newDiff(DIFF_EQUAL, 'b'), newDiff(DIFF_EQUAL, 'c')];
  dmp.diff_cleanupMerge(diffs);
  assertEquivalent([newDiff(DIFF_EQUAL, 'abc')], diffs);

  // Merge deletions.
  diffs = [newDiff(DIFF_DELETE, 'a'), newDiff(DIFF_DELETE, 'b'), newDiff(DIFF_DELETE, 'c')];
  dmp.diff_cleanupMerge(diffs);
  assertEquivalent([newDiff(DIFF_DELETE, 'abc')], diffs);

  // Merge insertions.
  diffs = [newDiff(DIFF_INSERT, 'a'), newDiff(DIFF_INSERT, 'b'), newDiff(DIFF_INSERT, 'c')];
  dmp.diff_cleanupMerge(diffs);
  assertEquivalent([newDiff(DIFF_INSERT, 'abc')], diffs);

  // Merge interweave.
  diffs = [newDiff(DIFF_DELETE, 'a'), newDiff(DIFF_INSERT, 'b'), newDiff(DIFF_DELETE, 'c'), newDiff(DIFF_INSERT, 'd'), newDiff(DIFF_EQUAL, 'e'), newDiff(DIFF_EQUAL, 'f')];
  dmp.diff_cleanupMerge(diffs);
  assertEquivalent([newDiff(DIFF_DELETE, 'ac'), newDiff(DIFF_INSERT, 'bd'), newDiff(DIFF_EQUAL, 'ef')], diffs);

  // Prefix and suffix detection.
  diffs = [newDiff(DIFF_DELETE, 'a'), newDiff(DIFF_INSERT, 'abc'), newDiff(DIFF_DELETE, 'dc')];
  dmp.diff_cleanupMerge(diffs);
  assertEquivalent([newDiff(DIFF_EQUAL, 'a'), newDiff(DIFF_DELETE, 'd'), newDiff(DIFF_INSERT, 'b'), newDiff(DIFF_EQUAL, 'c')], diffs);

  // Prefix and suffix detection with equalities.
  diffs = [newDiff(DIFF_EQUAL, 'x'), newDiff(DIFF_DELETE, 'a'), newDiff(DIFF_INSERT, 'abc'), newDiff(DIFF_DELETE, 'dc'), newDiff(DIFF_EQUAL, 'y')];
  dmp.diff_cleanupMerge(diffs);
  assertEquivalent([newDiff(DIFF_EQUAL, 'xa'), newDiff(DIFF_DELETE, 'd'), newDiff(DIFF_INSERT, 'b'), newDiff(DIFF_EQUAL, 'cy')], diffs);

  // Slide edit left.
  diffs = [newDiff(DIFF_EQUAL, 'a'), newDiff(DIFF_INSERT, 'ba'), newDiff(DIFF_EQUAL, 'c')];
  dmp.diff_cleanupMerge(diffs);
  assertEquivalent([newDiff(DIFF_INSERT, 'ab'), newDiff(DIFF_EQUAL, 'ac')], diffs);

  // Slide edit right.
  diffs = [newDiff(DIFF_EQUAL, 'c'), newDiff(DIFF_INSERT, 'ab'), newDiff(DIFF_EQUAL, 'a')];
  dmp.diff_cleanupMerge(diffs);
  assertEquivalent([newDiff(DIFF_EQUAL, 'ca'), newDiff(DIFF_INSERT, 'ba')], diffs);

  // Slide edit left recursive.
  diffs = [newDiff(DIFF_EQUAL, 'a'), newDiff(DIFF_DELETE, 'b'), newDiff(DIFF_EQUAL, 'c'), newDiff(DIFF_DELETE, 'ac'), newDiff(DIFF_EQUAL, 'x')];
  dmp.diff_cleanupMerge(diffs);
  assertEquivalent([newDiff(DIFF_DELETE, 'abc'), newDiff(DIFF_EQUAL, 'acx')], diffs);

  // Slide edit right recursive.
  diffs = [newDiff(DIFF_EQUAL, 'x'), newDiff(DIFF_DELETE, 'ca'), newDiff(DIFF_EQUAL, 'c'), newDiff(DIFF_DELETE, 'b'), newDiff(DIFF_EQUAL, 'a')];
  dmp.diff_cleanupMerge(diffs);
  assertEquivalent([newDiff(DIFF_EQUAL, 'xca'), newDiff(DIFF_DELETE, 'cba')], diffs);

  // Empty merge.
  diffs = [newDiff(DIFF_DELETE, 'b'), newDiff(DIFF_INSERT, 'ab'), newDiff(DIFF_EQUAL, 'c')];
  dmp.diff_cleanupMerge(diffs);
  assertEquivalent([newDiff(DIFF_INSERT, 'a'), newDiff(DIFF_EQUAL, 'bc')], diffs);

  // Empty equality.
  diffs = [newDiff(DIFF_EQUAL, ''), newDiff(DIFF_INSERT, 'a'), newDiff(DIFF_EQUAL, 'b')];
  dmp.diff_cleanupMerge(diffs);
  assertEquivalent([newDiff(DIFF_INSERT, 'a'), newDiff(DIFF_EQUAL, 'b')], diffs);
}

function testDiffCleanupSemanticLossless() {
  // Slide diffs to match logical boundaries.
  // Null case.
  var diffs = [];
  dmp.diff_cleanupSemanticLossless(diffs);
  assertEquivalent([], diffs);

  // Blank lines.
  diffs = [newDiff(DIFF_EQUAL, 'AAA\r\n\r\nBBB'), newDiff(DIFF_INSERT, '\r\nDDD\r\n\r\nBBB'), newDiff(DIFF_EQUAL, '\r\nEEE')];
  dmp.diff_cleanupSemanticLossless(diffs);
  assertEquivalent([newDiff(DIFF_EQUAL, 'AAA\r\n\r\n'), newDiff(DIFF_INSERT, 'BBB\r\nDDD\r\n\r\n'), newDiff(DIFF_EQUAL, 'BBB\r\nEEE')], diffs);

  // Line boundaries.
  diffs = [newDiff(DIFF_EQUAL, 'AAA\r\nBBB'), newDiff(DIFF_INSERT, ' DDD\r\nBBB'), newDiff(DIFF_EQUAL, ' EEE')];
  dmp.diff_cleanupSemanticLossless(diffs);
  assertEquivalent([newDiff(DIFF_EQUAL, 'AAA\r\n'), newDiff(DIFF_INSERT, 'BBB DDD\r\n'), newDiff(DIFF_EQUAL, 'BBB EEE')], diffs);

  // Word boundaries.
  diffs = [newDiff(DIFF_EQUAL, 'The c'), newDiff(DIFF_INSERT, 'ow and the c'), newDiff(DIFF_EQUAL, 'at.')];
  dmp.diff_cleanupSemanticLossless(diffs);
  assertEquivalent([newDiff(DIFF_EQUAL, 'The '), newDiff(DIFF_INSERT, 'cow and the '), newDiff(DIFF_EQUAL, 'cat.')], diffs);

  // Alphanumeric boundaries.
  diffs = [newDiff(DIFF_EQUAL, 'The-c'), newDiff(DIFF_INSERT, 'ow-and-the-c'), newDiff(DIFF_EQUAL, 'at.')];
  dmp.diff_cleanupSemanticLossless(diffs);
  assertEquivalent([newDiff(DIFF_EQUAL, 'The-'), newDiff(DIFF_INSERT, 'cow-and-the-'), newDiff(DIFF_EQUAL, 'cat.')], diffs);

  // Hitting the start.
  diffs = [newDiff(DIFF_EQUAL, 'a'), newDiff(DIFF_DELETE, 'a'), newDiff(DIFF_EQUAL, 'ax')];
  dmp.diff_cleanupSemanticLossless(diffs);
  assertEquivalent([newDiff(DIFF_DELETE, 'a'), newDiff(DIFF_EQUAL, 'aax')], diffs);

  // Hitting the end.
  diffs = [newDiff(DIFF_EQUAL, 'xa'), newDiff(DIFF_DELETE, 'a'), newDiff(DIFF_EQUAL, 'a')];
  dmp.diff_cleanupSemanticLossless(diffs);
  assertEquivalent([newDiff(DIFF_EQUAL, 'xaa'), newDiff(DIFF_DELETE, 'a')], diffs);

  // Sentence boundaries.
  diffs = [newDiff(DIFF_EQUAL, 'The xxx. The '), newDiff(DIFF_INSERT, 'zzz. The '), newDiff(DIFF_EQUAL, 'yyy.')];
  dmp.diff_cleanupSemanticLossless(diffs);
  assertEquivalent([newDiff(DIFF_EQUAL, 'The xxx.'), newDiff(DIFF_INSERT, ' The zzz.'), newDiff(DIFF_EQUAL, ' The yyy.')], diffs);
}

function testDiffCleanupSemantic() {
  // Cleanup semantically trivial equalities.
  // Null case.
  var diffs = [];
  dmp.diff_cleanupSemantic(diffs);
  assertEquivalent([], diffs);

  // No elimination #1.
  diffs = [newDiff(DIFF_DELETE, 'ab'), newDiff(DIFF_INSERT, 'cd'), newDiff(DIFF_EQUAL, '12'), newDiff(DIFF_DELETE, 'e')];
  dmp.diff_cleanupSemantic(diffs);
  assertEquivalent([newDiff(DIFF_DELETE, 'ab'), newDiff(DIFF_INSERT, 'cd'), newDiff(DIFF_EQUAL, '12'), newDiff(DIFF_DELETE, 'e')], diffs);

  // No elimination #2.
  diffs = [newDiff(DIFF_DELETE, 'abc'), newDiff(DIFF_INSERT, 'ABC'), newDiff(DIFF_EQUAL, '1234'), newDiff(DIFF_DELETE, 'wxyz')];
  dmp.diff_cleanupSemantic(diffs);
  assertEquivalent([newDiff(DIFF_DELETE, 'abc'), newDiff(DIFF_INSERT, 'ABC'), newDiff(DIFF_EQUAL, '1234'), newDiff(DIFF_DELETE, 'wxyz')], diffs);

  // Simple elimination.
  diffs = [newDiff(DIFF_DELETE, 'a'), newDiff(DIFF_EQUAL, 'b'), newDiff(DIFF_DELETE, 'c')];
  dmp.diff_cleanupSemantic(diffs);
  assertEquivalent([newDiff(DIFF_DELETE, 'abc'), newDiff(DIFF_INSERT, 'b')], diffs);

  // Backpass elimination.
  diffs = [newDiff(DIFF_DELETE, 'ab'), newDiff(DIFF_EQUAL, 'cd'), newDiff(DIFF_DELETE, 'e'), newDiff(DIFF_EQUAL, 'f'), newDiff(DIFF_INSERT, 'g')];
  dmp.diff_cleanupSemantic(diffs);
  assertEquivalent([newDiff(DIFF_DELETE, 'abcdef'), newDiff(DIFF_INSERT, 'cdfg')], diffs);

  // Multiple eliminations.
  diffs = [newDiff(DIFF_INSERT, '1'), newDiff(DIFF_EQUAL, 'A'), newDiff(DIFF_DELETE, 'B'), newDiff(DIFF_INSERT, '2'), newDiff(DIFF_EQUAL, '_'), newDiff(DIFF_INSERT, '1'), newDiff(DIFF_EQUAL, 'A'), newDiff(DIFF_DELETE, 'B'), newDiff(DIFF_INSERT, '2')];
  dmp.diff_cleanupSemantic(diffs);
  assertEquivalent([newDiff(DIFF_DELETE, 'AB_AB'), newDiff(DIFF_INSERT, '1A2_1A2')], diffs);

  // Word boundaries.
  diffs = [newDiff(DIFF_EQUAL, 'The c'), newDiff(DIFF_DELETE, 'ow and the c'), newDiff(DIFF_EQUAL, 'at.')];
  dmp.diff_cleanupSemantic(diffs);
  assertEquivalent([newDiff(DIFF_EQUAL, 'The '), newDiff(DIFF_DELETE, 'cow and the '), newDiff(DIFF_EQUAL, 'cat.')], diffs);

  // No overlap elimination.
  diffs = [newDiff(DIFF_DELETE, 'abcxx'), newDiff(DIFF_INSERT, 'xxdef')];
  dmp.diff_cleanupSemantic(diffs);
  assertEquivalent([newDiff(DIFF_DELETE, 'abcxx'), newDiff(DIFF_INSERT, 'xxdef')], diffs);

  // Overlap elimination.
  diffs = [newDiff(DIFF_DELETE, 'abcxxx'), newDiff(DIFF_INSERT, 'xxxdef')];
  dmp.diff_cleanupSemantic(diffs);
  assertEquivalent([newDiff(DIFF_DELETE, 'abc'), newDiff(DIFF_EQUAL, 'xxx'), newDiff(DIFF_INSERT, 'def')], diffs);

  // Reverse overlap elimination.
  diffs = [newDiff(DIFF_DELETE, 'xxxabc'), newDiff(DIFF_INSERT, 'defxxx')];
  dmp.diff_cleanupSemantic(diffs);
  assertEquivalent([newDiff(DIFF_INSERT, 'def'), newDiff(DIFF_EQUAL, 'xxx'), newDiff(DIFF_DELETE, 'abc')], diffs);

  // Two overlap eliminations.
  diffs = [newDiff(DIFF_DELETE, 'abcd1212'), newDiff(DIFF_INSERT, '1212efghi'), newDiff(DIFF_EQUAL, '----'), newDiff(DIFF_DELETE, 'A3'), newDiff(DIFF_INSERT, '3BC')];
  dmp.diff_cleanupSemantic(diffs);
  assertEquivalent([newDiff(DIFF_DELETE, 'abcd'), newDiff(DIFF_EQUAL, '1212'), newDiff(DIFF_INSERT, 'efghi'), newDiff(DIFF_EQUAL, '----'), newDiff(DIFF_DELETE, 'A'), newDiff(DIFF_EQUAL, '3'), newDiff(DIFF_INSERT, 'BC')], diffs);
}

function testDiffCleanupEfficiency() {
  // Cleanup operationally trivial equalities.
  dmp.Diff_EditCost = 4;
  // Null case.
  var diffs = [];
  dmp.diff_cleanupEfficiency(diffs);
  assertEquivalent([], diffs);

  // No elimination.
  diffs = [newDiff(DIFF_DELETE, 'ab'), newDiff(DIFF_INSERT, '12'), newDiff(DIFF_EQUAL, 'wxyz'), newDiff(DIFF_DELETE, 'cd'), newDiff(DIFF_INSERT, '34')];
  dmp.diff_cleanupEfficiency(diffs);
  assertEquivalent([newDiff(DIFF_DELETE, 'ab'), newDiff(DIFF_INSERT, '12'), newDiff(DIFF_EQUAL, 'wxyz'), newDiff(DIFF_DELETE, 'cd'), newDiff(DIFF_INSERT, '34')], diffs);

  // Four-edit elimination.
  diffs = [newDiff(DIFF_DELETE, 'ab'), newDiff(DIFF_INSERT, '12'), newDiff(DIFF_EQUAL, 'xyz'), newDiff(DIFF_DELETE, 'cd'), newDiff(DIFF_INSERT, '34')];
  dmp.diff_cleanupEfficiency(diffs);
  assertEquivalent([newDiff(DIFF_DELETE, 'abxyzcd'), newDiff(DIFF_INSERT, '12xyz34')], diffs);

  // Three-edit elimination.
  diffs = [newDiff(DIFF_INSERT, '12'), newDiff(DIFF_EQUAL, 'x'), newDiff(DIFF_DELETE, 'cd'), newDiff(DIFF_INSERT, '34')];
  dmp.diff_cleanupEfficiency(diffs);
  assertEquivalent([newDiff(DIFF_DELETE, 'xcd'), newDiff(DIFF_INSERT, '12x34')], diffs);

  // Backpass elimination.
  diffs = [newDiff(DIFF_DELETE, 'ab'), newDiff(DIFF_INSERT, '12'), newDiff(DIFF_EQUAL, 'xy'), newDiff(DIFF_INSERT, '34'), newDiff(DIFF_EQUAL, 'z'), newDiff(DIFF_DELETE, 'cd'), newDiff(DIFF_INSERT, '56')];
  dmp.diff_cleanupEfficiency(diffs);
  assertEquivalent([newDiff(DIFF_DELETE, 'abxyzcd'), newDiff(DIFF_INSERT, '12xy34z56')], diffs);

  // High cost elimination.
  dmp.Diff_EditCost = 5;
  diffs = [newDiff(DIFF_DELETE, 'ab'), newDiff(DIFF_INSERT, '12'), newDiff(DIFF_EQUAL, 'wxyz'), newDiff(DIFF_DELETE, 'cd'), newDiff(DIFF_INSERT, '34')];
  dmp.diff_cleanupEfficiency(diffs);
  assertEquivalent([newDiff(DIFF_DELETE, 'abwxyzcd'), newDiff(DIFF_INSERT, '12wxyz34')], diffs);
  dmp.Diff_EditCost = 4;
}

function testDiffPrettyHtml() {
  // Pretty print.
  var diffs = [newDiff(DIFF_EQUAL, 'a\n'), newDiff(DIFF_DELETE, '<B>b</B>'), newDiff(DIFF_INSERT, 'c&d')];
  assertEquals('<span>a&para;<br></span><del style="background:#ffe6e6;">&lt;B&gt;b&lt;/B&gt;</del><ins style="background:#e6ffe6;">c&amp;d</ins>', dmp.diff_prettyHtml(diffs));
}

function testDiffText() {
  // Compute the source and destination texts.
  var diffs = [newDiff(DIFF_EQUAL, 'jump'), newDiff(DIFF_DELETE, 's'), newDiff(DIFF_INSERT, 'ed'), newDiff(DIFF_EQUAL, ' over '), newDiff(DIFF_DELETE, 'the'), newDiff(DIFF_INSERT, 'a'), newDiff(DIFF_EQUAL, ' lazy')];
  assertEquals('jumps over the lazy', dmp.diff_text1(diffs));

  assertEquals('jumped over a lazy', dmp.diff_text2(diffs));
}

function testDiffDelta() {
  // Convert a diff into delta string.
  var diffs = [newDiff(DIFF_EQUAL, 'jump'), newDiff(DIFF_DELETE, 's'), newDiff(DIFF_INSERT, 'ed'), newDiff(DIFF_EQUAL, ' over '), newDiff(DIFF_DELETE, 'the'), newDiff(DIFF_INSERT, 'a'), newDiff(DIFF_EQUAL, ' lazy'), newDiff(DIFF_INSERT, 'old dog')];
  var text1 = dmp.diff_text1(diffs);
  assertEquals('jumps over the lazy', text1);

  var delta = dmp.diff_toDelta(diffs);
  assertEquals('=4\t-1\t+ed\t=6\t-3\t+a\t=5\t+old dog', delta);

  // Convert delta string into a diff.
  assertEquivalent(diffs, dmp.diff_fromDelta(text1, delta));

  // Generates error (19 != 20).
  try {
    dmp.diff_fromDelta(text1 + 'x', delta);
    assertEquals(Error, null);
  } catch (e) {
    // Exception expected.
  }

  // Generates error (19 != 18).
  try {
    dmp.diff_fromDelta(text1.substring(1), delta);
    assertEquals(Error, null);
  } catch (e) {
    // Exception expected.
  }

  // Generates error (%c3%xy invalid Unicode).
  try {
    dmp.diff_fromDelta('', '+%c3%xy');
    assertEquals(Error, null);
  } catch (e) {
    // Exception expected.
  }

  // Test deltas with special characters.
  diffs = [newDiff(DIFF_EQUAL, '\u0680 \x00 \t %'), newDiff(DIFF_DELETE, '\u0681 \x01 \n ^'), newDiff(DIFF_INSERT, '\u0682 \x02 \\ |')];
  text1 = dmp.diff_text1(diffs);
  assertEquals('\u0680 \x00 \t %\u0681 \x01 \n ^', text1);

  delta = dmp.diff_toDelta(diffs);
  assertEquals('=7\t-7\t+%DA%82 %02 %5C %7C', delta);

  // Convert delta string into a diff.
  assertEquivalent(diffs, dmp.diff_fromDelta(text1, delta));

  // Verify pool of unchanged characters.
  diffs = [newDiff(DIFF_INSERT, 'A-Z a-z 0-9 - _ . ! ~ * \' ( ) ; / ? : @ & = + $ , # ')];
  var text2 = dmp.diff_text2(diffs);
  assertEquals('A-Z a-z 0-9 - _ . ! ~ * \' ( ) ; / ? : @ & = + $ , # ', text2);

  delta = dmp.diff_toDelta(diffs);
  assertEquals('+A-Z a-z 0-9 - _ . ! ~ * \' ( ) ; / ? : @ & = + $ , # ', delta);

  // Convert delta string into a diff.
  assertEquivalent(diffs, dmp.diff_fromDelta('', delta));

  // 160 kb string.
  var a = 'abcdefghij';
  for (var i = 0; i < 14; i++) {
    a += a;
  }
  diffs = [newDiff(DIFF_INSERT, a)];
  delta = dmp.diff_toDelta(diffs);
  assertEquals('+' + a, delta);

  // Convert delta string into a diff.
  assertEquivalent(diffs, dmp.diff_fromDelta('', delta));
}

function testDiffXIndex() {
  // Translate a location in text1 to text2.
  // Translation on equality.
  assertEquals(5, dmp.diff_xIndex([newDiff(DIFF_DELETE, 'a'), newDiff(DIFF_INSERT, '1234'), newDiff(DIFF_EQUAL, 'xyz')], 2));

  // Translation on deletion.
  assertEquals(1, dmp.diff_xIndex([newDiff(DIFF_EQUAL, 'a'), newDiff(DIFF_DELETE, '1234'), newDiff(DIFF_EQUAL, 'xyz')], 3));
}

function testDiffLevenshtein() {
  // Levenshtein with trailing equality.
  assertEquals(4, dmp.diff_levenshtein([newDiff(DIFF_DELETE, 'abc'), newDiff(DIFF_INSERT, '1234'), newDiff(DIFF_EQUAL, 'xyz')]));
  // Levenshtein with leading equality.
  assertEquals(4, dmp.diff_levenshtein([newDiff(DIFF_EQUAL, 'xyz'), newDiff(DIFF_DELETE, 'abc'), newDiff(DIFF_INSERT, '1234')]));
  // Levenshtein with middle equality.
  assertEquals(7, dmp.diff_levenshtein([newDiff(DIFF_DELETE, 'abc'), newDiff(DIFF_EQUAL, 'xyz'), newDiff(DIFF_INSERT, '1234')]));
}

function testDiffBisect() {
  // Normal.
  var a = 'cat';
  var b = 'map';
  // Since the resulting diff hasn't been normalized, it would be ok if
  // the insertion and deletion pairs are swapped.
  // If the order changes, tweak this test as required.
  assertEquivalent([newDiff(DIFF_DELETE, 'c'), newDiff(DIFF_INSERT, 'm'), newDiff(DIFF_EQUAL, 'a'), newDiff(DIFF_DELETE, 't'), newDiff(DIFF_INSERT, 'p')], dmp.diff_bisect_(a, b, Number.MAX_VALUE));

  // Timeout.
  assertEquivalent([newDiff(DIFF_DELETE, 'cat'), newDiff(DIFF_INSERT, 'map')], dmp.diff_bisect_(a, b, 0));
}

function testDiffMain() {
  // Perform a trivial diff.
  // Null case.
  assertEquivalent([], dmp.diff_main('', '', false));

  // Equality.
  assertEquivalent([newDiff(DIFF_EQUAL, 'abc')], dmp.diff_main('abc', 'abc', false));

  // Simple insertion.
  assertEquivalent([newDiff(DIFF_EQUAL, 'ab'), newDiff(DIFF_INSERT, '123'), newDiff(DIFF_EQUAL, 'c')], dmp.diff_main('abc', 'ab123c', false));

  // Simple deletion.
  assertEquivalent([newDiff(DIFF_EQUAL, 'a'), newDiff(DIFF_DELETE, '123'), newDiff(DIFF_EQUAL, 'bc')], dmp.diff_main('a123bc', 'abc', false));

  // Two insertions.
  assertEquivalent([newDiff(DIFF_EQUAL, 'a'), newDiff(DIFF_INSERT, '123'), newDiff(DIFF_EQUAL, 'b'), newDiff(DIFF_INSERT, '456'), newDiff(DIFF_EQUAL, 'c')], dmp.diff_main('abc', 'a123b456c', false));

  // Two deletions.
  assertEquivalent([newDiff(DIFF_EQUAL, 'a'), newDiff(DIFF_DELETE, '123'), newDiff(DIFF_EQUAL, 'b'), newDiff(DIFF_DELETE, '456'), newDiff(DIFF_EQUAL, 'c')], dmp.diff_main('a123b456c', 'abc', false));

  // Perform a real diff.
  // Switch off the timeout.
  dmp.Diff_Timeout = 0;
  // Simple cases.
  assertEquivalent([newDiff(DIFF_DELETE, 'a'), newDiff(DIFF_INSERT, 'b')], dmp.diff_main('a', 'b', false));

  assertEquivalent([newDiff(DIFF_DELETE, 'Apple'), newDiff(DIFF_INSERT, 'Banana'), newDiff(DIFF_EQUAL, 's are a'), newDiff(DIFF_INSERT, 'lso'), newDiff(DIFF_EQUAL, ' fruit.')], dmp.diff_main('Apples are a fruit.', 'Bananas are also fruit.', false));

  assertEquivalent([newDiff(DIFF_DELETE, 'a'), newDiff(DIFF_INSERT, '\u0680'), newDiff(DIFF_EQUAL, 'x'), newDiff(DIFF_DELETE, '\t'), newDiff(DIFF_INSERT, '\0')], dmp.diff_main('ax\t', '\u0680x\0', false));

  // Overlaps.
  assertEquivalent([newDiff(DIFF_DELETE, '1'), newDiff(DIFF_EQUAL, 'a'), newDiff(DIFF_DELETE, 'y'), newDiff(DIFF_EQUAL, 'b'), newDiff(DIFF_DELETE, '2'), newDiff(DIFF_INSERT, 'xab')], dmp.diff_main('1ayb2', 'abxab', false));

  assertEquivalent([newDiff(DIFF_INSERT, 'xaxcx'), newDiff(DIFF_EQUAL, 'abc'), newDiff(DIFF_DELETE, 'y')], dmp.diff_main('abcy', 'xaxcxabc', false));

  assertEquivalent([newDiff(DIFF_DELETE, 'ABCD'), newDiff(DIFF_EQUAL, 'a'), newDiff(DIFF_DELETE, '='), newDiff(DIFF_INSERT, '-'), newDiff(DIFF_EQUAL, 'bcd'), newDiff(DIFF_DELETE, '='), newDiff(DIFF_INSERT, '-'), newDiff(DIFF_EQUAL, 'efghijklmnopqrs'), newDiff(DIFF_DELETE, 'EFGHIJKLMNOefg')], dmp.diff_main('ABCDa=bcd=efghijklmnopqrsEFGHIJKLMNOefg', 'a-bcd-efghijklmnopqrs', false));

  // Large equality.
  assertEquivalent([newDiff(DIFF_INSERT, ' '), newDiff(DIFF_EQUAL, 'a'), newDiff(DIFF_INSERT, 'nd'), newDiff(DIFF_EQUAL, ' [[Pennsylvania]]'), newDiff(DIFF_DELETE, ' and [[New')], dmp.diff_main('a [[Pennsylvania]] and [[New', ' and [[Pennsylvania]]', false));

  // Timeout.
  dmp.Diff_Timeout = 0.1;  // 100ms
  var a = '`Twas brillig, and the slithy toves\nDid gyre and gimble in the wabe:\nAll mimsy were the borogoves,\nAnd the mome raths outgrabe.\n';
  var b = 'I am the very model of a modern major general,\nI\'ve information vegetable, animal, and mineral,\nI know the kings of England, and I quote the fights historical,\nFrom Marathon to Waterloo, in order categorical.\n';
  // Increase the text lengths by 1024 times to ensure a timeout.
  for (var i = 0; i < 10; i++) {
    a += a;
    b += b;
  }
  var startTime = (new Date()).getTime();
  dmp.diff_main(a, b);
  var endTime = (new Date()).getTime();
  // Test that we took at least the timeout period.
  assertTrue(dmp.Diff_Timeout * 1000 <= endTime - startTime);
  // Test that we didn't take forever (be forgiving).
  // Theoretically this test could fail very occasionally if the
  // OS task swaps or locks up for a second at the wrong moment.
  assertTrue(dmp.Diff_Timeout * 1000 * 2 > endTime - startTime);
  dmp.Diff_Timeout = 0;

  // Test the linemode speedup.
  // Must be long to pass the 100 char cutoff.
  // Simple line-mode.
  a = '1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n';
  b = 'abcdefghij\nabcdefghij\nabcdefghij\nabcdefghij\nabcdefghij\nabcdefghij\nabcdefghij\nabcdefghij\nabcdefghij\nabcdefghij\nabcdefghij\nabcdefghij\nabcdefghij\n';
  assertEquivalent(dmp.diff_main(a, b, false), dmp.diff_main(a, b, true));

  // Single line-mode.
  a = '1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890';
  b = 'abcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghij';
  assertEquivalent(dmp.diff_main(a, b, false), dmp.diff_main(a, b, true));

  // Overlap line-mode.
  a = '1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n';
  b = 'abcdefghij\n1234567890\n1234567890\n1234567890\nabcdefghij\n1234567890\n1234567890\n1234567890\nabcdefghij\n1234567890\n1234567890\n1234567890\nabcdefghij\n';
  var texts_linemode = diff_rebuildtexts(dmp.diff_main(a, b, true));
  var texts_textmode = diff_rebuildtexts(dmp.diff_main(a, b, false));
  assertEquivalent(texts_textmode, texts_linemode);

  // Test null inputs.
  try {
    dmp.diff_main(null, null);
    assertEquals(Error, null);
  } catch (e) {
    // Exception expected.
  }
}


// MATCH TEST FUNCTIONS


function testMatchAlphabet() {
  // Initialise the bitmasks for Bitap.
  // Unique.
  assertEquivalent({'a':4, 'b':2, 'c':1}, dmp.match_alphabet_('abc'));

  // Duplicates.
  assertEquivalent({'a':37, 'b':18, 'c':8}, dmp.match_alphabet_('abcaba'));
}

function testMatchBitap() {
  // Bitap algorithm.
  dmp.Match_Distance = 100;
  dmp.Match_Threshold = 0.5;
  // Exact matches.
  assertEquals(5, dmp.match_bitap_('abcdefghijk', 'fgh', 5));

  assertEquals(5, dmp.match_bitap_('abcdefghijk', 'fgh', 0));

  // Fuzzy matches.
  assertEquals(4, dmp.match_bitap_('abcdefghijk', 'efxhi', 0));

  assertEquals(2, dmp.match_bitap_('abcdefghijk', 'cdefxyhijk', 5));

  assertEquals(-1, dmp.match_bitap_('abcdefghijk', 'bxy', 1));

  // Overflow.
  assertEquals(2, dmp.match_bitap_('123456789xx0', '3456789x0', 2));

  // Threshold test.
  dmp.Match_Threshold = 0.4;
  assertEquals(4, dmp.match_bitap_('abcdefghijk', 'efxyhi', 1));

  dmp.Match_Threshold = 0.3;
  assertEquals(-1, dmp.match_bitap_('abcdefghijk', 'efxyhi', 1));

  dmp.Match_Threshold = 0.0;
  assertEquals(1, dmp.match_bitap_('abcdefghijk', 'bcdef', 1));
  dmp.Match_Threshold = 0.5;

  // Multiple select.
  assertEquals(0, dmp.match_bitap_('abcdexyzabcde', 'abccde', 3));

  assertEquals(8, dmp.match_bitap_('abcdexyzabcde', 'abccde', 5));

  // Distance test.
  dmp.Match_Distance = 10;  // Strict location.
  assertEquals(-1, dmp.match_bitap_('abcdefghijklmnopqrstuvwxyz', 'abcdefg', 24));

  assertEquals(0, dmp.match_bitap_('abcdefghijklmnopqrstuvwxyz', 'abcdxxefg', 1));

  dmp.Match_Distance = 1000;  // Loose location.
  assertEquals(0, dmp.match_bitap_('abcdefghijklmnopqrstuvwxyz', 'abcdefg', 24));
}

function testMatchMain() {
  // Full match.
  // Shortcut matches.
  assertEquals(0, dmp.match_main('abcdef', 'abcdef', 1000));

  assertEquals(-1, dmp.match_main('', 'abcdef', 1));

  assertEquals(3, dmp.match_main('abcdef', '', 3));

  assertEquals(3, dmp.match_main('abcdef', 'de', 3));

  // Beyond end match.
  assertEquals(3, dmp.match_main("abcdef", "defy", 4));

  // Oversized pattern.
  assertEquals(0, dmp.match_main("abcdef", "abcdefy", 0));

  // Complex match.
  assertEquals(4, dmp.match_main('I am the very model of a modern major general.', ' that berry ', 5));

  // Test null inputs.
  try {
    dmp.match_main(null, null, 0);
    assertEquals(Error, null);
  } catch (e) {
    // Exception expected.
  }
}


// PATCH TEST FUNCTIONS


function testPatchObj() {
  // Patch Object.
  var p = new diff_match_patch.patch_obj();
  p.start1 = 20;
  p.start2 = 21;
  p.length1 = 18;
  p.length2 = 17;
  p.diffs = [newDiff(DIFF_EQUAL, 'jump'), newDiff(DIFF_DELETE, 's'), newDiff(DIFF_INSERT, 'ed'), newDiff(DIFF_EQUAL, ' over '), newDiff(DIFF_DELETE, 'the'), newDiff(DIFF_INSERT, 'a'), newDiff(DIFF_EQUAL, '\nlaz')];
  var strp = p.toString();
  assertEquals('@@ -21,18 +22,17 @@\n jump\n-s\n+ed\n  over \n-the\n+a\n %0Alaz\n', strp);
}

function testPatchFromText() {
  assertEquivalent([], dmp.patch_fromText(strp));

  var strp = '@@ -21,18 +22,17 @@\n jump\n-s\n+ed\n  over \n-the\n+a\n %0Alaz\n';
  assertEquals(strp, dmp.patch_fromText(strp)[0].toString());

  assertEquals('@@ -1 +1 @@\n-a\n+b\n', dmp.patch_fromText('@@ -1 +1 @@\n-a\n+b\n')[0].toString());

  assertEquals('@@ -1,3 +0,0 @@\n-abc\n', dmp.patch_fromText('@@ -1,3 +0,0 @@\n-abc\n')[0].toString());

  assertEquals('@@ -0,0 +1,3 @@\n+abc\n', dmp.patch_fromText('@@ -0,0 +1,3 @@\n+abc\n')[0].toString());

  // Generates error.
  try {
    dmp.patch_fromText('Bad\nPatch\n');
    assertEquals(Error, null);
  } catch (e) {
    // Exception expected.
  }
}

function testPatchToText() {
  var strp = '@@ -21,18 +22,17 @@\n jump\n-s\n+ed\n  over \n-the\n+a\n  laz\n';
  var p = dmp.patch_fromText(strp);
  assertEquals(strp, dmp.patch_toText(p));

  strp = '@@ -1,9 +1,9 @@\n-f\n+F\n oo+fooba\n@@ -7,9 +7,9 @@\n obar\n-,\n+.\n  tes\n';
  p = dmp.patch_fromText(strp);
  assertEquals(strp, dmp.patch_toText(p));
}

function testPatchAddContext() {
  dmp.Patch_Margin = 4;
  var p = dmp.patch_fromText('@@ -21,4 +21,10 @@\n-jump\n+somersault\n')[0];
  dmp.patch_addContext_(p, 'The quick brown fox jumps over the lazy dog.');
  assertEquals('@@ -17,12 +17,18 @@\n fox \n-jump\n+somersault\n s ov\n', p.toString());

  // Same, but not enough trailing context.
  p = dmp.patch_fromText('@@ -21,4 +21,10 @@\n-jump\n+somersault\n')[0];
  dmp.patch_addContext_(p, 'The quick brown fox jumps.');
  assertEquals('@@ -17,10 +17,16 @@\n fox \n-jump\n+somersault\n s.\n', p.toString());

  // Same, but not enough leading context.
  p = dmp.patch_fromText('@@ -3 +3,2 @@\n-e\n+at\n')[0];
  dmp.patch_addContext_(p, 'The quick brown fox jumps.');
  assertEquals('@@ -1,7 +1,8 @@\n Th\n-e\n+at\n  qui\n', p.toString());

  // Same, but with ambiguity.
  p = dmp.patch_fromText('@@ -3 +3,2 @@\n-e\n+at\n')[0];
  dmp.patch_addContext_(p, 'The quick brown fox jumps.  The quick brown fox crashes.');
  assertEquals('@@ -1,27 +1,28 @@\n Th\n-e\n+at\n  quick brown fox jumps. \n', p.toString());
}

function testPatchMake() {
  // Null case.
  var patches = dmp.patch_make('', '');
  assertEquals('', dmp.patch_toText(patches));

  var text1 = 'The quick brown fox jumps over the lazy dog.';
  var text2 = 'That quick brown fox jumped over a lazy dog.';
  // Text2+Text1 inputs.
  var expectedPatch = '@@ -1,8 +1,7 @@\n Th\n-at\n+e\n  qui\n@@ -21,17 +21,18 @@\n jump\n-ed\n+s\n  over \n-a\n+the\n  laz\n';
  // The second patch must be "-21,17 +21,18", not "-22,17 +21,18" due to rolling context.
  patches = dmp.patch_make(text2, text1);
  assertEquals(expectedPatch, dmp.patch_toText(patches));

  // Text1+Text2 inputs.
  expectedPatch = '@@ -1,11 +1,12 @@\n Th\n-e\n+at\n  quick b\n@@ -22,18 +22,17 @@\n jump\n-s\n+ed\n  over \n-the\n+a\n  laz\n';
  patches = dmp.patch_make(text1, text2);
  assertEquals(expectedPatch, dmp.patch_toText(patches));

  // Diff input.
  var diffs = dmp.diff_main(text1, text2, false);
  patches = dmp.patch_make(diffs);
  assertEquals(expectedPatch, dmp.patch_toText(patches));

  // Text1+Diff inputs.
  patches = dmp.patch_make(text1, diffs);
  assertEquals(expectedPatch, dmp.patch_toText(patches));

  // Text1+Text2+Diff inputs (deprecated).
  patches = dmp.patch_make(text1, text2, diffs);
  assertEquals(expectedPatch, dmp.patch_toText(patches));

  // Character encoding.
  patches = dmp.patch_make('`1234567890-=[]\\;\',./', '~!@#$%^&*()_+{}|:"<>?');
  assertEquals('@@ -1,21 +1,21 @@\n-%601234567890-=%5B%5D%5C;\',./\n+~!@#$%25%5E&*()_+%7B%7D%7C:%22%3C%3E?\n', dmp.patch_toText(patches));

  // Character decoding.
  diffs = [newDiff(DIFF_DELETE, '`1234567890-=[]\\;\',./'), newDiff(DIFF_INSERT, '~!@#$%^&*()_+{}|:"<>?')];
  assertEquivalent(diffs, dmp.patch_fromText('@@ -1,21 +1,21 @@\n-%601234567890-=%5B%5D%5C;\',./\n+~!@#$%25%5E&*()_+%7B%7D%7C:%22%3C%3E?\n')[0].diffs);

  // Long string with repeats.
  text1 = '';
  for (var x = 0; x < 100; x++) {
    text1 += 'abcdef';
  }
  text2 = text1 + '123';
  expectedPatch = '@@ -573,28 +573,31 @@\n cdefabcdefabcdefabcdefabcdef\n+123\n';
  patches = dmp.patch_make(text1, text2);
  assertEquals(expectedPatch, dmp.patch_toText(patches));

  // Test null inputs.
  try {
    dmp.patch_make(null);
    assertEquals(Error, null);
  } catch (e) {
    // Exception expected.
  }
}

function testPatchSplitMax() {
  // Assumes that dmp.Match_MaxBits is 32.
  var patches = dmp.patch_make('abcdefghijklmnopqrstuvwxyz01234567890', 'XabXcdXefXghXijXklXmnXopXqrXstXuvXwxXyzX01X23X45X67X89X0');
  dmp.patch_splitMax(patches);
  assertEquals('@@ -1,32 +1,46 @@\n+X\n ab\n+X\n cd\n+X\n ef\n+X\n gh\n+X\n ij\n+X\n kl\n+X\n mn\n+X\n op\n+X\n qr\n+X\n st\n+X\n uv\n+X\n wx\n+X\n yz\n+X\n 012345\n@@ -25,13 +39,18 @@\n zX01\n+X\n 23\n+X\n 45\n+X\n 67\n+X\n 89\n+X\n 0\n', dmp.patch_toText(patches));

  patches = dmp.patch_make('abcdef1234567890123456789012345678901234567890123456789012345678901234567890uvwxyz', 'abcdefuvwxyz');
  var oldToText = dmp.patch_toText(patches);
  dmp.patch_splitMax(patches);
  assertEquals(oldToText, dmp.patch_toText(patches));

  patches = dmp.patch_make('1234567890123456789012345678901234567890123456789012345678901234567890', 'abc');
  dmp.patch_splitMax(patches);
  assertEquals('@@ -1,32 +1,4 @@\n-1234567890123456789012345678\n 9012\n@@ -29,32 +1,4 @@\n-9012345678901234567890123456\n 7890\n@@ -57,14 +1,3 @@\n-78901234567890\n+abc\n', dmp.patch_toText(patches));

  patches = dmp.patch_make('abcdefghij , h : 0 , t : 1 abcdefghij , h : 0 , t : 1 abcdefghij , h : 0 , t : 1', 'abcdefghij , h : 1 , t : 1 abcdefghij , h : 1 , t : 1 abcdefghij , h : 0 , t : 1');
  dmp.patch_splitMax(patches);
  assertEquals('@@ -2,32 +2,32 @@\n bcdefghij , h : \n-0\n+1\n  , t : 1 abcdef\n@@ -29,32 +29,32 @@\n bcdefghij , h : \n-0\n+1\n  , t : 1 abcdef\n', dmp.patch_toText(patches));
}

function testPatchAddPadding() {
  // Both edges full.
  var patches = dmp.patch_make('', 'test');
  assertEquals('@@ -0,0 +1,4 @@\n+test\n', dmp.patch_toText(patches));
  dmp.patch_addPadding(patches);
  assertEquals('@@ -1,8 +1,12 @@\n %01%02%03%04\n+test\n %01%02%03%04\n', dmp.patch_toText(patches));

  // Both edges partial.
  patches = dmp.patch_make('XY', 'XtestY');
  assertEquals('@@ -1,2 +1,6 @@\n X\n+test\n Y\n', dmp.patch_toText(patches));
  dmp.patch_addPadding(patches);
  assertEquals('@@ -2,8 +2,12 @@\n %02%03%04X\n+test\n Y%01%02%03\n', dmp.patch_toText(patches));

  // Both edges none.
  patches = dmp.patch_make('XXXXYYYY', 'XXXXtestYYYY');
  assertEquals('@@ -1,8 +1,12 @@\n XXXX\n+test\n YYYY\n', dmp.patch_toText(patches));
  dmp.patch_addPadding(patches);
  assertEquals('@@ -5,8 +5,12 @@\n XXXX\n+test\n YYYY\n', dmp.patch_toText(patches));
}

function testPatchApply() {
  dmp.Match_Distance = 1000;
  dmp.Match_Threshold = 0.5;
  dmp.Patch_DeleteThreshold = 0.5;
  // Null case.
  var patches = dmp.patch_make('', '');
  var results = dmp.patch_apply(patches, 'Hello world.');
  assertEquivalent(['Hello world.', []], results);

  // Exact match.
  patches = dmp.patch_make('The quick brown fox jumps over the lazy dog.', 'That quick brown fox jumped over a lazy dog.');
  results = dmp.patch_apply(patches, 'The quick brown fox jumps over the lazy dog.');
  assertEquivalent(['That quick brown fox jumped over a lazy dog.', [true, true]], results);

  // Partial match.
  results = dmp.patch_apply(patches, 'The quick red rabbit jumps over the tired tiger.');
  assertEquivalent(['That quick red rabbit jumped over a tired tiger.', [true, true]], results);

  // Failed match.
  results = dmp.patch_apply(patches, 'I am the very model of a modern major general.');
  assertEquivalent(['I am the very model of a modern major general.', [false, false]], results);

  // Big delete, small change.
  patches = dmp.patch_make('x1234567890123456789012345678901234567890123456789012345678901234567890y', 'xabcy');
  results = dmp.patch_apply(patches, 'x123456789012345678901234567890-----++++++++++-----123456789012345678901234567890y');
  assertEquivalent(['xabcy', [true, true]], results);

  // Big delete, big change 1.
  patches = dmp.patch_make('x1234567890123456789012345678901234567890123456789012345678901234567890y', 'xabcy');
  results = dmp.patch_apply(patches, 'x12345678901234567890---------------++++++++++---------------12345678901234567890y');
  assertEquivalent(['xabc12345678901234567890---------------++++++++++---------------12345678901234567890y', [false, true]], results);

  // Big delete, big change 2.
  dmp.Patch_DeleteThreshold = 0.6;
  patches = dmp.patch_make('x1234567890123456789012345678901234567890123456789012345678901234567890y', 'xabcy');
  results = dmp.patch_apply(patches, 'x12345678901234567890---------------++++++++++---------------12345678901234567890y');
  assertEquivalent(['xabcy', [true, true]], results);
  dmp.Patch_DeleteThreshold = 0.5;

  // Compensate for failed patch.
  dmp.Match_Threshold = 0.0;
  dmp.Match_Distance = 0;
  patches = dmp.patch_make('abcdefghijklmnopqrstuvwxyz--------------------1234567890', 'abcXXXXXXXXXXdefghijklmnopqrstuvwxyz--------------------1234567YYYYYYYYYY890');
  results = dmp.patch_apply(patches, 'ABCDEFGHIJKLMNOPQRSTUVWXYZ--------------------1234567890');
  assertEquivalent(['ABCDEFGHIJKLMNOPQRSTUVWXYZ--------------------1234567YYYYYYYYYY890', [false, true]], results);
  dmp.Match_Threshold = 0.5;
  dmp.Match_Distance = 1000;

  // No side effects.
  patches = dmp.patch_make('', 'test');
  var patchstr = dmp.patch_toText(patches);
  dmp.patch_apply(patches, '');
  assertEquals(patchstr, dmp.patch_toText(patches));

  // No side effects with major delete.
  patches = dmp.patch_make('The quick brown fox jumps over the lazy dog.', 'Woof');
  patchstr = dmp.patch_toText(patches);
  dmp.patch_apply(patches, 'The quick brown fox jumps over the lazy dog.');
  assertEquals(patchstr, dmp.patch_toText(patches));

  // Edge exact match.
  patches = dmp.patch_make('', 'test');
  results = dmp.patch_apply(patches, '');
  assertEquivalent(['test', [true]], results);

  // Near edge exact match.
  patches = dmp.patch_make('XY', 'XtestY');
  results = dmp.patch_apply(patches, 'XY');
  assertEquivalent(['XtestY', [true]], results);

  // Edge partial match.
  patches = dmp.patch_make('y', 'y123');
  results = dmp.patch_apply(patches, 'x');
  assertEquivalent(['x123', [true]], results);
}
