/*
    drivers/rmnimbus.c
    
    Research machines Nimbus.
    
    2009-11-14, P.Harvey-Smith.
    
*/

#include "driver.h"
#include "image.h"
#include "cpu/i86/i86.h"
#include "devices/flopdrv.h"
#include "devices/messram.h"
#include "formats/flopimg.h"
#include "formats/pc_dsk.h"
#include "includes/rmnimbus.h"
#include "machine/z80sio.h"

static const unsigned char nimbus_palette[] =
{
	/*normal brightness */
	0x00,0x00,0x00, 	/* black */
	0x00,0x00,0x80,		/* blue */
    0x80,0x00,0x00,		/* red */
    0x80,0x00,0x80,		/* magenta */
	0x00,0x80,0x00, 	/* green */
    0x00,0x80,0x80,		/* cyan */
	0x80,0x80,0x00,		/* yellow */
	0x80,0x80,0x80,		/* light grey */

	/*enhanced brightness*/
	0x40,0x40,0x40, 	/* dark grey */
	0x00,0x00,0xFF,		/* light blue */
	0xFF,0x00,0x00,		/* light red */
	0xFF,0x00,0xFF,		/* light magenta */
	0x00,0xFF,0x00, 	/* light green */
	0x00,0xFF,0xFF,		/* light cyan */
	0xFF,0xFF,0x00,		/* yellow */
	0xFF,0xFF,0xFF		/* white */
};

static const floppy_config nimbus_floppy_config =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_DRIVE_DS_80,
	FLOPPY_OPTIONS_NAME(pc),
	DO_NOT_KEEP_GEOMETRY
};


static ADDRESS_MAP_START(nimbus_mem, ADDRESS_SPACE_PROGRAM, 16)
    AM_RANGE( 0x00000, 0xEFFFF ) AM_RAM
    AM_RANGE( 0xF0000, 0xFFFFF ) AM_ROM 
ADDRESS_MAP_END

static ADDRESS_MAP_START(nimbus_io, ADDRESS_SPACE_IO, 16)
    AM_RANGE( 0x0000, 0x002f) AM_READWRITE( nimbus_video_io_r, nimbus_video_io_w)
    AM_RANGE( 0x0030, 0x00ef) AM_READWRITE( nimbus_io_r, nimbus_io_w)
    AM_RANGE( 0X00f0, 0X00f7) AM_DEVREADWRITE8( Z80SIO_TAG, sio_r, sio_w, 0x00FF)
    AM_RANGE( 0x0400, 0x041f) AM_READWRITE8( nimbus_fdc_r, nimbus_fdc_w, 0x00FF)
	AM_RANGE( 0xff00, 0xffff) AM_READWRITE( i186_internal_port_r, i186_internal_port_w)/* CPU 80186         */
ADDRESS_MAP_END

