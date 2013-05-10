#include "kernel.h"
#include "sys.h"

#include "../mm/mm.h"
#include "../process/process.h"
#include "../vesa/vesa.h"
#define screen_width 800
#define screen_height 600
extern struct task_window * head;
extern struct task_window * tail;


struct KernelMessageQueue NRMessageQueue;
MSG Message;

int intmx = 400,intmy = 300; //Mouse old point
int oldlbutton = 0,oldrbutton = 0; //mouse button
int int_x = 0;
int int_y = 0;
int oldtime = 0,nowtime = 0;
char *key_map ="  1234567890-=  qwertyuiop[]  asdfghjkl;\"   \\zxcvbnm,./";


#define WM_LBUTTONUP 1
#define WM_LBUTTONDOWN  2
#define WM_RBUTTONDOWN 3
#define WM_RBUTTONUP 4
#define WM_MOUSEMOVE 5
#define WM_LBUTTONDBLCLK 6
#define WM_KEYDOWN                      0x0100
#define WM_KEYUP                        0x0101

void main();
//主函数
static _inline int fork(){
	_asm{
		mov edi,_sys_const_fork
			int 0x80
	}
}


void rename_main()
{
	init_interupt();
	_asm{
		push 0fh
			push 200000h
			pushfd
			push 07h
			push offset L0
			iretd
L0:
		mov eax,0fh
			mov ds,ax
			mov es,ax
			mov fs,ax
			mov gs,ax
			
	}
	main();
}
void main(){
	
    RECT r1;
    int xs=10,ys=27;
    MSG msg;
	unsigned short * recbuf = (unsigned short *)malloc(256);
				
	if (!fork()){
		

		if (!fork()){
			if(!fork()){
				r1.left = 100;
				r1.right =600;
				r1.top = 100;
				r1.bottom= 400;
				
				KnCreateWindow(&r1,(unsigned short *)current->twin.buf,&current->twin);
				DrawText(7,27,493,293,0,(unsigned short *)current->twin.buf);
				hanzi16(xs,ys,"C:\\>",0x7fff,(unsigned short *)current->twin.buf);
				for(;;)
				{
					if(KernelGetMessage(&current->twin.winmsgqueue,&msg))
					{
						
						switch(msg.message)
						{
						case WM_MOUSEMOVE:
							break;
						case WM_LBUTTONDOWN:
							break;
						case WM_KEYDOWN:
							ShowChar(xs+64,ys,*(key_map+msg.lParam),0x7fff,(unsigned short *)current->twin.buf);
							BitBltEx(xs+64+current->twin.x,ys+current->twin.y,xs+80+current->twin.x,ys+16+current->twin.y,xs+64,ys,(unsigned short *)current->twin.buf);
							xs += 16;
							if(xs > 800){
								xs = 0;ys += 16;
							}
							break;
						}
						
					}
				}
			}
			
			r1.left = 50;
			r1.right =200;
			r1.top = 50;
			r1.bottom= 300;
			
			KnCreateWindow(&r1,(unsigned short *)current->twin.buf,&current->twin);
			
			for(;;)
			{
				if(KernelGetMessage(&current->twin.winmsgqueue,&msg))
				{
					
					switch(msg.message)
					{
					case WM_LBUTTONDOWN:
						
						break;
					case WM_MOUSEMOVE:
						
						break;
					case WM_LBUTTONUP:
						
						break;
					case WM_KEYDOWN:
						ShowChar(xs,ys,*(key_map+msg.lParam),0x7c00,(unsigned short *)current->twin.buf);
						xs += 8;
						if(xs > 800){
							xs = 0;ys += 16;
						}
						break;
					}
					
				}
			}
			
		}
	
		init_window();
	}
	for(;;)
		schedule();
	
	
}

