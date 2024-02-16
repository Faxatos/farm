CC			=  gcc
AR          =  ar
CFLAGS	    += -std=c99 -Wall -Werror -g
ARFLAGS     =  rvs
INCDIR      = ./utils/includes -I ./utils/concurrent_queue -I ./utils/sorted_list -I ./utils/dynamic_array
INCLUDES	= -I. -I $(INCDIR)
LDFLAGS 	= -L.
OPTFLAGS	= -O3
LIBS        = -lpthread -lm
TESTFILES	:= test.sh

TARGETS		= farm generafile

.PHONY: all farm brokenfarm collector generafile clean cleantests cleanall
.SUFFIXES: .c .h

%.o: %.c
	@$(CC) $(CFLAGS) $(INCLUDES) $(OPTFLAGS) -c -o $@ $<

all: $(TARGETS)

farm: ./src/farm.o ./src/master.o ./src/worker.o ./src/collector.o ./utils/concurrent_queue/libBQueue.a ./utils/dynamic_array/libDArray.a ./utils/sorted_list/libSList.a
	$(CC) $(CFLAGS) $(INCLUDES) $(OPTFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)

collector: ./src/collector.o ./utils/sorted_list/libSList.a
	@$(CC) $(CFLAGS) $(INCLUDES) $(OPTFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)

brokenfarm: ./src/farm.o ./src/master.o ./src/broken_worker.o ./src/collector.o ./utils/concurrent_queue/libBQueue.a ./utils/dynamic_array/libDArray.a ./utils/sorted_list/libSList.a
	$(CC) $(CFLAGS) $(INCLUDES) $(OPTFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)

./utils/concurrent_queue/libBQueue.a: ./utils/concurrent_queue/conc_queue.o ./utils/concurrent_queue/conc_queue.h
	@$(AR) $(ARFLAGS) $@ $<

./utils/sorted_list/libSList.a: ./utils/sorted_list/sor_list.o ./utils/sorted_list/sor_list.h
	@$(AR) $(ARFLAGS) $@ $<

./utils/dynamic_array/libDArray.a: ./utils/dynamic_array/dyn_array.o ./utils/dynamic_array/dyn_array.h
	@$(AR) $(ARFLAGS) $@ $<

./src/broken_worker.o: ./src/worker.c 
	@$(CC) -D RETURN_AFTER_ONE_TASK $(CFLAGS) $(INCLUDES) $(OPTFLAGS) -c -o $@ $<
./src/farm.o: ./src/farm.c 
./src/master.o: ./src/master.c 
./src/worker.o: ./src/worker.c 
./src/collector.o: ./src/collector.c 

./utils/concurrent_queue/conc_queue.o: ./utils/concurrent_queue/conc_queue.c
./utils/sorted_list/sor_list.o: ./utils/sorted_list/sor_list.c
./utils/dynamic_array/dyn_array.o: ./utils/dynamic_array/dyn_array.c

generafile 	: 
	@$(CC) $(CFLAGS) ./src/generafile.c -o $@ 
clean		: 
	@\rm -f *.o *~ *.a src/*.o utils/*.o utils/*~ utils/concurrent_queue/*.o utils/concurrent_queue/*.a utils/dynamic_array/*.o utils/dynamic_array/*.a utils/sorted_list/*.o utils/sorted_list/*.a generafile farm collector brokenfarm
cleantests	: 
	@\rm -f *.dat *.txt
	@\rm -r testdir; 
cleanall	: clean cleantests
test		:
	@chmod +x ./$(TESTFILES)
	@./$(TESTFILES)



