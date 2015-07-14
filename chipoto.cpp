
#define rom2load "./trip8.ch8"
#define chip8_opsExecSpeed 1000000 // 0 ~ 1.000.000Hz
#define chip8_useThread
#define chip8_enableScreenFade
#define chip8_debug
//#define chip8_noSleep

#include "define.h"

namespace chip8 {
	
	u32 screenUpdate=0;
	
	static inline unsigned int exec(opcode op){
#ifdef chip8_debug
		debug::cpulog::add(op.raw,cpu.pc-ram.raw,cpu.stack.pointer);
#endif
		switch(op.a){
			case 0:
				if(op.bc == 0x0E){
					u8 tmp = op.d;
					
					// clear screen
					if(tmp == 0){
						//printf("clear screen");
						memset(screen.raw,0,chip8_ScreenPx);
						screenUpdate = 1;
						break;
					}
					
					// back from subroutine
					if(tmp == 0xE){
						//printf("back from subroutine");
						cpu.pc = cpu.stack.location[--cpu.stack.pointer];
						return 1;
						break;
					}
				}
				/*if(op.bcd){
					// call rca1802 program at op.bcd
					//printf("call rca1802 program at 0x%x -- unimplemented",op.bcd);
				} else { //opcode 0, wtf ?
					//printf("empty opcode, wtf ?\n");
					//debug();
				}*/
			break;
			case 1: // !! pc *= 2 ?
				//printf("jump to 0x%x",op.bcd);
				cpu.pc = &ram.raw[op.bcd];
				return 1;
			break;
			case 2: // !! pc *= 2 ?
				//printf("call subroutine at 0x%x (%u)",op.bcd,op.bcd);
				cpu.stack.location[cpu.stack.pointer++] = cpu.pc + 2;
				cpu.pc = &ram.raw[op.bcd];
				//cpu.pc = &ram.raw[op.raw & 0x0FFF];
				return 1;
			break;
			case 3:
				//printf("skip if %x == register %x",op.cd,op.b);
				// skip next instruction if value == register X
				//u8 x = op.b; u8 value = op.cd;
#ifdef chip8_debug
				if(cpu.reg[op.b] == op.cd) { cpu.pc += 2; LOGGREEN; } else LOGRED;
#else
				if(cpu.reg[op.b] == op.cd) cpu.pc += 2;
#endif				
			break;
			case 4:
				//printf("skip if %x != register %x(%x)",op.cd,op.b,cpu.reg[op.b]);
				// skip next instruction if op.cd != register b
				//u8 x = op.b; u8 value = op.cd;
#ifdef chip8_debug
				if(cpu.reg[op.b] != op.cd) { cpu.pc += 2; LOGGREEN; } else LOGRED;
#else
				if(cpu.reg[op.b] != op.cd) cpu.pc += 2;
#endif
			break;
			case 5:
				if(op.d != 0) break;
				//printf("skip if register %x == register %x",op.b,op.c);
				// skip next instruction if register b == register c
#ifdef chip8_debug
				if(cpu.reg[op.b] == cpu.reg[op.c]){ cpu.pc += 2; LOGGREEN; } else LOGRED;
#else
				if(cpu.reg[op.b] == cpu.reg[op.c]) cpu.pc += 2;
#endif
			break;
			case 6:
				//printf("set register %x to %x",op.b,op.cd);
				cpu.reg[op.b] = op.cd;
			break;
			case 7:
			{	//printf("add %x to register %x",op.cd,op.b);
				u32 v = cpu.reg[op.b] + op.cd;
				cpu.carry.raw = v > 0xff;
				cpu.reg[op.b] = v & 0xff; //+= op.cd; // v & 0xff;
#ifdef chip8_debug
				if(cpu.carry.raw) LOGYELLOW;
#endif
			}
			break;
			case 8:
				//printf("logical set of register %x with register %x",op.b,op.c);
				switch(op.d){
					case 0:
						//printf("set register %x to register %x(%x)",op.b,op.c,cpu.reg[op.c]);
						cpu.reg[op.b] = cpu.reg[op.c];
					break;
					case 1:
						//printf("register %x |= register %x(%x)",op.b,op.c,cpu.reg[op.c]);
						cpu.reg[op.b] |= cpu.reg[op.c];
					break;
					case 2:
						//printf("register %x &= register %x(%x)",op.b,op.c,cpu.reg[op.c]);
						cpu.reg[op.b] &= cpu.reg[op.c];
					break;
					case 3:
						//printf("register %x ^= register %x(%x)",op.b,op.c,cpu.reg[op.c]);
						cpu.reg[op.b] ^= cpu.reg[op.c];
					break;
					case 4: // !!
						//printf("register %x += register %x(%x)",op.b,op.c,cpu.reg[op.c]);
						{	u32 v = cpu.reg[op.b] + cpu.reg[op.c];
							cpu.carry.raw = v > 0xff;
							cpu.reg[op.b] = v & 0xff;//+= cpu.reg[op.c]; //= v & 0xff;
#ifdef chip8_debug
							if(cpu.carry.raw) LOGYELLOW;
#endif
						}
					break;
					case 5: // !!
						//printf("register %x -= register %x(%x)",op.b,op.c,cpu.reg[op.c]);
						cpu.carry.raw = cpu.reg[op.c] < cpu.reg[op.b];
						cpu.reg[op.b] -= cpu.reg[op.c];
#ifdef chip8_debug
						if(cpu.carry.raw) LOGYELLOW;
#endif
						break;
					case 6:
						//printf("register %x >>= 1",op.b);
						{	u8 r = op.b;
							cpu.vF.raw = cpu.regs[r].b0; // !!
							cpu.reg[r] >>= 1;
#ifdef chip8_debug
							if(cpu.vF.raw) LOGGREEN; else LOGRED;
#endif
						  
						}
					break;
					case 7:
						//printf("register %x = register %x(%x) - register %x(%x)",op.b,op.c,cpu.reg[op.c],op.b,cpu.reg[op.b]);
						{	u8 r = op.b;
							u8 v = cpu.reg[op.c];
							cpu.vF.raw = v > cpu.reg[r];
							cpu.reg[r] = v - cpu.reg[r];
#ifdef chip8_debug
							if(cpu.carry.raw) LOGYELLOW;
#endif
						}
					break;
					case 0xE:
						//printf("register %x <<= 1",op.b);
						{	u8 r = op.b;
							cpu.vF.raw = cpu.regs[r].b7; // !!
							cpu.reg[r] <<= 1;
#ifdef chip8_debug
							if(cpu.vF.raw) LOGGREEN; else LOGRED;
#endif
						}
					break;
				};
			break;
			case 9:
				if(op.d != 0) break;
				//printf("skip next instruction if register %x != register %x",op.b,op.c);
				// skip next instruction if register b != register c
#ifdef chip8_debug
				if(cpu.reg[op.b] != cpu.reg[op.c]){ cpu.pc += 2; LOGGREEN; } else LOGRED;
#else
				if(cpu.reg[op.b] != cpu.reg[op.c]) cpu.pc += 2;
#endif
			break;
			case 0xA:
				//printf("set ram pointer (I) to 0x%x",op.bcd);
				ram.index = &ram.raw[op.bcd];
			break;
			case 0xB: // !! pc *= 2 ?
				//printf("jump to 0x%x + register 0",op.bcd);
				cpu.pc = &ram.raw[cpu.v0.raw + op.bcd];
				return 1;
			break;
			case 0xC:
				//printf("set register %x to random(0xff) & %x",op.b,op.cd);
				cpu.reg[op.b] = ( rand()%255 ) & op.cd;
			break;
			case 0xD:
				{	int x = (char)cpu.reg[op.b], y = (char)cpu.reg[op.c];
					u32 sy = op.d;
					
					u8 xclip = x < 0 || x+7 >= chip8_ScreenPxX;
					u8 yclip = y < 0 || y+sy-1 >= chip8_ScreenPxY;

//					if(xclip || yclip || !x || !y || y==31)
//						printf("\n x %3i  y %3i  sy %u",x,y,sy);
					
					r8 * spr = (r8*)ram.index;
					
					if(yclip){
						if(y > 0){ // clip down
							if(y >= chip8_ScreenPxY) break;
							sy = chip8_ScreenPxY - y;
						} else { // clip up
							if(y <= -sy) break;
							sy += y;
							spr -= y;
							y=0;
						}
					}
					
					u8 *s, *send;
					
					if(!xclip){
						s = screen.line[y] + x;
						send = s + sy*chip8_ScreenPxX;
//						if(yclip) printf(" y clip (sy %u)",sy);
						cpu.vF.raw = 0;
						while(s != send){
							if(cpu.vF.raw){
								*s++ ^= spr->b0;
								*s++ ^= spr->b1;
								*s++ ^= spr->b2;
								*s++ ^= spr->b3;
								*s++ ^= spr->b4;
								*s++ ^= spr->b5;
								*s++ ^= spr->b6;
								*s++ ^= spr->b7;
							} else {
								if(*s){ *s ^= spr->b0; if(!*s++) cpu.vF.raw = 1; } else *s++ ^= spr->b0;
								if(*s){ *s ^= spr->b1; if(!*s++) cpu.vF.raw = 1; } else *s++ ^= spr->b1;
								if(*s){ *s ^= spr->b2; if(!*s++) cpu.vF.raw = 1; } else *s++ ^= spr->b2;
								if(*s){ *s ^= spr->b3; if(!*s++) cpu.vF.raw = 1; } else *s++ ^= spr->b3;
								if(*s){ *s ^= spr->b4; if(!*s++) cpu.vF.raw = 1; } else *s++ ^= spr->b4;
								if(*s){ *s ^= spr->b5; if(!*s++) cpu.vF.raw = 1; } else *s++ ^= spr->b5;
								if(*s){ *s ^= spr->b6; if(!*s++) cpu.vF.raw = 1; } else *s++ ^= spr->b6;
								if(*s){ *s ^= spr->b7; if(!*s++) cpu.vF.raw = 1; } else *s++ ^= spr->b7;
							}
							spr++; s += chip8_ScreenPxX - 8;
						};
					} else { // x clip
						u32 xstart, xend, xsize; // pos to blit in sprite lines
						if(x < 0) { // clip left
							if(x < -6) break;
							xstart = -x;
							xend = 8;
							xsize = xend - xstart;
							x = 0;
						} else { // clip right
							if(x >= chip8_ScreenPxX) break;
							xstart = 0;
							xend = chip8_ScreenPxX-x;
							xsize = xend;
						}
						
						s = screen.line[y] + x;
						send = s + sy*chip8_ScreenPxX;
						
//						printf(" x clip, x %2i y %2i | %u %u, %u px",x,y,xstart,xend,xsize);
						
						u8 *sline = s;
						
						/*while(s < send){
							memset(s,0xff,xsize);
							s += chip8_ScreenPxX;
						} break;*/
						
						cpu.vF.raw = 0;
						while(s < send){
							if(cpu.vF.raw){
								for(u32 n=xstart;n<xend;n++)
									*s++ ^= (spr->raw >> (7-n))&1;
							} else {
								for(u32 n=xstart;n<xend;n++)
									if(*s){
										*s ^= (spr->raw >> (7-n))&1;
										if(!*s++) cpu.vF.raw = 1;
										//printf("1");
									} else	*s++ ^= (spr->raw >> (7-n))&1;
							}
							spr++; sline += chip8_ScreenPxX; s = sline;
						};
					}
					
					screenUpdate = 1;
				}
			break;
			case 0xE:
				switch(op.cd){
					case 0x9E:
						//printf("skip next instruction if key at register %x(%c) is pressed",op.b,'a'+cpu.reg[op.b]);
						//printf("\n"); sleep(3);
#ifdef chip8_debug
						if(  keys.raw[ cpu.reg[op.b] ] ){ cpu.pc += 2; LOGGREEN; } else LOGRED;
						chip8::debug::hilightKey(cpu.reg[op.b]);
#else
						if(  keys.raw[ cpu.reg[op.b] ] ) cpu.pc += 2;
#endif
					break;
					case 0xA1:
						//printf("skip next instruction if key at register %x(%c) is not pressed",op.b,'a'+cpu.reg[op.b]);
						//printf("\n"); sleep(3);
#ifdef chip8_debug
						if(! keys.raw[ cpu.reg[op.b] ] ){ cpu.pc += 2; LOGGREEN; } else LOGRED;
						chip8::debug::hilightKey(cpu.reg[op.b]);
#else
						if(! keys.raw[ cpu.reg[op.b] ] ) cpu.pc += 2;
#endif
					break;
				};
			break;
			case 0xF:
				u8 x = op.b;
				switch(op.cd){
					case 0x07:
						//printf("set register %x with value of delay timer",op.b);
						cpu.reg[x] = timer.delay;
					break;
					case 0x0A:
						//printf("wait a key and set register b with key value => ");
						{	u32 n = 0x0F, done = 0;
							do {
#ifdef chip8_debug
								chip8::debug::hilightKeys[n] = 255;
#endif
								if( keys.raw[n] ){
									cpu.reg[x] = n;
									done = 1;
									break;
								}
							} while(n--);
							return !done;
						}
					break;
					case 0x15:
						//printf("set delay timer to register %x value (%x)",op.b,cpu.reg[op.b]);
						timer.delay = cpu.reg[x];
					break;
					case 0x18:
						//printf("set sound timer to register %x value (%x)",op.b,cpu.reg[op.b]);
						timer.sound = cpu.reg[x];
					break;
					case 0x1E:
						//printf("add value of register %x(%x) to ram pointer (%x)",op.b,cpu.reg[op.b],ram.index);
						ram.index += cpu.reg[x];
						if(ram.index - ram.raw > 0xfff){
							cpu.vF.raw = 1;
							ram.index -= 0xfff; // ??
						} else	cpu.vF.raw = 0;
#ifdef chip8_debug
						if(cpu.vF.raw) LOGYELLOW;
#endif
					break;
					case 0x29:
						//printf("set ram pointer (I) to location of sprite #register b");
						ram.index = ram.sprites[ cpu.reg[x] ].raw;
					break;
					case 0x33:
						{	unsigned int n = cpu.reg[x];
							//printf("itoa of register %x value (%x) to ram => I(100) I+1(10) I+2(1)",x,n);
							
							u8 * r = ram.index;
							*r = 0;
							while(n > 99){
								n -= 100; (*r)++;
							};
							r++; *r = 0;
							while(n > 9){
								n -= 10; (*r)++;
							};
							r++; *r = n;
						}
					break;
					case 0x55:
						//printf("store registers 0 to b at ram index I");
						memcpy(ram.index,cpu.reg,x+1);
					break;
					case 0x65:
						//printf("load registers 0 to b from ram index I");
						memcpy(cpu.reg,ram.index,x+1);
					break;
				};
			break;
		};
		
		//printf("\n");
		return 0;
	}
	
