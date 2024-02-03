// Projekat Tetris
// by Vrnjak Lamija & Selimović Denis
// Elektrotehnički fakultet Sarajevo

// clang-format off

//////// PORTAPACK CHANGES HIGHLIGHTED
int main();
void pause_game();
//////// PORTAPACK

#include "mbed.h"
#include "SPI_TFT_ILI9341.h"
#include "Arial12x12.h"

#define dp23 P0_0

//deklaracija display-a
SPI_TFT_ILI9341 display(dp2, dp1, dp6, dp24, dp23, dp25, "TFT");

//////// PORTAPACK - DISABLED ANALOGIN CLASS DUE TO OBJECT INITIALIZER CODE NOT RUNNING:
//analogni ulazi za joystick
// AnalogIn VRx(dp11);
// AnalogIn VRy(dp10);
//////// PORTAPACK

//taster na joysticku za rotaciju
InterruptIn taster(dp9);

//////// PORTAPACK - DISABLED ANALOGIN CLASS DUE TO OBJECT INITIALIZER CODE NOT RUNNING:
// AnalogIn random(dp13); //analogni ulaz za generisanje random vrijednosti
//////// PORTAPACK

//ticker za spustanje figure
//timer za debouncing tastera na joysticku
Ticker game, joystick;
Timer debounceTaster;


unsigned char level = 0; //mora biti tipa usigned char jer inače se može desiti da level bude manji od 0, a i da ne trošimo memoriju
const float delays[4] = {1.2, 0.7, 0.4, 0.25}; //svakih koliko se spusti jedan red, ovo provjeriti da li je presporo ili prebrzo, ovisi o levelu

//////// PORTAPACK - UNNEEDED JOYSTICK HYSTERESIS VARIABLES
//char leftBoundary = 1, rightBoundary = 5, downBoundary = 1, upBoundary = 5;// sada je ovo tipa char
//////// PORTAPACK

unsigned int score = 0; //stavio sam ovo unsigned int za veći opseg, mada je jako teško da se i int premaši, ali nmvz
bool firstTime = true; //ako je prvi put, figura se crta u Tickeru
bool gameStarted = false;
unsigned char nextFigure = 1; //ovo je sad globalna varijabla, da bi mogli unaprijed generisati sljedeću figuru radi prikaza


//white - no figure
//I - BLUE
//O - YELLOW
//T - PURPLE
//S - GREEN
//Z - RED
//J - MAROON
//L - ORANGE
const int colors[8] = {White, Blue, Yellow, Purple, Green, Red, Maroon, Orange};
const short DIMENSION = 16;
const short DIMENSION_NEXT = 12;
short board[20][10] = {0}; //matrica boja, 0 - 7 indeksi boja

short figuresX[7][4] = {{0,0,0,0}, {0,0,1,1}, {0,1,1,1}, {1,1,0,0}, {0,1,0,1}, {1,1,1,0}, {1,1,1,0}};
short figuresY[7][4] = {{0,1,2,3}, {1,0,0,1}, {1,1,2,0}, {0,1,1,2}, {0,1,1,2}, {2,1,0,0}, {0,1,2,2}};

unsigned int GenerateRandomSeed() {
//////// PORTAPACK - USE RTC FOR SEED
return LPC_RTC->CTIME0;
    // unsigned int randomNumber = 0;
    // for(int i = 0; i <= 32; i += 2) {
    //     randomNumber += ((random.read_u16() % 3) << i);
    //     wait_us(10);
    // }
    // return randomNumber;
//////// PORTAPACK
}

void Init() {
   //ovo su zajedničke osobine za sve prikaze na display-u
   //nikad se ne mijenjaju i pozvat ćemo je jednom prije petlje
    display.claim(stdout);
    display.set_orientation(2); // 2 ili 0, zavisi kako okrenemo display, provjerit ćemo na labu kako nam je najlakše povezat
    display.set_font((unsigned char*) Arial12x12);
}


void ShowScore() {
    //pomocna funkcija za prikazivanje score-a
    display.fillrect(165, 20, 235, 50, White); //popunimo pravugaonik da obrišemo stari score
    display.locate(200, 35); //valjda je na sredini pravougaonika
    printf("%d", score);
}

