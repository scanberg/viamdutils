﻿#include "pdb_utils.h"
#include <mol/element.h>
#include <mol/aminoacid.h>
#include <mol/molecule_utils.h>
#include <mol/hydrogen_bond.h>
#include <mol/trajectory_utils.h>
#include <core/string_utils.h>
#include <core/log.h>

#include <ctype.h>
#include <stdio.h>

namespace pdb {

inline CString extract_next_model(CString& pdb_string) {
    CString beg_mdl = find_string(pdb_string, "MODEL ");
    if (beg_mdl) {
		CString tmp_mdl = { beg_mdl.end(),  pdb_string.end() - beg_mdl.end() };
        CString end_mdl = find_string(tmp_mdl, "ENDMDL"); // @NOTE: The more characters as a search pattern, the merrier
        if (end_mdl) {
			// @NOTE: Only modify pdb_string if we found a complete model block.
			pdb_string = { end_mdl.end(), pdb_string.end() - end_mdl.end() };
            return {beg_mdl.beg(), end_mdl.end()};
        }
    }

    return {};
}

inline void extract_position(float* x, float* y, float* z, CString line) {
	// SLOW 
	// sscanf(line.substr(30).ptr, "%8f%8f%8f", &pos.x, &pos.y, &pos.z);

	// FASTER 🚴
	// pos.x = to_float(line.substr(30, 8));
	// pos.y = to_float(line.substr(38, 8));
	// pos.z = to_float(line.substr(46, 8));

	// FASTEST? 🏎️💨
	*x = fast_str_to_float(line.substr(30, 8));
	*y = fast_str_to_float(line.substr(38, 8));
	*z = fast_str_to_float(line.substr(46, 8));
}

inline void extract_simulation_box(mat3* box, CString line) {
	vec3 dim(to_float(line.substr(6, 9)), to_float(line.substr(15, 9)), to_float(line.substr(24, 9)));
	vec3 angles(to_float(line.substr(33, 7)), to_float(line.substr(40, 7)), to_float(line.substr(47, 7)));
	// @NOTE: If we are given a zero dim, just use unit length
	if (dim == vec3(0)) dim = vec3(1);
	(*box)[0].x = dim.x;
	(*box)[1].y = dim.y;
	(*box)[2].z = dim.z;
}

inline void extract_element(Element* element, CString line) {
	Element elem = Element::Unknown;
	if (line.size() >= 78) {
		// @NOTE: Try optional atom element field
		elem = element::get_from_string(line.substr(76, 2));
	}

	if (elem == Element::Unknown) {
		// @NOTE: Try to deduce from atom id
		const CString atom_id = line.substr(12, 4);
		const CString res_name = line.substr(17, 3);
		if (compare_n(atom_id, "CA", 2) && aminoacid::get_from_string(res_name) == AminoAcid::Unknown) {
			// @NOTE: Ambigous case where CA is probably calcium if not part of an amino acid
			elem = Element::Ca;
		}
		else {
			elem = element::get_from_string(atom_id);
		}
	}
	*element = elem;
}

inline void extract_trajectory_frame_data(TrajectoryFrame* frame, CString mdl_str) {
	ASSERT(frame);
	float* x = frame->atom_position.x;
	float* y = frame->atom_position.y;
	float* z = frame->atom_position.z;
	int32 atom_idx = 0;
	CString line;
	while (mdl_str && (line = extract_line(mdl_str))) {
		if (compare_n(line, "ATOM", 4) || compare_n(line, "HETATM", 6)) {
			extract_position(x + atom_idx, y + atom_idx, z + atom_idx, line);
			atom_idx++;
		}
		else if (compare_n(line, "CRYST1", 6)) {
			extract_simulation_box(&frame->box, line);
		}
	}
}

bool load_molecule_from_file(MoleculeStructure* mol, CString filename) {
    StringBuffer<256> zstr = filename; // Zero terminated
	FILE* file = fopen(zstr.cstr(), "rb");
    if (!file) {
        LOG_ERROR("Could not open file: %s", zstr.cstr());
        return false;
    }

    // @NOTE: We pray to the gods above and hope that one single frame will fit in this memory
    constexpr auto mem_size = MEGABYTES(32);
    void* mem = TMP_MALLOC(mem_size);
    defer{ TMP_FREE(mem); };

	auto bytes_read = fread(mem, 1, mem_size, file);
	CString pdb_str = { (uint8*)mem, (int64)bytes_read };

    return load_molecule_from_string(mol, pdb_str);
}

bool load_molecule_from_string(MoleculeStructure* mol, CString pdb_string) {
    ASSERT(mol);
    free_molecule_structure(mol);

    DynamicArray<float> pos_x;
    DynamicArray<float> pos_y;
    DynamicArray<float> pos_z;
    DynamicArray<Label> labels;
    DynamicArray<Element> elements;
    DynamicArray<ResIdx> residue_indices;
    DynamicArray<float> occupancies;
    DynamicArray<float> temp_factors;
    DynamicArray<Residue> residues;
    DynamicArray<Chain> chains;

	constexpr auto atom_reserve_size = 4096;
	constexpr auto residue_reserve_size = 128;
	constexpr auto chain_reserve_size = 64;

    pos_x.reserve(atom_reserve_size);
    pos_y.reserve(atom_reserve_size);
    pos_z.reserve(atom_reserve_size);
    labels.reserve(atom_reserve_size);
    elements.reserve(atom_reserve_size);
    residue_indices.reserve(atom_reserve_size);
    occupancies.reserve(atom_reserve_size);
    temp_factors.reserve(atom_reserve_size);
    residues.reserve(residue_reserve_size);
    chains.reserve(chain_reserve_size);

    int current_res_id = -1;
    char current_chain_id = -1;
    int num_atoms = 0;
    mat3 box(1);
    CString line;
    while (pdb_string && (line = extract_line(pdb_string))) {
        if (compare_n(line, "ATOM", 4) || compare_n(line, "HETATM", 6)) {
            vec3 pos;
			extract_position(&pos.x, &pos.y, &pos.z, line);
            pos_x.push_back(pos.x);
            pos_y.push_back(pos.y);
            pos_z.push_back(pos.z);

            labels.push_back(trim(line.substr(12, 4)));

            if (line.size() > 60) {
                occupancies.push_back(to_float(line.substr(54, 6)));
            }
            if (line.size() > 66) {
                temp_factors.push_back(to_float(line.substr(60, 6)));
            }

			Element elem;
			extract_element(&elem, line);
            elements.push_back(elem);

            auto res_id = to_int(line.substr(22, 4));
            char chain_id = line[21];

            // New Chain
            if (current_chain_id != chain_id && chain_id != ' ') {
                current_chain_id = chain_id;
                Chain chain;
                chain.res_range = {(ResIdx)residues.size(), (ResIdx)residues.size()};
                chain.atom_range = {num_atoms, num_atoms};
                chain.id = chain_id;
                chains.push_back(chain);
            }

            // New Residue
            if (res_id != current_res_id) {
                current_res_id = res_id;
                Residue res{};
                res.name = trim(line.substr(17, 3));
                res.id = res_id;
                res.chain_idx = (ChainIdx)(chains.size() - 1);
                res.atom_range = {num_atoms, num_atoms};
                residues.push_back(res);
                if (chains.size() > 0) {
                    chains.back().res_range.end++;
                }
            }
            if (residues.size() > 0) residues.back().atom_range.end++;
            if (chains.size() > 0) chains.back().atom_range.end++;

            residue_indices.push_back((ResIdx)(residues.size() - 1));
            num_atoms++;
        } else if (compare_n(line, "ENDMDL", 6) || compare_n(line, "END", 3)) {
            break;
        }
    }

    auto masses = compute_atom_masses(elements);
    auto radii = compute_atom_radii(elements);
    auto covalent_bonds = compute_covalent_bonds(residues, pos_x.data(), pos_y.data(), pos_z.data(), elements.data(), num_atoms);
    auto backbone_segments = compute_backbone_segments(residues, labels);
    auto backbone_sequences = compute_backbone_sequences(backbone_segments, residues);
    auto backbone_angles = compute_backbone_angles(backbone_segments, backbone_sequences, pos_x.data(), pos_y.data(), pos_z.data());
    auto donors = hydrogen_bond::compute_donors(elements, residue_indices, residues, covalent_bonds);
    auto acceptors = hydrogen_bond::compute_acceptors(elements);

    if (chains.size() == 0) {
        chains = compute_chains(residues);
    }

    init_molecule_structure(mol, num_atoms, (int32)covalent_bonds.count, (int32)residues.count, (int32)chains.count, (int32)backbone_segments.count, (int32)backbone_sequences.count,
                            (int32)donors.count, (int32)acceptors.count);

    // Copy data into molecule
    memcpy(mol->atom.position.x, pos_x.data(), pos_x.size_in_bytes());
    memcpy(mol->atom.position.y, pos_y.data(), pos_y.size_in_bytes());
    memcpy(mol->atom.position.z, pos_z.data(), pos_z.size_in_bytes());
    memset(mol->atom.velocity.x, 0, num_atoms * sizeof(float));
    memset(mol->atom.velocity.y, 0, num_atoms * sizeof(float));
    memset(mol->atom.velocity.z, 0, num_atoms * sizeof(float));
    memcpy(mol->atom.radius, radii.data(), num_atoms * sizeof(float));
    memcpy(mol->atom.mass, masses.data(), num_atoms * sizeof(float));
    memcpy(mol->atom.element, elements.ptr, elements.size_in_bytes());
    memcpy(mol->atom.label, labels.ptr, labels.size_in_bytes());
    memcpy(mol->atom.res_idx, residue_indices.ptr, residue_indices.size_in_bytes());

    memcpy(mol->residues.ptr, residues.ptr, residues.size_in_bytes());
    memcpy(mol->chains.ptr, chains.ptr, chains.size_in_bytes());
    memcpy(mol->covalent_bonds.ptr, covalent_bonds.ptr, covalent_bonds.size_in_bytes());
    memcpy(mol->backbone.segments.ptr, backbone_segments.ptr, backbone_segments.size_in_bytes());
    memcpy(mol->backbone.angles.ptr, backbone_angles.ptr, backbone_angles.size_in_bytes());
    memcpy(mol->backbone.sequences.ptr, backbone_sequences.ptr, backbone_sequences.size_in_bytes());
    memcpy(mol->hydrogen_bond.donors.ptr, donors.ptr, donors.size_in_bytes());
    memcpy(mol->hydrogen_bond.acceptors.ptr, acceptors.ptr, acceptors.size_in_bytes());

    return true;
}

bool load_trajectory_from_file(MoleculeTrajectory* traj, CString filename) {
	String pdb_string = allocate_and_read_textfile(filename);
	defer{ free_string(&pdb_string); };
	if (!pdb_string) {
		LOG_ERROR("Could not load pdb file");
		return false;
	}
	return load_trajectory_from_string(traj, pdb_string);
}

bool load_trajectory_from_string(MoleculeTrajectory* traj, CString pdb_string) {
	ASSERT(traj);
	free_trajectory(traj);

	CString mdl_str = extract_next_model(pdb_string);
	if (!mdl_str) {
		LOG_NOTE("Supplied string does not contain MODEL entry and is therefore not a trajectory");
		return false;
	}

	MoleculeInfo info;
	extract_molecule_info(&info, mdl_str);

	if (info.num_atoms == 0) {
		LOG_ERROR("Could not determine number of atoms in trajectory");
		return false;
	}

	// @NOTE: Search space for CRYST1 containing global simulation box parameters
	mat3 sim_box(0);
	CString box_str = { pdb_string.beg(), mdl_str.beg() - pdb_string.beg() };
	CString line;
	while ((line = extract_line(box_str))) {
		if (compare_n(line, "CRYST1", 6)) {
			extract_simulation_box(&sim_box, line);
			break;
		}
	}

	DynamicArray<CString> model_entries;
	model_entries.reserve(1024);

	do {
		model_entries.push_back(mdl_str);
	} while (pdb_string && (mdl_str = extract_next_model(pdb_string)));

	// Time between frames
	const float dt = 1.0f;
	init_trajectory(traj, info.num_atoms, (int32)model_entries.size(), dt, sim_box);
	traj->num_frames = (int32)model_entries.size();

	for (int64 i = 0; i < model_entries.size(); i++) {
		TrajectoryFrame* frame = traj->frame_buffer.data() + i;
		extract_trajectory_frame_data(frame, model_entries[i]);
	}

	return true;
}

bool init_trajectory_from_file(MoleculeTrajectory* traj, CString filename) {
	ASSERT(traj);
	free_trajectory(traj);

    StringBuffer<256> zstr = filename; // @NOTE: Zero terminate
    LOG_NOTE("Loading pdb trajectory from file: %s", zstr.cstr());
    FILE* file = fopen(zstr.cstr(), "rb");
    if (!file) {
        LOG_ERROR("Could not open file: %s", zstr.cstr());
        return false;
    }

	constexpr auto page_size = MEGABYTES(32);
    void* mem = TMP_MALLOC(2 * page_size);
    defer { TMP_FREE(mem); };
	uint8* page[2] = {(uint8*)mem, (uint8*)mem + page_size};

	auto bytes_read = fread(page[0], 1, 2 * page_size, file);
    int64 global_offset = 0;
	CString pdb_str = {page[0], (int64)bytes_read};
    CString mdl_str = extract_next_model(pdb_str);

	if (!mdl_str) {
		LOG_NOTE("File does not contain MODEL entry and is therefore not a trajectory");
		fclose(file);
		return false;
	}

	MoleculeInfo info;
	extract_molecule_info(&info, mdl_str);

	if (info.num_atoms == 0) {
		LOG_ERROR("Could not determine number of atoms in trajectory");
		fclose(file);
		return false;
	}
    
    // @NOTE: Search space for CRYST1 containing global simulation box parameters
    mat3 sim_box(0);
    CString box_str = {page[0], mdl_str.beg() - page[0]};
    CString line;
    while ((line = extract_line(box_str))) {
        if (compare_n(line, "CRYST1", 6)) {
            extract_simulation_box(&sim_box, line);
            break;
        }
    }

	DynamicArray<int64> offsets;
    do {
        offsets.push_back(global_offset + (mdl_str.ptr - page[0]));

		// @NOTE: Have we crossed the boundry to the second page
		if (mdl_str.ptr > page[1]) {
			// Copy contents of second page to first page and read in a new page...
			memcpy(page[0], page[1], page_size);
			bytes_read = fread(page[1], 1, page_size, file);
			
			// Modify pointers accordingly
			mdl_str.ptr -= page_size;
			pdb_str.ptr -= page_size;
			pdb_str.count += bytes_read;
			global_offset += page_size;
		}
	} while ((mdl_str = extract_next_model(pdb_str)));

	rewind(file);
    
    // Time between frames
    const float dt = 1.0f;
	init_trajectory(traj, info.num_atoms, (int32)offsets.size(), dt, sim_box);

	traj->file.handle = file;
	traj->file.path = allocate_string(filename);
	traj->file.tag = PDB_FILE_TAG;
	traj->num_frames = 0;
	traj->frame_offsets = allocate_array<int64>(offsets.size());
	memcpy(traj->frame_offsets.data(), offsets.data(), offsets.size_in_bytes());

	return true;
}

bool read_next_trajectory_frame(MoleculeTrajectory* traj) {
	ASSERT(traj);

	if (traj->file.handle == nullptr) {
		LOG_WARNING("No file handle is open");
		return false;
	}

	if (traj->file.tag != PDB_FILE_TAG) {
		LOG_ERROR("Wrong file tag for reading trajectory frame... Expected PDB_FILE_TAG");
		return false;
	}

	const auto num_frames = traj->frame_offsets.size();
	if (num_frames == 0) {
		LOG_WARNING("Trajectory does not contain any frames");
		return false;
	}

	const auto i = traj->num_frames;
	if (i == num_frames) {
		LOG_NOTE("Trajectory is fully loaded");
		return false;
	}
    
	int64 num_bytes = 0;
	if (num_frames == 1) {
		// @NOTE: Compute bytes of entire file
		FSEEK((FILE*)traj->file.handle, 0, SEEK_END);
		num_bytes = FTELL((FILE*)traj->file.handle) - traj->frame_offsets[0];
	}
	else {
		// @NOTE: Compute delta between frame offsets (in bytes)
		num_bytes = (i == num_frames - 1) ?
			(traj->frame_offsets[i] - traj->frame_offsets[i - 1]) :
			(traj->frame_offsets[i + 1] - traj->frame_offsets[i]);
	}

	void* mem = TMP_MALLOC(num_bytes);
	defer{ TMP_FREE(mem); };

	FSEEK((FILE*)traj->file.handle, traj->frame_offsets[i], SEEK_SET);
    const auto bytes_read = fread(mem, 1, num_bytes, (FILE*)traj->file.handle);
    
	CString mdl_str = { (uint8*)mem, (int64)bytes_read };
	TrajectoryFrame* frame = traj->frame_buffer.ptr + i;
	extract_trajectory_frame_data(frame, mdl_str);

	traj->num_frames++;
	return true;
}

bool close_file_handle(MoleculeTrajectory* traj) {
	ASSERT(traj);
	if (traj->file.tag != PDB_FILE_TAG) {
		LOG_ERROR("Wrong file tag for closing file handle... Expected PDB_FILE_TAG");
		return false;
	}

	if (traj->file.handle) {
		fclose((FILE*)traj->file.handle);
		traj->file.handle = nullptr;
		return true;
	}
	return false;
}

bool extract_molecule_info(MoleculeInfo* info, CString pdb_string) {
	ASSERT(info);

	int32 num_atoms = 0;
	int32 num_residues = 0;
	int32 num_chains = 0;

	uint32 curr_res_pattern = 0;
	uint8 curr_chain_pattern = 0;
	CString line;
	while (pdb_string && (line = extract_line(pdb_string))) {
		if (compare_n(line, "ATOM", 4) || compare_n(line, "HETATM", 6)) {
			const uint32 res_pattern = *(uint32*)(&line[22]);
			const uint8 chain_pattern = line[21];

			num_atoms++;
			if (res_pattern != curr_res_pattern) {
				num_residues++;
				curr_res_pattern = res_pattern;
			}
			if (chain_pattern != curr_chain_pattern) {
				num_chains++;
				curr_chain_pattern = chain_pattern;
			}
		}
		else if (compare_n(line, "ENDMDL", 6) || compare_n(line, "END", 3)) {
			break;
		}
	}

	info->num_atoms = num_atoms;
	info->num_residues = num_residues;
	info->num_chains = num_chains;

	return true;
}

}
