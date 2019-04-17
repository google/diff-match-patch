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

/*
 * Functions for diff, match and patch.
 * Computes the difference between two texts to create a patch.
 * Applies the patch onto another text, allowing for errors.
 *
 * @author fraser@google.com (Neil Fraser)
 */

part of DiffMatchPatch;

/**
 * The data structure representing a diff is a List of Diff objects:
 * {Diff(Operation.delete, 'Hello'), Diff(Operation.insert, 'Goodbye'),
 *  Diff(Operation.equal, ' world.')}
 * which means: delete 'Hello', add 'Goodbye' and keep ' world.'
 */
enum Operation { delete, insert, equal }

/**
 * Class containing the diff, match and patch methods.
 * Also contains the behaviour settings.
 */
class DiffMatchPatch {
  // Defaults.
  // Set these on your diff_match_patch instance to override the defaults.

  /**
   * Number of seconds to map a diff before giving up (0 for infinity).
   */
  double Diff_Timeout = 1.0;
  /**
   * Cost of an empty edit operation in terms of edit characters.
   */
  int Diff_EditCost = 4;
  /**
   * At what point is no match declared (0.0 = perfection, 1.0 = very loose).
   */
  double Match_Threshold = 0.5;
  /**
   * How far to search for a match (0 = exact location, 1000+ = broad match).
   * A match this many characters away from the expected location will add
   * 1.0 to the score (0.0 is a perfect match).
   */
  int Match_Distance = 1000;
  /**
   * When deleting a large block of text (over ~64 characters), how close do
   * the contents have to be to match the expected contents. (0.0 = perfection,
   * 1.0 = very loose).  Note that Match_Threshold controls how closely the
   * end points of a delete need to match.
   */
  double Patch_DeleteThreshold = 0.5;
  /**
   * Chunk size for context length.
   */
  int Patch_Margin = 4;

  /**
   * The number of bits in an int.
   */
  int Match_MaxBits = 32;

  //  DIFF FUNCTIONS

  /**
   * Find the differences between two texts.  Simplifies the problem by
   * stripping any common prefix or suffix off the texts before diffing.
   * [text1] is the old string to be diffed.
   * [text2] is the new string to be diffed.
   * [checklines] is an optional speedup flag.  If present and false, then don't
   *     run a line-level diff first to identify the changed areas.
   *     Defaults to true, which does a faster, slightly less optimal diff.
   * [deadline] is an optional time when the diff should be complete by.  Used
   *     internally for recursive calls.  Users should set DiffTimeout instead.
   * Returns a List of Diff objects.
   */
  List<Diff> diff_main(String text1, String text2,
      [bool checklines = true, DateTime deadline]) {
    // Set a deadline by which time the diff must be complete.
    if (deadline == null) {
      deadline = new DateTime.now();
      if (Diff_Timeout <= 0) {
        // One year should be sufficient for 'infinity'.
        deadline = deadline.add(new Duration(days: 365));
      } else {
        deadline = deadline
            .add(new Duration(milliseconds: (Diff_Timeout * 1000).toInt()));
      }
    }
    // Check for null inputs.
    if (text1 == null || text2 == null) {
      throw new ArgumentError('Null inputs. (diff_main)');
    }

    // Check for equality (speedup).
    List<Diff> diffs;
    if (text1 == text2) {
      diffs = [];
      if (!text1.isEmpty) {
        diffs.add(new Diff(Operation.equal, text1));
      }
      return diffs;
    }

    // Trim off common prefix (speedup).
    int commonlength = diff_commonPrefix(text1, text2);
    String commonprefix = text1.substring(0, commonlength);
    text1 = text1.substring(commonlength);
    text2 = text2.substring(commonlength);

    // Trim off common suffix (speedup).
    commonlength = diff_commonSuffix(text1, text2);
    String commonsuffix = text1.substring(text1.length - commonlength);
    text1 = text1.substring(0, text1.length - commonlength);
    text2 = text2.substring(0, text2.length - commonlength);

    // Compute the diff on the middle block.
    diffs = _diff_compute(text1, text2, checklines, deadline);

    // Restore the prefix and suffix.
    if (!commonprefix.isEmpty) {
      diffs.insert(0, new Diff(Operation.equal, commonprefix));
    }
    if (!commonsuffix.isEmpty) {
      diffs.add(new Diff(Operation.equal, commonsuffix));
    }

    diff_cleanupMerge(diffs);
    return diffs;
  }

  /**
   * Find the differences between two texts.  Assumes that the texts do not
   * have any common prefix or suffix.
   * [text1] is the old string to be diffed.
   * [text2] is the new string to be diffed.
   * [checklines] is a speedup flag.  If false, then don't run a
   *     line-level diff first to identify the changed areas.
   *     If true, then run a faster slightly less optimal diff.
   * [deadline] is the time when the diff should be complete by.
   * Returns a List of Diff objects.
   */
  List<Diff> _diff_compute(
      String text1, String text2, bool checklines, DateTime deadline) {
    List<Diff> diffs = <Diff>[];

    if (text1.length == 0) {
      // Just add some text (speedup).
      diffs.add(new Diff(Operation.insert, text2));
      return diffs;
    }

    if (text2.length == 0) {
      // Just delete some text (speedup).
      diffs.add(new Diff(Operation.delete, text1));
      return diffs;
    }

    String longtext = text1.length > text2.length ? text1 : text2;
    String shorttext = text1.length > text2.length ? text2 : text1;
    int i = longtext.indexOf(shorttext);
    if (i != -1) {
      // Shorter text is inside the longer text (speedup).
      Operation op =
          (text1.length > text2.length) ? Operation.delete : Operation.insert;
      diffs.add(new Diff(op, longtext.substring(0, i)));
      diffs.add(new Diff(Operation.equal, shorttext));
      diffs.add(new Diff(op, longtext.substring(i + shorttext.length)));
      return diffs;
    }

    if (shorttext.length == 1) {
      // Single character string.
      // After the previous speedup, the character can't be an equality.
      diffs.add(new Diff(Operation.delete, text1));
      diffs.add(new Diff(Operation.insert, text2));
      return diffs;
    }

    // Check to see if the problem can be split in two.
    final hm = _diff_halfMatch(text1, text2);
    if (hm != null) {
      // A half-match was found, sort out the return data.
      final text1_a = hm[0];
      final text1_b = hm[1];
      final text2_a = hm[2];
      final text2_b = hm[3];
      final mid_common = hm[4];
      // Send both pairs off for separate processing.
      final diffs_a = diff_main(text1_a, text2_a, checklines, deadline);
      final diffs_b = diff_main(text1_b, text2_b, checklines, deadline);
      // Merge the results.
      diffs = diffs_a;
      diffs.add(new Diff(Operation.equal, mid_common));
      diffs.addAll(diffs_b);
      return diffs;
    }

    if (checklines && text1.length > 100 && text2.length > 100) {
      return _diff_lineMode(text1, text2, deadline);
    }

    return _diff_bisect(text1, text2, deadline);
  }

  /**
   * Do a quick line-level diff on both strings, then rediff the parts for
   * greater accuracy.
   * This speedup can produce non-minimal diffs.
   * [text1] is the old string to be diffed.
   * [text2] is the new string to be diffed.
   * [deadline] is the time when the diff should be complete by.
   * Returns a List of Diff objects.
   */
  List<Diff> _diff_lineMode(String text1, String text2, DateTime deadline) {
    // Scan the text on a line-by-line basis first.
    final a = _diff_linesToChars(text1, text2);
    text1 = a['chars1'];
    text2 = a['chars2'];
    final linearray = a['lineArray'];

    final diffs = diff_main(text1, text2, false, deadline);

    // Convert the diff back to original text.
    _diff_charsToLines(diffs, linearray);
    // Eliminate freak matches (e.g. blank lines)
    diff_cleanupSemantic(diffs);

    // Rediff any replacement blocks, this time character-by-character.
    // Add a dummy entry at the end.
    diffs.add(new Diff(Operation.equal, ''));
    int pointer = 0;
    int count_delete = 0;
    int count_insert = 0;
    final text_delete = new StringBuffer();
    final text_insert = new StringBuffer();
    while (pointer < diffs.length) {
      switch (diffs[pointer].operation) {
        case Operation.insert:
          count_insert++;
          text_insert.write(diffs[pointer].text);
          break;
        case Operation.delete:
          count_delete++;
          text_delete.write(diffs[pointer].text);
          break;
        case Operation.equal:
          // Upon reaching an equality, check for prior redundancies.
          if (count_delete >= 1 && count_insert >= 1) {
            // Delete the offending records and add the merged ones.
            diffs.removeRange(pointer - count_delete - count_insert, pointer);
            pointer = pointer - count_delete - count_insert;
            final subDiff = diff_main(text_delete.toString(),
                text_insert.toString(), false, deadline);
            for (int j = subDiff.length - 1; j >= 0; j--) {
              diffs.insert(pointer, subDiff[j]);
            }
            pointer = pointer + subDiff.length;
          }
          count_insert = 0;
          count_delete = 0;
          text_delete.clear();
          text_insert.clear();
          break;
      }
      pointer++;
    }
    diffs.removeLast(); // Remove the dummy entry at the end.

    return diffs;
  }

