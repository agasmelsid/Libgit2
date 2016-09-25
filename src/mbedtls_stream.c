/*
 * Copyright (C) the libgit2 contributors. All rights reserved.
 *
 * This file is part of libgit2, distributed under the GNU GPL v2 with
 * a Linking Exception. For full terms see the included COPYING file.
 */

#ifdef GIT_MBEDTLS

#include <ctype.h>

#include "global.h"
#include "stream.h"
#include "socket_stream.h"
#include "git2/transport.h"

#ifdef GIT_CURL
# include "curl_stream.h"
#endif

#include "mbedtls/config.h"
#include <mbedtls/x509.h>
#include <mbedtls/x509_crt.h>
#include <mbedtls/error.h>
#include "mbedtls/net.h"
#include "mbedtls/debug.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#include "mbedtls/certs.h"

#ifndef OPENSSLDIR
# define OPENSSLDIR              "/usr/lib/ssl"
#endif
#define X509_CERT_DIR            OPENSSLDIR "/certs"
#define X509_CERT_FILE           OPENSSLDIR "/cert.pem"
#define X509_CERT_DIR_EVP        "SSL_CERT_DIR"
#define X509_CERT_FILE_EVP       "SSL_CERT_FILE"

mbedtls_ssl_config *git__ssl_conf;
mbedtls_entropy_context *mbedtls_entropy;

#define GIT_SSL_DEFAULT_CIPHERS "ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES256-GCM-SHA384:ECDHE-RSA-AES256-GCM-SHA384:DHE-RSA-AES128-GCM-SHA256:DHE-DSS-AES128-GCM-SHA256:DHE-RSA-AES256-GCM-SHA384:DHE-DSS-AES256-GCM-SHA384:ECDHE-ECDSA-AES128-SHA256:ECDHE-RSA-AES128-SHA256:ECDHE-ECDSA-AES128-SHA:ECDHE-RSA-AES128-SHA:ECDHE-ECDSA-AES256-SHA384:ECDHE-RSA-AES256-SHA384:ECDHE-ECDSA-AES256-SHA:ECDHE-RSA-AES256-SHA:DHE-RSA-AES128-SHA256:DHE-RSA-AES256-SHA256:DHE-RSA-AES128-SHA:DHE-RSA-AES256-SHA:DHE-DSS-AES128-SHA256:DHE-DSS-AES256-SHA256:DHE-DSS-AES128-SHA:DHE-DSS-AES256-SHA:AES128-GCM-SHA256:AES256-GCM-SHA384:AES128-SHA256:AES256-SHA256:AES128-SHA:AES256-SHA"

/**
 * This function aims to clean-up the SSL context which
 * we allocated.
 */
static void shutdown_ssl(void)
{
    if (git__ssl_conf) {
        mbedtls_x509_crt_free(git__ssl_conf->ca_chain);
        git__free(git__ssl_conf->ca_chain);
        mbedtls_ctr_drbg_free(git__ssl_conf->p_rng);
        git__free(git__ssl_conf->p_rng);
        mbedtls_ssl_config_free(git__ssl_conf);
        git__free(git__ssl_conf);
        git__ssl_conf = NULL;
    }
    if (mbedtls_entropy) {
        mbedtls_entropy_free(mbedtls_entropy);
        git__free(mbedtls_entropy);
        mbedtls_entropy = NULL;
    }
}

