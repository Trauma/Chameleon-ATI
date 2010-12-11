/*
 *  ATI injector
 *
 *  Copyright (C) 2009  Jasmin Fazlic, iNDi, netkas 
 * 
 *  Evergreen support and code revision by Trauma (2010)
 *
 *  ATI injector is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  ATI driver and injector is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with ATI injector.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  Alternatively you can choose to comply with APSL
 */


#include "libsaio.h"
#include "boot.h"
#include "bootstruct.h"
#include "pci.h"
#include "platform.h"
#include "device_inject.h"
#include "ati.h"

#ifndef DEBUG_ATI
#define DEBUG_ATI 0
#endif

#if DEBUG_ATI
#define DBG(x...)	printf(x)
#else
#define DBG(x...)
#endif

extern uint32_t devices_number;

// Shared Radeon HD keys
const char *ati_efidisplay_0[]					  = { "@0,ATY,EFIDisplay", "TMDSA" };
const char *ati_compatible_0[]					  = { "@0,compatible", "ATY,%s" };
const char *ati_compatible_1[]					  = { "@1,compatible", "ATY,%s" };
const char *ati_device_type_0[]					  = { "@0,device_type", "display" };
const char *ati_device_type_1[]					  = { "@1,device_type", "display" };
const char *ati_device_type[]					  = { "device_type", "ATY,%sParent" };
const char *ati_name_0[]						  = { "@0,name", "ATY,%s" };
const char *ati_name_1[]						  = { "@1,name", "ATY,%s" };
const char *ati_name[]							  = { "name", "ATY,%sParent" };
const char *ati_efi_version[]					  = { "ATY,EFIVersion", "Chameleon" };
struct ati_data_key ati_vendor_id				  = { 0x02, "ATY,VendorID", {0x02, 0x10} };
struct ati_data_key ati_vram_memsize_0			  = { 0x08, "@0,VRAM,memsize", {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00} };
struct ati_data_key ati_vram_memsize_1			  = { 0x08, "@1,VRAM,memsize", {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00} };

// Evergreen keys
const char *ati_compatible_2[]					  = { "@2,compatible", "ATY,%s" };
const char *ati_device_type_2[]					  = { "@2,device_type", "display" };
const char *ati_name_2[]						  = { "@2,name", "ATY,%s" };
const char *ati_hda_gfx[]						  = { "hda-gfx", "onboard-1" };
struct ati_data_key ati_aapl_blackscr_prefs_2	  = { 0x04, "AAPL02,blackscreen-preferences", {0x00, 0x00, 0x00, 0x00} };
struct ati_data_key ati_aapl02_coher			  = { 0x04, "AAPL02,Coherency", {0x02, 0x00, 0x00, 0x00} };
struct ati_data_key ati_vram_memsize_2			  = { 0x08, "@2,VRAM,memsize", {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00} };

// Legacy Radeon HD keys
struct ati_data_key ati_fb_offset_leg			  = { 0x08, "ATY,FrameBufferOffset", {0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00} };
struct ati_data_key ati_hwgpio_leg				  = { 0x04, "ATY,HWGPIO", {0x23, 0xa8, 0x48, 0x00} };
struct ati_data_key ati_iospace_offset_leg		  = { 0x08, "ATY,IOSpaceOffset", {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00} };
struct ati_data_key ati_mclk_leg				  = { 0x04, "ATY,MCLK", {0x00, 0x35, 0x0c, 0x00} };
struct ati_data_key ati_sclk_leg				  = { 0x04, "ATY,SCLK", {0x60, 0xae, 0x0a, 0x00} };
struct ati_data_key ati_refclk_leg				  = { 0x04, "ATY,RefCLK", {0x8c, 0x0a, 0x00, 0x00} };
struct ati_data_key ati_regspace_offset_leg		  = { 0x08, "ATY,RegisterSpaceOffset", {0x00, 0x00, 0x00, 0x00, 0x90, 0xa2, 0x00, 0x00} };
struct ati_data_key ati_aapl01_coher			  = { 0x04, "AAPL01,Coherency", {0x02, 0x00, 0x00, 0x00} };
struct ati_data_key ati_aapl_blackscr_prefs_0_leg = { 0x04, "AAPL00,blackscreen-preferences", {0x00, 0x00, 0x00, 0x00} };
struct ati_data_key ati_aapl_blackscr_prefs_1_leg = { 0x04, "AAPL01,blackscreen-preferences", {0x00, 0x00, 0x00, 0x00} };
struct ati_data_key ati_swgpio_info_leg			  = { 0x04, "ATY,SWGPIO Info", {0x00, 0x48, 0xa8, 0x23} };

