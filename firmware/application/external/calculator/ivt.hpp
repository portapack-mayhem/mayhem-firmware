/*

## Copyright 2021, zooxo/deetee
All rights reserverd

## The 3-Clause BSD License

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors
   may be used to endorse or promote products derived from this software without
   specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


  IVT (IVEE-TINY) - A FORTH-programable Scientific RPN Calculator
  that fits in 8 kilobytes (Arduino, ATTINY85)

  ____________________

   PREAMBLE
  ____________________

  IVT (IVEE-TINY) is the smallest member of the IV-calculator series (FORTH-like
  programable calculators). The name Ivee or IV stands for the roman number 4,
  which was the basis for naming FORTH (4th generation programming language).

  The hardware is simple:
    - AVR ATTINY85 microcontroller (8192/512/512 bytes of FLASH/RAM/EEPROM)
    - OLED-display (128x32, I2C, SSD1306)
    - 16 keys touch sensitive keypad (TTP229-BSF, 2-wire)
    - CR2032 battery

  IVT was made without making compromises to offer a complete FORTH-like
  programming environment and a maximum of mathematical functions (mostly
  written in FORTH itself). And it's amazing, how much calculating power fits
  into 8 kilobytes:
    - 80 intrinsic functions based on FORTH
    - A wide range of mathematical and scientific commands
      (DUP DROP SWAP ROT OVER / * - + PI SQRT POWER INV INT
      EXP LN SIN COS TAN ASIN ACOS ATAN GAMMA P>R R>P nPr nCr)
    - Statistics, line best fit and normal distibution (CDF/PDF)
    - Present value calculations
    - Programming: Up to 16 user definable programs with a total of 440 steps
      (< = <> > IF ELSE THEN BEGIN UNTIL)
    - A solver to find roots of user defined functions
    - A dictionary of all commands, words and programs
    - An user definable menu for fast access to all commands, words and programs
    - Storing of 10 numbers/constants (permanently)
    - Adjustable brightness of the display

  Have fun!
  deetee

  ____________________

   COMPILING
  ____________________

    As IVT consumes 8190 bytes of flash memory (maximal 8192 bytes possible)
    compiling with proper settings is essential. Use the Arduino IDE, load the
    right library for the ATTINY85 and use the following settings:

      - Library: "attiny by Davis A. Mellis"
      - Board: "ATtiny25/45/85 (No bootloader)"
      - Chip: "ATtiny85"
      - Clock: "8 MHz (internal)"
      - B.O.D. Level: B.O.D. Disabled
      - Save EEPROM: "EEPROM retained"
      - Timer 1 Clock: "CPU (CPU frequency)"
      - LTO: "Enabled"
      - millis()/micros(): "Disabled"

  ____________________

   KEYBOARD
  ____________________

    F(MENU) 7(SUM+) 8(PRG)  9(/)
    E(SWAP) 4(DICT) 5(USR)  6(*)
    N(ROT)  1(RCL)  2(STO)  3(-)
    C(CA)   0(PI)   .(INT)  D(+)

  ____________________

   LIMITS
  ____________________

    As a microprocessor is primarily not made to do such complex things like
    performing a powerful calculator there are some limits in performance and
    resources.
    Most obvious is the limited precision of the intrinsic float format (IEEE
    754, 32 bit). As four bytes only are used to represent a float respective
    double number the decimal digits of precision are limited to 6...7.

    In addition the resources of a microcontroller are limited like the FLASH
    memory (holds the executable program code), the RAM memory (holds variables
    and data while running) and the EEPROM (holds permanent data like settings
    or user programs). Due to the target of maximal calculating power IVT lacks
    on many "features of comfort". For example there is no error control - a
    division by zero results in a "non interpretable" display.

    However IVT tries to offer a maximum of features, comfort and performance
    with a minimum of required resources.

    LIMITS:
      24   ... Maximal data stack size
      7    ... Maximum number of displayed significant digits of a number
      10   ... Maximal amount of numbers saved permanently (0~9)
      16   ... Maximal number of user programs
      440  ... Maximal size (steps) of all user programs
      32   ... Maximal definable command slots of user menu

  ____________________

   BROWSING MENUS
  ____________________

    To navigate through the menu of some functions (MENU, DICT or USR)
    all selectable items are divided into four sections. Every section has its
    own up and down key (section I: E/N, section II: 4/1, section III: 5/2 and
    section IV: 6/3).
    To select one of the four presented items use the appropriate function key
    (F/7/8/9) or escape the menu with "C".
    There is some kind of hidden feature: If you leave the menu with the D-key
    the brightness of the display will be set (0~255, not permanent!).

  ____________________

   COMMANDS
  ____________________

     BASIC KEYS:
      0~9.     ... Digits and decimal point
      EE N     ... 10-exponent (actually Y*10^X) and negate (change sign)
      D        ... DUP (push stack) or complete number input
      C        ... DROP top of stack or clear entry when in number input
      F        ... Shift to select function keys or double press for user menu

     FUNCTION KEYS:
      + - * /  ... Basic operations
      MENU     ... Browse (and select) the user menu
      SUM+     ... Enter X-data for statistics or X,Y-data for linear regression
      PRG      ... Edit program (00~15) - enter program number first
      SWAP     ... Swap the two top numbers of stack (1 2 -> 2 1)
      DICT     ... Browse (and select) the complete dictionary
      USR      ... Set dictionary entry to user menu
      ROT      ... Rotate stack (1 2 3 -> 2 3 1)
      STO RCL  ... Store/recall number - enter memory number first (0~9)
      CA       ... Clear stack and memories for statistics (5~7)
      PI       ... Push PI to stack
      INT      ... Integer value

     DICTIONARY (4 sections):
      F  7  8  9   ... F-key, numbers
      EE 4  5  6   ... 10-exponent, numbers
      N  1  2  3   ... NEGATE, numbers
      C  0  .  D   ... Clear/DROP, number, dot, DUP
      M  S+ PR /   ... MENU, SUM+, PRG edit, divide

      >< DC US *   ... SWAP, DICT, set USER menu, multiply
      RT RC ST -   ... ROT, RCL, STO, subtract
      CA PI IN +   ... CA (clear all), PI, INT, add
      IF EL TH PC  ... IF, ELSE, THEN, nPr/nCr
      <  =  <> >   ... LESSTHAN, EQUAL, NOTEQUAL, GREATERTHAN

      BE UN SO I   ... BEGIN, UNTIL, SOLVE, INV (1/X)
      c  t- E  LN  ... COS, ATAN, EXP, LN
      s  t  s- c-  ... SIN, TAN, ASIN, ACOS
      OV SQ yx !L  ... OVER, SQRT, POW (y^x), ln(GAMMA)
      PV ND P> R>  ... Present value, normal distribution, P->R, R->P

      S+ Sc x- LR  ... SUM+, SUMclr (clears memories 5~9), MEAN/STDEV, L.R.
      00 01 02 03  ... User programs
      04 05 06 07  ... User programs
      08 09 10 11  ... User programs
      12 13 14 15  ... User programs

  ____________________

   PV, ND, P<>R, STAT
  ____________________

    PV     ... Present value of given interest rate (ie. 0.08) and periods
    ND     ... PDF (X) and CDF (Y) of standard normal distribution
    P> R>  ... Polar/Rectangular conversion
    STAT   ... Mean value (X) and standard deviation (Y).
               Note that the memories 5~9 (see RCL/STO) are used as statistic
               registers (Sxx, Sxy, n, Sx, Sy).
    LR     ... Line best fit (y = X * x + Y)

  ____________________

   PROGRAMMING
  ____________________

    IVT is able to deal with up to 16 user programs (named 00~15) with a total
    number of 440 steps/commands. The maximal size per user program rises from
    20 steps (program 00) to 35 steps (program 15).

    To edit a program, enter the program number (00~15) followed by PRG (F-8).
    The display shows P (for program), the program number (00~15), the program
    step number (vary with cursor keys E and N) and the command of this step.
    To insert a program step
      - press a key (number or DUP),
      - press a shifted key (press F twice to toggle) or
      - press DICT (F-4) to select a command from the dictionary.
    To delete a program step press ".".
    Leave and save the program with "C".
    To execute a program select the appropriate program number/name from DICT.
    Please note that the first user program (00) will be used by the solver.

  ____________________

   SOLVER
  ____________________

    To find a root of a function (programmed in user program 00) enter an
    appropriate start value and select the command SO from the dictionary.

  ____________________

   POWER CONSUMPTION
  ____________________

    As IVT provides a maximum of calculating power there where less resources
    for a good power management (i.e. idle, screen saver, auto power off) left.
    Merely the not needed timer1 and AD-convertes are set off to save
    approximately 0.9 mA.
    The power consumption of IVT depends mainly on the usage of the display.
    That's why per default the brightness of the display is set to minimum.
    Please note that you can (not permanently) set the brightness of the display
    with pressing D when in any menu (takes brightness value from stack).
    In total IVT consumes approximately 9 mA - so a a single battery (CR2032)
    which has a capacity of approximately 200 mAh should theoretically work at
    least 20 hours.
    Electrical current drawn by device (approximately):
      - ATTINY85 ... 5 mA
      - Keypad   ... 2 mA
      - Display  ... 1~4 mA (promt ~ display full of 8's)

  ____________________

   PROGRAM EXAMPLES
  ____________________

  ABS:   DUP 0 LT IF NEG THEN
  FRAC:  DUP INT -
  SINH:  EXP DUP INV NEG + 2 /          ... sinh=(exp(x)-exp(-x))/2
  COSH:  EXP DUP INV + 2 /              ... cosh=(exp(x)+exp(-x))/2
  TANH:  2 * EXP DUP 1 - SWAP 1 + /     ... tanh=(exp(2*x)-1)/(exp(2*x)+1)
  ASINH: DUP DUP * 1 + SQRT + LN        ... asinh(x)=ln(x+sqrt(x*x+1))
  ACOSH: DUP DUP * 1 - SQRT + LN        ... acosh(x)=ln(x+sqrt(x*x-1))
  ATANH: DUP 1 + SWAP NEG 1 + / SQRT LN ... atanh(x)=ln(sqrt((1+x)/(1-x)))
  POW10: 1 SWAP EE
  LOG:   LN 1 0 LN /                    ... log(x)=ln(x)/ln(10)
  %: OVER / 1 0 0 *                     ... %=x/B*100%
  CHG%: OVER - OVER / 1 0 0 *           ... chg%=(x-B)/B*100%

  QE: OVER 2 / DUP * SWAP - SQRT SWAP 2 / NEG
  SWAP OVER OVER - ROT ROT +            ... x12=-p/2+-sqrt(p*p/4-q)

  DEG<>RAD: DUP PI * 1 8 0 / SWAP 1 8 0-* PI /
  C<>F: DUP 1 . 8 * 3 2 + SWAP 3 2 - 1 . 8 /
  KM<>MI: DUP 1 . 6 0 9 3 4 4 DUP DUP ROT SWAP / ROT ROT *
  M<>FT: DUP 3 . 3 7 0 0 7 9 DUP DUP ROT * ROT ROT /
  CM<>IN: DUP 2 . 5 4 DUP DUP ROT SWAP / ROT ROT *
  KG<>LBS: DUP 2 . 2 0 4 6 2 3 DUP DUP ROT * ROT ROT /
  L<>GAL:  DUP 3. 7 8 5 4 1 2 DUP DUP ROT SWAP / ROT ROT *

  HMS2H: DOT 0 0 0 0 0 1 ADD // Round up to prevent leaps
  DUP DUP INT SWAP OVER - 1 0 0 * INT // hh mm
  ROT 3 PICK SUB 1 0 0 MULT OVER SUB 1 0 0 MULT, // ss
  3 6 0 0 / SWAP 6 0 / + + // ->s ->h

  H2HMS: DUP 3 6 0 0 * DUP ROT INT // h->s
  SWAP OVER 3 6 0 0 * - 60 DIV INT, // hh mm
  ROT OVER 6 0 MULT SUB 3 PICK 3 6 0 0 * - // ss
  1 0 0 0 0 / SWAP 1 0 0 / + + // hh.mmss

  nPr = n!/(n-r)! : OVER ROT ROT - 1 ROT ROT SWAP
  BEGIN SWAP ROT 1 ROT + DUP ROT * SWAP ROT OVER OVER SWAP LT UNTIL DROP DROP

  nCr = n!/(n-r)!/r! = nPr/r! : DUP ROT SWAP PERM 1 ROT
  BEGIN ROT ROT DUP ROT SWAP / ROT ROT 1 + SWAP OVER 1 - OVER SWAP LT UNTIL DROP DROP

  ____________________

   ATTINY85 PINS
  ____________________

                          _____
      Analog0/Reset P5 H1|* U  |H8     Vcc
            Analog2 P3 H2|     |H7 P2  SCK/Analog1
            Analog3 P4 H3|     |H6 P1  PWM1/MISO
                GND    H4|_____|H5 P0  PWM0/AREF/SDA/MOSI

  ____________________

   CIRCUIT DIAGRAM
  ____________________

     _________________
    |   OLED-DISPLAY  |
    |  128x32 SSD1306 |
    |_GND_VCC_SCK_SDA_|
       |   |   |   |
               |   |
     __|___|___|___|__
    | GND VCC P2  P0  |
    |  AVR  ATTINY85  |
    |_________P3__P1__|
               |   |
               |   |
     __|___|___|___|__
    | VCC GND SCL SDO |
    |Keypad TTP229-BSF|
    |-----------------|
    |  O O O O        |
    |                 |
    |  O O O O        |
    |  O O O O        |
    |      |          |... Solder to enable
    |  O O O O        |    16-key-mode (TP2=0)
    |                 |


*/

