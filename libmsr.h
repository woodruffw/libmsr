/* Everyone must include libmsr.h or they're doing it wrong! */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdint.h>

/**
 * Returned on operation success.
 */
#define LIBMSR_ERR_OK 0x0

/**
 * Returned on generic error.
 */
#define LIBMSR_ERR_GENERIC 0x1000

/**
 * Returned on error with ISO-formatted card.
 */
#define LIBMSR_ERR_ISO 0x1100

/**
 * Returned on error with device control.
 */
#define LIBMSR_ERR_DEVICE 0x2000

/**
 * Returned on error in serial I/O.
 */
#define LIBMSR_ERR_SERIAL 0x4000

/**
 * The maximum length, in bytes, of a track.
 */
#define MSR_MAX_TRACK_LEN 255

/**
 * The maximum number of tracks on a card.
 */
#define MSR_MAX_TRACKS 3

#define MSR_BLOCKING O_NONBLOCK
#define MSR_BAUD B9600

/**
 * ESC is frequently used as a start delimiter character.
 */
#define MSR_ESC	0x1B

/**
 * ASCII file separator character is used to separate track data.
 */
#define MSR_FS 0x1C

/**
 * A generic success status sent by the MSR device.
 */
#define MSR_STS_OK 0x30

/**
 * A generic error status sent by the MSR device.
 */
#define MSR_STS_ERR 0x41

/**
 * @brief Represents a command issued to the MSR device.
 * @details A MSR command consists of two bytes - an ASCII escape byte
 * (MSR_ESC) and the command byte.
 */
typedef struct msr_cmd {
	uint8_t msr_esc; /**< The escape byte. */
	uint8_t msr_cmd; /**< The command byte. */
} msr_cmd_t;

/**
 * Request a formatted read from the MSR device.
 */
#define MSR_CMD_READ 0x72

/**
 * Request a formatted write from the MSR device.
 */
#define MSR_CMD_WRITE 0x77

/**
 * Request a raw read from the MSR device.
 */
#define MSR_CMD_RAW_READ 0x6D

/**
 * Request a raw write from the MSR device.
 */
#define MSR_CMD_RAW_WRITE 0x6E

/**
 * A generic read/write error.
 */
#define MSR_STS_RW_ERR 0x31

/**
 * An error in command formatting during read/write.
 */
#define MSR_STS_RW_CMDFMT_ERR 0x32

/**
 * An invalid command issued during read/write.
 */
#define MSR_STS_RW_CMDBAD_ERR 0x34

/**
 * A bad card swipe during write.
 */
#define MSR_STS_RW_SWIPEBAD_ERR 0x39

/**
 * @brief Represents the end of a read/write command.
 * @note Currently unused.
 */
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

/**
 * Request a communications test.
 */
#define MSR_CMD_DIAG_COMM 0x65

/**
 * The response to a successful communications test.
 */
#define MSR_STS_COMM_OK 0x79

/**
 * Request a sensor test.
 */
#define MSR_CMD_DIAG_SENSOR 0x86

/**
 * The response to a successful sensor test.
 */
#define MSR_STS_SENSOR_OK MSR_STS_OK

/**
 * Request a RAM test.
 */
#define MSR_CMD_DIAG_RAM 0x87

/**
 * The response to a successful RAM test.
 */
#define MSR_STS_RAM_OK MSR_STS_OK

/**
 * The response to a failed RAM test.
 */
#define MSR_STS_RAM_ERR MSR_STS_ERR

/**
 * Request a set-leading-zero count change on a card.
 */
#define MSR_CMD_SLZ	0x7A

/**
 * The response to a successful SLZ change.
 */
#define MSR_STS_SLZ_OK MSR_STS_OK

/**
 * The response to a failed SLZ change.
 */
#define MSR_STS_SLZ_ERR	MSR_STS_ERR

/**
 * Request a leading-zero check on a card.
 * @see ::msr_lz_t
 */
#define MSR_CMD_CLZ 0x6C

/**
 * @brief Represents leading zero counts on a card.
 */
typedef struct msr_lz {
	uint8_t msr_esc; /**< The escape byte. */
	uint8_t msr_lz_tk1_3; /**< The LZC for tracks 1 and 3. */
	uint8_t msr_lz_tk2; /**< The LZC for track 2. */
} msr_lz_t;

/**
 * Request a track erasure.
 */
#define MSR_CMD_ERASE 0x63

