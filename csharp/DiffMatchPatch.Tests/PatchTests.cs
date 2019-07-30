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
using Xunit;

namespace Google.DiffMatchPatch.Tests
{
    public class PatchTests : diff_match_patch
    {
        [Fact]
        public void PatchObjTest()
        {
            // Patch Object.
            var p = new Patch
            {
                start1 = 20,
                start2 = 21,
                length1 = 18,
                length2 = 17,
                diffs = new List<Diff>
                {
                    new Diff(Operation.EQUAL, "jump"),
                    new Diff(Operation.DELETE, "s"),
                    new Diff(Operation.INSERT, "ed"),
                    new Diff(Operation.EQUAL, " over "),
                    new Diff(Operation.DELETE, "the"),
                    new Diff(Operation.INSERT, "a"),
                    new Diff(Operation.EQUAL, "\nlaz")
                }
            };
            const string strp = "@@ -21,18 +22,17 @@\n jump\n-s\n+ed\n  over \n-the\n+a\n %0alaz\n";
            Assert.Equal(strp, p.ToString());
        }

        [Fact]
        public void FromTextTest()
        {
            Assert.True(patch_fromText("").Count == 0);

            const string strp = "@@ -21,18 +22,17 @@\n jump\n-s\n+ed\n  over \n-the\n+a\n %0alaz\n";

            Assert.Equal(strp, patch_fromText(strp)[0].ToString());
            Assert.Equal("@@ -1 +1 @@\n-a\n+b\n", patch_fromText("@@ -1 +1 @@\n-a\n+b\n")[0].ToString());
            Assert.Equal("@@ -1,3 +0,0 @@\n-abc\n", patch_fromText("@@ -1,3 +0,0 @@\n-abc\n")[0].ToString());
            Assert.Equal("@@ -0,0 +1,3 @@\n+abc\n", patch_fromText("@@ -0,0 +1,3 @@\n+abc\n")[0].ToString());

            Assert.Throws<ArgumentException>(() => patch_fromText("Bad\nPatch\n"));
        }

        [Fact]
        public void ToTextTest()
        {
            var strp = "@@ -21,18 +22,17 @@\n jump\n-s\n+ed\n  over \n-the\n+a\n  laz\n";
            var patches = patch_fromText(strp);
            var result = patch_toText(patches);
            Assert.Equal(strp, result);

            strp = "@@ -1,9 +1,9 @@\n-f\n+F\n oo+fooba\n@@ -7,9 +7,9 @@\n obar\n-,\n+.\n  tes\n";
            patches = patch_fromText(strp);
            result = patch_toText(patches);
            Assert.Equal(strp, result);
        }

        [Fact]
        public void AddContextTest()
        {
            Patch_Margin = 4;
            var p = patch_fromText("@@ -21,4 +21,10 @@\n-jump\n+somersault\n")[0];
            patch_addContext(p, "The quick brown fox jumps over the lazy dog.");
            Assert.Equal("@@ -17,12 +17,18 @@\n fox \n-jump\n+somersault\n s ov\n", p.ToString());

            p = patch_fromText("@@ -21,4 +21,10 @@\n-jump\n+somersault\n")[0];
            patch_addContext(p, "The quick brown fox jumps.");
            Assert.Equal("@@ -17,10 +17,16 @@\n fox \n-jump\n+somersault\n s.\n", p.ToString());

            p = patch_fromText("@@ -3 +3,2 @@\n-e\n+at\n")[0];
            patch_addContext(p, "The quick brown fox jumps.");
            Assert.Equal("@@ -1,7 +1,8 @@\n Th\n-e\n+at\n  qui\n", p.ToString());

            p = patch_fromText("@@ -3 +3,2 @@\n-e\n+at\n")[0];
            patch_addContext(p, "The quick brown fox jumps.  The quick brown fox crashes.");
            Assert.Equal("@@ -1,27 +1,28 @@\n Th\n-e\n+at\n  quick brown fox jumps. \n", p.ToString());
        }

