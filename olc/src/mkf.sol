CC =  gcc
C_FLAGS =  -Dmoose -Wall -DOLD_RAND -DSYSV $(PROF) $(NOCRYPT)
L_FLAGS =  -Dmoose $(PROF)

O_FILES = act_comm.o act_enter.o act_info.o act_move.o act_obj.o act_wiz.o \
          alias.o ban.o comm.o const.o convert.o db.o db2.o db3.o effects.o \
          flags.o fight.o handler.o healer.o interp.o note.o lookup.o magic.o \
	  music.o mag2.o menu.o recycle.o save.o scan.o skills.o special.o \
	  tables.o update.o olc.o editor.o macro.o ident.o remort.o

moose: $(O_FILES)
	rm -f moose
	$(CC) $(L_FLAGS) -o moose $(O_FILES)

.c.o: merc.h
	$(CC) -c $(C_FLAGS) $<
