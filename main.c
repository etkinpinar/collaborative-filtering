#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define FILENAMESIZE 63
#define BOOKNAMESIZE 31

#define LINEBUFFER 1024

int getRatings(int**, int, int, FILE*);
double* calculateSimilarity(int, int**, int, int);
int* getKSimilar(int, double*, int);
int recommend(int, double*, int*, int**, int, int, char**);

int main()
{
    int i,k;

    char fileloc[FILENAMESIZE];
    printf("Enter the location of rating file: ");
    scanf("%s", fileloc);

    FILE *fp;
    fp = fopen(fileloc, "r");
    if(!fp){
        printf("Could not open the given file..");
        exit(3);
    }

    // read file to get number of all users
    char line[LINEBUFFER];
    int numOfUsers = -1;
    while(fgets(line, LINEBUFFER, fp))
        numOfUsers++;

    fseek(fp, 0, SEEK_SET);
    fgets(line, LINEBUFFER, fp);

    char line2[LINEBUFFER];
    strcpy(line2,line);
    // read file to get number of books
    char *token = strtok(line,";");
    int numOfBooks=0;
    while(token){
        numOfBooks++;
        token = strtok(NULL,";");
    }

    // a string array for book names
    char **bookNames = (char**)malloc(numOfBooks*sizeof(char*));
    token = strtok(line2,";");
    i = 0;
    while(i<numOfBooks){
        bookNames[i] = (char*)malloc(sizeof(char)*(strlen(token)+1));
        strcpy(bookNames[i],token);
        token = strtok(NULL,";\n");
        i++;
    }

    // creating rating matrix that have all users' rating both New Users' and Users'
    int **ratingMtx = (int **)malloc(numOfUsers*sizeof(int*));
    if (ratingMtx == NULL){
        printf("Error while allocating memory to matrix..");
        exit(1);
    }
    for (i=0;i<numOfUsers;i++){
        ratingMtx[i] = (int *)calloc(numOfBooks,sizeof(int));
        if (ratingMtx[i] == NULL){
            printf("Error while allocating memory to matrix..");
            exit(2);
        }
    }

    // in the same csv, we have both new user ratings and user ratings therefore
    // indexes below indicates where these different rows starts and ends.
    int endOfUserIdx = getRatings(ratingMtx, numOfBooks, numOfUsers, fp);
    int newUserIdx = endOfUserIdx + 1; // 0 0 0 0 0 0

    printf("Enter the number k (k < number of Users): ");
    scanf("%d", &k);
    while(k > endOfUserIdx || k < 0){
        printf("K should be greater than 0 and smaller than number of Users: ");
        scanf("%d",&k);
    }

    int userID;
    printf("Enter the Username for recommendation: NU");
    scanf("%d", &userID);

    if(userID > numOfUsers-newUserIdx || userID < 0){
        printf("Username is not valid..");
        exit(3);
    }
    //set new user index according to input
    newUserIdx = endOfUserIdx + userID;

    double *sim = calculateSimilarity(newUserIdx, ratingMtx, endOfUserIdx, numOfBooks);
    int* kSim = getKSimilar(k, sim, endOfUserIdx);

    for (i=0;i<k;i++){
        printf("%d. 'U%d', %.4f\n",i+1,kSim[i]+1,sim[kSim[i]]);
    }
    int recommendation= recommend(newUserIdx, sim, kSim, ratingMtx, k, numOfBooks, bookNames);

    printf("Book recommendation: %s\n", bookNames[recommendation]);

    return 0;
}
// function that writes ratings from file to rating matrix and
// returns start of new user rows
int getRatings(int** ratingMtx, int numOfBooks, int numOfUsers, FILE* fp ){

    char line[LINEBUFFER];
    int endOfUserIdx = 0;

    int i = 0;
    while(fgets(line, LINEBUFFER, fp)){
        int j = 0;
        char *token=strtok(line,";");
        if(!strcmp(token," ")){
            endOfUserIdx = i;
        }
        token=strtok(NULL,";");
        while(j<numOfBooks-1){
            if (!strcmp(token," "))
                ratingMtx[i][j] = 0;
            else
                ratingMtx[i][j] = atoi(token);
            token=strtok(NULL,";");
            j++;
        }
        ratingMtx[i][j] = atoi(token);
        i++;
    }
    return endOfUserIdx;
}
// calculate similarity values for given user in user index
double* calculateSimilarity(int userIdx, int** ratingMtx, int endOfUserIdx,  int numOfBooks){
    int i,j;
    double *sim = (double*)calloc((endOfUserIdx-1),sizeof(double));
    int readBooksNum = 0;
    // readBooks stores books that user read
    int* readBooks = (int*) malloc(sizeof(int));
    for (i = 0; i<numOfBooks; i++){
        if(ratingMtx[userIdx][i] > 0){
            readBooksNum++;
            readBooks = realloc(readBooks, readBooksNum * sizeof(int));
            readBooks[readBooksNum-1] = i;

        }
    }

    for (i = 0;i<endOfUserIdx;i++){
        int commonBooksNum = 0;
        int* commonBooks = (int*) malloc(sizeof(int));
        // find common books
        for (j = 0; j<readBooksNum; j++){
            if(ratingMtx[i][readBooks[j]] > 0){
                commonBooksNum++;
                commonBooks = realloc(commonBooks, commonBooksNum * sizeof(int));
                commonBooks[commonBooksNum-1] = readBooks[j];
            }
        }
        // calculating rA and rB for common books that both users read
        double rA = 0;
        double rB = 0;
        for(j=0;j<commonBooksNum;j++){
            rA += ratingMtx[userIdx][commonBooks[j]];
            rB += ratingMtx[i][commonBooks[j]];
        }
        rA = rA / commonBooksNum;
        rB = rB / commonBooksNum;

        double top = 0;
        double bottom1 = 0;
        double bottom2 = 0;
        // similarity calculation
        for (j = 0;j<commonBooksNum; j++){
            top += (ratingMtx[userIdx][commonBooks[j]] - rA) * (ratingMtx[i][commonBooks[j]] - rB);
            bottom1 += (ratingMtx[userIdx][commonBooks[j]] - rA) * (ratingMtx[userIdx][commonBooks[j]] - rA);
            bottom2 += (ratingMtx[i][commonBooks[j]] - rB) * (ratingMtx[i][commonBooks[j]] - rB);
        }
        sim[i] = top / (sqrt(bottom1) * sqrt(bottom2));
    }
    return sim;
}

