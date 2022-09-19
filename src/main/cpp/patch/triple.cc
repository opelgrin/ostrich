#include <sstream>
#include <vector>
#include "triple.h"
#include "patch_tree_key_comparator.h"

Triple::Triple() : subject(0), predicate(0), object(0) {}

Triple::Triple(const TripleID& tripleId) :
        subject(tripleId.getSubject()), predicate(tripleId.getPredicate()), object(tripleId.getObject()) {}
Triple::Triple(size_t subject, size_t predicate, size_t object) :
        subject(subject), predicate(predicate), object(object) {}

Triple::Triple(const string& s, const string& p, const string& o, std::shared_ptr<ModifiableDictionary> dict) {
  subject = !s.empty() ? dict->insert(const_cast<string&>(s), SUBJECT) : 0;
  predicate = !p.empty() ? dict->insert(const_cast<string&>(p), PREDICATE) : 0;
  object = !o.empty() ? dict->insert(const_cast<string&>(o), OBJECT) : 0;
}

const size_t Triple::get_subject() const {
    return subject;
}

const size_t Triple::get_predicate() const {
    return predicate;
}

const size_t Triple::get_object() const {
    return object;
}

const string Triple::get_subject(Dictionary& dict) const {
    return dict.idToString(get_subject(), SUBJECT);
}

const string Triple::get_predicate(Dictionary& dict) const {
    return dict.idToString(get_predicate(), PREDICATE);
}

const string Triple::get_object(Dictionary& dict) const {
    return dict.idToString(get_object(), OBJECT);
}

const string Triple::to_string() const {
    return std::to_string(get_subject()) + " " + std::to_string(get_predicate()) + " " + std::to_string(get_object()) + ".";
}

const string Triple::to_string(Dictionary& dict) const {
    return get_subject(dict) + " " + get_predicate(dict) + " " + get_object(dict) + ".";
}

const char* Triple::serialize(size_t* size) const {
    *size = sizeof(subject) + sizeof(predicate) + sizeof(object);
    char* bytes = new char[*size];

    memcpy(bytes, (char*)&subject, sizeof(subject));
    memcpy(&bytes[sizeof(subject)], (char*)&predicate, sizeof(predicate));
    memcpy(&bytes[sizeof(subject) + sizeof(predicate)], (char*)&object, sizeof(object));

    return bytes;
}

void Triple::deserialize(const char* data, size_t size) {
    memcpy(&subject, data,  sizeof(subject));
    memcpy(&predicate, &data[sizeof(subject)],  sizeof(predicate));
    memcpy(&object, &data[sizeof(subject) + sizeof(predicate)],  sizeof(object));
}

void Triple::set_subject(size_t subject) {
    this->subject = subject;
}

void Triple::set_predicate(size_t predicate) {
    this->predicate = predicate;
}

void Triple::set_object(size_t object) {
    this->object = object;
}

bool Triple::operator == (const Triple& rhs) const {
    return get_subject() == rhs.get_subject()
           && get_predicate() == rhs.get_predicate()
           && get_object() == rhs.get_object();
}

bool Triple::pattern_match_triple(const Triple& triple, const Triple& triple_pattern) {
    return (triple_pattern.get_subject() == 0 || triple_pattern.get_subject() == triple.get_subject())
             && (triple_pattern.get_predicate() == 0 || triple_pattern.get_predicate() == triple.get_predicate())
             && (triple_pattern.get_object() == 0 || triple_pattern.get_object() == triple.get_object());
}

bool Triple::is_all_matching_pattern(const Triple& triple_pattern) {
    return triple_pattern.get_subject() == 0 && triple_pattern.get_predicate() == 0 && triple_pattern.get_object() == 0;
}

std::size_t std::hash<Triple>::operator()(const Triple& triple) const {
    using std::size_t;
    using std::hash;
    return ((triple.get_subject()
          ^ (triple.get_predicate() << 1)) >> 1)
          ^ (triple.get_object() << 1);
}

