// Hoover Chess Utilities / PGN reader
// Copyright (C) 2022-2025  Sami Kiminki
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#ifndef HOOVER_CHESS_UTILS__PGN_READER__PGNPARSER_H_INCLUDED
#define HOOVER_CHESS_UTILS__PGN_READER__PGNPARSER_H_INCLUDED


#include "chessboard.h"
#include "pgnreader-types.h"
#include "pgnscanner.h"
#include "stringbuilder.h"

#include <array>
#include <bit>
#include <cstring>
#include <format>
#include <iterator>
#include <memory>
#include <string>
#include <vector>

namespace hoover_chess_utils::pgn_reader
{

/// @addtogroup PgnReaderImpl
/// @{

/// @brief PGN parser null semantic actions. This is useful only for testing and documentation purposes.
///
/// The overall sequence of the callbacks is as follows:
/// -# @coderef{gameStart()} – invoked at the beginning of each game
/// -# @coderef{pgnTag()} – invoked once per PGN tag pair
///    - Any number of comments may precede a PGN tag pair
///    - Escapes of tag pair values are processed before the callback is invoked
/// -# @coderef{moveTextSection()} – after PGN tag section and before the first move
///    - Note that the any comments between the last PGN tag and the first move are deferred after this callback
/// -# Move numbers, moves, NAGs, comments, variations
///    - The move-related callbacks are: @coderef{moveNum()}, @coderef{movePawn()},
///      @coderef{movePawnCapture()}, @coderef{movePawnPromo()}, @coderef{movePawnPromoCapture()},
///      @coderef{moveKnight()}, @coderef{moveBishop()}, @coderef{moveRook()}, @coderef{moveQueen()},
///      @coderef{moveKing()}, @coderef{moveShortCastle()}, @coderef{moveLongCastle()}. There are
///      no ordering restrictions.
///    - Any number of NAGs may follow a move. One callback to @coderef{nag()} per NAG.
///    - A variation starts with @coderef{variationStart()} and ends with @coderef{variationEnd()}.
///      - Variations may be nested. The parser will ensure that @coderef{variationStart()}/@coderef{variationEnd()}
///        call pairs are balanced or an exception is thrown.
///      - The parent line must have at least one move before a variation can start. The variation represents
///        an alternative to that move. However, after the variation has ended, another variation can
///        begin immediately, representing another alternative to the parent line move.
///    - Single line comments and block comments are not distinguished.
///      - A block comment triggers a @coderef{comment()}. Newlines are replaced with a single '\\n'
///        regardless of whether they used '\\n', '\\r', '\\n\\r', or '\\r\\n'
///      - A non-empty single line comment triggers a @coderef{comment()}
///      - Escapes are not processed
///      - Comments may appear anywhere except between a move and its NAG(s).
/// -# @coderef{gameTerminated()} – game end with a result. No other callbacks related to the game
///    can occur.
/// -# @coderef{endOfPGN()} – invoked at the end of the PGN file
/// -# Possible trailing comments
class PgnParser_NullActions
{
public:
    //// 1. Preamble

    /// @brief Called on game start before anything else
    void gameStart() { }

    //// 2. PGN tags section

    /// @brief Called on PGN tag pair
    ///
    /// @param[in]  key      PGN tag key
    /// @param[in]  value    PGN tag value (escapes processed)
    void pgnTag(const std::string_view &key, const std::string_view &value)
    {
        static_cast<void>(key);
        static_cast<void>(value);
    }

    /// @brief Called between the end of the tag pair section and the beginning
    /// of the move text section.
    void moveTextSection() { }

    //// 3. Move text section

    /// @brief Called on comment
    ///
    /// @param[in] str    Comment string. May be multiline string for multiline block comments.
    ///
    /// @remark On ordering of comments and other callbacks:
    /// - PGN comments before the first PGN tag pair: callbacks
    ///   after @coderef{gameStart()}.
    /// - PGN comments between the last PGN tag pair and the first move: callbacks
    ///   after @coderef{moveTextSection()}.
    /// - PGN comments after game termination marker (game result):
    ///   - If the PGN input has a next game: after @coderef{gameStart()}
    ///   - Otherwise: just before @coderef{endOfPGN()}
    void comment(const std::string_view &str)
    {
        static_cast<void>(str);
    }

