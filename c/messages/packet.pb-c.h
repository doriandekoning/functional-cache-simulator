/* Generated by the protocol buffer compiler.  DO NOT EDIT! */
/* Generated from: packet.proto */

#ifndef PROTOBUF_C_packet_2eproto__INCLUDED
#define PROTOBUF_C_packet_2eproto__INCLUDED

#include <protobuf-c/protobuf-c.h>

PROTOBUF_C__BEGIN_DECLS

#if PROTOBUF_C_VERSION_NUMBER < 1000000
# error This file was generated by a newer version of protoc-c which is incompatible with your libprotobuf-c headers. Please update your headers.
#elif 1003002 < PROTOBUF_C_MIN_COMPILER_VERSION
# error This file was generated by an older version of protoc-c which is incompatible with your libprotobuf-c headers. Please regenerate this file with a newer version of protoc-c.
#endif


typedef struct _Messages__Packet Messages__Packet;
typedef struct _Messages__PacketHeader Messages__PacketHeader;
typedef struct _Messages__PacketHeader__IdStringEntry Messages__PacketHeader__IdStringEntry;


/* --- enums --- */


/* --- messages --- */

struct  _Messages__Packet
{
  ProtobufCMessage base;
  uint64_t tick;
  uint32_t cmd;
  uint64_t addr;
  uint32_t size;
  protobuf_c_boolean has_flags;
  uint32_t flags;
  protobuf_c_boolean has_pkt_id;
  uint64_t pkt_id;
  protobuf_c_boolean has_pc;
  uint64_t pc;
  /*
   *Not used in the gem5 spec but used for the functional simulator
   */
  protobuf_c_boolean has_cpuid;
  uint64_t cpuid;
};
#define MESSAGES__PACKET__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&messages__packet__descriptor) \
    , 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }


struct  _Messages__PacketHeader__IdStringEntry
{
  ProtobufCMessage base;
  protobuf_c_boolean has_key;
  uint32_t key;
  char *value;
};
#define MESSAGES__PACKET_HEADER__ID_STRING_ENTRY__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&messages__packet_header__id_string_entry__descriptor) \
    , 0, 0, NULL }


struct  _Messages__PacketHeader
{
  ProtobufCMessage base;
  char *obj_id;
  protobuf_c_boolean has_ver;
  uint32_t ver;
  uint64_t tick_freq;
  size_t n_id_strings;
  Messages__PacketHeader__IdStringEntry **id_strings;
};
#define MESSAGES__PACKET_HEADER__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&messages__packet_header__descriptor) \
    , NULL, 0, 0u, 0, 0,NULL }


/* Messages__Packet methods */
void   messages__packet__init
                     (Messages__Packet         *message);
size_t messages__packet__get_packed_size
                     (const Messages__Packet   *message);
size_t messages__packet__pack
                     (const Messages__Packet   *message,
                      uint8_t             *out);
size_t messages__packet__pack_to_buffer
                     (const Messages__Packet   *message,
                      ProtobufCBuffer     *buffer);
Messages__Packet *
       messages__packet__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   messages__packet__free_unpacked
                     (Messages__Packet *message,
                      ProtobufCAllocator *allocator);
/* Messages__PacketHeader__IdStringEntry methods */
void   messages__packet_header__id_string_entry__init
                     (Messages__PacketHeader__IdStringEntry         *message);
/* Messages__PacketHeader methods */
void   messages__packet_header__init
                     (Messages__PacketHeader         *message);
size_t messages__packet_header__get_packed_size
                     (const Messages__PacketHeader   *message);
size_t messages__packet_header__pack
                     (const Messages__PacketHeader   *message,
                      uint8_t             *out);
size_t messages__packet_header__pack_to_buffer
                     (const Messages__PacketHeader   *message,
                      ProtobufCBuffer     *buffer);
Messages__PacketHeader *
       messages__packet_header__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   messages__packet_header__free_unpacked
                     (Messages__PacketHeader *message,
                      ProtobufCAllocator *allocator);
/* --- per-message closures --- */

typedef void (*Messages__Packet_Closure)
                 (const Messages__Packet *message,
                  void *closure_data);
typedef void (*Messages__PacketHeader__IdStringEntry_Closure)
                 (const Messages__PacketHeader__IdStringEntry *message,
                  void *closure_data);
typedef void (*Messages__PacketHeader_Closure)
                 (const Messages__PacketHeader *message,
                  void *closure_data);

/* --- services --- */


/* --- descriptors --- */

extern const ProtobufCMessageDescriptor messages__packet__descriptor;
extern const ProtobufCMessageDescriptor messages__packet_header__descriptor;
extern const ProtobufCMessageDescriptor messages__packet_header__id_string_entry__descriptor;

PROTOBUF_C__END_DECLS


#endif  /* PROTOBUF_C_packet_2eproto__INCLUDED */
