#pragma once

#include <core/array_types.h>
#include <core/math_utils.h>
#include <mol/molecule_structure.h>
#include <mol/molecule_trajectory.h>
#include <mol/molecule_dynamic.h>
#include <mol/aminoacid.h>

struct BackboneAnglesTrajectory {
    int num_segments = 0;
    int num_frames = 0;
    Array<vec2> angle_data{};
};

inline Array<vec2> get_backbone_angles(BackboneAnglesTrajectory& backbone_angle_traj, int frame_index) {
    if (backbone_angle_traj.angle_data.count == 0 || backbone_angle_traj.num_segments == 0) return {};
    ASSERT(frame_index < backbone_angle_traj.angle_data.count / backbone_angle_traj.num_segments);
    return Array<vec2>(&backbone_angle_traj.angle_data[frame_index * backbone_angle_traj.num_segments], backbone_angle_traj.num_segments);
}

inline Array<vec2> get_backbone_angles(BackboneAnglesTrajectory& backbone_angle_traj, int frame_offset, int frame_count) {
    if (backbone_angle_traj.angle_data.count == 0 || backbone_angle_traj.num_segments == 0) return {};
#ifdef DEBUG
    int32 num_frames = (int32)backbone_angle_traj.angle_data.count / backbone_angle_traj.num_segments;
    ASSERT(frame_offset < num_frames);
    ASSERT(frame_offset + frame_count <= num_frames);
#endif
    return backbone_angle_traj.angle_data.subarray(frame_offset * backbone_angle_traj.num_segments, frame_count * backbone_angle_traj.num_segments);
}

inline int32 get_backbone_angles_trajectory_current_frame_count(const BackboneAnglesTrajectory& backbone_angle_traj) {
    if (backbone_angle_traj.angle_data.count == 0 || backbone_angle_traj.num_segments == 0) return 0;
    return (int32)backbone_angle_traj.angle_data.count / backbone_angle_traj.num_segments;
}

inline Array<vec2> get_backbone_angles(BackboneAnglesTrajectory& backbone_angle_traj, int frame_index, Chain chain) {
    return get_backbone_angles(backbone_angle_traj, frame_index).subarray(chain.res_idx.beg, chain.res_idx.end - chain.res_idx.end);
}

DynamicArray<BackboneSequence> compute_backbone_sequences(Array<const BackboneSegment> segments, Array<const Residue> residues);
DynamicArray<BackboneSegment> compute_backbone_segments(Array<const Residue> residues, Array<const Label> atom_labels);
// DynamicArray<SplineSegment> compute_spline(Array<const vec3> atom_pos, Array<const uint32> colors, Array<const BackboneSegment> backbone, int32 num_subdivisions = 1, float tension = 0.5f);

// Computes the dihedral angles within the backbone:
// phi   = dihedral( C[i-1], N[i],  CA[i],  C[i])
// psi   = dihedral( N[i],  CA[i],   C[i],  N[i+1])
// As seen here https://en.wikipedia.org/wiki/Ramachandran_plot.
DynamicArray<BackboneAngle> compute_backbone_angles(Array<const BackboneSegment> backbone_segments, const float* pos_x, const float* pos_y, const float* pos_z);
void compute_backbone_angles(Array<BackboneAngle> dst, Array<const BackboneSegment> backbone_segments, const float* pos_x, const float* pos_y, const float* pos_z);

DynamicArray<BackboneAngle> compute_backbone_angles(Array<const BackboneSegment> segments, Array<const BackboneSequence> sequences, const float* pos_x, const float* pos_y, const float* pos_z);
void compute_backbone_angles(Array<BackboneAngle> dst, Array<const BackboneSegment> segments, Array<const BackboneSequence> sequences, const float* pos_x, const float* pos_y, const float* pos_z);

