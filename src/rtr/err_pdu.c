#include "err_pdu.h"

#include <unistd.h>
#include "pdu_sender.h"
#include "log.h"

typedef enum rtr_error_code {
	ERR_PDU_CORRUPT_DATA			= 0,
	ERR_PDU_INTERNAL_ERROR			= 1,
	ERR_PDU_NO_DATA_AVAILABLE		= 2,
	ERR_PDU_INVALID_REQUEST			= 3,
	ERR_PDU_UNSUP_PROTO_VERSION		= 4,
	ERR_PDU_UNSUP_PDU_TYPE			= 5,
	ERR_PDU_WITHDRAWAL_UNKNOWN		= 6,
	ERR_PDU_DUPLICATE_ANNOUNCE		= 7,
	/* RTRv1 only, so not used yet. */
	ERR_PDU_UNEXPECTED_PROTO_VERSION	= 8,
} rtr_error_code_t;

/*
 * TODO (urgent) According to the function below, NO_DATA_AVAILABLE is not
 * fatal. However, some callers of this function are terminating the connection
 * regardless of that.
 */
static int
err_pdu_send(int fd, rtr_error_code_t code, struct rtr_request const *request,
    char const *message_const)
{
	char *message;

	/*
	 * This function must always return error so callers can interrupt
	 * themselves easily.
	 * But note that not all callers should use this.
	 * TODO (now) Prevent errors to errors
	 * (It's harder than it seems, because request->pdu is sometimes NULL.)
	 */

	message = (message_const != NULL) ? strdup(message_const) : NULL;
	send_error_report_pdu(fd, code, request, message);
	free(message);

	return -EINVAL;
}

int
err_pdu_send_corrupt_data(int fd, struct rtr_request const *request,
    char const *message)
{
	return err_pdu_send(fd, ERR_PDU_CORRUPT_DATA, request, message);
}

/*
 * Please note: If you're planning to send this error due to a memory
 * allocation failure, you probably shouldn't; you'd likely only aggravate the
 * problem.
 */
int
err_pdu_send_internal_error(int fd)
{
	return err_pdu_send(fd, ERR_PDU_INTERNAL_ERROR, NULL, NULL);
}

int
err_pdu_send_no_data_available(int fd)
{
	return err_pdu_send(fd, ERR_PDU_NO_DATA_AVAILABLE, NULL, NULL);
}

int
err_pdu_send_invalid_request(int fd, struct rtr_request const *request,
    char const *message)
{
	return err_pdu_send(fd, ERR_PDU_INVALID_REQUEST, request, message);
}

/* Caution: @header is supposed to be in serialized form. */
int
err_pdu_send_invalid_request_truncated(int fd, unsigned char *header,
    char const *message)
{
	struct rtr_request request = {
		.bytes = header,
		.bytes_len = RTRPDU_HEADER_LEN,
		.pdu = NULL,
	};
	return err_pdu_send_invalid_request(fd, &request, message);
}

int
err_pdu_send_unsupported_proto_version(int fd)
{
	return err_pdu_send(fd, ERR_PDU_UNSUP_PROTO_VERSION, NULL, NULL);
}

int
err_pdu_send_unsupported_pdu_type(int fd, struct rtr_request const *request)
{
	return err_pdu_send(fd, ERR_PDU_UNSUP_PDU_TYPE, request, NULL);
}

bool
err_pdu_is_fatal(uint16_t code)
{
	/*
	 * Only NO_DATA_AVAILABLE error isn't fatal
	 *
	 * Addendum: Note that this is only non-fatal if we're the ones sending
	 * it. If the clients is the one telling us this, then it probably
	 * counts as "erroneous Error Report PDU", which is totally fatal.
	 */
	return code != ERR_PDU_NO_DATA_AVAILABLE;
}

char const *
err_pdu_to_string(uint16_t code)
{
	switch ((rtr_error_code_t) code) {
	case ERR_PDU_CORRUPT_DATA:
		return "Corrupt Data";
	case ERR_PDU_INTERNAL_ERROR:
		return "Internal Error";
	case ERR_PDU_NO_DATA_AVAILABLE:
		return "No Data Available";
	case ERR_PDU_INVALID_REQUEST:
		return "Invalid Request";
	case ERR_PDU_UNSUP_PROTO_VERSION:
		return "Unsupported Protocol Version";
	case ERR_PDU_UNSUP_PDU_TYPE:
		return "Unsupported PDU Type";
	case ERR_PDU_WITHDRAWAL_UNKNOWN:
		return "Withdrawal of Unknown Record";
	case ERR_PDU_DUPLICATE_ANNOUNCE:
		return "Duplicate Announcement Received";
	case ERR_PDU_UNEXPECTED_PROTO_VERSION:
		return "Unexpected Protocol Version";
	}

	return "Unknown error code";
}
