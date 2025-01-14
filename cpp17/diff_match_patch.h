/*
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

#ifndef DIFF_MATCH_PATCH_H
#define DIFF_MATCH_PATCH_H

#include "diff_match_patch_utils.h"

#include <cstdint>
#include <map>
#include <regex>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>
#ifdef USE_GTEST
    #include "gtest/gtest.h"
#endif

/*
 * Functions for diff, match and patch.
 * Computes the difference between two texts to create a patch.
 * Applies the patch onto another text, allowing for errors.
 *
 * @author fraser@google.com (Neil Fraser)
 *
 * Qt/C++ port by mikeslemmer@gmail.com (Mike Slemmer):
 * Qt->C++17 with native STL use only, port by scott@towel42.com (Scott Aron
 Bloom)
 *   C++17 was intentionally chosen for variant support
 *
 * Code known to compile with C++17 VS2022 and g++ 9.5.0
 *
 * Here is a trivial sample program which works properly when linked with this
 * library:
 *

 #include "diff_match_patch.h"
 int main(int argc, char **argv) {
   auto str1 = std::string("First string in diff");
   auto str2 = std::string("Second string in diff");

   diff_match_patch dmp;
   auto strPatch = dmp.patch_toText(dmp.patch_make(str1, str2));
   auto out = dmp.patch_apply(dmp.patch_fromText(strPatch), str1);
   auto strResult = out.first;

   // here, strResult will equal str2 above.
   return 0;
 }

*/

namespace NDiffMatchPatch
{
    class diff_match_patch_test;

    /**-
 * The data structure representing a diff is a Linked list of Diff objects:
 * {Diff(Operation.DELETE, "Hello"), Diff(Operation.INSERT, "Goodbye"),
 *  Diff(Operation.EQUAL, " world.")}
 * which means: delete "Hello", add "Goodbye" and keep " world."
 */
    enum class EOperation
    {
        eDELETE,
        eINSERT,
        eEQUAL
    };
    std::wstring toString( EOperation op );

    enum class EStringType
    {
        eDefault,
        eUnitTest,
        ePatch
    };
    /**
 * Class representing one diff operation.
 */
    class Diff
    {
    public:
        // The text associated with this diff operation.

        /**
   * Constructor.  Initializes the diff with the provided values.
   * @param operation One of INSERT, DELETE or EQUAL.
   * @param text The text being applied.
   */
        Diff() = default;

        template< typename STRING_TYPE >
        Diff( EOperation _operation, const STRING_TYPE &_text ) :
            fOperation( _operation ),
            fText( NUtils::to_wstring( _text ) )
        {
        }

        template< typename STRING_TYPE >
        Diff( const Diff &rhs, const STRING_TYPE &_text ) :
            fOperation( rhs.fOperation ),
            fText( NUtils::to_wstring( _text ) )
        {
        }

        inline bool isNull() const;
        std::wstring toString( EStringType stringType ) const;
        std::wstring toHtml() const;
        std::wstring toConsole() const;
        std::wstring toDelta() const;

        bool isDelete() const { return fOperation == EOperation::eDELETE; }
        bool isInsert() const { return fOperation == EOperation::eINSERT; }
        bool isEqual() const { return fOperation == EOperation::eEQUAL; }

        const std::wstring &text() const { return fText; }

        bool operator==( const Diff &d ) const;
        bool operator!=( const Diff &d ) const;

        EOperation fOperation{ EOperation::eDELETE };
        std::wstring fText;
    };
    using TDiffVector = std::vector< Diff >;

    /**
 * Class representing one patch operation.
 */
    class Patch
    {
    public:
        TDiffVector diffs;
        std::size_t length1{ 0 };
        std::size_t length2{ 0 };
        std::size_t start1{ 0 };
        std::size_t start2{ 0 };

        /**
   * Constructor.  Initializes with an empty list of diffs.
   */
        Patch();
        Patch( std::wstring &text );   // modifies text, and removes the text used
        bool isNull() const;
        std::wstring toString() const;

    private:
        std::wstring getPatchHeader() const;
        std::wstring getCoordinateString( std::size_t start, std::size_t length ) const;
    };
    using TPatchVector = std::vector< Patch >;

