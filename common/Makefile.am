AM_CPPFLAGS          = -I$(top_srcdir)/util -I$(top_srcdir)/include

lib_LIBRARIES        = libcommon.a

include_HEADERS      = lcm.h hierarchy.h flat_kb.h serializable.h list.h mv.h \
                       priority_queue.h qsort.h queue.h hash.h \
                       sorted_int_list.h tv.h obdd.h mdd.h variable.h \
                       fwrite.h fread.h fscan.h stack.h types.h fprint.h \
                       array.h

libcommon_a_SOURCES  = lcm.h lcm.c \
                       hierarchy.c hierarchy.h \
                       flat_kb.c flat_kb.h \
                       serializable.c serializable.h \
                       list.c list.h \
                       mv.c mv.h \
                       tv.c tv.h \
                       obdd.c obdd.h \
                       mdd.c mdd.h \
                       variable.c variable.h \
                       priority_queue.c priority_queue.h \
                       qsort.c qsort.h \
                       queue.c queue.h \
                       array.c array.h \
                       hash.c hash.h \
                       sorted_int_list.c sorted_int_list.h \
                       fwrite.c fwrite.h \
                       fread.c fread.h \
                       fscan.c fscan.h \
                       fprint.c fprint.h \
                       stack.c stack.h \
                       md5.c md5.h \
                       types.c types.h

TEMPLATES            = lcm.ct lcm.ht hierarchy.ct hierarchy.ht \
                       flat_kb.ct flat_kb.ht serializable.ct serializable.ht \
                       list.ct list.ht mv.ct mv.ht tv.ct tv.ht \
                       obdd.ct obdd.ht mdd.ct mdd.ht variable.ct variable.ht
DSFILES              = common.ds list.ds
TFILES               = lcmconf.t hierarchyconf.t flat_kbconf.t \
                       serializableconf.t listconf.t mvconf.t tvconf.t \
                       variableconf.t obddconf.t mddconf.t
EXTRA_DIST           = $(TEMPLATES) $(DSFILES) $(TFILES)
MAINTAINERCLEANFILES = lcm.c lcm.h hierarchy.c hierarchy.h \
                       flat_kb.c flat_kb.h serializable.c serializable.h \
                       list.c list.h mv.c mv.h tv.c tv.h  obdd.c obdd.h \
                       mdd.c mdd.h variable.c variable.h

serializable.h: serializable.ht common.ds serializableconf.t
	$(TM) -I$(srcdir) -I$(top_srcdir)/include common.ds serializable.ht > serializable.h

serializable.c: serializable.ct common.ds serializableconf.t
	$(TM) -I$(srcdir) -I$(top_srcdir)/include common.ds serializable.ct > serializable.c

variable.h: variable.ht common.ds variableconf.t
	$(TM) -I$(srcdir) -I$(top_srcdir)/include common.ds variable.ht > variable.h

variable.c: variable.ct common.ds variableconf.t
	$(TM) -I$(srcdir) -I$(top_srcdir)/include common.ds variable.ct > variable.c

hierarchy.h: hierarchy.ht common.ds hierarchyconf.t
	$(TM) -I$(srcdir) -I$(top_srcdir)/include common.ds hierarchy.ht > hierarchy.h

hierarchy.c: hierarchy.ct common.ds hierarchyconf.t
	$(TM) -I$(srcdir) -I$(top_srcdir)/include common.ds hierarchy.ct > hierarchy.c

flat_kb.h: flat_kb.ht common.ds flat_kbconf.t
	$(TM) -I$(srcdir) -I$(top_srcdir)/include common.ds flat_kb.ht > flat_kb.h

flat_kb.c: flat_kb.ct common.ds flat_kbconf.t
	$(TM) -I$(srcdir) -I$(top_srcdir)/include common.ds flat_kb.ct > flat_kb.c

lcm.h: lcm.ht common.ds lcmconf.t
	$(TM) -I$(srcdir) -I$(top_srcdir)/include common.ds lcm.ht > lcm.h

lcm.c: lcm.ct common.ds lcmconf.t
	$(TM) -I$(srcdir) -I$(top_srcdir)/include common.ds lcm.ct > lcm.c

mv.h: mv.ht common.ds mvconf.t
	$(TM) -I$(srcdir) -I$(top_srcdir)/include common.ds mv.ht > mv.h

mv.c: mv.ct common.ds mvconf.t
	$(TM) -I$(srcdir) -I$(top_srcdir)/include common.ds mv.ct > mv.c

tv.h: tv.ht common.ds tvconf.t
	$(TM) -I$(srcdir) -I$(top_srcdir)/include common.ds tv.ht > tv.h

tv.c: tv.ct common.ds tvconf.t
	$(TM) -I$(srcdir) -I$(top_srcdir)/include common.ds tv.ct > tv.c

obdd.h: obdd.ht common.ds obddconf.t
	$(TM) -I$(srcdir) -I$(top_srcdir)/include common.ds obdd.ht > obdd.h

obdd.c: obdd.ct common.ds obddconf.t
	$(TM) -I$(srcdir) -I$(top_srcdir)/include common.ds obdd.ct > obdd.c

mdd.h: mdd.ht common.ds mddconf.t
	$(TM) -I$(srcdir) -I$(top_srcdir)/include common.ds mdd.ht > mdd.h

mdd.c: mdd.ct common.ds mddconf.t
	$(TM) -I$(srcdir) -I$(top_srcdir)/include common.ds mdd.ct > mdd.c

list.h: list.ht list.ds listconf.t
	$(TM) -I$(srcdir) -I$(top_srcdir)/include list.ds list.ht > list.h

list.c: list.ct list.ds listconf.t
	$(TM) -I$(srcdir) -I$(top_srcdir)/include list.ds list.ct > list.c

lcm.c: lcm.h hierarchy.h flat_kb.h serializable.h list.h variable.h
hierarchy.c: lcm.h hierarchy.h flat_kb.h serializable.h list.h mv.h tv.h obdd.h mdd.h variable.h
serializable.c: lcm.h hierarchy.h flat_kb.h serializable.h list.h mv.h tv.h obdd.h mdd.h variable.h
list.c: list.h
mv.c: hierarchy.h flat_kb.h serializable.h list.h mv.h
tv.c: hierarchy.h flat_kb.h serializable.h list.h tv.h
obdd.c: hierarchy.h flat_kb.h serializable.h list.h obdd.h variable.h
mdd.c: hierarchy.h flat_kb.h serializable.h list.h mdd.h variable.h
variable.c: lcm.h hierarchy.h flat_kb.h serializable.h list.h mv.h tv.h obdd.h mdd.h variable.h

include $(top_srcdir)/local.mk
