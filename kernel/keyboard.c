#include "keyboard.h"
#include "memory.h"
#include "interrupt.h"
#include "printk.h"
#include "apic.h"

unsigned char PauseBreakCode[PB_CODE_SIZE] = {0xe1, 0x1d, 0x45, 0xe1, 0x9d, 0xc5};
unsigned char KeyCodeMapNormal[NR_SCAN_CODES * MAP_COLS] = {
    /*scan-code	unShift		Shift		*/
    /*--------------------------------------------------------------*/
    /*0x00*/ 0,
    0,
    /*0x01*/ 0,
    0, // ESC
    /*0x02*/ '1',
    '!',
    /*0x03*/ '2',
    '@',
    /*0x04*/ '3',
    '#',
    /*0x05*/ '4',
    '$',
    /*0x06*/ '5',
    '%',
    /*0x07*/ '6',
    '^',
    /*0x08*/ '7',
    '&',
    /*0x09*/ '8',
    '*',
    /*0x0a*/ '9',
    '(',
    /*0x0b*/ '0',
    ')',
    /*0x0c*/ '-',
    '_',
    /*0x0d*/ '=',
    '+',
    /*0x0e*/ 0,
    0, // BACKSPACE
    /*0x0f*/ 0,
    0, // TAB

    /*0x10*/ 'q',
    'Q',
    /*0x11*/ 'w',
    'W',
    /*0x12*/ 'e',
    'E',
    /*0x13*/ 'r',
    'R',
    /*0x14*/ 't',
    'T',
    /*0x15*/ 'y',
    'Y',
    /*0x16*/ 'u',
    'U',
    /*0x17*/ 'i',
    'I',
    /*0x18*/ 'o',
    'O',
    /*0x19*/ 'p',
    'P',
    /*0x1a*/ '[',
    '{',
    /*0x1b*/ ']',
    '}',
    /*0x1c*/ 0,
    0, // ENTER
    /*0x1d*/ 0x1d,
    0x1d, // CTRL Left
    /*0x1e*/ 'a',
    'A',
    /*0x1f*/ 's',
    'S',

    /*0x20*/ 'd',
    'D',
    /*0x21*/ 'f',
    'F',
    /*0x22*/ 'g',
    'G',
    /*0x23*/ 'h',
    'H',
    /*0x24*/ 'j',
    'J',
    /*0x25*/ 'k',
    'K',
    /*0x26*/ 'l',
    'L',
    /*0x27*/ ';',
    ':',
    /*0x28*/ '\'',
    '"',
    /*0x29*/ '`',
    '~',
    /*0x2a*/ 0x2a,
    0x2a, // SHIFT Left
    /*0x2b*/ '\\',
    '|',
    /*0x2c*/ 'z',
    'Z',
    /*0x2d*/ 'x',
    'X',
    /*0x2e*/ 'c',
    'C',
    /*0x2f*/ 'v',
    'V',

    /*0x30*/ 'b',
    'B',
    /*0x31*/ 'n',
    'N',
    /*0x32*/ 'm',
    'M',
    /*0x33*/ ',',
    '<',
    /*0x34*/ '.',
    '>',
    /*0x35*/ '/',
    '?',
    /*0x36*/ 0x36,
    0x36, // SHIFT Right
    /*0x37*/ '*',
    '*',
    /*0x38*/ 0x38,
    0x38, // ALT Left
    /*0x39*/ ' ',
    ' ',
    /*0x3a*/ 0,
    0, // CAPS LOCK
    /*0x3b*/ 0,
    0, // F1
    /*0x3c*/ 0,
    0, // F2
    /*0x3d*/ 0,
    0, // F3
    /*0x3e*/ 0,
    0, // F4
    /*0x3f*/ 0,
    0, // F5

    /*0x40*/ 0,
    0, // F6
    /*0x41*/ 0,
    0, // F7
    /*0x42*/ 0,
    0, // F8
    /*0x43*/ 0,
    0, // F9
    /*0x44*/ 0,
    0, // F10
    /*0x45*/ 0,
    0, // NUM LOCK
    /*0x46*/ 0,
    0, // SCROLL LOCK
    /*0x47*/ '7',
    0, /*PAD HONE*/
    /*0x48*/ '8',
    0, /*PAD UP*/
    /*0x49*/ '9',
    0, /*PAD PAGEUP*/
    /*0x4a*/ '-',
    0, /*PAD MINUS*/
    /*0x4b*/ '4',
    0, /*PAD LEFT*/
    /*0x4c*/ '5',
    0, /*PAD MID*/
    /*0x4d*/ '6',
    0, /*PAD RIGHT*/
    /*0x4e*/ '+',
    0, /*PAD PLUS*/
    /*0x4f*/ '1',
    0, /*PAD END*/

    /*0x50*/ '2',
    0, /*PAD DOWN*/
    /*0x51*/ '3',
    0, /*PAD PAGEDOWN*/
    /*0x52*/ '0',
    0, /*PAD INS*/
    /*0x53*/ '.',
    0, /*PAD DOT*/
    /*0x54*/ 0,
    0,
    /*0x55*/ 0,
    0,
    /*0x56*/ 0,
    0,
    /*0x57*/ 0,
    0, // F11
    /*0x58*/ 0,
    0, // F12
    /*0x59*/ 0,
    0,
    /*0x5a*/ 0,
    0,
    /*0x5b*/ 0,
    0,
    /*0x5c*/ 0,
    0,
    /*0x5d*/ 0,
    0,
    /*0x5e*/ 0,
    0,
    /*0x5f*/ 0,
    0,

    /*0x60*/ 0,
    0,
    /*0x61*/ 0,
    0,
    /*0x62*/ 0,
    0,
    /*0x63*/ 0,
    0,
    /*0x64*/ 0,
    0,
    /*0x65*/ 0,
    0,
    /*0x66*/ 0,
    0,
    /*0x67*/ 0,
    0,
    /*0x68*/ 0,
    0,
    /*0x69*/ 0,
    0,
    /*0x6a*/ 0,
    0,
    /*0x6b*/ 0,
    0,
    /*0x6c*/ 0,
    0,
    /*0x6d*/ 0,
    0,
    /*0x6e*/ 0,
    0,
    /*0x6f*/ 0,
    0,

    /*0x70*/ 0,
    0,
    /*0x71*/ 0,
    0,
    /*0x72*/ 0,
    0,
    /*0x73*/ 0,
    0,
    /*0x74*/ 0,
    0,
    /*0x75*/ 0,
    0,
    /*0x76*/ 0,
    0,
    /*0x77*/ 0,
    0,
    /*0x78*/ 0,
    0,
    /*0x79*/ 0,
    0,
    /*0x7a*/ 0,
    0,
    /*0x7b*/ 0,
    0,
    /*0x7c*/ 0,
    0,
    /*0x7d*/ 0,
    0,
    /*0x7e*/ 0,
    0,
    /*0x7f*/ 0,
    0,
};

