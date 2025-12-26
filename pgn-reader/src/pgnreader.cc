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

#include "pgnreader.h"

#include "chessboard.h"
#include "pgnparser.h"
#include "pgnreader-priv.h"
#include "pgnreader-string-utils.h"
#include "pgnscanner.h"

#include <array>
#include <cassert>
#include <cstdint>
#include <format>

namespace hoover_chess_utils::pgn_reader
{

namespace
{

/// @brief PGN reader parser actions
///
/// @tparam CompileTimeMinFilter     Actions that are guaranteed to be enabled
/// @tparam CompileTimeMaxFilter     Actions that can be enabled
template <typename CompileTimeMinFilter, typename CompileTimeMaxFilter>
class PgnReaderParserActions
{
private:
    // check that min caps are all included in the max caps
    static_assert(
        (CompileTimeMinFilter::getBitMask() & CompileTimeMaxFilter::getBitMask()) == CompileTimeMinFilter::getBitMask());

    static constexpr std::uint32_t ctMaxVariationLevel {
        CompileTimeMaxFilter::isEnabled(PgnReaderActionClass::Variation) ? 64U : 0U }; // includes the mainline (variation level 0)

    ChessBoard m_board { };
    ChessBoard m_prevBoard { };

    struct SaveState
    {
        ChessBoard m_board { };
        ChessBoard m_prevBoard { };
    };

    std::array<SaveState, ctMaxVariationLevel> m_variationParentStack { };
    std::uint32_t m_variationLevel { };

    PgnReaderActions &m_actions;
    PgnReaderActionFilter m_filter { };

    PgnScanner const &m_pgnScanner;


    template <PgnReaderActionClass action>
    inline bool isActionClassEnabled() noexcept
    {
        // guaranteed to be enabled?
        if constexpr (CompileTimeMinFilter::isEnabled(action))
            return true;

        // guaranteed not to be enabled
        if constexpr (!CompileTimeMaxFilter::isEnabled(action))
            return false;

        // not guaranteed but can be enabled
        return m_filter.isEnabled(action);
    }

    void badMove(
        PgnErrorCode pgnErrorCode,
        std::uint32_t plyNum,
        Piece piece,
        const SquareSet srcMask,
        Square dst,
        Piece promo,
        bool capture)
    {
        m_board = m_prevBoard;

        MiniString<2U> promoStr;
        if (promo != Piece::NONE)
        {
            promoStr.setLength(2U);
            promoStr[0U] = '=';
            promoStr[1U] = StringUtils::promoPieceChar(promo);
        }

        throw PgnError(
            pgnErrorCode,
            std::format("{} {}{}{}{}{}{}",
                        StringUtils::moveNumToString(moveNumOfPly(plyNum), colorOfPly(plyNum)).getStringView(),
                        StringUtils::pieceToSanStr(piece).getStringView(),
                        StringUtils::sourceMaskToString(srcMask).getStringView(),
                        capture ? std::string_view { "x" } : std::string_view { },
                        StringUtils::colChar(dst),
                        StringUtils::rowChar(dst),
                        promoStr.getStringView()));
    }

    void badCastlingMove(
        PgnErrorCode pgnErrorCode,
        std::uint32_t plyNum,
        bool shortCastle)
    {
        throw PgnError(
            pgnErrorCode,
            std::format("{} {}",
                        StringUtils::moveNumToString(moveNumOfPly(plyNum), colorOfPly(plyNum)).getStringView(),
                        shortCastle ? "O-O" : "O-O-O"));
    }

public:
    PgnReaderParserActions(PgnReaderActions &actions, PgnReaderActionFilter filter, const PgnScanner &pgnScanner) :
        m_actions { actions },
        m_filter { filter },
        m_pgnScanner { pgnScanner }
    {
        // nags require move reporting
        if (!m_filter.isEnabled(PgnReaderActionClass::Move))
        {
            m_filter.set(PgnReaderActionClass::NAG, false);
        }

        // sanity check that we support all enabled actions
        assert((filter.getBitMask() & CompileTimeMaxFilter::getBitMask()) == filter.getBitMask());

        if (isActionClassEnabled<PgnReaderActionClass::Move>())
        {
            m_actions.setBoardReferences(m_board, m_prevBoard);
        }
    }