  /**
   * Find the 'middle snake' of a diff, split the problem in two
   * and return the recursively constructed diff.
   * See Myers 1986 paper: An O(ND) Difference Algorithm and Its Variations.
   * [text1] is the old string to be diffed.
   * [text2] is the new string to be diffed.
   * [deadline] is the time at which to bail if not yet complete.
   * Returns a List of Diff objects.
   */
  List<Diff> _diff_bisect(String text1, String text2, DateTime deadline) {
    // Cache the text lengths to prevent multiple calls.
    final text1_length = text1.length;
    final text2_length = text2.length;
    final max_d = (text1_length + text2_length + 1) ~/ 2;
    final v_offset = max_d;
    final v_length = 2 * max_d;
    final v1 = new List<int>(v_length);
    final v2 = new List<int>(v_length);
    for (int x = 0; x < v_length; x++) {
      v1[x] = -1;
      v2[x] = -1;
    }
    v1[v_offset + 1] = 0;
    v2[v_offset + 1] = 0;
    final delta = text1_length - text2_length;
    // If the total number of characters is odd, then the front path will
    // collide with the reverse path.
    final front = (delta % 2 != 0);
    // Offsets for start and end of k loop.
    // Prevents mapping of space beyond the grid.
    int k1start = 0;
    int k1end = 0;
    int k2start = 0;
    int k2end = 0;
    for (int d = 0; d < max_d; d++) {
      // Bail out if deadline is reached.
      if ((new DateTime.now()).compareTo(deadline) == 1) {
        break;
      }

      // Walk the front path one step.
      for (int k1 = -d + k1start; k1 <= d - k1end; k1 += 2) {
        int k1_offset = v_offset + k1;
        int x1;
        if (k1 == -d || k1 != d && v1[k1_offset - 1] < v1[k1_offset + 1]) {
          x1 = v1[k1_offset + 1];
        } else {
          x1 = v1[k1_offset - 1] + 1;
        }
        int y1 = x1 - k1;
        while (
            x1 < text1_length && y1 < text2_length && text1[x1] == text2[y1]) {
          x1++;
          y1++;
        }
        v1[k1_offset] = x1;
        if (x1 > text1_length) {
          // Ran off the right of the graph.
          k1end += 2;
        } else if (y1 > text2_length) {
          // Ran off the bottom of the graph.
          k1start += 2;
        } else if (front) {
          int k2_offset = v_offset + delta - k1;
          if (k2_offset >= 0 && k2_offset < v_length && v2[k2_offset] != -1) {
            // Mirror x2 onto top-left coordinate system.
            int x2 = text1_length - v2[k2_offset];
            if (x1 >= x2) {
              // Overlap detected.
              return _diff_bisectSplit(text1, text2, x1, y1, deadline);
            }
          }
        }
      }

      // Walk the reverse path one step.
      for (int k2 = -d + k2start; k2 <= d - k2end; k2 += 2) {
        int k2_offset = v_offset + k2;
        int x2;
        if (k2 == -d || k2 != d && v2[k2_offset - 1] < v2[k2_offset + 1]) {
          x2 = v2[k2_offset + 1];
        } else {
          x2 = v2[k2_offset - 1] + 1;
        }
        int y2 = x2 - k2;
        while (x2 < text1_length &&
            y2 < text2_length &&
            text1[text1_length - x2 - 1] == text2[text2_length - y2 - 1]) {
          x2++;
          y2++;
        }
        v2[k2_offset] = x2;
        if (x2 > text1_length) {
          // Ran off the left of the graph.
          k2end += 2;
        } else if (y2 > text2_length) {
          // Ran off the top of the graph.
          k2start += 2;
        } else if (!front) {
          int k1_offset = v_offset + delta - k2;
          if (k1_offset >= 0 && k1_offset < v_length && v1[k1_offset] != -1) {
            int x1 = v1[k1_offset];
            int y1 = v_offset + x1 - k1_offset;
            // Mirror x2 onto top-left coordinate system.
            x2 = text1_length - x2;
            if (x1 >= x2) {
              // Overlap detected.
              return _diff_bisectSplit(text1, text2, x1, y1, deadline);
            }
          }
        }
      }
    }
    // Diff took too long and hit the deadline or
    // number of diffs equals number of characters, no commonality at all.
    return [
      new Diff(Operation.delete, text1),
      new Diff(Operation.insert, text2)
    ];
  }

  /**
   * Hack to allow unit tests to call private method.  Do not use.
   */
  List<Diff> test_diff_bisect(String text1, String text2, DateTime deadline) {
    return _diff_bisect(text1, text2, deadline);
  }

  /**
   * Given the location of the 'middle snake', split the diff in two parts
   * and recurse.
   * [text1] is the old string to be diffed.
   * [text2] is the new string to be diffed.
   * [x] is the index of split point in text1.
   * [y] is the index of split point in text2.
   * [deadline] is the time at which to bail if not yet complete.
   * Returns a List of Diff objects.
   */
  List<Diff> _diff_bisectSplit(
      String text1, String text2, int x, int y, DateTime deadline) {
    final text1a = text1.substring(0, x);
    final text2a = text2.substring(0, y);
    final text1b = text1.substring(x);
    final text2b = text2.substring(y);

    // Compute both diffs serially.
    final diffs = diff_main(text1a, text2a, false, deadline);
    final diffsb = diff_main(text1b, text2b, false, deadline);

    diffs.addAll(diffsb);
    return diffs;
  }

  /**
   * Split two texts into a list of strings.  Reduce the texts to a string of
   * hashes where each Unicode character represents one line.
   * [text1] is the first string.
   * [text2] is the second string.
   * Returns a Map containing the encoded text1, the encoded text2 and
   *     the List of unique strings.  The zeroth element of the List of
   *     unique strings is intentionally blank.
   */
  Map<String, dynamic> _diff_linesToChars(String text1, String text2) {
    final lineArray = <String>[];
    final lineHash = new HashMap<String, int>();
    // e.g. linearray[4] == 'Hello\n'
    // e.g. linehash['Hello\n'] == 4

    // '\x00' is a valid character, but various debuggers don't like it.
    // So we'll insert a junk entry to avoid generating a null character.
    lineArray.add('');

    // Allocate 2/3rds of the space for text1, the rest for text2.
    String chars1 = _diff_linesToCharsMunge(text1, lineArray, lineHash, 40000);
    String chars2 = _diff_linesToCharsMunge(text2, lineArray, lineHash, 65535);
    return {'chars1': chars1, 'chars2': chars2, 'lineArray': lineArray};
  }

  /**
   * Hack to allow unit tests to call private method.  Do not use.
   */
  Map<String, dynamic> test_diff_linesToChars(String text1, String text2) {
    return _diff_linesToChars(text1, text2);
  }

  /**
   * Split a text into a list of strings.  Reduce the texts to a string of
   * hashes where each Unicode character represents one line.
   * [text] is the string to encode.
   * [lineArray] is a List of unique strings.
   * [lineHash] is a Map of strings to indices.
   * [maxLines] is the maximum length for lineArray.
   * Returns an encoded string.
   */
  String _diff_linesToCharsMunge(String text, List<String> lineArray,
      Map<String, int> lineHash, int maxLines) {
    int lineStart = 0;
    int lineEnd = -1;
    String line;
    final chars = new StringBuffer();
    // Walk the text, pulling out a substring for each line.
    // text.split('\n') would would temporarily double our memory footprint.
    // Modifying text would create many large strings to garbage collect.
    while (lineEnd < text.length - 1) {
      lineEnd = text.indexOf('\n', lineStart);
      if (lineEnd == -1) {
        lineEnd = text.length - 1;
      }
      line = text.substring(lineStart, lineEnd + 1);

      if (lineHash.containsKey(line)) {
        chars.writeCharCode(lineHash[line]);
      } else {
        if (lineArray.length == maxLines) {
          // Bail out at 65535 because
          // final chars1 = new StringBuffer();
          // chars1.writeCharCode(65536);
          // chars1.toString().codeUnitAt(0) == 55296;
          line = text.substring(lineStart);
          lineEnd = text.length;
        }
        lineArray.add(line);
        lineHash[line] = lineArray.length - 1;
        chars.writeCharCode(lineArray.length - 1);
      }
      lineStart = lineEnd + 1;
    }
    return chars.toString();
  }

  /**
   * Rehydrate the text in a diff from a string of line hashes to real lines of
   * text.
   * [diffs] is a List of Diff objects.
   * [lineArray] is a List of unique strings.
   */
  void _diff_charsToLines(List<Diff> diffs, List<String> lineArray) {
    final text = new StringBuffer();
    for (Diff diff in diffs) {
      for (int j = 0; j < diff.text.length; j++) {
        text.write(lineArray[diff.text.codeUnitAt(j)]);
      }
      diff.text = text.toString();
      text.clear();
    }
  }

  /**
   * Hack to allow unit tests to call private method.  Do not use.
   */
  void test_diff_charsToLines(List<Diff> diffs, List<String> lineArray) {
     _diff_charsToLines(diffs, lineArray);
  }

