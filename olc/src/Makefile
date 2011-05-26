CC      = gcc
PROF    = -O2 -ggdb -DOLC_VERSION -DROM -DCODE_TEST
NOCRYPT =
C_FLAGS =  -Dmoose -Wall $(PROF) $(NOCRYPT)
L_FLAGS =  -Dmoose $(PROF) -lm

O_FILES = act_comm.o act_enter.o act_info.o act_move.o act_obj.o act_wiz.o \
          alias.o ban.o comm.o const.o db.o db2.o db3.o effects.o trade.o \
          flags.o fight.o gladiator.o handler.o healer.o interp.o note.o lookup.o magic.o \
	  mag2.o menu.o recycle.o save.o scan.o skills.o special.o tables.o\
	  update.o olc.o editor.o macro.o ident.o remort.o deity.o dns.o\
	  ../../gc/gc.a

moose: $(O_FILES)
	rm -f moose ../moose
	$(CC) $(L_FLAGS) -o moose $(O_FILES)
	cp moose ../moose

.c.o: merc.h
	$(CC) -c $(C_FLAGS) $<