static INPUT_PORTS_START( nimbus )
	PORT_START("KEY0") /* Key row 0 scancodes 00..07 */
	//PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("1") PORT_CODE(KEYCODE_1) PORT_CHAR('1')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("ESC")    PORT_CODE(KEYCODE_ESC)          PORT_CHAR(0x1B)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("1")      PORT_CODE(KEYCODE_1)            PORT_CHAR('1')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("2")      PORT_CODE(KEYCODE_2)            PORT_CHAR('2')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("3")      PORT_CODE(KEYCODE_3)            PORT_CHAR('3')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("4")      PORT_CODE(KEYCODE_4)            PORT_CHAR('4')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("5")      PORT_CODE(KEYCODE_5)            PORT_CHAR('5')
    PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("6")      PORT_CODE(KEYCODE_6)            PORT_CHAR('6')

	PORT_START("KEY1") /* Key row 1 scancodes 08..0F */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("7")      PORT_CODE(KEYCODE_1)            PORT_CHAR('7')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("8")      PORT_CODE(KEYCODE_ESC)          PORT_CHAR('8')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("9")      PORT_CODE(KEYCODE_1)            PORT_CHAR('9')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("0")      PORT_CODE(KEYCODE_2)            PORT_CHAR('0')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("-")      PORT_CODE(KEYCODE_MINUS)        PORT_CHAR('-')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("=")      PORT_CODE(KEYCODE_EQUALS)       PORT_CHAR('=')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("BSPACE") PORT_CODE(KEYCODE_BACKSPACE)    PORT_CHAR(0x08)
    PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("TAB")    PORT_CODE(KEYCODE_TAB)          PORT_CHAR(0x09)

	PORT_START("KEY2") /* Key row 2 scancodes 10..17 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Q")      PORT_CODE(KEYCODE_Q)            PORT_CHAR('Q')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("W")      PORT_CODE(KEYCODE_W)            PORT_CHAR('W')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("E")      PORT_CODE(KEYCODE_E)            PORT_CHAR('E')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("R")      PORT_CODE(KEYCODE_R)            PORT_CHAR('R')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("T")      PORT_CODE(KEYCODE_T)            PORT_CHAR('T')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Y")      PORT_CODE(KEYCODE_Y)            PORT_CHAR('Y')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("U")      PORT_CODE(KEYCODE_U)            PORT_CHAR('U')
    PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("I")      PORT_CODE(KEYCODE_I)            PORT_CHAR('I')

	PORT_START("KEY3") /* Key row 3 scancodes 18..1F */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("O")      PORT_CODE(KEYCODE_O)            PORT_CHAR('O')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("P")      PORT_CODE(KEYCODE_P)            PORT_CHAR('P')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("[")      PORT_CODE(KEYCODE_OPENBRACE)    PORT_CHAR('[')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("]")      PORT_CODE(KEYCODE_CLOSEBRACE)   PORT_CHAR(']')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("ENTER")  PORT_CODE(KEYCODE_ENTER)        PORT_CHAR(0x0D)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("CTRL")   PORT_CODE(KEYCODE_LCONTROL)     PORT_CODE(KEYCODE_RCONTROL) // Ether control
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("A")      PORT_CODE(KEYCODE_A)            PORT_CHAR('A')
    PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("S")      PORT_CODE(KEYCODE_S)            PORT_CHAR('S')

	PORT_START("KEY4") /* Key row 4 scancodes 20..27 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("D")      PORT_CODE(KEYCODE_D)            PORT_CHAR('D')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F")      PORT_CODE(KEYCODE_F)            PORT_CHAR('F')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("G")      PORT_CODE(KEYCODE_G)            PORT_CHAR('G')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("H")      PORT_CODE(KEYCODE_H)            PORT_CHAR('H')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("J")      PORT_CODE(KEYCODE_J)            PORT_CHAR('J')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("K")      PORT_CODE(KEYCODE_K)            PORT_CHAR('K')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("L")      PORT_CODE(KEYCODE_L)            PORT_CHAR('L')
    PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(";")      PORT_CODE(KEYCODE_QUOTE)        PORT_CHAR(';')

	PORT_START("KEY5") /* Key row 5 scancodes 28..2F */
    PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("#")      PORT_CODE(KEYCODE_QUOTE)        PORT_CHAR('#')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("\\")     PORT_CODE(KEYCODE_BACKSLASH)    PORT_CHAR('\\')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("LSHIFT") PORT_CODE(KEYCODE_LSHIFT)       PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("TILDE")  PORT_CODE(KEYCODE_TILDE)        PORT_CHAR('`')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Z")      PORT_CODE(KEYCODE_Z)            PORT_CHAR('Z')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("X")      PORT_CODE(KEYCODE_X)            PORT_CHAR('X')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("C")      PORT_CODE(KEYCODE_C)            PORT_CHAR('C')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("V")      PORT_CODE(KEYCODE_V)            PORT_CHAR('V')
	

	PORT_START("KEY6") /* Key row 6 scancodes 30..37 */
    PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("B")      PORT_CODE(KEYCODE_B)            PORT_CHAR('B')
    PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("N")      PORT_CODE(KEYCODE_N)            PORT_CHAR('N')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("M")      PORT_CODE(KEYCODE_M)            PORT_CHAR('M')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(",")      PORT_CODE(KEYCODE_COMMA)        PORT_CHAR(',')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(".")      PORT_CODE(KEYCODE_STOP)         PORT_CHAR('.')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("/")      PORT_CODE(KEYCODE_SLASH)        PORT_CHAR('/')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("RSHIFT") PORT_CODE(KEYCODE_RSHIFT)       PORT_CHAR(UCHAR_SHIFT_2)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("PRSCR")  PORT_CODE(KEYCODE_ASTERISK)     PORT_CHAR('*')
	
	PORT_START("KEY7") /* Key row 7 scancodes 38..3F */
    PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("ALT")    PORT_CODE(KEYCODE_LALT)         PORT_CODE(KEYCODE_RALT)
    PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("SPACE")  PORT_CODE(KEYCODE_SPACE)        PORT_CHAR(' ')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("CAPS")   PORT_CODE(KEYCODE_CAPSLOCK)    
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F1")     PORT_CODE(KEYCODE_F1)           
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F2")     PORT_CODE(KEYCODE_F2)           
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F3")     PORT_CODE(KEYCODE_F3)           
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F4")     PORT_CODE(KEYCODE_F4)           
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F5")     PORT_CODE(KEYCODE_F5)           
    
	
    PORT_START("KEY8") /* Key row 8 scancodes 40..47 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F6")     PORT_CODE(KEYCODE_F6)            
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F7")     PORT_CODE(KEYCODE_F7)            
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F8")     PORT_CODE(KEYCODE_F8)            
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F9")     PORT_CODE(KEYCODE_F9)            
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F10")    PORT_CODE(KEYCODE_F10)            
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("NUMLK")  PORT_CODE(KEYCODE_NUMLOCK)      
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("SCRLK")  PORT_CODE(KEYCODE_SCRLOCK)            
    PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("KP7")    PORT_CODE(KEYCODE_7_PAD)        PORT_CHAR('7')

	PORT_START("KEY9") /* Key row 9 scancodes 48..4F */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("KP8")    PORT_CODE(KEYCODE_8_PAD)        PORT_CHAR('8')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("KP9")    PORT_CODE(KEYCODE_9_PAD)        PORT_CHAR('9')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("KP-")    PORT_CODE(KEYCODE_MINUS_PAD)    PORT_CHAR('-')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("KP4")    PORT_CODE(KEYCODE_4_PAD)        PORT_CHAR('4')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("KP5")    PORT_CODE(KEYCODE_5_PAD)        PORT_CHAR('5')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("KP6")    PORT_CODE(KEYCODE_6_PAD)        PORT_CHAR('6')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("KP+")    PORT_CODE(KEYCODE_PLUS_PAD)     PORT_CHAR('+')
    PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("KP1")    PORT_CODE(KEYCODE_1_PAD)        PORT_CHAR('1')

	PORT_START("KEY10") /* Key row 10 scancodes 50..57 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("KP2")    PORT_CODE(KEYCODE_2_PAD)        PORT_CHAR('2')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("KP3")    PORT_CODE(KEYCODE_3_PAD)        PORT_CHAR('3')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("KP0")    PORT_CODE(KEYCODE_0_PAD)        PORT_CHAR('0')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("KP.")    PORT_CODE(KEYCODE_DEL_PAD)      PORT_CHAR('.')
	//PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("KP5")    PORT_CODE(KEYCODE_5_PAD)        PORT_CHAR('5')
	//PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("KP6")    PORT_CODE(KEYCODE_6_PAD)        PORT_CHAR('6')
	//PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("KP+")    PORT_CODE(KEYCODE_PLUS_PAD)     PORT_CHAR('+')
    //PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("KP1")    PORT_CODE(KEYCODE_1_PAD)        PORT_CHAR('1')
INPUT_PORTS_END




static PALETTE_INIT( nimbus )
{
	int i;

	for ( i = 0; i < sizeof(nimbus_palette) / 3; i++ ) {
		palette_set_color_rgb(machine, i, nimbus_palette[i*3], nimbus_palette[i*3+1], nimbus_palette[i*3+2]);
	}
}


static MACHINE_DRIVER_START( nimbus )
    /* basic machine hardware */
    MDRV_CPU_ADD(MAINCPU_TAG, I80186, 10000000)
    MDRV_CPU_PROGRAM_MAP(nimbus_mem)
    MDRV_CPU_IO_MAP(nimbus_io)
 
    MDRV_MACHINE_START( nimbus )
 
    MDRV_MACHINE_RESET(nimbus)
    
    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(100))
    MDRV_VIDEO_ATTRIBUTES(VIDEO_UPDATE_SCANLINE)
    
    MDRV_PALETTE_LENGTH(ARRAY_LENGTH(nimbus_palette) / 3)
	MDRV_PALETTE_INIT( nimbus )
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)

    MDRV_SCREEN_SIZE(650, 260)
    MDRV_SCREEN_VISIBLE_AREA(0, 639, 0, 249)
 
    MDRV_VIDEO_START(nimbus)
    MDRV_VIDEO_UPDATE(nimbus)
    MDRV_VIDEO_EOF(nimbus)


	MDRV_WD2793_ADD(FDC_TAG, nimbus_wd17xx_interface )
	MDRV_FLOPPY_4_DRIVES_ADD(nimbus_floppy_config)
    
    MDRV_RAM_ADD("messram")
	MDRV_RAM_DEFAULT_SIZE("512K")
	MDRV_RAM_EXTRA_OPTIONS("1024K,1536K")
    
    /* Peripheral chips */
    MDRV_Z80SIO_ADD(Z80SIO_TAG, 4000000, sio_intf)
