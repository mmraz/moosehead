CC      = gcc
PROF    = -O2 -ggdb
OLC	= -DGAME_VERSION -DROM
C_FLAGS =  -Dmoose -Wall $(PROF) $(OLC) -I/usr/local/include
L_FLAGS =  -lm -lcrypt -lpthread -ldl -latomic_ops -lgc

O_FILES = act_comm.o act_enter.o act_info.o act_move.o act_obj.o\
	  act_wiz.o alias.o ban.o comm.o const.o db.o db2.o dns.o\
	  db3.o deity.o editor.o effects.o fight.o flags.o gladiator.o\
	  handler.o healer.o ident.o interp.o lookup.o macro.o\
	  mag2.o magic.o menu.o note.o olc.o recycle.o remort.o\
	  save.o scan.o skills.o special.o tables.o trade.o update.o \
	  live_edit.o clan.o
#        imc.o imc-interp.o imc-version.o imc-mercbase.o imc-mail.o \
#        imc-util.o imc-events.o imc-config.o icec.o icec-mercbase.o ice.o gc.a

moose: $(O_FILES)
	rm -f moose
	$(CC) -o moose $(O_FILES) $(L_FLAGS) 
#	/usr/local/bin/super chown_moose moose
#	/usr/local/bin/super suid_moose moose

.c.o: merc.h
	$(CC) -c $(C_FLAGS) $<
