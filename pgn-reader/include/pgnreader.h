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

#ifndef HOOVER_CHESS_UTILS__PGN_READER__PGNREADER_H_INCLUDED
#define HOOVER_CHESS_UTILS__PGN_READER__PGNREADER_H_INCLUDED

#include "chessboard.h"
#include "pgnreader-error.h"
#include "pgnreader-types.h"

#include <cstddef>
#include <cstdint>
#include <format>
#include <string_view>

namespace hoover_chess_utils::pgn_reader
{

class ChessBoard;

/// @addtogroup PgnReaderAPI
/// @{

/// @brief Action for recoverable PGN reader error
///
/// @sa @coderef{PgnReaderActions::onError()}
enum class PgnReaderOnErrorAction
{
    /// @brief Abort processing
    Abort,

    /// @brief Continue from next game
    ContinueFromNextGame,

};

/// @brief Additional error info
///
/// @sa @coderef{PgnReaderActions::onError()}
struct PgnErrorInfo
{
    /// @brief Line number of the error. Numbering starts from 1.
    std::uint64_t lineNumber;
};

/// @brief Semantic actions for reading a PGN. The caller is expected to inherit
/// this class and override all callbacks of interest.
class PgnReaderActions
{
public:
    /// @brief Invoked when a new game starts
    ///
    /// Callback is always enabled.
    virtual void gameStart()
    {
    }

    /// @brief Invoked when a PGN tag has been read
    ///
    /// @param[in]  key     Key of the PGN tag
    /// @param[in]  value   Value of the PGN tag
    ///
    /// Callback is filtered by @coderef{PgnReaderActionClass::PgnTag}.
    virtual void pgnTag(std::string_view key, std::string_view value)
    {
        static_cast<void>(key);
        static_cast<void>(value);
    }

    /// @brief Invoked when all PGN tags of the game are read and before any
    /// moves are processed.
    ///
    /// The state of @c *curBoard is the game starting position. The state of
    /// @c *prevBoard is the same.
    ///
    /// Callback is always enabled.
    virtual void moveTextSection() { }

    /// @brief PGN comment
    ///
    /// @param[in]  comment    Comment text
    ///
    /// PGN comments may occur almost anywhere.
    ///
    /// Callback is filtered by @coderef{PgnReaderActionClass::Comment}.
    virtual void comment(std::string_view comment)
    {
        static_cast<void>(comment);
    }

    /// @brief Invoked after the game termination marker is processed.
    ///
    /// @param[in]  result    Game result
    ///
    /// Callback is always enabled.
    virtual void gameTerminated(PgnResult result)
    {
        static_cast<void>(result);
    }

    /// @brief Invoked when the PGN reader is instantiated.
    ///
    /// @param[in]  curBoard       Pointer to current board (after move)
    /// @param[in]  prevBoard      Pointer to previous board (before move)
    ///
    /// Callback is filtered by @coderef{PgnReaderActionClass::Move}.
    virtual void setBoardReferences(const ChessBoard &curBoard, const ChessBoard &prevBoard)
    {
        static_cast<void>(curBoard);
        static_cast<void>(prevBoard);
    }

    /// @brief Invoked after a move is processed.
    ///
    /// @param[in]  move     Move
    ///
    /// The state of @c *curBoard is after move.
    /// The state of @c *prevBoard is before move.
    ///
    /// Callback is filtered by @coderef{PgnReaderActionClass::Move}.
    virtual void afterMove(Move move)
    {
        static_cast<void>(move);
    }

    /// @brief Invoked after a numeric annotation glyph (NAG) is processed.
    ///
    /// @param[in] nagNum   Numeric value of the NAG.
    ///
    /// Callback is filtered by @coderef{PgnReaderActionClass::Move} and
    /// @coderef{PgnReaderActionClass::NAG} (both required).
    virtual void nag(std::uint8_t nagNum)
    {
        static_cast<void>(nagNum);
    }

    /// @brief Invoked after a variation start is processed.
    ///
    /// The state of @c *curBoard is beginning of variation (before any moves).
    /// The state of @c *prevBoard is the same.
    ///
    /// Callback is filtered by @coderef{PgnReaderActionClass::Variation}.
    virtual void variationStart()
    {
    }

    /// @brief Invoked after a variation end marker is encountered but before
    /// the board states have been restored.
    ///
    /// The state of @c *curBoard the end of variation, after the last move.
    /// The state of @c *prevBoard before the last move.
    ///
    /// Callback is filtered by @coderef{PgnReaderActionClass::Variation}.
    virtual void variationEnd()
    {
    }

