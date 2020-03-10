CFLAGS = -Wall -O2

all: fibonacci

# Control the build verbosity
ifeq ("$(VERBOSE)","1")
    Q :=
    VECHO = @true
else
    Q := @
    VECHO = @printf
endif

OBJS := \
	fibonacci.o \
	bignum.o \
	apm.o \
	sqr.o \
	mul.o \
	format.o
deps := $(OBJS:%.o=.%.o.d)

fibonacci: $(OBJS)
	$(VECHO) "  LD\t$@\n"
	$(Q)$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c
	@mkdir -p .$(DUT_DIR)
	$(VECHO) "  CC\t$@\n"
	$(Q)$(CC) -o $@ $(CFLAGS) -c -MMD -MF .$@.d $<

clean:
	rm -f $(OBJS) $(deps)
	$(RM) fibonacci

-include $(deps)
