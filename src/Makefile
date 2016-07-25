target = store_problem

objects = main.o

CFLAGS = -Wall -ansi -pedantic -g

LDLIBS = -pthread

all : $(target) reloc

$(target) : $(objects)
		$(CC) $(LDLIBS) -o $@ $^

reloc :
		mkdir -p ../build
		mv $(target) ../build

$(objects) : %.o : %.c
		$(CC) -c $(CFLAGS) $<

clean :
		rm -f *.o $(target) ../build

.PHONY : clean reloc
