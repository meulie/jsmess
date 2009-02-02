/*
	733 ASR emulation

	We are emulating a TI Model 733 ASR ("Silent 700") data terminal,
	interfaced through a TI asynchronous EIA/TTY interface module.

	The ASR features a printer, a keyboard and a tape unit (which is not
	emulated).  The ASR is attached to the computer with a serial interface.

	References:
	945401-9701 Model 990/4 Computer System Field Maintainance Manual p. C-1,
	945250-9701 990 Computer Family Systems Handbook pp. 5-9 through 5-16,
	0943442-9701 Model 990 Computer Reference Manual Preliminary pp. 3-13
	through 3-21 and 3-39 through 3-44.

	TODO:
	* separate ASR emulation from EIA interface emulation?
	* implement tape interface?

	Raphael Nabet 2003
*/

#include "driver.h"
#include "733_asr.h"

#define MAX_ASR 1

enum
{
	/*ASROutQueueSize = 32,*/

	asr_window_offset_x = 0,
	asr_window_offset_y = 0,
	asr_window_width = 640,
	asr_window_height = 480,
	asr_scroll_step = 8
};

static const rectangle asr_scroll_clear_window =
{
	asr_window_offset_x,									/* min_x */
	asr_window_offset_x+asr_window_width-1,					/* max_x */
	asr_window_offset_y+asr_window_height-asr_scroll_step,	/* min_y */
	asr_window_offset_y+asr_window_height-1					/* max_y */
};

static struct
{
	/*UINT8 OutQueue[ASROutQueueSize];
	int OutQueueHead;
	int OutQueueLen;*/

	UINT8 recv_buf;
	UINT8 xmit_buf;

	UINT8 status;
	UINT8 mode;

	int new_status_flag;

	int x;

	void (*int_callback)(running_machine *, int state);

	bitmap_t *bitmap;
} asr[MAX_ASR];

enum
{
	AS_wrq_mask = 1 << 3,
	AS_rrq_mask = 1 << 4,
	AS_dsr_mask = 1 << 6,
	AS_int_mask = 1 << 7,

	AM_dtr_mask = 1 << 1,
	AM_rts_mask = 1 << 2,
	AM_enint_mask = 1 << 6
};

enum
{
	asrfontdata_size = 96/*128*/*8
};

static const gfx_layout fontlayout =
{
	6, 8,			/* 6*8 characters */
	/*96*/128,				/* 96 characters */
	1,				/* 1 bit per pixel */
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 }, /* straightforward layout */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8 /* every char takes 8 consecutive bytes */
};

GFXDECODE_START( asr733 )
	GFXDECODE_ENTRY( asr733_chr_region, 0, fontlayout, 0, 1 )
GFXDECODE_END

PALETTE_INIT( asr733 )
{
	palette_set_color(machine,0,RGB_WHITE); /* white */
	palette_set_color(machine,1,RGB_BLACK); /* black */
}