    /// @brief Called on numeric annotation glyph
    ///
    /// @param[in]  nagNum    Glyph number
    ///
    /// @sa https://www.saremba.de/chessgml/standards/pgn/pgn-complete.htm#c10
    void nag(std::uint8_t nagNum)
    {
        static_cast<void>(nagNum);
    }

    /// @brief Called on move number
    ///
    /// @param[in]  moveNum   Move number
    ///
    /// @remark There is no white/black indication in the PGN import format, as the
    /// periods after the move number are ignored.
    void moveNum(std::uint32_t moveNum)
    {
        static_cast<void>(moveNum);
    }

    /// @brief Pawn advancing move (non-promoting)
    ///
    /// @param[in]  srcMask     Set of allowed squares as specified by the move
    /// @param[in]  dst         Destination square
    ///
    /// @remark
    /// - When source rank and file are specified, @p srcMask is a single square (@coderef{SquareSet::square()}).
    /// - When source rank is specified, @p srcMask is all squares of the rank (@coderef{SquareSet::row()}).
    /// - When source file is specified, @p srcMask is all squares of the file (@coderef{SquareSet::column()}).
    /// - Otherwise, @p srcMask is all squares of the board (@coderef{SquareSet::all()}).
    void movePawn(SquareSet srcMask, Square dst)
    {
        static_cast<void>(srcMask);
        static_cast<void>(dst);
    }

    /// @brief Pawn capturing move (non-promoting)
    ///
    /// @param[in]  srcMask     Set of allowed squares as specified by the move
    /// @param[in]  dst         Destination square
    ///
    /// @remark
    /// - When source rank and file are specified, @p srcMask is a single square (@coderef{SquareSet::square()}).
    /// - When source rank is specified, @p srcMask is all squares of the rank (@coderef{SquareSet::row()}).
    /// - When source file is specified, @p srcMask is all squares of the file (@coderef{SquareSet::column()}).
    /// - Otherwise, @p srcMask is all squares of the board (@coderef{SquareSet::all()}).
    void movePawnCapture(SquareSet srcMask, Square dst)
    {
        static_cast<void>(srcMask);
        static_cast<void>(dst);
    }

    /// @brief Pawn advancing move (promoting)
    ///
    /// @param[in]  srcMask     Set of allowed squares as specified by the move
    /// @param[in]  dst         Destination square
    /// @param[in]  promo       Promotion piece
    ///
    /// @remark
    /// - When source rank and file are specified, @p srcMask is a single square (@coderef{SquareSet::square()}).
    /// - When source rank is specified, @p srcMask is all squares of the rank (@coderef{SquareSet::row()}).
    /// - When source file is specified, @p srcMask is all squares of the file (@coderef{SquareSet::column()}).
    /// - Otherwise, @p srcMask is all squares of the board (@coderef{SquareSet::all()}).
    void movePawnPromo(SquareSet srcMask, Square dst, Piece promo)
    {
        static_cast<void>(srcMask);
        static_cast<void>(dst);
        static_cast<void>(promo);
    }

    /// @brief Pawn capturing move (promoting)
    ///
    /// @param[in]  srcMask     Set of allowed squares as specified by the move
    /// @param[in]  dst         Destination square
    /// @param[in]  promo       Promotion piece
    ///
    /// @remark
    /// - When source rank and file are specified, @p srcMask is a single square (@coderef{SquareSet::square()}).
    /// - When source rank is specified, @p srcMask is all squares of the rank (@coderef{SquareSet::row()}).
    /// - When source file is specified, @p srcMask is all squares of the file (@coderef{SquareSet::column()}).
    /// - Otherwise, @p srcMask is all squares of the board (@coderef{SquareSet::all()}).
    void movePawnPromoCapture(SquareSet srcMask, Square dst, Piece promo)
    {
        static_cast<void>(srcMask);
        static_cast<void>(dst);
        static_cast<void>(promo);
    }

    /// @brief Knight move
    ///
    /// @param[in]  srcMask     Set of allowed squares as specified by the move
    /// @param[in]  dst         Destination square
    /// @param[in]  capture     Whether the move is a capture
    ///
    /// @remark
    /// - When source rank and file are specified, @p srcMask is a single square (@coderef{SquareSet::square()}).
    /// - When source rank is specified, @p srcMask is all squares of the rank (@coderef{SquareSet::row()}).
    /// - When source file is specified, @p srcMask is all squares of the file (@coderef{SquareSet::column()}).
    /// - Otherwise, @p srcMask is all squares of the board (@coderef{SquareSet::all()}).
    void moveKnight(SquareSet srcMask, Square dst, bool capture)
    {
        static_cast<void>(srcMask);
        static_cast<void>(dst);
        static_cast<void>(capture);
    }