void init_backbone_angles_trajectory(BackboneAnglesTrajectory* data, const MoleculeDynamic& dynamic);
void free_backbone_angles_trajectory(BackboneAnglesTrajectory* data);
void compute_backbone_angles_trajectory(BackboneAnglesTrajectory* bb_angle_traj, const MoleculeDynamic& dynamic);

template <typename T>
int64 extract_data_from_mask(T* RESTRICT dst_data, const T* RESTRICT src_data, const bool* RESTRICT src_mask, int64 src_count) {
    int64 dst_count = 0;
    for (int64 i = 0; i < src_count; i++) {
        if (src_mask[i]) {
            dst_data[out_count] = src_data[i];
        }
    }
    return dst_count;
}

void translate_positions(float* RESTRICT pos_x, float* RESTRICT pos_y, float* RESTRICT pos_z, int64 count, const vec3& translation);
void transform_positions(float* RESTRICT pos_x, float* RESTRICT pos_y, float* RESTRICT pos_z, int64 count, const mat4& transformation);

void compute_bounding_box(vec3* out_min_box, vec3* out_max_box, const float* RESTRICT pos_x, const float* RESTRICT pos_y, const float* RESTRICT pos_z, int64 count);
void compute_bounding_box(vec3* out_min_box, vec3* out_max_box, const float* RESTRICT pos_x, const float* RESTRICT pos_y, const float* RESTRICT pos_z, const float* radii, int64 count);

vec3 compute_com(const float* RESTRICT pos_x, const float* RESTRICT pos_y, const float* RESTRICT pos_z, int64 count);
vec3 compute_com(const float* RESTRICT pos_x, const float* RESTRICT pos_y, const float* RESTRICT pos_z, const float* RESTRICT mass, int64 count);
vec3 compute_com(const float* RESTRICT pos_x, const float* RESTRICT pos_y, const float* RESTRICT pos_z, const Element* RESTRICT element, int64 count);

void linear_interpolation(float* RESTRICT out_x, float* RESTRICT out_y, float* RESTRICT out_z,
						  const float* RESTRICT in_x0, const float* RESTRICT in_y0, const float* RESTRICT in_z0,
						  const float* RESTRICT in_x1, const float* RESTRICT in_y1, const float* RESTRICT in_z1,
						  int64 count, float t);

void linear_interpolation_pbc(float* RESTRICT out_x, float* RESTRICT out_y, float* RESTRICT out_z,
							  const float* RESTRICT p0_x, const float* RESTRICT p0_y, const float* RESTRICT p0_z,
							  const float* RESTRICT p1_x, const float* RESTRICT p1_y, const float* RESTRICT p1_z,
							  int64 count, float t, const mat3& sim_box);

void cubic_interpolation(float* RESTRICT out_x, float* RESTRICT out_y, float* RESTRICT out_z,
						 const float* RESTRICT p0_x, const float* RESTRICT p0_y, const float* RESTRICT p0_z,
						 const float* RESTRICT p1_x, const float* RESTRICT p1_y, const float* RESTRICT p1_z,
						 const float* RESTRICT p2_x, const float* RESTRICT p2_y, const float* RESTRICT p2_z,
						 const float* RESTRICT p3_x, const float* RESTRICT p3_y, const float* RESTRICT p3_z,
						 int64 count, float t);

void cubic_interpolation_pbc_scalar(float* RESTRICT out_x, float* RESTRICT out_y, float* RESTRICT out_z,
									const float* RESTRICT p0_x, const float* RESTRICT p0_y, const float* RESTRICT p0_z,
									const float* RESTRICT p1_x, const float* RESTRICT p1_y, const float* RESTRICT p1_z,
									const float* RESTRICT p2_x, const float* RESTRICT p2_y, const float* RESTRICT p2_z,
									const float* RESTRICT p3_x, const float* RESTRICT p3_y, const float* RESTRICT p3_z,
									int64 count, float t, const mat3& sim_box);