// ***** I N C L U D E S

// #include <TinyWireM.h>  // I2C wire communication with display
// #include <avr/power.h>  // Needed for (simple) power management
// #include <EEPROM.h>     // For saving data to EEPROM

// ***** F O N T (4x8)

// Compatibility
#define PROGMEM
typedef uint8_t byte;
typedef bool boolean;
uint8_t dummy = 0;

class A {
   public:
    void write(uint32_t ee, uint8_t p) {
        (void)ee;
        (void)p;
    }
    uint8_t read(uint32_t ee) {
        (void)ee;
        return 0;
    }
    uint8_t& operator[](uint32_t ee) {
        (void)ee;
        return dummy;
    }
    size_t length() {
        return 0;
    }
} EEPROM;

static void _keyf(void);
static void _num(void);
static void _e(void);
static void _neg(void);
static void _drop(void);
static void _dot(void);
static void _dup(void);
static void _menu(void);
static void _sumadd(void);
static void _prgedit(void);
static void _div(void);  // 16 F-keys

static void _swap(void);
static void _dict(void);
static void _usrset(void);
static void _mul(void);
static void _rot(void);
static void _mrcl(void);
static void _msto(void);
static void _sub(void);
static void _clr(void);
static void _pi(void);
static void _int(void);
static void _add(void);
static void _condif(void);
static void _condelse(void);
static void _condthen(void);
static void _permcomb(void);  // 32 Intrinsic functions
static void _condlt(void);
static void _condeq(void);
static void _condne(void);
static void _condgt(void);

static void _begin(void);
static void _until(void);
static void _solve(void);
static void _inv(void);
static void _cos(void);
static void _atan(void);
static void _exp(void);
static void _ln(void);
static void _sin(void);
static void _tan(void);
static void _asin(void);
static void _acos(void);  // 48 Builtin functions
static void _over(void);
static void _sqrt(void);
static void _pow(void);
static void _gammaln(void);
static void _pv(void);
static void _nd(void);
static void _pol2rect(void);
static void _rect2pol(void);