static KeyboardInBuf *KBIB;
static HwInterruptT KBController;
static int ShiftL, ShiftR, CtrlL, CtrlR, AltL, AltR, Caps;

unsigned char GetScanCode()
{

    unsigned char x;

    if (KBIB->count == 0)
        while (KBIB->count == 0)
            nop();

    if (KBIB->PTail == (KBIB->buf + KB_BUF_SIZE))
        KBIB->PTail = KBIB->buf;
    x = *(KBIB->PTail++);
    KBIB->count--;

    // ColorPrintfk(RED, BLACK, "IN PUT BUFFER %d\n", KBIB->count);
    return x;
}
void AnalyzeKeyCode()
{
    unsigned char x;
    int i;
    unsigned char key = 0;
    int make = 0;

    x = GetScanCode();
    if (x == 0xe1)
    {
        for (i = 1; i < PB_CODE_SIZE; i++)
            if (GetScanCode() != PauseBreakCode[i])
                break;

        if (i < PB_CODE_SIZE)
            ColorPrintfk(RED, BLACK, "There Is A Unkown KeyCode\n");
        else
        {
            ColorPrintfk(BLUE, BLACK, "PauseBreak Key Has Been Pushed\n");
            key = PAUSE_BREAK;
        }
    }

    if (x == 0xe0)
    {
        x = GetScanCode();
        make = (x & FLAG_BREAK ? 0 : 1);
        switch (x & 0x7f)
        {
        case 0x2a:
        {
            if (GetScanCode() == 0xe0)
                if (GetScanCode() == 0x37)
                {
                    key = PRINT_SCREEN;
                    make = 0;
                }
            break;
        }
        case 0x1D:
        {
            CtrlL = make;
            key = OTHER_KEY;
            break;
        }
        case 0x38:
        {
            AltR = make;
            key = OTHER_KEY;
            break;
        }
        case 0x48:
        {
            CtrlL = 1;
            key = OTHER_KEY;
            break;
        }
        case 0x4b:
        {
            CtrlL = 1;
            key = OTHER_KEY;
            break;
        }
        case 0x50:
        {
            CtrlL = 1;
            key = OTHER_KEY;
            break;
        }
        case 0x4d:
        {
            CtrlL = 1;
            key = OTHER_KEY;
            break;
        }
        case 0x52:
        {
            CtrlL = 1;
            key = OTHER_KEY;
            break;
        }
        case 0x47:
        {
            CtrlL = 1;
            key = OTHER_KEY;
            break;
        }
        case 0x49:
        {
            CtrlL = 1;
            key = OTHER_KEY;
            break;
        }
        case 0x53:
        {
            CtrlL = 1;
            key = OTHER_KEY;
            break;
        }
        case 0x4f:
        {
            CtrlL = 1;
            key = OTHER_KEY;
            break;
        }
        case 0x51:
        {
            CtrlL = 1;
            key = OTHER_KEY;
            break;
        }
        case 0x5b:
        {
            CtrlL = 1;
            key = OTHER_KEY;
            break;
        }
        case 0x5c:
        {
            CtrlL = 1;
            key = OTHER_KEY;
            break;
        }
        case 0x5d:
        {
            CtrlL = 1;
            key = OTHER_KEY;
            break;
        }
        case 0x35:
        {
            CtrlL = 1;
            key = OTHER_KEY;
            break;
        }
        case 0x1c:
        {
            CtrlL = 1;
            key = OTHER_KEY;
            break;
        }

        default:
            break;
        }
    }

    if (key == 0)
    {
        unsigned char *KeyRow = NULL;
        int column = 0;
        make = (x & FLAG_BREAK ? 0 : 1);
        KeyRow = &KeyCodeMapNormal[(x & 0x7f) * MAP_COLS];
        if (ShiftL | ShiftR)
            column = 1;

        key = KeyRow[column];

        switch (x & 0x7f)
        {
        case 0x2a:
        {
            ShiftL = make;
            key = 0;
            break;
        }
        case 0x1d:
        {
            CtrlL = make;
            key = 0;
            break;
        }
        case 0x38:
        {
            AltL = make;
            key = 0;
            break;
        }
        case 0x36:
        {
            ShiftR = make;
            key = 0;
            break;
        }
        case 0x3a:
        {
            Caps ^= 1;
            key = 0;
            break;
        }

        default:
        {
            if (!make)
                key = 0;
            break;
        }
        }

        if (key)
            ColorPrintfk(BLUE, BLACK, "%c", key);
    }
}

