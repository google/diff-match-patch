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

using System.Collections.Generic;
using Xunit;

namespace Google.DiffMatchPatch.Tests
{
    public class MatchTests : diff_match_patch
    {
        [Fact]
        public void AlphabetTest()
        {
            // Initialise the bitmasks for Bitap.
            var bitmask = new Dictionary<char, int> {{'a', 4}, {'b', 2}, {'c', 1}};
            Assert.Equal(bitmask, match_alphabet("abc"));

            bitmask.Clear();
            bitmask.Add('a', 37);
            bitmask.Add('b', 18);
            bitmask.Add('c', 8);
            Assert.Equal(bitmask, match_alphabet("abcaba"));
        }

        [Fact]
        public void BitapTest()
        {
            // Bitap algorithm.
            Match_Distance = 100;
            Match_Threshold = 0.5f;
            Assert.Equal(5, match_bitap("abcdefghijk", "fgh", 5));

            Assert.Equal(5, match_bitap("abcdefghijk", "fgh", 0));

            Assert.Equal(4, match_bitap("abcdefghijk", "efxhi", 0));

            Assert.Equal(2, match_bitap("abcdefghijk", "cdefxyhijk", 5));

            Assert.Equal(-1, match_bitap("abcdefghijk", "bxy", 1));

            Assert.Equal(2, match_bitap("123456789xx0", "3456789x0", 2));

            Assert.Equal(0, match_bitap("abcdef", "xxabc", 4));

            Assert.Equal(3, match_bitap("abcdef", "defyy", 4));

            Assert.Equal(0, match_bitap("abcdef", "xabcdefy", 0));

            Match_Threshold = 0.4f;
            Assert.Equal(4, match_bitap("abcdefghijk", "efxyhi", 1));

            Match_Threshold = 0.3f;
            Assert.Equal(-1, match_bitap("abcdefghijk", "efxyhi", 1));

            Match_Threshold = 0.0f;
            Assert.Equal(1, match_bitap("abcdefghijk", "bcdef", 1));

            Match_Threshold = 0.5f;
            Assert.Equal(0, match_bitap("abcdexyzabcde", "abccde", 3));

            Assert.Equal(8, match_bitap("abcdexyzabcde", "abccde", 5));

            Match_Distance = 10; // Strict location.
            Assert.Equal(-1, match_bitap("abcdefghijklmnopqrstuvwxyz", "abcdefg", 24));

            Assert.Equal(0, match_bitap("abcdefghijklmnopqrstuvwxyz", "abcdxxefg", 1));

            Match_Distance = 1000; // Loose location.
            Assert.Equal(0, match_bitap("abcdefghijklmnopqrstuvwxyz", "abcdefg", 24));
        }

        [Fact]
        public void MainTest()
        {
            // Full match.
            Assert.Equal(0, match_main("abcdef", "abcdef", 1000));

            Assert.Equal(-1, match_main("", "abcdef", 1));

            Assert.Equal(3, match_main("abcdef", "", 3));

            Assert.Equal(3, match_main("abcdef", "de", 3));

            Assert.Equal(3, match_main("abcdef", "defy", 4));

            Assert.Equal(0, match_main("abcdef", "abcdefy", 0));

            Match_Threshold = 0.7f;
            Assert.Equal(4, match_main("I am the very model of a modern major general.", " that berry ", 5));
            Match_Threshold = 0.5f;

            // Test null inputs -- not needed because nulls can't be passed in C#.
        }
    }
}
