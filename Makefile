CFLAGS=-Wall -g -pthread
LDFLAGS=-pthread

OBJS=main.o map.o agent.o connection.o pheromone.o

brmlife: $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^


clean:
	rm -f $(OBJS) brmlife


DEP_FILES_1 = $(foreach src,$(OBJS),.deps/$(src))
DEP_FILES = $(DEP_FILES_1:%.o=%.P)

DEPS_MAGIC := $(shell mkdir .deps > /dev/null 2>&1 || :)

ifdef DEP_FILES
-include $(DEP_FILES)
endif

%.o: %.cc
	$(CXX) $(CFLAGS) -Wp,-MD,.deps/$(*F).pp -c $<
	@-cp .deps/$(*F).pp .deps/$(*F).P; \
		tr ' ' '\012' < .deps/$(*F).pp \
			| sed -e 's/^\\$$//' -e '/^$$/ d' -e '/:$$/ d' -e 's/$$/ :/' \
			>> .deps/$(*F).P; \
		rm .deps/$(*F).pp
