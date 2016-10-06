#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <strings.h>
#include <err.h>
#include <string.h>

#include "libmsr.h"

/* Thanks Club Mate and h1kari! Toorcon 10 */

/*
 * Issue a command to the MSR206
 *
 * This function writes a command byte <c> to the MSR206 via the
 * serial port, via the file descriptor, <fd>. MSR206 commands are
 * issued by sending an ESC character to the device followed by
 * the command byte. Note that we do not check for a response
 * to the command here, as different commands provoke different
 * responses from the device (some may not provoke a response
 * at all).
 *
 * This function will fail if the serial port is not initialized
 * or the descriptor <fd> is invalid.
 */
int msr_cmd (int fd, uint8_t c)
{
	msr_cmd_t	cmd;

	cmd.msr_esc = MSR_ESC;
	cmd.msr_cmd = c;

	return (msr_serial_write (fd, &cmd, sizeof(cmd)));
}

/*
 * Check the number of leading zeros
 *
 * This function queries the device to read its current setting
 * for the number of leading zeroes that will be written when
 * writing ISO formatted data tracks. There are two values: one
 * for tracks 1 and 3, and a second for track 2.
 *
 * This function will fail if the serial port is not initialized
 * or the descriptor <fd> is invalid.
 */
int msr_zeros (int fd)
{
	msr_lz_t lz;

	msr_cmd (fd, MSR_CMD_CLZ);
	msr_serial_read (fd, &lz, sizeof(lz));

#ifdef MSR_DEBUG
	printf("zero13: %d zero: %d\n", lz.msr_lz_tk1_3, lz.msr_lz_tk2);
#endif

	return LIBMSR_ERR_OK;
}

/*
 * Read the start of an ISO formatted read response
 *
 * This is a helper routine used by msr_iso_read() to parse the
 * start delimiter returned by the MSR206 in response to an
 * ISO read command. This delimiter contains 3 characters, the
 * last of which is the MSR_RW_START indication byte. Once this
 * byte is detected, the routine returns.
 *
 * This function will fail if the serial port is not initialized
 * or the descriptor <fd> is invalid, or if the MSR_RW_START byte
 * is not seen in the first 3 characters read by the device.
 */
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
#ifdef MSR_DEBUG
		printf ("read returned error status: 0x%x\n", m.msr_sts);
#endif
		return LIBMSR_ERR_DEVICE;
	}

	return LIBMSR_ERR_OK;
}

/*
 * Perform a communications diagnostic test.
 *
 * This function issues an MSR_CMD_DIAG_COMM command to the device
 * to perform a communications diagnostic test. After issuing this
 * command, the device will respond with an MSR_STS_COMM_OK byte
 * if the test passes.
 *
 * This function will fail if the serial port is not initialized
 * or the descriptor <fd> is invalid, or if the status code
 * returned by the device is not MSR_STS_COMM_OK.
 */
int msr_commtest (int fd)
{
	int r;
	uint8_t buf[2];

	r = msr_cmd (fd, MSR_CMD_DIAG_COMM);

	if (r == -1) {
#ifdef MSR_DEBUG
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
#ifdef MSR_DEBUG
		printf("Communications test failure\n");
#endif
		return LIBMSR_ERR_DEVICE;
	}

	return LIBMSR_ERR_OK;
}

/*
 * Check firmware revision.
 *
 * This function issues an MSR_CMD_FWREV command to the device
 * to retrieve its firmware revision code. The revision is
 * printed on the standard output.
 *
 * This function will fail if the serial port is not initialized
 * or the descriptor <fd> is invalid.
 */
int msr_fwrev (int fd)
{
	uint8_t		buf[64];

	memset(buf, 0, sizeof(buf));

	if (msr_cmd (fd, MSR_CMD_FWREV) != 0)
            return LIBMSR_ERR_SERIAL;

	msr_serial_readchar (fd, &buf[0]);

	/* read the result "REV?X.XX" */

	msr_serial_read (fd, buf, 8);
	buf[8] = '\0';

	printf ("Firmware Version: %s\n", buf);

	return LIBMSR_ERR_OK;
}

