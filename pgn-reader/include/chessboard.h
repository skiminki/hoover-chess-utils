// Hoover Chess Utilities / PGN reader
// Copyright (C) 2024-2025  Sami Kiminki
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

#ifndef HOOVER_CHESS_UTILS__PGN_READER__CHESSBOARD_H_INCLUDED
#define HOOVER_CHESS_UTILS__PGN_READER__CHESSBOARD_H_INCLUDED

#include "pgnreader-types.h"
#include "chessboard-types.h"
#include "chessboard-types-squareset.h"

#include <array>
#include <cassert>
#include <cinttypes>
#include <type_traits>

namespace hoover_chess_utils::pgn_reader
{

// forward declarations
class ChessBoard;
struct MoveGenFunctions;
class MoveGenFunctionTables;


/// @ingroup PgnReaderAPI
/// @brief Board representation using an array.
///
/// The layout is as follows:
///
///     index = row * 8 + column
///
/// @sa @coderef{ChessBoard::setBoard()}
/// @sa @coderef{getIndexOfSquare()}
using ArrayBoard = std::array<PieceAndColor, 64U>;

/// @ingroup PgnReaderAPI
/// @brief Board representation using @coderef{SquareSet}s (bit boards)
///
/// The intersection of @coderef{pawns}, @coderef{knights}, @coderef{knights},
/// @coderef{knights}, @coderef{knights}, @coderef{knights} must be empty.
///
/// @sa @coderef{ChessBoard::setBoard()}
struct BitBoard
{
    /// @brief Pawn squares
    SquareSet pawns;

    /// @brief Knight squares
    SquareSet knights;

    /// @brief Bishop squares
    SquareSet bishops;

    /// @brief Rook squares
    SquareSet rooks;

    /// @brief Queen squares
    SquareSet queens;

    /// @brief King squares
    SquareSet kings;

    /// @brief Squares of white pieces. Square of black pieces are implied.
    SquareSet whitePieces;
};

/// @ingroup PgnReaderAPI
/// @brief Move type (4 bits, range: 0..15)
///
/// Unless otherwise stated, @coderef{Move::getSrc()} and @coderef{Move::getDst()}.
/// specify the source and destination squares of the move.
///
/// @sa @coderef{Move::getTypeAndPromotion()}
enum class MoveTypeAndPromotion : std::uint8_t
{
    /// @brief Regular non-capturing, non-promoting pawn move
    REGULAR_PAWN_MOVE    = 0U,

    /// @brief Pawn capture, except en passant
    REGULAR_PAWN_CAPTURE = 1U,

    /// @brief Knight move
    REGULAR_KNIGHT_MOVE  = 2U,

    /// @brief Bishop move
    REGULAR_BISHOP_MOVE  = 3U,

    /// @brief Rook move
    REGULAR_ROOK_MOVE    = 4U,

    /// @brief Queen move
    REGULAR_QUEEN_MOVE   = 5U,

    /// @brief King move (not castling)
    REGULAR_KING_MOVE    = 6U,

    /// @brief Pawn en-passant capture
    ///
    /// <dl>
    ///   <dt>Pawn source square</dt>
    ///   <dd>@coderef{Move::getSrc()}</dd>
    ///
    ///   <dt>Pawn destination square</dt>
    ///   <dd>@coderef{Move::getDst()}</dd>
    ///
    ///   <dt>En passant square</dt>
    ///   <dd><tt>@coderef{makeSquare}(@coderef{columnOf}(@coderef{Move::getDst()}), @coderef{rowOf}(@coderef{Move::getSrc()}))</tt></dd>
    /// </dl>
    EN_PASSANT           = 7U,

    /// @brief Short castling
    ///
    /// <dl>
    ///   <dt>King source square</dt>
    ///   <dd>@coderef{Move::getSrc()}</dd>
    ///
    ///   <dt>King destination square</dt>
    ///   <dd><tt>@coderef{makeSquare}(6U, @coderef{rowOf}(@coderef{Move::getSrc()}))</tt></dd>
    ///
    ///   <dt>Rook source square</dt>
    ///   <dd>@coderef{Move::getDst()}</dd>
    ///
    ///   <dt>Rook destination square</dt>
    ///   <dd><tt>@coderef{makeSquare}(5U, @coderef{rowOf}(@coderef{Move::getDst()}))</tt></dd>
    /// </dl>
    CASTLING_SHORT       = 8U,

    /// @brief Long castling
    ///
    /// <dl>
    ///   <dt>King source square</dt>
    ///   <dd>@coderef{Move::getSrc()}</dd>
    ///
    ///   <dt>King destination square</dt>
    ///   <dd><tt>@coderef{makeSquare}(2U, @coderef{rowOf}(@coderef{Move::getSrc()}))</tt></dd>
    ///
    ///   <dt>Rook source square</dt>
    ///   <dd>@coderef{Move::getDst()}</dd>
    ///
    ///   <dt>Rook destination square</dt>
    ///   <dd><tt>@coderef{makeSquare}(3U, @coderef{rowOf}(@coderef{Move::getDst()}))</tt></dd>
    /// </dl>
    CASTLING_LONG        = 9U,

    /// @brief Pawn promotion to knight
    PROMO_KNIGHT         = 10U,

    /// @brief Pawn promotion to bishop
    PROMO_BISHOP         = 11U,

    /// @brief Pawn promotion to rook
    PROMO_ROOK           = 12U,

    /// @brief Pawn promotion to queen
    PROMO_QUEEN          = 13U,

    /// @brief Illegal move type
    ///
    /// Represents an illegal move. Returned by single move generators in
    /// case the generated move count is not 1.
    ///
    /// @note The destination square for illegal moves must be
    /// @coderef{Square::H8} for fast implementation of
    /// @coderef{Move::isIllegal()}.
    ILLEGAL              = 15U
};

/// @ingroup PgnReaderAPI
/// @brief A legal move. **Important: see the note!**
///
/// @note Always use one of the move generators to construct a legal
/// move. @coderef{ChessBoard::doMove()} assumes that the move is legal and it does not
/// perform any legality checks of its own.
class Move
{
private:
    /// @brief Encoded move
    ///
    /// <table>
    /// <caption>Bitfield</caption>
    /// <tr>
    ///   <th>15</th><th>14</th><th>13</th><th>12</th><th>11</th><th>10</th><th>9</th><th>8</th>
    ///   <th>7</th><th>6</th><th>5</th><th>4</th><th>3</th><th>2</th><th>1</th><th>0</th>
    /// </tr>
    /// <tr>
    ///   <td colspan="6">Destination square</td>
    ///   <td colspan="4">Move type and promotion</td>
    ///   <td colspan="6">Source square</td>
    /// </tr>
    /// </table>
    std::uint16_t m_encoded { };

public:
    /// @brief Default constructor (null move)
    constexpr Move() noexcept = default;

    /// @brief Constructor
    ///
    /// @param[in] src           Move source square. Must be a valid square.
    /// @param[in] dst           Move destination square. Must be a valid square.
    /// @param[in] typeAndPromo  Move type and promotion piece
    ///
    /// See @coderef{MoveTypeAndPromotion} on how @c src and @c dst are used for
    /// en-passant and castling move types.
    ///
    /// @note See @coderef{MoveTypeAndPromotion::ILLEGAL} on constraints for
    /// illegal moves.
    constexpr Move(Square src, Square dst, MoveTypeAndPromotion typeAndPromo) noexcept :
        m_encoded(
            (static_cast<std::uint16_t>(src)) |
            (static_cast<std::uint16_t>(typeAndPromo) << 6) |
            (static_cast<std::uint16_t>(dst) << 10U)
            )
    {
        assert(isValidSquare(src));
        assert(isValidSquare(dst));
        assert((typeAndPromo <= MoveTypeAndPromotion::PROMO_QUEEN) ||
               (typeAndPromo == MoveTypeAndPromotion::ILLEGAL));
    }

    /// @brief Constructor (copy)
    constexpr Move(const Move &) noexcept = default;

    /// @brief Constructor (move)
    constexpr Move(Move &&) noexcept = default;

    /// @brief Assignment
    Move &operator = (const Move &) noexcept = default;

    /// @brief Move assignment
    Move &operator = (Move &&) noexcept = default;

    /// @brief Destructor
    ~Move() noexcept = default;

    /// @brief Returns move source square
    ///
    /// See @coderef{MoveTypeAndPromotion} on how @c src and @c dst are used for
    /// en-passant and castling move types.
    constexpr Square getSrc() const noexcept
    {
        return static_cast<Square>(m_encoded & 63U);
    }

    /// @brief Returns move destination square
    ///
    /// See @coderef{MoveTypeAndPromotion} on how @c src and @c dst are used for
    /// en-passant and castling move types.
    constexpr Square getDst() const noexcept
    {
        return static_cast<Square>(m_encoded >> 10U);
    }

    /// @brief Returns move type and promotion piece
    constexpr MoveTypeAndPromotion getTypeAndPromotion() const noexcept
    {
        return static_cast<MoveTypeAndPromotion>((m_encoded >> 6U) & 0xFU);
    }

    /// @brief Checks whether a move type is regular
    ///
    /// @return Whether the move type is a regular pawn/piece move
    ///
    /// Regular move types are the following:
    /// - @coderef{MoveTypeAndPromotion::REGULAR_PAWN_MOVE}
    /// - @coderef{MoveTypeAndPromotion::REGULAR_PAWN_CAPTURE}
    /// - @coderef{MoveTypeAndPromotion::REGULAR_KNIGHT_MOVE}
    /// - @coderef{MoveTypeAndPromotion::REGULAR_BISHOP_MOVE}
    /// - @coderef{MoveTypeAndPromotion::REGULAR_ROOK_MOVE}
    /// - @coderef{MoveTypeAndPromotion::REGULAR_QUEEN_MOVE}
    /// - @coderef{MoveTypeAndPromotion::REGULAR_KING_MOVE}
    constexpr bool isRegularMove() const noexcept
    {
        return getTypeAndPromotion() <= MoveTypeAndPromotion::REGULAR_KING_MOVE;
    }

    /// @brief Checks whether a move type is en-passant pawn capture
    ///
    /// @return Whether the move type is en-passant pawn capture
    ///
    /// En-passant pawn capture move type is the following;
    /// - @coderef{MoveTypeAndPromotion::EN_PASSANT}
    constexpr bool isEnPassantMove() const noexcept
    {
        return getTypeAndPromotion() == MoveTypeAndPromotion::EN_PASSANT;
    }

