#include "patch_tree_iterator.h"
#include "positioned_triple_iterator.h"

PositionedTripleIterator::PositionedTripleIterator(PatchTreeIterator* it, int patch_id, Triple triple_pattern)
        : it(it), patch_id(patch_id), triple_pattern(triple_pattern) {}

PositionedTripleIterator::~PositionedTripleIterator() {
    delete it;
}

bool PositionedTripleIterator::next(PositionedTriple *positioned_triple, bool silent_step) {
    PatchTreeKey key;
    PatchTreeDeletionValue value;
    bool ret = it->next_deletion(&key, &value, silent_step);
    if(ret) {
        positioned_triple->triple = key;
        positioned_triple->position = value.get(patch_id).get_patch_positions().get_by_pattern(triple_pattern);
    }
    return ret;
}