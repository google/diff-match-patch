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

import * as diff_match_patch_test from './diff_match_patch_test';

// Counters for unit test results.
var test_good = 0;
var test_bad = 0;

// If expected and actual are the identical, print 'Ok', otherwise 'Fail!'
export function assertEquals(msg: any, expected: any, actual?: any) {
    if (typeof actual == 'undefined') {
        // msg is optional.
        actual = expected;
        expected = msg;
        msg = 'Expected: \'' + expected + '\' Actual: \'' + actual + '\'';
    }
    if (expected === actual) {
        console.log("Ok");
        test_good++;
        return true;
    } else {
        console.error("Fail!");
        msg = msg.replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;');
        console.error(msg);
        test_bad++;
        return false;
    }
}

export function assertTrue(msg: any, actual?: any) {
    if (typeof actual == 'undefined') {
        // msg is optional.
        actual = msg;
        return assertEquals(true, actual);
    } else {
        return assertEquals(msg, true, actual);
    }
}

export function assertFalse(msg: any, actual?: any) {
    if (typeof actual == 'undefined') {
        // msg is optional.
        actual = msg;
        return assertEquals(false, actual);
    } else {
        return assertEquals(msg, false, actual);
    }
}

function runTests() {

    for (var x = 0; x < tests.length; x++) {
        console.log(tests[x]);
        eval((diff_match_patch_test as any)[tests[x]]());
    }
}

var tests = [
    'testDiffCommonPrefix',
    'testDiffCommonSuffix',
    'testDiffCommonOverlap',
    'testDiffHalfMatch',
    'testDiffLinesToChars',
    'testDiffCharsToLines',
    'testDiffCleanupMerge',
    'testDiffCleanupSemanticLossless',
    'testDiffCleanupSemantic',
    'testDiffCleanupEfficiency',
    'testDiffPrettyHtml',
    'testDiffText',
    'testDiffDelta',
    'testDiffXIndex',
    'testDiffLevenshtein',
    'testDiffBisect',
    'testDiffMain',

    'testMatchAlphabet',
    'testMatchBitap',
    'testMatchMain',

    'testPatchObj',
    'testPatchFromText',
    'testPatchToText',
    'testPatchAddContext',
    'testPatchMake',
    'testPatchSplitMax',
    'testPatchAddPadding',
    'testPatchApply'];


var startTime = (new Date()).getTime();
runTests();
var endTime = (new Date()).getTime();
console.log("Done");
console.log('Tests passed: ' + test_good + '\nTests failed: ' + test_bad);
console.log('Total time: ' + (endTime - startTime) + ' ms');

