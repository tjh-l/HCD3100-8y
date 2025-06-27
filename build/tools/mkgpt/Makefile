
mkgpt_src = \
	$(CURDIR)/mkgpt.c

$(CURDIR)/mkgpt : $(mkgpt_src)
	gcc -s \
	$^ \
	-Wall -Wno-uninitialized -Os -std=gnu99 \
	-o $@

all : $(CURDIR)/mkgpt

.PHONY : clean
clean:
	-rm *.o *.mod
	-rm $(CURDIR)/mkgpt
