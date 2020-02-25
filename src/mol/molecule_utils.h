#pragma once

#include <core/bitfield.h>
#include <core/array_types.h>
#include <core/math_utils.h>
#include <mol/molecule_structure.h>
#include <mol/molecule_trajectory.h>
#include <mol/molecule_dynamic.h>
#include <mol/aminoacid.h>

struct AABB {
    vec3 min = {};
    vec3 max = {};
    vec3 ext() const { return max - min; }
};

struct EigenFrame {
    vec3 vectors[3];
    float values[3];
};

DynamicArray<BackboneSequence> compute_backbone_sequences(Array<const BackboneSegment> segments, Array<const Residue> residues);
DynamicArray<BackboneSegment> compute_backbone_segments(Array<const Residue> residues, Array<const Label> atom_labels);
// DynamicArray<SplineSegment> compute_spline(Array<const vec3> atom_pos, Array<const uint32> colors, Array<const BackboneSegment> backbone, int32 num_subdivisions = 1, float tension = 0.5f);

// Computes the dihedral angles within the backbone:
// phi   = dihedral( C[i-1], N[i],  CA[i],  C[i])
// psi   = dihedral( N[i],  CA[i],   C[i],  N[i+1])
// As explained here https://en.wikipedia.org/wiki/Ramachandran_plot.
//DynamicArray<BackboneAngle> compute_backbone_angles(Array<const BackboneSegment> backbone_segments, const float* pos_x, const float* pos_y, const float* pos_z);
void compute_backbone_angles(BackboneAngle* dst, const BackboneSegment* backbone_segments, const float* pos_x, const float* pos_y, const float* pos_z, i64 num_segments);

//DynamicArray<BackboneAngle> compute_backbone_angles(Array<const BackboneSegment> segments, Array<const BackboneSequence> sequences, const float* pos_x, const float* pos_y, const float* pos_z);
//void compute_backbone_angles(BackboneAngle* dst, const BackboneSegment* segments, Array<const BackboneSequence> sequences, const float* pos_x, const float* pos_y, const float* pos_z);

void translate(float* in_out_x, float* in_out_y, float* in_out_z, i64 count, const vec3& translation);

// Transforms points as homogeneous vectors[x,y,z,w*] with supplied transformation matrix (NO perspective division is done)
// W-component is supplied by user
void transform_ref(float* in_out_x, float* in_out_y, float* in_out_z, i64 count, const mat4& transformation, float w_comp = 1.0f);

// Transforms points as homogeneous vectors[x,y,z,w*] with supplied transformation matrix (NO perspective division is done)
// W-component is supplied by user
void transform(float* in_out_x, float* in_out_y, float* in_out_z, i64 count, const mat4& transformation, float w_comp = 1.0f);
void transform(float* out_x, float* out_y, float* out_z, float* in_x, float* in_y, float* in_z, i64 count, const mat4& transformation,
               float w_comp = 1.0f);

// Transforms points as homogeneous vectors[x,y,z,1] with supplied transformation matrix and applies 'perspective' division
void homogeneous_transform(float* in_out_x, float* in_out_y, float* in_out_z, i64 count, const mat4& transformation);

// Computes the minimun spanning Axis aligned bounding box which contains all supplied points [x,y,z,(r)adius)]
AABB compute_aabb(const float* in_x, const float* in_y, const float* in_z, i64 count);
AABB compute_aabb(const float* in_x, const float* in_y, const float* in_z, const float* in_r, i64 count);

vec3 compute_com(const float* in_x, const float* in_y, const float* in_z, i64 count);
vec3 compute_com(const float* in_x, const float* in_y, const float* in_z, const float* in_mass, i64 count);
vec3 compute_com(const float* in_x, const float* in_y, const float* in_z, const Element* element, i64 count);

vec3 compute_com_periodic(const float* in_x, const float* in_y, const float* in_z, const float* in_mass, i64 count, const mat3& box);
vec3 compute_com_periodic_ref(const float* in_x, const float* in_y, const float* in_z, const float* in_mass, i64 count, const mat3& box);

mat3 compute_covariance_matrix(const float* in_x, const float* in_y, const float* in_z, const float* in_mass, i64 count, const vec3& com);