/**
 * The response to a successful track erasure.
 */
#define MSR_STS_ERASE_OK MSR_STS_OK

/**
 * The response to a failed track erasure.
 */
#define MSR_STS_ERASE_ERR MSR_STS_ERR

/**
 * Erase *only* track 1.
 */
#define MSR_ERASE_TK1 0x00

/**
 * Erase *only* track 2.
 */
#define MSR_ERASE_TK2 0x02

/**
 * Erase *only* track 3.
 */
#define MSR_ERASE_TK3 0x04

/**
 * Erase track 1 *and* track 2.
 */
#define MSR_ERASE_TK1_TK2 0x03

/**
 * Erase track 1 *and* track 3.
 */
#define MSR_ERASE_TK1_TK3 0x05

/**
 * Erase track 2 *and* track 3.
 */
#define MSR_ERASE_TK2_TK3 0x06

/**
 * Erase all three tracks.
 */
#define MSR_ERASE_ALL 0x07

/**
 * Request a change in the MSR device's bits-per-inch (BPI) setting.
 */
#define MSR_CMD_SETBPI 0x62

/**
 * The response to a successful BPI change.
 */
#define MSR_STS_BPI_OK MSR_STS_OK

/**
 * The response to a failed BPI change.
 */
#define MSR_STS_BPI_ERR MSR_STS_ERR

/**
 * Request the MSR device's model number.
 */
#define MSR_CMD_MODEL 0x74

/**
 * The response to a successful model number request.
 */
#define MSR_STS_MODEL_OK 0x53

#define MSR_MODEL_MSR206_1 0x31
#define MSR_MODEL_MSR206_2 0x32
#define MSR_MODEL_MSR206_3 0x33
#define MSR_MODEL_MSR206_5 0x35

/**
 * @brief Represents the MSR's model.
 * @note Currently unused.
 */
typedef struct msr_model {
	uint8_t msr_esc;
	uint8_t msr_model;
	uint8_t msr_s;
} msr_model_t;

/**
 * Request the MSR device's firmware revision.
 */
#define MSR_CMD_FWREV 0x76

/**
 * The format of the firmware revision response.
 */
#define MSR_FWREV_FMT "REV?X.XX"

/**
 * Request a change in the MSR device's BPC settings.
 */
#define MSR_CMD_SETBPC 0x6F

/**
 * The response to a successful BPC change request.
 */
#define MSR_STS_BPC_OK MSR_STS_OK

/**
 * The response to a failed BPC change request.
 */
#define MSR_STS_BPC_ERR MSR_STS_ERR

/**
 * @brief Represents the MSR's BPC settings for each track.
 */
typedef struct msr_bpc {
	uint8_t msr_bpctk1; /**< The BPC for the first track */
	uint8_t msr_bpctk2; /**< The BPC for the second track */
	uint8_t msr_bpctk3; /**< The BPC for the third track */
} msr_bpc_t;

/**
 * Request that the MSR device change into Hi-Co mode.
 * @see msr_set_hi_co()
 */
#define MSR_CMD_SETCO_HI 0x78

/**
 * Request that the MSR device change into Lo-Co mode.
 * @see msr_set_lo_co()
 */
#define MSR_CMD_SETCO_LO 0x79

/**
 * The response to a successful coercivity change request.
 */
#define MSR_STS_CO_OK MSR_STS_OK

/**
 * The response to a failed coercivity change request.
 */
#define MSR_STS_CO_ERR MSR_STS_ERR

/**
 * Request the MSR device's current coercivity setting.
 * @see msr_get_co()
 */
#define MSR_CMD_GETCO 0x64

/**
 * The response when the MSR device is in Hi-Co mode.
 */
#define MSR_CO_HI 0x68

/**
 * The response when the MSR device is in Lo-Co mode.
 */
#define MSR_CO_LO 0x6C

/**
 * Request a reset from the MSR device.
 *
 * @see msr_reset() and msr_init()
 */
#define MSR_CMD_RESET 0x61

/**
 * Request that the MSR device turn off all LEDs.
 *
 * @see msr_flash_led()
 */
#define MSR_CMD_LED_OFF 0x81

/**
 * Request that the MSR device turn on all LEDs.
 *
 * @see msr_flash_led()
 */
#define MSR_CMD_LED_ON 0x82

/**
 * Request that the MSR device turn on the green LED.
 *
 * @see msr_flash_led()
 */
