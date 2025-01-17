/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2019 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file string.cpp
 * @brief Some useful functions to handle strings.
 */

#include <clocale>
#include <cmath>
#include <macros.h>
#include <richio.h>                        // StrPrintf
#include <kicad_string.h>


/**
 * Illegal file name characters used to insure file names will be valid on all supported
 * platforms.  This is the list of illegal file name characters for Windows which includes
 * the illegal file name characters for Linux and OSX.
 */
static const char illegalFileNameChars[] = "\\/:\"<>|";


bool ConvertSmartQuotesAndDashes( wxString* aString )
{
    bool retVal = false;

    for( wxString::iterator ii = aString->begin(); ii != aString->end(); ++ii )
    {
        if( *ii == L'\u00B4' || *ii == L'\u2018' || *ii == L'\u2019' )
        {
            *ii = '\'';
            retVal = true;
        }
        if( *ii == L'\u201C' || *ii == L'\u201D' )
        {
            *ii = '"';
            retVal = true;
        }
        if( *ii == L'\u2013' || *ii == L'\u2014' )
        {
            *ii = '-';
            retVal = true;
        }
    }

    return retVal;
}


/**
 * These Escape/Unescape routines use HTML-entity-reference-style encoding to handle
 * characters which are:
 *   (a) not legal in filenames
 *   (b) used as control characters in LIB_IDs
 *   (c) used to delineate hierarchical paths
 */
wxString EscapeString( const wxString& aSource, ESCAPE_CONTEXT aContext )
{
    wxString converted;

    for( wxUniChar c: aSource )
    {
        if( aContext == CTX_NETNAME )
        {
            if( c == '/' )
                converted += "{slash}";
            else if( c == '\n' || c == '\r' )
                converted += "";    // drop
            else
                converted += c;
        }
        else if( aContext == CTX_LIBID )
        {
            if( c == '{' )
                converted += "{brace}";
            else if( c == ':' )
                converted += "{colon}";
            else if( c == '\n' || c == '\r' )
                converted += "";    // drop
            else
                converted += c;
        }
        else if( aContext == CTX_QUOTED_STR )
        {
            if( c == '\"' )
                converted += "{dblquote}";
            else
                converted += c;
        }
        else if( aContext == CTX_LINE )
        {
            if( c == '\n' || c == '\r' )
                converted += "{return}";
            else
                converted += c;
        }
        else if( aContext == CTX_FILENAME )
        {
            if( c == '{' )
                converted += "{brace}";
            else if( c == '/' )
                converted += "{slash}";
            else if( c == '\\' )
                converted += "{backslash}";
            else if( c == '\"' )
                converted += "{dblquote}";
            else if( c == '<' )
                converted += "{lt}";
            else if( c == '>' )
                converted += "{gt}";
            else if( c == '|' )
                converted += "{bar}";
            else if( c == ':' )
                converted += "{colon}";
            else if( c == '\t' )
                converted += "{tab}";
            else if( c == '\n' || c == '\r' )
                converted += "{return}";
            else
                converted += c;
        }
        else
            converted += c;
    }

    return converted;
}


wxString UnescapeString( const wxString& aSource )
{
    wxString newbuf;
    size_t   sourceLen = aSource.length();

    for( size_t i = 0; i < sourceLen; ++i )
    {
        if( ( aSource[i] == '$' || aSource[i] == '^' || aSource[i] == '_' )
                && i + 1 < sourceLen && aSource[i+1] == '{' )
        {
            for( ; i < sourceLen; ++i )
            {
                newbuf += aSource[i];

                if( aSource[i] == '}' )
                    break;
            }
        }
        else if( aSource[i] == '{' )
        {
            wxString token;
            int      depth = 1;

            for( i = i + 1; i < sourceLen; ++i )
            {
                if( aSource[i] == '{' )
                    depth++;
                else if( aSource[i] == '}' )
                    depth--;

                if( depth <= 0 )
                    break;
                else
                    token.append( aSource[i] );
            }

            if(      token == wxS( "dblquote" ) )  newbuf.append( wxS( "\"" ) );
            else if( token == wxS( "quote" ) )     newbuf.append( wxS( "'" ) );
            else if( token == wxS( "lt" ) )        newbuf.append( wxS( "<" ) );
            else if( token == wxS( "gt" ) )        newbuf.append( wxS( ">" ) );
            else if( token == wxS( "backslash" ) ) newbuf.append( wxS( "\\" ) );
            else if( token == wxS( "slash" ) )     newbuf.append( wxS( "/" ) );
            else if( token == wxS( "bar" ) )       newbuf.append( wxS( "|" ) );
            else if( token == wxS( "colon" ) )     newbuf.append( wxS( ":" ) );
            else if( token == wxS( "space" ) )     newbuf.append( wxS( " " ) );
            else if( token == wxS( "dollar" ) )    newbuf.append( wxS( "$" ) );
            else if( token == wxS( "tab" ) )       newbuf.append( wxS( "\t" ) );
            else if( token == wxS( "return" ) )    newbuf.append( wxS( "\n" ) );
            else if( token == wxS( "brace" ) )     newbuf.append( wxS( "{" ) );
            else if( token.IsEmpty() )             newbuf.append( wxS( "{" ) );
            else
            {
                newbuf.append( "{" + UnescapeString( token ) + "}" );
            }
        }
        else
        {
            newbuf.append( aSource[i] );
        }
    }

    return newbuf;
}


