/** @file

Copyright (c) 2006 - 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  Mtftp4Rrq.c

Abstract:

  Routines to process Rrq (download)


**/

#include "Mtftp4Impl.h"

VOID
Mtftp4RrqInput (
  IN NET_BUF                *UdpPacket,
  IN UDP_POINTS             *Points,
  IN EFI_STATUS             IoStatus,
  IN VOID                   *Context
  );


/**
  Start the MTFTP session to download. It will first initialize some
  of the internal states then build and send a RRQ reqeuest packet, at
  last, it will start receive for the downloading.

  @param  Instance              The Mtftp session
  @param  Operation             The MTFTP opcode, it may be a EFI_MTFTP4_OPCODE_RRQ
                                or EFI_MTFTP4_OPCODE_DIR.

  @retval EFI_SUCCESS           The mtftp download session is started.
  @retval Others                Failed to start downloading.

**/
EFI_STATUS
Mtftp4RrqStart (
  IN MTFTP4_PROTOCOL        *Instance,
  IN UINT16                 Operation
  )
{
  EFI_STATUS                Status;

  //
  // The valid block number range are [1, 0xffff]. For example:
  // the client sends an RRQ request to the server, the server
  // transfers the DATA1 block. If option negoitation is ongoing,
  // the server will send back an OACK, then client will send ACK0.
  //
  Status = Mtftp4InitBlockRange (&Instance->Blocks, 1, 0xffff);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = Mtftp4SendRequest (Instance);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  return UdpIoRecvDatagram (Instance->UnicastPort, Mtftp4RrqInput, Instance, 0);
}


