EXEC=rastertodpl
CFLAGS = -O3 $(shell cups-config --cflags)
LDFLAGS = $(shell cups-config --ldflags)
LDFLAGS += -DDEBUG
LDLIBS = $(shell cups-config --image --libs)
CUPSDATADIR = $(shell cups-config --datadir)
PPDPATH=${CUPSDATADIR}/model
CUPSDIR = $(shell cups-config --serverbin)

all: $(EXEC) ppd

.PHONY: ppd clean install uninstall

$(EXEC): rastertodpl.c

ppd:
	ppdc dpl.drv

install: all
	touch /home/theo/log.txt
	chmod 777 /home/theo/log.txt
	install -s $(EXEC) $(CUPSDIR)/filter/
	install -m 644 ppd/*.ppd $(PPDPATH)/datamax/
	

uninstall:
	rm -f $(CUPSDIR)/filter/$(EXEC)
	rm -f $(PPDPATH)/datamax/*.ppd


clean:
	rm -f $(EXEC)
	rm -f ppd/*.ppd
