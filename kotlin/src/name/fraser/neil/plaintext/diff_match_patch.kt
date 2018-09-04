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

package name.fraser.neil.plaintext

import java.io.UnsupportedEncodingException
import java.net.URLDecoder
import java.net.URLEncoder
import java.util.*
import java.util.regex.Matcher
import java.util.regex.Pattern

/*
 * Functions for diff, match and patch.
 * Computes the difference between two texts to create a patch.
 * Applies the patch onto another text, allowing for errors.
 *
 * @author fraser@google.com (Neil Fraser)
 */

/**
 * Class containing the diff, match and patch methods.
 * Also contains the behaviour settings.
 */
class diff_match_patch {

  // Defaults.
  // Set these on your diff_match_patch instance to override the defaults.

  /**
   * Number of seconds to map a diff before giving up (0 for infinity).
   */
  var Diff_Timeout = 1.0f
  /**
   * Cost of an empty edit operation in terms of edit characters.
   */
  var Diff_EditCost: Short = 4
  /**
   * At what point is no match declared (0.0 = perfection, 1.0 = very loose).
   */
  var Match_Threshold = 0.5f
  /**
   * How far to search for a match (0 = exact location, 1000+ = broad match).
   * A match this many characters away from the expected location will add
   * 1.0 to the score (0.0 is a perfect match).
   */
  var Match_Distance = 1000
  /**
   * When deleting a large block of text (over ~64 characters), how close do
   * the contents have to be to match the expected contents. (0.0 = perfection,
   * 1.0 = very loose).  Note that Match_Threshold controls how closely the
   * end points of a delete need to match.
   */
  var Patch_DeleteThreshold = 0.5f
  /**
   * Chunk size for context length.
   */
  var Patch_Margin: Short = 4

  /**
   * The number of bits in an int.
   */
  private val Match_MaxBits: Short = 32

  // Define some regex patterns for matching boundaries.
  private val BLANKLINEEND = Pattern.compile("\\n\\r?\\n\\Z", Pattern.DOTALL)
  private val BLANKLINESTART = Pattern.compile("\\A\\r?\\n\\r?\\n", Pattern.DOTALL)

  /**
   * Internal class for returning results from diff_linesToChars().
   * Other less paranoid languages just use a three-element array.
   */
  protected class LinesToCharsResult(
    var chars1: String, var chars2: String,
    var lineArray: List<String>
  )

  //  DIFF FUNCTIONS

  /**
   * The data structure representing a diff is a Linked list of Diff objects:
   * {Diff(Operation.DELETE, "Hello"), Diff(Operation.INSERT, "Goodbye"),
   * Diff(Operation.EQUAL, " world.")}
   * which means: delete "Hello", add "Goodbye" and keep " world."
   */
  enum class Operation {
    DELETE, INSERT, EQUAL
  }

  /**
   * Find the differences between two texts.
   * @param text1 Old string to be diffed.
   * @param text2 New string to be diffed.
   * @param checklines Speedup flag.  If false, then don't run a
   * line-level diff first to identify the changed areas.
   * If true, then run a faster slightly less optimal diff.
   * @return Linked List of Diff objects.
   */
  @JvmOverloads
  fun diff_main(
    text1: String, text2: String,
    checklines: Boolean = true
  ): LinkedList<Diff> {
    // Set a deadline by which time the diff must be complete.
    val deadline: Long
    if (Diff_Timeout <= 0) {
      deadline = java.lang.Long.MAX_VALUE
    } else {
      deadline = System.currentTimeMillis() + (Diff_Timeout * 1000).toLong()
    }
    return diff_main(text1, text2, checklines, deadline)
  }

  /**
   * Find the differences between two texts.  Simplifies the problem by
   * stripping any common prefix or suffix off the texts before diffing.
   * @param text1 Old string to be diffed.
   * @param text2 New string to be diffed.
   * @param checklines Speedup flag.  If false, then don't run a
   * line-level diff first to identify the changed areas.
   * If true, then run a faster slightly less optimal diff.
   * @param deadline Time when the diff should be complete by.  Used
   * internally for recursive calls.  Users should set DiffTimeout instead.
   * @return Linked List of Diff objects.
   */
  private fun diff_main(
    text1: String?, text2: String?,
    checklines: Boolean, deadline: Long
  ): LinkedList<Diff> {
    var text1 = text1
    var text2 = text2
    // Check for null inputs.
    if (text1 == null || text2 == null) {
      throw IllegalArgumentException("Null inputs. (diff_main)")
    }

    // Check for equality (speedup).
    val diffs: LinkedList<Diff>
    if (text1 == text2) {
      diffs = LinkedList()
      if (text1.length != 0) {
        diffs.add(Diff(Operation.EQUAL, text1))
      }
      return diffs
    }

    // Trim off common prefix (speedup).
    var commonlength = diff_commonPrefix(text1, text2)
    val commonprefix = text1.substring(0, commonlength)
    text1 = text1.substring(commonlength)
    text2 = text2.substring(commonlength)

    // Trim off common suffix (speedup).
    commonlength = diff_commonSuffix(text1, text2)
    val commonsuffix = text1.substring(text1.length - commonlength)
    text1 = text1.substring(0, text1.length - commonlength)
    text2 = text2.substring(0, text2.length - commonlength)

    // Compute the diff on the middle block.
    diffs = diff_compute(text1, text2, checklines, deadline)

    // Restore the prefix and suffix.
    if (commonprefix.length != 0) {
      diffs.addFirst(Diff(Operation.EQUAL, commonprefix))
    }
    if (commonsuffix.length != 0) {
      diffs.addLast(Diff(Operation.EQUAL, commonsuffix))
    }

    diff_cleanupMerge(diffs)
    return diffs
  }

  /**
   * Find the differences between two texts.  Assumes that the texts do not
   * have any common prefix or suffix.
   * @param text1 Old string to be diffed.
   * @param text2 New string to be diffed.
   * @param checklines Speedup flag.  If false, then don't run a
   * line-level diff first to identify the changed areas.
   * If true, then run a faster slightly less optimal diff.
   * @param deadline Time when the diff should be complete by.
   * @return Linked List of Diff objects.
   */
  private fun diff_compute(
    text1: String, text2: String,
    checklines: Boolean, deadline: Long
  ): LinkedList<Diff> {
    var diffs = LinkedList<Diff>()

    if (text1.length == 0) {
      // Just add some text (speedup).
      diffs.add(Diff(Operation.INSERT, text2))
      return diffs
    }

    if (text2.length == 0) {
      // Just delete some text (speedup).
      diffs.add(Diff(Operation.DELETE, text1))
      return diffs
    }

    val longtext = if (text1.length > text2.length) text1 else text2
    val shorttext = if (text1.length > text2.length) text2 else text1
    val i = longtext.indexOf(shorttext)
    if (i != -1) {
      // Shorter text is inside the longer text (speedup).
      val op = if (text1.length > text2.length)
        Operation.DELETE
      else
        Operation.INSERT
      diffs.add(Diff(op, longtext.substring(0, i)))
      diffs.add(Diff(Operation.EQUAL, shorttext))
      diffs.add(Diff(op, longtext.substring(i + shorttext.length)))
      return diffs
    }

    if (shorttext.length == 1) {
      // Single character string.
      // After the previous speedup, the character can't be an equality.
      diffs.add(Diff(Operation.DELETE, text1))
      diffs.add(Diff(Operation.INSERT, text2))
      return diffs
    }

    // Check to see if the problem can be split in two.
    val hm = diff_halfMatch(text1, text2)
    if (hm != null) {
      // A half-match was found, sort out the return data.
      val text1_a = hm[0]
      val text1_b = hm[1]
      val text2_a = hm[2]
      val text2_b = hm[3]
      val mid_common = hm[4]
      // Send both pairs off for separate processing.
      val diffs_a = diff_main(
        text1_a, text2_a,
        checklines, deadline
      )
      val diffs_b = diff_main(
        text1_b, text2_b,
        checklines, deadline
      )
      // Merge the results.
      diffs = diffs_a
      diffs.add(Diff(Operation.EQUAL, mid_common))
      diffs.addAll(diffs_b)
      return diffs
    }

    return if (checklines && text1.length > 100 && text2.length > 100) {
      diff_lineMode(text1, text2, deadline)
    } else diff_bisect(text1, text2, deadline)
  }

  /**
   * Do a quick line-level diff on both strings, then rediff the parts for
   * greater accuracy.
   * This speedup can produce non-minimal diffs.
   * @param text1 Old string to be diffed.
   * @param text2 New string to be diffed.
   * @param deadline Time when the diff should be complete by.
   * @return Linked List of Diff objects.
   */
  private fun diff_lineMode(
    text1: String, text2: String,
    deadline: Long
  ): LinkedList<Diff> {
    var text1 = text1
    var text2 = text2
    // Scan the text on a line-by-line basis first.
    val a = diff_linesToChars(text1, text2)
    text1 = a.chars1
    text2 = a.chars2
    val linearray = a.lineArray

    val diffs = diff_main(text1, text2, false, deadline)

    // Convert the diff back to original text.
    diff_charsToLines(diffs, linearray)
    // Eliminate freak matches (e.g. blank lines)
    diff_cleanupSemantic(diffs)

    // Rediff any replacement blocks, this time character-by-character.
    // Add a dummy entry at the end.
    diffs.add(Diff(Operation.EQUAL, ""))
    var count_delete = 0
    var count_insert = 0
    var text_delete = ""
    var text_insert = ""
    val pointer = diffs.listIterator()
    var thisDiff: Diff? = pointer.next()
    while (thisDiff != null) {
      when (thisDiff.operation) {
        diff_match_patch.Operation.INSERT -> {
          count_insert++
          text_insert += thisDiff.text
        }
        diff_match_patch.Operation.DELETE -> {
          count_delete++
          text_delete += thisDiff.text
        }
        diff_match_patch.Operation.EQUAL -> {
          // Upon reaching an equality, check for prior redundancies.
          if (count_delete >= 1 && count_insert >= 1) {
            // Delete the offending records and add the merged ones.
            pointer.previous()
            for (j in 0 until count_delete + count_insert) {
              pointer.previous()
              pointer.remove()
            }
            for (subDiff in diff_main(
              text_delete, text_insert, false,
              deadline
            )) {
              pointer.add(subDiff)
            }
          }
          count_insert = 0
          count_delete = 0
          text_delete = ""
          text_insert = ""
        }
      }
      thisDiff = if (pointer.hasNext()) pointer.next() else null
    }
    diffs.removeLast()  // Remove the dummy entry at the end.

    return diffs
  }