// Known cards as of 2010/08/28
static struct ati_chipsets_t ATIKnownChipsets[] = {
	{ 0x00000000,  "Unknown" } ,
	{ 0x100294C3,  "ATI Radeon HD 2400 Series"}  ,
	{ 0x100294C4,  "ATI Radeon HD 2400 Series"}  ,
	{ 0x100294C6,  "ATI Radeon HD 2400 Series"}  ,
	{ 0x100294C9,  "ATI Radeon HD 2400 Series"}  ,
	{ 0x10029589,  "ATI Radeon HD 2600 Series"}  ,
	{ 0x10029588,  "ATI Radeon HD 2600 Series"}  ,
	{ 0x10029581,  "ATI Radeon HD 2600 Series"}  ,
	{ 0x10029583,  "ATI Radeon HD 2600 Series"}  ,
	{ 0x10029586,  "ATI Radeon HD 2600 Series"}  ,
	{ 0x10029587,  "ATI Radeon HD 2600 Series"}  ,
	{ 0x10029400,  "ATI Radeon HD 2900 Series"}  ,
	{ 0x10029405,  "ATI Radeon HD 2900 GT"}  ,
	{ 0x10029501,  "ATI Radeon HD 3800 Series"}  ,
	{ 0x10029505,  "ATI Radeon HD 3800 Series"}  ,
	{ 0x10029515,  "ATI Radeon HD 3800 Series"}  ,
	{ 0x10029507,  "ATI Radeon HD 3800 Series"}  ,
	{ 0x10029500,  "ATI Radeon HD 3800 Series"}  ,
	{ 0x1002950F,  "ATI Radeon HD 3800 X2"}  ,
	{ 0x100295C5,  "ATI Radeon HD 3400 Series"}  ,
	{ 0x100295C7,  "ATI Radeon HD 3400 Series"}  ,
	{ 0x100295C0,  "ATI Radeon HD 3400 Series"}  ,
	{ 0x10029596,  "ATI Radeon HD 3600 Series"}  ,
	{ 0x10029590,  "ATI Radeon HD 3600 Series"}  ,
	{ 0x10029599,  "ATI Radeon HD 3600 Series"}  ,
	{ 0x10029597,  "ATI Radeon HD 3600 Series"}  ,
	{ 0x10029598,  "ATI Radeon HD 3600 Series"}  ,
	{ 0x10029442,  "ATI Radeon HD 4850"}  ,
	{ 0x10029440,  "ATI Radeon HD 4870"}  ,
	{ 0x1002944C,  "ATI Radeon HD 4830"}  ,
	{ 0x10029460,  "ATI Radeon HD 4890"}  ,
	{ 0x10029462,  "ATI Radeon HD 4890"}  ,
	{ 0x10029441,  "ATI Radeon HD 4870 X2"}  ,
	{ 0x10029443,  "ATI Radeon HD 4850 X2"}  ,
	{ 0x10029444,  "ATI Radeon HD 4800 Series"}  ,
	{ 0x10029446,  "ATI Radeon HD 4800 Series"}  ,
	{ 0x1002944E,  "ATI Radeon HD 4730"}  ,
	{ 0x10029450,  "ATI Radeon HD 4800 Series"}  ,
	{ 0x10029452,  "ATI Radeon HD 4800 Series"}  ,		
	{ 0x10029456,  "ATI Radeon HD 4800 Series"}  ,		
	{ 0x1002944A,  "ATI Radeon HD 4800 Mobility Series"}  ,
	{ 0x1002945A,  "ATI Radeon HD 4800 Mobility Series"}  ,
	{ 0x1002945B,  "ATI Radeon HD 4800 Mobility Series"}  ,
	{ 0x1002944B,  "ATI Radeon HD 4800 Mobility Series"}  ,
	{ 0x10029490,  "ATI Radeon HD 4670"}  ,
	{ 0x10029498,  "ATI Radeon HD 4650"}  ,
	{ 0x10029490,  "ATI Radeon HD 4600 Series"}  ,
	{ 0x10029498,  "ATI Radeon HD 4600 Series"}  ,
	{ 0x1002949E,  "ATI Radeon HD 4600 Series"}  ,
	{ 0x10029480,  "ATI Radeon HD 4600 Series"}  ,
	{ 0x10029488,  "ATI Radeon HD 4600 Series"}  ,
	{ 0x10029540,  "ATI Radeon HD 4500 Series"}  ,
	{ 0x10029541,  "ATI Radeon HD 4500 Series"}  ,
	{ 0x1002954E,  "ATI Radeon HD 4500 Series"}  ,
	{ 0x10029552,  "ATI Radeon HD 4300 Mobility Series"}  ,
	{ 0x10029553,  "ATI Radeon HD 4500 Mobility Series"}  ,
	{ 0x1002954F,  "ATI Radeon HD 4300 Series"} ,
	{ 0x100294B3,  "ATI Radeon HD 4770"} ,
	{ 0x100268F9,  "ATI Radeon HD 5400 Series"} ,
	{ 0x100268D9,  "ATI Radeon HD 5500 Series"} ,
	{ 0x100268DA,  "ATI Radeon HD 5500 Series"} ,
	{ 0x100268B9,  "ATI Radeon HD 5600 Series"} ,
	{ 0x100268D8,  "ATI Radeon HD 5670"} ,
	{ 0x100268B8,  "ATI Radeon HD 5770"} ,
	{ 0x100268BE,  "ATI Radeon HD 5750"} ,
	{ 0x10026898,  "ATI Radeon HD 5870"} ,
	{ 0x10026899,  "ATI Radeon HD 5850"} ,
	{ 0x1002689E,  "ATI Radeon HD 5830"} ,
	{ 0x1002689C,  "ATI Radeon HD 5970"}
};

