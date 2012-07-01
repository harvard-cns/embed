#
F77= g77
#
OPTSCLD=  -O
DIRPPRN= /n/fs/grad/minlanyu/embed/mflow


.f.o:
	$(F77) -c $(OPTSCLD)  $<

dimacs2pprn:	dimacs2pprn.o
		$(F77) -o dimacs2pprn $(OPTSCLD) dimacs2pprn.o 

lintest:	lintest.o
		$(F77) -o lintest $(OPTSCLD) lintest.o -L$(DIRPPRN) -lpprn

nolintest:	nolintest.o
		$(F77) -o nolintest $(OPTSCLD) nolintest.o -L$(DIRPPRN) -lpprn