  /**
   * Find the 'middle snake' of a diff, split the problem in two
   * and return the recursively constructed diff.
   * See Myers 1986 paper: An O(ND) Difference Algorithm and Its Variations.
   * @param text1 Old string to be diffed.
   * @param text2 New string to be diffed.
   * @param deadline Time at which to bail if not yet complete.
   * @return LinkedList of Diff objects.
   */
  protected fun diff_bisect(
    text1: String, text2: String,
    deadline: Long
  ): LinkedList<Diff> {
    // Cache the text lengths to prevent multiple calls.
    val text1_length = text1.length
    val text2_length = text2.length
    val max_d = (text1_length + text2_length + 1) / 2
    val v_length = 2 * max_d
    val v1 = IntArray(v_length)
    val v2 = IntArray(v_length)
    for (x in 0 until v_length) {
      v1[x] = -1
      v2[x] = -1
    }
    v1[max_d + 1] = 0
    v2[max_d + 1] = 0
    val delta = text1_length - text2_length
    // If the total number of characters is odd, then the front path will
    // collide with the reverse path.
    val front = delta % 2 != 0
    // Offsets for start and end of k loop.
    // Prevents mapping of space beyond the grid.
    var k1start = 0
    var k1end = 0
    var k2start = 0
    var k2end = 0
    for (d in 0 until max_d) {
      // Bail out if deadline is reached.
      if (System.currentTimeMillis() > deadline) {
        break
      }

      // Walk the front path one step.
      var k1 = -d + k1start
      while (k1 <= d - k1end) {
        val k1_offset = max_d + k1
        var x1: Int
        if (k1 == -d || k1 != d && v1[k1_offset - 1] < v1[k1_offset + 1]) {
          x1 = v1[k1_offset + 1]
        } else {
          x1 = v1[k1_offset - 1] + 1
        }
        var y1 = x1 - k1
        while (x1 < text1_length && y1 < text2_length
          && text1[x1] == text2[y1]
        ) {
          x1++
          y1++
        }
        v1[k1_offset] = x1
        if (x1 > text1_length) {
          // Ran off the right of the graph.
          k1end += 2
        } else if (y1 > text2_length) {
          // Ran off the bottom of the graph.
          k1start += 2
        } else if (front) {
          val k2_offset = max_d + delta - k1
          if (k2_offset >= 0 && k2_offset < v_length && v2[k2_offset] != -1) {
            // Mirror x2 onto top-left coordinate system.
            val x2 = text1_length - v2[k2_offset]
            if (x1 >= x2) {
              // Overlap detected.
              return diff_bisectSplit(text1, text2, x1, y1, deadline)
            }
          }
        }
        k1 += 2
      }

      // Walk the reverse path one step.
      var k2 = -d + k2start
      while (k2 <= d - k2end) {
        val k2_offset = max_d + k2
        var x2: Int
        if (k2 == -d || k2 != d && v2[k2_offset - 1] < v2[k2_offset + 1]) {
          x2 = v2[k2_offset + 1]
        } else {
          x2 = v2[k2_offset - 1] + 1
        }
        var y2 = x2 - k2
        while (x2 < text1_length && y2 < text2_length
          && text1[text1_length - x2 - 1] == text2[text2_length - y2 - 1]
        ) {
          x2++
          y2++
        }
        v2[k2_offset] = x2
        if (x2 > text1_length) {
          // Ran off the left of the graph.
          k2end += 2
        } else if (y2 > text2_length) {
          // Ran off the top of the graph.
          k2start += 2
        } else if (!front) {
          val k1_offset = max_d + delta - k2
          if (k1_offset >= 0 && k1_offset < v_length && v1[k1_offset] != -1) {
            val x1 = v1[k1_offset]
            val y1 = max_d + x1 - k1_offset
            // Mirror x2 onto top-left coordinate system.
            x2 = text1_length - x2
            if (x1 >= x2) {
              // Overlap detected.
              return diff_bisectSplit(text1, text2, x1, y1, deadline)
            }
          }
        }
        k2 += 2
      }
    }
    // Diff took too long and hit the deadline or
    // number of diffs equals number of characters, no commonality at all.
    val diffs = LinkedList<Diff>()
    diffs.add(Diff(Operation.DELETE, text1))
    diffs.add(Diff(Operation.INSERT, text2))
    return diffs
  }

  /**
   * Given the location of the 'middle snake', split the diff in two parts
   * and recurse.
   * @param text1 Old string to be diffed.
   * @param text2 New string to be diffed.
   * @param x Index of split point in text1.
   * @param y Index of split point in text2.
   * @param deadline Time at which to bail if not yet complete.
   * @return LinkedList of Diff objects.
   */
  private fun diff_bisectSplit(
    text1: String, text2: String,
    x: Int, y: Int, deadline: Long
  ): LinkedList<Diff> {
    val text1a = text1.substring(0, x)
    val text2a = text2.substring(0, y)
    val text1b = text1.substring(x)
    val text2b = text2.substring(y)

    // Compute both diffs serially.
    val diffs = diff_main(text1a, text2a, false, deadline)
    val diffsb = diff_main(text1b, text2b, false, deadline)

    diffs.addAll(diffsb)
    return diffs
  }

  /**
   * Split two texts into a list of strings.  Reduce the texts to a string of
   * hashes where each Unicode character represents one line.
   * @param text1 First string.
   * @param text2 Second string.
   * @return An object containing the encoded text1, the encoded text2 and
   * the List of unique strings.  The zeroth element of the List of
   * unique strings is intentionally blank.
   */
  protected fun diff_linesToChars(text1: String, text2: String): LinesToCharsResult {
    val lineArray = ArrayList<String>()
    val lineHash = HashMap<String, Int>()
    // e.g. linearray[4] == "Hello\n"
    // e.g. linehash.get("Hello\n") == 4

    // "\x00" is a valid character, but various debuggers don't like it.
    // So we'll insert a junk entry to avoid generating a null character.
    lineArray.add("")

    // Allocate 2/3rds of the space for text1, the rest for text2.
    val chars1 = diff_linesToCharsMunge(text1, lineArray, lineHash, 40000)
    val chars2 = diff_linesToCharsMunge(text2, lineArray, lineHash, 65535)
    return LinesToCharsResult(chars1, chars2, lineArray)
  }

  /**
   * Split a text into a list of strings.  Reduce the texts to a string of
   * hashes where each Unicode character represents one line.
   * @param text String to encode.
   * @param lineArray List of unique strings.
   * @param lineHash Map of strings to indices.
   * @param maxLines Maximum length of lineArray.
   * @return Encoded string.
   */
  private fun diff_linesToCharsMunge(
    text: String, lineArray: MutableList<String>,
    lineHash: MutableMap<String, Int>, maxLines: Int
  ): String {
    var lineStart = 0
    var lineEnd = -1
    var line: String
    val chars = StringBuilder()
    // Walk the text, pulling out a substring for each line.
    // text.split('\n') would would temporarily double our memory footprint.
    // Modifying text would create many large strings to garbage collect.
    while (lineEnd < text.length - 1) {
      lineEnd = text.indexOf('\n', lineStart)
      if (lineEnd == -1) {
        lineEnd = text.length - 1
      }
      line = text.substring(lineStart, lineEnd + 1)

      if (lineHash.containsKey(line)) {
        chars.append((lineHash[line] as Int).toChar().toString())
      } else {
        if (lineArray.size == maxLines) {
          // Bail out at 65535 because
          // String.valueOf((char) 65536).equals(String.valueOf(((char) 0)))
          line = text.substring(lineStart)
          lineEnd = text.length
        }
        lineArray.add(line)
        lineHash[line] = lineArray.size - 1
        chars.append((lineArray.size - 1).toChar().toString())
      }
      lineStart = lineEnd + 1
    }
    return chars.toString()
  }

  /**
   * Rehydrate the text in a diff from a string of line hashes to real lines of
   * text.
   * @param diffs List of Diff objects.
   * @param lineArray List of unique strings.
   */
  protected fun diff_charsToLines(
    diffs: List<Diff>,
    lineArray: List<String>
  ) {
    var text: StringBuilder
    for (diff in diffs) {
      text = StringBuilder()
      for (j in 0 until diff.text!!.length) {
        text.append(lineArray[diff.text!![j].toInt()])
      }
      diff.text = text.toString()
    }
  }

  /**
   * Determine the common prefix of two strings
   * @param text1 First string.
   * @param text2 Second string.
   * @return The number of characters common to the start of each string.
   */
  fun diff_commonPrefix(text1: String, text2: String): Int {
    // Performance analysis: https://neil.fraser.name/news/2007/10/09/
    val n = Math.min(text1.length, text2.length)
    for (i in 0 until n) {
      if (text1[i] != text2[i]) {
        return i
      }
    }
    return n
  }

  /**
   * Determine the common suffix of two strings
   * @param text1 First string.
   * @param text2 Second string.
   * @return The number of characters common to the end of each string.
   */
  fun diff_commonSuffix(text1: String?, text2: String?): Int {
    // Performance analysis: https://neil.fraser.name/news/2007/10/09/
    val text1_length = text1!!.length
    val text2_length = text2!!.length
    val n = Math.min(text1_length, text2_length)
    for (i in 1..n) {
      if (text1[text1_length - i] != text2[text2_length - i]) {
        return i - 1
      }
    }
    return n
  }

  /**
   * Determine if the suffix of one string is the prefix of another.
   * @param text1 First string.
   * @param text2 Second string.
   * @return The number of characters common to the end of the first
   * string and the start of the second string.
   */
  protected fun diff_commonOverlap(text1: String, text2: String): Int {
    var text1 = text1
    var text2 = text2
    // Cache the text lengths to prevent multiple calls.
    val text1_length = text1.length
    val text2_length = text2.length
    // Eliminate the null case.
    if (text1_length == 0 || text2_length == 0) {
      return 0
    }
    // Truncate the longer string.
    if (text1_length > text2_length) {
      text1 = text1.substring(text1_length - text2_length)
    } else if (text1_length < text2_length) {
      text2 = text2.substring(0, text1_length)
    }
    val text_length = Math.min(text1_length, text2_length)
    // Quick check for the worst case.
    if (text1 == text2) {
      return text_length
    }

    // Start by looking for a single character match
    // and increase length until no match is found.
    // Performance analysis: https://neil.fraser.name/news/2010/11/04/
    var best = 0
    var length = 1
    while (true) {
      val pattern = text1.substring(text_length - length)
      val found = text2.indexOf(pattern)
      if (found == -1) {
        return best
      }
      length += found
      if (found == 0 || text1.substring(text_length - length) == text2.substring(0, length)) {
        best = length
        length++
      }
    }
  }

