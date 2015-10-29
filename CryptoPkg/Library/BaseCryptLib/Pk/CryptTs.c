/** @file
  RFC3161 Timestamp Countersignature Verification over OpenSSL.
  The timestamp is generated by a TimeStamping Authority (TSA) and asserts that a
  publisher's signature existed before the specified time. The timestamp extends
  the lifetime of the signature when a signing certificate expires or is later
  revoked.

Copyright (c) 2014 - 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "InternalCryptLib.h"

#include <openssl/asn1.h>
#include <openssl/asn1t.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/pkcs7.h>

//
// OID ASN.1 Value for SPC_RFC3161_OBJID ("1.3.6.1.4.1.311.3.3.1")
//
UINT8 mSpcRFC3161OidValue[] = {
  0x2b, 0x06, 0x01, 0x04, 0x01, 0x82, 0x37, 0x03, 0x03, 0x01
  };

///
/// The messageImprint field SHOULD contain the hash of the datum to be
/// time-stamped.  The hash is represented as an OCTET STRING.  Its
/// length MUST match the length of the hash value for that algorithm
/// (e.g., 20 bytes for SHA-1 or 16 bytes for MD5).
///
/// MessageImprint ::= SEQUENCE  {
///   hashAlgorithm                AlgorithmIdentifier,
///   hashedMessage                OCTET STRING  }
///
typedef struct {
  X509_ALGOR         *HashAlgorithm;
  ASN1_OCTET_STRING  *HashedMessage;
} TS_MESSAGE_IMPRINT;

//
// ASN.1 Functions for TS_MESSAGE_IMPRINT
//
DECLARE_ASN1_FUNCTIONS (TS_MESSAGE_IMPRINT)
ASN1_SEQUENCE (TS_MESSAGE_IMPRINT) = {
  ASN1_SIMPLE (TS_MESSAGE_IMPRINT, HashAlgorithm, X509_ALGOR),
  ASN1_SIMPLE (TS_MESSAGE_IMPRINT, HashedMessage, ASN1_OCTET_STRING)
} ASN1_SEQUENCE_END (TS_MESSAGE_IMPRINT)
IMPLEMENT_ASN1_FUNCTIONS (TS_MESSAGE_IMPRINT)

///
/// Accuracy represents the time deviation around the UTC time contained
/// in GeneralizedTime of time-stamp token.
///
/// Accuracy ::= SEQUENCE {
///       seconds        INTEGER              OPTIONAL,
///       millis     [0] INTEGER  (1..999)    OPTIONAL,
///       micros     [1] INTEGER  (1..999)    OPTIONAL  }
///
typedef struct {
  ASN1_INTEGER  *Seconds;
  ASN1_INTEGER  *Millis;
  ASN1_INTEGER  *Micros;
} TS_ACCURACY;

//
// ASN.1 Functions for TS_ACCURACY
//
DECLARE_ASN1_FUNCTIONS (TS_ACCURACY)
ASN1_SEQUENCE (TS_ACCURACY) = {
  ASN1_OPT     (TS_ACCURACY, Seconds, ASN1_INTEGER),
  ASN1_IMP_OPT (TS_ACCURACY, Millis,  ASN1_INTEGER, 0),
  ASN1_IMP_OPT (TS_ACCURACY, Micros,  ASN1_INTEGER, 1)
} ASN1_SEQUENCE_END (TS_ACCURACY)
IMPLEMENT_ASN1_FUNCTIONS (TS_ACCURACY)

///
/// The timestamp token info resulting from a successful timestamp request,
/// as defined in RFC 3161.
///
///  TSTInfo ::= SEQUENCE  {
///     version                      INTEGER  { v1(1) },
///     policy                       TSAPolicyId,
///     messageImprint               MessageImprint,
///       -- MUST have the same value as the similar field in
///       -- TimeStampReq
///     serialNumber                 INTEGER,
///       -- Time-Stamping users MUST be ready to accommodate integers
///       -- up to 160 bits.
///     genTime                      GeneralizedTime,
///     accuracy                     Accuracy                 OPTIONAL,
///     ordering                     BOOLEAN             DEFAULT FALSE,
///     nonce                        INTEGER                  OPTIONAL,
///       -- MUST be present if the similar field was present
///       -- in TimeStampReq.  In that case it MUST have the same value.
///     tsa                          [0] GeneralName          OPTIONAL,
///     extensions                   [1] IMPLICIT Extensions   OPTIONAL  }
///
typedef struct {
  ASN1_INTEGER              *Version;
  ASN1_OBJECT               *Policy;
  TS_MESSAGE_IMPRINT        *MessageImprint;
  ASN1_INTEGER              *SerialNumber;
  ASN1_GENERALIZEDTIME      *GenTime;
  TS_ACCURACY               *Accuracy;
  ASN1_BOOLEAN              Ordering;
  ASN1_INTEGER              *Nonce;
  GENERAL_NAME              *Tsa;
  STACK_OF(X509_EXTENSION)  *Extensions;
} TS_TST_INFO;

//
// ASN.1 Functions for TS_TST_INFO
//
DECLARE_ASN1_FUNCTIONS (TS_TST_INFO)
ASN1_SEQUENCE (TS_TST_INFO) = {
  ASN1_SIMPLE (TS_TST_INFO, Version, ASN1_INTEGER),
  ASN1_SIMPLE (TS_TST_INFO, Policy, ASN1_OBJECT),
  ASN1_SIMPLE (TS_TST_INFO, MessageImprint, TS_MESSAGE_IMPRINT),
  ASN1_SIMPLE (TS_TST_INFO, SerialNumber, ASN1_INTEGER),
  ASN1_SIMPLE (TS_TST_INFO, GenTime, ASN1_GENERALIZEDTIME),
  ASN1_OPT    (TS_TST_INFO, Accuracy, TS_ACCURACY),
  ASN1_OPT    (TS_TST_INFO, Ordering, ASN1_FBOOLEAN),
  ASN1_OPT    (TS_TST_INFO, Nonce, ASN1_INTEGER),
  ASN1_EXP_OPT(TS_TST_INFO, Tsa, GENERAL_NAME, 0),
  ASN1_IMP_SEQUENCE_OF_OPT (TS_TST_INFO, Extensions, X509_EXTENSION, 1)
} ASN1_SEQUENCE_END (TS_TST_INFO)
IMPLEMENT_ASN1_FUNCTIONS (TS_TST_INFO)


/**
  Verification callback function to override any existing callbacks in OpenSSL
  for intermediate TSA certificate supports.

  @param[in]  Status   Original status before calling this callback.
  @param[in]  Context  X509 store context.

  @retval     1        Current X509 certificate is verified successfully.
  @retval     0        Verification failed.

**/
int
TSVerifyCallback (
  IN int             Status,
  IN X509_STORE_CTX  *Context
  )
{
  X509_OBJECT  *Obj;
  INTN         Error;
  INTN         Index;
  INTN         Count;

  Obj   = NULL;
  Error = (INTN) X509_STORE_CTX_get_error (Context);

  //
  // X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT and X509_V_ERR_UNABLE_TO_GET_ISSUER_
  // CERT_LOCALLY mean a X509 certificate is not self signed and its issuer
  // can not be found in X509_verify_cert of X509_vfy.c.
  // In order to support intermediate certificate node, we override the
  // errors if the certification is obtained from X509 store, i.e. it is
  // a trusted ceritifcate node that is enrolled by user.
  // Besides,X509_V_ERR_CERT_UNTRUSTED and X509_V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE
  // are also ignored to enable such feature.
  //
  if ((Error == X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT) ||
      (Error == X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY)) {
    Obj = (X509_OBJECT *) malloc (sizeof (X509_OBJECT));
    if (Obj == NULL) {
      return 0;
    }

    Obj->type      = X509_LU_X509;
    Obj->data.x509 = Context->current_cert;

    CRYPTO_w_lock (CRYPTO_LOCK_X509_STORE);

    if (X509_OBJECT_retrieve_match (Context->ctx->objs, Obj)) {
      Status = 1;
    } else {
      //
      // If any certificate in the chain is enrolled as trusted certificate,
      // pass the certificate verification.
      //
      if (Error == X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY) {
        Count = (INTN) sk_X509_num (Context->chain);
        for (Index = 0; Index < Count; Index++) {
          Obj->data.x509 = sk_X509_value (Context->chain, (int) Index);
          if (X509_OBJECT_retrieve_match (Context->ctx->objs, Obj)) {
            Status = 1;
            break;
          }
        }
      }
    }

    CRYPTO_w_unlock (CRYPTO_LOCK_X509_STORE);
  }

  if ((Error == X509_V_ERR_CERT_UNTRUSTED) ||
      (Error == X509_V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE)) {
    Status = 1;
  }

  if (Obj != NULL) {
    OPENSSL_free (Obj);
  }

  return Status;
}

