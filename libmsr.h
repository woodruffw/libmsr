/* Everyone must include libmsr.h or they're doing it wrong! */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdint.h>

/* Error codes, to be returned from library functions. */

#define LIBMSR_ERR_OK 0x0 /* no error */
#define LIBMSR_ERR_GENERIC 0x1000 /* generic errors */
#define LIBMSR_ERR_ISO 0x1100 /* errors with ISO formatted cards */
#define LIBMSR_ERR_DEVICE 0x2000 /* errors in device control */
#define LIBMSR_ERR_SERIAL 0x4000 /* errors in serial I/O */


/*
 * Track lengths when doing raw accesses can be at most 256 byte
 * in size, since the size field is only 8 bits wide. So we use this
 * as our maximum size.
 */

#define MSR_MAX_TRACK_LEN 255
#define MSR_MAX_TRACKS 3
#define MSR_BLOCKING O_NONBLOCK
#define MSR_BAUD B9600

/* MSR206 definitions, moved from msr206.h. */

/* ESC is frequently used as a start delimiter character */

#define MSR_ESC	0x1B /* Escape character */

/* ASCII file separator character is used to separate track data */

#define MSR_FS 0x1C /* File separator */

#define MSR_STS_OK 0x30 /* Ok */
#define MSR_STS_ERR 0x41 /* General error */

typedef struct msr_cmd {
	uint8_t msr_esc;
	uint8_t msr_cmd;
} msr_cmd_t;

/* Read/write commands */

#define MSR_CMD_READ 0x72 /* Formatted read */
#define MSR_CMD_WRITE 0x77 /* Formatted write */
#define MSR_CMD_RAW_READ 0x6D /* Raw read */
#define MSR_CMD_RAW_WRITE 0x6E /* Raw write */

/* Status byte values from read/write commands */

#define MSR_STS_RW_ERR 0x31	/* Read/write error */
#define MSR_STS_RW_CMDFMT_ERR 0x32 /* Command format error */
#define MSR_STS_RW_CMDBAD_ERR 0x34 /* Invalid command */
#define MSR_STS_RW_SWIPEBAD_ERR 0x39 /* Invalud card swipe in write mode */

typedef struct msr_end {
	uint8_t msr_enddelim;
	uint8_t msr_fs;
	uint8_t msr_esc;
	uint8_t msr_sts;
} msr_end_t;

/*
 * Read/write start and end delimiters.
 * The empty delimiter occurs when reading a track with no data.
 */

#define MSR_RW_START 0x73 /* 's' */
#define MSR_RW_END 0x3F /* '?' */
#define MSR_RW_BAD 0x2A /* '*' */
#define MSR_RW_EMPTY 0x2B /* '+' */

/*
 * Serial port communications test
 * If serial communications are working properly, the device
 * should respond with a 'y' command.
 */

#define MSR_CMD_DIAG_COMM 0x65 /* Communications test */
#define MSR_STS_COMM_OK 0x79

/*
 * Sensor diagnostic command. Will respond with MSR_STS_OK once
 * a card swipe is detected. Can be interrupted by a reset.
 */

#define MSR_CMD_DIAG_SENSOR 0x86 /* Card sensor test */
#define MSR_STS_SENSOR_OK MSR_STS_OK

/*
 * RAM diagnostic command. Will return MSR_STS_OK if RAM checks
 * good, otherwise MSR_STS_ERR.
 */

#define MSR_CMD_DIAG_RAM 0x87 /* RAM test */
#define MSR_STS_RAM_OK MSR_STS_OK
#define MSR_STS_RAM_ERR MSR_STS_ERR

/*
 * Set leading zero count. Responds with MSR_STS_OK if values
 * set ok, otherwise MSR_STS_ERR
 */

#define MSR_CMD_SLZ	0x7A /* Set leading zeros */
#define MSR_STS_SLZ_OK MSR_STS_OK
#define MSR_STS_SLZ_ERR	MSR_STS_ERR

/*
 * Get leading zero count. Returns leading zero counts for
 * track 1/3 and 2.
 */

#define MSR_CMD_CLZ 0x6C /* Check leading zeros */

typedef struct msr_lz {
	uint8_t msr_esc;
	uint8_t msr_lz_tk1_3;
	uint8_t msr_lz_tk2;
} msr_lz_t;