void ShowNextFigure() {
    //prikaz sljedeće figure koristeći pomoćnu varijablu nextFigure
    display.fillrect(165, 70, 235, 120, White);
    int upperLeftX = 176, upperLeftY = 83;
    for(int i = 0; i < 4; i++) {
        int x = upperLeftX + DIMENSION_NEXT * figuresY[nextFigure - 1][i], y = upperLeftY + DIMENSION_NEXT * figuresX[nextFigure - 1][i];
        display.fillrect(x, y, x + DIMENSION_NEXT, y + DIMENSION_NEXT, colors[nextFigure]);
        display.rect(x, y, x + DIMENSION_NEXT, y + DIMENSION_NEXT, Black);
    }
}

//funkcija za crtanje cursora za odabir levela
void DrawCursor(int color, unsigned char lev) {
    display.fillrect(60, lev * 70 + 50, 72, lev * 70 + 50 + 12,  color);
}

// PORTAPACK - ADDED EXTRA LEVEL:
void ShowLevelMenu() {
    //ovdje inicijalizujemo display
    display.cls(); // brišemo prethodno
    display.background(Black);
    display.foreground(White);
    display.locate(80, 50);
    printf("LEVEL 1");
    display.locate(80, 120);
    printf("LEVEL 2");
    display.locate(80, 190);
    printf("LEVEL 3");
    display.locate(80, 260);
    printf("LEVEL 4");
    DrawCursor(White, level);
}

//////// PORTAPACK - KLUDGED FOR BUTTONS VS JOYSTICK:
void ReadJoystickForLevel(){
    unsigned char old = level;
    if(but_UP){
//        upBoundary = 4;
        (level == 0) ? level = 3 : level--;
    }
    else if(but_DOWN){
        //ne radi ona prethodna varijanta jer % vraća i negastivni rezultat
        //to što ne koristimo unsigned tip ne pomaže jer će doći do overflow-a
//        downBoundary = 2;
        level = (level + 1) % 4;
    }
    else {
//        downBoundary = 1;
//        upBoundary = 5;
    }
    DrawCursor(Black, old); //na prethodni level popunimo bojom pozadine
    DrawCursor(White, level); //na novi level popunimo bijelom bojom - pozadina je crna
   //koristio sam fillrect, jer njega svakako moramo koristiti, jer možda budemo morali da brišemo fillcircle iz biblioteke
}
//////// PORTAPACK

void EndPlay() {
    joystick.detach();
    score = 0;
    firstTime = true;
    gameStarted = false;
    for(int i = 0; i < 20; i++) {
        for(int j = 0; j < 10; j++) {
            board[i][j] = 0;
        }
    }
//////// PORTAPACK - FIX TO REINITIALIZE SCREEN
//    ShowLevelMenu();
//    joystick.attach(&ReadJoystickForLevel, 0.3);
main();
//////// PORTAPACK
}

void StartGame()
{
    display.cls(); // brišemo ShowLevelMenu
    display.background(White);
    display.foreground(Black);
    display.fillrect(0, 0, 160, 320, White);
    display.fillrect(160, 0, 240, 320, Black); //dio za prikazivanje rezultata će biti crni pravougaonik, a tabla je bijeli
    ShowScore();
}

void copyCoordinates(short X[], short Y[], unsigned char index)
{
    //umjesto one prošle metode za kopiranje, ova prima index, čisto da nam olakša život
    for(int i = 0; i < 4; i++) {
        X[i] = figuresX[index][i];
        Y[i] = figuresY[index][i];
    }
}

bool BottomEdge(int x){
    return x > 19;
}

bool LeftEdge(int y){
    return y  < 0;
}

bool RightEdge(int y){
    return y > 9;
}

bool OutOfBounds(int y, int x){
    return y < 0 || y > 9 || x > 19;
}

void PutBorders(short x, short y) {
    for(int i = x - 1; i <= x + 1; i++) {
        for(int j = y - 1; j <= y + 1; j++) {
            if(i < 0 || i > 9 || j < 0 || j > 19 || board[j][i] == 0) continue;
            display.rect(i * DIMENSION, j * DIMENSION, (i + 1) * DIMENSION, (j + 1) * DIMENSION, Black);
        }
    }
}