    void gameStart()
    {
        m_board.loadStartPos();
        m_actions.gameStart();
    }

    void pgnTag(const std::string_view &key, const std::string_view &value)
    {
        if (isActionClassEnabled<PgnReaderActionClass::PgnTag>())
        {
            m_actions.pgnTag(key, value);
        }

        if (isActionClassEnabled<PgnReaderActionClass::Move>() && key == std::string_view { "FEN" })
        {
            m_board.loadFEN(value);
        }
    }

    void moveTextSection()
    {
        m_actions.moveTextSection();
    }

    void moveNum(std::uint32_t moveNum)
    {
        if (isActionClassEnabled<PgnReaderActionClass::Move>())
        {
            if (isActionClassEnabled<PgnReaderActionClass::Variation>() || m_variationLevel == 0U)
            {
                if (moveNumOfPly(m_board.getCurrentPlyNum()) != moveNum)
                {
                    throw PgnError(
                        PgnErrorCode::UNEXPECTED_MOVE_NUM,
                        std::format("Expected move {} but got {}",
                                    moveNumOfPly(m_board.getCurrentPlyNum()),
                                    moveNum));
                }
            }
        }
    }

    void moveValidationError(
        const Move illegalMove, const Piece piece, const SquareSet srcMask, Square dst,
        const Piece promo, const bool capture)
    {
        badMove(
            illegalMove == Move::illegalNoMove() ? PgnErrorCode::ILLEGAL_MOVE : PgnErrorCode::AMBIGUOUS_MOVE,
            m_board.getCurrentPlyNum(), piece, srcMask, dst, promo, capture);
    }

    void movePawn(const SquareSet srcMask, const Square dst)
    {
        if (isActionClassEnabled<PgnReaderActionClass::Move>())
        {
            if (isActionClassEnabled<PgnReaderActionClass::Variation>() || m_variationLevel == 0U)
            {
                m_prevBoard = m_board;

                const Move m { m_board.generateSingleMoveForPawnAndDestNoCapture(srcMask, dst) };
                if (!m.isIllegal()) [[likely]]
                {
                    m_board.doMove(m);
                    m_actions.afterMove(m);
                }
                else
                    moveValidationError(m, Piece::PAWN, srcMask, dst, Piece::NONE, false);
            }
        }
    }

    void movePawnCapture(const SquareSet srcMask, const Square dst)
    {
        if (isActionClassEnabled<PgnReaderActionClass::Move>())
        {
            if (isActionClassEnabled<PgnReaderActionClass::Variation>() || m_variationLevel == 0U)
            {
                m_prevBoard = m_board;

                const Move m { m_board.generateSingleMoveForPawnAndDestCapture(srcMask, dst) };
                if (!m.isIllegal()) [[likely]]
                {
                    m_board.doMove(m);
                    m_actions.afterMove(m);
                }
                else
                    moveValidationError(m, Piece::PAWN, srcMask, dst, Piece::NONE, true);
            }
        }
    }

    void movePawnPromo(const SquareSet srcMask, const Square dst, const Piece promo)
    {
        if (isActionClassEnabled<PgnReaderActionClass::Move>())
        {
            if (isActionClassEnabled<PgnReaderActionClass::Variation>() || m_variationLevel == 0U)
            {
                m_prevBoard = m_board;

                const Move m { m_board.generateSingleMoveForPawnAndDestPromoNoCapture(srcMask, dst, promo) };
                if (!m.isIllegal()) [[likely]]
                {
                    m_board.doMove(m);
                    m_actions.afterMove(m);
                }
                else
                    moveValidationError(m, Piece::PAWN, srcMask, dst, promo, false);
            }
        }
    }

    void movePawnPromoCapture(const SquareSet srcMask, const Square dst, const Piece promo)
    {
        if (isActionClassEnabled<PgnReaderActionClass::Move>())
        {
            if (isActionClassEnabled<PgnReaderActionClass::Variation>() || m_variationLevel == 0U)
            {
                m_prevBoard = m_board;

                const Move m { m_board.generateSingleMoveForPawnAndDestPromoCapture(srcMask, dst, promo) };
                if (!m.isIllegal()) [[likely]]
                {
                    m_board.doMove(m);
                    m_actions.afterMove(m);
                }
                else
                    moveValidationError(m, Piece::PAWN, srcMask, dst, promo, true);
            }
        }
    }