  /**
   * Determine the common prefix of two strings
   * [text1] is the first string.
   * [text2] is the second string.
   * Returns the number of characters common to the start of each string.
   */
  int diff_commonPrefix(String text1, String text2) {
    // TODO: Once Dart's performance stabilizes, determine if linear or binary
    // search is better.
    // Performance analysis: https://neil.fraser.name/news/2007/10/09/
    final n = min(text1.length, text2.length);
    for (int i = 0; i < n; i++) {
      if (text1[i] != text2[i]) {
        return i;
      }
    }
    return n;
  }

  /**
   * Determine the common suffix of two strings
   * [text1] is the first string.
   * [text2] is the second string.
   * Returns the number of characters common to the end of each string.
   */
  int diff_commonSuffix(String text1, String text2) {
    // TODO: Once Dart's performance stabilizes, determine if linear or binary
    // search is better.
    // Performance analysis: https://neil.fraser.name/news/2007/10/09/
    final text1_length = text1.length;
    final text2_length = text2.length;
    final n = min(text1_length, text2_length);
    for (int i = 1; i <= n; i++) {
      if (text1[text1_length - i] != text2[text2_length - i]) {
        return i - 1;
      }
    }
    return n;
  }

  /**
   * Determine if the suffix of one string is the prefix of another.
   * [text1] is the first string.
   * [text2] is the second string.
   * Returns the number of characters common to the end of the first
   *     string and the start of the second string.
   */
  int _diff_commonOverlap(String text1, String text2) {
    // Eliminate the null case.
    if (text1.isEmpty || text2.isEmpty) {
      return 0;
    }
    // Cache the text lengths to prevent multiple calls.
    final text1_length = text1.length;
    final text2_length = text2.length;
    // Truncate the longer string.
    if (text1_length > text2_length) {
      text1 = text1.substring(text1_length - text2_length);
    } else if (text1_length < text2_length) {
      text2 = text2.substring(0, text1_length);
    }
    final text_length = min(text1_length, text2_length);
    // Quick check for the worst case.
    if (text1 == text2) {
      return text_length;
    }

    // Start by looking for a single character match
    // and increase length until no match is found.
    // Performance analysis: https://neil.fraser.name/news/2010/11/04/
    int best = 0;
    int length = 1;
    while (true) {
      String pattern = text1.substring(text_length - length);
      int found = text2.indexOf(pattern);
      if (found == -1) {
        return best;
      }
      length += found;
      if (found == 0 ||
          text1.substring(text_length - length) == text2.substring(0, length)) {
        best = length;
        length++;
      }
    }
  }

  /**
   * Hack to allow unit tests to call private method.  Do not use.
   */
  int test_diff_commonOverlap(String text1, String text2) {
    return _diff_commonOverlap(text1, text2);
  }

  /**
   * Do the two texts share a substring which is at least half the length of
   * the longer text?
   * This speedup can produce non-minimal diffs.
   * [text1] is the first string.
   * [text2] is the second string.
   * Returns a five element List of Strings, containing the prefix of text1,
   *     the suffix of text1, the prefix of text2, the suffix of text2 and the
   *     common middle.  Or null if there was no match.
   */
  List<String> _diff_halfMatch(String text1, String text2) {
    if (Diff_Timeout <= 0) {
      // Don't risk returning a non-optimal diff if we have unlimited time.
      return null;
    }
    final longtext = text1.length > text2.length ? text1 : text2;
    final shorttext = text1.length > text2.length ? text2 : text1;
    if (longtext.length < 4 || shorttext.length * 2 < longtext.length) {
      return null; // Pointless.
    }

    // First check if the second quarter is the seed for a half-match.
    final hm1 = _diff_halfMatchI(
        longtext, shorttext, ((longtext.length + 3) / 4).ceil().toInt());
    // Check again based on the third quarter.
    final hm2 = _diff_halfMatchI(
        longtext, shorttext, ((longtext.length + 1) / 2).ceil().toInt());
    List<String> hm;
    if (hm1 == null && hm2 == null) {
      return null;
    } else if (hm2 == null) {
      hm = hm1;
    } else if (hm1 == null) {
      hm = hm2;
    } else {
      // Both matched.  Select the longest.
      hm = hm1[4].length > hm2[4].length ? hm1 : hm2;
    }

    // A half-match was found, sort out the return data.
    if (text1.length > text2.length) {
      return hm;
      //return [hm[0], hm[1], hm[2], hm[3], hm[4]];
    } else {
      return [hm[2], hm[3], hm[0], hm[1], hm[4]];
    }
  }

  /**
   * Hack to allow unit tests to call private method.  Do not use.
   */
  List<String> test_diff_halfMatch(String text1, String text2) {
    return _diff_halfMatch(text1, text2);
  }

  /**
   * Does a substring of shorttext exist within longtext such that the
   * substring is at least half the length of longtext?
   * [longtext] is the longer string.
   * [shorttext is the shorter string.
   * [i] Start index of quarter length substring within longtext.
   * Returns a five element String array, containing the prefix of longtext,
   *     the suffix of longtext, the prefix of shorttext, the suffix of
   *     shorttext and the common middle.  Or null if there was no match.
   */
  List<String> _diff_halfMatchI(String longtext, String shorttext, int i) {
    // Start with a 1/4 length substring at position i as a seed.
    final seed =
        longtext.substring(i, i + (longtext.length / 4).floor().toInt());
    int j = -1;
    String best_common = '';
    String best_longtext_a = '', best_longtext_b = '';
    String best_shorttext_a = '', best_shorttext_b = '';
    while ((j = shorttext.indexOf(seed, j + 1)) != -1) {
      int prefixLength =
          diff_commonPrefix(longtext.substring(i), shorttext.substring(j));
      int suffixLength = diff_commonSuffix(
          longtext.substring(0, i), shorttext.substring(0, j));
      if (best_common.length < suffixLength + prefixLength) {
        best_common = shorttext.substring(j - suffixLength, j) +
            shorttext.substring(j, j + prefixLength);
        best_longtext_a = longtext.substring(0, i - suffixLength);
        best_longtext_b = longtext.substring(i + prefixLength);
        best_shorttext_a = shorttext.substring(0, j - suffixLength);
        best_shorttext_b = shorttext.substring(j + prefixLength);
      }
    }
    if (best_common.length * 2 >= longtext.length) {
      return [
        best_longtext_a,
        best_longtext_b,
        best_shorttext_a,
        best_shorttext_b,
        best_common
      ];
    } else {
      return null;
    }
  }

  /**
   * Reduce the number of edits by eliminating semantically trivial equalities.
   * [diffs] is a List of Diff objects.
   */
  void diff_cleanupSemantic(List<Diff> diffs) {
    bool changes = false;
    // Stack of indices where equalities are found.
    final equalities = <int>[];
    // Always equal to diffs[equalities.last()].text
    String lastEquality = null;
    int pointer = 0; // Index of current position.
    // Number of characters that changed prior to the equality.
    int length_insertions1 = 0;
    int length_deletions1 = 0;
    // Number of characters that changed after the equality.
    int length_insertions2 = 0;
    int length_deletions2 = 0;
    while (pointer < diffs.length) {
      if (diffs[pointer].operation == Operation.equal) {
        // Equality found.
        equalities.add(pointer);
        length_insertions1 = length_insertions2;
        length_deletions1 = length_deletions2;
        length_insertions2 = 0;
        length_deletions2 = 0;
        lastEquality = diffs[pointer].text;
      } else {
        // An insertion or deletion.
        if (diffs[pointer].operation == Operation.insert) {
          length_insertions2 += diffs[pointer].text.length;
        } else {
          length_deletions2 += diffs[pointer].text.length;
        }
        // Eliminate an equality that is smaller or equal to the edits on both
        // sides of it.
        if (lastEquality != null &&
            (lastEquality.length <=
                max(length_insertions1, length_deletions1)) &&
            (lastEquality.length <=
                max(length_insertions2, length_deletions2))) {
          // Duplicate record.
          diffs.insert(
              equalities.last, new Diff(Operation.delete, lastEquality));
          // Change second copy to insert.
          diffs[equalities.last + 1].operation = Operation.insert;
          // Throw away the equality we just deleted.
          equalities.removeLast();
          // Throw away the previous equality (it needs to be reevaluated).
          if (!equalities.isEmpty) {
            equalities.removeLast();
          }
          pointer = equalities.isEmpty ? -1 : equalities.last;
          length_insertions1 = 0; // Reset the counters.
          length_deletions1 = 0;
          length_insertions2 = 0;
          length_deletions2 = 0;
          lastEquality = null;
          changes = true;
        }
      }
      pointer++;
    }

    // Normalize the diff.
    if (changes) {
      diff_cleanupMerge(diffs);
    }
    _diff_cleanupSemanticLossless(diffs);

    // Find any overlaps between deletions and insertions.
    // e.g: <del>abcxxx</del><ins>xxxdef</ins>
    //   -> <del>abc</del>xxx<ins>def</ins>
    // e.g: <del>xxxabc</del><ins>defxxx</ins>
    //   -> <ins>def</ins>xxx<del>abc</del>
    // Only extract an overlap if it is as big as the edit ahead or behind it.
    pointer = 1;
    while (pointer < diffs.length) {
      if (diffs[pointer - 1].operation == Operation.delete &&
          diffs[pointer].operation == Operation.insert) {
        String deletion = diffs[pointer - 1].text;
        String insertion = diffs[pointer].text;
        int overlap_length1 = _diff_commonOverlap(deletion, insertion);
        int overlap_length2 = _diff_commonOverlap(insertion, deletion);
        if (overlap_length1 >= overlap_length2) {
          if (overlap_length1 >= deletion.length / 2 ||
              overlap_length1 >= insertion.length / 2) {
            // Overlap found.
            // Insert an equality and trim the surrounding edits.
            diffs.insert(
                pointer,
                new Diff(
                    Operation.equal, insertion.substring(0, overlap_length1)));
            diffs[pointer - 1].text =
                deletion.substring(0, deletion.length - overlap_length1);
            diffs[pointer + 1].text = insertion.substring(overlap_length1);
            pointer++;
          }
        } else {
          if (overlap_length2 >= deletion.length / 2 ||
              overlap_length2 >= insertion.length / 2) {
            // Reverse overlap found.
            // Insert an equality and swap and trim the surrounding edits.
            diffs.insert(
                pointer,
                new Diff(
                    Operation.equal, deletion.substring(0, overlap_length2)));
            diffs[pointer - 1] = new Diff(Operation.insert,
                insertion.substring(0, insertion.length - overlap_length2));
            diffs[pointer + 1] =
                new Diff(Operation.delete, deletion.substring(overlap_length2));
            pointer++;
          }
        }
        pointer++;
      }
      pointer++;
    }
  }