int sys_init_window(){
	readpic();
	//初始化消息队列
	InitMessageQueue(&NRMessageQueue);
	
	drawtaskbar();
	copy_memory((void *)video_address,(void *)current->twin.buf,0x100000);	
	initDesktop();
	
	
	return 1;
}
void initDesktop(){
	struct task_window *p,*s;
	struct task_window *tmp;
	struct task_struct **q;
	unsigned short* mousebuf;
	char * tmpbuf;
	char str1[8];
	int mx=0,my=0,dragx=0,dragy=0,mmx=0,mmy=0,counter=0,i,j,xs=0,ys=0;
	int oldx=400;
	int oldy=300;
	RECT r1,r2,r3,r4,r5;
	int draginfo = 0;
	unsigned short * pf = (unsigned short *)video_address;
	mousebuf = (unsigned short*)malloc(256);
	
	r2.left = oldx;
	r2.right = oldx +11;
	r2.top = oldy;
	r2.bottom = oldy +19;
	
	saveold(&r2,mousebuf);
	drawmouse(oldx,oldy,(unsigned short*)&__arrow);
	
	
	for(;;)
	{
		if(sys_KernelGetMessage(&NRMessageQueue,&Message))
		{
			switch(Message.message)
			{
			case WM_MOUSEMOVE:
				//画旧图(鼠标遮盖区域)
				
				
				if(draginfo == 1){
					mx = Message.lParam - dragx;
					my = Message.wParam - dragy;
					
					for( s = tail->previous; s ;s =s->previous){
						r3.left = s->x;r3.top = s->y;r3.right = s->right;r3.bottom = s->bottom;
						r4.left =tail->x;r4.top = tail->y;r4.right = tail->right;r4.bottom = tail->bottom;
						sys_Rect_Intersect(&s->rects,&r3,&r4);//假定这个函数可行，相交的区域是需要重画的部分
                        sys_writemap(&s->rects,(char *)s->map,1);
					}
					
					r1.left =tail->x + mx;
					r1.top = tail->y + my;
					r1.right = tail->right+ mx;
					r1.bottom =tail->bottom+ my;
					
					//++++++++++++++++++++++++++++++++++++++++
					
					sys_SaveWindowProperty(&r1,tail);
					
					BitBltEx(tail->x,tail->y,tail->right,tail->bottom,0,0,(unsigned short *)tail->buf);
					/************************************************************************/
					/*                   前台窗口和每个后台窗口相交  
					将相交的结果复制给相应的窗口RECT
					*/
					/************************************************************************/
					
					for( s = tail->previous; s ;s =s->previous){
						
						for(p = s->next ; p ; p = p->next){
							r3.left = s->x;r3.top = s->y;r3.right = s->right;r3.bottom = s->bottom;
							r4.left = p->x;r4.top = p->y;r4.right = p->right;r4.bottom = p->bottom;
							sys_Rect_Intersect(&r5,&r3,&r4);
							sys_writemap(&r5,(char *)s->map,0);
						}
						
						tmpbuf = (char *)s->map;
						
						for (i = s->rects.top; i< s->rects.bottom;i++)
						{
							for (j = s->rects.left;j < s->rects.right;j++)
							{
								if (tmpbuf[i*screen_width+j]==0x11)
								{
									//    drawpointtoscreen(j,i,getpointcolor(j-s->x,i-s->y,(unsigned short *)s->buf));
									pf[i*800+j]=((unsigned short*)s->buf)[(i-s->y)*800+(j-s->x)];
									j++;
									pf[i*800+j]=((unsigned short*)s->buf)[(i-s->y)*800+(j-s->x)];
								}
								
							}
						}
						
					}
					/********************************/
					
					dragy = Message.wParam;
					dragx = Message.lParam;
				}else
				{
					r2.left = oldx;
					r2.right = oldx +11;
					r2.top = oldy;
					r2.bottom = oldy +19;
					drawold(&r2,(unsigned short *)mousebuf);
					
					r2.left = Message.lParam;
					r2.right = Message.lParam +11;
					r2.top = Message.wParam;
					r2.bottom = Message.wParam +19;
					
					saveold(&r2,(unsigned short *)mousebuf);
				}
				drawmouse(Message.lParam,Message.wParam,(unsigned short*)&__arrow);
				oldy = Message.wParam;
				oldx = Message.lParam;
				
				
				break;
			case WM_LBUTTONDOWN:
				for(p=tail;p->previous;p=p->previous)
				{
					if(IsinRec(p->x,p->y,p->right,p->bottom,Message.lParam,Message.wParam))
					{
						if(sys_bring_window_to_top(p)){
							r1.bottom = tail->bottom;
							r1.top = tail->y;
							r1.left = tail->x;
							r1.right = tail->right;
							sys_SaveWindowProperty(&r1,tail);
							BitBltEx(tail->x,tail->y,tail->right,tail->bottom,0,0,(unsigned short *)tail->buf);
						}
						break;
					}
					
				}
				for(p=tail;p;p=p->previous)
				{
					if(IsinRec(p->x,p->y,p->right-22,p->y+18,Message.lParam,Message.wParam))
					{
						draginfo = 1;
						break;
					}
				}
				drawmouse(Message.lParam,Message.wParam,(unsigned short*)&__arrow);
				
				dragx = Message.lParam;
				dragy = Message.wParam;
				break;
			case WM_LBUTTONUP:
				draginfo = 0;
				break;
			case WM_RBUTTONDOWN:
				//	hanzi16(Message.lParam,Message.wParam,"右键按下",0x7c00);
				break;
			case WM_RBUTTONUP:
				break;
			case WM_LBUTTONDBLCLK:
				//	hanzi16(Message.lParam,Message.wParam,"双击事件",0x7c00);
				break;
			case WM_KEYDOWN:
				//case WM_KEYUP:
                KernelPutMessage(&(tail->winmsgqueue),&Message);
				for (q=&task[1];*q;q++)
				{
					if(&(*q)->twin == tail)
						wakeup(*q);
				}
				break;
			}
			//转发消息
			for(p=tail;p;p=p->previous)
			{
				if(IsinRec(p->x,p->y,p->right,p->bottom,Message.lParam,Message.wParam))
				{
					
					sys_KernelPutMessage(&(p->winmsgqueue),&Message);
					
					for (q=&task[1];*q;q++)
					{
						if(&(*q)->twin == p)
							wakeup(*q);
					}
					break;
				}
				
			}
			
		}else{
		}
		
	}
	
	
}