  /**
   * Do the two texts share a substring which is at least half the length of
   * the longer text?
   * This speedup can produce non-minimal diffs.
   * @param text1 First string.
   * @param text2 Second string.
   * @return Five element String array, containing the prefix of text1, the
   * suffix of text1, the prefix of text2, the suffix of text2 and the
   * common middle.  Or null if there was no match.
   */
  protected fun diff_halfMatch(text1: String, text2: String): Array<String>? {
    if (Diff_Timeout <= 0) {
      // Don't risk returning a non-optimal diff if we have unlimited time.
      return null
    }
    val longtext = if (text1.length > text2.length) text1 else text2
    val shorttext = if (text1.length > text2.length) text2 else text1
    if (longtext.length < 4 || shorttext.length * 2 < longtext.length) {
      return null  // Pointless.
    }

    // First check if the second quarter is the seed for a half-match.
    val hm1 = diff_halfMatchI(
      longtext, shorttext,
      (longtext.length + 3) / 4
    )
    // Check again based on the third quarter.
    val hm2 = diff_halfMatchI(
      longtext, shorttext,
      (longtext.length + 1) / 2
    )
    val hm: Array<String>
    hm = if (hm1 == null && hm2 == null) {
      return null
    } else if (hm2 == null) {
      hm1!!
    } else if (hm1 == null) {
      hm2
    } else {
      // Both matched.  Select the longest.
      if (hm1[4].length > hm2[4].length) hm1 else hm2
    }

    // A half-match was found, sort out the return data.
    return if (text1.length > text2.length) {
      hm
      //return new String[]{hm[0], hm[1], hm[2], hm[3], hm[4]};
    } else {
      arrayOf(hm[2], hm[3], hm[0], hm[1], hm[4])
    }
  }

  /**
   * Does a substring of shorttext exist within longtext such that the
   * substring is at least half the length of longtext?
   * @param longtext Longer string.
   * @param shorttext Shorter string.
   * @param i Start index of quarter length substring within longtext.
   * @return Five element String array, containing the prefix of longtext, the
   * suffix of longtext, the prefix of shorttext, the suffix of shorttext
   * and the common middle.  Or null if there was no match.
   */
  private fun diff_halfMatchI(longtext: String, shorttext: String, i: Int): Array<String>? {
    // Start with a 1/4 length substring at position i as a seed.
    val seed = longtext.substring(i, i + longtext.length / 4)
    var j = -1
    var best_common = ""
    var best_longtext_a = ""
    var best_longtext_b = ""
    var best_shorttext_a = ""
    var best_shorttext_b = ""
    val run = { j = shorttext.indexOf(seed, j + 1); j }
    while (run() != -1) {
      val prefixLength = diff_commonPrefix(
        longtext.substring(i),
        shorttext.substring(j)
      )
      val suffixLength = diff_commonSuffix(
        longtext.substring(0, i),
        shorttext.substring(0, j)
      )
      if (best_common.length < suffixLength + prefixLength) {
        best_common = shorttext.substring(j - suffixLength, j) + shorttext.substring(j, j + prefixLength)
        best_longtext_a = longtext.substring(0, i - suffixLength)
        best_longtext_b = longtext.substring(i + prefixLength)
        best_shorttext_a = shorttext.substring(0, j - suffixLength)
        best_shorttext_b = shorttext.substring(j + prefixLength)
      }
    }
    return if (best_common.length * 2 >= longtext.length) {
      arrayOf(best_longtext_a, best_longtext_b, best_shorttext_a, best_shorttext_b, best_common)
    } else {
      null
    }
  }

  /**
   * Reduce the number of edits by eliminating semantically trivial equalities.
   * @param diffs LinkedList of Diff objects.
   */
  fun diff_cleanupSemantic(diffs: LinkedList<Diff>) {
    if (diffs.isEmpty()) {
      return
    }
    var changes = false
    val equalities = ArrayDeque<Diff>()  // Double-ended queue of qualities.
    var lastEquality: String? = null // Always equal to equalities.peek().text
    var pointer: MutableListIterator<Diff> = diffs.listIterator()
    // Number of characters that changed prior to the equality.
    var length_insertions1 = 0
    var length_deletions1 = 0
    // Number of characters that changed after the equality.
    var length_insertions2 = 0
    var length_deletions2 = 0
    var thisDiff: Diff? = pointer.next()
    while (thisDiff != null) {
      if (thisDiff.operation == Operation.EQUAL) {
        // Equality found.
        equalities.push(thisDiff)
        length_insertions1 = length_insertions2
        length_deletions1 = length_deletions2
        length_insertions2 = 0
        length_deletions2 = 0
        lastEquality = thisDiff.text
      } else {
        // An insertion or deletion.
        if (thisDiff.operation == Operation.INSERT) {
          length_insertions2 += thisDiff.text!!.length
        } else {
          length_deletions2 += thisDiff.text!!.length
        }
        // Eliminate an equality that is smaller or equal to the edits on both
        // sides of it.
        if (lastEquality != null && lastEquality.length <= Math.max(length_insertions1, length_deletions1)
          && lastEquality.length <= Math.max(length_insertions2, length_deletions2)
        ) {
          //System.out.println("Splitting: '" + lastEquality + "'");
          // Walk back to offending equality.
          while (thisDiff !== equalities.peek()) {
            thisDiff = pointer.previous()
          }
          pointer.next()

          // Replace equality with a delete.
          pointer.set(Diff(Operation.DELETE, lastEquality))
          // Insert a corresponding an insert.
          pointer.add(Diff(Operation.INSERT, lastEquality))

          equalities.pop()  // Throw away the equality we just deleted.
          if (!equalities.isEmpty()) {
            // Throw away the previous equality (it needs to be reevaluated).
            equalities.pop()
          }
          if (equalities.isEmpty()) {
            // There are no previous equalities, walk back to the start.
            while (pointer.hasPrevious()) {
              pointer.previous()
            }
          } else {
            // There is a safe equality we can fall back to.
            thisDiff = equalities.peek()
            while (thisDiff !== pointer.previous()) {
              // Intentionally empty loop.
            }
          }

          length_insertions1 = 0  // Reset the counters.
          length_insertions2 = 0
          length_deletions1 = 0
          length_deletions2 = 0
          lastEquality = null
          changes = true
        }
      }
      thisDiff = if (pointer.hasNext()) pointer.next() else null
    }

    // Normalize the diff.
    if (changes) {
      diff_cleanupMerge(diffs)
    }
    diff_cleanupSemanticLossless(diffs)

    // Find any overlaps between deletions and insertions.
    // e.g: <del>abcxxx</del><ins>xxxdef</ins>
    //   -> <del>abc</del>xxx<ins>def</ins>
    // e.g: <del>xxxabc</del><ins>defxxx</ins>
    //   -> <ins>def</ins>xxx<del>abc</del>
    // Only extract an overlap if it is as big as the edit ahead or behind it.
    pointer = diffs.listIterator()
    var prevDiff: Diff? = null
    thisDiff = null
    if (pointer.hasNext()) {
      prevDiff = pointer.next()
      if (pointer.hasNext()) {
        thisDiff = pointer.next()
      }
    }
    while (thisDiff != null) {
      if (prevDiff!!.operation == Operation.DELETE && thisDiff.operation == Operation.INSERT) {
        val deletion = prevDiff.text
        val insertion = thisDiff.text
        val overlap_length1 = this.diff_commonOverlap(deletion!!, insertion!!)
        val overlap_length2 = this.diff_commonOverlap(insertion, deletion)
        if (overlap_length1 >= overlap_length2) {
          if (overlap_length1 >= deletion!!.length / 2.0 || overlap_length1 >= insertion.length / 2.0) {
            // Overlap found. Insert an equality and trim the surrounding edits.
            pointer.previous()
            pointer.add(
              Diff(
                Operation.EQUAL,
                insertion!!.substring(0, overlap_length1)
              )
            )
            prevDiff.text = deletion.substring(0, deletion.length - overlap_length1)
            thisDiff.text = insertion.substring(overlap_length1)
            // pointer.add inserts the element before the cursor, so there is
            // no need to step past the new element.
          }
        } else {
          if (overlap_length2 >= deletion!!.length / 2.0 || overlap_length2 >= insertion.length / 2.0) {
            // Reverse overlap found.
            // Insert an equality and swap and trim the surrounding edits.
            pointer.previous()
            pointer.add(
              Diff(
                Operation.EQUAL,
                deletion.substring(0, overlap_length2)
              )
            )
            prevDiff.operation = Operation.INSERT
            prevDiff.text = insertion!!.substring(0, insertion.length - overlap_length2)
            thisDiff.operation = Operation.DELETE
            thisDiff.text = deletion.substring(overlap_length2)
            // pointer.add inserts the element before the cursor, so there is
            // no need to step past the new element.
          }
        }
        thisDiff = if (pointer.hasNext()) pointer.next() else null
      }
      prevDiff = thisDiff
      thisDiff = if (pointer.hasNext()) pointer.next() else null
    }
  }