MACHINE_DRIVER_END


ROM_START( nimbus )
    ROM_REGION( 0x100000, MAINCPU_TAG, 0 )
    
//  ROM_LOAD16_BYTE("sys1-1.31a-16128-1986-06-18.rom", 0x0001, 0x8000, CRC(6416eb05) SHA1(1b640163a7efbc24381c7b24976a8609c066959b))
//	ROM_RELOAD(0xf0001,0x8000)
//  ROM_LOAD16_BYTE("sys2-1.31a-16129-1986-06-18.rom", 0x0000, 0x8000, CRC(b224359d) SHA1(456bbe37afcd4429cca76ba2d6bd534dfda3fc9c))
//	ROM_RELOAD(0xf0000,0x8000)

    ROM_LOAD16_BYTE("sys-1-1.32f-22779-1989-10-20.rom", 0x0001, 0x8000, CRC(786c31e8) SHA1(da7f828f7f96087518bea1a3d89fee59b283b4ba))
	ROM_RELOAD(0xf0001,0x8000)
    ROM_LOAD16_BYTE("sys-2-1.32f-22779-1989-10-20.rom", 0x0000, 0x8000, CRC(0be3db64) SHA1(af806405ec6fbc20385705f90d5059a47de17b08))
	ROM_RELOAD(0xf0000,0x8000)
    
ROM_END

/*    YEAR  NAME        PARENT  COMPAT  MACHINE INPUT   INIT  COMPANY  FULLNAME   FLAGS */
COMP( 1986, nimbus,     0,      0,      nimbus, nimbus, 0,   "Research Machines", "nimbus", GAME_NO_SOUND | GAME_NOT_WORKING)
