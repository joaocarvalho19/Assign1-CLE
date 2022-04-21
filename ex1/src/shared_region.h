#ifndef ASSIGN1_CLE_SHARED_REGION_H
#define ASSIGN1_CLE_SHARED_REGION_H

//create structure
typedef struct {
    int  id;
    int number_of_words;
    int number_of_vowels_words; /*number of words started with vowel*/
    int number_of_consoants_words; /*number of words ended with consoant*/
    int status_word;
    bool done;
} INFOSFILE;

//all required functions
/** /brief allocate files */
void allocFiles (int nFileNames, char *fileNames[]);

/** \brief return 0 if has new char to process, return 1 if hasn't more chars on files */
int getChar(int threadId, char *buf, INFOSFILE *partialInfo);

/** \brief save results */
void saveResult (int threadId, INFOSFILE *partialInfo);

/** \breif print all results */
void printProcessingResults();

/** \brief check if the char is a char for word  */
bool isCharToWord(char c);

/** \brief check if char is vogal or not  */
bool isVogal(char val);

/** \brief check if char is consonant or not */
bool isConsonant(char val);
#endif //ASSIGN1_CLE_SHARED_REGION_H