  /**
   * Look for single edits surrounded on both sides by equalities
   * which can be shifted sideways to align the edit to a word boundary.
   * e.g: The c<ins>at c</ins>ame. -> The <ins>cat </ins>came.
   * @param diffs LinkedList of Diff objects.
   */
  fun diff_cleanupSemanticLossless(diffs: LinkedList<Diff>) {
    var equality1: String?
    var edit: String?
    var equality2: String?
    var commonString: String
    var commonOffset: Int
    var score: Int
    var bestScore: Int
    var bestEquality1: String
    var bestEdit: String
    var bestEquality2: String
    // Create a new iterator at the start.
    val pointer = diffs.listIterator()
    var prevDiff: Diff? = if (pointer.hasNext()) pointer.next() else null
    var thisDiff: Diff? = if (pointer.hasNext()) pointer.next() else null
    var nextDiff: Diff? = if (pointer.hasNext()) pointer.next() else null
    // Intentionally ignore the first and last element (don't need checking).
    while (nextDiff != null) {
      if (prevDiff!!.operation == Operation.EQUAL && nextDiff.operation == Operation.EQUAL) {
        // This is a single edit surrounded by equalities.
        equality1 = prevDiff.text
        edit = thisDiff!!.text
        equality2 = nextDiff.text

        // First, shift the edit as far left as possible.
        commonOffset = diff_commonSuffix(equality1, edit)
        if (commonOffset != 0) {
          commonString = edit!!.substring(edit.length - commonOffset)
          equality1 = equality1!!.substring(0, equality1.length - commonOffset)
          edit = commonString + edit.substring(0, edit.length - commonOffset)
          equality2 = commonString + equality2!!
        }

        // Second, step character by character right, looking for the best fit.
        bestEquality1 = equality1!!
        bestEdit = edit!!
        bestEquality2 = equality2!!
        bestScore = diff_cleanupSemanticScore(equality1!!, edit) + diff_cleanupSemanticScore(edit!!, equality2)
        while (edit!!.length != 0 && equality2!!.length != 0
          && edit[0] == equality2[0]
        ) {
          equality1 += edit[0]
          edit = edit.substring(1) + equality2[0]
          equality2 = equality2.substring(1)
          score = diff_cleanupSemanticScore(equality1, edit) + diff_cleanupSemanticScore(edit, equality2)
          // The >= encourages trailing rather than leading whitespace on edits.
          if (score >= bestScore) {
            bestScore = score
            bestEquality1 = equality1
            bestEdit = edit
            bestEquality2 = equality2
          }
        }

        if (prevDiff.text != bestEquality1) {
          // We have an improvement, save it back to the diff.
          if (bestEquality1.isNotEmpty()) {
            prevDiff.text = bestEquality1
          } else {
            pointer.previous() // Walk past nextDiff.
            pointer.previous() // Walk past thisDiff.
            pointer.previous() // Walk past prevDiff.
            pointer.remove() // Delete prevDiff.
            pointer.next() // Walk past thisDiff.
            pointer.next() // Walk past nextDiff.
          }
          thisDiff.text = bestEdit
          if (bestEquality2.isNotEmpty()) {
            nextDiff.text = bestEquality2
          } else {
            pointer.remove() // Delete nextDiff.
            nextDiff = thisDiff
            thisDiff = prevDiff
          }
        }
      }
      prevDiff = thisDiff
      thisDiff = nextDiff
      nextDiff = if (pointer.hasNext()) pointer.next() else null
    }
  }

  /**
   * Given two strings, compute a score representing whether the internal
   * boundary falls on logical boundaries.
   * Scores range from 6 (best) to 0 (worst).
   * @param one First string.
   * @param two Second string.
   * @return The score.
   */
  private fun diff_cleanupSemanticScore(one: String, two: String): Int {
    if (one.isEmpty() || two.isEmpty()) {
      // Edges are the best.
      return 6
    }

    // Each port of this function behaves slightly differently due to
    // subtle differences in each language's definition of things like
    // 'whitespace'.  Since this function's purpose is largely cosmetic,
    // the choice has been made to use each language's native features
    // rather than force total conformity.
    val char1 = one[one.length - 1]
    val char2 = two[0]
    val nonAlphaNumeric1 = !Character.isLetterOrDigit(char1)
    val nonAlphaNumeric2 = !Character.isLetterOrDigit(char2)
    val whitespace1 = nonAlphaNumeric1 && Character.isWhitespace(char1)
    val whitespace2 = nonAlphaNumeric2 && Character.isWhitespace(char2)
    val lineBreak1 = whitespace1 && Character.getType(char1) == Character.CONTROL.toInt()
    val lineBreak2 = whitespace2 && Character.getType(char2) == Character.CONTROL.toInt()
    val blankLine1 = lineBreak1 && BLANKLINEEND.matcher(one).find()
    val blankLine2 = lineBreak2 && BLANKLINESTART.matcher(two).find()

    if (blankLine1 || blankLine2) {
      // Five points for blank lines.
      return 5
    } else if (lineBreak1 || lineBreak2) {
      // Four points for line breaks.
      return 4
    } else if (nonAlphaNumeric1 && !whitespace1 && whitespace2) {
      // Three points for end of sentences.
      return 3
    } else if (whitespace1 || whitespace2) {
      // Two points for whitespace.
      return 2
    } else if (nonAlphaNumeric1 || nonAlphaNumeric2) {
      // One point for non-alphanumeric.
      return 1
    }
    return 0
  }

  /**
   * Reduce the number of edits by eliminating operationally trivial equalities.
   * @param diffs LinkedList of Diff objects.
   */
  fun diff_cleanupEfficiency(diffs: LinkedList<Diff>) {
    if (diffs.isEmpty()) {
      return
    }
    var changes = false
    val equalities = ArrayDeque<Diff>()  // Double-ended queue of equalities.
    var lastEquality: String? = null // Always equal to equalities.peek().text
    val pointer = diffs.listIterator()
    // Is there an insertion operation before the last equality.
    var pre_ins = false
    // Is there a deletion operation before the last equality.
    var pre_del = false
    // Is there an insertion operation after the last equality.
    var post_ins = false
    // Is there a deletion operation after the last equality.
    var post_del = false
    var thisDiff: Diff? = pointer.next()
    var safeDiff: Diff = thisDiff!!  // The last Diff that is known to be unsplittable.
    while (thisDiff != null) {
      if (thisDiff.operation == Operation.EQUAL) {
        // Equality found.
        if (thisDiff.text!!.length < Diff_EditCost && (post_ins || post_del)) {
          // Candidate found.
          equalities.push(thisDiff)
          pre_ins = post_ins
          pre_del = post_del
          lastEquality = thisDiff.text
        } else {
          // Not a candidate, and can never become one.
          equalities.clear()
          lastEquality = null
          safeDiff = thisDiff
        }
        post_del = false
        post_ins = post_del
      } else {
        // An insertion or deletion.
        if (thisDiff.operation == Operation.DELETE) {
          post_del = true
        } else {
          post_ins = true
        }
        /*
         * Five types to be split:
         * <ins>A</ins><del>B</del>XY<ins>C</ins><del>D</del>
         * <ins>A</ins>X<ins>C</ins><del>D</del>
         * <ins>A</ins><del>B</del>X<ins>C</ins>
         * <ins>A</del>X<ins>C</ins><del>D</del>
         * <ins>A</ins><del>B</del>X<del>C</del>
         */
        if (lastEquality != null && (pre_ins && pre_del && post_ins && post_del || lastEquality.length < Diff_EditCost / 2 && ((if (pre_ins) 1 else 0) + (if (pre_del) 1 else 0)
            + (if (post_ins) 1 else 0) + if (post_del) 1 else 0) == 3)
        ) {
          //System.out.println("Splitting: '" + lastEquality + "'");
          // Walk back to offending equality.
          while (thisDiff !== equalities.peek()) {
            thisDiff = pointer.previous()
          }
          pointer.next()

          // Replace equality with a delete.
          pointer.set(Diff(Operation.DELETE, lastEquality))
          // Insert a corresponding an insert.
          pointer.add(Diff(Operation.INSERT, lastEquality))

          equalities.pop()  // Throw away the equality we just deleted.
          lastEquality = null
          if (pre_ins && pre_del) {
            // No changes made which could affect previous entry, keep going.
            post_del = true
            post_ins = post_del
            equalities.clear()
            safeDiff = thisDiff!!
          } else {
            if (!equalities.isEmpty()) {
              // Throw away the previous equality (it needs to be reevaluated).
              equalities.pop()
            }
            thisDiff = if (equalities.isEmpty()) {
              // There are no previous questionable equalities,
              // walk back to the last known safe diff.
              safeDiff
            } else {
              // There is an equality we can fall back to.
              equalities.peek()
            }
            while (thisDiff !== pointer.previous()) {
              // Intentionally empty loop.
            }
            post_del = false
            post_ins = post_del
          }

          changes = true
        }
      }
      thisDiff = if (pointer.hasNext()) pointer.next() else null
    }

    if (changes) {
      diff_cleanupMerge(diffs)
    }
  }

