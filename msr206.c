#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <strings.h>
#include <err.h>
#include <string.h>

#include "libmsr.h"

/* Thanks Club Mate and h1kari! Toorcon 10 */

int msr_cmd (int fd, uint8_t c)
{
	msr_cmd_t	cmd;

	cmd.msr_esc = MSR_ESC;
	cmd.msr_cmd = c;

	return (msr_serial_write (fd, &cmd, sizeof(cmd)));
}

int msr_zeros (int fd, msr_lz_t *lz)
{
	msr_cmd (fd, MSR_CMD_CLZ);
	msr_serial_read (fd, lz, sizeof(msr_lz_t));

#ifdef DEBUG
	printf("zero13: %d zero: %d\n", lz->msr_lz_tk1_3, lz->msr_lz_tk2);
#endif

	return LIBMSR_ERR_OK;
}

static int getstart (int fd)
{
	uint8_t b;
	int i;

	for (i = 0; i < 3; i++) {
		msr_serial_readchar(fd, &b);
		if (b == MSR_RW_START)
			break;
	}

	if (i == 3)
		return LIBMSR_ERR_ISO;

	return LIBMSR_ERR_OK;
}

/*
 * Read the end of an ISO formatted read response
 *
 * This is a helper routine used by msr_iso_read() to parse the
 * end delimiter returned by the MSR206 in response to an ISO
 * read command. The end delimiter is a sequence of four bytes
 * (handled here as a four byte structure), the last byte of
 * which contains the command status code.
 *
 * This function will fail if the serial port is not initialized
 * or the descriptor <fd> is invalid, or if the status code
 * returned by the device is not MSR_STS_OK.
 */
static int getend (int fd)
{
	msr_end_t m;

	msr_serial_read (fd, &m, sizeof(m));

	if (m.msr_sts != MSR_STS_OK) {
#ifdef DEBUG
		printf ("read returned error status: 0x%x\n", m.msr_sts);
#endif
		return LIBMSR_ERR_DEVICE;
	}

	return LIBMSR_ERR_OK;
}

int msr_commtest (int fd)
{
	int r;
	uint8_t buf[2];

	r = msr_cmd (fd, MSR_CMD_DIAG_COMM);

	if (r == -1) {
#ifdef DEBUG
   		err(1, "Commtest write failed");
#endif
   		return LIBMSR_ERR_SERIAL;
	}

	/*
	 * Read the result. Note: we're supposed to get back
	 * two characters: an escape and a 'y' character. But
	 * with my serial USB adapter, the escape sometimes
	 * gets lost. As a workaround, we scan only for the 'y'
	 * and discard the escape.
	 */

	while (1) {
		msr_serial_readchar (fd, &buf[0]);
		if (buf[0] == MSR_STS_COMM_OK)
			break;
	}

	if (buf[0] != MSR_STS_COMM_OK) {
#ifdef DEBUG
		printf("Communications test failure\n");
#endif
		return LIBMSR_ERR_DEVICE;
	}

	return LIBMSR_ERR_OK;
}

int msr_fwrev (int fd, uint8_t *buf)
{
	if (msr_cmd (fd, MSR_CMD_FWREV) < 0)
            return LIBMSR_ERR_SERIAL;

	msr_serial_readchar (fd, &buf[0]);

	/* read the result "REV?X.XX" */

	msr_serial_read (fd, buf, 8);
	buf[8] = '\0';

#ifdef DEBUG
	printf ("Firmware Version: %s\n", buf);
#endif

	return LIBMSR_ERR_OK;
}

int msr_model (int fd, uint8_t *buf)
{
	msr_model_t	m;

	msr_cmd (fd, MSR_CMD_MODEL);

	/* read the result as the value of X in "MSR206-X" */

	msr_serial_read (fd, &m, sizeof(m));

	if (m.msr_s != MSR_STS_MODEL_OK)
		return LIBMSR_ERR_DEVICE;

	snprintf((char *) buf, 10, "MSR-206-%c", m.msr_model);

#ifdef DEBUG
	printf("%s\n", buf);
#endif

	return LIBMSR_ERR_OK;
}