#define MSR_CMD_LED_GRN_ON 0x83

/**
 * Request that the MSR device turn on the yellow LED.
 *
 * @see msr_flash_led()
 */
#define MSR_CMD_LED_YLW_ON 0x84

/**
 * Request that the MSR device turn on the red LED.
 *
 * @see msr_flash_led()
 */
#define MSR_CMD_LED_RED_ON 0x85

/**
 * @brief Represents a single track on a magnetic card.
 */
typedef struct msr_track {
	uint8_t msr_tk_data[MSR_MAX_TRACK_LEN]; /**< The track data */
	uint8_t msr_tk_len; /**< The track length */
} msr_track_t;

/**
 * @brief Represents all tracks on a magnetic card.
 */
typedef struct msr_tracks {
	msr_track_t	msr_tracks[MSR_MAX_TRACKS]; /** The array of tracks */
} msr_tracks_t;

/**
 * @brief Open a serial connection to the MSR device.
 *
 * @param path The path to the serial device.
 * @param fd The int pointer to store the file descriptor in.
 * @param blocking The blocking flag (e.g., ::MSR_BLOCKING)
 * @param baud The baud rate of the serial device (e.g., ::MSR_BAUD)
 * @return ::LIBMSR_ERR_OK on success
 * @return ::LIBMSR_ERR_SERIAL on failure
 */
extern int msr_serial_open(char *path, int *fd, int blocking, speed_t baud);

/**
 * @brief Close a serial connection to the MSR device.
 *
 * @param fd The file descriptor to close.
 * @return ::LIBMSR_ERR_OK
 */
extern int msr_serial_close(int fd);

/**
 * @brief Read a single character from the MSR device.
 *
 * @param fd The file descriptor to read from.
 * @param c A pointer to write the character into.
 *
 * @return The character read
 */
extern int msr_serial_readchar(int fd, uint8_t *c);

/**
 * @brief Write a series of bytes to the MSR device.
 *
 * @param fd The file descriptor to write to.
 * @param buf The buffer to write.
 * @param len The length of the buffer.
 * @return The number of bytes written, or -1 on error.
 */
extern int msr_serial_write(int fd, void *buf, size_t len);

/**
 * @brief Read a series of bytes from the MSR device.
 *
 * @param fd The file descriptor to read from.
 * @param buf The buffer to read into.
 * @param len The length of the buffer.
 * @return ::LIBMSR_ERR_OK
 */
extern int msr_serial_read(int fd, void *buf, size_t len);

/**
 * @brief Get the MSR device's current leading-zero setting.
 * @details The leading-zero setting is used by the device to determine
 * how many leading zeros to write when writing ISO-formatted tracks. There
 * are two values to the setting: one for tracks 1 and 3, and one for track 2.
 *
 * @param fd The device's fd.
 * @param lz A pointer to the ::msr_lz_t to populate.
 *
 * @return ::LIBMSR_ERR_OK
 */
extern int msr_zeros(int fd, msr_lz_t *lz);

/**
 * @brief Perform a communications test.
 * @details This function issues an ::MSR_CMD_DIAG_COMM command to the device
 * to perform a communications diagnostic test. After issuing this
 * command, the device will respond with an ::MSR_STS_COMM_OK byte
 * if the test passes.
 *
 * @param fd The device's fd.
 * @return ::LIBMSR_ERR_OK on success.
 * @return ::LIBMSR_ERR_SERIAL on serial I/O failure.
 * @return ::LIBMSR_ERR_DEVICE on device failure.
 */
extern int msr_commtest(int fd);

/**
 * @brief Initialize the MSR device.
 * @details This function issues a reset command to the MSR206 device, and
 * then performs a communications diagnostic test. If the test
 * succeeds, another reset is issued to ready the device for a
 * read or write operation. Typically, msr_init() must be called
 * before any significant operation, including reading and writing
 * cards.
 *
 * @param fd The device's fd.
 * @return ::LIBMSR_ERR_OK on success.
 * @return ::LIBMSR_ERR_DEVICE on device failure.
 */
extern int msr_init(int fd);

/**
 * @brief Check the device's firmware revision.
 * @details This function issues an ::MSR_CMD_FWREV command to the device
 * to retrieve its firmware revision code. The revision is
 * saved in the supplied buffer, which *must* be at least 9 bytes.
 *
 * @param fd The device's fd.
 * @param buf The buffer to write the revision to.
 *
 * @return ::LIBMSR_ERR_OK on success.
 * @return ::LIBMSR_ERR_SERIAL on serial I/O failure.
 */