  /**
   * Reorder and merge like edit sections.  Merge equalities.
   * Any edit section can move as long as it doesn't cross an equality.
   * @param diffs LinkedList of Diff objects.
   */
  fun diff_cleanupMerge(diffs: LinkedList<Diff>) {
    diffs.add(Diff(Operation.EQUAL, ""))  // Add a dummy entry at the end.
    var pointer: MutableListIterator<Diff> = diffs.listIterator()
    var count_delete = 0
    var count_insert = 0
    var text_delete = ""
    var text_insert = ""
    var thisDiff: Diff? = pointer.next()
    var prevEqual: Diff? = null
    var commonlength: Int
    while (thisDiff != null) {
      when (thisDiff.operation) {
        diff_match_patch.Operation.INSERT -> {
          count_insert++
          text_insert += thisDiff.text
          prevEqual = null
        }
        diff_match_patch.Operation.DELETE -> {
          count_delete++
          text_delete += thisDiff.text
          prevEqual = null
        }
        diff_match_patch.Operation.EQUAL -> {
          if (count_delete + count_insert > 1) {
            val both_types = count_delete != 0 && count_insert != 0
            // Delete the offending records.
            pointer.previous()  // Reverse direction.
            while (count_delete-- > 0) {
              pointer.previous()
              pointer.remove()
            }
            while (count_insert-- > 0) {
              pointer.previous()
              pointer.remove()
            }
            if (both_types) {
              // Factor out any common prefixies.
              commonlength = diff_commonPrefix(text_insert, text_delete)
              if (commonlength != 0) {
                if (pointer.hasPrevious()) {
                  thisDiff = pointer.previous()
                  assert(thisDiff.operation == Operation.EQUAL) { "Previous diff should have been an equality." }
                  thisDiff.text += text_insert.substring(0, commonlength)
                  pointer.next()
                } else {
                  pointer.add(
                    Diff(
                      Operation.EQUAL,
                      text_insert.substring(0, commonlength)
                    )
                  )
                }
                text_insert = text_insert.substring(commonlength)
                text_delete = text_delete.substring(commonlength)
              }
              // Factor out any common suffixies.
              commonlength = diff_commonSuffix(text_insert, text_delete)
              if (commonlength != 0) {
                thisDiff = pointer.next()
                thisDiff.text = text_insert.substring(text_insert.length - commonlength) + thisDiff.text!!
                text_insert = text_insert.substring(0, text_insert.length - commonlength)
                text_delete = text_delete.substring(0, text_delete.length - commonlength)
                pointer.previous()
              }
            }
            // Insert the merged records.
            if (text_delete.length != 0) {
              pointer.add(Diff(Operation.DELETE, text_delete))
            }
            if (text_insert.length != 0) {
              pointer.add(Diff(Operation.INSERT, text_insert))
            }
            // Step forward to the equality.
            thisDiff = if (pointer.hasNext()) pointer.next() else null
          } else if (prevEqual != null) {
            // Merge this equality with the previous one.
            prevEqual.text += thisDiff.text
            pointer.remove()
            thisDiff = pointer.previous()
            pointer.next()  // Forward direction
          }
          count_insert = 0
          count_delete = 0
          text_delete = ""
          text_insert = ""
          prevEqual = thisDiff
        }
      }
      thisDiff = if (pointer.hasNext()) pointer.next() else null
    }
    if (diffs.last.text!!.length == 0) {
      diffs.removeLast()  // Remove the dummy entry at the end.
    }

    /*
     * Second pass: look for single edits surrounded on both sides by equalities
     * which can be shifted sideways to eliminate an equality.
     * e.g: A<ins>BA</ins>C -> <ins>AB</ins>AC
     */
    var changes = false
    // Create a new iterator at the start.
    // (As opposed to walking the current one back.)
    pointer = diffs.listIterator()
    var prevDiff: Diff? = if (pointer.hasNext()) pointer.next() else null
    thisDiff = if (pointer.hasNext()) pointer.next() else null
    var nextDiff: Diff? = if (pointer.hasNext()) pointer.next() else null
    // Intentionally ignore the first and last element (don't need checking).
    while (nextDiff != null) {
      if (prevDiff!!.operation == Operation.EQUAL && nextDiff.operation == Operation.EQUAL) {
        // This is a single edit surrounded by equalities.
        if (thisDiff!!.text!!.endsWith(prevDiff.text!!)) {
          // Shift the edit over the previous equality.
          thisDiff.text = prevDiff.text!! +
            thisDiff.text!!.substring(0, thisDiff.text!!.length - prevDiff.text!!.length)
          nextDiff.text = prevDiff.text!! + nextDiff.text!!
          pointer.previous() // Walk past nextDiff.
          pointer.previous() // Walk past thisDiff.
          pointer.previous() // Walk past prevDiff.
          pointer.remove() // Delete prevDiff.
          pointer.next() // Walk past thisDiff.
          thisDiff = pointer.next() // Walk past nextDiff.
          nextDiff = if (pointer.hasNext()) pointer.next() else null
          changes = true
        } else if (thisDiff.text!!.startsWith(nextDiff.text!!)) {
          // Shift the edit over the next equality.
          prevDiff.text += nextDiff.text
          thisDiff.text = thisDiff.text!!.substring(nextDiff.text!!.length) + nextDiff.text!!
          pointer.remove() // Delete nextDiff.
          nextDiff = if (pointer.hasNext()) pointer.next() else null
          changes = true
        }
      }
      prevDiff = thisDiff
      thisDiff = nextDiff
      nextDiff = if (pointer.hasNext()) pointer.next() else null
    }
    // If shifts were made, the diff needs reordering and another shift sweep.
    if (changes) {
      diff_cleanupMerge(diffs)
    }
  }

  /**
   * loc is a location in text1, compute and return the equivalent location in
   * text2.
   * e.g. "The cat" vs "The big cat", 1->1, 5->8
   * @param diffs List of Diff objects.
   * @param loc Location within text1.
   * @return Location within text2.
   */
  fun diff_xIndex(diffs: List<Diff>, loc: Int): Int {
    var chars1 = 0
    var chars2 = 0
    var last_chars1 = 0
    var last_chars2 = 0
    var lastDiff: Diff? = null
    for (aDiff in diffs) {
      if (aDiff.operation != Operation.INSERT) {
        // Equality or deletion.
        chars1 += aDiff.text!!.length
      }
      if (aDiff.operation != Operation.DELETE) {
        // Equality or insertion.
        chars2 += aDiff.text!!.length
      }
      if (chars1 > loc) {
        // Overshot the location.
        lastDiff = aDiff
        break
      }
      last_chars1 = chars1
      last_chars2 = chars2
    }
    return if (lastDiff != null && lastDiff.operation == Operation.DELETE) {
      // The location was deleted.
      last_chars2
    } else last_chars2 + (loc - last_chars1)
    // Add the remaining character length.
  }

  /**
   * Convert a Diff list into a pretty HTML report.
   * @param diffs List of Diff objects.
   * @return HTML representation.
   */
  fun diff_prettyHtml(diffs: List<Diff>): String {
    val html = StringBuilder()
    for (aDiff in diffs) {
      val text = aDiff.text!!.replace("&", "&amp;").replace("<", "&lt;")
        .replace(">", "&gt;").replace("\n", "&para;<br>")
      when (aDiff.operation) {
        diff_match_patch.Operation.INSERT -> html.append("<ins style=\"background:#e6ffe6;\">").append(text)
          .append("</ins>")
        diff_match_patch.Operation.DELETE -> html.append("<del style=\"background:#ffe6e6;\">").append(text)
          .append("</del>")
        diff_match_patch.Operation.EQUAL -> html.append("<span>").append(text).append("</span>")
      }
    }
    return html.toString()
  }

  /**
   * Compute and return the source text (all equalities and deletions).
   * @param diffs List of Diff objects.
   * @return Source text.
   */
  fun diff_text1(diffs: List<Diff>): String {
    val text = StringBuilder()
    for (aDiff in diffs) {
      if (aDiff.operation != Operation.INSERT) {
        text.append(aDiff.text)
      }
    }
    return text.toString()
  }

  /**
   * Compute and return the destination text (all equalities and insertions).
   * @param diffs List of Diff objects.
   * @return Destination text.
   */
  fun diff_text2(diffs: List<Diff>): String {
    val text = StringBuilder()
    for (aDiff in diffs) {
      if (aDiff.operation != Operation.DELETE) {
        text.append(aDiff.text)
      }
    }
    return text.toString()
  }

  /**
   * Compute the Levenshtein distance; the number of inserted, deleted or
   * substituted characters.
   * @param diffs List of Diff objects.
   * @return Number of changes.
   */
  fun diff_levenshtein(diffs: List<Diff>): Int {
    var levenshtein = 0
    var insertions = 0
    var deletions = 0
    for (aDiff in diffs) {
      when (aDiff.operation) {
        diff_match_patch.Operation.INSERT -> insertions += aDiff.text!!.length
        diff_match_patch.Operation.DELETE -> deletions += aDiff.text!!.length
        diff_match_patch.Operation.EQUAL -> {
          // A deletion and an insertion is one substitution.
          levenshtein += Math.max(insertions, deletions)
          insertions = 0
          deletions = 0
        }
      }
    }
    levenshtein += Math.max(insertions, deletions)
    return levenshtein
  }

  /**
   * Crush the diff into an encoded string which describes the operations
   * required to transform text1 into text2.
   * E.g. =3\t-2\t+ing  -> Keep 3 chars, delete 2 chars, insert 'ing'.
   * Operations are tab-separated.  Inserted text is escaped using %xx notation.
   * @param diffs List of Diff objects.
   * @return Delta text.
   */
  fun diff_toDelta(diffs: List<Diff>): String {
    val text = StringBuilder()
    for (aDiff in diffs) {
      when (aDiff.operation) {
        diff_match_patch.Operation.INSERT -> try {
          text.append("+").append(
            URLEncoder.encode(aDiff.text, "UTF-8")
              .replace('+', ' ')
          ).append("\t")
        } catch (e: UnsupportedEncodingException) {
          // Not likely on modern system.
          throw Error("This system does not support UTF-8.", e)
        }

        diff_match_patch.Operation.DELETE -> text.append("-").append(aDiff.text!!.length).append("\t")
        diff_match_patch.Operation.EQUAL -> text.append("=").append(aDiff.text!!.length).append("\t")
      }
    }
    var delta = text.toString()
    if (delta.length != 0) {
      // Strip off trailing tab character.
      delta = delta.substring(0, delta.length - 1)
      delta = unescapeForEncodeUriCompatability(delta)
    }
    return delta
  }

  /**
   * Given the original text1, and an encoded string which describes the
   * operations required to transform text1 into text2, compute the full diff.
   * @param text1 Source string for the diff.
   * @param delta Delta text.
   * @return Array of Diff objects or null if invalid.
   * @throws IllegalArgumentException If invalid input.
   */
  @Throws(IllegalArgumentException::class)
  fun diff_fromDelta(text1: String, delta: String): LinkedList<Diff> {
    val diffs = LinkedList<Diff>()
    var pointer = 0  // Cursor in text1
    val tokens = delta.split("\t".toRegex()).dropLastWhile { it.isEmpty() }.toTypedArray()
    for (token in tokens) {
      if (token.length == 0) {
        // Blank tokens are ok (from a trailing \t).
        continue
      }
      // Each token begins with a one character parameter which specifies the
      // operation of this token (delete, insert, equality).
      var param = token.substring(1)
      when (token[0]) {
        '+' -> {
          // decode would change all "+" to " "
          param = param.replace("+", "%2B")
          try {
            param = URLDecoder.decode(param, "UTF-8")
          } catch (e: UnsupportedEncodingException) {
            // Not likely on modern system.
            throw Error("This system does not support UTF-8.", e)
          } catch (e: IllegalArgumentException) {
            // Malformed URI sequence.
            throw IllegalArgumentException(
              "Illegal escape in diff_fromDelta: $param", e
            )
          }

          diffs.add(Diff(Operation.INSERT, param))
        }
        '-',
          // Fall through.
        '=' -> {
          val n: Int
          try {
            n = Integer.parseInt(param)
          } catch (e: NumberFormatException) {
            throw IllegalArgumentException(
              "Invalid number in diff_fromDelta: $param", e
            )
          }

          if (n < 0) {
            throw IllegalArgumentException(
              "Negative number in diff_fromDelta: $param"
            )
          }
          val text: String
          try {
            val run = { pointer += n; pointer }
            text = text1.substring(pointer, run())
          } catch (e: StringIndexOutOfBoundsException) {
            throw IllegalArgumentException(
              "Delta length (" + pointer
                + ") larger than source text length (" + text1.length
                + ").", e
            )
          }

          if (token[0] == '=') {
            diffs.add(Diff(Operation.EQUAL, text))
          } else {
            diffs.add(Diff(Operation.DELETE, text))
          }
        }
        else ->
          // Anything else is an error.
          throw IllegalArgumentException(
            "Invalid diff operation in diff_fromDelta: " + token[0]
          )
      }
    }
    if (pointer != text1.length) {
      throw IllegalArgumentException(
        "Delta length (" + pointer
          + ") smaller than source text length (" + text1.length + ")."
      )
    }
    return diffs
  }

  //  MATCH FUNCTIONS