    /// @brief Bishop move
    ///
    /// @param[in]  srcMask     Set of allowed squares as specified by the move
    /// @param[in]  dst         Destination square
    /// @param[in]  capture     Whether the move is a capture
    ///
    /// @remark
    /// - When source rank and file are specified, @p srcMask is a single square (@coderef{SquareSet::square()}).
    /// - When source rank is specified, @p srcMask is all squares of the rank (@coderef{SquareSet::row()}).
    /// - When source file is specified, @p srcMask is all squares of the file (@coderef{SquareSet::column()}).
    /// - Otherwise, @p srcMask is all squares of the board (@coderef{SquareSet::all()}).
    void moveBishop(SquareSet srcMask, Square dst, bool capture)
    {
        static_cast<void>(srcMask);
        static_cast<void>(dst);
        static_cast<void>(capture);
    }

    /// @brief Rook move
    ///
    /// @param[in]  srcMask     Set of allowed squares as specified by the move
    /// @param[in]  dst         Destination square
    /// @param[in]  capture     Whether the move is a capture
    ///
    /// @remark
    /// - When source rank and file are specified, @p srcMask is a single square (@coderef{SquareSet::square()}).
    /// - When source rank is specified, @p srcMask is all squares of the rank (@coderef{SquareSet::row()}).
    /// - When source file is specified, @p srcMask is all squares of the file (@coderef{SquareSet::column()}).
    /// - Otherwise, @p srcMask is all squares of the board (@coderef{SquareSet::all()}).
    void moveRook(SquareSet srcMask, Square dst, bool capture)
    {
        static_cast<void>(srcMask);
        static_cast<void>(dst);
        static_cast<void>(capture);
    }

    /// @brief Queen move
    ///
    /// @param[in]  srcMask     Set of allowed squares as specified by the move
    /// @param[in]  dst         Destination square
    /// @param[in]  capture     Whether the move is a capture
    ///
    /// @remark
    /// - When source rank and file are specified, @p srcMask is a single square (@coderef{SquareSet::square()}).
    /// - When source rank is specified, @p srcMask is all squares of the rank (@coderef{SquareSet::row()}).
    /// - When source file is specified, @p srcMask is all squares of the file (@coderef{SquareSet::column()}).
    /// - Otherwise, @p srcMask is all squares of the board (@coderef{SquareSet::all()}).
    void moveQueen(SquareSet srcMask, Square dst, bool capture)
    {
        static_cast<void>(srcMask);
        static_cast<void>(dst);
        static_cast<void>(capture);
    }

    /// @brief King move
    ///
    /// @param[in]  srcMask     Set of allowed squares as specified by the move
    /// @param[in]  dst         Destination square
    /// @param[in]  capture     Whether the move is a capture
    ///
    /// @remark
    /// - When source rank and file are specified, @p srcMask is a single square (@coderef{SquareSet::square()}).
    /// - When source rank is specified, @p srcMask is all squares of the rank (@coderef{SquareSet::row()}).
    /// - When source file is specified, @p srcMask is all squares of the file (@coderef{SquareSet::column()}).
    /// - Otherwise, @p srcMask is all squares of the board (@coderef{SquareSet::all()}).
    void moveKing(SquareSet srcMask, Square dst, bool capture)
    {
        static_cast<void>(srcMask);
        static_cast<void>(dst);
        static_cast<void>(capture);
    }

    /// @brief Short castling move
    void moveShortCastle()
    {
    }

    /// @brief Long castling move
    void moveLongCastle()
    {
    }

    /// @brief Beginning of a recursive annotation variation (RAV)
    void variationStart()
    {
    }

    /// @brief End of a recursive annotation variation (RAV)
    void variationEnd()
    {
    }

    /// @brief Called on game end.
    ///
    /// @param[in]  result     Result of the game
    void gameTerminated(PgnResult result)
    {
        static_cast<void>(result);
    }