wxString TitleCaps( const wxString& aString )
{
    wxArrayString words;
    wxString      result;

    wxStringSplit( aString, words, ' ' );

    for( const wxString& word : words )
    {
        if( !result.IsEmpty() )
            result += wxT( " " );

        result += word.Capitalize();
    }

    return result;
}


int ReadDelimitedText( wxString* aDest, const char* aSource )
{
    std::string utf8;               // utf8 but without escapes and quotes.
    bool        inside = false;
    const char* start = aSource;
    char        cc;

    while( (cc = *aSource++) != 0  )
    {
        if( cc == '"' )
        {
            if( inside )
                break;          // 2nd double quote is end of delimited text

            inside = true;      // first delimiter found, make note, do not copy
        }

        else if( inside )
        {
            if( cc == '\\' )
            {
                cc = *aSource++;

                if( !cc )
                    break;

                // do no copy the escape byte if it is followed by \ or "
                if( cc != '"' && cc != '\\' )
                    utf8 += '\\';

                utf8 += cc;
            }
            else
            {
                utf8 += cc;
            }
        }
    }

    *aDest = FROM_UTF8( utf8.c_str() );

    return aSource - start;
}


int ReadDelimitedText( char* aDest, const char* aSource, int aDestSize )
{
    if( aDestSize <= 0 )
        return 0;

    bool        inside = false;
    const char* start = aSource;
    char*       limit = aDest + aDestSize - 1;
    char        cc;

    while( (cc = *aSource++) != 0 && aDest < limit )
    {
        if( cc == '"' )
        {
            if( inside )
                break;          // 2nd double quote is end of delimited text

            inside = true;      // first delimiter found, make note, do not copy
        }

        else if( inside )
        {
            if( cc == '\\' )
            {
                cc = *aSource++;

                if( !cc )
                    break;

                // do no copy the escape byte if it is followed by \ or "
                if( cc != '"' && cc != '\\' )
                    *aDest++ = '\\';

                if( aDest < limit )
                    *aDest++ = cc;
            }
            else
            {
                *aDest++ = cc;
            }
        }
    }

    *aDest = 0;

    return aSource - start;
}


std::string EscapedUTF8( wxString aString )
{
    // No new-lines allowed in quoted strings
    aString.Replace( "\r\n", "\r" );
    aString.Replace( "\n", "\r" );

    std::string utf8 = TO_UTF8( aString );

    std::string ret;

    ret += '"';

    for( std::string::const_iterator it = utf8.begin();  it!=utf8.end();  ++it )
    {
        // this escaping strategy is designed to be compatible with ReadDelimitedText():
        if( *it == '"' )
        {
            ret += '\\';
            ret += '"';
        }
        else if( *it == '\\' )
        {
            ret += '\\';    // double it up
            ret += '\\';
        }
        else
        {
            ret += *it;
        }
    }

    ret += '"';

    return ret;
}


wxString EscapeHTML( const wxString& aString )
{
    wxString converted;

    for( wxUniChar c: aString )
    {
        if( c == '\"' )
            converted += "&quot;";
        else if( c == '\'' )
            converted += "&apos;";
        else if( c == '&' )
            converted += "&amp;";
        else if( c == '<' )
            converted += "&lt;";
        else if( c == '>' )
            converted += "&gt;";
        else
            converted += c;
    }

    return converted;
}


