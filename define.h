
#define chip8_fpsDelaySpeed 60

#ifndef chip8_noSleep
	#define chip8_mainSleep 0
	#define chip8_threadSleep 0
#endif

#define chip8_screenFadeFps 200
#define chip8_keyHilightTime 1500
#define chip8_historySize 1024

#define chip8_SpritesOffset 80
#define chip8_StackSize 16
#define chip8_RamSize 4096
#define chip8_InterpreterHeaderSize 512
#define chip8_ScreenPxX 64
#define chip8_ScreenPxY 32
#define chip8_ScreenPx chip8_ScreenPxX * chip8_ScreenPxY

using namespace std;

#define u8 uint8_t
#define u16 uint16_t
#define u32 uint32_t

#include <SDL2/SDL.h>

#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>

#ifdef chip8_debug
	#include <curses.h>
#endif

#ifdef chip8_useThread
	#include <pthread.h>
#endif

#define chip8_msExecSpeed 1000000/chip8_opsExecSpeed
#define chip8_msDelaySpeed 1000000/chip8_fpsDelaySpeed
#define chip8_screenFadeMs 1000000/chip8_screenFadeFps

namespace chip8 {
#ifdef chip8_useThread
	u32 state; // 0 stop, 1 run
#endif
	#pragma pack(push, 1)
	typedef union spriteLine_t {
		u8 raw;
		struct {
			u8 value : 4;
			u8 : 4;
		};
		struct {
			u8 col1 : 1;
			u8 col2 : 1;
			u8 col3 : 1;
			u8 col4 : 1;
			u8 : 4;
		};
	} spriteLine;
	
	typedef union sprite_t {
		u8 raw[5];
		spriteLine line[5];
	} sprite;
	#pragma pack(pop)
	
	struct ram {
		#pragma pack(push, 1)
		union {
			u8 raw[chip8_RamSize];
			struct {
				union { // interpreter code or font storage
					u8 interpreter[chip8_InterpreterHeaderSize];
					struct {
						u8 spriteJunkStart[chip8_SpritesOffset];
						union {
							u8 spriteRaw[5 * 16]; // 16 char of 4*5px .. 5o / char => 80o
							sprite sprites[16];
						};
						u8 spriteJunkEnd[chip8_InterpreterHeaderSize - ( chip8_SpritesOffset + 80 )];
					};
				};
				u8 usable[chip8_RamSize - (chip8_InterpreterHeaderSize + 96 + 256)];
				u8 stack[96];
				u8 display[256];
			};
		};
		#pragma pack(pop)
		
		union {
			u8 * index;
			u8 * i;
			u8 * I;
		};
	} ram;
	
	#pragma pack(push, 1)
	
	/*typedef struct {
		u8 : 1;
	} bit;
	
	typedef struct {
		bit raw[8];
	} byte;*/
	
	//typedef u8 bit:1;
	
	/* cpu register */
	typedef union r8_t {
		u8 raw;
		//bit bits[8];
		//byte bits;
		#if __BYTE_ORDER == __LITTLE_ENDIAN
		struct {
			u8 b7 : 1;
			u8 b6 : 1;
			u8 b5 : 1;
			u8 b4 : 1;
			u8 b3 : 1;
			u8 b2 : 1;
			u8 b1 : 1;
			u8 b0 : 1;
		};
		struct {
			u8 b : 4;
			u8 a : 4;
		};
		#elif __BYTE_ORDER == __BIG_ENDIAN
		struct {
			u8 b0 : 1;
			u8 b1 : 1;
			u8 b2 : 1;
			u8 b3 : 1;
			u8 b4 : 1;
			u8 b5 : 1;
			u8 b6 : 1;
			u8 b7 : 1;
		};
		struct {
			u8 a : 4;
			u8 b : 4;
		};
		#endif
	} r8;
	
	/* cpu opcode */
	typedef union opcode_t {
		u16 raw;
		u8 b8[2];
		#if __BYTE_ORDER == __LITTLE_ENDIAN
		struct {
			u16 bcd : 12;
			u16 : 4;
		};
		struct {
			u16 : 4;
			u16 bc : 8;
			u16 : 4;
		};
		struct {
			u8 cd;
			u8 ab;
		};
		struct {
			u16 : 4 ;
			u16 abc : 12;
		};
		struct {
			u8 d : 4;
			u8 c : 4;
			u8 b : 4;
			u8 a : 4;
		};
		#elif __BYTE_ORDER == __BIG_ENDIAN
		struct {
			u16 : 4;
			u16 bcd : 12;
		};
		struct {
			u16 : 4;
			u16 bc : 8;
			u16 : 4;
		};
		struct {
			u8 ab;
			u8 cd;
		};
		struct {
			u16 abc : 12;
			u16 : 4 ;
		};
		struct {
			u8 a : 4;
			u8 b : 4;
			u8 c : 4;
			u8 d : 4;
		};
		#endif
	} opcode;
	
	#pragma pack(pop)
	
	struct cpu {
		#pragma pack(push, 1)
		union { // registers
			u8 reg[16];
			r8 regs[16];
			struct {
				#if __BYTE_ORDER == __LITTLE_ENDIAN
				r8 v0;
				r8 v1;
				r8 v2;
				r8 v3;
				r8 v4;
				r8 v5;
				r8 v6;
				r8 v7;
				r8 v8;
				r8 v9;
				r8 vA;
				r8 vB;
				r8 vC;
				r8 vD;
				r8 vE;
				union {
					r8 carry;
					r8 vF;
				};
				#elif __BYTE_ORDER == __BIG_ENDIAN
				union {
					r8 carry;
					r8 vF;
				};
				r8 vE;
				r8 vD;
				r8 vC;
				r8 vB;
				r8 vA;
				r8 v9;
				r8 v8;
				r8 v7;
				r8 v6;
				r8 v5;
				r8 v4;
				r8 v3;
				r8 v2;
				r8 v1;
				r8 v0;
				#endif
			};
		};
		#pragma pack(pop)
		
		union {
			u8 * programCounter;
			u8 * pc;
			u8 * PC;
		};
		
		struct stack {
			u8 * location[chip8_StackSize];
			u8 pointer;
		} stack;
	} cpu;
	
	union screen {
		u8 raw[chip8_ScreenPx];
		u8 line[chip8_ScreenPxY][chip8_ScreenPxX];
	} screen;
	
	struct timer {
		u8 delay;
		u8 sound;
	} timer;
	
	#pragma pack(push, 1)
	union keys {
		u8 raw[16];
		struct pad {
			#if __BYTE_ORDER == __LITTLE_ENDIAN
			u8 : 8;
			u8 : 8;
			u8 : 8;
			u8 : 8;
			u8 right;
			u8 : 8;
			u8 : 8;
			u8 left;
			u8 : 8;
			u8 : 8;
			u8 down;
			u8 : 8;
			u8 : 8;
			u8 up;
			u8 : 8;
			u8 : 8;
			#elif __BYTE_ORDER == __BIG_ENDIAN
			u8 : 8;
			u8 : 8;
			u8 up;
			u8 : 8;
			u8 : 8;
			u8 down;
			u8 : 8;
			u8 : 8;
			u8 left;
			u8 : 8;
			u8 : 8;
			u8 right;
			u8 : 8;
			u8 : 8;
			u8 : 8;
			u8 : 8;
			#endif
		} pad;
	} keys;
	#pragma pack(pop)
};

#ifdef chip8_debug
#include "debug.h"
#endif
