// Copyright (C) 2012 Intel Corporation.
// Copyright 2013 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "private/ewk_error_private.h"

#include "public/ewk_error.h"

/* LCOV_EXCL_START */
int ConvertErrorCode(int error_code) {
  switch (error_code) {
    case net::ERR_ABORTED:
      return EWK_ERROR_CODE_CANCELED;

    case net::ERR_FILE_EXISTS:
    case net::ERR_FILE_NOT_FOUND:
    case net::ERR_FILE_NO_SPACE:
    case net::ERR_FILE_PATH_TOO_LONG:
    case net::ERR_FILE_TOO_BIG:
    case net::ERR_FILE_VIRUS_INFECTED:
      return EWK_ERROR_CODE_FAILED_FILE_IO;

    case net::ERR_CONNECTION_ABORTED:
    case net::ERR_CONNECTION_CLOSED:
    case net::ERR_CONNECTION_FAILED:
    case net::ERR_CONNECTION_REFUSED:
    case net::ERR_CONNECTION_RESET:
      return EWK_ERROR_CODE_CANT_CONNECT;

    case net::ERR_DNS_MALFORMED_RESPONSE:
    case net::ERR_DNS_SERVER_REQUIRES_TCP:
    case net::ERR_DNS_SERVER_FAILED:
    case net::ERR_DNS_TIMED_OUT:
    case net::ERR_DNS_CACHE_MISS:
    case net::ERR_DNS_SEARCH_EMPTY:
    case net::ERR_DNS_SORT_ERROR:
    case net::ERR_HOST_RESOLVER_QUEUE_TOO_LARGE:
    case net::ERR_NAME_NOT_RESOLVED:
    case net::ERR_NAME_RESOLUTION_FAILED:
      return EWK_ERROR_CODE_CANT_LOOKUP_HOST;

    case net::ERR_BAD_SSL_CLIENT_AUTH_CERT:
    case net::ERR_CERT_ERROR_IN_SSL_RENEGOTIATION:
    case net::ERR_SSL_CLIENT_AUTH_SIGNATURE_FAILED:
    case net::ERR_SSL_HANDSHAKE_NOT_COMPLETED:
      return EWK_ERROR_CODE_FAILED_TLS_HANDSHAKE;

    case net::ERR_CERT_AUTHORITY_INVALID:
    case net::ERR_CERT_COMMON_NAME_INVALID:
    case net::ERR_CERT_CONTAINS_ERRORS:
    case net::ERR_CERT_DATE_INVALID:
    case net::ERR_CERT_INVALID:
    case net::ERR_CERT_NAME_CONSTRAINT_VIOLATION:
    case net::ERR_CERT_NON_UNIQUE_NAME:
    case net::ERR_CERT_NO_REVOCATION_MECHANISM:
    case net::ERR_CERT_REVOKED:
    case net::ERR_CERT_UNABLE_TO_CHECK_REVOCATION:
    case net::ERR_CERT_WEAK_KEY:
    case net::ERR_CERT_WEAK_SIGNATURE_ALGORITHM:
      return EWK_ERROR_CODE_INVALID_CERTIFICATE;

    case net::ERR_TIMED_OUT:
    case net::ERR_CONNECTION_TIMED_OUT:
      return EWK_ERROR_CODE_REQUEST_TIMEOUT;

    case net::ERR_TOO_MANY_REDIRECTS:
      return EWK_ERROR_CODE_TOO_MANY_REDIRECTS;

    case net::ERR_TEMPORARILY_THROTTLED:
      return EWK_ERROR_CODE_TOO_MANY_REQUESTS;

    case net::ERR_ADDRESS_INVALID:
    case net::ERR_INVALID_URL:
      return EWK_ERROR_CODE_BAD_URL;

    case net::ERR_DISALLOWED_URL_SCHEME:
    case net::ERR_UNKNOWN_URL_SCHEME:
      return EWK_ERROR_CODE_UNSUPPORTED_SCHEME;

    case net::ERR_CLIENT_AUTH_CERT_TYPE_UNSUPPORTED:
    case net::ERR_INVALID_AUTH_CREDENTIALS:
    case net::ERR_MALFORMED_IDENTITY:
    case net::ERR_MISCONFIGURED_AUTH_ENVIRONMENT:
    case net::ERR_MISSING_AUTH_CREDENTIALS:
    case net::ERR_PROXY_AUTH_REQUESTED_WITH_NO_CONNECTION:
    case net::ERR_PROXY_AUTH_UNSUPPORTED:
    case net::ERR_UNEXPECTED_PROXY_AUTH:
    case net::ERR_UNSUPPORTED_AUTH_SCHEME:
      return EWK_ERROR_CODE_AUTHENTICATION;

    case net::ERR_CACHE_CHECKSUM_MISMATCH:
    case net::ERR_CACHE_CHECKSUM_READ_FAILURE:
    case net::ERR_CACHE_LOCK_TIMEOUT:
    case net::ERR_CACHE_RACE:
    case net::ERR_IMPORT_SERVER_CERT_FAILED:
      return EWK_ERROR_CODE_INTERNAL_SERVER;

    default:
      return EWK_ERROR_CODE_UNKNOWN;
  }
}

const char* GetDescriptionFromErrorCode(int error_code) {
  switch (ConvertErrorCode(error_code)) {
    case EWK_ERROR_CODE_CANCELED:
      return "User canceled";
    case EWK_ERROR_CODE_CANT_SUPPORT_MIMETYPE:
      return "Can't show page for this MIME Type";
    case EWK_ERROR_CODE_FAILED_FILE_IO:
      return "Error regarding to file io";
    case EWK_ERROR_CODE_CANT_CONNECT:
      return "Cannot connect to Network";
    case EWK_ERROR_CODE_CANT_LOOKUP_HOST:
      return "Fail to look up host from DNS";
    case EWK_ERROR_CODE_FAILED_TLS_HANDSHAKE:
      return "Fail to SSL/TLS handshake";
    case EWK_ERROR_CODE_INVALID_CERTIFICATE:
      return "Received certificate is invalid";
    case EWK_ERROR_CODE_REQUEST_TIMEOUT:
      return "Connection timeout";
    case EWK_ERROR_CODE_TOO_MANY_REDIRECTS:
      return "Too many redirects";
    case EWK_ERROR_CODE_TOO_MANY_REQUESTS:
      return "Too many requests during this load";
    case EWK_ERROR_CODE_BAD_URL:
      return "Malformed url";
    case EWK_ERROR_CODE_UNSUPPORTED_SCHEME:
      return "Unsupported scheme";
    case EWK_ERROR_CODE_AUTHENTICATION:
      return "User authentication failed on server";
    case EWK_ERROR_CODE_INTERNAL_SERVER:
      return "Web server has internal server error";
    case EWK_ERROR_CODE_UNKNOWN:
      return "Unknown";
    default:
      NOTREACHED();
      return "Unknown";
  }
}
/* LCOV_EXCL_STOP */