    /**
 * Class containing the diff, match and patch methods.
 * Also contains the behaviour settings.
 */

    class diff_match_patch
    {
        friend class NDiffMatchPatch::diff_match_patch_test;
#ifdef USE_GTEST
        FRIEND_TEST( diff_match_patch_test, testDiffCommonOverlap );
        FRIEND_TEST( diff_match_patch_test, testDiffHalfmatch );
        FRIEND_TEST( diff_match_patch_test, testDiffLinesToChars );
        FRIEND_TEST( diff_match_patch_test, testDiffCharsToLines );
        FRIEND_TEST( diff_match_patch_test, testDiffBisect );
        FRIEND_TEST( diff_match_patch_test, testMatchAlphabet );
        FRIEND_TEST( diff_match_patch_test, testMatchBitap );
        FRIEND_TEST( diff_match_patch_test, testPatchAddContext );
#endif
    public:
        // Defaults.
        // Set these on your diff_match_patch instance to override the defaults.

        // Number of seconds to map a diff before giving up (0 for infinity).
        float Diff_Timeout{ 1.0f };
        // Cost of an empty edit operation in terms of edit characters.
        short Diff_EditCost{ 4 };
        // At what point is no match declared (0.0 = perfection, 1.0 = very loose).
        float Match_Threshold{ 0.5f };
        // How far to search for a match (0 = exact location, 1000+ = broad match).
        // A match this many characters away from the expected location will add
        // 1.0 to the score (0.0 is a perfect match).
        int64_t Match_Distance{ 1000 };
        // When deleting a large block of text (over ~64 characters), how close does
        // the contents have to match the expected contents. (0.0 = perfection,
        // 1.0 = very loose).  Note that Match_Threshold controls how closely the
        // end points of a delete need to match.
        float Patch_DeleteThreshold{ 0.5f };
        // Chunk size for context length.
        short Patch_Margin{ 4 };

        short Match_MaxBits{ 32 };   // unit tests are based on 32 bits

    private:
        // Define some regex patterns for matching boundaries.
        static std::wregex BLANKLINEEND;
        static std::wregex BLANKLINESTART;

    public:
        using TStringVector = std::vector< std::wstring >;
        using TVariant = std::variant< std::wstring, TStringVector >;
        using TVariantVector = std::vector< TVariant >;
        using TCharPosMap = std::map< wchar_t, std::size_t >;

        diff_match_patch();

        //  DIFF FUNCTIONS

        /**
   * Find the differences between two texts.
   * Run a faster slightly less optimal diff.
   * This method allows the 'checklines' of diff_main() to be optional.
   * Most of the time checklines is wanted, so default to true.
   * @param text1 Old string to be diffed.
   * @param text2 New string to be diffed.
   * @return Linked List of Diff objects.
   */
        TDiffVector diff_main( const std::wstring &text1, const std::wstring &text2 );
        TDiffVector diff_main( const std::string &text1, const std::string &text2 );

        /**
   * Find the differences between two texts.
   * @param text1 Old string to be diffed.
   * @param text2 New string to be diffed.
   * @param checklines Speedup flag.  If false, then don't run a
   *     line-level diff first to identify the changed areas.
   *     If true, then run a faster slightly less optimal diff.
   * @return Linked List of Diff objects.
   */
        TDiffVector diff_main( const std::wstring &text1, const std::wstring &text2, bool checklines );
        TDiffVector diff_main( const std::string &text1, const std::string &text2, bool checklines );

        /**
   * Find the differences between two texts.  Simplifies the problem by
   * stripping any common prefix or suffix off the texts before diffing.
   * @param text1 Old string to be diffed.
   * @param text2 New string to be diffed.
   * @param checklines Speedup flag.  If false, then don't run a
   *     line-level diff first to identify the changed areas.
   *     If true, then run a faster slightly less optimal diff.
   * @param deadline Time when the diff should be complete by.  Used
   *     internally for recursive calls.  Users should set DiffTimeout instead.
   * @return Linked List of Diff objects.
   */
    private:
        TDiffVector diff_main( const std::wstring &text1, const std::wstring &text2, bool checklines, clock_t deadline );
        TDiffVector diff_main( const std::string &text1, const std::string &text2, bool checklines, clock_t deadline );