//Message.h消息函数
//取消息函数，， 
int sys_KernelGetMessage(struct KernelMessageQueue *kmq,MSG *km)
{
	
	if(kmq->head==kmq->tail) //队列为空 
	{
		sys_sleep();
		sys_schedule();
		return 0;
	}
	
	kmq->head++;
	//kmq->head=++kmq->head%10; //头指针向后移
	if(kmq->head > 10) kmq->head=1;
	
	*km=kmq->krmsgqueue[kmq->head];//取消息
	
	
	return 1;
	
}

int sys_KernelPutMessage(struct KernelMessageQueue *kmq,MSG *km)
{
	int tail=kmq->tail+1; //尾指针后移，未使用0 
	
	if(tail > 10) tail=1;
	
	if(tail==kmq->head) //队列已满 
	{
		return 0;
	}
	
	kmq->tail=tail;//加入到队列尾部 
	
	kmq->krmsgqueue[kmq->tail]=*km; // 放入第1个消息 
	
	return 1;
	
}
//初始化队列 
void  InitMessageQueue(struct KernelMessageQueue *kmq)
{
	kmq->head=0;
	kmq->tail=0;
	
}
void KeyboardPutMsg(int al){
	int makecode;
	MSG message;
	
	makecode = al &  0x80 ? 0 : 1;
	
	if (makecode)
	{
		message.message = WM_KEYDOWN ;
		message.lParam = al;
	}else{
        message.message = WM_KEYUP;
		message.lParam = al;
	}
	KernelPutMessage(&NRMessageQueue,&message);
	
	wakeup(task[1]);
	
}
//放消息函数,鼠标中断函数 
void MousePutMsg(int x,int y,int leftbutton,int rightbutton )
{
	MSG message;
	int dtime;
	
	if ((intmx == x) && (intmy == y))
	{
		if (oldlbutton ==1 && leftbutton == 0)
		{
			message.message = WM_LBUTTONUP;
			message.lParam = x;
			message.wParam = y;
		}
		else if(oldlbutton ==0 && leftbutton == 1)
		{
			message.message = WM_LBUTTONDOWN;
			message.lParam = x;
			message.wParam = y;
			
			nowtime = GetSystemTime();
			dtime = nowtime - oldtime;
			
			message.timeticks =  dtime; //消息的时间戳,用来记录2次鼠标消息产生的时差
			
			if (message.timeticks < 50 && message.timeticks > 0)
			{
				message.message = WM_LBUTTONDBLCLK;
				message.lParam = x;
				message.wParam = y;
			}
			oldtime = nowtime;
		}
		if (oldrbutton ==1 && rightbutton == 0)
		{
			message.message = WM_RBUTTONUP;
			message.lParam = x;
			message.wParam = y;
		}
		else if(oldrbutton==0 && rightbutton == 1)
		{
			message.message = WM_RBUTTONDOWN;
			message.lParam = x;
			message.wParam = y;
		}
		
		oldlbutton = leftbutton;
		oldrbutton = rightbutton;
	}
	else
	{
		message.message = WM_MOUSEMOVE;
		message.lParam = x;
		message.wParam = y;
		intmx = x;
		intmy = y;
	}
	
	KernelPutMessage(&NRMessageQueue,&message);
    
	wakeup(task[1]);
}


