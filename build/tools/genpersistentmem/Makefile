
all : $(CURDIR)/genpersistentmem $(CURDIR)/dbgpersistentmem

genpersistentmem_src = \
	$(CURDIR)/main.c

dbgpersistentmem_src = \
	$(CURDIR)/main-debug.c

$(CURDIR)/genpersistentmem : $(genpersistentmem_src)
	gcc -s \
	$^ \
	-Wall -Wno-uninitialized -Os -std=gnu99 \
	-I$(CURDIR)/../../../SOURCE/linux-drivers/include/uapi \
	-I$(CURDIR)/../../../components/kernel/source/include/uapi \
	-o $@

$(CURDIR)/dbgpersistentmem : $(dbgpersistentmem_src)
	gcc -s \
	$^ \
	-Wall -Wno-uninitialized -Os -std=gnu99 \
	-I$(CURDIR)/../../../SOURCE/linux-drivers/include/uapi \
	-I$(CURDIR)/../../../components/kernel/source/include/uapi \
	-o $@

.PHONY : clean
clean:
	-rm *.o *.mod
	-rm $(CURDIR)/genpersistentmem
	-rm $(CURDIR)/dbgpersistentmem
