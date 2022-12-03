//Used for exit constants mostly
#include <stdlib.h>
//printf and puts
#include <stdio.h>
//string comparison functions
#include <string.h>
//Used to seed the rng for random games
#include <time.h>

//Length of a word.
//Note: You cannot change this without changing the word list too.
//You also must adapt the scanf calls for the new length
#define WORD_LENGTH 5
//Number of tries allowed to find the word
#define MAX_TRIES 6
//Number of characters in the alphabet + 1
#define ALPHA_SIZE 27
//Constant Array Size for buffer array from menu function
#define MAX 4 
//Maximum amount of words in ALL.TXT 
# define ALL_WORDS 12947
//If set, the word and a few stats are printed before the game starts
//#define DEBUG

//Very cheap error termination script
#define err(x) fprintf(stderr, EOL "[%s:%i] Fatal error: %s" EOL, __FILE__, __LINE__, x);abort();

//Note: CRLF is also used for telnet.
//If you want to make it available in a BBS you may want to force a CRLF
#ifdef WIN32
#define EOL "\r\n"
#else
#define EOL "\n"
#endif

//Cheap boolean
#define bool int
#define false (0)
#define true (!false)

//Files for lists that contain all words and solutions
FILE * fpAll,  * fpSol;
//Number of words in the solution list
long wordCount = 0;
//Selected word from solution list
char word[WORD_LENGTH + 1] = {0};
//Possible characters (used to show unused characters)
//The size specifier is necessary or its value will be readonly
char alpha[ALPHA_SIZE] = "abcdefghijklmnopqrstuvwxyz";
//Memory cache:
//0-25 File position in the complete list with words that start with the given letter a-z
//26: Number of words in the solution list
long memcache[ALPHA_SIZE];
//Stores candidate words from All.txt
//               Rows      Cols
char candidates[ALL_WORDS][WORD_LENGTH + 1] = {0};


//Reads solution list and picks a random word
long setup(void);
//Pick the given word
int pickWord(char * word, int index);
//Computer guesses from All.TXT list instead of SOLUTION.TXT
int pickGuess(char * word, int index);
//Checks if the supplied word is in the list
bool hasWord(const char * word);
//Convert to lowercase
int toLower(char * str);
//Checks the entered word against the solution
bool checkWord(const char * guess);
//Checks if the entered string is a potentially valid word
bool isWord(const char* word);
//Gets the first position of the given char in the given string
int strpos(const char * str, char search);
//Removes characters in the supplied argument from the alphabet
void removeAlpha(const char * guess);
//Runs the main game loop
void gameLoop(void);
//Game loop for computer v computer
void myGameLoop(void); 
int findCandidate();
//Runs the menu
//passing array to menu function
int menu(char *menuInput); 
//Shows the help text
void help(void);

//Main function
int main() {
	int gameId;
	
	//Main Menu input
	char menuInput[MAX];
	
	setbuf(stdout, NULL);
	//Note: This will search for the file in the current directory
	//Switched \\ to / for wsl
	fpAll = fopen("lists/ALL.TXT", "r");
	fpSol = fopen("lists/SOLUTION.TXT", "r");
	if (fpAll == NULL || fpSol == NULL) {
		err("error opening wordle lists");
	}
	#ifdef DEBUG
	printf("Word count: %i" EOL, setup());
	#else
	setup();
	#endif
	gameId = menu(menuInput);
	//Print Menu Input to make sure it copied over from the menu function
	printf("%s", menuInput); 
	printf("\n");
	//If menuInput is ai, then myGameLoop function is called, else gameLoop()
	if (gameId >= 0) {
		pickWord(word, gameId);
		#ifdef DEBUG
		printf("Word: %s" EOL, word);
		#endif
	} else {
		return EXIT_SUCCESS;
	}
	printf("Running game #%i" EOL, gameId + 1);
	if(strcmp(menuInput, "ai") == 0) {
		myGameLoop();
	}else {
		gameLoop();
	}
	fclose(fpAll);
	fclose(fpSol);
	return EXIT_SUCCESS;
}