	static inline unsigned int exec(u8 *o){
		opcode op; op.raw = ( *o << 8) | o[1]; return exec(op);
	}
	
	unsigned char chip8_fontset[80] =
	{ 
	  0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
	  0x20, 0x60, 0x20, 0x20, 0x70, // 1
	  0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
	  0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
	  0x90, 0x90, 0xF0, 0x10, 0x10, // 4
	  0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
	  0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
	  0xF0, 0x10, 0x20, 0x40, 0x40, // 7
	  0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
	  0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
	  0xF0, 0x90, 0xF0, 0x90, 0x90, // A
	  0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
	  0xF0, 0x80, 0x80, 0x80, 0xF0, // C
	  0xE0, 0x90, 0x90, 0x90, 0xE0, // D
	  0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
	  0xF0, 0x80, 0xF0, 0x80, 0x80  // F
	};
	
	void init(void){
		printf("init chip8 system ..\n");
		ram.index = ram.raw;
		cpu.pc = ram.raw + 0x200;
		memcpy(chip8::ram.spriteRaw,chip8_fontset,80);
		printf("system is init.\n");
	}
	
	void setSprite(u8 n, sprite *s){
		memcpy(ram.sprites[n].raw,s,sizeof(sprite));
	}
	
	void setSprite(u8 n, u32 s){
		sprite * ps = &ram.sprites[n];
		u8* rs = (u8*)&s;
		r8 r;
		r.raw = *rs++;
		ps->line[0].value = r.b;
		ps->line[1].value = r.a;
		r.raw = *rs++;
		ps->line[2].value = r.b;
		ps->line[3].value = r.a;
		r.raw = *rs++;
		ps->line[4].value = r.b;
	}
};