static void _sumadd(void);
static void _sumclr(void);
static void _sumstat(void);
static void _sumlr(void);

#undef __I
#undef __N
#undef __O
#undef __P

#define PI 3.141592653589793

static void _numinput(byte k);
static double dpush(double d);
static double dpop(void);
static void seekmem(byte n);
static double dpush(double d);
static void apush(int addr);
static int apop(void);
static void _condseek(void);
static int eeudist(byte i);
static void _mstorcl(boolean issto);

template <class T>
void EEwrite(int ee, const T& value);

template <class T>
void EEread(int ee, T& value);

#define pgm_read_byte(_x) (*(_x))

// Font table
#define __0 0
#define __1 1
#define __2 2
#define __3 3
#define __4 4
#define __5 5
#define __6 6
#define __7 7
#define __8 8
#define __9 9
#define __A 10
#define __B 11
#define __C 12
#define __D 13
#define __E 14
#define __F 15
#define __H 16
#define __I 17
#define __L 18
#define __M 19
#define __N 20
#define __O 21
#define __P 22
#define __R 23
#define __S 24
#define __T 25
#define __U 26
#define __V 27
#define __c 28
#define __s 29
#define __t 30
#define ___ 31  // space
#define __DOT 32
#define __MULT 33
#define __ADD 34
#define __SUB 35
#define __DIV 36
#define __EM 37  // !
#define __LT 38
#define __EQ 39
#define __GT 40
#define __ARROW 41
#define __FINV 42
#define __POW1 43
#define __PI 44
#define __SQRT 45
#define __MEAN 46
#define __SWAP 47

#define FONTWIDTH 4  // Font width
const byte font[] PROGMEM = {
    0xff, 0x81, 0x81, 0xff,  // 0
    0x00, 0x02, 0xff, 0x00,  // 1
    0xf9, 0x89, 0x89, 0x8f,  // 2
    0x81, 0x89, 0x89, 0xff,  // 3
    0x0f, 0x08, 0x08, 0xff,  // 4
    0x8f, 0x89, 0x89, 0xf9,  // 5
    0xff, 0x89, 0x89, 0xf8,  // 6
    0x03, 0x01, 0x01, 0xff,  // 7
    0xff, 0x89, 0x89, 0xff,  // 8
    0x0f, 0x89, 0x89, 0xff,  // 9
    0xff, 0x09, 0x09, 0xff,  // A
    0xff, 0x89, 0x8f, 0xf8,  // B
    0xff, 0x81, 0x81, 0x80,  // C
    0x81, 0xfF, 0x81, 0xfF,  // D
    0xfF, 0x89, 0x89, 0x81,  // E
    0xfF, 0x09, 0x09, 0x01,  // F
    0xfF, 0x08, 0x08, 0xfF,  // H
    0x81, 0xff, 0x81, 0x80,  // I
    0xfF, 0x80, 0x80, 0x80,  // L
    0xfF, 0x06, 0x06, 0xfF,  // M
    0xfF, 0x0c, 0x18, 0xfF,  // N
    0xff, 0x81, 0x81, 0xff,  // O
    0xfF, 0x09, 0x09, 0x0f,  // P
    0xfF, 0x09, 0xf9, 0x8f,  // R
    0x8f, 0x89, 0x89, 0xf8,  // S
    0x01, 0xff, 0x01, 0x01,  // T
    0xfF, 0x80, 0x80, 0xfF,  // U
    0x1F, 0xf0, 0xf0, 0x1F,  // V
    0xfc, 0x84, 0x84, 0x84,  // c
    0x9c, 0x94, 0x94, 0xf4,  // s
    0x04, 0xff, 0x84, 0x80,  // t
    0x00, 0x00, 0x00, 0x00,  // space
    0xc0, 0xc0, 0x00, 0x00,  // .
    0x24, 0x18, 0x18, 0x24,  // *
    0x10, 0x38, 0x10, 0x00,  // +
    0x08, 0x08, 0x08, 0x08,  // -
    0xc0, 0x30, 0x0c, 0x03,  // /
    0x00, 0xbf, 0x00, 0x00,  // !
    0x08, 0x14, 0x22, 0x41,  // <
    0x14, 0x14, 0x14, 0x14,  // =
    0x41, 0x22, 0x14, 0x08,  // >
    0x7f, 0x3e, 0x1c, 0x08,  // arrow
    0xff, 0xc1, 0xf5, 0xff,  // inverted F
    0x08, 0x08, 0x02, 0x1f,  // ^-1
    0x0c, 0xfc, 0x04, 0xfc,  // PI
    0x10, 0xff, 0x01, 0x01,  // sqrt
    0xda, 0x22, 0x22, 0xda,  // mean
    0x22, 0x72, 0x27, 0x22,  // swap
};

// *****  D I S P L A Y

// DEFINES
#define CONTRAST 0x00     // Initial contrast/brightness
#define DADDRESS 0x3C     // I2C slave address
#define DPAGES 4          // Lines of screen
#define DCOMMAND 0x00     // Command byte
#define DDATA 0x40        // Data byte
#define SCREENWIDTH 128   // Screen width in pixel
#define MAXSTRBUF 10      // Maximal length of string buffer sbuf[]
#define CHARW 3           // Character width (3 x FONTWIDTH)
#define CHARH 4           // Character height (4 lines)
#define DIGITS 8          // Number of digits when printing a number
#define ALMOSTZERO 1e-37  // Limits to decide if sci or fix
#define FIXMIN 1e-3       // Limits for fix display guarantee maximal
#define FIXMAX 1e7        // number of significant digits
#define PRINTNUMBER 9     // Digits to print number (small space for '.') - printsbuf
#define PRINTMENU 8       // Digits to print menu (pairs of two) - printsbuf

// VARIABLES
// static byte renderram = 0xB0, drawram = 0x40;  // Masks to address GDDRAM of display
static byte sbuf[MAXSTRBUF];        // Holds string to print
static boolean isnewnumber = true;  // True if stack has to be lifted before entering a new number
static byte decimals = 0;           // Number of decimals entered (input after decimal dot)
static boolean isdot = false;       // True if dot was pressed and decimals will be entered

// MACROS
#define _abs(x) ((x < 0) ? (-x) : (x))  // abs()-substitute macro
#define _ones(x) ((x) % 10)             // Calculates ones unit
#define _tens(x) (((x) / 10) % 10)      // Calculates tens unit
#define _huns(x) (((x) / 100) % 10)     // Calculates hundreds unit
#define _tsds(x) (((x) / 1000) % 10)    // Calculates thousands unit

// SUBPROGRAMS
// static void dbegin(void) {  // Initialize communication
//     TinyWireM.begin();
// }
// static void dsendstart(void) {  // Start communication
//     TinyWireM.beginTransmission(DADDRESS);
// }
// static bool dsendbyte(byte b) {  // Send byte
//     return (TinyWireM.write(b));
// }
// static void dsendstop(void) {  // Stop communication
//     TinyWireM.endTransmission();
// }
// static void dsenddatastart(void) {  // Start data transfer
//     dsendstart();
//     dsendbyte(DDATA);
// }
// static void dsenddatabyte(byte b) {  // Send data byte
//     if (!dsendbyte(b)) {
//         dsendstop();
//         dsenddatastart();
//         dsendbyte(b);
//     }
// }
// static void dsendcmdstart(void) {  // Start command transfer
//     dsendstart();
//     dsendbyte(DCOMMAND);
// }
// static void dsendcmd(byte cmd) {  // Send command
//     dsendcmdstart();
//     dsendbyte(cmd);
//     dsendstop();
// }