//hdddriver.h 读硬盘驱动 
//写端口
void  outputio(unsigned short port, unsigned char data )
{
	_asm{
		mov al,data
			mov dx,port
			out dx,al
	}
}
//读端口
char  inputio(unsigned short port)
{
	_asm{
		mov dx,port
			in al,dx
	}
}
/**/
//移动到了process.c里
/**/
//file.h   读文件
//大约等同于C语言的_open ,由于未实现内存管理，现使用内存都直接指定 \
//2010-1-19 文件名字符串出错，未知原因
//读取文件不得大于4M=8(fat)*128(cu)*8(簇大小)*512(扇区)
int openfile(const char *filename,char cmd, unsigned int *buffer)
{
	unsigned int i;
	unsigned char j;
	//date 1/15 add this code
	//fat32 一些数据结构
	bdata boot_data=(bdata)0x100000;
	fat32s fatindex=(fat32s)0x200000;
	unsigned int ReservedSector;
	unsigned int RootStartAddress;
	unsigned int FatStartAddress;
	unsigned int RootSize;
	unsigned int DataStartSector;
	unsigned int FatContent;
	char file1[11]={0,};
	char file2[11]={0,};
	//读第一分区第一扇区,0x100000并不是一个安全的地方
	lw_read(1,*((unsigned int *)0x7dc6),(unsigned int *)0x100000,0x20);
	//初始化init
	ReservedSector = (unsigned int)boot_data->ReservedSector;
	RootStartAddress = ReservedSector+boot_data->Hidesec+boot_data->SectorByFat*2;
	FatStartAddress = ReservedSector+boot_data->Hidesec;
	RootSize = (unsigned int)boot_data->SectorbyCu;
	DataStartSector = RootSize + RootStartAddress;
	j = (unsigned char)boot_data->SectorbyCu; //每簇多少扇区
	//read root index读根目录到2M这个地方
	lw_read((unsigned char)RootSize,RootStartAddress,(unsigned int *)0x200000,0x20);
	
	_asm nop;
	
	
	for(i=0;i<(RootSize*16) || fatindex->FileName[0] == 0;fatindex++,i++)
	{
		TranslateFilename(file1,fatindex->FileName);
		UpperString(file2,filename);
		if(strcmp(file1,file2)==0)
			break;
	} 
	if(i==RootSize*16)return 0;	
	
	_asm nop;
	//读取fat表，这儿只读了所有个扇区，已经足够了，我不知道是否要全部加载
	for (i=0;i<boot_data->SectorByFat;i=i+8)
	{
		lw_read(8,FatStartAddress+i,(unsigned int *)(0x210000+i*512),0x20);
	}
	
	
	
	//先读起始簇
	lw_read(j,DataStartSector+(fatindex->CuOffset-3)*j,buffer,0x20);
	buffer=buffer+128*j;
	//读取当前簇内容，是否为0xfffffff，如果是则结束。
	FatContent=*(unsigned int *)(fatindex->CuOffset*4+0x210000);
	if(FatContent == 0xfffffff) return 1;
	//读取下一簇
	lw_read(j,DataStartSector+(FatContent-3)*j,buffer,0x20);
	buffer=buffer+128*j;
	//循环读取，直至最后一簇
	while(1)
	{
		FatContent=*(unsigned int *)(FatContent*4+0x210000);
		if(FatContent==0x0fffffff)break;
		lw_read(j,DataStartSector+(FatContent-3)*j,buffer,0x20);
		buffer=buffer+128*j;
	}
	return 1;
}
void readpic()
{
	openfile("HZK16",0x20,(unsigned int *)0x300000);
}

