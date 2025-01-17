#pragma once

#include <common/common.h>
#include <array>
#include <cstdint>
#include <deque>
#include <lib2k/random.hpp>
#include <lib2k/static_vector.hpp>
#include <lib2k/types.hpp>
#include <numeric>
#include <optional>
#include <ranges>
#include <vector>
#include "action.hpp"
#include "bag.hpp"
#include "delayed_auto_shift.hpp"
#include "entry_delay.hpp"
#include "garbage.hpp"
#include "input.hpp"
#include "key_state.hpp"
#include "line_clear_delay.hpp"
#include "lock_delay.hpp"
#include "matrix.hpp"
#include "tetromino.hpp"

struct ObserverTetrion;

struct ObpfTetrion {
    static constexpr auto garbage_delay_frames = u64{ 10 * 60 };

private:
    static constexpr auto spawn_position = Vec2{ 3, 0 };
    static constexpr auto spawn_rotation = Rotation::North;

    ObpfActionHandler m_action_handler = nullptr;
    void* m_action_handler_user_data = nullptr;
    Matrix m_matrix;
    std::optional<Tetromino> m_active_tetromino;
    std::optional<Tetromino> m_ghost_tetromino;
    std::optional<TetrominoType> m_hold_piece;
    std::optional<TetrominoType> m_old_hold_piece;
    bool m_is_hold_possible = true;
    u64 m_start_frame;
    u64 m_next_frame = 0;
    KeyState m_last_key_state;
    std::mt19937_64 m_bags_rng;
    std::array<Bag, 2> m_bags;
    std::mt19937_64 m_garbage_rng;
    usize m_bag_index = 0;
    DelayedAutoShiftState m_auto_shift_state;
    LockDelayState m_lock_delay_state;
    EntryDelay m_entry_delay;
    LineClearDelay m_line_clear_delay;
    u32 m_num_lines_cleared = 0;
    u64 m_score = 0;
    u64 m_next_gravity_frame = gravity_delay_by_level(0);  // todo: offset by starting frame given by the server
    bool m_is_soft_dropping = false;
    std::optional<u64> m_game_over_since_frame;
    std::deque<GarbageSendEvent> m_garbage_receive_queue;
    std::string m_player_name;

    static constexpr u64 gravity_delay_by_level(u32 const level) {
        constexpr auto delays = std::array<u64, 13>{
            60, 48, 37, 28, 21, 16, 11, 8, 6, 4, 3, 2, 1,
        };
        return delays.at(std::min(static_cast<usize>(level), delays.size() - 1));
    }

public:
    enum class DownMovementType {
        Gravity,
        SoftDrop,
    };

    explicit ObpfTetrion(u64 const seed, u64 const start_frame, std::string player_name = "https://twitch.tv/coder2k")
        : m_start_frame{ start_frame },
          m_bags_rng{ seed },
          m_bags{ create_two_bags(m_bags_rng) },
          m_garbage_rng{ seed },
          m_player_name{ std::move(player_name) } {
        static_assert(std::same_as<std::remove_const_t<decltype(seed)>, c2k::Random::Seed>);
    }

    ObpfTetrion(ObpfTetrion const& other) = default;
    ObpfTetrion(ObpfTetrion&& other) noexcept = default;
    ObpfTetrion& operator=(ObpfTetrion const& other) = default;
    ObpfTetrion& operator=(ObpfTetrion&& other) noexcept = default;
    virtual ~ObpfTetrion() = default;

    void set_action_handler(ObpfActionHandler const handler, void* const user_data) {
        m_action_handler = handler;
        m_action_handler_user_data = user_data;
    }

    [[nodiscard]] virtual u8 id() const {
        return 0;
    }

    [[nodiscard]] Matrix& matrix() {
        return m_matrix;
    }

    [[nodiscard]] Matrix const& matrix() const {
        return m_matrix;
    }

    [[nodiscard]] std::optional<Tetromino> active_tetromino() const {
        return m_active_tetromino;
    }

    [[nodiscard]] std::optional<Tetromino> ghost_tetromino() const {
        return m_ghost_tetromino;
    }

    void apply_expired_garbage();
    [[nodiscard]] virtual std::optional<GarbageSendEvent> simulate_next_frame(KeyState key_state);
    [[nodiscard]] virtual std::vector<ObserverTetrion*> get_observers() const;
    virtual void on_client_disconnected(u8 client_id);
    [[nodiscard]] LineClearDelay::State line_clear_delay_state() const;
    [[nodiscard]] std::array<TetrominoType, 6> get_preview_tetrominos() const;
    [[nodiscard]] std::optional<TetrominoType> hold_piece() const;

    void receive_garbage(GarbageSendEvent garbage);

    [[nodiscard]] u64 next_frame() const {
        return m_next_frame;
    }

    [[nodiscard]] u32 level() const;

    [[nodiscard]] u64 score() const {
        return m_score;
    }

    [[nodiscard]] u32 num_lines_cleared() const {
        return m_num_lines_cleared;
    }

    [[nodiscard]] std::optional<u64> game_over_since_frame() const {
        return m_game_over_since_frame;
    }

    [[nodiscard]] virtual bool is_observer() const {
        return false;
    }

    [[nodiscard]] virtual bool is_connected() const {
        return true;
    }

    [[nodiscard]] u64 frames_until_game_start() const {
        if (m_next_frame >= m_start_frame) {
            return 0;
        }
        return m_start_frame - m_next_frame;
    }

    [[nodiscard]] u32 garbage_queue_length() const {
        auto const range =
            m_garbage_receive_queue | std::views::transform([](auto const& event) { return event.num_lines; });
        return std::accumulate(range.begin(), range.end(), u32{ 0 });
    }

    [[nodiscard]] usize garbage_queue_num_events() const {
        return m_garbage_receive_queue.size();
    }

    [[nodiscard]] GarbageSendEvent garbage_queue_event(usize const index) const {
        return m_garbage_receive_queue.at(index);
    }

    [[nodiscard]] std::string const& player_name() const {
        return m_player_name;
    }

private:
    void freeze_and_destroy_active_tetromino();
    [[nodiscard]] bool is_tetromino_completely_invisible(Tetromino const& tetromino) const;
    [[nodiscard]] bool is_tetromino_completely_visible(Tetromino const& tetromino) const;
    [[nodiscard]] bool is_tetromino_position_valid(Tetromino const& tetromino) const;
    [[nodiscard]] bool is_active_tetromino_position_valid() const;
    void spawn_next_tetromino();
    void process_keys(KeyState key_state);
    void handle_key_press(Key key);
    void handle_key_release(Key key);
    void move_left();
    void move_right();
    void move_down(DownMovementType movement_type);
    void rotate(RotationDirection direction);
    void rotate_clockwise();
    void rotate_counter_clockwise();
    void hard_drop();
    void hold();
    [[nodiscard]] bool determine_lines_to_clear();
    [[nodiscard]] u64 score_for_num_lines_cleared(std::size_t num_lines_cleared) const;
    void clear_lines(c2k::StaticVector<u8, 4> lines);
    void refresh_ghost_tetromino();
    void on_touch_event() const;

    [[nodiscard]] bool is_game_over() const {
        return m_game_over_since_frame.has_value();
    }

    [[nodiscard]] static std::array<Bag, 2> create_two_bags(std::mt19937_64& random);
};