/*
	Initialize the asr core
*/
void asr733_init(running_machine *machine)
{
	UINT8 *dst;

	static const unsigned char fontdata6x8[asrfontdata_size] =
	{	/* ASCII characters */
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x20,0x20,0x20,0x20,0x00,0x20,0x00,
		0x50,0x50,0x50,0x00,0x00,0x00,0x00,0x00,0x00,0x50,0xf8,0x50,0xf8,0x50,0x00,0x00,
		0x20,0x70,0xc0,0x70,0x18,0xf0,0x20,0x00,0x40,0xa4,0x48,0x10,0x20,0x48,0x94,0x08,
		0x60,0x90,0xa0,0x40,0xa8,0x90,0x68,0x00,0x10,0x20,0x40,0x00,0x00,0x00,0x00,0x00,
		0x20,0x40,0x40,0x40,0x40,0x40,0x20,0x00,0x10,0x08,0x08,0x08,0x08,0x08,0x10,0x00,
		0x20,0xa8,0x70,0xf8,0x70,0xa8,0x20,0x00,0x00,0x20,0x20,0xf8,0x20,0x20,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x30,0x30,0x60,0x00,0x00,0x00,0xf8,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x30,0x30,0x00,0x00,0x08,0x10,0x20,0x40,0x80,0x00,0x00,
		0x70,0x88,0x88,0x88,0x88,0x88,0x70,0x00,0x10,0x30,0x10,0x10,0x10,0x10,0x10,0x00,
		0x70,0x88,0x08,0x10,0x20,0x40,0xf8,0x00,0x70,0x88,0x08,0x30,0x08,0x88,0x70,0x00,
		0x10,0x30,0x50,0x90,0xf8,0x10,0x10,0x00,0xf8,0x80,0xf0,0x08,0x08,0x88,0x70,0x00,
		0x70,0x80,0xf0,0x88,0x88,0x88,0x70,0x00,0xf8,0x08,0x08,0x10,0x20,0x20,0x20,0x00,
		0x70,0x88,0x88,0x70,0x88,0x88,0x70,0x00,0x70,0x88,0x88,0x88,0x78,0x08,0x70,0x00,
		0x00,0x00,0x30,0x30,0x00,0x30,0x30,0x00,0x00,0x00,0x30,0x30,0x00,0x30,0x30,0x60,
		0x10,0x20,0x40,0x80,0x40,0x20,0x10,0x00,0x00,0x00,0xf8,0x00,0xf8,0x00,0x00,0x00,
		0x40,0x20,0x10,0x08,0x10,0x20,0x40,0x00,0x70,0x88,0x08,0x10,0x20,0x00,0x20,0x00,
		0x70,0x88,0xb8,0xa8,0xb8,0x80,0x70,0x00,0x70,0x88,0x88,0xf8,0x88,0x88,0x88,0x00,
		0xf0,0x88,0x88,0xf0,0x88,0x88,0xf0,0x00,0x70,0x88,0x80,0x80,0x80,0x88,0x70,0x00,
		0xf0,0x88,0x88,0x88,0x88,0x88,0xf0,0x00,0xf8,0x80,0x80,0xf0,0x80,0x80,0xf8,0x00,
		0xf8,0x80,0x80,0xf0,0x80,0x80,0x80,0x00,0x70,0x88,0x80,0x98,0x88,0x88,0x70,0x00,
		0x88,0x88,0x88,0xf8,0x88,0x88,0x88,0x00,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x00,
		0x08,0x08,0x08,0x08,0x88,0x88,0x70,0x00,0x88,0x90,0xa0,0xc0,0xa0,0x90,0x88,0x00,
		0x80,0x80,0x80,0x80,0x80,0x80,0xf8,0x00,0x88,0xd8,0xa8,0x88,0x88,0x88,0x88,0x00,
		0x88,0xc8,0xa8,0x98,0x88,0x88,0x88,0x00,0x70,0x88,0x88,0x88,0x88,0x88,0x70,0x00,
		0xf0,0x88,0x88,0xf0,0x80,0x80,0x80,0x00,0x70,0x88,0x88,0x88,0x88,0x88,0x70,0x08,
		0xf0,0x88,0x88,0xf0,0x88,0x88,0x88,0x00,0x70,0x88,0x80,0x70,0x08,0x88,0x70,0x00,
		0xf8,0x20,0x20,0x20,0x20,0x20,0x20,0x00,0x88,0x88,0x88,0x88,0x88,0x88,0x70,0x00,
		0x88,0x88,0x88,0x88,0x88,0x50,0x20,0x00,0x88,0x88,0x88,0x88,0xa8,0xd8,0x88,0x00,
		0x88,0x50,0x20,0x20,0x20,0x50,0x88,0x00,0x88,0x88,0x88,0x50,0x20,0x20,0x20,0x00,
		0xf8,0x08,0x10,0x20,0x40,0x80,0xf8,0x00,0x30,0x20,0x20,0x20,0x20,0x20,0x30,0x00,
		0x40,0x40,0x20,0x20,0x10,0x10,0x08,0x08,0x30,0x10,0x10,0x10,0x10,0x10,0x30,0x00,
		0x20,0x50,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xfc,
		0x40,0x20,0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x70,0x08,0x78,0x88,0x78,0x00,
		0x80,0x80,0xf0,0x88,0x88,0x88,0xf0,0x00,0x00,0x00,0x70,0x88,0x80,0x80,0x78,0x00,
		0x08,0x08,0x78,0x88,0x88,0x88,0x78,0x00,0x00,0x00,0x70,0x88,0xf8,0x80,0x78,0x00,
		0x18,0x20,0x70,0x20,0x20,0x20,0x20,0x00,0x00,0x00,0x78,0x88,0x88,0x78,0x08,0x70,
		0x80,0x80,0xf0,0x88,0x88,0x88,0x88,0x00,0x20,0x00,0x20,0x20,0x20,0x20,0x20,0x00,
		0x20,0x00,0x20,0x20,0x20,0x20,0x20,0xc0,0x80,0x80,0x90,0xa0,0xe0,0x90,0x88,0x00,
		0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x00,0x00,0x00,0xf0,0xa8,0xa8,0xa8,0xa8,0x00,
		0x00,0x00,0xb0,0xc8,0x88,0x88,0x88,0x00,0x00,0x00,0x70,0x88,0x88,0x88,0x70,0x00,
		0x00,0x00,0xf0,0x88,0x88,0xf0,0x80,0x80,0x00,0x00,0x78,0x88,0x88,0x78,0x08,0x08,
		0x00,0x00,0xb0,0xc8,0x80,0x80,0x80,0x00,0x00,0x00,0x78,0x80,0x70,0x08,0xf0,0x00,
		0x20,0x20,0x70,0x20,0x20,0x20,0x18,0x00,0x00,0x00,0x88,0x88,0x88,0x98,0x68,0x00,
		0x00,0x00,0x88,0x88,0x88,0x50,0x20,0x00,0x00,0x00,0xa8,0xa8,0xa8,0xa8,0x50,0x00,
		0x00,0x00,0x88,0x50,0x20,0x50,0x88,0x00,0x00,0x00,0x88,0x88,0x88,0x78,0x08,0x70,
		0x00,0x00,0xf8,0x10,0x20,0x40,0xf8,0x00,0x08,0x10,0x10,0x20,0x10,0x10,0x08,0x00,
		0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x40,0x20,0x20,0x10,0x20,0x20,0x40,0x00,
		0x00,0x68,0xb0,0x00,0x00,0x00,0x00,0x00,0x20,0x50,0x20,0x50,0xa8,0x50,0x00,0x00
	};

	dst = memory_region(machine, asr733_chr_region);

	memcpy(dst, fontdata6x8, asrfontdata_size);
}