int git_mbedtls_stream_global_init(void)
{
    int ret, isdir;
    char *crtpath;
    struct stat statbuf;
    // const int *cipherids;
    // const char *ciphers = git_libgit2__ssl_ciphers();

    mbedtls_ctr_drbg_context *ctr_drbg;

    mbedtls_entropy = git__malloc(sizeof(mbedtls_entropy_context));
    mbedtls_entropy_init(mbedtls_entropy);

    // Seeding the random number generator
    ctr_drbg = git__malloc(sizeof(mbedtls_ctr_drbg_context));
    mbedtls_ctr_drbg_init(ctr_drbg);
    if (mbedtls_ctr_drbg_seed(ctr_drbg,
                mbedtls_entropy_func,
                mbedtls_entropy, NULL, 0) != 0) {
        mbedtls_ctr_drbg_free(ctr_drbg);
        mbedtls_entropy_free(mbedtls_entropy);
        git__free(ctr_drbg);
        git__free(mbedtls_entropy);
        return -1;
    }

    // configure TLSv1
    git__ssl_conf = git__malloc(sizeof(mbedtls_ssl_config));
    mbedtls_ssl_config_init(git__ssl_conf);
    if ( mbedtls_ssl_config_defaults(git__ssl_conf,
                MBEDTLS_SSL_IS_CLIENT,
                MBEDTLS_SSL_TRANSPORT_STREAM,
                MBEDTLS_SSL_PRESET_DEFAULT ) != 0) {
        mbedtls_ctr_drbg_free(ctr_drbg);
        git__free(ctr_drbg);
        mbedtls_ssl_config_free(git__ssl_conf);
        git__free(git__ssl_conf);
        git__ssl_conf = NULL;
        return -1;
    }

    mbedtls_ssl_conf_authmode(git__ssl_conf, MBEDTLS_SSL_VERIFY_REQUIRED);
    mbedtls_ssl_conf_rng(git__ssl_conf, mbedtls_ctr_drbg_random, ctr_drbg);

    // find locations for which CA certificates
    isdir = 0;
    crtpath = getenv(X509_CERT_FILE_EVP);
    ret = crtpath != NULL && stat(crtpath, &statbuf) == 0 && S_ISREG(statbuf.st_mode) ? 0 : 1;
    if (ret) {
        isdir = 1;
        crtpath = getenv(X509_CERT_DIR_EVP);
        ret = crtpath != NULL && stat(crtpath, &statbuf) == 0 && S_ISDIR(statbuf.st_mode) ? 0 : 1;
    }
    if (ret) {
        isdir = 0;
        crtpath = X509_CERT_FILE;
        ret = crtpath != NULL && stat(crtpath, &statbuf) == 0 && S_ISREG(statbuf.st_mode) ? 0 : 1;
    }
    if (ret) {
        isdir = 1;
        crtpath = X509_CERT_DIR;
        ret = crtpath != NULL && stat(crtpath, &statbuf) == 0 && S_ISDIR(statbuf.st_mode) ? 0 : 1;
    }

    // cannot find CA certificates
    if (ret) {
        mbedtls_ctr_drbg_free(ctr_drbg);
        git__free(ctr_drbg);
        mbedtls_ssl_config_free(git__ssl_conf);
        git__free(git__ssl_conf);
        git__ssl_conf = NULL;
        return -1;
    } else {
        // set root certificates
        mbedtls_x509_crt *cacert = git__malloc(sizeof(mbedtls_x509_crt));
        mbedtls_x509_crt_init(cacert);
        if (isdir)
            ret = mbedtls_x509_crt_parse_path(cacert, crtpath);
        else
            ret = mbedtls_x509_crt_parse_file(cacert, crtpath);

        if (ret) {
            giterr_set(GITERR_SSL, "failed to load CA certificates: %d", ret);
            mbedtls_x509_crt_free(cacert);
            git__free(cacert);
            mbedtls_ctr_drbg_free(ctr_drbg);
            git__free(ctr_drbg);
            mbedtls_ssl_config_free(git__ssl_conf);
            git__free(git__ssl_conf);
            git__ssl_conf = NULL;
            return -1;
        } else {
            mbedtls_ssl_conf_ca_chain(git__ssl_conf, cacert, NULL);
        }
    }

    // set the list of allowed ciphersuites
    // if (!ciphers) {
    //     cipherids = mbedtls_ssl_list_ciphersuites();
    // }
    // mbedtls_ssl_conf_ciphersuites(git__ssl_conf, cipherids);

    git__on_shutdown(shutdown_ssl);

    return 0;
}

static int bio_read(void *b, unsigned char *buf, size_t len)
{
    git_stream *io = (git_stream *) b;
    return (int) git_stream_read(io, buf, len);
}

static int bio_write(void *b, const unsigned char *buf, size_t len)
{
    git_stream *io = (git_stream *) b;
    return (int) git_stream_write(io, (const char *)buf, len, 0);
}