    /// @brief Called at the end of the PGN after everything else.
    void endOfPGN() { }
};

/// @brief The PGN parser
///
/// @tparam T_ActionHandler     Semantic action handler. See @coderef{PgnParser_NullActions} for description.
///
/// <table><caption>Grammar for PGN parsing</caption>
/// <tr>
///   <th colspan="3">Rule</th>
///   <th>Parsing function</th>
///   <th>Remarks</th>
/// </tr>
/// <tr>
///   <td>PGN</td>
///   <td>&rArr;</td>
///   <td>GAME* COMMENT* @pgnscannertoken{END_OF_FILE}</td>
///   <td>@coderef{parse()}</td>
///   <td>Any number of games with possibly trailing comments.</td>
/// </tr>
/// <tr>
///   <td>GAME</td>
///   <td>&rArr;</td>
///   <td>TAGPAIRS MOVETEXT</td>
///   <td>@coderef{parse()}</td>
///   <td>A PGN game consists of a list of tag pairs followed by move text section.</td>
/// </tr>
/// <tr>
///   <td>TAGPAIRS</td>
///   <td>&rArr;</td>
///   <td>(COMMENT* TAGPAIR)*</td>
///   <td>@coderef{parse()}</td>
///   <td>List of PGN key/value tags.</td>
/// </tr>
/// <tr>
///   <td>TAGPAIR</td>
///   <td>&rArr;</td>
///   <td>@pgnscannertoken{TAG_START} @pgnscannertoken{TAG_KEY} @pgnscannertoken{TAG_VALUE} @pgnscannertoken{TAG_END}</td>
///   <td>@coderef{parseTagPair()}</td>
///   <td>A single PGN key/value tag.</td>
/// </tr>
/// <tr>
///   <td>MOVETEXT</td>
///   <td>&rArr;</td>
///   <td>LINE @pgnscannertoken{RESULT}</td>
///   <td>@coderef{parse()}</td>
///   <td>The move text section of a PGN game.</td>
/// </tr>
/// <tr>
///   <td>LINE</td>
///   <td>&rArr;</td>
///   <td>(COMMENT | @pgnscannertoken{MOVENUM})* (MOVE_ITEM (COMMENT | @pgnscannertoken{MOVENUM} | MOVE_ITEM | VARIATION)* )?</td>
///   <td>@coderef{parseLine()}</td>
///   <td>A line consists of any number of moves, interleaved with optional
///       comments, move numbers, and variations. A variation may not appear
///       before the first move. Move numbers are completely optional in
///       PGNs. When present, however, they are validated to be correct by
///       @coderef{PgnReader}.</td>
/// </tr>
/// <tr>
///   <td>VARIATION</td>
///   <td>&rArr;</td>
///   <td>@pgnscannertoken{VARIATION_START} LINE @pgnscannertoken{VARIATION_END}</td>
///   <td>@coderef{parseVariation()}</td>
///   <td>Variation is an alternative line to a move.</td>
/// </tr>
/// <tr>
///   <td>MOVE_ITEM</td>
///   <td>&rArr;</td>
///   <td>MOVE &lt;nag&gt;*</td>
///   <td>@coderef{parseLine()}<br>@coderef{parseNagsAfterMove()}</td>
///   <td>A move followed by any number of numeric annotation glyphs (NAGs)</td>
/// </tr>
/// <tr>
///   <td rowspan="11">MOVE</td>
///   <td rowspan="11">&rArr;</td>
///   <td>@coderef{PgnScannerToken::MOVE_PAWN} |</td>
///   <td rowspan="11">@coderef{parseLine()}</td>
///   <td>A pawn advancing move (non-promoting)</td>
/// </tr>
/// <tr>
///   <td>@coderef{PgnScannerToken::MOVE_PAWN_CAPTURE} |</td>
///   <td>A pawn capture move (non-promoting)</td>
/// </tr>
/// <tr>
///   <td>@coderef{PgnScannerToken::MOVE_PAWN_PROMO} |</td>
///   <td>A pawn advancing move (promoting)</td>
/// </tr>
/// <tr>
///   <td>@coderef{PgnScannerToken::MOVE_PAWN_PROMO_CAPTURE} |</td>
///   <td>A pawn capture move (promoting)</td>
/// </tr>
/// <tr>
///   <td>@coderef{PgnScannerToken::MOVE_PIECE_KNIGHT} |</td>
///   <td>Knight move</td>
/// </tr>
/// <tr>
///   <td>@coderef{PgnScannerToken::MOVE_PIECE_BISHOP} |</td>
///   <td>Bishop move</td>
/// </tr>
/// <tr>
///   <td>@coderef{PgnScannerToken::MOVE_PIECE_ROOK} |</td>
///   <td>Rook move</td>
/// </tr>
/// <tr>
///   <td>@coderef{PgnScannerToken::MOVE_PIECE_QUEEN} |</td>
///   <td>Queen move</td>
/// </tr>
/// <tr>
///   <td>@coderef{PgnScannerToken::MOVE_PIECE_KING} |</td>
///   <td>King move</td>
/// </tr>
/// <tr>
///   <td>@coderef{PgnScannerToken::MOVE_SHORT_CASTLE} |</td>
///   <td>Short castling move (O-O)</td>
/// </tr>
/// <tr>
///   <td>@coderef{PgnScannerToken::MOVE_LONG_CASTLE}</td>
///   <td>Long castling move (O-O-O)</td>
/// </tr>
/// <tr>
///   <td rowspan="2">COMMENT</td>
///   <td rowspan="2">&rArr;</td>
///   <td>@pgnscannertoken{COMMENT_START} (@pgnscannertoken{COMMENT_TEXT} | @pgnscannertoken{COMMENT_NEWLINE})* @pgnscannertoken{COMMENT_END}</td>
///   <td>@coderef{parseCommentBlock()}</td>
///   <td>PGN block comment.</td>
/// </tr>
/// <tr>
///   <td>@pgnscannertoken{COMMENT_TEXT}</td>
///   <td>@coderef{parseSingleLineComment()}</td>
///   <td>PGN single line comment.</td>
/// </tr>
/// </table>
///
/// Strings may use backslashes for quoting the next character as is. This is
/// useful only for quoting a quote (@c ") or a backslash (@c \\) character.
///
/// The rules are derived from the description in
/// https://www.saremba.de/chessgml/standards/pgn/pgn-complete.htm .
///
/// @remark The grammar used by this parser differs from the grammar in https://www.saremba.de/chessgml/standards/pgn/pgn-complete.htm#c18
/// in various ways:
/// - Comments are not included in the specification. However, they must be
///   handled by a concrete parser implementation.
/// - The syntax in specification allows variations without parent
///   moves. This implementation decided to disallow that in the parser rather
///   than in the later stages.
/// - The syntax in specification allows NAGs without the preceding token being
///   a move or another NAG. This implementation is stricter.
template <typename T_ActionHandler>
class PgnParser
{
private:
    PgnScanner &m_scanner;
    T_ActionHandler &m_actionHandler;
    StringBuilder m_strBuilder { };
    StringBuilder m_strBuilder2 { };

