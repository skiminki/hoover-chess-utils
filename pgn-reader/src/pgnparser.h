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


// Used PGN parsing rules are as follows:
//
// PGN                 = (COMMENT* GAME)* <end_of_file>
//
// GAME                = TAGPAIRS MOVETEXT
//
// TAGPAIRS            = (COMMENT* TAGPAIR)*
// TAGPAIR             = <tag_start> <tag_key> <tag_value> <tag_end>
//
// MOVETEXT            = LINE <result>
//
// LINE                = COMMENT* (MOVE_ITEM COMMENT* VARIATION* COMMENT*)*
//
// VARIATION           = <variation_start> LINE <variation_end>
//
// MOVE_ITEM           = (<movenum_white> | <movenum_black>)? MOVE <nag>*
//
// ; note: <move_*> terminals are generally in form:
// ; - piece
// ; - source square mask encoding the possible source squares
// ; - capture
// ; - destination square
// ; - promotion piece (Piece::NONE if not a pawn promotion)
// ; - check/mate mark (ignored)
// MOVE              = <move>
//                   | <move_short_castle>
//                   | <move_long_castle>
//
// COMMENT           = <comment_start> (<comment_text> | <comment_newline>)* <comment_end>
//                   | <comment_text>
//
// See pgnscannertokens.h for the used tokens (terminals). Tokens are here in
// lowercase in brackets, e.g., <end_of_file>.
//
// The rules are derived from the informal description in
// https://www.saremba.de/chessgml/standards/pgn/pgn-complete.htm .


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

// PGN parser actions are structured as follows:
//
// - PGN tags section
// - Move text section
// - Game result and game termination marker
class PgnParser_NullActions
{
public:
    //// 1. Preamble

    // game start, next state: 2. PGN tags section
    void gameStart() { }

    // End of PGN, no more actions from the parser
    void endOfPGN() { }


    //// 2. PGN tags section

    // new PGN tag
    void pgnTag(const std::string_view &key, const std::string_view &value)
    {
        static_cast<void>(key);
        static_cast<void>(value);
    }

    // end of PGN tags section, next state: 3. Move text section
    void moveTextSection() { }

    //// 3. Move text section

    // Comments can appear almost anywhere: start of game, after move, start of variation, etc
    void comment(const std::string_view &str)
    {
        static_cast<void>(str);
    }

    // numeric annotation glyph
    void nag(std::uint8_t nagNum)
    {
        static_cast<void>(nagNum);
    }

    void moveNum(std::uint32_t plyNum)
    {
        static_cast<void>(plyNum);
    }

    // moves
    void movePawn(SquareSet srcMask, Square dst)
    {
        static_cast<void>(srcMask);
        static_cast<void>(dst);
    }

    void movePawnCapture(SquareSet srcMask, Square dst)
    {
        static_cast<void>(srcMask);
        static_cast<void>(dst);
    }

    void movePawnPromo(SquareSet srcMask, Square dst, Piece promo)
    {
        static_cast<void>(srcMask);
        static_cast<void>(dst);
        static_cast<void>(promo);
    }

    void movePawnPromoCapture(SquareSet srcMask, Square dst, Piece promo)
    {
        static_cast<void>(srcMask);
        static_cast<void>(dst);
        static_cast<void>(promo);
    }

    void moveKnight(SquareSet srcMask, Square dst, bool capture)
    {
        static_cast<void>(srcMask);
        static_cast<void>(dst);
        static_cast<void>(capture);
    }

    void moveBishop(SquareSet srcMask, Square dst, bool capture)
    {
        static_cast<void>(srcMask);
        static_cast<void>(dst);
        static_cast<void>(capture);
    }

    void moveRook(SquareSet srcMask, Square dst, bool capture)
    {
        static_cast<void>(srcMask);
        static_cast<void>(dst);
        static_cast<void>(capture);
    }

    void moveQueen(SquareSet srcMask, Square dst, bool capture)
    {
        static_cast<void>(srcMask);
        static_cast<void>(dst);
        static_cast<void>(capture);
    }

    void moveKing(SquareSet srcMask, Square dst, bool capture)
    {
        static_cast<void>(srcMask);
        static_cast<void>(dst);
        static_cast<void>(capture);
    }

    void moveShortCastle()
    {
    }

    void moveLongCastle()
    {
    }

    void variationStart()
    {
    }

    void variationEnd()
    {
    }

    // Game termination, next state: 1. Preamble
    void gameTerminated(PgnResult result)
    {
        static_cast<void>(result);
    }
};

template <typename T_ActionHandler>
class PgnParser
{
private:
    PgnScanner &scanner;
    T_ActionHandler &actionHandler;
    StringBuilder strBuilder { };
    StringBuilder strBuilder2 { };

