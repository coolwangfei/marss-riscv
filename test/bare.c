#define SS() asm volatile ("csrs 0x800, zero")
#define SE() asm volatile ("csrs 0x801, zero")

__attribute__((naked))int main(void)
{

    asm volatile("auipc sp, %0"::"i"(0x1000));
    SS();
    int a = 0;
    int b = 0;
    a++;
    b++;
    int c = a + b;
    SE();
    return c;
}

