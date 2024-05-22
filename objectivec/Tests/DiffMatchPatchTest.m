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
 *
 * Author: fraser@google.com (Neil Fraser)
 * ObjC port: jan@geheimwerk.de (Jan Weiß)
 */

#import "DiffMatchPatchTest.h"

#import "DiffMatchPatch.h"
#import "NSMutableDictionary+DMPExtensions.h"

#define stringForBOOL(A)  ([((NSNumber *)A) boolValue] ? @"true" : @"false")

@interface DiffMatchPatchTest (PrivatMethods)
- (NSArray *)diff_rebuildtexts:(NSMutableArray *)diffs;
@end

@implementation DiffMatchPatchTest

- (void)test_diff_commonPrefixTest {
  DiffMatchPatch *dmp = [DiffMatchPatch new];

  // Detect any common suffix.
  // Null case.
  XCTAssertEqual((NSUInteger)0, [dmp diff_commonPrefixOfFirstString:@"abc" andSecondString:@"xyz"], @"Common suffix null case failed.");

  // Non-null case.
  XCTAssertEqual((NSUInteger)4, [dmp diff_commonPrefixOfFirstString:@"1234abcdef" andSecondString:@"1234xyz"], @"Common suffix non-null case failed.");

  // Whole case.
  XCTAssertEqual((NSUInteger)4, [dmp diff_commonPrefixOfFirstString:@"1234" andSecondString:@"1234xyz"], @"Common suffix whole case failed.");

  [dmp release];
}

- (void)test_diff_commonSuffixTest {
  DiffMatchPatch *dmp = [DiffMatchPatch new];

  // Detect any common suffix.
  // Null case.
  XCTAssertEqual((NSUInteger)0, [dmp diff_commonSuffixOfFirstString:@"abc" andSecondString:@"xyz"], @"Detect any common suffix. Null case.");

  // Non-null case.
  XCTAssertEqual((NSUInteger)4, [dmp diff_commonSuffixOfFirstString:@"abcdef1234" andSecondString:@"xyz1234"], @"Detect any common suffix. Non-null case.");

  // Whole case.
  XCTAssertEqual((NSUInteger)4, [dmp diff_commonSuffixOfFirstString:@"1234" andSecondString:@"xyz1234"], @"Detect any common suffix. Whole case.");

  [dmp release];
}

- (void)test_diff_commonOverlapTest {
  DiffMatchPatch *dmp = [DiffMatchPatch new];

  // Detect any suffix/prefix overlap.
  // Null case.
  XCTAssertEqual((NSUInteger)0, [dmp diff_commonOverlapOfFirstString:@"" andSecondString:@"abcd"], @"Detect any suffix/prefix overlap. Null case.");

  // Whole case.
  XCTAssertEqual((NSUInteger)3, [dmp diff_commonOverlapOfFirstString:@"abc" andSecondString:@"abcd"], @"Detect any suffix/prefix overlap. Whole case.");

  // No overlap.
  XCTAssertEqual((NSUInteger)0, [dmp diff_commonOverlapOfFirstString:@"123456" andSecondString:@"abcd"], @"Detect any suffix/prefix overlap. No overlap.");

  // Overlap.
  XCTAssertEqual((NSUInteger)3, [dmp diff_commonOverlapOfFirstString:@"123456xxx" andSecondString:@"xxxabcd"], @"Detect any suffix/prefix overlap. Overlap.");

  // Unicode.
  // Some overly clever languages (C#) may treat ligatures as equal to their
  // component letters.  E.g. U+FB01 == 'fi'
  XCTAssertEqual((NSUInteger)0, [dmp diff_commonOverlapOfFirstString:@"fi" andSecondString:@"\U0000fb01i"], @"Detect any suffix/prefix overlap. Unicode.");

  [dmp release];
}

- (void)test_diff_halfmatchTest {
  DiffMatchPatch *dmp = [DiffMatchPatch new];
  dmp.Diff_Timeout = 1;
  NSArray *expectedResult = nil;

  // No match.
  XCTAssertNil([dmp diff_halfMatchOfFirstString:@"1234567890" andSecondString:@"abcdef"], @"No match #1.");

  XCTAssertNil([dmp diff_halfMatchOfFirstString:@"12345" andSecondString:@"23"], @"No match #2.");

  // Single Match.
  expectedResult = [NSArray arrayWithObjects:@"12", @"90", @"a", @"z", @"345678", nil];
  XCTAssertEqualObjects(expectedResult, [dmp diff_halfMatchOfFirstString:@"1234567890" andSecondString:@"a345678z"], @"Single Match #1.");

  expectedResult = [NSArray arrayWithObjects:@"a", @"z", @"12", @"90", @"345678", nil];
  XCTAssertEqualObjects(expectedResult, [dmp diff_halfMatchOfFirstString:@"a345678z" andSecondString:@"1234567890"], @"Single Match #2.");

  expectedResult = [NSArray arrayWithObjects:@"abc", @"z", @"1234", @"0", @"56789", nil];
  XCTAssertEqualObjects(expectedResult, [dmp diff_halfMatchOfFirstString:@"abc56789z" andSecondString:@"1234567890"], @"Single Match #3.");

  expectedResult = [NSArray arrayWithObjects:@"a", @"xyz", @"1", @"7890", @"23456", nil];
  XCTAssertEqualObjects(expectedResult, [dmp diff_halfMatchOfFirstString:@"a23456xyz" andSecondString:@"1234567890"], @"Single Match #4.");

  // Multiple Matches.
  expectedResult = [NSArray arrayWithObjects:@"12123", @"123121", @"a", @"z", @"1234123451234", nil];
  XCTAssertEqualObjects(expectedResult, [dmp diff_halfMatchOfFirstString:@"121231234123451234123121" andSecondString:@"a1234123451234z"], @"Multiple Matches #1.");

  expectedResult = [NSArray arrayWithObjects:@"", @"-=-=-=-=-=", @"x", @"", @"x-=-=-=-=-=-=-=", nil];
  XCTAssertEqualObjects(expectedResult, [dmp diff_halfMatchOfFirstString:@"x-=-=-=-=-=-=-=-=-=-=-=-=" andSecondString:@"xx-=-=-=-=-=-=-="], @"Multiple Matches #2.");

  expectedResult = [NSArray arrayWithObjects:@"-=-=-=-=-=", @"", @"", @"y", @"-=-=-=-=-=-=-=y", nil];
  XCTAssertEqualObjects(expectedResult, [dmp diff_halfMatchOfFirstString:@"-=-=-=-=-=-=-=-=-=-=-=-=y" andSecondString:@"-=-=-=-=-=-=-=yy"], @"Multiple Matches #3.");

  // Non-optimal halfmatch.
  // Optimal diff would be -q+x=H-i+e=lloHe+Hu=llo-Hew+y not -qHillo+x=HelloHe-w+Hulloy
  expectedResult = [NSArray arrayWithObjects:@"qHillo", @"w", @"x", @"Hulloy", @"HelloHe", nil];
  XCTAssertEqualObjects(expectedResult, [dmp diff_halfMatchOfFirstString:@"qHilloHelloHew" andSecondString:@"xHelloHeHulloy"], @"Non-optimal halfmatch.");

  // Optimal no halfmatch.
  dmp.Diff_Timeout = 0;
  XCTAssertNil([dmp diff_halfMatchOfFirstString:@"qHilloHelloHew" andSecondString:@"xHelloHeHulloy"], @"Optimal no halfmatch.");

  [dmp release];
}

- (void)test_diff_linesToCharsTest {
  DiffMatchPatch *dmp = [DiffMatchPatch new];
  NSArray *result;

  // Convert lines down to characters.
  NSMutableArray *tmpVector = [NSMutableArray array];  // Array of NSString objects.
  [tmpVector addObject:@""];
  [tmpVector addObject:@"alpha\n"];
  [tmpVector addObject:@"beta\n"];
  result = [dmp diff_linesToCharsForFirstString:@"alpha\nbeta\nalpha\n" andSecondString:@"beta\nalpha\nbeta\n"];
  XCTAssertEqualObjects(@"\001\002\001", [result objectAtIndex:0], @"Shared lines #1.");
  XCTAssertEqualObjects(@"\002\001\002", [result objectAtIndex:1], @"Shared lines #2.");
  XCTAssertEqualObjects(tmpVector, (NSArray *)[result objectAtIndex:2], @"Shared lines #3.");

  [tmpVector removeAllObjects];
  [tmpVector addObject:@""];
  [tmpVector addObject:@"alpha\r\n"];
  [tmpVector addObject:@"beta\r\n"];
  [tmpVector addObject:@"\r\n"];
  result = [dmp diff_linesToCharsForFirstString:@"" andSecondString:@"alpha\r\nbeta\r\n\r\n\r\n"];
  XCTAssertEqualObjects(@"", [result objectAtIndex:0], @"Empty string and blank lines #1.");
  XCTAssertEqualObjects(@"\001\002\003\003", [result objectAtIndex:1], @"Empty string and blank lines #2.");
  XCTAssertEqualObjects(tmpVector, (NSArray *)[result objectAtIndex:2], @"Empty string and blank lines #3.");

  [tmpVector removeAllObjects];
  [tmpVector addObject:@""];
  [tmpVector addObject:@"a"];
  [tmpVector addObject:@"b"];
  result = [dmp diff_linesToCharsForFirstString:@"a" andSecondString:@"b"];
  XCTAssertEqualObjects(@"\001", [result objectAtIndex:0], @"No linebreaks #1.");
  XCTAssertEqualObjects(@"\002", [result objectAtIndex:1], @"No linebreaks #2.");
  XCTAssertEqualObjects(tmpVector, (NSArray *)[result objectAtIndex:2], @"No linebreaks #3.");

  // More than 256 to reveal any 8-bit limitations.
  unichar n = 300;
  [tmpVector removeAllObjects];
  NSMutableString *lines = [NSMutableString string];
  NSMutableString *chars = [NSMutableString string];
  NSString *currentLine;
  for (unichar i = 1; i < n + 1; i++) {
    currentLine = [NSString stringWithFormat:@"%d\n", (int)i];
    [tmpVector addObject:currentLine];
    [lines appendString:currentLine];
    [chars appendString:[NSString stringWithFormat:@"%C", i]];
  }
  XCTAssertEqual((NSUInteger)n, tmpVector.count, @"More than 256 #1.");
  XCTAssertEqual((NSUInteger)n, chars.length, @"More than 256 #2.");
  [tmpVector insertObject:@"" atIndex:0];
  result = [dmp diff_linesToCharsForFirstString:lines andSecondString:@""];
  XCTAssertEqualObjects(chars, [result objectAtIndex:0], @"More than 256 #3.");
  XCTAssertEqualObjects(@"", [result objectAtIndex:1], @"More than 256 #4.");
  XCTAssertEqualObjects(tmpVector, (NSArray *)[result objectAtIndex:2], @"More than 256 #5.");

  [dmp release];
}

