#include <stdarg.h>
#include <stddef.h>

#include "printk.h"
#include "lib.h"
#include "font.h"

#define     TAB_WIDTH 8
#define     MAX_BUFFER_LEN  4096
#define     XCharResolution \
            (screenInfo.XPixelResolution / screenInfo.charWidth)*screenInfo.charWidth
#define     YCharResolution \
            (screenInfo.YPixelResolution / screenInfo.charHeight)*screenInfo.charHeight

#define     AutoPutChar(x) \
            if(screenInfo.cursorX < XCharResolution){ \
                Putchar(x); \
                screenInfo.cursorX++; \
            }else{ \
                screenInfo.cursorX = 0; \
                screenInfo.cursorY < YCharResolution ? screenInfo.YPixelResolution++ : 0; \
                Putchar(x); \
                screenInfo.cursorX++; \
            } \


ScreenInfo screenInfo;

int ColorPrintfk(int ForeColor, int BackColor, const char* fmt, ...) {
    char buffer[MAX_BUFFER_LEN];
    va_list args;
    va_start(args, fmt);

    int BufferLen = VsPrintfk(buffer, fmt, args);  // used for formatting string unsafe

    va_end(args);

    for(int i = 0, Offset; i < BufferLen; i++){
        if(buffer[i] == '\\'){
            i++;
            switch (buffer[i]){
            case 't':
                Offset = TAB_WIDTH - screenInfo.cursorX % TAB_WIDTH;
                while(Offset--){
                    Putchar(' ', ForeColor, BackColor);
                    screenInfo.cursorX++;
                }
                break;

            case 'n':
                if(screenInfo.cursorY < (screenInfo.YPixelResolution / screenInfo.charHeight))
                    screenInfo.cursorY++;
    
                screenInfo.cursorX = 0;
                break;

            case 'b':
                if(screenInfo.cursorX > 0){
                    screenInfo.cursorX--;
                    Putchar(' ', ForeColor, BackColor);
                    screenInfo.cursorX++;
                }else{
                    // TO BE COMPLETE
                }
                break;

            case '\\':
                if(screenInfo.cursorX < XCharResolution){
                    Putchar('\\', ForeColor, BackColor);
                    screenInfo.cursorX++;
                }else{
                    screenInfo.cursorX = 0;
                    screenInfo.cursorY < YCharResolution ? screenInfo.YPixelResolution++ : 0;
                    Putchar('\\', ForeColor, BackColor);
                }
                break;

            default:
                if(screenInfo.cursorX < XCharResolution){
                    Putchar('?', ForeColor, BackColor);
                    screenInfo.cursorX++;
                }else{
                    screenInfo.cursorX = 0;
                    screenInfo.cursorY < YCharResolution ? screenInfo.YPixelResolution++ : 0;
                    Putchar('?', ForeColor, BackColor);
                    screenInfo.cursorX++;
                }
            }
        }else{
            if(screenInfo.cursorX < XCharResolution){
                Putchar(buffer[i], ForeColor, BackColor);
                screenInfo.cursorX++;
            }else{
                screenInfo.cursorX = 0;
                screenInfo.cursorY < YCharResolution ? screenInfo.YPixelResolution++ : 0;
                Putchar(buffer[i], ForeColor, BackColor);
                screenInfo.cursorX++;
            }
        }
    }
    return 0;
}

int VsPrintfk(char* buffer, const char* fmt, va_list args){
    
    char    CurrentChar  =  *fmt;
    int     BufferIndex  =  0;
    char    TempBuffer[20];

    int     Upper;
    unsigned int ArgNum;
    int     IndexIncre;
    char    *ArgString;
    void    *ArgPtr;

    while(CurrentChar   != '\0'){

        if(CurrentChar  == '%'){

            CurrentChar = *(fmt++);

            if(CurrentChar == '\0') break;



            switch (CurrentChar){
            case '%':
                buffer[BufferIndex++] = '%';
                break;

            case 'x':
            case 'X':
                Upper = CurrentChar < 'a' ? 1 : 0; 
                
                ArgNum = va_arg(args, unsigned int);

                IndexIncre = ToHexString(TempBuffer, ArgNum, Upper);

                if(IndexIncre + BufferIndex >= MAX_BUFFER_LEN){
                    CurrentChar = 0;
                    break;
                }

                StringCopy(TempBuffer, buffer + BufferIndex);
                BufferIndex += IndexIncre;
                break;

            case 'd':
                ArgNum = va_arg(args, int);

                IndexIncre = ToDeciString(TempBuffer, ArgNum);

                if(IndexIncre + BufferIndex >= MAX_BUFFER_LEN){
                    CurrentChar = 0;
                    break;
                }

                StringCopy(TempBuffer, buffer + BufferIndex);
                BufferIndex += IndexIncre;
                break;

            case 's':
                ArgString = va_arg(args, char*);
                IndexIncre = StringLen(ArgString);

                if(IndexIncre + BufferIndex >= MAX_BUFFER_LEN){
                    CurrentChar = 0;
                    break;
                }

                StringCopy(ArgString, buffer + BufferIndex);
                BufferIndex += IndexIncre;
                break;

            case 'c':
                buffer[BufferIndex++] = va_arg(args, int);
                break;
            
            case 'p':              
                ArgPtr = va_arg(args, void*);

                IndexIncre = ToPtrString(TempBuffer, ArgPtr);

                if(IndexIncre + BufferIndex >= MAX_BUFFER_LEN){
                    CurrentChar = 0;
                    break;
                }

                StringCopy(TempBuffer, buffer + BufferIndex);
                BufferIndex += IndexIncre;
                break;

            default:
                buffer[BufferIndex++] = '?';
                break;
            }
            
            CurrentChar = *(fmt++);

        }else{

            buffer[BufferIndex++] = CurrentChar;
            CurrentChar = *(fmt++);

        }
    }

    buffer[BufferIndex] = '\0';
    return BufferIndex + 1;
}

// TO BE COMPLETE
void Putchar(char character, uint32_t ForeColor, uint32_t BackColor){
    int CursorX             = screenInfo.cursorX;
    int CursorY             = screenInfo.cursorY;
    int CharHeight          = screenInfo.charHeight;
    int CharWidth           = screenInfo.charWidth;
    int XPixelResolution    = screenInfo.XPixelResolution;
    int YPixelResolution    = screenInfo.YPixelResolution;
    uint32_t* FramBuffer    = screenInfo.framebuffer;

    uint8_t CharNum         = (uint8_t)character;
    unsigned char* CharPlot = font_ascii[CharNum];

    for(int i = 0; i < CharHeight; i++){
        uint8_t PlotLine    = (uint8_t)CharPlot[i];
        uint8_t Pattern     = 1 << 7;
        int PixelPos        = (CursorY * CharHeight + i) * XPixelResolution + (CursorX * CharWidth);
        while(Pattern){
            uint8_t PixelBit = PlotLine & Pattern;
            uint32_t Pixel  = PixelBit ? ForeColor : BackColor;
            *(FramBuffer + PixelPos) = Pixel;
            PixelPos++;
            Pattern >>= 1;
        }
    }
}

int PrintkInit(){
    screenInfo = (ScreenInfo){
        .XPixelResolution = 1440,
        .YPixelResolution = 900,
        .cursorX = 0,
        .cursorY = 0,
        .charWidth = 8,
        .charHeight = 16,
        .framebuffer = (uint32_t*)0xffff800000a00000
    };
    return 1;
}