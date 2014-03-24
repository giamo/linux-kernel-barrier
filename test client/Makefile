all: test sleeper awaker releaser
	
test: test.c
	gcc -Wall -o test test.c barrier_client.h -lpthread

sleeper: sleeper.c
	gcc -Wall -o sleeper sleeper.c barrier_client.h

awaker: awaker.c
	gcc -Wall -o awaker awaker.c barrier_client.h

releaser: releaser.c
	gcc -Wall -o releaser releaser.c barrier_client.h