- (void)test_diff_charsToLinesTest {
  DiffMatchPatch *dmp = [DiffMatchPatch new];

  // Convert chars up to lines.
  NSArray *diffs = [NSArray arrayWithObjects:
      [Diff diffWithOperation:DIFF_EQUAL andText:@"\001\002\001"],
      [Diff diffWithOperation:DIFF_INSERT andText:@"\002\001\002"], nil];
  NSMutableArray *tmpVector = [NSMutableArray array]; // Array of NSString objects.
  [tmpVector addObject:@""];
  [tmpVector addObject:@"alpha\n"];
  [tmpVector addObject:@"beta\n"];
  [dmp diff_chars:diffs toLines:tmpVector];
  NSArray *expectedResult = [NSArray arrayWithObjects:
      [Diff diffWithOperation:DIFF_EQUAL andText:@"alpha\nbeta\nalpha\n"],
      [Diff diffWithOperation:DIFF_INSERT andText:@"beta\nalpha\nbeta\n"], nil];
  XCTAssertEqualObjects(expectedResult, diffs, @"Shared lines.");

  // More than 256 to reveal any 8-bit limitations.
  unichar n = 300;
  [tmpVector removeAllObjects];
  NSMutableString *lines = [NSMutableString string];
  NSMutableString *chars = [NSMutableString string];
  NSString *currentLine;
  for (unichar i = 1; i < n + 1; i++) {
    currentLine = [NSString stringWithFormat:@"%d\n", (int)i];
    [tmpVector addObject:currentLine];
    [lines appendString:currentLine];
    [chars appendString:[NSString stringWithFormat:@"%C", i]];
  }
  XCTAssertEqual((NSUInteger)n, tmpVector.count, @"More than 256 #1.");
  XCTAssertEqual((NSUInteger)n, chars.length, @"More than 256 #2.");
  [tmpVector insertObject:@"" atIndex:0];
  diffs = [NSArray arrayWithObject:[Diff diffWithOperation:DIFF_DELETE andText:chars]];
  [dmp diff_chars:diffs toLines:tmpVector];
  XCTAssertEqualObjects([NSArray arrayWithObject:[Diff diffWithOperation:DIFF_DELETE andText:lines]], diffs, @"More than 256 #3.");

  // More than 65536 to verify any 16-bit limitation.
  lines = [NSMutableString string];
  for (int i = 1; i < 66000; i++) {
    currentLine = [NSString stringWithFormat:@"%d\n", i];
    [lines appendString:currentLine];
  }
  NSArray *result;
  result = [dmp diff_linesToCharsForFirstString:lines andSecondString:@""];
  diffs = [NSArray arrayWithObject:[Diff diffWithOperation:DIFF_INSERT andText:result[0]]];
  [dmp diff_chars:diffs toLines:result[2]];
  Diff *myDiff = diffs.firstObject;
  XCTAssertEqualObjects(lines, myDiff.text, @"More than 65536.");

  [dmp release];
}

- (void)test_diff_cleanupMergeTest {
  DiffMatchPatch *dmp = [DiffMatchPatch new];
  NSMutableArray *expectedResult = nil;

  // Cleanup a messy diff.
  // Null case.
  NSMutableArray *diffs = [NSMutableArray array];
  [dmp diff_cleanupMerge:diffs];
  XCTAssertEqualObjects([NSMutableArray array], diffs, @"Null case.");

  // No change case.
  diffs = [NSMutableArray arrayWithObjects:[Diff diffWithOperation:DIFF_EQUAL andText:@"a"], [Diff diffWithOperation:DIFF_DELETE andText:@"b"], [Diff diffWithOperation:DIFF_INSERT andText:@"c"], nil];
  [dmp diff_cleanupMerge:diffs];
  expectedResult = [NSMutableArray arrayWithObjects:[Diff diffWithOperation:DIFF_EQUAL andText:@"a"], [Diff diffWithOperation:DIFF_DELETE andText:@"b"], [Diff diffWithOperation:DIFF_INSERT andText:@"c"], nil];
  XCTAssertEqualObjects(expectedResult, diffs, @"No change case.");

  // Merge equalities.
  diffs = [NSMutableArray arrayWithObjects:[Diff diffWithOperation:DIFF_EQUAL andText:@"a"], [Diff diffWithOperation:DIFF_EQUAL andText:@"b"], [Diff diffWithOperation:DIFF_EQUAL andText:@"c"], nil];
  [dmp diff_cleanupMerge:diffs];
  expectedResult = [NSMutableArray arrayWithObjects:[Diff diffWithOperation:DIFF_EQUAL andText:@"abc"], nil];
  XCTAssertEqualObjects(expectedResult, diffs, @"Merge equalities.");

  // Merge deletions.
  diffs = [NSMutableArray arrayWithObjects:[Diff diffWithOperation:DIFF_DELETE andText:@"a"], [Diff diffWithOperation:DIFF_DELETE andText:@"b"], [Diff diffWithOperation:DIFF_DELETE andText:@"c"], nil];
  [dmp diff_cleanupMerge:diffs];
  expectedResult = [NSMutableArray arrayWithObjects:[Diff diffWithOperation:DIFF_DELETE andText:@"abc"], nil];
  XCTAssertEqualObjects(expectedResult, diffs, @"Merge deletions.");

  // Merge insertions.
  diffs = [NSMutableArray arrayWithObjects:[Diff diffWithOperation:DIFF_INSERT andText:@"a"], [Diff diffWithOperation:DIFF_INSERT andText:@"b"], [Diff diffWithOperation:DIFF_INSERT andText:@"c"], nil];
  [dmp diff_cleanupMerge:diffs];
  expectedResult = [NSMutableArray arrayWithObjects:[Diff diffWithOperation:DIFF_INSERT andText:@"abc"], nil];
  XCTAssertEqualObjects(expectedResult, diffs, @"Merge insertions.");

  // Merge interweave.
  diffs = [NSMutableArray arrayWithObjects:[Diff diffWithOperation:DIFF_DELETE andText:@"a"], [Diff diffWithOperation:DIFF_INSERT andText:@"b"], [Diff diffWithOperation:DIFF_DELETE andText:@"c"], [Diff diffWithOperation:DIFF_INSERT andText:@"d"], [Diff diffWithOperation:DIFF_EQUAL andText:@"e"], [Diff diffWithOperation:DIFF_EQUAL andText:@"f"], nil];
  [dmp diff_cleanupMerge:diffs];
  expectedResult = [NSMutableArray arrayWithObjects:[Diff diffWithOperation:DIFF_DELETE andText:@"ac"], [Diff diffWithOperation:DIFF_INSERT andText:@"bd"], [Diff diffWithOperation:DIFF_EQUAL andText:@"ef"], nil];
  XCTAssertEqualObjects(expectedResult, diffs, @"Merge interweave.");

  // Prefix and suffix detection.
  diffs = [NSMutableArray arrayWithObjects:[Diff diffWithOperation:DIFF_DELETE andText:@"a"], [Diff diffWithOperation:DIFF_INSERT andText:@"abc"], [Diff diffWithOperation:DIFF_DELETE andText:@"dc"], nil];
  [dmp diff_cleanupMerge:diffs];
  expectedResult = [NSMutableArray arrayWithObjects:[Diff diffWithOperation:DIFF_EQUAL andText:@"a"], [Diff diffWithOperation:DIFF_DELETE andText:@"d"], [Diff diffWithOperation:DIFF_INSERT andText:@"b"], [Diff diffWithOperation:DIFF_EQUAL andText:@"c"], nil];
  XCTAssertEqualObjects(expectedResult, diffs, @"Prefix and suffix detection.");

  // Prefix and suffix detection with equalities.
  diffs = [NSMutableArray arrayWithObjects:[Diff diffWithOperation:DIFF_EQUAL andText:@"x"], [Diff diffWithOperation:DIFF_DELETE andText:@"a"], [Diff diffWithOperation:DIFF_INSERT andText:@"abc"], [Diff diffWithOperation:DIFF_DELETE andText:@"dc"], [Diff diffWithOperation:DIFF_EQUAL andText:@"y"], nil];
  [dmp diff_cleanupMerge:diffs];
  expectedResult = [NSMutableArray arrayWithObjects:[Diff diffWithOperation:DIFF_EQUAL andText:@"xa"], [Diff diffWithOperation:DIFF_DELETE andText:@"d"], [Diff diffWithOperation:DIFF_INSERT andText:@"b"], [Diff diffWithOperation:DIFF_EQUAL andText:@"cy"], nil];
  XCTAssertEqualObjects(expectedResult, diffs, @"Prefix and suffix detection with equalities.");

  // Slide edit left.
  diffs = [NSMutableArray arrayWithObjects:[Diff diffWithOperation:DIFF_EQUAL andText:@"a"], [Diff diffWithOperation:DIFF_INSERT andText:@"ba"], [Diff diffWithOperation:DIFF_EQUAL andText:@"c"], nil];
  [dmp diff_cleanupMerge:diffs];
  expectedResult = [NSMutableArray arrayWithObjects:[Diff diffWithOperation:DIFF_INSERT andText:@"ab"], [Diff diffWithOperation:DIFF_EQUAL andText:@"ac"], nil];
  XCTAssertEqualObjects(expectedResult, diffs, @"Slide edit left.");

  // Slide edit right.
  diffs = [NSMutableArray arrayWithObjects:[Diff diffWithOperation:DIFF_EQUAL andText:@"c"], [Diff diffWithOperation:DIFF_INSERT andText:@"ab"], [Diff diffWithOperation:DIFF_EQUAL andText:@"a"], nil];
  [dmp diff_cleanupMerge:diffs];
  expectedResult = [NSMutableArray arrayWithObjects:[Diff diffWithOperation:DIFF_EQUAL andText:@"ca"], [Diff diffWithOperation:DIFF_INSERT andText:@"ba"], nil];
  XCTAssertEqualObjects(expectedResult, diffs, @"Slide edit right.");

  // Slide edit left recursive.
  diffs = [NSMutableArray arrayWithObjects:[Diff diffWithOperation:DIFF_EQUAL andText:@"a"], [Diff diffWithOperation:DIFF_DELETE andText:@"b"], [Diff diffWithOperation:DIFF_EQUAL andText:@"c"], [Diff diffWithOperation:DIFF_DELETE andText:@"ac"], [Diff diffWithOperation:DIFF_EQUAL andText:@"x"], nil];
  [dmp diff_cleanupMerge:diffs];
  expectedResult = [NSMutableArray arrayWithObjects:[Diff diffWithOperation:DIFF_DELETE andText:@"abc"], [Diff diffWithOperation:DIFF_EQUAL andText:@"acx"], nil];
  XCTAssertEqualObjects(expectedResult, diffs, @"Slide edit left recursive.");

  // Slide edit right recursive.
  diffs = [NSMutableArray arrayWithObjects:[Diff diffWithOperation:DIFF_EQUAL andText:@"x"], [Diff diffWithOperation:DIFF_DELETE andText:@"ca"], [Diff diffWithOperation:DIFF_EQUAL andText:@"c"], [Diff diffWithOperation:DIFF_DELETE andText:@"b"], [Diff diffWithOperation:DIFF_EQUAL andText:@"a"], nil];
  [dmp diff_cleanupMerge:diffs];
  expectedResult = [NSMutableArray arrayWithObjects:[Diff diffWithOperation:DIFF_EQUAL andText:@"xca"], [Diff diffWithOperation:DIFF_DELETE andText:@"cba"], nil];
  XCTAssertEqualObjects(expectedResult, diffs, @"Slide edit right recursive.");

  // Empty merge.
  diffs = [NSMutableArray arrayWithObjects:[Diff diffWithOperation:DIFF_DELETE andText:@"b"], [Diff diffWithOperation:DIFF_INSERT andText:@"ab"], [Diff diffWithOperation:DIFF_EQUAL andText:@"c"], nil];
  [dmp diff_cleanupMerge:diffs];
  expectedResult = [NSMutableArray arrayWithObjects:[Diff diffWithOperation:DIFF_INSERT andText:@"a"], [Diff diffWithOperation:DIFF_EQUAL andText:@"bc"], nil];
  XCTAssertEqualObjects(expectedResult, diffs, @"Empty merge.");

  // Empty equality.
  diffs = [NSMutableArray arrayWithObjects:[Diff diffWithOperation:DIFF_EQUAL andText:@""], [Diff diffWithOperation:DIFF_INSERT andText:@"a"], [Diff diffWithOperation:DIFF_EQUAL andText:@"b"], nil];
  [dmp diff_cleanupMerge:diffs];
  expectedResult = [NSMutableArray arrayWithObjects:[Diff diffWithOperation:DIFF_INSERT andText:@"a"], [Diff diffWithOperation:DIFF_EQUAL andText:@"b"], nil];
  XCTAssertEqualObjects(expectedResult, diffs, @"Empty equality.");

  [dmp release];
}