/*
 * Check device model.
 *
 * This function issues an MSR_CMD_MODEL command to the device
 * to retrieve its model code. The model code is printed on
 * the standard output.
 *
 * This function will fail if the serial port is not initialized
 * or the descriptor <fd> is invalid, or if the device does not
 * return an MSR_STS_MODEL_OK status.
 */
int msr_model (int fd)
{
	msr_model_t	m;

	msr_cmd (fd, MSR_CMD_MODEL);

	/* read the result as the value of X in "MSR206-X" */

	msr_serial_read (fd, &m, sizeof(m));

	if (m.msr_s != MSR_STS_MODEL_OK)
		return LIBMSR_ERR_DEVICE;

	printf("Device Model: MSR-206-%c\n", m.msr_model);

	return LIBMSR_ERR_OK;
}

/*
 * Toggle LED state
 *
 * This function is used to manually control the LEDs on the
 * MSR206. The device has one green, one yellow and one red LED
 * mounted in it. A given LED is controlled by specifying <led>
 * as follows:
 *
 * MSR_CMD_LED_GRN_ON - turn on green LED
 * MSR_CMD_LED_YLW_ON - turn on yellow LED
 * MSR_CMD_LED_RED_ON - turn on red LED
 * MSR_CMD_LED_OFF - turn all LEDs off
 *
 * After an LED control command is issued to the device, the
 * routine will pause for a tenth of a second to allow time for the
 * command to be processed and the LED to light up.
 *
 * This function will fail if the serial port is not initialized
 * or the descriptor <fd> is invalid.
 */
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

/*
 * Read a single ISO formatted track
 *
 * This is a helper function used by msr_iso_read() to read a
 * single track in response to an ISO track read command. The routine
 * reads track <t> into buffer <buf> of length <len>, via file
 * descriptor <fd>. The track data should be prefixed by an ESC
 * character, followed by a single byte indicating the track number,
 * followed by an arbitrary amount of track data. The end of
 * track is detected when an MSR_RW_END or MSR_ESC character
 * is read.
 *
 * The buffer <buf> must be large enough to hold the data read
 * from the card. (The track length can't be more than 255 bytes,
 * for various reasons.) If the length <len> is smaller than the
 * actual amount of track data, then the data returned will be
 * truncated. (That is, if there are 100 bytes of data, but the
 * buffer is only 50 bytes long, only the first 50 bytes will be
 * returned.) If the length <len> is larger than the actual amount
 * of track data, <len> will be adjusted to reflect the actual
 * number of bytes read.
 *
 * This function will fail if the serial port is not initialized
 * or the descriptor <fd> is invalid, or if the track being read
 * does not match the specified track number <t>.
 */
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

/*
 * Read a single raw track
 *
 * This is a helper function used by msr_raw_read() to read a
 * single track in response to a RAW track read command. The routine
 * reads track <t> into buffer <buf> of length <len>, via file
 * descriptor <fd>. It is similar to the gettrack_iso() routine
 * above, except that the track delimiter format is a little different.
 * The track data should be prefixed by an ESC character, followed by a
 * single byte indicating the track number, followed by another byte
 * indicating the number of bytes of track data that follow. This is
 * required as format of the data is arbitrary, making it impossible
 * to use a given character as an 'end of track' sentinel.
 *
 * The buffer <buf> must be large enough to hold the data read
 * from the card. (The track length can't be more than 255 bytes,
 * for various reasons.) If the length <len> is smaller than the
 * actual amount of track data, then the data returned will be
 * truncated. (That is, if there are 100 bytes of data, but the
 * buffer is only 50 bytes long, only the first 50 bytes will be
 * returned.) If the length <len> is larger than the actual amount
 * of track data, <len> will be adjusted to reflect the actual
 * number of bytes read.
 *
 * This function will fail if the serial port is not initialized
 * or the descriptor <fd> is invalid, or if the track being read
 * does not match the specified track number <t>.
 */
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