    bool m_inMoveTextSection { };
    std::vector<std::string> m_pendingComments { };

    void unexpectedTokenError(
        PgnErrorCode errorCode,
        std::uint32_t expectedTokenMask,
        PgnScannerToken token)
    {
        std::string expectedTokens { };

        while (expectedTokenMask != 0U)
        {
            if (!expectedTokens.empty())
                expectedTokens += " | ";

            expectedTokens += PgnScanner::scannerTokenToString(PgnScannerToken { static_cast<std::uint8_t>(std::countr_zero(expectedTokenMask)) });
            expectedTokenMask = expectedTokenMask & (expectedTokenMask - 1U);
        }

        throw PgnError(
            errorCode,
            std::format(
                "Expected token {} but got: {} ({})",
                expectedTokens, PgnScanner::scannerTokenToString(token), static_cast<std::uint8_t>(token)));
    }

    void parseTagPair()
    {
        m_strBuilder.clear();
        m_strBuilder2.clear();
        PgnScannerToken token { m_scanner.nextToken() };
        if (token != PgnScannerToken::TAG_KEY) [[unlikely]]
            unexpectedTokenError(
                PgnErrorCode::BAD_PGN_TAG,
                pgnScannerTokenToMaskBit(PgnScannerToken::TAG_KEY),
                token);

        m_strBuilder.appendString(m_scanner.YYText(), m_scanner.YYLeng());

        token = m_scanner.nextToken();
        if (token != PgnScannerToken::TAG_VALUE) [[unlikely]]
            unexpectedTokenError(
                PgnErrorCode::BAD_PGN_TAG,
                pgnScannerTokenToMaskBit(PgnScannerToken::TAG_VALUE),
                token);

        // TAG value string has format "...", so we'll crop the first and the
        // last chars away. In addition, we need to parse escapes
        bool escape = false;
        for (const char c : std::string_view { m_scanner.YYText() + 1, static_cast<std::size_t>(m_scanner.YYLeng() - 2) })
        {
            if (escape)
            {
                m_strBuilder2.pushBack(c);
                escape = false;
            }
            else if (c != '\\')
            {
                m_strBuilder2.pushBack(c);
            }
            else
                escape = true;
        }

        m_actionHandler.pgnTag(
            m_strBuilder.getStringView(),
            m_strBuilder2.getStringView());

        token = m_scanner.nextToken();
        if (token != PgnScannerToken::TAG_END) [[unlikely]]
            unexpectedTokenError(
                PgnErrorCode::BAD_PGN_TAG,
                pgnScannerTokenToMaskBit(PgnScannerToken::TAG_END),
                token);
    }