class Tetromino{
private:
    short X[4];
    short Y[4];
    short boardX, boardY;
    unsigned char colorIndex;//dodao sam colorIndex zasad, jer nema drugog načina da popunimo matricu sa indeksima boja
    //ovo je najbezbolnija varijanta što se memorije tiče
public:
    Tetromino(){
//////// PORTAPACK - NOTE - DEFAULT INITIALIZER CODE DOESN'T GET EXECUTED FOR SOME REASON:
        unsigned char r = rand() % 7 + 1;
        Initialize(r);
    }

    Tetromino(unsigned char colorIndex) {
        Initialize(colorIndex);
    }

    void Initialize(unsigned char colorIndex) {
        Tetromino::colorIndex = colorIndex;
        boardX = 0;
        boardY = 4; //3,4 ili 5 najbolje da vidimo kad imamo display
        copyCoordinates(X, Y, colorIndex - 1);
    }

    void Rotate(){
       short pivotX = X[1], pivotY = Y[1];
        //prvi elemnti u matrici su pivoti oko koje rotiramo

       short newX[4]; //pozicije blokova nakon rotiranja ako sve bude ok
       short newY[4];

       for(int i = 0; i < 4; i++){
           short tmp = X[i], tmp2 = Y[i];
           newX[i] = pivotX + pivotY - tmp2;
           newY[i] = tmp + pivotX - pivotY;

           if(OutOfBounds(boardY + newY[i], boardX + newX[i]) || board[boardX + newX[i]][boardY + newY[i]] != 0) return;
       }
       DeleteFigure();
       for(int i = 0; i < 4; i++){
           X[i] = newX[i];
           Y[i] = newY[i];
       }
       DrawFigure();
   }

    void DrawFigure() {
        for(int i = 0; i < 4; i++) {
            //display je deklarisani display logy iz biblioteke one
            //računamo gornji lijevi pixel po x i y
            //donji desni dobijemo kad dodamo DIMENZIJU
            //stavio sam 16 za početak, možemo se opet skontati na labu
            //ovo pretpostavlja da nema margina, mogu se lagano dodati uz neku konstantu kao offset
            int upperLeftX = (boardX + X[i]) * DIMENSION, upperLeftY = (boardY + Y[i]) * DIMENSION;
            display.fillrect( upperLeftY, upperLeftX, upperLeftY + DIMENSION, upperLeftX + DIMENSION, colors[colorIndex]);
            //ovo boji granice blokova u crno, možemo skloniti ako ti se ne sviđa

//////// PORTAPACK - HIDE DEFAULT WHITE BLOCK (ALTERNATE KLUDGE FOR TETRONIMO INITIALIZATION CODE NOT RUNNING AT CONSTRUCTION)
if (colorIndex != White)
//////// PORTAPACK
            display.rect( upperLeftY, upperLeftX, upperLeftY + DIMENSION, upperLeftX + DIMENSION, Black);
        }
    }

    void DeleteFigure() {
        for(int i = 0; i < 4; i++) {
            //ista logika kao u DrawFigure, samo popunjavamo sve blokove sa bijelim pravougaonicima
            short upperLeftX = (boardX + X[i]) * DIMENSION, upperLeftY = (boardY + Y[i]) * DIMENSION;
            display.fillrect( upperLeftY, upperLeftX, upperLeftY + DIMENSION, upperLeftX + DIMENSION, White);
            PutBorders(upperLeftY, upperLeftX);
        }
    }

    void OnAttached() {
        //metoda se poziva kad figura prestanje da se krece
        //popunimo matricu indeksom boje
        for(int i = 0; i < 4; i++) {
            board[boardX + X[i]][boardY + Y[i]] = colorIndex;
            //btw board bi mogao biti niz tipa unsigned char, ali to ćemo vidjet kasnije
        }
    }

   bool MoveDown(char delta = 1){
//////// PORTAPACK - MOVE DEFAULT WHITE BLOCK TO BOTTOM IMMEDIATELY (ALTERNATE KLUDGE FOR TETRONIMO INITIALIZATION CODE NOT RUNNING AT CONSTRUCTION)
if (colorIndex == White) delta = 19;
//////// PORTAPACK
       if(!InCollisionDown(delta)){
            DeleteFigure();
            boardX+=delta;
            DrawFigure();
            return true;
       }

       return false;
   }

   void MoveLeft(){
       if(!InCollisionLeft()){
           DeleteFigure();
           boardY--;
           DrawFigure();
       }
   }

