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

public class Speedtest {
  /*TODO Replace with BenchmarkDotNet
  public static void Main(string[] args) {
    string text1 = System.IO.File.ReadAllText("Speedtest1.txt");
    string text2 = System.IO.File.ReadAllText("Speedtest2.txt");

    diff_match_patch dmp = new diff_match_patch();
    dmp.Diff_Timeout = 0;

    // Execute one reverse diff as a warmup.
    dmp.diff_main(text2, text1);
    GC.Collect();
    GC.WaitForPendingFinalizers();

    DateTime ms_start = DateTime.Now;
    dmp.diff_main(text1, text2);
    DateTime ms_end = DateTime.Now;

    Console.WriteLine("Elapsed time: " + (ms_end - ms_start));
  }*/
}