// static const byte initscreen[] PROGMEM = {
//     // Initialization sequence
//     0xC8,        // Set scan direction (C0 scan from COM0 to COM[N-1] or C8 mirroring)
//     0xA1,        // Set segment remap (A0 regular or A1 flip)
//     0xA8, 0x1F,  // Set mux ratio (N+1) where: 14<N<64 ... 3F or 1F
//     0xDA, 0x02,  // COM config pin mapping:            right/left left/right
//     // //                         sequential     02        22
//     // //                         alternate      12        32
//     0x20, 0x00,      // Horizontal addressing mode (line feed after EOL)
//     0x8D, 0x14,      // Charge pump (0x14 enable or 0x10 disable)
//     0x81, CONTRAST,  // Set contrast (0...minimal)
//     0xAF             // Display on
// };
// static void dinit(void) {  // Run initialization sequence
//     dbegin();
//     dsendstart();
//     dsendbyte(DCOMMAND);
//     for (byte i = 0; i < sizeof(initscreen); i++) dsendbyte(pgm_read_byte(&initscreen[i]));
//     dsendstop();
// }

// void dcontrast(byte c) {  // Set contrast
//     dsendcmdstart();
//     dsendbyte(0x81);
//     dsendbyte(c);
//     dsendstop();
// }
/*static void don(void) { // Display on
  dsendcmd(0xAF);
  }
  static void doff(void) { // Display off
  dsendcmd(0xAE);
  }*/

// static void drender(void) {  // Render current half of GDDRAM to oled display
//     renderram ^= 0x04;
// }
// static void ddisplay(void) {  // Swap GDDRAM to other half and render
//     drawram ^= 0x20;
//     dsendcmd(drawram);
//     drender();
// }

// static void dsetcursor(byte x, byte y) {  // Set cursor to position (x|y)
//     dsendcmdstart();
//     dsendbyte(renderram | (y & 0x07));
//     dsendbyte(0x10 | (x >> 4));
//     dsendbyte(x & 0x0f);
//     dsendstop();
// }
// static void dfill(byte b) {  // Fill screen with byte/pattern b
//     dsetcursor(0, 0);
//     dsenddatastart();
//     for (int i = 0; i < SCREENWIDTH * DPAGES; i++) dsenddatabyte(b);
//     dsendstop();
// }

// static byte expand2bit(byte b) {                 // Expand 2 bits 000000ab
//     b = (b | (b << 3)) & 0x11;                   // 000a000b
//     for (byte i = 0; i < 3; i++) b |= (b << 1);  // aaaabbbb
//     return (b);
// }

static double pow10(int8_t e) {  // Calculates 10 raised to the power of e
    double f = 1.0F;
    if (e > 0)
        while (e--) f *= 10.0F;
    else
        while (e++) f /= 10.0F;
    return (f);
}

// static void printcat(byte c, byte x) {                            // Print char c at position x
//     for (byte y = 0; y < CHARH; y++) {                            // Lines (4)
//         dsetcursor(x, y);                                         // Set cursor
//         for (byte j = 0; j < FONTWIDTH; j++) {                    // Fontbyte - shifted one pixel down
//             byte bits = pgm_read_byte(&font[FONTWIDTH * c + j]);  // Fontbyte
//             bits = expand2bit((bits >> (y * 2)) & 0x03);          // Expand 000000ab
//             dsenddatastart();
//             for (byte i = 0; i < CHARW; i++) dsenddatabyte(bits);
//             dsendstop();
//         }
//     }
// }

static void printsbuf(byte digits) {  // Print "digits" elements of sbuf[] (shrink ".")
    int8_t dotx = 0;
    for (byte i = 0; i < digits; i++) {
        printcat(sbuf[i], /*(FONTWIDTH + 1) * CHARW * i + dotx*/ i);
        if (digits == PRINTMENU) {
            if (i != 0 && i % 2) dotx += 3;  // Menu - group in pairs
        } else if (sbuf[i] == __DOT)
            dotx = -6;  // Number - small space for dot
    }
}

static void sbufclr(void) {  // Clear sbuf
    for (byte i = 0; i < sizeof(sbuf); i++) sbuf[i] = ___;
}

static void printnum(double f) {  // Print number
    int8_t ee = 0;                // Fixed format
    int8_t e = 1;                 // Exponent
    long m;                       // Mantissa
    sbufclr();
    if (f < 0.0F) {  // # Manage sign
        f = -f;
        sbuf[0] = __SUB;
    }
    if (f >= ALMOSTZERO && (f < FIXMIN || f >= FIXMAX)) {  // # SCI format
        ee = log10(f);                                     // Exponent
        if (ee < 0) ee--;
        f /= pow10(ee);
    }
    if (f >= 1.0F) e = log10(f) + 1;  // Calculate new exponent if (f !< 1)
    double a = pow10(7 - e);          // # Calculate pre dot
    double d = (f * a + 0.5) / a;     // Rounding
    m = d;
    for (byte i = e; i > 0; i--) {
        sbuf[i] = _ones(m);
        m /= 10;
    }
    sbuf[e + 1] = __DOT;
    if ((long)f >= (long)d) d = f;  // # Calculate after dot (and suppress trailing zeros)
    m = (d - (long)d) * a + 0.5;
    boolean istrail = true;
    for (byte i = DIGITS; i > e + 1; i--) {
        byte one = _ones(m);
        if (!istrail || ((isnewnumber || i - e - 1 <= decimals) && (!isnewnumber || one != 0))) {
            sbuf[i] = one;    // Assign digit
            istrail = false;  // End of trailing zeros
        }
        m /= 10L;
    }
    if (ee) {  // # Scientific exponent if applicable
        sbuf[6] = (ee < 0) ? __SUB : ___;
        if (ee < 0) ee = -ee;
        sbuf[8] = _ones(ee);
        sbuf[7] = _tens(ee);
    }
    printsbuf(PRINTNUMBER);
}

// ***** K E Y B O A R D

// Defines
#define KCLOCK 3  // SCL - Keyboard clock pin
#define KDATA 1   // SDO - Keyboard data pin

#define PREENDKEY 254  // Only evaluate keys smaller
#define ENDKEY 255     // Evaluate keys smaller

// Variables
static byte key = PREENDKEY, oldkey = PREENDKEY;  // Holds entered and old key (prevent keyrepeat)

// static byte getkey(void) {  // Read keypad (TTP229-BSF)
//     byte k;
//     for (byte i = 0; i < 16; i++) {
//         digitalWrite(KCLOCK, LOW);
//         k = digitalRead(KDATA);
//         digitalWrite(KCLOCK, HIGH);
//         if (!k) return (i);
//     }
//     return (ENDKEY);
// }

static byte key2nr(byte k) {  // Convert key to number on keypad
    if (k >= 1 && k <= 3)
        return (k + 6);
    else if (k >= 5 && k <= 7)
        return (k - 1);
    else if (k >= 9 && k <= 11)
        return (k - 8);
    else
        return (0);
}

// ***** A P P L I C A T I O N

#define RAD (180.0F / PI)  // 180/PI ... used for _atan and _cos
#define MAXCMDI 48         // Number of commands of intrinsic functions
#define MAXCMDB 64         // 120 End of builtin commands
#define MAXCMDU 80         // 160 End of user commands
#define ISF 1              // F-key demanded
#define MAXPRG 16          // Maximal number of user programs
#define MAXPRGBUF 36       // Maximal size of prgbuf
#define MEDIMENU 2         // Number of menu entry lines
#define MEDIDICT 5         // Number of dict entry lines

// EEPROM dimensions and addresses
#define MEMSTO 10     // Number of memories
#define EESTO 0       // Memories (MEMSTOx4 bytes)
#define MENUITEMS 32  // Number of selectable user menu items
#define EEMENU 40     // User menu (MENUITEMS bytes)
#define EEUSTART 72   // User programs
#define EEUEND EEPROM.length()
#define EEU (EEUEND - EEUSTART)  // Available user memory
#define UL0 20                   // Length of first user program
#define UN 16                    // Number of user programs

