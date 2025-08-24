/*
 * Hoover Chess Utilities / PGN reader
 * Copyright (C) 2022-2025  Sami Kiminki
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

%option   8bit
%option   batch
%option   c++
%option   full
%option   nodefault
%option   noinput
%option   nounistd
%option   nounput
%option   noyywrap
%option   warn
%option   yyclass="hoover_chess_utils::pgn_reader::PgnScanner"
%option   yylineno

%x PGNTAG PGNCOMMENT

%{
#undef yyFlexLexer
#include "pgnscanner.h"

#include <format>

#undef  YY_DECL
#define YY_DECL hoover_chess_utils::pgn_reader::PgnScannerToken hoover_chess_utils::pgn_reader::PgnScanner::yylexex()
%}

nag_number     0|[1-9][0-9]*
move_number    [1-9][0-9]*
pawn_char      [P]
piece_char     [KQRBN]
promo_char     [QRBN]
col_char       [a-h]
row_char       [1-8]
cm             [+#]?
non_blank_char [^ \t\v\r\n]
blank          [ \t\v]+
tagkey         [[:alnum:]_]+
string         \"(\\\\|\\\"|[^"\\\r\n])*\"
newline        \n\r|\r\n|\n|\r
bcomment_char  [^}\r\n]
bcomment_last  [^}\r\n\t ]
scomment_char  [^\r\n]
symtoken       [[:alnum:]][[:alnum:]_+#=:-]*

%%

%.*               // PGN escape data: ignore

"["               { BEGIN(PGNTAG); return PgnScannerToken::TAG_START; }
<PGNTAG>{tagkey}  { return PgnScannerToken::TAG_KEY; }
<PGNTAG>{string}  { return PgnScannerToken::TAG_VALUE; }
<PGNTAG>{newline} // ignore
<PGNTAG>"]"       { BEGIN(INITIAL); return PgnScannerToken::TAG_END; }

"("               { return PgnScannerToken::VARIATION_START; }
")"               { return PgnScannerToken::VARIATION_END; }

{move_number}"."       { setTokenInfo_MOVENUM(YYText(), YYText() + (YYLeng() - 1), Color::WHITE); return PgnScannerToken::MOVENUM; }
{move_number}"..."     { setTokenInfo_MOVENUM(YYText(), YYText() + (YYLeng() - 3), Color::BLACK); return PgnScannerToken::MOVENUM; }

{piece_char}{col_char}{row_char}{cm}      {
    setTokenInfo_PIECE_MOVE(
        SquareSet::all(),
        getPieceForChar(YYText()[0]),
        false,
        charCoordToSq(YYText()[1], YYText()[2]));
    return PgnScannerToken::MOVE_PIECE;
}

{piece_char}"x"{col_char}{row_char}{cm}      {
    setTokenInfo_PIECE_MOVE(
        SquareSet::all(),
        getPieceForChar(YYText()[0]),
        true,
        charCoordToSq(YYText()[2], YYText()[3]));
    return PgnScannerToken::MOVE_PIECE;
}

{piece_char}{col_char}?{row_char}?"x"?{col_char}{row_char}{cm}      {
    return tokenizePieceMove(std::string_view(YYText(), YYLeng()));
}

{col_char}x{col_char}{row_char}={promo_char}{cm} {
    setTokenInfo_PAWN_MOVE(
        colCharToMask(YYText()[0]),
        true,
        charCoordToSq(YYText()[2], YYText()[3]),
        getPieceForChar(YYText()[5]));
    return PgnScannerToken::MOVE_PAWN_PROMO_CAPTURE;
}

{col_char}x{col_char}{row_char}{cm} {
    setTokenInfo_PAWN_MOVE(
        colCharToMask(YYText()[0]),
        true,
        charCoordToSq(YYText()[2],
        YYText()[3]),
        Piece::NONE);
    return PgnScannerToken::MOVE_PAWN_CAPTURE;
}

{col_char}{row_char}={promo_char}{cm}  {
    setTokenInfo_PAWN_MOVE(
        SquareSet::all(),
        false,
        charCoordToSq(YYText()[0], YYText()[1]),
        getPieceForChar(YYText()[3]));
    return PgnScannerToken::MOVE_PAWN_PROMO;
}

{col_char}{row_char}{cm} {
    setTokenInfo_PAWN_MOVE(
        SquareSet::all(),
        false,
        charCoordToSq(YYText()[0], YYText()[1]),
        Piece::NONE);
    return PgnScannerToken::MOVE_PAWN;
}

{pawn_char}?{col_char}?{row_char}?"x"?{col_char}{row_char}("="{promo_char})?{cm}  {
    return tokenizeUnusualPawnMove(std::string_view(YYText(), YYLeng()));
}

"O-O"{cm}                             {
    return PgnScannerToken::MOVE_SHORT_CASTLE;
}

"O-O-O"{cm}                           {
    return PgnScannerToken::MOVE_LONG_CASTLE;
}


[!?]{3}        {
    // malformed move suffix
    BEGIN(INITIAL);
    setTokenInfo_ERROR("Bad move suffix");
    return PgnScannerToken::ERROR;
}

"!"            { setTokenInfo_NAG(1); return PgnScannerToken::NAG; }
"!!"           { setTokenInfo_NAG(3); return PgnScannerToken::NAG; }
"?"            { setTokenInfo_NAG(2); return PgnScannerToken::NAG; }
"??"           { setTokenInfo_NAG(4); return PgnScannerToken::NAG; }
"!?"           { setTokenInfo_NAG(5); return PgnScannerToken::NAG; }
"?!"           { setTokenInfo_NAG(6); return PgnScannerToken::NAG; }
"$"{nag_number}    {
    setTokenInfo_NAG(asciiToUnsigned<std::uint8_t>(YYText() + 1, YYText() + YYLeng(), "NAG"));
    return PgnScannerToken::NAG;
}


"{"{blank}?                               {
    BEGIN(PGNCOMMENT);
    return PgnScannerToken::COMMENT_START;
}

<PGNCOMMENT>{blank}?{newline}             {
    return PgnScannerToken::COMMENT_NEWLINE;
}

<PGNCOMMENT>{bcomment_char}*{bcomment_last} {
    return PgnScannerToken::COMMENT_TEXT;
}

<PGNCOMMENT>"}"                           {
    BEGIN(INITIAL);
    return PgnScannerToken::COMMENT_END;
}

";"{scomment_char}* { return PgnScannerToken::COMMENT_TEXT; }


"1-0"          { setTokenInfo_RESULT(PgnResult::WHITE_WIN); return PgnScannerToken::RESULT; }
"0-1"          { setTokenInfo_RESULT(PgnResult::BLACK_WIN); return PgnScannerToken::RESULT; }
"1/2-1/2"      { setTokenInfo_RESULT(PgnResult::DRAW); return PgnScannerToken::RESULT; }
"*"            { setTokenInfo_RESULT(PgnResult::UNKNOWN); return PgnScannerToken::RESULT; }

{newline}+     // nothing
<*>{blank}+    // nothing

<<EOF>>        return hoover_chess_utils::pgn_reader::PgnScannerToken::END_OF_FILE;

{symtoken}       {
    BEGIN(INITIAL);
    setTokenInfo_ERROR("Bad symbol token");
    return PgnScannerToken::ERROR;
}

<*>.|[\n\r]        {
    BEGIN(INITIAL);
    setTokenInfo_ERROR("Unexpected character");
    return PgnScannerToken::ERROR;
}

%%

void hoover_chess_utils::pgn_reader::PgnScanner::setTokenInfo_MOVENUM(const char *str, const char *end, Color color)
{
    const std::uint32_t moveNum { asciiToUnsigned<std::uint32_t>(str, end, "move number") };
    m_tokenInfo.moveNum = PgnScannerTokenInfo_MOVENUM { moveNum, color };
}

void hoover_chess_utils::pgn_reader::PgnScanner::setTokenInfo_PAWN_MOVE(
    SquareSet srcMask, bool capture, Square dstSq, Piece promoPiece)
{
    m_tokenInfo.pawnMove = PgnScannerTokenInfo_PAWN_MOVE { srcMask, capture, dstSq, promoPiece };
}

void hoover_chess_utils::pgn_reader::PgnScanner::setTokenInfo_PIECE_MOVE(
    SquareSet srcMask, Piece piece, bool capture, Square dstSq)
{
    m_tokenInfo.pieceMove = PgnScannerTokenInfo_PIECE_MOVE { srcMask, piece, capture, dstSq };
}

void hoover_chess_utils::pgn_reader::PgnScanner::setTokenInfo_NAG(std::uint8_t nag)
{
    m_tokenInfo.nag = PgnScannerTokenInfo_NAG { nag };
}

void hoover_chess_utils::pgn_reader::PgnScanner::setTokenInfo_RESULT(PgnResult result)
{
    m_tokenInfo.result = PgnScannerTokenInfo_RESULT { result };
}

void hoover_chess_utils::pgn_reader::PgnScanner::setTokenInfo_ERROR(const char *errorMessage)
{
    m_tokenInfo.error = PgnScannerTokenInfo_ERROR { errorMessage };
}

hoover_chess_utils::pgn_reader::PgnScannerToken
hoover_chess_utils::pgn_reader::PgnScanner::tokenizePieceMove(std::string_view str)
{
    // {piece_char}{col_char}?{row_char}?x?{col_char}{row_char}{cm}

    SquareSet srcMask { SquareSet::all() };
    char dstCol { };
    char dstRow { };
    bool capture { };

    auto i = str.begin();

    ++i;

    while (i != str.end())
    {
        const char c { *i };
        if (c >= 'a' && c <= 'h')
        {
            if (dstCol)
                srcMask &= colCharToMask(dstCol);

            dstCol = c;
        }
        else if (c >= '1' && c <= '8')
        {
            if (dstRow)
                srcMask &= rowCharToMask(dstRow);

            dstRow = c;
        }
        else if (c == 'x')
            capture = true;

        ++i;
    }

    setTokenInfo_PIECE_MOVE(srcMask, getPieceForChar(str[0]), capture, charCoordToSq(dstCol, dstRow));
    return PgnScannerToken::MOVE_PIECE;
}

hoover_chess_utils::pgn_reader::PgnScannerToken
hoover_chess_utils::pgn_reader::PgnScanner::tokenizeUnusualPawnMove(std::string_view str)
{
    // {pawn_char}?{col_char}?{row_char}?x?{col_char}{row_char}(={promo_char})?{cm}

    SquareSet srcMask { SquareSet::all() };
    char dstCol { };
    char dstRow { };
    bool capture { };

    auto i = str.begin();

    while (i != str.end())
    {
        const char c { *i };
        if (c >= 'a' && c <= 'h')
        {
            if (dstCol)
                srcMask &= colCharToMask(dstCol);

            dstCol = c;
        }
        else if (c >= '1' && c <= '8')
        {
            if (dstRow)
                srcMask &= rowCharToMask(dstRow);

            dstRow = c;
        }
        else if (c == 'x')
        {
            capture = true;
        }
        else if (c == '=')
        {
            setTokenInfo_PAWN_MOVE(
                srcMask,
                capture,
                charCoordToSq(dstCol, dstRow),
                getPieceForChar(i[1U]));
            return capture ? PgnScannerToken::MOVE_PAWN_PROMO_CAPTURE : PgnScannerToken::MOVE_PAWN_PROMO;
        }

        ++i;
    }

    setTokenInfo_PAWN_MOVE(
        srcMask,
        capture,
        charCoordToSq(dstCol, dstRow),
        Piece::NONE);

    return capture ? PgnScannerToken::MOVE_PAWN_CAPTURE : PgnScannerToken::MOVE_PAWN;
}