EigenFrame compute_eigen_frame(const float* in_x, const float* in_y, const float* in_z, const float* in_mass, i64 count);

// clang-format off
void linear_interpolation(float* out_x, float* out_y, float* out_z,
						  const float* in_x0, const float* in_y0, const float* in_z0,
						  const float* in_x1, const float* in_y1, const float* in_z1,
						  i64 count, float t);

void linear_interpolation_pbc(float* out_x, float* out_y, float* out_z,
							  const float* in_x0, const float* in_y0, const float* in_z0,
							  const float* in_x1, const float* in_y1, const float* in_z1,
							  i64 count, float t, const mat3& sim_box);

void cubic_interpolation(float* out_x, float* out_y, float* out_z,
						 const float* in_x0, const float* in_y0, const float* in_z0,
						 const float* in_x1, const float* in_y1, const float* in_z1,
						 const float* in_x2, const float* in_y2, const float* in_z2,
						 const float* in_x3, const float* in_y3, const float* in_z3,
						 i64 count, float t);

void cubic_interpolation_pbc(float* out_x, float* out_y, float* out_z,
							 const float* in_x0, const float* in_y0, const float* in_z0,
							 const float* in_x1, const float* in_y1, const float* in_z1,
                             const float* in_x2, const float* in_y2, const float* in_z2,
							 const float* in_x3, const float* in_y3, const float* in_z3,
							 i64 count, float t, const mat3& sim_box);
// clang-format on

inline vec3 apply_pbc(const vec3& pos, const mat3& sim_box) {
    const vec3 ext = sim_box * vec3(1.0f);
    vec3 p = math::fract(pos / ext);
    if (p.x < 0.0f) p.x += 1.0f;
    if (p.y < 0.0f) p.y += 1.0f;
    if (p.z < 0.0f) p.z += 1.0f;
    return p * ext;
}

inline vec3 apply_pbc(const vec3& pos) {
    vec3 p = math::fract(pos);
    if (p.x < 0.0f) p.x += 1.0f;
    if (p.y < 0.0f) p.y += 1.0f;
    if (p.z < 0.0f) p.z += 1.0f;
    return p;
}

void apply_pbc(float* x, float* y, float* z, const float* mass, i64 count, const mat3& sim_box);
void apply_pbc(float* x, float* y, float* z, const float* mass, const Sequence* sequences, i64 num_sequences, const mat3& sim_box);

void recenter_trajectory(MoleculeDynamic* dynamic, Bitfield atom_mask);

// This computes heuristical covalent bonds in a hierarchical way (first internal, then external per residue) and stores the indices to the bonds
// within the residues. Only adjacent residues can form external covalent bonds in this function.
DynamicArray<Bond> compute_covalent_bonds(Array<Residue> residues, const float* pos_x, const float* pos_y, const float* pos_z, const Element* element, i64 count);

// This is computes heuristical covalent bonds between any atoms without hierarchical constraints.
DynamicArray<Bond> compute_covalent_bonds(const float* pos_x, const float* pos_y, const float* pos_z, const Element* element, i64 count);

bool has_covalent_bond(const Residue& res_a, const Residue& res_b);
bool valid_segment(const BackboneSegment& seg);

DynamicArray<Sequence> compute_sequences(Array<const Residue> residue);

DynamicArray<float> compute_atom_radii(Array<const Element> elements);
void compute_atom_radii(float* out_radii, const Element* element, i64 count);

DynamicArray<float> compute_atom_masses(Array<const Element> elements);
void compute_atom_masses(float* out_mass, const Element* element, i64 count);

bool is_amino_acid(const Residue& res);
bool is_dna(const Residue& res);

DynamicArray<Label> get_unique_residue_types(const MoleculeStructure& mol);
DynamicArray<ResIdx> get_residues_by_name(const MoleculeStructure& mol, CStringView name);

DynamicArray<AtomRange> find_equivalent_structures(const MoleculeStructure& mol, AtomRange ref);
DynamicArray<int> find_equivalent_structures(const MoleculeStructure& mol, Bitfield ref_mask, int ref_offset);