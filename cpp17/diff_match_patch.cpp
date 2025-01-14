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

#include "diff_match_patch.h"

#include <algorithm>
#include <codecvt>
#include <ctime>
#include <cwctype>
#include <limits>
#include <list>
#include <stack>
#include <unordered_map>

#include "diff_match_patch_utils.h"

namespace NDiffMatchPatch
{
#ifdef WIN32
    static std::size_t kZERO{ 0ULL };
    static std::size_t kONE{ 1ULL };
#else
    static unsigned long kZERO{ 0UL };
    static unsigned long kONE{ 1UL };
#endif

    std::wstring toString( EOperation op )
    {
        switch ( op )
        {
            case EOperation::eINSERT:
                return L"INSERT";
            case EOperation::eDELETE:
                return L"DELETE";
            case EOperation::eEQUAL:
                return L"EQUAL";
        }
        throw "Invalid operation.";
    }

    //////////////////////////
    //
    // Diff Class
    //
    //////////////////////////

    /**
 * Constructor.  Initializes the diff with the provided values.
 * @param operation One of INSERT, DELETE or EQUAL
 * @param text The text being applied
 */

    /**
 * Display a human-readable version of this Diff.
 * @return text version
 */
    std::wstring Diff::toString( EStringType stringType ) const
    {
        std::wstring retVal;
        if ( stringType == EStringType::ePatch )
        {
            switch ( fOperation )
            {
                case EOperation::eINSERT:
                    retVal += L"+";
                    break;
                case EOperation::eDELETE:
                    retVal += L"-";
                    break;
                case EOperation::eEQUAL:
                    retVal += L" ";
                    break;
            }
            retVal += NUtils::toPercentEncoding( fText, L" !~*'();/?:@&=+$,#" ) + std::wstring( L"\n" );
        }
        else if ( stringType == EStringType::eUnitTest )
        {
            retVal = L"(" + NDiffMatchPatch::toString( fOperation ) + L" " + text() + L")";
        }
        else if ( stringType == EStringType::eDefault )
        {
            retVal = fText;
            // Replace linebreaks with Pilcrow signs.
            std::replace( retVal.begin(), retVal.end(), L'\n', L'\u00b6' );
            retVal = std::wstring( L"Diff(" ) + NDiffMatchPatch::toString( fOperation ) + std::wstring( L",\"" ) + retVal + std::wstring( L"\")" );
        }
        return retVal;
    }

    std::wstring Diff::toHtml() const
    {
        auto text = this->text();
        NUtils::replace( text, L"&", L"&amp;" );
        NUtils::replace( text, L"<", L"&lt;" );
        NUtils::replace( text, L">", L"&gt;" );
        NUtils::replace( text, L"\n", L"&para;<br>" );
        if ( isInsert() )
            return std::wstring( L"<ins style=\"background:#e6ffe6;\">" ) + text + std::wstring( L"</ins>" );
        else if ( isDelete() )
            return std::wstring( L"<del style=\"background:#ffe6e6;\">" ) + text + std::wstring( L"</del>" );
        else if ( isEqual() )
            return std::wstring( L"<span>" ) + text + std::wstring( L"</span>" );
        return {};
    }

    std::wstring Diff::toConsole() const
    {
        static std::wstring kRed{ L"\033[0;31m" };
        static std::wstring kGreen{ L"\033[0;32m" };
        static std::wstring kYellow{ L"\033[0;33m" };
        static std::wstring kReset{ L"\033[m" };
        static std::wstring kEOL{ NUtils::fromPercentEncoding( L"%C2%B6" ) + L"\n" };

        auto retVal = text();
        NUtils::replace( retVal, L"\n", kEOL );
        if ( isInsert() )
            return kGreen + retVal + kReset;
        else if ( isDelete() )
            return kRed + retVal + kReset;
        else if ( isEqual() )
            return retVal;
        return {};
    }

    std::wstring Diff::toDelta() const
    {
        if ( isInsert() )
            return L"+" + NUtils::toPercentEncoding( text(), L" !~*'();/?:@&=+$,#" ) + L"\t";
        else if ( isDelete() )
            return L"-" + std::to_wstring( text().length() ) + L"\t";
        else if ( isEqual() )
            return L"=" + std::to_wstring( text().length() ) + L"\t";
        return {};
    }

    /**
 * Is this Diff equivalent to another Diff?
 * @param d Another Diff to compare against
 * @return true or false
 */
    bool Diff::operator==( const Diff &d ) const
    {
        return ( d.fOperation == fOperation ) && ( d.fText == fText );
    }

    bool Diff::operator!=( const Diff &d ) const
    {
        return !( operator==( d ) );
    }

    /////////////////////////////////////////////
    //
    // Patch Class
    //
    /////////////////////////////////////////////

    /**
 * Constructor.  Initializes with an empty list of diffs.
 */
    Patch::Patch()
    {
    }

    Patch::Patch( std::wstring &text )
    {
        std::wsmatch matches;
        auto patchHeader = std::wregex( LR"(^@@ -(\d+),?(\d*) \+(\d+),?(\d*) @@$)" );
        if ( !std::regex_match( text, matches, patchHeader ) || ( matches.size() != 5 ) )
        {
            throw std::wstring( L"Invalid patch string: " + text );
        }
        start1 = NUtils::toInt( matches[ 1 ].str() );
        if ( !matches[ 2 ].length() )
        {
            start1--;
            length1 = 1;
        }
        else if ( matches[ 2 ].str() == L"0" )
        {
            length1 = 0;
        }
        else
        {
            start1--;
            length1 = NUtils::toInt( matches[ 2 ].str() );
        }

        start2 = NUtils::toInt( matches[ 3 ].str() );
        if ( !matches[ 4 ].length() )
        {
            start2--;
            length2 = 1;
        }
        else if ( matches[ 4 ].str() == L"0" )
        {
            length2 = 0;
        }
        else
        {
            start2--;
            length2 = NUtils::toInt( matches[ 4 ].str() );
        }
        text.erase( text.begin() );
    }

    bool Patch::isNull() const
    {
        if ( start1 == 0 && start2 == 0 && length1 == 0 && length2 == 0 && diffs.empty() )
        {
            return true;
        }
        return false;
    }

    /**
 * Emulate GNU diff's format.
 * Header: @@ -382,8 +481,9 @@
 * Indices are printed as 1-based, not 0-based.
 * @return The GNU diff string
 */
    std::wstring Patch::toString() const
    {
        auto text = getPatchHeader();
        // Escape the body of the patch with %xx notation.
        for ( auto &&aDiff : diffs )
        {
            text += aDiff.toString( EStringType::ePatch );
        }

        return text;
    }

    std::wstring Patch::getPatchHeader() const
    {
        auto coords1 = getCoordinateString( start1, length1 );
        auto coords2 = getCoordinateString( start2, length2 );
        auto text = std::wstring( L"@@ -" ) + coords1 + std::wstring( L" +" ) + coords2 + std::wstring( L" @@\n" );
        return text;
    }

    std::wstring Patch::getCoordinateString( std::size_t start, std::size_t length ) const
    {
        std::wstring retVal;
        if ( length == 0 )
        {
            retVal = std::to_wstring( start ) + std::wstring( L",0" );
        }
        else if ( length == 1 )
        {
            retVal = std::to_wstring( start + 1 );
        }
        else
        {
            retVal = std::to_wstring( start + 1 ) + std::wstring( L"," ) + std::to_wstring( length );
        }
        return retVal;
    }

    /////////////////////////////////////////////
    //
    // diff_match_patch Class
    //
    /////////////////////////////////////////////

    // all class members initialized in the class
    diff_match_patch::diff_match_patch()
    {
    }

    TDiffVector diff_match_patch::diff_main( const std::wstring &text1, const std::wstring &text2 )
    {
        return diff_main( text1, text2, true );
    }

    TDiffVector diff_match_patch::diff_main( const std::wstring &text1, const std::wstring &text2, bool checklines )
    {
        // Set a deadline by which time the diff must be complete.
        clock_t deadline;
        if ( Diff_Timeout <= 0 )
        {
            deadline = std::numeric_limits< clock_t >::max();
        }
        else
        {
            deadline = clock() + (clock_t)( Diff_Timeout * CLOCKS_PER_SEC );
        }
        return diff_main( text1, text2, checklines, deadline );
    }

    TDiffVector diff_match_patch::diff_main( const std::wstring &text1, const std::wstring &text2, bool checklines, clock_t deadline )
    {
        // Check for equality (speedup).
        TDiffVector diffs;
        if ( text1 == text2 )
        {
            if ( !text1.empty() )
            {
                diffs.emplace_back( EOperation::eEQUAL, text1 );
            }
            return diffs;
        }

        if ( !text1.empty() && text2.empty() )
        {
            diffs.emplace_back( EOperation::eDELETE, text1 );
            return diffs;
        }

        if ( text1.empty() && !text2.empty() )
        {
            diffs.emplace_back( EOperation::eINSERT, text2 );
            return diffs;
        }

        // Trim off common prefix (speedup).
        auto commonlength = diff_commonPrefix( text1, text2 );
        auto commonprefix = text1.substr( 0, commonlength );
        auto textChopped1 = text1.substr( commonlength );
        auto textChopped2 = text2.substr( commonlength );

        // Trim off common suffix (speedup).
        commonlength = diff_commonSuffix( textChopped1, textChopped2 );
        auto commonsuffix = textChopped1.substr( textChopped1.length() - commonlength );
        textChopped1 = textChopped1.substr( 0, textChopped1.length() - commonlength );
        textChopped2 = textChopped2.substr( 0, textChopped2.length() - commonlength );

        // Compute the diff on the middle block.
        diffs = diff_compute( textChopped1, textChopped2, checklines, deadline );

        // Restore the prefix and suffix.
        if ( !commonprefix.empty() )
        {
            diffs.emplace( diffs.begin(), EOperation::eEQUAL, commonprefix );
        }
        if ( !commonsuffix.empty() )
        {
            diffs.emplace_back( EOperation::eEQUAL, commonsuffix );
        }

        diff_cleanupMerge( diffs );

        return diffs;
    }

    TDiffVector diff_match_patch::diff_main( const std::string &text1, const std::string &text2 )
    {
        return diff_main( NUtils::to_wstring( text1 ), NUtils::to_wstring( text2 ) );
    }

