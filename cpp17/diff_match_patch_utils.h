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

#ifndef DIFF_MATCH_PATCH_UTILS_H
#define DIFF_MATCH_PATCH_UTILS_H
//
#include <cassert>
#include <string>
#include <vector>
namespace NDiffMatchPatch
{
    namespace NUtils
    {
        using TStringVector = std::vector< std::wstring >;

        /*
 * Utility functions to replace Qt built in methods
 */

        /**
 * A safer version of std::wstring.mid(pos).  This one returns "" instead of
 * null when the postion equals the string length.
 * @param str String to take a substring from.
 * @param pos Position to start the substring from.
 * @return Substring.
 */
        std::wstring safeMid( const std::wstring &str, std::size_t pos );

        /**
 * A safer version of std::wstring.mid(pos, len).  This one returns "" instead
 * of null when the postion equals the string length.
 * @param str String to take a substring from.
 * @param pos Position to start the substring from.
 * @param len Length of substring.
 * @return Substring.
 */
        std::wstring safeMid( const std::wstring &str, std::size_t pos, std::size_t len );

        /**

 * replaces QString::replace
 * @param haystack String to replace all needles with to
 * @param needle Substring to search for in the haystack
 * @param to replacement string
 * @return void.
 */
        void replace( std::wstring &haystack, const std::wstring &needle, const std::wstring &to );

        /**
 * replaces returns the html percent encoded character equivalent
 * @param c the input Character to return the encoded string of
 * @param exclude The list of chars that are NOT to be encoded
 * @param include The list of chars that are to be encoded
 * @return the encoded string
 */
        std::wstring toPercentEncoding( wchar_t c, const std::wstring &exclude = std::wstring(), const std::wstring &include = std::wstring() );

        /**
 * return the html percent encoded string equivalent
 * @param input the input String to return the encoded string of
 * @param exclude The list of chars that are NOT to be encoded
 * @param include The list of chars that are to be encoded
 * @return the encoded string
 */
        std::wstring toPercentEncoding( const std::wstring &input, const std::wstring &exclude = std::wstring(), const std::wstring &include = std::wstring() );

        /**
 * returns the string equivalent removing any percent encoding and replacing it
 * with the correct character
 * @param input the input String to return the encoded string of
 * @return the decoded string
 */
        std::wstring fromPercentEncoding( const std::wstring &input );

        /**
 * replaces returns integer value of the character, '0'-'9' = 0-9, 'A'-'F' =
 * 10-15, 'a'-'f' = 10-15
 * @param input the value to return  the integer value of
 * @return the integer value of the character
 */
        wchar_t getIntValue( wchar_t ch );

        /**
 * return the integer value of the string
 * @param string the String to be converted to an integer
 * @return the integer version, on an invalid input returns 0
 */
        int64_t toInt( const std::wstring &string );

        /**
 * return true if the string has the suffix
 * @param string the String to check to see if it ends with suffix
 * @param suffix the String to see if the input string ends with
 * @return True if the string ends with suffix
 */
        bool endsWith( const std::wstring &string, const std::wstring &suffix );

        /**
 * return a TStringVector of the string split by separator
 * @param string the String to be split
 * @param separator the String to search in the input string to split on
 * @param if true, empty values will be removed
 * @return the split string
 */
        TStringVector splitString( const std::wstring &string, const std::wstring &separator, bool skipEmptyParts );

        /**
 * splices the objects vector into the input vector
 * @param input The input vector to splice out from
 * @param start The position of the first item to remove from the input vector
 * @param count How many values to remove from the input vector
 * @param objects optional objects to insert where the previous objects were
 * removed
 * @return the character as a single character string
 */
        template< typename T >
        static std::vector< T > Splice( std::vector< T > &input, std::size_t start, std::size_t count, const std::vector< T > &objects = {} )
        {
            auto deletedRange = std::vector< T >( { input.begin() + start, input.begin() + start + count } );
            input.erase( input.begin() + start, input.begin() + start + count );
            input.insert( input.begin() + start, objects.begin(), objects.end() );

            return deletedRange;
        }

        /**
 * splices the objects vector into the input vector
 * @param input The input vector to splice out from
 * @param start The position of the first item to remove from the input vector
 * @param count How many values to remove from the input vector
 * @param object individual object to insert where the previous objects were
 * removed
 * @return the character as a single character string
 */
        template< typename T1 >
        using base_type = typename std::remove_reference< std::remove_cv_t< std::decay_t< T1 > > >;

        template< typename T1 >
        using base_type_t = typename base_type< T1 >::type;

        template< typename STRING_TYPE >
        using is_wstring = typename std::is_same< std::wstring, base_type_t< STRING_TYPE > >;

        template< typename STRING_TYPE >
        using is_wcharstar = typename std::is_same< wchar_t *, base_type_t< STRING_TYPE > >;

        template< typename STRING_TYPE >
        using is_string = typename std::is_same< std::string, base_type_t< STRING_TYPE > >;

        template< typename STRING_TYPE >
        using is_charstar = typename std::is_same< char *, base_type_t< STRING_TYPE > >;

        template< typename T >
        static std::vector< T > Splice( std::vector< T > &input, std::size_t start, std::size_t count, const T &object )
        {
            return Splice( input, start, count, std::vector< T >( { object } ) );
        }