void set_tssldt_desc(unsigned long n,unsigned long addr,char tp)
{
	_asm mov ebx,n
		_asm mov ax,104
		_asm mov word ptr [ebx],ax // 将TSS 长度放入描述符长度域(第0-1 字节)。
		_asm mov eax,addr
		_asm mov word ptr [ebx+2],ax // 将基地址的低字放入描述符第2-3 字节。
		_asm ror eax,16 // 将基地址高字移入ax 中。
		_asm mov byte ptr [ebx+4],al // 将基地址高字中低字节移入描述符第4 字节。
		_asm mov al,tp
		_asm mov byte ptr [ebx+5],al // 将标志类型字节移入描述符的第5 字节。
		_asm mov al,80h
		_asm mov byte ptr [ebx+6],al // 描述符的第6 字节置0。
		_asm mov byte ptr [ebx+7],ah // 将基地址高字中高字节移入描述符第7 字节。
		_asm ror eax,16 // eax 清零。
}

void set_tss_desc(unsigned long n,unsigned long addr){
	set_tssldt_desc(n,addr,(char)0xe9);
}

void set_ldt_desc(unsigned long n,unsigned long addr){
	set_tssldt_desc(n,addr,(char)0xe2);
}


BOOL IsinRec(int x1,int y1,int x2,int y2,int x,int y){
	if((x>x1&&x<x2)&&(y>y1&&y<y2))
		return true;
	return false;
}

void HandleDivideError(){
	//	hanzi16(400,300,"除法错误",0x7c00);
	//	_asm hlt;
}

void HandleSingleStepException(){
	hanzi16(400,300,"调试异常",0x7c00,(unsigned short *)video_address);
	_asm hlt;
}

void HandleNmi(){
	hanzi16(400,300,"非屏幕中断",0x7c00,(unsigned short *)video_address);
	_asm hlt;
}
void HandleBreakpointException(){
	hanzi16(400,300,"断点异常",0x7c00,(unsigned short *)video_address);
	_asm hlt;
}
//;int4溢出
void HandleOverflow(){
	hanzi16(400,300,"溢出",0x7c00,(unsigned short *)video_address);
	_asm hlt;
}
//			;int5越界
void HandleBoundsCheck(){
	hanzi16(400,300,"越界",0x7c00,(unsigned short *)video_address);
	_asm hlt;
}

//			   ;int6无效的操作码
void HandleInvalOpcode(){
	hanzi16(400,300,"无效的操作码",0x7c00,(unsigned short *)video_address);	
	_asm hlt;
}
//		   ;int7设备不可用（无数学协处理器）
void HandleCoprNotAvailable(){
	hanzi16(400,300,"无数学协处理器",0x7c00,(unsigned short *)video_address);
	_asm hlt;
}
//				;int8 双重错误
void HandleDoubleFault(){
	hanzi16(400,300,"双重错误",0x7c00,(unsigned short *)video_address);
	_asm hlt;
}
//			;int9 xie保留
void HandleCoprSegOverrun(){
	hanzi16(400,300,"保留",0x7c00,(unsigned short *)video_address);
	_asm hlt;
}