int msr_flash_led (int fd, uint8_t led)
{
	struct timespec pause = { .tv_sec = 0, .tv_nsec = 100000000};
	int r;

	r = msr_cmd (fd, led);

	if (r == -1)
		return LIBMSR_ERR_SERIAL | LIBMSR_ERR_DEVICE;

	nanosleep(&pause, NULL);

	/* No response, look at the lights Dr. Love */
	return LIBMSR_ERR_OK;
}

static int gettrack_iso (int fd, int t, uint8_t * buf, uint8_t * len)
{
	uint8_t b;
	int i = 0;
	int l = 0;

	/* Start delimiter should be ESC <track number> */

	msr_serial_readchar (fd, &b);
	if (b != MSR_ESC) {
		*len = 0;
		return LIBMSR_ERR_DEVICE;
	}

	msr_serial_readchar (fd, &b);
	if (b != t) {
		*len = 0;
		return LIBMSR_ERR_DEVICE;
	}

	while (1) {
		msr_serial_readchar (fd, &b);
		if (b == '%')
			continue;
		if (b == ';')
			continue;
		if (b == MSR_RW_END)
			break;
		if (b == MSR_ESC)
			break;
		/* Avoid overflowing the buffer */
		if (i < *len) {
			l++;
			buf[i] = b;
		}
		i++;
	}

	if (b == MSR_RW_END) {
		*len = l;
		return LIBMSR_ERR_OK;
	} else {
		*len = 0;
		msr_serial_readchar (fd, &b);
	}

	return LIBMSR_ERR_DEVICE;
}

static int gettrack_raw (int fd, int t, uint8_t * buf, uint8_t * len)
{
	uint8_t b, s;
	int i = 0;
	int l = 0;

	/* Start delimiter should be ESC <track number> */

	msr_serial_readchar (fd, &b);
	if (b != MSR_ESC) {
		*len = 0;
		return LIBMSR_ERR_DEVICE;
	}

	msr_serial_readchar (fd, &b);
	if (b != t) {
		*len = 0;
		return LIBMSR_ERR_DEVICE;
	}

	msr_serial_readchar (fd, &s);

	if (!s) {
		*len = 0;
		return LIBMSR_ERR_OK;
	}

	for (i = 0; i < s; i++) {
		msr_serial_readchar (fd, &b);
		/* Avoid overflowing the buffer */
		if (i < *len) {
			l++;
			buf[i] = b;
		}
	}

	*len = l;

	return LIBMSR_ERR_OK;
}

int msr_sensor_test (int fd)
{
	uint8_t b[4];

	msr_cmd (fd, MSR_CMD_DIAG_SENSOR);

#ifdef DEBUG
	printf("Attempting sensor test -- please slide a card...\n");
#endif

	msr_serial_read (fd, &b, 2);

	if (b[0] == MSR_ESC && b[1] == MSR_STS_SENSOR_OK) {
		return LIBMSR_ERR_OK;
	}

#ifdef DEBUG
	printf("It appears that the sensor did not sense a magnetic card.\n");
#endif

	return LIBMSR_ERR_DEVICE;
}

int msr_ram_test (int fd)
{
	uint8_t b[2] = {0};

	msr_cmd (fd, MSR_CMD_DIAG_RAM);

	msr_serial_read(fd, b, sizeof(b));

	if (b[0] == MSR_ESC && b[1] == MSR_STS_RAM_OK) {
 		return LIBMSR_ERR_OK;
	}

#ifdef DEBUG
	printf("It appears that the RAM test failed\n");
	printf("Got 0x%02x 0x%02x in response.\n", b[0], b[1]);
#endif

	return LIBMSR_ERR_DEVICE;
}

int msr_get_co(int fd)
{
	char b[2] = {0};

	msr_cmd(fd, MSR_CMD_GETCO);

	msr_serial_read(fd, &b, 2);

	if (b[0] == MSR_ESC && (b[1] == MSR_CO_HI || b[1] == MSR_CO_LO)) {
		return b[1];
	}

#ifdef DEBUG
	printf("It appears that the reader did not return its coercivity.\n");
	printf("Got 0x%02x 0x%02x in response.\n", b[0], b[1]);
#endif

	return LIBMSR_ERR_DEVICE;
}

