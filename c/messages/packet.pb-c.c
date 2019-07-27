/* Generated by the protocol buffer compiler.  DO NOT EDIT! */
/* Generated from: packet.proto */

/* Do not generate deprecated warnings for self */
#ifndef PROTOBUF_C__NO_DEPRECATED
#define PROTOBUF_C__NO_DEPRECATED
#endif

#include "packet.pb-c.h"
void   messages__packet__init
                     (Messages__Packet         *message)
{
  static const Messages__Packet init_value = MESSAGES__PACKET__INIT;
  *message = init_value;
}
size_t messages__packet__get_packed_size
                     (const Messages__Packet *message)
{
  assert(message->base.descriptor == &messages__packet__descriptor);
  return protobuf_c_message_get_packed_size ((const ProtobufCMessage*)(message));
}
size_t messages__packet__pack
                     (const Messages__Packet *message,
                      uint8_t       *out)
{
  assert(message->base.descriptor == &messages__packet__descriptor);
  return protobuf_c_message_pack ((const ProtobufCMessage*)message, out);
}
size_t messages__packet__pack_to_buffer
                     (const Messages__Packet *message,
                      ProtobufCBuffer *buffer)
{
  assert(message->base.descriptor == &messages__packet__descriptor);
  return protobuf_c_message_pack_to_buffer ((const ProtobufCMessage*)message, buffer);
}
Messages__Packet *
       messages__packet__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data)
{
  return (Messages__Packet *)
     protobuf_c_message_unpack (&messages__packet__descriptor,
                                allocator, len, data);
}
void   messages__packet__free_unpacked
                     (Messages__Packet *message,
                      ProtobufCAllocator *allocator)
{
  if(!message)
    return;
  assert(message->base.descriptor == &messages__packet__descriptor);
  protobuf_c_message_free_unpacked ((ProtobufCMessage*)message, allocator);
}
void   messages__packet_header__id_string_entry__init
                     (Messages__PacketHeader__IdStringEntry         *message)
{
  static const Messages__PacketHeader__IdStringEntry init_value = MESSAGES__PACKET_HEADER__ID_STRING_ENTRY__INIT;
  *message = init_value;
}
void   messages__packet_header__init
                     (Messages__PacketHeader         *message)
{
  static const Messages__PacketHeader init_value = MESSAGES__PACKET_HEADER__INIT;
  *message = init_value;
}
size_t messages__packet_header__get_packed_size
                     (const Messages__PacketHeader *message)
{
  assert(message->base.descriptor == &messages__packet_header__descriptor);
  return protobuf_c_message_get_packed_size ((const ProtobufCMessage*)(message));
}
size_t messages__packet_header__pack
                     (const Messages__PacketHeader *message,
                      uint8_t       *out)
{
  assert(message->base.descriptor == &messages__packet_header__descriptor);
  return protobuf_c_message_pack ((const ProtobufCMessage*)message, out);
}
size_t messages__packet_header__pack_to_buffer
                     (const Messages__PacketHeader *message,
                      ProtobufCBuffer *buffer)
{
  assert(message->base.descriptor == &messages__packet_header__descriptor);
  return protobuf_c_message_pack_to_buffer ((const ProtobufCMessage*)message, buffer);
}
Messages__PacketHeader *
       messages__packet_header__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data)
{
  return (Messages__PacketHeader *)
     protobuf_c_message_unpack (&messages__packet_header__descriptor,
                                allocator, len, data);
}
void   messages__packet_header__free_unpacked
                     (Messages__PacketHeader *message,
                      ProtobufCAllocator *allocator)
{
  if(!message)
    return;
  assert(message->base.descriptor == &messages__packet_header__descriptor);
  protobuf_c_message_free_unpacked ((ProtobufCMessage*)message, allocator);
}
static const ProtobufCFieldDescriptor messages__packet__field_descriptors[8] =
{
  {
    "tick",
    1,
    PROTOBUF_C_LABEL_REQUIRED,
    PROTOBUF_C_TYPE_UINT64,
    0,   /* quantifier_offset */
    offsetof(Messages__Packet, tick),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "cmd",
    2,
    PROTOBUF_C_LABEL_REQUIRED,
    PROTOBUF_C_TYPE_UINT32,
    0,   /* quantifier_offset */
    offsetof(Messages__Packet, cmd),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "addr",
    3,
    PROTOBUF_C_LABEL_REQUIRED,
    PROTOBUF_C_TYPE_UINT64,
    0,   /* quantifier_offset */
    offsetof(Messages__Packet, addr),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "size",
    4,
    PROTOBUF_C_LABEL_REQUIRED,
    PROTOBUF_C_TYPE_UINT32,
    0,   /* quantifier_offset */
    offsetof(Messages__Packet, size),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "flags",
    5,
    PROTOBUF_C_LABEL_OPTIONAL,
    PROTOBUF_C_TYPE_UINT32,
    offsetof(Messages__Packet, has_flags),
    offsetof(Messages__Packet, flags),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "pkt_id",
    6,
    PROTOBUF_C_LABEL_OPTIONAL,
    PROTOBUF_C_TYPE_UINT64,
    offsetof(Messages__Packet, has_pkt_id),
    offsetof(Messages__Packet, pkt_id),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "pc",
    7,
    PROTOBUF_C_LABEL_OPTIONAL,
    PROTOBUF_C_TYPE_UINT64,
    offsetof(Messages__Packet, has_pc),
    offsetof(Messages__Packet, pc),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "cpuID",
    8,
    PROTOBUF_C_LABEL_OPTIONAL,
    PROTOBUF_C_TYPE_UINT64,
    offsetof(Messages__Packet, has_cpuid),
    offsetof(Messages__Packet, cpuid),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
};
static const unsigned messages__packet__field_indices_by_name[] = {
  2,   /* field[2] = addr */
  1,   /* field[1] = cmd */
  7,   /* field[7] = cpuID */
  4,   /* field[4] = flags */
  6,   /* field[6] = pc */
  5,   /* field[5] = pkt_id */
  3,   /* field[3] = size */
  0,   /* field[0] = tick */
};
static const ProtobufCIntRange messages__packet__number_ranges[1 + 1] =
{
  { 1, 0 },
  { 0, 8 }
};
const ProtobufCMessageDescriptor messages__packet__descriptor =
{
  PROTOBUF_C__MESSAGE_DESCRIPTOR_MAGIC,
  "messages.Packet",
  "Packet",
  "Messages__Packet",
  "messages",
  sizeof(Messages__Packet),
  8,
  messages__packet__field_descriptors,
  messages__packet__field_indices_by_name,
  1,  messages__packet__number_ranges,
  (ProtobufCMessageInit) messages__packet__init,
  NULL,NULL,NULL    /* reserved[123] */
};
static const ProtobufCFieldDescriptor messages__packet_header__id_string_entry__field_descriptors[2] =
{
  {
    "key",
    1,
    PROTOBUF_C_LABEL_OPTIONAL,
    PROTOBUF_C_TYPE_UINT32,
    offsetof(Messages__PacketHeader__IdStringEntry, has_key),
    offsetof(Messages__PacketHeader__IdStringEntry, key),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "value",
    2,
    PROTOBUF_C_LABEL_OPTIONAL,
    PROTOBUF_C_TYPE_STRING,
    0,   /* quantifier_offset */
    offsetof(Messages__PacketHeader__IdStringEntry, value),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
};
static const unsigned messages__packet_header__id_string_entry__field_indices_by_name[] = {
  0,   /* field[0] = key */
  1,   /* field[1] = value */
};
static const ProtobufCIntRange messages__packet_header__id_string_entry__number_ranges[1 + 1] =
{
  { 1, 0 },
  { 0, 2 }
};
const ProtobufCMessageDescriptor messages__packet_header__id_string_entry__descriptor =
{
  PROTOBUF_C__MESSAGE_DESCRIPTOR_MAGIC,
  "messages.PacketHeader.IdStringEntry",
  "IdStringEntry",
  "Messages__PacketHeader__IdStringEntry",
  "messages",
  sizeof(Messages__PacketHeader__IdStringEntry),
  2,
  messages__packet_header__id_string_entry__field_descriptors,
  messages__packet_header__id_string_entry__field_indices_by_name,
  1,  messages__packet_header__id_string_entry__number_ranges,
  (ProtobufCMessageInit) messages__packet_header__id_string_entry__init,
  NULL,NULL,NULL    /* reserved[123] */
};
static const uint32_t messages__packet_header__ver__default_value = 0u;
static const ProtobufCFieldDescriptor messages__packet_header__field_descriptors[4] =
{
  {
    "obj_id",
    1,
    PROTOBUF_C_LABEL_REQUIRED,
    PROTOBUF_C_TYPE_STRING,
    0,   /* quantifier_offset */
    offsetof(Messages__PacketHeader, obj_id),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "ver",
    2,
    PROTOBUF_C_LABEL_OPTIONAL,
    PROTOBUF_C_TYPE_UINT32,
    offsetof(Messages__PacketHeader, has_ver),
    offsetof(Messages__PacketHeader, ver),
    NULL,
    &messages__packet_header__ver__default_value,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "tick_freq",
    3,
    PROTOBUF_C_LABEL_REQUIRED,
    PROTOBUF_C_TYPE_UINT64,
    0,   /* quantifier_offset */
    offsetof(Messages__PacketHeader, tick_freq),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "id_strings",
    4,
    PROTOBUF_C_LABEL_REPEATED,
    PROTOBUF_C_TYPE_MESSAGE,
    offsetof(Messages__PacketHeader, n_id_strings),
    offsetof(Messages__PacketHeader, id_strings),
    &messages__packet_header__id_string_entry__descriptor,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
};
static const unsigned messages__packet_header__field_indices_by_name[] = {
  3,   /* field[3] = id_strings */
  0,   /* field[0] = obj_id */
  2,   /* field[2] = tick_freq */
  1,   /* field[1] = ver */
};
static const ProtobufCIntRange messages__packet_header__number_ranges[1 + 1] =
{
  { 1, 0 },
  { 0, 4 }
};
const ProtobufCMessageDescriptor messages__packet_header__descriptor =
{
  PROTOBUF_C__MESSAGE_DESCRIPTOR_MAGIC,
  "messages.PacketHeader",
  "PacketHeader",
  "Messages__PacketHeader",
  "messages",
  sizeof(Messages__PacketHeader),
  4,
  messages__packet_header__field_descriptors,
  messages__packet_header__field_indices_by_name,
  1,  messages__packet_header__number_ranges,
  (ProtobufCMessageInit) messages__packet_header__init,
  NULL,NULL,NULL    /* reserved[123] */
};
