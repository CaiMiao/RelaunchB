// xSPDX-License-Identifier: Zlib
//
// Copyright (C) 2005 Michael Noland (joat)
// Copyright (C) 2005 Jason Rogers (Dovoto)
// Copyright (C) 2005-2015 Dave Murphy (WinterMute)
// Copyright (C) 2023 Antonio Niño Díaz

// Default ARM7 core

// #include <dswifi7.h>
#include <nds.h>
// #include <maxmod7.h>
#include <string.h>

#define SD_IRQ_STATUS (*(vu32*)0x400481C)

void my_installSystemFIFO(void);
void my_sdmmc_get_cid(int devicenumber, u32 *cid);

u8 my_i2cReadRegister(u8 device, u8 reg);
u8 my_i2cWriteRegister(u8 device, u8 reg, u8 data);

//---------------------------------------------------------------------------------
void ReturntoDSiMenu() {
    //---------------------------------------------------------------------------------
        if (isDSiMode()) {
            i2cWriteRegister(0x4A, 0x70, 0x01);		// Bootflag = Warmboot/SkipHealthSafety
            i2cWriteRegister(0x4A, 0x11, 0x01);		// Reset to DSi Menu
        } else {
            u8 readCommand = readPowerManagement(0x10);
            readCommand |= BIT(0);
            writePowerManagement(0x10, readCommand);
        }
    }

volatile bool exit_loop = false;

void power_button_callback(void)
{
    exit_loop = true;
}

void vblank_handler(void)
{
	if(fifoCheckValue32(FIFO_USER_02)) {
		ReturntoDSiMenu();
	}
    inputGetAndSend();
    // Wifi_Update();
}

void set_ctr(u32* ctr){
	for (int i = 0; i < 4; i++) REG_AES_IV[i] = ctr[3-i];
}

// 10 11  22 23 24 25
void aes(void* in, void* out, void* iv, u32 method){ //this is sort of a bodged together dsi aes function adapted from this 3ds function
	REG_AES_CNT = ( AES_CNT_MODE(method) |           //https://github.com/TiniVi/AHPCFW/blob/master/source/aes.c#L42
					AES_WRFIFO_FLUSH |				 //as long as the output changes when keyslot values change, it's good enough.
					AES_RDFIFO_FLUSH | 
					AES_CNT_KEY_APPLY | 
					AES_CNT_KEYSLOT(3) |
					AES_CNT_DMA_WRITE_SIZE(2) |
					AES_CNT_DMA_READ_SIZE(1)
					);

	if (iv != NULL) set_ctr((u32*)iv);
	REG_AES_BLKCNT = (1 << 16);
	REG_AES_CNT |= 0x80000000;
	
	for (int j = 0; j < 0x10; j+=4) REG_AES_WRFIFO = *((u32*)(in+j));
	while(((REG_AES_CNT >> 0x5) & 0x1F) < 0x4); //wait for every word to get processed
	for (int j = 0; j < 0x10; j+=4) *((u32*)(out+j)) = REG_AES_RDFIFO;
	//REG_AES_CNT &= ~0x80000000;
	//if (method & (AES_CTR_DECRYPT | AES_CTR_ENCRYPT)) add_ctr((u8*)iv);
}

