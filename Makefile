include $(MACHBASE_HOME)/install/machbase_env.mk
include ./Makefile.base

INCLUDES += $(INC_OPT)/$(MACHBASE_HOME)/include
LD_LIBS += ./lib/librdkafka.a  -lsasl2 -lssl -lcrypto  -lz ./lib/libzlog.a
CFLAGS += -I../src

all: kafka_to_machbase

kafka_to_machbase: lib/librdkafka.a kafka_to_machbase.o machbase_lib.o lib/libzlog.a
	$(LD_CC) $(LD_FLAGS) $(LD_OUT_OPT)$@ $< kafka_to_machbase.o machbase_lib.o $(LIB_OPT)machbasecli$(LIB_AFT)  $(LIBDIR_OPT)$(MACHBASE_HOME)/lib $(LD_LIBS)

kafka_to_machbase.o: kafka_to_machbase.c lib/libzlog.a lib/librdkafka.a
	$(CC) $(INCLUDES) $(CFLAGS) -g -c kafka_to_machbase.c -o $@ 

machbase_lib.o: machbase_lib.c lib/libzlog.a lib/librdkafka.a
	$(CC) $(INCLUDES) $(CFLAGS) -g -c machbase_lib.c -o $@ 

lib/libzlog.a:
	cd zlog-* && $(MAKE) 
	cp zlog-*/src/libzlog.a lib/
	cp zlog-*/src/zlog.h include

lib/librdkafka.a:
	cd librdkafka && ./configure
	cd librdkafka && $(MAKE)
	cp librdkafka/src/librdkafka.a lib/
	cp librdkafka/src/rdkafka.h include/

clean:
	cd lib && rm -f *
	cd include && rm -f *
	cd zlog-* && $(MAKE) $@
	cd librdkafka && $(MAKE) $@
	rm -f kafka_to_machbase *.o