// Default known working framebuffers
static struct ati_chipsets_t ATIKnownFramebuffers[] = {
	{ 0x00000000,  "Megalodon" },
	{ 0x10029589,  "Lamna"}  ,
	{ 0x10029588,  "Lamna"}  ,
	{ 0x100294C3,  "Iago"}  ,
	{ 0x100294C4,  "Iago"}  ,
	{ 0x100294C6,  "Iago"}  ,
	{ 0x10029400,  "Franklin"}  ,
	{ 0x10029405,  "Franklin"}  ,
	{ 0x10029581,  "Hypoprion"}  ,
	{ 0x10029583,  "Hypoprion"}  ,
	{ 0x10029586,  "Hypoprion"}  ,
	{ 0x10029587,  "Hypoprion"}  ,
	{ 0x100294C9,  "Iago"}  ,
	{ 0x10029501,  "Megalodon"}  ,
	{ 0x10029505,  "Megalodon"}  ,
	{ 0x10029515,  "Megalodon"}  ,
	{ 0x10029507,  "Megalodon"}  ,
	{ 0x10029500,  "Megalodon"}  ,
	{ 0x1002950F,  "Triakis"}  ,
	{ 0x100295C5,  "Iago"}  ,
	{ 0x100295C7,  "Iago"}  ,
	{ 0x100295C0,  "Iago"}  ,
	{ 0x10029596,  "Megalodon"}  ,
	{ 0x10029590,  "Megalodon"}  ,
	{ 0x10029599,  "Megalodon"}  ,
	{ 0x10029597,  "Megalodon"}  ,
	{ 0x10029598,  "Megalodon"}  ,
	{ 0x10029442,  "Motmot"}  ,
	{ 0x10029440,  "Motmot"}  ,
	{ 0x1002944C,  "Motmot"}  ,
	{ 0x10029460,  "Motmot"}  ,
	{ 0x10029462,  "Motmot"}  ,
	{ 0x10029441,  "Motmot"}  ,
	{ 0x10029443,  "Motmot"}  ,
	{ 0x10029444,  "Motmot"}  ,
	{ 0x10029446,  "Motmot"}  ,
	{ 0x1002944E,  "Motmot"}  ,
	{ 0x10029450,  "Motmot"}  ,
	{ 0x10029452,  "Motmot"}  ,		
	{ 0x10029456,  "Motmot"}  ,		
	{ 0x1002944A,  "Motmot"}  ,
	{ 0x1002945A,  "Motmot"}  ,
	{ 0x1002945B,  "Motmot"}  ,
	{ 0x1002944B,  "Motmot"}  ,
	{ 0x10029490,  "Peregrine"}  ,
	{ 0x10029498,  "Peregrine"}  ,
	{ 0x1002949E,  "Peregrine"}  ,
	{ 0x10029480,  "Peregrine"}  ,
	{ 0x10029488,  "Peregrine"}  ,
	{ 0x10029540,  "Peregrine"}  ,
	{ 0x10029541,  "Peregrine"}  ,
	{ 0x1002954E,  "Peregrine"}  ,
	{ 0x10029552,  "Peregrine"}  ,
	{ 0x10029553,  "Peregrine"}  ,
	{ 0x1002954F,  "Peregrine"}  ,
	{ 0x100294B3,  "Peregrine"},
	{ 0x100294B5,  "Peregrine"},
	{ 0x100268F9,  "Uakari"} ,
	{ 0x100268D9,  "Uakari"} ,
	{ 0x100268DA,  "Uakari"} ,
	{ 0x100268B9,  "Vervet"} , 
	{ 0x100268d8,  "Baboon"} ,
	{ 0x100268B8,  "Vervet"} ,
	{ 0x100268BE,  "Vervet"} ,
	{ 0x10026898,  "Uakari"} ,
	{ 0x10026899,  "Uakari"} ,
	{ 0x1002689c,  "Uakari"} ,
	{ 0x1002689e,  "Uakari"}
};

