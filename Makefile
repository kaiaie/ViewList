RM = DEL
SRC = ViewList.c MainWindow.c Utils.c
RC = ViewListResources.rc
OBJ = $(SRC:.c=.o) $(RC:.rc=.o)
OPT = -Wall -Werror -pedantic -O2
EXE = ViewList.exe

all: $(EXE)

$(EXE): $(OBJ)
	gcc -o $@ $(OBJ) -mwindows

-include $(SRC:.c=.d)

%.d: %.rc
	gcc -MM -MG $< > $@

%.d: %.c
	gcc -MM -MG $< > $@

%.o: %.c
	gcc $(OPT) -c $< -o $@
	
%.o: %.rc
	windres -i $< -o $@

clean:
	CMD /C IF EXIST $(EXE) $(RM) $(EXE)
	CMD /C IF EXIST *.o $(RM) *.o
	CMD /C IF EXIST *.d $(RM) *.d

.PHONY: all clean
