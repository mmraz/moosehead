#include <stdio.h>
#include <math.h>
#include <time.h>

int main()
{
int foo=50;
int percent=50;

srandom(time(NULL));

  while(foo != 0)
  {
	printf("%d\n",foo);
	foo = remainder((random()>>6),100) + 50;
  }

return;

while(percent != 0)
{
  printf("%d\n",1+percent);
  while ( (percent = (random()>>6) & (128-1) ) > 99 );
}

}

