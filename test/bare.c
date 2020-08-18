#define SS() asm volatile ("csrs 0x800, zero")
#define SE() asm volatile ("csrs 0x801, zero")

int main(void)
{

    SS();
    int a = 0;
    int b = 0;
    a++;
    b++;
    int c = a + b;
    SE();
    return c;
}