//			  ;int10 无效TSS
void HandleInvalTss(){
	hanzi16(400,300,"无效的TSS",0x7c00,(unsigned short *)video_address);
	_asm hlt;
}
//			  ;int11 段不存在
void HandleSegmentNotPresent(){
	hanzi16(400,300,"段不存在",0x7c00,(unsigned short *)video_address);
	_asm hlt;
}
//			  ;int12 堆栈段错误
void HandleStackException(){
	hanzi16(400,300,"堆栈段错误或溢出",0x7c00,(unsigned short *)video_address);
	_asm hlt;
}
//				 ;int13 常规保护错误
void HandleGeneralProtection(){
	hanzi16(400,300,"常规保护错误",0x7c00,(unsigned short *)video_address);
	_asm hlt;
}
//				;int14 页错误
void HandlePageFault(long address,long error){
	unsigned long *table;
	unsigned long dir;
	unsigned long mid;
	
	dir =*(unsigned long*)(((address>>20) & 0xffc)  + current->tss.cr3);
	mid = (address>>10) & 0xffc;
	table = (unsigned long*)((dir & 0xfffff000)+mid);
	
	if (error&1)
		un_wp_page(table);
	else
		put_page(get_free_page(),address,current->tss.cr3);	
}
//			   ;int15 保留
void HandleCoprError(){
	hanzi16(400,300,"保留",0x7c00,(unsigned short *)video_address);
	_asm hlt;
	
}


void init_interupt(){
	
	*(short*)&idt = (unsigned short)handle_divide_error;
	*((short*)&idt+3) = (unsigned short)((long)handle_divide_error>>16);
	
	*((short*)&idt+4) = (unsigned short)handle_single_step_exception;
	*((short*)&idt+7) = (unsigned short)((long)handle_single_step_exception>>16);
	
	*((short*)&idt+8) = (unsigned short)handle_nmi;
	*((short*)&idt+11) = (unsigned short)((long)handle_nmi>>16);
	
	*((short*)&idt+12) = (unsigned short)handle_breakpoint_exception;
	*((short*)&idt+15) = (unsigned short)((long)handle_breakpoint_exception>>16);
	
	*((short*)&idt+16) = (unsigned short)handle_overflow;
	*((short*)&idt+19) = (unsigned short)((long)handle_overflow>>16);
	
	*((short*)&idt+20) = (unsigned short)handle_bounds_check;
	*((short*)&idt+23) = (unsigned short)((long)handle_bounds_check>>16);
	
	*((short*)&idt+24) = (unsigned short)handle_inval_opcode;
	*((short*)&idt+27) = (unsigned short)((long)handle_inval_opcode>>16);
	
	*((short*)&idt+28) = (unsigned short)handle_copr_not_available;
	*((short*)&idt+31) = (unsigned short)((long)handle_copr_not_available>>16);
	
	*((short*)&idt+32) = (unsigned short)handle_double_fault;
	*((short*)&idt+35) = (unsigned short)((long)handle_double_fault>>16);
	
	*((short*)&idt+36) = (unsigned short)handle_copr_seg_overrun;
	*((short*)&idt+39) = (unsigned short)((long)handle_copr_seg_overrun>>16);
	
	*((short*)&idt+40) = (unsigned short)handle_inval_tss;
	*((short*)&idt+43) = (unsigned short)((long)handle_inval_tss>>16);
	
	*((short*)&idt+44) = (unsigned short)handle_segment_not_present;
	*((short*)&idt+47) = (unsigned short)((long)handle_segment_not_present>>16);
	
	*((short*)&idt+48) = (unsigned short)handle_stack_exception;
	*((short*)&idt+51) = (unsigned short)((long)handle_stack_exception>>16);
	
	*((short*)idt+52) = (unsigned short)handle_general_protection;
	*((short*)&idt+55) = (unsigned short)((long)handle_general_protection>>16);
	
	*((short*)&idt+56) = (unsigned short)handle_page_fault;
	*((short*)&idt+59) = (unsigned short)((long)handle_page_fault>>16);
	
	*((short*)&idt+60) = (unsigned short)handle_copr_error;
	*((short*)&idt+63) = (unsigned short)((long)handle_copr_error>>16);
	
	
}
int kitoa(int num,char* str){
	int i;
	int temp;
	char *str_const="0123456789ABCDEF";
	if(num == 0){
		str = "0";
	}
	for (i=0;i<8;i++){
		
		temp =(num >> (i*4)) & 0xf;
		str[7-i] = str_const[temp];
	}
	
	for (i=0;i<8;i++)
	{
		if(str[i] =='0')
			str[i] = ' ';
		else
			break;
	}
	str[8] = 0;
	return 1;
}

