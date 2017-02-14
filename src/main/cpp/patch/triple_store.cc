#include "triple_store.h"
#include "patch_tree_addition_value.h"
#include "patch_tree_key_comparator.h"
#include "../simpleprogresslistener.h"

using namespace std;
using namespace kyotocabinet;

TripleStore::TripleStore(string base_file_name, DictionaryManager* dict, int8_t kc_opts, bool readonly) : dict(dict) {
    // Construct trees
    index_spo_deletions = new TreeDB();
    index_spo = new TreeDB();
    index_sop = new TreeDB();
    index_pso = new TreeDB();
    index_pos = new TreeDB();
    index_osp = new TreeDB();

    // Set the triple comparators
    index_spo_deletions->tune_comparator(spo_comparator = new PatchTreeKeyComparator(comp_s, comp_p, comp_o, dict));
    index_spo->tune_comparator(spo_comparator);
    index_sop->tune_comparator(sop_comparator = new PatchTreeKeyComparator(comp_s, comp_o, comp_p, dict));
    index_pso->tune_comparator(pso_comparator = new PatchTreeKeyComparator(comp_p, comp_s, comp_o, dict));
    index_pos->tune_comparator(pos_comparator = new PatchTreeKeyComparator(comp_p, comp_o, comp_s, dict));
    index_osp->tune_comparator(osp_comparator = new PatchTreeKeyComparator(comp_o, comp_s, comp_p, dict));
    element_comparator = new PatchElementComparator(spo_comparator);

    index_spo_deletions->tune_options(kc_opts);
    index_spo->tune_options(kc_opts);
    index_sop->tune_options(kc_opts);
    index_pso->tune_options(kc_opts);
    index_pos->tune_options(kc_opts);
    index_osp->tune_options(kc_opts);

    // Open the databases
    open(index_spo_deletions, base_file_name + "_spo_deletions", readonly);
    open(index_spo, base_file_name + "_spo", readonly);
    open(index_sop, base_file_name + "_sop", readonly);
    open(index_pso, base_file_name + "_pso", readonly);
    open(index_pos, base_file_name + "_pos", readonly);
    open(index_osp, base_file_name + "_osp", readonly);
}

TripleStore::~TripleStore() {
    // Close the databases
    close(index_spo_deletions, "spo_deletions");
    close(index_spo, "spo");
    close(index_sop, "sop");
    close(index_pso, "pso");
    close(index_pos, "pos");
    close(index_osp, "osp");

    delete spo_comparator;
    delete sop_comparator;
    delete pso_comparator;
    delete pos_comparator;
    delete osp_comparator;
    delete element_comparator;
}

void TripleStore::open(TreeDB* db, string name, bool readonly) {
    db->tune_map(KC_MEMORY_MAP_SIZE);
    //db->tune_buckets(1LL * 1000 * 1000);
    db->tune_page_cache(KC_PAGE_CACHE_SIZE);
    db->tune_defrag(8);
    if (!db->open(name, readonly ? TreeDB::OREADER : (TreeDB::OWRITER | TreeDB::OCREATE))) {
        cerr << "open " << name << " error: " << db->error().name() << endl;
    }
}

void TripleStore::close(TreeDB* db, string name) {
    if (!db->close()) {
        cerr << "close " << name << " error: " << db->error().name() << endl;
    }
    delete db;
}

TreeDB* TripleStore::getTree(Triple triple_pattern) {
    bool s = triple_pattern.get_subject() > 0;
    bool p = triple_pattern.get_predicate() > 0;
    bool o = triple_pattern.get_object() > 0;

    if( s &  p &  o) return index_spo;
    if( s &  p & !o) return index_spo;
    if( s & !p &  o) return index_sop;
    if( s & !p & !o) return index_spo;
    if(!s &  p &  o) return index_pos;
    if(!s &  p & !o) return index_pso;
    if(!s & !p &  o) return index_osp;
    /*if(!s & !p & !o) */return index_spo;
}

TreeDB* TripleStore::getDefaultAdditionsTree() {
    return index_spo;
}