    template <Piece piece>
    inline void movePieceGen(const SquareSet srcMask, const Square dst, const bool capture)
    {
        if (isActionClassEnabled<PgnReaderActionClass::Move>())
        {
            if (isActionClassEnabled<PgnReaderActionClass::Variation>() || m_variationLevel == 0U)
            {
                m_prevBoard = m_board;

                Move m;

                if constexpr (piece == Piece::KNIGHT)
                    m = m_board.generateSingleMoveForKnightAndDest(srcMask, dst);
                else if constexpr (piece == Piece::BISHOP)
                    m = m_board.generateSingleMoveForBishopAndDest(srcMask, dst);
                else if constexpr (piece == Piece::ROOK)
                    m = m_board.generateSingleMoveForRookAndDest(srcMask, dst);
                else if constexpr (piece == Piece::QUEEN)
                    m = m_board.generateSingleMoveForQueenAndDest(srcMask, dst);
                else {
                    static_assert(piece == Piece::KING);
                    m = m_board.generateSingleMoveForKingAndDest(srcMask, dst);
                }

                if (!m.isIllegal()) [[likely]]
                {
                    if (m_board.getOccupancyMask().isMember(dst) != capture) [[unlikely]]
                    {
                        badMove(
                            PgnErrorCode::ILLEGAL_MOVE,
                            m_board.getCurrentPlyNum(), piece, srcMask, dst, Piece::NONE, capture);
                    }

                    m_board.doMove(m);
                    m_actions.afterMove(m);
                }
                else
                    moveValidationError(m, piece, srcMask, dst, Piece::NONE, capture);
            }
        }
    }

    void moveKnight(const SquareSet srcMask, const Square dst, const bool capture)
    {
        movePieceGen<Piece::KNIGHT>(srcMask, dst, capture);
    }

    void moveBishop(const SquareSet srcMask, const Square dst, const bool capture)
    {
        movePieceGen<Piece::BISHOP>(srcMask, dst, capture);
    }

    void moveRook(const SquareSet srcMask, const Square dst, const bool capture)
    {
        movePieceGen<Piece::ROOK>(srcMask, dst, capture);
    }

    void moveQueen(const SquareSet srcMask, const Square dst, const bool capture)
    {
        movePieceGen<Piece::QUEEN>(srcMask, dst, capture);
    }

    void moveKing(const SquareSet srcMask, const Square dst, const bool capture)
    {
        movePieceGen<Piece::KING>(srcMask, dst, capture);
    }

    void moveShortCastle()
    {
        if (isActionClassEnabled<PgnReaderActionClass::Move>())
        {
            if (isActionClassEnabled<PgnReaderActionClass::Variation>() || m_variationLevel == 0U)
            {
                m_prevBoard = m_board;

                const Move m { m_board.generateSingleMoveForShortCastling() };

                if (!m.isIllegal()) [[likely]]
                {
                    m_board.doMove(m);
                    m_actions.afterMove(m);
                }
                else
                {
                    badCastlingMove(PgnErrorCode::ILLEGAL_MOVE, m_board.getCurrentPlyNum(), true);
                }
            }
        }
    }

    void moveLongCastle()
    {
        if (isActionClassEnabled<PgnReaderActionClass::Move>())
        {
            if (isActionClassEnabled<PgnReaderActionClass::Variation>() || m_variationLevel == 0U)
            {
                m_prevBoard = m_board;

                const Move m { m_board.generateSingleMoveForLongCastling() };

                if (!m.isIllegal()) [[likely]]
                {
                    m_board.doMove(m);
                    m_actions.afterMove(m);
                }
                else
                {
                    badCastlingMove(PgnErrorCode::ILLEGAL_MOVE, m_board.getCurrentPlyNum(), false);
                }
            }
        }
    }

    void comment(const std::string_view &str)
    {
        if (isActionClassEnabled<PgnReaderActionClass::Comment>())
        {
            if (isActionClassEnabled<PgnReaderActionClass::Variation>() || m_variationLevel == 0U)
            {
                m_actions.comment(str);
            }
        }
    }

