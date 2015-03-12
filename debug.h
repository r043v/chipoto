
#define LOGBLACK chip8::debug::cpulog::currentColor=4
#define LOGGREEN chip8::debug::cpulog::currentColor=1
#define LOGRED chip8::debug::cpulog::currentColor=2
#define LOGYELLOW chip8::debug::cpulog::currentColor=3

namespace chip8 {

	namespace debug {
		
		u32 opbysecond, opbyloop;
	  
		WINDOW *create_newwin(int height, int width, int starty, int startx)
		{	WINDOW *local_win;
			local_win = newwin(height, width, starty, startx);
			box(local_win, 0 , 0);
			wrefresh(local_win);
			return local_win;
		}

		void destroy_win(WINDOW *local_win)
		{	wborder(local_win, ' ', ' ', ' ',' ',' ',' ',' ',' ');
			wrefresh(local_win);
			delwin(local_win);
		}
	  
		enum  { c8d_all, c8d_registers, c8d_keys, c8d_ram, c8d_rom, c8d_exec, c8d_screen, c8d_machine, c8d_log };
		WINDOW *wregisters=0, *wkeys=0, *wram=0, *wrom=0, *wprog=0, *wscreen=0, *wmachine=0;
		
		char **dasm_s=0;
		char * unknowOpCode = (char*)"unknow";
		
		char * dasm(u16 opc, u8 stack){//u8 * pc){
			//u16 opc = ( *pc << 8) | pc[1];
			if(opc == 0xffff) dasm_s[opc] = unknowOpCode;
			if(dasm_s[opc]) return dasm_s[opc];
			char bf[256];
			opcode op; op.raw = opc;
			*bf = 0;
			switch(op.a){
				case 0:
					if(op.bc == 0x0E){
						u8 tmp = op.d;
						
						// clear screen
						if(tmp == 0){
							strcpy(bf,"clear screen");
							break;
						}
						
						// back from subroutine
						if(tmp == 0xE){
							strcpy(bf,"back from subroutine");
							break;
						}
					}
					
					if(op.bcd){
						sprintf(bf,"call rca1802 program at 0x%x",op.bcd);
					} else { //opcode 0, wtf ?
						strcpy(bf,"empty opcode, wtf ?");
					}
				break;
				case 1:
					sprintf(bf,"jump to 0x%x",op.bcd);
				break;
				case 2:
					sprintf(bf,"call subroutine at 0x%03x",op.bcd);
				break;
				case 3:
					sprintf(bf,"skip next if 0x%02x == v%X",op.cd,op.b);
				break;
				case 4:
					sprintf(bf,"skip next if 0x%02x != v%X",op.cd,op.b);
				break;
				case 5:
					sprintf(bf,"skip next if v%X == v%X",op.b,op.c);
				break;
				case 6:
					sprintf(bf,"set v%X to 0x%02x",op.b,op.cd);
				break;
				case 7:
					sprintf(bf,"v%X += 0x%02x",op.b,op.cd);
				break;
				case 8:
					switch(op.d){
						case 0:
							sprintf(bf,"v%X = v%X",op.b,op.c);
						break;
						case 1:
							sprintf(bf,"v%X |= v%X",op.b,op.c);
						break;
						case 2:
							sprintf(bf,"v%X &= v%X",op.b,op.c);
						break;
						case 3:
							sprintf(bf,"v%X ^= v%X",op.b,op.c);
						break;
						case 4:
							sprintf(bf,"v%X += v%X",op.b,op.c);
						break;
						case 5:
							sprintf(bf,"v%X -= v%X",op.b,op.c);
						break;
						case 6:
							sprintf(bf,"v%X >>= 1",op.b);
						break;
						case 7:
							sprintf(bf,"v%X = v%X - v%X",op.b,op.c,op.b);
						break;
						case 0xE:
							sprintf(bf,"v%X <<= 1",op.b);
						break;
					};
				break;
				case 9:
					sprintf(bf,"skip next if v%X != v%X",op.b,op.c);
				break;
				case 0xA:
					sprintf(bf,"set ram pointer (I) to 0x%03x",op.bcd);
				break;
				case 0xB:
					sprintf(bf,"jump to 0x%03x + register 0",op.bcd);
				break;
				case 0xC:
					sprintf(bf,"set v%X to random(0xff) & 0x%02x",op.b,op.cd);
				break;
				case 0xD:
					sprintf(bf,"blit ram sprite of 8x%u at [v%X,v%X]",op.d,op.b,op.c);
				break;
				case 0xE:
					switch(op.cd){
						case 0x9E:
							sprintf(bf,"skip next if key v%X is pressed",op.b);
						break;
						case 0xA1:
							sprintf(bf,"skip next if key v%X is not pressed",op.b);
						break;
					};
				break;
				case 0xF:
					switch(op.cd){
						case 0x07:
							sprintf(bf,"v%X = delay timer",op.b);
						break;
						case 0x0A:
							sprintf(bf,"v%X = wait a key",op.b);
						break;
						case 0x15:
							sprintf(bf,"delay timer = v%X",op.b);
						break;
						case 0x18:
							sprintf(bf,"sound timer = v%X",op.b);
						break;
						case 0x1E:
							sprintf(bf,"ram += v%X",op.b);
						break;
						case 0x29:
							sprintf(bf,"ram = sprite v%X",op.b);
						break;
						case 0x33:
							sprintf(bf,"itoa of v%X to ram",op.b);
						break;
						case 0x55:
							sprintf(bf,"store in ram v0 to v%X",op.b);
						break;
						case 0x65:
							sprintf(bf,"load v0 to v%X from ram",op.b);
						break;
					};
				break;
			};
			
			char * txt;
			
			//sprintf(bf,"%04x  %x.%x.%x.%x",opc,op.a,op.b,op.c,op.d);
			
			//char stackstring[32] = { 0 };
			//memset(stackstring,'-',stack);
			//stackstring[stack] = 0;
			
			if(*bf){
				sprintf(bf,"%s | %x%x%x%x",bf,op.a,op.b,op.c,op.d);
				u32 l = strlen(bf);
				txt = (char*)malloc(l+1);
				strcpy(txt,bf);
			} else {
				txt = unknowOpCode;
			}
			
			dasm_s[opc] = txt;
			return dasm_s[opc];
		}
		