- (void)test_diff_cleanupSemanticLosslessTest {
  DiffMatchPatch *dmp = [DiffMatchPatch new];
  NSMutableArray *expectedResult = nil;

  // Slide diffs to match logical boundaries.
  // Null case.
  NSMutableArray *diffs = [NSMutableArray array];
  [dmp diff_cleanupSemanticLossless:diffs];
  XCTAssertEqualObjects([NSMutableArray array], diffs, @"Null case.");

  // Blank lines.
  diffs = [NSMutableArray arrayWithObjects:
      [Diff diffWithOperation:DIFF_EQUAL andText:@"AAA\r\n\r\nBBB"],
      [Diff diffWithOperation:DIFF_INSERT andText:@"\r\nDDD\r\n\r\nBBB"],
      [Diff diffWithOperation:DIFF_EQUAL andText:@"\r\nEEE"], nil];
  [dmp diff_cleanupSemanticLossless:diffs];
  expectedResult = [NSMutableArray arrayWithObjects:
      [Diff diffWithOperation:DIFF_EQUAL andText:@"AAA\r\n\r\n"],
      [Diff diffWithOperation:DIFF_INSERT andText:@"BBB\r\nDDD\r\n\r\n"],
      [Diff diffWithOperation:DIFF_EQUAL andText:@"BBB\r\nEEE"], nil];
  XCTAssertEqualObjects(expectedResult, diffs, @"Blank lines.");

  // Line boundaries.
  diffs = [NSMutableArray arrayWithObjects:
      [Diff diffWithOperation:DIFF_EQUAL andText:@"AAA\r\nBBB"],
      [Diff diffWithOperation:DIFF_INSERT andText:@" DDD\r\nBBB"],
      [Diff diffWithOperation:DIFF_EQUAL andText:@" EEE"], nil];
  [dmp diff_cleanupSemanticLossless:diffs];
  expectedResult = [NSMutableArray arrayWithObjects:
      [Diff diffWithOperation:DIFF_EQUAL andText:@"AAA\r\n"],
      [Diff diffWithOperation:DIFF_INSERT andText:@"BBB DDD\r\n"],
      [Diff diffWithOperation:DIFF_EQUAL andText:@"BBB EEE"], nil];
  XCTAssertEqualObjects(expectedResult, diffs, @"Line boundaries.");

  // Word boundaries.
  diffs = [NSMutableArray arrayWithObjects:
      [Diff diffWithOperation:DIFF_EQUAL andText:@"The c"],
      [Diff diffWithOperation:DIFF_INSERT andText:@"ow and the c"],
      [Diff diffWithOperation:DIFF_EQUAL andText:@"at."], nil];
  [dmp diff_cleanupSemanticLossless:diffs];
  expectedResult = [NSMutableArray arrayWithObjects:
      [Diff diffWithOperation:DIFF_EQUAL andText:@"The "],
      [Diff diffWithOperation:DIFF_INSERT andText:@"cow and the "],
      [Diff diffWithOperation:DIFF_EQUAL andText:@"cat."], nil];
  XCTAssertEqualObjects(expectedResult, diffs, @"Word boundaries.");

  // Alphanumeric boundaries.
  diffs = [NSMutableArray arrayWithObjects:
      [Diff diffWithOperation:DIFF_EQUAL andText:@"The-c"],
      [Diff diffWithOperation:DIFF_INSERT andText:@"ow-and-the-c"],
      [Diff diffWithOperation:DIFF_EQUAL andText:@"at."], nil];
  [dmp diff_cleanupSemanticLossless:diffs];
  expectedResult = [NSMutableArray arrayWithObjects:
      [Diff diffWithOperation:DIFF_EQUAL andText:@"The-"],
      [Diff diffWithOperation:DIFF_INSERT andText:@"cow-and-the-"],
      [Diff diffWithOperation:DIFF_EQUAL andText:@"cat."], nil];
  XCTAssertEqualObjects(expectedResult, diffs, @"Alphanumeric boundaries.");

  // Hitting the start.
  diffs = [NSMutableArray arrayWithObjects:
      [Diff diffWithOperation:DIFF_EQUAL andText:@"a"],
      [Diff diffWithOperation:DIFF_DELETE andText:@"a"],
      [Diff diffWithOperation:DIFF_EQUAL andText:@"ax"], nil];
  [dmp diff_cleanupSemanticLossless:diffs];
  expectedResult = [NSMutableArray arrayWithObjects:
      [Diff diffWithOperation:DIFF_DELETE andText:@"a"],
      [Diff diffWithOperation:DIFF_EQUAL andText:@"aax"], nil];
  XCTAssertEqualObjects(expectedResult, diffs, @"Hitting the start.");

  // Hitting the end.
  diffs = [NSMutableArray arrayWithObjects:
      [Diff diffWithOperation:DIFF_EQUAL andText:@"xa"],
      [Diff diffWithOperation:DIFF_DELETE andText:@"a"],
      [Diff diffWithOperation:DIFF_EQUAL andText:@"a"], nil];
  [dmp diff_cleanupSemanticLossless:diffs];
  expectedResult = [NSMutableArray arrayWithObjects:
      [Diff diffWithOperation:DIFF_EQUAL andText:@"xaa"],
      [Diff diffWithOperation:DIFF_DELETE andText:@"a"], nil];
  XCTAssertEqualObjects(expectedResult, diffs, @"Hitting the end.");

  // Alphanumeric boundaries.
  diffs = [NSMutableArray arrayWithObjects:
      [Diff diffWithOperation:DIFF_EQUAL andText:@"The xxx. The "],
      [Diff diffWithOperation:DIFF_INSERT andText:@"zzz. The "],
      [Diff diffWithOperation:DIFF_EQUAL andText:@"yyy."], nil];
  [dmp diff_cleanupSemanticLossless:diffs];
  expectedResult = [NSMutableArray arrayWithObjects:
      [Diff diffWithOperation:DIFF_EQUAL andText:@"The xxx."],
      [Diff diffWithOperation:DIFF_INSERT andText:@" The zzz."],
      [Diff diffWithOperation:DIFF_EQUAL andText:@" The yyy."], nil];
  XCTAssertEqualObjects(expectedResult, diffs, @"Sentence boundaries.");

  [dmp release];
}

- (void)test_diff_cleanupSemanticTest {
  DiffMatchPatch *dmp = [DiffMatchPatch new];
  NSMutableArray *expectedResult = nil;

  // Cleanup semantically trivial equalities.
  // Null case.
  NSMutableArray *diffs = [NSMutableArray array];
  [dmp diff_cleanupSemantic:diffs];
  XCTAssertEqualObjects([NSMutableArray array], diffs, @"Null case.");

  // No elimination #1.
  diffs = [NSMutableArray arrayWithObjects:
      [Diff diffWithOperation:DIFF_DELETE andText:@"ab"],
      [Diff diffWithOperation:DIFF_INSERT andText:@"cd"],
      [Diff diffWithOperation:DIFF_EQUAL andText:@"12"],
      [Diff diffWithOperation:DIFF_DELETE andText:@"e"], nil];
  [dmp diff_cleanupSemantic:diffs];
  expectedResult = [NSMutableArray arrayWithObjects:
      [Diff diffWithOperation:DIFF_DELETE andText:@"ab"],
      [Diff diffWithOperation:DIFF_INSERT andText:@"cd"],
      [Diff diffWithOperation:DIFF_EQUAL andText:@"12"],
      [Diff diffWithOperation:DIFF_DELETE andText:@"e"], nil];
  XCTAssertEqualObjects(expectedResult, diffs, @"No elimination #1.");

  // No elimination #2.
  diffs = [NSMutableArray arrayWithObjects:
      [Diff diffWithOperation:DIFF_DELETE andText:@"abc"],
      [Diff diffWithOperation:DIFF_INSERT andText:@"ABC"],
      [Diff diffWithOperation:DIFF_EQUAL andText:@"1234"],
      [Diff diffWithOperation:DIFF_DELETE andText:@"wxyz"], nil];
  [dmp diff_cleanupSemantic:diffs];
  expectedResult = [NSMutableArray arrayWithObjects:
      [Diff diffWithOperation:DIFF_DELETE andText:@"abc"],
      [Diff diffWithOperation:DIFF_INSERT andText:@"ABC"],
      [Diff diffWithOperation:DIFF_EQUAL andText:@"1234"],
      [Diff diffWithOperation:DIFF_DELETE andText:@"wxyz"], nil];
  XCTAssertEqualObjects(expectedResult, diffs, @"No elimination #2.");

  // Simple elimination.
  diffs = [NSMutableArray arrayWithObjects:
      [Diff diffWithOperation:DIFF_DELETE andText:@"a"],
      [Diff diffWithOperation:DIFF_EQUAL andText:@"b"],
      [Diff diffWithOperation:DIFF_DELETE andText:@"c"], nil];
  [dmp diff_cleanupSemantic:diffs];
  expectedResult = [NSMutableArray arrayWithObjects:
      [Diff diffWithOperation:DIFF_DELETE andText:@"abc"],
      [Diff diffWithOperation:DIFF_INSERT andText:@"b"], nil];
  XCTAssertEqualObjects(expectedResult, diffs, @"Simple elimination.");

  // Backpass elimination.
  diffs = [NSMutableArray arrayWithObjects:
      [Diff diffWithOperation:DIFF_DELETE andText:@"ab"],
      [Diff diffWithOperation:DIFF_EQUAL andText:@"cd"],
      [Diff diffWithOperation:DIFF_DELETE andText:@"e"],
      [Diff diffWithOperation:DIFF_EQUAL andText:@"f"],
      [Diff diffWithOperation:DIFF_INSERT andText:@"g"], nil];
  [dmp diff_cleanupSemantic:diffs];
  expectedResult = [NSMutableArray arrayWithObjects:
      [Diff diffWithOperation:DIFF_DELETE andText:@"abcdef"],
      [Diff diffWithOperation:DIFF_INSERT andText:@"cdfg"], nil];
  XCTAssertEqualObjects(expectedResult, diffs, @"Backpass elimination.");

  // Multiple eliminations.
  diffs = [NSMutableArray arrayWithObjects:
      [Diff diffWithOperation:DIFF_INSERT andText:@"1"],
      [Diff diffWithOperation:DIFF_EQUAL andText:@"A"],
      [Diff diffWithOperation:DIFF_DELETE andText:@"B"],
      [Diff diffWithOperation:DIFF_INSERT andText:@"2"],
      [Diff diffWithOperation:DIFF_EQUAL andText:@"_"],
      [Diff diffWithOperation:DIFF_INSERT andText:@"1"],
      [Diff diffWithOperation:DIFF_EQUAL andText:@"A"],
      [Diff diffWithOperation:DIFF_DELETE andText:@"B"],
      [Diff diffWithOperation:DIFF_INSERT andText:@"2"], nil];
  [dmp diff_cleanupSemantic:diffs];
  expectedResult = [NSMutableArray arrayWithObjects:
      [Diff diffWithOperation:DIFF_DELETE andText:@"AB_AB"],
      [Diff diffWithOperation:DIFF_INSERT andText:@"1A2_1A2"], nil];
  XCTAssertEqualObjects(expectedResult, diffs, @"Multiple eliminations.");

  // Word boundaries.
  diffs = [NSMutableArray arrayWithObjects:
      [Diff diffWithOperation:DIFF_EQUAL andText:@"The c"],
      [Diff diffWithOperation:DIFF_DELETE andText:@"ow and the c"],
      [Diff diffWithOperation:DIFF_EQUAL andText:@"at."], nil];
  [dmp diff_cleanupSemantic:diffs];
  expectedResult = [NSMutableArray arrayWithObjects:
      [Diff diffWithOperation:DIFF_EQUAL andText:@"The "],
      [Diff diffWithOperation:DIFF_DELETE andText:@"cow and the "],
      [Diff diffWithOperation:DIFF_EQUAL andText:@"cat."], nil];
  XCTAssertEqualObjects(expectedResult, diffs, @"Word boundaries.");

  // No overlap elimination.
  diffs = [NSMutableArray arrayWithObjects:
      [Diff diffWithOperation:DIFF_DELETE andText:@"abcxx"],
      [Diff diffWithOperation:DIFF_INSERT andText:@"xxdef"], nil];
  [dmp diff_cleanupSemantic:diffs];
  expectedResult = [NSMutableArray arrayWithObjects:
      [Diff diffWithOperation:DIFF_DELETE andText:@"abcxx"],
      [Diff diffWithOperation:DIFF_INSERT andText:@"xxdef"], nil];
  XCTAssertEqualObjects(expectedResult, diffs, @"No overlap elimination.");

  // Overlap elimination.
  diffs = [NSMutableArray arrayWithObjects:
      [Diff diffWithOperation:DIFF_DELETE andText:@"abcxxx"],
      [Diff diffWithOperation:DIFF_INSERT andText:@"xxxdef"], nil];
  [dmp diff_cleanupSemantic:diffs];
  expectedResult = [NSMutableArray arrayWithObjects:
      [Diff diffWithOperation:DIFF_DELETE andText:@"abc"],
      [Diff diffWithOperation:DIFF_EQUAL andText:@"xxx"],
      [Diff diffWithOperation:DIFF_INSERT andText:@"def"], nil];
  XCTAssertEqualObjects(expectedResult, diffs, @"Overlap elimination.");

  // Reverse overlap elimination.
  diffs = [NSMutableArray arrayWithObjects:
      [Diff diffWithOperation:DIFF_DELETE andText:@"xxxabc"],
      [Diff diffWithOperation:DIFF_INSERT andText:@"defxxx"], nil];
  [dmp diff_cleanupSemantic:diffs];
  expectedResult = [NSMutableArray arrayWithObjects:
      [Diff diffWithOperation:DIFF_INSERT andText:@"def"],
      [Diff diffWithOperation:DIFF_EQUAL andText:@"xxx"],
      [Diff diffWithOperation:DIFF_DELETE andText:@"abc"], nil];
  XCTAssertEqualObjects(expectedResult, diffs, @"Reverse overlap elimination.");

  // Two overlap eliminations.
  diffs = [NSMutableArray arrayWithObjects:
      [Diff diffWithOperation:DIFF_DELETE andText:@"abcd1212"],
      [Diff diffWithOperation:DIFF_INSERT andText:@"1212efghi"],
      [Diff diffWithOperation:DIFF_EQUAL andText:@"----"],
      [Diff diffWithOperation:DIFF_DELETE andText:@"A3"],
      [Diff diffWithOperation:DIFF_INSERT andText:@"3BC"], nil];
  [dmp diff_cleanupSemantic:diffs];
  expectedResult = [NSMutableArray arrayWithObjects:
      [Diff diffWithOperation:DIFF_DELETE andText:@"abcd"],
      [Diff diffWithOperation:DIFF_EQUAL andText:@"1212"],
      [Diff diffWithOperation:DIFF_INSERT andText:@"efghi"],
      [Diff diffWithOperation:DIFF_EQUAL andText:@"----"],
      [Diff diffWithOperation:DIFF_DELETE andText:@"A"],
      [Diff diffWithOperation:DIFF_EQUAL andText:@"3"],
      [Diff diffWithOperation:DIFF_INSERT andText:@"BC"], nil];
  XCTAssertEqualObjects(expectedResult, diffs, @"Two overlap eliminations.");

  [dmp release];
}

