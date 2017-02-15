#include <iostream>

#include "patch_tree_key_comparator.h"

PatchTreeKeyComparator::PatchTreeKeyComparator(comp compare_1, comp compare_2, comp compare_3, DictionaryManager* dict)
        : compare_1(compare_1), compare_2(compare_2), compare_3(compare_3), dict(dict) {}

int32_t PatchTreeKeyComparator::compare(const char* akbuf, size_t aksiz, const char* bkbuf, size_t bksiz) {
    PatchTreeKey element1;
    PatchTreeKey element2;
    element1.deserialize(akbuf, aksiz);
    element2.deserialize(bkbuf, bksiz);
    int comp_1 = compare_1(element1, element2, *dict);
    if(!comp_1) {
        int comp_2 = compare_2(element1, element2, *dict);
        if(!comp_2) {
            return compare_3(element1, element2, *dict);
        }
        return comp_2;
    }
    return comp_1;
};

int32_t PatchTreeKeyComparator::compare(const PatchTreeKey& element1, const PatchTreeKey& element2) {
    int comp_1 = compare_1(element1, element2, *dict);
    if(!comp_1) {
        int comp_2 = compare_2(element1, element2, *dict);
        if(!comp_2) {
            return compare_3(element1, element2, *dict);
        }
        return comp_2;
    }
    return comp_1;
}

comp comp_s = [] (const PatchTreeKey& e1, const PatchTreeKey& e2, DictionaryManager& dict) {
    unsigned int max_id = (unsigned int) -1;
    //if ((e1.get_subject() == max_id && e2.get_subject() == max_id) || (e1.get_subject() == 0 && e2.get_subject() == 0)) return 0; // Should never occur
    if (e1.get_subject() == max_id || e2.get_subject() == 0) return 1;
    if (e2.get_subject() == max_id || e1.get_subject() == 0) return -1;
    // If MSB is not set, id is HDT
    if (!(e1.get_subject() & 2147483648) && !(e2.get_subject() & 2147483648)) {
        return dict.compareComponent(e1.get_subject(), e2.get_subject(), SUBJECT);
    }
    //Else, translate to string and compare
    return e1.get_subject(dict).compare(e2.get_subject(dict));
};

comp comp_p = [] (const PatchTreeKey& e1, const PatchTreeKey& e2, DictionaryManager& dict) {
    unsigned int max_id = (unsigned int) -1;
    //if ((e1.get_predicate() == max_id && e2.get_predicate() == max_id) || (e1.get_predicate() == 0 && e2.get_predicate() == 0)) return 0; // Should never occur
    if (e1.get_predicate() == max_id || e2.get_predicate() == 0) return 1;
    if (e2.get_predicate() == max_id || e1.get_predicate() == 0) return -1;
    // If MSB is not set, id is HDT
    if (!(e1.get_predicate() & 2147483648) && !(e2.get_predicate() & 2147483648)) {
        return dict.compareComponent(e1.get_predicate(), e2.get_predicate(), PREDICATE);
    }
    //Else, translate to string and compare
    return e1.get_predicate(dict).compare(e2.get_predicate(dict));
};

comp comp_o = [] (const PatchTreeKey& e1, const PatchTreeKey& e2, DictionaryManager& dict) {
    unsigned int max_id = (unsigned int) -1;
    //if ((e1.get_object() == max_id && e2.get_object() == max_id) || (e1.get_object() == 0 && e2.get_object() == 0)) return 0; // Should never occur
    if (e1.get_object() == max_id || e2.get_object() == 0) return 1;
    if (e2.get_object() == max_id || e1.get_object() == 0) return -1;
    // If MSB is not set, id is HDT
    if (!(e1.get_object() & 2147483648) && !(e2.get_object() & 2147483648)) {
        return dict.compareComponent(e1.get_object(), e2.get_object(), OBJECT);
    }
    //Else, translate to string and compare
    return e1.get_object(dict).compare(e2.get_object(dict));
};