        /**
   * Find the differences between two texts.  Assumes that the texts do not
   * have any common prefix or suffix.
   * @param text1 Old string to be diffed.
   * @param text2 New string to be diffed.
   * @param checklines Speedup flag.  If false, then don't run a
   *     line-level diff first to identify the changed areas.
   *     If true, then run a faster slightly less optimal diff.
   * @param deadline Time when the diff should be complete by.
   * @return Linked List of Diff objects.
   */
    private:
        TDiffVector diff_compute( const std::wstring &text1, const std::wstring &text2, bool checklines, clock_t deadline );
        TDiffVector diff_compute( const std::string &text1, const std::string &text2, bool checklines, clock_t deadline );

        /**
   * Do a quick line-level diff on both strings, then rediff the parts for
   * greater accuracy.
   * This speedup can produce non-minimal diffs.
   * @param text1 Old string to be diffed.
   * @param text2 New string to be diffed.
   * @param deadline Time when the diff should be complete by.
   * @return Linked List of Diff objects.
   */
    private:
        TDiffVector diff_lineMode( std::wstring text1, std::wstring text2, clock_t deadline );
        TDiffVector diff_lineMode( std::string text1, std::string text2, clock_t deadline );

        /**
   * Find the 'middle snake' of a diff, split the problem in two
   * and return the recursively constructed diff.
   * See Myers 1986 paper: An O(ND) Difference Algorithm and Its Variations.
   * @param text1 Old string to be diffed.
   * @param text2 New string to be diffed.
   * @return Linked List of Diff objects.
   */
    protected:
        TDiffVector diff_bisect( const std::wstring &text1, const std::wstring &text2, clock_t deadline );
        TDiffVector diff_bisect( const std::string &text1, const std::string &text2, clock_t deadline );

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
    private:
        TDiffVector diff_bisectSplit( const std::wstring &text1, const std::wstring &text2, std::size_t x, std::size_t y, clock_t deadline );
        TDiffVector diff_bisectSplit( const std::string &text1, const std::string &text2, std::size_t x, std::size_t y, clock_t deadline );

        /**
   * Split two texts into a list of strings.  Reduce the texts to a string of
   * hashes where each Unicode character represents one line.
   * @param text1 First string.
   * @param text2 Second string.
   * @return Three element Object array, containing the encoded text1, the
   *     encoded text2 and the List of unique strings.  The zeroth element
   *     of the List of unique strings is intentionally blank.
   */
    protected:
        std::vector< TVariant > diff_linesToChars( const std::wstring &text1,
                                                   const std::wstring &text2 );   // return elems 0 and 1 are std::wstring,
        // elem 2 is TStringVector
        std::vector< TVariant > diff_linesToChars( const std::string &text1, const std::string &text2 );

        /**
   * Split a text into a list of strings.  Reduce the texts to a string of
   * hashes where each Unicode character represents one line.
   * @param text String to encode.
   * @param lineArray List of unique strings.
   * @param lineHash Map of strings to indices.
   * @return Encoded string.
   */
    private:
        std::wstring diff_linesToCharsMunge( const std::wstring &text, TStringVector &lineArray, std::unordered_map< std::wstring, std::size_t > &lineHash );

        /**
   * Rehydrate the text in a diff from a string of line hashes to real lines of
   * text.
   * @param diffs LinkedList of Diff objects.
   * @param lineArray List of unique strings.
   */
    private:
        void diff_charsToLines( TDiffVector &diffs, const TStringVector &lineArray );

        /**
   * Determine the common prefix of two strings.
   * @param text1 First string.
   * @param text2 Second string.
   * @return The number of characters common to the start of each string.
   */
    public:
        std::size_t diff_commonPrefix( const std::wstring &text1, const std::wstring &text2 );
        std::size_t diff_commonPrefix( const std::string &text1, const std::string &text2 );