    /// @brief Checks whether a move type is a pawn promotion
    ///
    /// @return Whether the move type is a pawn promotion move
    ///
    /// Pawn promotion move types are the following:
    /// - @coderef{MoveTypeAndPromotion::PROMO_KNIGHT}
    /// - @coderef{MoveTypeAndPromotion::PROMO_BISHOP}
    /// - @coderef{MoveTypeAndPromotion::PROMO_ROOK}
    /// - @coderef{MoveTypeAndPromotion::PROMO_QUEEN}
    constexpr bool isPromotionMove() const noexcept
    {
        return
            getTypeAndPromotion() >= MoveTypeAndPromotion::PROMO_KNIGHT &&
            getTypeAndPromotion() <= MoveTypeAndPromotion::PROMO_QUEEN;
    }

    /// @brief Checks whether a move type is a castling move
    ///
    /// @return Whether the move type is a castling move
    ///
    /// Pawn promotion move types are the following:
    /// - @coderef{MoveTypeAndPromotion::CASTLING_SHORT}
    /// - @coderef{MoveTypeAndPromotion::CASTLING_LONG}
    constexpr bool isCastlingMove() const noexcept
    {
        return
            getTypeAndPromotion() >= MoveTypeAndPromotion::CASTLING_SHORT &&
            getTypeAndPromotion() <= MoveTypeAndPromotion::CASTLING_LONG;
    }

    /// @brief Returns promotion piece of a promotion move
    constexpr Piece getPromotionPiece() const noexcept
    {
        assert(isPromotionMove());

        static_assert(
            (static_cast<std::uint8_t>(MoveTypeAndPromotion::PROMO_KNIGHT) & 0x7U) ==
            static_cast<std::uint8_t>(Piece::KNIGHT));
        static_assert(
            (static_cast<std::uint8_t>(MoveTypeAndPromotion::PROMO_BISHOP) & 0x7U) ==
            static_cast<std::uint8_t>(Piece::BISHOP));
        static_assert(
            (static_cast<std::uint8_t>(MoveTypeAndPromotion::PROMO_ROOK)   & 0x7U) ==
            static_cast<std::uint8_t>(Piece::ROOK));
        static_assert(
            (static_cast<std::uint8_t>(MoveTypeAndPromotion::PROMO_QUEEN)  & 0x7U) ==
            static_cast<std::uint8_t>(Piece::QUEEN));

        return Piece(static_cast<std::uint8_t>(getTypeAndPromotion()) & 0x7U);
    }

    /// @brief Checks whether the move type is illegal
    ///
    /// @return Whether move is one of the illegal moves
    ///
    /// @note See @coderef{MoveTypeAndPromotion::ILLEGAL} on constraints for
    /// illegal moves.
    constexpr bool isIllegal() const noexcept
    {
        // this is slightly faster than extracting and comparing the
        // MoveTypeAndPromotion field
        return m_encoded >= 0xFFC0U;
    }

    /// @brief Returns raw encoded value. Usually only used in debugging.
    ///
    /// @return Encoded value of move
    constexpr std::uint16_t getEncodedValue() const noexcept
    {
        return m_encoded;
    }

    /// @brief Comparator (equality)
    ///
    /// @param[in] o     Another move
    /// @return Comparison result
    constexpr bool operator == (const Move &o) const noexcept
    {
        return m_encoded == o.m_encoded;
    }

    /// @brief Token for illegal move: no moves generated
    ///
    /// @return Illegal move token: no moves generated
    static constexpr Move illegalNoMove() noexcept
    {
        return Move { Square::A1, Square::H8, MoveTypeAndPromotion::ILLEGAL };
    }

    /// @brief Token for illegal move: ambiguous move generation
    ///
    /// @return Illegal move token: ambiguous move generation
    static constexpr Move illegalAmbiguousMove() noexcept
    {
        return Move { Square::A2, Square::H8, MoveTypeAndPromotion::ILLEGAL };
    }
};

/// @ingroup PgnReaderAPI
/// @brief Move list returned by @coderef{ChessBoard::generateMoves()}.
///
/// The move list is big enough to store all possible moves for any given position.
using MoveList = std::array<Move, 256U>;

/// @ingroup PgnReaderAPI
/// @brief Short move list returned by move generators for writing a SAN
/// move. That is, when the piece type and destination square are known, and
/// moves need to be generated to resolve the required disambiguation.
using ShortMoveList = std::array<Move, 8U>;

/// @ingroup PgnReaderImpl
/// @brief Move generator type
enum class MoveGenType : std::uint8_t
{
    /// @brief Move generator for when the king is not in check. All moves
    /// are considered.
    NO_CHECK = 0U,

    /// @brief Move generator for when the king is in check (single). King
    /// moves are considered first, castling moves are not considered at
    /// all, and regular piece moves must either capture the checker or
    /// intercept the check.
    CHECK,

    /// @brief Move generator for when the king is in double check. Only
    /// king moves are considered.
    DOUBLE_CHECK,
};


/// @ingroup PgnReaderAPI
/// @brief The chessboard
///
/// The following move generator variants are provided:
///
/// <table>
/// <tr>
///    <th>Family</th>
///    <th>Description</th>
///    <th>Functions</th>
/// </tr>
/// <tr>
///    <td>Full move list</td>
///    <td>These generators produce all legal moves for a position.</td>
///    <td>@coderef{generateMoves()}<br>
///        @coderef{getNumberOfLegalMoves()}</td>
/// </tr>
/// <tr>
///    <td>Piece/destination move list</td>
///    <td>These generators produce all legal moves for a specified piece and
///        piece destination square. This family is useful to resolve the
///        minimal SAN notation for a move, for instance.</td>
///    <td>@coderef{generateMovesForPawnAndDestNoCapture()}<br>
///        @coderef{generateMovesForPawnAndDestCapture()}<br>
///        @coderef{generateMovesForPawnAndDestPromoNoCapture()}<br>
///        @coderef{generateMovesForPawnAndDestPromoCapture()}<br>
///        @coderef{generateMovesForKnightAndDest()}<br>
///        @coderef{generateMovesForBishopAndDest()}<br>
///        @coderef{generateMovesForRookAndDest()}<br>
///        @coderef{generateMovesForQueenAndDest()}<br>
///        @coderef{generateMovesForKingAndDest()}<br>
///        @coderef{generateMovesForLongCastling()}<br>
///        @coderef{generateMovesForShortCastling()}</td>
/// </tr>
/// <tr>
///    <td>Piece/destination/source single move</td>
///    <td>These generators produce a single legal move for a piece, destination square,
///        and allowed source square set. This family is useful for playing out moves
///        in SAN notation in PGNs, for instance.</td>
///    <td>@coderef{generateSingleMoveForPawnAndDestNoCapture()}<br>
///        @coderef{generateSingleMoveForPawnAndDestCapture()}<br>
///        @coderef{generateSingleMoveForPawnAndDestPromoNoCapture()}<br>
///        @coderef{generateSingleMoveForPawnAndDestPromoCapture()}<br>
///        @coderef{generateSingleMoveForKnightAndDest()}<br>
///        @coderef{generateSingleMoveForBishopAndDest()}<br>
///        @coderef{generateSingleMoveForRookAndDest()}<br>
///        @coderef{generateSingleMoveForQueenAndDest()}<br>
///        @coderef{generateSingleMoveForKingAndDest()}<br>
///        @coderef{generateSingleMoveForLongCastling()}<br>
///        @coderef{generateSingleMoveForShortCastling()}</td>
/// </tr>
/// </table>
class ChessBoard final
{
public:

    /// @brief Constructor (default)
    ///
    /// The default constructor sets the board to the standard starting
    /// position.
    ChessBoard() noexcept;

    /// @brief Constructor (copy)
    ChessBoard(const ChessBoard &) noexcept = default;

    /// @brief Constructor (move)
    ChessBoard(ChessBoard &&) noexcept = default;

    /// @brief Assignment
    ChessBoard &operator = (const ChessBoard &) noexcept = default;

    /// @brief Move assignment
    ChessBoard &operator = (ChessBoard &&) noexcept = default;

    /// @brief Destructor
    ~ChessBoard() noexcept = default;

    /// @brief Sets the board from an array board (aka Mailbox)
    ///
    /// @param[in] board                 Array board containing pieces for squares
    /// @param[in] whiteLongCastleRook   White long castling rook or
    ///                                  @coderef{Square::NONE} if no long castling rights.
    /// @param[in] whiteShortCastleRook  White short castling rook or
    ///                                  @coderef{Square::NONE} if no short castling rights.
    /// @param[in] blackLongCastleRook   Black long castling rook or
    ///                                  @coderef{Square::NONE} if no long castling rights.
    /// @param[in] blackShortCastleRook  Black short castling rook or
    ///                                  @coderef{Square::NONE} if no short castling rights.
    /// @param[in] epSquare              En-passant square or @coderef{Square::NONE} if
    ///                                  en-passant capture is not possible.
    /// @param[in] halfMoveClock         Half-move clock
    /// @param[in] plyNum                Ply num. Use @coderef{makePlyNum()} if necessary.
    ///
    /// @throws PgnError(PgnErrorCode::BAD_FEN)  Invalid input parameter, e.g., bad Square value
    /// @throws PgnError(PgnErrorCode::BAD_FEN)  Inconsistent castling rights, e.g., castling rook
    ///                                          not found, king not on 1st/8th rank, castling rook
    ///                                          on the wrong side
    /// @throws PgnError(PgnErrorCode::BAD_FEN)  Pawns on 1st/8th rank
    /// @throws PgnError(PgnErrorCode::BAD_FEN)  EP square on wrong rank or not empty; no EP pawn to capture.
    /// @throws PgnError(PgnErrorCode::BAD_FEN)  Wrong number of kings
    /// @throws PgnError(PgnErrorCode::BAD_FEN)  Opponent king is in check
    /// @throws PgnError(PgnErrorCode::BAD_FEN)  Move number overflow (max: 99999)
    ///
    /// EP capture does not have to be a legal move as long as the EP square is
    /// empty and there is a pawn to capture. If case there is no legal EP
    /// capture moves (but the other requirements are met), this function resets
    /// the EP square.
    ///
    /// @note In case an exception is thrown, the board may be left in a bad
    /// state.
    ///
    /// @sa @coderef{loadFEN()}
    void setBoard(
        const ArrayBoard &board,
        Square whiteLongCastleRook, Square whiteShortCastleRook,
        Square blackLongCastleRook, Square blackShortCastleRook,
        Square epSquare,
        std::uint8_t halfMoveClock, std::uint32_t plyNum);

