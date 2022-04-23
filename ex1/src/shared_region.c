#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <string.h>
#include <locale.h>
#include <wchar.h>

typedef struct {
    int  id;
    int number_of_words;
    int number_of_vowels_words; /*number of words started with vowel*/
    int number_of_consoants_words; /*number of words ended with consoant*/
    int status_word;
    bool done;
} INFOSFILE;

static long pos;
static INFOSFILE * infofile;
static pthread_mutex_t accessCR = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t store;
static pthread_cond_t process;
static bool stored;
static bool processed;
static pthread_once_t init = PTHREAD_ONCE_INIT;

static char ** fileNamesForRegion;
static int nFiles;
bool firstProcessing = true;
int fileInProcessing = 0;
int readen_chars = 0;
int MAX_SIZE_WRD = 50;
int MAX_BYTES_READ = 12;


/** \brief check if char is consonant or not */
bool isConsonant(char val){
    /* create array with all consoants */
    char arr[21] = {'b', 'c','d', 'f', 'g','h', 'j', 'k', 'l', 'm','n', 'p', 'q', 'r', 's', 't', 'v', 'w', 'x', 'y', 'z'};
    int i;
    for(i = 0; i < 21; i++){
        if(arr[i] == val)
            return true;
    }
    return false;
}

/** \brief check if char is vogal or not  */
bool isVogal(char val){
    /* create array with all vogals */
    char arr[21] = {'a','e','i','o','u'};
    int i;
    for(i = 0; i < 21; i++){
        if(arr[i] == val)
            return true;
    }
    return false;
}

/** \brief check if the char is a char for word  */
bool isCharToWord(char c){
    /* to check if char is to word need to be vogal and consonant, apostophe and underscore is char to word too*/
    if (isVogal(c) || isConsonant(c) || c==L'\'' || c==L'_' || c==0x27){
        return true;
    }
    return false;
}

/** \brief Convert multibyte to singlebyte and return the right character  */
char convert_multibyte(wchar_t c){
    switch (c) {
        // Vogal A
        case L'à': c=L'a'; break;
        case L'á': c=L'a'; break;
        case L'â': c=L'a'; break;
        case L'ã': c=L'a'; break;
        case L'Á': c=L'a'; break;
        case L'À': c=L'a'; break;
        case L'Â': c=L'a'; break;
        case L'Ã': c=L'a'; break;

            //Vogal E
        case L'è': c=L'e';break;
        case L'é': c=L'e';break;
        case L'ê': c=L'e';break;
        case L'É': c=L'e';break;
        case L'È': c=L'e';break;
        case L'Ê': c=L'e';break;

            //Vogal I
        case L'í': c=L'i';break;
        case L'ì': c=L'i';break;

        case L'Í': c=L'i';break;
        case L'Ì': c=L'i';break;
            //Vogal O
        case L'ó': c=L'o'; break;
        case L'ò': c=L'o'; break;
        case L'ô': c=L'o'; break;
        case L'õ': c=L'o'; break;

        case L'Ó': c=L'o'; break;
        case L'Ò': c=L'o'; break;
        case L'Ô': c=L'o'; break;
        case L'Õ': c=L'o'; break;
            //Vogal U
        case L'ú': c=L'u';break;
        case L'ù': c=L'u';break;
        case L'ü': c=L'u';break;
        case L'Ú': c=L'u';break;
        case L'Ù': c=L'u';break;

            //C Cedil
        case L'Ç': c=L'c';break;
        case L'ç': c=L'c';break;

            // Several Marks
        case L'«': c=L'"';break;
        case L'»': c=L'"';break;
        case L'‒': c=L'-';break;
        case L'–': c=L'-';break;
        case L'—': c=L'-';break;
        case L'…': c=L'.';break;
        case L'”': c=L'"'; break;
        case L'“': c=L'"'; break;
        case L'`': c=0x27; break;
        case L'’': c=0x27; break;
        case L'‘': c=0x27; break;

        default:    break;
    }
    return tolower(c);
}

/** \brief initialize conditions for pthread */
static void initialization (void) {
    processed = false;
    stored = false;

    /* initialises the condition variables */
    pthread_cond_init (&process, NULL);
    pthread_cond_init (&store, NULL);

    setlocale(LC_CTYPE, "");
}

/** /brief allocate files */
void allocFiles(int numberOfFiles, char *fileNames[]) {
    /* Enter monitor*/
    if ((pthread_mutex_lock (&accessCR)) != 0) {
        perror ("error on entering monitor(CF)");
        int status = EXIT_FAILURE;
        pthread_exit(&status);
    }

    /*save number of files*/
    nFiles = numberOfFiles;
    /* memory allocate for file names */
    fileNamesForRegion = malloc(nFiles * sizeof(char*));

    /*memory allocations for info files*/
    infofile = (INFOSFILE *)malloc(sizeof(INFOSFILE) * nFiles);

    for (int i=0; i<numberOfFiles; i++) {
        fileNamesForRegion[i] = malloc((12) * sizeof(char));
        strcpy(fileNamesForRegion[i], fileNames[i]);                                         /*add file name to array */
        infofile[i].done = false;                                                       /*Initialize as file not done */
    }

    /* initializations */
    pthread_once (&init, initialization);

    /* Exit monitor*/
    if ((pthread_mutex_unlock (&accessCR)) != 0) {
        perror ("error on exiting monitor(CF)");
        int status = EXIT_FAILURE;
        pthread_exit(&status);
    }

}

