#----------------------------------------------------------------------------------------
# Goals for Build
#----------------------------------------------------------------------------------------

.PHONY: all

all: buildAA makecia

buildAA:
	# @$(MAKE) -C bootloader
	# @$(MAKE) -C bootstub
	@$(MAKE) -C main
	@$(MAKE) -C menu
	@mkdir "Relaunch/" || :
	@mkdir "Relaunch/_nds" || :
	@mkdir "Relaunch/_nds/Relaunch" || :
	@mv "menu/rbmenu.nds" "Relaunch/_nds/Relaunch/rbmenu.nds"
	@mv "main/RelaunchB.nds" "Relaunch/RelaunchB.nds"

makecia:
	@./make_cia --srl="Relaunch/RelaunchB.nds"

clean:
	@echo clean build directories
	# @$(MAKE) -C bootloader clean
	# @$(MAKE) -C bootstub clean
	@$(MAKE) -C main clean
	@$(MAKE) -C menu clean
	@rm -r Relaunch/

	@echo clean package files