    /// @brief Sets the board from @coderef{SquareSet}s (bit board)
    ///
    /// @param[in] board                 Array board containing pieces for squares
    /// @param[in] whiteLongCastleRook   White long castling rook or
    ///                                  @coderef{Square::NONE} if no long castling rights.
    /// @param[in] whiteShortCastleRook  White short castling rook or
    ///                                  @coderef{Square::NONE} if no short castling rights.
    /// @param[in] blackLongCastleRook   Black long castling rook or
    ///                                  @coderef{Square::NONE} if no long castling rights.
    /// @param[in] blackShortCastleRook  Black short castling rook or
    ///                                  @coderef{Square::NONE} if no short castling rights.
    /// @param[in] epSquare              En-passant square or @coderef{Square::NONE} if
    ///                                  en-passant capture is not possible.
    /// @param[in] halfMoveClock         Half-move clock
    /// @param[in] plyNum                Ply num. Use @coderef{makePlyNum()} if necessary.
    ///
    /// @throws PgnError(PgnErrorCode::BAD_FEN)  Invalid input parameter, e.g., bad Square value
    /// @throws PgnError(PgnErrorCode::BAD_FEN)  Inconsistent castling rights, e.g., castling rook
    ///                                          not found, king not on 1st/8th rank, castling rook
    ///                                          on the wrong side
    /// @throws PgnError(PgnErrorCode::BAD_FEN)  Pawns on 1st/8th rank
    /// @throws PgnError(PgnErrorCode::BAD_FEN)  EP square on wrong rank or not empty; no EP pawn to capture.
    /// @throws PgnError(PgnErrorCode::BAD_FEN)  Wrong number of kings
    /// @throws PgnError(PgnErrorCode::BAD_FEN)  Opponent king is in check
    /// @throws PgnError(PgnErrorCode::BAD_FEN)  Move number overflow (max: 99999)
    /// @throws PgnError(PgnErrorCode::BAD_FEN)  More than one piece occupying a square (intersection of
    ///                                          per-piece @coderef{SquareSet}s is not empty)
    ///
    /// EP capture does not have to be a legal move as long as the EP square is
    /// empty and there is a pawn to capture.
    ///
    /// @note In case an exception is thrown, the board may be left in a bad
    /// state.
    ///
    /// @sa @coderef{loadFEN()}
    void setBoard(
        const BitBoard &board,
        Square whiteLongCastleRook, Square whiteShortCastleRook,
        Square blackLongCastleRook, Square blackShortCastleRook,
        Square epSquare,
        std::uint8_t halfMoveClock, std::uint32_t plyNum);

    /// @brief Sets the board from FEN
    ///
    /// @param[in]  fen    Position described in Forsyth–Edwards Notation (FEN)
    ///
    /// @throws PgnError(PgnErrorCode::BAD_FEN)  Malformed FEN (e.g., too many rows or columns;
    ///                                          bad character; bad castling indicator)
    /// @throws PgnError(PgnErrorCode::BAD_FEN)  Pawns on 1st/8th rank
    /// @throws PgnError(PgnErrorCode::BAD_FEN)  EP square on wrong rank or not empty; no EP pawn to capture.
    /// @throws PgnError(PgnErrorCode::BAD_FEN)  Wrong number of kings
    /// @throws PgnError(PgnErrorCode::BAD_FEN)  Opponent king is in check
    ///
    /// The castling indicator may be in FEN, Shredded-FEN, or X-FEN format.
    ///
    /// EP capture does not have to be a legal move as long as the EP square is
    /// empty and there is a pawn to capture. If case there is no legal EP
    /// capture moves (but the other requirements are met), this function resets
    /// the EP square.
    ///
    /// @note In case an exception is thrown, the board may be left in a bad
    /// state.
    ///
    /// @sa @coderef{setBoard()}
    /// @sa https://en.wikipedia.org/wiki/Forsyth%E2%80%93Edwards_Notation
    void loadFEN(std::string_view fen);

    /// @brief Determines the status of the position
    ///
    /// @return Status of the position
    ///
    /// This function determines whether there are any legal moves. Then, the
    /// status is returned as per the following table:
    ///
    /// <table>
    /// <tr>
    ///   <th>Legal moves</th>
    ///   <th>@coderef{getCheckers()}</th>
    ///   <th>Returned status</th>
    /// </tr>
    /// <tr>
    ///   <td>Yes</td>
    ///   <td>@coderef{SquareSet::none()}</td>
    ///   <td>@coderef{PositionStatus::NORMAL}</td>
    /// </tr>
    /// <tr>
    ///   <td>Yes</td>
    ///   <td>Other</td>
    ///   <td>@coderef{PositionStatus::CHECK}</td>
    /// </tr>
    /// <tr>
    ///   <td>No</td>
    ///   <td>@coderef{SquareSet::none()}</td>
    ///   <td>@coderef{PositionStatus::STALEMATE}</td>
    /// </tr>
    /// <tr>
    ///   <td>No</td>
    ///   <td>Other</td>
    ///   <td>@coderef{PositionStatus::MATE}</td>
    /// </tr>
    /// </table>
    inline PositionStatus determineStatus() const noexcept;

    /// @brief Loads the starting position
    ///
    /// Resets the board to the regular starting position. Resets also half move
    /// clock and ply count.
    ///
    /// This is equivalent to assigning a default-constructed @coderef{ChessBoard}.
    void loadStartPos() noexcept
    {
        *this = ChessBoard { };
    }

    /// @brief Prints the current board to stdout. This is intended only for
    /// debugging purposes.
    void printBoard() const;

    /// @brief Returns the piece on a specified square
    ///
    /// @param[in] sq     Square to query
    /// @return           Piece on @c sq or @coderef{Piece::NONE}.
    ///
    /// @note This function may be slower than expected, since it decodes the
    /// piece from bit masks.
    Piece getSquarePieceNoColor(Square sq) const noexcept;

    /// @brief Returns the piece and color on a specified square
    ///
    /// @param[in] sq     Square to query
    /// @return           Piece and color on @c sq or @coderef{PieceAndColor::NONE}.
    ///
    /// @note This function may be slower than expected, since it decodes the
    /// piece from bit masks.
    PieceAndColor getSquarePiece(Square sq) const noexcept;

    /// @brief Returns squares that are occupied by pieces (including pawns).
    ///
    /// @return Occupied squares
    inline SquareSet getOccupancyMask() const noexcept
    {
        return m_occupancyMask;
    }

    /// @brief Returns squares occupied by white pieces (including pawns).
    ///
    /// @return Occupied squares
    inline SquareSet getWhitePieces() const noexcept
    {
        const std::uint64_t flipBits { getTurn() == Color::WHITE ? UINT64_C(0) : ~UINT64_C(0) };

        return m_occupancyMask & (m_turnColorMask ^ SquareSet { flipBits });
    }

    /// @brief Returns squares occupied by white pieces (including pawns).
    ///
    /// @return Occupied squares
    inline SquareSet getBlackPieces() const noexcept
    {
        const std::uint64_t flipBits { getTurn() == Color::WHITE ? ~UINT64_C(0) : UINT64_C(0) };

        return m_occupancyMask & (m_turnColorMask ^ SquareSet { flipBits });
    }

    /// @brief Returns squares that are occupied by pieces (including pawns)
    /// that belong to the side to move.
    ///
    /// @return Occupied squares, pieces belonging to the side to move
    inline SquareSet getPiecesInTurn() const noexcept
    {
        return m_turnColorMask;
    }

    /// @brief Returns squares occupied by pawns
    ///
    /// @return Pawns
    inline SquareSet getPawns() const noexcept
    {
        return m_pawns;
    }

    /// @brief Returns squares occupied by knights
    ///
    /// @return Knights
    inline SquareSet getKnights() const noexcept
    {
        return m_knights;
    }

    /// @brief Returns squares occupied by bishops
    ///
    /// @return Bishops
    inline SquareSet getBishops() const noexcept
    {
        return m_bishops & ~m_rooks;
    }

    /// @brief Returns squares occupied by rooks
    ///
    /// @return Rooks
    inline SquareSet getRooks() const noexcept
    {
        return m_rooks & ~m_bishops;
    }

    /// @brief Returns squares occupied by queens
    ///
    /// @return Queens
    inline SquareSet getQueens() const noexcept
    {
        return m_rooks & m_bishops;
    }

    /// @brief Returns squares occupied by bishops and queens (diagonal sliders)
    ///
    /// @return Bishops and queens
    inline SquareSet getBishopsAndQueens() const noexcept
    {
        return m_bishops;
    }

    /// @brief Returns squares occupied by rooks and queens (horizontal/vertical sliders)
    ///
    /// @return Rooks and queens
    inline SquareSet getRooksAndQueens() const noexcept
    {
        return m_rooks;
    }

    /// @brief Returns squares occupied by kings
    ///
    /// @return Kings
    inline SquareSet getKings() const noexcept
    {
        return m_kings;
    }

    /// @brief Returns whether the king is in check
    ///
    /// @return Whether the king is in check
    inline bool isInCheck() const noexcept
    {
        return m_checkers != SquareSet::none();
    }

    /// @brief Returns squares occupied by checkers
    ///
    /// @return Checkers
    inline SquareSet getCheckers() const noexcept
    {
        return m_checkers;
    }

    /// @brief Returns squares occupied by absolutely pinned pieces
    ///
    /// @return Pinned pieces
    inline SquareSet getPinnedPieces() const noexcept
    {
        return m_pinnedPieces;
    }

    /// @brief Shorthand for
    /// <tt>@coderef{getCastlingRook}(@coderef{Color::WHITE}, @false)</tt>.
    ///
    /// @return Rook square if white can castle long; or @coderef{Square::NONE}
    inline Square getWhiteLongCastleRook() const noexcept
    {
        return getCastlingRook(Color::WHITE, false);
    }

    /// @brief Shorthand for
    /// <tt>@coderef{getCastlingRook}(@coderef{Color::WHITE}, @true)</tt>.
    ///
    /// @return Rook square if white can castle short; or @coderef{Square::NONE}
    inline Square getWhiteShortCastleRook() const noexcept
    {
        return getCastlingRook(Color::WHITE, true);
    }

    /// @brief Shorthand for
    /// <tt>@coderef{getCastlingRook}(@coderef{Color::BLACK}, @false)</tt>.
    ///
    /// @return Rook square if black can castle long; or @coderef{Square::NONE}
    inline Square getBlackLongCastleRook() const noexcept
    {
        return getCastlingRook(Color::BLACK, false);
    }

    /// @brief Shorthand for
    /// <tt>@coderef{getCastlingRook}(@coderef{Color::BLACK}, @true)</tt>.
    ///
    /// @return Rook square if black can castle short; or @coderef{Square::NONE}
    inline Square getBlackShortCastleRook() const noexcept
    {
        return getCastlingRook(Color::BLACK, true);
    }

    /// @brief Returns castling rook square if castling rights apply or
    /// @coderef{Square::NONE} if no castling rights.
    ///
    /// @param[in] c              Side to move
    /// @param[in] shortCastling  Whether short castling is in question
    /// @return                   Square of the rook or @coderef{Square::NONE}
    ///                           if no castling rights.
    inline Square getCastlingRook(Color c, bool shortCastling) const noexcept
    {
        return m_castlingRooks[getCastlingRookIndex(c, shortCastling)];
    }