// get k similar users and return array of user indexes
int* getKSimilar(int k, double* sim, int sizeOfSim){
    int i, j, maxIdx;

    //get copy of similarity array
    double *tmpSim = (double*)malloc(sizeOfSim*sizeof(double));
    for (i = 0;i<sizeOfSim;i++)
        tmpSim[i] = sim[i];

    int *kSimUsers = (int*)calloc(k,sizeof(int));
    for (i = 0; i < k; i++){
        maxIdx = i;
        for (j = 0; j < sizeOfSim; j++)
            if (tmpSim[j] > tmpSim[maxIdx])
                maxIdx = j;

        tmpSim[maxIdx] = -1;

        kSimUsers[i] = maxIdx;
    }
    free(tmpSim);
    return kSimUsers;
}
// calculation function for unread books according to formula and returns recommended books index
int recommend(int userIdx, double* sim, int* kSim, int** ratingMtx, int k, int numOfBooks, char** bookNames){
    int i, j, x;
    int unreadNum = 0;  // variable to store number of book that is not read
    int *unreadArr = (int*) malloc(sizeof(int));
    double rA = 0;
    for (i = 0; i<numOfBooks;i++){
        rA += ratingMtx[userIdx][i];
        if (ratingMtx[userIdx][i] == 0){
            unreadNum++;
            unreadArr = realloc(unreadArr, unreadNum*sizeof(int));
            unreadArr[unreadNum-1] = i;
        }
    }

    rA = rA / numOfBooks;

    double recommendationScore = 0;
    int recommendationIdx;
    printf("\nPredicted score for unread books:\n");
    for(x = 0;x<unreadNum;x++){
        double top = 0;
        double bottom = 0;
        for (i = 0;i<k;i++){
            double rB = 0;
            for (j = 0;j<numOfBooks;j++)
                rB += ratingMtx[kSim[i]][j];
            rB = rB / numOfBooks;

            top += sim[kSim[i]] * (ratingMtx[kSim[i]][unreadArr[x]] - rB);
            bottom += sim[kSim[i]];
        }
            double score = rA + (top / bottom);
            if (score > recommendationScore){
                recommendationIdx = unreadArr[x];
                recommendationScore = score;
            }
            printf("%d. '%s', %.4f\n", x+1, bookNames[unreadArr[x]], score);
    }

    return recommendationIdx;
}
