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

part of DiffMatchPatch;

/**
 * Class representing one diff operation.
 */
class Diff {
  /**
   * One of: Operation.insert, Operation.delete or Operation.equal.
   */
  Operation operation;
  /**
   * The text associated with this diff operation.
   */
  String text;

  /**
   * Constructor.  Initializes the diff with the provided values.
   * [operation] is one of Operation.insert, Operation.delete or Operation.equal.
   * [text] is the text being applied.
   */
  Diff(this.operation, this.text);

  /**
   * Display a human-readable version of this Diff.
   * Returns a text version.
   */
  String toString() {
    String prettyText = this.text.replaceAll('\n', '\u00b6');
    return 'Diff(${this.operation},"$prettyText")';
  }

  /**
   * Is this Diff equivalent to another Diff?
   * [other] is another Diff to compare against.
   * Returns true or false.
   */
  @override
  bool operator ==(Object other) =>
  	      identical(this, other) ||
          other is Diff &&
              runtimeType == other.runtimeType &&
              operation == other.operation &&
              text == other.text;

  /**
   * Generate a uniquely identifiable hashcode for this Diff.
   * Returns numeric hashcode.
   */
  @override
  int get hashCode =>
      operation.hashCode ^ text.hashCode;
}
