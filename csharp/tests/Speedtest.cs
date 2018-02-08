// Copyright 2010 Google Inc.
// All Right Reserved.

/*
 * To compile with Mono:
 *   mcs Speedtest.cs ../DiffMatchPatch.cs
 * To run with Mono:
 *   mono Speedtest.exe
*/

using DiffMatchPatch;
using System;
using System.Collections.Generic;

public class Speedtest {
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
  }
}