  /**
   * Look for single edits surrounded on both sides by equalities
   * which can be shifted sideways to align the edit to a word boundary.
   * e.g: The c<ins>at c</ins>ame. -> The <ins>cat </ins>came.
   * [diffs] is a List of Diff objects.
   */
  void _diff_cleanupSemanticLossless(List<Diff> diffs) {
    /**
     * Given two strings, compute a score representing whether the internal
     * boundary falls on logical boundaries.
     * Scores range from 6 (best) to 0 (worst).
     * Closure, but does not reference any external variables.
     * [one] the first string.
     * [two] the second string.
     * Returns the score.
     */
    int _diff_cleanupSemanticScore(String one, String two) {
      if (one.isEmpty || two.isEmpty) {
        // Edges are the best.
        return 6;
      }

      // Each port of this function behaves slightly differently due to
      // subtle differences in each language's definition of things like
      // 'whitespace'.  Since this function's purpose is largely cosmetic,
      // the choice has been made to use each language's native features
      // rather than force total conformity.
      String char1 = one[one.length - 1];
      String char2 = two[0];
      bool nonAlphaNumeric1 = char1.contains(nonAlphaNumericRegex_);
      bool nonAlphaNumeric2 = char2.contains(nonAlphaNumericRegex_);
      bool whitespace1 = nonAlphaNumeric1 && char1.contains(whitespaceRegex_);
      bool whitespace2 = nonAlphaNumeric2 && char2.contains(whitespaceRegex_);
      bool lineBreak1 = whitespace1 && char1.contains(linebreakRegex_);
      bool lineBreak2 = whitespace2 && char2.contains(linebreakRegex_);
      bool blankLine1 = lineBreak1 && one.contains(blanklineEndRegex_);
      bool blankLine2 = lineBreak2 && two.contains(blanklineStartRegex_);

      if (blankLine1 || blankLine2) {
        // Five points for blank lines.
        return 5;
      } else if (lineBreak1 || lineBreak2) {
        // Four points for line breaks.
        return 4;
      } else if (nonAlphaNumeric1 && !whitespace1 && whitespace2) {
        // Three points for end of sentences.
        return 3;
      } else if (whitespace1 || whitespace2) {
        // Two points for whitespace.
        return 2;
      } else if (nonAlphaNumeric1 || nonAlphaNumeric2) {
        // One point for non-alphanumeric.
        return 1;
      }
      return 0;
    }

    int pointer = 1;
    // Intentionally ignore the first and last element (don't need checking).
    while (pointer < diffs.length - 1) {
      if (diffs[pointer - 1].operation == Operation.equal &&
          diffs[pointer + 1].operation == Operation.equal) {
        // This is a single edit surrounded by equalities.
        String equality1 = diffs[pointer - 1].text;
        String edit = diffs[pointer].text;
        String equality2 = diffs[pointer + 1].text;

        // First, shift the edit as far left as possible.
        int commonOffset = diff_commonSuffix(equality1, edit);
        if (commonOffset != 0) {
          String commonString = edit.substring(edit.length - commonOffset);
          equality1 = equality1.substring(0, equality1.length - commonOffset);
          edit = commonString + edit.substring(0, edit.length - commonOffset);
          equality2 = commonString + equality2;
        }

        // Second, step character by character right, looking for the best fit.
        String bestEquality1 = equality1;
        String bestEdit = edit;
        String bestEquality2 = equality2;
        int bestScore = _diff_cleanupSemanticScore(equality1, edit) +
            _diff_cleanupSemanticScore(edit, equality2);
        while (!edit.isEmpty && !equality2.isEmpty && edit[0] == equality2[0]) {
          equality1 = equality1 + edit[0];
          edit = edit.substring(1) + equality2[0];
          equality2 = equality2.substring(1);
          int score = _diff_cleanupSemanticScore(equality1, edit) +
              _diff_cleanupSemanticScore(edit, equality2);
          // The >= encourages trailing rather than leading whitespace on edits.
          if (score >= bestScore) {
            bestScore = score;
            bestEquality1 = equality1;
            bestEdit = edit;
            bestEquality2 = equality2;
          }
        }

        if (diffs[pointer - 1].text != bestEquality1) {
          // We have an improvement, save it back to the diff.
          if (!bestEquality1.isEmpty) {
            diffs[pointer - 1].text = bestEquality1;
          } else {
            diffs.removeAt(pointer - 1);
            pointer--;
          }
          diffs[pointer].text = bestEdit;
          if (!bestEquality2.isEmpty) {
            diffs[pointer + 1].text = bestEquality2;
          } else {
            diffs.removeAt(pointer + 1);
            pointer--;
          }
        }
      }
      pointer++;
    }
  }

  /**
   * Hack to allow unit tests to call private method.  Do not use.
   */
  void test_diff_cleanupSemanticLossless(List<Diff> diffs) {
    _diff_cleanupSemanticLossless(diffs);
  }

  // Define some regex patterns for matching boundaries.
  RegExp nonAlphaNumericRegex_ = new RegExp(r'[^a-zA-Z0-9]');
  RegExp whitespaceRegex_ = new RegExp(r'\s');
  RegExp linebreakRegex_ = new RegExp(r'[\r\n]');
  RegExp blanklineEndRegex_ = new RegExp(r'\n\r?\n$');
  RegExp blanklineStartRegex_ = new RegExp(r'^\r?\n\r?\n');

  /**
   * Reduce the number of edits by eliminating operationally trivial equalities.
   * [diffs] is a List of Diff objects.
   */
  void diff_cleanupEfficiency(List<Diff> diffs) {
    bool changes = false;
    // Stack of indices where equalities are found.
    final equalities = <int>[];
    // Always equal to diffs[equalities.last()].text
    String lastEquality = null;
    int pointer = 0; // Index of current position.
    // Is there an insertion operation before the last equality.
    bool pre_ins = false;
    // Is there a deletion operation before the last equality.
    bool pre_del = false;
    // Is there an insertion operation after the last equality.
    bool post_ins = false;
    // Is there a deletion operation after the last equality.
    bool post_del = false;
    while (pointer < diffs.length) {
      if (diffs[pointer].operation == Operation.equal) {
        // Equality found.
        if (diffs[pointer].text.length < Diff_EditCost &&
            (post_ins || post_del)) {
          // Candidate found.
          equalities.add(pointer);
          pre_ins = post_ins;
          pre_del = post_del;
          lastEquality = diffs[pointer].text;
        } else {
          // Not a candidate, and can never become one.
          equalities.clear();
          lastEquality = null;
        }
        post_ins = post_del = false;
      } else {
        // An insertion or deletion.
        if (diffs[pointer].operation == Operation.delete) {
          post_del = true;
        } else {
          post_ins = true;
        }
        /*
         * Five types to be split:
         * <ins>A</ins><del>B</del>XY<ins>C</ins><del>D</del>
         * <ins>A</ins>X<ins>C</ins><del>D</del>
         * <ins>A</ins><del>B</del>X<ins>C</ins>
         * <ins>A</del>X<ins>C</ins><del>D</del>
         * <ins>A</ins><del>B</del>X<del>C</del>
         */
        if (lastEquality != null &&
            ((pre_ins && pre_del && post_ins && post_del) ||
                ((lastEquality.length < Diff_EditCost / 2) &&
                    ((pre_ins ? 1 : 0) +
                            (pre_del ? 1 : 0) +
                            (post_ins ? 1 : 0) +
                            (post_del ? 1 : 0)) ==
                        3))) {
          // Duplicate record.
          diffs.insert(
              equalities.last, new Diff(Operation.delete, lastEquality));
          // Change second copy to insert.
          diffs[equalities.last + 1].operation = Operation.insert;
          equalities.removeLast(); // Throw away the equality we just deleted.
          lastEquality = null;
          if (pre_ins && pre_del) {
            // No changes made which could affect previous entry, keep going.
            post_ins = post_del = true;
            equalities.clear();
          } else {
            if (!equalities.isEmpty) {
              equalities.removeLast();
            }
            pointer = equalities.isEmpty ? -1 : equalities.last;
            post_ins = post_del = false;
          }
          changes = true;
        }
      }
      pointer++;
    }

    if (changes) {
      diff_cleanupMerge(diffs);
    }
  }