bool NoPrintableChars( wxString aString )
{
    return aString.Trim( true ).Trim( false ).IsEmpty();
}


char* StrPurge( char* text )
{
    static const char whitespace[] = " \t\n\r\f\v";

    if( text )
    {
        while( *text && strchr( whitespace, *text ) )
            ++text;

        char* cp = text + strlen( text ) - 1;

        while( cp >= text && strchr( whitespace, *cp ) )
            *cp-- = '\0';
    }

    return text;
}


char* GetLine( FILE* File, char* Line, int* LineNum, int SizeLine )
{
    do {
        if( fgets( Line, SizeLine, File ) == NULL )
            return NULL;

        if( LineNum )
            *LineNum += 1;

    } while( Line[0] == '#' || Line[0] == '\n' ||  Line[0] == '\r' || Line[0] == 0 );

    strtok( Line, "\n\r" );
    return Line;
}


wxString DateAndTime()
{
    wxDateTime datetime = wxDateTime::Now();

    datetime.SetCountry( wxDateTime::Country_Default );
    return datetime.Format( wxDefaultDateTimeFormat, wxDateTime::Local );
}


int StrNumCmp( const wxString& aString1, const wxString& aString2, bool aIgnoreCase )
{
    int nb1 = 0, nb2 = 0;

    auto str1 = aString1.begin();
    auto str2 = aString2.begin();

    while( str1 != aString1.end() && str2 != aString2.end() )
    {
        wxUniChar c1 = *str1;
        wxUniChar c2 = *str2;

        if( wxIsdigit( c1 ) && wxIsdigit( c2 ) ) // Both characters are digits, do numeric compare.
        {
            nb1 = 0;
            nb2 = 0;

            do
            {
                c1 = *str1;
                nb1 = nb1 * 10 + (int) c1 - '0';
                ++str1;
            } while( str1 != aString1.end() && wxIsdigit( *str1 ) );

            do
            {
                c2 = *str2;
                nb2 = nb2 * 10 + (int) c2 - '0';
                ++str2;
            } while( str2 != aString2.end() && wxIsdigit( *str2 ) );

            if( nb1 < nb2 )
                return -1;

            if( nb1 > nb2 )
                return 1;

            c1 = ( str1 != aString1.end() ) ? *str1 : wxUniChar( 0 );
            c2 = ( str2 != aString2.end() ) ? *str2 : wxUniChar( 0 );
        }

        // Any numerical comparisons to here are identical.
        if( aIgnoreCase )
        {
            if( wxToupper( c1 ) < wxToupper( c2 ) )
                return -1;

            if( wxToupper( c1 ) > wxToupper( c2 ) )
                return 1;
        }
        else
        {
            if( c1 < c2 )
                return -1;

            if( c1 > c2 )
                return 1;
        }

        if( str1 != aString1.end() )
            ++str1;

        if( str2 != aString2.end() )
            ++str2;
    }

    if( str1 == aString1.end() && str2 != aString2.end() )
    {
        return -1;   // Identical to here but aString1 is longer.
    }
    else if( str1 != aString1.end() && str2 == aString2.end() )
    {
        return 1;    // Identical to here but aString2 is longer.
    }

    return 0;
}


bool WildCompareString( const wxString& pattern, const wxString& string_to_tst,
                        bool case_sensitive )
{
    const wxChar* cp = NULL, * mp = NULL;
    const wxChar* wild, * str;
    wxString      _pattern, _string_to_tst;

    if( case_sensitive )
    {
        wild = pattern.GetData();
        str = string_to_tst.GetData();
    }
    else
    {
        _pattern = pattern;
        _pattern.MakeUpper();
        _string_to_tst = string_to_tst;
        _string_to_tst.MakeUpper();
        wild   = _pattern.GetData();
        str = _string_to_tst.GetData();
    }

    while( ( *str ) && ( *wild != '*' ) )
    {
        if( ( *wild != *str ) && ( *wild != '?' ) )
            return false;

        wild++;
        str++;
    }

    while( *str )
    {
        if( *wild == '*' )
        {
            if( !*++wild )
                return 1;
            mp = wild;
            cp = str + 1;
        }
        else if( ( *wild == *str ) || ( *wild == '?' ) )
        {
            wild++;
            str++;
        }
        else
        {
            wild   = mp;
            str = cp++;
        }
    }

    while( *wild == '*' )
    {
        wild++;
    }

    return !*wild;
}


