// uses less memory than the one in libc.a
char* memset(char* dest, char val, int n)
{
	int i;
	for(i = 0; i < n; i++)
		*dest++ = val;
	return dest;
}
