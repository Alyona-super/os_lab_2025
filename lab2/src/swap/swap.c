#include "swap.h"

void Swap(char *left, char *right)
{
	char ch_new = *right;
	*right = *left;
	*left = ch_new;
}