TripleVersion::TripleVersion(int patch_id, const Triple& triple) : patch_id(patch_id), triple(triple) {}

const char *TripleVersion::serialize(size_t *size) const {
    *size = sizeof(TripleVersion);
    char* bytes = (char *) malloc(*size);
    memcpy(bytes, (char*)&patch_id, sizeof(patch_id));
    memcpy(&bytes[sizeof(patch_id)], (char*)&triple, sizeof(triple));
    return bytes;
}

StringTriple::StringTriple() = default;

StringTriple::StringTriple(string s, string p, string o) : subject(std::move(s)), predicate(std::move(p)),
                                                           object(std::move(o)) {}

Triple StringTriple::get_as_triple(std::shared_ptr<ModifiableDictionary> dict) const {
    return {subject, predicate, object, dict};
}

std::string StringTriple::get_subject() const {
    return subject;
}

std::string StringTriple::get_predicate() const {
    return predicate;
}

std::string StringTriple::get_object() const {
    return object;
}

std::string StringTriple::to_string() const {
    return subject + " " + predicate + " " + object + ".";
}

bool StringTriple::operator==(const StringTriple &other) const {
    return subject == other.subject && predicate == other.predicate && object == other.object;
}

bool StringTriple::operator<(const StringTriple &other) const {
    return to_string() < other.to_string();
}

void StringTriple::set_subject(std::string new_subject) {
    this->subject = std::move(new_subject);
}

void StringTriple::set_predicate(std::string new_predicate) {
    this->predicate = std::move(new_predicate);
}

void StringTriple::set_object(std::string new_object) {
    this->object = std::move(new_object);
}


TripleVersions::TripleVersions() : triple(new Triple()), versions(new vector<int>()), dict(nullptr) {}

TripleVersions::TripleVersions(Triple* triple, std::vector<int>* versions, std::shared_ptr<DictionaryManager> dictionary) : triple(triple), versions(versions), dict(dictionary) {}

TripleVersions::~TripleVersions() {
    delete triple;
    delete versions;
}

Triple* TripleVersions::get_triple() {
    return triple;
}

const Triple *TripleVersions::get_triple_const() const {
    return triple;
}

vector<int>* TripleVersions::get_versions() {
    return versions;
}

std::shared_ptr<DictionaryManager> TripleVersions::get_dictionary() const {
    return dict;
}

void TripleVersions::set_dictionary(std::shared_ptr<DictionaryManager> dictionary) {
    dict = dictionary;
}


TripleVersionsString::TripleVersionsString() = default;

TripleVersionsString::TripleVersionsString(StringTriple triple, std::vector<int> versions) : triple(
        std::move(triple)), versions(std::move(versions)) {}

StringTriple *TripleVersionsString::get_triple() {
    return &triple;
}

std::vector<int> *TripleVersionsString::get_versions() {
    return &versions;
}

bool TripleVersionsString::operator<(const TripleVersionsString &other) const {
    return (triple < other.triple);
}


TripleDelta::TripleDelta() : triple(new Triple()), addition(true), dict(nullptr) {}

TripleDelta::TripleDelta(Triple* triple, bool addition, std::shared_ptr<DictionaryManager> dictionary) : triple(triple), addition(addition), dict(dictionary) {}

Triple* TripleDelta::get_triple() {
    return triple;
}

const Triple* TripleDelta::get_triple_const() const {
    return triple;
}

bool TripleDelta::is_addition() {
    return addition;
}

TripleDelta::~TripleDelta() {
    delete triple;
}

void TripleDelta::set_addition(bool addition) {
    this->addition = addition;
}

std::shared_ptr<DictionaryManager> TripleDelta::get_dictionary() const {
    return dict;
}

void TripleDelta::set_dictionary(std::shared_ptr<DictionaryManager> dictionary) {
    dict = dictionary;
}