/** \brief return 0 if has new char to process, return 1 if hasn't more chars on files */
int getChar(int threadId, char *buffer, INFOSFILE *partialInfo) {
    /* enter monitor */
    if ((pthread_mutex_lock (&accessCR)) != 0) {
        perror ("error on entering monitor(CF)");
        int status = EXIT_FAILURE;
        pthread_exit(&status);
    }

    /* Waiting to be stored*/
    if (firstProcessing == false) {
        while (stored==false) {
            if ((pthread_cond_wait (&store, &accessCR)) != 0) {
                perror ("error on waiting");
                int status = EXIT_FAILURE;
                pthread_exit (&status);
            }
        }
    }

    /* check if file already done*/
    if (infofile[fileInProcessing].done == true) {
        /*check if is it the last file to be processed*/
        if (fileInProcessing == nFiles - 1) {
            if ((pthread_mutex_unlock (&accessCR)) != 0) {
                perror ("error on exiting monitor(CF)");
                int status = EXIT_FAILURE;
                pthread_exit(&status);
            }
            return 1;            /*if is the last file and it is done, end */
        }

        /* Got to the next file */
        fileInProcessing++;
        firstProcessing = true;
    }


    if (firstProcessing == true) {
        /* initialize variables */
        infofile[fileInProcessing].id = fileInProcessing;
        infofile[fileInProcessing].number_of_words = 0;
        infofile[fileInProcessing].number_of_consoants_words = 0;
        infofile[fileInProcessing].number_of_vowels_words = 0;
        infofile[fileInProcessing].status_word = 0;
    }

    FILE *f = fopen(fileNamesForRegion[fileInProcessing], "r");

    /*if isn't the first processing on file, go to next position that was reading */
    if (firstProcessing==false) fseek(f, pos, SEEK_SET );

    /* change firstProcessing */
    firstProcessing = false;

    /*check file*/
    if(f == 0) {
        perror("fopen");
        exit(1);
    }

    wchar_t c;

    /* get next char */
    c = fgetwc(f);

    /* update pos */
    pos = ftell(f);

    /* convert char readed*/
    char converted_char = convert_multibyte(c);
    char last_char = buffer[readen_chars-1];

    /*check if number of readen chars is lower than max defined*/
    if(readen_chars<MAX_BYTES_READ){
        /* update buffer */
        buffer[readen_chars] = converted_char;
        readen_chars++;
    } else {
        /* if readen_chars is greater than max defined*/
        /* if new character is word character , add array to avoid failure*/
        if(isCharToWord(converted_char) || isConsonant(last_char)){
            buffer[readen_chars] = converted_char;
            readen_chars++;
        }else{
            /*if new character is not character for word, restart array*/
            memset(buffer, 0, MAX_BYTES_READ+MAX_SIZE_WRD);
            readen_chars = 0;
            buffer[readen_chars] = converted_char;
            readen_chars++;
        }
    }

    fclose(f);

    /* if end of file, update file to done */
    if ( c == WEOF)  {
        infofile[fileInProcessing].done = true;
    }

    *partialInfo = infofile[fileInProcessing];

    processed = true;

    /* signal to process */
    if ((pthread_cond_signal (&process)) != 0) {
        perror ("error on waiting in fifoEmpty");
        int status = EXIT_FAILURE;
        pthread_exit (&status);
    }
    stored = false;

    /* enter monitor */
    if ((pthread_mutex_unlock (&accessCR)) != 0) {
        perror ("error on exiting monitor(CF)");
        int status = EXIT_FAILURE;
        pthread_exit(&status);
    }

    return 0;
}

/** \brief save results */
void saveResult(int threadId, INFOSFILE *partialInfo) {
    /* enter monitor */
    if ((pthread_mutex_lock (&accessCR)) != 0) {
        perror ("error on entering monitor(CF)");
        int status = EXIT_FAILURE;
        pthread_exit(&status);
    }

    /* wait to processed*/
    while (processed == false) {
        if ((pthread_cond_wait (&process, &accessCR)) != 0) {
            perror ("error on waiting in fifoEmpty");
            int status = EXIT_FAILURE;
            pthread_exit (&status);
        }
    }

    /* save new partial info */
    infofile[fileInProcessing] = *partialInfo;

    stored = true;
    /* signel to stored */
    if ((pthread_cond_signal (&store)) != 0) {
        perror ("error on waiting in fifoEmpty");
        int status = EXIT_FAILURE;
        pthread_exit (&status);
    }
    processed = false;

    /* exit monitor */
    if ((pthread_mutex_unlock (&accessCR)) != 0) {
        perror ("error on exiting monitor(CF)");
        int status = EXIT_FAILURE;
        pthread_exit(&status);
    }

}

/** \breif print all results */
void printProcessingResults(){

    /* for each partial file info */
    for (int i=0; i<nFiles; i++) {
        printf("\nFile name: %s\n", fileNamesForRegion[i]);
        printf("Total number of words = %d\n", infofile[i].number_of_words);
        printf("Total number of words started with vowel= %d\n", infofile[i].number_of_vowels_words);
        printf("Total number of words ended with consonant= %d\n", infofile[i].number_of_consoants_words);
    }
}