    PgnScannerToken parseLine(PgnScannerToken token)
    {
        bool variationAllowed { };

        while (true)
        {
            switch (token)
            {
                case PgnScannerToken::MOVENUM:
                    handleMoveNum(m_scanner.getTokenInfo().moveNum);
                    token = m_scanner.nextToken();
                    break;

                case PgnScannerToken::COMMENT_START:
                    parseCommentBlock();
                    token = m_scanner.nextToken();
                    break;

                case PgnScannerToken::COMMENT_TEXT:
                    parseSingleLineComment();
                    token = m_scanner.nextToken();
                    break;

                case PgnScannerToken::MOVE_PAWN:
                    handleMovePawn(m_scanner.getTokenInfo().pawnMove);
                    token = parseNagsAfterMove();
                    variationAllowed = true;
                    break;

                case PgnScannerToken::MOVE_PAWN_CAPTURE:
                    handleMovePawnCapture(m_scanner.getTokenInfo().pawnMove);
                    token = parseNagsAfterMove();
                    variationAllowed = true;
                    break;

                case PgnScannerToken::MOVE_PAWN_PROMO:
                    handleMovePawnPromo(m_scanner.getTokenInfo().pawnMove);
                    token = parseNagsAfterMove();
                    variationAllowed = true;
                    break;

                case PgnScannerToken::MOVE_PAWN_PROMO_CAPTURE:
                    handleMovePawnPromoCapture(m_scanner.getTokenInfo().pawnMove);
                    token = parseNagsAfterMove();
                    variationAllowed = true;
                    break;

                case PgnScannerToken::MOVE_PIECE_KNIGHT:
                    handleMoveKnight(m_scanner.getTokenInfo().pieceMove);
                    token = parseNagsAfterMove();
                    variationAllowed = true;
                    break;

                case PgnScannerToken::MOVE_PIECE_BISHOP:
                    handleMoveBishop(m_scanner.getTokenInfo().pieceMove);
                    token = parseNagsAfterMove();
                    variationAllowed = true;
                    break;

                case PgnScannerToken::MOVE_PIECE_ROOK:
                    handleMoveRook(m_scanner.getTokenInfo().pieceMove);
                    token = parseNagsAfterMove();
                    variationAllowed = true;
                    break;

                case PgnScannerToken::MOVE_PIECE_QUEEN:
                    handleMoveQueen(m_scanner.getTokenInfo().pieceMove);
                    token = parseNagsAfterMove();
                    variationAllowed = true;
                    break;

                case PgnScannerToken::MOVE_PIECE_KING:
                    handleMoveKing(m_scanner.getTokenInfo().pieceMove);
                    token = parseNagsAfterMove();
                    variationAllowed = true;
                    break;

                case PgnScannerToken::MOVE_SHORT_CASTLE:
                    handleMoveShortCastle();
                    token = parseNagsAfterMove();
                    variationAllowed = true;
                    break;

                case PgnScannerToken::MOVE_LONG_CASTLE:
                    handleMoveLongCastle();
                    token = parseNagsAfterMove();
                    variationAllowed = true;
                    break;

                case PgnScannerToken::VARIATION_START:
                    if (variationAllowed) [[likely]]
                    {
                        token = parseVariation();
                        break;
                    }
                    else [[unlikely]]
                    {
                        throw PgnError(
                            PgnErrorCode::UNEXPECTED_TOKEN,
                            std::format(
                                "VARIATION_START not allowed in a line before move"));
                    }

                default:
                    return token;
            }
        }
    }