int menu(char *menuInput) {
	char buffer[21];
	int gameId = 0;
	int scan = 0;
	puts(
		"Main Menu" EOL
		"=========" EOL
		"AI: Computer vs Computer" EOL //My addition to menu
		"NEW: start a new game." EOL
		"LOAD <num>: load a specific game" EOL
		"HELP: More information" EOL
		"EXIT: End game");
	printf("The game number must be in range of 1-%li" EOL, wordCount);
	while (true) {
		printf("Input: ");
		while ((scan = scanf("%20s", buffer)) != 1) {
			if (scan == EOF) {
				return -1;
			}
		}
		toLower(buffer); 
		
		//Copies user input into main function
		for (int i = 0; i < MAX; i++) {menuInput[i] = buffer[i];}  
		
		if (strcmp(buffer, "exit") == 0) {
			return -1;
		} else if (strcmp(buffer, "help") == 0) {
			help();
		} else if (strcmp(buffer, "ai") == 0){
			return rand() % wordCount; 
		}else if (strcmp(buffer, "new") == 0) {
			return rand() % wordCount;
		} else if (strcmp(buffer, "load") == 0) {
			if (scanf("%i",  & gameId) == 1) {
				if (gameId > 0 && gameId <= wordCount) {
					return gameId - 1;
				}
			}
			puts("Invalid number");
		} else {
			puts("Invalid input");
		}
	}
}


void help() {
	printf("Wordle is a simple game:" EOL "Guess the %i letter word within %i tries" EOL, WORD_LENGTH, MAX_TRIES);
	puts(
		"After every guess, hints are shown for each character." EOL
		"They look like this:" EOL
		"  _ = Character not found at all" EOL
		"  # = Character found and position correct" EOL
		"  o = Character found but position wrong" EOL
		"Unused letters of the alphabet are shown next to the hint" EOL
		EOL
		"The game prefers valid positions over invalid positions," EOL
		"And it handles double letters properly." EOL
		"Guessing \"RATES\" when the word is \"TESTS\" shows \"__oo#\"");
}

void gameLoop() {
	char guess[WORD_LENGTH + 1] = {0};
	int guesses = 0;
	int scan = 0;
	puts(
		"word\tunused alphabet" EOL
		"====\t===============");
	while (guesses < MAX_TRIES && strcmp(guess, word)) {
		printf("Guess %i: ", guesses + 1);
		if ((scan = scanf("%5s", guess)) == 1 && strlen(guess) == WORD_LENGTH) {
			toLower(guess);
			//Do not bother doing all the test logic if we've found the word.
			if (strcmp(guess, word)) {
				if (isWord(guess) && hasWord(guess)) {
					++guesses;
					//TODO: Check guess against word
					if (checkWord(guess)) {
						removeAlpha(guess);
						printf("\t%s\n", alpha);
					}
				} else {
					puts("Word not in list");
				}
			}
		} else {
			if (scan == EOF) {
				exit(EXIT_FAILURE);
			}
			printf("Invalid word. Must be %i characters\n", WORD_LENGTH);
		}
	}
	if (strcmp(guess, word)) {
		printf("You lose. The word was %s\n", word);
	} else {
		puts("You win");
	}
}
void myGameLoop(void){
	//-For guess #1 have computer pick random word using rand() % wordCount and pass it into pickword()
	//-Then compare 1st guess to word
	//-Then search for words in ALL.TXT that contain the letters that fit; find which letter in first guess corresponds to 'o' or '#'
	int ndx = 0; //index number for candidate guess in all.txt list
	char guess[WORD_LENGTH + 1] = {0};
	char guessTwo[WORD_LENGTH + 1] = {0};
	char guessThree[WORD_LENGTH + 1] = {0};
	char guessFour[WORD_LENGTH + 1] = {0};
	char guessFive[WORD_LENGTH + 1] = {0};	 
	int guessOne = rand() % wordCount; //gameId for first guess chosen randomly

	pickGuess(guess, guessOne); 
	puts(
		"word\tunused alphabet" EOL
		"====\t===============");
	printf("Guess 1: %s" EOL, guess);
	//For debugging to make sure rand() is selecting two different words
	printf("Solution: %s" EOL, word);
	if (strcmp(guess, word)){
		if (checkWord(guess)) {
	 	removeAlpha(guess);
	 	printf("\t%s\n", alpha);
	}
	}else{
		puts("Computer Wins");
	}
	
	//2nd Guess
	ndx = findCandidate();
	strcpy(guessTwo, candidates[ndx]);
	printf("Guess 2: %s" EOL, guessTwo); 
	if (strcmp(guessTwo, word)){
		if (checkWord(guessTwo)) {
		removeAlpha(guessTwo);
	 	printf("\t%s\n", alpha);
	}
	}else{
		puts("Computer Wins");
	}

	//3rd Guess
	ndx = findCandidate();
	strcpy(guessThree, candidates[ndx]);
	printf("Guess 3: %s" EOL, guessThree); 
	if (strcmp(guessThree, word)){
		if (checkWord(guessThree)) {
		removeAlpha(guessThree);
	 	printf("\t%s\n", alpha);
	}
	}else{
		puts("Computer Wins");
	}

	//4th Guess
	ndx = findCandidate();
	printf("ndx = %d\n", ndx);
	if (ndx != 0){
		strcpy(guessFour, candidates[ndx]);
		printf("Guess 4: %s" EOL, guessFour); 
		if (strcmp(guessFour, word)){
			if (checkWord(guessFour)) {
			removeAlpha(guessFour);
	 		printf("\t%s\n", alpha);
		}
		}else{
			puts("Computer Wins");
		}
	}

	//5th guess
	ndx = findCandidate();
	printf("ndx = %d\n", ndx); 
	if (ndx != 0){
		strcpy(guessFive, candidates[ndx]);
		printf("Guess 5: %s" EOL, guessFive); 
		if (strcmp(guessFive, word)){
			if (checkWord(guessFive)) {
			removeAlpha(guessFive);
	 		printf("\t%s\n", alpha);
		}
		}else{
			puts("Computer Wins");
		}
	}

	//if letters in word and guess match, then the candidate words for the next 
	//guess need to include the letters that the first guess got correct
	int pos = 0;
	int right_pos = 0; 
	for(int i = 0; i < WORD_LENGTH - 1; i++){
		pos = strpos(guess, word[i]);
		printf("position %d= %d\n", i, pos); 
		if(pos >= 0){
			//store position in array of correct positions
			right_pos = pos;
		}
	}
	printf("Right Position = %d", right_pos);
	// for(int i = 0; i < 3; i++){
	// 	printf("Right Letter: %c\n", guess[right_pos]); 
	// }
	printf("Right Letter: %c\n", guess[right_pos]);
	
	// while (int i = 0; i < WORD_LENGTH; i++){
	// 	if (pos>= 0){
	// 		while(i < ALL_WORDS && fgets(candidates[i], WORD_LENGTH + 1, fpAll) != NULL){
	// 			fclose(fpAll);
	// 			fopen("lists/ALL.TXT", "r");

	// 		}
	// 	}
	// }
}

