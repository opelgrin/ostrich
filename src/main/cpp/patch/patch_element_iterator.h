#ifndef TPFPATCH_STORE_PATCH_ELEMENT_ITERATOR_H
#define TPFPATCH_STORE_PATCH_ELEMENT_ITERATOR_H

#include <thread>
#include "patch_element.h"

class PatchElementIterator {
public:
    PatchElementIterator();
    virtual ~PatchElementIterator() = 0;
    virtual bool next(PatchElement* element) = 0;
    virtual void goToStart() = 0;
};

class PatchElementIteratorTripleStrings : public PatchElementIterator {
protected:
    DictionaryManager* dict;
    IteratorTripleString* it;
    bool additions;
public:
    PatchElementIteratorTripleStrings(DictionaryManager* dict, IteratorTripleString* subIt, bool additions);
    ~PatchElementIteratorTripleStrings();
    bool next(PatchElement* element);
    void goToStart();
};

class PatchElementIteratorCombined : public PatchElementIterator {
protected:
    int pos;
    std::vector<PatchElementIterator*> iterators;
    long passed;
public:
    PatchElementIteratorCombined();
    ~PatchElementIteratorCombined();
    bool next(PatchElement* element);
    void appendIterator(PatchElementIterator* it);
    long getPassed();
    void goToStart();
};

class PatchElementIteratorVector : public PatchElementIterator {
protected:
    const std::vector<PatchElement>* elements;
    std::vector<PatchElement>::const_iterator it;
public:
    PatchElementIteratorVector(const std::vector<PatchElement>* elements);
    bool next(PatchElement* element);
    void goToStart();
};

class PatchElementIteratorBuffered : public PatchElementIterator {
protected:
    PatchElementIterator* it;
    std::queue<PatchElement> buffer;
    unsigned long buffer_size;
    bool ended;
    std::condition_variable buffer_trigger_fill;
    std::condition_variable buffer_trigger_nonempty;
    bool shutdown_thread = false;
    std::mutex lock_thread_fill;
    std::mutex lock_thread_nonempty;
    std::thread thread;
protected:
    void fill_buffer();
public:
    PatchElementIteratorBuffered(PatchElementIterator* it, unsigned long buffer_size);
    ~PatchElementIteratorBuffered();
    bool next(PatchElement* element);
    void goToStart();
};

#endif //TPFPATCH_STORE_PATCH_ELEMENT_ITERATOR_H