/*
 * Erase card tracks. Returns MSR_STS_OK on success or
 * MSR_STS_ERR.
 */

#define MSR_CMD_ERASE 0x63 /* Erase card tracks */
#define MSR_STS_ERASE_OK MSR_STS_OK
#define MSR_STS_ERASE_ERR MSR_STS_ERR

#define MSR_ERASE_TK1 0x00
#define MSR_ERASE_TK2 0x02
#define MSR_ERASE_TK3 0x04
#define MSR_ERASE_TK1_TK2 0x03
#define MSR_ERASE_TK1_TK3 0x05
#define MSR_ERASE_TK2_TK3 0x06
#define MSR_ERASE_ALL 0x07

/*
 * Set bits per inch. Returns MSR_STS_OK on success or
 * MSR_STS_ERR.
 */

#define MSR_CMD_SETBPI 0x62 /* Set bits per inch */
#define MSR_STS_BPI_OK MSR_STS_OK
#define MSR_STS_BPI_ERR MSR_STS_ERR

/*
 * Get device model number. Returns a value indicating a model
 * number, plus an 'S'.
 */

#define MSR_CMD_MODEL 0x74 /* Read model */
#define MSR_STS_MODEL_OK 0x53

#define MSR_MODEL_MSR206_1 0x31
#define MSR_MODEL_MSR206_2 0x32
#define MSR_MODEL_MSR206_3 0x33
#define MSR_MODEL_MSR206_5 0x35

typedef struct msr_model {
	uint8_t msr_esc;
	uint8_t msr_model;
	uint8_t msr_s;
} msr_model_t;

/*
 * Get firmware revision. Response is a string in
 * the form of "REV?X.XX" where X.XX is the firmware
 * rev, and ? can be:
 *
 * MSR206: '0'
 * MSR206HC: 'H'
 * MSR206HC: 'L'
 */

#define MSR_CMD_FWREV 0x76 /* Read firmware revision */
#define MSR_FWREV_FMT "REV?X.XX"

/*
 * Set bits per character. Returns MSR_STS_OK on success, accompanied
 * by resulting per-track BPC settings.
 */

#define MSR_CMD_SETBPC 0x6F /* Set bits per character */
#define MSR_STS_BPC_OK MSR_STS_OK
#define MSR_STS_BPC_ERR MSR_STS_ERR

typedef struct msr_bpc {
	uint8_t msr_bpctk1;
	uint8_t msr_bpctk2;
	uint8_t msr_bpctk3;
} msr_bpc_t;

/*
 * Set coercivity high or low. Returns MSR_STS_OK on success.
 */

#define MSR_CMD_SETCO_HI 0x78 /* Set coercivity high */
#define MSR_CMD_SETCO_LO 0x79 /* Set coercivity low */
#define MSR_STS_CO_OK MSR_STS_OK
#define MSR_STS_CO_ERR MSR_STS_ERR

/*
 * Get coercivity. Returns 'h' for high coercivity, 'l' for low.
 * NOTE: The user manual lies here, the returns are lower case!
 */

#define MSR_CMD_GETCO 0x64 /* Read coercivity setting */
#define MSR_CO_HI 0x68
#define MSR_CO_LO 0x6C

/* The following commands have no response codes */

#define MSR_CMD_RESET 0x61 /* Reset device */
#define MSR_CMD_LED_OFF 0x81 /* All LEDs off */
#define MSR_CMD_LED_ON 0x82 /* All LEDs on */
#define MSR_CMD_LED_GRN_ON 0x83 /* Green LED on */
#define MSR_CMD_LED_YLW_ON 0x84 /* Yellow LED on */
#define MSR_CMD_LED_RED_ON 0x85 /* Red LED on */

typedef struct msr_track {
	uint8_t msr_tk_data[MSR_MAX_TRACK_LEN];
	uint8_t msr_tk_len;
} msr_track_t;

typedef struct msr_tracks {
	msr_track_t	msr_tracks[MSR_MAX_TRACKS];
} msr_tracks_t;

/* Makstripe definitions, moved from makstripe.h. */