/*
 * Perform sensor test
 *
 * This function issues an MSR_CMD_DIAG_SENSOR command to perform
 * a diagnostic sense on the mechanical card sensor in the MSR206.
 * After issuing the command, the user is instructed to slide a card
 * through the reader. No actual read is performed, however the
 * hardware tests to verify that the card sensor registers the
 * presense of the card in the track. If the card registers correctly,
 * the device will return a status code of MSR_STS_SENSOR_OK.
 *
 * This function will fail if the serial port is not initialized
 * or the descriptor <fd> is invalid, or if the device does not
 * return the MSR_STS_SENSOR_OK status code.
 */
int msr_sensor_test (int fd)
{
	uint8_t b[4];

	msr_cmd (fd, MSR_CMD_DIAG_SENSOR);

#ifdef MSR_DEBUG
	printf("Attempting sensor test -- please slide a card...\n");
#endif

	msr_serial_read (fd, &b, 2);

	if (b[0] == MSR_ESC && b[1] == MSR_STS_SENSOR_OK) {
		return LIBMSR_ERR_OK;
	}

#ifdef MSR_DEBUG
	printf("It appears that the sensor did not sense a magnetic card.\n");
#endif

	return LIBMSR_ERR_DEVICE;
}

/*
 * Perform RAM test
 *
 * This function issues an MSR_CMD_DIAG_RAM command to perform
 * a diagnostic sense on the MSR206's internal RAM. If the RAM
 * checks good, the device will return a status code of MSR_STS_RAM_OK.
 *
 * This function will fail if the serial port is not initialized
 * or the descriptor <fd> is invalid, or if the device does not
 * return the MSR_STS_RAM_OK status code.
 */
int msr_ram_test (int fd)
{
	uint8_t b[2] = {0};

	msr_cmd (fd, MSR_CMD_DIAG_RAM);

	msr_serial_read(fd, b, sizeof(b));

	if (b[0] == MSR_ESC && b[1] == MSR_STS_RAM_OK) {
 		return LIBMSR_ERR_OK;
	}

#ifdef MSR_DEBUG
	printf("It appears that the RAM test failed\n");
	printf("Got 0x%02x 0x%02x in response.\n", b[0], b[1]);
#endif

	return (-1);
}

/*
 * Set coercivity to high
 *
 * This function issues an MSR_CMD_SETCO_HI command to switch the
 * device to high coercivity mode. It is unclear if this affects both
 * the read and write operations, though presumably it only affects
 * writes by channeling more power to the write head so that it can
 * update high-coercivity media.
 *
 * This function will fail if the serial port is not initialized
 * or the descriptor <fd> is invalid, or if the device does not
 * return the MSR_STS_OK status code.
 */
int msr_set_hi_co (int fd)
{
	char b[2] = {0};

	msr_cmd (fd, MSR_CMD_SETCO_HI);

	/* read the result "<esc>0" if OK, unknown or no response if fail */
	msr_serial_read (fd, &b, 2);

	if (b[0] == MSR_ESC && b[1] == MSR_STS_OK) {
#ifdef MSR_DEBUG
		printf("Hi-Co mode: enabled.\n");
#endif
		return LIBMSR_ERR_OK;
	}

#ifdef MSR_DEBUG
	printf("It appears that the reader did not switch to Hi-Co mode.\n");
	printf("Got 0x%02x 0x%02x in response.\n", b[0], b[1]);
#endif

	return LIBMSR_ERR_DEVICE;
}

/*
 * Set coercivity to low
 *
 * This function issues an MSR_CMD_SETCO_HI command to switch the
 * device to high coercivity mode. It is unclear if this affects both
 * the read and write operations, though presumably it only affects
 * writes by channeling less power to the write head so that it can
 * update high-coercivity media.
 *
 * This function will fail if the serial port is not initialized
 * or the descriptor <fd> is invalid, or if the device does not
 * return the MSR_STS_OK status code.
 */