extern int msr_fwrev(int fd, uint8_t *buf);

/**
 * @brief Check the device's model.
 * @details This function issues an ::MSR_CMD_MODEL command to the device
 * to retrieve its model code. The model code is saved in the supplied
 * buffer, which *must* be at least 10 bytes.
 *
 * @param fd The device's fd.
 * @param buf The buffer to write the model to.
 *
 * @return ::LIBMSR_ERR_OK on success.
 * @return ::LIBMSR_ERR_DEVICE on device failure.
 */
extern int msr_model(int fd, uint8_t *buf);

/**
 * @brief Check the device's sensor.
 * @details This function issues an ::MSR_CMD_DIAG_SENSOR command to perform
 * a diagnostic sense on the mechanical card sensor in the MSR206.
 * After issuing the command, the user is instructed to slide a card
 * through the reader. No actual read is performed, however the
 * hardware tests to verify that the card sensor registers the
 * presence of the card in the track. If the card registers correctly,
 * the device will return a status code of ::MSR_STS_SENSOR_OK.
 *
 * @param fd The device's fd.
 * @return ::LIBMSR_ERR_OK on success.
 * @return ::LIBMSR_ERR_DEVICE on device failure.
 */
extern int msr_sensor_test(int fd);

/**
 * @brief Check the device's RAM.
 * @details This function issues an ::MSR_CMD_DIAG_RAM command to perform
 * a diagnostic sense on the MSR206's internal RAM. If the RAM
 * checks good, the device will return a status code of ::MSR_STS_RAM_OK.
 *
 * @param fd The device's fd.
 * @return ::LIBMSR_ERR_OK on success.
 * @return ::LIBMSR_ERR_DEVICE on device failure.
 */
extern int msr_ram_test(int fd);

/**
 * @brief Get the device's coercivity level.
 * @details This function issues an ::MSR_CMD_GETCO command to retrieve the
 * device's current coercivity setting, which is either ::MSR_CO_HI ('H') or
 * ::MSR_CO_LO ('L').
 *
 * @param fd The device's fd.
 * @return ::MSR_CO_HI if the device is in hi-co mode.
 * @return ::MSR_CO_LO if the device is in lo-co mode.
 * @return ::LIBMSR_ERR_DEVICE on device failure.
 */
extern int msr_get_co(int fd);

/**
 * @brief Set the device's coercivity to high.
 * @details This function issues an ::MSR_CMD_SETCO_HI command to switch the
 * device to high coercivity mode. It is unclear if this affects both
 * the read and write operations, though presumably it only affects
 * writes by channeling more power to the write head so that it can
 * update high-coercivity media.
 *
 * @param fd The device's fd.
 * @return ::LIBMSR_ERR_OK on success.
 * @return ::LIBMSR_ERR_DEVICE on device failure.
 */
extern int msr_set_hi_co(int fd);

/**
 * @brief Set the device's coercivity to low.
 * @details This function issues an ::MSR_CMD_SETCO_LO command to switch the
 * device to low coercivity mode. It is unclear if this affects both
 * the read and write operations, though presumably it only affects
 * writes by channeling less power to the write head so that it can
 * update low-coercivity media.
 *
 * @param fd The device's fd.
 * @return ::LIBMSR_ERR_OK on success.
 * @return ::LIBMSR_ERR_DEVICE on device failure.
 */
extern int msr_set_lo_co(int fd);

/**
 * @brief Reset the MSR device.
 * @details This function issues an ::MSR_CMD_RESET command to reset the device.
 * This command does not return a status code. The routine pauses
 * for a tenth of a second to wait for the reset to complete.
 *
 * @param fd The device's fd.
 * @return ::LIBMSR_ERR_OK.
 */
extern int msr_reset(int fd);

/**
 * @brief Read an ISO formatted card.
 * @details This routine issues an ::MSR_CMD_READ command to the device to
 * read an ISO formatted magstripe card. After the command is
 * issued, the user must swipe a card through the MSR206. The
 * function will block until the card is read and the MSR206 is
 * ready to return data from the tracks. The caller must allocate
 * a pointer to an ::msr_tracks_t structure and supply a pointer
 * to this structure via the tracks argument.
 *
 * @param fd The device's fd.
 * @param tracks A pointer to the ::msr_tracks_t to populate.
 * @return ::LIBMSR_ERR_OK on success.
 * @return ::LIBMSR_ERR_SERIAL on serial I/O failure.
 */