    TDiffVector diff_match_patch::diff_main( const std::string &text1, const std::string &text2, bool checklines )
    {
        return diff_main( NUtils::to_wstring( text1 ), NUtils::to_wstring( text2 ), checklines );
    }

    TDiffVector diff_match_patch::diff_main( const std::string &text1, const std::string &text2, bool checklines, clock_t deadline )
    {
        return diff_main( NUtils::to_wstring( text1 ), NUtils::to_wstring( text2 ), checklines, deadline );
    }

    TDiffVector diff_match_patch::diff_compute( const std::wstring &text1, const std::wstring &text2, bool checklines, clock_t deadline )
    {
        TDiffVector diffs;

        if ( text1.empty() )
        {
            // Just add some text (speedup).
            diffs.emplace_back( EOperation::eINSERT, text2 );
            return diffs;
        }

        if ( text2.empty() )
        {
            // Just delete some text (speedup).
            diffs.emplace_back( EOperation::eDELETE, text1 );
            return diffs;
        }

        {
            auto [ longtext, shorttext ] = ( text1.length() > text2.length() ) ? std::make_pair( text1, text2 ) : std::make_pair( text2, text1 );
            auto i = longtext.find( shorttext );
            if ( i != std::string::npos )
            {
                // Shorter text is inside the longer text (speedup).
                const auto op = ( text1.length() > text2.length() ) ? EOperation::eDELETE : EOperation::eINSERT;
                diffs.emplace_back( op, longtext.substr( 0, i ) );
                diffs.emplace_back( EOperation::eEQUAL, shorttext );
                diffs.emplace_back( op, safeMid( longtext, i + shorttext.length() ) );
                return diffs;
            }

            if ( shorttext.length() == 1 )
            {
                // Single character string.
                // After the previous speedup, the character can't be an equality.
                diffs.emplace_back( EOperation::eDELETE, text1 );
                diffs.emplace_back( EOperation::eINSERT, text2 );
                return diffs;
            }
            // Garbage collect longtext and shorttext by scoping out.
        }

        // Check to see if the problem can be split in two.
        const TStringVector hm = diff_halfMatch( text1, text2 );
        if ( !hm.empty() )
        {
            // A half-match was found, sort out the return data.
            auto &&text1_a = hm[ 0 ];
            auto &&text1_b = hm[ 1 ];
            auto &&text2_a = hm[ 2 ];
            auto &&text2_b = hm[ 3 ];
            auto &&mid_common = hm[ 4 ];
            // Send both pairs off for separate processing.
            diffs = diff_main( text1_a, text2_a, checklines, deadline );
            const auto &&diffs_b = diff_main( text1_b, text2_b, checklines, deadline );
            // Merge the results.
            diffs.emplace_back( EOperation::eEQUAL, mid_common );
            diffs.insert( diffs.end(), diffs_b.begin(), diffs_b.end() );
            return diffs;
        }

        // Perform a real diff.
        if ( checklines && ( text1.length() > 100 ) && ( text2.length() > 100 ) )
        {
            return diff_lineMode( text1, text2, deadline );
        }

        return diff_bisect( text1, text2, deadline );
    }

    TDiffVector diff_match_patch::diff_compute( const std::string &text1, const std::string &text2, bool checklines, clock_t deadline )
    {
        return diff_compute( NUtils::to_wstring( text1 ), NUtils::to_wstring( text2 ), checklines, deadline );
    }

    TDiffVector diff_match_patch::diff_lineMode( std::wstring text1, std::wstring text2, clock_t deadline )
    {
        // Scan the text on a line-by-line basis first.
        auto a = diff_linesToChars( text1, text2 );
        text1 = std::get< std::wstring >( a[ 0 ] );
        text2 = std::get< std::wstring >( a[ 1 ] );
        auto linearray = std::get< TStringVector >( a[ 2 ] );

        auto diffs = diff_main( text1, text2, false, deadline );

        // Convert the diff back to original text.
        diff_charsToLines( diffs, linearray );
        // Eliminate freak matches (e.g. blank lines)
        diff_cleanupSemantic( diffs );

        // Rediff any replacement blocks, this time character-by-character.
        // Add a dummy entry at the end.
        diffs.emplace_back( EOperation::eEQUAL, L"" );
        std::size_t pointer = 0;
        int count_delete = 0;
        int count_insert = 0;
        std::wstring text_delete;
        std::wstring text_insert;
        while ( pointer < diffs.size() )
        {
            if ( diffs[ pointer ].isInsert() )
            {
                count_insert++;
                text_insert += diffs[ pointer ].text();
            }
            else if ( diffs[ pointer ].isDelete() )
            {
                count_delete++;
                text_delete += diffs[ pointer ].text();
            }
            else if ( diffs[ pointer ].isEqual() )
            {   // Upon reaching an equality, check for prior redundancies.
                if ( count_delete >= 1 && count_insert >= 1 )
                {
                    // Delete the offending records and add the merged ones.
                    auto numElements = count_delete + count_insert;
                    auto start = diffs.begin() + pointer - numElements;
                    auto end = start + numElements;
                    diffs.erase( start, end );
                    pointer = pointer - count_delete - count_insert;
                    auto subDiff = diff_main( text_delete, text_insert, false, deadline );
                    diffs.insert( diffs.begin() + pointer, subDiff.begin(), subDiff.end() );
                    pointer = pointer + subDiff.size();
                }
                count_insert = 0;
                count_delete = 0;
                text_delete.clear();
                text_insert.clear();
            }
            pointer++;
        }
        diffs.pop_back();   // Remove the dummy entry at the end.

        return diffs;
    }

    TDiffVector diff_match_patch::diff_lineMode( std::string text1, std::string text2, clock_t deadline )
    {
        return diff_lineMode( NUtils::to_wstring( text1 ), NUtils::to_wstring( text2 ), deadline );
    }

    // using int64_t rather thant size_t due to the backward walking nature of the
    // algorithm
    TDiffVector diff_match_patch::diff_bisect( const std::wstring &text1, const std::wstring &text2, clock_t deadline )
    {
        // Cache the text lengths to prevent multiple calls.
        auto text1_length = static_cast< int64_t >( text1.length() );
        auto text2_length = static_cast< int64_t >( text2.length() );
        auto max_d = ( text1_length + text2_length + 1 ) / 2;
        auto v_offset = max_d;
        auto v_length = 2 * max_d;
        auto v1 = std::vector< int64_t >( v_length, -1 );
        auto v2 = std::vector< int64_t >( v_length, -1 );
        v1[ v_offset + 1 ] = 0;
        v2[ v_offset + 1 ] = 0;
        auto delta = text1_length - text2_length;
        // If the total number of characters is odd, then the front path will
        // collide with the reverse path.
        bool front = ( delta % 2 != 0 );
        // Offsets for start and end of k loop.
        // Prevents mapping of space beyond the grid.
        int64_t k1start = 0;
        int64_t k1end = 0;
        int64_t k2start = 0;
        int64_t k2end = 0;
        for ( int64_t d = 0; d < max_d; d++ )
        {
            // Bail out if deadline is reached.
            if ( clock() > deadline )
            {
                break;
            }

            // Walk the front path one step.
            for ( auto k1 = -d + k1start; k1 <= d - k1end; k1 += 2 )
            {
                auto k1_offset = v_offset + k1;
                int64_t x1;
                if ( ( k1 == -d ) || ( k1 != d ) && ( v1[ k1_offset - 1 ] < v1[ k1_offset + 1 ] ) )
                {
                    x1 = v1[ k1_offset + 1 ];
                }
                else
                {
                    x1 = v1[ k1_offset - 1 ] + 1;
                }
                int64_t y1 = x1 - k1;
                while ( ( x1 < text1_length ) && ( y1 < text2_length ) && ( text1[ x1 ] == text2[ y1 ] ) )
                {
                    x1++;
                    y1++;
                }
                v1[ k1_offset ] = x1;
                if ( x1 > text1_length )
                {
                    // Ran off the right of the graph.
                    k1end += 2;
                }
                else if ( y1 > text2_length )
                {
                    // Ran off the bottom of the graph.
                    k1start += 2;
                }
                else if ( front )
                {
                    auto k2_offset = v_offset + delta - k1;
                    if ( ( k2_offset >= 0 ) && ( k2_offset < v_length ) && ( v2[ k2_offset ] != -1 ) )
                    {
                        // Mirror x2 onto top-left coordinate system.
                        auto x2 = text1_length - v2[ k2_offset ];
                        if ( x1 >= x2 )
                        {
                            // Overlap detected.
                            return diff_bisectSplit( text1, text2, x1, y1, deadline );
                        }
                    }
                }
            }

            // Walk the reverse path one step.
            for ( auto k2 = -d + k2start; k2 <= d - k2end; k2 += 2 )
            {
                auto k2_offset = v_offset + k2;
                int64_t x2;
                if ( ( k2 == -d ) || ( k2 != d ) && ( v2[ k2_offset - 1 ] < v2[ k2_offset + 1 ] ) )
                {
                    x2 = v2[ k2_offset + 1 ];
                }
                else
                {
                    x2 = v2[ k2_offset - 1 ] + 1;
                }
                auto y2 = x2 - k2;
                while ( ( x2 < text1_length ) && ( y2 < text2_length ) && ( text1[ text1_length - x2 - 1 ] == text2[ text2_length - y2 - 1 ] ) )
                {
                    x2++;
                    y2++;
                }
                v2[ k2_offset ] = x2;
                if ( x2 > text1_length )
                {
                    // Ran off the left of the graph.
                    k2end += 2;
                }
                else if ( y2 > text2_length )
                {
                    // Ran off the top of the graph.
                    k2start += 2;
                }
                else if ( !front )
                {
                    auto k1_offset = v_offset + delta - k2;
                    if ( ( k1_offset >= 0 ) && ( k1_offset < v_length ) && ( v1[ k1_offset ] != -1 ) )
                    {
                        auto x1 = v1[ k1_offset ];
                        auto y1 = v_offset + x1 - k1_offset;
                        // Mirror x2 onto top-left coordinate system.
                        x2 = text1_length - v2[ k2_offset ];
                        if ( x1 >= x2 )
                        {
                            // Overlap detected.
                            return diff_bisectSplit( text1, text2, x1, y1, deadline );
                        }
                    }
                }
            }
        }
        // Diff took too long and hit the deadline or
        // number of diffs equals number of characters, no commonality at all.
        auto diffs = TDiffVector( { Diff( EOperation::eDELETE, text1 ), Diff( EOperation::eINSERT, text2 ) } );
        return diffs;
    }