int msr_set_lo_co (int fd)
{
	char b[2] = {0};

	msr_cmd (fd, MSR_CMD_SETCO_LO);

	/* read the result "<esc>0" if OK, unknown or no response if fail */
	msr_serial_read (fd, &b, 2);

	if (b[0] == MSR_ESC && b[1] == MSR_STS_OK) {
#ifdef MSR_DEBUG
		printf("Lo-Co mode: enabled.\n");
#endif
		return LIBMSR_ERR_OK;
	}

#ifdef MSR_DEBUG
	printf("It appears that the reader did not switch to Lo-Co mode.\n");
	printf("Got 0x%02x 0x%02x in response.\n", b[0], b[1]);
#endif

	return LIBMSR_ERR_DEVICE;
}

/*
 * Reset the device
 *
 * This function issues an MSR_CMD_RESET command to reset the device.
 * This command does not return a status code. The routine pauses
 * for a tenth of a second to wait for the reset to complete.
 *
 * This function will fail if the serial port is not initialized
 * or the descriptor <fd> is invalid.
 */
int msr_reset (int fd)
{
	struct timespec pause = { .tv_sec = 0, .tv_nsec = 100000000};

	msr_cmd (fd, MSR_CMD_RESET);

	nanosleep(&pause, NULL);

	return LIBMSR_ERR_OK;
}

/*
 * Read an ISO formatted card
 *
 * This routine issues an MSR_CMD_READ command to the device to
 * read an ISO formatted magstripe card. After the command is
 * issued, the user must swipe a card through the MSR206. The
 * function will block until the card is read and the MSR206 is
 * ready to return data from the tracks. The caller must allocate
 * a pointer to an msr_tracks_t structure and supply a pointer
 * to this structure via the <tracks> argument.
 *
 * This function will fail if the serial port is not initialized
 * or the descriptor <fd> is invalid.
 */
int msr_iso_read(int fd, msr_tracks_t * tracks)
{
	int r, i;

	r = msr_cmd (fd, MSR_CMD_READ);

	if (r == -1) {
#ifdef MSR_DEBUG
		err(1, "Command write failed");
#endif
	}

    /* Wait for start delimiter. */
	if (getstart (fd) == -1) {
#ifdef MSR_DEBUG
		err(1, "get start delimiter failed");
#endif
	}

    /* Read track data */
	for (i = 0; i < MSR_MAX_TRACKS; i++)
		gettrack_iso (fd, i + 1, tracks->msr_tracks[i].msr_tk_data,
		    &tracks->msr_tracks[i].msr_tk_len);

    /* Wait for end delimiter. */
	if (getend (fd) == -1) {
#ifdef MSR_DEBUG
		warnx("read failed");
#endif
		return LIBMSR_ERR_SERIAL;
	}

	return LIBMSR_ERR_OK;
}

/*
 * Erase one or more tracks on a card
 *
 * This routine issues an MSR_CMD_ERASE command to the device to
 * read erase a magstripe card. After the command is issued,
 * the user must swipe a card through the MSR206. The function
 * will block until the device returns a response code indicating
 * that the erase operation completed successfully.
 *
 * This function can erase various combinations of tracks, or erase
 * all of them, depending on the <tracks> value specified:
 *
 * MSR_ERASE_TK1	erase track 1 only
 * MSR_ERASE_TK2	erase track 2 only
 * MSR_ERASE_TK3	erase track 3 only
 * MSR_ERASE_TK1_TK2	erase tracks 1 and 2
 * MSR_ERASE_TK1_TK3	erase tracks 1 and 3
 * MSR_ERASE_TK2_TK3	erase tracks 2 and 3
 * MSR_ERASE_ALL	erase all tracks
 *
 * This function will fail if the serial port is not initialized
 * or the descriptor <fd> is invalid, or if the device does not
 * return an MSR_STS_ERASE_OK status code.
 */