bool ApplyModifier( double& value, const wxString& aString )
{
    static const wxString modifiers( wxT( "pnumkKM" ) );

    if( !aString.length() )
        return false;

    wxChar   modifier;
    wxString units;

    if( modifiers.Find( aString[ 0 ] ) >= 0 )
    {
        modifier = aString[ 0 ];
        units = aString.Mid( 1 ).Trim();
    }
    else
    {
        modifier = ' ';
        units = aString.Mid( 0 ).Trim();
    }

    if( units.length()
            && !units.CmpNoCase( wxT( "F" ) )
            && !units.CmpNoCase( wxT( "hz" ) )
            && !units.CmpNoCase( wxT( "W" ) )
            && !units.CmpNoCase( wxT( "V" ) )
            && !units.CmpNoCase( wxT( "H" ) ) )
        return false;

    if( modifier == 'p' )
        value *= 1.0e-12;
    if( modifier == 'n' )
        value *= 1.0e-9;
    else if( modifier == 'u' )
        value *= 1.0e-6;
    else if( modifier == 'm' )
        value *= 1.0e-3;
    else if( modifier == 'k' || modifier == 'K' )
        value *= 1.0e3;
    else if( modifier == 'M' )
        value *= 1.0e6;
    else if( modifier == 'G' )
        value *= 1.0e9;

    return true;
}


int ValueStringCompare( wxString strFWord, wxString strSWord )
{
    // Compare unescaped text
    strFWord = UnescapeString( strFWord );
    strSWord = UnescapeString( strSWord );

    // The different sections of the two strings
    wxString strFWordBeg, strFWordMid, strFWordEnd;
    wxString strSWordBeg, strSWordMid, strSWordEnd;

    // Split the two strings into separate parts
    SplitString( strFWord, &strFWordBeg, &strFWordMid, &strFWordEnd );
    SplitString( strSWord, &strSWordBeg, &strSWordMid, &strSWordEnd );

    // Compare the Beginning section of the strings
    int isEqual = strFWordBeg.CmpNoCase( strSWordBeg );

    if( isEqual > 0 )
        return 1;
    else if( isEqual < 0 )
        return -1;
    else
    {
        // If the first sections are equal compare their digits
        double lFirstNumber  = 0;
        double lSecondNumber = 0;
        bool   endingIsModifier = false;

        strFWordMid.ToDouble( &lFirstNumber );
        strSWordMid.ToDouble( &lSecondNumber );

        endingIsModifier |= ApplyModifier( lFirstNumber, strFWordEnd );
        endingIsModifier |= ApplyModifier( lSecondNumber, strSWordEnd );

        if( lFirstNumber > lSecondNumber )
            return 1;
        else if( lFirstNumber < lSecondNumber )
            return -1;
        // If the first two sections are equal and the endings are modifiers then compare them
        else if( !endingIsModifier )
            return strFWordEnd.CmpNoCase( strSWordEnd );
        // Ran out of things to compare; they must match
        else
            return 0;
    }
}


int SplitString( wxString  strToSplit,
                 wxString* strBeginning,
                 wxString* strDigits,
                 wxString* strEnd )
{
    static const wxString separators( wxT( ".," ) );

    // Clear all the return strings
    strBeginning->Empty();
    strDigits->Empty();
    strEnd->Empty();

    // There no need to do anything if the string is empty
    if( strToSplit.length() == 0 )
        return 0;

    // Starting at the end of the string look for the first digit
    int ii;

    for( ii = (strToSplit.length() - 1); ii >= 0; ii-- )
    {
        if( wxIsdigit( strToSplit[ii] ) )
            break;
    }

    // If there were no digits then just set the single string
    if( ii < 0 )
    {
        *strBeginning = strToSplit;
    }
    else
    {
        // Since there is at least one digit this is the trailing string
        *strEnd = strToSplit.substr( ii + 1 );

        // Go to the end of the digits
        int position = ii + 1;

        for( ; ii >= 0; ii-- )
        {
            if( !wxIsdigit( strToSplit[ii] ) && separators.Find( strToSplit[ii] ) < 0 )
                break;
        }

        // If all that was left was digits, then just set the digits string
        if( ii < 0 )
            *strDigits = strToSplit.substr( 0, position );

        /* We were only looking for the last set of digits everything else is
         * part of the preamble */
        else
        {
            *strDigits    = strToSplit.substr( ii + 1, position - ii - 1 );
            *strBeginning = strToSplit.substr( 0, ii + 1 );
        }
    }

    return 0;
}


