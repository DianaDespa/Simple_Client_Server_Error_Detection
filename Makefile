all: send recv 

send: Sender.o link_emulator/lib.o
	gcc -g Sender.o link_emulator/lib.o -o send

recv: Receiver.o link_emulator/lib.o
	gcc -g Receiver.o link_emulator/lib.o -o recv

.c.o: 
	gcc -Wall -g -c $? 

clean:
	rm -f *.o send recv 
