#define SQUARE(x) ((x)*(x))
#define N 4
#ifdef N
int a = SQUARE(N);
#else
int a;
#endif
#if (N > 2) && defined(SQUARE)
ok __LINE__ __FILE__
#endif
#undef N