    PgnScannerToken parseVariation()
    {
        m_actionHandler.variationStart();

        PgnScannerToken token { m_scanner.nextToken() };
        token = parseLine(token);

        if (token != PgnScannerToken::VARIATION_END) [[unlikely]]
        {
            unexpectedTokenError(
                PgnErrorCode::UNEXPECTED_TOKEN,
                pgnScannerTokenToMaskBit(PgnScannerToken::VARIATION_END),
                token);
        }

        m_actionHandler.variationEnd();

        return m_scanner.nextToken();
    }

    inline void handleMovePawn(const PgnScannerTokenInfo_PAWN_MOVE &move) const
    {
        m_actionHandler.movePawn(
            move.srcMask,
            move.dstSq);
    }

    inline void handleMovePawnCapture(const PgnScannerTokenInfo_PAWN_MOVE &move) const
    {
        m_actionHandler.movePawnCapture(
            move.srcMask,
            move.dstSq);
    }

    inline void handleMovePawnPromo(const PgnScannerTokenInfo_PAWN_MOVE &move) const
    {
        m_actionHandler.movePawnPromo(
            move.srcMask,
            move.dstSq,
            move.promoPiece);
    }

    inline void handleMovePawnPromoCapture(const PgnScannerTokenInfo_PAWN_MOVE &move) const
    {
        m_actionHandler.movePawnPromoCapture(
            move.srcMask,
            move.dstSq,
            move.promoPiece);
    }

    inline void handleMoveKnight(const PgnScannerTokenInfo_PIECE_MOVE &move) const
    {
        m_actionHandler.moveKnight(move.srcMask, move.dstSq, move.capture);
    }

    inline void handleMoveBishop(const PgnScannerTokenInfo_PIECE_MOVE &move) const
    {
        m_actionHandler.moveBishop(move.srcMask, move.dstSq, move.capture);
    }

    inline void handleMoveRook(const PgnScannerTokenInfo_PIECE_MOVE &move) const
    {
        m_actionHandler.moveRook(move.srcMask, move.dstSq, move.capture);
    }

    inline void handleMoveQueen(const PgnScannerTokenInfo_PIECE_MOVE &move) const
    {
        m_actionHandler.moveQueen(move.srcMask, move.dstSq, move.capture);
    }

    inline void handleMoveKing(const PgnScannerTokenInfo_PIECE_MOVE &move) const
    {
        m_actionHandler.moveKing(move.srcMask, move.dstSq, move.capture);
    }

    inline void handleMoveShortCastle() const
    {
        m_actionHandler.moveShortCastle();
    }

    inline void handleMoveLongCastle() const
    {
        m_actionHandler.moveLongCastle();
    }

    void handleMoveNum(const PgnScannerTokenInfo_MOVENUM &moveNum) const
    {
        m_actionHandler.moveNum(moveNum.num);
    }

    PgnScannerToken parseNagsAfterMove()
    {
        PgnScannerToken token { m_scanner.nextToken() };

        while (token == PgnScannerToken::NAG)
        {
            m_actionHandler.nag(m_scanner.getTokenInfo().nag.nag);
            token = m_scanner.nextToken();
        }

        return token;
    }

    PgnScannerToken parseComments(PgnScannerToken token)
    {
        while (true)
        {
            switch (token)
            {
                case PgnScannerToken::COMMENT_START:
                    parseCommentBlock();
                    token = m_scanner.nextToken();
                    break;

                case PgnScannerToken::COMMENT_TEXT:
                    parseSingleLineComment();
                    token = m_scanner.nextToken();
                    break;

                default:
                    return token;
            }
        }
    }

    void parseCommentBlock()
    {
        m_strBuilder.clear();

        std::size_t pendingNewlines { };

        while (true)
        {
            const PgnScannerToken token { m_scanner.nextToken() };
            switch (token)
            {
                case PgnScannerToken::COMMENT_TEXT:
                    if (!m_strBuilder.isEmpty())
                    {
                        while (pendingNewlines > 0U)
                        {
                            m_strBuilder.pushBack('\n');
                            --pendingNewlines;
                        }
                    }
                    pendingNewlines = 0U;
                    m_strBuilder.appendString(m_scanner.YYText(), m_scanner.YYLeng());
                    break;
                case PgnScannerToken::COMMENT_NEWLINE:
                    ++pendingNewlines;
                    break;
                case PgnScannerToken::COMMENT_END:
                    if (m_inMoveTextSection)
                        m_actionHandler.comment(m_strBuilder.getStringView());
                    else
                        m_pendingComments.push_back(std::string { m_strBuilder.getStringView() });

                    return;

                default: [[unlikely]]
                    unexpectedTokenError(
                        PgnErrorCode::UNEXPECTED_TOKEN,
                        pgnScannerTokenToMaskBit(PgnScannerToken::COMMENT_TEXT) |
                        pgnScannerTokenToMaskBit(PgnScannerToken::COMMENT_NEWLINE) |
                        pgnScannerTokenToMaskBit(PgnScannerToken::COMMENT_END),
                        token);
            }
        }
    }