    TDiffVector diff_match_patch::diff_bisect( const std::string &text1, const std::string &text2, clock_t deadline )
    {
        return diff_bisect( NUtils::to_wstring( text1 ), NUtils::to_wstring( text2 ), deadline );
    }

    TDiffVector diff_match_patch::diff_bisectSplit( const std::wstring &text1, const std::wstring &text2, std::size_t x, std::size_t y, clock_t deadline )
    {
        auto text1a = text1.substr( 0, x );
        auto text2a = text2.substr( 0, y );
        auto text1b = safeMid( text1, x );
        auto text2b = safeMid( text2, y );

        // Compute both diffs serially.
        TDiffVector diffs = diff_main( text1a, text2a, false, deadline );
        TDiffVector diffsb = diff_main( text1b, text2b, false, deadline );

        diffs.insert( diffs.end(), diffsb.begin(), diffsb.end() );
        return diffs;
    }

    TDiffVector diff_match_patch::diff_bisectSplit( const std::string &text1, const std::string &text2, std::size_t x, std::size_t y, clock_t deadline )
    {
        return diff_bisectSplit( NUtils::to_wstring( text1 ), NUtils::to_wstring( text2 ), x, y, deadline );
    }

    diff_match_patch::TVariantVector diff_match_patch::diff_linesToChars( const std::wstring &text1, const std::wstring &text2 )
    {
        TStringVector lineArray;
        std::unordered_map< std::wstring, std::size_t > lineHash;
        // e.g. linearray[4] == "Hello\n"
        // e.g. linehash.get("Hello\n") == 4

        // "\x00" is a valid character, but various debuggers don't like it.
        // So we'll insert a junk entry to avoid generating a nullptr character.
        lineArray.emplace_back( L"" );

        const std::wstring chars1 = diff_linesToCharsMunge( text1, lineArray, lineHash );
        const std::wstring chars2 = diff_linesToCharsMunge( text2, lineArray, lineHash );

        TVariantVector listRet;
        listRet.emplace_back( chars1 );
        listRet.emplace_back( chars2 );
        listRet.emplace_back( lineArray );
        return listRet;
    }

    std::vector< diff_match_patch::diff_match_patch::TVariant > diff_match_patch::diff_linesToChars( const std::string &text1, const std::string &text2 )
    {
        return diff_linesToChars( NUtils::to_wstring( text1 ), NUtils::to_wstring( text2 ) );
    }

    std::wstring diff_match_patch::diff_linesToCharsMunge( const std::wstring &text, TStringVector &lineArray, std::unordered_map< std::wstring, std::size_t > &lineHash )
    {
        std::size_t lineStart = 0;
        std::size_t lineEnd = std::string::npos;
        std::wstring line;
        std::wstring chars;
        // Walk the text, pulling out a substring for each line.
        // text.split('\n') would would temporarily double our memory footprint.
        // Modifying text would create many large strings to garbage collect.
        bool firstTime = true;
        while ( ( firstTime && ( lineEnd == -1 ) && !text.empty() ) || lineEnd < ( text.length() - 1 ) )
        {
            firstTime = false;
            lineEnd = text.find( '\n', lineStart );
            if ( lineEnd == -1 )
            {
                lineEnd = text.length() - 1;
            }
            line = safeMid( text, lineStart, lineEnd + 1 - lineStart );

            auto pos = lineHash.find( line );
            if ( pos != lineHash.end() )
            {
                chars += static_cast< wchar_t >( ( *pos ).second );
            }
            else
            {
                lineArray.emplace_back( line );
                lineHash[ line ] = lineArray.size() - 1;
                chars += static_cast< wchar_t >( lineArray.size() - 1 );
            }

            lineStart = lineEnd + 1;
        }
        return chars;
    }

    void diff_match_patch::diff_charsToLines( TDiffVector &diffs, const TStringVector &lineArray )
    {
        for ( auto &&diff : diffs )
        {
            std::wstring text;
            for ( auto &&y : diff.text() )
            {
                text += lineArray[ y ];
            }
            diff.fText = text;
        }
    }

    std::size_t diff_match_patch::diff_commonPrefix( const std::wstring &text1, const std::wstring &text2 )
    {
        // Performance analysis: http://neil.fraser.name/news/2007/10/09/
        const auto n = std::min( text1.length(), text2.length() );
        for ( std::size_t i = 0; i < n; i++ )
        {
            if ( text1[ i ] != text2[ i ] )
            {
                return i;
            }
        }
        return n;
    }

    std::size_t diff_match_patch::diff_commonPrefix( const std::string &text1, const std::string &text2 )
    {
        return diff_commonPrefix( NUtils::to_wstring( text1 ), NUtils::to_wstring( text2 ) );
    }

    std::size_t diff_match_patch::diff_commonSuffix( const std::wstring &text1, const std::wstring &text2 )
    {
        // Performance analysis: http://neil.fraser.name/news/2007/10/09/
        const auto text1_length = text1.length();
        const auto text2_length = text2.length();
        const auto n = std::min( text1_length, text2_length );
        for ( std::size_t i = 1; i <= n; i++ )
        {
            if ( text1[ text1_length - i ] != text2[ text2_length - i ] )
            {
                return i - 1;
            }
        }
        return n;
    }

    std::size_t diff_match_patch::diff_commonSuffix( const std::string &text1, const std::string &text2 )
    {
        return diff_commonSuffix( NUtils::to_wstring( text1 ), NUtils::to_wstring( text2 ) );
    }

    std::size_t diff_match_patch::diff_commonOverlap( const std::wstring &text1, const std::wstring &text2 )
    {
        // Cache the text lengths to prevent multiple calls.
        const auto text1_length = text1.length();
        const auto text2_length = text2.length();
        // Eliminate the nullptr case.
        if ( text1_length == 0 || text2_length == 0 )
        {
            return 0;
        }
        // Truncate the longer string.
        std::wstring text1_trunc = text1;
        std::wstring text2_trunc = text2;
        if ( text1_length > text2_length )
        {
            text1_trunc = text1.substr( text1_length - text2_length );
        }
        else if ( text1_length < text2_length )
        {
            text2_trunc = text2.substr( 0, text1_length );
        }
        const auto text_length = std::min( text1_length, text2_length );
        // Quick check for the worst case.
        if ( text1_trunc == text2_trunc )
        {
            return text_length;
        }

        // Start by looking for a single character match
        // and increase length until no match is found.
        // Performance analysis: http://neil.fraser.name/news/2010/11/04/
        std::size_t best = 0;
        std::size_t length = 1;
        while ( true )
        {
            std::wstring pattern = ( length < text1_trunc.length() ) ? text1_trunc.substr( text_length - length ) : std::wstring();
            if ( pattern.empty() )
                return best;

            auto found = text2_trunc.find( pattern );
            if ( found == std::string::npos )
            {
                return best;
            }
            length += found;
            if ( found == 0 || text1_trunc.substr( text_length - length ) == text2_trunc.substr( 0, length ) )
            {
                best = length;
                length++;
            }
        }
    }

    std::size_t diff_match_patch::diff_commonOverlap( const std::string &text1, const std::string &text2 )
    {
        return diff_commonOverlap( NUtils::to_wstring( text1 ), NUtils::to_wstring( text2 ) );
    }

    diff_match_patch::TStringVector diff_match_patch::diff_halfMatch( const std::wstring &text1, const std::wstring &text2 )
    {
        if ( Diff_Timeout <= 0 )
        {
            // Don't risk returning a non-optimal diff if we have unlimited time.
            return {};
        }
        const std::wstring longtext = text1.length() > text2.length() ? text1 : text2;
        const std::wstring shorttext = text1.length() > text2.length() ? text2 : text1;
        if ( ( longtext.length() < 4 ) || ( ( shorttext.length() * 2 ) < longtext.length() ) )
        {
            return {};   // Pointless.
        }

        // First check if the second quarter is the seed for a half-match.
        const TStringVector hm1 = diff_halfMatchI( longtext, shorttext, ( longtext.length() + 3 ) / 4 );
        // Check again based on the third quarter.
        const TStringVector hm2 = diff_halfMatchI( longtext, shorttext, ( longtext.length() + 1 ) / 2 );
        TStringVector hm;
        if ( hm1.empty() && hm2.empty() )
        {
            return {};
        }
        else if ( hm2.empty() )
        {
            hm = hm1;
        }
        else if ( hm1.empty() )
        {
            hm = hm2;
        }
        else
        {
            // Both matched.  Select the longest.
            hm = hm1[ 4 ].length() > hm2[ 4 ].length() ? hm1 : hm2;
        }

        // A half-match was found, sort out the return data.
        if ( text1.length() > text2.length() )
        {
            return hm;
        }
        else
        {
            TStringVector listRet( { hm[ 2 ], hm[ 3 ], hm[ 0 ], hm[ 1 ], hm[ 4 ] } );
            return listRet;
        }
    }

    diff_match_patch::TStringVector diff_match_patch::diff_halfMatch( const std::string &text1, const std::string &text2 )
    {
        return diff_halfMatch( NUtils::to_wstring( text1 ), NUtils::to_wstring( text2 ) );
    }

    diff_match_patch::TStringVector diff_match_patch::diff_halfMatchI( const std::wstring &longtext, const std::wstring &shorttext, std::size_t i )
    {
        // Start with a 1/4 length substring at position i as a seed.
        const std::wstring seed = safeMid( longtext, i, longtext.length() / 4 );
        std::size_t j = std::string::npos;
        std::wstring best_common;
        std::wstring best_longtext_a, best_longtext_b;
        std::wstring best_shorttext_a, best_shorttext_b;
        while ( ( j = shorttext.find( seed, j + 1 ) ) != std::string::npos )
        {
            const auto prefixLength = diff_commonPrefix( safeMid( longtext, i ), safeMid( shorttext, j ) );
            const auto suffixLength = diff_commonSuffix( longtext.substr( 0, i ), shorttext.substr( 0, j ) );
            if ( best_common.length() < suffixLength + prefixLength )
            {
                best_common = safeMid( shorttext, j - suffixLength, suffixLength ) + safeMid( shorttext, j, prefixLength );
                best_longtext_a = longtext.substr( 0, i - suffixLength );
                best_longtext_b = safeMid( longtext, i + prefixLength );
                best_shorttext_a = shorttext.substr( 0, j - suffixLength );
                best_shorttext_b = safeMid( shorttext, j + prefixLength );
            }
        }
        if ( best_common.length() * 2 >= longtext.length() )
        {
            TStringVector listRet( { best_longtext_a, best_longtext_b, best_shorttext_a, best_shorttext_b, best_common } );
            return listRet;
        }
        else
        {
            return {};
        }
    }