void KeyboardHandler(struct PtRegs *regs, unsigned long nr, unsigned long arg)
{
    unsigned char x = IN8b(0x60);
    if (KBIB->PHead == KBIB->buf + KB_BUF_SIZE)
        KBIB->PHead = KBIB->buf;

    *(KBIB->PHead) = x;
    KBIB->count++;
    KBIB->PHead++;
}

void KeyboardExit()
{
    UnregisterIrq(0x21);
    kfree(KBIB);
}

void KeyboardInit()
{
    IoApicRetEntry entry;
    BuildController(&KBController);

    KBIB = (KeyboardInBuf *)kmalloc(sizeof(KeyboardInBuf), 0);
    KBIB->PHead = KBIB->buf;
    KBIB->PTail = KBIB->buf;
    KBIB->count = 0;
    memset(KBIB->buf, 0, KB_BUF_SIZE);

    entry.vector = 0x21;
    entry.DelivMode = DELIV_M_FIXED;
    entry.DestMode = DEST_M_PHYSICAL;
    entry.IntMask = IOAPIC_INT_MASKED;
    entry.IntPol = IOAPIC_INTPOL_H;
    entry.IRR = IOAPIC_IRR_RESET;
    entry.Trigger = IOAPIC_TRIGGER_EDGE;
    entry.DelivStatus = DELIV_S_IDLE;
    entry.reserverd = 0;
    entry.DestField.physical.reserverd1 = 0;
    entry.DestField.physical.physic_dst = 0;
    entry.DestField.physical.reserverd2 = 0;

    WAIT_KB_WRITE();
    OUT8b(KB_CMD_PORT, KB_CONF_WRITE);
    WAIT_KB_WRITE();
    OUT8b(KB_DATA_PORT, KB_INIT_CONF);

    for (int i = 0; i < 1000000; i++)
        nop();

    ShiftL = 0;
    ShiftR = 0;
    CtrlL = 0;
    CtrlR = 0;
    AltL = 0;
    AltR = 0;
    Caps = 0;

    RegisterIrq(0x21, &entry, &KeyboardHandler, (unsigned long)KBIB, &KBController, "PS/2 Keyboard");
}