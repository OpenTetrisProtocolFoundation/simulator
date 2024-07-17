#include <obpf/simulator.h>
#include <spdlog/spdlog.h>
#include <gsl/gsl>
#include <memory>
#include <simulator/matrix.hpp>
#include <simulator/tetrion.hpp>
#include <simulator/tetromino.hpp>

enum class TetrominoSelection {
    ActiveTetromino,
    GhostTetromino,
};

[[nodiscard]] static bool try_get_tetromino(
    ObpfTetrion const* const tetrion,
    ObpfTetromino* const out_tetromino,
    TetrominoSelection const selection
) {
    auto const tetromino =
        (selection == TetrominoSelection::ActiveTetromino ? tetrion->active_tetromino() : tetrion->ghost_tetromino());

    if (not tetromino.has_value()) {
        return false;
    }
    auto const& mino_positions = get_mino_positions(tetromino.value());
    auto const result = ObpfTetromino{
        .mino_positions = {
            ObpfVec2{ gsl::narrow<std::uint8_t>(mino_positions.at(0).x), gsl::narrow<std::uint8_t>(mino_positions.at(0).y), },
            ObpfVec2{ gsl::narrow<std::uint8_t>(mino_positions.at(1).x), gsl::narrow<std::uint8_t>(mino_positions.at(1).y), },
            ObpfVec2{ gsl::narrow<std::uint8_t>(mino_positions.at(2).x), gsl::narrow<std::uint8_t>(mino_positions.at(2).y), },
            ObpfVec2{ gsl::narrow<std::uint8_t>(mino_positions.at(3).x), gsl::narrow<std::uint8_t>(mino_positions.at(3).y), },
        },
        .type = static_cast<ObpfTetrominoType>(tetromino->type),
    };
    *out_tetromino = result;
    return true;
}

ObpfTetrion* obpf_create_tetrion(uint64_t const seed) {
    return new ObpfTetrion{ seed };
}

ObpfLineClearDelayState obpf_tetrion_get_line_clear_delay_state(ObpfTetrion const* tetrion) {
    auto [lines, countdown] = tetrion->line_clear_delay_state();
    auto const original_size = lines.size();
    while (lines.size() < decltype(lines)::capacity()) {
        lines.push_back(0);
    }
    return ObpfLineClearDelayState{
        .count = gsl::narrow<u8>(original_size),
        .first = lines.at(0),
        .second = lines.at(1),
        .third = lines.at(2),
        .fourth = lines.at(3),
        .countdown = countdown,
        .delay = LineClearDelay::delay,
    };
}

bool obpf_tetrion_try_get_active_tetromino(ObpfTetrion const* const tetrion, ObpfTetromino* const out_tetromino) {
    return try_get_tetromino(tetrion, out_tetromino, TetrominoSelection::ActiveTetromino);
}

bool obpf_tetrion_try_get_ghost_tetromino(ObpfTetrion const* tetrion, ObpfTetromino* out_tetromino) {
    return try_get_tetromino(tetrion, out_tetromino, TetrominoSelection::GhostTetromino);
}

void obpf_tetrion_simulate_up_until(ObpfTetrion* const tetrion, uint64_t const frame) {
    tetrion->simulate_up_until(frame);
}

void obpf_tetrion_enqueue_event(ObpfTetrion* const tetrion, ObpfEvent const event) {
    auto const e = Event{
        .key = static_cast<Key>(event.key),
        .type = static_cast<EventType>(event.type),
        .frame = event.frame,
    };
    tetrion->enqueue_event(e);
}

void obpf_destroy_tetrion(ObpfTetrion const* const tetrion) {
    delete tetrion;
}

ObpfMatrix const* obpf_tetrion_matrix(ObpfTetrion const* const tetrion) {
    return reinterpret_cast<ObpfMatrix const*>(std::addressof(tetrion->matrix()));
}

ObpfTetrominoType obpf_matrix_get(ObpfMatrix const* const matrix, ObpfVec2 const position) {
    auto const pos = Vec2{ position.x, position.y };
    return static_cast<ObpfTetrominoType>((*reinterpret_cast<Matrix const*>(matrix))[pos]);
}

uint8_t obpf_tetrion_width() {
    return uint8_t{ Matrix::width };
}

uint8_t obpf_tetrion_height() {
    return uint8_t{ Matrix::height };
}

ObpfPreviewPieces obpf_tetrion_get_preview_pieces(ObpfTetrion const* tetrion) {
    auto const preview_tetrominos = tetrion->get_preview_tetrominos();
    ObpfPreviewPieces result{};
    for (usize i = 0; i < preview_tetrominos.size(); ++i) {
        result.types[i] = static_cast<ObpfTetrominoType>(preview_tetrominos.at(i));
    }
    return result;
}

ObpfMinoPositions obpf_tetromino_get_mino_positions(ObpfTetrominoType const type, ObpfRotation const rotation) {
    auto const tetromino = Tetromino{
        Vec2{ 0, 0 },
        static_cast<Rotation>(rotation),
        static_cast<TetrominoType>(type)
    };
    auto const mino_positions = get_mino_positions(tetromino);
    ObpfMinoPositions result{};
    for (usize i = 0; i < mino_positions.size(); ++i) {
        result.positions[i] = ObpfVec2{
            gsl::narrow<std::uint8_t>(mino_positions.at(i).x),
            gsl::narrow<std::uint8_t>(mino_positions.at(i).y),
        };
    }
    return result;
}