        /**
   * Determine the common suffix of two strings.
   * @param text1 First string.
   * @param text2 Second string.
   * @return The number of characters common to the end of each string.
   */
    public:
        std::size_t diff_commonSuffix( const std::wstring &text1, const std::wstring &text2 );
        std::size_t diff_commonSuffix( const std::string &text1, const std::string &text2 );

        /**
   * Determine if the suffix of one string is the prefix of another.
   * @param text1 First string.
   * @param text2 Second string.
   * @return The number of characters common to the end of the first
   *     string and the start of the second string.
   */
    protected:
        std::size_t diff_commonOverlap( const std::wstring &text1, const std::wstring &text2 );
        std::size_t diff_commonOverlap( const std::string &text1, const std::string &text2 );

        /**
   * Do the two texts share a substring which is at least half the length of
   * the longer text?
   * This speedup can produce non-minimal diffs.
   * @param text1 First string.
   * @param text2 Second string.
   * @return Five element String array, containing the prefix of text1, the
   *     suffix of text1, the prefix of text2, the suffix of text2 and the
   *     common middle.  Or null if there was no match.
   */
    protected:
        TStringVector diff_halfMatch( const std::wstring &text1, const std::wstring &text2 );
        TStringVector diff_halfMatch( const std::string &text1, const std::string &text2 );

        /**
   * Does a substring of shorttext exist within longtext such that the
   * substring is at least half the length of longtext?
   * @param longtext Longer string.
   * @param shorttext Shorter string.
   * @param i Start index of quarter length substring within longtext.
   * @return Five element String array, containing the prefix of longtext, the
   *     suffix of longtext, the prefix of shorttext, the suffix of shorttext
   *     and the common middle.  Or null if there was no match.
   */
    private:
        TStringVector diff_halfMatchI( const std::wstring &longtext, const std::wstring &shorttext, std::size_t i );
        TStringVector diff_halfMatchI( const std::string &longtext, const std::string &shorttext, std::size_t i );

        /**
   * Reduce the number of edits by eliminating semantically trivial equalities.
   * @param diffs LinkedList of Diff objects.
   */
    public:
        void diff_cleanupSemantic( TDiffVector &diffs );

        /**
   * Look for single edits surrounded on both sides by equalities
   * which can be shifted sideways to align the edit to a word boundary.
   * e.g: The c<ins>at c</ins>ame. -> The <ins>cat </ins>came.
   * @param diffs LinkedList of Diff objects.
   */
    public:
        void diff_cleanupSemanticLossless( TDiffVector &diffs );

        /**
   * Given two strings, compute a score representing whether the internal
   * boundary falls on logical boundaries.
   * Scores range from 6 (best) to 0 (worst).
   * @param one First string.
   * @param two Second string.
   * @return The score.
   */
    private:
        int64_t diff_cleanupSemanticScore( const std::wstring &one, const std::wstring &two );
        int64_t diff_cleanupSemanticScore( const std::string &one, const std::string &two );

        /**
   * Reduce the number of edits by eliminating operationally trivial equalities.
   * @param diffs LinkedList of Diff objects.
   */
    public:
        void diff_cleanupEfficiency( TDiffVector &diffs );

        /**
   * Reorder and merge like edit sections.  Merge equalities.
   * Any edit section can move as long as it doesn't cross an equality.
   * @param diffs LinkedList of Diff objects.
   */
    public:
        void diff_cleanupMerge( TDiffVector &diffs );

        /**
   * loc is a location in text1, compute and return the equivalent location in
   * text2.
   * e.g. "The cat" vs "The big cat", 1->1, 5->8
   * @param diffs LinkedList of Diff objects.
   * @param loc Location within text1.
   * @return Location within text2.
   */
    public:
        std::size_t diff_xIndex( const TDiffVector &diffs, std::size_t loc );

        /**
   * Convert a Diff list into a pretty HTML report.
   * @param diffs LinkedList of Diff objects.
   * @return HTML representation.
   */
    public:
        std::wstring diff_prettyHtml( const TDiffVector &diffs );

        /**
   * Convert a Diff list into a pretty Console report. Red for delete, and green for insert
   * @param diffs LinkedList of Diff objects.
   * @return Console representation.
   */
    public:
        std::wstring diff_prettyConsole( const TDiffVector &diffs );