int msr_set_hi_co (int fd)
{
	char b[2] = {0};

	msr_cmd (fd, MSR_CMD_SETCO_HI);

	/* read the result "<esc>0" if OK, unknown or no response if fail */
	msr_serial_read (fd, &b, 2);

	if (b[0] == MSR_ESC && b[1] == MSR_STS_OK) {
#ifdef DEBUG
		printf("Hi-Co mode: enabled.\n");
#endif
		return LIBMSR_ERR_OK;
	}

#ifdef DEBUG
	printf("It appears that the reader did not switch to Hi-Co mode.\n");
	printf("Got 0x%02x 0x%02x in response.\n", b[0], b[1]);
#endif

	return LIBMSR_ERR_DEVICE;
}

int msr_set_lo_co (int fd)
{
	char b[2] = {0};

	msr_cmd (fd, MSR_CMD_SETCO_LO);

	/* read the result "<esc>0" if OK, unknown or no response if fail */
	msr_serial_read (fd, &b, 2);

	if (b[0] == MSR_ESC && b[1] == MSR_STS_OK) {
#ifdef DEBUG
		printf("Lo-Co mode: enabled.\n");
#endif
		return LIBMSR_ERR_OK;
	}

#ifdef DEBUG
	printf("It appears that the reader did not switch to Lo-Co mode.\n");
	printf("Got 0x%02x 0x%02x in response.\n", b[0], b[1]);
#endif

	return LIBMSR_ERR_DEVICE;
}

int msr_reset (int fd)
{
	struct timespec pause = { .tv_sec = 0, .tv_nsec = 100000000};

	msr_cmd (fd, MSR_CMD_RESET);

	nanosleep(&pause, NULL);

	return LIBMSR_ERR_OK;
}

int msr_iso_read(int fd, msr_tracks_t * tracks)
{
	int r, i;

	r = msr_cmd (fd, MSR_CMD_READ);

	if (r == -1) {
#ifdef DEBUG
		err(1, "Command write failed");
#endif
	}

    /* Wait for start delimiter. */
	if (getstart (fd) == -1) {
#ifdef DEBUG
		err(1, "get start delimiter failed");
#endif
	}

    /* Read track data */
	for (i = 0; i < MSR_MAX_TRACKS; i++)
		gettrack_iso (fd, i + 1, tracks->msr_tracks[i].msr_tk_data,
		    &tracks->msr_tracks[i].msr_tk_len);

    /* Wait for end delimiter. */
	if (getend (fd) == -1) {
#ifdef DEBUG
		warnx("read failed");
#endif
		return LIBMSR_ERR_SERIAL;
	}

	return LIBMSR_ERR_OK;
}

int msr_erase (int fd, uint8_t tracks)
{
	uint8_t b[2];

	msr_cmd (fd, MSR_CMD_ERASE);
	msr_serial_write (fd, &tracks, 1);

	if (msr_serial_read (fd, b, 2) == -1) {
#ifdef DEBUG
		err(1, "read erase response failed");
#endif
	}

	if (b[0] == MSR_ESC && b[1] == MSR_STS_ERASE_OK) {
		return LIBMSR_ERR_OK;
	}
	else {
#ifdef DEBUG
		printf ("%x %x\n", b[0], b[1]);
#endif
	}

	return LIBMSR_ERR_DEVICE;
}

int msr_iso_write(int fd, msr_tracks_t * tracks)
{
	int i;
	uint8_t buf[4];

	msr_cmd(fd, MSR_CMD_WRITE);
	msr_cmd(fd, MSR_RW_START);

	for (i = 0; i < MSR_MAX_TRACKS; i++) {
		buf[0] = MSR_ESC;
		buf[1] = i + 1;
		msr_serial_write (fd, buf, 2);
		msr_serial_write (fd, tracks->msr_tracks[i].msr_tk_data,
			tracks->msr_tracks[i].msr_tk_len);
	}

	buf[0] = MSR_RW_END;
	buf[1] = MSR_FS;
	msr_serial_write (fd, buf, 2);

	msr_serial_read(fd, buf, 2);

	if (buf[1] != MSR_STS_OK) {
#ifdef DEBUG
		warnx("iso write failed: 0x%02x", buf[1]);
#endif
		return LIBMSR_ERR_DEVICE;
	}

	return LIBMSR_ERR_OK;
}