int GetTrailingInt( const wxString& aStr )
{
    int number = 0;
    int base = 1;

    // Trim and extract the trailing numeric part
    int index = aStr.Len() - 1;

    while( index >= 0 )
    {
        const char chr = aStr.GetChar( index );

        if( chr < '0' || chr > '9' )
            break;

        number += ( chr - '0' ) * base;
        base *= 10;
        index--;
    }

    return number;
}


wxString GetIllegalFileNameWxChars()
{
    return FROM_UTF8( illegalFileNameChars );
}


bool ReplaceIllegalFileNameChars( std::string* aName, int aReplaceChar )
{
    bool changed = false;
    std::string result;
    result.reserve( aName->length() );

    for( std::string::iterator it = aName->begin();  it != aName->end();  ++it )
    {
        if( strchr( illegalFileNameChars, *it ) )
        {
            if( aReplaceChar )
                StrPrintf( &result, "%c", aReplaceChar );
            else
                StrPrintf( &result, "%%%02x", *it );

            changed = true;
        }
        else
        {
            result += *it;
        }
    }

    if( changed )
        *aName = result;

    return changed;
}


bool ReplaceIllegalFileNameChars( wxString& aName, int aReplaceChar )
{
    bool changed = false;
    wxString result;
    result.reserve( aName.Length() );
    wxString illWChars = GetIllegalFileNameWxChars();

    for( wxString::iterator it = aName.begin();  it != aName.end();  ++it )
    {
        if( illWChars.Find( *it ) != wxNOT_FOUND )
        {
            if( aReplaceChar )
                result += aReplaceChar;
            else
                result += wxString::Format( "%%%02x", *it );

            changed = true;
        }
        else
        {
            result += *it;
        }
    }

    if( changed )
        aName = result;

    return changed;
}


void wxStringSplit( const wxString& aText, wxArrayString& aStrings, wxChar aSplitter )
{
    wxString tmp;

    for( unsigned ii = 0; ii < aText.Length(); ii++ )
    {
        if( aText[ii] == aSplitter )
        {
            aStrings.Add( tmp );
            tmp.Clear();
        }

        else
            tmp << aText[ii];
    }

    if( !tmp.IsEmpty() )
    {
        aStrings.Add( tmp );
    }
}


void StripTrailingZeros( wxString& aStringValue, unsigned aTrailingZeroAllowed )
{
    struct lconv* lc      = localeconv();
    char          sep     = lc->decimal_point[0];
    unsigned      sep_pos = aStringValue.Find( sep );

    if( sep_pos > 0 )
    {
        // We want to keep at least aTrailingZeroAllowed digits after the separator
        unsigned min_len = sep_pos + aTrailingZeroAllowed + 1;

        while( aStringValue.Len() > min_len )
        {
            if( aStringValue.Last() == '0' )
                aStringValue.RemoveLast();
            else
                break;
        }
    }
}


std::string Double2Str( double aValue )
{
    char    buf[50];
    int     len;

    if( aValue != 0.0 && std::fabs( aValue ) <= 0.0001 )
    {
        // For these small values, %f works fine,
        // and %g gives an exponent
        len = sprintf( buf,  "%.16f", aValue );

        while( --len > 0 && buf[len] == '0' )
            buf[len] = '\0';

        if( buf[len] == '.' )
            buf[len] = '\0';
        else
            ++len;
    }
    else
    {
        // For these values, %g works fine, and sometimes %f
        // gives a bad value (try aValue = 1.222222222222, with %.16f format!)
        len = sprintf( buf, "%.10g", aValue );
    }

    return std::string( buf, len );
}


wxString AngleToStringDegrees( double aAngle )
{
    wxString text;

    text.Printf( wxT( "%.3f" ), aAngle / 10.0 );
    StripTrailingZeros( text, 1 );

    return text;
}