        /**
   * Compute and return the source text (all equalities and deletions).
   * @param diffs LinkedList of Diff objects.
   * @return Source text.
   */
    public:
        std::wstring diff_text1( const TDiffVector &diffs );

        /**
   * Compute and return the destination text (all equalities and insertions).
   * @param diffs LinkedList of Diff objects.
   * @return Destination text.
   */
    public:
        std::wstring diff_text2( const TDiffVector &diffs );

        /**
   * Compute the Levenshtein distance; the number of inserted, deleted or
   * substituted characters.
   * @param diffs LinkedList of Diff objects.
   * @return Number of changes.
   */
    public:
        std::size_t diff_levenshtein( const TDiffVector &diffs );

        /**
   * Crush the diff into an encoded string which describes the operations
   * required to transform text1 into text2.
   * E.g. =3\t-2\t+ing  -> Keep 3 chars, delete 2 chars, insert 'ing'.
   * Operations are tab-separated.  Inserted text is escaped using %xx notation.
   * @param diffs Array of diff tuples.
   * @return Delta text.
   */
    public:
        std::wstring diff_toDelta( const TDiffVector &diffs );

        /**
   * Given the original text1, and an encoded string which describes the
   * operations required to transform text1 into text2, compute the full diff.
   * @param text1 Source string for the diff.
   * @param delta Delta text.
   * @return Array of diff tuples or null if invalid.
   * @throws std::wstring If invalid input.
   */
    public:
        TDiffVector diff_fromDelta( const std::wstring &text1, const std::wstring &delta );
        TDiffVector diff_fromDelta( const std::string &text1, const std::string &delta );

        //  MATCH FUNCTIONS

        /**
   * Locate the best instance of 'pattern' in 'text' near 'loc'.
   * Returns -1 if no match found.
   * @param text The text to search.
   * @param pattern The pattern to search for.
   * @param loc The location to search around.
   * @return Best match index or -1.
   */
    public:
        std::size_t match_main( const std::wstring &text, const std::wstring &pattern, std::size_t loc );
        std::size_t match_main( const std::string &text, const std::string &pattern, std::size_t loc );

        /**
   * Locate the best instance of 'pattern' in 'text' near 'loc' using the
   * Bitap algorithm.  Returns -1 if no match found.
   * @param text The text to search.
   * @param pattern The pattern to search for.
   * @param loc The location to search around.
   * @return Best match index or -1.
   */
    protected:
        std::size_t match_bitap( const std::wstring &text, const std::wstring &pattern, std::size_t loc );
        std::size_t match_bitap( const std::string &text, const std::string &pattern, std::size_t loc );

        /**
   * Compute and return the score for a match with e errors and x location.
   * @param e Number of errors in match.
   * @param x Location of match.
   * @param loc Expected location of match.
   * @param pattern Pattern being sought.
   * @return Overall score for match (0.0 = good, 1.0 = bad).
   */
    private:
        double match_bitapScore( int64_t e, int64_t x, int64_t loc, const std::wstring &pattern );

        /**
   * Initialise the alphabet for the Bitap algorithm.
   * @param pattern The text to encode.
   * @return Hash of character locations.
   */
    protected:
        TCharPosMap match_alphabet( const std::wstring &pattern );
        TCharPosMap match_alphabet( const std::string &pattern );

        //  PATCH FUNCTIONS

        /**
   * Increase the context until it is unique,
   * but don't let the pattern expand beyond Match_MaxBits.
   * @param patch The patch to grow.
   * @param text Source text.
   */
    protected:
        void patch_addContext( Patch &patch, const std::wstring &text );
        void patch_addContext( Patch &patch, const std::string &text );

        /**
   * Compute a list of patches to turn text1 into text2.
   * A set of diffs will be computed.
   * @param text1 Old text.
   * @param text2 New text.
   * @return LinkedList of Patch objects.
   */
    public:
        TPatchVector patch_make( const std::wstring &text1, const std::wstring &text2 );
        TPatchVector patch_make( const std::string &text1, const std::string &text2 );