/**
  Convert ASN.1 GeneralizedTime to EFI Time.

  @param[in]  Asn1Time         Pointer to the ASN.1 GeneralizedTime to be converted.
  @param[out] SigningTime      Return the corresponding EFI Time.

  @retval  TRUE   The time convertion succeeds.
  @retval  FALSE  Invalid parameters.

**/
BOOLEAN
EFIAPI
ConvertAsn1TimeToEfiTime (
  IN  ASN1_TIME  *Asn1Time,
  OUT EFI_TIME   *EfiTime
  )
{
  CONST CHAR8  *Str;
  UINTN        Index;

  if ((Asn1Time == NULL) || (EfiTime == NULL)) {
    return FALSE;
  }

  Str = (CONST CHAR8*)Asn1Time->data;
  SetMem (EfiTime, 0, sizeof (EFI_TIME));

  Index = 0;
  if (Asn1Time->type == V_ASN1_UTCTIME) {               /* two digit year */
    EfiTime->Year  = (Str[Index++] - '0') * 10;
    EfiTime->Year += (Str[Index++] - '0');
    if (EfiTime->Year < 70) {
      EfiTime->Year += 100;
    }
  } else if (Asn1Time->type == V_ASN1_GENERALIZEDTIME) { /* four digit year */
    EfiTime->Year  = (Str[Index++] - '0') * 1000;
    EfiTime->Year += (Str[Index++] - '0') * 100;
    EfiTime->Year += (Str[Index++] - '0') * 10;
    EfiTime->Year += (Str[Index++] - '0');
    if ((EfiTime->Year < 1900) || (EfiTime->Year > 9999)) {
      return FALSE;
    }
  }

  EfiTime->Month   = (Str[Index++] - '0') * 10;
  EfiTime->Month  += (Str[Index++] - '0');
  if ((EfiTime->Month < 1) || (EfiTime->Month > 12)) {
    return FALSE;
  }

  EfiTime->Day     = (Str[Index++] - '0') * 10;
  EfiTime->Day    += (Str[Index++] - '0');
  if ((EfiTime->Day < 1) || (EfiTime->Day > 31)) {
    return FALSE;
  }

  EfiTime->Hour    = (Str[Index++] - '0') * 10;
  EfiTime->Hour   += (Str[Index++] - '0');
  if (EfiTime->Hour > 23) {
    return FALSE;
  }

  EfiTime->Minute  = (Str[Index++] - '0') * 10;
  EfiTime->Minute += (Str[Index++] - '0');
  if (EfiTime->Minute > 59) {
    return FALSE;
  }

  EfiTime->Second  = (Str[Index++] - '0') * 10;
  EfiTime->Second += (Str[Index++] - '0');
  if (EfiTime->Second > 59) {
    return FALSE;
  }

  /* Note: we did not adjust the time based on time zone information */

  return TRUE;
}

