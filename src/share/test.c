#include <stdio.h>
#define SIM_START() asm("csrs 0x800,zero")
#define SIM_STOP() asm("csrs  0x801,zero")

int main()
{
	printf("Hello, World!\n");
    int a,b,c;
    SIM_START();
    a=1;
    b=2;
    c=a+b;
    SIM_STOP();
	return 0;
}