  /**
   * Reorder and merge like edit sections.  Merge equalities.
   * Any edit section can move as long as it doesn't cross an equality.
   * [diffs] is a List of Diff objects.
   */
  void diff_cleanupMerge(List<Diff> diffs) {
    diffs.add(new Diff(Operation.equal, '')); // Add a dummy entry at the end.
    int pointer = 0;
    int count_delete = 0;
    int count_insert = 0;
    String text_delete = '';
    String text_insert = '';
    int commonlength;
    while (pointer < diffs.length) {
      switch (diffs[pointer].operation) {
        case Operation.insert:
          count_insert++;
          text_insert += diffs[pointer].text;
          pointer++;
          break;
        case Operation.delete:
          count_delete++;
          text_delete += diffs[pointer].text;
          pointer++;
          break;
        case Operation.equal:
          // Upon reaching an equality, check for prior redundancies.
          if (count_delete + count_insert > 1) {
            if (count_delete != 0 && count_insert != 0) {
              // Factor out any common prefixies.
              commonlength = diff_commonPrefix(text_insert, text_delete);
              if (commonlength != 0) {
                if ((pointer - count_delete - count_insert) > 0 &&
                    diffs[pointer - count_delete - count_insert - 1]
                            .operation ==
                        Operation.equal) {
                  final i = pointer - count_delete - count_insert - 1;
                  diffs[i].text =
                      diffs[i].text + text_insert.substring(0, commonlength);
                } else {
                  diffs.insert(
                      0,
                      new Diff(Operation.equal,
                          text_insert.substring(0, commonlength)));
                  pointer++;
                }
                text_insert = text_insert.substring(commonlength);
                text_delete = text_delete.substring(commonlength);
              }

              // Factor out any common suffixies.
              commonlength = diff_commonSuffix(text_insert, text_delete);
              if (commonlength != 0) {
                diffs[pointer].text =
                    text_insert.substring(text_insert.length - commonlength) +
                        diffs[pointer].text;
                text_insert =
                    text_insert.substring(0, text_insert.length - commonlength);
                text_delete =
                    text_delete.substring(0, text_delete.length - commonlength);
              }
            }
            // Delete the offending records and add the merged ones.
            pointer -= count_delete + count_insert;
            diffs.removeRange(pointer, pointer + count_delete + count_insert);
            if (!text_delete.isEmpty) {
              diffs.insert(pointer, new Diff(Operation.delete, text_delete));
              pointer++;
            }
            if (!text_insert.isEmpty) {
              diffs.insert(pointer, new Diff(Operation.insert, text_insert));
              pointer++;
            }
            pointer++;
          } else if (pointer != 0 &&
              diffs[pointer - 1].operation == Operation.equal) {
            // Merge this equality with the previous one.
            diffs[pointer - 1].text =
                diffs[pointer - 1].text + diffs[pointer].text;
            diffs.removeAt(pointer);
          } else {
            pointer++;
          }
          count_insert = 0;
          count_delete = 0;
          text_delete = '';
          text_insert = '';
          break;
      }
    }
    if (diffs.last.text.isEmpty) {
      diffs.removeLast(); // Remove the dummy entry at the end.
    }

    // Second pass: look for single edits surrounded on both sides by equalities
    // which can be shifted sideways to eliminate an equality.
    // e.g: A<ins>BA</ins>C -> <ins>AB</ins>AC
    bool changes = false;
    pointer = 1;
    // Intentionally ignore the first and last element (don't need checking).
    while (pointer < diffs.length - 1) {
      if (diffs[pointer - 1].operation == Operation.equal &&
          diffs[pointer + 1].operation == Operation.equal) {
        // This is a single edit surrounded by equalities.
        if (diffs[pointer].text.endsWith(diffs[pointer - 1].text)) {
          // Shift the edit over the previous equality.
          diffs[pointer].text = diffs[pointer - 1].text +
              diffs[pointer].text.substring(0,
                  diffs[pointer].text.length - diffs[pointer - 1].text.length);
          diffs[pointer + 1].text =
              diffs[pointer - 1].text + diffs[pointer + 1].text;
          diffs.removeAt(pointer - 1);
          changes = true;
        } else if (diffs[pointer].text.startsWith(diffs[pointer + 1].text)) {
          // Shift the edit over the next equality.
          diffs[pointer - 1].text =
              diffs[pointer - 1].text + diffs[pointer + 1].text;
          diffs[pointer].text =
              diffs[pointer].text.substring(diffs[pointer + 1].text.length) +
                  diffs[pointer + 1].text;
          diffs.removeAt(pointer + 1);
          changes = true;
        }
      }
      pointer++;
    }
    // If shifts were made, the diff needs reordering and another shift sweep.
    if (changes) {
      diff_cleanupMerge(diffs);
    }
  }

  /**
   * loc is a location in text1, compute and return the equivalent location in
   * text2.
   * e.g. "The cat" vs "The big cat", 1->1, 5->8
   * [diffs] is a List of Diff objects.
   * [loc] is the location within text1.
   * Returns the location within text2.
   */
  int diff_xIndex(List<Diff> diffs, int loc) {
    int chars1 = 0;
    int chars2 = 0;
    int last_chars1 = 0;
    int last_chars2 = 0;
    Diff lastDiff = null;
    for (Diff aDiff in diffs) {
      if (aDiff.operation != Operation.insert) {
        // Equality or deletion.
        chars1 += aDiff.text.length;
      }
      if (aDiff.operation != Operation.delete) {
        // Equality or insertion.
        chars2 += aDiff.text.length;
      }
      if (chars1 > loc) {
        // Overshot the location.
        lastDiff = aDiff;
        break;
      }
      last_chars1 = chars1;
      last_chars2 = chars2;
    }
    if (lastDiff != null && lastDiff.operation == Operation.delete) {
      // The location was deleted.
      return last_chars2;
    }
    // Add the remaining character length.
    return last_chars2 + (loc - last_chars1);
  }

  /**
   * Convert a Diff list into a pretty HTML report.
   * [diffs] is a List of Diff objects.
   * Returns an HTML representation.
   */
  String diff_prettyHtml(List<Diff> diffs) {
    final html = new StringBuffer();
    for (Diff aDiff in diffs) {
      String text = aDiff.text
          .replaceAll('&', '&amp;')
          .replaceAll('<', '&lt;')
          .replaceAll('>', '&gt;')
          .replaceAll('\n', '&para;<br>');
      switch (aDiff.operation) {
        case Operation.insert:
          html.write('<ins style="background:#e6ffe6;">');
          html.write(text);
          html.write('</ins>');
          break;
        case Operation.delete:
          html.write('<del style="background:#ffe6e6;">');
          html.write(text);
          html.write('</del>');
          break;
        case Operation.equal:
          html.write('<span>');
          html.write(text);
          html.write('</span>');
          break;
      }
    }
    return html.toString();
  }

  /**
   * Compute and return the source text (all equalities and deletions).
   * [diffs] is a List of Diff objects.
   * Returns the source text.
   */
  String diff_text1(List<Diff> diffs) {
    final text = new StringBuffer();
    for (Diff aDiff in diffs) {
      if (aDiff.operation != Operation.insert) {
        text.write(aDiff.text);
      }
    }
    return text.toString();
  }

  /**
   * Compute and return the destination text (all equalities and insertions).
   * [diffs] is a List of Diff objects.
   * Returns the destination text.
   */
  String diff_text2(List<Diff> diffs) {
    final text = new StringBuffer();
    for (Diff aDiff in diffs) {
      if (aDiff.operation != Operation.delete) {
        text.write(aDiff.text);
      }
    }
    return text.toString();
  }

  /**
   * Compute the Levenshtein distance; the number of inserted, deleted or
   * substituted characters.
   * [diffs] is a List of Diff objects.
   * Returns the number of changes.
   */
  int diff_levenshtein(List<Diff> diffs) {
    int levenshtein = 0;
    int insertions = 0;
    int deletions = 0;
    for (Diff aDiff in diffs) {
      switch (aDiff.operation) {
        case Operation.insert:
          insertions += aDiff.text.length;
          break;
        case Operation.delete:
          deletions += aDiff.text.length;
          break;
        case Operation.equal:
          // A deletion and an insertion is one substitution.
          levenshtein += max(insertions, deletions);
          insertions = 0;
          deletions = 0;
          break;
      }
    }
    levenshtein += max(insertions, deletions);
    return levenshtein;
  }

  /**
   * Crush the diff into an encoded string which describes the operations
   * required to transform text1 into text2.
   * E.g. =3\t-2\t+ing  -> Keep 3 chars, delete 2 chars, insert 'ing'.
   * Operations are tab-separated.  Inserted text is escaped using %xx notation.
   * [diffs] is a List of Diff objects.
   * Returns the delta text.
   */
  String diff_toDelta(List<Diff> diffs) {
    final text = new StringBuffer();
    for (Diff aDiff in diffs) {
      switch (aDiff.operation) {
        case Operation.insert:
          text.write('+');
          text.write(Uri.encodeFull(aDiff.text));
          text.write('\t');
          break;
        case Operation.delete:
          text.write('-');
          text.write(aDiff.text.length);
          text.write('\t');
          break;
        case Operation.equal:
          text.write('=');
          text.write(aDiff.text.length);
          text.write('\t');
          break;
      }
    }
    String delta = text.toString();
    if (!delta.isEmpty) {
      // Strip off trailing tab character.
      delta = delta.substring(0, delta.length - 1);
    }
    return delta.replaceAll('%20', ' ');
  }

