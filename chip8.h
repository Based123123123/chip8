#include "defines.h"
#include "display.h"
#ifndef chip8_h
	#define chip8_h
	#define clockSpeed 500
	#define ramSize 0x0fff
	#define font 0x0050
	#define PCInit 0x0200
	#define stackBegin 0x0ea0
	#define stackEnd 0x0ecf
	#define VX 0x0ef0
	#define V(X) ram[VX+(X)]
	#define S(SP) ram[stackBegin+(SP)]
	struct Chip8{
		ui8 ram[ramSize];
		bool displayData[displayHeight*displayWidth];
		ui16 PC;
		ui16 I;
		ui8 SP;	//Stack Pointer
		ui8 delayTimer;
		ui8 soundTimer;
		Display *display;
		bool keypad[16];
		bool wait;
		bool draw;
		Chip8(const i8* rom);
		void loop();
		ui16 fetch();
		void decode(ui16 instr);
		ui8 getKey();
		void checkKey();
	};
#endif
