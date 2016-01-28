#include <HDTManager.hpp>
#include <HDTVocabulary.hpp>
#include <Dictionary.hpp>
#include <FourSectionDictionary.hpp>
#include <PlainDictionary.hpp>

using namespace std;
using namespace hdt;


class DictionaryManager : ModifiableDictionary {

    Dictionary* hdtDict; // Dictionary from HDT file
    Dictionary* patchDict; // Additional dictionary

    const unsigned int bitmask;

public:
  DictionaryManager(Dictionary* hdtDict, HDTSpecification &spec): bitmask( 0x80000000 ), hdtDict(hdtDict) {
    // Create additional dictionary
    std::string dictType = spec.get("dictionary.type");
  	if(dictType == HDTVocabulary::DICTIONARY_TYPE_FOUR) {
  		patchDict = new FourSectionDictionary(spec);
  	} else if(dictType==HDTVocabulary::DICTIONARY_TYPE_PLAIN) {
  		patchDict = new PlainDictionary(spec);
  	} else {
  		patchDict = new FourSectionDictionary(spec);
  	}
  };

  std::string idToString(unsigned int id, TripleComponentRole position)
  {
    // Check whether id is from HDT or not (MSB is not set)
    if (id & bitmask) {
      return hdtDict->idToString(id, position);
    }

    return patchDict->idToString(id, position);
  }

  unsigned int stringToId(std::string &key, TripleComponentRole position)
  {
    // First ask HDT
    try
    {
      return hdtDict->stringToId(key, position);
    } catch( exception e )
    {
       // ID is not in there
    }
    // Set MSB to 1
    return bitmask | patchDict->stringToId(key, position);
  }
};
