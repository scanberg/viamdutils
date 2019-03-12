#pragma once

#include <core/types.h>
#include <core/array_types.h>
#include <core/vector_types.h>
#include <core/string_utils.h>
#include <core/common.h>

struct TrajectoryFrame {
    int32 index = 0;
    float32 time = 0;
    mat3 box{};
    struct {
        float* x = nullptr;
        float* y = nullptr;
        float* z = nullptr;
    } atom_position;
};

struct MoleculeTrajectory {
    enum Type { NVT, NPT };

    int32 num_atoms = 0;
    int32 num_frames = 0;
    float32 total_simulation_time = 0;
    Type simulation_type = NVT;

    String path_to_file{};
    void* file_handle = nullptr;

    // @NOTE: The frame_buffer may not contain all frames in trajectory.
    // If the trajectory is large, frame_buffer will be used as a cache towards the trajectory streamed from disk.
    Array<TrajectoryFrame> frame_buffer{};

    // This is the position data of the full trajectory
    struct {
        float* x = nullptr;
        float* y = nullptr;
        float* z = nullptr;
    } position_data;

    // These are the offsets for each frame within the compressed blob of XTC data
    Array<int64> frame_offsets{};

    operator bool() const { return num_atoms > 0 && frame_buffer.count > 0; }
};

// Allocates memory and initializes trajectory
bool init_trajectory(MoleculeTrajectory* traj, int32 num_atoms, int32 num_frames, float32 time_between_frames = 1.0f, const mat3& sim_box = {});

// Frees memory allocated by trajectory
void free_trajectory(MoleculeTrajectory* traj);