		namespace cpulog {
			struct opcode {
				opcode *next, *prev; u16 opc, pc; u8 stack; u8 color; char * dasm;
			};
			
			int opcodesNb = 0;
			struct opcode *first=0, *last=0;
		
			int lines2log = 0;
			int currentColor = 0;
			void add(u16 opc, u16 pc, u8 stack){
				struct opcode *o ;
				if(opcodesNb >= chip8_historySize){
					o = first;
					first = first->next;
					first->prev = 0;
				} else {
					o = (struct opcode*)malloc(sizeof(struct opcode));
					if(!opcodesNb++) first = o;
				}
				if(o){
					struct opcode * prev = last; last = o;
					o->opc = opc;
					o->pc = pc;
					o->stack = stack;
					o->color = 4;
					o->dasm = dasm(opc,stack);
					o->prev = prev;
					if(prev) prev->next = o;
					o->next = 0;
					LOGBLACK;
				}
			}
		}
		
		u32 hilightKeys[16];//= {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
		
		void hilightKey(u32 key){
			unsigned int tick = SDL_GetTicks();
			static unsigned int lasttick=tick;
			static unsigned int active = 0;
			if(key > 0xf){
				
				if(active){
					u32 t = tick - lasttick;
					for(int n=0;n<16;n++)
						if(hilightKeys[n]){
							if(hilightKeys[n] <= t){
								hilightKeys[n] = 0;
								if(active) active--;
							} else hilightKeys[n] -= t;
						}
				}
				
				lasttick = tick;
				return;
			}
			
			hilightKeys[key] = chip8_keyHilightTime; active++;
		}
		
		void init(void){
			dasm_s = (char**)malloc(0xffff*sizeof(char*));
			memset(dasm_s,0,0xffff*sizeof(char*));
			initscr(); noecho(); curs_set(FALSE);
			memset(hilightKeys,0,16*sizeof(u32));
			start_color();

			init_pair(4, COLOR_WHITE, COLOR_BLACK);
			init_pair(1, COLOR_GREEN, COLOR_BLACK);
			init_pair(2, COLOR_RED, COLOR_BLACK);
			init_pair(3, COLOR_YELLOW, COLOR_BLACK);
			
			int tlines, tcols;
			getmaxyx(stdscr, tlines, tcols);
			cpulog::lines2log = tlines-2;
			
			wregisters = create_newwin(18,8,0,0);
			wkeys = create_newwin(10,16,0,8);
			wmachine = create_newwin(8,16,10,8); //tcols-24
			wram = create_newwin(tlines,24,0,24); //tcols-24
			wrom = create_newwin(tlines,52,0,48); //tcols-24
			
			//wscreen = create_newwin(chip8_ScreenPxY,chip8_ScreenPxX,0,16);
			wborder(wregisters, 0, 0, 0, 0, 0, 0, 0, 0);
			wborder(wkeys, 0, 0, 0, 0, 0, 0, 0, 0);
			wborder(wram, 0, 0, 0, 0, 0, 0, 0, 0);
			wborder(wmachine, 0, 0, 0, 0, 0, 0, 0, 0);
			wborder(wrom, 0, 0, 0, 0, 0, 0, 0, 0);
			//scrollok(wrom, TRUE);
		}
		
		void end(void){
			destroy_win(wregisters);
			destroy_win(wkeys);
			destroy_win(wram);
			destroy_win(wmachine);
			destroy_win(wrom);
			endwin();
			
			if(dasm_s){
				u32 n = 0xffff;
				while(n--) if(dasm_s[n] && dasm_s[n] != unknowOpCode) free(dasm_s[n]);
				free(dasm_s);
			}
			
			cpulog::opcode * o = cpulog::first;
			while(1){
				cpulog::opcode * next = o->next;
				free(o); if(!next) break; o = next;
			};
		}
		