/**

  Check the validity of TimeStamp Token Information.

  @param[in]  TstInfo          Pointer to the TS_TST_INFO structure.
  @param[in]  TimestampedData  Pointer to the data to be time-stamped.
  @param[in]  DataSize         Size of timestamped data in bytes.

  @retval  TRUE   The TimeStamp Token Information is valid.
  @retval  FALSE  Invalid TimeStamp Token Information.

**/
BOOLEAN
EFIAPI
CheckTSTInfo (
  IN  CONST TS_TST_INFO  *TstInfo,
  IN  CONST UINT8        *TimestampedData,
  IN  UINTN              DataSize
  )
{
  BOOLEAN             Status;
  TS_MESSAGE_IMPRINT  *Imprint;
  X509_ALGOR          *HashAlgo;
  CONST EVP_MD        *Md;
  EVP_MD_CTX          MdCtx;
  UINTN               MdSize;
  UINT8               *HashedMsg;

  //
  // Initialization
  //
  Status    = FALSE;
  HashAlgo  = NULL;
  HashedMsg = NULL;

  //
  // -- Check version number of Timestamp:
  //   The version field (currently v1) describes the version of the time-stamp token.
  //   Conforming time-stamping servers MUST be able to provide version 1 time-stamp tokens.
  //
  if ((ASN1_INTEGER_get (TstInfo->Version)) != 1) {
    return FALSE;
  }

  //
  // -- Check Policies
  //   The policy field MUST indicate the TSA's policy under which the response was produced.
  //
  if (TstInfo->Policy == NULL) {
    /// NOTE: Need to check if the requested and returned policies.
    ///       We have no information about the Requested TSA Policy.
    return FALSE;
  }

  //
  // -- Compute & Check Message Imprint
  //
  Imprint  = TstInfo->MessageImprint;
  HashAlgo = X509_ALGOR_dup (Imprint->HashAlgorithm);

  Md = EVP_get_digestbyobj (HashAlgo->algorithm);
  if (Md == NULL) {
    goto _Exit;
  }

  MdSize = EVP_MD_size (Md);
  HashedMsg = AllocateZeroPool (MdSize);
  if (HashedMsg == NULL) {
    goto _Exit;
  }
  EVP_DigestInit (&MdCtx, Md);
  EVP_DigestUpdate (&MdCtx, TimestampedData, DataSize);
  EVP_DigestFinal (&MdCtx, HashedMsg, NULL);
  if ((MdSize == (UINTN)ASN1_STRING_length (Imprint->HashedMessage)) &&
      (CompareMem (HashedMsg, ASN1_STRING_data (Imprint->HashedMessage), MdSize) != 0)) {
    goto _Exit;
  }

  //
  // -- Check Nonces
  //
  if (TstInfo->Nonce != NULL) {
    //
    // Nonces is optional, No error if no nonce is returned;
    //
  }

  //
  // -- Check if the TSA name and signer certificate is matched.
  //
  if (TstInfo->Tsa != NULL) {
    //
    //  Ignored the optional Tsa field checking.
    //
  }

  Status = TRUE;

_Exit:
  X509_ALGOR_free (HashAlgo);
  if (HashedMsg != NULL) {
    FreePool (HashedMsg);
  }

  return Status;
}

