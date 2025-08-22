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

#include "chessboard.h"

#include "bittricks.h"
#include "pgnreader-error.h"

#include <cstdint>
#include <format>
#include <tuple>


namespace hoover_chess_utils::pgn_reader
{

namespace
{

constexpr inline std::string_view castlingSideToString(bool shortCastling) noexcept
{
    return shortCastling ? std::string_view("short") : std::string_view("long");
}

constexpr inline std::string_view sideToString(Color side) noexcept
{
    return side == Color::WHITE ? std::string_view("white") : std::string_view("black");
}

std::uint32_t readNumber(std::string_view::iterator start, std::string_view::iterator end,
                         std::uint32_t maxValue)
{
    assert(start != end);
    assert(maxValue <= 99999); // assert that we don't need to check for std::uint32_t overflows

    std::uint32_t num { };
    std::string_view::iterator i { start };

    do
    {
        std::uint8_t digit = (*i++) - '0';

        if (digit >= 10U) [[unlikely]]
            throw PgnError(PgnErrorCode::BAD_FEN, std::format("Bad number: {}", std::string_view { start, end }));

        num = num * 10U + digit;
        if (num > maxValue) [[unlikely]]
            throw PgnError(PgnErrorCode::BAD_FEN, std::format("Number overflow: {}", std::string_view { start, end }));
    }
    while (i != end);

    return num;
}

std::string_view::iterator parseBoard(std::string_view::iterator i, std::string_view::iterator end, BitBoard &board)
{
    // parse the board
    std::uint8_t col { 0U };
    std::uint8_t row { 7U };

    while (i != end)
    {
        const std::uint8_t c { static_cast<std::uint8_t>(*i) };

        if (c >= 64U && c <= 127U)
        {
            if (col >= 8U) [[unlikely]]
                throw PgnError(PgnErrorCode::BAD_FEN, std::format("Board setup: too many pieces on row {}", row + 1U));

            // piece specifier
            std::uint8_t charOffset = c & 31U;

            constexpr std::uint32_t validChars {
                (std::uint32_t { 1 } << ('P' - 64U)) |
                (std::uint32_t { 1 } << ('N' - 64U)) |
                (std::uint32_t { 1 } << ('B' - 64U)) |
                (std::uint32_t { 1 } << ('R' - 64U)) |
                (std::uint32_t { 1 } << ('Q' - 64U)) |
                (std::uint32_t { 1 } << ('K' - 64U))
            };

            if (((validChars >> charOffset) & 1U) == 0U) [[unlikely]]
            {
                throw PgnError(
                    PgnErrorCode::BAD_FEN,
                    std::format("Board setup: bad character '{}'", static_cast<char>(c)));
            }

            SquareSet bit { SquareSet::square(col, row) };

            board.pawns       |= (charOffset == 'P' - 64U) ? bit : SquareSet::none();
            board.knights     |= (charOffset == 'N' - 64U) ? bit : SquareSet::none();
            board.bishops     |= (charOffset == 'B' - 64U) ? bit : SquareSet::none();
            board.rooks       |= (charOffset == 'R' - 64U) ? bit : SquareSet::none();
            board.queens      |= (charOffset == 'Q' - 64U) ? bit : SquareSet::none();
            board.kings       |= (charOffset == 'K' - 64U) ? bit : SquareSet::none();

            board.whitePieces |= (c < 96U ? bit : SquareSet::none());

            ++col;
        }
        else if (c >= '1' && c <= '8')
        {
            col += (c - '0');
        }
        else if (c == '/')
        {
            if (row == 0U) [[unlikely]]
                throw PgnError(PgnErrorCode::BAD_FEN, "Board setup: too many rows");

            col = 0U;
            --row;
        }
        else [[unlikely]]
        {
            throw PgnError(
                PgnErrorCode::BAD_FEN,
                std::format("Board setup: bad character '{}'", static_cast<char>(c)));
        }

        ++i;
    }

    if (row != 0U) [[unlikely]]
        throw PgnError(PgnErrorCode::BAD_FEN, "Board setup: not enough rows");

    return i;
}

template <bool shortCastling, Color color>
void findAndSetCastlingRook(const BitBoard &board, Square &castlingRook)
{
    if (castlingRook != Square::NONE)
    {
        throw PgnError(
            PgnErrorCode::BAD_FEN,
            std::format("Bad castling indicator: {} {} castling already set",
                        sideToString(color),
                        castlingSideToString(shortCastling)));
    }

    SquareSet castlingRow;
    SquareSet pieceMask;

    if constexpr (color == Color::WHITE)
    {
        castlingRow = SquareSet::row(0U);
        pieceMask = board.whitePieces;
    }
    else
    {
        castlingRow = SquareSet::row(7U);
        pieceMask = ~board.whitePieces;
    }

    const Square kingSq { (board.kings & pieceMask & castlingRow).firstSquare() };
    if (kingSq == Square::NONE) [[unlikely]]
    {
        throw PgnError(
            PgnErrorCode::BAD_FEN,
            "Bad castling indicator: king not on castling rank");
    }

    Square rookSquare;
    if constexpr (shortCastling)
    {
        const SquareSet validRookSquares {
            castlingRow &~ SquareSet { BitTricks::bits0ToN(static_cast<std::uint8_t>(kingSq)) } };

        rookSquare = (validRookSquares & board.rooks & pieceMask).lastSquare();
    }
    else
    {
        const SquareSet validRookSquares {
            castlingRow & SquareSet { BitTricks::bits0ToN(static_cast<std::uint8_t>(kingSq)) } };
        rookSquare = (validRookSquares & board.rooks & pieceMask).firstSquare();
    }

    if (rookSquare == Square::NONE)
    {
        throw PgnError(
            PgnErrorCode::BAD_FEN,
            std::format("Bad castling indicator: no rook for {} {} castling",
                        sideToString(color),
                        castlingSideToString(shortCastling)));
    }

    castlingRook = rookSquare;
}

template <Color color>
void setCastlingRookFRC(
    const BitBoard &board, const Square castlingRook,
    Square &castlingRookLong, Square &castlingRookShort)
{
    SquareSet castlingRow;
    SquareSet pieceMask;

    if constexpr (color == Color::WHITE)
    {
        castlingRow = SquareSet::row(0U);
        pieceMask = board.whitePieces;
    }
    else
    {
        castlingRow = SquareSet::row(7U);
        pieceMask = ~board.whitePieces;
    }

    const Square kingSq { (board.kings & pieceMask & castlingRow).firstSquare() };
    if (kingSq == Square::NONE) [[unlikely]]
    {
        throw PgnError(
            PgnErrorCode::BAD_FEN,
            "Bad castling indicator: king not on castling rank");
    }

    if (castlingRook > kingSq)
    {
        if (castlingRookShort != Square::NONE) [[unlikely]]
            throw PgnError(
                PgnErrorCode::BAD_FEN,
                std::format("Bad castling indicator: {} short castling already set",
                            sideToString(color)));

        castlingRookShort = castlingRook;
    }
    else
    {
        if (castlingRookLong != Square::NONE) [[unlikely]]
            throw PgnError(
                PgnErrorCode::BAD_FEN,
                std::format("Bad castling indicator: {} long castling already set",
                            sideToString(color)));

        castlingRookLong = castlingRook;
    }
}

void parseCastling(
    std::string_view::iterator s, std::string_view::iterator end, const BitBoard &board,
    Square &whiteLongCastleRook, Square &whiteShortCastleRook, Square &blackLongCastleRook, Square &blackShortCastleRook)
{
    if ((end - s == 1) && *s == '-')
        return;

    std::string_view::iterator i { s };

    do
    {
        const char castlingChar { *i++ };
        switch (castlingChar)
        {
            case 'A':
            case 'B':
            case 'C':
            case 'D':
            case 'E':
            case 'F':
            case 'G':
            case 'H':
                setCastlingRookFRC<Color::WHITE>(
                    board, makeSquare(castlingChar - 'A', 0U),
                    whiteLongCastleRook, whiteShortCastleRook);
                break;

            case 'a':
            case 'b':
            case 'c':
            case 'd':
            case 'e':
            case 'f':
            case 'g':
            case 'h':
                setCastlingRookFRC<Color::BLACK>(
                    board, makeSquare(castlingChar - 'a', 7U),
                    blackLongCastleRook, blackShortCastleRook);
                break;

            case 'K':
                findAndSetCastlingRook<true, Color::WHITE>(board, whiteShortCastleRook);
                break;

            case 'Q':
                findAndSetCastlingRook<false, Color::WHITE>(board, whiteLongCastleRook);
                break;

            case 'k':
                findAndSetCastlingRook<true, Color::BLACK>(board, blackShortCastleRook);
                break;

            case 'q':
                findAndSetCastlingRook<false, Color::BLACK>(board, blackLongCastleRook);
                break;

                // fall-through
            default:
                throw PgnError(PgnErrorCode::BAD_FEN, std::format("Bad castling rights: '{}'", std::string_view { s, end }));
        }
    }
    while (i != end);
}

std::string_view::iterator parseEnPassant(std::string_view::iterator i, std::string_view::iterator end, Square &epSquare)
{
    // the space eater takes care of i != end before this function is invoked
    assert(i != end);

    char c = *i++;
    if (c == '-')
        return i;

    std::uint8_t col = c - 'a';
    if (col > 7U)
        throw PgnError(PgnErrorCode::BAD_FEN, "Bad en passant file");

    if (i == end) [[unlikely]]
        throw PgnError(PgnErrorCode::BAD_FEN, "Missing en passant rank");

    c = *i++;
    std::uint8_t row = c - '1';
    if (row > 7U)
        throw PgnError(PgnErrorCode::BAD_FEN, "Bad en passant rank");

    epSquare = makeSquare(col, row);

    return i;
}

std::tuple<std::string_view::iterator, std::string_view::iterator> nextToken(
    std::string_view::iterator i, std::string_view::iterator end, std::string_view tokenName)
{
    // eat spaces before token if any
    while (i != end)
    {
        if (*i != ' ')
            break;

        ++i;
    }

    // read the token itself
    bool gotTokenCharacter { false };
    std::string_view::iterator tokenStart { i };

    while (i != end)
    {
        if (*i == ' ')
            break;

        gotTokenCharacter = true;
        ++i;
    }

    if (!gotTokenCharacter)
        throw PgnError(
            PgnErrorCode::BAD_FEN, std::format("Premature end while reading {}", tokenName));

    return std::make_tuple(tokenStart, i);
}

} // anonymous namespace

void ChessBoard::loadFEN(std::string_view fen)
{
    BitBoard board { };
    Color turn { };
    std::uint32_t moveNum { };
    m_epSquare = Square::NONE;
    setCastlingRook(Color::WHITE, false, Square::NONE);
    setCastlingRook(Color::WHITE, true, Square::NONE);
    setCastlingRook(Color::BLACK, false, Square::NONE);
    setCastlingRook(Color::BLACK, true, Square::NONE);

    std::string_view::iterator tokenStart;
    std::string_view::iterator tokenEnd;
    const auto end { fen.end() };

    // parse board
    std::tie(tokenStart, tokenEnd) = nextToken(fen.begin(), end, std::string_view { "board" });
    parseBoard(tokenStart, tokenEnd, board);

    std::tie(tokenStart, tokenEnd) = nextToken(tokenEnd, end, std::string_view { "side to move" });

    if (tokenEnd - tokenStart != 1)
        throw PgnError(
            PgnErrorCode::BAD_FEN,
            std::format("Bad side to move indicator: {}", std::string_view { tokenStart, tokenEnd }));

    switch (*tokenStart)
    {
        case 'w':
            turn = Color::WHITE;
            break;

        case 'b':
            turn = Color::BLACK;
            break;

        default:
            throw PgnError(
                PgnErrorCode::BAD_FEN,
                std::format("Bad side to move indicator: {}", std::string_view { tokenStart, tokenEnd }));
    }

    std::tie(tokenStart, tokenEnd) = nextToken(tokenEnd, end, std::string_view { "castling rights" });

    // castling next
    parseCastling(
        tokenStart, tokenEnd, board,
        getCastlingRookRef(Color::WHITE, false),
        getCastlingRookRef(Color::WHITE, true),
        getCastlingRookRef(Color::BLACK, false),
        getCastlingRookRef(Color::BLACK, true));

    std::tie(tokenStart, tokenEnd) = nextToken(tokenEnd, end, std::string_view { "en passant" });

    // en passant
    parseEnPassant(tokenStart, tokenEnd, m_epSquare);

    std::tie(tokenStart, tokenEnd) = nextToken(tokenEnd, end, std::string_view { "half-move clock" });

    // half-move clock
    m_halfMoveClock = readNumber(tokenStart, tokenEnd, 255U);

    std::tie(tokenStart, tokenEnd) = nextToken(tokenEnd, end, std::string_view { "full-move clock" });

    // full move clock
    moveNum = readNumber(tokenStart, tokenEnd, 99999U);
    if (moveNum < 1U)
        throw PgnError(PgnErrorCode::BAD_FEN, "Invalid move number");

    while (tokenEnd != end)
    {
        if (*tokenEnd != ' ')
            throw PgnError(PgnErrorCode::BAD_FEN, std::format("Garbage at the end of FEN: {}", std::string_view { tokenEnd, end }));

        ++tokenEnd;
    }

    m_plyNum = makePlyNum(moveNum, turn);

    // set the board
    m_pawns   = board.pawns;
    m_knights = board.knights;
    m_bishops = board.bishops | board.queens;
    m_rooks   = board.rooks | board.queens;
    m_kings   = board.kings;

    m_occupancyMask = m_pawns | m_knights | m_bishops | m_rooks | m_kings;
    m_turnColorMask =
        (getTurn() == Color::WHITE ?
         m_occupancyMask & board.whitePieces : m_occupancyMask & ~board.whitePieces);

    // kings
    m_kingSq    = (m_kings &  m_turnColorMask).firstSquare();
    m_oppKingSq = (m_kings & ~m_turnColorMask).firstSquare();

    validateBoard();
}

}