- (void)test_diff_cleanupEfficiencyTest {
  DiffMatchPatch *dmp = [DiffMatchPatch new];
  NSMutableArray *expectedResult = nil;

  // Cleanup operationally trivial equalities.
  dmp.Diff_EditCost = 4;
  // Null case.
  NSMutableArray *diffs = [NSMutableArray array];
  [dmp diff_cleanupEfficiency:diffs];
  XCTAssertEqualObjects([NSMutableArray array], diffs, @"Null case.");

  // No elimination.
  diffs = [NSMutableArray arrayWithObjects:
      [Diff diffWithOperation:DIFF_DELETE andText:@"ab"],
      [Diff diffWithOperation:DIFF_INSERT andText:@"12"],
      [Diff diffWithOperation:DIFF_EQUAL andText:@"wxyz"],
      [Diff diffWithOperation:DIFF_DELETE andText:@"cd"],
      [Diff diffWithOperation:DIFF_INSERT andText:@"34"], nil];
  [dmp diff_cleanupEfficiency:diffs];
  expectedResult = [NSMutableArray arrayWithObjects:
      [Diff diffWithOperation:DIFF_DELETE andText:@"ab"],
      [Diff diffWithOperation:DIFF_INSERT andText:@"12"],
      [Diff diffWithOperation:DIFF_EQUAL andText:@"wxyz"],
      [Diff diffWithOperation:DIFF_DELETE andText:@"cd"],
      [Diff diffWithOperation:DIFF_INSERT andText:@"34"], nil];
  XCTAssertEqualObjects(expectedResult, diffs, @"No elimination.");

  // Four-edit elimination.
  diffs = [NSMutableArray arrayWithObjects:
      [Diff diffWithOperation:DIFF_DELETE andText:@"ab"],
      [Diff diffWithOperation:DIFF_INSERT andText:@"12"],
      [Diff diffWithOperation:DIFF_EQUAL andText:@"xyz"],
      [Diff diffWithOperation:DIFF_DELETE andText:@"cd"],
      [Diff diffWithOperation:DIFF_INSERT andText:@"34"], nil];
  [dmp diff_cleanupEfficiency:diffs];
  expectedResult = [NSMutableArray arrayWithObjects:
      [Diff diffWithOperation:DIFF_DELETE andText:@"abxyzcd"],
      [Diff diffWithOperation:DIFF_INSERT andText:@"12xyz34"], nil];
  XCTAssertEqualObjects(expectedResult, diffs, @"Four-edit elimination.");

  // Three-edit elimination.
  diffs = [NSMutableArray arrayWithObjects:
      [Diff diffWithOperation:DIFF_INSERT andText:@"12"],
      [Diff diffWithOperation:DIFF_EQUAL andText:@"x"],
      [Diff diffWithOperation:DIFF_DELETE andText:@"cd"],
      [Diff diffWithOperation:DIFF_INSERT andText:@"34"], nil];
  [dmp diff_cleanupEfficiency:diffs];
  expectedResult = [NSMutableArray arrayWithObjects:
      [Diff diffWithOperation:DIFF_DELETE andText:@"xcd"],
      [Diff diffWithOperation:DIFF_INSERT andText:@"12x34"], nil];
  XCTAssertEqualObjects(expectedResult, diffs, @"Three-edit elimination.");

  // Backpass elimination.
  diffs = [NSMutableArray arrayWithObjects:
      [Diff diffWithOperation:DIFF_DELETE andText:@"ab"],
      [Diff diffWithOperation:DIFF_INSERT andText:@"12"],
      [Diff diffWithOperation:DIFF_EQUAL andText:@"xy"],
      [Diff diffWithOperation:DIFF_INSERT andText:@"34"],
      [Diff diffWithOperation:DIFF_EQUAL andText:@"z"],
      [Diff diffWithOperation:DIFF_DELETE andText:@"cd"],
      [Diff diffWithOperation:DIFF_INSERT andText:@"56"], nil];
  [dmp diff_cleanupEfficiency:diffs];
  expectedResult = [NSMutableArray arrayWithObjects:
      [Diff diffWithOperation:DIFF_DELETE andText:@"abxyzcd"],
      [Diff diffWithOperation:DIFF_INSERT andText:@"12xy34z56"], nil];
  XCTAssertEqualObjects(expectedResult, diffs, @"Backpass elimination.");

  // High cost elimination.
  dmp.Diff_EditCost = 5;
  diffs = [NSMutableArray arrayWithObjects:
      [Diff diffWithOperation:DIFF_DELETE andText:@"ab"],
      [Diff diffWithOperation:DIFF_INSERT andText:@"12"],
      [Diff diffWithOperation:DIFF_EQUAL andText:@"wxyz"],
      [Diff diffWithOperation:DIFF_DELETE andText:@"cd"],
      [Diff diffWithOperation:DIFF_INSERT andText:@"34"], nil];
  [dmp diff_cleanupEfficiency:diffs];
  expectedResult = [NSMutableArray arrayWithObjects:
      [Diff diffWithOperation:DIFF_DELETE andText:@"abwxyzcd"],
      [Diff diffWithOperation:DIFF_INSERT andText:@"12wxyz34"], nil];
  XCTAssertEqualObjects(expectedResult, diffs, @"High cost elimination.");
  dmp.Diff_EditCost = 4;

  [dmp release];
}

- (void)test_diff_prettyHtmlTest {
  DiffMatchPatch *dmp = [DiffMatchPatch new];

  // Pretty print.
  NSMutableArray *diffs = [NSMutableArray arrayWithObjects:
      [Diff diffWithOperation:DIFF_EQUAL andText:@"a\n"],
      [Diff diffWithOperation:DIFF_DELETE andText:@"<B>b</B>"],
      [Diff diffWithOperation:DIFF_INSERT andText:@"c&d"], nil];
  NSString *expectedResult = @"<span>a&para;<br></span><del style=\"background:#ffe6e6;\">&lt;B&gt;b&lt;/B&gt;</del><ins style=\"background:#e6ffe6;\">c&amp;d</ins>";
  XCTAssertEqualObjects(expectedResult, [dmp diff_prettyHtml:diffs], @"Pretty print.");

  [dmp release];
}

- (void)test_diff_textTest {
  DiffMatchPatch *dmp = [DiffMatchPatch new];

  // Compute the source and destination texts.
  NSMutableArray *diffs = [NSMutableArray arrayWithObjects:
      [Diff diffWithOperation:DIFF_EQUAL andText:@"jump"],
      [Diff diffWithOperation:DIFF_DELETE andText:@"s"],
      [Diff diffWithOperation:DIFF_INSERT andText:@"ed"],
      [Diff diffWithOperation:DIFF_EQUAL andText:@" over "],
      [Diff diffWithOperation:DIFF_DELETE andText:@"the"],
      [Diff diffWithOperation:DIFF_INSERT andText:@"a"],
      [Diff diffWithOperation:DIFF_EQUAL andText:@" lazy"], nil];
  XCTAssertEqualObjects(@"jumps over the lazy", [dmp diff_text1:diffs], @"Compute the source and destination texts #1");

  XCTAssertEqualObjects(@"jumped over a lazy", [dmp diff_text2:diffs], @"Compute the source and destination texts #2");

  [dmp release];
}

