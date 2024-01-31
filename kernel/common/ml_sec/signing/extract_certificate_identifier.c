#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <openssl/x509.h>
#include <openssl/opensslv.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/asn1.h>
#include <openssl/err.h>

#if OPENSSL_VERSION_NUMBER < 0x10100000L
#define USE_LEGACY_OPENSSL
#endif

int main(int argc, char *argv[])
{
	BIO *certificate_bio   = NULL;
	X509 *certificate_x509 = NULL;
	int ret  = 0;
	size_t i = 0;

	size_t issuer_start_idx;
	const unsigned char *issuer_bytes_start;
	long issuer_bytes_len;
	int tag, xclass;
	int asn1_ret;

	ASN1_INTEGER *serial;
	X509_NAME *issuer;
	char *issuer_name;
	size_t issuer_length;

	if (argc != 2) {
		printf("Usage: %s <certificate_file_path>\n", argv[0]);
		ret = 1;
		goto exit;
	}

	OpenSSL_add_all_algorithms();
	ERR_load_BIO_strings();
	ERR_load_crypto_strings();

	certificate_bio = BIO_new(BIO_s_file());
	ret = BIO_read_filename(certificate_bio, argv[1]);
	if (!ret) {
		printf("\nerror: failed to read filename\n");
		ret = 1;
		goto exit;
	}

	certificate_x509 = PEM_read_bio_X509(certificate_bio, NULL, 0, NULL);
	if (!certificate_x509) {
		printf("\nerror: error loading certificate into memory\n");
		ret = 2;
		goto exit;
	}

	/* Get serial number */
	serial = X509_get_serialNumber(certificate_x509);
	if (serial->length == 0) {
		printf("\nerror: empty serial number");
		ret = 3;
		goto exit;
	}

	/* Get issuer name */
	issuer = X509_get_issuer_name(certificate_x509);
	if (issuer == NULL) {
		printf("\nerror: failed to extract issuer name");
		ret = 4;
		goto exit;
	}
#ifdef USE_LEGACY_OPENSSL
#ifdef OPENSSL_NO_BUFFER
	issuer_name = issuer->bytes;
	issuer_length = strlen(issuer_name);
#else
	issuer_name = issuer->bytes->data;
	issuer_length = issuer->bytes->length;
#endif
#else
	issuer_name = X509_NAME_oneline(issuer, NULL, 0);
	issuer_length = strlen(issuer_name);
#endif /* USE_LEGACY_OPENSSL */
	if (issuer_name == NULL || issuer_length == 0) {
		printf("\nerror: empty issuer name");
		ret = 5;
		goto exit;
	}

	/* Parse the issuer "sequence" header so we can skip it */
	issuer_bytes_start = (const unsigned char *)issuer_name;
	asn1_ret = ASN1_get_object(&issuer_bytes_start,
				   &issuer_bytes_len,
				   &tag,
				   &xclass,
				   issuer_length);

	if (asn1_ret & 0x80) {
		printf("\nerror: ASN1_get_object");
		ret = 6;
		goto exit;
	}

	if (tag != V_ASN1_SEQUENCE) {
		printf("\nerror: unexpected issuer head tag");
		ret = 7;
		goto exit;
	}

	issuer_start_idx = issuer_bytes_start -
		(const unsigned char *)issuer_name;

	/* Print serial leading zero if needed */
	if ((signed char)(serial->data[i]) < 0)
		printf("00");

	/* Print serial */
	for (i = 0; i < serial->length; i++)
		printf("%02x", serial->data[i]);

	/* Print issuer name */
	for (i = issuer_start_idx; i < issuer_length; i++)
		printf("%02x", (unsigned char)issuer_name[i]);


exit:
	if (certificate_x509 != NULL)
		X509_free(certificate_x509);

	if (certificate_bio != NULL)
		BIO_free_all(certificate_bio);

	return ret;
}





