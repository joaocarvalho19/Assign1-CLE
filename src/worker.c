#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <wchar.h>
#include <locale.h>
#include <stdbool.h>
#include <libgen.h>
#include <getopt.h>
#include <time.h>

/** \brief print usage */
static void printUsage (char *cmdName){
    fprintf (stderr, "\nSynopsis: %s OPTIONS [filename / positive number]\n"
                     " OPTIONS:\n"
                     " -h --- print this help\n"
                     " -f --- filename\n"
                     " -n --- positive number\n", cmdName);
}

/** \brief check if char is consoant or not */
bool isConsonant(char val){
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
    char arr[21] = {'a','e','i','o','u'};
    int i;
    for(i = 0; i < 21; i++){
        if(arr[i] == val)
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

/** \brief check if the char is a char for word  */
bool isCharToWord(char c){
    if (isVogal(c) || isConsonant(c) || c==L'\'' || c==L'_' || c==0x27){
        return true;
    }
    return false;
}

/** \brief main function */
int main(int argc, char **argv){
    setlocale(LC_ALL, "");

    int num_words, vowel_begin_words, consonant_end_words;
    bool wordStatus = false; // true: in word    false: out word
    int opt; /* selected option */
    int fileCounter = 0;
    char *files_names[30];
    opterr = 0;
    double t0, t1, t2; /* time limits */
    t2 = 0.0;


    /*INITIALIZE*/
    FILE *file;
    do{
        switch ((opt = getopt (argc, argv, "f:h"))){
            case 'f': /* file name */
                if (optarg[0] == '-'){
                    fprintf (stderr, "%s: file name is missing\n", basename (argv[0]));
                    printUsage (basename (argv[0]));
                    return EXIT_FAILURE;
                }
                files_names[fileCounter] = optarg;
                fileCounter++;
                break;
            case 'h': /* help mode */
                printUsage (basename (argv[0]));
                return EXIT_SUCCESS;
            case '?': /* invalid option */
                fprintf (stderr, "%s: invalid option\n", basename (argv[0]));
                printUsage (basename (argv[0]));
                return EXIT_FAILURE;
            case -1: break;
        }
    } while (opt != -1);
    if (argc == 1) {
        fprintf(stderr, "%s: invalid format\n", basename(argv[0]));
        printUsage(basename(argv[0]));
        return EXIT_FAILURE;
    }


    //for all files in arguments
    for (int i = 0; i < fileCounter; ++i) {
        t0 = ((double) clock ()) / CLOCKS_PER_SEC;

        wchar_t c, last_char;
        char *fName = files_names[i];
        //counters
        num_words = 0;
        vowel_begin_words = 0;
        consonant_end_words = 0;

        //char file_path[20] = "texts/";
        //strcat(file_path, argv[i]);
        file = fopen(fName, "rb");

        //File validator
        if (file == NULL) {
            printf("Error");
            exit(1);
        }

        while ((c = fgetwc(file)) != EOF) {
            c = convert_multibyte(c); //clean char

            if (isCharToWord(c)) {
                //if out Word
                if (!wordStatus) {
                    wordStatus = true; // change to in word
                    num_words++;
                    if (isVogal(c)) {
                        vowel_begin_words++;
                    }
                }
            } else {
                if (isCharToWord(last_char)) {
                    wordStatus = false;
                    if (isConsonant(last_char)) {
                        consonant_end_words++;
                    }
                }
            }
            last_char = c;
        };


        printf("\nFile: %s\n", fName);
        printf("Number of words: %d\n", num_words);
        printf("Number of words started with a vowel %d \n", vowel_begin_words);
        printf("Number of words ended with a consonant %d \n", consonant_end_words);
        fclose(file);

        t1 = ((double) clock ()) / CLOCKS_PER_SEC;
        t2 += t1 - t0;

    }
    printf ("\nElapsed time = %.6f s\n", t2);


    return 0;
}