  /**
   * Locate the best instance of 'pattern' in 'text' near 'loc'.
   * Returns -1 if no match found.
   * @param text The text to search.
   * @param pattern The pattern to search for.
   * @param loc The location to search around.
   * @return Best match index or -1.
   */
  fun match_main(text: String?, pattern: String?, loc: Int): Int {
    var loc = loc
    // Check for null inputs.
    if (text == null || pattern == null) {
      throw IllegalArgumentException("Null inputs. (match_main)")
    }

    loc = Math.max(0, Math.min(loc, text.length))
    return if (text == pattern) {
      // Shortcut (potentially not guaranteed by the algorithm)
      0
    } else if (text.isEmpty()) {
      // Nothing to match.
      -1
    } else if (loc + pattern.length <= text.length && text.substring(loc, loc + pattern.length) == pattern) {
      // Perfect match at the perfect spot!  (Includes case of null pattern)
      loc
    } else {
      // Do a fuzzy compare.
      match_bitap(text, pattern, loc)
    }
  }

  /**
   * Locate the best instance of 'pattern' in 'text' near 'loc' using the
   * Bitap algorithm.  Returns -1 if no match found.
   * @param text The text to search.
   * @param pattern The pattern to search for.
   * @param loc The location to search around.
   * @return Best match index or -1.
   */
  protected fun match_bitap(text: String, pattern: String, loc: Int): Int {
    assert(Match_MaxBits.toInt() == 0 || pattern.length <= Match_MaxBits) { "Pattern too long for this application." }

    // Initialise the alphabet.
    val s = match_alphabet(pattern)

    // Highest score beyond which we give up.
    var score_threshold = Match_Threshold.toDouble()
    // Is there a nearby exact match? (speedup)
    var best_loc = text.indexOf(pattern, loc)
    if (best_loc != -1) {
      score_threshold = Math.min(
        match_bitapScore(0, best_loc, loc, pattern),
        score_threshold
      )
      // What about in the other direction? (speedup)
      best_loc = text.lastIndexOf(pattern, loc + pattern.length)
      if (best_loc != -1) {
        score_threshold = Math.min(
          match_bitapScore(0, best_loc, loc, pattern),
          score_threshold
        )
      }
    }

    // Initialise the bit arrays.
    val matchmask = 1 shl pattern.length - 1
    best_loc = -1

    var bin_min: Int
    var bin_mid: Int
    var bin_max = pattern.length + text.length
    // Empty initialization added to appease Java compiler.
    var last_rd = IntArray(0)
    for (d in 0 until pattern.length) {
      // Scan for the best match; each iteration allows for one more error.
      // Run a binary search to determine how far from 'loc' we can stray at
      // this error level.
      bin_min = 0
      bin_mid = bin_max
      while (bin_min < bin_mid) {
        if (match_bitapScore(d, loc + bin_mid, loc, pattern) <= score_threshold) {
          bin_min = bin_mid
        } else {
          bin_max = bin_mid
        }
        bin_mid = (bin_max - bin_min) / 2 + bin_min
      }
      // Use the result from this iteration as the maximum for the next.
      bin_max = bin_mid
      var start = Math.max(1, loc - bin_mid + 1)
      val finish = Math.min(loc + bin_mid, text.length) + pattern.length

      val rd = IntArray(finish + 2)
      rd[finish + 1] = (1 shl d) - 1
      for (j in finish downTo start) {
        val charMatch: Int
        if (text.length <= j - 1 || !s.containsKey(text[j - 1])) {
          // Out of range.
          charMatch = 0
        } else {
          charMatch = s[text[j - 1]]!!
        }
        if (d == 0) {
          // First pass: exact match.
          rd[j] = rd[j + 1] shl 1 or 1 and charMatch
        } else {
          // Subsequent passes: fuzzy match.
          rd[j] = (rd[j + 1] shl 1 or 1 and charMatch
            or (last_rd[j + 1] or last_rd[j] shl 1 or 1) or last_rd[j + 1])
        }
        if (rd[j] and matchmask != 0) {
          val score = match_bitapScore(d, j - 1, loc, pattern)
          // This match will almost certainly be better than any existing
          // match.  But check anyway.
          if (score <= score_threshold) {
            // Told you so.
            score_threshold = score
            best_loc = j - 1
            if (best_loc > loc) {
              // When passing loc, don't exceed our current distance from loc.
              start = Math.max(1, 2 * loc - best_loc)
            } else {
              // Already passed loc, downhill from here on in.
              break
            }
          }
        }
      }
      if (match_bitapScore(d + 1, loc, loc, pattern) > score_threshold) {
        // No hope for a (better) match at greater error levels.
        break
      }
      last_rd = rd
    }
    return best_loc
  }

  /**
   * Compute and return the score for a match with e errors and x location.
   * @param e Number of errors in match.
   * @param x Location of match.
   * @param loc Expected location of match.
   * @param pattern Pattern being sought.
   * @return Overall score for match (0.0 = good, 1.0 = bad).
   */
  private fun match_bitapScore(e: Int, x: Int, loc: Int, pattern: String): Double {
    val accuracy = e.toFloat() / pattern.length
    val proximity = Math.abs(loc - x)
    return if (Match_Distance == 0) {
      // Dodge divide by zero error.
      if (proximity == 0) accuracy.toDouble() else 1.0
    } else (accuracy + proximity / Match_Distance.toFloat()).toDouble()
  }

  /**
   * Initialise the alphabet for the Bitap algorithm.
   * @param pattern The text to encode.
   * @return Hash of character locations.
   */
  protected fun match_alphabet(pattern: String): Map<Char, Int> {
    val s = HashMap<Char, Int>()
    val char_pattern = pattern.toCharArray()
    for (c in char_pattern) {
      s[c] = 0
    }
    for ((i, c) in char_pattern.withIndex()) {
      s[c] = s[c]!! or (1 shl pattern.length - i - 1)
    }
    return s
  }

  //  PATCH FUNCTIONS

  /**
   * Increase the context until it is unique,
   * but don't let the pattern expand beyond Match_MaxBits.
   * @param patch The patch to grow.
   * @param text Source text.
   */
  protected fun patch_addContext(patch: Patch, text: String) {
    if (text.length == 0) {
      return
    }
    var pattern = text.substring(patch.start2, patch.start2 + patch.length1)
    var padding = 0

    // Look for the first and last matches of pattern in text.  If two different
    // matches are found, increase the pattern length.
    while (text.indexOf(pattern) != text.lastIndexOf(pattern) && pattern.length < Match_MaxBits.toInt() - Patch_Margin.toInt() - Patch_Margin.toInt()) {
      padding += Patch_Margin.toInt()
      pattern = text.substring(
        Math.max(0, patch.start2 - padding),
        Math.min(text.length, patch.start2 + patch.length1 + padding)
      )
    }
    // Add one chunk for good luck.
    padding += Patch_Margin.toInt()

    // Add the prefix.
    val prefix = text.substring(
      Math.max(0, patch.start2 - padding),
      patch.start2
    )
    if (prefix.length != 0) {
      patch.diffs.addFirst(Diff(Operation.EQUAL, prefix))
    }
    // Add the suffix.
    val suffix = text.substring(
      patch.start2 + patch.length1,
      Math.min(text.length, patch.start2 + patch.length1 + padding)
    )
    if (suffix.length != 0) {
      patch.diffs.addLast(Diff(Operation.EQUAL, suffix))
    }

    // Roll back the start points.
    patch.start1 -= prefix.length
    patch.start2 -= prefix.length
    // Extend the lengths.
    patch.length1 += prefix.length + suffix.length
    patch.length2 += prefix.length + suffix.length
  }

  /**
   * Compute a list of patches to turn text1 into text2.
   * A set of diffs will be computed.
   * @param text1 Old text.
   * @param text2 New text.
   * @return LinkedList of Patch objects.
   */
  fun patch_make(text1: String?, text2: String?): LinkedList<Patch> {
    if (text1 == null || text2 == null) {
      throw IllegalArgumentException("Null inputs. (patch_make)")
    }
    // No diffs provided, compute our own.
    val diffs = diff_main(text1, text2, true)
    if (diffs.size > 2) {
      diff_cleanupSemantic(diffs)
      diff_cleanupEfficiency(diffs)
    }
    return patch_make(text1, diffs)
  }

  /**
   * Compute a list of patches to turn text1 into text2.
   * text1 will be derived from the provided diffs.
   * @param diffs Array of Diff objects for text1 to text2.
   * @return LinkedList of Patch objects.
   */
  fun patch_make(diffs: LinkedList<Diff>?): LinkedList<Patch> {
    if (diffs == null) {
      throw IllegalArgumentException("Null inputs. (patch_make)")
    }
    // No origin string provided, compute our own.
    val text1 = diff_text1(diffs)
    return patch_make(text1, diffs)
  }

  /**
   * Compute a list of patches to turn text1 into text2.
   * text2 is ignored, diffs are the delta between text1 and text2.
   * @param text1 Old text
   * @param text2 Ignored.
   * @param diffs Array of Diff objects for text1 to text2.
   * @return LinkedList of Patch objects.
   */
  @Deprecated("Prefer patch_make(String text1, LinkedList<Diff> diffs).")
  fun patch_make(
    text1: String, text2: String,
    diffs: LinkedList<Diff>
  ): LinkedList<Patch> {
    return patch_make(text1, diffs)
  }