    void parseSingleLineComment()
    {
        const char *commentStart { m_scanner.YYText() };
        const char *commentEnd { commentStart + static_cast<std::size_t>(m_scanner.YYLeng()) };

        ++commentStart; // eat the ';'

        // eat the white spaces from the beginning
        while (commentStart != commentEnd)
        {
            switch (*commentStart)
            {
                case ' ':
                case '\t':
                case '\v':
                    ++commentStart;
                    continue;

                default:
                    break;
            }
            break;
        }

        // eat the white spaces from the end
        while (commentStart != commentEnd)
        {
            switch (*(commentEnd - 1U))
            {
                case ' ':
                case '\t':
                case '\v':
                    --commentEnd;
                    continue;

                default:
                    break;
            }
            break;
        }

        if (commentStart != commentEnd)
        {
            if (m_inMoveTextSection)
                m_actionHandler.comment(std::string_view { commentStart, commentEnd });
            else
                m_pendingComments.push_back(std::string { commentStart, commentEnd });
        }
    }

    void flushPendingComments()
    {
        for (const auto &str : m_pendingComments)
            m_actionHandler.comment(str);

        m_pendingComments.clear();
    }

public:
    /// @brief Constructor
    ///
    /// @param[in]  scanner          PGN scanner
    /// @param[in]  actionHandler    Semantic action handler. See @coderef{PgnParser_NullActions} for description.
    PgnParser(PgnScanner &scanner, T_ActionHandler &actionHandler) :
        m_scanner { scanner },
        m_actionHandler { actionHandler }
    {
    }

    PgnParser(const PgnParser &) = delete;
    PgnParser(PgnParser &&) = delete;
    PgnParser &operator = (const PgnParser &) & = delete;
    PgnParser &operator = (PgnParser &&) & = delete;

    void parse()
    {
        try
        {
            // every full iteration parses a game
            while (true)
            {
                PgnScannerToken token { m_scanner.nextToken() };

                // PGN ==> GAME* COMMENT* <end_of_file>
                while (true)
                {
                    if (token == PgnScannerToken::END_OF_FILE)
                    {
                        flushPendingComments();
                        m_actionHandler.endOfPGN();
                        return;
                    }
                    else if (token == PgnScannerToken::COMMENT_START)
                        parseCommentBlock();
                    else if (token == PgnScannerToken::COMMENT_TEXT)
                        parseSingleLineComment();
                    else
                        break;

                    token = m_scanner.nextToken();
                }

                // GAME = TAGPAIRS MOVETEXT
                m_actionHandler.gameStart();

                // TAGPAIRS = (COMMENT | TAGPAIR)*
                while (true)
                {
                    if (token == PgnScannerToken::TAG_START)
                    {
                        flushPendingComments();
                        parseTagPair();
                    }
                    else if (token == PgnScannerToken::COMMENT_START)
                        parseCommentBlock();
                    else if (token == PgnScannerToken::COMMENT_TEXT)
                        parseSingleLineComment();
                    else
                        break;

                    token = m_scanner.nextToken();
                }

                // MOVETEXT
                m_actionHandler.moveTextSection();
                flushPendingComments();
                m_inMoveTextSection = true;

                token = parseLine(token);

                if (token != PgnScannerToken::RESULT) [[unlikely]]
                    unexpectedTokenError(
                        PgnErrorCode::UNEXPECTED_TOKEN,
                        pgnScannerTokenToMaskBit(PgnScannerToken::RESULT),
                        token);

                m_actionHandler.gameTerminated(m_scanner.getTokenInfo().result.result);
                m_inMoveTextSection = false;
            }
        }
        catch (const PgnError &ex)
        {
            // add position info in the exception
            throw PgnError(m_scanner, ex);
        }
    }
};

/// @}

}

#endif