    bool inMoveTextSection { };
    std::vector<std::string> pendingComments { };

    void unexpectedTokenError(
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
            PgnErrorCode::UNEXPECTED_TOKEN,
            std::format(
                "Expected token {} but got: {} ({})",
                expectedTokens, PgnScanner::scannerTokenToString(token), static_cast<std::uint8_t>(token)));
    }

    void parseTagPair()
    {
        strBuilder.clear();
        strBuilder2.clear();
        PgnScannerToken token { scanner.nextToken() };
        if (token != PgnScannerToken::TAG_KEY)
            throw PgnError(
                PgnErrorCode::BAD_PGN_TAG,
                std::format("Expected token TAG_KEY but got: {} ({})",
                            static_cast<std::uint8_t>(token),
                            PgnScanner::scannerTokenToString(token)));

        strBuilder.appendString(scanner.YYText(), scanner.YYLeng());

        token = scanner.nextToken();
        if (token != PgnScannerToken::TAG_VALUE)
            throw PgnError(
                PgnErrorCode::BAD_PGN_TAG,
                std::format("Expected token TAG_VALUE but got: {} ({})",
                            static_cast<std::uint8_t>(token),
                            PgnScanner::scannerTokenToString(token)));

        // TAG value string has format "...", so we'll crop the first and the
        // last chars away. In addition, we need to parse escapes
        bool escape = false;
        for (const char c : std::string_view { scanner.YYText() + 1, static_cast<std::size_t>(scanner.YYLeng() - 2) })
        {
            if (escape)
            {
                strBuilder2.pushBack(c);
                escape = false;
            }
            else if (c != '\\')
            {
                strBuilder2.pushBack(c);
            }
            else
                escape = true;
        }

        actionHandler.pgnTag(
            strBuilder.getStringView(),
            strBuilder2.getStringView());

        token = scanner.nextToken();
        if (token != PgnScannerToken::TAG_END)
            throw PgnError(
                PgnErrorCode::BAD_PGN_TAG,
                std::format("Expected token TAG_END but got: {} ({})",
                            static_cast<std::uint8_t>(token),
                            PgnScanner::scannerTokenToString(token)));
    }


    static constexpr std::uint32_t ctLineTokenMask_moves {
        pgnScannerTokenToMaskBit(PgnScannerToken::MOVE_PAWN) |
        pgnScannerTokenToMaskBit(PgnScannerToken::MOVE_PAWN_CAPTURE) |
        pgnScannerTokenToMaskBit(PgnScannerToken::MOVE_PAWN_PROMO) |
        pgnScannerTokenToMaskBit(PgnScannerToken::MOVE_PAWN_PROMO_CAPTURE) |
        pgnScannerTokenToMaskBit(PgnScannerToken::MOVE_PIECE) |
        pgnScannerTokenToMaskBit(PgnScannerToken::MOVE_SHORT_CASTLE) |
        pgnScannerTokenToMaskBit(PgnScannerToken::MOVE_LONG_CASTLE)
    };

    static constexpr std::uint32_t ctLineTokenMask_allExceptVariations {
        ctLineTokenMask_moves |
        pgnScannerTokenToMaskBit(PgnScannerToken::MOVENUM) |
        pgnScannerTokenToMaskBit(PgnScannerToken::COMMENT_START) |
        pgnScannerTokenToMaskBit(PgnScannerToken::COMMENT_TEXT)
    };

    static constexpr std::uint32_t ctLineTokenMask_all {
        ctLineTokenMask_moves |
        ctLineTokenMask_allExceptVariations |
        pgnScannerTokenToMaskBit(PgnScannerToken::VARIATION_START)
    };

    PgnScannerToken parseLine(PgnScannerToken token)
    {

        // initially, we except all but variations. Variations are enabled after
        // a move is parsed.
        std::uint32_t validTokenMask { ctLineTokenMask_allExceptVariations };

        while (true)
        {
            if ((pgnScannerTokenToMaskBit(token) & validTokenMask) == 0U)
            {
                // An unallowed line token? If so, it is an error
                if ((pgnScannerTokenToMaskBit(token) & ctLineTokenMask_all) != 0U) [[unlikely]]
                    unexpectedTokenError(validTokenMask, token);
            }

            switch (token)
            {
                case PgnScannerToken::MOVENUM:
                    handleMoveNum(scanner.getTokenInfo().moveNum);
                    token = scanner.nextToken();

                    // after MOVENUM, only move tokens are allowed
                    validTokenMask = ctLineTokenMask_moves;
                    break;

                case PgnScannerToken::COMMENT_START:
                    parseCommentBlock();
                    token = scanner.nextToken();
                    break;

                case PgnScannerToken::COMMENT_TEXT:
                    parseSingleLineComment();
                    token = scanner.nextToken();
                    break;

                case PgnScannerToken::MOVE_PAWN:
                    handleMovePawn(scanner.getTokenInfo().pawnMove);
                    token = parseNagsAfterMove();
                    validTokenMask = ctLineTokenMask_all;
                    break;

                case PgnScannerToken::MOVE_PAWN_CAPTURE:
                    handleMovePawnCapture(scanner.getTokenInfo().pawnMove);
                    token = parseNagsAfterMove();
                    validTokenMask = ctLineTokenMask_all;
                    break;

                case PgnScannerToken::MOVE_PAWN_PROMO:
                    handleMovePawnPromo(scanner.getTokenInfo().pawnMove);
                    token = parseNagsAfterMove();
                    validTokenMask = ctLineTokenMask_all;
                    break;

                case PgnScannerToken::MOVE_PAWN_PROMO_CAPTURE:
                    handleMovePawnPromoCapture(scanner.getTokenInfo().pawnMove);
                    token = parseNagsAfterMove();
                    validTokenMask = ctLineTokenMask_all;
                    break;

                case PgnScannerToken::MOVE_PIECE:
                    switch (scanner.getTokenInfo().pieceMove.piece)
                    {
                        case Piece::KNIGHT:
                            handleMoveKnight(scanner.getTokenInfo().pieceMove);
                            break;

                        case Piece::BISHOP:
                            handleMoveBishop(scanner.getTokenInfo().pieceMove);
                            break;

                        case Piece::ROOK:
                            handleMoveRook(scanner.getTokenInfo().pieceMove);
                            break;

                        case Piece::QUEEN:
                            handleMoveQueen(scanner.getTokenInfo().pieceMove);
                            break;

                        default: // KING
                            handleMoveKing(scanner.getTokenInfo().pieceMove);
                            break;
                    }
                    token = parseNagsAfterMove();
                    validTokenMask = ctLineTokenMask_all;
                    break;

                case PgnScannerToken::MOVE_SHORT_CASTLE:
                    handleMoveShortCastle();
                    token = parseNagsAfterMove();
                    validTokenMask = ctLineTokenMask_all;
                    break;

                case PgnScannerToken::MOVE_LONG_CASTLE:
                    handleMoveLongCastle();
                    token = parseNagsAfterMove();
                    validTokenMask = ctLineTokenMask_all;
                    break;

                case PgnScannerToken::VARIATION_START:
                    token = parseVariation();
                    break;

                default:
                    return token;
            }
        }
    }

    PgnScannerToken parseVariation()
    {
        actionHandler.variationStart();

        PgnScannerToken token { scanner.nextToken() };
        token = parseLine(token);

        if (token != PgnScannerToken::VARIATION_END)
        {
            throw PgnError(
                PgnErrorCode::UNEXPECTED_TOKEN,
                std::format("Expected token VARIATION_END but got: {} ({})",
                            static_cast<std::uint8_t>(token),
                            PgnScanner::scannerTokenToString(token)));
        }

        actionHandler.variationEnd();

        return scanner.nextToken();
    }

    inline void handleMovePawn(const PgnScannerTokenInfo_PAWN_MOVE &move) const
    {
        actionHandler.movePawn(
            move.srcMask,
            move.dstSq);
    }

    inline void handleMovePawnCapture(const PgnScannerTokenInfo_PAWN_MOVE &move) const
    {
        actionHandler.movePawnCapture(
            move.srcMask,
            move.dstSq);
    }

    inline void handleMovePawnPromo(const PgnScannerTokenInfo_PAWN_MOVE &move) const
    {
        actionHandler.movePawnPromo(
            move.srcMask,
            move.dstSq,
            move.promoPiece);
    }

    inline void handleMovePawnPromoCapture(const PgnScannerTokenInfo_PAWN_MOVE &move) const
    {
        actionHandler.movePawnPromoCapture(
            move.srcMask,
            move.dstSq,
            move.promoPiece);
    }

    inline void handleMoveKnight(const PgnScannerTokenInfo_PIECE_MOVE &move) const
    {
        actionHandler.moveKnight(move.srcMask, move.dstSq, move.capture);
    }

    inline void handleMoveBishop(const PgnScannerTokenInfo_PIECE_MOVE &move) const
    {
        actionHandler.moveBishop(move.srcMask, move.dstSq, move.capture);
    }

    inline void handleMoveRook(const PgnScannerTokenInfo_PIECE_MOVE &move) const
    {
        actionHandler.moveRook(move.srcMask, move.dstSq, move.capture);
    }

    inline void handleMoveQueen(const PgnScannerTokenInfo_PIECE_MOVE &move) const
    {
        actionHandler.moveQueen(move.srcMask, move.dstSq, move.capture);
    }

    inline void handleMoveKing(const PgnScannerTokenInfo_PIECE_MOVE &move) const
    {
        actionHandler.moveKing(move.srcMask, move.dstSq, move.capture);
    }

    inline void handleMoveShortCastle() const
    {
        actionHandler.moveShortCastle();
    }

    inline void handleMoveLongCastle() const
    {
        actionHandler.moveLongCastle();
    }

    void handleMoveNum(const PgnScannerTokenInfo_MOVENUM &moveNum) const
    {
        actionHandler.moveNum(makePlyNum(moveNum.num, moveNum.color));
    }

    PgnScannerToken parseNagsAfterMove()
    {
        PgnScannerToken token { scanner.nextToken() };

        while (token == PgnScannerToken::NAG)
        {
            actionHandler.nag(scanner.getTokenInfo().nag.nag);
            token = scanner.nextToken();
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
                    token = scanner.nextToken();
                    break;

                case PgnScannerToken::COMMENT_TEXT:
                    parseSingleLineComment();
                    token = scanner.nextToken();
                    break;

                default:
                    return token;
            }
        }
    }

    void parseCommentBlock()
    {
        strBuilder.clear();

        std::size_t pendingNewlines { };

        while (true)
        {
            const PgnScannerToken token { scanner.nextToken() };
            switch (token)
            {
                case PgnScannerToken::COMMENT_TEXT:
                    if (!strBuilder.isEmpty())
                    {
                        while (pendingNewlines > 0U)
                        {
                            strBuilder.pushBack('\n');
                            --pendingNewlines;
                        }
                    }
                    pendingNewlines = 0U;
                    strBuilder.appendString(scanner.YYText(), scanner.YYLeng());
                    break;
                case PgnScannerToken::COMMENT_NEWLINE:
                    ++pendingNewlines;
                    break;
                case PgnScannerToken::COMMENT_END:
                    if (inMoveTextSection)
                        actionHandler.comment(strBuilder.getStringView());
                    else
                        pendingComments.push_back(std::string { strBuilder.getStringView() });

                    return;

                default:
                    throw PgnError(
                        PgnErrorCode::UNEXPECTED_TOKEN,
                        std::format("Expected token COMMENT_TEXT | COMMENT_NEWLINE | COMMENT_END but got: {} ({})",
                                    static_cast<std::uint8_t>(token),
                                    PgnScanner::scannerTokenToString(token)));
            }
        }
    }

    void parseSingleLineComment()
    {
        const char *commentStart { scanner.YYText() };
        const char *commentEnd { commentStart + static_cast<std::size_t>(scanner.YYLeng()) };

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
            if (inMoveTextSection)
                actionHandler.comment(std::string_view { commentStart, commentEnd });
            else
                pendingComments.push_back(std::string { commentStart, commentEnd });
        }
    }

    void flushPendingComments()
    {
        for (const auto &str : pendingComments)
            actionHandler.comment(str);

        pendingComments.clear();
    }