    void variationStart()
    {
        if (isActionClassEnabled<PgnReaderActionClass::Variation>())
        {
            m_variationParentStack[m_variationLevel].m_board = m_board;
            m_variationParentStack[m_variationLevel].m_prevBoard = m_prevBoard;
            ++m_variationLevel;
            m_board = m_prevBoard;

            m_actions.variationStart();
        }
        else
        {
            ++m_variationLevel;
        }
    }

    void variationEnd()
    {
        if (isActionClassEnabled<PgnReaderActionClass::Variation>())
        {
            m_actions.variationEnd();

            --m_variationLevel;
            m_board = m_variationParentStack[m_variationLevel].m_board;
            m_prevBoard = m_variationParentStack[m_variationLevel].m_prevBoard;
        }
        else
        {
            --m_variationLevel;
        }
    }

    void gameTerminated(PgnResult result)
    {
        m_actions.gameTerminated(result);
    }

    void endOfPGN()
    {
        m_actions.endOfPGN();
    }

    void nag(std::uint8_t nagNum)
    {
        if (isActionClassEnabled<PgnReaderActionClass::NAG>())
        {
            if (isActionClassEnabled<PgnReaderActionClass::Variation>() || m_variationLevel == 0U)
            {
                m_actions.nag(nagNum);
            }
        }
    }

};

void skipToNextGame(PgnScanner &pgnScanner)
{
    // skip tokens until we hit EOF or RESULT
    while ((pgnScanner.getCurrentToken() != PgnScannerToken::END_OF_FILE) &&
           (pgnScanner.getCurrentToken() != PgnScannerToken::RESULT))
        static_cast<void>(pgnScanner.nextTokenNoThrowOnErrorToken());
}

template <typename MinFilter, typename MaxFilter>
bool tryProcessPgn(std::string_view pgn, PgnReaderActions &actions, PgnReaderActionFilter filter)
{
    // does this parser expect actions that are not enabled?
    // - currently, our only parser supports disabling all actions, so just
    //   protect this assumption with an assert
    assert((filter.getBitMask() & MinFilter::getBitMask()) == MinFilter::getBitMask());

    // does this parser lack actions?
    if ((filter.getBitMask() & MaxFilter::getBitMask()) != filter.getBitMask())
        return false;

    PgnScanner pgnScanner { pgn.data(), pgn.size() };

    while (true)
    {
        try
        {
            using ParserActions = PgnReaderParserActions<MinFilter, MaxFilter>;
            ParserActions readerActions { actions, filter, pgnScanner };

            PgnParser<ParserActions> parser { pgnScanner, readerActions };
            parser.parse();

            // end of input
            return true;
        }
        catch (const PgnError &ex)
        {
            PgnErrorInfo errorInfo { };
            errorInfo.lineNumber = pgnScanner.lineno();

            const PgnReaderOnErrorAction onErrorAction { actions.onError(ex, errorInfo) };

            switch (onErrorAction)
            {
                case PgnReaderOnErrorAction::Abort:
                    throw;

                case PgnReaderOnErrorAction::ContinueFromNextGame:
                    skipToNextGame(pgnScanner);
                    if (pgnScanner.getCurrentToken() == PgnScannerToken::END_OF_FILE)
                        // end of input
                        return true;

                    break;

                default:
                    throw PgnError(
                        PgnErrorCode::INTERNAL_ERROR,
                        std::format("PgnReader::readFromMemory: Unsupported PgnReaderOnErrorAction {}",
                                    static_cast<unsigned>(onErrorAction)));
            }
        }
    }
}

}

void PgnReader::readFromMemory(std::string_view pgn, PgnReaderActions &actions, PgnReaderActionFilter filter)
{
    // select the appropriate compile-time parser+actions combo

    // Full bells and whistles, maximum flexibility. Seems to be the fastest approach, likely due
    // to cache locality.
    if (tryProcessPgn<
             PgnReaderActionCompileTimeFilter
             <
             >,
             PgnReaderActionCompileTimeFilter<
                 PgnReaderActionClass::PgnTag,
                 PgnReaderActionClass::Move,
                 PgnReaderActionClass::NAG,
                 PgnReaderActionClass::Variation,
                 PgnReaderActionClass::Comment
             > >(pgn, actions, filter))
        return;

    throw PgnError(
        PgnErrorCode::INTERNAL_ERROR,
        std::format("Could not find PGN parser for filter bits {}", filter.getBitMask()));
}

}