    diff_match_patch::TStringVector diff_match_patch::diff_halfMatchI( const std::string &longtext, const std::string &shorttext, std::size_t i )
    {
        return diff_halfMatchI( NUtils::to_wstring( longtext ), NUtils::to_wstring( shorttext ), i );
    }

    void diff_match_patch::diff_cleanupSemantic( TDiffVector &diffs )
    {
        if ( diffs.empty() )
            return;

        bool changes = false;
        // Stack of indices where equalities are found.
        std::stack< std::size_t > equalities;   // stack of equalities
        // Always equal to equalities[equalitiesLength-1][1]
        std::wstring lastEquality;
        std::size_t pointer = 0;   // Index of current position.
        // Number of characters that changed prior to the equality.
        std::size_t length_insertions1 = 0;
        std::size_t length_deletions1 = 0;
        // Number of characters that changed after the equality.
        std::size_t length_insertions2 = 0;
        std::size_t length_deletions2 = 0;
        while ( pointer < diffs.size() )
        {
            if ( diffs[ pointer ].isEqual() )
            {   // Equality found.
                equalities.push( pointer );
                length_insertions1 = length_insertions2;
                length_deletions1 = length_deletions2;
                length_insertions2 = 0;
                length_deletions2 = 0;
                lastEquality = diffs[ pointer ].text();
            }
            else
            {   // an insertion or deletion
                if ( diffs[ pointer ].isInsert() )
                {
                    length_insertions2 += diffs[ pointer ].text().length();
                }
                else
                {
                    length_deletions2 += diffs[ pointer ].text().length();
                }
                // Eliminate an equality that is smaller or equal to the edits on both
                // sides of it.
                if ( !lastEquality.empty() && ( lastEquality.length() <= std::max( length_insertions1, length_deletions1 ) ) && ( lastEquality.length() <= std::max( length_insertions2, length_deletions2 ) ) )
                {
                    // Duplicate record.
                    diffs.insert( diffs.begin() + equalities.top(), Diff( EOperation::eDELETE, lastEquality ) );
                    // Change second copy to insert.
                    diffs[ equalities.top() + 1 ].fOperation = EOperation::eINSERT;
                    // Throw away the equality we just deleted.
                    equalities.pop();
                    if ( !equalities.empty() )
                    {
                        equalities.pop();
                    }
                    pointer = !equalities.empty() ? equalities.top() : -1;
                    length_insertions1 = 0;   // Reset the counters.
                    length_deletions1 = 0;
                    length_insertions2 = 0;
                    length_deletions2 = 0;
                    lastEquality.clear();
                    changes = true;
                }
            }
            pointer++;
        }

        // Normalize the diff.
        if ( changes )
        {
            diff_cleanupMerge( diffs );
        }
        diff_cleanupSemanticLossless( diffs );

        // Find any overlaps between deletions and insertions.
        // e.g: <del>abcxxx</del><ins>xxxdef</ins>
        //   -> <del>abc</del>xxx<ins>def</ins>
        // e.g: <del>xxxabc</del><ins>defxxx</ins>
        //   -> <ins>def</ins>xxx<del>abc</del>
        // Only extract an overlap if it is as big as the edit ahead or behind it.
        pointer = 1;
        while ( pointer < diffs.size() )
        {
            if ( diffs[ pointer - 1 ].isDelete() && diffs[ pointer ].isInsert() )
            {
                auto deletion = diffs[ pointer - 1 ].text();
                auto insertion = diffs[ pointer ].text();
                std::size_t overlap_length1 = diff_commonOverlap( deletion, insertion );
                std::size_t overlap_length2 = diff_commonOverlap( insertion, deletion );
                if ( overlap_length1 >= overlap_length2 )
                {
                    if ( overlap_length1 >= deletion.length() / 2.0 || overlap_length1 >= insertion.length() / 2.0 )
                    {
                        // Overlap found.
                        // Insert an equality and trim the surrounding edits.
                        diffs.emplace( diffs.begin() + pointer, EOperation::eEQUAL, insertion.substr( 0, overlap_length1 ) );
                        diffs[ pointer - 1 ].fText = deletion.substr( 0, deletion.length() - overlap_length1 );
                        diffs[ pointer + 1 ].fText = insertion.substr( overlap_length1 );
                        pointer++;
                    }
                }
                else
                {
                    if ( overlap_length2 >= deletion.length() / 2.0 || overlap_length2 >= insertion.length() / 2.0 )
                    {
                        // Reverse overlap found.
                        // Insert an equality and swap and trim the surrounding edits.
                        diffs.emplace( diffs.begin() + pointer, EOperation::eEQUAL, deletion.substr( 0, overlap_length2 ) );
                        diffs[ pointer - 1 ].fOperation = EOperation::eINSERT;
                        diffs[ pointer - 1 ].fText = insertion.substr( 0, insertion.length() - overlap_length2 );
                        diffs[ pointer + 1 ].fOperation = EOperation::eDELETE;
                        diffs[ pointer + 1 ].fText = deletion.substr( overlap_length2 );
                        pointer++;
                    }
                }
                pointer++;
            }
            pointer++;
        }
    }

    void diff_match_patch::diff_cleanupSemanticLossless( TDiffVector &diffs )
    {
        int pointer = 1;
        // Intentionally ignore the first and last element (don't need checking).
        while ( ( pointer != -1 ) && !diffs.empty() && ( pointer < ( diffs.size() - 1 ) ) )
        {
            if ( diffs[ pointer - 1 ].isEqual() && diffs[ pointer + 1 ].isEqual() )
            {
                // This is a single edit surrounded by equalities.
                auto equality1 = diffs[ pointer - 1 ].text();
                auto edit = diffs[ pointer ].text();
                auto equality2 = diffs[ pointer + 1 ].text();

                // First, shift the edit as far left as possible.
                auto commonOffset = diff_commonSuffix( equality1, edit );
                if ( commonOffset > 0 )
                {
                    auto commonString = safeMid( edit, edit.length() - commonOffset );
                    equality1 = equality1.substr( 0, equality1.length() - commonOffset );
                    edit = commonString + edit.substr( 0, edit.length() - commonOffset );
                    equality2 = commonString + equality2;
                }

                // Second, step character by character right,
                // looking for the best fit.
                auto bestEquality1 = equality1;
                auto bestEdit = edit;
                auto bestEquality2 = equality2;
                auto bestScore = diff_cleanupSemanticScore( equality1, edit ) + diff_cleanupSemanticScore( edit, equality2 );
                while ( !edit.empty() && !equality2.empty() && edit[ 0 ] == equality2[ 0 ] )
                {
                    equality1 += edit[ 0 ];
                    edit = edit.substr( 1 ) + equality2[ 0 ];
                    equality2 = equality2.substr( 1 );
                    auto score = diff_cleanupSemanticScore( equality1, edit ) + diff_cleanupSemanticScore( edit, equality2 );
                    // The >= encourages trailing rather than leading whitespace on
                    // edits.
                    if ( score >= bestScore )
                    {
                        bestScore = score;
                        bestEquality1 = equality1;
                        bestEdit = edit;
                        bestEquality2 = equality2;
                    }
                }

                if ( diffs[ pointer - 1 ].text() != bestEquality1 )
                {
                    // We have an improvement, save it back to the diff.
                    if ( !bestEquality1.empty() )
                    {
                        diffs[ pointer - 1 ].fText = bestEquality1;
                    }
                    else
                    {
                        diffs.erase( diffs.begin() + pointer - 1 );
                        pointer--;
                    }
                    diffs[ pointer ].fText = bestEdit;
                    if ( !bestEquality2.empty() )
                    {
                        diffs[ pointer + 1 ].fText = bestEquality2;
                    }
                    else
                    {
                        diffs.erase( diffs.begin() + pointer + 1 );
                        pointer--;
                    }
                }
            }
            pointer++;
        }
    }

    int64_t diff_match_patch::diff_cleanupSemanticScore( const std::wstring &one, const std::wstring &two )
    {
        if ( one.empty() || two.empty() )
        {
            // Edges are the best.
            return 6;
        }

        // Each port of this function behaves slightly differently due to
        // subtle differences in each language's definition of things like
        // 'whitespace'.  Since this function's purpose is largely cosmetic,
        // the choice has been made to use each language's native features
        // rather than force total conformity.
        auto char1 = one[ one.length() - 1 ];
        auto char2 = two[ 0 ];
        bool nonAlphaNumeric1 = !std::iswalnum( char1 );
        bool nonAlphaNumeric2 = !std::iswalnum( char2 );
        bool whitespace1 = nonAlphaNumeric1 && std::iswspace( char1 );
        bool whitespace2 = nonAlphaNumeric2 && std::iswspace( char2 );
        bool lineBreak1 = whitespace1 && std::iswcntrl( char1 );
        bool lineBreak2 = whitespace2 && std::iswcntrl( char2 );
        bool blankLine1 = lineBreak1 && std::regex_search( one, BLANKLINEEND );
        bool blankLine2 = lineBreak2 && std::regex_search( two, BLANKLINESTART );

        if ( blankLine1 || blankLine2 )
        {
            // Five points for blank lines.
            return 5;
        }
        else if ( lineBreak1 || lineBreak2 )
        {
            // Four points for line breaks.
            return 4;
        }
        else if ( nonAlphaNumeric1 && !whitespace1 && whitespace2 )
        {
            // Three points for end of sentences.
            return 3;
        }
        else if ( whitespace1 || whitespace2 )
        {
            // Two points for whitespace.
            return 2;
        }
        else if ( nonAlphaNumeric1 || nonAlphaNumeric2 )
        {
            // One point for non-alphanumeric.
            return 1;
        }
        return 0;
    }

    int64_t diff_match_patch::diff_cleanupSemanticScore( const std::string &one, const std::string &two )
    {
        return diff_cleanupSemanticScore( NUtils::to_wstring( one ), NUtils::to_wstring( two ) );
    }

