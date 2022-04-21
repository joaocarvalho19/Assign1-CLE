#ifndef ASSIGN1_EX2_CLE_SHARED_REGION_H
#define ASSIGN1_EX2_CLE_SHARED_REGION_H
int N;

//create structure
typedef struct {
    int fileId;
    int numberOfMatrices;
    int orders;
    double *determinants;
    int nextMatrixToProcessed;
    bool done;
} INFOSFILE;

//all required functions
/** \brief allocate files */
void allocFiles (int nFileNames, char *fileNames[]);

/** \brief return 0 if has new matrix to process, return 1 if hasn't more matrix on files */
int getMatrix(int threadId,  double*** buffer, INFOSFILE *partialInfo, int *min_size);

/** \brief Save partial info */
void saveResult(int threadId, double** buffer, INFOSFILE *infos);

/** \brief Show all final results */
void printResults();

/** \brief Free matrix */
void destroy_matrix(double** matrix);

/** \brief Show matrix */
void printMatrix(double** mat);

#endif