/* This is an attempt at reversing the MAKStripe usb device magic numbers. */
/* It appears to be a very similar protocol to the MSR-206. */
/* The device may have as many as 20 possible card read buffers internally? */
/* We have confirmed only a single read buffer; it appears to be clobbered */
/* by all subsequent reads. */
/* Please see the README.MAKStripe file for more information. */

/*
 * Read (populate buffer from card, and show us the data):
 * Send: R<0x7>
 * Response: Ready
 * <swipe card>
 * Response: RD<0xXX><0xYY><0x20><data>RD=OK
 */

/*
 * Populate buffer from host:
 * Send: X<number of bytes><0x7>
 * Response: WB<space>
 * Send: data samples
 * Response: WB=OK
 */

/*
 * Populate buffer from card (read, but don't show us the data)
 * Send: W
 * Response: RA
 * <swipe card>
 * Response: RA=OK
 */

/*
 * Copy buffer (copy data to card card)
 * Send: C<0x7>
 * Response: CP<space>
 * <swipe card>
 * Response: CP=OK
 */

/* This is the basic way to send a command */
/* This appears to be the way that R/W/S/C operate. */
typedef struct mak_cmd {
	uint8_t mak_cmd;
	uint8_t mak_track_mask;
} mak_generic_cmd_t;

typedef struct mak_cmd_load_buf {
	uint8_t mak_cmd;
	uint16_t mak_len;
} mak_load_buf_t;

/* This is for the erase/eRase commands. */
typedef struct mak_cmd_erase {
	uint8_t mak_cmd;
	uint8_t mak_track_mask;
	uint8_t mak_wtf;
} mak_cmd_erase_t;

/* This is the byte sent as the suffix for many commands. */
#define MAK_ESC 0x04 /* The bits formerly known as <EOT> */

/* For proper serial setup. */
/* The MAKStripe requires blocking IO! You will facepalm! */
#define MAK_BLOCKING O_NONBLOCK
#define MAK_BAUD B38400

/* This command is possibly a command that resets the MAKStripe. */
/* It appears that after sending this command, the device prints some data. */
/* At first, we thought that this might be the firmware query command. */
/* However, it appears that this is used to cancel operations in progress */
/* additionally, it's used at other times. */
/* It is likely an unintended consequence that this produces a firmware */
/* version string. It probably does this because this command resets the */
/* device and it prints a boot loader or something to its serial port. */
#define MAK_FIRMWARE_QUERY_CMD '?' /* ?<MAK_ESC>*/
/* The response is the firmware information. XXX TODO: Don't hardcode... */
#define MAK_FIRMWARE_QUERY_RESP "MSUSB CI.270209"
#define MAK_FIRMWARE_QUERY_STS_OK /* UNKNOWN */
#define MAK_FIRMWARE_QUERY_STS_ERR /* UNKNOWN */

#define MAK_RESET_CMD '?'
#define MAK_RESET_RESP MAK_FIRMWARE_QUERY_RESP

/* Populate the buffer in the MAKStripe from the reader head. */
/* Returns populated data from the buffer in the MAKStripe to the host computer. */
#define MAKSTRIPE_READ_CMD 'R' /* R<MAK_ESC> */
#define MAKSTRIPE_READ_RESP "Ready " /* Sing it: "One of these things is not like the others..." */
/* Swipe a card here and wait for data. */
/* Sample data follows and ends with the status response. */
/* Sample data format is as follows: 'RD '<16bits of length data><data samples> */
#define MAKSTRIPE_READ_BUF_PREFIX "RD "
#define MAKSTRIPE_READ_STS_OK "RD=OK"
#define MAKSTRIPE_READ_STS_ERR /* UNKNOWN */

/* Populate the buffer in the MAKStripe from the host computer. */
/* Data is packed in an unknown format as of yet. */
#define MAKSTRIPE_POPULATE_BUF_CMD 'X' /* X<num of bytes><MAK_ESC> */
#define MAKSTRIPE_POPULATE_BUF_RESP "WB " /* Acknowledge that MAKStripe is ready for data. */
/* Now write out bytes to <fd> */
#define MAKSTRIPE_POPULATE_BUF_OK "WB=OK" /* Literal "WB=OK" */
#define MAKSTRIPE_POPULATE_BUF_ERR /* UNKNOWN */