unsigned long *test(){
	
	unsigned long address = 0x12345678;
	
	unsigned long temp =*(unsigned long*)(((address>>20) & 0xffc)  + 0x800000);
	unsigned long tmp = (address>>10) & 0xffc;
	return (unsigned long*)((temp & 0xfffff000)+tmp) ;
	
}

int GetSystemTime(){
	return systemcount;
}

int sys_savemousexy(int x,int y,int * x1,int * y1){
	*x1 = x;
	*y1 = y;
	return 1;
}

void drawtaskbar(){
	drawrec(0,0,screen_width -  1, screen_height - 29,0x210,(unsigned short *)current->twin.buf);//COL8_008484
	drawrec(0,screen_height - 28, screen_width -  1, screen_height - 28,0x6318,(unsigned short *)current->twin.buf);//COL8_C6C6C6
	drawrec(0,screen_height - 27, screen_width -  1, screen_height - 27,0xffff,(unsigned short *)current->twin.buf);//COL8_FFFFFF
	drawrec(0,screen_height - 26, screen_width -  1, screen_height -  1,0x6318,(unsigned short *)current->twin.buf);//COL8_C6C6C6
	
	drawrec( 3, screen_height - 24, 59, screen_height - 24,0xffff,(unsigned short *)current->twin.buf);//COL8_FFFFFF
	drawrec( 2, screen_height - 24,  2, screen_height -  4,0xffff,(unsigned short *)current->twin.buf);//COL8_FFFFFF
	drawrec( 3, screen_height -  4, 59, screen_height -  4,0x4210,(unsigned short *)current->twin.buf);//COL8_848484
	drawrec( 59,screen_height - 23, 59, screen_height -  5,0x4210,(unsigned short *)current->twin.buf);//COL8_848484
	drawrec(  2,screen_height -  3, 59, screen_height -  3,0,(unsigned short *)current->twin.buf);//COL8_000000
	drawrec( 60,screen_height - 24, 60, screen_height -  3,0,(unsigned short *)current->twin.buf);//COL8_000000
	
	drawrec( screen_width - 100, screen_height - 24, screen_width -  4, screen_height - 24,0x4210,(unsigned short *)current->twin.buf);//COL8_848484
	drawrec( screen_width - 100, screen_height - 23, screen_width - 100, screen_height -  4,0x4210,(unsigned short *)current->twin.buf);//COL8_848484
	drawrec( screen_width - 100, screen_height -  3, screen_width -  4, screen_height -  3,0xffff,(unsigned short *)current->twin.buf);//COL8_FFFFFF
	drawrec( screen_width -  3, screen_height - 24, screen_width -  3, screen_height -  3,0xffff,(unsigned short *)current->twin.buf);//COL8_FFFFFF
	
	hanzi16(15,screen_height - 21,"开始",0,(unsigned short *)current->twin.buf);
	hanzi16(16,screen_height - 21,"开始",0,(unsigned short *)current->twin.buf);
	hanzi16(screen_width - 97,screen_height - 21,"12：00",0,(unsigned short *)current->twin.buf);
}

int sys_Rect_Intersect(RECT* pdrc, const RECT* psrc1, const RECT* psrc2)
{  /*求两个RECT的相交的部分，也是一个RECT*/
    pdrc->left = (psrc1->left > psrc2->left) ? psrc1->left : psrc2->left;
    pdrc->top  = (psrc1->top > psrc2->top) ? psrc1->top : psrc2->top;
    pdrc->right = (psrc1->right < psrc2->right) ? psrc1->right : psrc2->right;
    pdrc->bottom = (psrc1->bottom < psrc2->bottom)? psrc1->bottom : psrc2->bottom;
    if(pdrc->left >= pdrc->right || pdrc->top >= pdrc->bottom)
        return FALSE;
    return TRUE;
}

