C = gcc
#OPT = -O3
#OPT = -g
#WARN = -Wall
#LIB = mythread.a 

CFLAGS = $(OPT) $(INC) $(LIB)

# List all your .cc files here (source files, excluding header files)
SIM_SRC = p4.c


# List corresponding compiled object files here (.o files)
SIM_OBJ = p4.o
 
#################################

# default rule

all: p4


# rule for making sim_cache

p4 : $(SIM_OBJ)
	$(CC) -o p4 $(CFLAGS) $(SIM_OBJ) -lpthread 


# generic rule for converting any .cc file to any .o file
 
.cc.o:
	$(CC) $(CFLAGS)  -c $*.cc


# type "make clean" to remove all .o files plus the sim_cache binary

clean:
	rm -f *.o p4 list* endpoints bfile


# type "make clobber" to remove all .o files (leaves sim_cache binary)

clobber:
	rm -f *.o



