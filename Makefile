CC=gcc
LIBSOCKET=-lnsl
CCFLAGS=-Wall -g
SRV=server
CLT=client

build: $(SRV) $(CLT)

$(SRV):$(SRV).c
	$(CC) -o $(SRV) $(CCFLAGS) $(LIBSOCKET) $(SRV).c

$(CLT):	$(CLT).c
	$(CC) -o $(CLT) $(CCFLAGS) $(LIBSOCKET) $(CLT).c

clean:
	rm -f *.o *~
	rm -f $(SRV) $(CLT)
	rm *.log