static uint32_t accessROM(pci_dt_t *ati_dev, unsigned int mode)
{
	uint32_t		bar[7];
	volatile uint32_t	*regs;
	
	bar[2] = pci_config_read32(ati_dev->dev.addr, 0x18 );
	regs = (uint32_t *) (bar[2] & ~0x0f);
	
	if (mode) {
		if (mode != 1) {
			return 0xe00002c7;
		}
		REG32W(0x179c, 0x00080000);
		REG32W(0x1798, 0x00080721);
		REG32W(0x17a0, 0x00080621);
		REG32W(0x1600, 0x14030300);
		REG32W(0x1798, 0x21);
		REG32W(0x17a0, 0x21);
		REG32W(0x179c, 0x00);
		REG32W(0x17a0, 0x21);
		REG32W(0x1798, 0x21);
		REG32W(0x1798, 0x21);
	} else {
		REG32W(0x1600, 0x14030302);	
		REG32W(0x1798, 0x21);
		REG32W(0x17a0, 0x21);
		REG32W(0x179c, 0x00080000);
		REG32W(0x17a0, 0x00080621);
		REG32W(0x1798, 0x00080721);
		REG32W(0x1798, 0x21);
		REG32W(0x17a0, 0x21);
		REG32W(0x179c, 0x00);
		REG32W(0x1604, 0x0400e9fc);
		REG32W(0x161c, 0x00);
		REG32W(0x1620, 0x9f);
		REG32W(0x1618, 0x00040004);
		REG32W(0x161c, 0x00);
		REG32W(0x1604, 0xe9fc);
		REG32W(0x179c, 0x00080000);
		REG32W(0x1798, 0x00080721);
		REG32W(0x17a0, 0x00080621);
		REG32W(0x1798, 0x21);
		REG32W(0x17a0, 0x21);
		REG32W(0x179c, 0x00);
	}
	return 0;
}