static int ssl_set_error(mbedtls_ssl_context *ssl, int error)
{
    char errbuf[512];
    int ret = -1;

    assert(error != MBEDTLS_ERR_SSL_WANT_READ);
    assert(error != MBEDTLS_ERR_SSL_WANT_WRITE);

    if (error != 0)
        mbedtls_strerror( error, errbuf, 512 );

    switch(error){
    case 0:
        giterr_set(GITERR_SSL, "SSL error: unknown error");
        break;
    case MBEDTLS_ERR_X509_CERT_VERIFY_FAILED:
        giterr_set(GITERR_SSL, "SSL error: %x[%x] - %s", error, ssl->session_negotiate->verify_result, errbuf);
        ret = GIT_ECERTIFICATE;
        break;
    default:
        giterr_set(GITERR_SSL, "SSL error: %x - %s", error, errbuf);
    }

    return ret;
}

static int ssl_teardown(mbedtls_ssl_context *ssl)
{
    int ret = 0;

    ret = mbedtls_ssl_close_notify(ssl);
    if (ret < 0)
        ret = ssl_set_error(ssl, ret);

    mbedtls_ssl_free(ssl);
    return ret;
}

static int check_host_name(const char *name, const char *host)
{
    if (!strcasecmp(name, host))
        return 0;

    if (gitno__match_host(name, host) < 0)
        return -1;

    return 0;
}

static int verify_server_cert(mbedtls_ssl_context *ssl, const char *host)
{
    const mbedtls_x509_crt *cert;
    const mbedtls_x509_sequence *alts;
    int ret, matched = -1;
    size_t sn_size = 512;
    char subject_name[sn_size], alt_name[sn_size];


    if (( ret = mbedtls_ssl_get_verify_result(ssl) ) != 0) {
        char vrfy_buf[512];
        mbedtls_x509_crt_verify_info( vrfy_buf, sizeof( vrfy_buf ), "  ! ", ret );
        giterr_set(GITERR_SSL, "The SSL certificate is invalid: %s", vrfy_buf);
        return GIT_ECERTIFICATE;
    }

    cert = mbedtls_ssl_get_peer_cert(ssl);
    if (!cert) {
        giterr_set(GITERR_SSL, "the server did not provide a certificate");
        return -1;
    }

    /* Check the alternative names */
    alts = &cert->subject_alt_names;
    while (alts != NULL && matched != 1) {
        // Buffer is too small
        if( alts->buf.len >= sn_size )
            goto on_error;

        memcpy(alt_name, alts->buf.p, alts->buf.len);
        alt_name[alts->buf.len] = '\0';

        if (!memchr(alt_name, '\0', alts->buf.len)) {
            if (check_host_name(alt_name, host) < 0)
                matched = 0;
            else
                matched = 1;
        }

        alts = alts->next;
    }
    if (matched == 0)
        goto cert_fail_name;

    if (matched == 1)
        return 0;

    /* If no alternative names are available, check the common name */
    ret = mbedtls_x509_dn_gets(subject_name, sn_size, &cert->subject);
    if (ret == 0)
        goto on_error;
    if (memchr(subject_name, '\0', ret))
        goto cert_fail_name;

    if (check_host_name(subject_name, host) < 0)
        goto cert_fail_name;

    return 0;

on_error:
    return ssl_set_error(ssl, 0);

cert_fail_name:
    giterr_set(GITERR_SSL, "hostname does not match certificate");
    return GIT_ECERTIFICATE;
}

typedef struct {
    git_stream parent;
    git_stream *io;
    bool connected;
    char *host;
    mbedtls_ssl_context *ssl;
    git_cert_x509 cert_info;
} mbedtls_stream;


int mbedtls_connect(git_stream *stream)
{
    int ret;
    mbedtls_stream *st = (mbedtls_stream *) stream;

    if ((ret = git_stream_connect(st->io)) < 0)
        return ret;

    st->connected = true;

    mbedtls_ssl_set_hostname(st->ssl, st->host);

    mbedtls_ssl_set_bio(st->ssl, st->io, bio_write, bio_read, NULL);

    if ((ret = mbedtls_ssl_handshake(st->ssl)) != 0)
        return ssl_set_error(st->ssl, ret);

    return verify_server_cert(st->ssl, st->host);
}

