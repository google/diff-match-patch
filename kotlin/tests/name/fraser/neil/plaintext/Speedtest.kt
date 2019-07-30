// Copyright 2010 Google Inc. All Rights Reserved.

/**
 * Diff Speed Test
 *
 * Compile from diff-match-patch/java with:
 * javac -d classes src/name/fraser/neil/plaintext/diff_match_patch.java tests/name/fraser/neil/plaintext/Speedtest.java
 * Execute with:
 * java -classpath classes name/fraser/neil/plaintext/Speedtest
 *
 * @author fraser@google.com (Neil Fraser)
 */

package name.fraser.neil.plaintext

import java.io.BufferedReader
import java.io.FileReader
import java.io.IOException

object Speedtest {

  @Throws(IOException::class)
  @JvmStatic
  fun main(args: Array<String>) {
    val text1 = readFile("tests/name/fraser/neil/plaintext/Speedtest1.txt")
    val text2 = readFile("tests/name/fraser/neil/plaintext/Speedtest2.txt")

    val dmp = diff_match_patch()
    dmp.Diff_Timeout = 0

    // Execute one reverse diff as a warmup.
    dmp.diff_main(text2, text1, false)

    val start_time = System.nanoTime()
    dmp.diff_main(text1, text2, false)
    val end_time = System.nanoTime()
    System.out.printf("Elapsed time: %f\n", (end_time - start_time) / 1000000000.0)
  }

  @Throws(IOException::class)
  private fun readFile(filename: String): String {
    // Read a file from disk and return the text contents.
    val sb = StringBuilder()
    val input = FileReader(filename)
    val bufRead = BufferedReader(input)
    try {
      var line: String? = bufRead.readLine()
      while (line != null) {
        sb.append(line).append('\n')
        line = bufRead.readLine()
      }
    } finally {
      bufRead.close()
      input.close()
    }
    return sb.toString()
  }
}