    /// @brief Error handler for recoverable errors
    ///
    /// @param[in]  error             Error object
    /// @param[in]  additionalInfo    Additional information on error
    /// @return                       Recovery actions. Default: @coderef{PgnReaderOnErrorAction::Abort}
    ///
    /// Caller-provided error handler is invoked on recoverable PGN
    /// errors. Those are tokenizer (scanner) errors, parser errors, board setup
    /// errors (FEN errors), move replay errors, and other errors related to the
    /// correctness of the PGN input. When such an error occurs, this callback
    /// function is invoked, which then returns the policy action.
    ///
    /// The actions are:
    /// - @coderef{PgnReaderOnErrorAction::Abort} (default) --- Abort processing and
    ///   re-throw the PGN error exception as is. This is the default policy in
    ///   case error handler is not overridden.
    /// - @coderef{PgnReaderOnErrorAction::ContinueFromNextGame} --- Abort
    ///   processing the current game. Skip to the next game and continue from
    ///   there. Note the following:
    ///   - No further action handlers are invoked for the current game. In
    ///     particular, there will be no calls to close already started
    ///     variations, nor there will be a call to @coderef{gameTerminated()}
    ///     to indicate that the game ended.
    ///   - The exception will not be re-thrown by
    ///     @coderef{PgnReader::readFromMemory()}.
    ///
    /// Internally, the procedure to skip to the next game when
    /// @coderef{PgnReaderOnErrorAction::ContinueFromNextGame} is returned is
    /// straightforward. The tokenizer is invoked until it returns either RESULT
    /// (i.e., @c "1-0", @c "1/2-1/2", @c "0-1", @c "*") or END_OF_FILE
    /// tokens. It is possible that more than more error is returned for a
    /// single erroneous game.
    virtual PgnReaderOnErrorAction onError(const PgnError &error, const PgnErrorInfo &additionalInfo)
    {
        static_cast<void>(error);
        static_cast<void>(additionalInfo);

        return PgnReaderOnErrorAction::Abort;
    }
};

/// @brief PGN reader filterable action classes
enum class PgnReaderActionClass : unsigned int
{
    /// @brief PGN tag actions
    PgnTag = 0U,

    /// @brief Move actions, including boards
    Move,

    /// @brief Numeric annotation glyphs actions
    ///
    /// @remark Requires also @coderef{Move}.
    NAG,

    /// @brief Variation start/end, including any actions within a variation
    Variation,

    /// @brief Comment actions
    Comment,
};

/// @brief PGN reader action filter
class PgnReaderActionFilter
{
private:
    std::uint32_t m_filterBits { };

    static constexpr std::uint32_t actionBit(PgnReaderActionClass action) noexcept
    {
        const unsigned int shift { static_cast<unsigned int>(action) };
        if (shift < 32U) [[likely]]
        {
            return UINT32_C(1) << shift;
        }
        else [[unlikely]]
        {
            return 0U;
        }
    }

    constexpr void enablePack() noexcept
    {
    }

    template <typename... Args>
    constexpr void enablePack(PgnReaderActionClass action, Args... rest) noexcept
    {
        set(action, true);
        enablePack(rest...);
    }

public:
    /// @brief Constructor
    ///
    /// @tparam     Args     Argument types. Must be @coderef{PgnReaderActionClass}.
    /// @param[in]  enabled  Initially enabled actions
    template <typename... Args>
    constexpr PgnReaderActionFilter(Args... enabled) noexcept
    {
        enablePack(enabled...);
    }

    /// @brief Enables/disables an action class
    ///
    /// @param[in]  action    Action class to enable/disable
    /// @param[in]  enable    Whether to enable or disable
    constexpr inline void set(PgnReaderActionClass action, bool enable) noexcept
    {
        const std::uint32_t mask { actionBit(action) };

        if (enable)
            m_filterBits |= mask;
        else
            m_filterBits &= ~mask;
    }

    /// @brief Returns whether an action class is enabled
    ///
    /// @param[in]  action    Action class
    /// return                Status
    constexpr inline bool isEnabled(PgnReaderActionClass action) noexcept
    {
        return (m_filterBits & actionBit(action)) != 0U;
    }

    /// @brief Returns a bit mask of enabled action classes. When bit N is set,
    /// action class with numeric value @c N is enabled.
    ///
    /// return                Bit mask
    constexpr inline std::uint32_t getBitMask() noexcept
    {
        return m_filterBits;
    }
};

/// @brief The PGN reader interface
class PgnReader
{
public:
    /// @brief Reads and processes a PGN from memory
    ///
    /// @param[in]  pgn          PGN contents
    /// @param[in]  actions      Semantic action callbacks
    /// @param[in]  filter       Filter of enabled action classes
    /// @throws PgnError         PGN processing failed
    ///
    /// This function reads and processes a PGN. Action callbacks of @c actions
    /// are invoked based on the action class filter and the PGN contents. For
    /// instance, after a move is processed,
    /// @coderef{PgnReaderActions::afterMove()} is invoked if
    /// @coderef{PgnReaderActionClass::Move} is enabled @c filter.
    ///
    /// The moves of the PGN are validated only when
    /// @coderef{PgnReaderActionClass::Move} is enabled.
    ///
    /// Disabling unused callbacks may improve performance.
    ///
    /// In case an error occurs when reading the PGN,
    /// @coderef{PgnReaderActions::onError()} is invoked. The return value of
    /// the caller-provided error handler specifies whether an attempt is made
    /// to continue. See the @c onError() documentation for details.
    static void readFromMemory(std::string_view pgn, PgnReaderActions &actions, PgnReaderActionFilter filter);
};

/// @}

}

#endif
