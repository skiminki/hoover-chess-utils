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

#include "pgnreader-string-utils.h"
#include "pgnreader-error.h"

#include <format>
#include <tuple>

namespace hoover_chess_utils::pgn_reader
{

namespace {

// returns:
// - column disambiguation required
// - row disambiguation required
constexpr std::tuple<bool, bool> disambiguationTest(
    const Move m,
    const ShortMoveList &moves,
    std::size_t numMoves)
{
    if (numMoves <= 1U)
        return std::make_tuple(false, false);

    std::size_t numMatchesCol { };
    std::size_t numMatchesRow { };

    for (size_t i { }; i < numMoves; ++i)
    {
        const auto &moveCand = moves[i];

        // disambiguation by column
        numMatchesCol += (pgn_reader::columnOf(m.getSrc()) == pgn_reader::columnOf(moveCand.getSrc()));
        numMatchesRow += (pgn_reader::rowOf(m.getSrc()) == pgn_reader::rowOf(moveCand.getSrc()));
    }

    const bool multiColMatch { numMatchesCol != 1U };
    const bool multiRowMatch { numMatchesRow != 1U };

    // multiColMatch  multiRowMatch  result          Notes
    // -----------------------------------------------------------------
    // false          false          (true,  false)  In case there's a single col match, column always disambiguates
    // false          true           (true,  false)  Same as above
    // true           false          (false, true )  In case there's single row match and multiple col matches, row disambiguator is used
    // true           true           (true,  true )  In case of multiple col/row matches, both row/col are needed

    return std::make_tuple(
        !multiColMatch  || multiRowMatch,
        multiColMatch);
}

consteval auto initializePieceAndColorNames() noexcept
{
    std::array<std::array<char, 2U>, 15U> ret;

    for (std::size_t i { }; i < ret.size(); ++i)
    {
        ret[i][0U] = '?';
        ret[i][1U] = '?';
    }

    for (PieceAndColor pc :
             { PieceAndColor::WHITE_NONE, PieceAndColor::BLACK_NONE })
    {
        ret[static_cast<std::size_t>(pc)][0U] = ' ';
        ret[static_cast<std::size_t>(pc)][1U] = ' ';
    }

    for (std::pair<Piece, char> pcc :
             { std::pair<Piece, char> { Piece::PAWN,   'P' },
               std::pair<Piece, char> { Piece::KNIGHT, 'N' },
               std::pair<Piece, char> { Piece::BISHOP, 'B' },
               std::pair<Piece, char> { Piece::ROOK,   'R' },
               std::pair<Piece, char> { Piece::QUEEN,  'Q' },
               std::pair<Piece, char> { Piece::KING,   'K' }})
    {
        const PieceAndColor pcw { makePieceAndColor(pcc.first, Color::WHITE) };
        ret[static_cast<std::size_t>(pcw)][0U] = 'w';
        ret[static_cast<std::size_t>(pcw)][1U] = pcc.second;

        const PieceAndColor pcb { makePieceAndColor(pcc.first, Color::BLACK) };
        ret[static_cast<std::size_t>(pcb)][0U] = 'b';
        ret[static_cast<std::size_t>(pcb)][1U] = pcc.second;
    }

    return ret;
}

consteval auto initializeSquareNames() noexcept
{
    std::array<std::array<char, 2U>, 64U> ret;

    for (std::size_t i { }; i < 64U; ++i)
    {
        std::array<char, 2U> squareName {
            StringUtils::colChar(getSquareForIndex(i)),
            StringUtils::rowChar(getSquareForIndex(i)) };

        ret[i] = squareName;
    }

    return ret;
}

constexpr auto ctPieceAndColorNames { initializePieceAndColorNames() };
constexpr auto ctSquareNames { initializeSquareNames() };

std::array<std::string_view, 16U> ctMoveTypeAndPromotionNames {
    "REGULAR_PAWN_MOVE",
    "REGULAR_PAWN_CAPTURE",
    "REGULAR_KNIGHT_MOVE",
    "REGULAR_BISHOP_MOVE",
    "REGULAR_ROOK_MOVE",
    "REGULAR_QUEEN_MOVE",
    "REGULAR_KING_MOVE",
    "EN_PASSANT",
    "CASTLING_SHORT",
    "CASTLING_LONG",
    "PROMO_KNIGHT",
    "PROMO_BISHOP",
    "PROMO_ROOK",
    "PROMO_QUEEN",
    "??",
    "ILLEGAL",
};

}

MiniString<2U> StringUtils::sourceMaskToString(SquareSet srcMask) noexcept
{
    MiniString<2U> ret { MiniString_Uninitialized { } };

    // note: branches are roughly ordered in SAN disambiguation order, since that's likely to be optimal

    if (srcMask == SquareSet::all())
    {
        // 1st: no disambiguation
        ret.setLength(0U);
    }
    else if (srcMask.popcount() != 1U)
    {
        if ((srcMask & SquareSet::row(0U)).popcount() == 1U)
        {
            // 2nd: column
            ret.setLength(1U);
            ret[0U] = colChar(srcMask.firstSquare());
        }
        else
        {
            // 3rd: row
            ret.setLength(1U);
            ret[0U] = rowChar(srcMask.firstSquare());
        }
    }
    else
    {
        // 4th: both source col+row
        const Square sq { srcMask.firstSquare() };
        ret.setLength(2U);
        ret[0U] = colChar(sq);
        ret[1U] = rowChar(sq);
    }

    return ret;
}

MiniString<7U> StringUtils::moveToSanAndPlay(ChessBoard &board, Move move)
{
    MiniString<7U> ret { MiniString_Uninitialized() };
    std::size_t i { };

    const SquareSet srcBit { SquareSet::square(move.getSrc()) };

    ShortMoveList moves;
    std::size_t numMoves { };

    switch (move.getTypeAndPromotion())
    {
        case MoveTypeAndPromotion::REGULAR_PAWN_MOVE:
            numMoves = board.generateMovesForPawnAndDestNoCapture(moves, srcBit, move.getDst());
            ret[i++] = colChar(move.getDst());
            ret[i++] = rowChar(move.getDst());
            break;

        case MoveTypeAndPromotion::REGULAR_PAWN_CAPTURE:
        case MoveTypeAndPromotion::EN_PASSANT:
            ret[i++] = colChar(move.getSrc());
            ret[i++] = 'x';
            numMoves = board.generateMovesForPawnAndDestCapture(moves, srcBit, move.getDst());
            ret[i++] = colChar(move.getDst());
            ret[i++] = rowChar(move.getDst());
            break;

        case MoveTypeAndPromotion::REGULAR_KNIGHT_MOVE:
            ret[i++] = 'N';
            numMoves = board.generateMovesForKnightAndDest(moves, SquareSet::all(), move.getDst());
            goto disambiguation;

        case MoveTypeAndPromotion::REGULAR_BISHOP_MOVE:
            ret[i++] = 'B';
            numMoves = board.generateMovesForBishopAndDest(moves, SquareSet::all(), move.getDst());
            goto disambiguation;

        case MoveTypeAndPromotion::REGULAR_ROOK_MOVE:
            ret[i++] = 'R';
            numMoves = board.generateMovesForRookAndDest(moves, SquareSet::all(), move.getDst());
            goto disambiguation;

        case MoveTypeAndPromotion::REGULAR_QUEEN_MOVE:
            ret[i++] = 'Q';
            numMoves = board.generateMovesForQueenAndDest(moves, SquareSet::all(), move.getDst());

        disambiguation:
            {
                const auto [ needCol, needRow ] = disambiguationTest(move, moves, numMoves);

                // disambiguation needed
                if (needCol)
                {
                    // next: column is a disambiguator
                    ret[i++] = colChar(move.getSrc());
                }

                if (needRow)
                {
                    // next: row is a disambiguator
                    ret[i++] = rowChar(move.getSrc());
                }
            }

            // capture?
            if ((SquareSet::square(move.getDst()) & board.getOccupancyMask()) != SquareSet::none())
            {
                ret[i++] = 'x';
            }

            ret[i++] = colChar(move.getDst());
            ret[i++] = rowChar(move.getDst());
            break;

        case MoveTypeAndPromotion::REGULAR_KING_MOVE:
            ret[i++] = 'K';
            numMoves = board.generateMovesForKingAndDest(moves, srcBit, move.getDst());

            // capture?
            if ((SquareSet::square(move.getDst()) & board.getOccupancyMask()) != SquareSet::none())
            {
                ret[i++] = 'x';
            }

            ret[i++] = colChar(move.getDst());
            ret[i++] = rowChar(move.getDst());
            break;

        case MoveTypeAndPromotion::CASTLING_SHORT:
            ret[i++] = 'O';
            ret[i++] = '-';
            ret[i++] = 'O';
            numMoves = board.generateMovesForShortCastling(moves);
            break;

        case MoveTypeAndPromotion::CASTLING_LONG:
            ret[i++] = 'O';
            ret[i++] = '-';
            ret[i++] = 'O';
            ret[i++] = '-';
            ret[i++] = 'O';
            numMoves = board.generateMovesForLongCastling(moves);
            break;

        case MoveTypeAndPromotion::PROMO_KNIGHT:
        case MoveTypeAndPromotion::PROMO_BISHOP:
        case MoveTypeAndPromotion::PROMO_ROOK:
        case MoveTypeAndPromotion::PROMO_QUEEN:
        {
            // capture?
            if (columnOf(move.getSrc()) != columnOf(move.getDst()))
            {
                ret[i++] = colChar(move.getSrc());
                ret[i++] = 'x';
                numMoves = board.generateMovesForPawnAndDestPromoCapture(moves, srcBit, move.getDst(), move.getPromotionPiece());
            }
            else
            {
                numMoves = board.generateMovesForPawnAndDestPromoNoCapture(moves, srcBit, move.getDst(), move.getPromotionPiece());
            }

            ret[i++] = colChar(move.getDst());
            ret[i++] = rowChar(move.getDst());
            ret[i++] = '=';
            ret[i++] = promoPieceChar(move.getPromotionPiece());

            break;
        }

        default:
            throw std::logic_error(
                std::format(
                    "Bad move type and promotion: {}",
                    static_cast<std::uint8_t>(move.getTypeAndPromotion())));
    }

    // move legality check
    bool found { };
    for (std::size_t i { }; (!found) && i < numMoves; ++i)
    {
        found = (moves[i] == move);
    }

    if (!found)
    {
        throw PgnError(
            PgnErrorCode::ILLEGAL_MOVE,
            std::format(
                "{} {} --> {} (raw encoding: {:x})",
                moveTypeAndPromotionToString(move.getTypeAndPromotion()),
                squareToString(move.getSrc(), "??"),
                squareToString(move.getDst(), "??"),
                move.getEncodedValue()));
    }

    // now play it
    board.doMove(move);

    // are we in check?
    if (board.isInCheck())
    {
        // ok, full status resolution needed. Are we also in mate?
        if (board.determineStatus() == PositionStatus::MATE)
            ret[i++] = '#';
        else
            ret[i++] = '+';
    }

    // finally, set length
    ret.setLength(i);

    return ret;
}

std::string_view StringUtils::pieceAndColorToString(PieceAndColor pc) noexcept
{
    std::size_t i { static_cast<std::size_t>(pc) };

    if (i >= ctPieceAndColorNames.size()) [[unlikely]]
        i = 7U; // maps to '??'

    const auto &name { ctPieceAndColorNames[i] };
    return std::string_view { name.data(), name.size() };
}

std::string_view StringUtils::squareToString(Square sq, std::string_view emptySquareName) noexcept
{
    const std::size_t i { static_cast<std::size_t>(sq) };

    if (i < ctSquareNames.size()) [[likely]]
    {
        const auto &squareName { ctSquareNames[i] };
        return std::string_view { squareName.data(), squareName.size() };
    }
    else
    {
        return emptySquareName;
    }
}

std::string_view StringUtils::moveTypeAndPromotionToString(MoveTypeAndPromotion typeAndPromotion) noexcept
{
    std::size_t i { static_cast<std::size_t>(typeAndPromotion) };

    if (i >= ctMoveTypeAndPromotionNames.size()) [[unlikely]]
        i = 14U; // maps to "??"

    return ctMoveTypeAndPromotionNames[i];
}

namespace
{

constexpr std::array<char, 16U> ctBitPlanesToFenChar {
    'P', 'N', 'B', '?', 'R', '?', 'Q', '?',
    'K', '?', '?', '?', '?', '?', '?', '?',
};

template <bool shortCastling, bool black>
char getCastlingRightsChar(Square rookSq, SquareSet castlingRookCandidates)
{
    constexpr char caseFlipBit { static_cast<char>(black) * 32 };
    if constexpr (shortCastling)
    {
        if (castlingRookCandidates.lastSquare() == rookSq)
            return 'K' ^ caseFlipBit;
    }
    else
    {
        if (castlingRookCandidates.firstSquare() == rookSq)
            return 'Q' ^ caseFlipBit;
    }

    return columnOf(rookSq) + ('A' ^ caseFlipBit);
}

}

void StringUtils::boardToFEN(const ChessBoard &board, MiniString<96U> &fen) noexcept
{
    char *i { fen.data() };

    // pieces to FEN
    const SquareSet blackPieces { board.getBlackPieces() };
    std::uint8_t currentShift { 56U };

    while (true)
    {
        // make sure current shift is aligned at row beginning and in bounds
        assert((currentShift & 7U) == 0U);
        assert(currentShift <= 56U);

        // Remaining occupancy for the row + a terminator. Used to detect the
        // next occupied square and when the row is fully printed.
        SquareSet rowRemainingBits {
            ((board.getOccupancyMask() >> currentShift) & SquareSet { 0xFFU }) | SquareSet { 0x01'00U } };

        while (true)
        {
            // number of empty squares
            std::uint8_t numEmptySquares { static_cast<std::uint8_t>(rowRemainingBits.firstSquare()) };

            if (numEmptySquares > 0U)
            {
                *i++ = '0' + numEmptySquares;
                currentShift += numEmptySquares;
            }

            rowRemainingBits >>= numEmptySquares + 1U;
            if (rowRemainingBits == SquareSet::none())
                break;

            const std::size_t fenCharOffset =
                (((static_cast<std::uint64_t>(board.getKnights())          >> currentShift) & 1U))       |
                (((static_cast<std::uint64_t>(board.getBishopsAndQueens()) >> currentShift) & 1U) << 1U) |
                (((static_cast<std::uint64_t>(board.getRooksAndQueens())   >> currentShift) & 1U) << 2U) |
                (((static_cast<std::uint64_t>(board.getKings())            >> currentShift) & 1U) << 3U);

            char pieceChar { ctBitPlanesToFenChar[fenCharOffset] };

            // flip the case bit for black pieces
            pieceChar ^= ((static_cast<std::uint64_t>(blackPieces) >> currentShift) & 1U) << 5U;

            // store and next column
            *i++ = pieceChar;
            ++currentShift;
        }

        if (currentShift == 8U)
            break;

        currentShift -= 16U;
        *i++ = '/';
    }

    *i++ = ' ';
    *i++ = board.getTurn() == Color::WHITE ? 'w' : 'b';
    *i++ = ' ';

    {
        const char *const castlingRightsStart = i;
        const SquareSet rooks { board.getRooks() };

        Square sq { board.getWhiteShortCastleRook() };
        if (sq != Square::NONE)
            *i++ = getCastlingRightsChar<true,  false>(sq, rooks & SquareSet::row(0U));

        sq = board.getWhiteLongCastleRook();
        if (sq != Square::NONE)
            *i++ = getCastlingRightsChar<false, false>(sq, rooks & SquareSet::row(0U));

        sq = board.getBlackShortCastleRook();
        if (sq != Square::NONE)
            *i++ = getCastlingRightsChar<true,  true >(sq, rooks & SquareSet::row(7U));

        sq = board.getBlackLongCastleRook();
        if (sq != Square::NONE)
            *i++ = getCastlingRightsChar<false, true >(sq, rooks & SquareSet::row(7U));

        if (castlingRightsStart == i)
            *i++ = '-';
    }

    *i++ = ' ';

    if (board.canEpCapture())
    {
        *i++ = colChar(board.getEpSquare());
        *i++ = rowChar(board.getEpSquare());
    }
    else
        *i++ = '-';

    *i++ = ' ';
    i = genUnsignedToString<3U>(i, board.getHalfMoveClock());
    *i++ = ' ';
    i = genUnsignedToString<10U>(i, moveNumOfPly(board.getCurrentPlyNum()));

    fen.setLength(i - fen.data());
}

}