- (void)test_diff_deltaTest {
  DiffMatchPatch *dmp = [DiffMatchPatch new];
  NSMutableArray *expectedResult = nil;
  NSError *error = nil;

  // Convert a diff into delta string.
  NSMutableArray *diffs = [NSMutableArray arrayWithObjects:
      [Diff diffWithOperation:DIFF_EQUAL andText:@"jump"],
      [Diff diffWithOperation:DIFF_DELETE andText:@"s"],
      [Diff diffWithOperation:DIFF_INSERT andText:@"ed"],
      [Diff diffWithOperation:DIFF_EQUAL andText:@" over "],
      [Diff diffWithOperation:DIFF_DELETE andText:@"the"],
      [Diff diffWithOperation:DIFF_INSERT andText:@"a"],
      [Diff diffWithOperation:DIFF_EQUAL andText:@" lazy"],
      [Diff diffWithOperation:DIFF_INSERT andText:@"old dog"], nil];
  NSString *text1 = [dmp diff_text1:diffs];
  XCTAssertEqualObjects(@"jumps over the lazy", text1, @"Convert a diff into delta string 1.");

  NSString *delta = [dmp diff_toDelta:diffs];
  XCTAssertEqualObjects(@"=4\t-1\t+ed\t=6\t-3\t+a\t=5\t+old dog", delta, @"Convert a diff into delta string 2.");

  // Convert delta string into a diff.
  XCTAssertEqualObjects(diffs, [dmp diff_fromDeltaWithText:text1 andDelta:delta error:NULL], @"Convert delta string into a diff.");

  // Generates error (19 < 20).
  diffs = [dmp diff_fromDeltaWithText:[text1 stringByAppendingString:@"x"] andDelta:delta error:&error];
  if (diffs != nil || error == nil) {
    XCTFail(@"diff_fromDelta: Too long.");
  }
  error = nil;

  // Generates error (19 > 18).
  diffs = [dmp diff_fromDeltaWithText:[text1 substringFromIndex:1] andDelta:delta error:&error];
  if (diffs != nil || error == nil) {
    XCTFail(@"diff_fromDelta: Too short.");
  }
  error = nil;

  // Generates error (%c3%xy invalid Unicode).
  diffs = [dmp diff_fromDeltaWithText:@"" andDelta:@"+%c3%xy" error:&error];
  if (diffs != nil || error == nil) {
    XCTFail(@"diff_fromDelta: Invalid character.");
  }
  error = nil;

  // Test deltas with special characters.
  unichar zero = (unichar)0;
  unichar one = (unichar)1;
  unichar two = (unichar)2;
  diffs = [NSMutableArray arrayWithObjects:
      [Diff diffWithOperation:DIFF_EQUAL andText:[NSString stringWithFormat:@"\U00000680 %C \t %%", zero]],
      [Diff diffWithOperation:DIFF_DELETE andText:[NSString stringWithFormat:@"\U00000681 %C \n ^", one]],
      [Diff diffWithOperation:DIFF_INSERT andText:[NSString stringWithFormat:@"\U00000682 %C \\ |", two]], nil];
  text1 = [dmp diff_text1:diffs];
  NSString *expectedString = [NSString stringWithFormat:@"\U00000680 %C \t %%\U00000681 %C \n ^", zero, one];
  XCTAssertEqualObjects(expectedString, text1, @"Test deltas with special characters.");

  delta = [dmp diff_toDelta:diffs];
  // Upper case, because to CFURLCreateStringByAddingPercentEscapes() uses upper.
  XCTAssertEqualObjects(@"=7\t-7\t+%DA%82 %02 %5C %7C", delta, @"diff_toDelta: Unicode 1.");

  XCTAssertEqualObjects(diffs, [dmp diff_fromDeltaWithText:text1 andDelta:delta error:NULL], @"diff_fromDelta: Unicode 2.");

  // Verify pool of unchanged characters.
  diffs = [NSMutableArray arrayWithObject:
       [Diff diffWithOperation:DIFF_INSERT andText:@"A-Z a-z 0-9 - _ . ! ~ * ' ( ) ; / ? : @ & = + $ , # "]];
  NSString *text2 = [dmp diff_text2:diffs];
  XCTAssertEqualObjects(@"A-Z a-z 0-9 - _ . ! ~ * ' ( ) ; / ? : @ & = + $ , # ", text2, @"diff_text2: Unchanged characters 1.");

  delta = [dmp diff_toDelta:diffs];
  XCTAssertEqualObjects(@"+A-Z a-z 0-9 - _ . ! ~ * ' ( ) ; / ? : @ & = + $ , # ", delta, @"diff_toDelta: Unchanged characters 2.");

  // Convert delta string into a diff.
  expectedResult = [dmp diff_fromDeltaWithText:@"" andDelta:delta error:NULL];
  XCTAssertEqualObjects(diffs, expectedResult, @"diff_fromDelta: Unchanged characters. Convert delta string into a diff.");

  // 160 kb string.
  NSString *a = @"abcdefghij";
  NSMutableString *aMutable = [NSMutableString stringWithString:a];
  for (int i = 0; i < 14; i++) {
    [aMutable appendString:aMutable];
  }
  a = aMutable;
  diffs = [NSMutableArray arrayWithObject:
           [Diff diffWithOperation:DIFF_INSERT andText:a]];
  delta = [dmp diff_toDelta:diffs];
  XCTAssertEqualObjects([@"+" stringByAppendingString:a], delta, @"diff_toDelta: 160kb string.");
  
  // Convert delta string into a diff.
  expectedResult = [dmp diff_fromDeltaWithText:@"" andDelta:delta error:NULL];
  XCTAssertEqualObjects(diffs, expectedResult, @"diff_fromDelta: 160kb string. Convert delta string into a diff.");

  [dmp release];
}

- (void)test_diff_xIndexTest {
  DiffMatchPatch *dmp = [DiffMatchPatch new];

  // Translate a location in text1 to text2.
  NSMutableArray *diffs = [NSMutableArray arrayWithObjects:
      [Diff diffWithOperation:DIFF_DELETE andText:@"a"],
      [Diff diffWithOperation:DIFF_INSERT andText:@"1234"],
      [Diff diffWithOperation:DIFF_EQUAL andText:@"xyz"], nil] /* Diff */;
  XCTAssertEqual((NSUInteger)5, [dmp diff_xIndexIn:diffs location:2], @"diff_xIndex: Translation on equality. Translate a location in text1 to text2.");

  diffs = [NSMutableArray arrayWithObjects:
      [Diff diffWithOperation:DIFF_EQUAL andText:@"a"],
      [Diff diffWithOperation:DIFF_DELETE andText:@"1234"],
      [Diff diffWithOperation:DIFF_EQUAL andText:@"xyz"], nil] /* Diff */;
  XCTAssertEqual((NSUInteger)1, [dmp diff_xIndexIn:diffs location:3], @"diff_xIndex: Translation on deletion.");

  [dmp release];
}

- (void)test_diff_levenshteinTest {
  DiffMatchPatch *dmp = [DiffMatchPatch new];

  NSMutableArray *diffs = [NSMutableArray arrayWithObjects:
      [Diff diffWithOperation:DIFF_DELETE andText:@"abc"],
      [Diff diffWithOperation:DIFF_INSERT andText:@"1234"],
      [Diff diffWithOperation:DIFF_EQUAL andText:@"xyz"], nil] /* Diff */;
  XCTAssertEqual((NSUInteger)4, [dmp diff_levenshtein:diffs], @"diff_levenshtein: Levenshtein with trailing equality.");

  diffs = [NSMutableArray arrayWithObjects:
      [Diff diffWithOperation:DIFF_EQUAL andText:@"xyz"],
      [Diff diffWithOperation:DIFF_DELETE andText:@"abc"],
      [Diff diffWithOperation:DIFF_INSERT andText:@"1234"], nil] /* Diff */;
  XCTAssertEqual((NSUInteger)4, [dmp diff_levenshtein:diffs], @"diff_levenshtein: Levenshtein with leading equality.");

  diffs = [NSMutableArray arrayWithObjects:
      [Diff diffWithOperation:DIFF_DELETE andText:@"abc"],
      [Diff diffWithOperation:DIFF_EQUAL andText:@"xyz"],
      [Diff diffWithOperation:DIFF_INSERT andText:@"1234"], nil] /* Diff */;
  XCTAssertEqual((NSUInteger)7, [dmp diff_levenshtein:diffs], @"diff_levenshtein: Levenshtein with middle equality.");

  [dmp release];
}

- (void)diff_bisectTest;
{
  DiffMatchPatch *dmp = [DiffMatchPatch new];

  // Normal.
  NSString *a = @"cat";
  NSString *b = @"map";
  // Since the resulting diff hasn't been normalized, it would be ok if
  // the insertion and deletion pairs are swapped.
  // If the order changes, tweak this test as required.
  NSMutableArray *diffs = [NSMutableArray arrayWithObjects:[Diff diffWithOperation:DIFF_DELETE andText:@"c"], [Diff diffWithOperation:DIFF_INSERT andText:@"m"], [Diff diffWithOperation:DIFF_EQUAL andText:@"a"], [Diff diffWithOperation:DIFF_DELETE andText:@"t"], [Diff diffWithOperation:DIFF_INSERT andText:@"p"], nil];
  XCTAssertEqualObjects(diffs, [dmp diff_bisectOfOldString:a andNewString:b deadline:[[NSDate distantFuture] timeIntervalSinceReferenceDate]], @"Bisect test.");

  // Timeout.
  diffs = [NSMutableArray arrayWithObjects:[Diff diffWithOperation:DIFF_DELETE andText:@"cat"], [Diff diffWithOperation:DIFF_INSERT andText:@"map"], nil];
  XCTAssertEqualObjects(diffs, [dmp diff_bisectOfOldString:a andNewString:b deadline:[[NSDate distantPast] timeIntervalSinceReferenceDate]], @"Bisect timeout.");

  [dmp release];
}