static uint8_t *readAtomBIOS(pci_dt_t *ati_dev)
{
	uint32_t		bar[7];
	uint32_t		*BIOSBase;
	uint32_t		counter;
	volatile uint32_t	*regs;
	
	bar[2] = pci_config_read32(ati_dev->dev.addr, 0x18 );
	regs = (volatile uint32_t *) (bar[2] & ~0x0f);
	accessROM(ati_dev, 0);
	REG32W(0xa8, 0);
	REG32R(0xac);
	REG32W(0xa8, 0);
	REG32R(0xac);	
	
	BIOSBase = malloc(0x10000);
	REG32W(0xa8, 0);
	BIOSBase[0] = REG32R(0xac);
	counter = 4;
	do {
		REG32W(0xa8, counter);
		BIOSBase[counter/4] = REG32R(0xac);
		counter +=4;
	} while (counter != 0x10000);
	accessROM((pci_dt_t *)regs, 1);
	
	if (*(uint16_t *)BIOSBase != 0xAA55) {
		verbose("Wrong ATI BIOS signature: %04x\n", *(uint16_t *)BIOSBase);
		return 0;
	}
	return (uint8_t *)BIOSBase;
}

uint32_t getvramsizekb(pci_dt_t *ati_dev)
{
	uint32_t		bar[7];
	uint32_t		size;
	volatile uint32_t	*regs;
	
	bar[2] = pci_config_read32(ati_dev->dev.addr, 0x18 );
	regs = (uint32_t *) (bar[2] & ~0x0f);
	if ((ati_dev->device_id >= 0x6800) || (ati_dev->device_id <= 0x68FF)) {
		size = (REG32R(R6XX_CONFIG_MEMSIZE) >> 10) * 1024 * 1024;
	} else {
		size = (REG32R(R6XX_CONFIG_MEMSIZE)) >> 10;
	}
	return size;
}

static bool radeon_card_posted(pci_dt_t *ati_dev)
{
	uint32_t		bar[7];
	uint32_t		val;
	volatile uint32_t	*regs;
	uint8_t *biosimage = (uint8_t *)0x000C0000;
	
	bar[2] = pci_config_read32(ati_dev->dev.addr, 0x18);
	regs = (uint32_t *) (bar[2] & ~0x0f);
	val = (REG32R(AVIVO_D1CRTC_CONTROL) | REG32R(AVIVO_D2CRTC_CONTROL));
	
	if (val & AVIVO_CRTC_EN) { // Fail on evergreen cards 
		verbose("ATI card was POSTed AVIVO way\n");
		return true;
	} else if ((uint8_t)biosimage[0] == 0x55 && (uint8_t)biosimage[1] == 0xaa) { // Evergreen's alternative, but should fail on X2/CrossFire
		struct  pci_rom_pci_header_t *rom_pci_header;   
		rom_pci_header = (struct pci_rom_pci_header_t*)(biosimage + (uint8_t)biosimage[24] + (uint8_t)biosimage[25]*256);
		if (rom_pci_header->signature == 0x52494350) {
			if (rom_pci_header->device == ati_dev->device_id) {
				verbose("ATI card was POSTed PCI ROM way\n");
				return true;
			}
		}
	}
	verbose("ATI card was not POSTed\n"); // Both alternatives failed, then will try readAtomBIOS method
	return false;
}

static uint32_t load_ati_bios_file(const char *filename, uint8_t *buf, int bufsize)
{
	int	fd;
	int	size;
	
	if ((fd = open_bvdev("bt(0,0)", filename, 0)) < 0) {
		return 0;
	}
	size = file_size(fd);
	if (size > bufsize) {
		verbose("Filesize of %s is bigger than expected! Truncating to 0x%x Bytes!\n", filename, bufsize);
		size = bufsize;
	}
	size = read(fd, (char *)buf, size);
	close(fd);
	return size > 0 ? size : 0;
}

static char *get_ati_model(uint32_t id)
{
	int	i;
	
	for (i=0; i< (sizeof(ATIKnownChipsets) / sizeof(ATIKnownChipsets[0])); i++) {
		if (ATIKnownChipsets[i].device == id) {
			return ATIKnownChipsets[i].name;
		}
	}
	return ATIKnownChipsets[0].name;
}

static char *get_ati_fb(uint32_t id)
{
	int	i;
	
	for (i=0; i< (sizeof(ATIKnownFramebuffers) / sizeof(ATIKnownFramebuffers[0])); i++) {
		if (ATIKnownFramebuffers[i].device == id) {
			return ATIKnownFramebuffers[i].name;
		}
	}
	return ATIKnownFramebuffers[0].name;
}