int mbedtls_certificate(git_cert **out, git_stream *stream)
{
    unsigned char *encoded_cert;
    mbedtls_stream *st = (mbedtls_stream *) stream;

    const mbedtls_x509_crt *cert = mbedtls_ssl_get_peer_cert(st->ssl);
    if (!cert) {
        giterr_set(GITERR_SSL, "the server did not provide a certificate");
        return -1;
    }

    /* Retrieve the length of the certificate first */
    if (cert->raw.len == 0) {
        giterr_set(GITERR_NET, "failed to retrieve certificate information");
        return -1;
    }

    encoded_cert = git__malloc(cert->raw.len);
    GITERR_CHECK_ALLOC(encoded_cert);
    memcpy(encoded_cert, cert->raw.p, cert->raw.len);

    st->cert_info.parent.cert_type = GIT_CERT_X509;
    st->cert_info.data = encoded_cert;
    st->cert_info.len = cert->raw.len;

    *out = &st->cert_info.parent;

    return 0;
}

static int mbedtls_set_proxy(git_stream *stream, const git_proxy_options *proxy_options)
{
    mbedtls_stream *st = (mbedtls_stream *) stream;

    return git_stream_set_proxy(st->io, proxy_options);
}

ssize_t mbedtls_stream_write(git_stream *stream, const char *data, size_t len, int flags)
{
    mbedtls_stream *st = (mbedtls_stream *) stream;
    int ret;

    GIT_UNUSED(flags);

    if ((ret = mbedtls_ssl_write(st->ssl, (const unsigned char *)data, len)) <= 0) {
        return ssl_set_error(st->ssl, ret);
    }

    return ret;
}

ssize_t mbedtls_stream_read(git_stream *stream, void *data, size_t len)
{
    mbedtls_stream *st = (mbedtls_stream *) stream;
    int ret;

    if ((ret = mbedtls_ssl_read(st->ssl, (unsigned char *)data, len)) <= 0)
        ssl_set_error(st->ssl, ret);

    return ret;
}

int mbedtls_stream_close(git_stream *stream)
{
    mbedtls_stream *st = (mbedtls_stream *) stream;
    int ret = 0;

    if (st->connected && (ret = ssl_teardown(st->ssl)) != 0)
        return -1;

    st->connected = false;

    return git_stream_close(st->io);
}

void mbedtls_stream_free(git_stream *stream)
{
    mbedtls_stream *st = (mbedtls_stream *) stream;

    git__free(st->host);
    git__free(st->cert_info.data);
    git_stream_free(st->io);
    git__free(st->ssl);
    git__free(st);
}

int git_mbedtls_stream_new(git_stream **out, const char *host, const char *port)
{
    int error;
    mbedtls_stream *st;

    st = git__calloc(1, sizeof(mbedtls_stream));
    GITERR_CHECK_ALLOC(st);

#ifdef GIT_CURL
    error = git_curl_stream_new(&st->io, host, port);
#else
    error = git_socket_stream_new(&st->io, host, port);
#endif

    if (error < 0)
        return error;

    st->ssl = git__malloc(sizeof(mbedtls_ssl_context));
    GITERR_CHECK_ALLOC(st->ssl);
    mbedtls_ssl_init(st->ssl);
    if( (error = mbedtls_ssl_setup(st->ssl, git__ssl_conf)) != 0 ) {
        mbedtls_ssl_free(st->ssl);
        giterr_set(GITERR_SSL, "failed to create ssl object");
        return -1;
    }

    st->host = git__strdup(host);
    GITERR_CHECK_ALLOC(st->host);

    st->parent.version = GIT_STREAM_VERSION;
    st->parent.encrypted = 1;
    st->parent.proxy_support = git_stream_supports_proxy(st->io);
    st->parent.connect = mbedtls_connect;
    st->parent.certificate = mbedtls_certificate;
    st->parent.set_proxy = mbedtls_set_proxy;
    st->parent.read = mbedtls_stream_read;
    st->parent.write = mbedtls_stream_write;
    st->parent.close = mbedtls_stream_close;
    st->parent.free = mbedtls_stream_free;

    *out = (git_stream *) st;
    return 0;
}

#else

#include "stream.h"
#include "git2/sys/openssl.h"

int git_mbedtls_stream_global_init(void)
{
    return 0;
}

int git_mbedtls_stream_new(git_stream **out, const char *host, const char *port)
{
    GIT_UNUSED(out);
    GIT_UNUSED(host);
    GIT_UNUSED(port);

    giterr_set(GITERR_SSL, "mbedTLS is not supported in this version");
    return -1;
}

#endif