  /**
   * Compute a list of patches to turn text1 into text2.
   * text2 is not provided, diffs are the delta between text1 and text2.
   * @param text1 Old text.
   * @param diffs Array of Diff objects for text1 to text2.
   * @return LinkedList of Patch objects.
   */
  fun patch_make(text1: String?, diffs: LinkedList<Diff>?): LinkedList<Patch> {
    if (text1 == null || diffs == null) {
      throw IllegalArgumentException("Null inputs. (patch_make)")
    }

    val patches = LinkedList<Patch>()
    if (diffs.isEmpty()) {
      return patches  // Get rid of the null case.
    }
    var patch = Patch()
    var char_count1 = 0  // Number of characters into the text1 string.
    var char_count2 = 0  // Number of characters into the text2 string.
    // Start with text1 (prepatch_text) and apply the diffs until we arrive at
    // text2 (postpatch_text). We recreate the patches one by one to determine
    // context info.
    var prepatch_text: String = text1
    var postpatch_text: String = text1
    for (aDiff in diffs) {
      if (patch.diffs.isEmpty() && aDiff.operation != Operation.EQUAL) {
        // A new patch starts here.
        patch.start1 = char_count1
        patch.start2 = char_count2
      }

      when (aDiff.operation) {
        diff_match_patch.Operation.INSERT -> {
          patch.diffs.add(aDiff)
          patch.length2 += aDiff.text!!.length
          postpatch_text = (postpatch_text.substring(0, char_count2)
            + aDiff.text + postpatch_text.substring(char_count2))
        }
        diff_match_patch.Operation.DELETE -> {
          patch.length1 += aDiff.text!!.length
          patch.diffs.add(aDiff)
          postpatch_text = postpatch_text.substring(0, char_count2) +
            postpatch_text.substring(char_count2 + aDiff.text!!.length)
        }
        diff_match_patch.Operation.EQUAL -> {
          if (aDiff.text!!.length <= 2 * Patch_Margin
            && !patch.diffs.isEmpty() && aDiff !== diffs.last
          ) {
            // Small equality inside a patch.
            patch.diffs.add(aDiff)
            patch.length1 += aDiff.text!!.length
            patch.length2 += aDiff.text!!.length
          }

          if (aDiff.text!!.length >= 2 * Patch_Margin && !patch.diffs.isEmpty()) {
            // Time for a new patch.
            if (!patch.diffs.isEmpty()) {
              patch_addContext(patch, prepatch_text)
              patches.add(patch)
              patch = Patch()
              // Unlike Unidiff, our patch lists have a rolling context.
              // https://github.com/google/diff-match-patch/wiki/Unidiff
              // Update prepatch text & pos to reflect the application of the
              // just completed patch.
              prepatch_text = postpatch_text
              char_count1 = char_count2
            }
          }
        }
      }

      // Update the current character count.
      if (aDiff.operation != Operation.INSERT) {
        char_count1 += aDiff.text!!.length
      }
      if (aDiff.operation != Operation.DELETE) {
        char_count2 += aDiff.text!!.length
      }
    }
    // Pick up the leftover patch if not empty.
    if (!patch.diffs.isEmpty()) {
      patch_addContext(patch, prepatch_text)
      patches.add(patch)
    }

    return patches
  }

  /**
   * Given an array of patches, return another array that is identical.
   * @param patches Array of Patch objects.
   * @return Array of Patch objects.
   */
  fun patch_deepCopy(patches: LinkedList<Patch>): LinkedList<Patch> {
    val patchesCopy = LinkedList<Patch>()
    for (aPatch in patches) {
      val patchCopy = Patch()
      for (aDiff in aPatch.diffs) {
        val diffCopy = Diff(aDiff.operation, aDiff.text)
        patchCopy.diffs.add(diffCopy)
      }
      patchCopy.start1 = aPatch.start1
      patchCopy.start2 = aPatch.start2
      patchCopy.length1 = aPatch.length1
      patchCopy.length2 = aPatch.length2
      patchesCopy.add(patchCopy)
    }
    return patchesCopy
  }

  /**
   * Merge a set of patches onto the text.  Return a patched text, as well
   * as an array of true/false values indicating which patches were applied.
   * @param patches Array of Patch objects
   * @param text Old text.
   * @return Two element Object array, containing the new text and an array of
   * boolean values.
   */
  fun patch_apply(patches: LinkedList<Patch>, text: String): Array<Any> {
    var patches = patches
    var text = text
    if (patches.isEmpty()) {
      return arrayOf(text, BooleanArray(0))
    }

    // Deep copy the patches so that no changes are made to originals.
    patches = patch_deepCopy(patches)

    val nullPadding = patch_addPadding(patches)
    text = nullPadding + text + nullPadding
    patch_splitMax(patches)

    var x = 0
    // delta keeps track of the offset between the expected and actual location
    // of the previous patch.  If there are patches expected at positions 10 and
    // 20, but the first patch was found at 12, delta is 2 and the second patch
    // has an effective expected position of 22.
    var delta = 0
    val results = BooleanArray(patches.size)
    for (aPatch in patches) {
      val expected_loc = aPatch.start2 + delta
      val text1 = diff_text1(aPatch.diffs)
      var start_loc: Int
      var end_loc = -1
      if (text1.length > this.Match_MaxBits) {
        // patch_splitMax will only provide an oversized pattern in the case of
        // a monster delete.
        start_loc = match_main(
          text,
          text1.substring(0, this.Match_MaxBits.toInt()), expected_loc
        )
        if (start_loc != -1) {
          end_loc = match_main(
            text,
            text1.substring(text1.length - this.Match_MaxBits),
            expected_loc + text1.length - this.Match_MaxBits
          )
          if (end_loc == -1 || start_loc >= end_loc) {
            // Can't find valid trailing context.  Drop this patch.
            start_loc = -1
          }
        }
      } else {
        start_loc = match_main(text, text1, expected_loc)
      }
      if (start_loc == -1) {
        // No match found.  :(
        results[x] = false
        // Subtract the delta for this failed patch from subsequent patches.
        delta -= aPatch.length2 - aPatch.length1
      } else {
        // Found a match.  :)
        results[x] = true
        delta = start_loc - expected_loc
        val text2: String
        if (end_loc == -1) {
          text2 = text.substring(
            start_loc,
            Math.min(start_loc + text1.length, text.length)
          )
        } else {
          text2 = text.substring(
            start_loc,
            Math.min(end_loc + this.Match_MaxBits, text.length)
          )
        }
        if (text1 == text2) {
          // Perfect match, just shove the replacement text in.
          text = (text.substring(0, start_loc) + diff_text2(aPatch.diffs)
            + text.substring(start_loc + text1.length))
        } else {
          // Imperfect match.  Run a diff to get a framework of equivalent
          // indices.
          val diffs = diff_main(text1, text2, false)
          if (text1.length > this.Match_MaxBits && diff_levenshtein(diffs) / text1.length.toFloat() > this.Patch_DeleteThreshold) {
            // The end points match, but the content is unacceptably bad.
            results[x] = false
          } else {
            diff_cleanupSemanticLossless(diffs)
            var index1 = 0
            for (aDiff in aPatch.diffs) {
              if (aDiff.operation != Operation.EQUAL) {
                val index2 = diff_xIndex(diffs, index1)
                if (aDiff.operation == Operation.INSERT) {
                  // Insertion
                  text = (text.substring(0, start_loc + index2) + aDiff.text
                    + text.substring(start_loc + index2))
                } else if (aDiff.operation == Operation.DELETE) {
                  // Deletion
                  text = text.substring(0, start_loc + index2) + text.substring(
                    start_loc + diff_xIndex(
                      diffs,
                      index1 + aDiff.text!!.length
                    )
                  )
                }
              }
              if (aDiff.operation != Operation.DELETE) {
                index1 += aDiff.text!!.length
              }
            }
          }
        }
      }
      x++
    }
    // Strip the padding off.
    text = text.substring(nullPadding.length, text.length - nullPadding.length)
    return arrayOf(text, results)
  }

  /**
   * Add some padding on text start and end so that edges can match something.
   * Intended to be called only from within patch_apply.
   * @param patches Array of Patch objects.
   * @return The padding string added to each side.
   */
  fun patch_addPadding(patches: LinkedList<Patch>): String {
    val paddingLength = this.Patch_Margin
    var nullPadding = ""
    for (x in 1..paddingLength) {
      nullPadding += x.toChar().toString()
    }

    // Bump all the patches forward.
    for (aPatch in patches) {
      aPatch.start1 += paddingLength.toInt()
      aPatch.start2 += paddingLength.toInt()
    }

    // Add some padding on start of first diff.
    var patch = patches.first
    var diffs = patch.diffs
    if (diffs.isEmpty() || diffs.first.operation != Operation.EQUAL) {
      // Add nullPadding equality.
      diffs.addFirst(Diff(Operation.EQUAL, nullPadding))
      patch.start1 -= paddingLength.toInt()  // Should be 0.
      patch.start2 -= paddingLength.toInt()  // Should be 0.
      patch.length1 += paddingLength.toInt()
      patch.length2 += paddingLength.toInt()
    } else if (paddingLength > diffs.first.text!!.length) {
      // Grow first equality.
      val firstDiff = diffs.first
      val extraLength = paddingLength - firstDiff.text!!.length
      firstDiff.text = nullPadding.substring(firstDiff.text!!.length) + firstDiff.text!!
      patch.start1 -= extraLength
      patch.start2 -= extraLength
      patch.length1 += extraLength
      patch.length2 += extraLength
    }

    // Add some padding on end of last diff.
    patch = patches.last
    diffs = patch.diffs
    if (diffs.isEmpty() || diffs.last.operation != Operation.EQUAL) {
      // Add nullPadding equality.
      diffs.addLast(Diff(Operation.EQUAL, nullPadding))
      patch.length1 += paddingLength.toInt()
      patch.length2 += paddingLength.toInt()
    } else if (paddingLength > diffs.last.text!!.length) {
      // Grow last equality.
      val lastDiff = diffs.last
      val extraLength = paddingLength - lastDiff.text!!.length
      lastDiff.text += nullPadding.substring(0, extraLength)
      patch.length1 += extraLength
      patch.length2 += extraLength
    }

    return nullPadding
  }