int msr_erase (int fd, uint8_t tracks)
{
	uint8_t b[2];

	msr_cmd (fd, MSR_CMD_ERASE);
	msr_serial_write (fd, &tracks, 1);

	if (msr_serial_read (fd, b, 2) == -1) {
#ifdef MSR_DEBUG
		err(1, "read erase response failed");
#endif
	}

	if (b[0] == MSR_ESC && b[1] == MSR_STS_ERASE_OK) {
		return LIBMSR_ERR_OK;
	}
	else {
#ifdef MSR_DEBUG
		printf ("%x %x\n", b[0], b[1]);
#endif
	}

	return LIBMSR_ERR_DEVICE;
}

/*
 * Write an ISO formatted card
 *
 * This routine issues an MSR_CMD_WRITE command to the device to
 * write to an ISO formatted magstripe card. The card does not need
 * to be erased (any existing data is overwritten). The caller must
 * allocate an msr_tracks_t structure and populate it with the data
 * to be written to the card, then supply a pointer to this structure
 * via the <tracks> argument. The data in the tracks must meet the
 * ISO requirements.
 *
 * After the write command is issued and the track data is written
 * to the device, the function will block until a status code is
 * read from the device.
 *
 * This function will fail if the serial port is not initialized
 * or the descriptor <fd> is invalid, or if the MST_STS_OK status
 * code is not returned.
 */
int msr_iso_write(int fd, msr_tracks_t * tracks)
{
	int i;
	uint8_t buf[4];

	msr_cmd (fd, MSR_CMD_WRITE);

	buf[0] = MSR_ESC;
	buf[1] = MSR_RW_START;
	msr_serial_write (fd, buf, 2);

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

	msr_serial_readchar (fd, &buf[0]);
	msr_serial_readchar (fd, &buf[0]);

	if (buf[0] != MSR_STS_OK) {
#ifdef MSR_DEBUG
		warnx("iso write failed");
#endif
		return LIBMSR_ERR_DEVICE;
	}

	return LIBMSR_ERR_OK;
}

/*
 * Read raw data from a card
 *
 * This routine issues an MSR_CMD_RAW_READ command to the device
 * to read arbitrary data from 3 tracks on a magstripe card. After
 * the command is issued, the user must swipe a card through the MSR206.
 * The function will block until the card is read and the MSR206 is
 * ready to return data from the tracks. The caller must allocate
 * a pointer to an msr_tracks_t structure and supply a pointer
 * to this structure via the <tracks> argument.
 *
 * Unlike the msr_iso_read() function, this routine bypasses the
 * MSR206's internal data parser and returns data representing the
 * raw bit pattern on the magnetic media. It is up to the caller to
 * decode this data into a useful form.
 *
 * This function will fail if the serial port is not initialized
 * or the descriptor <fd> is invalid.
 */
int msr_raw_read(int fd, msr_tracks_t * tracks)
{
	int r, i;

	r = msr_cmd(fd, MSR_CMD_RAW_READ);

	if (r == -1) {
#ifdef MSR_DEBUG
		err(1, "Command write failed");
#endif
	}

	if (getstart(fd) == -1) {
#ifdef MSR_DEBUG
		err(1, "get start delimiter failed");
#endif
	}

	for (i = 0; i < MSR_MAX_TRACKS; i++) {
		gettrack_raw(fd, i + 1, tracks->msr_tracks[i].msr_tk_data,
		    &tracks->msr_tracks[i].msr_tk_len);
	}

	if (getend(fd) == -1) {
#ifdef MSR_DEBUG
		err(1, "read failed");
#endif
		return LIBMSR_ERR_DEVICE;
	}

	return LIBMSR_ERR_OK;
}