/**
  Verifies the validility of a TimeStamp Token as described in RFC 3161 ("Internet
  X.509 Public Key Infrastructure Time-Stamp Protocol (TSP)").

  If TSToken is NULL, then return FALSE.
  If TimestampedData is NULL, then return FALSE.

  @param[in]  TSToken          Pointer to the RFC3161 TimeStamp Token, which is generated
                               by a TSA and located in the software publisher's SignerInfo
                               structure.
  @param[in]  TokenSize        Size of the TimeStamp Token in bytes.
  @param[in]  TsaCert          Pointer to a trusted/root TSA certificate encoded in DER.
  @param[in]  CertSize         Size of the trusted TSA certificate in bytes.
  @param[in]  TimestampedData  Pointer to the data to be time-stamped.
  @param[in]  DataSize         Size of timestamped data in bytes.
  @param[out] SigningTime      Return the time of timestamp generation time if the timestamp
                               signature is valid.

  @retval  TRUE   The specified timestamp token is valid.
  @retval  FALSE  Invalid timestamp token.

**/
BOOLEAN
EFIAPI
TimestampTokenVerify (
  IN  CONST UINT8  *TSToken,
  IN  UINTN        TokenSize,
  IN  CONST UINT8  *TsaCert,
  IN  UINTN        CertSize,
  IN  CONST UINT8  *TimestampedData,
  IN  UINTN        DataSize,
  OUT EFI_TIME     *SigningTime
  )
{
  BOOLEAN      Status;
  CONST UINT8  *TokenTemp;
  PKCS7        *Pkcs7;
  X509         *Cert;
  CONST UINT8  *CertTemp;
  X509_STORE   *CertStore;
  BIO          *OutBio;
  UINT8        *TstData;
  UINTN        TstSize;
  CONST UINT8  *TstTemp;
  TS_TST_INFO  *TstInfo;

  Status = FALSE;

  //
  // Check input parameters
  //
  if ((TSToken == NULL) || (TsaCert == NULL) || (TimestampedData == NULL) ||
      (TokenSize > INT_MAX) || (CertSize > INT_MAX) || (DataSize > INT_MAX)) {
    return FALSE;
  }

  //
  // Initializations
  //
  if (SigningTime != NULL) {
    SetMem (SigningTime, sizeof (EFI_TIME), 0);
  }
  Pkcs7     = NULL;
  Cert      = NULL;
  CertStore = NULL;
  OutBio    = NULL;
  TstData   = NULL;
  TstInfo   = NULL;

  //
  // TimeStamp Token should contain one valid DER-encoded ASN.1 PKCS#7 structure.
  //
  TokenTemp = TSToken;
  Pkcs7     = d2i_PKCS7 (NULL, (const unsigned char **) &TokenTemp, (int) TokenSize);
  if (Pkcs7 == NULL) {
    goto _Exit;
  }

  //
  // The timestamp signature (TSA's response) will be one PKCS#7 signed data.
  //
  if (!PKCS7_type_is_signed (Pkcs7)) {
    goto _Exit;
  }

  //
  // Read the trusted TSA certificate (DER-encoded), and Construct X509 Certificate.
  //
  CertTemp = TsaCert;
  Cert = d2i_X509 (NULL, &CertTemp, (long) CertSize);
  if (Cert == NULL) {
    goto _Exit;
  }

  //
  // Setup X509 Store for trusted certificate.
  //
  CertStore = X509_STORE_new ();
  if ((CertStore == NULL) || !(X509_STORE_add_cert (CertStore, Cert))) {
    goto _Exit;
  }

  //
  // Register customized X509 verification callback function to support
  // trusted intermediate TSA certificate anchor.
  //
  CertStore->verify_cb = TSVerifyCallback;

  X509_STORE_set_purpose (CertStore, X509_PURPOSE_ANY);

  //
  // Verifies the PKCS#7 signedData structure, and output the signed contents.
  //
  OutBio = BIO_new (BIO_s_mem ());
  if (OutBio == NULL) {
    goto _Exit;
  }
  if (!PKCS7_verify (Pkcs7, NULL, CertStore, NULL, OutBio, PKCS7_BINARY)) {
    goto _Exit;
  }

  //
  // Read the signed contents detached in timestamp signature.
  //
  TstData = AllocateZeroPool (2048);
  if (TstData == NULL) {
    goto _Exit;
  }
  TstSize = BIO_read (OutBio, (void *) TstData, 2048);

  //
  // Construct TS_TST_INFO structure from the signed contents.
  //
  TstTemp = TstData;
  TstInfo = d2i_TS_TST_INFO (NULL, (const unsigned char **) &TstTemp,
              (int)TstSize);
  if (TstInfo == NULL) {
    goto _Exit;
  }

  //
  // Check TS_TST_INFO structure.
  //
  Status = CheckTSTInfo (TstInfo, TimestampedData, DataSize);
  if (!Status) {
    goto _Exit;
  }

  //
  // Retrieve the signing time from TS_TST_INFO structure.
  //
  if (SigningTime != NULL) {
    SetMem (SigningTime, sizeof (EFI_TIME), 0);
    Status = ConvertAsn1TimeToEfiTime (TstInfo->GenTime, SigningTime);
  }

_Exit:
  //
  // Release Resources
  //
  PKCS7_free (Pkcs7);
  X509_free (Cert);
  X509_STORE_free (CertStore);
  BIO_free (OutBio);
  TS_TST_INFO_free (TstInfo);

  if (TstData != NULL) {
    FreePool (TstData);
  }

  return Status;
}