  /**
   * Given the original text1, and an encoded string which describes the
   * operations required to transform text1 into text2, compute the full diff.
   * [text1] is the source string for the diff.
   * [delta] is the delta text.
   * Returns a List of Diff objects or null if invalid.
   * Throws ArgumentError if invalid input.
   */
  List<Diff> diff_fromDelta(String text1, String delta) {
    final diffs = <Diff>[];
    int pointer = 0; // Cursor in text1
    final tokens = delta.split('\t');
    for (String token in tokens) {
      if (token.length == 0) {
        // Blank tokens are ok (from a trailing \t).
        continue;
      }
      // Each token begins with a one character parameter which specifies the
      // operation of this token (delete, insert, equality).
      String param = token.substring(1);
      switch (token[0]) {
        case '+':
          // decode would change all "+" to " "
          param = param.replaceAll('+', '%2B');
          try {
            param = Uri.decodeFull(param);
          } on ArgumentError {
            // Malformed URI sequence.
            throw new ArgumentError('Illegal escape in diff_fromDelta: $param');
          }
          diffs.add(new Diff(Operation.insert, param));
          break;
        case '-':
        // Fall through.
        case '=':
          int n;
          try {
            n = int.parse(param);
          } on FormatException {
            throw new ArgumentError('Invalid number in diff_fromDelta: $param');
          }
          if (n < 0) {
            throw new ArgumentError(
                'Negative number in diff_fromDelta: $param');
          }
          String text;
          try {
            text = text1.substring(pointer, pointer += n);
          } on RangeError {
            throw new ArgumentError('Delta length ($pointer)'
                ' larger than source text length (${text1.length}).');
          }
          if (token[0] == '=') {
            diffs.add(new Diff(Operation.equal, text));
          } else {
            diffs.add(new Diff(Operation.delete, text));
          }
          break;
        default:
          // Anything else is an error.
          throw new ArgumentError(
              'Invalid diff operation in diff_fromDelta: ${token[0]}');
      }
    }
    if (pointer != text1.length) {
      throw new ArgumentError('Delta length ($pointer)'
          ' smaller than source text length (${text1.length}).');
    }
    return diffs;
  }

  //  MATCH FUNCTIONS

  /**
   * Locate the best instance of 'pattern' in 'text' near 'loc'.
   * Returns -1 if no match found.
   * [text] is the text to search.
   * [pattern] is the pattern to search for.
   * [loc] is the location to search around.
   * Returns the best match index or -1.
   */
  int match_main(String text, String pattern, int loc) {
    // Check for null inputs.
    if (text == null || pattern == null) {
      throw new ArgumentError('Null inputs. (match_main)');
    }

    loc = max(0, min(loc, text.length));
    if (text == pattern) {
      // Shortcut (potentially not guaranteed by the algorithm)
      return 0;
    } else if (text.length == 0) {
      // Nothing to match.
      return -1;
    } else if (loc + pattern.length <= text.length &&
        text.substring(loc, loc + pattern.length) == pattern) {
      // Perfect match at the perfect spot!  (Includes case of null pattern)
      return loc;
    } else {
      // Do a fuzzy compare.
      return _match_bitap(text, pattern, loc);
    }
  }

  /**
   * Locate the best instance of 'pattern' in 'text' near 'loc' using the
   * Bitap algorithm.  Returns -1 if no match found.
   * [text] is the the text to search.
   * [pattern] is the pattern to search for.
   * [loc] is the location to search around.
   * Returns the best match index or -1.
   */
  int _match_bitap(String text, String pattern, int loc) {
    if (Match_MaxBits != 0 && pattern.length > Match_MaxBits) {
      throw new Exception('Pattern too long for this application.');
    }
    // Initialise the alphabet.
    Map<String, int> s = _match_alphabet(pattern);

    // Highest score beyond which we give up.
    double score_threshold = Match_Threshold;
    // Is there a nearby exact match? (speedup)
    int best_loc = text.indexOf(pattern, loc);
    if (best_loc != -1) {
      score_threshold =
          min(_match_bitapScore(0, best_loc, loc, pattern), score_threshold);
      // What about in the other direction? (speedup)
      best_loc = text.lastIndexOf(pattern, loc + pattern.length);
      if (best_loc != -1) {
        score_threshold =
            min(_match_bitapScore(0, best_loc, loc, pattern), score_threshold);
      }
    }

    // Initialise the bit arrays.
    final matchmask = 1 << (pattern.length - 1);
    best_loc = -1;

    int bin_min, bin_mid;
    int bin_max = pattern.length + text.length;
    List<int> last_rd;
    for (int d = 0; d < pattern.length; d++) {
      // Scan for the best match; each iteration allows for one more error.
      // Run a binary search to determine how far from 'loc' we can stray at
      // this error level.
      bin_min = 0;
      bin_mid = bin_max;
      while (bin_min < bin_mid) {
        if (_match_bitapScore(d, loc + bin_mid, loc, pattern) <=
            score_threshold) {
          bin_min = bin_mid;
        } else {
          bin_max = bin_mid;
        }
        bin_mid = ((bin_max - bin_min) / 2 + bin_min).toInt();
      }
      // Use the result from this iteration as the maximum for the next.
      bin_max = bin_mid;
      int start = max(1, loc - bin_mid + 1);
      int finish = min(loc + bin_mid, text.length) + pattern.length;

      final rd = new List<int>(finish + 2);
      rd[finish + 1] = (1 << d) - 1;
      for (int j = finish; j >= start; j--) {
        int charMatch;
        if (text.length <= j - 1 || !s.containsKey(text[j - 1])) {
          // Out of range.
          charMatch = 0;
        } else {
          charMatch = s[text[j - 1]];
        }
        if (d == 0) {
          // First pass: exact match.
          rd[j] = ((rd[j + 1] << 1) | 1) & charMatch;
        } else {
          // Subsequent passes: fuzzy match.
          rd[j] = ((rd[j + 1] << 1) | 1) & charMatch |
              (((last_rd[j + 1] | last_rd[j]) << 1) | 1) |
              last_rd[j + 1];
        }
        if ((rd[j] & matchmask) != 0) {
          double score = _match_bitapScore(d, j - 1, loc, pattern);
          // This match will almost certainly be better than any existing
          // match.  But check anyway.
          if (score <= score_threshold) {
            // Told you so.
            score_threshold = score;
            best_loc = j - 1;
            if (best_loc > loc) {
              // When passing loc, don't exceed our current distance from loc.
              start = max(1, 2 * loc - best_loc);
            } else {
              // Already passed loc, downhill from here on in.
              break;
            }
          }
        }
      }
      if (_match_bitapScore(d + 1, loc, loc, pattern) > score_threshold) {
        // No hope for a (better) match at greater error levels.
        break;
      }
      last_rd = rd;
    }
    return best_loc;
  }

  /**
   * Hack to allow unit tests to call private method.  Do not use.
   */
  int test_match_bitap(String text, String pattern, int loc) {
    return _match_bitap(text, pattern, loc);
  }

  /**
   * Compute and return the score for a match with e errors and x location.
   * [e] is the number of errors in match.
   * [x] is the location of match.
   * [loc] is the expected location of match.
   * [pattern] is the pattern being sought.
   * Returns the overall score for match (0.0 = good, 1.0 = bad).
   */
  double _match_bitapScore(int e, int x, int loc, String pattern) {
    final accuracy = e / pattern.length;
    final proximity = (loc - x).abs();
    if (Match_Distance == 0) {
      // Dodge divide by zero error.
      return proximity == 0 ? accuracy : 1.0;
    }
    return accuracy + proximity / Match_Distance;
  }

  /**
   * Initialise the alphabet for the Bitap algorithm.
   * [pattern] is the the text to encode.
   * Returns a Map of character locations.
   */
  Map<String, int> _match_alphabet(String pattern) {
    final s = new HashMap<String, int>();
    for (int i = 0; i < pattern.length; i++) {
      s[pattern[i]] = 0;
    }
    for (int i = 0; i < pattern.length; i++) {
      s[pattern[i]] = s[pattern[i]] | (1 << (pattern.length - i - 1));
    }
    return s;
  }

  /**
   * Hack to allow unit tests to call private method.  Do not use.
   */
  Map<String, int> test_match_alphabet(String pattern) {
    return _match_alphabet(pattern);
  }

  //  PATCH FUNCTIONS