static int devprop_add_iopciconfigspace(struct DevPropDevice *device, pci_dt_t *ati_dev)
{
	int	i;
	uint8_t	*config_space;
	
	if (!device || !ati_dev) {
		return 0;
	}
	verbose("Dumping PCI config space, 256 bytes\n");
	config_space = malloc(256);
	for (i=0; i<=255; i++) {
		config_space[i] = pci_config_read8( ati_dev->dev.addr, i);
	}
	devprop_add_value(device, "ATY,PCIConfigSpace", config_space, 256);
	free (config_space);
	return 1;
}

static int devprop_add_ati_template_shared(struct DevPropDevice *device)
{
	if(!device)
		return 0;	
	if(!DP_ADD_TEMP_VAL(device, ati_device_type_0))
		return 0;
	if(!DP_ADD_TEMP_VAL(device, ati_device_type_1))
		return 0;
	if(!DP_ADD_TEMP_VAL(device, ati_efidisplay_0))
		return 0;
	if(!DP_ADD_TEMP_VAL(device, ati_efi_version))
		return 0;
	if(!DP_ADD_TEMP_VAL(device, ati_name_1))
		return 0;
	if(!DP_ADD_TEMP_VAL_DATA(device, ati_aapl01_coher))
		return 0;
	if(!DP_ADD_TEMP_VAL_DATA(device, ati_vendor_id))
		return 0;
	if(!DP_ADD_TEMP_VAL_DATA(device, ati_aapl_blackscr_prefs_0_leg))
		return 0;
	if(!DP_ADD_TEMP_VAL_DATA(device, ati_aapl_blackscr_prefs_1_leg))
		return 0;	
	return 1;
}

static int devprop_add_ati_template_evergreen(struct DevPropDevice *device)
{
	if(!device)
		return 0;
	if(!DP_ADD_TEMP_VAL(device, ati_hda_gfx))
		return 0;
	if(!DP_ADD_TEMP_VAL_DATA(device, ati_aapl02_coher))
		return 0;
	if(!DP_ADD_TEMP_VAL_DATA(device, ati_aapl_blackscr_prefs_2))
		return 0;
	return 1;
}

static int devprop_add_ati_template_leg(struct DevPropDevice *device)
{
	if(!device)
		return 0;
	if(!DP_ADD_TEMP_VAL_DATA(device, ati_hwgpio_leg))
		return 0;
	if(!DP_ADD_TEMP_VAL_DATA(device, ati_iospace_offset_leg))
		return 0;
	if(!DP_ADD_TEMP_VAL_DATA(device, ati_regspace_offset_leg))
		return 0;
	if(!DP_ADD_TEMP_VAL_DATA(device, ati_aapl_blackscr_prefs_0_leg))
		return 0;
	if(!DP_ADD_TEMP_VAL_DATA(device, ati_aapl_blackscr_prefs_1_leg))
		return 0;
	if(!DP_ADD_TEMP_VAL_DATA(device, ati_swgpio_info_leg))
		return 0;
	return 1;
}