/* Show what's in the device buffer. */
#define MAKSTRIPE_SHOW_BUFFER_CMD 'S' /*S<MAK_ESC>*/
#define MAKSTRIPE_SHOW_BUFFER_RESP /* The response is the buffered data. */
#define MAKSTRIPE_SHOW_BUFFER_STS_OK "RB=1 OK"
#define MAKSTRIPE_SHOW_BUFFER_STS_ERR /* UNKNOWN */

/* Undefined as of yet but appears to be a valid command byte. */
#define MAKSTRIPE_WRITE_BUF_CMD 'W' /* W<MAK_ESC> */
#define MAKSTRIPE_WRITE_BUF_RESP /* UNKNOWN */
#define MAKSTRIPE_WRITE_BUF_STS_OK "WB "
#define MAKSTRIPE_WRITE_BUF_STS_ERR /* UNKNOWN */

/* It's possible to swipe a reference card and then issue a clone command. */
/* It appears to buffer the reference card in the device and then write it */
/* to the next. MAKSTRIPE_CLONE esentially copies the buffer onto the card. */
/* Cloning steps: Issue MAKSTRIPE_READ and follow it with MAKSTRIPE_CLONE */
#define MAKSTRIPE_CLONE_CMD 'C' /* W<MAK_ESC> */
#define MAKSTRIPE_CLONE_RESP "CP "
#define MAKSTRIPE_CLONE_STS_OK "CP=OK" /* Really pedobear? Party van is on the way! */
#define MAKSTRIPE_CLONE_STS_ERR /* UNKNOWN */

/* These are the generic ways that we can expect to commonly discuss a track */
#define MAKSTRIPE_TK1	0x01
#define MAKSTRIPE_TK2	0x02
#define MAKSTRIPE_TK3	0x04
#define MAKSTRIPE_TK_ALL (MAKSTRIPE_TK1 | MAKSTRIPE_TK2 | MAKSTRIPE_TK3)

/*
* These are the magic bytes for the format command
* The format command seems to be an 'F' followed by a single byte
* track mask, followed by " d" (space, lower case d). It's unclear
* what the " d" means.
*/

#define MAKSTRIPE_FMT_CMD 'F' /* F<MAKSTRIPE_FMT_TK1>" d" */
#define MAKSTRIPE_FMT_RESP "FM "
#define MAKSTRIPE_FMT_OK "FM=OK"
#define MAKSTRIPE_FMT_ERR /* UNKNOWN */
#define MAKSTRIPE_FMT_TK1 MAKSTRIPE_TK1
#define MAKSTRIPE_FMT_TK2 MAKSTRIPE_TK2
#define MAKSTRIPE_FMT_TK3 MAKSTRIPE_TK3
#define MAKSTRIPE_FMT_TK1_TK2 MAKSTRIPE_TK1 | MAKSTRIPE_TK2 /* Should be: 0x03 */
#define MAKSTRIPE_FMT_TK1_TK3 MAKSTRIPE_TK1 | MAKSTRIPE_TK3 /* Should be: 0x05 */
#define MAKSTRIPE_FMT_TK2_TK3 MAKSTRIPE_TK2 | MAKSTRIPE_TK3 /* Should be: 0x06 */
#define MAKSTRIPE_FMT_ALL MAKSTRIPE_TK_ALL /*  etc: 0x07 */

/* These are the magic bytes for the Erase command */
/* These are the low flux bit erase commands */
/* "Erase selected tracks in FLUX 0 direction." */
#define MAKSTRIPE_ErASE_CMD 'E' /* E<MAKSTRIPE_FMT_TK1><MAK_ESC> XXX: Confirm with usb dump */
#define MAKSTRIPE_ErASE_RESP "Er "
#define MAKSTRIPE_ErASE_OK "Er=OK"
#define MAKSTRIPE_ErASE_ERR /* UNKNOWN */
#define MAKSTRIPE_ErASE_TK1 MAKSTRIPE_TK1
#define MAKSTRIPE_ErASE_TK2 MAKSTRIPE_TK2
#define MAKSTRIPE_ErASE_TK3 MAKSTRIPE_TK3
#define MAKSTRIPE_ErASE_TK1_TK2 MAKSTRIPE_TK1 | MAKSTRIPE_TK2 /* Should be: 0x03 */
#define MAKSTRIPE_ErASE_TK1_TK3 MAKSTRIPE_TK1 | MAKSTRIPE_TK3 /* Should be: 0x05 */
#define MAKSTRIPE_ErASE_TK2_TK3 MAKSTRIPE_TK2 | MAKSTRIPE_TK3 /* Should be: 0x06 */
#define MAKSTRIPE_ErASE_ALL MAKSTRIPE_TK1 | MAKSTRIPE_TK2 | MAKSTRIPE_TK3 /*  etc: 0x07 */