int sys_bring_window_to_top(struct task_window *p){
	
	struct task_window *tmp; 
	//由于head，tail是全局变量，属于内核，所以要内陷到内核态去保存
	if( p!=tail){
		//让p指向下一节点
		p->previous->next = p->next;
		p->next->previous = p->previous;
		
		tail->next = p;
        tmp = tail;
		
		tail = p;	
		
		tail->previous = tmp;
		tail->next = 0;	
		
		return 1;
	}
	
	return 0;
}

int sys_writemap(PRECT r,char * bmp,int b){
	
	int p_offset,x,y,dx,dy;
	unsigned long q;
	unsigned long * p;
    char * tmp = bmp;
	
	dx = (r->right - r->left) / 4;
	dy = (r->right - r->left) % 4;
	
	for ( y = r->top;y < r->bottom; y++)
	{
		for( x = 0;x < dx; x++){
			
			p_offset = (y<<9) + (y<<5) + (y<<8) + x * 4 + r->left;
			
			q = (unsigned long)tmp + p_offset;
			
			p = (unsigned long *) q;
			if(b == 1)
				*p = 0x11111111;
			else
				*p = 0;
		}
	}
	
	for ( y = r->top;y < r->bottom; y++)
	{
		for( x = 0;x < dy; x++){
			
			p_offset = (y<<9) + (y<<5) + (y<<8) + x+ r->left + dx * 4;
			if(b==1)
				*(tmp + p_offset) = 0x11;
			else
				*(tmp + p_offset) = 0;
			
		}
	}
	return 0;
	
}

char * TranslateFilename(char * newname,const char *oldname){
	int i;
	char ext[3];
	for (i = 10; i >= 8; i-- )
		ext[i-8] = oldname[i];
	
	for (i = 2 ;i >= 0; i-- ){
		if(ext[i] != 0x20)
			break;
		ext[i] = '\0';
	}
	ext[3] = '\0';
	
	for (i = 7; i >= 0; i-- )
		newname[i] = oldname[i];
	
	for (i = 7; i >= 0; i-- ){
		if(newname[i] != 0x20)
			break;
		newname[i] = '\0';
	}
	
	newname[8] = '\0';
	if(ext[0]|ext[1]|ext[2]){
		strcat(newname,".");
		strcat(newname,ext);
	}
	
	return newname;
}

int UpperString(char * nname,char * oname){
	for(;*oname;nname++,oname++)
		*nname = (*oname >= 97 && *oname <= 122) ? *oname - 0x20 : *oname;
	return 0;
}

//第2种实现窗口重叠的方法，矩形剪切

void rectclip(PRECT src,PRECT des){

	 PRECT rec;
	 RECT tmp;
	 tmp.bottom = src->bottom;
	 tmp.left = src->left;
	 tmp.top = src->top;
	 tmp.right = src->right;

     if(des->top > tmp.top)
	 {
		 rec =(PRECT)malloc(sizeof(struct tagRECT));
		 
		 rec->left  = tmp.left;
		 rec->top   = tmp.top;
		 rec->right = tmp.right;
		 rec->bottom = des->top;
		 tmp.top = des->top;
		 //加入链表
	 }
	 if(des->bottom < tmp.bottom)
	 {
		 rec =(PRECT)malloc(sizeof(struct tagRECT));
		 rec->top  = des->bottom;
		 rec->left   = tmp.left;
		 rec->right = tmp.right;
		 rec->bottom = tmp.bottom;
		 tmp.bottom = des->bottom;
		 //加入链表
	 }
	 if(des->left > tmp.left)
	 {
		 rec =(PRECT)malloc(sizeof(struct tagRECT));
		 rec->left  = tmp.left;
		 rec->top   = tmp.top;
		 rec->right = des->left;
		 rec->bottom = tmp.bottom;
		 //加入链表
	 }
	 if(des->right < tmp.right)
	 {
		 rec =(PRECT)malloc(sizeof(struct tagRECT));
		 rec->left  = des->right;
		 rec->top   = tmp.top;
		 rec->right = tmp.right;
		 rec->bottom = tmp.bottom;
		 //加入链表
	 }

}