        template< typename T >
        std::wstring to_wstring( const T & /*value*/, bool /*doubleQuoteEmpty*/ )
        {
            assert( false );
            return {};
        }

        /**
 * return the single character wide string for the given character
 * @param value the char to be converted to an wstring
 * @param doubleQuoteEmpty, if the return value would be empty, return ""
 * @return the character as a single character string
 */
        inline std::wstring to_wstring( const char &value, bool doubleQuoteEmpty )
        {
            if ( doubleQuoteEmpty && ( value == 0 ) )
                return LR"("")";

            return std::wstring( 1, static_cast< wchar_t >( value ) );
        }

        template<>
        inline std::wstring to_wstring( const bool &value, bool /*doubleQuoteOnEmpty*/ )
        {
            std::wstring retVal = std::wstring( value ? L"true" : L"false" );
            return retVal;
        }

        template<>
        inline std::wstring to_wstring( const std::vector< bool >::reference &value, bool /*doubleQuoteOnEmpty*/ )
        {
            std::wstring retVal = std::wstring( value ? L"true" : L"false" );
            return retVal;
        }

        template<>
        inline std::wstring to_wstring( const std::string &string, bool doubleQuoteEmpty )
        {
            if ( doubleQuoteEmpty && string.empty() )
                return LR"("")";

            std::wstring wstring( string.size(),
                                  L' ' );   // Overestimate number of code points.
            wstring.resize( std::mbstowcs( &wstring[ 0 ], string.c_str(),
                                           string.size() ) );   // Shrink to fit.
            return wstring;
        }

        template<>
        inline std::wstring to_wstring( const wchar_t &value, bool doubleQuoteEmpty )
        {
            if ( doubleQuoteEmpty && ( value == 0 ) )
                return LR"("")";

            return std::wstring( 1, value );
        }

        template<>
        inline std::wstring to_wstring( const int &value, bool doubleQuoteEmpty )
        {
            return to_wstring( static_cast< wchar_t >( value ), doubleQuoteEmpty );
        }

        template<>
        inline std::wstring to_wstring( const std::wstring &value, bool doubleQuoteEmpty )
        {
            if ( doubleQuoteEmpty && value.empty() )
                return LR"("")";

            return value;
        }

        template< typename T >
        inline std::wstring to_wstring( const std::vector< T > &values, bool doubleQuoteEmpty )
        {
            std::wstring retVal = L"(";
            bool first = true;
            for ( auto &&curr : values )
            {
                if ( !first )
                {
                    retVal += L", ";
                }
                retVal += to_wstring( curr, doubleQuoteEmpty );
                first = false;
            }
            retVal += L")";
            return retVal;
        }

        template<>
        inline std::wstring to_wstring( const std::vector< bool > &boolArray, bool doubleQuoteOnEmpty )
        {
            if ( boolArray.empty() && doubleQuoteOnEmpty )
                return LR"("")";
            std::wstring retVal = L"(";
            bool first = true;
            for ( auto &&curr : boolArray )
            {
                if ( !first )
                    retVal += L", ";
                first = false;
                retVal += to_wstring( curr, doubleQuoteOnEmpty );
            }
            retVal += L")";
            return retVal;
        }

        template< typename T >
        inline typename std::enable_if_t< std::is_integral_v< T >, std::wstring > to_wstring( const std::initializer_list< T > &values, bool doubleQuoteEmpty )
        {
            if ( doubleQuoteEmpty && ( values.size() == 0 ) )
                return LR"("")";

            std::wstring retVal;
            for ( auto &&curr : values )
            {
                retVal += to_wstring( curr, false );
            }
            return retVal;
        }

        template< typename T >
        inline typename std::enable_if_t< !std::is_integral_v< T >, std::wstring > to_wstring( const std::initializer_list< T > &values, bool doubleQuoteEmpty )
        {
            std::wstring retVal = L"(";
            bool first = true;
            for ( auto &&curr : values )
            {
                if ( !first )
                {
                    retVal += L", ";
                }
                retVal += to_wstring( curr, doubleQuoteEmpty );
                first = false;
            }
            retVal += L")";
            return retVal;
        }

        //template< typename T >
        //std::wstring to_wstring( const T &value )
        //{
        //    return to_wstring( value, false );
        //}

        template< typename STRING_TYPE >
        std::wstring to_wstring( const STRING_TYPE &string )
        {
            static_assert(
                is_wstring< STRING_TYPE >::value   //
                || is_wcharstar< STRING_TYPE >::value   //
                || is_string< STRING_TYPE >::value   //
                || is_charstar< STRING_TYPE >::value   //
                || std::is_same_v< char, STRING_TYPE >   //
                || std::is_same_v< wchar_t, STRING_TYPE >   //
            );

            if constexpr ( is_wstring< STRING_TYPE >::value )
                return string;
            else if constexpr ( is_wcharstar< STRING_TYPE >::value )
                return std::wstring( string ? string : L"" );
            else if constexpr ( is_string< STRING_TYPE >::value )
                return to_wstring( string, false );
            else if constexpr ( is_charstar< STRING_TYPE >::value )
                return to_wstring( std::string( string ? string : "" ), false );
            else if constexpr ( std::is_same_v< char, STRING_TYPE > )
                return to_wstring( string, false );
            else if constexpr ( std::is_same_v< wchar_t, STRING_TYPE > )
                return to_wstring( string, false );
        }

    }
}

#endif