TreeDB* TripleStore::getDeletionsTree() {
    return index_spo_deletions;
}

void TripleStore::insertAdditionSingle(const PatchTreeKey* key, const PatchTreeAdditionValue* value, DB::Cursor* cursor) {
    size_t key_size, value_size;
    const char *raw_key = key->serialize(&key_size);
    const char *raw_value = value->serialize(&value_size);

    if (cursor != NULL) {
        cursor->set_value(raw_value, value_size, false);
    } else {
        index_spo->set(raw_key, key_size, raw_value, value_size);
    }
    index_sop->set(raw_key, key_size, raw_value, value_size);
    index_pso->set(raw_key, key_size, raw_value, value_size);
    index_pos->set(raw_key, key_size, raw_value, value_size);
    index_osp->set(raw_key, key_size, raw_value, value_size);

    free((char*) raw_key);
    free((char*) raw_value);

    // Flush db to disk
    if (++flush_counter_additions > FLUSH_TRIPLES_COUNT) {
        index_spo->synchronize();
        index_sop->synchronize();
        index_pso->synchronize();
        index_pos->synchronize();
        index_osp->synchronize();
        flush_counter_additions = 0;
    }
}

void TripleStore::insertAdditionSingle(const PatchTreeKey* key, int patch_id, bool local_change, bool ignore_existing, DB::Cursor* cursor) {
    // Look up the value for the given triple key in the tree.
    PatchTreeAdditionValue value;
    if (!ignore_existing) {
        // We assume that are indexes are sane, we only check one of them
        size_t key_size, value_size;
        const char *raw_key = key->serialize(&key_size);
        const char *raw_value = cursor == NULL ? index_spo->get(raw_key, key_size, &value_size) : cursor->get_value(&value_size, false);
        if (raw_value) {
            value.deserialize(raw_value, value_size);
            delete[] raw_value;
        }
        free((char*) raw_key);
    }
    value.add(patch_id);
    if (local_change) {
        value.set_local_change(patch_id);
    }

    insertAdditionSingle(key, &value);
}

void TripleStore::insertDeletionSingle(const PatchTreeKey* key, const PatchTreeDeletionValue* value, DB::Cursor* cursor) {
    size_t key_size, value_size;
    const char *raw_key = key->serialize(&key_size);
    const char *raw_value = value->serialize(&value_size);

    if (cursor != NULL) {
        cursor->set_value(raw_value, value_size, false);
    } else {
        index_spo_deletions->set(raw_key, key_size, raw_value, value_size);
    }

    free((char*) raw_key);
    free((char*) raw_value);

    // Flush db to disk
    if (++flush_counter_deletions > FLUSH_TRIPLES_COUNT) {
        index_spo_deletions->synchronize();
        flush_counter_deletions = 0;
    }
}

void TripleStore::insertDeletionSingle(const PatchTreeKey* key, const PatchPositions& patch_positions, int patch_id, bool local_change, bool ignore_existing, DB::Cursor* cursor) {
    PatchTreeDeletionValue deletion_value;
    if (!ignore_existing) {
        size_t key_size, value_size;
        const char *raw_key = key->serialize(&key_size);
        const char *raw_value = cursor == NULL ? index_spo_deletions->get(raw_key, key_size, &value_size) : cursor->get_value(&value_size, false);
        if (raw_value) {
            deletion_value.deserialize(raw_value, value_size);
            delete[] raw_value;
        }
        free((char*) raw_key);
    }
    PatchTreeDeletionValueElement element = PatchTreeDeletionValueElement(patch_id, patch_positions);
    if (local_change) {
        element.set_local_change();
    }
    deletion_value.add(element);

    insertDeletionSingle(key, &deletion_value);
}

PatchTreeKeyComparator *TripleStore::get_spo_comparator() const {
    return spo_comparator;
}

PatchElementComparator *TripleStore::get_element_comparator() const {
    return element_comparator;
}

DictionaryManager *TripleStore::get_dict_manager() const {
    return dict;
}