int asr733_init_term(running_machine *machine, int unit, void (*int_callback)(running_machine *, int state))
{
	const device_config *screen = video_screen_first(machine->config);
	int width = video_screen_get_width(screen);
	int height = video_screen_get_height(screen);
	const rectangle *visarea = video_screen_get_visible_area(screen);

	asr[unit].bitmap = auto_bitmap_alloc(width, height, BITMAP_FORMAT_INDEXED16);

	bitmap_fill(asr[unit].bitmap, visarea, 0);

	asr[unit].int_callback = int_callback;

	return 0;
}

static void asr_field_interrupt(running_machine *machine, int unit)
{
	if ((asr[unit].mode & AM_enint_mask) && (asr[unit].new_status_flag))	/* right??? */
	{
		asr[unit].status |= AS_int_mask;
		if (asr[unit].int_callback)
			(*asr[unit].int_callback)(machine, 1);
	}
	else
	{
		asr[unit].status &= ~AS_int_mask;
		if (asr[unit].int_callback)
			(*asr[unit].int_callback)(machine, 0);
	}
}

void asr733_reset(running_machine *machine, int unit)
{
	/*asr[unit].OutQueueLen = 0;*/

	asr[unit].status = AS_dsr_mask | AS_wrq_mask;
	asr[unit].mode = 0;

	asr_field_interrupt(machine, unit);
}