- (void)test_diff_mainTest {
  DiffMatchPatch *dmp = [DiffMatchPatch new];

  // Perform a trivial diff.
  NSMutableArray *diffs = [NSMutableArray array];
  XCTAssertEqualObjects(diffs, [dmp diff_mainOfOldString:@"" andNewString:@"" checkLines:NO], @"diff_main: Null case.");

  diffs = [NSMutableArray arrayWithObjects:[Diff diffWithOperation:DIFF_EQUAL andText:@"abc"], nil];
  XCTAssertEqualObjects(diffs, [dmp diff_mainOfOldString:@"abc" andNewString:@"abc" checkLines:NO], @"diff_main: Equality.");

  diffs = [NSMutableArray arrayWithObjects:[Diff diffWithOperation:DIFF_EQUAL andText:@"ab"], [Diff diffWithOperation:DIFF_INSERT andText:@"123"], [Diff diffWithOperation:DIFF_EQUAL andText:@"c"], nil];
  XCTAssertEqualObjects(diffs, [dmp diff_mainOfOldString:@"abc" andNewString:@"ab123c" checkLines:NO], @"diff_main: Simple insertion.");

  diffs = [NSMutableArray arrayWithObjects:[Diff diffWithOperation:DIFF_EQUAL andText:@"a"], [Diff diffWithOperation:DIFF_DELETE andText:@"123"], [Diff diffWithOperation:DIFF_EQUAL andText:@"bc"], nil];
  XCTAssertEqualObjects(diffs, [dmp diff_mainOfOldString:@"a123bc" andNewString:@"abc" checkLines:NO], @"diff_main: Simple deletion.");

  diffs = [NSMutableArray arrayWithObjects:[Diff diffWithOperation:DIFF_EQUAL andText:@"a"], [Diff diffWithOperation:DIFF_INSERT andText:@"123"], [Diff diffWithOperation:DIFF_EQUAL andText:@"b"], [Diff diffWithOperation:DIFF_INSERT andText:@"456"], [Diff diffWithOperation:DIFF_EQUAL andText:@"c"], nil];
  XCTAssertEqualObjects(diffs, [dmp diff_mainOfOldString:@"abc" andNewString:@"a123b456c" checkLines:NO], @"diff_main: Two insertions.");

  diffs = [NSMutableArray arrayWithObjects:[Diff diffWithOperation:DIFF_EQUAL andText:@"a"], [Diff diffWithOperation:DIFF_DELETE andText:@"123"], [Diff diffWithOperation:DIFF_EQUAL andText:@"b"], [Diff diffWithOperation:DIFF_DELETE andText:@"456"], [Diff diffWithOperation:DIFF_EQUAL andText:@"c"], nil];
  XCTAssertEqualObjects(diffs, [dmp diff_mainOfOldString:@"a123b456c" andNewString:@"abc" checkLines:NO], @"diff_main: Two deletions.");

  // Perform a real diff.
  // Switch off the timeout.
  dmp.Diff_Timeout = 0;
  diffs = [NSMutableArray arrayWithObjects:[Diff diffWithOperation:DIFF_DELETE andText:@"a"], [Diff diffWithOperation:DIFF_INSERT andText:@"b"], nil];
  XCTAssertEqualObjects(diffs, [dmp diff_mainOfOldString:@"a" andNewString:@"b" checkLines:NO], @"diff_main: Simple case #1.");

  diffs = [NSMutableArray arrayWithObjects:[Diff diffWithOperation:DIFF_DELETE andText:@"Apple"], [Diff diffWithOperation:DIFF_INSERT andText:@"Banana"], [Diff diffWithOperation:DIFF_EQUAL andText:@"s are a"], [Diff diffWithOperation:DIFF_INSERT andText:@"lso"], [Diff diffWithOperation:DIFF_EQUAL andText:@" fruit."], nil];
  XCTAssertEqualObjects(diffs, [dmp diff_mainOfOldString:@"Apples are a fruit." andNewString:@"Bananas are also fruit." checkLines:NO], @"diff_main: Simple case #2.");

  diffs = [NSMutableArray arrayWithObjects:[Diff diffWithOperation:DIFF_DELETE andText:@"a"], [Diff diffWithOperation:DIFF_INSERT andText:@"\U00000680"], [Diff diffWithOperation:DIFF_EQUAL andText:@"x"], [Diff diffWithOperation:DIFF_DELETE andText:@"\t"], [Diff diffWithOperation:DIFF_INSERT andText:[NSString stringWithFormat:@"%C", 0]], nil];
  NSString *aString = [NSString stringWithFormat:@"\U00000680x%C", 0];
  XCTAssertEqualObjects(diffs, [dmp diff_mainOfOldString:@"ax\t" andNewString:aString checkLines:NO], @"diff_main: Simple case #3.");

  diffs = [NSMutableArray arrayWithObjects:[Diff diffWithOperation:DIFF_DELETE andText:@"1"], [Diff diffWithOperation:DIFF_EQUAL andText:@"a"], [Diff diffWithOperation:DIFF_DELETE andText:@"y"], [Diff diffWithOperation:DIFF_EQUAL andText:@"b"], [Diff diffWithOperation:DIFF_DELETE andText:@"2"], [Diff diffWithOperation:DIFF_INSERT andText:@"xab"], nil];
  XCTAssertEqualObjects(diffs, [dmp diff_mainOfOldString:@"1ayb2" andNewString:@"abxab" checkLines:NO], @"diff_main: Overlap #1.");

  diffs = [NSMutableArray arrayWithObjects:[Diff diffWithOperation:DIFF_INSERT andText:@"xaxcx"], [Diff diffWithOperation:DIFF_EQUAL andText:@"abc"], [Diff diffWithOperation:DIFF_DELETE andText:@"y"], nil];
  XCTAssertEqualObjects(diffs, [dmp diff_mainOfOldString:@"abcy" andNewString:@"xaxcxabc" checkLines:NO], @"diff_main: Overlap #2.");

  diffs = [NSMutableArray arrayWithObjects:[Diff diffWithOperation:DIFF_DELETE andText:@"ABCD"], [Diff diffWithOperation:DIFF_EQUAL andText:@"a"], [Diff diffWithOperation:DIFF_DELETE andText:@"="], [Diff diffWithOperation:DIFF_INSERT andText:@"-"], [Diff diffWithOperation:DIFF_EQUAL andText:@"bcd"], [Diff diffWithOperation:DIFF_DELETE andText:@"="], [Diff diffWithOperation:DIFF_INSERT andText:@"-"], [Diff diffWithOperation:DIFF_EQUAL andText:@"efghijklmnopqrs"], [Diff diffWithOperation:DIFF_DELETE andText:@"EFGHIJKLMNOefg"], nil];
  XCTAssertEqualObjects(diffs, [dmp diff_mainOfOldString:@"ABCDa=bcd=efghijklmnopqrsEFGHIJKLMNOefg" andNewString:@"a-bcd-efghijklmnopqrs" checkLines:NO], @"diff_main: Overlap #3.");

  diffs = [NSMutableArray arrayWithObjects:[Diff diffWithOperation:DIFF_INSERT andText:@" "], [Diff diffWithOperation:DIFF_EQUAL andText:@"a"], [Diff diffWithOperation:DIFF_INSERT andText:@"nd"], [Diff diffWithOperation:DIFF_EQUAL andText:@" [[Pennsylvania]]"], [Diff diffWithOperation:DIFF_DELETE andText:@" and [[New"], nil];
  XCTAssertEqualObjects(diffs, [dmp diff_mainOfOldString:@"a [[Pennsylvania]] and [[New" andNewString:@" and [[Pennsylvania]]" checkLines:NO], @"diff_main: Large equality.");

  dmp.Diff_Timeout = 0.1f;  // 100ms
  NSString *a = @"`Twas brillig, and the slithy toves\nDid gyre and gimble in the wabe:\nAll mimsy were the borogoves,\nAnd the mome raths outgrabe.\n";
  NSString *b = @"I am the very model of a modern major general,\nI've information vegetable, animal, and mineral,\nI know the kings of England, and I quote the fights historical,\nFrom Marathon to Waterloo, in order categorical.\n";
  NSMutableString *aMutable = [NSMutableString stringWithString:a];
  NSMutableString *bMutable = [NSMutableString stringWithString:b];
  // Increase the text lengths by 1024 times to ensure a timeout.
  for (int i = 0; i < 10; i++) {
    [aMutable appendString:aMutable];
    [bMutable appendString:bMutable];
  }
  a = aMutable;
  b = bMutable;
  NSTimeInterval startTime = [NSDate timeIntervalSinceReferenceDate];
  [dmp diff_mainOfOldString:a andNewString:b];
  NSTimeInterval endTime = [NSDate timeIntervalSinceReferenceDate];
  // Test that we took at least the timeout period.
  XCTAssertTrue((dmp.Diff_Timeout <= (endTime - startTime)), @"Test that we took at least the timeout period.");
   // Test that we didn't take forever (be forgiving).
   // Theoretically this test could fail very occasionally if the
   // OS task swaps or locks up for a second at the wrong moment.
   // This will fail when running this as PPC code thru Rosetta on Intel.
  XCTAssertTrue(((dmp.Diff_Timeout * 2) > (endTime - startTime)), @"Test that we didn't take forever (be forgiving).");
  dmp.Diff_Timeout = 0;

  // Test the linemode speedup.
  // Must be long to pass the 200 character cutoff.
  a = @"1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n";
  b = @"abcdefghij\nabcdefghij\nabcdefghij\nabcdefghij\nabcdefghij\nabcdefghij\nabcdefghij\nabcdefghij\nabcdefghij\nabcdefghij\nabcdefghij\nabcdefghij\nabcdefghij\n";
  XCTAssertEqualObjects([dmp diff_mainOfOldString:a andNewString:b checkLines:YES], [dmp diff_mainOfOldString:a andNewString:b checkLines:NO], @"diff_main: Simple line-mode.");

  a = @"1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890";
  b = @"abcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghij";
  XCTAssertEqualObjects([dmp diff_mainOfOldString:a andNewString:b checkLines:YES], [dmp diff_mainOfOldString:a andNewString:b checkLines:NO], @"diff_main: Single line-mode.");

  a = @"1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n";
  b = @"abcdefghij\n1234567890\n1234567890\n1234567890\nabcdefghij\n1234567890\n1234567890\n1234567890\nabcdefghij\n1234567890\n1234567890\n1234567890\nabcdefghij\n";
  NSArray *texts_linemode = [self diff_rebuildtexts:[dmp diff_mainOfOldString:a andNewString:b checkLines:YES]];
  NSArray *texts_textmode = [self diff_rebuildtexts:[dmp diff_mainOfOldString:a andNewString:b checkLines:NO]];
  XCTAssertEqualObjects(texts_textmode, texts_linemode, @"diff_main: Overlap line-mode.");

  // CHANGEME: Test null inputs

  [dmp release];
}


#pragma mark Match Test Functions
//  MATCH TEST FUNCTIONS


- (void)test_match_alphabetTest {
  DiffMatchPatch *dmp = [DiffMatchPatch new];

  // Initialise the bitmasks for Bitap.
  NSMutableDictionary *bitmask = [NSMutableDictionary dictionary];

  [bitmask diff_setUnsignedIntegerValue:4 forUnicharKey:'a'];
  [bitmask diff_setUnsignedIntegerValue:2 forUnicharKey:'b'];
  [bitmask diff_setUnsignedIntegerValue:1 forUnicharKey:'c'];
  XCTAssertEqualObjects(bitmask, [dmp match_alphabet:@"abc"], @"match_alphabet: Unique.");

  [bitmask removeAllObjects];
  [bitmask diff_setUnsignedIntegerValue:37 forUnicharKey:'a'];
  [bitmask diff_setUnsignedIntegerValue:18 forUnicharKey:'b'];
  [bitmask diff_setUnsignedIntegerValue:8 forUnicharKey:'c'];
  XCTAssertEqualObjects(bitmask, [dmp match_alphabet:@"abcaba"], @"match_alphabet: Duplicates.");

  [dmp release];
}

- (void)test_match_bitapTest {
  DiffMatchPatch *dmp = [DiffMatchPatch new];

  // Bitap algorithm.
  dmp.Match_Distance = 100;
  dmp.Match_Threshold = 0.5f;
  XCTAssertEqual((NSUInteger)5, [dmp match_bitapOfText:@"abcdefghijk" andPattern:@"fgh" near:5], @"match_bitap: Exact match #1.");

  XCTAssertEqual((NSUInteger)5, [dmp match_bitapOfText:@"abcdefghijk" andPattern:@"fgh" near:0], @"match_bitap: Exact match #2.");

  XCTAssertEqual((NSUInteger)4, [dmp match_bitapOfText:@"abcdefghijk" andPattern:@"efxhi" near:0], @"match_bitap: Fuzzy match #1.");

  XCTAssertEqual((NSUInteger)2, [dmp match_bitapOfText:@"abcdefghijk" andPattern:@"cdefxyhijk" near:5], @"match_bitap: Fuzzy match #2.");

  XCTAssertEqual((NSUInteger)NSNotFound, [dmp match_bitapOfText:@"abcdefghijk" andPattern:@"bxy" near:1], @"match_bitap: Fuzzy match #3.");

  XCTAssertEqual((NSUInteger)2, [dmp match_bitapOfText:@"123456789xx0" andPattern:@"3456789x0" near:2], @"match_bitap: Overflow.");

  XCTAssertEqual((NSUInteger)0, [dmp match_bitapOfText:@"abcdef" andPattern:@"xxabc" near:4], @"match_bitap: Before start match.");

  XCTAssertEqual((NSUInteger)3, [dmp match_bitapOfText:@"abcdef" andPattern:@"defyy" near:4], @"match_bitap: Beyond end match.");

  XCTAssertEqual((NSUInteger)0, [dmp match_bitapOfText:@"abcdef" andPattern:@"xabcdefy" near:0], @"match_bitap: Oversized pattern.");

  dmp.Match_Threshold = 0.4f;
  XCTAssertEqual((NSUInteger)4, [dmp match_bitapOfText:@"abcdefghijk" andPattern:@"efxyhi" near:1], @"match_bitap: Threshold #1.");

  dmp.Match_Threshold = 0.3f;
  XCTAssertEqual((NSUInteger)NSNotFound, [dmp match_bitapOfText:@"abcdefghijk" andPattern:@"efxyhi" near:1], @"match_bitap: Threshold #2.");

  dmp.Match_Threshold = 0.0f;
  XCTAssertEqual((NSUInteger)1, [dmp match_bitapOfText:@"abcdefghijk" andPattern:@"bcdef" near:1], @"match_bitap: Threshold #3.");

  dmp.Match_Threshold = 0.5f;
  XCTAssertEqual((NSUInteger)0, [dmp match_bitapOfText:@"abcdexyzabcde" andPattern:@"abccde" near:3], @"match_bitap: Multiple select #1.");

  XCTAssertEqual((NSUInteger)8, [dmp match_bitapOfText:@"abcdexyzabcde" andPattern:@"abccde" near:5], @"match_bitap: Multiple select #2.");

  dmp.Match_Distance = 10;  // Strict location.
  XCTAssertEqual((NSUInteger)NSNotFound, [dmp match_bitapOfText:@"abcdefghijklmnopqrstuvwxyz" andPattern:@"abcdefg" near:24], @"match_bitap: Distance test #1.");

  XCTAssertEqual((NSUInteger)0, [dmp match_bitapOfText:@"abcdefghijklmnopqrstuvwxyz" andPattern:@"abcdxxefg" near:1], @"match_bitap: Distance test #2.");

  dmp.Match_Distance = 1000;  // Loose location.
  XCTAssertEqual((NSUInteger)0, [dmp match_bitapOfText:@"abcdefghijklmnopqrstuvwxyz" andPattern:@"abcdefg" near:24], @"match_bitap: Distance test #3.");

  [dmp release];
}

