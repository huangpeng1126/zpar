#include "definitions.h"
#include "english/pos/penn.h"
#include "reader.cpp"
#include "writer.h"
//#include "common/depparser/depparser_base.h" //for conll, etc. Not really necessary to include the full file, probably

#include "linguistics/word_tokenized.h"
#include "linguistics/taggedword.h"
#include "linguistics/dependency.h"
#include "linguistics/conll.h"
#include "linguistics/tagset.h"

#include "tags.h"


/*
 * 2013-09-24, Carlos Gomez Rodriguez
 *
 * Compile a map from words to lists of tags from the training treebank, which will be used as gold standard data
 * for the morphological analysis
 */

namespace english
{

//map from words to tags, extracted from the training set
std::map< std::string , std::set<CTag> > trainingSetLexicon;

//tags that will be assigned to unknown words
std::set<CTag> tagsForUnknownWords;

bool initLexicon ( const std::string sInputFile , bool bCoNLL )
{
	CSentenceReader *input_reader;
	std::ifstream *is;

	CTwoStringVector input_sent;
	CCoNLLInput input_conll;

	is = 0;
	input_reader = 0;
	if (bCoNLL)
		is = new std::ifstream(sInputFile.c_str());
	else
		input_reader = new CSentenceReader(sInputFile);

	if ( is->fail() ) return false;

	bool bReadSuccessful;

	// Read the next example
	if (bCoNLL)
		bReadSuccessful = ( (*is) >> input_conll );
	else
		bReadSuccessful = input_reader->readTaggedSentence(&input_sent, false, TAG_SEPARATOR);

	while( bReadSuccessful )
	{
		//Store the word/tag pairs that have been read
		if (bCoNLL)
		{
			for ( int i = 0 ; i < input_conll.size() ; i++ )
			{
				//add tag to the set of tags associated with that word
				std::set<CTag> & entry = trainingSetLexicon[input_conll[i].word];
				entry.insert(input_conll[i].tag);
			}
		}
		else
		{
			for ( int i = 0 ; i < input_sent.size() ; i++ )
			{
				//add tag to the set of tags associated with that word
				std::set<CTag> & entry = trainingSetLexicon[input_sent[i].first];
				entry.insert(input_sent[i].second);
			}
		}

		// Read the next example
		if (bCoNLL)
			bReadSuccessful = ( (*is) >> input_conll );
		else
			bReadSuccessful = input_reader->readTaggedSentence(&input_sent, false, TAG_SEPARATOR);
	}

	is->close();
	if (bCoNLL)
	   delete is;
	else
	   delete input_reader;

	//now, initialize the tags for unknown words
    for (int i=1; i<PENN_TAG_COUNT; i++)
       if ( !PENN_TAG_CLOSED[i] )
          tagsForUnknownWords.insert(CTag(i));

	//And we are done.
	return true;

}

bool isKnown ( const std::string & word )
{
	std::set<english::CTag> & setOfTags = trainingSetLexicon[word];
	return !setOfTags.empty();
}


std::set<CTag> getTagsForUnknownWord ( const std::string & word )
{
	//return tagsForUnknownWords;

	//copy the generic set of tags:
	std::set<CTag> result = std::set<CTag>(tagsForUnknownWords);

	//superlatives must end with st
	if ( word.length() < 2 || word[word.length()-2] != 's' || word[word.length()-1] != 't' )
	{
		result.erase(CTag(PENN_TAG_ADJECTIVE_SUPERLATIVE));
		result.erase(CTag(PENN_TAG_ADVERB_SUPERLATIVE));
	}

	//vbz must end with s
	if ( word.length() < 2 || word[word.length()-1] != 's' )
	{
		result.erase(CTag(PENN_TAG_VERB_THIRD_SINGLE));
	}

	return result;

}

std::set<CTag> getPossibleTags ( const std::string & word )
{
	if ( isKnown(word) )
	{
		return english::trainingSetLexicon[word];
	}
	else
		return getTagsForUnknownWord ( word );
}






} //namespace english


/**
 * Unit test
 */
int main( void )
{
	std::string filename;
	std::cout << "Enter filename for Penn treebank in CoNLL format: ";
	std::cin >> filename;
	bool bSuccess = english::initLexicon(filename,true);
	std::cout << "Successfully loaded the lexicon? " << bSuccess << "\n";
	for(;;)
	{
		std::string word;
		std::cout << "Enter a word: ";
		std::cin >> word;
		std::cout << "That word is " << (english::isKnown(word)?"known":"unknown") << ".\n";
		std::cout << "And it can be: ";
		std::set<english::CTag> setOfTags = english::getPossibleTags(word);
		for ( std::set<english::CTag>::iterator it = setOfTags.begin() ; it != setOfTags.end() ; ++it )
		{
			std::cout << " " << *it;
		}
		std::cout << "\n";
	}
}