        /**
   * Compute a list of patches to turn text1 into text2.
   * text1 will be derived from the provided diffs.
   * @param diffs Array of diff tuples for text1 to text2.
   * @return LinkedList of Patch objects.
   */
    public:
        TPatchVector patch_make( const TDiffVector &diffs );

        /**
   * Compute a list of patches to turn text1 into text2.
   * text2 is ignored, diffs are the delta between text1 and text2.
   * @param text1 Old text.
   * @param text2 Ignored.
   * @param diffs Array of diff tuples for text1 to text2.
   * @return LinkedList of Patch objects.
   * @deprecated Prefer patch_make(const std::wstring &text1, const std::list<
   * Diff > &diffs).
   */
    public:
        TPatchVector patch_make( const std::wstring &text1, const std::wstring &text2, const TDiffVector &diffs );
        TPatchVector patch_make( const std::string &text1, const std::string &text2, const TDiffVector &diffs );

        /**
   * Compute a list of patches to turn text1 into text2.
   * text2 is not provided, diffs are the delta between text1 and text2.
   * @param text1 Old text.
   * @param diffs Array of diff tuples for text1 to text2.
   * @return LinkedList of Patch objects.
   */
    public:
        TPatchVector patch_make( const std::wstring &text1, const TDiffVector &diffs );
        TPatchVector patch_make( const std::string &text1, const TDiffVector &diffs );

        /**
   * Given an array of patches, return another array that is identical.
   * @param patches Array of patch objects.
   * @return Array of patch objects.
   */
    public:
        TPatchVector patch_deepCopy( const TPatchVector &patches );

        /**
   * Merge a set of patches onto the text.  Return a patched text, as well
   * as an array of true/false values indicating which patches were applied.
   * @param patches Array of patch objects.
   * @param text Old text.
   * @return Two element Object array, containing the new text and an array of
   *      boolean values.
   */
    public:
        std::pair< std::wstring, std::vector< bool > > patch_apply( TPatchVector patches, std::wstring text );
        std::pair< std::wstring, std::vector< bool > > patch_apply( TPatchVector patches, std::string text );

        /**
   * Add some padding on text start and end so that edges can match something.
   * Intended to be called only from within patch_apply.
   * @param patches Array of patch objects.
   * @return The padding string added to each side.
   */
    public:
        std::wstring patch_addPadding( TPatchVector &patches );

        /**
   * Look through the patches and break up any which are longer than the
   * maximum limit of the match algorithm.
   * Intended to be called only from within patch_apply.
   * @param patches LinkedList of Patch objects.
   */
    public:
        void patch_splitMax( TPatchVector &patches );

        /**
   * Take a list of patches and return a textual representation.
   * @param patches List of Patch objects.
   * @return Text representation of patches.
   */
    public:
        std::wstring patch_toText( const TPatchVector &patches );

        /**
   * Parse a textual representation of patches and return a List of Patch
   * objects.
   * @param textline Text representation of patches.
   * @return List of Patch objects.
   * @throws std::wstring If invalid input.
   */
    public:
        TPatchVector patch_fromText( const std::wstring &textline );
        TPatchVector patch_fromText( const std::string &textline );

        /**
   * A safer version of std::wstring.mid(pos).  This one returns "" instead of
   * null when the postion equals the string length.
   * @param str String to take a substring from.
   * @param pos Position to start the substring from.
   * @return Substring.
   */
    private:
        static std::wstring safeMid( const std::wstring &str, std::size_t pos );

        /**
   * A safer version of std::wstring.mid(pos, len).  This one returns "" instead
   * of null when the postion equals the string length.
   * @param str String to take a substring from.
   * @param pos Position to start the substring from.
   * @param len Length of substring.
   * @return Substring.
   */
    private:
        static std::wstring safeMid( const std::wstring &str, std::size_t pos, std::size_t len );
    };

    namespace NUtils
    {
        std::wstring to_wstring( const diff_match_patch::TVariant &variant, bool doubleQuoteEmpty );
        std::wstring to_wstring( const Diff &diff, bool doubleQuoteEmpty );
        std::wstring to_wstring( const Patch &patch, bool doubleQuoteEmpty );
    }   // namespace NUtils
}
#endif   // DIFF_MATCH_PATCH_H