void cubic_interpolation_pbc(float* RESTRICT out_x, float* RESTRICT out_y, float* RESTRICT out_z,
							 const float* RESTRICT p0_x, const float* RESTRICT p0_y, const float* RESTRICT p0_z,
							 const float* RESTRICT p1_x, const float* RESTRICT p1_y, const float* RESTRICT p1_z,
                             const float* RESTRICT p2_x, const float* RESTRICT p2_y, const float* RESTRICT p2_z,
							 const float* RESTRICT p3_x, const float* RESTRICT p3_y, const float* RESTRICT p3_z,
							 int64 count, float t, const mat3& sim_box);

void cubic_interpolation_pbc_256(float* RESTRICT out_x, float* RESTRICT out_y, float* RESTRICT out_z,
								 const float* RESTRICT p0_x, const float* RESTRICT p0_y, const float* RESTRICT p0_z,
								 const float* RESTRICT p1_x, const float* RESTRICT p1_y, const float* RESTRICT p1_z,
								 const float* RESTRICT p2_x, const float* RESTRICT p2_y, const float* RESTRICT p2_z,
								 const float* RESTRICT p3_x, const float* RESTRICT p3_y, const float* RESTRICT p3_z,
								 int64 count, float t, const mat3& sim_box);

void compute_velocities(float* RESTRICT out_x, float* RESTRICT out_y, float* RESTRICT out_z,
						const float* RESTRICT in_x0, const float* RESTRICT in_y0, const float* RESTRICT in_z0,
						const float* RESTRICT in_x1, const float* RESTRICT in_y1, const float* RESTRICT in_z1,
						int64 count, float dt);

void compute_velocities_pbc(float* RESTRICT out_x, float* RESTRICT out_y, float* RESTRICT out_z,
							const float* RESTRICT prv_x, const float* RESTRICT prv_y, const float* RESTRICT prv_z,
							const float* RESTRICT cur_x, const float* RESTRICT cur_y, const float* RESTRICT cur_z,
                            int64 count, float dt, const mat3& sim_box);

inline vec3 apply_pbc(const vec3& pos, const mat3& sim_box) {
    const vec3 ext = sim_box * vec3(1, 1, 1);
    vec3 out;
    for (int i = 0; i < 3; i++) {
        out[i] = (pos[i] > ext[i]) ? (pos[i] - ext[i]) : (pos[i]);
    }
    return out;
}

void apply_pbc_atoms(float* pos_x, float* pos_y, float* pos_z, int64 count, const mat3& sim_box);
void apply_pbc_residues(float* pos_x, float* pos_y, float* pos_z, Array<const Residue> residues, const mat3& sim_box);
void apply_pbc_chains(float* pos_x, float* pos_y, float* pos_z, Array<const Chain> chains, const mat3& sim_box);

void recenter_trajectory(MoleculeDynamic* dynamic, ResIdx center_res_idx);

// This computes heuristical covalent bonds in a hierarchical way (first internal, then external per residue) and stores the indices to the bonds
// within the residues. Only adjacent residues can form external covalent bonds in this function.
DynamicArray<Bond> compute_covalent_bonds(Array<Residue> residues, const float* pos_x, const float* pos_y, const float* pos_z, const ResIdx* res_idx, const Element* element, int64 count);

// This is computes heuristical covalent bonds between any atoms without hierarchical constraints.
DynamicArray<Bond> compute_covalent_bonds(const float* pos_x, const float* pos_y, const float* pos_z, const Element* element, int64 count);

bool has_covalent_bond(const Residue& res_a, const Residue& res_b);
bool valid_segment(const BackboneSegment& seg);

DynamicArray<Chain> compute_chains(Array<const Residue> residue);

DynamicArray<float> compute_atom_radii(Array<const Element> elements);
void compute_atom_radii(float* out_radii, const Element* element, int64 count);

DynamicArray<float> compute_atom_masses(Array<const Element> elements);
void compute_atom_masses(float* out_mass, const Element* element, int64 count);

bool is_amino_acid(const Residue& res);
bool is_dna(const Residue& res);