   void MoveRight(){
       if(!InCollisionRight()){
           DeleteFigure();
           boardY++;
           DrawFigure();
       }
   }

   void SoftDrop() {
       //ovo je funkcija za soft drop
       //obrisemo figuru, postavimo novu poziciju dva reda ispod, nacrtamo figuru
       DeleteFigure();
       MoveDown(2);
       DrawFigure();
       //treba još vidjeti koje izmjene u tickeru trebaju
       score += 2 * (level +1); //prema igrici koju smo igrali, dobije se 14 poena kad se uradi hard drop
       ShowScore();
   }


   bool InCollisionDown(char delta = 1){
       int newX, newY; //da bi bilo citljivije

       for(int i = 0; i < 4; i++){
        newX = boardX + X[i] + delta;
        newY = boardY + Y[i];

        if(BottomEdge(newX) || board[newX][newY] != 0){
            return true;
        }
        //jedna figura je u koliziji ako
        //pozicija na koju zelimo da pomjerimo bilo koji blok dotakne dno ili lijevu ili desnu ivicu ekrana
        //ili ako je pozicija na koju zelimo da pomjerimo bilo koji od blokova vec zauzeta a da nije dio figure prije pomijeranja
       }

       return false;
   }

   bool InCollisionLeft(){
       int newX, newY;

       for(int i = 0; i < 4; i++){
        newX = boardX + X[i];
        newY = boardY + Y[i] - 1;

        if(LeftEdge(newY) || board[newX][newY] != 0){
            return true;
        }
       }

       return false;
   }


   bool InCollisionRight(){
       int newX, newY;

       for(int i = 0; i < 4; i++){
        newX = boardX + X[i];
        newY = boardY + Y[i] + 1;

        if(RightEdge(newY) || board[newX][newY] != 0){
            return true;
        }
       }

       return false;
   }
};


Tetromino currentTetromino;

//////// PORTAPACK - KLUDGED FOR BUTTONS VS JOYSTICK:
void ReadJoystickForFigure() {
    if(but_LEFT) {
//        leftBoundary = 2;
        currentTetromino.MoveLeft();
    }
    else if(but_RIGHT) {
//        rightBoundary = 4;
        currentTetromino.MoveRight();
    }
    else if(but_UP) {
//        downBoundary = 2;
pause_game();
    }
    else if(but_DOWN) {
//        upBoundary = 4;
        currentTetromino.SoftDrop();
    }
    else {
//        leftBoundary = 1;
//        rightBoundary = 5;
//        downBoundary = 1;
//        upBoundary = 5;
    }
}
//////// PORTAPACK

void CheckLines(short &firstLine, short &numberOfLines)
{
    //vraća preko reference prvu liniju koja je popunjena do kraja, kao i takvih linija
    firstLine = -1; //postavljen na -1 jer ako nema linija koje treba brisati ispod u UpdateBoard neće se ući u petlju
    numberOfLines = 0;
    for(int i = 19; i >= 0; i--) {
        short temp = 0;
        for(int j = 0; j < 10; j++) {
            if(board[i][j] == 0) {
                if(numberOfLines > 0) return;
                break;
            }//ako je makar jedna bijela, prekida se brojanje
            temp++;
        }
        if(temp == 10) { //ako je temo došao do 10, niti jedna bijela - puna linija
            numberOfLines++;
            if(firstLine == -1) firstLine = i; //ovo mijenjamo samo prvi put
        }
    }
}

unsigned int UpdateScore (short numOfLines){
    unsigned int newIncrement = 0;
    switch(numOfLines) {
        case 1 : newIncrement =  40; break;
        case 2 : newIncrement =  100; break;
        case 3 : newIncrement =  300; break;
        case 4 : newIncrement =  1200; break;
        default : newIncrement = 0; break; //update funkcije za score, još sam vratio ovo na 0
    }
    return newIncrement * (level + 1);
}