  /**
   * Look through the patches and break up any which are longer than the
   * maximum limit of the match algorithm.
   * Intended to be called only from within patch_apply.
   * @param patches LinkedList of Patch objects.
   */
  fun patch_splitMax(patches: LinkedList<Patch>) {
    val patch_size = Match_MaxBits
    var precontext: String
    var postcontext: String
    var patch: Patch
    var start1: Int
    var start2: Int
    var empty: Boolean
    var diff_type: Operation?
    var diff_text: String?
    val pointer = patches.listIterator()
    var bigpatch: Patch? = if (pointer.hasNext()) pointer.next() else null
    while (bigpatch != null) {
      if (bigpatch.length1 <= Match_MaxBits) {
        bigpatch = if (pointer.hasNext()) pointer.next() else null
        continue
      }
      // Remove the big old patch.
      pointer.remove()
      start1 = bigpatch.start1
      start2 = bigpatch.start2
      precontext = ""
      while (!bigpatch.diffs.isEmpty()) {
        // Create one of several smaller patches.
        patch = Patch()
        empty = true
        patch.start1 = start1 - precontext.length
        patch.start2 = start2 - precontext.length
        if (precontext.length != 0) {
          patch.length2 = precontext.length
          patch.length1 = patch.length2
          patch.diffs.add(Diff(Operation.EQUAL, precontext))
        }
        while (!bigpatch.diffs.isEmpty() && patch.length1 < patch_size - Patch_Margin) {
          diff_type = bigpatch.diffs.first.operation
          diff_text = bigpatch.diffs.first.text
          if (diff_type == Operation.INSERT) {
            // Insertions are harmless.
            patch.length2 += diff_text!!.length
            start2 += diff_text.length
            patch.diffs.addLast(bigpatch.diffs.removeFirst())
            empty = false
          } else if (diff_type == Operation.DELETE && patch.diffs.size == 1
            && patch.diffs.first.operation == Operation.EQUAL
            && diff_text!!.length > 2 * patch_size
          ) {
            // This is a large deletion.  Let it pass in one chunk.
            patch.length1 += diff_text.length
            start1 += diff_text.length
            empty = false
            patch.diffs.add(Diff(diff_type, diff_text))
            bigpatch.diffs.removeFirst()
          } else {
            // Deletion or equality.  Only take as much as we can stomach.
            diff_text = diff_text!!.substring(
              0, Math.min(
                diff_text.length,
                patch_size.toInt() - patch.length1 - Patch_Margin.toInt()
              )
            )
            patch.length1 += diff_text.length
            start1 += diff_text.length
            if (diff_type == Operation.EQUAL) {
              patch.length2 += diff_text.length
              start2 += diff_text.length
            } else {
              empty = false
            }
            patch.diffs.add(Diff(diff_type, diff_text))
            if (diff_text == bigpatch.diffs.first.text) {
              bigpatch.diffs.removeFirst()
            } else {
              bigpatch.diffs.first.text = bigpatch.diffs.first.text!!
                .substring(diff_text.length)
            }
          }
        }
        // Compute the head context for the next patch.
        precontext = diff_text2(patch.diffs)
        precontext = precontext.substring(Math.max(0, precontext.length - Patch_Margin))
        // Append the end context for this patch.
        if (diff_text1(bigpatch.diffs).length > Patch_Margin) {
          postcontext = diff_text1(bigpatch.diffs).substring(0, Patch_Margin.toInt())
        } else {
          postcontext = diff_text1(bigpatch.diffs)
        }
        if (postcontext.length != 0) {
          patch.length1 += postcontext.length
          patch.length2 += postcontext.length
          if (!patch.diffs.isEmpty() && patch.diffs.last.operation == Operation.EQUAL) {
            patch.diffs.last.text += postcontext
          } else {
            patch.diffs.add(Diff(Operation.EQUAL, postcontext))
          }
        }
        if (!empty) {
          pointer.add(patch)
        }
      }
      bigpatch = if (pointer.hasNext()) pointer.next() else null
    }
  }

  /**
   * Take a list of patches and return a textual representation.
   * @param patches List of Patch objects.
   * @return Text representation of patches.
   */
  fun patch_toText(patches: List<Patch>): String {
    val text = StringBuilder()
    for (aPatch in patches) {
      text.append(aPatch)
    }
    return text.toString()
  }

  /**
   * Parse a textual representation of patches and return a List of Patch
   * objects.
   * @param textline Text representation of patches.
   * @return List of Patch objects.
   * @throws IllegalArgumentException If invalid input.
   */
  @Throws(IllegalArgumentException::class)
  fun patch_fromText(textline: String): List<Patch> {
    val patches = LinkedList<Patch>()
    if (textline.isEmpty()) {
      return patches
    }
    val textList = Arrays.asList(*textline.split("\n".toRegex()).dropLastWhile { it.isEmpty() }.toTypedArray())
    val text = LinkedList(textList)
    var patch: Patch
    val patchHeader = Pattern.compile("^@@ -(\\d+),?(\\d*) \\+(\\d+),?(\\d*) @@$")
    var m: Matcher
    var sign: Char
    var line: String
    while (!text.isEmpty()) {
      m = patchHeader.matcher(text.first)
      if (!m.matches()) {
        throw IllegalArgumentException(
          "Invalid patch string: " + text.first
        )
      }
      patch = Patch()
      patches.add(patch)
      patch.start1 = Integer.parseInt(m.group(1))
      if (m.group(2).length == 0) {
        patch.start1--
        patch.length1 = 1
      } else if (m.group(2) == "0") {
        patch.length1 = 0
      } else {
        patch.start1--
        patch.length1 = Integer.parseInt(m.group(2))
      }

      patch.start2 = Integer.parseInt(m.group(3))
      when {
        m.group(4).isEmpty() -> {
          patch.start2--
          patch.length2 = 1
        }
        m.group(4) == "0" -> patch.length2 = 0
        else -> {
          patch.start2--
          patch.length2 = Integer.parseInt(m.group(4))
        }
      }
      text.removeFirst()

      while (!text.isEmpty()) {
        try {
          sign = text.first[0]
        } catch (e: IndexOutOfBoundsException) {
          // Blank line?  Whatever.
          text.removeFirst()
          continue
        }

        line = text.first.substring(1)
        line = line.replace("+", "%2B")  // decode would change all "+" to " "
        try {
          line = URLDecoder.decode(line, "UTF-8")
        } catch (e: UnsupportedEncodingException) {
          // Not likely on modern system.
          throw Error("This system does not support UTF-8.", e)
        } catch (e: IllegalArgumentException) {
          // Malformed URI sequence.
          throw IllegalArgumentException(
            "Illegal escape in patch_fromText: $line", e
          )
        }

        if (sign == '-') {
          // Deletion.
          patch.diffs.add(Diff(Operation.DELETE, line))
        } else if (sign == '+') {
          // Insertion.
          patch.diffs.add(Diff(Operation.INSERT, line))
        } else if (sign == ' ') {
          // Minor equality.
          patch.diffs.add(Diff(Operation.EQUAL, line))
        } else if (sign == '@') {
          // Start of next patch.
          break
        } else {
          // WTF?
          throw IllegalArgumentException(
            "Invalid patch mode '$sign' in: $line"
          )
        }
        text.removeFirst()
      }
    }
    return patches
  }

  /**
   * Class representing one diff operation.
   */
  class Diff
  /**
   * Constructor.  Initializes the diff with the provided values.
   * @param operation One of INSERT, DELETE or EQUAL.
   * @param text The text being applied.
   */
    (
    /**
     * One of: INSERT, DELETE or EQUAL.
     */
    var operation: Operation?,
    /**
     * The text associated with this diff operation.
     */
    var text: String?
  )// Construct a diff with the specified operation and text.
  {

    /**
     * Display a human-readable version of this Diff.
     * @return text version.
     */
    override fun toString(): String {
      val prettyText = this.text!!.replace('\n', '\u00b6')
      return "Diff(" + this.operation + ",\"" + prettyText + "\")"
    }

    /**
     * Create a numeric hash value for a Diff.
     * This function is not used by DMP.
     * @return Hash value.
     */
    override fun hashCode(): Int {
      val prime = 31
      var result = if (operation == null) 0 else operation!!.hashCode()
      result += prime * if (text == null) 0 else text!!.hashCode()
      return result
    }

    /**
     * Is this Diff equivalent to another Diff?
     * @param obj Another Diff to compare against.
     * @return true or false.
     */
    override fun equals(obj: Any?): Boolean {
      if (this === obj) {
        return true
      }
      if (obj == null) {
        return false
      }
      if (javaClass != obj.javaClass) {
        return false
      }
      val other = obj as Diff?
      if (operation != other!!.operation) {
        return false
      }
      if (text == null) {
        if (other.text != null) {
          return false
        }
      } else if (text != other.text) {
        return false
      }
      return true
    }
  }

  /**
   * Class representing one patch operation.
   */
  class Patch {
    var diffs: LinkedList<Diff> = LinkedList()
    var start1: Int = 0
    var start2: Int = 0
    var length1: Int = 0
    var length2: Int = 0

    /**
     * Emulate GNU diff's format.
     * Header: @@ -382,8 +481,9 @@
     * Indices are printed as 1-based, not 0-based.
     * @return The GNU diff string.
     */
    override fun toString(): String {
      val coords1: String
      val coords2: String
      if (this.length1 == 0) {
        coords1 = this.start1.toString() + ",0"
      } else if (this.length1 == 1) {
        coords1 = Integer.toString(this.start1 + 1)
      } else {
        coords1 = (this.start1 + 1).toString() + "," + this.length1
      }
      if (this.length2 == 0) {
        coords2 = this.start2.toString() + ",0"
      } else if (this.length2 == 1) {
        coords2 = Integer.toString(this.start2 + 1)
      } else {
        coords2 = (this.start2 + 1).toString() + "," + this.length2
      }
      val text = StringBuilder()
      text.append("@@ -").append(coords1).append(" +").append(coords2)
        .append(" @@\n")
      // Escape the body of the patch with %xx notation.
      for (aDiff in this.diffs) {
        when (aDiff.operation) {
          diff_match_patch.Operation.INSERT -> text.append('+')
          diff_match_patch.Operation.DELETE -> text.append('-')
          diff_match_patch.Operation.EQUAL -> text.append(' ')
        }
        try {
          text.append(URLEncoder.encode(aDiff.text, "UTF-8").replace('+', ' '))
            .append("\n")
        } catch (e: UnsupportedEncodingException) {
          // Not likely on modern system.
          throw Error("This system does not support UTF-8.", e)
        }
      }
      return unescapeForEncodeUriCompatability(text.toString())
    }
  }

  companion object {

    /**
     * Unescape selected chars for compatability with JavaScript's encodeURI.
     * In speed critical applications this could be dropped since the
     * receiving application will certainly decode these fine.
     * Note that this function is case-sensitive.  Thus "%3f" would not be
     * unescaped.  But this is ok because it is only called with the output of
     * URLEncoder.encode which returns uppercase hex.
     *
     * Example: "%3F" -> "?", "%24" -> "$", etc.
     *
     * @param str The string to escape.
     * @return The escaped string.
     */
    private fun unescapeForEncodeUriCompatability(str: String): String {
      return str.replace("%21", "!").replace("%7E", "~")
        .replace("%27", "'").replace("%28", "(").replace("%29", ")")
        .replace("%3B", ";").replace("%2F", "/").replace("%3F", "?")
        .replace("%3A", ":").replace("%40", "@").replace("%26", "&")
        .replace("%3D", "=").replace("%2B", "+").replace("%24", "$")
        .replace("%2C", ",").replace("%23", "#")
    }
  }
}
/**
 * Find the differences between two texts.
 * Run a faster, slightly less optimal diff.
 * This method allows the 'checklines' of diff_main() to be optional.
 * Most of the time checklines is wanted, so default to true.
 * @param text1 Old string to be diffed.
 * @param text2 New string to be diffed.
 * @return Linked List of Diff objects.
 */