    /// @brief Returns en-passant square
    ///
    /// @return En-passant square or @coderef{Square::NONE} is EP-capture is not
    /// legal.
    ///
    /// @remark When EP square is other than @coderef{Square::NONE}, at least
    /// one EP capture is guaranteed to be legal.
    ///
    /// @sa @coderef{canEpCapture()}
    inline Square getEpSquare() const noexcept
    {
        return m_epSquare;
    }

    /// @brief Current half-move clock
    ///
    /// Half-move clock is used to determine 50-move rule and similar draw
    /// termination rules.
    inline std::uint8_t getHalfMoveClock() const noexcept
    {
        return m_halfMoveClock;
    }

    /// @brief Full move counter in plies
    ///
    /// Ply number 0 means white's first move, and ply number 1 means black's first
    /// move.
    ///
    /// @sa @coderef{moveNumOfPly()}, @coderef{colorOfPly()}
    inline std::uint32_t getCurrentPlyNum() const noexcept
    {
        return m_plyNum;
    }

    /// @brief Returns side to move
    ///
    /// @return Side to move
    inline Color getTurn() const noexcept
    {
        return Color { static_cast<std::uint8_t>((m_plyNum & 1U) * 8U) };
    }

    /// @brief Generates a legal non-promoting, non-capturing pawn move with a known
    /// destination square.
    ///
    /// @param[in]  srcSqMask       Allowed source squares
    /// @param[in]  dst             Destination square
    /// @return                     Move. In case a move was not found or it was
    ///                             ambiguous, @coderef{Move::isIllegal()} returns @true.
    inline Move generateSingleMoveForPawnAndDestNoCapture(SquareSet srcSqMask, Square dst) const noexcept;

    /// @brief Generates a legal non-promoting, capturing pawn move with a known
    /// destination square.
    ///
    /// @param[in]  srcSqMask       Allowed source squares
    /// @param[in]  dst             Destination square
    /// @return                     Move. In case a move was not found or it was
    ///                             ambiguous, @coderef{Move::isIllegal()} returns @true.
    inline Move generateSingleMoveForPawnAndDestCapture(SquareSet srcSqMask, Square dst) const noexcept;

    /// @brief Generates a legal promoting, non-capturing pawn move with a known
    /// destination square.
    ///
    /// @param[in]  srcSqMask       Allowed source squares
    /// @param[in]  dst             Destination square
    /// @param[in]  promo           Promotion piece
    /// @return                     Move. In case a move was not found or it was
    ///                             ambiguous, @coderef{Move::isIllegal()} returns @true.
    inline Move generateSingleMoveForPawnAndDestPromoNoCapture(SquareSet srcSqMask, Square dst, Piece promo) const noexcept;

    /// @brief Generates a legal promoting, capturing pawn move with a known
    /// destination square.
    ///
    /// @param[in]  srcSqMask       Allowed source squares
    /// @param[in]  dst             Destination square
    /// @param[in]  promo           Promotion piece
    /// @return                     Move. In case a move was not found or it was
    ///                             ambiguous, @coderef{Move::isIllegal()} returns @true.
    inline Move generateSingleMoveForPawnAndDestPromoCapture(SquareSet srcSqMask, Square dst, Piece promo) const noexcept;

    /// @brief Generates a legal knight move with a known destination square. May be
    /// capturing.
    ///
    /// @param[in]  srcSqMask       Allowed source squares
    /// @param[in]  dst             Destination square
    /// @return                     Move. In case a move was not found or it was
    ///                             ambiguous, @coderef{Move::isIllegal()} returns @true.
    inline Move generateSingleMoveForKnightAndDest(SquareSet srcSqMask, Square dst) const noexcept;

    /// @brief Generates a legal bishop move with a known destination square. May be
    /// capturing.
    ///
    /// @param[in]  srcSqMask       Allowed source squares
    /// @param[in]  dst             Destination square
    /// @return                     Move. In case a move was not found or it was
    ///                             ambiguous, @coderef{Move::isIllegal()} returns @true.
    inline Move generateSingleMoveForBishopAndDest(SquareSet srcSqMask, Square dst) const noexcept;

    /// @brief Generates a legal rook move with a known destination square. May be
    /// capturing.
    ///
    /// @param[in]  srcSqMask       Allowed source squares
    /// @param[in]  dst             Destination square
    /// @return                     Move. In case a move was not found or it was
    ///                             ambiguous, @coderef{Move::isIllegal()} returns @true.
    inline Move generateSingleMoveForRookAndDest(SquareSet srcSqMask, Square dst) const noexcept;

    /// @brief Generates a legal queen move with a known destination square. May be
    /// capturing.
    ///
    /// @param[in]  srcSqMask       Allowed source squares
    /// @param[in]  dst             Destination square
    /// @return                     Move. In case a move was not found or it was
    ///                             ambiguous, @coderef{Move::isIllegal()} returns @true.
    inline Move generateSingleMoveForQueenAndDest(SquareSet srcSqMask, Square dst) const noexcept;

    /// @brief Generates a legal king move with a known destination square. May be
    /// capturing.
    ///
    /// @param[in]  srcSqMask       Allowed source squares
    /// @param[in]  dst             Destination square
    /// @return                     Move. In case a move was not found or it was
    ///                             ambiguous, @coderef{Move::isIllegal()} returns @true.
    inline Move generateSingleMoveForKingAndDest(SquareSet srcSqMask, Square dst) const noexcept;

    /// @brief Generates a legal short castling move.
    ///
    /// @return                     Move. In case a move was not found or it was
    ///                             ambiguous, @coderef{Move::isIllegal()} returns @true.
    inline Move generateSingleMoveForShortCastling() const noexcept;

    /// @brief Generates a legal long castling move.
    ///
    /// @return                     Move. In case a move was not found or it was
    ///                             ambiguous, @coderef{Move::isIllegal()} returns @true.
    inline Move generateSingleMoveForLongCastling() const noexcept;

    /// @brief Generates a list of legal non-promoting, non-capturing pawn moves with a known
    /// destination square.
    ///
    /// @param[out] moves           List of returned moves
    /// @param[in]  srcSqMask       Allowed source squares
    /// @param[in]  dst             Destination square
    /// @return                     Number of returned moves
    inline std::size_t generateMovesForPawnAndDestNoCapture(ShortMoveList &moves, SquareSet srcSqMask, Square dst) const noexcept;

    /// @brief Generates a list of legal non-promoting, capturing pawn moves with a known
    /// destination square.
    ///
    /// @param[out] moves           List of returned moves
    /// @param[in]  srcSqMask       Allowed source squares
    /// @param[in]  dst             Destination square
    /// @return                     Number of returned moves
    inline std::size_t generateMovesForPawnAndDestCapture(ShortMoveList &moves, SquareSet srcSqMask, Square dst) const noexcept;

    /// @brief Generates a list of legal promoting, non-capturing pawn moves with a known
    /// destination square.
    ///
    /// @param[out] moves           List of returned moves
    /// @param[in]  srcSqMask       Allowed source squares
    /// @param[in]  dst             Destination square
    /// @param[in]  promo           Promotion piece
    /// @return                     Number of returned moves
    inline std::size_t generateMovesForPawnAndDestPromoNoCapture(ShortMoveList &moves, SquareSet srcSqMask, Square dst, Piece promo) const noexcept;

    /// @brief Generates a list of legal promoting, capturing pawn moves with a known
    /// destination square.
    ///
    /// @param[out] moves           List of returned moves
    /// @param[in]  srcSqMask       Allowed source squares
    /// @param[in]  dst             Destination square
    /// @param[in]  promo           Promotion piece
    /// @return                     Number of returned moves
    inline std::size_t generateMovesForPawnAndDestPromoCapture(ShortMoveList &moves, SquareSet srcSqMask, Square dst, Piece promo) const noexcept;

    /// @brief Generates a list of legal knight moves with a known destination
    /// square. May be capturing.
    ///
    /// @param[out] moves           List of returned moves
    /// @param[in]  srcSqMask       Allowed source squares
    /// @param[in]  dst             Destination square
    /// @return                     Number of returned moves
    inline std::size_t generateMovesForKnightAndDest(ShortMoveList &moves, SquareSet srcSqMask, Square dst) const noexcept;

    /// @brief Generates a list of legal bishop moves with a known destination
    /// square. May be capturing.
    ///
    /// @param[out] moves           List of returned moves
    /// @param[in]  srcSqMask       Allowed source squares
    /// @param[in]  dst             Destination square
    /// @return                     Number of returned moves
    inline std::size_t generateMovesForBishopAndDest(ShortMoveList &moves, SquareSet srcSqMask, Square dst) const noexcept;

    /// @brief Generates a list of legal rook moves with a known destination
    /// square. May be capturing.
    ///
    /// @param[out] moves           List of returned moves
    /// @param[in]  srcSqMask       Allowed source squares
    /// @param[in]  dst             Destination square
    /// @return                     Number of returned moves
    inline std::size_t generateMovesForRookAndDest(ShortMoveList &moves, SquareSet srcSqMask, Square dst) const noexcept;

    /// @brief Generates a list of legal queen moves with a known destination
    /// square. May be capturing.
    ///
    /// @param[out] moves           List of returned moves
    /// @param[in]  srcSqMask       Allowed source squares
    /// @param[in]  dst             Destination square
    /// @return                     Number of returned moves
    inline std::size_t generateMovesForQueenAndDest(ShortMoveList &moves, SquareSet srcSqMask, Square dst) const noexcept;

    /// @brief Generates a list of legal king moves with a known destination
    /// square. May be capturing.
    ///
    /// @param[out] moves           List of returned moves
    /// @param[in]  srcSqMask       Allowed source squares
    /// @param[in]  dst             Destination square
    /// @return                     Number of returned moves
    inline std::size_t generateMovesForKingAndDest(ShortMoveList &moves, SquareSet srcSqMask, Square dst) const noexcept;

    /// @brief Generates the short castling move in a list, if legal.
    ///
    /// @param[out] moves           List of returned moves
    /// @return                     Number of returned moves
    inline std::size_t generateMovesForShortCastling(ShortMoveList &moves) const noexcept;

    /// @brief Generates the long castling move in a list, if legal.
    ///
    /// @param[out] moves           List of returned moves
    /// @return                     Number of returned moves
    inline std::size_t generateMovesForLongCastling(ShortMoveList &moves) const noexcept;

    /// @brief Validates en-passant capture
    ///
    /// @return Whether there is a legal en-passant capture move
    ///
    /// This function determines whether there is a legal en-passant capture
    /// move. This function is useful for generating FENs, position databases,
    /// and similar.
    bool canEpCapture() const noexcept
    {
        return m_epSquare != Square::NONE;
    }