  /**
   * Increase the context until it is unique,
   * but don't let the pattern expand beyond Match_MaxBits.
   * [patch] is the phe patch to grow.
   * [text] is the source text.
   */
  void _patch_addContext(Patch patch, String text) {
    if (text.isEmpty) {
      return;
    }
    String pattern = text.substring(patch.start2, patch.start2 + patch.length1);
    int padding = 0;

    // Look for the first and last matches of pattern in text.  If two different
    // matches are found, increase the pattern length.
    while (text.indexOf(pattern) != text.lastIndexOf(pattern) &&
        pattern.length < Match_MaxBits - Patch_Margin - Patch_Margin) {
      padding += Patch_Margin;
      pattern = text.substring(max(0, patch.start2 - padding),
          min(text.length, patch.start2 + patch.length1 + padding));
    }
    // Add one chunk for good luck.
    padding += Patch_Margin;

    // Add the prefix.
    final prefix = text.substring(max(0, patch.start2 - padding), patch.start2);
    if (!prefix.isEmpty) {
      patch.diffs.insert(0, new Diff(Operation.equal, prefix));
    }
    // Add the suffix.
    final suffix = text.substring(patch.start2 + patch.length1,
        min(text.length, patch.start2 + patch.length1 + padding));
    if (!suffix.isEmpty) {
      patch.diffs.add(new Diff(Operation.equal, suffix));
    }

    // Roll back the start points.
    patch.start1 -= prefix.length;
    patch.start2 -= prefix.length;
    // Extend the lengths.
    patch.length1 += prefix.length + suffix.length;
    patch.length2 += prefix.length + suffix.length;
  }

  /**
   * Hack to allow unit tests to call private method.  Do not use.
   */
  void test_patch_addContext(Patch patch, String text) {
    _patch_addContext(patch, text);
  }

  /**
   * Compute a list of patches to turn text1 into text2.
   * Use diffs if provided, otherwise compute it ourselves.
   * There are four ways to call this function, depending on what data is
   * available to the caller:
   * Method 1:
   * [a] = text1, [opt_b] = text2
   * Method 2:
   * [a] = diffs
   * Method 3 (optimal):
   * [a] = text1, [opt_b] = diffs
   * Method 4 (deprecated, use method 3):
   * [a] = text1, [opt_b] = text2, [opt_c] = diffs
   * Returns a List of Patch objects.
   */
  List<Patch> patch_make(a, [opt_b, opt_c]) {
    String text1;
    List<Diff> diffs;
    if (a is String && opt_b is String && opt_c == null) {
      // Method 1: text1, text2
      // Compute diffs from text1 and text2.
      text1 = a;
      diffs = diff_main(text1, opt_b, true);
      if (diffs.length > 2) {
        diff_cleanupSemantic(diffs);
        diff_cleanupEfficiency(diffs);
      }
    } else if (a is List && opt_b == null && opt_c == null) {
      // Method 2: diffs
      // Compute text1 from diffs.
      diffs = a;
      text1 = diff_text1(diffs);
    } else if (a is String && opt_b is List && opt_c == null) {
      // Method 3: text1, diffs
      text1 = a;
      diffs = opt_b;
    } else if (a is String && opt_b is String && opt_c is List) {
      // Method 4: text1, text2, diffs
      // text2 is not used.
      text1 = a;
      diffs = opt_c;
    } else {
      throw new ArgumentError('Unknown call format to patch_make.');
    }

    final patches = <Patch>[];
    if (diffs.isEmpty) {
      return patches; // Get rid of the null case.
    }
    Patch patch = new Patch();
    int char_count1 = 0; // Number of characters into the text1 string.
    int char_count2 = 0; // Number of characters into the text2 string.
    // Start with text1 (prepatch_text) and apply the diffs until we arrive at
    // text2 (postpatch_text). We recreate the patches one by one to determine
    // context info.
    String prepatch_text = text1;
    String postpatch_text = text1;
    for (Diff aDiff in diffs) {
      if (patch.diffs.isEmpty && aDiff.operation != Operation.equal) {
        // A new patch starts here.
        patch.start1 = char_count1;
        patch.start2 = char_count2;
      }

      switch (aDiff.operation) {
        case Operation.insert:
          patch.diffs.add(aDiff);
          patch.length2 += aDiff.text.length;
          postpatch_text = postpatch_text.substring(0, char_count2) +
              aDiff.text +
              postpatch_text.substring(char_count2);
          break;
        case Operation.delete:
          patch.length1 += aDiff.text.length;
          patch.diffs.add(aDiff);
          postpatch_text = postpatch_text.substring(0, char_count2) +
              postpatch_text.substring(char_count2 + aDiff.text.length);
          break;
        case Operation.equal:
          if (aDiff.text.length <= 2 * Patch_Margin &&
              !patch.diffs.isEmpty &&
              aDiff != diffs.last) {
            // Small equality inside a patch.
            patch.diffs.add(aDiff);
            patch.length1 += aDiff.text.length;
            patch.length2 += aDiff.text.length;
          }

          if (aDiff.text.length >= 2 * Patch_Margin) {
            // Time for a new patch.
            if (!patch.diffs.isEmpty) {
              _patch_addContext(patch, prepatch_text);
              patches.add(patch);
              patch = new Patch();
              // Unlike Unidiff, our patch lists have a rolling context.
              // https://github.com/google/diff-match-patch/wiki/Unidiff
              // Update prepatch text & pos to reflect the application of the
              // just completed patch.
              prepatch_text = postpatch_text;
              char_count1 = char_count2;
            }
          }
          break;
      }

      // Update the current character count.
      if (aDiff.operation != Operation.insert) {
        char_count1 += aDiff.text.length;
      }
      if (aDiff.operation != Operation.delete) {
        char_count2 += aDiff.text.length;
      }
    }
    // Pick up the leftover patch if not empty.
    if (!patch.diffs.isEmpty) {
      _patch_addContext(patch, prepatch_text);
      patches.add(patch);
    }

    return patches;
  }

  /**
   * Given an array of patches, return another array that is identical.
   * [patches] is a List of Patch objects.
   * Returns a List of Patch objects.
   */
  List<Patch> patch_deepCopy(List<Patch> patches) {
    final patchesCopy = <Patch>[];
    for (Patch aPatch in patches) {
      final patchCopy = new Patch();
      for (Diff aDiff in aPatch.diffs) {
        patchCopy.diffs.add(new Diff(aDiff.operation, aDiff.text));
      }
      patchCopy.start1 = aPatch.start1;
      patchCopy.start2 = aPatch.start2;
      patchCopy.length1 = aPatch.length1;
      patchCopy.length2 = aPatch.length2;
      patchesCopy.add(patchCopy);
    }
    return patchesCopy;
  }

  /**
   * Merge a set of patches onto the text.  Return a patched text, as well
   * as an array of true/false values indicating which patches were applied.
   * [patches] is a List of Patch objects
   * [text] is the old text.
   * Returns a two element List, containing the new text and a List of
   *      bool values.
   */
  List patch_apply(List<Patch> patches, String text) {
    if (patches.isEmpty) {
      return [text, []];
    }

    // Deep copy the patches so that no changes are made to originals.
    patches = patch_deepCopy(patches);

    final nullPadding = patch_addPadding(patches);
    text = nullPadding + text + nullPadding;
    patch_splitMax(patches);

    int x = 0;
    // delta keeps track of the offset between the expected and actual location
    // of the previous patch.  If there are patches expected at positions 10 and
    // 20, but the first patch was found at 12, delta is 2 and the second patch
    // has an effective expected position of 22.
    int delta = 0;
    final results = new List<bool>(patches.length);
    for (Patch aPatch in patches) {
      int expected_loc = aPatch.start2 + delta;
      String text1 = diff_text1(aPatch.diffs);
      int start_loc;
      int end_loc = -1;
      if (text1.length > Match_MaxBits) {
        // patch_splitMax will only provide an oversized pattern in the case of
        // a monster delete.
        start_loc =
            match_main(text, text1.substring(0, Match_MaxBits), expected_loc);
        if (start_loc != -1) {
          end_loc = match_main(
              text,
              text1.substring(text1.length - Match_MaxBits),
              expected_loc + text1.length - Match_MaxBits);
          if (end_loc == -1 || start_loc >= end_loc) {
            // Can't find valid trailing context.  Drop this patch.
            start_loc = -1;
          }
        }
      } else {
        start_loc = match_main(text, text1, expected_loc);
      }
      if (start_loc == -1) {
        // No match found.  :(
        results[x] = false;
        // Subtract the delta for this failed patch from subsequent patches.
        delta -= aPatch.length2 - aPatch.length1;
      } else {
        // Found a match.  :)
        results[x] = true;
        delta = start_loc - expected_loc;
        String text2;
        if (end_loc == -1) {
          text2 = text.substring(
              start_loc, min(start_loc + text1.length, text.length));
        } else {
          text2 = text.substring(
              start_loc, min(end_loc + Match_MaxBits, text.length));
        }
        if (text1 == text2) {
          // Perfect match, just shove the replacement text in.
          text = text.substring(0, start_loc) +
              diff_text2(aPatch.diffs) +
              text.substring(start_loc + text1.length);
        } else {
          // Imperfect match.  Run a diff to get a framework of equivalent
          // indices.
          final diffs = diff_main(text1, text2, false);
          if (text1.length > Match_MaxBits &&
              diff_levenshtein(diffs) / text1.length > Patch_DeleteThreshold) {
            // The end points match, but the content is unacceptably bad.
            results[x] = false;
          } else {
            _diff_cleanupSemanticLossless(diffs);
            int index1 = 0;
            for (Diff aDiff in aPatch.diffs) {
              if (aDiff.operation != Operation.equal) {
                int index2 = diff_xIndex(diffs, index1);
                if (aDiff.operation == Operation.insert) {
                  // Insertion
                  text = text.substring(0, start_loc + index2) +
                      aDiff.text +
                      text.substring(start_loc + index2);
                } else if (aDiff.operation == Operation.delete) {
                  // Deletion
                  text = text.substring(0, start_loc + index2) +
                      text.substring(start_loc +
                          diff_xIndex(diffs, index1 + aDiff.text.length));
                }
              }
              if (aDiff.operation != Operation.delete) {
                index1 += aDiff.text.length;
              }
            }
          }
        }
      }
      x++;
    }
    // Strip the padding off.
    text = text.substring(nullPadding.length, text.length - nullPadding.length);
    return [text, results];
  }