// VARIABLES
static boolean isprintscreen = true;  // True, if screen should be printed
// static unsigned int mp;               // MEMPOINTER (builtin and user functions)
static byte setfgm = 0;  // F-key variables
static byte select;      // Selected options
static byte medi = 0;    // MEnu and DIctionary

static boolean issetusr = false;   // True if dict is used for setting usr menu
static boolean isprgdict = false;  // To select dict entry for program
static byte setusrselect;          // Stores selected cmd value to store
static boolean isprgedit = false;  // True, if program is edited
static byte prgnr;                 // Number of program edited
static int prgaddr;                // Address of recent program in EEPROM (includes EEUSTART)
static byte prgpos;                // Position in edited program
static byte prglength;             // Size of program in program buffer
static byte prgbuf[MAXPRGBUF];     // Program buffer

#define DELTAX 1E-4              // Delta for solver
static byte runs;                // Solver cycle runs
static boolean issolve = false;  // True if solving is demanded

#define DATASTACKSIZE 24  // DATA STACK
double ds[DATASTACKSIZE];
static byte dp = 0;

#define ADDRSTACKSIZE 24  // ADDRESS STACK
static int as[ADDRSTACKSIZE];
static byte ap = 0;

byte cl = 0;  // CONDITIONAL LEVEL

// Command code defines
#define _7 1  // Intrinsic commands
#define _8 2
#define _9 3
#define _4 5
#define _5 6
#define _6 7
#define _NEG 8
#define _1 9
#define _2 10
#define _3 11
#define _DROP 12
#define _0 13
#define _DOT 14
#define _DUP 15
#define _DIV 19
#define _SWAP 20
#define _MULT 23
#define _RCL 25
#define _STO 26
#define _ROT 24
#define _SUB 27
#define _PI 29
#define _ADD 31
#define _IF 32
#define _ELSE 33
#define _THEN 34
#define _LT 36
#define _NE 38
#define _INV 43
#define _BEGIN 40
#define _UNTIL 41
#define _COS 44
#define _ATAN 45
#define _EXP 46
#define _LN 47
#define _SIN MAXCMDI + 0  // Builtin commands (mem)
#define _TAN MAXCMDI + 1
#define _ASIN MAXCMDI + 2
#define _ACOS MAXCMDI + 3
#define _OVER MAXCMDI + 4
#define _SQRT MAXCMDI + 5
#define _POW MAXCMDI + 6
#define _PV MAXCMDI + 7
#define _SUMADD MAXCMDI + 8
#define _SUMCLR MAXCMDI + 9
#define _SUMLR MAXCMDI + 10
#define _SUMSTAT MAXCMDI + 11
#define _GAMMALN MAXCMDI + 12
#define _POL2RECT MAXCMDI + 13
#define _RECT2POL MAXCMDI + 14
#define _ND MAXCMDI + 15
#define _PERMCOMB MAXCMDI + 16
#define _END 255  // Function delimiter

// Builtin functions (mem)
const byte mem[] PROGMEM = {
    _END,  // Necessary to prevent function starting with mp = 0
    _9,
    _0,
    _SWAP,
    _SUB,
    _COS,
    _END,  // 0 SIN =cos(90-x)
    _DUP,
    _SIN,
    _SWAP,
    _COS,
    _DIV,
    _END,  // 1 TAN =sin/cos
    _DUP,
    _MULT,
    _INV,
    _1,
    _SUB,
    _SQRT,
    _INV,
    _ATAN,
    _END,  // 2 ASIN =atan(1/(sqrt(1/x/x-1))
    _DUP,
    _MULT,
    _INV,
    _1,
    _SUB,
    _SQRT,
    _ATAN,
    _END,  // 3 ACOS =atan(sqrt(1/x/x-1))
    _SWAP,
    _DUP,
    _ROT,
    _ROT,
    _END,  // 4 OVER
    _DUP,
    _0,
    _NE,
    _IF,
    _LN,
    _2,
    _DIV,
    _EXP,
    _THEN,
    _END,  // 5 SQRT =exp(ln(x)/2)
    _SWAP,
    _LN,
    _MULT,
    _EXP,
    _END,  // 6 POW a^b=exp(b*ln(a))
    _OVER,
    _1,
    _ADD,
    _SWAP,
    _POW,
    _DUP,
    _1,
    _SUB,
    _SWAP,
    _DIV,
    _SWAP,
    _DIV,
    _END,  // 7 PV PV(i,n)=((1+i)^n-1)/(1+i)^n/i

    _7,
    _RCL,
    _1,
    _ADD,
    _7,
    _STO,  // 8 SUMADD - n
    _DUP,
    _8,
    _RCL,
    _ADD,
    _8,
    _STO,  // X
    _DUP,
    _DUP,
    _MULT,
    _5,
    _RCL,
    _ADD,
    _5,
    _STO,  // XX
    _OVER,
    _MULT,
    _6,
    _RCL,
    _ADD,
    _6,
    _STO,  // XY
    _9,
    _RCL,
    _ADD,
    _9,
    _STO,
    _7,
    _RCL,
    _END,  // Y push(n)

    _0,
    _DUP,
    _DUP,
    _DUP,
    _DUP,
    _DUP,  // 9 CLRSUMCLR
    _5,
    _STO,
    _6,
    _STO,
    _7,
    _STO,
    _8,
    _STO,
    _9,
    _STO,
    _END,

    _6,
    _RCL,
    _7,
    _RCL,
    _MULT,
    _8,
    _RCL,
    _9,
    _RCL,
    _MULT,
    _SUB,  // 10 SUMLR - a
    _5,
    _RCL,
    _7,
    _RCL,
    _MULT,
    _8,
    _RCL,
    _DUP,
    _MULT,
    _SUB,
    _DIV,
    _DUP,
    _8,
    _RCL,
    _MULT,
    _NEG,
    _9,
    _RCL,
    _ADD,
    _7,
    _RCL,
    _DIV,
    _SWAP,
    _END,  // b

    _8,
    _RCL,
    _7,
    _RCL,
    _DIV,  // 11 STAT - mean (X/n)
    _DUP,
    _DUP,
    _MULT,
    _7,
    _RCL,
    _MULT,
    _NEG,
    _5,
    _RCL,
    _ADD,  // stddev (XX-n*m^2)/(n-1)
    _7,
    _RCL,
    _1,
    _SUB,
    _DIV,
    _SQRT,
    _SWAP,
    _END,

    _1,
    _ADD,
    _DUP,
    _DUP,
    _DUP,
    _DUP,
    _1,
    _2,
    _MULT,  // 12 GAMMALN: ln!=(ln(2*PI)-ln(x))/2+x*(ln(x+1/(12*x-1/10/x))-1)
    _SWAP,
    _1,
    _0,
    _MULT,
    _INV,
    _SUB,
    _INV,
    _ADD,
    _LN,
    _1,
    _SUB,
    _MULT,
    _SWAP,
    _LN,
    _NEG,
    _2,
    _PI,
    _MULT,
    _LN,
    _ADD,
    _2,
    _DIV,
    _ADD,
    _END,

    _DUP,
    _ROT,
    _DUP,
    _COS,
    _SWAP,
    _SIN,
    _ROT,
    _MULT,
    _ROT,
    _ROT,
    _MULT,
    _END,  // 13 P>R y=r*sin(a) x=r*cos(a)

    _DUP,
    _MULT,
    _SWAP,
    _DUP,
    _MULT,
    _DUP,
    _ROT,
    _DUP,
    _ROT,
    _ADD,
    _SQRT,  // 14 R>P r=sqrt(x*x+y*y) a=atan(y/x)
    _ROT,
    _ROT,
    _DIV,
    _SQRT,
    _ATAN,
    _SWAP,
    _END,

    _DUP,
    _DUP,
    _DUP,
    _DUP,
    _MULT,
    _MULT,
    _DOT,
    _0,
    _7,
    _MULT,  // 15 ND
    _SWAP,
    _1,
    _DOT,
    _6,
    _MULT,
    _NEG,
    _ADD,
    _EXP,
    _1,
    _ADD,
    _INV,
    _SWAP,  // CDF ~ 1/(1+exp(-0.07*x^3-1.6*x))
    _DUP,
    _MULT,
    _NEG,
    _2,
    _DIV,
    _EXP,
    _2,
    _PI,
    _MULT,
    _SQRT,
    _INV,
    _MULT,
    _END,  // PDF = exp(-x*x/2)/sqrt(2*PI)

    _DUP,
    _ROT,
    _SWAP,  // 16 PERM COMB
    _OVER,
    _ROT,
    _ROT,
    _SUB,
    _1,
    _ROT,
    _ROT,
    _SWAP,  // PERM
    _BEGIN,
    _SWAP,
    _ROT,
    _1,
    _ROT,
    _ADD,
    _DUP,
    _ROT,
    _MULT,
    _SWAP,
    _ROT,
    _OVER,
    _OVER,
    _SWAP,
    _LT,
    _UNTIL,
    _DROP,
    _DROP,
    _DUP,
    _ROT,
    _1,
    _SWAP,  // COMB
    _BEGIN,
    _ROT,
    _ROT,
    _DUP,
    _ROT,
    _SWAP,
    _DIV,
    _ROT,
    _ROT,
    _1,
    _ADD,
    _SWAP,
    _OVER,
    _1,
    _SUB,
    _OVER,
    _SWAP,
    _LT,
    _UNTIL,
    _DROP,
    _DROP,
    _END,

};