        [Fact]
        public void MakeTest()
        {
            var patches = patch_make("", "");
            Assert.Equal("", patch_toText(patches));

            var text1 = "The quick brown fox jumps over the lazy dog.";
            var text2 = "That quick brown fox jumped over a lazy dog.";
            var expectedPatch =
                "@@ -1,8 +1,7 @@\n Th\n-at\n+e\n  qui\n@@ -21,17 +21,18 @@\n jump\n-ed\n+s\n  over \n-a\n+the\n  laz\n";
            // The second patch must be "-21,17 +21,18", not "-22,17 +21,18" due to rolling context.
            patches = patch_make(text2, text1);
            Assert.Equal(expectedPatch, patch_toText(patches));

            expectedPatch =
                "@@ -1,11 +1,12 @@\n Th\n-e\n+at\n  quick b\n@@ -22,18 +22,17 @@\n jump\n-s\n+ed\n  over \n-the\n+a\n  laz\n";
            patches = patch_make(text1, text2);
            Assert.Equal(expectedPatch, patch_toText(patches));

            var diffs = diff_main(text1, text2, false);
            patches = patch_make(diffs);
            Assert.Equal(expectedPatch, patch_toText(patches));

            patches = patch_make(text1, diffs);
            Assert.Equal(expectedPatch, patch_toText(patches));

            patches = patch_make(text1, text2, diffs);
            Assert.Equal(expectedPatch, patch_toText(patches));

            patches = patch_make("`1234567890-=[]\\;',./", "~!@#$%^&*()_+{}|:\"<>?");
            Assert.Equal("@@ -1,21 +1,21 @@\n-%601234567890-=%5b%5d%5c;',./\n+~!@#$%25%5e&*()_+%7b%7d%7c:%22%3c%3e?\n",
                patch_toText(patches));

            diffs = new List<Diff>
            {
                new Diff(Operation.DELETE, "`1234567890-=[]\\;',./"),
                new Diff(Operation.INSERT, "~!@#$%^&*()_+{}|:\"<>?")
            };
            Assert.Equal(diffs,
                patch_fromText(
                        "@@ -1,21 +1,21 @@\n-%601234567890-=%5B%5D%5C;',./\n+~!@#$%25%5E&*()_+%7B%7D%7C:%22%3C%3E?\n")[
                        0]
                    .diffs);

            text1 = "";
            for (var x = 0; x < 100; x++)
            {
                text1 += "abcdef";
            }

            text2 = text1 + "123";
            expectedPatch = "@@ -573,28 +573,31 @@\n cdefabcdefabcdefabcdefabcdef\n+123\n";
            patches = patch_make(text1, text2);
            Assert.Equal(expectedPatch, patch_toText(patches));

            // Test null inputs -- not needed because nulls can't be passed in C#.
        }

        [Fact]
        public void SplitMaxTest()
        {
            // Assumes that Match_MaxBits is 32.

            var patches = patch_make("abcdefghijklmnopqrstuvwxyz01234567890",
                "XabXcdXefXghXijXklXmnXopXqrXstXuvXwxXyzX01X23X45X67X89X0");
            patch_splitMax(patches);
            Assert.Equal(
                "@@ -1,32 +1,46 @@\n+X\n ab\n+X\n cd\n+X\n ef\n+X\n gh\n+X\n ij\n+X\n kl\n+X\n mn\n+X\n op\n+X\n qr\n+X\n st\n+X\n uv\n+X\n wx\n+X\n yz\n+X\n 012345\n@@ -25,13 +39,18 @@\n zX01\n+X\n 23\n+X\n 45\n+X\n 67\n+X\n 89\n+X\n 0\n",
                patch_toText(patches));

            patches = patch_make("abcdef1234567890123456789012345678901234567890123456789012345678901234567890uvwxyz",
                "abcdefuvwxyz");
            var oldToText = patch_toText(patches);
            patch_splitMax(patches);
            Assert.Equal(oldToText, patch_toText(patches));

            patches = patch_make("1234567890123456789012345678901234567890123456789012345678901234567890", "abc");
            patch_splitMax(patches);
            Assert.Equal(
                "@@ -1,32 +1,4 @@\n-1234567890123456789012345678\n 9012\n@@ -29,32 +1,4 @@\n-9012345678901234567890123456\n 7890\n@@ -57,14 +1,3 @@\n-78901234567890\n+abc\n",
                patch_toText(patches));

            patches = patch_make("abcdefghij , h : 0 , t : 1 abcdefghij , h : 0 , t : 1 abcdefghij , h : 0 , t : 1",
                "abcdefghij , h : 1 , t : 1 abcdefghij , h : 1 , t : 1 abcdefghij , h : 0 , t : 1");
            patch_splitMax(patches);
            Assert.Equal(
                "@@ -2,32 +2,32 @@\n bcdefghij , h : \n-0\n+1\n  , t : 1 abcdef\n@@ -29,32 +29,32 @@\n bcdefghij , h : \n-0\n+1\n  , t : 1 abcdef\n",
                patch_toText(patches));
        }

        [Fact]
        public void AddPaddingTest()
        {
            var patches = patch_make("", "test");
            Assert.Equal("@@ -0,0 +1,4 @@\n+test\n",
                patch_toText(patches));
            patch_addPadding(patches);
            Assert.Equal("@@ -1,8 +1,12 @@\n %01%02%03%04\n+test\n %01%02%03%04\n",
                patch_toText(patches));

            patches = patch_make("XY", "XtestY");
            Assert.Equal("@@ -1,2 +1,6 @@\n X\n+test\n Y\n",
                patch_toText(patches));
            patch_addPadding(patches);
            Assert.Equal("@@ -2,8 +2,12 @@\n %02%03%04X\n+test\n Y%01%02%03\n",
                patch_toText(patches));

            patches = patch_make("XXXXYYYY", "XXXXtestYYYY");
            Assert.Equal("@@ -1,8 +1,12 @@\n XXXX\n+test\n YYYY\n",
                patch_toText(patches));
            patch_addPadding(patches);
            Assert.Equal("@@ -5,8 +5,12 @@\n XXXX\n+test\n YYYY\n",
                patch_toText(patches));
        }

