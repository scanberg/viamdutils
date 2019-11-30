#pragma once

#include <core/types.h>
#include <core/array_types.h>
#include <core/string_types.h>
#include <core/bitfield.h>
#include <mol/molecule_dynamic.h>

struct StoredSelection {
	CStringView name;
	Bitfield mask;
};

namespace filter {
void initialize();
void shutdown();
bool compute_filter_mask(Bitfield mask, CStringView filter, const MoleculeStructure& molecule, ArrayView<const StoredSelection> stored_selectons = {});
bool filter_uses_selection(CStringView filter, ArrayView<const StoredSelection> stored_selectons);

}  // namespace filter
