/***************************************************************************

        Pecom driver by Miodrag Milanovic

        08/11/2008 Preliminary driver.

****************************************************************************/

#include "driver.h"
#include "cpu/cdp1802/cdp1802.h"
#include "video/cdp1869.h"
#include "devices/cassette.h"
#include "includes/pecom.h"


/* Address maps */
static ADDRESS_MAP_START(pecom64_mem, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE( 0x0000, 0x3fff ) AM_RAMBANK(1)
	AM_RANGE( 0x4000, 0x7fff ) AM_RAMBANK(2)
    AM_RANGE( 0x8000, 0xbfff ) AM_ROM  // ROM 1
    AM_RANGE( 0xc000, 0xf3ff ) AM_ROM  // ROM 2
    AM_RANGE( 0xf000, 0xf7ff ) AM_RAMBANK(3) // CDP1869 / ROM
    AM_RANGE( 0xf800, 0xffff ) AM_RAMBANK(4) // CDP1869 / ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( pecom64_io, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x01, 0x01) AM_WRITE(pecom_bank_w)
	AM_RANGE(0x03, 0x03) AM_READ(pecom_keyboard_r) AM_DEVWRITE(CDP1869_VIDEO, CDP1869_TAG, cdp1869_out3_w)
	AM_RANGE(0x04, 0x04) AM_DEVWRITE(CDP1869_VIDEO, CDP1869_TAG, cdp1869_out4_w)
	AM_RANGE(0x05, 0x05) AM_DEVWRITE(CDP1869_VIDEO, CDP1869_TAG, cdp1869_out5_w)
	AM_RANGE(0x06, 0x06) AM_DEVWRITE(CDP1869_VIDEO, CDP1869_TAG, cdp1869_out6_w)
	AM_RANGE(0x07, 0x07) AM_DEVWRITE(CDP1869_VIDEO, CDP1869_TAG, cdp1869_out7_w)
ADDRESS_MAP_END 

/* Input ports */
static INPUT_PORTS_START( pecom )
	PORT_START("LINE0")
		PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_ENTER)
		PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_LALT) PORT_CODE(KEYCODE_RALT) // LF
	PORT_START("LINE1")
		PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_BACKSPACE) // ESC
		PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_BACKSLASH) // / ? - same key connected as on SLASH
	PORT_START("LINE2")
		PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_0)
		PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_1)
	PORT_START("LINE3")
		PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_2)
		PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_3)
	PORT_START("LINE4")
		PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_4)
		PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_5)
	PORT_START("LINE5")
		PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_6)
		PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_7)
	PORT_START("LINE6")
		PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_8)
		PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_9)
	PORT_START("LINE7")
		PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_QUOTE) // : *
		PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COLON) // ; +
	PORT_START("LINE8")
		PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COMMA) // , <
		PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_EQUALS) // = _
	PORT_START("LINE9")
		PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_STOP) // . >
		PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_SLASH) // / ?
	PORT_START("LINE10")
		PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_SPACE) // space
		PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_A)
	PORT_START("LINE11")
		PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_B)
		PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_C)
	PORT_START("LINE12")
		PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_D)
		PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_E)
	PORT_START("LINE13")
		PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F)
		PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_G)
	PORT_START("LINE14")
		PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_H)
		PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_I)
	PORT_START("LINE15")
		PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_J)
		PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_K)
	PORT_START("LINE16")
		PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_L)
		PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_M)
	PORT_START("LINE17")
		PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_N)
		PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_O)
	PORT_START("LINE18")
		PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_P)
		PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Q)
	PORT_START("LINE19")
		PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_R)
		PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_S)
	PORT_START("LINE20")
		PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_T)
		PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_U)
	PORT_START("LINE21")
		PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_V)
		PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_W)
	PORT_START("LINE22")
		PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_X)
		PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Y)
	PORT_START("LINE23")
		PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Z)
		PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_DOWN) // sh up
	PORT_START("LINE24")
		PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_LEFT) // dj left
		PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_RIGHT) // cj right
	PORT_START("LINE25")
		PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_UP)  //ch up
		PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_DEL) // Del
	PORT_START("CNT")		
		PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_LCONTROL) PORT_CODE(KEYCODE_RCONTROL) // CTL
		PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_RSHIFT) PORT_CODE(KEYCODE_LSHIFT)// Shift Lock
		PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_CAPSLOCK) // CAPS
		PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_ESC) // Break
INPUT_PORTS_END


static SYSTEM_CONFIG_START(pecom64)
	CONFIG_RAM_DEFAULT(32 * 1024)
SYSTEM_CONFIG_END

static const cassette_config pecom_cassette_config =
{
	cassette_default_formats,
	NULL,
	CASSETTE_STOPPED | CASSETTE_MOTOR_ENABLED | CASSETTE_SPEAKER_ENABLED
};

/* Machine driver */
static MACHINE_DRIVER_START( pecom64 )
	MDRV_DRIVER_DATA(pecom_state)
	
    /* basic machine hardware */ 
	MDRV_CPU_ADD("main", CDP1802, CDP1869_DOT_CLK_PAL/3)
	MDRV_CPU_PROGRAM_MAP(pecom64_mem, 0)
  	MDRV_CPU_IO_MAP(pecom64_io, 0)
  	MDRV_CPU_CONFIG(pecom64_cdp1802_config)
  	
  	MDRV_MACHINE_START( pecom )
  	MDRV_MACHINE_RESET( pecom )
		
	// video hardware

	MDRV_IMPORT_FROM(pecom_video)

	// sound hardware	
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD("cdp1869", CDP1869, CDP1869_DOT_CLK_PAL)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
	MDRV_SOUND_ADD("cassette", WAVE, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
	
	MDRV_CASSETTE_ADD( "cassette", pecom_cassette_config )
MACHINE_DRIVER_END

/* ROM definition */
ROM_START( pecom64 )
	  ROM_REGION( 0x10000, "main", ROMREGION_ERASEFF )
	  ROM_SYSTEM_BIOS(0, "ver4", "version 4")
	  ROMX_LOAD( "pecom64-1.bin", 0x8000, 0x4000, CRC(9a433b47) SHA1(dadb8c399e0a25a2693e10e42a2d7fc2ea9ad427), ROM_BIOS(1) )
	  ROMX_LOAD( "pecom64-2.bin", 0xc000, 0x4000, CRC(2116cadc) SHA1(03f11055cd221d438a40a41874af8fba0fa116d9), ROM_BIOS(1) )
	  ROM_SYSTEM_BIOS(1, "ver1", "version 1")
	  ROMX_LOAD( "170887-rom1.bin", 0x8000, 0x4000, CRC(43710fb4) SHA1(f84f75061c9ac3e34af93141ecabd3c955881aa2), ROM_BIOS(2) )
	  ROMX_LOAD( "170887-rom2.bin", 0xc000, 0x4000, CRC(d0d34f08) SHA1(7baab17d1e68771b8dcef97d0fffc655beabef28), ROM_BIOS(2) )
ROM_END

/* Driver */

/*    YEAR  NAME   PARENT  COMPAT  		MACHINE  	INPUT   INIT  	CONFIG	 COMPANY  FULLNAME   	FLAGS */
COMP( 1987, pecom64,     0,      0, 	pecom64, 	pecom, 	pecom, pecom64,  "Ei Nis", "Pecom 64",	0)