    /// @brief Generates a list of all legal moves for the current position
    ///
    /// @param[in] moves  Caller-allocated move list
    /// @return           Number of moves added in the move list
    inline std::size_t generateMoves(MoveList &moves) const noexcept;

    /// @brief Returns the number of legal moves for the current position. This
    /// function is mostly useful for calculating the moves in perft leaf
    /// positions.
    ///
    /// @return           Number of legal moves
    ///
    /// @sa https://www.chessprogramming.org/Perft
    inline std::size_t getNumberOfLegalMoves() const noexcept;

    /// @brief Determines whether any legal moves as available in the current
    /// position.
    ///
    /// @return           Legal moves available.
    inline bool hasLegalMoves() const noexcept;

    /// @brief Applies a move on the current position. The move is assumed to be
    /// legal, and it must be from one of the generators. No legality checks are
    /// done by this function.
    ///
    /// @param[in] m    Move to apply
    ///
    /// This function does the following:
    /// - Updates bit boards, castling rights, and EP square@fullonly{ (@coderef{m_epSquare})} as per the move
    /// @fullonly{- Updates @coderef{m_kingSq}}
    /// - Updates half move clock @fullonly{(@coderef{m_halfMoveClock})} and ply number @fullonly{(@coderef{m_plyNum})}
    /// @fullonly{  - Increase of @coderef{m_plyNum} changes the side to move}
    /// @fullonly{- Swaps @coderef{m_kingSq} and @coderef{m_oppKingSq}}
    /// - Flips turn color mask @fullonly{(@coderef{m_turnColorMask})}@apionly{(see @coderef{getPiecesInTurn()})}
    /// - Recalculates checkers @fullonly{(@coderef{m_checkers})}@apionly{(see @coderef{getCheckers()})}
    void doMove(Move m) noexcept;

    /// @brief Comparison operator (equality)
    ///
    /// @param[in]  o    Another chessboard
    /// @return          Comparison result
    bool operator == (const ChessBoard &o) const noexcept;

private:
    /// @brief Bit board of occupied squares
    ///
    /// @sa @coderef{getOccupancyMask()}
    SquareSet m_occupancyMask { SquareSet::row(0U) | SquareSet::row(1U) | SquareSet::row(6U) | SquareSet::row(7U) };

    /// @brief Occupied squares that have the same color piece as @coderef{getTurn()}
    ///
    /// @sa @coderef{getPiecesInTurn()}, @coderef{getWhitePieces()}, @coderef{getBlackPieces()}
    SquareSet m_turnColorMask { SquareSet::row(0U) | SquareSet::row(1U) };

    /// @brief Pawns
    ///
    /// @sa @coderef{getPawns()}
    SquareSet m_pawns { SquareSet::row(1U) | SquareSet::row(6U) };

    /// @brief Knights
    ///
    /// @sa @coderef{getKnights()}
    SquareSet m_knights {
        SquareSet::square(Square::B1) | SquareSet::square(Square::G1) |
        SquareSet::square(Square::B8) | SquareSet::square(Square::G8) };

    /// @brief Kings
    ///
    /// @sa @coderef{getKings()}
    SquareSet m_kings { SquareSet::square(Square::E1) | SquareSet::square(Square::E8) };

    /// @brief Bishops and queens
    ///
    /// @sa @coderef{getBishopsAndQueens()}
    SquareSet m_bishops {
        SquareSet::square(Square::C1) | SquareSet::square(Square::F1) |
        SquareSet::square(Square::C8) | SquareSet::square(Square::F8) |
        SquareSet::square(Square::D1) | SquareSet::square(Square::D8) };

    /// @brief Rooks and queens
    ///
    /// @sa @coderef{getRooksAndQueens()}
    SquareSet m_rooks {
        SquareSet::square(Square::A1) | SquareSet::square(Square::H1) |
        SquareSet::square(Square::A8) | SquareSet::square(Square::H8) |
        SquareSet::square(Square::D1) | SquareSet::square(Square::D8) };

    /// @brief Squares where there is a checker
    ///
    /// @remark Set by @coderef{updateCheckersAndPins()}
    SquareSet m_checkers { };

    /// @brief Squares with pinned pieces
    ///
    /// @remark Set by @coderef{updateCheckersAndPins()}
    SquareSet m_pinnedPieces { };

    /// @brief Move generator functions for this position
    ///
    /// @sa @coderef{MoveGenType}
    const MoveGenFunctions *m_moveGenFns;

    /// @brief Current ply number
    ///
    /// @sa @coderef{getCurrentPlyNum()}
    std::uint32_t m_plyNum { };

    /// @brief Current half move clock
    ///
    /// @sa @coderef{getHalfMoveClock()}
    std::uint8_t m_halfMoveClock { };

    /// @brief King in turn
    ///
    /// @sa @coderef{m_kings}, @coderef{m_turnColorMask}
    Square m_kingSq { Square::E1 };

    /// @brief King in not turn
    ///
    /// @sa @coderef{m_kings}, @coderef{m_turnColorMask}
    Square m_oppKingSq { Square::E8 };

    /// @brief En-passant square
    ///
    /// @sa @coderef{getEpSquare()}
    Square m_epSquare { Square::NONE };

    /// @brief Castling rooks
    ///
    /// The castling rooks are in order:
    /// - white long castling rook
    /// - white short castling rook
    /// - black long castling rook
    /// - black short castling rook
    ///
    /// Rook square @coderef{Square::NONE} signifies no castling right.
    ///
    /// @sa @coderef{getCastlingRookIndex()}
    std::array<Square, 4U> m_castlingRooks { Square::A1, Square::H1, Square::A8, Square::H8 };

    /// @brief Determines checkers and pinned pieces.
    ///
    /// This function sets @coderef{m_checkers} and @coderef{m_pinnedPieces}.
    void updateCheckersAndPins() noexcept;

    /// @brief Validates the board for items that are common for both
    /// @coderef{setBoard()} and @coderef{loadFEN()} and sets @coderef{m_checkers}.
    ///
    /// @throws PgnError(PgnErrorCode::BAD_FEN)   Validation failed
    void validateBoard();

    /// @brief Returns the array index for a castling rook
    ///
    /// @param[in] c              Side
    /// @param[in] shortCastling  Whether short castling is in question
    /// @return                   Array index for accessing @coderef{m_castlingRooks}.
    ///
    /// @sa @coderef{getCastlingRook()}
    static constexpr inline std::size_t getCastlingRookIndex(Color c, bool shortCastling) noexcept
    {
        static_assert(static_cast<std::size_t>(Color::WHITE) == 0U);
        static_assert(static_cast<std::size_t>(Color::BLACK) == 8U);

        assert((c == Color::WHITE) || (c == Color::BLACK));
        [[assume((c == Color::WHITE) || (c == Color::BLACK))]];

        return (static_cast<std::size_t>(c) / 4U) + static_cast<std::size_t>(shortCastling);
    }

    /// @brief Sets/resets castling rights and the associated rook
    ///
    /// @param[in]  c              Side
    /// @param[in]  shortCastling  Whether short castling rights are set/reset
    /// @param[in]  sq             Castling rook or @coderef{Square::NONE} for no castling rights
    inline void setCastlingRook(Color c, bool shortCastling, Square sq) noexcept
    {
        m_castlingRooks[getCastlingRookIndex(c, shortCastling)] = sq;
    }

    /// @brief Gets a reference to the castling rights rook for direct read/write access
    ///
    /// @param[in]  c              Side
    /// @param[in]  shortCastling  Whether short castling rights are accessed
    /// @return                    Square for the castling rook
    inline Square &getCastlingRookRef(Color c, bool shortCastling) noexcept
    {
        return m_castlingRooks[getCastlingRookIndex(c, shortCastling)];
    }

    static inline constexpr MoveTypeAndPromotion pieceToTypeAndPromotion(Piece promotion) noexcept
    {
        static_assert(
            (static_cast<unsigned>(Piece::KNIGHT) | 0x08U) == static_cast<unsigned>(MoveTypeAndPromotion::PROMO_KNIGHT));
        static_assert(
            (static_cast<unsigned>(Piece::BISHOP) | 0x08U) == static_cast<unsigned>(MoveTypeAndPromotion::PROMO_BISHOP));
        static_assert(
            (static_cast<unsigned>(Piece::ROOK)   | 0x08U) == static_cast<unsigned>(MoveTypeAndPromotion::PROMO_ROOK));
        static_assert(
            (static_cast<unsigned>(Piece::QUEEN)  | 0x08U) == static_cast<unsigned>(MoveTypeAndPromotion::PROMO_QUEEN));

        assert(promotion >= Piece::KNIGHT && promotion <= Piece::QUEEN);
        [[assume(promotion >= Piece::KNIGHT && promotion <= Piece::QUEEN)]];

        return static_cast<MoveTypeAndPromotion>(static_cast<uint16_t>(promotion) | 0x08U);
    }

    static constexpr inline std::uint8_t getKingColumnAfterCastling(MoveTypeAndPromotion typeAndPromo) noexcept
    {
        static_assert((static_cast<std::uint8_t>(MoveTypeAndPromotion::CASTLING_SHORT) & 1U) == 0U);
        static_assert((static_cast<std::uint8_t>(MoveTypeAndPromotion::CASTLING_LONG) & 1U) == 1U);

        return 6U - (static_cast<std::uint8_t>(typeAndPromo) & 1U) * 4U;
    }

    static constexpr inline std::uint8_t getRookColumnAfterCastling(MoveTypeAndPromotion typeAndPromo) noexcept
    {
        static_assert((static_cast<std::uint8_t>(MoveTypeAndPromotion::CASTLING_SHORT) & 1U) == 0U);
        static_assert((static_cast<std::uint8_t>(MoveTypeAndPromotion::CASTLING_LONG) & 1U) == 1U);

        return 5U - (static_cast<std::uint8_t>(typeAndPromo) & 1U) * 2U;
    }

    // note: assumes that the move is legal
    static constexpr inline bool isPawnDoubleSquareMove(Square src, Square dst) noexcept
    {
        const std::uint8_t srcNum { static_cast<std::uint8_t>(src) };
        const std::uint8_t dstNum { static_cast<std::uint8_t>(dst) };

        return ((srcNum - dstNum) & 15U) == 0U;
    }

    template <MoveGenType type>
    inline SquareSet blocksAllChecksMaskTempl(Square dst) const noexcept;