// Command names
static const byte cmd[] PROGMEM = {
    __FINV,
    ___,
    ___,
    __7,
    ___,
    __8,
    ___,
    __9,  // 0 Primary keys
    __E,
    __E,
    ___,
    __4,
    ___,
    __5,
    ___,
    __6,
    __N,
    ___,
    ___,
    __1,
    ___,
    __2,
    ___,
    __3,
    __C,
    ___,
    ___,
    __0,
    ___,
    __DOT,
    ___,
    __D,
    __M,
    ___,
    __S,
    __ADD,
    __P,
    __R,
    ___,
    __DIV,  // 16 F-keys

    __SWAP,
    ___,
    __D,
    __C,
    __U,
    __S,
    ___,
    __MULT,
    __R,
    __T,
    __R,
    __C,
    __S,
    __T,
    ___,
    __SUB,
    __C,
    __A,
    __PI,
    ___,
    __I,
    __N,
    ___,
    __ADD,
    __I,
    __F,
    __E,
    __L,
    __T,
    __H,
    __P,
    __C,  // 32 Intrinsic functions
    __LT,
    ___,
    __EQ,
    ___,
    __LT,
    __GT,
    ___,
    __GT,

    __B,
    __E,
    __U,
    __N,
    __S,
    __O,
    __I,
    ___,
    __c,
    ___,
    __t,
    __POW1,
    __E,
    ___,
    __L,
    __N,
    __s,
    ___,
    __t,
    ___,
    __s,
    __POW1,
    __c,
    __POW1,  // 48 Builtin functions
    __O,
    __V,
    __SQRT,
    ___,
    __P,
    ___,
    __EM,
    __L,
    __P,
    __V,
    __N,
    __D,
    __P,
    __ARROW,
    __R,
    __ARROW,

    __S,
    __ADD,
    __S,
    __c,
    __MEAN,
    ___,
    __L,
    __R,
    __0,
    __0,
    __0,
    __1,
    __0,
    __2,
    __0,
    __3,  // 64 User functions
    __0,
    __4,
    __0,
    __5,
    __0,
    __6,
    __0,
    __7,
    __0,
    __8,
    __0,
    __9,
    __1,
    __0,
    __1,
    __1,
    __1,
    __2,
    __1,
    __3,
    __1,
    __4,
    __1,
    __5,
};

// FUNCTION POINTER ARRAY
static void (*dispatch[])(void) = {
    // Function pointer array
    &_keyf,
    &_num,
    &_num,
    &_num,  // 00 Primary keys
    &_e,
    &_num,
    &_num,
    &_num,
    &_neg,
    &_num,
    &_num,
    &_num,
    &_drop,
    &_num,
    &_dot,
    &_dup,
    &_menu,
    &_sumadd,
    &_prgedit,
    &_div,  // 16 F-keys

    &_swap,
    &_dict,
    &_usrset,
    &_mul,
    &_rot,
    &_mrcl,
    &_msto,
    &_sub,
    &_clr,
    &_pi,
    &_int,
    &_add,
    &_condif,
    &_condelse,
    &_condthen,
    &_permcomb,  // 32 Intrinsic functions
    &_condlt,
    &_condeq,
    &_condne,
    &_condgt,

    &_begin,
    &_until,
    &_solve,
    &_inv,
    &_cos,
    &_atan,
    &_exp,
    &_ln,
    &_sin,
    &_tan,
    &_asin,
    &_acos,  // 48 Builtin functions
    &_over,
    &_sqrt,
    &_pow,
    &_gammaln,
    &_pv,
    &_nd,
    &_pol2rect,
    &_rect2pol,

    &_sumadd,
    &_sumclr,
    &_sumstat,
    &_sumlr,
};
// static void _nop(void) {} // NOP - no operation
static void _num(void) {  // Insert number
    _numinput(key2nr(key));
}
static void _add(void) {  // ADD +
    dpush(dpop() + dpop());
}
static void _acos(void) {  // ACOS
    seekmem(_ACOS);
}
static void _asin(void) {  // ASIN
    seekmem(_ASIN);
}
static void _atan(void) {  // ATAN
    dpush(atan(dpop()) * RAD);
}
static void _begin(void) {  // BEGIN
    apush(mp);
}
static void _ce(void) {  // CE
    if (isdot) {
        if (decimals) {
            decimals--;
            double a = pow10(decimals);
            dpush(((long)(dpop() * a) / a));
        } else
            isdot = false;
    } else {
        long a = dpop() / 10.0F;
        if (!a)
            isnewnumber = true;
        else
            dpush(a);
    }
}
static void _clr(void) {  // CLR
    dp = 0;
    _sumclr();
}
static void _condelse(void) {  // CONDITION ELSE
    _condseek();               // Seek next THEN
    cl--;
}
static void _condeq(void) {  // CONDITION =
    dpush(dpop() == dpop());
}
static void _condgt(void) {  // CONDITION >
    dpush(dpop() < dpop());
}
static void _condif(void) {    // CONDITION IF
    cl++;                      // Increment conditional level
    if (!dpop()) _condseek();  // FALSE-Clause - seek next ELSE or THEN
}
static void _condlt(void) {  // CONDITION <
    _condgt();
    dpush(!dpop());
}
static void _condne(void) {  // CONDITION <>
    _condeq();
    dpush(!dpop());
}
static void _condseek(void) {  // CONDITION - seek next ELSE or THEN
    boolean isloop = true;
    byte cltmp = 0;  // Local conditional level
    while (isloop) {
        byte c = 0;
        if (mp < sizeof(mem))
            c = pgm_read_byte(mem + mp++);  // Builtin
        else if (mp < sizeof(mem) + EEU)
            c = EEPROM[mp++ - sizeof(mem) + EEUSTART];
        if (mp >= sizeof(mem) + EEU)
            isloop = false;  // No corresponding ELSE or THEN
        else if (c == _IF)
            cltmp++;  // Nested IF found
        else if (cltmp && c == _THEN)
            cltmp--;  // Nested IF ended
        else if (!cltmp && (c == _ELSE || c == _THEN))
            isloop = false;
    }
}
static void _condthen(void) {  // CONDITION THEN
    cl--;                      // Decrement conditional level
}
static void _cos(void) {  // COS
    dpush(cos(dpop() / RAD));
}
static void _dict(void) {  // DICT
    select = 0;
    medi = MEDIDICT;
}
static void _div(void) {  // DIV /
    _inv();
    _mul();
}
static void _dot(void) {  // DOT .
    if (isnewnumber) {
        dpush(0.0F);  // Start new number with 0
        decimals = 0;
        isnewnumber = false;
    }
    isdot = true;
}
static void _drop(void) {  // DROP
    if (!isnewnumber)
        _ce();  // Clear entry
    else if (dp)
        dp--;  // Clear TOS
}
static void _dup(void) {  // DUP
    if (isnewnumber && dp) dpush(ds[dp - 1]);
}
static void _e(void) {  // E
    dpush(pow10(dpop()));
    _mul();
}
static void _exp(void) {         // EXP
    boolean isneg = false;       // True for negative x
    if (dpush(dpop()) < 0.0F) {  // Taylor series ist weak for negative x ... calculate exp(-x)=1/exp(x)
        _neg();
        isneg = true;
    }
    dpush(1.0F);  // Stack: x res
    for (byte i = 255; i; i--) {
        _swap();
        _dup();
        _rot();  // Duplicate x (TOS-1)
        dpush(i);
        _div();
        _mul();
        dpush(1.0F);
        _add();  // res = 1.0 + x * res / i;
    }
    if (isneg) _inv();  // Negative argument
    _swap();
    _drop();  // Remove x (TOS-1)
}
static void _gammaln(void) {  // GAMMALN
    seekmem(_GAMMALN);
}
static void _int(void) {  // INT
    dpush((long)dpop());
}
static void _inv(void) {  // INV
    dpush(1.0F / dpop());
}
static void _keyf(void) {  // KEY-F
    fgm = ISF;
    setfgm = 0;
}
static void _ln(void) {  // LN
    dpush(log(dpop()));
}
static void _menu(void) {  // MENU
    select = 0;
    medi = MEDIMENU;
}
static void _mrcl(void) {  // MRCL
    _mstorcl(false);
}
static void _msto(void) {  // MSTO
    _mstorcl(true);
}
static void _mstorcl(boolean issto) {  // MRCL
    byte nr = dpop();
    byte addr = EESTO + nr * sizeof(double);  // Has to be <255 (byte)
    if (nr < MEMSTO) {
        if (issto)
            EEwrite(addr, dpop());
        else {
            double a;
            EEread(addr, a);
            dpush(a);
        }
    }
}
static void _mul(void) {  // MULT *
    dpush(dpop() * dpop());
}
/*static void _nand(void) { // NAND
  long b = dpop();
  dpush(~((long)dpop() & b));
  }*/