extern int msr_iso_read(int fd, msr_tracks_t *tracks);

/**
 * @brief Write an ISO formatted card.
 * @details This routine issues an ::MSR_CMD_WRITE command to the device to
 * write to an ISO formatted magstripe card. The card does not need
 * to be erased (any existing data is overwritten). The caller must
 * allocate an ::msr_tracks_t structure and populate it with the data
 * to be written to the card, then supply a pointer to this structure
 * via the tracks argument. The data in the tracks must meet the
 * ISO requirements.
 *
 * After the write command is issued and the track data is written
 * to the device, the function will block until a status code is
 * read from the device.
 *
 * @param fd The device's fd.
 * @param tracks A pointer to the ::msr_tracks_t data to write.
 * @return ::LIBMSR_ERR_OK on success.
 * @return ::LIBMSR_ERR_DEVICE on device failure.
 */
extern int msr_iso_write(int fd, msr_tracks_t *tracks);

/**
 * @brief Read raw data from a card.
 * @details This routine issues an ::MSR_CMD_RAW_READ command to the device
 * to read arbitrary data from 3 tracks on a magstripe card. After
 * the command is issued, the user must swipe a card through the MSR206.
 * The function will block until the card is read and the MSR206 is
 * ready to return data from the tracks. The caller must allocate
 * a pointer to an ::msr_tracks_t structure and supply a pointer
 * to this structure via the tracks argument.
 *
 * Unlike the msr_iso_read() function, this routine bypasses the
 * MSR206's internal data parser and returns data representing the
 * raw bit pattern on the magnetic media. It is up to the caller to
 * decode this data into a useful form.
 *
 * @param fd The device's fd.
 * @param tracks A pointer to the ::msr_tracks_t to populate.
 * @return ::LIBMSR_ERR_OK on success.
 * @return ::LIBMSR_ERR_DEVICE on device failure.
 */
extern int msr_raw_read(int fd, msr_tracks_t *tracks);

/**
 * @brief Write raw data to a card.
 * @details This routine issues an ::MSR_CMD_RAW_WRITE command to the device to
 * write arbitrary data to a magstripe card. The card does not need
 * to be erased (any existing data is overwritten). The caller must
 * allocate an ::msr_tracks_t structure and populate it with the data
 * to be written to the card, then supply a pointer to this structure
 * via the tracks argument. The data can be in any format.
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
 * @param fd The device's fd.
 * @param tracks A pointer to the ::msr_tracks_t data to write.
 * @return ::LIBMSR_ERR_OK on success.
 * @return ::LIBMSR_ERR_DEVICE on device failure.
 */
extern int msr_raw_write(int fd, msr_tracks_t *tracks);

/**
 * @brief Erase one or more tracks on a card.
 * @details This routine issues an ::MSR_CMD_ERASE command to the device to
 * read erase a magstripe card. After the command is issued,
 * the user must swipe a card through the MSR206. The function
 * will block until the device returns a response code indicating
 * that the erase operation completed successfully.
 *
 * This function can erase various combinations of tracks, or erase
 * all of them, depending on the tracks value specified:
 *
 * ::MSR_ERASE_TK1	erase track 1 only
 *
 * ::MSR_ERASE_TK2	erase track 2 only
 *
 * ::MSR_ERASE_TK3	erase track 3 only
 *
 * ::MSR_ERASE_TK1_TK2	erase tracks 1 and 2
 *
 * ::MSR_ERASE_TK1_TK3	erase tracks 1 and 3
 *
 * ::MSR_ERASE_TK2_TK3	erase tracks 2 and 3
 *
 * ::MSR_ERASE_ALL	erase all tracks
 *
 * @param fd The device's fd.
 * @param tracks The tracks to delete.
 * @return ::LIBMSR_ERR_OK on success.
 * @return ::LIBMSR_ERR_DEVICE on device failure.
 */
extern int msr_erase(int fd, uint8_t tracks);

