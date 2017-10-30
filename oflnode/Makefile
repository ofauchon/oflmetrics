CFLAGS := $(CFLAGS) -DNODE_ID=0x22
BOARD ?= redbee-econotag
#BOARD ?= m12
PORT ?=/dev/ttyUSB1

# Check prerequisite (
MC1322X=/home/olivier/dev/oflmetrics/libmc1322x
MC1322X_LDR=$(MC1322X)/tools/mc1322x-load.pl
MC1322X_FLA=$(MC1322X)/tests/flasher_$(BOARD).bin
MC1322X_BBMC=$(MC1322X)/tools/ftditools/bbmc

export PATH := /home/olivier/CodeSourcery/Sourcery_G++_Lite/bin/:$(PATH)





# all off the common objects for each target
# a COBJ is made for EACH board and goes the obj_$(BOARD)_board directory
# board specific code is OK in these files
COBJS :=  libs/utils.o libs/utils-paquet.o  libs/xtea.o libs/ds1820.o libs/crc8.o

SRC := 


# all of the target programs to build
TARGETS := 

# these targets are built with space reserved for variables needed by ROM services
# this space is initialized with a rom call to rom_data_init
TARGETS_WITH_ROM_VARS := oflnode

##################################################

# you shouldn't need to edit anything below here #
##################################################

# This Makefile includes the default rule at the top
# it needs to be included first
-include $(MC1322X)/Makefile.include

# this rule will become the default_goal if
# $(MC1322X)/Makefile.include doesn't exist it will try to update the
# submodule, check if $(MC1322X) exists, and if it does
# try make again

check:
	@echo "Sanity checks"
	@if [ ! -d "$(MC1322X)" ] ; then echo "*** cannot find MC1322X directory '$(MC1322X)'" ; exit 2; fi 
	@if [ ! -e "$(MC1322X_FLA)" ] ; then echo "*** cannot find MC1322X flasher $(MC1322X_FLA), Try to build it this way : cd $(MC1322X)/tests; make BOARD=$(BOARD) " ; exit 2; fi 
	@if [ ! -e "$(MC1322X_BBMC)" ] ; then echo "*** cannot find MC1322X bbmc helper $(MC1322X_bbmc), Try to build it this way : cd $(MC1322X)/tools/ftdi-tools; make " ; exit 2; fi 
	@echo "Everything seems OK"
	

submodule: 
	git submodule update --init
	@if [ ! -d $(MC1322X) ] ; then echo "*** cannot find MC1322X directory $(MC1322X)" ; exit 2; fi
	$(MAKE)

.PHONY: submodule



flash: 
	 $(MC1322X_LDR) -t $(PORT) -f $(MC1322X_FLA) -s oflnode_$(BOARD).bin  -c "${MC1322X_BBMC} -l redbee-econotag erase"  -b 200


debug: 
	 openocd -f interface/ftdi/redbee-econotag.cfg -f target/mc13224v.cfg
	