u8 * loadFile(const char *path, u32*size, u8*ptr)
{	FILE *f = fopen(path,"rb");
	if(!f){ if(size) *size=0; return 0; }
	fseek(f,0,SEEK_END);
	u32 fsize = ftell(f); if(size) *size = fsize;
	u8 * file = ptr ? ptr : (u8*)malloc(fsize);
	if(!file){ fclose(f); return 0; }
	fseek(f,0,SEEK_SET);
	fread(file,fsize,1,f); fclose(f);
	return file;
}

static inline unsigned long getTick(void){
	static struct timeval t;
	//static struct timespec t;
	//clock_gettime(CLOCK_REALTIME,&t);
	static unsigned long mt = 0;
	if(!gettimeofday(&t,0)) mt = 1000000 * t.tv_sec + t.tv_usec;
	//printf("\ntime : %u",mt);
	return mt;//t.tv_nsec;
}

unsigned long tick;
#ifdef chip8_useThread
static inline void * chip8Exec(void *a){
while(chip8::state){
#else
static inline void chip8Exec(void){
#endif
	static unsigned long lastExecTick=tick;
#ifdef chip8_debug
	static unsigned long lastSecondTick=tick;
	static u32 opcpt=0;
#endif
	
	tick = getTick();

#ifdef chip8_debug
	chip8::debug::opbyloop = 0;
#endif
	while(tick - lastExecTick >= chip8_msExecSpeed){
		if(!chip8::exec(chip8::cpu.pc)) chip8::cpu.pc += 2;
#ifdef chip8_debug
		if(tick - lastSecondTick >= 1000000){
			chip8::debug::opbysecond = opcpt; opcpt = 0; lastSecondTick = tick;
		} else opcpt++;

		chip8::debug::cpulog::last->color = chip8::debug::cpulog::currentColor;
		chip8::debug::opbyloop++;
#endif
		lastExecTick += chip8_msExecSpeed;
	};
/*
#ifdef chip8_debug
	if(chip8::debug::opbyloop) chip8::debug::update(chip8::debug::c8d_all);
#endif
*/
	//
#ifdef chip8_useThread
#ifdef chip8_threadSleep
usleep(chip8_threadSleep);
#endif
}; return 0;
#endif
}

int main(void){
	chip8::init();
	u8 * romPtr = &chip8::ram.raw[0x200]; u32  romSize;

	u8 * rom = loadFile(rom2load,&romSize,romPtr);
	if(!rom){ printf("loading rom error !"); return 1; }

	printf("rom loaded at 0x%x %uo",rom-chip8::ram.raw,romSize);
	//for(u32 n=0;n<romSize;n++) printf("%02x.",rom[n]);
	printf("\nstart emulation ..\n");

#ifdef chip8_debug
	chip8::debug::init();
#endif
	
	// init screen output, with a sdl backend
	SDL_Init(SDL_INIT_VIDEO);
	SDL_Window *screen = SDL_CreateWindow("chipoto",
                          SDL_WINDOWPOS_UNDEFINED,
                          SDL_WINDOWPOS_UNDEFINED,
                          chip8_ScreenPxX, chip8_ScreenPxY,
                          SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);
	SDL_Renderer *renderer = SDL_CreateRenderer(screen, -1, 0);
	SDL_Texture * texture = SDL_CreateTexture(renderer,
                               SDL_PIXELFORMAT_ARGB8888,
                               SDL_TEXTUREACCESS_STREAMING,
                               chip8_ScreenPxX, chip8_ScreenPxY);

	u32 myscreen[chip8_ScreenPx];
	memset(myscreen,0,chip8_ScreenPx*sizeof(u32));
	SDL_Event e;
	
	tick = getTick();
	
#ifdef chip8_useThread
	// emulation start
	chip8::state = 1;
	// launch exec thread
	pthread_t texec;
	int iret = pthread_create( &texec, NULL, chip8Exec, (void*)0);
	if(iret){
		fprintf(stderr,"Error - pthread_create() return code: %d\n",iret);
		exit(EXIT_FAILURE);
	}
	//printf("pthread_create() for thread 1 returns: %d\n",iret);
#endif
	
	while(1){
#ifdef chip8_mainSleep
	usleep(chip8_mainSleep);
#endif
#ifndef chip8_useThread
		chip8Exec();
#endif
		// check delay
		{	static unsigned long lastDelayTick = tick;

			if(tick - lastDelayTick >= chip8_msDelaySpeed){
				if(chip8::timer.delay) chip8::timer.delay--;
				if(chip8::timer.sound) chip8::timer.sound--;
				lastDelayTick = tick;// += chip8_DelaySpeedMs;
#ifdef chip8_debug
	// update debug window 60 time/second, with timers
	//if(chip8::debug::opbyloop)
	  chip8::debug::update(chip8::debug::c8d_all);
#endif
			};
		}

#ifdef chip8_enableScreenFade
		static unsigned long lastFadeSpeed = tick;
		u32 step = 0;
		unsigned long fadetime = tick - lastFadeSpeed;
		if(fadetime >= chip8_screenFadeMs){
			while(fadetime >= chip8_screenFadeMs){
				fadetime -= chip8_screenFadeMs;
				step++;
			};	lastFadeSpeed = tick;
		}
		
		if(step || chip8::screenUpdate){
			// break;
			// update & fade screen
			for(u32 n=0;n<chip8_ScreenPx;n++){
				int px = chip8::screen.raw[n];
				if(px)	myscreen[n] = 0xffffff;
				else	if(myscreen[n]){
						int color = myscreen[n]&0xff;
						if(step){
							if(color == 0xff) color = 64;
							if(color > step){
								color -= step;
								myscreen[n] = (color<<16) | (color);
							} else myscreen[n] = 0;
						}
					}
			}
			chip8::screenUpdate=1;
		}
#else
		if(chip8::screenUpdate){
			for(u32 n=0;n<chip8_ScreenPx;n++)
				myscreen[n] = chip8::screen.raw[n] ? 0xffffff : 0;
		}
#endif
		if(chip8::screenUpdate){
			SDL_UpdateTexture(texture, NULL, myscreen, chip8_ScreenPxX * sizeof (Uint32));
			SDL_RenderClear(renderer);
			SDL_RenderCopy(renderer, texture, NULL, NULL);
			SDL_RenderPresent(renderer);
			chip8::screenUpdate = 0;
		} // end of screen update
		
		while(SDL_PollEvent(&e)){
			if(e.type == SDL_QUIT){
#ifdef chip8_useThread
			chip8::state = 0;
			pthread_join( texec, NULL);
#endif
			  
			  
#ifdef chip8_debug
				chip8::debug::end();
#endif
				return 0;
			}
			
			if(e.type == SDL_KEYDOWN){
				char k = (char)e.key.keysym.sym;
				if(k >= 'a' && k < 'a'+16){
					k -= 'a';
					chip8::keys.raw[k] = 1;
#ifdef chip8_debug
					chip8::debug::hilightKey(k);
#endif
				}
				//printf("\npress key %c\n",(char)e.key.keysym.sym);
				continue;
			}
			
			if(e.type == SDL_KEYUP){
				char k = (char)e.key.keysym.sym;
				if(k >= 'a' && k < 'a'+16){
					k -= 'a';
					chip8::keys.raw[k] = 0;
#ifdef chip8_debug
					chip8::debug::hilightKey(k);
#endif
				}
				continue;
			}
		};
	}
	
	return 0;
}