/*
 * Write raw track data to a card
 *
 * This routine issues an MSR_CMD_RAW_WRITE command to the device to
 * write arbitrary data to a magstripe card. The card does not need
 * to be erased (any existing data is overwritten). The caller must
 * allocate an msr_tracks_t structure and populate it with the data
 * to be written to the card, then supply a pointer to this structure
 * via the <tracks> argument. The data can be in any format.
 *
 * After the write command is issued and the track data is written
 * to the device, the function will block until a status code is
 * read from the device.
 *
 * Unlike the msr_iso_write() routine, this function bypasses the
 * MSR206's internal parser and writes the bit pattern represented
 * by the track data unmodified to the card. It's up to the caller
 * to format the data in a meaningful way.
 *
 * This function will fail if the serial port is not initialized
 * or the descriptor <fd> is invalid, or if the MST_STS_OK status
 * code is not returned.
 */
int msr_raw_write(int fd, msr_tracks_t * tracks)
{
	int i;
	uint8_t buf[4];

	msr_cmd (fd, MSR_CMD_RAW_WRITE);


	buf[0] = MSR_ESC;
	buf[1] = MSR_RW_START;
	msr_serial_write (fd, buf, 2);

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

	msr_serial_readchar (fd, &buf[0]);
	msr_serial_readchar (fd, &buf[0]);

	if (buf[0] != MSR_STS_OK) {
#ifdef MSR_DEBUG
		warnx("raw write failed");
#endif
		return LIBMSR_ERR_DEVICE;
	}

	return LIBMSR_ERR_OK;
}

/*
 * Initialize the MSR206
 *
 * This function issues a reset command to the MSR206 device, and
 * then performs a communications diagnostic test. If the test
 * succeeds, another reset is issued to ready the device for a
 * read or write operation. Typically, msr_init() must be called
 * before any significant operation, including reading and writing
 * cards.
 *
 * This function will fail if the serial port is not initialized
 * or the descriptor <fd> is invalid.
 */
int msr_init(int fd)
{
	msr_reset (fd);

	if (msr_commtest (fd) == -1) {
		return LIBMSR_ERR_DEVICE;
	}

	msr_reset (fd);

	return LIBMSR_ERR_OK;
}

/*
 * Set BPI value
 *
 * This function issues an MSR_CMD_SETBPI command to set the bits per
 * inch configuration for track 2. (This command has no effect for
 * other tracks.) It is assumed that this command only has meaning
 * when writing data. The MSR206 supports writing data on track 2
 * with <bpi> values. Valid options are either 75 or 210 bits per inch.
 *
 * This function will fail if the serial port is not initialized
 * or the descriptor <fd> is invalid, or if the MSR_STS_OK status
 * is not returned.
 */
int msr_set_bpi (int fd, uint8_t bpi)
{
	uint8_t b[2] = {0};

	msr_cmd (fd, MSR_CMD_SETBPI);
	msr_serial_write (fd, &bpi, 1);
	msr_serial_read (fd, &b, 2);

	if (b[0] == MSR_ESC && b[1] == MSR_STS_OK) {
#ifdef MSR_DEBUG
		printf("Set bits per inch to: %d\n", bpi);
#endif
		return LIBMSR_ERR_OK;
	}

#ifdef MSR_DEBUG
	warnx ("Set bpi failed\n");
#endif

	return LIBMSR_ERR_DEVICE;
}

/*
 * Set BPC value
 *
 * This function issues an MSR_CMD_SETBPC command to bits per character
 * configuration for all 3 tracks. (This command has no effect for
 * other tracks.) It is assumed that this command only has meaning
 * when using ISO read or write commands. The MSR206 supports writing
 * data with <bpc> values from 5 to 8.
 *
 * This function will fail if the serial port is not initialized
 * or the descriptor <fd> is invalid, or if the MSR_STS_OK status
 * is not returned.
 */
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
#ifdef MSR_DEBUG
		printf ("Set bpc... %d %d %d\n", bpc.msr_bpctk1,
		    bpc.msr_bpctk2, bpc.msr_bpctk3);
#endif
		return LIBMSR_ERR_OK;
	}

#ifdef MSR_DEBUG
	warnx("failed to set bpc");
#endif

	return LIBMSR_ERR_DEVICE;
}