public:
    PgnParser(PgnScanner &_scanner, T_ActionHandler &_actionHandler) :
        scanner { _scanner },
        actionHandler { _actionHandler }
    {
    }

    void parse()
    {
        try
        {
            // every iteration parses a game
            while (true)
            {
                PgnScannerToken token { scanner.nextToken() };

                // PGN = (COMMENT | GAME)* <end_of_file>
                while (true)
                {
                    if (token == PgnScannerToken::END_OF_FILE)
                    {
                        flushPendingComments();
                        actionHandler.endOfPGN();
                        return;
                    }
                    else if (token == PgnScannerToken::COMMENT_START)
                        parseCommentBlock();
                    else if (token == PgnScannerToken::COMMENT_TEXT)
                        parseSingleLineComment();
                    else
                        break;

                    token = scanner.nextToken();
                }

                // GAME = TAGPAIRS MOVETEXT
                actionHandler.gameStart();

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

                    token = scanner.nextToken();
                }

                // MOVETEXT
                actionHandler.moveTextSection();
                flushPendingComments();
                inMoveTextSection = true;

                token = parseLine(token);

                if (token != PgnScannerToken::RESULT)
                {
                    throw PgnError(
                        PgnErrorCode::UNEXPECTED_TOKEN,
                        std::format("Expected token RESULT but got: {} ({})",
                                    static_cast<std::uint8_t>(token),
                                    PgnScanner::scannerTokenToString(token)));
                }

                actionHandler.gameTerminated(scanner.getTokenInfo().result.result);
                inMoveTextSection = false;
            }
        }
        catch (const PgnError &ex)
        {
            // add position info in the exception
            throw PgnError(scanner, ex);
        }
    }
};

}

#endif