    // Define some regex patterns for matching boundaries.
    std::wregex diff_match_patch::BLANKLINEEND = std::wregex( LR"(\n\r?\n$)" );
    std::wregex diff_match_patch::BLANKLINESTART = std::wregex( LR"(^\r?\n\r?\n)" );

    void diff_match_patch::diff_cleanupEfficiency( TDiffVector &diffs )
    {
        bool changes = false;
        // Stack of indices where equalities are found.
        std::stack< std::size_t > equalities;
        // Always equal to equalities[equalitiesLength-1][1]
        std::wstring lastEquality;
        std::size_t pointer = 0;   // Index of current position.
        // Is there an insertion operation before the last equality.
        bool pre_ins = false;
        // Is there a deletion operation before the last equality.
        bool pre_del = false;
        // Is there an insertion operation after the last equality.
        bool post_ins = false;
        // Is there a deletion operation after the last equality.
        bool post_del = false;
        while ( pointer < diffs.size() )
        {
            if ( diffs[ pointer ].isEqual() )
            {   // Equality found.
                if ( diffs[ pointer ].text().length() < Diff_EditCost && ( post_ins || post_del ) )
                {
                    // Candidate found.
                    equalities.push( pointer );
                    pre_ins = post_ins;
                    pre_del = post_del;
                    lastEquality = diffs[ pointer ].text();
                }
                else
                {
                    // Not a candidate, and can never become one.
                    equalities = {};
                    lastEquality.clear();
                }
                post_ins = post_del = false;
            }
            else
            {   // An insertion or deletion.
                if ( diffs[ pointer ].isDelete() )
                {
                    post_del = true;
                }
                else
                {
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
                if ( ( lastEquality.length() != 0 ) && ( ( pre_ins && pre_del && post_ins && post_del ) || ( ( lastEquality.length() < Diff_EditCost / 2 ) && ( ( pre_ins ? 1 : 0 ) + ( pre_del ? 1 : 0 ) + ( post_ins ? 1 : 0 ) + ( post_del ? 1 : 0 ) ) == 3 ) ) )
                {
                    // Duplicate record.
                    diffs.emplace( diffs.begin() + equalities.top(), EOperation::eDELETE, lastEquality );
                    // Change second copy to insert.
                    diffs[ equalities.top() + 1 ].fOperation = EOperation::eINSERT;
                    equalities.pop();   // Throw away the equality we just deleted.
                    lastEquality.clear();
                    if ( pre_ins && pre_del )
                    {
                        // No changes made which could affect previous entry, keep going.
                        post_ins = post_del = true;
                        equalities = {};
                    }
                    else
                    {
                        if ( !equalities.empty() )
                        {
                            equalities.pop();
                        }

                        pointer = !equalities.empty() ? equalities.top() : -1;
                        post_ins = post_del = false;
                    }
                    changes = true;
                }
            }
            pointer++;
        }

        if ( changes )
        {
            diff_cleanupMerge( diffs );
        }
    }

    void diff_match_patch::diff_cleanupMerge( TDiffVector &diffs )
    {
        diffs.emplace_back( EOperation::eEQUAL, L"" );
        int pointer = 0;
        int count_delete = 0;
        int count_insert = 0;
        std::wstring text_delete;
        std::wstring text_insert;

        while ( pointer < diffs.size() )
        {
            if ( diffs[ pointer ].isInsert() )
            {
                count_insert++;
                text_insert += diffs[ pointer ].text();
                pointer++;
            }
            else if ( diffs[ pointer ].isDelete() )
            {
                count_delete++;
                text_delete += diffs[ pointer ].text();
                pointer++;
            }
            else if ( diffs[ pointer ].isEqual() )
            {
                // Upon reaching an equality, check for prior redundancies.
                if ( count_delete + count_insert > 1 )
                {
                    if ( count_delete != 0 && count_insert != 0 )
                    {
                        // Factor out any common prefixies.
                        auto commonlength = diff_commonPrefix( text_insert, text_delete );
                        if ( commonlength != 0 )
                        {
                            if ( ( pointer > ( count_delete + count_insert ) ) && diffs[ pointer - ( count_delete + count_insert ) - 1 ].isEqual() )
                            {
                                diffs[ pointer - count_delete - count_insert - 1 ].fText += text_insert.substr( 0, commonlength );
                            }
                            else
                            {
                                diffs.emplace( diffs.begin(), EOperation::eEQUAL, text_insert.substr( 0, commonlength ) );
                                pointer++;
                            }
                            text_insert = text_insert.substr( commonlength );
                            text_delete = text_delete.substr( commonlength );
                        }
                        // Factor out any common suffixies.
                        commonlength = diff_commonSuffix( text_insert, text_delete );
                        if ( commonlength != 0 )
                        {
                            diffs[ pointer ].fText = safeMid( text_insert, text_insert.length() - commonlength ) + diffs[ pointer ].text();
                            text_insert = text_insert.substr( 0, text_insert.length() - commonlength );
                            text_delete = text_delete.substr( 0, text_delete.length() - commonlength );
                        }
                    }
                    // Delete the offending records and add the merged ones.
                    pointer -= count_delete + count_insert;
                    NUtils::Splice( diffs, pointer, count_delete + count_insert );
                    if ( !text_delete.empty() )
                    {
                        NUtils::Splice( diffs, pointer, 0, { Diff( EOperation::eDELETE, text_delete ) } );
                        pointer++;
                    }
                    if ( !text_insert.empty() )
                    {
                        NUtils::Splice( diffs, pointer, 0, { Diff( EOperation::eINSERT, text_insert ) } );
                        pointer++;
                    }
                    pointer++;
                }
                else if ( pointer != 0 && diffs[ pointer - 1 ].isEqual() )
                {
                    // Merge this equality with the previous one.
                    diffs[ pointer - 1 ].fText += diffs[ pointer ].text();
                    diffs.erase( diffs.begin() + pointer );
                }
                else
                {
                    pointer++;
                }
                count_insert = 0;
                count_delete = 0;
                text_delete.clear();
                text_insert.clear();
            }
        }
        if ( diffs.back().text().empty() )
        {
            diffs.pop_back();   // Remove the dummy entry at the end.
        }

        // Second pass: look for single edits surrounded on both sides by
        // equalities which can be shifted sideways to eliminate an equality.
        // e.g: A<ins>BA</ins>C -> <ins>AB</ins>AC
        bool changes = false;
        pointer = 1;
        // Intentionally ignore the first and last element (don't need checking).
        while ( !diffs.empty() && pointer < ( diffs.size() - 1 ) )
        {
            if ( diffs[ pointer - 1 ].isEqual() && diffs[ pointer + 1 ].isEqual() )
            {
                // This is a single edit surrounded by equalities.
                if ( NUtils::endsWith( diffs[ pointer ].text(), diffs[ pointer - 1 ].text() ) )
                {
                    // Shift the edit over the previous equality.
                    diffs[ pointer ].fText = diffs[ pointer - 1 ].text() + diffs[ pointer ].text().substr( 0, diffs[ pointer ].text().length() - diffs[ pointer - 1 ].text().length() );
                    diffs[ pointer + 1 ].fText = diffs[ pointer - 1 ].text() + diffs[ pointer + 1 ].text();
                    NUtils::Splice( diffs, pointer - 1, 1 );
                    changes = true;
                }
                else if ( diffs[ pointer ].text().find( diffs[ pointer + 1 ].text() ) == 0 )
                {
                    // Shift the edit over the next equality.
                    diffs[ pointer - 1 ].fText += diffs[ pointer + 1 ].text();
                    diffs[ pointer ].fText = diffs[ pointer ].text().substr( diffs[ pointer + 1 ].text().length() ) + diffs[ pointer + 1 ].text();
                    NUtils::Splice( diffs, pointer + 1, 1 );
                    changes = true;
                }
            }
            pointer++;
        }
        // If shifts were made, the diff needs reordering and another shift sweep.
        if ( changes )
        {
            diff_cleanupMerge( diffs );
        }
    }
    std::size_t diff_match_patch::diff_xIndex( const TDiffVector &diffs, std::size_t loc )
    {
        std::size_t chars1 = 0;
        std::size_t chars2 = 0;
        std::size_t last_chars1 = 0;
        std::size_t last_chars2 = 0;
        Diff lastDiff;
        for ( auto &&aDiff : diffs )
        {
            if ( !aDiff.isInsert() )
            {
                // Equality or deletion.
                chars1 += aDiff.text().length();
            }
            if ( !aDiff.isDelete() )
            {
                // Equality or insertion.
                chars2 += aDiff.text().length();
            }
            if ( chars1 > loc )
            {
                // Overshot the location.
                lastDiff = aDiff;
                break;
            }
            last_chars1 = chars1;
            last_chars2 = chars2;
        }
        if ( lastDiff.isDelete() )
        {
            // The location was deleted.
            return last_chars2;
        }
        // Add the remaining character length.
        return last_chars2 + ( loc - last_chars1 );
    }

    std::wstring diff_match_patch::diff_prettyHtml( const TDiffVector &diffs )
    {
        std::wstring html;
        for ( auto &&aDiff : diffs )
        {
            html += aDiff.toHtml();
        }
        return html;
    }

    std::wstring diff_match_patch::diff_prettyConsole( const TDiffVector &diffs )
    {
        std::wstring retVal;
        for ( auto &&aDiff : diffs )
        {
            retVal += aDiff.toConsole();
        }
        return retVal;
    }

    std::wstring diff_match_patch::diff_text1( const TDiffVector &diffs )
    {
        std::wstring text;
        for ( auto &&aDiff : diffs )
        {
            if ( !aDiff.isInsert() )
            {
                text += aDiff.text();
            }
        }
        return text;
    }

    std::wstring diff_match_patch::diff_text2( const TDiffVector &diffs )
    {
        std::wstring text;
        for ( auto &&aDiff : diffs )
        {
            if ( !aDiff.isDelete() )
            {
                text += aDiff.text();
            }
        }
        return text;
    }

    std::size_t diff_match_patch::diff_levenshtein( const TDiffVector &diffs )
    {
        std::size_t levenshtein = 0;
        std::size_t insertions = 0;
        std::size_t deletions = 0;
        for ( auto &&aDiff : diffs )
        {
            if ( aDiff.isInsert() )
                insertions += aDiff.text().length();
            else if ( aDiff.isDelete() )
                deletions += aDiff.text().length();
            else if ( aDiff.isEqual() )
            {
                // A deletion and an insertion is one substitution.
                levenshtein += std::max( insertions, deletions );
                insertions = 0;
                deletions = 0;
            }
        }
        levenshtein += std::max( insertions, deletions );
        return levenshtein;
    }

    std::wstring diff_match_patch::diff_toDelta( const TDiffVector &diffs )
    {
        std::wstring text;
        for ( auto &&aDiff : diffs )
        {
            text += aDiff.toDelta();
        }
        if ( !text.empty() )
        {
            // Strip off trailing tab character.
            text = text.substr( 0, text.length() - 1 );
        }
        return text;
    }

    TDiffVector diff_match_patch::diff_fromDelta( const std::wstring &text1, const std::wstring &delta )
    {
        TDiffVector diffs;
        std::size_t pointer = 0;   // Cursor in text1
        auto tokens = NUtils::splitString( delta, L"\t", false );
        for ( auto &&token : tokens )
        {
            if ( token.empty() )
            {
                // Blank tokens are ok (from a trailing \t).
                continue;
            }
            // Each token begins with a one character parameter which specifies the
            // operation of this token (delete, insert, equality).
            std::wstring param = safeMid( token, 1 );
            switch ( token[ 0 ] )
            {
                case '+':
                    NUtils::replace( param, L"+", L"%2b" );
                    param = NUtils::fromPercentEncoding( param );
                    diffs.emplace_back( EOperation::eINSERT, param );
                    break;
                case '-':
                    // Fall through.
                case '=':
                    {
                        auto n = NUtils::toInt( param );
                        if ( n < 0 )
                        {
                            throw std::wstring( L"Negative number in diff_fromDelta: " + param );
                        }
                        std::wstring text;
                        if ( ( pointer + n ) > text1.length() )
                        {
                            throw std::wstring( L"Delta length (" + std::to_wstring( pointer + n ) + L") larger than source text length (" + std::to_wstring( text1.length() ) + L")." );
                        }

                        text = safeMid( text1, pointer, n );
                        pointer += n;
                        if ( token[ 0 ] == L'=' )
                        {
                            diffs.emplace_back( EOperation::eEQUAL, text );
                        }
                        else
                        {
                            diffs.emplace_back( EOperation::eDELETE, text );
                        }
                        break;
                    }
                default:
                    throw std::wstring( L"Invalid diff operation in diff_fromDelta: " + token[ 0 ] );
            }
        }
        if ( pointer != text1.length() )
        {
            throw std::wstring( L"Delta length (" ) + std::to_wstring( pointer ) + L") smaller than source text length (" + std::to_wstring( text1.length() ) + L")";
        }
        return diffs;
    }

    TDiffVector diff_match_patch::diff_fromDelta( const std::string &text1, const std::string &delta )
    {
        return diff_fromDelta( NUtils::to_wstring( text1 ), NUtils::to_wstring( delta ) );
    }

    //  MATCH FUNCTIONS

    std::size_t diff_match_patch::match_main( const std::wstring &text, const std::wstring &pattern, std::size_t loc )
    {
        // Check for null inputs not needed since null can't be passed via
        // std::wstring

        loc = std::max( kZERO, std::min( loc, text.length() ) );
        if ( text == pattern )
        {
            // Shortcut (potentially not guaranteed by the algorithm)
            return 0;
        }
        else if ( text.empty() )
        {
            // Nothing to match.
            return -1;
        }
        else if ( loc + pattern.length() <= text.length() && safeMid( text, loc, pattern.length() ) == pattern )
        {
            // Perfect match at the perfect spot!  (Includes case of nullptr pattern)
            return loc;
        }
        else
        {
            // Do a fuzzy compare.
            return match_bitap( text, pattern, loc );
        }
    }

    std::size_t diff_match_patch::match_main( const std::string &text, const std::string &pattern, std::size_t loc )
    {
        return match_main( NUtils::to_wstring( text ), NUtils::to_wstring( pattern ), loc );
    }

    std::size_t diff_match_patch::match_bitap( const std::wstring &text, const std::wstring &pattern, std::size_t loc )
    {
        if ( !( Match_MaxBits == 0 || pattern.length() <= Match_MaxBits ) )
        {
            throw "Pattern too long for this application.";
        }

        // Initialise the alphabet.
        auto &&s = match_alphabet( pattern );

        // Highest score beyond which we give up.
        double score_threshold = Match_Threshold;
        // Is there a nearby exact match? (speedup)
        auto best_loc = text.find( pattern, loc );
        if ( best_loc != std::string::npos )
        {
            score_threshold = std::min( match_bitapScore( 0, best_loc, loc, pattern ), score_threshold );
            // What about in the other direction? (speedup)
            auto start = std::min( loc + pattern.length(), text.length() );
            best_loc = text.rfind( pattern, start );
            if ( best_loc != std::string::npos )
            {
                score_threshold = std::min( match_bitapScore( 0, best_loc, loc, pattern ), score_threshold );
            }
        }

        // Initialise the bit arrays.
        auto matchmask = 1 << ( pattern.length() - 1 );
        best_loc = std::string::npos;

        std::size_t bin_min, bin_mid;
        auto bin_max = pattern.length() + text.length();
        std::vector< int64_t > rd;
        std::vector< int64_t > last_rd;
        for ( int d = 0; d < pattern.length(); d++ )
        {
            // Scan for the best match; each iteration allows for one more error.
            // Run a binary search to determine how far from 'loc' we can stray at
            // this error level.
            bin_min = 0;
            bin_mid = bin_max;
            while ( bin_min < bin_mid )
            {
                if ( match_bitapScore( d, loc + bin_mid, loc, pattern ) <= score_threshold )
                {
                    bin_min = bin_mid;
                }
                else
                {
                    bin_max = bin_mid;
                }
                bin_mid = ( bin_max - bin_min ) / 2 + bin_min;
            }
            // Use the result from this iteration as the maximum for the next.
            bin_max = bin_mid;
            auto start = std::max( kONE, ( loc > bin_mid ) ? ( loc - bin_mid + 1 ) : kZERO );
            auto finish = std::min( loc + bin_mid, text.length() ) + pattern.length();

            rd = std::vector< int64_t >( finish + 2, 0 );
            rd[ finish + 1 ] = ( 1 << d ) - 1;
            for ( auto j = finish; ( j != -1 ) && ( j >= start ); j-- )
            {
                int64_t charMatch;
                if ( text.length() <= j - 1 )
                {
                    // Out of range.
                    charMatch = 0;
                }
                else
                {
                    auto pos = s.find( text[ j - 1 ] );
                    if ( pos == s.end() )
                        charMatch = 0;
                    else
                        charMatch = ( *pos ).second;
                }
                if ( d == 0 )
                {
                    // First pass: exact match.
                    rd[ j ] = ( ( rd[ j + 1 ] << 1 ) | 1 ) & charMatch;
                }
                else
                {
                    // Subsequent passes: fuzzy match.
                    rd[ j ] = ( ( rd[ j + 1 ] << 1 ) | 1 ) & charMatch | ( ( ( last_rd[ j + 1 ] | last_rd[ j ] ) << 1 ) | 1 ) | last_rd[ j + 1 ];
                }
                if ( ( rd[ j ] & matchmask ) != 0 )
                {
                    double score = match_bitapScore( d, j - 1, loc, pattern );
                    // This match will almost certainly be better than any existing
                    // match.  But check anyway.
                    if ( score <= score_threshold )
                    {
                        // Told you so.
                        score_threshold = score;
                        best_loc = j - 1;
                        if ( best_loc > loc )
                        {
                            // When passing loc, don't exceed our current distance from loc.
                            start = std::max( kONE, ( 2 * loc > best_loc ) ? 2 * loc - best_loc : 1 );
                        }
                        else
                        {
                            // Already passed loc, downhill from here on in.
                            break;
                        }
                    }
                }
            }
            if ( match_bitapScore( d + 1, loc, loc, pattern ) > score_threshold )
            {
                // No hope for a (better) match at greater error levels.
                break;
            }
            last_rd = std::move( rd );
        }
        return best_loc;
    }

    std::size_t diff_match_patch::match_bitap( const std::string &text, const std::string &pattern, std::size_t loc )
    {
        return match_bitap( NUtils::to_wstring( text ), NUtils::to_wstring( pattern ), loc );
    }

    double diff_match_patch::match_bitapScore( int64_t e, int64_t x, int64_t loc, const std::wstring &pattern )
    {
        const float accuracy = static_cast< float >( e ) / pattern.length();
        const auto proximity = std::abs( loc - x );
        if ( Match_Distance == 0 )
        {
            // Dodge divide by zero error.
            return proximity == 0 ? accuracy : 1.0;
        }
        return accuracy + ( proximity / static_cast< float >( Match_Distance ) );
    }

    diff_match_patch::TCharPosMap diff_match_patch::match_alphabet( const std::wstring &pattern )
    {
        TCharPosMap s;
        std::size_t i;
        for ( i = 0; i < pattern.length(); i++ )
        {
            auto c = pattern[ i ];
            s[ c ] = 0;
        }
        for ( i = 0; i < pattern.length(); i++ )
        {
            auto c = pattern[ i ];
            auto pos = s.find( c );
            std::size_t prev = 0;
            if ( pos != s.end() )
                prev = ( *pos ).second;
            s[ c ] = prev | ( 1ULL << ( pattern.length() - i - 1 ) );
        }
        return s;
    }

    diff_match_patch::TCharPosMap diff_match_patch::match_alphabet( const std::string &pattern )
    {
        return match_alphabet( NUtils::to_wstring( pattern ) );
    }

    //  PATCH FUNCTIONS

    void diff_match_patch::patch_addContext( Patch &patch, const std::wstring &text )
    {
        if ( text.empty() )
        {
            return;
        }
        std::wstring pattern = safeMid( text, patch.start2, patch.length1 );
        std::size_t padding = 0;

        // Look for the first and last matches of pattern in text.  If two different
        // matches are found, increase the pattern length.
        while ( ( text.find( pattern ) != text.rfind( pattern ) ) && ( pattern.length() < ( Match_MaxBits - Patch_Margin - Patch_Margin ) ) )
        {
            padding += Patch_Margin;
            pattern = safeMid( text, std::max( kZERO, ( ( patch.start2 > padding ) ? patch.start2 - padding : 0UL ) ), std::min( text.length(), patch.start2 + patch.length1 + padding ) - std::max( kZERO, ( patch.start2 > padding ) ? patch.start2 - padding : 0 ) );
        }
        // Add one chunk for good luck.
        padding += Patch_Margin;

        // Add the prefix.
        std::wstring prefix = safeMid( text, std::max( kZERO, ( ( patch.start2 > padding ) ? patch.start2 - padding : 0UL ) ), patch.start2 - std::max( kZERO, ( ( patch.start2 > padding ) ? patch.start2 - padding : 0UL ) ) );
        if ( !prefix.empty() )
        {
            patch.diffs.emplace( patch.diffs.begin(), EOperation::eEQUAL, prefix );
        }
        // Add the suffix.
        std::wstring suffix = safeMid( text, patch.start2 + patch.length1, std::min( text.length(), patch.start2 + patch.length1 + padding ) - ( patch.start2 + patch.length1 ) );
        if ( !suffix.empty() )
        {
            patch.diffs.emplace_back( EOperation::eEQUAL, suffix );
        }

        // Roll back the start points.
        patch.start1 -= prefix.length();
        patch.start2 -= prefix.length();
        // Extend the lengths.
        patch.length1 += prefix.length() + suffix.length();
        patch.length2 += prefix.length() + suffix.length();
    }

    void diff_match_patch::patch_addContext( Patch &patch, const std::string &text )
    {
        return patch_addContext( patch, NUtils::to_wstring( text ) );
    }

    TPatchVector diff_match_patch::patch_make( const std::wstring &text1, const std::wstring &text2 )
    {
        // Check for null inputs not needed since null can't be passed via
        // std::wstring

        // No diffs provided, compute our own.
        TDiffVector diffs = diff_main( text1, text2, true );
        if ( diffs.size() > 2 )
        {
            diff_cleanupSemantic( diffs );
            diff_cleanupEfficiency( diffs );
        }

        return patch_make( text1, diffs );
    }

    TPatchVector diff_match_patch::patch_make( const TDiffVector &diffs )
    {
        // No origin string provided, compute our own.
        const std::wstring text1 = diff_text1( diffs );
        return patch_make( text1, diffs );
    }

    TPatchVector diff_match_patch::patch_make( const std::wstring &text1, const std::wstring & /*text2*/, const TDiffVector &diffs )
    {
        // text2 is entirely unused.
        return patch_make( text1, diffs );
    }

    TPatchVector diff_match_patch::patch_make( const std::wstring &text1, const TDiffVector &diffs )
    {
        // Check for null inputs not needed since null can't be passed via
        // std::wstring

        TPatchVector patches;
        if ( diffs.empty() )
        {
            return patches;   // Get rid of the nullptr case.
        }
        Patch patch;
        std::size_t char_count1 = 0;   // Number of characters into the text1 string.
        std::size_t char_count2 = 0;   // Number of characters into the text2 string.
        // Start with text1 (prepatch_text) and apply the diffs until we arrive at
        // text2 (postpatch_text).  We recreate the patches one by one to determine
        // context info.
        std::wstring prepatch_text = text1;
        std::wstring postpatch_text = text1;
        for ( auto &&aDiff : diffs )
        {
            if ( patch.diffs.empty() && !aDiff.isEqual() )
            {
                // A new patch starts here.
                patch.start1 = char_count1;
                patch.start2 = char_count2;
            }

            if ( aDiff.isInsert() )
            {
                patch.diffs.push_back( aDiff );
                patch.length2 += aDiff.text().length();
                postpatch_text = postpatch_text.substr( 0, char_count2 ) + aDiff.text() + safeMid( postpatch_text, char_count2 );
            }
            else if ( aDiff.isDelete() )
            {
                patch.length1 += aDiff.text().length();
                patch.diffs.push_back( aDiff );
                postpatch_text = postpatch_text.substr( 0, char_count2 ) + safeMid( postpatch_text, char_count2 + aDiff.text().length() );
            }
            else if ( aDiff.isEqual() )
            {
                if ( aDiff.text().length() <= 2 * Patch_Margin && !patch.diffs.empty() && !( aDiff == diffs.back() ) )
                {
                    // Small equality inside a patch.
                    patch.diffs.push_back( aDiff );
                    patch.length1 += aDiff.text().length();
                    patch.length2 += aDiff.text().length();
                }

                if ( aDiff.text().length() >= 2 * Patch_Margin )
                {
                    // Time for a new patch.
                    if ( !patch.diffs.empty() )
                    {
                        patch_addContext( patch, prepatch_text );
                        patches.emplace_back( patch );
                        patch = Patch();
                        // Unlike Unidiff, our patch lists have a rolling context.
                        // http://code.google.com/p/google-diff-match-patch/wiki/Unidiff
                        // Update prepatch text & pos to reflect the application of the
                        // just completed patch.
                        prepatch_text = postpatch_text;
                        char_count1 = char_count2;
                    }
                }
            }

            // Update the current character count.
            if ( !aDiff.isInsert() )
            {
                char_count1 += aDiff.text().length();
            }
            if ( !aDiff.isDelete() )
            {
                char_count2 += aDiff.text().length();
            }
        }
        // Pick up the leftover patch if not empty.
        if ( !patch.diffs.empty() )
        {
            patch_addContext( patch, prepatch_text );
            patches.emplace_back( patch );
        }

        return patches;
    }

    TPatchVector diff_match_patch::patch_make( const std::string &text1, const TDiffVector &diffs )
    {
        return patch_make( NUtils::to_wstring( text1 ), diffs );
    }

    TPatchVector diff_match_patch::patch_make( const std::string &text1, const std::string &text2, const TDiffVector &diffs )
    {
        return patch_make( NUtils::to_wstring( text1 ), NUtils::to_wstring( text2 ), diffs );
    }

    TPatchVector diff_match_patch::patch_make( const std::string &text1, const std::string &text2 )
    {
        return patch_make( NUtils::to_wstring( text1 ), NUtils::to_wstring( text2 ) );
    }

    TPatchVector diff_match_patch::patch_deepCopy( const TPatchVector &patches )
    {
        TPatchVector patchesCopy;
        for ( auto &&aPatch : patches )
        {
            Patch patchCopy = Patch();
            for ( auto &&aDiff : aPatch.diffs )
            {
                patchCopy.diffs.push_back( aDiff );
            }
            patchCopy.start1 = aPatch.start1;
            patchCopy.start2 = aPatch.start2;
            patchCopy.length1 = aPatch.length1;
            patchCopy.length2 = aPatch.length2;
            patchesCopy.emplace_back( patchCopy );
        }
        return patchesCopy;
    }

    std::pair< std::wstring, std::vector< bool > > diff_match_patch::patch_apply( TPatchVector patches, std::wstring text )
    {
        if ( patches.empty() )
        {
            return { text, std::vector< bool >( 0 ) };
        }

        // Deep copy the patches so that no changes are made to originals.
        patches = patch_deepCopy( patches );

        std::wstring nullPadding = patch_addPadding( patches );
        text = nullPadding + text + nullPadding;
        patch_splitMax( patches );

        std::size_t x = 0;
        // delta keeps track of the offset between the expected and actual location
        // of the previous patch.  If there are patches expected at positions 10 and
        // 20, but the first patch was found at 12, delta is 2 and the second patch
        // has an effective expected position of 22.
        uint64_t delta = 0;
        std::vector< bool > results( patches.size() );
        for ( auto &&aPatch : patches )
        {
            auto expected_loc = aPatch.start2 + delta;
            std::wstring text1 = diff_text1( aPatch.diffs );
            std::size_t start_loc;
            std::size_t end_loc = std::string::npos;
            if ( text1.length() > Match_MaxBits )
            {
                // patch_splitMax will only provide an oversized pattern in the case of
                // a monster delete.
                start_loc = match_main( text, text1.substr( 0, Match_MaxBits ), expected_loc );
                if ( start_loc != -1 )
                {
                    end_loc = match_main( text, text1.substr( text1.length() - Match_MaxBits ), expected_loc + text1.length() - Match_MaxBits );
                    if ( end_loc == -1 || start_loc >= end_loc )
                    {
                        // Can't find valid trailing context.  Drop this patch.
                        start_loc = -1;
                    }
                }
            }
            else
            {
                start_loc = match_main( text, text1, expected_loc );
            }
            if ( start_loc == -1 )
            {
                // No match found.  :(
                results[ x ] = false;
                // Subtract the delta for this failed patch from subsequent patches.
                delta -= aPatch.length2 - aPatch.length1;
            }
            else
            {
                // Found a match.  :)
                results[ x ] = true;
                delta = start_loc - expected_loc;
                std::wstring text2;
                if ( end_loc == -1 )
                {
                    text2 = safeMid( text, start_loc, text1.length() );
                }
                else
                {
                    text2 = safeMid( text, start_loc, end_loc + Match_MaxBits - start_loc );
                }
                if ( text1 == text2 )
                {
                    // Perfect match, just shove the replacement text in.
                    text = text.substr( 0, start_loc ) + diff_text2( aPatch.diffs ) + safeMid( text, start_loc + text1.length() );
                }
                else
                {
                    // Imperfect match.  Run a diff to get a framework of equivalent
                    // indices.
                    TDiffVector diffs = diff_main( text1, text2, false );
                    if ( text1.length() > Match_MaxBits && diff_levenshtein( diffs ) / static_cast< float >( text1.length() ) > Patch_DeleteThreshold )
                    {
                        // The end points match, but the content is unacceptably bad.
                        results[ x ] = false;
                    }
                    else
                    {
                        diff_cleanupSemanticLossless( diffs );
                        std::size_t index1 = 0;
                        for ( auto &&aDiff : aPatch.diffs )
                        {
                            if ( !aDiff.isEqual() )
                            {
                                auto index2 = diff_xIndex( diffs, index1 );
                                if ( aDiff.isInsert() )
                                {
                                    // Insertion
                                    text = text.substr( 0, start_loc + index2 ) + aDiff.text() + safeMid( text, start_loc + index2 );
                                }
                                else if ( aDiff.isDelete() )
                                {
                                    // Deletion
                                    text = text.substr( 0, start_loc + index2 ) + safeMid( text, start_loc + diff_xIndex( diffs, index1 + aDiff.text().length() ) );
                                }
                            }
                            if ( !aDiff.isDelete() )
                            {
                                index1 += aDiff.text().length();
                            }
                        }
                    }
                }
            }
            x++;
        }
        // Strip the padding off.
        text = safeMid( text, nullPadding.length(), text.length() - 2 * nullPadding.length() );
        return { text, results };
    }

    std::pair< std::wstring, std::vector< bool > > diff_match_patch::patch_apply( TPatchVector patches, std::string text )
    {
        return patch_apply( patches, NUtils::to_wstring( text ) );
    }

    std::wstring diff_match_patch::patch_addPadding( TPatchVector &patches )
    {
        auto paddingLength = Patch_Margin;
        std::wstring nullPadding;
        for ( char x = 1; x <= paddingLength; x++ )
        {
            nullPadding += NUtils::to_wstring( x );
        }

        // Bump all the patches forward.
        for ( auto &&aPatch : patches )
        {
            aPatch.start1 += paddingLength;
            aPatch.start2 += paddingLength;
        }

        // Add some padding on start of first diff.
        // auto && patch = patches.front();
        // TDiffVector & diffs = patch.diffs;
        if ( patches.front().diffs.empty() || !patches.front().diffs.front().isEqual() )
        {
            // Add nullPadding equality.
            patches.front().diffs.emplace( patches.front().diffs.begin(), EOperation::eEQUAL, nullPadding );
            patches.front().start1 -= paddingLength;   // Should be 0.
            patches.front().start2 -= paddingLength;   // Should be 0.
            patches.front().length1 += paddingLength;
            patches.front().length2 += paddingLength;
        }
        else if ( paddingLength > patches.front().diffs.front().text().length() )
        {
            // Grow first equality.
            auto &&firstDiff = patches.front().diffs.front();
            auto extraLength = paddingLength - firstDiff.text().length();
            firstDiff.fText = nullPadding.substr( firstDiff.text().length() ) + firstDiff.text();
            patches.front().start1 -= extraLength;
            patches.front().start2 -= extraLength;
            patches.front().length1 += extraLength;
            patches.front().length2 += extraLength;
        }

        // Add some padding on end of last diff.
        // patch = patches.back();
        // diffs = patch.diffs;
        if ( ( patches.back().diffs.size() == 0 ) || !patches.back().diffs.back().isEqual() )
        {
            // Add nullPadding equality.
            patches.back().diffs.emplace_back( EOperation::eEQUAL, nullPadding );
            patches.back().length1 += paddingLength;
            patches.back().length2 += paddingLength;
        }
        else if ( paddingLength > patches.back().diffs.back().text().length() )
        {
            // Grow last equality.
            // Diff &lastDiff = patches.back().diffs.back();
            auto extraLength = paddingLength - patches.back().diffs.back().text().length();
            patches.back().diffs.back().fText += nullPadding.substr( 0, extraLength );
            patches.back().length1 += extraLength;
            patches.back().length2 += extraLength;
        }

        return nullPadding;
    }

    void diff_match_patch::patch_splitMax( TPatchVector &patches )
    {
        auto patch_size = Match_MaxBits;
        for ( int x = 0; x < patches.size(); x++ )
        {
            if ( patches[ x ].length1 <= patch_size )
            {
                continue;
            }
            Patch bigpatch = patches[ x ];
            // Remove the big old patch.
            NUtils::Splice( patches, x--, 1 );
            auto start1 = bigpatch.start1;
            auto start2 = bigpatch.start2;
            std::wstring precontext;
            while ( !bigpatch.diffs.empty() )
            {
                // Create one of several smaller patches.
                Patch patch;
                bool empty = true;
                patch.start1 = start1 - precontext.length();
                patch.start2 = start2 - precontext.length();
                if ( precontext.length() != 0 )
                {
                    patch.length1 = patch.length2 = precontext.length();
                    patch.diffs.emplace_back( EOperation::eEQUAL, precontext );
                }
                while ( !bigpatch.diffs.empty() && ( patch.length1 < ( patch_size - Patch_Margin ) ) )
                {
                    auto diff_text = bigpatch.diffs[ 0 ].text();
                    if ( bigpatch.diffs[ 0 ].isInsert() )
                    {
                        // Insertions are harmless.
                        patch.length2 += diff_text.length();
                        start2 += diff_text.length();
                        patch.diffs.push_back( bigpatch.diffs.front() );
                        bigpatch.diffs.erase( bigpatch.diffs.begin() );
                        empty = false;
                    }
                    else if ( ( bigpatch.diffs[ 0 ].isDelete() ) && ( patch.diffs.size() == 1 ) && ( patch.diffs.front().isEqual() ) && ( diff_text.length() > 2 * patch_size ) )
                    {
                        // This is a large deletion.  Let it pass in one chunk.
                        patch.length1 += diff_text.length();
                        start1 += diff_text.length();
                        empty = false;
                        patch.diffs.emplace_back( bigpatch.diffs[ 0 ] );
                        bigpatch.diffs.erase( bigpatch.diffs.begin() );
                    }
                    else
                    {
                        // Deletion or equality.  Only take as much as we can stomach.
                        diff_text = diff_text.substr( 0, std::min( diff_text.length(), ( patch_size > ( patch.length1 + Patch_Margin ) ) ? ( patch_size - patch.length1 - Patch_Margin ) : ( -1 * 1UL ) ) );
                        patch.length1 += diff_text.length();
                        start1 += diff_text.length();
                        if ( bigpatch.diffs[ 0 ].isEqual() )
                        {
                            patch.length2 += diff_text.length();
                            start2 += diff_text.length();
                        }
                        else
                        {
                            empty = false;
                        }
                        patch.diffs.emplace_back( bigpatch.diffs[ 0 ], diff_text );
                        if ( diff_text == bigpatch.diffs[ 0 ].text() )
                        {
                            bigpatch.diffs.erase( bigpatch.diffs.begin() );
                        }
                        else
                        {
                            bigpatch.diffs[ 0 ].fText = bigpatch.diffs[ 0 ].text().substr( diff_text.length() );
                        }
                    }
                }
                // Compute the head context for the next patch.
                precontext = diff_text2( patch.diffs );
                precontext = precontext.substr( std::max( kZERO, ( precontext.length() > Patch_Margin ) ? ( precontext.length() - Patch_Margin ) : 0 ) );

                std::wstring postcontext;
                // Append the end context for this patch.
                if ( diff_text1( bigpatch.diffs ).length() > Patch_Margin )
                {
                    postcontext = diff_text1( bigpatch.diffs ).substr( 0, Patch_Margin );
                }
                else
                {
                    postcontext = diff_text1( bigpatch.diffs );
                }

                if ( postcontext.length() != 0 )
                {
                    patch.length1 += postcontext.length();
                    patch.length2 += postcontext.length();
                    if ( ( patch.diffs.size() != 0 ) && ( patch.diffs[ patch.diffs.size() - 1 ].isEqual() ) )
                    {
                        patch.diffs[ patch.diffs.size() - 1 ].fText += postcontext;
                    }
                    else
                    {
                        patch.diffs.emplace_back( EOperation::eEQUAL, postcontext );
                    }
                }
                if ( !empty )
                {
                    NUtils::Splice( patches, ++x, 0ULL, patch );
                }
            }
        }
    }

    std::wstring diff_match_patch::patch_toText( const TPatchVector &patches )
    {
        std::wstring text;
        for ( auto &&aPatch : patches )
        {
            text += aPatch.toString();
        }
        return text;
    }

    TPatchVector diff_match_patch::patch_fromText( const std::wstring &textline )
    {
        TPatchVector patches;
        if ( textline.empty() )
        {
            return patches;
        }
        auto text = NUtils::splitString( textline, L"\n", true );
        int textPointer = 0;
        std::wstring line;
        while ( textPointer < text.size() )
        {
            patches.push_back( text[ textPointer ] );
            auto &patch = patches.back();
            textPointer++;

            while ( textPointer < text.size() )
            {
                if ( text[ textPointer ].empty() )
                {
                    ++textPointer;
                    continue;
                }

                auto sign = text[ textPointer ][ 0 ];

                line = text[ textPointer ].substr( 1 );
                NUtils::replace( line, L"+", L"%2b" );
                line = NUtils::fromPercentEncoding( line );
                if ( sign == '-' )
                {
                    // Deletion.
                    patch.diffs.emplace_back( EOperation::eDELETE, line );
                }
                else if ( sign == '+' )
                {
                    // Insertion.
                    patch.diffs.emplace_back( EOperation::eINSERT, line );
                }
                else if ( sign == ' ' )
                {
                    // Minor equality.
                    patch.diffs.emplace_back( EOperation::eEQUAL, line );
                }
                else if ( sign == '@' )
                {
                    // Start of next patch.
                    break;
                }
                else
                {
                    // WTF?
                    throw std::wstring( std::wstring( L"Invalid patch mode '" ) + sign + std::wstring( L" in: " ) + line );
                    return {};
                }
                textPointer++;
            }
        }
        return patches;
    }

    TPatchVector diff_match_patch::patch_fromText( const std::string &textline )
    {
        return patch_fromText( NUtils::to_wstring( textline ) );
    }

    std::wstring diff_match_patch::safeMid( const std::wstring &str, std::size_t pos )
    {
        return safeMid( str, pos, std::string::npos );
    }

    std::wstring diff_match_patch::safeMid( const std::wstring &str, std::size_t pos, std::size_t len )
    {
        return ( pos == str.length() ) ? std::wstring() : str.substr( pos, len );
    }

    std::wstring NUtils::to_wstring( const diff_match_patch::TVariant &variant, bool doubleQuoteEmpty )
    {
        std::wstring retVal;
        if ( std::holds_alternative< std::wstring >( variant ) )
            retVal = std::get< std::wstring >( variant );

        if ( doubleQuoteEmpty && retVal.empty() )
            return LR"("")";

        return retVal;
    }

    std::wstring NUtils::to_wstring( const Patch &patch, bool doubleQuoteEmpty )
    {
        auto retVal = patch.toString();
        if ( doubleQuoteEmpty && retVal.empty() )
            return LR"("")";
        return retVal;
    }

    std::wstring NUtils::to_wstring( const Diff &diff, bool doubleQuoteEmpty )
    {
        auto retVal = diff.toString( EStringType::eDefault );
        if ( doubleQuoteEmpty && retVal.empty() )
            return LR"("")";
        return retVal;
    }
}
