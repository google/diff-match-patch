/*
 * Diff Match and Patch
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

using System;
using System.Collections.Generic;
using System.Text;
using Xunit;

namespace Google.DiffMatchPatch.Tests
{
    public class DiffTests : diff_match_patch
    {
        [Fact]
        public void CommonPrefixTest()
        {
            // Detect any common suffix.
            Assert.Equal(0, diff_commonPrefix("abc", "xyz"));
            Assert.Equal(4, diff_commonPrefix("1234abcdef", "1234xyz"));
            Assert.Equal(4, diff_commonPrefix("1234", "1234xyz"));
        }

        [Fact]
        public void CommonSuffixTest()
        {
            // Detect any common suffix.
            Assert.Equal(0, diff_commonSuffix("abc", "xyz"));
            Assert.Equal(4, diff_commonSuffix("abcdef1234", "xyz1234"));
            Assert.Equal(4, diff_commonSuffix("1234", "xyz1234"));
        }

        [Fact]
        public void CommonOverlapTest()
        {
            // Detect any suffix/prefix overlap.
            Assert.Equal(0, diff_commonOverlap("", "abcd"));
            Assert.Equal(3, diff_commonOverlap("abc", "abcd"));
            Assert.Equal(0, diff_commonOverlap("123456", "abcd"));
            Assert.Equal(3, diff_commonOverlap("123456xxx", "xxxabcd"));

            // Some overly clever languages (C#) may treat ligatures as equal to their
            // component letters.  E.g. U+FB01 == 'fi'
            Assert.Equal(0, diff_commonOverlap("fi", "\ufb01i"));
        }

        [Fact]
        public void HalfmatchTest()
        {
            Diff_Timeout = 1;
            Assert.Null(diff_halfMatch("1234567890", "abcdef"));
            Assert.Null(diff_halfMatch("12345", "23"));

            Assert.Equal(new[] {"12", "90", "a", "z", "345678"}, diff_halfMatch("1234567890", "a345678z"));
            Assert.Equal(new[] {"a", "z", "12", "90", "345678"}, diff_halfMatch("a345678z", "1234567890"));
            Assert.Equal(new[] {"abc", "z", "1234", "0", "56789"}, diff_halfMatch("abc56789z", "1234567890"));
            Assert.Equal(new[] {"a", "xyz", "1", "7890", "23456"}, diff_halfMatch("a23456xyz", "1234567890"));
            Assert.Equal(new[] {"12123", "123121", "a", "z", "1234123451234"},
                diff_halfMatch("121231234123451234123121", "a1234123451234z"));
            Assert.Equal(new[] {"", "-=-=-=-=-=", "x", "", "x-=-=-=-=-=-=-="},
                diff_halfMatch("x-=-=-=-=-=-=-=-=-=-=-=-=", "xx-=-=-=-=-=-=-="));
            Assert.Equal(new[] {"-=-=-=-=-=", "", "", "y", "-=-=-=-=-=-=-=y"},
                diff_halfMatch("-=-=-=-=-=-=-=-=-=-=-=-=y", "-=-=-=-=-=-=-=yy"));

            // Optimal diff would be -q+x=H-i+e=lloHe+Hu=llo-Hew+y not -qHillo+x=HelloHe-w+Hulloy
            Assert.Equal(new[] {"qHillo", "w", "x", "Hulloy", "HelloHe"},
                diff_halfMatch("qHilloHelloHew", "xHelloHeHulloy"));

            Diff_Timeout = 0;
            Assert.Null(diff_halfMatch("qHilloHelloHew", "xHelloHeHulloy"));
        }

        [Fact]
        public void LinesToCharsTest()
        {
            // Convert lines down to characters.
            var tmpVector = new List<string> {"", "alpha\n", "beta\n"};
            var result = diff_linesToChars("alpha\nbeta\nalpha\n", "beta\nalpha\nbeta\n");

            Assert.Equal("\u0001\u0002\u0001", (string) result[0]);
            Assert.Equal("\u0002\u0001\u0002", (string) result[1]);
            Assert.Equal(tmpVector, (List<string>) result[2]);

            tmpVector.Clear();
            tmpVector.Add("");
            tmpVector.Add("alpha\r\n");
            tmpVector.Add("beta\r\n");
            tmpVector.Add("\r\n");
            result = diff_linesToChars("", "alpha\r\nbeta\r\n\r\n\r\n");

            Assert.Equal("", (string) result[0]);
            Assert.Equal("\u0001\u0002\u0003\u0003", (string) result[1]);
            Assert.Equal(tmpVector, (List<string>) result[2]);

            tmpVector.Clear();
            tmpVector.Add("");
            tmpVector.Add("a");
            tmpVector.Add("b");
            result = diff_linesToChars("a", "b");

            Assert.Equal("\u0001", (string) result[0]);
            Assert.Equal("\u0002", (string) result[1]);
            Assert.Equal(tmpVector, (List<string>) result[2]);

            // More than 256 to reveal any 8-bit limitations.
            const int n = 300;
            tmpVector.Clear();
            var lineList = new StringBuilder();
            var charList = new StringBuilder();
            for (var i = 1; i < n + 1; i++)
            {
                tmpVector.Add(i + "\n");
                lineList.Append(i + "\n");
                charList.Append(Convert.ToChar(i));
            }

            Assert.Equal(n, tmpVector.Count);
            var lines = lineList.ToString();
            var chars = charList.ToString();
            Assert.Equal(n, chars.Length);
            tmpVector.Insert(0, "");
            result = diff_linesToChars(lines, "");
            Assert.Equal(chars, (string) result[0]);
            Assert.Equal("", (string) result[1]);
            Assert.Equal(tmpVector, (List<string>) result[2]);
        }

        [Fact]
        public void CharsToLinesTest()
        {
            // First check that Diff equality works.
            Assert.True(new Diff(Operation.EQUAL, "a").Equals(new Diff(Operation.EQUAL, "a")));

            Assert.Equal(new Diff(Operation.EQUAL, "a"), new Diff(Operation.EQUAL, "a"));

            // Convert chars up to lines.
            var diffs = new List<Diff>
            {
                new Diff(Operation.EQUAL, "\u0001\u0002\u0001"),
                new Diff(Operation.INSERT, "\u0002\u0001\u0002")
            };
            var tmpVector = new List<string> {"", "alpha\n", "beta\n"};
            diff_charsToLines(diffs, tmpVector);
            Assert.Equal(new List<Diff>
            {
                new Diff(Operation.EQUAL, "alpha\nbeta\nalpha\n"),
                new Diff(Operation.INSERT, "beta\nalpha\nbeta\n")
            }, diffs);

            // More than 256 to reveal any 8-bit limitations.
            const int n = 300;
            tmpVector.Clear();
            var lineList = new StringBuilder();
            var charList = new StringBuilder();
            for (var i = 1; i < n + 1; i++)
            {
                tmpVector.Add(i + "\n");
                lineList.Append(i + "\n");
                charList.Append(Convert.ToChar(i));
            }

            Assert.Equal(n, tmpVector.Count);
            var lines = lineList.ToString();
            var chars = charList.ToString();
            Assert.Equal(n, chars.Length);
            tmpVector.Insert(0, "");
            diffs = new List<Diff> {new Diff(Operation.DELETE, chars)};
            diff_charsToLines(diffs, tmpVector);
            Assert.Equal(new List<Diff>
                {new Diff(Operation.DELETE, lines)}, diffs);

            // More than 65536 to verify any 16-bit limitation.
            lineList = new StringBuilder();
            for (var i = 0; i < 66000; i++)
            {
                lineList.Append(i + "\n");
            }

            chars = lineList.ToString();
            var result = diff_linesToChars(chars, "");
            diffs = new List<Diff> {new Diff(Operation.INSERT, (string) result[0])};
            diff_charsToLines(diffs, (List<string>) result[2]);
            Assert.Equal(chars, diffs[0].text);
        }

        [Fact]
        public void CleanupMergeTest()
        {
            // Cleanup a messy diff.
            // Null case.
            var diffs = new List<Diff>();
            Assert.Equal(new List<Diff>(), diffs);

            diffs = new List<Diff>
            {
                new Diff(Operation.EQUAL, "a"),
                new Diff(Operation.DELETE, "b"),
                new Diff(Operation.INSERT, "c")
            };
            diff_cleanupMerge(diffs);
            Assert.Equal(
                new List<Diff>
                {
                    new Diff(Operation.EQUAL, "a"),
                    new Diff(Operation.DELETE, "b"),
                    new Diff(Operation.INSERT, "c")
                }, diffs);

            diffs = new List<Diff>
            {
                new Diff(Operation.EQUAL, "a"),
                new Diff(Operation.EQUAL, "b"),
                new Diff(Operation.EQUAL, "c")
            };
            diff_cleanupMerge(diffs);
            Assert.Equal(new List<Diff> {new Diff(Operation.EQUAL, "abc")}, diffs);

            diffs = new List<Diff>
            {
                new Diff(Operation.DELETE, "a"),
                new Diff(Operation.DELETE, "b"),
                new Diff(Operation.DELETE, "c")
            };
            diff_cleanupMerge(diffs);
            Assert.Equal(new List<Diff> {new Diff(Operation.DELETE, "abc")}, diffs);

            diffs = new List<Diff>
            {
                new Diff(Operation.INSERT, "a"),
                new Diff(Operation.INSERT, "b"),
                new Diff(Operation.INSERT, "c")
            };
            diff_cleanupMerge(diffs);
            Assert.Equal(new List<Diff> {new Diff(Operation.INSERT, "abc")}, diffs);

            diffs = new List<Diff>
            {
                new Diff(Operation.DELETE, "a"),
                new Diff(Operation.INSERT, "b"),
                new Diff(Operation.DELETE, "c"),
                new Diff(Operation.INSERT, "d"),
                new Diff(Operation.EQUAL, "e"),
                new Diff(Operation.EQUAL, "f")
            };
            diff_cleanupMerge(diffs);
            Assert.Equal(
                new List<Diff>
                {
                    new Diff(Operation.DELETE, "ac"),
                    new Diff(Operation.INSERT, "bd"),
                    new Diff(Operation.EQUAL, "ef")
                }, diffs);

            diffs = new List<Diff>
            {
                new Diff(Operation.DELETE, "a"),
                new Diff(Operation.INSERT, "abc"),
                new Diff(Operation.DELETE, "dc")
            };
            diff_cleanupMerge(diffs);
            Assert.Equal(
                new List<Diff>
                {
                    new Diff(Operation.EQUAL, "a"),
                    new Diff(Operation.DELETE, "d"),
                    new Diff(Operation.INSERT, "b"),
                    new Diff(Operation.EQUAL, "c")
                }, diffs);

            diffs = new List<Diff>
            {
                new Diff(Operation.EQUAL, "x"),
                new Diff(Operation.DELETE, "a"),
                new Diff(Operation.INSERT, "abc"),
                new Diff(Operation.DELETE, "dc"),
                new Diff(Operation.EQUAL, "y")
            };
            diff_cleanupMerge(diffs);
            Assert.Equal(
                new List<Diff>
                {
                    new Diff(Operation.EQUAL, "xa"),
                    new Diff(Operation.DELETE, "d"),
                    new Diff(Operation.INSERT, "b"),
                    new Diff(Operation.EQUAL, "cy")
                }, diffs);

            diffs = new List<Diff>
            {
                new Diff(Operation.EQUAL, "a"),
                new Diff(Operation.INSERT, "ba"),
                new Diff(Operation.EQUAL, "c")
            };
            diff_cleanupMerge(diffs);
            Assert.Equal(new List<Diff> {new Diff(Operation.INSERT, "ab"), new Diff(Operation.EQUAL, "ac")}, diffs);

            diffs = new List<Diff>
            {
                new Diff(Operation.EQUAL, "c"),
                new Diff(Operation.INSERT, "ab"),
                new Diff(Operation.EQUAL, "a")
            };
            diff_cleanupMerge(diffs);
            Assert.Equal(new List<Diff> {new Diff(Operation.EQUAL, "ca"), new Diff(Operation.INSERT, "ba")}, diffs);

            diffs = new List<Diff>
            {
                new Diff(Operation.EQUAL, "a"),
                new Diff(Operation.DELETE, "b"),
                new Diff(Operation.EQUAL, "c"),
                new Diff(Operation.DELETE, "ac"),
                new Diff(Operation.EQUAL, "x")
            };
            diff_cleanupMerge(diffs);
            Assert.Equal(new List<Diff> {new Diff(Operation.DELETE, "abc"), new Diff(Operation.EQUAL, "acx")}, diffs);

            diffs = new List<Diff>
            {
                new Diff(Operation.EQUAL, "x"),
                new Diff(Operation.DELETE, "ca"),
                new Diff(Operation.EQUAL, "c"),
                new Diff(Operation.DELETE, "b"),
                new Diff(Operation.EQUAL, "a")
            };
            diff_cleanupMerge(diffs);
            Assert.Equal(new List<Diff> {new Diff(Operation.EQUAL, "xca"), new Diff(Operation.DELETE, "cba")}, diffs);

            diffs = new List<Diff>
            {
                new Diff(Operation.DELETE, "b"),
                new Diff(Operation.INSERT, "ab"),
                new Diff(Operation.EQUAL, "c")
            };
            diff_cleanupMerge(diffs);
            Assert.Equal(new List<Diff> {new Diff(Operation.INSERT, "a"), new Diff(Operation.EQUAL, "bc")}, diffs);

            diffs = new List<Diff>
            {
                new Diff(Operation.EQUAL, ""),
                new Diff(Operation.INSERT, "a"),
                new Diff(Operation.EQUAL, "b")
            };
            diff_cleanupMerge(diffs);
            Assert.Equal(new List<Diff> {new Diff(Operation.INSERT, "a"), new Diff(Operation.EQUAL, "b")}, diffs);
        }

        [Fact]
        public void CleanupSemanticLosslessTest()
        {
            // Slide diffs to match logical boundaries.
            var diffs = new List<Diff>();
            diff_cleanupSemanticLossless(diffs);
            Assert.Equal(new List<Diff>(), diffs);

            diffs = new List<Diff>
            {
                new Diff(Operation.EQUAL, "AAA\r\n\r\nBBB"),
                new Diff(Operation.INSERT, "\r\nDDD\r\n\r\nBBB"),
                new Diff(Operation.EQUAL, "\r\nEEE")
            };
            diff_cleanupSemanticLossless(diffs);
            Assert.Equal(new List<Diff>
            {
                new Diff(Operation.EQUAL, "AAA\r\n\r\n"),
                new Diff(Operation.INSERT, "BBB\r\nDDD\r\n\r\n"),
                new Diff(Operation.EQUAL, "BBB\r\nEEE")
            }, diffs);

            diffs = new List<Diff>
            {
                new Diff(Operation.EQUAL, "AAA\r\nBBB"),
                new Diff(Operation.INSERT, " DDD\r\nBBB"),
                new Diff(Operation.EQUAL, " EEE")
            };
            diff_cleanupSemanticLossless(diffs);
            Assert.Equal(new List<Diff>
            {
                new Diff(Operation.EQUAL, "AAA\r\n"),
                new Diff(Operation.INSERT, "BBB DDD\r\n"),
                new Diff(Operation.EQUAL, "BBB EEE")
            }, diffs);

            diffs = new List<Diff>
            {
                new Diff(Operation.EQUAL, "The c"),
                new Diff(Operation.INSERT, "ow and the c"),
                new Diff(Operation.EQUAL, "at.")
            };
            diff_cleanupSemanticLossless(diffs);
            Assert.Equal(new List<Diff>
            {
                new Diff(Operation.EQUAL, "The "),
                new Diff(Operation.INSERT, "cow and the "),
                new Diff(Operation.EQUAL, "cat.")
            }, diffs);

            diffs = new List<Diff>
            {
                new Diff(Operation.EQUAL, "The-c"),
                new Diff(Operation.INSERT, "ow-and-the-c"),
                new Diff(Operation.EQUAL, "at.")
            };
            diff_cleanupSemanticLossless(diffs);
            Assert.Equal(new List<Diff>
            {
                new Diff(Operation.EQUAL, "The-"),
                new Diff(Operation.INSERT, "cow-and-the-"),
                new Diff(Operation.EQUAL, "cat.")
            }, diffs);

            diffs = new List<Diff>
            {
                new Diff(Operation.EQUAL, "a"),
                new Diff(Operation.DELETE, "a"),
                new Diff(Operation.EQUAL, "ax")
            };
            diff_cleanupSemanticLossless(diffs);
            Assert.Equal(new List<Diff>
            {
                new Diff(Operation.DELETE, "a"),
                new Diff(Operation.EQUAL, "aax")
            }, diffs);

            diffs = new List<Diff>
            {
                new Diff(Operation.EQUAL, "xa"),
                new Diff(Operation.DELETE, "a"),
                new Diff(Operation.EQUAL, "a")
            };
            diff_cleanupSemanticLossless(diffs);
            Assert.Equal(new List<Diff>
            {
                new Diff(Operation.EQUAL, "xaa"),
                new Diff(Operation.DELETE, "a")
            }, diffs);

            diffs = new List<Diff>
            {
                new Diff(Operation.EQUAL, "The xxx. The "),
                new Diff(Operation.INSERT, "zzz. The "),
                new Diff(Operation.EQUAL, "yyy.")
            };
            diff_cleanupSemanticLossless(diffs);
            Assert.Equal(new List<Diff>
            {
                new Diff(Operation.EQUAL, "The xxx."),
                new Diff(Operation.INSERT, " The zzz."),
                new Diff(Operation.EQUAL, " The yyy.")
            }, diffs);
        }

        [Fact]
        public void CleanupSemanticTest()
        {
            // Cleanup semantically trivial equalities.
            // Null case.
            var diffs = new List<Diff>();
            diff_cleanupSemantic(diffs);
            Assert.Equal(new List<Diff>(), diffs);

            diffs = new List<Diff>
            {
                new Diff(Operation.DELETE, "ab"),
                new Diff(Operation.INSERT, "cd"),
                new Diff(Operation.EQUAL, "12"),
                new Diff(Operation.DELETE, "e")
            };
            diff_cleanupSemantic(diffs);
            Assert.Equal(new List<Diff>
            {
                new Diff(Operation.DELETE, "ab"),
                new Diff(Operation.INSERT, "cd"),
                new Diff(Operation.EQUAL, "12"),
                new Diff(Operation.DELETE, "e")
            }, diffs);

            diffs = new List<Diff>
            {
                new Diff(Operation.DELETE, "abc"),
                new Diff(Operation.INSERT, "ABC"),
                new Diff(Operation.EQUAL, "1234"),
                new Diff(Operation.DELETE, "wxyz")
            };
            diff_cleanupSemantic(diffs);
            Assert.Equal(new List<Diff>
            {
                new Diff(Operation.DELETE, "abc"),
                new Diff(Operation.INSERT, "ABC"),
                new Diff(Operation.EQUAL, "1234"),
                new Diff(Operation.DELETE, "wxyz")
            }, diffs);

            diffs = new List<Diff>
            {
                new Diff(Operation.DELETE, "a"),
                new Diff(Operation.EQUAL, "b"),
                new Diff(Operation.DELETE, "c")
            };
            diff_cleanupSemantic(diffs);
            Assert.Equal(new List<Diff>
            {
                new Diff(Operation.DELETE, "abc"),
                new Diff(Operation.INSERT, "b")
            }, diffs);

            diffs = new List<Diff>
            {
                new Diff(Operation.DELETE, "ab"),
                new Diff(Operation.EQUAL, "cd"),
                new Diff(Operation.DELETE, "e"),
                new Diff(Operation.EQUAL, "f"),
                new Diff(Operation.INSERT, "g")
            };
            diff_cleanupSemantic(diffs);
            Assert.Equal(new List<Diff>
            {
                new Diff(Operation.DELETE, "abcdef"),
                new Diff(Operation.INSERT, "cdfg")
            }, diffs);

            diffs = new List<Diff>
            {
                new Diff(Operation.INSERT, "1"),
                new Diff(Operation.EQUAL, "A"),
                new Diff(Operation.DELETE, "B"),
                new Diff(Operation.INSERT, "2"),
                new Diff(Operation.EQUAL, "_"),
                new Diff(Operation.INSERT, "1"),
                new Diff(Operation.EQUAL, "A"),
                new Diff(Operation.DELETE, "B"),
                new Diff(Operation.INSERT, "2")
            };
            diff_cleanupSemantic(diffs);
            Assert.Equal(new List<Diff>
            {
                new Diff(Operation.DELETE, "AB_AB"),
                new Diff(Operation.INSERT, "1A2_1A2")
            }, diffs);

            diffs = new List<Diff>
            {
                new Diff(Operation.EQUAL, "The c"),
                new Diff(Operation.DELETE, "ow and the c"),
                new Diff(Operation.EQUAL, "at.")
            };
            diff_cleanupSemantic(diffs);
            Assert.Equal(new List<Diff>
            {
                new Diff(Operation.EQUAL, "The "),
                new Diff(Operation.DELETE, "cow and the "),
                new Diff(Operation.EQUAL, "cat.")
            }, diffs);

            diffs = new List<Diff>
            {
                new Diff(Operation.DELETE, "abcxx"),
                new Diff(Operation.INSERT, "xxdef")
            };
            diff_cleanupSemantic(diffs);
            Assert.Equal(new List<Diff>
            {
                new Diff(Operation.DELETE, "abcxx"),
                new Diff(Operation.INSERT, "xxdef")
            }, diffs);

            diffs = new List<Diff>
            {
                new Diff(Operation.DELETE, "abcxxx"),
                new Diff(Operation.INSERT, "xxxdef")
            };
            diff_cleanupSemantic(diffs);
            Assert.Equal(new List<Diff>
            {
                new Diff(Operation.DELETE, "abc"),
                new Diff(Operation.EQUAL, "xxx"),
                new Diff(Operation.INSERT, "def")
            }, diffs);

            diffs = new List<Diff>
            {
                new Diff(Operation.DELETE, "xxxabc"),
                new Diff(Operation.INSERT, "defxxx")
            };
            diff_cleanupSemantic(diffs);
            Assert.Equal(new List<Diff>
            {
                new Diff(Operation.INSERT, "def"),
                new Diff(Operation.EQUAL, "xxx"),
                new Diff(Operation.DELETE, "abc")
            }, diffs);

            diffs = new List<Diff>
            {
                new Diff(Operation.DELETE, "abcd1212"),
                new Diff(Operation.INSERT, "1212efghi"),
                new Diff(Operation.EQUAL, "----"),
                new Diff(Operation.DELETE, "A3"),
                new Diff(Operation.INSERT, "3BC")
            };
            diff_cleanupSemantic(diffs);
            Assert.Equal(new List<Diff>
            {
                new Diff(Operation.DELETE, "abcd"),
                new Diff(Operation.EQUAL, "1212"),
                new Diff(Operation.INSERT, "efghi"),
                new Diff(Operation.EQUAL, "----"),
                new Diff(Operation.DELETE, "A"),
                new Diff(Operation.EQUAL, "3"),
                new Diff(Operation.INSERT, "BC")
            }, diffs);
        }

        [Fact]
        public void CleanupEfficiencyTest()
        {
            // Cleanup operationally trivial equalities.
            Diff_EditCost = 4;
            var diffs = new List<Diff>();
            diff_cleanupEfficiency(diffs);
            Assert.Equal(new List<Diff>(), diffs);

            diffs = new List<Diff>
            {
                new Diff(Operation.DELETE, "ab"),
                new Diff(Operation.INSERT, "12"),
                new Diff(Operation.EQUAL, "wxyz"),
                new Diff(Operation.DELETE, "cd"),
                new Diff(Operation.INSERT, "34")
            };
            diff_cleanupEfficiency(diffs);
            Assert.Equal(new List<Diff>
            {
                new Diff(Operation.DELETE, "ab"),
                new Diff(Operation.INSERT, "12"),
                new Diff(Operation.EQUAL, "wxyz"),
                new Diff(Operation.DELETE, "cd"),
                new Diff(Operation.INSERT, "34")
            }, diffs);

            diffs = new List<Diff>
            {
                new Diff(Operation.DELETE, "ab"),
                new Diff(Operation.INSERT, "12"),
                new Diff(Operation.EQUAL, "xyz"),
                new Diff(Operation.DELETE, "cd"),
                new Diff(Operation.INSERT, "34")
            };
            diff_cleanupEfficiency(diffs);
            Assert.Equal(new List<Diff>
            {
                new Diff(Operation.DELETE, "abxyzcd"),
                new Diff(Operation.INSERT, "12xyz34")
            }, diffs);

            diffs = new List<Diff>
            {
                new Diff(Operation.INSERT, "12"),
                new Diff(Operation.EQUAL, "x"),
                new Diff(Operation.DELETE, "cd"),
                new Diff(Operation.INSERT, "34")
            };
            diff_cleanupEfficiency(diffs);
            Assert.Equal(new List<Diff>
            {
                new Diff(Operation.DELETE, "xcd"),
                new Diff(Operation.INSERT, "12x34")
            }, diffs);

            diffs = new List<Diff>
            {
                new Diff(Operation.DELETE, "ab"),
                new Diff(Operation.INSERT, "12"),
                new Diff(Operation.EQUAL, "xy"),
                new Diff(Operation.INSERT, "34"),
                new Diff(Operation.EQUAL, "z"),
                new Diff(Operation.DELETE, "cd"),
                new Diff(Operation.INSERT, "56")
            };
            diff_cleanupEfficiency(diffs);
            Assert.Equal(new List<Diff>
            {
                new Diff(Operation.DELETE, "abxyzcd"),
                new Diff(Operation.INSERT, "12xy34z56")
            }, diffs);

            Diff_EditCost = 5;
            diffs = new List<Diff>
            {
                new Diff(Operation.DELETE, "ab"),
                new Diff(Operation.INSERT, "12"),
                new Diff(Operation.EQUAL, "wxyz"),
                new Diff(Operation.DELETE, "cd"),
                new Diff(Operation.INSERT, "34")
            };
            diff_cleanupEfficiency(diffs);
            Assert.Equal(new List<Diff>
            {
                new Diff(Operation.DELETE, "abwxyzcd"),
                new Diff(Operation.INSERT, "12wxyz34")
            }, diffs);
            Diff_EditCost = 4;
        }

        [Fact]
        public void PrettyHtmlTest()
        {
            // Pretty print.
            var diffs = new List<Diff>
            {
                new Diff(Operation.EQUAL, "a\n"),
                new Diff(Operation.DELETE, "<B>b</B>"),
                new Diff(Operation.INSERT, "c&d")
            };
            Assert.Equal(
                "<span>a&para;<br></span><del style=\"background:#ffe6e6;\">&lt;B&gt;b&lt;/B&gt;</del><ins style=\"background:#e6ffe6;\">c&amp;d</ins>",
                diff_prettyHtml(diffs));
        }

        [Fact]
        public void TextTest()
        {
            // Compute the source and destination texts.
            var diffs = new List<Diff>
            {
                new Diff(Operation.EQUAL, "jump"),
                new Diff(Operation.DELETE, "s"),
                new Diff(Operation.INSERT, "ed"),
                new Diff(Operation.EQUAL, " over "),
                new Diff(Operation.DELETE, "the"),
                new Diff(Operation.INSERT, "a"),
                new Diff(Operation.EQUAL, " lazy")
            };
            Assert.Equal("jumps over the lazy", diff_text1(diffs));

            Assert.Equal("jumped over a lazy", diff_text2(diffs));
        }

        [Fact]
        public void DeltaTest()
        {
            // Convert a diff into delta string.
            var diffs = new List<Diff>
            {
                new Diff(Operation.EQUAL, "jump"),
                new Diff(Operation.DELETE, "s"),
                new Diff(Operation.INSERT, "ed"),
                new Diff(Operation.EQUAL, " over "),
                new Diff(Operation.DELETE, "the"),
                new Diff(Operation.INSERT, "a"),
                new Diff(Operation.EQUAL, " lazy"),
                new Diff(Operation.INSERT, "old dog")
            };
            var text1 = diff_text1(diffs);
            Assert.Equal("jumps over the lazy", text1);

            var delta = diff_toDelta(diffs);
            Assert.Equal("=4\t-1\t+ed\t=6\t-3\t+a\t=5\t+old dog", delta);

            // Convert delta string into a diff.
            Assert.Equal(diffs, diff_fromDelta(text1, delta));

            // Generates error (19 < 20).
            Assert.Throws<ArgumentException>(() => diff_fromDelta(text1 + "x", delta));

            // Generates error (19 > 18).
            Assert.Throws<ArgumentException>(() => diff_fromDelta(text1.Substring(1), delta));

            // Generates error (%c3%xy invalid Unicode).
            //TODO:rolshevsky:Assert.Throws<ArgumentException>(() => diff_fromDelta("", "+%c3%xy"));

            // Test deltas with special characters.
            const char zero = (char) 0;
            const char one = (char) 1;
            const char two = (char) 2;
            diffs = new List<Diff>
            {
                new Diff(Operation.EQUAL, "\u0680 " + zero + " \t %"),
                new Diff(Operation.DELETE, "\u0681 " + one + " \n ^"),
                new Diff(Operation.INSERT, "\u0682 " + two + " \\ |")
            };
            text1 = diff_text1(diffs);
            Assert.Equal("\u0680 " + zero + " \t %\u0681 " + one + " \n ^", text1);

            delta = diff_toDelta(diffs);
            // Lowercase, due to UrlEncode uses lower.
            Assert.Equal("=7\t-7\t+%da%82 %02 %5c %7c", delta);

            Assert.Equal(diffs, diff_fromDelta(text1, delta));

            // Verify pool of unchanged characters.
            diffs = new List<Diff>
            {
                new Diff(Operation.INSERT, "A-Z a-z 0-9 - _ . ! ~ * ' ( ) ; / ? : @ & = + $ , # ")
            };
            var text2 = diff_text2(diffs);
            Assert.Equal("A-Z a-z 0-9 - _ . ! ~ * \' ( ) ; / ? : @ & = + $ , # ", text2);

            delta = diff_toDelta(diffs);
            Assert.Equal("+A-Z a-z 0-9 - _ . ! ~ * \' ( ) ; / ? : @ & = + $ , # ", delta);

            // Convert delta string into a diff.
            Assert.Equal(diffs, diff_fromDelta("", delta));

            // 160 kb string.
            var a = "abcdefghij";
            for (var i = 0; i < 14; i++)
            {
                a += a;
            }

            diffs = new List<Diff> {new Diff(Operation.INSERT, a)};
            delta = diff_toDelta(diffs);
            Assert.Equal("+" + a, delta);

            // Convert delta string into a diff.
            Assert.Equal(diffs, diff_fromDelta("", delta));
        }

        [Fact]
        public void InvalidDeltaTest()
        {
        }

        [Fact]
        public void X_IndexTest()
        {
            // Translate a location in text1 to text2.
            var diffs = new List<Diff>
            {
                new Diff(Operation.DELETE, "a"),
                new Diff(Operation.INSERT, "1234"),
                new Diff(Operation.EQUAL, "xyz")
            };
            Assert.Equal(5, diff_xIndex(diffs, 2));

            diffs = new List<Diff>
            {
                new Diff(Operation.EQUAL, "a"),
                new Diff(Operation.DELETE, "1234"),
                new Diff(Operation.EQUAL, "xyz")
            };
            Assert.Equal(1, diff_xIndex(diffs, 3));
        }

        [Fact]
        public void LevenshteinTest()
        {
            var diffs = new List<Diff>
            {
                new Diff(Operation.DELETE, "abc"),
                new Diff(Operation.INSERT, "1234"),
                new Diff(Operation.EQUAL, "xyz")
            };
            Assert.Equal(4, diff_levenshtein(diffs));

            diffs = new List<Diff>
            {
                new Diff(Operation.EQUAL, "xyz"),
                new Diff(Operation.DELETE, "abc"),
                new Diff(Operation.INSERT, "1234")
            };
            Assert.Equal(4, diff_levenshtein(diffs));

            diffs = new List<Diff>
            {
                new Diff(Operation.DELETE, "abc"),
                new Diff(Operation.EQUAL, "xyz"),
                new Diff(Operation.INSERT, "1234")
            };
            Assert.Equal(7, diff_levenshtein(diffs));
        }

        [Fact]
        public void BisectTest()
        {
            // Normal.
            const string a = "cat";
            const string b = "map";
            // Since the resulting diff hasn't been normalized, it would be ok if
            // the insertion and deletion pairs are swapped.
            // If the order changes, tweak this test as required.
            var diffs = new List<Diff>
            {
                new Diff(Operation.DELETE, "c"),
                new Diff(Operation.INSERT, "m"),
                new Diff(Operation.EQUAL, "a"),
                new Diff(Operation.DELETE, "t"),
                new Diff(Operation.INSERT, "p")
            };
            Assert.Equal(diffs, diff_bisect(a, b, DateTime.MaxValue));

            // Timeout.
            diffs = new List<Diff> {new Diff(Operation.DELETE, "cat"), new Diff(Operation.INSERT, "map")};
            Assert.Equal(diffs, diff_bisect(a, b, DateTime.MinValue));
        }

        [Fact]
        public void MainTest()
        {
            // Perform a trivial diff.
            var diffs = new List<Diff>();
            Assert.Equal(diffs, diff_main("", "", false));

            diffs = new List<Diff> {new Diff(Operation.EQUAL, "abc")};
            Assert.Equal(diffs, diff_main("abc", "abc", false));

            diffs = new List<Diff>
            {
                new Diff(Operation.EQUAL, "ab"),
                new Diff(Operation.INSERT, "123"),
                new Diff(Operation.EQUAL, "c")
            };
            Assert.Equal(diffs, diff_main("abc", "ab123c", false));

            diffs = new List<Diff>
            {
                new Diff(Operation.EQUAL, "a"),
                new Diff(Operation.DELETE, "123"),
                new Diff(Operation.EQUAL, "bc")
            };
            Assert.Equal(diffs, diff_main("a123bc", "abc", false));

            diffs = new List<Diff>
            {
                new Diff(Operation.EQUAL, "a"),
                new Diff(Operation.INSERT, "123"),
                new Diff(Operation.EQUAL, "b"),
                new Diff(Operation.INSERT, "456"),
                new Diff(Operation.EQUAL, "c")
            };
            Assert.Equal(diffs, diff_main("abc", "a123b456c", false));

            diffs = new List<Diff>
            {
                new Diff(Operation.EQUAL, "a"),
                new Diff(Operation.DELETE, "123"),
                new Diff(Operation.EQUAL, "b"),
                new Diff(Operation.DELETE, "456"),
                new Diff(Operation.EQUAL, "c")
            };
            Assert.Equal(diffs, diff_main("a123b456c", "abc", false));

            // Perform a real diff.
            // Switch off the timeout.
            Diff_Timeout = 0;
            diffs = new List<Diff> {new Diff(Operation.DELETE, "a"), new Diff(Operation.INSERT, "b")};
            Assert.Equal(diffs, diff_main("a", "b", false));

            diffs = new List<Diff>
            {
                new Diff(Operation.DELETE, "Apple"),
                new Diff(Operation.INSERT, "Banana"),
                new Diff(Operation.EQUAL, "s are a"),
                new Diff(Operation.INSERT, "lso"),
                new Diff(Operation.EQUAL, " fruit.")
            };
            Assert.Equal(diffs, diff_main("Apples are a fruit.", "Bananas are also fruit.", false));

            diffs = new List<Diff>
            {
                new Diff(Operation.DELETE, "a"),
                new Diff(Operation.INSERT, "\u0680"),
                new Diff(Operation.EQUAL, "x"),
                new Diff(Operation.DELETE, "\t"),
                new Diff(Operation.INSERT, new string(new[] {(char) 0}))
            };
            Assert.Equal(diffs, diff_main("ax\t", "\u0680x" + (char) 0, false));

            diffs = new List<Diff>
            {
                new Diff(Operation.DELETE, "1"),
                new Diff(Operation.EQUAL, "a"),
                new Diff(Operation.DELETE, "y"),
                new Diff(Operation.EQUAL, "b"),
                new Diff(Operation.DELETE, "2"),
                new Diff(Operation.INSERT, "xab")
            };
            Assert.Equal(diffs, diff_main("1ayb2", "abxab", false));

            diffs = new List<Diff>
            {
                new Diff(Operation.INSERT, "xaxcx"),
                new Diff(Operation.EQUAL, "abc"),
                new Diff(Operation.DELETE, "y")
            };
            Assert.Equal(diffs, diff_main("abcy", "xaxcxabc", false));

            diffs = new List<Diff>
            {
                new Diff(Operation.DELETE, "ABCD"),
                new Diff(Operation.EQUAL, "a"),
                new Diff(Operation.DELETE, "="),
                new Diff(Operation.INSERT, "-"),
                new Diff(Operation.EQUAL, "bcd"),
                new Diff(Operation.DELETE, "="),
                new Diff(Operation.INSERT, "-"),
                new Diff(Operation.EQUAL, "efghijklmnopqrs"),
                new Diff(Operation.DELETE, "EFGHIJKLMNOefg")
            };
            Assert.Equal(diffs, diff_main("ABCDa=bcd=efghijklmnopqrsEFGHIJKLMNOefg", "a-bcd-efghijklmnopqrs", false));

            diffs = new List<Diff>
            {
                new Diff(Operation.INSERT, " "),
                new Diff(Operation.EQUAL, "a"),
                new Diff(Operation.INSERT, "nd"),
                new Diff(Operation.EQUAL, " [[Pennsylvania]]"),
                new Diff(Operation.DELETE, " and [[New")
            };
            Assert.Equal(diffs, diff_main("a [[Pennsylvania]] and [[New", " and [[Pennsylvania]]", false));

            Diff_Timeout = 0.1f; // 100ms
            var a =
                "`Twas brillig, and the slithy toves\nDid gyre and gimble in the wabe:\nAll mimsy were the borogoves,\nAnd the mome raths outgrabe.\n";
            var b =
                "I am the very model of a modern major general,\nI've information vegetable, animal, and mineral,\nI know the kings of England, and I quote the fights historical,\nFrom Marathon to Waterloo, in order categorical.\n";
            // Increase the text lengths by 1024 times to ensure a timeout.
            for (var i = 0; i < 10; i++)
            {
                a += a;
                b += b;
            }

            var startTime = DateTime.Now;
            diff_main(a, b);
            var endTime = DateTime.Now;
            // Test that we took at least the timeout period.
            Assert.True(new TimeSpan(((long) (Diff_Timeout * 1000)) * 10000) <= endTime - startTime);
            // Test that we didn't take forever (be forgiving).
            // Theoretically this test could fail very occasionally if the
            // OS task swaps or locks up for a second at the wrong moment.
            Assert.True(new TimeSpan(((long) (Diff_Timeout * 1000)) * 10000 * 2) > endTime - startTime);
            Diff_Timeout = 0;

            // Test the linemode speedup.
            // Must be long to pass the 100 char cutoff.
            a =
                "1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n";
            b =
                "abcdefghij\nabcdefghij\nabcdefghij\nabcdefghij\nabcdefghij\nabcdefghij\nabcdefghij\nabcdefghij\nabcdefghij\nabcdefghij\nabcdefghij\nabcdefghij\nabcdefghij\n";
            Assert.Equal(diff_main(a, b, true), diff_main(a, b, false));

            a =
                "1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890";
            b =
                "abcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghij";
            Assert.Equal(diff_main(a, b, true), diff_main(a, b, false));

            a =
                "1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n1234567890\n";
            b =
                "abcdefghij\n1234567890\n1234567890\n1234567890\nabcdefghij\n1234567890\n1234567890\n1234567890\nabcdefghij\n1234567890\n1234567890\n1234567890\nabcdefghij\n";
            var textsLinemode = RebuildTexts(diff_main(a, b, true));
            var textsTextmode = RebuildTexts(diff_main(a, b, false));
            Assert.Equal(textsTextmode, textsLinemode);

            // Test null inputs -- not needed because nulls can't be passed in C#.
        }

        private static string[] RebuildTexts(IEnumerable<Diff> diffs)
        {
            string[] text = {"", ""};
            foreach (var myDiff in diffs)
            {
                if (myDiff.operation != Operation.INSERT)
                {
                    text[0] += myDiff.text;
                }

                if (myDiff.operation != Operation.DELETE)
                {
                    text[1] += myDiff.text;
                }
            }

            return text;
        }
    }
}