    /// @brief Generates legal pawn moves, the actual implementation.
    ///
    /// @tparam     IteratorType       Move list iterator type
    /// @tparam     type               Move generator type
    /// @tparam     legalDestinations  Parameter type for legal destinations
    /// @tparam     turn               Side to move
    /// @param[in]  i                  Move list iterator (begin of list)
    /// @param[in]  legalDestinations  Legal destinations
    /// @return                        Move list iterator (end of generated moves)
    template <typename IteratorType, MoveGenType type, typename ParamType, Color turn>
    IteratorType generateMovesForPawnsTempl(
        IteratorType i,
        ParamType legalDestinations) const noexcept;

    /// @brief Generates legal pawn moves, dispatch for side-to-move.
    ///
    /// @tparam     IteratorType       Move list iterator type
    /// @tparam     type               Move generator type
    /// @tparam     legalDestinations  Parameter type for legal destinations
    /// @param[in]  i                  Move list iterator (begin of list)
    /// @param[in]  legalDestinations  Legal destinations
    /// @return                        Move list iterator (end of generated moves)
    template <typename IteratorType, MoveGenType type, typename ParamType>
    IteratorType generateMovesForPawns(
        IteratorType i,
        ParamType legalDestinations) const noexcept;

    /// @brief Generates legal knight moves
    ///
    /// @tparam     IteratorType       Move list iterator type
    /// @tparam     type               Move generator type
    /// @tparam     legalDestinations  Parameter type for legal destinations
    /// @param[in]  i                  Move list iterator (begin of list)
    /// @param[in]  sq                 Source square
    /// @param[in]  legalDestinations  Legal destinations
    /// @return                        Move list iterator (end of generated moves)
    template <typename IteratorType, MoveGenType type, typename ParamType>
    IteratorType generateMovesForKnight(
        IteratorType i,
        Square sq,
        ParamType legalDestinations) const noexcept;

    /// @brief Generates legal bishop moves for queen or bishop.
    ///
    /// @tparam     IteratorType       Move list iterator type
    /// @tparam     type               Move generator type
    /// @tparam     legalDestinations  Parameter type for legal destinations
    /// @tparam     pinned             Whether the piece is pinned
    /// @param[in]  i                  Move list iterator (begin of list)
    /// @param[in]  sq                 Source square
    /// @param[in]  legalDestinations  Legal destinations
    /// @param[in]  typeAndPromo       @coderef{MoveTypeAndPromotion::REGULAR_BISHOP_MOVE} or
    ///                                @coderef{MoveTypeAndPromotion::REGULAR_QUEEN_MOVE} depending
    ///                                on the piece.
    /// @return                        Move list iterator (end of generated moves)
    template <typename IteratorType, MoveGenType type, typename ParamType, bool pinned>
    IteratorType generateMovesForBishop(
        IteratorType i,
        Square sq,
        ParamType legalDestinations,
        MoveTypeAndPromotion typeAndPromo) const noexcept;

    /// @brief Generates legal rook moves for queen or rook.
    ///
    /// @tparam     IteratorType       Move list iterator type
    /// @tparam     type               Move generator type
    /// @tparam     legalDestinations  Parameter type for legal destinations
    /// @tparam     pinned             Whether the piece is pinned
    /// @param[in]  i                  Move list iterator (begin of list)
    /// @param[in]  sq                 Source square
    /// @param[in]  legalDestinations  Legal destinations
    /// @param[in]  typeAndPromo       @coderef{MoveTypeAndPromotion::REGULAR_ROOK_MOVE} or
    ///                                @coderef{MoveTypeAndPromotion::REGULAR_QUEEN_MOVE} depending
    ///                                on the piece.
    /// @return                        Move list iterator (end of generated moves)
    template <typename IteratorType, MoveGenType type, typename ParamType, bool pinned>
    IteratorType generateMovesForRook(
        IteratorType i,
        Square sq,
        ParamType legalDestinations,
        MoveTypeAndPromotion typeAndPromo) const noexcept;

    /// @brief Generates legal king moves
    ///
    /// @tparam     IteratorType       Move list iterator type
    /// @tparam     type               Move generator type
    /// @param[in]  i                  Move list iterator (begin of list)
    /// @param[in]  attackedSquares    Attacked squares
    /// @return                        Move list iterator (end of generated moves)
    ///
    /// @sa (@coderef{Attacks::determineAttackedSquares()})
    template <typename IteratorType>
    IteratorType generateMovesForKing(
        IteratorType i,
        SquareSet attackedSquares) const noexcept;

    /// @brief Generates the legal castling move, if any.
    ///
    /// @tparam     type               Move generator type. Must be
    ///                                @coderef{MoveGenType::NO_CHECK}.
    /// @tparam     MoveStoreFn        Move store function
    /// @tparam     shortCastling      Whether the move is short castling
    /// @param[in]  attackedSquares    Attacked squares
    /// @param[in]  store              Move store target
    ///
    /// @sa (@coderef{Attacks::determineAttackedSquares()})
    template <MoveGenType type, typename MoveStoreFn, bool shortCastling>
    void generateMovesForCastlingStoreFnTempl(SquareSet attackedSquares, MoveStoreFn::Store &store) const noexcept;

    /// @brief Generates all legal moves when not in check
    ///
    /// @tparam     IteratorType       Move list iterator type
    /// @param[in]  i                  Move list iterator (begin of list)
    /// @return                        Move list iterator (end of generated moves)
    ///
    /// @sa (@coderef{Attacks::determineAttackedSquares()})
    template <typename IteratorType>
    inline IteratorType generateAllLegalMovesTemplNoCheck(
        IteratorType i) const noexcept;

    /// @brief Generates all legal moves when in check (but not in double check)
    ///
    /// @tparam     IteratorType       Move list iterator type
    /// @param[in]  i                  Move list iterator (begin of list)
    /// @return                        Move list iterator (end of generated moves)
    ///
    /// @sa (@coderef{Attacks::determineAttackedSquares()})
    template <typename IteratorType>
    inline IteratorType generateAllLegalMovesTemplInCheck(
        IteratorType i) const noexcept;

    /// @brief Generates all legal moves when in double check
    ///
    /// @tparam     IteratorType       Move list iterator type
    /// @param[in]  i                  Move list iterator (begin of list)
    /// @return                        Move list iterator (end of generated moves)
    ///
    /// @sa (@coderef{Attacks::determineAttackedSquares()})
    template <typename IteratorType>
    inline IteratorType generateAllLegalMovesTemplInDoubleCheck(
        IteratorType i) const noexcept;

    /// @brief Generates all legal moves for a specific iterator type. Selects
    /// the move generator based on whether the king is not in check, in check,
    /// or in double check.
    ///
    /// @tparam     type               Move generator type
    /// @tparam     IteratorType       Move list iterator type
    /// @param[in]  i                  Move list iterator (begin of list)
    /// @return                        Move list iterator (end of generated moves)
    ///
    /// Depending on the iterator type, we generate the following information
    ///
    /// <table>
    /// <tr>
    ///   <th>Iterator type</th>
    ///   <th>Generated return</th>
    /// </tr>
    /// <tr>
    ///   <td>@c MoveList::iterator</td>
    ///   <td>Proper list of legal moves</td>
    /// </tr>
    /// <tr>
    ///   <td>@coderef{LegalMoveCounterIterator}</td>
    ///   <td>Number of legal moves</td>
    /// </tr>
    /// <tr>
    ///   <td>@coderef{LegalMoveDetectorIterator}</td>
    ///   <td>Whether there are any legal moves</td>
    /// </tr>
    /// </table>
    template <MoveGenType type, typename IteratorType>
    inline IteratorType generateMovesIterTempl(
        IteratorType i) const noexcept;


    template <MoveGenType type, typename MoveStoreFn>
    inline void generateMovesForPawnAndDestNoCaptureStoreFnTempl(
        SquareSet srcSqMask, Square dst, typename MoveStoreFn::Store &store) const noexcept;

    template <MoveGenType type, typename MoveStoreFn>
    inline void generateMovesForPawnAndDestCaptureStoreFnTempl(
        SquareSet srcSqMask, Square dst, typename MoveStoreFn::Store &store) const noexcept;

    template <MoveGenType type, typename MoveStoreFn>
    inline void generateMovesForPawnAndDestPromoNoCaptureStoreFnTempl(
        SquareSet srcSqMask, Square dst, Piece promo, typename MoveStoreFn::Store &store) const noexcept;

    template <MoveGenType type, typename MoveStoreFn>
    inline void generateMovesForPawnAndDestPromoCaptureStoreFnTempl(
        SquareSet srcSqMask, Square dst, Piece promo, typename MoveStoreFn::Store &store) const noexcept;

    template <MoveGenType type, typename MoveStoreFn>
    inline void generateMovesForKnightAndDestStoreFnTempl(
        SquareSet srcSqMask, Square dst, typename MoveStoreFn::Store &store) const noexcept;

    template <MoveGenType type, MoveTypeAndPromotion moveType, typename MoveStoreFn>
    inline void generateMovesForSliderAndDestStoreFnTempl(
        SquareSet srcSqMask, Square dst, typename MoveStoreFn::Store &store) const noexcept;

    template <MoveGenType type, typename MoveStoreFn>
    inline void generateMovesForKingAndDestStoreFnTempl(
        SquareSet srcSqMask, Square dst, typename MoveStoreFn::Store &store) const noexcept;


    template <typename... Args>
    static Move generateSingleIllegalNoMove(const ChessBoard &board, Args... args) noexcept;

    template <MoveGenType type>
    static Move generateSingleMoveForPawnAndDestNoCaptureTempl(const ChessBoard &board, SquareSet srcSqMask, Square dst) noexcept;

    template <MoveGenType type>
    static Move generateSingleMoveForPawnAndDestCaptureTempl(const ChessBoard &board, SquareSet srcSqMask, Square dst) noexcept;

    template <MoveGenType type>
    static Move generateSingleMoveForPawnAndDestPromoNoCaptureTempl(const ChessBoard &board, SquareSet srcSqMask, Square dst, Piece promo) noexcept;

    template <MoveGenType type>
    static Move generateSingleMoveForPawnAndDestPromoCaptureTempl(const ChessBoard &board, SquareSet srcSqMask, Square dst, Piece promo) noexcept;

    template <MoveGenType type>
    static Move generateSingleMoveForKnightAndDestTempl(const ChessBoard &board, SquareSet srcSqMask, Square dst) noexcept;

    template <MoveGenType type>
    static Move generateSingleMoveForBishopAndDestTempl(const ChessBoard &board, SquareSet srcSqMask, Square dst) noexcept;

    template <MoveGenType type>
    static Move generateSingleMoveForRookAndDestTempl(const ChessBoard &board, SquareSet srcSqMask, Square dst) noexcept;

    template <MoveGenType type>
    static Move generateSingleMoveForQueenAndDestTempl(const ChessBoard &board, SquareSet srcSqMask, Square dst) noexcept;
    template <MoveGenType type>
    static Move generateSingleMoveForKingAndDestTempl(const ChessBoard &board, SquareSet srcSqMask, Square dst) noexcept;