static void _nd(void) {  // ND
    seekmem(_ND);
}
static void _neg(void) {  // NEGATE
    dpush(-dpop());
}
static void _numinput(byte k) {  // NUM Numeric input (0...9)
    if (isdot) {                 // Append decimal
        dpush(k);
        dpush(pow10(++decimals));
        _div();
        _add();
    } else if (isnewnumber)
        dpush((double)k);  // Push new numeral
    else {                 // Append numeral
        dpush(10.0F);
        _mul();
        dpush(k);
        _add();
    }
    isnewnumber = false;
}
static void _over(void) {  // OVER
    seekmem(_OVER);
}
void _permcomb(void) {  // PERM COMB
    seekmem(_PERMCOMB);
}
static void _pi(void) {  // PI
    dpush(PI);
}
void _pol2rect(void) {  // POL2RECT
    seekmem(_POL2RECT);
}
void _pow(void) {  // POWER
    seekmem(_POW);
}
void _pv(void) {  // PRESENT VALUE
    seekmem(_PV);
}
static void _prgedit(void) {  // PRGEDIT
    isprgedit = true;
    if ((prgnr = dpop()) >= MAXPRG) prgnr = 0;
    prgpos = prglength = 0;
    prgaddr = EEUSTART + eeudist(prgnr);
    byte len = prgnr + UL0, prgstep = EEPROM[prgaddr];  // Max program length and first step
    while (prglength < len && prgstep != _END) {
        prgbuf[prglength] = prgstep;              // Load step from EEPROM to prgbuf
        prgstep = EEPROM[prgaddr + ++prglength];  // Next step
    }
}
void _rect2pol(void) {  // RECT2POL
    seekmem(_RECT2POL);
}
static void _rot(void) {  // ROT
    if (dp > 2) {
        double a = dpop(), b = dpop(), c = dpop();
        dpush(b);
        dpush(a);
        dpush(c);
    }
}
static void _sin(void) {  // SIN
    seekmem(_SIN);
}
static void _solve(void) {  // SOLVE
    _dup();
    _dup();  // 3 x0 on stack
    runs = 0;
    issolve = true;
}
static void _sqrt(void) {  // SQRT
    seekmem(_SQRT);
}
static void _sub(void) {  // SUB -
    _neg();
    _add();
}
static void _sumadd(void) {  // SUM+
    seekmem(_SUMADD);
}
static void _sumclr(void) {  // SUMclr
    seekmem(_SUMCLR);
}
static void _sumlr(void) {  // SUM LR
    seekmem(_SUMLR);
}
static void _sumstat(void) {  // SUM STAT
    seekmem(_SUMSTAT);
}
static void _swap(void) {  // SWAP
    if (dp > 1) {
        double a = dpop(), b = dpop();
        dpush(a);
        dpush(b);
    }
}
static void _tan(void) {  // TAN
    seekmem(_TAN);
}
static void _until(void) {  // UNTIL
    if (!ap)
        ;  // No BEGIN for this UNTIL
    else if (dpop())
        apop();  // Go on (delete return address)
    else
        apush(mp = apop());  // Go back to BEGIN
}
static void _usrset(void) {  // USR
    select = 0;
    medi = MEDIDICT;
    issetusr = true;
}

// SUBPROGRAMS

static void delayshort(byte ms) {  // Delay in ms (8MHz)
    for (long k = 0; k < ms * 8000L; k++) __asm__ __volatile__("nop\n\t");
}

static int eeudist(byte i) {  // Distance of user program number i from EEUSTART
    return ((2 * UL0 + i - 1) * i / 2);
}

static void execute(byte cmd) {  // Execute command
    if (cmd < MAXCMDB)
        (*dispatch[cmd])();  // Dispatch intrinsic/builtin command
    else if (cmd < MAXCMDU)
        mp = eeudist(cmd - MAXCMDB) + sizeof(mem);  // Execute user command
    if ((cmd % 4 == 0 && cmd != 12) || cmd > 14) {  // New number - except: 0-9. DROP
        decimals = 0;
        isdot = false;
        isnewnumber = true;
    }
    if (fgm && setfgm) fgm = setfgm = 0;  // Hold demanded F-key status one cycle
    setfgm = 1;
}

static void prgstepins(byte c) {  // Insert step (character c in prgbuf at prgeditstart)
    for (byte i = prglength; i > prgpos; i--) prgbuf[i] = prgbuf[i - 1];
    prgbuf[prgpos + 1] = c;
    prglength++;
    prgpos++;
}

template <class T>
void EEwrite(int ee, const T& value) {  // Write any datatype to EEPROM
    const byte* p = (const byte*)(const void*)&value;
    for (byte i = 0; i < sizeof(value); i++) EEPROM.write(ee++, *p++);
}
template <class T>
void EEread(int ee, T& value) {  // Read any datatype from EEPROM
    byte* p = (byte*)(void*)&value;
    for (byte i = 0; i < sizeof(value); i++) *p++ = EEPROM.read(ee++);
}