  /**
   * Add some padding on text start and end so that edges can match something.
   * Intended to be called only from within patch_apply.
   * [patches] is a List of Patch objects.
   * Returns the padding string added to each side.
   */
  String patch_addPadding(List<Patch> patches) {
    final paddingLength = Patch_Margin;
    final paddingCodes = <int>[];
    for (int x = 1; x <= paddingLength; x++) {
      paddingCodes.add(x);
    }
    String nullPadding = new String.fromCharCodes(paddingCodes);

    // Bump all the patches forward.
    for (Patch aPatch in patches) {
      aPatch.start1 += paddingLength;
      aPatch.start2 += paddingLength;
    }

    // Add some padding on start of first diff.
    Patch patch = patches[0];
    List<Diff> diffs = patch.diffs;
    if (diffs.isEmpty || diffs[0].operation != Operation.equal) {
      // Add nullPadding equality.
      diffs.insert(0, new Diff(Operation.equal, nullPadding));
      patch.start1 -= paddingLength; // Should be 0.
      patch.start2 -= paddingLength; // Should be 0.
      patch.length1 += paddingLength;
      patch.length2 += paddingLength;
    } else if (paddingLength > diffs[0].text.length) {
      // Grow first equality.
      Diff firstDiff = diffs[0];
      int extraLength = paddingLength - firstDiff.text.length;
      firstDiff.text =
          nullPadding.substring(firstDiff.text.length) + firstDiff.text;
      patch.start1 -= extraLength;
      patch.start2 -= extraLength;
      patch.length1 += extraLength;
      patch.length2 += extraLength;
    }

    // Add some padding on end of last diff.
    patch = patches.last;
    diffs = patch.diffs;
    if (diffs.isEmpty || diffs.last.operation != Operation.equal) {
      // Add nullPadding equality.
      diffs.add(new Diff(Operation.equal, nullPadding));
      patch.length1 += paddingLength;
      patch.length2 += paddingLength;
    } else if (paddingLength > diffs.last.text.length) {
      // Grow last equality.
      Diff lastDiff = diffs.last;
      int extraLength = paddingLength - lastDiff.text.length;
      lastDiff.text = lastDiff.text + nullPadding.substring(0, extraLength);
      patch.length1 += extraLength;
      patch.length2 += extraLength;
    }

    return nullPadding;
  }

  /**
   * Look through the patches and break up any which are longer than the
   * maximum limit of the match algorithm.
   * Intended to be called only from within patch_apply.
   * [patches] is a List of Patch objects.
   */
  patch_splitMax(List<Patch> patches) {
    final patch_size = Match_MaxBits;
    for (var x = 0; x < patches.length; x++) {
      if (patches[x].length1 <= patch_size) {
        continue;
      }
      Patch bigpatch = patches[x];
      // Remove the big old patch.
      patches.removeAt(x--);
      int start1 = bigpatch.start1;
      int start2 = bigpatch.start2;
      String precontext = '';
      while (!bigpatch.diffs.isEmpty) {
        // Create one of several smaller patches.
        final patch = new Patch();
        bool empty = true;
        patch.start1 = start1 - precontext.length;
        patch.start2 = start2 - precontext.length;
        if (!precontext.isEmpty) {
          patch.length1 = patch.length2 = precontext.length;
          patch.diffs.add(new Diff(Operation.equal, precontext));
        }
        while (!bigpatch.diffs.isEmpty &&
            patch.length1 < patch_size - Patch_Margin) {
          Operation diff_type = bigpatch.diffs[0].operation;
          String diff_text = bigpatch.diffs[0].text;
          if (diff_type == Operation.insert) {
            // Insertions are harmless.
            patch.length2 += diff_text.length;
            start2 += diff_text.length;
            patch.diffs.add(bigpatch.diffs[0]);
            bigpatch.diffs.removeAt(0);
            empty = false;
          } else if (diff_type == Operation.delete &&
              patch.diffs.length == 1 &&
              patch.diffs[0].operation == Operation.equal &&
              diff_text.length > 2 * patch_size) {
            // This is a large deletion.  Let it pass in one chunk.
            patch.length1 += diff_text.length;
            start1 += diff_text.length;
            empty = false;
            patch.diffs.add(new Diff(diff_type, diff_text));
            bigpatch.diffs.removeAt(0);
          } else {
            // Deletion or equality.  Only take as much as we can stomach.
            diff_text = diff_text.substring(
                0,
                min(diff_text.length,
                    patch_size - patch.length1 - Patch_Margin));
            patch.length1 += diff_text.length;
            start1 += diff_text.length;
            if (diff_type == Operation.equal) {
              patch.length2 += diff_text.length;
              start2 += diff_text.length;
            } else {
              empty = false;
            }
            patch.diffs.add(new Diff(diff_type, diff_text));
            if (diff_text == bigpatch.diffs[0].text) {
              bigpatch.diffs.removeAt(0);
            } else {
              bigpatch.diffs[0].text =
                  bigpatch.diffs[0].text.substring(diff_text.length);
            }
          }
        }
        // Compute the head context for the next patch.
        precontext = diff_text2(patch.diffs);
        precontext =
            precontext.substring(max(0, precontext.length - Patch_Margin));
        // Append the end context for this patch.
        String postcontext;
        if (diff_text1(bigpatch.diffs).length > Patch_Margin) {
          postcontext = diff_text1(bigpatch.diffs).substring(0, Patch_Margin);
        } else {
          postcontext = diff_text1(bigpatch.diffs);
        }
        if (!postcontext.isEmpty) {
          patch.length1 += postcontext.length;
          patch.length2 += postcontext.length;
          if (!patch.diffs.isEmpty &&
              patch.diffs.last.operation == Operation.equal) {
            patch.diffs.last.text = patch.diffs.last.text + postcontext;
          } else {
            patch.diffs.add(new Diff(Operation.equal, postcontext));
          }
        }
        if (!empty) {
          patches.insert(++x, patch);
        }
      }
    }
  }

  /**
   * Take a list of patches and return a textual representation.
   * [patches] is a List of Patch objects.
   * Returns a text representation of patches.
   */
  String patch_toText(List<Patch> patches) {
    final text = new StringBuffer();
    text.writeAll(patches);
    return text.toString();
  }

  /**
   * Parse a textual representation of patches and return a List of Patch
   * objects.
   * [textline] is a text representation of patches.
   * Returns a List of Patch objects.
   * Throws ArgumentError if invalid input.
   */
  List<Patch> patch_fromText(String textline) {
    final patches = <Patch>[];
    if (textline.isEmpty) {
      return patches;
    }
    final text = textline.split('\n');
    int textPointer = 0;
    final patchHeader =
        new RegExp('^@@ -(\\d+),?(\\d*) \\+(\\d+),?(\\d*) @@\$');
    while (textPointer < text.length) {
      Match m = patchHeader.firstMatch(text[textPointer]);
      if (m == null) {
        throw new ArgumentError('Invalid patch string: ${text[textPointer]}');
      }
      final patch = new Patch();
      patches.add(patch);
      patch.start1 = int.parse(m.group(1));
      if (m.group(2).isEmpty) {
        patch.start1--;
        patch.length1 = 1;
      } else if (m.group(2) == '0') {
        patch.length1 = 0;
      } else {
        patch.start1--;
        patch.length1 = int.parse(m.group(2));
      }

      patch.start2 = int.parse(m.group(3));
      if (m.group(4).isEmpty) {
        patch.start2--;
        patch.length2 = 1;
      } else if (m.group(4) == '0') {
        patch.length2 = 0;
      } else {
        patch.start2--;
        patch.length2 = int.parse(m.group(4));
      }
      textPointer++;

      while (textPointer < text.length) {
        if (!text[textPointer].isEmpty) {
          final sign = text[textPointer][0];
          String line;
          try {
            line = Uri.decodeFull(text[textPointer].substring(1));
          } on ArgumentError {
            // Malformed URI sequence.
            throw new ArgumentError('Illegal escape in patch_fromText: $line');
          }
          if (sign == '-') {
            // Deletion.
            patch.diffs.add(new Diff(Operation.delete, line));
          } else if (sign == '+') {
            // Insertion.
            patch.diffs.add(new Diff(Operation.insert, line));
          } else if (sign == ' ') {
            // Minor equality.
            patch.diffs.add(new Diff(Operation.equal, line));
          } else if (sign == '@') {
            // Start of next patch.
            break;
          } else {
            // WTF?
            throw new ArgumentError('Invalid patch mode "$sign" in: $line');
          }
        }
        textPointer++;
      }
    }
    return patches;
  }
}