/**
  Build and send a ACK packet for the download session.

  @param  Instance              The Mtftp session
  @param  BlkNo                 The BlkNo to ack.

  @retval EFI_OUT_OF_RESOURCES  Failed to allocate memory for the packet
  @retval EFI_SUCCESS           The ACK has been sent
  @retval Others                Failed to send the ACK.

**/
EFI_STATUS
Mtftp4RrqSendAck (
  IN MTFTP4_PROTOCOL        *Instance,
  IN UINT16                 BlkNo
  )
{
  EFI_MTFTP4_PACKET         *Ack;
  NET_BUF                   *Packet;

  Packet = NetbufAlloc (sizeof (EFI_MTFTP4_ACK_HEADER));

  if (Packet == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Ack = (EFI_MTFTP4_PACKET *) NetbufAllocSpace (
                                Packet,
                                sizeof (EFI_MTFTP4_ACK_HEADER),
                                FALSE
                                );

  Ack->Ack.OpCode   = HTONS (EFI_MTFTP4_OPCODE_ACK);
  Ack->Ack.Block[0] = HTONS (BlkNo);

  return Mtftp4SendPacket (Instance, Packet);
}


/**
  Deliver the received data block to the user, which can be saved
  in the user provide buffer or through the CheckPacket callback.

  @param  Instance              The Mtftp session
  @param  Packet                The received data packet
  @param  Len                   The packet length

  @retval EFI_SUCCESS           The data is saved successfully
  @retval EFI_ABORTED           The user tells to abort by return an error  through
                                CheckPacket
  @retval EFI_BUFFER_TOO_SMALL  The user's buffer is too small and buffer length is
                                 updated to the actual buffer size needed.

**/
EFI_STATUS
Mtftp4RrqSaveBlock (
  IN MTFTP4_PROTOCOL        *Instance,
  IN EFI_MTFTP4_PACKET      *Packet,
  IN UINT32                 Len
  )
{
  EFI_MTFTP4_TOKEN          *Token;
  EFI_STATUS                Status;
  UINT16                    Block;
  UINT64                    Start;
  UINT32                    DataLen;

  Token   = Instance->Token;
  Block   = NTOHS (Packet->Data.Block);
  DataLen = Len - MTFTP4_DATA_HEAD_LEN;

  //
  // This is the last block, save the block no
  //
  if (DataLen < Instance->BlkSize) {
    Instance->LastBlock = Block;
    Mtftp4SetLastBlockNum (&Instance->Blocks, Block);
  }

  //
  // Remove this block number from the file hole. If Mtftp4RemoveBlockNum
  // returns EFI_NOT_FOUND, the block has been saved, don't save it again.
  //
  Status = Mtftp4RemoveBlockNum (&Instance->Blocks, Block);

  if (Status == EFI_NOT_FOUND) {
    return EFI_SUCCESS;
  } else if (EFI_ERROR (Status)) {
    return Status;
  }

  if (Token->CheckPacket != NULL) {
    Status = Token->CheckPacket (&Instance->Mtftp4, Token, (UINT16) Len, Packet);

    if (EFI_ERROR (Status)) {
      Mtftp4SendError (
        Instance,
        EFI_MTFTP4_ERRORCODE_ILLEGAL_OPERATION,
        "User aborted download"
        );

      return EFI_ABORTED;
    }
  }

  if (Token->Buffer != NULL) {
    Start = MultU64x32 (Block - 1, Instance->BlkSize);

    if (Start + DataLen <= Token->BufferSize) {
      NetCopyMem ((UINT8 *) Token->Buffer + Start, Packet->Data.Data, DataLen);

      //
      // Update the file size when received the last block
      //
      if (Instance->LastBlock == Block) {
        Token->BufferSize = Start + DataLen;
      }

    } else if (Instance->LastBlock != 0) {
      //
      // Don't save the data if the buffer is too small, return
      // EFI_BUFFER_TOO_SMALL if received the last packet. This
      // will give a accurate file length.
      //
      Token->BufferSize = Start + DataLen;

      Mtftp4SendError (
        Instance,
        EFI_MTFTP4_ERRORCODE_DISK_FULL,
        "User provided memory block is too small"
        );

      return EFI_BUFFER_TOO_SMALL;
    }
  }

  return EFI_SUCCESS;
}


/**
  Function to process the received data packets. It will save the block
  then send back an ACK if it is active.

  @param  Instance              The downloading MTFTP session
  @param  Packet                The packet received
  @param  Len                   The length of the packet
  @param  Multicast             Whether this packet is multicast or unicast
  @param  Completed             Return whether the download has completed

  @retval EFI_SUCCESS           The data packet is successfully processed
  @retval EFI_ABORTED           The download is aborted by the user
  @retval EFI_BUFFER_TOO_SMALL  The user provided buffer is too small

**/
EFI_STATUS
Mtftp4RrqHandleData (
  IN  MTFTP4_PROTOCOL       *Instance,
  IN  EFI_MTFTP4_PACKET     *Packet,
  IN  UINT32                Len,
  IN  BOOLEAN               Multicast,
  OUT BOOLEAN               *Completed
  )
{
  EFI_STATUS                Status;
  UINT16                    BlockNum;
  INTN                      Expected;

  *Completed  = FALSE;
  BlockNum    = NTOHS (Packet->Data.Block);
  Expected    = Mtftp4GetNextBlockNum (&Instance->Blocks);

  ASSERT (Expected >= 0);

  //
  // If we are active and received an unexpected packet, retransmit
  // the last ACK then restart receiving. If we are passive, save
  // the block.
  //
  if (Instance->Master && (Expected != BlockNum)) {
    Mtftp4Retransmit (Instance);
    return EFI_SUCCESS;
  }

  Status = Mtftp4RrqSaveBlock (Instance, Packet, Len);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Reset the passive client's timer whenever it received a
  // valid data packet.
  //
  if (!Instance->Master) {
    Mtftp4SetTimeout (Instance);
  }

  //
  // Check whether we have received all the blocks. Send the ACK if we
  // are active (unicast client or master client for multicast download).
  // If we have received all the blocks, send an ACK even if we are passive
  // to tell the server that we are done.
  //
  Expected = Mtftp4GetNextBlockNum (&Instance->Blocks);

  if (Instance->Master || (Expected < 0)) {
    if (Expected < 0) {
      //
      // If we are passive client, then the just received Block maybe
      // isn't the last block. We need to send an ACK to the last block
      // to inform the server that we are done. If we are active client,
      // the Block == Instance->LastBlock.
      //
      BlockNum   = Instance->LastBlock;
      *Completed = TRUE;

    } else {
      BlockNum = (UINT16) (Expected - 1);
    }

    Mtftp4RrqSendAck (Instance, BlockNum);
  }

  return EFI_SUCCESS;
}


/**
  Validate whether the options received in the server's OACK packet is valid.
  The options are valid only if:
  1. The server doesn't include options not requested by us
  2. The server can only use smaller blksize than that is requested
  3. The server can only use the same timeout as requested
  4. The server doesn't change its multicast channel.

  @param  This                  The downloading Mtftp session
  @param  Reply                 The options in the OACK packet
  @param  Request               The requested options

  @return TRUE if the options in the OACK is OK, otherwise FALSE.

**/
BOOLEAN
Mtftp4RrqOackValid (
  IN MTFTP4_PROTOCOL        *This,
  IN MTFTP4_OPTION          *Reply,
  IN MTFTP4_OPTION          *Request
  )
{

  //
  // It is invalid for server to return options we don't request
  //
  if ((Reply->Exist &~Request->Exist) != 0) {
    return FALSE;
  }

  //
  // Server can only specify a smaller block size to be used and
  // return the timeout matches that requested.
  //
  if (((Reply->Exist & MTFTP4_BLKSIZE_EXIST) && (Reply->BlkSize > Request->BlkSize)) ||
      ((Reply->Exist & MTFTP4_TIMEOUT_EXIST) && (Reply->Timeout != Request->Timeout))) {
    return FALSE;
  }

  //
  // The server can send ",,master" to client to change its master
  // setting. But if it use the specific multicast channel, it can't
  // change the setting.
  //
  if ((Reply->Exist & MTFTP4_MCAST_EXIST) && (This->McastIp != 0)) {
    if ((Reply->McastIp != 0) && (Reply->McastIp != This->McastIp)) {
      return FALSE;
    }

    if ((Reply->McastPort != 0) && (Reply->McastPort != This->McastPort)) {
      return FALSE;
    }
  }

  return TRUE;
}


/**
  Configure a UDP IO port to receive the multicast.

  @param  McastIo               The UDP IO port to configure
  @param  Context               The opaque parameter to the function which is the
                                MTFTP session.

  @retval EFI_SUCCESS           The udp child is successfully configured.
  @retval Others                Failed to configure the UDP child.

**/
STATIC
EFI_STATUS
Mtftp4RrqConfigMcastPort (
  IN UDP_IO_PORT            *McastIo,
  IN VOID                   *Context
  )
{
  MTFTP4_PROTOCOL           *Instance;
  EFI_MTFTP4_CONFIG_DATA    *Config;
  EFI_UDP4_CONFIG_DATA      UdpConfig;
  EFI_IPv4_ADDRESS          Group;
  EFI_STATUS                Status;
  IP4_ADDR                  Ip;

  Instance                     = (MTFTP4_PROTOCOL *) Context;
  Config                       = &Instance->Config;

  UdpConfig.AcceptBroadcast    = FALSE;
  UdpConfig.AcceptPromiscuous  = FALSE;
  UdpConfig.AcceptAnyPort      = FALSE;
  UdpConfig.AllowDuplicatePort = FALSE;
  UdpConfig.TypeOfService      = 0;
  UdpConfig.TimeToLive         = 64;
  UdpConfig.DoNotFragment      = FALSE;
  UdpConfig.ReceiveTimeout     = 0;
  UdpConfig.TransmitTimeout    = 0;
  UdpConfig.UseDefaultAddress  = Config->UseDefaultSetting;
  UdpConfig.StationAddress     = Config->StationIp;
  UdpConfig.SubnetMask         = Config->SubnetMask;
  UdpConfig.StationPort        = Instance->McastPort;
  UdpConfig.RemotePort         = 0;

  Ip = HTONL (Instance->ServerIp);
  NetCopyMem (&UdpConfig.RemoteAddress, &Ip, sizeof (EFI_IPv4_ADDRESS));

  Status = McastIo->Udp->Configure (McastIo->Udp, &UdpConfig);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // join the multicast group
  //
  Ip = HTONL (Instance->McastIp);
  NetCopyMem (&Group, &Ip, sizeof (EFI_IPv4_ADDRESS));

  return McastIo->Udp->Groups (McastIo->Udp, TRUE, &Group);
}


/**
  Function to process the OACK. It will first validate the OACK
  packet, then update the various negotiated parameters.

  @param  Instance              The download MTFTP session
  @param  Packet                The packet received
  @param  Len                   The packet length
  @param  Multicast             Whether this packet is received as a multicast
  @param  Completed             Returns whether the download has completed. NOT
                                used  by this function.

  @retval EFI_DEVICE_ERROR      Failed to create/start a multicast UDP child
  @retval EFI_TFTP_ERROR        Some error happened during the process
  @retval EFI_SUCCESS           The OACK is successfully processed.

**/
EFI_STATUS
Mtftp4RrqHandleOack (
  IN  MTFTP4_PROTOCOL       *Instance,
  IN  EFI_MTFTP4_PACKET     *Packet,
  IN  UINT32                Len,
  IN  BOOLEAN               Multicast,
  OUT BOOLEAN               *Completed
  )
{
  MTFTP4_OPTION             Reply;
  EFI_STATUS                Status;
  INTN                      Expected;

  *Completed = FALSE;

  //
  // If already started the master download, don't change the
  // setting. Master download always succeeds.
  //
  Expected = Mtftp4GetNextBlockNum (&Instance->Blocks);
  ASSERT (Expected != -1);

  if (Instance->Master && (Expected != 1)) {
    return EFI_SUCCESS;
  }

  //
  // Parse and validate the options from server
  //
  NetZeroMem (&Reply, sizeof (MTFTP4_OPTION));

  Status = Mtftp4ParseOptionOack (Packet, Len, &Reply);

  if (EFI_ERROR (Status) ||
      !Mtftp4RrqOackValid (Instance, &Reply, &Instance->RequestOption)) {
    //
    // Don't send an ERROR packet if the error is EFI_OUT_OF_RESOURCES.
    //
    if (Status != EFI_OUT_OF_RESOURCES) {
      Mtftp4SendError (
        Instance,
        EFI_MTFTP4_ERRORCODE_ILLEGAL_OPERATION,
        "Mal-formated OACK packet"
        );
    }

    return EFI_TFTP_ERROR;
  }

  if (Reply.Exist & MTFTP4_MCAST_EXIST) {

    //
    // Save the multicast info. Always update the Master, only update the
    // multicast IP address, block size, timeoute at the first time. If IP
    // address is updated, create a UDP child to receive the multicast.
    //
    Instance->Master = Reply.Master;

    if (Instance->McastIp == 0) {
      if ((Reply.McastIp == 0) || (Reply.McastPort == 0)) {
        Mtftp4SendError (
          Instance,
          EFI_MTFTP4_ERRORCODE_ILLEGAL_OPERATION,
          "Illegal multicast setting"
          );

        return EFI_TFTP_ERROR;
      }

      //
      // Create a UDP child then start receive the multicast from it.
      //
      Instance->McastIp      = Reply.McastIp;
      Instance->McastPort    = Reply.McastPort;
      Instance->McastUdpPort = UdpIoCreatePort (
                                 Instance->Service->Controller,
                                 Instance->Service->Image,
                                 Mtftp4RrqConfigMcastPort,
                                 Instance
                                 );

      if (Instance->McastUdpPort == NULL) {
        return EFI_DEVICE_ERROR;
      }

      Status = UdpIoRecvDatagram (Instance->McastUdpPort, Mtftp4RrqInput, Instance, 0);

      if (EFI_ERROR (Status)) {
        Mtftp4SendError (
          Instance,
          EFI_MTFTP4_ERRORCODE_ACCESS_VIOLATION,
          "Failed to create socket to receive multicast packet"
          );

        return Status;
      }

      //
      // Update the parameters used.
      //
      if (Reply.BlkSize != 0) {
        Instance->BlkSize = Reply.BlkSize;
      }

      if (Reply.Timeout != 0) {
        Instance->Timeout = Reply.Timeout;
      }
    }

  } else {
    Instance->Master = TRUE;

    if (Reply.BlkSize != 0) {
      Instance->BlkSize = Reply.BlkSize;
    }

    if (Reply.Timeout != 0) {
      Instance->Timeout = Reply.Timeout;
    }
  }

  //
  // Send an ACK to (Expected - 1) which is 0 for unicast download,
  // or tell the server we want to receive the Expected block.
  //
  return Mtftp4RrqSendAck (Instance, (UINT16) (Expected - 1));
}


/**
  The packet process callback for MTFTP download.

  @param  UdpPacket             The packet received
  @param  Points                The local/remote access point of the packet
  @param  IoStatus              The status of the receiving
  @param  Context               Opaque parameter, which is the MTFTP session

  @return None

**/
VOID
Mtftp4RrqInput (
  IN NET_BUF                *UdpPacket,
  IN UDP_POINTS             *Points,
  IN EFI_STATUS             IoStatus,
  IN VOID                   *Context
  )
{
  MTFTP4_PROTOCOL           *Instance;
  EFI_MTFTP4_PACKET         *Packet;
  BOOLEAN                   Completed;
  BOOLEAN                   Multicast;
  EFI_STATUS                Status;
  UINT16                    Opcode;
  UINT32                    Len;

  Instance  = (MTFTP4_PROTOCOL *) Context;
  NET_CHECK_SIGNATURE (Instance, MTFTP4_PROTOCOL_SIGNATURE);

  Status    = EFI_SUCCESS;
  Packet    = NULL;
  Completed = FALSE;
  Multicast = FALSE;

  if (EFI_ERROR (IoStatus)) {
    Status = IoStatus;
    goto ON_EXIT;
  }

  ASSERT (UdpPacket != NULL);

  //
  // Find the port this packet is from to restart receive correctly.
  //
  Multicast = (BOOLEAN) (Points->LocalAddr == Instance->McastIp);

  if (UdpPacket->TotalSize < MTFTP4_OPCODE_LEN) {
    goto ON_EXIT;
  }

  //
  // Client send initial request to server's listening port. Server
  // will select a UDP port to communicate with the client. The server
  // is required to use the same port as RemotePort to multicast the
  // data.
  //
  if (Points->RemotePort != Instance->ConnectedPort) {
    if (Instance->ConnectedPort != 0) {
      goto ON_EXIT;
    } else {
      Instance->ConnectedPort = Points->RemotePort;
    }
  }

  //
  // Copy the MTFTP packet to a continuous buffer if it isn't already so.
  //
  Len = UdpPacket->TotalSize;

  if (UdpPacket->BlockOpNum > 1) {
    Packet = NetAllocatePool (Len);

    if (Packet == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto ON_EXIT;
    }

    NetbufCopy (UdpPacket, 0, Len, (UINT8 *) Packet);

  } else {
    Packet = (EFI_MTFTP4_PACKET *) NetbufGetByte (UdpPacket, 0, NULL);
  }

  Opcode = NTOHS (Packet->OpCode);

  //
  // Call the user's CheckPacket if provided. Abort the transmission
  // if CheckPacket returns an EFI_ERROR code.
  //
  if ((Instance->Token->CheckPacket != NULL) &&
      ((Opcode == EFI_MTFTP4_OPCODE_OACK) || (Opcode == EFI_MTFTP4_OPCODE_ERROR))) {

    Status = Instance->Token->CheckPacket (
                                &Instance->Mtftp4,
                                Instance->Token,
                                (UINT16) Len,
                                Packet
                                );

    if (EFI_ERROR (Status)) {
      //
      // Send an error message to the server to inform it
      //
      if (Opcode != EFI_MTFTP4_OPCODE_ERROR) {
        Mtftp4SendError (
          Instance,
          EFI_MTFTP4_ERRORCODE_REQUEST_DENIED,
          "User aborted the transfer"
          );
      }

      Status = EFI_ABORTED;
      goto ON_EXIT;
    }
  }

  switch (Opcode) {
  case EFI_MTFTP4_OPCODE_DATA:
    if ((Len > (UINT32) (MTFTP4_DATA_HEAD_LEN + Instance->BlkSize)) ||
        (Len < (UINT32) MTFTP4_DATA_HEAD_LEN)) {
      goto ON_EXIT;
    }

    Status = Mtftp4RrqHandleData (Instance, Packet, Len, Multicast, &Completed);
    break;

  case EFI_MTFTP4_OPCODE_OACK:
    if (Multicast || (Len <= MTFTP4_OPCODE_LEN)) {
      goto ON_EXIT;
    }

    Status = Mtftp4RrqHandleOack (Instance, Packet, Len, Multicast, &Completed);
    break;

  case EFI_MTFTP4_OPCODE_ERROR:
    Status = EFI_TFTP_ERROR;
    break;
  }

ON_EXIT:

  //
  // Free the resources, then if !EFI_ERROR (Status), restart the
  // receive, otherwise end the session.
  //
  if ((Packet != NULL) && (UdpPacket->BlockOpNum > 1)) {
    NetFreePool (Packet);
  }

  if (UdpPacket != NULL) {
    NetbufFree (UdpPacket);
  }

  if (!EFI_ERROR (Status) && !Completed) {
    if (Multicast) {
      Status = UdpIoRecvDatagram (Instance->McastUdpPort, Mtftp4RrqInput, Instance, 0);
    } else {
      Status = UdpIoRecvDatagram (Instance->UnicastPort, Mtftp4RrqInput, Instance, 0);
    }
  }

  if (EFI_ERROR (Status) || Completed) {
    Mtftp4CleanOperation (Instance, Status);
  }
}
