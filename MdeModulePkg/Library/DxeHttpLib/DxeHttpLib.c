/** @file
  This library is used to share code between UEFI network stack modules.
  It provides the helper routines to parse the HTTP message byte stream.

Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at<BR>
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Uefi.h>
#include <Library/NetLib.h>
#include <Library/HttpLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>

#define BIT(x)  (1 << x)

#define NET_IS_HEX_CHAR(Ch)   \
  ((('0' <= (Ch)) && ((Ch) <= '9')) ||  \
   (('A' <= (Ch)) && ((Ch) <= 'F')) ||  \
   (('a' <= (Ch)) && ((Ch) <= 'f')))

//
// Field index of the HTTP URL parse result.
//
#define   HTTP_URI_FIELD_SCHEME           0
#define   HTTP_URI_FIELD_AUTHORITY        1
#define   HTTP_URI_FIELD_PATH             2
#define   HTTP_URI_FIELD_QUERY            3
#define   HTTP_URI_FIELD_FRAGMENT         4
#define   HTTP_URI_FIELD_USERINFO         5
#define   HTTP_URI_FIELD_HOST             6
#define   HTTP_URI_FIELD_PORT             7
#define   HTTP_URI_FIELD_MAX              8

//
// Structure to store the parse result of a HTTP URL.
//
typedef struct {
    UINT32      Offset;
    UINT32      Length;
} HTTP_URL_FILED_DATA;

typedef struct {
  UINT16                  FieldBitMap;
  HTTP_URL_FILED_DATA     FieldData[HTTP_URI_FIELD_MAX];
} HTTP_URL_PARSER;

typedef enum {
  UrlParserUrlStart,
  UrlParserScheme,
  UrlParserSchemeColon,            // ":"
  UrlParserSchemeColonSlash,       // ":/"
  UrlParserSchemeColonSlashSlash,  // "://"
  UrlParserAuthority,
  UrlParserAtInAuthority,
  UrlParserPath,
  UrlParserQueryStart,    // "?"
  UrlParserQuery,
  UrlParserFragmentStart, // "#"
  UrlParserFragment,
  UrlParserUserInfo,
  UrlParserHostStart,     // "@"
  UrlParserHost,
  UrlParserPortStart,     // ":"
  UrlParserPort,
  UrlParserStateMax
} HTTP_URL_PARSE_STATE;

/**
  Decode a percent-encoded URI component to the ASCII character.
  
  Decode the input component in Buffer according to RFC 3986. The caller is responsible to make 
  sure ResultBuffer points to a buffer with size equal or greater than ((AsciiStrSize (Buffer))
  in bytes. 

  @param[in]    Buffer           The pointer to a percent-encoded URI component.
  @param[in]    BufferLength     Length of Buffer in bytes.
  @param[out]   ResultBuffer     Point to the buffer to store the decode result.
  @param[out]   ResultLength     Length of decoded string in ResultBuffer in bytes.

  @retval EFI_SUCCESS            Successfully decoded the URI.
  @retval EFI_INVALID_PARAMETER  Buffer is not a valid percent-encoded string.
  
**/
EFI_STATUS
EFIAPI
UriPercentDecode (
  IN      CHAR8            *Buffer,
  IN      UINT32            BufferLength,
     OUT  CHAR8            *ResultBuffer,
     OUT  UINT32           *ResultLength
  )
{
  UINTN           Index;
  UINTN           Offset;
  CHAR8           HexStr[3];

  if (Buffer == NULL || BufferLength == 0 || ResultBuffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  
  Index = 0;
  Offset = 0;
  HexStr[2] = '\0';
  while (Index < BufferLength) {
    if (Buffer[Index] == '%') {
      if (!NET_IS_HEX_CHAR (Buffer[Index+1]) || !NET_IS_HEX_CHAR (Buffer[Index+2])) {
        return EFI_INVALID_PARAMETER;
      }
      HexStr[0] = Buffer[Index+1];
      HexStr[1] = Buffer[Index+2];
      ResultBuffer[Offset] = (CHAR8) AsciiStrHexToUintn (HexStr);
      Index += 3;
    } else {
      ResultBuffer[Offset] = Buffer[Index];
      Index++;
    }
    Offset++;
  }

  *ResultLength = (UINT32) Offset;
    
  return EFI_SUCCESS;
}

/**
  This function return the updated state accroding to the input state and next character of
  the authority.

  @param[in]       Char           Next character.
  @param[in]       State          Current value of the parser state machine.

  @return          Updated state value.
**/
HTTP_URL_PARSE_STATE
NetHttpParseAuthorityChar (
  IN  CHAR8                  Char,
  IN  HTTP_URL_PARSE_STATE   State
  )
{

  //
  // RFC 3986:
  // The authority component is preceded by a double slash ("//") and is
  // terminated by the next slash ("/"), question mark ("?"), or number
  // sign ("#") character, or by the end of the URI.
  //
  if (Char == ' ' || Char == '\r' || Char == '\n') {
    return UrlParserStateMax;
  }

  //
  // authority   = [ userinfo "@" ] host [ ":" port ]
  //
  switch (State) {
  case UrlParserUserInfo:
    if (Char == '@') {
      return UrlParserHostStart;
    }
    break;

  case UrlParserHost:
  case UrlParserHostStart:
    if (Char == ':') {
      return UrlParserPortStart;
    }
    return UrlParserHost;

  case UrlParserPort:
  case UrlParserPortStart:
    return UrlParserPort;

  default:
    break;
  }

  return State;
}

/**
  This function parse the authority component of the input URL and update the parser.

  @param[in]       Url            The pointer to a HTTP URL string.
  @param[in]       FoundAt        TRUE if there is an at sign ('@') in the authority component.
  @param[in, out]  UrlParser      Pointer to the buffer of the parse result.

  @retval EFI_SUCCESS             Successfully parse the authority.
  @retval Other                   Error happened.

**/
EFI_STATUS
NetHttpParseAuthority (
  IN      CHAR8              *Url,
  IN      BOOLEAN            FoundAt,
  IN OUT  HTTP_URL_PARSER    *UrlParser
  )
{
  CHAR8                 *Char;
  CHAR8                 *Authority;
  UINT32                Length;
  HTTP_URL_PARSE_STATE  State;
  UINT32                Field;
  UINT32                OldField;
  
  ASSERT ((UrlParser->FieldBitMap & BIT (HTTP_URI_FIELD_AUTHORITY)) != 0);

  //
  // authority   = [ userinfo "@" ] host [ ":" port ]
  //
  if (FoundAt) {
    State = UrlParserUserInfo;
  } else {
    State = UrlParserHost;
  }

  Field = HTTP_URI_FIELD_MAX;
  OldField = Field;
  Authority = Url + UrlParser->FieldData[HTTP_URI_FIELD_AUTHORITY].Offset;
  Length = UrlParser->FieldData[HTTP_URI_FIELD_AUTHORITY].Length;
  for (Char = Authority; Char < Authority + Length; Char++) {
    State = NetHttpParseAuthorityChar (*Char, State);
    switch (State) {
    case UrlParserStateMax:
      return EFI_INVALID_PARAMETER;

    case UrlParserHostStart:
    case UrlParserPortStart:
      continue;

    case UrlParserUserInfo:
      Field = HTTP_URI_FIELD_USERINFO;
      break;
      
    case UrlParserHost:
      Field = HTTP_URI_FIELD_HOST;
      break;
      
    case UrlParserPort:
      Field = HTTP_URI_FIELD_PORT;
      break;

    default:
      ASSERT (FALSE);
    }

    //
    // Field not changed, count the length.
    //
    ASSERT (Field < HTTP_URI_FIELD_MAX);
    if (Field == OldField) {
      UrlParser->FieldData[Field].Length++;
      continue;
    }

    //
    // New field start
    //
    UrlParser->FieldBitMap |= BIT (Field);
    UrlParser->FieldData[Field].Offset = (UINT32) (Char - Url);
    UrlParser->FieldData[Field].Length = 1;
    OldField = Field;
  }

  return EFI_SUCCESS;
}

/**
  This function return the updated state accroding to the input state and next character of a URL.

  @param[in]       Char           Next character.
  @param[in]       State          Current value of the parser state machine.

  @return          Updated state value.

**/
HTTP_URL_PARSE_STATE
NetHttpParseUrlChar (
  IN  CHAR8                  Char,
  IN  HTTP_URL_PARSE_STATE   State
  )
{
  if (Char == ' ' || Char == '\r' || Char == '\n') {
    return UrlParserStateMax;
  }
  
  //
  // http_URL = "http:" "//" host [ ":" port ] [ abs_path [ "?" query ]]
  // 
  // Request-URI    = "*" | absolute-URI | path-absolute | authority
  // 
  // absolute-URI  = scheme ":" hier-part [ "?" query ]
  // path-absolute = "/" [ segment-nz *( "/" segment ) ]
  // authority   = [ userinfo "@" ] host [ ":" port ]
  //
  switch (State) {
  case UrlParserUrlStart:
    if (Char == '*' || Char == '/') {
      return UrlParserPath;
    }
    return UrlParserScheme;

  case UrlParserScheme:
    if (Char == ':') {
      return UrlParserSchemeColon;
    }
    break;

  case UrlParserSchemeColon:
    if (Char == '/') {
      return UrlParserSchemeColonSlash;
    }
    break;

  case UrlParserSchemeColonSlash:
    if (Char == '/') {
      return UrlParserSchemeColonSlashSlash;
    }
    break;

  case UrlParserAtInAuthority:
    if (Char == '@') {
      return UrlParserStateMax;
    }

  case UrlParserAuthority:
  case UrlParserSchemeColonSlashSlash:
    if (Char == '@') {
      return UrlParserAtInAuthority;
    }
    if (Char == '/') {
      return UrlParserPath;
    }
    if (Char == '?') {
      return UrlParserQueryStart;
    }
    if (Char == '#') {
      return UrlParserFragmentStart;
    }
    return UrlParserAuthority;

  case UrlParserPath:
    if (Char == '?') {
      return UrlParserQueryStart;
    }
    if (Char == '#') {
      return UrlParserFragmentStart;
    }
    break;

  case UrlParserQuery:
  case UrlParserQueryStart:
    if (Char == '#') {
      return UrlParserFragmentStart;
    }
    return UrlParserQuery;

  case UrlParserFragmentStart:
    return UrlParserFragment;
    
  default:
    break;
  }

  return State;
}
/**
  Create a URL parser for the input URL string.

  This function will parse and dereference the input HTTP URL into it components. The original
  content of the URL won't be modified and the result will be returned in UrlParser, which can
  be used in other functions like NetHttpUrlGetHostName().

  @param[in]    Url                The pointer to a HTTP URL string.
  @param[in]    Length             Length of Url in bytes.
  @param[in]    IsConnectMethod    Whether the Url is used in HTTP CONNECT method or not.
  @param[out]   UrlParser          Pointer to the returned buffer to store the parse result.

  @retval EFI_SUCCESS              Successfully dereferenced the HTTP URL.
  @retval EFI_INVALID_PARAMETER    UrlParser is NULL or Url is not a valid HTTP URL.
  @retval EFI_OUT_OF_RESOURCES     Could not allocate needed resources.

**/
EFI_STATUS
EFIAPI
HttpParseUrl (
  IN      CHAR8              *Url,
  IN      UINT32             Length,
  IN      BOOLEAN            IsConnectMethod,
     OUT  VOID               **UrlParser
  )
{
  HTTP_URL_PARSE_STATE  State;
  CHAR8                 *Char;
  UINT32                Field;
  UINT32                OldField;
  BOOLEAN               FoundAt;
  EFI_STATUS            Status;
  HTTP_URL_PARSER       *Parser;
  
  if (Url == NULL || Length == 0 || UrlParser == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Parser = AllocateZeroPool (sizeof (HTTP_URL_PARSER));
  if (Parser == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  
  if (IsConnectMethod) {
    //
    // According to RFC 2616, the authority form is only used by the CONNECT method.
    //
    State = UrlParserAuthority;
  } else {
    State = UrlParserUrlStart;
  }

  Field = HTTP_URI_FIELD_MAX;
  OldField = Field;
  FoundAt = FALSE;
  for (Char = Url; Char < Url + Length; Char++) {
    //
    // Update state machine accoring to next char.
    //
    State = NetHttpParseUrlChar (*Char, State);

    switch (State) {
    case UrlParserStateMax:
      return EFI_INVALID_PARAMETER;
      
    case UrlParserSchemeColon:
    case UrlParserSchemeColonSlash:
    case UrlParserSchemeColonSlashSlash:
    case UrlParserQueryStart:
    case UrlParserFragmentStart:
      //
      // Skip all the delimiting char: "://" "?" "@"
      //
      continue;
    
    case UrlParserScheme:
      Field = HTTP_URI_FIELD_SCHEME;
      break;

    case UrlParserAtInAuthority:
      FoundAt = TRUE;
    case UrlParserAuthority:
      Field = HTTP_URI_FIELD_AUTHORITY;
      break;

    case UrlParserPath:
      Field = HTTP_URI_FIELD_PATH;
      break;

    case UrlParserQuery:
      Field = HTTP_URI_FIELD_QUERY;
      break;

    case UrlParserFragment:
      Field = HTTP_URI_FIELD_FRAGMENT;
      break;

    default:
      ASSERT (FALSE);
    }

    //
    // Field not changed, count the length.
    //
    ASSERT (Field < HTTP_URI_FIELD_MAX);
    if (Field == OldField) {
      Parser->FieldData[Field].Length++;
      continue;
    }

    //
    // New field start
    //
    Parser->FieldBitMap |= BIT (Field);
    Parser->FieldData[Field].Offset = (UINT32) (Char - Url);
    Parser->FieldData[Field].Length = 1;
    OldField = Field;
  }

  //
  // If has authority component, continue to parse the username, host and port.
  //
  if ((Parser->FieldBitMap & BIT (HTTP_URI_FIELD_AUTHORITY)) != 0) {
    Status = NetHttpParseAuthority (Url, FoundAt, Parser);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  *UrlParser = Parser;
  return EFI_SUCCESS;  
}

/**
  Get the Hostname from a HTTP URL.

  This function will return the HostName according to the Url and previous parse result ,and
  it is the caller's responsibility to free the buffer returned in *HostName.

  @param[in]    Url                The pointer to a HTTP URL string.
  @param[in]    UrlParser          URL Parse result returned by NetHttpParseUrl().
  @param[out]   HostName           Pointer to a buffer to store the HostName.

  @retval EFI_SUCCESS              Successfully get the required component.
  @retval EFI_INVALID_PARAMETER    Uri is NULL or HostName is NULL or UrlParser is invalid.
  @retval EFI_NOT_FOUND            No hostName component in the URL.
  @retval EFI_OUT_OF_RESOURCES     Could not allocate needed resources.
  
**/
EFI_STATUS
EFIAPI
HttpUrlGetHostName (
  IN      CHAR8              *Url,
  IN      VOID               *UrlParser,
     OUT  CHAR8              **HostName
  )
{
  CHAR8                *Name;
  EFI_STATUS           Status;
  UINT32               ResultLength;
  HTTP_URL_PARSER      *Parser;

  if (Url == NULL || UrlParser == NULL || HostName == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Parser = (HTTP_URL_PARSER*) UrlParser;

  if ((Parser->FieldBitMap & BIT (HTTP_URI_FIELD_HOST)) == 0) {
    return EFI_NOT_FOUND;
  }

  Name = AllocatePool (Parser->FieldData[HTTP_URI_FIELD_HOST].Length + 1);
  if (Name == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  
  Status = UriPercentDecode (
             Url + Parser->FieldData[HTTP_URI_FIELD_HOST].Offset,
             Parser->FieldData[HTTP_URI_FIELD_HOST].Length,
             Name,
             &ResultLength
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Name[ResultLength] = '\0';
  *HostName = Name;
  return EFI_SUCCESS;
}


/**
  Get the IPv4 address from a HTTP URL.

  This function will return the IPv4 address according to the Url and previous parse result.

  @param[in]    Url                The pointer to a HTTP URL string.
  @param[in]    UrlParser          URL Parse result returned by NetHttpParseUrl().
  @param[out]   Ip4Address         Pointer to a buffer to store the IP address.

  @retval EFI_SUCCESS              Successfully get the required component.
  @retval EFI_INVALID_PARAMETER    Uri is NULL or Ip4Address is NULL or UrlParser is invalid.
  @retval EFI_NOT_FOUND            No IPv4 address component in the URL.
  @retval EFI_OUT_OF_RESOURCES     Could not allocate needed resources.
  
**/
EFI_STATUS
EFIAPI
HttpUrlGetIp4 (
  IN      CHAR8              *Url,
  IN      VOID               *UrlParser,
     OUT  EFI_IPv4_ADDRESS   *Ip4Address
  )
{
  CHAR8                *Ip4String;
  EFI_STATUS           Status;
  UINT32               ResultLength;
  HTTP_URL_PARSER      *Parser;
  
  if (Url == NULL || UrlParser == NULL || Ip4Address == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Parser = (HTTP_URL_PARSER*) UrlParser;

  if ((Parser->FieldBitMap & BIT (HTTP_URI_FIELD_HOST)) == 0) {
    return EFI_INVALID_PARAMETER;
  }

  Ip4String = AllocatePool (Parser->FieldData[HTTP_URI_FIELD_HOST].Length + 1);
  if (Ip4String == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  
  Status = UriPercentDecode (
             Url + Parser->FieldData[HTTP_URI_FIELD_HOST].Offset,
             Parser->FieldData[HTTP_URI_FIELD_HOST].Length,
             Ip4String,
             &ResultLength
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Ip4String[ResultLength] = '\0';
  Status = NetLibAsciiStrToIp4 (Ip4String, Ip4Address);
  FreePool (Ip4String);

  return Status;
}

/**
  Get the IPv6 address from a HTTP URL.

  This function will return the IPv6 address according to the Url and previous parse result.

  @param[in]    Url                The pointer to a HTTP URL string.
  @param[in]    UrlParser          URL Parse result returned by NetHttpParseUrl().
  @param[out]   Ip6Address         Pointer to a buffer to store the IP address.

  @retval EFI_SUCCESS              Successfully get the required component.
  @retval EFI_INVALID_PARAMETER    Uri is NULL or Ip6Address is NULL or UrlParser is invalid.
  @retval EFI_NOT_FOUND            No IPv6 address component in the URL.
  @retval EFI_OUT_OF_RESOURCES     Could not allocate needed resources.
  
**/
EFI_STATUS
EFIAPI
HttpUrlGetIp6 (
  IN      CHAR8              *Url,
  IN      VOID               *UrlParser,
     OUT  EFI_IPv6_ADDRESS   *Ip6Address
  )
{
  CHAR8                *Ip6String;
  CHAR8                *Ptr;
  UINT32               Length;
  EFI_STATUS           Status;
  UINT32               ResultLength;
  HTTP_URL_PARSER      *Parser;
  
  if (Url == NULL || UrlParser == NULL || Ip6Address == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Parser = (HTTP_URL_PARSER*) UrlParser;

  if ((Parser->FieldBitMap & BIT (HTTP_URI_FIELD_HOST)) == 0) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // IP-literal = "[" ( IPv6address / IPvFuture  ) "]"
  //
  Length = Parser->FieldData[HTTP_URI_FIELD_HOST].Length;
  if (Length < 2) {
    return EFI_INVALID_PARAMETER;
  }

  Ptr    = Url + Parser->FieldData[HTTP_URI_FIELD_HOST].Offset;
  if ((Ptr[0] != '[') || (Ptr[Length - 1] != ']')) {
    return EFI_INVALID_PARAMETER;
  }

  Ip6String = AllocatePool (Length);
  if (Ip6String == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  
  Status = UriPercentDecode (
             Ptr + 1,
             Length - 2,
             Ip6String,
             &ResultLength
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }
  
  Ip6String[ResultLength] = '\0';
  Status = NetLibAsciiStrToIp6 (Ip6String, Ip6Address);
  FreePool (Ip6String);

  return Status;
}

/**
  Get the port number from a HTTP URL.

  This function will return the port number according to the Url and previous parse result.

  @param[in]    Url                The pointer to a HTTP URL string.
  @param[in]    UrlParser          URL Parse result returned by NetHttpParseUrl().
  @param[out]   Port               Pointer to a buffer to store the port number.

  @retval EFI_SUCCESS              Successfully get the required component.
  @retval EFI_INVALID_PARAMETER    Uri is NULL or Port is NULL or UrlParser is invalid.
  @retval EFI_NOT_FOUND            No port number in the URL.
  @retval EFI_OUT_OF_RESOURCES     Could not allocate needed resources.
  
**/
EFI_STATUS
EFIAPI
HttpUrlGetPort (
  IN      CHAR8              *Url,
  IN      VOID               *UrlParser,
     OUT  UINT16             *Port
  )
{
  CHAR8         *PortString;
  EFI_STATUS    Status;
  UINT32        ResultLength;
  HTTP_URL_PARSER      *Parser;

  if (Url == NULL || UrlParser == NULL || Port == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Parser = (HTTP_URL_PARSER*) UrlParser;

  if ((Parser->FieldBitMap & BIT (HTTP_URI_FIELD_PORT)) == 0) {
    return EFI_INVALID_PARAMETER;
  }

  PortString = AllocatePool (Parser->FieldData[HTTP_URI_FIELD_PORT].Length + 1);
  if (PortString == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = UriPercentDecode (
             Url + Parser->FieldData[HTTP_URI_FIELD_PORT].Offset,
             Parser->FieldData[HTTP_URI_FIELD_PORT].Length,
             PortString,
             &ResultLength
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  PortString[ResultLength] = '\0';
  *Port = (UINT16) AsciiStrDecimalToUintn (Url + Parser->FieldData[HTTP_URI_FIELD_PORT].Offset);

  return  EFI_SUCCESS;
}

/**
  Release the resource of the URL parser.

  @param[in]    UrlParser            Pointer to the parser.
  
**/
VOID
EFIAPI
HttpUrlFreeParser (
  IN      VOID               *UrlParser
  )
{
  FreePool (UrlParser);
}

/**
  Find a specified header field according to the field name.

  @param[in]   HeaderCount      Number of HTTP header structures in Headers list. 
  @param[in]   Headers          Array containing list of HTTP headers.
  @param[in]   FieldName        Null terminated string which describes a field name. 

  @return    Pointer to the found header or NULL.

**/
EFI_HTTP_HEADER *
HttpIoFindHeader (
  IN  UINTN                HeaderCount,
  IN  EFI_HTTP_HEADER      *Headers,
  IN  CHAR8                *FieldName
  )
{
  UINTN                 Index;
  
  if (HeaderCount == 0 || Headers == NULL || FieldName == NULL) {
    return NULL;
  }

  for (Index = 0; Index < HeaderCount; Index++){
    //
    // Field names are case-insensitive (RFC 2616).
    //
    if (AsciiStriCmp (Headers[Index].FieldName, FieldName) == 0) {
      return &Headers[Index];
    }
  }
  return NULL;
}

typedef enum {
  BodyParserBodyStart,
  BodyParserBodyIdentity,
  BodyParserChunkSizeStart,
  BodyParserChunkSize,
  BodyParserChunkSizeEndCR,
  BodyParserChunkExtStart,
  BodyParserChunkDataStart,
  BodyParserChunkDataEnd,
  BodyParserChunkDataEndCR,
  BodyParserComplete,
  BodyParserStateMax
} HTTP_BODY_PARSE_STATE;

typedef struct {
  BOOLEAN                       IgnoreBody;    // "MUST NOT" include a message-body
  BOOLEAN                       IsChunked;     // "chunked" transfer-coding.
  BOOLEAN                       ContentLengthIsValid;
  UINTN                         ContentLength; // Entity length (not the message-body length), invalid until ContentLengthIsValid is TRUE
  
  HTTP_BODY_PARSER_CALLBACK     Callback;
  VOID                          *Context;
  UINTN                         ParsedBodyLength;
  HTTP_BODY_PARSE_STATE         State;
  UINTN                         CurrentChunkSize;
  UINTN                         CurrentChunkParsedSize;
} HTTP_BODY_PARSER;

/**

  Convert an Ascii char to its uppercase.

  @param[in]       Char           Ascii character.

  @return          Uppercase value of the input Char.

**/
CHAR8
HttpIoCharToUpper (
  IN      CHAR8                    Char
  )
{
  if (Char >= 'a' && Char <= 'z') {
    return  Char - ('a' - 'A');
  }

  return Char;
}

/**
  Convert an hexadecimal char to a value of type UINTN.

  @param[in]       Char           Ascii character.

  @return          Value translated from Char.

**/
UINTN
HttpIoHexCharToUintn (
  IN CHAR8           Char
  )
{
  if (Char >= '0' && Char <= '9') {
    return Char - '0';
  }

  return (10 + HttpIoCharToUpper (Char) - 'A');
}

/**
  Get the value of the content length if there is a "Content-Length" header.

  @param[in]    HeaderCount        Number of HTTP header structures in Headers.
  @param[in]    Headers            Array containing list of HTTP headers.
  @param[out]   ContentLength      Pointer to save the value of the content length.

  @retval EFI_SUCCESS              Successfully get the content length.
  @retval EFI_NOT_FOUND            No "Content-Length" header in the Headers.

**/
EFI_STATUS
HttpIoParseContentLengthHeader (
  IN     UINTN                HeaderCount,
  IN     EFI_HTTP_HEADER      *Headers,
     OUT UINTN                *ContentLength
  )
{
  EFI_HTTP_HEADER       *Header;
  
  Header = HttpIoFindHeader (HeaderCount, Headers, "Content-Length");
  if (Header == NULL) {
    return EFI_NOT_FOUND;
  }

  *ContentLength = AsciiStrDecimalToUintn (Header->FieldValue);
  return EFI_SUCCESS;
}

/**

  Check whether the HTTP message is using the "chunked" transfer-coding.

  @param[in]    HeaderCount        Number of HTTP header structures in Headers.
  @param[in]    Headers            Array containing list of HTTP headers.

  @return       The message is "chunked" transfer-coding (TRUE) or not (FALSE).
 
**/
BOOLEAN
HttpIoIsChunked (
  IN   UINTN                    HeaderCount,
  IN   EFI_HTTP_HEADER          *Headers
  )
{
  EFI_HTTP_HEADER       *Header;


  Header = HttpIoFindHeader (HeaderCount, Headers, "Transfer-Encoding");
  if (Header == NULL) {
    return FALSE;
  }

  if (AsciiStriCmp (Header->FieldValue, "identity") != 0) {
    return TRUE;
  }

  return FALSE;
}

/**
  Check whether the HTTP message should have a message-body.

  @param[in]    Method             The HTTP method (e.g. GET, POST) for this HTTP message.
  @param[in]    StatusCode         Response status code returned by the remote host.

  @return       The message should have a message-body (FALSE) or not (TRUE).

**/
BOOLEAN
HttpIoNoMessageBody (
  IN   EFI_HTTP_METHOD          Method,
  IN   EFI_HTTP_STATUS_CODE     StatusCode
  )
{
  //
  // RFC 2616:
  // All responses to the HEAD request method
  // MUST NOT include a message-body, even though the presence of entity-
  // header fields might lead one to believe they do. All 1xx
  // (informational), 204 (no content), and 304 (not modified) responses
  // MUST NOT include a message-body. All other responses do include a
  // message-body, although it MAY be of zero length.
  //
  if (Method == HttpMethodHead) {
    return TRUE;
  }

  if ((StatusCode == HTTP_STATUS_100_CONTINUE) ||
      (StatusCode == HTTP_STATUS_101_SWITCHING_PROTOCOLS) ||
      (StatusCode == HTTP_STATUS_204_NO_CONTENT) ||
      (StatusCode == HTTP_STATUS_304_NOT_MODIFIED))
  {
    return TRUE;
  }

  return FALSE;
}

/**
  Initialize a HTTP message-body parser.

  This function will create and initialize a HTTP message parser according to caller provided HTTP message
  header information. It is the caller's responsibility to free the buffer returned in *UrlParser by HttpFreeMsgParser().

  @param[in]    Method             The HTTP method (e.g. GET, POST) for this HTTP message.
  @param[in]    StatusCode         Response status code returned by the remote host.
  @param[in]    HeaderCount        Number of HTTP header structures in Headers.
  @param[in]    Headers            Array containing list of HTTP headers.
  @param[in]    Callback           Callback function that is invoked when parsing the HTTP message-body,
                                   set to NULL to ignore all events.
  @param[in]    Context            Pointer to the context that will be passed to Callback.
  @param[out]   MsgParser          Pointer to the returned buffer to store the message parser.

  @retval EFI_SUCCESS              Successfully initialized the parser.
  @retval EFI_OUT_OF_RESOURCES     Could not allocate needed resources.
  @retval EFI_INVALID_PARAMETER    MsgParser is NULL or HeaderCount is not NULL but Headers is NULL.
  @retval Others                   Failed to initialize the parser.

**/
EFI_STATUS
EFIAPI
HttpInitMsgParser (
  IN     EFI_HTTP_METHOD               Method,
  IN     EFI_HTTP_STATUS_CODE          StatusCode,
  IN     UINTN                         HeaderCount,
  IN     EFI_HTTP_HEADER               *Headers,
  IN     HTTP_BODY_PARSER_CALLBACK     Callback,
  IN     VOID                          *Context,
    OUT  VOID                          **MsgParser
  )
{
  EFI_STATUS            Status;
  HTTP_BODY_PARSER      *Parser;
  
  if (HeaderCount != 0 && Headers == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (MsgParser == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Parser = AllocateZeroPool (sizeof (HTTP_BODY_PARSER));
  if (Parser == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Parser->State = BodyParserBodyStart;
  
  //
  // Determine the message length accroding to RFC 2616.
  // 1. Check whether the message "MUST NOT" have a message-body.
  //
  Parser->IgnoreBody = HttpIoNoMessageBody (Method, StatusCode);
  //
  // 2. Check whether the message using "chunked" transfer-coding.
  //
  Parser->IsChunked  = HttpIoIsChunked (HeaderCount, Headers);
  //
  // 3. Check whether the message has a Content-Length header field.
  //
  Status = HttpIoParseContentLengthHeader (HeaderCount, Headers, &Parser->ContentLength);
  if (!EFI_ERROR (Status)) {
    Parser->ContentLengthIsValid = TRUE;
  }
  //
  // 4. Range header is not supported now, so we won't meet media type "multipart/byteranges".
  // 5. By server closing the connection
  //
  
  //
  // Set state to skip body parser if the message shouldn't have a message body.
  //
  if (Parser->IgnoreBody) {
    Parser->State = BodyParserComplete;
  } else {
    Parser->Callback = Callback;
    Parser->Context  = Context;
  }

  *MsgParser = Parser;
  return EFI_SUCCESS;
}

/**
  Parse message body.

  Parse BodyLength of message-body. This function can be called repeatedly to parse the message-body partially.

  @param[in, out]    MsgParser            Pointer to the message parser.
  @param[in]         BodyLength           Length in bytes of the Body.
  @param[in]         Body                 Pointer to the buffer of the message-body to be parsed.

  @retval EFI_SUCCESS                Successfully parse the message-body.
  @retval EFI_INVALID_PARAMETER      MsgParser is NULL or Body is NULL or BodyLength is 0.
  @retval Others                     Operation aborted.

**/
EFI_STATUS
EFIAPI
HttpParseMessageBody (
  IN OUT VOID              *MsgParser,
  IN     UINTN             BodyLength,
  IN     CHAR8             *Body
  )
{
  CHAR8                 *Char;
  UINTN                 RemainderLengthInThis;
  UINTN                 LengthForCallback;
  EFI_STATUS            Status;
  HTTP_BODY_PARSER      *Parser;
  
  if (BodyLength == 0 || Body == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (MsgParser == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Parser = (HTTP_BODY_PARSER*) MsgParser;

  if (Parser->IgnoreBody) {
    Parser->State = BodyParserComplete;
    if (Parser->Callback != NULL) {
      Status = Parser->Callback (
                 BodyParseEventOnComplete,
                 Body,
                 0,
                 Parser->Context
                 );
      if (EFI_ERROR (Status)) {
        return Status;
      }
    }
    return EFI_SUCCESS;
  }

  if (Parser->State == BodyParserBodyStart) {
    Parser->ParsedBodyLength = 0;
    if (Parser->IsChunked) {
      Parser->State = BodyParserChunkSizeStart;
    } else {
      Parser->State = BodyParserBodyIdentity;
    }
  }

  //
  // The message body might be truncated in anywhere, so we need to parse is byte-by-byte.
  //
  for (Char = Body; Char < Body + BodyLength; ) {

    switch (Parser->State) {
    case BodyParserStateMax:
      return EFI_ABORTED;

    case BodyParserComplete:
      if (Parser->Callback != NULL) {
        Status = Parser->Callback (
                   BodyParseEventOnComplete,
                   Char,
                   0,
                   Parser->Context
                   );
        if (EFI_ERROR (Status)) {
          return Status;
        }
      }
      return EFI_SUCCESS;
    
    case BodyParserBodyIdentity:
      //
      // Identity transfer-coding, just notify user to save the body data.
      //
      if (Parser->Callback != NULL) {
        Status = Parser->Callback (
                   BodyParseEventOnData,
                   Char,
                   MIN (BodyLength, Parser->ContentLength - Parser->ParsedBodyLength),
                   Parser->Context
                   );
        if (EFI_ERROR (Status)) {
          return Status;
        }
      }
      Char += MIN (BodyLength, Parser->ContentLength - Parser->ParsedBodyLength);
      Parser->ParsedBodyLength += MIN (BodyLength, Parser->ContentLength - Parser->ParsedBodyLength);
      if (Parser->ParsedBodyLength == Parser->ContentLength) {
        Parser->State = BodyParserComplete;
      }
      break;

    case BodyParserChunkSizeStart:
      //
      // First byte of chunk-size, the chunk-size might be truncated.
      //
      Parser->CurrentChunkSize = 0;
      Parser->State = BodyParserChunkSize;
    case BodyParserChunkSize:
      if (!NET_IS_HEX_CHAR (*Char)) {
        if (*Char == ';') {
          Parser->State = BodyParserChunkExtStart;
          Char++;
        } else if (*Char == '\r') {
          Parser->State = BodyParserChunkSizeEndCR;
          Char++;
        } else {
          Parser->State = BodyParserStateMax;
        }
        break;
      }

      if (Parser->CurrentChunkSize > (((~((UINTN) 0)) - 16) / 16)) {
        return EFI_INVALID_PARAMETER;
      }
      Parser->CurrentChunkSize = Parser->CurrentChunkSize * 16 + HttpIoHexCharToUintn (*Char);
      Char++;
      break;

    case BodyParserChunkExtStart:
      //
      // Ignore all the chunk extensions.
      //
      if (*Char == '\r') {
        Parser->State = BodyParserChunkSizeEndCR;
       }
      Char++;
      break;
      
    case BodyParserChunkSizeEndCR:
      if (*Char != '\n') {
        Parser->State = BodyParserStateMax;
        break;
      }
      Parser->State = BodyParserChunkDataStart;
      Parser->CurrentChunkParsedSize = 0;
      Char++;
      break;

    case BodyParserChunkDataStart:
      if (Parser->CurrentChunkSize == 0) {
        //
        // This is the last chunk, the trailer header is unsupported.
        //
        Parser->ContentLengthIsValid = TRUE;
        Parser->State = BodyParserComplete;
        break;
      }
      
      //
      // First byte of chunk-data, the chunk data also might be truncated.
      //
      RemainderLengthInThis = BodyLength - (Char - Body);
      LengthForCallback = MIN (Parser->CurrentChunkSize - Parser->CurrentChunkParsedSize, RemainderLengthInThis);
      if (Parser->Callback != NULL) {
        Status = Parser->Callback (
                   BodyParseEventOnData,
                   Char,
                   LengthForCallback,
                   Parser->Context
                   );
        if (EFI_ERROR (Status)) {
          return Status;
        }
      }
      Char += LengthForCallback;
      Parser->ContentLength += LengthForCallback;
      Parser->CurrentChunkParsedSize += LengthForCallback;
      if (Parser->CurrentChunkParsedSize == Parser->CurrentChunkSize) {
        Parser->State = BodyParserChunkDataEnd;
      }           
      break;

    case BodyParserChunkDataEnd:
      if (*Char == '\r') {
        Parser->State = BodyParserChunkDataEndCR;
      } else {
        Parser->State = BodyParserStateMax;
      }
      Char++;
      break;

    case BodyParserChunkDataEndCR:
      if (*Char != '\n') {
        Parser->State = BodyParserStateMax;
        break;
      }
      Char++;
      Parser->State = BodyParserChunkSizeStart;
      break;     

    default:
      break;
    }

  }

  if (Parser->State == BodyParserStateMax) {
    return EFI_ABORTED;
  }

  return EFI_SUCCESS;
}

/**
  Check whether the message-body is complete or not.

  @param[in]    MsgParser            Pointer to the message parser.

  @retval TRUE                       Message-body is complete.
  @retval FALSE                      Message-body is not complete.

**/
BOOLEAN
EFIAPI
HttpIsMessageComplete (
  IN VOID              *MsgParser
  )
{
  HTTP_BODY_PARSER      *Parser;

  Parser = (HTTP_BODY_PARSER*) MsgParser;

  if (Parser->State == BodyParserComplete) {
    return TRUE;
  }
  return FALSE;
}

/**
  Get the content length of the entity.

  Note that in trunk transfer, the entity length is not valid until the whole message body is received.

  @param[in]    MsgParser            Pointer to the message parser.
  @param[out]   ContentLength        Pointer to store the length of the entity.

  @retval EFI_SUCCESS                Successfully to get the entity length.
  @retval EFI_NOT_READY              Entity length is not valid yet.
  @retval EFI_INVALID_PARAMETER      MsgParser is NULL or ContentLength is NULL.
  
**/
EFI_STATUS
EFIAPI
HttpGetEntityLength (
  IN  VOID              *MsgParser,
  OUT UINTN             *ContentLength
  )
{
  HTTP_BODY_PARSER      *Parser;

  if (MsgParser == NULL || ContentLength == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Parser = (HTTP_BODY_PARSER*) MsgParser;

  if (!Parser->ContentLengthIsValid) {
    return EFI_NOT_READY;
  }

  *ContentLength = Parser->ContentLength;
  return EFI_SUCCESS;
}

/**
  Release the resource of the message parser.

  @param[in]    MsgParser            Pointer to the message parser.
  
**/
VOID
EFIAPI
HttpFreeMsgParser (
  IN  VOID           *MsgParser
  )
{
  FreePool (MsgParser);
}
