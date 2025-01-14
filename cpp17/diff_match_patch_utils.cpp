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

#include "diff_match_patch_utils.h"

#include <codecvt>
#include <locale>

namespace NDiffMatchPatch
{
    namespace NUtils
    {
        std::wstring safeMid( const std::wstring &str, std::size_t pos )
        {
            return safeMid( str, pos, std::string::npos );
        }

        std::wstring safeMid( const std::wstring &str, std::size_t pos, std::size_t len )
        {
            return ( pos == str.length() ) ? std::wstring() : str.substr( pos, len );
        }

        void replace( std::wstring &inString, const std::wstring &from, const std::wstring &to )
        {
            std::size_t pos = inString.find( from );
            while ( pos != std::wstring::npos )
            {
                inString.replace( pos, from.length(), to );
                pos = inString.find( from, pos + to.length() );
            }
        }

        wchar_t toHexUpper( wchar_t value )
        {
            return L"0123456789ABCDEF"[ value & 0xF ];
        }

        std::wstring toPercentEncoding( wchar_t c, const std::wstring &exclude, const std::wstring &include )
        {
            std::wstring retVal;

            if ( ( ( c >= 0x61 && c <= 0x7A )   // ALPHA
                   || ( c >= 0x41 && c <= 0x5A )   // ALPHA
                   || ( c >= 0x30 && c <= 0x39 )   // DIGIT
                   || c == 0x2D   // -
                   || c == 0x2E   // .
                   || c == 0x5F   // _
                   || c == 0x7E   // ~
                   || ( exclude.find( c ) != std::string::npos ) )
                 && ( include.find( c ) == std::string::npos ) )
            {
                retVal = std::wstring( 1, c );
            }
            else
            {
                retVal = L'%';
                retVal += toHexUpper( ( c & 0xf0 ) >> 4 );
                retVal += toHexUpper( c & 0xf );
            }
            return retVal;
        }

        std::wstring toPercentEncoding( const std::wstring &input, const std::wstring &exclude /*= std::wstring()*/, const std::wstring &include /*= std::wstring() */ )
        {
            if ( input.empty() )
                return {};
            std::wstring retVal;
            retVal.reserve( input.length() * 3 );

            static_assert( sizeof( wchar_t ) <= 4, "wchar_t is greater that 32 bit" );

            std::wstring_convert< std::codecvt_utf8< wchar_t > > utf8_conv;
            for ( auto &&c : input )
            {
                auto currStr = std::wstring( 1, c );
                auto asBytes = utf8_conv.to_bytes( currStr );
                for ( auto &&ii : asBytes )
                {
                    if ( ii )
                        retVal += toPercentEncoding( ii, exclude, include );
                }
            }
            return retVal;
        }

        wchar_t getValue( wchar_t ch )
        {
            if ( ch >= '0' && ch <= '9' )
                ch -= '0';
            else if ( ch >= 'a' && ch <= 'f' )
                ch = ch - 'a' + 10;
            else if ( ch >= 'A' && ch <= 'F' )
                ch = ch - 'A' + 10;
            else
                throw std::wstring( L"Invalid Character %" ) + ch;

            return ch;
        }

        std::wstring fromPercentEncoding( const std::wstring &input )
        {
            if ( input.empty() )
                return {};
            std::string retVal;
            retVal.reserve( input.length() );
            for ( auto ii = 0ULL; ii < input.length(); ++ii )
            {
                auto c = input[ ii ];
                if ( c == L'%' && ( ii + 2 ) < input.length() )
                {
                    auto a = input[ ++ii ];
                    auto b = input[ ++ii ];
                    a = getValue( a );
                    b = getValue( b );
                    a = a << 4;
                    auto value = a | b;
                    retVal += std::string( 1, value );
                }
                else if ( c == '+' )
                    retVal += ' ';
                else
                {
                    retVal += c;
                }
            }
            std::wstring_convert< std::codecvt_utf8< wchar_t > > utf8_conv;
            auto asBytes = utf8_conv.from_bytes( retVal );

            return asBytes;
        }

        bool endsWith( const std::wstring &string, const std::wstring &suffix )
        {
            if ( suffix.length() > string.length() )
                return false;

            return string.compare( string.length() - suffix.length(), suffix.length(), suffix ) == 0;
        }

        TStringVector splitString( const std::wstring &string, const std::wstring &separator, bool skipEmptyParts )
        {
            if ( separator.empty() )
            {
                if ( !skipEmptyParts || !string.empty() )
                    return { string };
                return {};
            }

            TStringVector strings;
            auto prevPos = 0ULL;
            auto startPos = string.find_first_of( separator );
            while ( startPos != std::string::npos )
            {
                auto start = prevPos ? prevPos + 1 : prevPos;
                auto len = prevPos ? ( startPos - prevPos - 1 ) : startPos;
                auto curr = string.substr( start, len );
                prevPos = startPos;
                if ( !skipEmptyParts || !curr.empty() )
                    strings.emplace_back( curr );
                startPos = string.find_first_of( separator, prevPos + 1 );
            }
            auto remainder = string.substr( prevPos ? prevPos + 1 : prevPos );
            if ( !skipEmptyParts || !remainder.empty() )
                strings.emplace_back( remainder );

            return strings;
        }

        int64_t toInt( const std::wstring &string )
        {
            int64_t retVal = 0;
            try
            {
                std::size_t lastPos{};
                retVal = std::stoul( string, &lastPos );
                if ( lastPos != string.length() )
                    return 0;
            }
            catch ( ... )
            {
            }
            return retVal;
        }

    }   // namespace NUtils
}