    template <MoveGenType type>
    static Move generateSingleMoveForShortCastlingTempl(const ChessBoard &board) noexcept;

    template <MoveGenType type>
    static Move generateSingleMoveForLongCastlingTempl(const ChessBoard &board) noexcept;


    template <typename... Args>
    static std::size_t generateNoMoves(const ChessBoard &board, ShortMoveList &move, Args... args) noexcept;

    template <MoveGenType type>
    static std::size_t generateMovesForPawnAndDestNoCaptureTempl(const ChessBoard &board, ShortMoveList &moves, SquareSet srcSqMask, Square dst) noexcept;

    template <MoveGenType type>
    static std::size_t generateMovesForPawnAndDestCaptureTempl(const ChessBoard &board, ShortMoveList &moves, SquareSet srcSqMask, Square dst) noexcept;

    template <MoveGenType type>
    static std::size_t generateMovesForPawnAndDestPromoNoCaptureTempl(const ChessBoard &board, ShortMoveList &moves, SquareSet srcSqMask, Square dst, Piece promo) noexcept;

    template <MoveGenType type>
    static std::size_t generateMovesForPawnAndDestPromoCaptureTempl(const ChessBoard &board, ShortMoveList &moves, SquareSet srcSqMask, Square dst, Piece promo) noexcept;

    template <MoveGenType type>
    static std::size_t generateMovesForKnightAndDestTempl(const ChessBoard &board, ShortMoveList &moves, SquareSet srcSqMask, Square dst) noexcept;

    template <MoveGenType type>
    static std::size_t generateMovesForBishopAndDestTempl(const ChessBoard &board, ShortMoveList &moves, SquareSet srcSqMask, Square dst) noexcept;

    template <MoveGenType type>
    static std::size_t generateMovesForRookAndDestTempl(const ChessBoard &board, ShortMoveList &moves, SquareSet srcSqMask, Square dst) noexcept;

    template <MoveGenType type>
    static std::size_t generateMovesForQueenAndDestTempl(const ChessBoard &board, ShortMoveList &moves, SquareSet srcSqMask, Square dst) noexcept;

    template <MoveGenType type>
    static std::size_t generateMovesForKingAndDestTempl(const ChessBoard &board, ShortMoveList &moves, SquareSet srcSqMask, Square dst) noexcept;

    template <MoveGenType type>
    static std::size_t generateMovesForShortCastlingTempl(const ChessBoard &board, ShortMoveList &moves) noexcept;

    template <MoveGenType type>
    static std::size_t generateMovesForLongCastlingTempl(const ChessBoard &board, ShortMoveList &moves) noexcept;


    template <MoveGenType type>
    static std::size_t generateMovesTempl(const ChessBoard &board, MoveList &moves) noexcept;

    template <MoveGenType type>
    static bool hasLegalMovesTempl(const ChessBoard &board) noexcept;

    template <MoveGenType type>
    static std::size_t getNumberOfLegalMovesTempl(const ChessBoard &board) noexcept;

    void calculateMasks(const ArrayBoard &board) noexcept;

    friend MoveGenFunctionTables;
};

/// @ingroup PgnReaderImpl
/// @brief Move generator functions
struct MoveGenFunctions
{
    /// @brief Generates a legal non-promoting, non-capturing pawn move with a known
    /// destination square.
    ///
    /// @param[in]  board           The chess board
    /// @param[in]  srcSqMask       Allowed source squares
    /// @param[in]  dst             Destination square
    /// @return                     Move. In case a move was not found or it was
    ///                             ambiguous, @coderef{Move::isIllegal()} returns @true.
    Move (*generateSingleMoveForPawnAndDestNoCapture)(const ChessBoard &board, SquareSet srcSqMask, Square dst) noexcept;

    /// @brief Generates a legal non-promoting, capturing pawn move with a known
    /// destination square.
    ///
    /// @param[in]  board           The chess board
    /// @param[in]  srcSqMask       Allowed source squares
    /// @param[in]  dst             Destination square
    /// @return                     Move. In case a move was not found or it was
    ///                             ambiguous, @coderef{Move::isIllegal()} returns @true.
    Move (*generateSingleMoveForPawnAndDestCapture)(const ChessBoard &board, SquareSet srcSqMask, Square dst)noexcept;

    /// @brief Generates a legal promoting, non-capturing pawn move with a known
    /// destination square.
    ///
    /// @param[in]  board           The chess board
    /// @param[in]  srcSqMask       Allowed source squares
    /// @param[in]  dst             Destination square
    /// @param[in]  promo           Promotion piece
    /// @return                     Move. In case a move was not found or it was
    ///                             ambiguous, @coderef{Move::isIllegal()} returns @true.
    Move (*generateSingleMoveForPawnAndDestPromoNoCapture)(const ChessBoard &board, SquareSet srcSqMask, Square dst, Piece promo) noexcept;

    /// @brief Generates a legal promoting, capturing pawn move with a known
    /// destination square.
    ///
    /// @param[in]  board           The chess board
    /// @param[in]  srcSqMask       Allowed source squares
    /// @param[in]  dst             Destination square
    /// @param[in]  promo           Promotion piece
    /// @return                     Move. In case a move was not found or it was
    ///                             ambiguous, @coderef{Move::isIllegal()} returns @true.
    Move (*generateSingleMoveForPawnAndDestPromoCapture)(const ChessBoard &board, SquareSet srcSqMask, Square dst, Piece promo) noexcept;

    /// @brief Generates a legal knight move with a known destination square. May be
    /// capturing.
    ///
    /// @param[in]  board           The chess board
    /// @param[in]  srcSqMask       Allowed source squares
    /// @param[in]  dst             Destination square
    /// @return                     Move. In case a move was not found or it was
    ///                             ambiguous, @coderef{Move::isIllegal()} returns @true.
    Move (*generateSingleMoveForKnightAndDest)(const ChessBoard &board, SquareSet srcSqMask, Square dst) noexcept;

    /// @brief Generates a legal bishop move with a known destination square. May be
    /// capturing.
    ///
    /// @param[in]  board           The chess board
    /// @param[in]  srcSqMask       Allowed source squares
    /// @param[in]  dst             Destination square
    /// @return                     Move. In case a move was not found or it was
    ///                             ambiguous, @coderef{Move::isIllegal()} returns @true.
    Move (*generateSingleMoveForBishopAndDest)(const ChessBoard &board, SquareSet srcSqMask, Square dst) noexcept;

    /// @brief Generates a legal rook move with a known destination square. May be
    /// capturing.
    ///
    /// @param[in]  board           The chess board
    /// @param[in]  srcSqMask       Allowed source squares
    /// @param[in]  dst             Destination square
    /// @return                     Move. In case a move was not found or it was
    ///                             ambiguous, @coderef{Move::isIllegal()} returns @true.
    Move (*generateSingleMoveForRookAndDest)(const ChessBoard &board, SquareSet srcSqMask, Square dst) noexcept;

    /// @brief Generates a legal queen move with a known destination square. May be
    /// capturing.
    ///
    /// @param[in]  board           The chess board
    /// @param[in]  srcSqMask       Allowed source squares
    /// @param[in]  dst             Destination square
    /// @return                     Move. In case a move was not found or it was
    ///                             ambiguous, @coderef{Move::isIllegal()} returns @true.
    Move (*generateSingleMoveForQueenAndDest)(const ChessBoard &board, SquareSet srcSqMask, Square dst) noexcept;

    /// @brief Generates a legal king move with a known destination square. May be
    /// capturing.
    ///
    /// @param[in]  board           The chess board
    /// @param[in]  srcSqMask       Allowed source squares
    /// @param[in]  dst             Destination square
    /// @return                     Move. In case a move was not found or it was
    ///                             ambiguous, @coderef{Move::isIllegal()} returns @true.
    Move (*generateSingleMoveForKingAndDest)(const ChessBoard &board, SquareSet srcSqMask, Square dst) noexcept;

    /// @brief Generates a legal short castling move.
    ///
    /// @return                     Move. In case a move was not found or it was
    ///                             ambiguous, @coderef{Move::isIllegal()} returns @true.
    Move (*generateSingleMoveForShortCastling)(const ChessBoard &board) noexcept;

    /// @brief Generates a legal long castling move.
    ///
    /// @return                     Move. In case a move was not found or it was
    ///                             ambiguous, @coderef{Move::isIllegal()} returns @true.
    Move (*generateSingleMoveForLongCastling)(const ChessBoard &board) noexcept;

    /// @brief Generates a list of legal non-promoting, non-capturing pawn moves with a known
    /// destination square.
    ///
    /// @param[in]  board           The chess board
    /// @param[out] moves           List of returned moves
    /// @param[in]  srcSqMask       Allowed source squares
    /// @param[in]  dst             Destination square
    /// @return                     Number of returned moves
    std::size_t (*generateMovesForPawnAndDestNoCapture)(const ChessBoard &board, ShortMoveList &moves, SquareSet srcSqMask, Square dst) noexcept;

    /// @brief Generates a list of legal non-promoting, capturing pawn moves with a known
    /// destination square.
    ///
    /// @param[in]  board           The chess board
    /// @param[out] moves           List of returned moves
    /// @param[in]  srcSqMask       Allowed source squares
    /// @param[in]  dst             Destination square
    /// @return                     Number of returned moves
    std::size_t (*generateMovesForPawnAndDestCapture)(const ChessBoard &board, ShortMoveList &moves, SquareSet srcSqMask, Square dst) noexcept;

    /// @brief Generates a list of legal promoting, non-capturing pawn moves with a known
    /// destination square.
    ///
    /// @param[in]  board           The chess board
    /// @param[out] moves           List of returned moves
    /// @param[in]  srcSqMask       Allowed source squares
    /// @param[in]  dst             Destination square
    /// @param[in]  promo           Promotion piece
    /// @return                     Number of returned moves
    std::size_t (*generateMovesForPawnAndDestPromoNoCapture)(const ChessBoard &board, ShortMoveList &moves, SquareSet srcSqMask, Square dst, Piece promo) noexcept;

    /// @brief Generates a list of legal promoting, capturing pawn moves with a known
    /// destination square.
    ///
    /// @param[in]  board           The chess board
    /// @param[out] moves           List of returned moves
    /// @param[in]  srcSqMask       Allowed source squares
    /// @param[in]  dst             Destination square
    /// @param[in]  promo           Promotion piece
    /// @return                     Number of returned moves
    std::size_t (*generateMovesForPawnAndDestPromoCapture)(const ChessBoard &board, ShortMoveList &moves, SquareSet srcSqMask, Square dst, Piece promo) noexcept;

