/* integer math */

int powi(int base, int power)
{
    int result = 1;
    while (power)
    {
        if (power & 1)
            result *= base;
        power >>= 1;
        base *= base;
    }
    return result;
}

int log2i(int x)
{
    int result = 0;
    while (x >>= 1) result++;
    return result;
}

int log10i(int x)
{
    int result = 0;
    while(x /= 10) result++;
    return result;
}
