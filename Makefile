CFLAGS=-Wall -Werror -pipe
LDFLAGS=-lm
LD=gcc
DEPFLAGS=$(CPPFLAGS) $(CFLAGS) -MM
MAKEDEPEND=$(CC) $(DEPFLAGS) -o $*.d $<

SOURCES = $(wildcard *.c)

decode-dimm: $(SOURCES:.c=.o)
	$(LD) $(LDFLAGS) -o decode-dimm $(SOURCES:.c=.o)

clean:
	rm -f *.o *.d decode-dimm

%.o: %.c
	@$(MAKEDEPEND)
	$(COMPILE.c) -o $@ $<

%.d: %.c
	$(MAKEDEPEND)

-include $(SOURCES:.c=.d)