static void upn(byte n, byte l) {  // Selection up l lines
    if (select > n * l && select <= (n + 1) * l - 1)
        select--;
    else
        select = (n + 1) * l - 1;
}
static void downn(byte n, byte l) {  // Selection down l lines
    if (select >= n * l && select < (n + 1) * l - 1)
        select++;
    else
        select = n * l;
}
static byte menuselect(byte lines) {  // Selection (1 line = 16 items)
    char k = key;
    if (k <= 3)
        return (select * 4 + k);  // Execute
    else if (k >= 4 && k <= 7)
        upn(k - 4, lines);  // # Up
    else if (k >= 8 && k <= 11)
        downn(k - 8, lines);       // # Down0
    if (k == 12) return (ENDKEY);  // Escape
    /*if (k == 13) { // Hidden feature: Display off
      doff();
      return (ENDKEY);
      }*/
    if (k == 15) {  // Hidden feature: Set contrast
        // dcontrast(dpop());
        return (ENDKEY);
    }
    return (PREENDKEY);
}

// *** S T A C K

static void floatstack() {
    memmove(ds, &ds[1], (DATASTACKSIZE - 1) * sizeof(double));
    dp--;
}

static double dpush(double d) {             // Push complex number to data-stack
    if (dp >= DATASTACKSIZE) floatstack();  // Float stack
    return (ds[dp++] = d);
}
static double dpop(void) {  // Pop real number from data-stack
    return (dp ? ds[--dp] : 0.0F);
}

static void apush(int addr) {  // Push address (int) to address-stack
    as[ap++] = addr;
}
static int apop(void) {  // Pop address (int) from address-stack
    return (ap ? as[--ap] : NULL);
}

static void seekmem(byte n) {  // Find run-address (mp) of n-th builtin function
    mp = 0;
    while (n + 1 - MAXCMDI)
        if (pgm_read_byte(&mem[mp++]) == _END) n--;
}

// ***** P R I N T I N G

static boolean printscreen(void) {  // Print screen due to state
    // dfill(0x00);

    if (medi) {  // MEnu or DIct
        for (byte i = 0; i < 4; i++) {
            byte nr = select * 4 + i;
            if (medi == MEDIMENU) nr = EEPROM[EEMENU + nr];
            for (byte j = 0; j < 2; j++) sbuf[2 * i + j] = pgm_read_byte(&cmd[2 * nr + j]);
        }
        printsbuf(PRINTMENU);
    }

    else if (isprgedit) {  // # Edit program
        sbuf[0] = ___;
        sbuf[1] = __P;
        sbuf[3] = _ones(prgnr);
        sbuf[2] = _tens(prgnr);  // Program number
        sbuf[5] = _ones(prgpos);
        sbuf[4] = _tens(prgpos);                                                                 // Program pos
        for (byte i = 0; i < 2; i++) sbuf[6 + i] = pgm_read_byte(&cmd[2 * prgbuf[prgpos] + i]);  // Command
        printsbuf(PRINTMENU);
    }

    else if (dp)
        printnum(ds[dp - 1]);  // Stack

    else
        printcat(__ARROW, 0);  // Empty stack prompt

    if (!isnewnumber) printcat(__ARROW, 0);  // Num input indicator

    if (fgm) printcat(__FINV, 0);  // F indicator

    // ddisplay();
    return (false);  // To determine isprintscreen
}

// *****  S E T U P  A N D  L O O P

// void setup() {
//     dinit();    // Init display
//     drender();  // Render current half of GDDRAM
//     pinMode(KCLOCK, OUTPUT);
//     pinMode(KDATA, INPUT);  // Init keyboard pins
//     // power_timer0_disable(); // Power off timer0 (saves 0.1 mA)
//     power_timer1_disable();  // Power off timer1 (saves 0.6 mA)
//     ADCSRA &= ~bit(ADEN);    // Power off AD converter (saves 0.4 mA)
// }

void loop() {
    if (isprintscreen) isprintscreen = printscreen();  // Print screen

    if (mp) {  // *** Execute/run code
        if (mp < sizeof(mem))
            key = pgm_read_byte(&mem[mp++]);  // Builtin
        else if (mp < sizeof(mem) + EEU)
            key = EEPROM[mp++ - sizeof(mem) + EEUSTART];  // User
        else
            mp = 0;                                    // cmd > MAXCMDU
        if (key >= MAXCMDI && key != _END) apush(mp);  // Subroutine detected - branch
        if (key == _END) {                             // _END reached
            if (ap)
                mp = apop();  // End of subroutine - return
            else {            // End of run
                mp = 0;
                if (!issolve) isprintscreen = true;  // Print screen if not in solver
            }
        } else
            execute(key);
    }

    else {               // *** No run - get key
        key = getkey();  // READ KEY
        delayshort(1);   // Give keypad some time to settle

        if (issolve) {  // # SOLVE
            if (++runs < 3) {
                if (runs == 2) {  // Second run - f(x0+dx)
                    _swap();
                    dpush(DELTAX);
                    _add();  // x0+DELTAX ... Prepare new x-value
                }
                execute(MAXCMDB);  // Execute first user program
            } else {               // Third run - x1
                _swap();
                _div();
                dpush(-1.0);
                _add();  // f1/f0-1
                dpush(DELTAX);
                _swap();
                _div();                        // diffx=DELTAX/(f1/f0-1)
                double diffx = dpush(dpop());  // Rescue diffx for exit condition
                _sub();                        // x1=x0-diffx ... improved x-value
                runs = 0;
                if (diffx < DELTAX && diffx > -DELTAX) {  // Exit
                    isnewnumber = isprintscreen = true;
                    issolve = false;
                } else {  // 3 x1 on stack
                    _dup();
                    _dup();
                }
            }
        }

        if (key != oldkey) {
            oldkey = key;  // New valid key is old key
            if (key < ENDKEY) {
                // don(); // Wake up display (in case of doff)

                if (medi) {  // ### Menu or Dict
                    byte sel = menuselect(medi);
                    if (sel < PREENDKEY) {       // Item selected
                        if (medi == MEDIMENU) {  // # MENU
                            if (issetusr) {      // Save user menu entry
                                EEPROM[EEMENU + sel] = setusrselect;
                                issetusr = false;
                            } else
                                execute(EEPROM[EEMENU + sel]);  // Execute selected command
                            medi = 0;
                        } else {             // # DICT
                            if (issetusr) {  // Go on to menu
                                setusrselect = sel;
                                select = 0;
                                medi = MEDIMENU;
                            } else if (isprgdict) {  // Go back to prgedit
                                prgstepins(sel);
                                isprgdict = false;
                                isprgedit = true;
                                medi = 0;
                            } else {
                                execute(sel);  // Execute command directly
                                medi = 0;
                            }
                        }
                    } else if (sel == ENDKEY) {
                        issetusr = false;  // Escape
                        medi = 0;
                    }
                }

                else if (isprgedit) {  // ### Program edit
                    if (key == 0)
                        fgm = fgm ? 0 : ISF;            // # F-key toggle
                    else if (key == 5 && fgm == ISF) {  // # F-4 ... insert step via dict
                        select = fgm = 0;
                        medi = MEDIDICT;
                        isprgdict = true;
                        isprgedit = false;
                    } else if (fgm == ISF) {  // # Insert F-shifted key
                        prgstepins(key + fgm * 16);
                        fgm = 0;
                    } else if (key == 4) {                     // # Up
                        if (prgpos) prgpos--;                  /// else if (prglength) prgpos = prglength - 1;
                    } else if (key == 8) {                     // # Down
                        if (prgpos < prglength - 1) prgpos++;  /// else prgpos = 0;
                    } else if (key == 12) {                    // # Escape and save program to EEPROM
                        for (byte i = 0; i < prglength; i++) EEPROM[prgaddr + i] = prgbuf[i];
                        EEPROM[prgaddr + prglength] = _END;
                        isprgedit = false;
                    } else if (key == 14) {  // # Dot - Delete step
                        if (prglength) {
                            for (byte i = prgpos; i < prglength; i++) prgbuf[i] = prgbuf[i + 1];
                            prglength--;
                        }
                    } else
                        prgstepins(key);  // # Insert direct key
                }

                else
                    execute(key + fgm * 16);  // ### Execute key

                isprintscreen = true;
            }
        }
    }  // End of evaluating key
}
