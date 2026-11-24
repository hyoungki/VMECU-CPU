

LOCAL_HOME = $(shell pwd)
TARGET_BIN=$(LOCAL_HOME)/KEC_BIN


export LOCAL_HOME TARGET_BIN

CD=cd



all:
	@for i in lib WDT CLI HOST LINK MMI SCAN SCU SIM ; do \
	if [ -d $$i ] ; then \
		echo "compiling" $$i;\
		cd $$i ; \
		$(MAKE) all || exit 1;\
		cd ..;\
	else \
		echo "no dir";\
		exit 1 ;\
	fi ;\
	 done


hkkim:
	@echo "compiling lib" 
	@for i in  lib WDT CLI HOST LINK MMI SCAN SCU SIM ; do \
	if [ -d $$i ] ; then \
		echo $$i;\
		cd $$i ; \
		$(MAKE) all;\
		cd ..;\
	else \
		echo "no dir";\
		exit 1 ;\
	fi ;\
	 done



clean:
	@echo "clean subdir" 
	for i in lib WDT CLI HOST LINK MMI SCAN SCU SIM ; do \
		cd $$i ; \
		$(MAKE) clean;\
		cd ..;\
	 done


# @if [ -d $$i ] ; then \
#	@if [ -d $$i ]; then $(CD) $$i; $(MAKE) all; fi \
#	done
#fi;\

#echo $(shell pwd) \
	

PORT: 
	@echo "force to compile PORT"
	@if [ -d PORT-TEST  ]; then $(CD) PORT-TEST; $(MAKE) clean; $(MAKE) all; fi
	
TKEXIT: 
	@echo "force to compile EXIT"
	@if [ -d EXIT ]; then $(CD) EXIT; $(MAKE) clean; $(MAKE) all; fi
	
cli2: 
	@echo "force to compile CLI"
	@if [ -d CLI ]; then $(CD) CLI; $(MAKE) clean; $(MAKE) all; fi


LHS:
	@echo "compiling lib"
	@if [ -d lib ]; then $(CD) lib; $(MAKE) all; fi
	@if [ -d CLI ]; then $(CD) CLI; $(MAKE) all; fi
	@if [ -d HOST ]; then $(CD) HOST; $(MAKE) all; fi
	@if [ -d MMI ]; then $(CD) MMI; $(MAKE) all; fi
	@if [ -d SCAN ]; then $(CD) SCAN; $(MAKE) all; fi
	@if [ -d SCU ]; then $(CD) SCU; $(MAKE) all; fi
	@if [ -d SIM ]; then $(CD) SIM; $(MAKE) all; fi





clean2:
	@echo "clean subdirs"
	@if [ -d lib ]; then $(CD) lib; $(MAKE) clean; fi
	@if [ -d WDT ]; then $(CD) WDT; $(MAKE) clean; fi
	@if [ -d CLI ]; then $(CD) CLI; $(MAKE) clean; fi
	@if [ -d HOST ]; then $(CD) HOST; $(MAKE) clean; fi
	@if [ -d LINK ]; then $(CD) LINK; $(MAKE) clean; fi



PHONY : clean cli TKEXIT 