/* These are the magic bytes for the eRase command */
/* These are the high flux bit erase commands */
/* "Erase selected tracks in FLUX 1 direction." */
#define MAKSTRIPE_eRASE_CMD 'e' /* e<MAKSTRIPE_FMT_TK1><MAK_ESC> XXX: Confirm with usb dump */
#define MAKSTRIPE_eRASE_RESP "eR "
#define MAKSTRIPE_eRASE_OK "eR=OK"
#define MAKSTRIPE_eRASE_ERR /* UNKNOWN */
#define MAKSTRIPE_eRASE_TK1 MAKSTRIPE_TK1
#define MAKSTRIPE_eRASE_TK2 MAKSTRIPE_TK2
#define MAKSTRIPE_eRASE_TK3 MAKSTRIPE_TK3
#define MAKSTRIPE_eRASE_TK1_TK2 MAKSTRIPE_TK1 | MAKSTRIPE_TK2 /* Should be: 0x03 */
#define MAKSTRIPE_eRASE_TK1_TK3 MAKSTRIPE_TK1 | MAKSTRIPE_TK3 /* Should be: 0x05 */
#define MAKSTRIPE_eRASE_TK2_TK3 MAKSTRIPE_TK2 | MAKSTRIPE_TK3 /* Should be: 0x06 */
#define MAKSTRIPE_eRASE_ALL MAKSTRIPE_TK1 | MAKSTRIPE_TK2 | MAKSTRIPE_TK3 /*  etc: 0x07 */

/*
 * Serial I/O functions, moved from serialio.h and prefixed with msr_.
 */
extern int msr_serial_open (char *, int *,  int, speed_t);
extern int msr_serial_close (int);
extern int msr_serial_readchar (int, uint8_t *);
extern int msr_serial_write (int, void *, size_t);
extern int msr_serial_read (int, void *, size_t);

extern int msr_zeros (int);
extern int msr_commtest (int);
extern int msr_init (int);
extern int msr_fwrev (int);
extern int msr_model (int);
extern int msr_sensor_test (int);
extern int msr_ram_test (int);
extern int msr_get_co(int);
extern int msr_set_hi_co (int);
extern int msr_set_lo_co (int);
extern int msr_reset(int);
extern int msr_iso_read (int, msr_tracks_t *);
extern int msr_iso_write (int, msr_tracks_t *);
extern int msr_raw_read (int, msr_tracks_t *);
extern int msr_raw_write (int, msr_tracks_t *);
extern int msr_erase (int, uint8_t);
extern int msr_flash_led (int, uint8_t);
extern int msr_set_bpi (int, uint8_t);
extern int msr_set_bpc (int, uint8_t, uint8_t, uint8_t);

extern int msr_dumpbits (uint8_t *, int);
extern int msr_getbit (uint8_t *, uint8_t, int);
extern int msr_setbit (uint8_t *, uint8_t, int, int);
extern int msr_decode (uint8_t *, uint8_t, uint8_t *, uint8_t *, int);

extern int msr_reverse_tracks (msr_tracks_t *);
extern int msr_reverse_track (int, msr_tracks_t *);

extern void msr_pretty_output_hex (int fd, msr_tracks_t tracks);
extern void msr_pretty_output_string (int fd, msr_tracks_t tracks);
extern void msr_pretty_output_bits (int fd, msr_tracks_t tracks);

extern void msr_pretty_printer_hex (msr_tracks_t tracks);
extern void msr_pretty_printer_string (msr_tracks_t tracks);
extern void msr_pretty_printer_bits (msr_tracks_t tracks);

extern const unsigned char msr_reverse_byte (const unsigned char);