/* write a single char on screen */
static void asr_draw_char(running_machine *machine, int unit, int character, int x, int y, int color)
{
	drawgfx(asr[unit].bitmap, machine->gfx[0], character-32, color, 0, 0,
				x+1, y, NULL, /*TRANSPARENCY_PEN*/TRANSPARENCY_NONE, 0);
}

static void asr_linefeed(running_machine *machine, int unit)
{
	UINT8 buf[asr_window_width];
	int y;

	for (y=asr_window_offset_y; y<asr_window_offset_y+asr_window_height-asr_scroll_step; y++)
	{
		extract_scanline8(asr[unit].bitmap, asr_window_offset_x, y+asr_scroll_step, asr_window_width, buf);
		draw_scanline8(asr[unit].bitmap, asr_window_offset_x, y, asr_window_width, buf, machine->pens);
	}

	bitmap_fill(asr[unit].bitmap, &asr_scroll_clear_window, 0);
}

static void asr_transmit(running_machine *machine, int unit, UINT8 data)
{
	switch (data)
	{
	/* aux device control chars */
	case 0x05:
		/* ENQ -> "WRU": ??? */
		break;

	case 0x11:
		/* DC1 -> "X-ON": transmit on??? */
		break;

	case 0x12:
		/* DC2 -> "X-OFF": transmit off??? */
		break;

	case 0x13:
		/* DC3 -> "TAPE": tape on??? */
		break;

	case 0x14:
		/* DC4 -> "-T-A-P-E-" ("TAPE" with overstrike): tape off??? */
		break;


	/* printer control chars */
	case 0x07:
		/* BELL: 250ms beep */
		break;

	case 0x08:
		/* BS: backspace */
		if (asr[unit].x > 0)
			asr[unit].x--;
		break;

	case 0x0A:
		/* LF: line feed */
		asr_linefeed(machine, unit);
		break;

	case 0x0D:
		/* CR: carriage return */
		asr[unit].x = 0;
		break;


	default:
		if ((data < 0x20) || (data == 0x7f) || (data >= 0x80))
			/* ignore control characters */
			break;

		if (asr[unit].x == 80)
		{
			asr[unit].x = 0;
			asr_linefeed(machine, unit);
		}
		asr_draw_char(machine, unit, data, asr_window_offset_x+asr[unit].x*8, asr_window_offset_y+asr_window_height-8, 0);
		asr[unit].x++;
		break;
	}

	asr[unit].status |= AS_wrq_mask;
	asr[unit].new_status_flag = 1;	/* right??? */
	asr_field_interrupt(machine, unit);
}

#if 0
static void asr_receive_callback(int dummy)
{
	(void) dummy;

	ASR.recv_buf = ASR.OutQueue[ASR.OutQueueHead];
	ASR.OutQueueHead = (ASR.OutQueueHead + 1) % ASROutQueueSize;
	ASR.OutQueueLen--;

	ASR.status |= AS_rrq_mask;
	ASR.new_status_flag = 1;	/* right??? */
	asr_field_interrupt(machine, 0);
}
#endif

/*
	0-7: receive buffer
	8: XMITING transmit in progress, 1 if transmitting
	9: TIMERR timing error, 1 if error
	10: RCR reverse channel receive, not used
	    "ASR733/33 ID" 1 -> TTY (???) (2270509-9701 pp. G-9 & G-10)
	11: WRQ write request, 1 if ready to transmit
	12: RRQ read request, 1 if ready to receive
	13: DCD data carrier detect, not used
	14: DSR data set ready, 1 if online
	15: INT interrupt, 1 if interrupt
*/
static int asr733_cru_r(int offset, int unit)
{
	int reply = 0;

	switch (offset)
	{
	case 0:
		/* receive buffer */
		reply = asr[unit].recv_buf;
		break;

	case 1:
		/* status register */
		reply = asr[unit].status;
		break;
	}

	return reply;
}