int msr_raw_read(int fd, msr_tracks_t * tracks)
{
	int r, i;

	r = msr_cmd(fd, MSR_CMD_RAW_READ);

	if (r == -1) {
#ifdef DEBUG
		err(1, "Command write failed");
#endif
	}

	if (getstart(fd) == -1) {
#ifdef DEBUG
		err(1, "get start delimiter failed");
#endif
	}

	for (i = 0; i < MSR_MAX_TRACKS; i++) {
		gettrack_raw(fd, i + 1, tracks->msr_tracks[i].msr_tk_data,
		    &tracks->msr_tracks[i].msr_tk_len);
	}

	if (getend(fd) == -1) {
#ifdef DEBUG
		err(1, "read failed");
#endif
		return LIBMSR_ERR_DEVICE;
	}

	return LIBMSR_ERR_OK;
}

int msr_raw_write(int fd, msr_tracks_t * tracks)
{
	int i;
	uint8_t buf[4];

	msr_cmd(fd, MSR_CMD_RAW_WRITE);
	msr_cmd(fd, MSR_RW_START);

	for (i = 0; i < MSR_MAX_TRACKS; i++) {
		buf[0] = MSR_ESC; /* start delimiter */
		buf[1] = i + 1; /* track number */
		buf[2] = tracks->msr_tracks[i].msr_tk_len; /* data length */
		msr_serial_write (fd, buf, 3);
		msr_serial_write (fd, tracks->msr_tracks[i].msr_tk_data,
			tracks->msr_tracks[i].msr_tk_len);
	}

	buf[0] = MSR_RW_END;
	buf[1] = MSR_FS;
	msr_serial_write (fd, buf, 2);

	msr_serial_read(fd, buf, 2);

	if (buf[1] != MSR_STS_OK) {
#ifdef DEBUG
		warnx("raw write failed: 0x%02x", buf[1]);
#endif
		return LIBMSR_ERR_DEVICE;
	}

	return LIBMSR_ERR_OK;
}

int msr_init(int fd)
{
	msr_reset (fd);

	if (msr_commtest (fd) == -1) {
		return LIBMSR_ERR_DEVICE;
	}

	msr_reset (fd);

	return LIBMSR_ERR_OK;
}

int msr_set_bpi (int fd, uint8_t bpi)
{
	uint8_t b[2] = {0};

	msr_cmd (fd, MSR_CMD_SETBPI);
	msr_serial_write (fd, &bpi, 1);
	msr_serial_read (fd, &b, 2);

	if (b[0] == MSR_ESC && b[1] == MSR_STS_OK) {
#ifdef DEBUG
		printf("Set bits per inch to: %d\n", bpi);
#endif
		return LIBMSR_ERR_OK;
	}

#ifdef DEBUG
	warnx ("Set bpi failed\n");
#endif

	return LIBMSR_ERR_DEVICE;
}

int msr_set_bpc (int fd, uint8_t bpc1, uint8_t bpc2, uint8_t bpc3)
{
	uint8_t b[2] = {0};
	msr_bpc_t bpc;

	bpc.msr_bpctk1 = bpc1;
	bpc.msr_bpctk2 = bpc2;
	bpc.msr_bpctk3 = bpc3;

	msr_cmd (fd, MSR_CMD_SETBPC);
	msr_serial_write (fd, &bpc, sizeof(bpc));

	msr_serial_read (fd, &b, 2);
	if (b[0] == MSR_ESC && b[1] == MSR_STS_OK) {
		msr_serial_read (fd, &bpc, sizeof(bpc));
#ifdef DEBUG
		printf ("Set bpc... %d %d %d\n", bpc.msr_bpctk1,
		    bpc.msr_bpctk2, bpc.msr_bpctk3);
#endif
		return LIBMSR_ERR_OK;
	}

#ifdef DEBUG
	warnx("failed to set bpc");
#endif

	return LIBMSR_ERR_DEVICE;
}
