.SILENT :

SUBDIRS = bootalize fat12display libfat12

all:
	for dir in $(SUBDIRS) ; do \
		echo [Make] $$dir ; \
		make -C $$dir; \
	done
