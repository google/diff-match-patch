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
using System.IO;
using BenchmarkDotNet.Attributes;

namespace Google.DiffMatchPatch.Tests.Performance
{
    public class DiffMainTest
    {
        private string _text1;
        private string _text2;

        [GlobalSetup]
        public void Init()
        {
            _text1 = File.ReadAllText("Speedtest1.txt");
            _text2 = File.ReadAllText("Speedtest2.txt");
        }

        [Benchmark]
        public List<Diff> DiffMain()
        {
            var dmp = new diff_match_patch {Diff_Timeout = 0};

            return dmp.diff_main(_text1, _text2);
        }
    }
}