- (void)test_match_mainTest {
  DiffMatchPatch *dmp = [DiffMatchPatch new];

  // Full match.
  XCTAssertEqual((NSUInteger)0, [dmp match_mainForText:@"abcdef" pattern:@"abcdef" near:1000], @"match_main: Equality.");

  XCTAssertEqual((NSUInteger)NSNotFound, [dmp match_mainForText:@"" pattern:@"abcdef" near:1], @"match_main: Null text.");

  XCTAssertEqual((NSUInteger)3, [dmp match_mainForText:@"abcdef" pattern:@"" near:3], @"match_main: Null pattern.");

  XCTAssertEqual((NSUInteger)3, [dmp match_mainForText:@"abcdef" pattern:@"de" near:3], @"match_main: Exact match.");

  XCTAssertEqual((NSUInteger)3, [dmp match_mainForText:@"abcdef" pattern:@"defy" near:4], @"match_main: Beyond end match.");

  XCTAssertEqual((NSUInteger)0, [dmp match_mainForText:@"abcdef" pattern:@"abcdefy" near:0], @"match_main: Oversized pattern.");

  dmp.Match_Threshold = 0.7f;
  XCTAssertEqual((NSUInteger)4, [dmp match_mainForText:@"I am the very model of a modern major general." pattern:@" that berry " near:5], @"match_main: Complex match.");
  dmp.Match_Threshold = 0.5f;

  // CHANGEME: Test null inputs

  [dmp release];
}


#pragma mark Patch Test Functions
//  PATCH TEST FUNCTIONS


- (void)test_patch_patchObjTest {
  // Patch Object.
  Patch *p = [[Patch new] autorelease];
  p.start1 = 20;
  p.start2 = 21;
  p.length1 = 18;
  p.length2 = 17;
  p.diffs = [NSMutableArray arrayWithObjects:
      [Diff diffWithOperation:DIFF_EQUAL andText:@"jump"],
      [Diff diffWithOperation:DIFF_DELETE andText:@"s"],
      [Diff diffWithOperation:DIFF_INSERT andText:@"ed"],
      [Diff diffWithOperation:DIFF_EQUAL andText:@" over "],
      [Diff diffWithOperation:DIFF_DELETE andText:@"the"],
      [Diff diffWithOperation:DIFF_INSERT andText:@"a"],
      [Diff diffWithOperation:DIFF_EQUAL andText:@"\nlaz"], nil];
  NSString *strp = @"@@ -21,18 +22,17 @@\n jump\n-s\n+ed\n  over \n-the\n+a\n %0Alaz\n";
  XCTAssertEqualObjects(strp, [p description], @"Patch: description.");
}

- (void)test_patch_fromTextTest {
  DiffMatchPatch *dmp = [DiffMatchPatch new];

  XCTAssertTrue(((NSMutableArray *)[dmp patch_fromText:@"" error:NULL]).count == 0, @"patch_fromText: #0.");

  NSString *strp = @"@@ -21,18 +22,17 @@\n jump\n-s\n+ed\n  over \n-the\n+a\n %0Alaz\n";
  XCTAssertEqualObjects(strp, [[[dmp patch_fromText:strp error:NULL] objectAtIndex:0] description], @"patch_fromText: #1.");

  XCTAssertEqualObjects(@"@@ -1 +1 @@\n-a\n+b\n", [[[dmp patch_fromText:@"@@ -1 +1 @@\n-a\n+b\n" error:NULL] objectAtIndex:0] description], @"patch_fromText: #2.");

  XCTAssertEqualObjects(@"@@ -1,3 +0,0 @@\n-abc\n", [[[dmp patch_fromText:@"@@ -1,3 +0,0 @@\n-abc\n" error:NULL] objectAtIndex:0] description], @"patch_fromText: #3.");

  XCTAssertEqualObjects(@"@@ -0,0 +1,3 @@\n+abc\n", [[[dmp patch_fromText:@"@@ -0,0 +1,3 @@\n+abc\n" error:NULL] objectAtIndex:0] description], @"patch_fromText: #4.");

  // Generates error.
  NSError *error = nil;
  NSMutableArray *patches = [dmp patch_fromText:@"Bad\nPatch\n" error:&error];
  if (patches != nil || error == nil) {
    // Error expected.
    XCTFail(@"patch_fromText: #5.");
  }
  error = nil;

  [dmp release];
}

- (void)test_patch_toTextTest {
  DiffMatchPatch *dmp = [DiffMatchPatch new];

  NSString *strp = @"@@ -21,18 +22,17 @@\n jump\n-s\n+ed\n  over \n-the\n+a\n  laz\n";
  NSMutableArray *patches;
  patches = [dmp patch_fromText:strp error:NULL];
  XCTAssertEqualObjects(strp, [dmp patch_toText:patches], @"toText Test #1");

  strp = @"@@ -1,9 +1,9 @@\n-f\n+F\n oo+fooba\n@@ -7,9 +7,9 @@\n obar\n-,\n+.\n  tes\n";
  patches = [dmp patch_fromText:strp error:NULL];
  XCTAssertEqualObjects(strp, [dmp patch_toText:patches], @"toText Test #2");

  [dmp release];
}

- (void)test_patch_addContextTest {
  DiffMatchPatch *dmp = [DiffMatchPatch new];

  dmp.Patch_Margin = 4;
  Patch *p;
  p = [[dmp patch_fromText:@"@@ -21,4 +21,10 @@\n-jump\n+somersault\n" error:NULL] objectAtIndex:0];
  [dmp patch_addContextToPatch:p sourceText:@"The quick brown fox jumps over the lazy dog."];
  XCTAssertEqualObjects(@"@@ -17,12 +17,18 @@\n fox \n-jump\n+somersault\n s ov\n", [p description], @"patch_addContext: Simple case.");

  p = [[dmp patch_fromText:@"@@ -21,4 +21,10 @@\n-jump\n+somersault\n" error:NULL] objectAtIndex:0];
  [dmp patch_addContextToPatch:p sourceText:@"The quick brown fox jumps."];
  XCTAssertEqualObjects(@"@@ -17,10 +17,16 @@\n fox \n-jump\n+somersault\n s.\n", [p description], @"patch_addContext: Not enough trailing context.");

  p = [[dmp patch_fromText:@"@@ -3 +3,2 @@\n-e\n+at\n" error:NULL] objectAtIndex:0];
  [dmp patch_addContextToPatch:p sourceText:@"The quick brown fox jumps."];
  XCTAssertEqualObjects(@"@@ -1,7 +1,8 @@\n Th\n-e\n+at\n  qui\n", [p description], @"patch_addContext: Not enough leading context.");

  p = [[dmp patch_fromText:@"@@ -3 +3,2 @@\n-e\n+at\n" error:NULL] objectAtIndex:0];
  [dmp patch_addContextToPatch:p sourceText:@"The quick brown fox jumps.  The quick brown fox crashes."];
  XCTAssertEqualObjects(@"@@ -1,27 +1,28 @@\n Th\n-e\n+at\n  quick brown fox jumps. \n", [p description], @"patch_addContext: Ambiguity.");

  [dmp release];
}

- (void)test_patch_makeTest {
  DiffMatchPatch *dmp = [DiffMatchPatch new];

  NSMutableArray *patches;
  patches = [dmp patch_makeFromOldString:@"" andNewString:@""];
  XCTAssertEqualObjects(@"", [dmp patch_toText:patches], @"patch_make: Null case.");

  NSString *text1 = @"The quick brown fox jumps over the lazy dog.";
  NSString *text2 = @"That quick brown fox jumped over a lazy dog.";
  NSString *expectedPatch = @"@@ -1,8 +1,7 @@\n Th\n-at\n+e\n  qui\n@@ -21,17 +21,18 @@\n jump\n-ed\n+s\n  over \n-a\n+the\n  laz\n";
  // The second patch must be @"-21,17 +21,18", not @"-22,17 +21,18" due to rolling context.
  patches = [dmp patch_makeFromOldString:text2 andNewString:text1];
  XCTAssertEqualObjects(expectedPatch, [dmp patch_toText:patches], @"patch_make: Text2+Text1 inputs.");

  expectedPatch = @"@@ -1,11 +1,12 @@\n Th\n-e\n+at\n  quick b\n@@ -22,18 +22,17 @@\n jump\n-s\n+ed\n  over \n-the\n+a\n  laz\n";
  patches = [dmp patch_makeFromOldString:text1 andNewString:text2];
  XCTAssertEqualObjects(expectedPatch, [dmp patch_toText:patches], @"patch_make: Text1+Text2 inputs.");

  NSMutableArray *diffs = [dmp diff_mainOfOldString:text1 andNewString:text2 checkLines:NO];
  patches = [dmp patch_makeFromDiffs:diffs];
  XCTAssertEqualObjects(expectedPatch, [dmp patch_toText:patches], @"patch_make: Diff input.");

  patches = [dmp patch_makeFromOldString:text1 andDiffs:diffs];
  XCTAssertEqualObjects(expectedPatch, [dmp patch_toText:patches], @"patch_make: Text1+Diff inputs.");

  patches = [dmp patch_makeFromOldString:text1 newString:text2 diffs:diffs];
  XCTAssertEqualObjects(expectedPatch, [dmp patch_toText:patches], @"patch_make: Text1+Text2+Diff inputs (deprecated).");

  patches = [dmp patch_makeFromOldString:@"`1234567890-=[]\\;',./" andNewString:@"~!@#$%^&*()_+{}|:\"<>?"];
  XCTAssertEqualObjects(@"@@ -1,21 +1,21 @@\n-%601234567890-=%5B%5D%5C;',./\n+~!@#$%25%5E&*()_+%7B%7D%7C:%22%3C%3E?\n",
      [dmp patch_toText:patches],
      @"patch_toText: Character encoding.");

  diffs = [NSMutableArray arrayWithObjects:
      [Diff diffWithOperation:DIFF_DELETE andText:@"`1234567890-=[]\\;',./"],
      [Diff diffWithOperation:DIFF_INSERT andText:@"~!@#$%^&*()_+{}|:\"<>?"], nil];
  XCTAssertEqualObjects(diffs,
      ((Patch *)[[dmp patch_fromText:@"@@ -1,21 +1,21 @@\n-%601234567890-=%5B%5D%5C;',./\n+~!@#$%25%5E&*()_+%7B%7D%7C:%22%3C%3E?\n" error:NULL] objectAtIndex:0]).diffs,
      @"patch_fromText: Character decoding.");

  NSMutableString *text1Mutable = [NSMutableString string];
  for (int x = 0; x < 100; x++) {
    [text1Mutable appendString:@"abcdef"];
  }
  text1 = text1Mutable;
  text2 = [text1 stringByAppendingString:@"123"];
  // CHANGEME: Why does this implementation produce a different, more brief patch?
  //expectedPatch = @"@@ -573,28 +573,31 @@\n cdefabcdefabcdefabcdefabcdef\n+123\n";
  expectedPatch = @"@@ -597,4 +597,7 @@\n cdef\n+123\n";
  patches = [dmp patch_makeFromOldString:text1 andNewString:text2];
  XCTAssertEqualObjects(expectedPatch, [dmp patch_toText:patches], @"patch_make: Long string with repeats.");

  // CHANGEME: Test null inputs

  [dmp release];
}