/*
	0-7: transmit buffer
	8: not used
	9: DTR data terminal ready (set to 1)
	10: RTS request to send (set to 1)
	11: CLRWRQ clear write request (write any value to execute)
	12: CLRRRQ clear read request (write any value to execute)
	13: CLRNSF clear new status flag - clear DSR/DCD interrupts (write any value to execute)
	14: enable interrupts, 1 to enable interrupts
	15: diagnostic mode, 0 for normal mode
*/
static void asr733_cru_w(running_machine *machine, int offset, int data, int unit)
{
	switch (offset)
	{
	case 0:
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
	case 6:
	case 7:
		/* transmit buffer */
		if (data)
			asr[unit].xmit_buf |= 1 << offset;
		else
			asr[unit].xmit_buf &= ~ (1 << offset);
		if ((offset == 7) && (asr[unit].mode & AM_dtr_mask) && (asr[unit].mode & AM_rts_mask))	/* right??? */
			asr_transmit(machine, unit, asr[unit].xmit_buf);
		break;

	case 8:		/* not used */
		break;

	case 9:		/* data terminal ready (set to 1) */
	case 10:	/* request to send (set to 1) */
	case 14:	/* enable interrupts, 1 to enable interrupts */
	case 15:	/* diagnostic mode, 0 for normal mode */
		if (data)
			asr[unit].mode |= 1 << (offset - 8);
		else
			asr[unit].mode &= ~ (1 << (offset - 8));
		if (offset == 14)
			asr_field_interrupt(machine, unit);
		break;

	case 11:	/* clear write request (write any value to execute) */
	case 12:	/* clear read request (write any value to execute) */
		asr[unit].status &= ~ (1 << (offset - 8));
		asr_field_interrupt(machine, unit);
		break;

	case 13:	/* clear new status flag - whatever it means (write any value to execute) */
		asr[unit].new_status_flag = 0;
		asr_field_interrupt(machine, unit);
		break;
	}
}

 READ8_HANDLER(asr733_0_cru_r)
{
	return asr733_cru_r(offset, 0);
}

WRITE8_HANDLER(asr733_0_cru_w)
{
	asr733_cru_w(space->machine, offset, data, 0);
}


/*
	Video refresh
*/
void asr733_refresh(running_machine *machine, bitmap_t *bitmap, int unit, int x, int y)
{
	copybitmap(bitmap, asr[unit].bitmap, 0, 0, x, y, NULL);
}


static const unsigned char key_translate[3][51] =
{
	{	/* unshifted */
		'1',
		'2',
		'3',
		'4',
		'5',
		'6',
		'7',
		'8',
		'9',
		'0',
		':',
		'-',

		0x1b,
		'Q',
		'W',
		'E',
		'R',
		'T',
		'Y',
		'U',
		'I',
		'O',
		'P',
		0x0a,
		0x0d,

		0,
		'A',
		'S',
		'D',
		'F',
		'G',
		'H',
		'J',
		'K',
		'L',
		';',
		0x08,
		0,

		0,
		'Z',
		'X',
		'C',
		'V',
		'B',
		'N',
		'M',
		',',
		'.',
		'/',
		0,

		' '
	},
	{	/* shifted */
		'!',
		'"',
		'#',
		'$',
		'%',
		'&',
		'^',
		'(',
		')',
		' ',
		'*',
		'=',

		0x1b,
		'Q',
		'W',
		'E',
		'R',
		'T',
		'Y',
		'U',
		'I',
		'_',
		'@',
		0x0a,
		0x0d,

		0,
		'A',
		'S',
		'D',
		'F',
		'G',
		'H',
		'J',
		0,
		'/',
		'+',
		0x08,
		0,

		0,
		'Z',
		'X',
		'C',
		'V',
		'B',
		'^',
		'|',
		'<',
		'>',
		'?',
		0,

		' '
	},
	{	/* control */
		'1',
		'2',
		'3',
		'4',
		'5',
		'6',
		'7',
		'8',
		'9',
		'0',
		':',
		'-',

		0x1b,
		0x11,
		0x17,
		0x05,
		0x12,
		0x14,
		0x19,
		0x15,
		0x09,
		0x0f,
		0x10,
		0x0a,
		0x0d,

		0,
		0x01,
		0x13,
		0x04,
		0x06,
		0x07,
		0x08,
		0x0a,
		0x0b,
		0x0c,
		';',
		0x08,
		0,

		0,
		0x1a,
		0x18,
		0x03,
		0x16,
		0x02,
		0x0e,
		0x0d,
		',',
		'.',
		'/',
		0,

		' '
	}
};