int findCandidate(){
	fclose(fpAll);
	fpAll = fopen("lists/ALL.TXT", "r"); 
	int i = 0;
	int isCandidate; 
	bool duplicateLetters = false;
	int firstCandidate = 0;
	while(i < ALL_WORDS && fgets(candidates[i], WORD_LENGTH + 1, fpAll) != NULL){ 
		//if all of the characters in computer Selected (checker) exist in alpha[], then return true
		//if true then stop the loop and print checker
		isCandidate = 0;
		candidates[i][5] = '\0';
		duplicateLetters = false;
		for(int a = 0; a < WORD_LENGTH; a++){
			for (int b = 0; b < WORD_LENGTH; b++){
				if(candidates[i][a] == candidates[i][b] && a != b){
					duplicateLetters = true;
				}
			}
		}
		if(duplicateLetters == false){
			for (int j = 0; j < WORD_LENGTH; j++) {
				for (int k = 0; k < ALPHA_SIZE; k++) {
					if (candidates[i][j] == alpha[k]) {
						isCandidate++;
					}
				}
			} 
			if (isCandidate == WORD_LENGTH) {
				firstCandidate = i;
				printf("breaking\n");
				printf("isCandidate = %d\n", isCandidate); 
				break;
			}else if ((i == ALL_WORDS - 1) && (isCandidate != WORD_LENGTH)){
				printf("No words found that fit the remaining alphabet\n");
			}
		}
		i++;
	}
	return firstCandidate;
}

void removeAlpha(const char * guess) {
	int i = 0;
	int pos = 0;
	if (guess != NULL) {
		while (guess[i]) {
			pos = strpos(alpha, guess[i]);
			if (pos >= 0) {
				alpha[pos] = '_';
			}
			++i;
		}
	}
}

int strpos(const char * str, char search) {
	int i = 0;
	if (str != NULL) {
		while (str[i]) {
			if (str[i] == search) {
				return i;
			}
			++i;
		}
	}
	return -1;
}