- (void)test_patch_splitMaxTest {
  // Assumes that Match_MaxBits is 32.
  DiffMatchPatch *dmp = [DiffMatchPatch new];
  NSMutableArray *patches;

  patches = [dmp patch_makeFromOldString:@"abcdefghijklmnopqrstuvwxyz01234567890" andNewString:@"XabXcdXefXghXijXklXmnXopXqrXstXuvXwxXyzX01X23X45X67X89X0"];
  [dmp patch_splitMax:patches];
  XCTAssertEqualObjects(@"@@ -1,32 +1,46 @@\n+X\n ab\n+X\n cd\n+X\n ef\n+X\n gh\n+X\n ij\n+X\n kl\n+X\n mn\n+X\n op\n+X\n qr\n+X\n st\n+X\n uv\n+X\n wx\n+X\n yz\n+X\n 012345\n@@ -25,13 +39,18 @@\n zX01\n+X\n 23\n+X\n 45\n+X\n 67\n+X\n 89\n+X\n 0\n", [dmp patch_toText:patches], @"Assumes that Match_MaxBits is 32 #1");

  patches = [dmp patch_makeFromOldString:@"abcdef1234567890123456789012345678901234567890123456789012345678901234567890uvwxyz" andNewString:@"abcdefuvwxyz"];
  NSString *oldToText = [dmp patch_toText:patches];
  [dmp patch_splitMax:patches];
  XCTAssertEqualObjects(oldToText, [dmp patch_toText:patches], @"Assumes that Match_MaxBits is 32 #2");

  patches = [dmp patch_makeFromOldString:@"1234567890123456789012345678901234567890123456789012345678901234567890" andNewString:@"abc"];
  [dmp patch_splitMax:patches];
  XCTAssertEqualObjects(@"@@ -1,32 +1,4 @@\n-1234567890123456789012345678\n 9012\n@@ -29,32 +1,4 @@\n-9012345678901234567890123456\n 7890\n@@ -57,14 +1,3 @@\n-78901234567890\n+abc\n", [dmp patch_toText:patches], @"Assumes that Match_MaxBits is 32 #3");

  patches = [dmp patch_makeFromOldString:@"abcdefghij , h : 0 , t : 1 abcdefghij , h : 0 , t : 1 abcdefghij , h : 0 , t : 1" andNewString:@"abcdefghij , h : 1 , t : 1 abcdefghij , h : 1 , t : 1 abcdefghij , h : 0 , t : 1"];
  [dmp patch_splitMax:patches];
  XCTAssertEqualObjects(@"@@ -2,32 +2,32 @@\n bcdefghij , h : \n-0\n+1\n  , t : 1 abcdef\n@@ -29,32 +29,32 @@\n bcdefghij , h : \n-0\n+1\n  , t : 1 abcdef\n", [dmp patch_toText:patches], @"Assumes that Match_MaxBits is 32 #4");

  [dmp release];
}

- (void)test_patch_addPaddingTest {
  DiffMatchPatch *dmp = [DiffMatchPatch new];

  NSMutableArray *patches;
  patches = [dmp patch_makeFromOldString:@"" andNewString:@"test"];
  XCTAssertEqualObjects(@"@@ -0,0 +1,4 @@\n+test\n",
      [dmp patch_toText:patches],
      @"patch_addPadding: Both edges full.");
  [dmp patch_addPadding:patches];
  XCTAssertEqualObjects(@"@@ -1,8 +1,12 @@\n %01%02%03%04\n+test\n %01%02%03%04\n",
      [dmp patch_toText:patches],
      @"patch_addPadding: Both edges full.");

  patches = [dmp patch_makeFromOldString:@"XY" andNewString:@"XtestY"];
  XCTAssertEqualObjects(@"@@ -1,2 +1,6 @@\n X\n+test\n Y\n",
      [dmp patch_toText:patches],
      @"patch_addPadding: Both edges partial.");
  [dmp patch_addPadding:patches];
  XCTAssertEqualObjects(@"@@ -2,8 +2,12 @@\n %02%03%04X\n+test\n Y%01%02%03\n",
      [dmp patch_toText:patches],
      @"patch_addPadding: Both edges partial.");

  patches = [dmp patch_makeFromOldString:@"XXXXYYYY" andNewString:@"XXXXtestYYYY"];
  XCTAssertEqualObjects(@"@@ -1,8 +1,12 @@\n XXXX\n+test\n YYYY\n",
      [dmp patch_toText:patches],
      @"patch_addPadding: Both edges none.");
  [dmp patch_addPadding:patches];
  XCTAssertEqualObjects(@"@@ -5,8 +5,12 @@\n XXXX\n+test\n YYYY\n",
      [dmp patch_toText:patches],
      @"patch_addPadding: Both edges none.");

  [dmp release];
}

- (void)test_patch_applyTest {
  DiffMatchPatch *dmp = [DiffMatchPatch new];

  dmp.Match_Distance = 1000;
  dmp.Match_Threshold = 0.5f;
  dmp.Patch_DeleteThreshold = 0.5f;
  NSMutableArray *patches;
  patches = [dmp patch_makeFromOldString:@"" andNewString:@""];
  NSArray *results = [dmp patch_apply:patches toString:@"Hello world."];
  NSMutableArray *boolArray = [results objectAtIndex:1];
  NSString *resultStr = [NSString stringWithFormat:@"%@\t%lu", [results objectAtIndex:0], (unsigned long)boolArray.count];
  XCTAssertEqualObjects(@"Hello world.\t0", resultStr, @"patch_apply: Null case.");

  patches = [dmp patch_makeFromOldString:@"The quick brown fox jumps over the lazy dog." andNewString:@"That quick brown fox jumped over a lazy dog."];
  results = [dmp patch_apply:patches toString:@"The quick brown fox jumps over the lazy dog."];
  boolArray = [results objectAtIndex:1];
  resultStr = [NSString stringWithFormat:@"%@\t%@\t%@", [results objectAtIndex:0], stringForBOOL([boolArray objectAtIndex:0]), stringForBOOL([boolArray objectAtIndex:1])];
  XCTAssertEqualObjects(@"That quick brown fox jumped over a lazy dog.\ttrue\ttrue", resultStr, @"patch_apply: Exact match.");

  results = [dmp patch_apply:patches toString:@"The quick red rabbit jumps over the tired tiger."];
  boolArray = [results objectAtIndex:1];
  resultStr = [NSString stringWithFormat:@"%@\t%@\t%@", [results objectAtIndex:0], stringForBOOL([boolArray objectAtIndex:0]), stringForBOOL([boolArray objectAtIndex:1])];
  XCTAssertEqualObjects(@"That quick red rabbit jumped over a tired tiger.\ttrue\ttrue", resultStr, @"patch_apply: Partial match.");

  results = [dmp patch_apply:patches toString:@"I am the very model of a modern major general."];
  boolArray = [results objectAtIndex:1];
  resultStr = [NSString stringWithFormat:@"%@\t%@\t%@", [results objectAtIndex:0], stringForBOOL([boolArray objectAtIndex:0]), stringForBOOL([boolArray objectAtIndex:1])];
  XCTAssertEqualObjects(@"I am the very model of a modern major general.\tfalse\tfalse", resultStr, @"patch_apply: Failed match.");

  patches = [dmp patch_makeFromOldString:@"x1234567890123456789012345678901234567890123456789012345678901234567890y" andNewString:@"xabcy"];
  results = [dmp patch_apply:patches toString:@"x123456789012345678901234567890-----++++++++++-----123456789012345678901234567890y"];
  boolArray = [results objectAtIndex:1];
  resultStr = [NSString stringWithFormat:@"%@\t%@\t%@", [results objectAtIndex:0], stringForBOOL([boolArray objectAtIndex:0]), stringForBOOL([boolArray objectAtIndex:1])];
  XCTAssertEqualObjects(@"xabcy\ttrue\ttrue", resultStr, @"patch_apply: Big delete, small change.");

  patches = [dmp patch_makeFromOldString:@"x1234567890123456789012345678901234567890123456789012345678901234567890y" andNewString:@"xabcy"];
  results = [dmp patch_apply:patches toString:@"x12345678901234567890---------------++++++++++---------------12345678901234567890y"];
  boolArray = [results objectAtIndex:1];
  resultStr = [NSString stringWithFormat:@"%@\t%@\t%@", [results objectAtIndex:0], stringForBOOL([boolArray objectAtIndex:0]), stringForBOOL([boolArray objectAtIndex:1])];
  XCTAssertEqualObjects(@"xabc12345678901234567890---------------++++++++++---------------12345678901234567890y\tfalse\ttrue", resultStr, @"patch_apply: Big delete, big change 1.");

  dmp.Patch_DeleteThreshold = 0.6f;
  patches = [dmp patch_makeFromOldString:@"x1234567890123456789012345678901234567890123456789012345678901234567890y" andNewString:@"xabcy"];
  results = [dmp patch_apply:patches toString:@"x12345678901234567890---------------++++++++++---------------12345678901234567890y"];
  boolArray = [results objectAtIndex:1];
  resultStr = [NSString stringWithFormat:@"%@\t%@\t%@", [results objectAtIndex:0], stringForBOOL([boolArray objectAtIndex:0]), stringForBOOL([boolArray objectAtIndex:1])];
  XCTAssertEqualObjects(@"xabcy\ttrue\ttrue", resultStr, @"patch_apply: Big delete, big change 2.");
  dmp.Patch_DeleteThreshold = 0.5f;

  dmp.Match_Threshold = 0.0f;
  dmp.Match_Distance = 0;
  patches = [dmp patch_makeFromOldString:@"abcdefghijklmnopqrstuvwxyz--------------------1234567890" andNewString:@"abcXXXXXXXXXXdefghijklmnopqrstuvwxyz--------------------1234567YYYYYYYYYY890"];
  results = [dmp patch_apply:patches toString:@"ABCDEFGHIJKLMNOPQRSTUVWXYZ--------------------1234567890"];
  boolArray = [results objectAtIndex:1];
  resultStr = [NSString stringWithFormat:@"%@\t%@\t%@", [results objectAtIndex:0], stringForBOOL([boolArray objectAtIndex:0]), stringForBOOL([boolArray objectAtIndex:1])];
  XCTAssertEqualObjects(@"ABCDEFGHIJKLMNOPQRSTUVWXYZ--------------------1234567YYYYYYYYYY890\tfalse\ttrue", resultStr, @"patch_apply: Compensate for failed patch.");
  dmp.Match_Threshold = 0.5f;
  dmp.Match_Distance = 1000;

  patches = [dmp patch_makeFromOldString:@"" andNewString:@"test"];
  NSString *patchStr = [dmp patch_toText:patches];
  [dmp patch_apply:patches toString:@""];
  XCTAssertEqualObjects(patchStr, [dmp patch_toText:patches], @"patch_apply: No side effects.");

  patches = [dmp patch_makeFromOldString:@"The quick brown fox jumps over the lazy dog." andNewString:@"Woof"];
  patchStr = [dmp patch_toText:patches];
  [dmp patch_apply:patches toString:@"The quick brown fox jumps over the lazy dog."];
  XCTAssertEqualObjects(patchStr, [dmp patch_toText:patches], @"patch_apply: No side effects with major delete.");

  patches = [dmp patch_makeFromOldString:@"" andNewString:@"test"];
  results = [dmp patch_apply:patches toString:@""];
  boolArray = [results objectAtIndex:1];
  resultStr = [NSString stringWithFormat:@"%@\t%@", [results objectAtIndex:0], stringForBOOL([boolArray objectAtIndex:0])];
  XCTAssertEqualObjects(@"test\ttrue", resultStr, @"patch_apply: Edge exact match.");

  patches = [dmp patch_makeFromOldString:@"XY" andNewString:@"XtestY"];
  results = [dmp patch_apply:patches toString:@"XY"];
  boolArray = [results objectAtIndex:1];
  resultStr = [NSString stringWithFormat:@"%@\t%@", [results objectAtIndex:0], stringForBOOL([boolArray objectAtIndex:0])];
  XCTAssertEqualObjects(@"XtestY\ttrue", resultStr, @"patch_apply: Near edge exact match.");

  patches = [dmp patch_makeFromOldString:@"y" andNewString:@"y123"];
  results = [dmp patch_apply:patches toString:@"x"];
  boolArray = [results objectAtIndex:1];
  resultStr = [NSString stringWithFormat:@"%@\t%@", [results objectAtIndex:0], stringForBOOL([boolArray objectAtIndex:0])];
  XCTAssertEqualObjects(@"x123\ttrue", resultStr, @"patch_apply: Edge partial match.");

  [dmp release];
}


#pragma mark Test Utility Functions
//  TEST UTILITY FUNCTIONS


- (NSArray *)diff_rebuildtexts:(NSMutableArray *)diffs;
{
  NSArray *text = [NSMutableArray arrayWithObjects:[NSMutableString string], [NSMutableString string], nil];
  for (Diff *myDiff in diffs) {
    if (myDiff.operation != DIFF_INSERT) {
      [[text objectAtIndex:0] appendString:myDiff.text];
    }
    if (myDiff.operation != DIFF_DELETE) {
      [[text objectAtIndex:1] appendString:myDiff.text];
    }
  }
  return text;
}

@end