		void debugSprite(u8 n){
			printf("\ndebug sprite %1x",n);
			sprite * ps = &ram.sprites[n];
			u32 nl = 0;
			spriteLine * l;
			do {	l = &ps->line[nl];
				//printf("\n%b",l->value);
				printf("\n%u%u%u%u",l->col1,l->col2,l->col3,l->col4);
			} while(nl++ != 4);
		}

		void update(int what) {
			switch(what) {
				case c8d_all:
					for(int dbg=1;dbg<=c8d_log;dbg++) update(dbg);
				break;
				case c8d_screen:
				{	/*u8 s[chip8_ScreenPx+1], *sp = s, *send = &s[chip8_ScreenPx] ;
					u8 *scrp = chip8::screen.raw;
					while(sp != send)
						*sp++ = *scrp++ ? '@' : ' ';
					*sp = 0;
					wmove(wscreen,0,0);
					wprintw(wscreen,"%s",s);
					wrefresh(wscreen);*/
				} break;
				case c8d_registers:
				{	wmove(wregisters,0,1);
					wprintw(wregisters," regs ");
					
					u32 n = 0;
					do {
						wmove(wregisters,n+1,1);
						wprintw(wregisters,"%1x 0x%02X",n,cpu.reg[n]);
					} while(++n < 16);

					wrefresh(wregisters);
				} break;
				case c8d_keys :
				{	wmove(wkeys,0,1);
					wprintw(wkeys," keys ");
					hilightKey(0xff);
					
					u32 n = 0, l = 1;
					do {	wattron(wkeys,COLOR_PAIR( hilightKeys[n] ? 1 : 4 ));
						wmove(wkeys,l,1);
						wprintw(wkeys,"%c %s",'a'+n,keys.raw[n] ? "oooo" : "----"); n++;
						wattron(wkeys,COLOR_PAIR( hilightKeys[n] ? 1 : 4 ));
						wmove(wkeys,l,8);
						wprintw(wkeys,"%c %s",'a'+n,keys.raw[n] ? "oooo" : "----"); n++;
						l++;
					} while(n < 16);
					wrefresh(wkeys);
				} break;
				case c8d_ram:
				{	wmove(wram,0,1);
					wprintw(wram," ram ");
					u8 *r = ram.index, *rend = r + cpulog::lines2log; u32 n = 1; r8 * rr;
					if(rend > &ram.raw[4095]) rend = &ram.raw[4095];
					while(r != rend){
						wmove(wram,n++,1);
						rr = (r8*)r;
						wprintw(wram,"0x%03x 0x%02x 0b%u%u%u%u%u%u%u%u",r-ram.raw,*r,
							rr->b0,
							rr->b1,
							rr->b2,
							rr->b3,
							rr->b4,
							rr->b5,
							rr->b6,
							rr->b7   
						); r++;
					};
					
					wrefresh(wram);
				} break;
				case c8d_machine:
					wmove(wmachine,1,1);
					wprintw(wmachine,"    pc 0x%03x",cpu.pc - ram.raw);
					wmove(wmachine,2,1);
					wprintw(wmachine,"   ram 0x%03x",ram.index - ram.raw);
					wmove(wmachine,3,1);
					wprintw(wmachine," stack %u",cpu.stack.pointer);
					wmove(wmachine,4,1);
					wprintw(wmachine," timer 0x%02x",timer.delay);
					wmove(wmachine,5,1);
					wprintw(wmachine," sound 0x%02x",timer.sound);
					wrefresh(wmachine);
				break;
				case c8d_rom:
				{	wmove(wrom,0,1);
					wattron(wrom,COLOR_PAIR(4));
					
					static unsigned long t_opbyloop=0, t_opbyloopNb=0, t_opbysec=0, t_opbysecNb=0, lastopbysec=0;
					static u32 opbysecondf = 0;
					
					t_opbyloop += opbyloop; t_opbyloopNb++;
					
					
					float opbyloopf = (float)t_opbyloop / (float)t_opbyloopNb;
					
					if(opbysecond != lastopbysec){
						t_opbysec += opbysecond;
						t_opbysecNb++;
						opbysecondf = t_opbysec / t_opbysecNb;
						lastopbysec = opbysecond;
					}
					
					wprintw(wrom," exec | %2.1f op/f | %u op/s | %2.1f f/s ",opbyloopf,opbysecondf,(float)opbysecondf/opbyloopf);
					struct cpulog::opcode *log = cpulog::last;
					u32 n = 0, l = 1;
					if(log) while(n < cpulog::lines2log){
						wmove(wrom,l++,1);
						//wprintw(wrom,"%03x %46s",pc+n,dasm(&cpu.pc[n]));
						wattron(wrom,COLOR_PAIR(4));
						if(log->color) wattron(wrom,COLOR_PAIR(log->color));
						wprintw(wrom,"%03x %46s",log->pc,log->dasm);
						if(!log->prev) break; log = log->prev;
						n++;
					};
					wrefresh(wrom);
				} break;
			};
		}
	};
};