bool setup_ati_devprop(pci_dt_t *ati_dev)
{
	struct DevPropDevice	*device;
	char			*devicepath;
	char			*model;
	const char		*framebuffer;
	char			biosVersion[32];
	char			tmp[64];
	uint8_t			*rom = NULL;
	uint32_t		rom_size = 0;
	uint8_t			*bios;
	uint32_t		bios_size;
	uint32_t		vram_size;
	uint32_t		boot_display;
	bool			doit;
	bool			toFree;
	bool			evergreen = false;
	
	devicepath = get_pci_dev_path(ati_dev);
	
	vram_size = getvramsizekb(ati_dev);
	model = get_ati_model((ati_dev->vendor_id << 16) | ati_dev->device_id);
	verbose("%s %dMB [%04x:%04x] :: %s\n", model, (uint32_t)(vram_size / 1024), ati_dev->vendor_id, ati_dev->device_id, devicepath);
	
	if ((framebuffer = getStringForKey(kAtiFb, &bootInfo->bootConfig))!=NULL)
	{
		verbose("Using custom FrameBuffer: %s\n", framebuffer);
	}
	else
	{
		framebuffer = get_ati_fb((ati_dev->vendor_id << 16) | ati_dev->device_id);
		verbose("Using default FrameBuffer: %s\n", framebuffer);
	}
	
	if (!string){
		string = devprop_create_string();
	}
	
	device = devprop_add_device(string, devicepath);
	
	if (!device) {
		verbose("Failed to initialize dev-props string dev-entry, press any key...\n");
		getc();
		return false;
	}
	
	if (strncmp(model, "ATI Radeon HD 4", 15) == 0) {
		devprop_add_ati_template_shared(device);
	} else if (strncmp(model, "ATI Radeon HD 5", 15) == 0) {
		devprop_add_ati_template_shared(device);
		devprop_add_ati_template_evergreen(device);
		evergreen = true;
	} else {
		devprop_add_ati_template_shared(device);
		devprop_add_ati_template_leg(device);
		devprop_add_iopciconfigspace(device, ati_dev);
	}
	
	vram_size = getvramsizekb(ati_dev) * 1024;
	if ((vram_size > 0x80000000) || (vram_size == 0)) {
		vram_size = 0x10000000;
		verbose("VRAM reported wrong, defaulting to 256 mb\n");
	}
	devprop_add_value(device, "VRAM,totalsize", (uint8_t*)&vram_size, 4);
	ati_vram_memsize_0.data[2] = (vram_size >> 16) & 0xFF; //4,5 are 0x00 anyway
	ati_vram_memsize_0.data[3] = (vram_size >> 24) & 0xFF;
	ati_vram_memsize_0.data[6] = (vram_size >> 16) & 0xFF; //4,5 are 0x00 anyway
	ati_vram_memsize_0.data[7] = (vram_size >> 24) & 0xFF;
	ati_vram_memsize_1.data[6] = (vram_size >> 16) & 0xFF; //4,5 are 0x00 anyway
	ati_vram_memsize_1.data[7] = (vram_size >> 24) & 0xFF;
	DP_ADD_TEMP_VAL_DATA(device, ati_vram_memsize_0);
	DP_ADD_TEMP_VAL_DATA(device, ati_vram_memsize_1);
	
	if (devices_number == 1) {
		boot_display = 1;
	}
	devprop_add_value(device, "@0,AAPL,boot-display", (uint8_t*)&boot_display, 4);
	devprop_add_value(device, "model", (uint8_t*)model, (strlen(model) + 1));
	devprop_add_value(device, "ATY,DeviceID", (uint8_t*)&ati_dev->device_id, 2);
	
	//fb setup
	
	sprintf(tmp, "Slot-%x",devices_number);
	devprop_add_value(device, "AAPL,slot-name", (uint8_t*)tmp, strlen(tmp) + 1);
	devices_number++;
	
	sprintf(tmp, ati_compatible_0[1], framebuffer);
	devprop_add_value(device, (char *) ati_compatible_0[0], (uint8_t *)tmp, strlen(tmp) + 1);
	
	sprintf(tmp, ati_compatible_1[1], framebuffer);
	devprop_add_value(device, (char *) ati_compatible_1[0], (uint8_t *)tmp, strlen(tmp) + 1);
	
	sprintf(tmp, ati_device_type[1], framebuffer);
	devprop_add_value(device, (char *) ati_device_type[0], (uint8_t *)tmp, strlen(tmp) + 1);
	
	sprintf(tmp, ati_name[1], framebuffer);
	devprop_add_value(device, (char *) ati_name[0], (uint8_t *)tmp, strlen(tmp) + 1);
	
	sprintf(tmp, ati_name_0[1], framebuffer);
	devprop_add_value(device, (char *) ati_name_0[0], (uint8_t *)tmp, strlen(tmp) + 1);
	
	sprintf(tmp, ati_name_1[1], framebuffer);
	devprop_add_value(device, (char *) ati_name_1[0], (uint8_t *)tmp, strlen(tmp) + 1);
	
	if (evergreen){
		ati_vram_memsize_2.data[6] = (vram_size >> 16) & 0xFF; //4,5 are 0x00 anyway
		ati_vram_memsize_2.data[7] = (vram_size >> 24) & 0xFF;
		DP_ADD_TEMP_VAL_DATA(device, ati_vram_memsize_2);
		
		sprintf(tmp, ati_compatible_2[1], framebuffer);
		devprop_add_value(device, (char *) ati_compatible_2[0], (uint8_t *)tmp, strlen(tmp) + 1);
		
		sprintf(tmp, ati_name_2[1], framebuffer);
		devprop_add_value(device, (char *) ati_name_2[0], (uint8_t *)tmp, strlen(tmp) + 1);
		
		sprintf(tmp, ati_device_type_2[1], framebuffer);
		devprop_add_value(device, (char *) ati_device_type_2[0], (uint8_t *)tmp, strlen(tmp) + 1);
	}
	
	sprintf(tmp, "/Extra/%04x_%04x.rom", (uint16_t)ati_dev->vendor_id, (uint16_t)ati_dev->device_id);
	if (getBoolForKey(kUseAtiROM, &doit, &bootInfo->bootConfig) && doit) {
		verbose("Looking for ATI Video BIOS File %s\n", tmp);
		rom = malloc(0x20000);
		rom_size = load_ati_bios_file(tmp, rom, 0x20000);
		if (rom_size > 0) {
			verbose("Using ATI Video BIOS File %s (%d Bytes)\n", tmp, rom_size);
			toFree = true;
			if (rom_size > 0x10000) {
				rom_size = 0x10000; //we dont need rest anyway;
			}
		} else {
			verbose("ERROR: unable to open ATI Video BIOS File %s\n", tmp);
		}
	}
	
	if (rom_size == 0) {
		if (radeon_card_posted(ati_dev)) {		
			bios = (uint8_t *)0x000C0000; // try to dump from legacy space, otherwise can result in 100% fan speed
			toFree = false;
		} else {
			bios = readAtomBIOS(ati_dev); // readAtomBios result in bug on some cards (100% fan speed and black screen)
			toFree = true;
		}
	} else {
		bios = rom;	//going custom rom way
	}
	
	if (bios[0] == 0x55 && bios[1] == 0xaa) {
		bios_size = bios[2] * 512;
		
		struct  pci_rom_pci_header_t *rom_pci_header;
		rom_pci_header = (struct pci_rom_pci_header_t*)(bios + bios[24] + bios[25]*256);
		
		if (rom_pci_header->signature == 0x52494350) {
			if (rom_pci_header->device != ati_dev->device_id) {
				verbose("ATI Bios image (%x) doesnt match card (%x), ignoring\n", rom_pci_header->device, ati_dev->device_id);
			} else {
				if (toFree) {
					verbose("Adding binimage to card %x from mmio space with size %x\n", ati_dev->device_id, bios_size);
				} else {
					verbose("Adding binimage to card %x from legacy space with size %x\n", ati_dev->device_id, bios_size);
				}
				devprop_add_value(device, "ATY,bin_image", bios, bios_size);
			}
		} else {
			verbose("Wrong pci header signature %x\n", rom_pci_header->signature);
		}
	} else {
		verbose("ATI Bios image not found at  %x, content %x %x\n", bios, bios[0], bios[1]);
	}
	
	// get bios version
	uint8_t* version_str = NULL;
	unsigned int i;
	// only search the first 256 bytes
	for(i = 0; i <= 256; i++) {
		if(bios[i] == '1' && bios[i+1] == '1' && bios[i+2] == '3' && bios[i+3] == '-') {
			// 113-  was found, extract bios version
			version_str=bios+i+4;
		}
		if (version_str) 
			sprintf(biosVersion, "%s", version_str);
	}
	sprintf(biosVersion, "113-%s", version_str);
	devprop_add_value(device, "ATY,Rom#", (uint8_t*)biosVersion, strlen(biosVersion) + 1);
	
	if (toFree) {
		free(bios);
	}
	stringdata = malloc(sizeof(uint8_t) * string->length);
	memcpy(stringdata, (uint8_t*)devprop_generate_string(string), string->length);
	stringlength = string->length;
	
	return true;
}