bool checkWord(const char * guess) {
	if (strlen(guess) == strlen(word)) {
		int i = 0;
		int pos = -1;
		//Copy is used to blank found characters
		//This avoids wrong reports for double letters, for example "l" in "balls"
		char copy[WORD_LENGTH + 1];
		char result[WORD_LENGTH + 1];
		result[WORD_LENGTH] = 0;
		strcpy(copy, word);
		//Do all correct positions first
		while (copy[i]) {
			if (copy[i] == guess[i]) {
				//Character found and position correct
				result[i] = '#';             //how to reveal what letter is the hashtag and what letter is the 'o'? and then be able to search for words in All.TXT w/letters
				printf("%c" EOL, guess[i]);
				copy[i] = '_';
			} else {
				//Fills remaining slots with blanks
				//We could do this before the loop as well
				result[i] = '_';
			}
			++i;
		}
		i = 0;
		while (copy[i]) {
			pos = strpos(copy, guess[i]);
			//Char must exist but do not overwrite a good guess
			if (pos >= 0 && result[i] != '#') {     //pos is wherever word and guess have the same letter, puts an 'o' in that position and blanks it out on word
				//Character found but position wrong
				result[i] = 'o';
				printf("%c" EOL, guess[i]);
				copy[pos] = '_';
			}
			++i;
		}
		printf("%s", result);
		return true;
	}else{
		printf("NOT WORKING");
	}
	return false;
}

int toLower(char * str) {
	int i = 0;
	while (str[i]) {
		if (str[i] >= 'A' && str[i] <= 'Z') {
			str[i] |= 0x20; //Make lowercase
		}
		++i;
	}
	return i;
}

int hasWord(const char * word) {
	//A bit longer to also contain the line terminator
	char buffer[WORD_LENGTH + 4];
	//Don't bother if the argument is invalid
	if (word == NULL || strlen(word) != WORD_LENGTH || !isWord(word)) {
		return false;
	}
	fseek(fpAll, memcache[word[0]-'a'], SEEK_SET);
	//Stop comparing once we are beyond the current letter
	while (fgets(buffer, WORD_LENGTH + 4, fpAll) != NULL && buffer[0]==word[0]) {
		buffer[WORD_LENGTH]=0;
		if (strcmp(word, buffer) == 0) {
			return true;
		}
	}
	return false;
}

bool isWord(const char* word){
	int i=-1;
	if(strlen(word) == WORD_LENGTH){
		while(word[++i]){
			if(word[i]<'a' || word[i]>'z'){
				return false;
			}
		}
		return true;
	}
	return false;
}

long setup() {
	FILE* cache;
	char currentChar;
	char currentWord[WORD_LENGTH + 1];
	bool success = false;
	
	//Don't bother if setup was already performed
	if (wordCount > 0) {
		return wordCount;
	}
	srand((int)time(0));
	
	if ((cache = fopen("LISTS\\CACHE.BIN","rb")) != NULL) {
		printf("Reading cache... ");
	    success = fread(memcache, sizeof(long), ALPHA_SIZE, cache) == ALPHA_SIZE;
	    fclose(cache);
		if(success){
			puts(" [OK]");
			return wordCount = memcache[ALPHA_SIZE - 1];
		}
		else{
			puts(" [FAIL]");
		}
	}
	
	printf("Loading word list...");
	fseek(fpSol, 0, SEEK_SET);
	while (fgets(currentWord, WORD_LENGTH + 1, fpSol) != NULL) {
		//Only increment if word length is correct
		if (strlen(currentWord) == WORD_LENGTH) {
			++wordCount;
		}
	}
	puts(" [OK]");
	memcache[ALPHA_SIZE-1] = wordCount;

	if (!success) {
	    printf("Building cache...");
		currentChar = 'a';
		memcache[0] = 0;
		fseek(fpAll, 0, SEEK_SET);
		while (fgets(currentWord, WORD_LENGTH + 1, fpAll) != NULL) {
			//Only proceed if word length is correct
			if (strlen(currentWord) == WORD_LENGTH) {
			    if (currentChar != currentWord[0]) {
			        currentChar = currentWord[0];
			        memcache[currentChar - 'a'] = ftell(fpAll) - 5;
			    }
			}
		}
		cache = fopen("LISTS\\CACHE.BIN", "wb");
		if (cache == NULL) {
			puts(" [FAIL]");
		}
		else{
			fwrite(memcache, sizeof(long), ALPHA_SIZE, cache);
			fclose(cache);
			puts(" [OK]");
		}
	}

	return wordCount;
}

int pickWord(char * word, int index) {
	int i = 0;
	fseek(fpSol, 0, SEEK_SET);
	while (i <= index && fgets(word, WORD_LENGTH + 1, fpSol) != NULL) { //fgets stores the solution into word
		if (strlen(word) == WORD_LENGTH) { 
			++i;
		}
	}
	return index;
}

int pickGuess(char * word, int index) {
	int i = 0;
	fseek(fpAll, 0, SEEK_SET);
	while (i <= index && fgets(word, WORD_LENGTH + 1, fpAll) != NULL) { //fgets stores the solution into word
		if (strlen(word) == WORD_LENGTH) { 
			++i;
		}
	}
	return index;
} 