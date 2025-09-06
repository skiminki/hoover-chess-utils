// Hoover Chess Utilities / PGN reader
// Copyright (C) 2025  Sami Kiminki
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

#include "gtest/gtest.h"

#include <array>
#include <cstring>
#include <string_view>

namespace hoover_chess_utils::pgn_reader::unit_test
{

namespace
{

using TestMiniString = MiniString<15U>;

union MiniStringStorage
{
    TestMiniString str;
    std::array<char, 16> raw;

    MiniStringStorage() { }
    ~MiniStringStorage() { }
};

}

TEST(MiniString, constructor_default)
{
    MiniStringStorage storage;

    memset(storage.raw.data(), 1, sizeof storage.raw);

    // just make sure that non-zero raw storage gives a non-zero length
    EXPECT_NE(storage.str.size(), 0U);

    new (&storage.str) MiniString<15>();

    // constructor initializes the length to 0
    EXPECT_EQ(storage.str.size(), 0U);
}

TEST(MiniString, constructor_uninitialized)
{
    MiniStringStorage storage;

    memset(storage.raw.data(), 1, sizeof storage.raw);

    // just make sure that non-zero raw storage gives a non-zero length
    EXPECT_NE(storage.str.size(), 0U);

    new (&storage.str) TestMiniString { MiniString_Uninitialized { } };

    // non-initializing constructor doesn't do anything
    EXPECT_NE(storage.str.size(), 0U);
}

TEST(MiniString, assing_str)
{
    MiniString<8> str { MiniString_Uninitialized { } };

    str.assign("42");
    EXPECT_EQ(std::string_view("42"), str.getStringView());

    str.assign("123456789"); // one char too many
    EXPECT_EQ(std::string_view("12345678"), str.getStringView());
}

TEST(MiniString, assing_str_len)
{
    MiniString<8> str { MiniString_Uninitialized { } };

    str.assign("123456789", 0U);
    EXPECT_EQ(std::string_view(""), str.getStringView());

    str.assign("123456789", 7U);
    EXPECT_EQ(std::string_view("1234567"), str.getStringView());

    str.assign("123456789", 8U);
    EXPECT_EQ(std::string_view("12345678"), str.getStringView());

    str.assign("123456789", 9U);
    EXPECT_EQ(std::string_view("12345678"), str.getStringView());
}

TEST(StringUtils, sourceMaskToString)
{
    // no source mask specified (all are legal)
    EXPECT_EQ(StringUtils::sourceMaskToString(SquareSet::all()).getStringView(), std::string_view(""));

    // row mask specified
    EXPECT_EQ(StringUtils::sourceMaskToString(SquareSet::row(0U)).getStringView(), std::string_view("1"));
    EXPECT_EQ(StringUtils::sourceMaskToString(SquareSet::row(3U)).getStringView(), std::string_view("4"));
    EXPECT_EQ(StringUtils::sourceMaskToString(SquareSet::row(7U)).getStringView(), std::string_view("8"));

    // column mask specified
    EXPECT_EQ(StringUtils::sourceMaskToString(SquareSet::column(0U)).getStringView(), std::string_view("a"));
    EXPECT_EQ(StringUtils::sourceMaskToString(SquareSet::column(3U)).getStringView(), std::string_view("d"));
    EXPECT_EQ(StringUtils::sourceMaskToString(SquareSet::column(7U)).getStringView(), std::string_view("h"));

    // square specified
    EXPECT_EQ(StringUtils::sourceMaskToString(SquareSet::square(0U, 0U)).getStringView(), std::string_view("a1"));
    EXPECT_EQ(StringUtils::sourceMaskToString(SquareSet::square(1U, 0U)).getStringView(), std::string_view("b1"));
    EXPECT_EQ(StringUtils::sourceMaskToString(SquareSet::square(0U, 1U)).getStringView(), std::string_view("a2"));
    EXPECT_EQ(StringUtils::sourceMaskToString(SquareSet::square(7U, 7U)).getStringView(), std::string_view("h8"));
}

TEST(StringUtils, plyNumToString)
{
    EXPECT_EQ(StringUtils::plyNumToString(0U).getStringView(), std::string_view { "1." });
    EXPECT_EQ(StringUtils::plyNumToString(1U).getStringView(), std::string_view { "1..." });
    EXPECT_EQ(StringUtils::plyNumToString(100U).getStringView(), std::string_view { "51." });
    EXPECT_EQ(StringUtils::plyNumToString(101U).getStringView(), std::string_view { "51..." });
    EXPECT_EQ(StringUtils::plyNumToString(std::numeric_limits<std::uint32_t>::max()).getStringView(),
              std::string_view { "2147483648..." });
}

TEST(StringUtils, moveNumToString)
{
    EXPECT_EQ(StringUtils::moveNumToString(0U, Color::WHITE).getStringView(), std::string_view { "0." });
    EXPECT_EQ(StringUtils::moveNumToString(0U, Color::BLACK).getStringView(), std::string_view { "0..." });
    EXPECT_EQ(StringUtils::moveNumToString(1U, Color::WHITE).getStringView(), std::string_view { "1." });
    EXPECT_EQ(StringUtils::moveNumToString(2U, Color::BLACK).getStringView(), std::string_view { "2..." });
    EXPECT_EQ(StringUtils::moveNumToString(9U, Color::WHITE).getStringView(), std::string_view { "9." });
    EXPECT_EQ(StringUtils::moveNumToString(10U, Color::BLACK).getStringView(), std::string_view { "10..." });
    EXPECT_EQ(StringUtils::moveNumToString(11U, Color::WHITE).getStringView(), std::string_view { "11." });
    EXPECT_EQ(StringUtils::moveNumToString(100U, Color::BLACK).getStringView(), std::string_view { "100..." });
    EXPECT_EQ(StringUtils::moveNumToString(std::numeric_limits<std::uint32_t>::max(), Color::BLACK).getStringView(),
              std::string_view { "4294967295..." });
}

TEST(StringUtils, promoPieceChar)
{
    EXPECT_EQ(StringUtils::promoPieceChar(Piece::KNIGHT), 'N');
    EXPECT_EQ(StringUtils::promoPieceChar(Piece::BISHOP), 'B');
    EXPECT_EQ(StringUtils::promoPieceChar(Piece::ROOK),   'R');
    EXPECT_EQ(StringUtils::promoPieceChar(Piece::QUEEN),  'Q');
}

TEST(StringUtils, pieceToSanStr)
{
    EXPECT_EQ(StringUtils::pieceToSanStr(Piece::NONE).getStringView(),   std::string_view("?"));
    EXPECT_EQ(StringUtils::pieceToSanStr(Piece::PAWN).getStringView(),   std::string_view(""));
    EXPECT_EQ(StringUtils::pieceToSanStr(Piece::KNIGHT).getStringView(), std::string_view("N"));
    EXPECT_EQ(StringUtils::pieceToSanStr(Piece::BISHOP).getStringView(), std::string_view("B"));
    EXPECT_EQ(StringUtils::pieceToSanStr(Piece::ROOK).getStringView(),   std::string_view("R"));
    EXPECT_EQ(StringUtils::pieceToSanStr(Piece::QUEEN).getStringView(),  std::string_view("Q"));
    EXPECT_EQ(StringUtils::pieceToSanStr(Piece::KING).getStringView(),   std::string_view("K"));
    EXPECT_EQ(StringUtils::pieceToSanStr(Piece { 7U }).getStringView(),  std::string_view("?"));
    EXPECT_EQ(StringUtils::pieceToSanStr(Piece { 8U }).getStringView(),  std::string_view("?"));
    EXPECT_EQ(StringUtils::pieceToSanStr(Piece { 255U }).getStringView(),  std::string_view("?"));
}

TEST(StringUtils, squareToString)
{
    for (std::size_t i { }; i < 256U; ++i)
    {
        Square sq { static_cast<Square>(i) };

        if (sq <= Square::H8)
        {
            const char ex[2] { static_cast<char>('a' + columnOf(sq)), static_cast<char>('1' + rowOf(sq)) };

            const auto squareName1 { StringUtils::squareToString(sq, "NONE") };
            EXPECT_EQ(squareName1, std::string_view(ex, ex+2)) << "Square index " << i;

            const auto squareName2 { StringUtils::squareToString(sq, "??") };
            EXPECT_EQ(std::string_view(ex, ex+2), squareName2) << "Square index " << i;
        }
        else
        {
            EXPECT_EQ(std::string_view("NONE"), StringUtils::squareToString(sq, "NONE"))
                << "Square index " << i;
            EXPECT_EQ(std::string_view("  "), StringUtils::squareToString(sq, "  "))
                << "Square index " << i;
        }

    }
}

TEST(StringUtils, moveTypeAndPromotionToString)
{
    for (std::size_t i { }; i < 256U; ++i)
    {
        MoveTypeAndPromotion tap { static_cast<MoveTypeAndPromotion>(i) };
        std::string_view ex;

        switch (tap)
        {
#define C(e)                                                       \
            case MoveTypeAndPromotion::e:              \
                ex = #e;                                           \
                break

            C(REGULAR_PAWN_MOVE);
            C(REGULAR_PAWN_CAPTURE);
            C(REGULAR_KNIGHT_MOVE);
            C(REGULAR_BISHOP_MOVE);
            C(REGULAR_ROOK_MOVE);
            C(REGULAR_QUEEN_MOVE);
            C(REGULAR_KING_MOVE);
            C(EN_PASSANT);
            C(CASTLING_SHORT);
            C(CASTLING_LONG);
            C(PROMO_KNIGHT);
            C(PROMO_BISHOP);
            C(PROMO_ROOK);
            C(PROMO_QUEEN);
            C(ILLEGAL);

            default:
                ex = "??";
                break;
        }

        EXPECT_EQ(ex, StringUtils::moveTypeAndPromotionToString(tap)) << "MoveTypeAndPromotion index " << i;
    }
}

TEST(StringUtils, pieceAndColorToString)
{
    for (std::size_t i { }; i < 256U; ++i)
    {
        PieceAndColor pc { static_cast<PieceAndColor>(i) };
        std::string_view ex;

        switch (pc)
        {
            case PieceAndColor::WHITE_PAWN:
                ex = "wP";
                break;

            case PieceAndColor::WHITE_KNIGHT:
                ex = "wN";
                break;

            case PieceAndColor::WHITE_BISHOP:
                ex = "wB";
                break;

            case PieceAndColor::WHITE_ROOK:
                ex = "wR";
                break;

            case PieceAndColor::WHITE_QUEEN:
                ex = "wQ";
                break;

            case PieceAndColor::WHITE_KING:
                ex = "wK";
                break;

            case PieceAndColor::BLACK_PAWN:
                ex = "bP";
                break;

            case PieceAndColor::BLACK_KNIGHT:
                ex = "bN";
                break;

            case PieceAndColor::BLACK_BISHOP:
                ex = "bB";
                break;

            case PieceAndColor::BLACK_ROOK:
                ex = "bR";
                break;

            case PieceAndColor::BLACK_QUEEN:
                ex = "bQ";
                break;

            case PieceAndColor::BLACK_KING:
                ex = "bK";
                break;

            case PieceAndColor::WHITE_NONE:
            case PieceAndColor::BLACK_NONE:
                ex = "  ";
                break;

            default:
                ex = "??";
                break;
        }

        EXPECT_EQ(ex, StringUtils::pieceAndColorToString(pc)) << "Piece and color index " << i;
    }
}

namespace
{

auto posAndMoveToSan(
    const char *fen,
    Square src,
    Square dest,
    MoveTypeAndPromotion typeAndPromotion)
{
    ChessBoard board;
    board.loadFEN(fen);
    return StringUtils::moveToSan(board, Move { src, dest, typeAndPromotion });
}

}

TEST(StringUtils, moveToSan)
{
    //// PAWN MOVES
    // regular pawn move
    EXPECT_EQ(
        posAndMoveToSan(
            "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
            Square::E2, Square::E4, MoveTypeAndPromotion::REGULAR_PAWN_MOVE).getStringView(),
        std::string_view("e4"));

    // regular pawn capture
    EXPECT_EQ(
        posAndMoveToSan(
            "rnbqkbnr/ppp1pppp/8/3p4/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 2",
            Square::E4, Square::D5, MoveTypeAndPromotion::REGULAR_PAWN_CAPTURE).getStringView(),
        std::string_view("exd5"));

    // en passant pawn capture
    EXPECT_EQ(
        posAndMoveToSan(
            "rnbqkbnr/pp2pppp/8/2ppP3/8/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 3",
            Square::E5, Square::D6, MoveTypeAndPromotion::EN_PASSANT).getStringView(),
        std::string_view("exd6"));

    // non-capturing promotions
    EXPECT_EQ(
        posAndMoveToSan(
            "8/5P2/8/8/8/8/8/1k1K4 w - - 0 1",
            Square::F7, Square::F8, MoveTypeAndPromotion::PROMO_KNIGHT).getStringView(),
        std::string_view("f8=N"));

    EXPECT_EQ(
        posAndMoveToSan(
            "8/5P2/8/8/8/8/8/1k1K4 w - - 0 1",
            Square::F7, Square::F8, MoveTypeAndPromotion::PROMO_BISHOP).getStringView(),
        std::string_view("f8=B"));

    EXPECT_EQ(
        posAndMoveToSan(
            "8/5P2/8/8/8/8/8/1k1K4 w - - 0 1",
            Square::F7, Square::F8, MoveTypeAndPromotion::PROMO_ROOK).getStringView(),
        std::string_view("f8=R"));

    EXPECT_EQ(
        posAndMoveToSan(
            "8/5P2/8/8/8/8/8/1k1K4 w - - 0 1",
            Square::F7, Square::F8, MoveTypeAndPromotion::PROMO_QUEEN).getStringView(),
        std::string_view("f8=Q"));

    // capturing promotions
    EXPECT_EQ(
        posAndMoveToSan(
            "4q3/5P2/8/8/8/8/8/1k1K4 w - - 0 1",
            Square::F7, Square::E8, MoveTypeAndPromotion::PROMO_KNIGHT).getStringView(),
        std::string_view("fxe8=N"));

    EXPECT_EQ(
        posAndMoveToSan(
            "4q3/5P2/8/8/8/8/8/1k1K4 w - - 0 1",
            Square::F7, Square::E8, MoveTypeAndPromotion::PROMO_BISHOP).getStringView(),
        std::string_view("fxe8=B"));

    EXPECT_EQ(
        posAndMoveToSan(
            "4q3/5P2/8/8/8/8/8/1k1K4 w - - 0 1",
            Square::F7, Square::E8, MoveTypeAndPromotion::PROMO_ROOK).getStringView(),
        std::string_view("fxe8=R"));

    EXPECT_EQ(
        posAndMoveToSan(
            "4q3/5P2/8/8/8/8/8/1k1K4 w - - 0 1",
            Square::F7, Square::E8, MoveTypeAndPromotion::PROMO_QUEEN).getStringView(),
        std::string_view("fxe8=Q"));

    //// Piece moves
    // non-capturing
    EXPECT_EQ(
        posAndMoveToSan(
            "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
            Square::G1, Square::F3, MoveTypeAndPromotion::REGULAR_KNIGHT_MOVE).getStringView(),
        std::string_view("Nf3"));

    EXPECT_EQ(
        posAndMoveToSan(
            "rnbqkbnr/ppp1pppp/8/3p4/8/6P1/PPPPPP1P/RNBQKBNR w KQkq - 0 2",
            Square::F1, Square::G2, MoveTypeAndPromotion::REGULAR_BISHOP_MOVE).getStringView(),
        std::string_view("Bg2"));


    EXPECT_EQ(
        posAndMoveToSan(
            "rnbqkb1r/pppppppp/5n2/8/8/5N2/PPPPPPPP/RNBQKB1R w KQkq - 2 2",
            Square::H1, Square::G1, MoveTypeAndPromotion::REGULAR_ROOK_MOVE).getStringView(),
        std::string_view("Rg1"));


    EXPECT_EQ(
        posAndMoveToSan(
            "rnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 2",
            Square::D1, Square::F3, MoveTypeAndPromotion::REGULAR_QUEEN_MOVE).getStringView(),
        std::string_view("Qf3"));

    EXPECT_EQ(
        posAndMoveToSan(
            "8/8/8/6n1/8/5N2/8/1k1K4 w - - 0 1",
            Square::F3, Square::G5, MoveTypeAndPromotion::REGULAR_KNIGHT_MOVE).getStringView(),
        std::string_view("Nxg5"));

    // no disambiguation
    EXPECT_EQ(
        posAndMoveToSan(
            "8/3N4/8/8/8/3N1N2/8/1k1K4 w - - 0 1",
            Square::F3, Square::D4, MoveTypeAndPromotion::REGULAR_KNIGHT_MOVE).getStringView(),
        std::string_view("Nd4"));

    // source column disambiguation
    EXPECT_EQ(
        posAndMoveToSan(
            "8/3N4/8/8/8/3N1N2/8/1k1K4 w - - 0 1",
            Square::F3, Square::E1, MoveTypeAndPromotion::REGULAR_KNIGHT_MOVE).getStringView(),
        std::string_view("Nfe1"));
    EXPECT_EQ(
        posAndMoveToSan(
            "8/3N4/8/8/8/5N2/8/1k1K4 w - - 0 1",
            Square::F3, Square::E5, MoveTypeAndPromotion::REGULAR_KNIGHT_MOVE).getStringView(),
        std::string_view("Nfe5"));

    // source row disambiguation
    EXPECT_EQ(
        posAndMoveToSan(
            "8/3N4/8/8/8/3N1N2/8/1k1K4 w - - 0 1",
            Square::D3, Square::C5, MoveTypeAndPromotion::REGULAR_KNIGHT_MOVE).getStringView(),
        std::string_view("N3c5"));

    // source square disambiguation
    EXPECT_EQ(
        posAndMoveToSan(
            "8/3N4/8/8/8/3N1N2/8/1k1K4 w - - 0 1",
            Square::D3, Square::E5, MoveTypeAndPromotion::REGULAR_KNIGHT_MOVE).getStringView(),
        std::string_view("Nd3e5"));

    // King moves
    EXPECT_EQ(
        posAndMoveToSan(
            "1k6/8/8/8/3qK3/8/8/8 w - - 0 1",
            Square::E4, Square::F5, MoveTypeAndPromotion::REGULAR_KING_MOVE).getStringView(),
        std::string_view("Kf5"));
    EXPECT_EQ(
        posAndMoveToSan(
            "1k6/8/8/8/3qK3/8/8/8 w - - 0 1",
            Square::E4, Square::D4, MoveTypeAndPromotion::REGULAR_KING_MOVE).getStringView(),
        std::string_view("Kxd4"));

    // Castling
    EXPECT_EQ(
        posAndMoveToSan(
            "rnbqk2r/ppppppbp/5np1/8/8/5NP1/PPPPPPBP/RNBQK2R w KQkq - 2 4",
            Square::E1, Square::H1, MoveTypeAndPromotion::CASTLING_SHORT).getStringView(),
        std::string_view("O-O"));
    EXPECT_EQ(
        posAndMoveToSan(
            "r3kbnr/pppqpppp/2n5/3p1b2/3P1B2/2N5/PPPQPPPP/R3KBNR w KQkq - 6 5",
            Square::E1, Square::A1, MoveTypeAndPromotion::CASTLING_LONG).getStringView(),
        std::string_view("O-O-O"));

    // Check/mate indicator
    EXPECT_EQ(
        posAndMoveToSan(
            "rnbqkbnr/ppppp1pp/8/5p2/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 2",
            Square::D1, Square::H5, MoveTypeAndPromotion::REGULAR_QUEEN_MOVE).getStringView(),
        std::string_view("Qh5+"));
    EXPECT_EQ(
        posAndMoveToSan(
            "rnbqkbnr/pppp1ppp/4p3/8/5PP1/8/PPPPP2P/RNBQKBNR b KQkq - 0 2",
            Square::D8, Square::H4, MoveTypeAndPromotion::REGULAR_QUEEN_MOVE).getStringView(),
        std::string_view("Qh4#"));

    // Longest possible moves
    EXPECT_EQ(
        posAndMoveToSan(
            "3k4/Q2p3Q/8/8/Q2Q2Q1/8/8/3K4 w - - 0 1",
            Square::A4, Square::D7, MoveTypeAndPromotion::REGULAR_QUEEN_MOVE).getStringView(),
        std::string_view("Qa4xd7#"));
    EXPECT_EQ(
        posAndMoveToSan(
            "2nk4/3P4/8/8/3K4/8/8/8 w - - 0 1",
            Square::D7, Square::C8, MoveTypeAndPromotion::PROMO_ROOK).getStringView(),
        std::string_view("dxc8=R+"));
}

#define TEST_EXPECT_THROW_PGN_ERROR(st, code)   \
    EXPECT_THROW(                               \
        try {                                   \
            st;                                 \
        }                                       \
        catch (const PgnError &e)               \
        {                                       \
            printf("- %s\n", e.what());         \
            EXPECT_EQ(e.getCode(), code);       \
            throw;                              \
        }, PgnError)                            \

TEST(StringUtils, moveToSan_illegal)
{
    TEST_EXPECT_THROW_PGN_ERROR(
        posAndMoveToSan(
            "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
            Square::E2, Square::E5, MoveTypeAndPromotion::REGULAR_PAWN_MOVE),
        PgnErrorCode::ILLEGAL_MOVE);

    EXPECT_THROW(
        posAndMoveToSan(
            "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
            Square::E2, Square::E5, MoveTypeAndPromotion::ILLEGAL),
        std::logic_error);
}

TEST(StringUtils, boardToFEN)
{
    ChessBoard board { };
    FenString fen { MiniString_Uninitialized { } };

    StringUtils::boardToFEN(board, fen);

    EXPECT_EQ(
        std::string_view { "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1" },
        fen.getStringView());
}

}