        [Fact]
        public void ApplyTest()
        {
            Match_Distance = 1000;
            Match_Threshold = 0.5f;
            Patch_DeleteThreshold = 0.5f;
            var patches = patch_make("", "");
            var results = patch_apply(patches, "Hello world.");
            var boolArray = (bool[]) results[1];
            var resultStr = results[0] + "\t" + boolArray.Length;
            Assert.Equal("Hello world.\t0", resultStr);

            patches = patch_make("The quick brown fox jumps over the lazy dog.",
                "That quick brown fox jumped over a lazy dog.");
            results = patch_apply(patches, "The quick brown fox jumps over the lazy dog.");
            boolArray = (bool[]) results[1];
            resultStr = results[0] + "\t" + boolArray[0] + "\t" + boolArray[1];
            Assert.Equal("That quick brown fox jumped over a lazy dog.\tTrue\tTrue", resultStr);

            results = patch_apply(patches, "The quick red rabbit jumps over the tired tiger.");
            boolArray = (bool[]) results[1];
            resultStr = results[0] + "\t" + boolArray[0] + "\t" + boolArray[1];
            Assert.Equal("That quick red rabbit jumped over a tired tiger.\tTrue\tTrue", resultStr);

            results = patch_apply(patches, "I am the very model of a modern major general.");
            boolArray = (bool[]) results[1];
            resultStr = results[0] + "\t" + boolArray[0] + "\t" + boolArray[1];
            Assert.Equal("I am the very model of a modern major general.\tFalse\tFalse", resultStr);

            patches = patch_make("x1234567890123456789012345678901234567890123456789012345678901234567890y", "xabcy");
            results = patch_apply(patches,
                "x123456789012345678901234567890-----++++++++++-----123456789012345678901234567890y");
            boolArray = (bool[]) results[1];
            resultStr = results[0] + "\t" + boolArray[0] + "\t" + boolArray[1];
            Assert.Equal("xabcy\tTrue\tTrue", resultStr);

            patches = patch_make("x1234567890123456789012345678901234567890123456789012345678901234567890y", "xabcy");
            results = patch_apply(patches,
                "x12345678901234567890---------------++++++++++---------------12345678901234567890y");
            boolArray = (bool[]) results[1];
            resultStr = results[0] + "\t" + boolArray[0] + "\t" + boolArray[1];
            Assert.Equal(
                "xabc12345678901234567890---------------++++++++++---------------12345678901234567890y\tFalse\tTrue",
                resultStr);

            Patch_DeleteThreshold = 0.6f;
            patches = patch_make("x1234567890123456789012345678901234567890123456789012345678901234567890y", "xabcy");
            results = patch_apply(patches,
                "x12345678901234567890---------------++++++++++---------------12345678901234567890y");
            boolArray = (bool[]) results[1];
            resultStr = results[0] + "\t" + boolArray[0] + "\t" + boolArray[1];
            Assert.Equal("xabcy\tTrue\tTrue", resultStr);
            Patch_DeleteThreshold = 0.5f;

            Match_Threshold = 0.0f;
            Match_Distance = 0;
            patches = patch_make("abcdefghijklmnopqrstuvwxyz--------------------1234567890",
                "abcXXXXXXXXXXdefghijklmnopqrstuvwxyz--------------------1234567YYYYYYYYYY890");
            results = patch_apply(patches, "ABCDEFGHIJKLMNOPQRSTUVWXYZ--------------------1234567890");
            boolArray = (bool[]) results[1];
            resultStr = results[0] + "\t" + boolArray[0] + "\t" + boolArray[1];
            Assert.Equal("ABCDEFGHIJKLMNOPQRSTUVWXYZ--------------------1234567YYYYYYYYYY890\tFalse\tTrue", resultStr);
            Match_Threshold = 0.5f;
            Match_Distance = 1000;

            patches = patch_make("", "test");
            var patchStr = patch_toText(patches);
            patch_apply(patches, "");
            Assert.Equal(patchStr, patch_toText(patches));

            patches = patch_make("The quick brown fox jumps over the lazy dog.", "Woof");
            patchStr = patch_toText(patches);
            patch_apply(patches, "The quick brown fox jumps over the lazy dog.");
            Assert.Equal(patchStr, patch_toText(patches));

            patches = patch_make("", "test");
            results = patch_apply(patches, "");
            boolArray = (bool[]) results[1];
            resultStr = results[0] + "\t" + boolArray[0];
            Assert.Equal("test\tTrue", resultStr);

            patches = patch_make("XY", "XtestY");
            results = patch_apply(patches, "XY");
            boolArray = (bool[]) results[1];
            resultStr = results[0] + "\t" + boolArray[0];
            Assert.Equal("XtestY\tTrue", resultStr);

            patches = patch_make("y", "y123");
            results = patch_apply(patches, "x");
            boolArray = (bool[]) results[1];
            resultStr = results[0] + "\t" + boolArray[0];
            Assert.Equal("x123\tTrue", resultStr);
        }
    }
}
