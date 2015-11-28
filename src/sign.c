
#include <stdio.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <netdb.h>
#include <unistd.h>
#include <netinet/in.h>
static char rcsid[] = "$Id: sign.c,v 1.2 1999/07/14 18:14:09 mud Exp $";

#define PORT 4000
#define MESSAGE "\n\n\n Go to thesled.us 4000 - we'll try a second migration tonight \n\n\n"

int main(int argc, char **argv)
{
	struct sockaddr_in
		listen_struct,
		accept_struct;

	fd_set
		readmask;

	int
		listener,
		acceptor,
		size,
		on;


	if((listener = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("Couldn't create client socket:");
		exit(-1);
	}

	on = 1;
	size = sizeof(struct sockaddr_in);
	setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on));

	listen_struct.sin_family = AF_INET;
	listen_struct.sin_addr.s_addr = INADDR_ANY;
	listen_struct.sin_port = htons(PORT);

	if(bind(listener, (struct sockaddr *)&listen_struct, size) < 0)
	{
		perror("Couldn't bind to client socket:");
		exit(-1);
	}

	if(listen(listener, 5) < 0)
	{
		perror("Couldn't listen on the client socket:");
		exit(-1);
	}

	switch(fork())
	{
		case -1:
			perror("Fork failed:");
			exit(-1);
		case 0:
			break;
		default:
			exit(0);
	}

	fclose(stdin);
	fclose(stdout);
	fclose(stderr);

	while(1 == 1)
	{
		FD_ZERO(&readmask);
		FD_SET(listener, &readmask);

		if(select(FD_SETSIZE, &readmask, 0, 0, 0) < 0)
		{
			continue;
		}

		if(FD_ISSET(listener, &readmask))
		{
			acceptor = accept(listener, (struct sockaddr *)&accept_struct, &size);

			write(acceptor, MESSAGE, strlen(MESSAGE));
			sleep(10);
			close(acceptor);
		}
	}
}