/**
 * @brief Toggle the LEDs on the MSR device.
 * @details This function is used to manually control the LEDs on the
 * MSR206. The device has one green, one yellow and one red LED
 * mounted in it. A given LED is controlled by specifying led
 * as follows:
 *
 * ::MSR_CMD_LED_GRN_ON - turn on green LED
 *
 * ::MSR_CMD_LED_YLW_ON - turn on yellow LED
 *
 * ::MSR_CMD_LED_RED_ON - turn on red LED
 *
 * ::MSR_CMD_LED_OFF - turn all LEDs off
 *
 * After an LED control command is issued to the device, the
 * routine will pause for a tenth of a second to allow time for the
 * command to be processed and the LED to light up.
 *
 * @param fd The device's fd.
 * @param led The LED to control.
 * @return ::LIBMSR_ERR_OK on success.
 * @return ::LIBMSR_ERR_SERIAL | ::LIBMSR_ERR_DEVICE on error.
 */
extern int msr_flash_led(int fd, uint8_t led);

/**
 * @brief Set the MSR device's BPI value.
 * @details This function issues an ::MSR_CMD_SETBPI command to set the bits per
 * inch configuration for track 2. (This command has no effect for
 * other tracks.) It is assumed that this command only has meaning
 * when writing data. The MSR206 supports writing data on track 2
 * with bpi values. Valid options are either 75 or 210 bits per inch.
 *
 * @param fd The device's fd.
 * @param bpi The new BPI value.
 * @return ::LIBMSR_ERR_OK on success.
 * @return ::LIBMSR_ERR_DEVICE on device failure.
 */
extern int msr_set_bpi(int fd, uint8_t bpi);

/**
 * @brief Set the MSR device's BPC value for each track.
 * @details This function issues an ::MSR_CMD_SETBPC command to bits per
 * character configuration for all 3 tracks. (This command has no effect for
 * other tracks.) It is assumed that this command only has meaning
 * when using ISO read or write commands. The MSR206 supports writing
 * data with BPC values from 5 to 8.
 *
 * @param fd The device's fd.
 * @param bpc1 The new BPC value for track 1.
 * @param bpc2 The new BPC value for track 2.
 * @param bpc3 The new BPC value for track 3.
 * @return ::LIBMSR_ERR_OK on success.
 * @return ::LIBMSR_ERR_DEVICE on device failure.
 */
extern int msr_set_bpc(int fd, uint8_t bpc1, uint8_t bpc2, uint8_t bpc3);

/**
 * @brief Reverse a ::msr_tracks_t structure in-place.
 *
 * @param tracks A pointer to the ::msr_tracks_t to reverse.
 * @return ::LIBMSR_ERR_OK
 */
extern int msr_reverse_tracks(msr_tracks_t *tracks);

/**
 * @brief Reverse a ::msr_track_t structure in-place.
 *
 * @param track A point to the ::msr_track_t to reverse.
 * @return ::LIBMSR_ERR_OK
 */
extern int msr_reverse_track(msr_track_t *track);

/**
 * @brief Dump a "pretty" hexadecimal representation of tracks to a fd.
 *
 * @param fd The fd to write to.
 * @param tracks The tracks to dump.
 */
extern void msr_pretty_output_hex(int fd, msr_tracks_t tracks);

/**
 * @brief Dump a "pretty" string representation of tracks to a fd.
 *
 * @param fd The fd to write to.
 * @param tracks The tracks to dump.
 */
extern void msr_pretty_output_string(int fd, msr_tracks_t tracks);

/**
 * @brief Dump a "pretty" binary representation of tracks to a fd.
 *
 * @param fd The fd to write to.
 * @param tracks The tracks to dump.
 */
extern void msr_pretty_output_bits(int fd, msr_tracks_t tracks);

/**
 * @brief Dump a "pretty" hexadecimal representation of tracks to stdout.
 *
 * @param tracks The tracks to dump.
 */
extern void msr_pretty_printer_hex(msr_tracks_t tracks);

/**
 * @brief Dump a "pretty" string representation of tracks to stdout.
 *
 * @param tracks The tracks to dump.
 */
extern void msr_pretty_printer_string(msr_tracks_t tracks);

/**
 * @brief Dump a "pretty" binary representation of tracks to stdout.
 *
 * @param tracks The tracks to dump.
 */
extern void msr_pretty_printer_bits(msr_tracks_t tracks);

/**
 * @brief Reverse a single byte.
 *
 * @param byte The byte to reverse.
 * @return The reversed byte.
 */
extern const unsigned char msr_reverse_byte(const unsigned char byte);