/*
	keyboard handler: should be called regularly by machine code, for instance
	every Video Blank Interrupt.
*/
void asr733_keyboard(running_machine *machine, int unit)
{
	typedef enum
	{
		/* key modifier states */
		unshifted = 0, shift, control,
		/* special value to stop repeat if the modifier state changes */
		special_debounce = -1
	} modifier_state_t;

	static unsigned char repeat_timer;
	enum { repeat_delay = 5 /* approx. 1/10s */ };
	static UINT8 last_key_pressed = 0x80;
	static modifier_state_t last_modifier_state;

	UINT16 key_buf[6];
	int i, j;
	modifier_state_t modifier_state;
	int repeat_mode;

	static const char *const keynames[] = { "KEY0", "KEY1", "KEY2", "KEY3" };

	/* read current key state */
	/* 2008-05 FP: in 733_asr.h there are only 4 input ports defined... */
	/* for (i = 0; i < 6; i++) */
	for (i = 0; i < 4; i++)
	{
		key_buf[i] = input_port_read(machine, keynames[i]);
	}

	/* process key modifiers */
	if (key_buf[1] & 0x0200)
		modifier_state = control;
	else if ((key_buf[2] & 0x0040) || (key_buf[3] & 0x0002))
		modifier_state = shift;
	else
		modifier_state = unshifted;

	/* test repeat key */
	repeat_mode = key_buf[2] & 0x0020;

	/* remove modifier keys */
	key_buf[1] &= ~0x0200;
	key_buf[2] &= ~0x0060;
	key_buf[3] &= ~0x0002;

	if (! repeat_mode)
		/* reset REPEAT timer if the REPEAT key is not pressed */
		repeat_timer = 0;

	if ( ! (last_key_pressed & 0x80) && (key_buf[last_key_pressed >> 4] & (1 << (last_key_pressed & 0xf))))
	{
		/* last key has not been released */
		if (modifier_state == last_modifier_state)
		{
			/* handle REPEAT mode if applicable */
			if ((repeat_mode) && (++repeat_timer == repeat_delay))
			{
				if (asr[unit].status & AS_rrq_mask)
				{	/* keyboard buffer full */
					repeat_timer--;
				}
				else
				{	/* repeat current key */
					asr[unit].status |= AS_rrq_mask;
					asr[unit].new_status_flag = 1;	/* right??? */
					asr_field_interrupt(machine, unit);
					repeat_timer = 0;
				}
			}
		}
		else
		{
			repeat_timer = 0;
			last_modifier_state = special_debounce;
		}
	}
	else
	{
		last_key_pressed = 0x80;

		if (asr[unit].status & AS_rrq_mask)
		{	/* keyboard buffer full */
			/* do nothing */
		}
		else
		{
			for (i=0; i<6; i++)
			{
				for (j=0; j<16; j++)
				{
					if (key_buf[i] & (1 << j))
					{
						last_key_pressed = (i << 4) | j;
						last_modifier_state = modifier_state;

						asr[unit].recv_buf = (int)key_translate[modifier_state][last_key_pressed];
						asr[unit].status |= AS_rrq_mask;
						asr[unit].new_status_flag = 1;	/* right??? */
						asr_field_interrupt(machine, unit);
						return;
					}
				}
			}
		}
	}
}
