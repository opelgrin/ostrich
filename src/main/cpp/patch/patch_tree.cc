#include <iostream>
#include <kchashdb.h>

#include "patch_tree.h"
#include "patch_elements.h"

using namespace std;
using namespace kyotocabinet;

PatchTree::PatchTree(string file_name) {
    // Set the triple comparator
    // TODO: this comparator should eventually be removed if serialization is fully implemented
    class ComparatorImpl : public Comparator {
        int32_t compare(const char* akbuf, size_t aksiz, const char* bkbuf, size_t bksiz) {
            PatchTreeKey* element1 = (PatchTreeKey*) akbuf;
            PatchTreeKey* element2 = (PatchTreeKey*) bkbuf;
            int comp_subject = element1->get_subject().compare(element2->get_subject());
            //cout << "comp: " << element1->subject << " ? " << element2->subject << " = " << comp_subject << endl; // TODO
            if(!comp_subject) {
                int comp_predicate = element1->get_predicate().compare(element2->get_predicate());
                if(!comp_predicate) {
                    return element1->get_object().compare(element2->get_object());
                }
                return comp_predicate;
            }
            return comp_subject;
        };
    };
    ComparatorImpl* comparator = new ComparatorImpl();
    db.tune_comparator(comparator);

    // Open the database
    if (!db.open(file_name, HashDB::OWRITER | HashDB::OCREATE)) {
        cerr << "open error: " << db.error().name() << endl;
    }
};

PatchTree::~PatchTree() {
    // Close the database
    if (!db.close()) {
        cerr << "close error: " << db.error().name() << endl;
    }
}

int PatchTree::append(PatchElements patch, int patch_id) {
    for(int i = 0; i < patch.getSize(); i++) {
        PatchElement patchElement = patch.get(i);
        cout << "appending... " << patchElement.get_triple().get_subject() << endl; // TODO

        // Look up the triple in the tree.
        // If it does not exist, simply add our new element to the tree.
        // Otherwise, we have to append our value to the existing element.
        size_t key_size, value_size;
        const char * raw_key = patchElement.get_triple().serialize(&key_size);
        PatchTreeValue* value = (PatchTreeValue *) db.get(raw_key, key_size, &value_size);
        int patch_position = 0; // TODO: the relative position in the list.
        PatchTreeValue newValueElement = PatchTreeValue(patch_id, patch_position, patchElement.is_addition(), false);
        int values = 1;
        if(value) {
            // Value already exists
            PatchTreeValue* it = value;
            while(it->next) {
                if(it->patch_id == patch_id) {
                    cerr << "Already found a patch with id: " << patch_id << endl;
                    return -1;
                }
                it += sizeof(PatchTreeValue);
                values++;
            }
            value = (PatchTreeValue *) realloc(value, sizeof(PatchTreeValue) * values);
            it = value + sizeof(PatchTreeValue) * (values - 1);
            it->next = true;
            it += sizeof(PatchTreeValue);
            *it = newValueElement;
        } else {
            // Value does not exist yet
            value = (PatchTreeValue *) malloc(sizeof(PatchTreeValue));
            *value = newValueElement;
        }

        db.set(raw_key, key_size, (const char *) value, sizeof(PatchTreeValue) * values);
    }
    return 0;
}

PatchTreeIterator PatchTree::iterator(PatchTreeKey *key) {
    DB::Cursor* cursor = db.cursor();
    cursor->jump((const char *) &key, sizeof(key));
    PatchTreeIterator patchTreeIterator(cursor);
    return patchTreeIterator;
}