int main(int argc, char *argv[])
{
    nocashMessage("ARM7 main.c main");

	*(vu32*)0x400481C = 0;				// Clear SD IRQ stat register
	*(vu32*)0x4004820 = 0;				// Clear SD IRQ mask register

	REG_MBK9 = 0; // Allow full DSi WRAM access to ARM9

    // Initialize sound hardware
    enableSound();

    // Read user information from the firmware (name, birthday, etc)
    // readUserSettings();

    // Stop LED blinking
    ledBlink(0);

    // Using the calibration values read from the firmware with
    // readUserSettings(), calculate some internal values to convert raw
    // coordinates into screen coordinates.
    // touchInit();

    irqInit();
    fifoInit();

    // installSoundFIFO();
    installSystemFIFO(); // Sleep mode, storage, firmware...
	// my_installSystemFIFO();
    // installWifiFIFO();
    // if (isDSiMode())
    //     installCameraFIFO();

    // Initialize Maxmod. It uses timer 0 internally.
    // mmInstall(FIFO_MAXMOD);

    // This sets a callback that is called when the power button in a DSi
    // console is pressed. It has no effect in a DS.
    setPowerButtonCB(power_button_callback);

    // Read current date from the RTC and setup an interrupt to update the time
    // regularly. The interrupt simply adds one second every time, it doesn't
    // read the date. Reading the RTC is very slow, so it's a bad idea to do it
    // frequently.
    initClockIRQTimer(3);

    // Now that the FIFO is setup we can start sending input data to the ARM9.
    irqSet(IRQ_VBLANK, vblank_handler);
    irqEnable(IRQ_VBLANK);

	// Check for 3DS
	if(isDSiMode() || (REG_SCFG_EXT & BIT(22))) {
		u8 byteBak = my_i2cReadRegister(0x4A, 0x71);
		my_i2cWriteRegister(0x4A, 0x71, 0xD2);
		fifoSendValue32(FIFO_USER_05, my_i2cReadRegister(0x4A, 0x71));
		my_i2cWriteRegister(0x4A, 0x71, byteBak);
	}

	if (isDSiMode() || ((REG_SCFG_EXT & BIT(17)) && (REG_SCFG_EXT & BIT(18)))) {
		u8 *out=(u8*)0x02F00000;
		memset(out, 0, 16);

		// first check whether we can read the console ID directly and it was not hidden by SCFG
		if (((*(vu16*)0x04004000) & (1u << 10)) == 0 && ((*(vu8*)0x04004D08) & 0x1) == 0)
		{
			// The console id registers are readable, so use them!
			memcpy(out, (u8*)0x04004D00, 8);
		}
		if(out[0] == 0 || out[1] == 0) {
			// For getting ConsoleID without reading from 0x4004D00...
			u8 base[16]={0};
			u8 in[16]={0};
			u8 iv[16]={0};
			u8 *scratch=(u8*)0x02F00200;
			u8 *key3=(u8*)0x40044D0;

			aes(in, base, iv, 2);

			//write consecutive 0-255 values to any byte in key3 until we get the same aes output as "base" above - this reveals the hidden byte. this way we can uncover all 16 bytes of the key3 normalkey pretty easily.
			//greets to Martin Korth for this trick https://problemkaputt.de/gbatek.htm#dsiaesioports (Reading Write-Only Values)
			for(int i=0;i<16;i++){
				for(int j=0;j<256;j++){
					*(key3+i)=j & 0xFF;
					aes(in, scratch, iv, 2);
					if(!memcmp(scratch, base, 16)){
						out[i]=j;
						//hit++;
						break;
					}
				}
			}
		}
	}

	fifoSendValue32(FIFO_USER_03, REG_SCFG_EXT);
	fifoSendValue32(FIFO_USER_07, *(u16*)(0x4004700));
	fifoSendValue32(FIFO_USER_06, 1);

    while (!exit_loop)
    {
        const uint16_t key_mask = KEY_SELECT | KEY_START | KEY_L | KEY_R;
        uint16_t keys_pressed = ~REG_KEYINPUT;

        if ((keys_pressed & key_mask) == key_mask)
            exit_loop = true;

        if (*(u32*)(0x2FFFD0C) == 0x454D4D43) {
            my_sdmmc_get_cid(true, (u32*)0x2FFD7BC);	// Get eMMC CID
            *(u32*)(0x2FFFD0C) = 0;
        }
        resyncClock();

        // Send SD status
        if(isDSiMode() || *(u16*)(0x4004700) != 0)
            fifoSendValue32(FIFO_USER_04, SD_IRQ_STATUS);

        // // Dump EEPROM save
        // if(fifoCheckAddress(FIFO_USER_01)) {
        //     switch(fifoGetValue32(FIFO_USER_01)) {
        //         case 0x44414552: // 'READ'
        //             readEeprom((u8 *)fifoGetAddress(FIFO_USER_01), fifoGetValue32(FIFO_USER_01), fifoGetValue32(FIFO_USER_01));
        //             break;
        //         case 0x54495257: // 'WRIT'
        //             writeEeprom(fifoGetValue32(FIFO_USER_01), (u8 *)fifoGetAddress(FIFO_USER_01), fifoGetValue32(FIFO_USER_01));
        //             break;
        //     }
        // }

        swiWaitForVBlank();
    }

    return 0;
}