/**
  Verifies the validility of a RFC3161 Timestamp CounterSignature embedded in PE/COFF Authenticode
  signature.

  If AuthData is NULL, then return FALSE.

  @param[in]  AuthData     Pointer to the Authenticode Signature retrieved from signed
                           PE/COFF image to be verified.
  @param[in]  DataSize     Size of the Authenticode Signature in bytes.
  @param[in]  TsaCert      Pointer to a trusted/root TSA certificate encoded in DER, which
                           is used for TSA certificate chain verification.
  @param[in]  CertSize     Size of the trusted certificate in bytes.
  @param[out] SigningTime  Return the time of timestamp generation time if the timestamp
                           signature is valid.

  @retval  TRUE   The specified Authenticode includes a valid RFC3161 Timestamp CounterSignature.
  @retval  FALSE  No valid RFC3161 Timestamp CounterSignature in the specified Authenticode data.

**/
BOOLEAN
EFIAPI
ImageTimestampVerify (
  IN  CONST UINT8  *AuthData,
  IN  UINTN        DataSize,
  IN  CONST UINT8  *TsaCert,
  IN  UINTN        CertSize,
  OUT EFI_TIME     *SigningTime
  )
{
  BOOLEAN                      Status;
  PKCS7                        *Pkcs7;
  CONST UINT8                  *Temp;
  STACK_OF(PKCS7_SIGNER_INFO)  *SignerInfos;
  PKCS7_SIGNER_INFO            *SignInfo;
  UINTN                        Index;
  STACK_OF(X509_ATTRIBUTE)     *Sk;
  X509_ATTRIBUTE               *Xa;
  ASN1_OBJECT                  *XaObj;
  ASN1_TYPE                    *Asn1Type;
  ASN1_OCTET_STRING            *EncDigest;
  UINT8                        *TSToken;
  UINTN                        TokenSize;

  //
  // Input Parameters Checking.
  //
  if ((AuthData == NULL) || (TsaCert == NULL)) {
    return FALSE;
  }

  if ((DataSize > INT_MAX) || (CertSize > INT_MAX)) {
    return FALSE;
  }

  //
  // Register & Initialize necessary digest algorithms for PKCS#7 Handling.
  //
  if ((EVP_add_digest (EVP_md5 ()) == 0) || (EVP_add_digest (EVP_sha1 ()) == 0) ||
      (EVP_add_digest (EVP_sha256 ()) == 0) || (EVP_add_digest_alias (SN_sha1WithRSAEncryption, SN_sha1WithRSA)) == 0) {
    return FALSE;
  }

  //
  // Initialization.
  //
  Status    = FALSE;
  Pkcs7     = NULL;
  SignInfo  = NULL;

  //
  // Decode ASN.1-encoded Authenticode data into PKCS7 structure.
  //
  Temp  = AuthData;
  Pkcs7 = d2i_PKCS7 (NULL, (const unsigned char **) &Temp, (int) DataSize);
  if (Pkcs7 == NULL) {
    goto _Exit;
  }

  //
  // Check if there is one and only one signer.
  //
  SignerInfos = PKCS7_get_signer_info (Pkcs7);
  if (!SignerInfos || (sk_PKCS7_SIGNER_INFO_num (SignerInfos) != 1)) {
    goto _Exit;
  }

  //
  // Locate the TimeStamp CounterSignature.
  //
  SignInfo = sk_PKCS7_SIGNER_INFO_value (SignerInfos, 0);
  if (SignInfo == NULL) {
    goto _Exit;
  }

  //
  // Locate Message Digest which will be the data to be time-stamped.
  //
  EncDigest = SignInfo->enc_digest;
  if (EncDigest == NULL) {
    goto _Exit;
  }

  //
  // The RFC3161 timestamp counterSignature is contained in unauthenticatedAttributes field
  // of SignerInfo.
  //
  Sk = SignInfo->unauth_attr;
  if (Sk == NULL) {             // No timestamp counterSignature.
    goto _Exit;
  }

  Asn1Type = NULL;
  for (Index = 0; Index < (UINTN) sk_X509_ATTRIBUTE_num (Sk); Index++) {
    //
    // Search valid RFC3161 timestamp counterSignature based on OBJID.
    //
    Xa = sk_X509_ATTRIBUTE_value (Sk, (int)Index);
    if (Xa == NULL) {
      continue;
    }
    XaObj = X509_ATTRIBUTE_get0_object(Xa);
    if (XaObj == NULL) {
      continue;
    }
    if ((XaObj->length != sizeof (mSpcRFC3161OidValue)) ||
        (CompareMem (XaObj->data, mSpcRFC3161OidValue, sizeof (mSpcRFC3161OidValue)) != 0)) {
      continue;
    }
    Asn1Type = X509_ATTRIBUTE_get0_type(Xa, 0);
  }

  if (Asn1Type == NULL) {
    Status = FALSE;
    goto _Exit;
  }
  TSToken   = Asn1Type->value.octet_string->data;
  TokenSize = Asn1Type->value.octet_string->length;

  //
  // TimeStamp counterSignature (Token) verification.
  //
  Status = TimestampTokenVerify (
             TSToken,
             TokenSize,
             TsaCert,
             CertSize,
             EncDigest->data,
             EncDigest->length,
             SigningTime
             );

_Exit:
  //
  // Release Resources
  //
  PKCS7_free (Pkcs7);

  return Status;
}