    /// @brief Generates a list of legal knight moves with a known destination
    /// square. May be capturing.
    ///
    /// @param[in]  board           The chess board
    /// @param[out] moves           List of returned moves
    /// @param[in]  srcSqMask       Allowed source squares
    /// @param[in]  dst             Destination square
    /// @return                     Number of returned moves
    std::size_t (*generateMovesForKnightAndDest)(const ChessBoard &board, ShortMoveList &moves, SquareSet srcSqMask, Square dst) noexcept;

    /// @brief Generates a list of legal bishop moves with a known destination
    /// square. May be capturing.
    ///
    /// @param[in]  board           The chess board
    /// @param[out] moves           List of returned moves
    /// @param[in]  srcSqMask       Allowed source squares
    /// @param[in]  dst             Destination square
    /// @return                     Number of returned moves
    std::size_t (*generateMovesForBishopAndDest)(const ChessBoard &board, ShortMoveList &moves, SquareSet srcSqMask, Square dst) noexcept;

    /// @brief Generates a list of legal rook moves with a known destination
    /// square. May be capturing.
    ///
    /// @param[in]  board           The chess board
    /// @param[out] moves           List of returned moves
    /// @param[in]  srcSqMask       Allowed source squares
    /// @param[in]  dst             Destination square
    /// @return                     Number of returned moves
    std::size_t (*generateMovesForRookAndDest)(const ChessBoard &board, ShortMoveList &moves, SquareSet srcSqMask, Square dst) noexcept;

    /// @brief Generates a list of legal queen moves with a known destination
    /// square. May be capturing.
    ///
    /// @param[in]  board           The chess board
    /// @param[out] moves           List of returned moves
    /// @param[in]  srcSqMask       Allowed source squares
    /// @param[in]  dst             Destination square
    /// @return                     Number of returned moves
    std::size_t (*generateMovesForQueenAndDest)(const ChessBoard &board, ShortMoveList &moves, SquareSet srcSqMask, Square dst) noexcept;

    /// @brief Generates a list of legal king moves with a known destination
    /// square. May be capturing.
    ///
    /// @param[in]  board           The chess board
    /// @param[out] moves           List of returned moves
    /// @param[in]  srcSqMask       Allowed source squares
    /// @param[in]  dst             Destination square
    /// @return                     Number of returned moves
    std::size_t (*generateMovesForKingAndDest)(const ChessBoard &board, ShortMoveList &moves, SquareSet srcSqMask, Square dst) noexcept;

    /// @brief Generates the short castling move in a list, if legal.
    ///
    /// @param[in]  board           The chess board
    /// @param[out] moves           List of returned moves
    /// @return                     Number of returned moves
    std::size_t (*generateMovesForShortCastling)(const ChessBoard &board, ShortMoveList &moves) noexcept;

    /// @brief Generates the long castling move in a list, if legal.
    ///
    /// @param[in]  board           The chess board
    /// @param[out] moves           List of returned moves
    /// @return                     Number of returned moves
    std::size_t (*generateMovesForLongCastling)(const ChessBoard &board, ShortMoveList &moves) noexcept;

    /// @brief Generates a list of all legal moves for the current position
    ///
    /// @param[in] moves            Caller-allocated move list
    /// @return                     Number of moves added in the move list
    std::size_t (*generateMoves)(const ChessBoard &board, MoveList &moves) noexcept;

    /// @brief Returns the number of legal moves for the current position. This
    /// function is mostly useful for calculating the moves in perft leaf
    /// positions.
    ///
    /// @param[in]  board           The chess board
    /// @return                     Number of legal moves
    ///
    /// @sa https://www.chessprogramming.org/Perft
    std::size_t (*getNumberOfLegalMoves)(const ChessBoard &board) noexcept;

    /// @brief Determines whether there are any legal moves
    /// function is mostly useful for calculating the moves in perft leaf
    /// positions.
    ///
    /// @return                     Whether legal moves exist
    bool (*hasLegalMoves)(const ChessBoard &board) noexcept;
};

Move ChessBoard::generateSingleMoveForPawnAndDestNoCapture(SquareSet srcSqMask, Square dst) const noexcept
{
    return m_moveGenFns->generateSingleMoveForPawnAndDestNoCapture(*this, srcSqMask, dst);
}

Move ChessBoard::generateSingleMoveForPawnAndDestCapture(SquareSet srcSqMask, Square dst) const noexcept
{
    return m_moveGenFns->generateSingleMoveForPawnAndDestCapture(*this, srcSqMask, dst);
}

Move ChessBoard::generateSingleMoveForPawnAndDestPromoNoCapture(SquareSet srcSqMask, Square dst, Piece promo) const noexcept
{
    return m_moveGenFns->generateSingleMoveForPawnAndDestPromoNoCapture(*this, srcSqMask, dst, promo);
}

Move ChessBoard::generateSingleMoveForPawnAndDestPromoCapture(SquareSet srcSqMask, Square dst, Piece promo) const noexcept
{
    return m_moveGenFns->generateSingleMoveForPawnAndDestPromoCapture(*this, srcSqMask, dst, promo);
}

Move ChessBoard::generateSingleMoveForKnightAndDest(SquareSet srcSqMask, Square dst) const noexcept
{
    return m_moveGenFns->generateSingleMoveForKnightAndDest(*this, srcSqMask, dst);
}

Move ChessBoard::generateSingleMoveForBishopAndDest(SquareSet srcSqMask, Square dst) const noexcept
{
    return m_moveGenFns->generateSingleMoveForBishopAndDest(*this, srcSqMask, dst);
}

Move ChessBoard::generateSingleMoveForRookAndDest(SquareSet srcSqMask, Square dst) const noexcept
{
    return m_moveGenFns->generateSingleMoveForRookAndDest(*this, srcSqMask, dst);
}

Move ChessBoard::generateSingleMoveForQueenAndDest(SquareSet srcSqMask, Square dst) const noexcept
{
    return m_moveGenFns->generateSingleMoveForQueenAndDest(*this, srcSqMask, dst);
}

Move ChessBoard::generateSingleMoveForKingAndDest(SquareSet srcSqMask, Square dst) const noexcept
{
    return m_moveGenFns->generateSingleMoveForKingAndDest(*this, srcSqMask, dst);
}

Move ChessBoard::generateSingleMoveForShortCastling() const noexcept
{
    return m_moveGenFns->generateSingleMoveForShortCastling(*this);
}

Move ChessBoard::generateSingleMoveForLongCastling() const noexcept
{
    return m_moveGenFns->generateSingleMoveForLongCastling(*this);
}

std::size_t ChessBoard::generateMovesForPawnAndDestNoCapture(ShortMoveList &moves, SquareSet srcSqMask, Square dst) const noexcept
{
    return m_moveGenFns->generateMovesForPawnAndDestNoCapture(*this, moves, srcSqMask, dst);
}

std::size_t ChessBoard::generateMovesForPawnAndDestCapture(ShortMoveList &moves, SquareSet srcSqMask, Square dst) const noexcept
{
    return m_moveGenFns->generateMovesForPawnAndDestCapture(*this, moves, srcSqMask, dst);
}

std::size_t ChessBoard::generateMovesForPawnAndDestPromoNoCapture(ShortMoveList &moves, SquareSet srcSqMask, Square dst, Piece promo) const noexcept
{
    return m_moveGenFns->generateMovesForPawnAndDestPromoNoCapture(*this, moves, srcSqMask, dst, promo);
}

std::size_t ChessBoard::generateMovesForPawnAndDestPromoCapture(ShortMoveList &moves, SquareSet srcSqMask, Square dst, Piece promo) const noexcept
{
    return m_moveGenFns->generateMovesForPawnAndDestPromoCapture(*this, moves, srcSqMask, dst, promo);
}

std::size_t ChessBoard::generateMovesForKnightAndDest(ShortMoveList &moves, SquareSet srcSqMask, Square dst) const noexcept
{
    return m_moveGenFns->generateMovesForKnightAndDest(*this, moves, srcSqMask, dst);
}

std::size_t ChessBoard::generateMovesForBishopAndDest(ShortMoveList &moves, SquareSet srcSqMask, Square dst) const noexcept
{
    return m_moveGenFns->generateMovesForBishopAndDest(*this, moves, srcSqMask, dst);
}

std::size_t ChessBoard::generateMovesForRookAndDest(ShortMoveList &moves, SquareSet srcSqMask, Square dst) const noexcept
{
    return m_moveGenFns->generateMovesForRookAndDest(*this, moves, srcSqMask, dst);
}

std::size_t ChessBoard::generateMovesForQueenAndDest(ShortMoveList &moves, SquareSet srcSqMask, Square dst) const noexcept
{
    return m_moveGenFns->generateMovesForQueenAndDest(*this, moves, srcSqMask, dst);
}

std::size_t ChessBoard::generateMovesForKingAndDest(ShortMoveList &moves, SquareSet srcSqMask, Square dst) const noexcept
{
    return m_moveGenFns->generateMovesForKingAndDest(*this, moves, srcSqMask, dst);
}

std::size_t ChessBoard::generateMovesForShortCastling(ShortMoveList &moves) const noexcept
{
    return m_moveGenFns->generateMovesForShortCastling(*this, moves);
}

std::size_t ChessBoard::generateMovesForLongCastling(ShortMoveList &moves) const noexcept
{
    return m_moveGenFns->generateMovesForLongCastling(*this, moves);
}

std::size_t ChessBoard::generateMoves(MoveList &moves) const noexcept
{
    return m_moveGenFns->generateMoves(*this, moves);
}

std::size_t ChessBoard::getNumberOfLegalMoves() const noexcept
{
    return m_moveGenFns->getNumberOfLegalMoves(*this);
}

bool ChessBoard::hasLegalMoves() const noexcept
{
    return m_moveGenFns->hasLegalMoves(*this);
}

PositionStatus ChessBoard::determineStatus() const noexcept
{
    static_assert(PositionStatus::NORMAL    == PositionStatus { 0U });
    static_assert(PositionStatus::CHECK     == PositionStatus { 1U });
    static_assert(PositionStatus::STALEMATE == PositionStatus { 2U });
    static_assert(PositionStatus::MATE      == PositionStatus { 3U });

    const bool noLegalMoves { !m_moveGenFns->hasLegalMoves(*this) };
    const bool inCheck { m_checkers != SquareSet::none() };

    return PositionStatus { static_cast<std::uint8_t>(noLegalMoves * 2U + inCheck) };
}


static_assert(Move::illegalNoMove().isIllegal());
static_assert(Move::illegalNoMove().getTypeAndPromotion() == MoveTypeAndPromotion::ILLEGAL);
static_assert(Move::illegalAmbiguousMove().isIllegal());
static_assert(Move::illegalAmbiguousMove().getTypeAndPromotion() == MoveTypeAndPromotion::ILLEGAL);

}

#endif