void UpdateBoard()
{
    short firstLine, numberOfLines;
     //pozivamo funkciju
    do {
        CheckLines(firstLine, numberOfLines);
        for(int i = firstLine; i >= numberOfLines; i--) {
            for(int j = 0; j < 10; j++) {
                board[i][j] = board[i - numberOfLines][j];
                board[i - numberOfLines][j] = 0;
                short tmp = i - numberOfLines;
                display.fillrect( j * DIMENSION,i * DIMENSION, (j + 1) * DIMENSION , (i + 1) * DIMENSION , colors[board[i][j]]); // bojimo novi blok
                display.fillrect( j * DIMENSION, tmp * DIMENSION,(j + 1) * DIMENSION, (tmp + 1) * DIMENSION , White);
                if(board[i][j] != 0)
                    display.rect( j * DIMENSION,i * DIMENSION, (j + 1) * DIMENSION , (i + 1) * DIMENSION , Black);
            }
        }
        score += UpdateScore(numberOfLines);
    }
    while(numberOfLines != 0); //ovdje se mijenja globalna varijabla score
}

bool IsOver() {
    for(int i = 0; i < 10; i++) {
        if(board[0][i] != 0) return true;
    }
    return false;
}

void ShowGameOverScreen() {
//////// PORTAPACK - SKIP CLS
//    display.cls();
//    display.background(Black);
//    display.foreground(White);
display.background(White);
display.foreground(Black);
//////// PORTAPACK
    display.locate(60, 120);
    printf("GAME OVER");
    display.locate(40, 150);
    printf("YOUR SCORE IS %d", score);
    wait(5); //ovaj prikaz traje 3s (možemo mijenjati) a nakon toga se ponovo prikazuje meni sa levelima
}

void InitGame() {
    if(firstTime) {
//////// PORTAPACK - NOTE - ATTEMPTED WORKAROUND FOR SKIPPED INITIALIZER CODE - BUT ANY OF THESE CRASHES FIRMWARE AT POWER-UP EVEN IF THERE'S NO TETRIS APP INSTALLED:
// currentTetromino = Tetromino(rand() % 7 + 1);  // TEST #1
// currentTetromino.Initialize(rand() % 7 + 1);   // TEST #2
//////// PORTAPACK
        currentTetromino.DrawFigure();
        nextFigure = rand() % 7 + 1;
        ShowNextFigure();
        firstTime = false;
    }
}

void PlayGame(){
    InitGame();
    if(!currentTetromino.MoveDown()){
        currentTetromino.OnAttached();
        UpdateBoard();
        ShowScore();

        currentTetromino = Tetromino(nextFigure);
        currentTetromino.DrawFigure();
        nextFigure = rand() % 7 + 1;
        ShowNextFigure();
        if(IsOver()) {
            //ako je igra završena brišemo sve sa displey-a, prikazujemo poruku i score
            //takođe moramo dettach-at ticker
            game.detach();
            ShowGameOverScreen(); //prikaz da je kraj igre, ima wait od 3s
            EndPlay();
        }
    }

}

void OnTasterPressed(){
    if(debounceTaster.read_ms() > 200) {
        if(gameStarted){
            currentTetromino.Rotate();
        }
        else{
            joystick.detach();
            gameStarted = true;
            StartGame(); //pocinje igra, prikazuje se tabla
            joystick.attach(&ReadJoystickForFigure, 0.3);
            game.attach(&PlayGame, delays[level]); //svakih nekoliko spusta figuru jedan red nize
        }
        debounceTaster.reset();
    }
}

void SetTaster() {
    taster.mode(PullUp); //mora se aktivirati pull up otpornik na tasteru joystick-a
    taster.fall(&OnTasterPressed);
    debounceTaster.reset();
    debounceTaster.start();
}

int main() {
    srand(GenerateRandomSeed());
    Init();
    ShowLevelMenu();
    joystick.attach(&ReadJoystickForLevel, 0.3);
    SetTaster();

//////// PORTAPACK: CAN'T HANG HERE IN MAYHEM OR WE WON'T GET ANY CALLBACKS
//    while(1);
return 0;
//////// PORTAPACK
}

//////// PORTAPACK - ADDED PAUSE FEATURE:
void pause_game() {
    game.detach();
    joystick.detach();
    display.locate(180, 200);
    printf("PAUSED");
    while ((get_switches_state().to_ulong() & 0x10) == 0);  // wait for SELECT button to resume
    printf("      ");
    joystick.attach(&ReadJoystickForFigure, 0.3);
    game.attach(&PlayGame, delays[level]); //svakih nekoliko spusta figuru jedan red nize
};
//////// PORTAPACK