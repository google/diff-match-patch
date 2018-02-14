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

package name.fraser.neil.plaintext;

import name.fraser.neil.plaintext.diff_match_patch;

import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;

public class Speedtest {

  public static void main(String args[]) {
    String text1 = readFile("tests/name/fraser/neil/plaintext/Speedtest1.txt");
    String text2 = readFile("tests/name/fraser/neil/plaintext/Speedtest2.txt");

    diff_match_patch dmp = new diff_match_patch();
    dmp.Diff_Timeout = 0;

    // Execute one reverse diff as a warmup.
    dmp.diff_main(text2, text1, false);
    System.gc();

    long start_time = System.currentTimeMillis();
    dmp.diff_main(text1, text2, false);
    long end_time = System.currentTimeMillis();
    System.out.printf("Elapsed time: %f\n", ((end_time - start_time) / 1000.0));
  }

  private static String readFile(String filename) {
    // Read a file from disk and return the text contents.
    StringBuffer strbuf = new StringBuffer();
    try {
      FileReader input = new FileReader(filename);
      BufferedReader bufRead = new BufferedReader(input);
      String line = bufRead.readLine();
      while (line != null) {
        strbuf.append(line);
        strbuf.append('\n');
        line = bufRead.readLine();
      }

      bufRead.close();

    } catch (IOException e) {
      e.printStackTrace();
    }
    return strbuf.toString();
  }
}
