/*
 * Minimal librdkafka header for Kafka CLI Tool
 * This is a subset of the full rdkafka.h for compilation purposes.
 * For production use, use the complete header from the librdkafka distribution.
 */

#ifndef _RDKAFKA_H_
#define _RDKAFKA_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#ifndef _SSIZE_T_DEFINED
#define _SSIZE_T_DEFINED
typedef SSIZE_T ssize_t;
#endif
#define RD_EXPORT __declspec(dllimport)
#else
#include <sys/socket.h>
#define RD_EXPORT
#endif

typedef struct rd_kafka_s rd_kafka_t;
typedef struct rd_kafka_topic_s rd_kafka_topic_t;
typedef struct rd_kafka_conf_s rd_kafka_conf_t;
typedef struct rd_kafka_topic_conf_s rd_kafka_topic_conf_t;
typedef struct rd_kafka_queue_s rd_kafka_queue_t;
typedef struct rd_kafka_op_s rd_kafka_event_t;
typedef struct rd_kafka_topic_result_s rd_kafka_topic_result_t;
typedef struct rd_kafka_consumer_group_metadata_s rd_kafka_consumer_group_metadata_t;

typedef enum rd_kafka_type_t {
    RD_KAFKA_PRODUCER,
    RD_KAFKA_CONSUMER
} rd_kafka_type_t;

typedef enum rd_kafka_timestamp_type_t {
    RD_KAFKA_TIMESTAMP_NOT_AVAILABLE,
    RD_KAFKA_TIMESTAMP_CREATE_TIME,
    RD_KAFKA_TIMESTAMP_LOG_APPEND_TIME
} rd_kafka_timestamp_type_t;

typedef enum rd_kafka_conf_res_t {
    RD_KAFKA_CONF_UNKNOWN = -2,
    RD_KAFKA_CONF_INVALID = -1,
    RD_KAFKA_CONF_OK = 0
} rd_kafka_conf_res_t;

typedef enum rd_kafka_resp_err_t {
    RD_KAFKA_RESP_ERR__BEGIN = -200,
    RD_KAFKA_RESP_ERR__BAD_MSG = -199,
    RD_KAFKA_RESP_ERR__BAD_COMPRESSION = -198,
    RD_KAFKA_RESP_ERR__DESTROY = -197,
    RD_KAFKA_RESP_ERR__FAIL = -196,
    RD_KAFKA_RESP_ERR__TRANSPORT = -195,
    RD_KAFKA_RESP_ERR__CRIT_SYS_RESOURCE = -194,
    RD_KAFKA_RESP_ERR__RESOLVE = -193,
    RD_KAFKA_RESP_ERR__MSG_TIMED_OUT = -192,
    RD_KAFKA_RESP_ERR__PARTITION_EOF = -191,
    RD_KAFKA_RESP_ERR__UNKNOWN_PARTITION = -190,
    RD_KAFKA_RESP_ERR__FS = -189,
    RD_KAFKA_RESP_ERR__UNKNOWN_TOPIC = -188,
    RD_KAFKA_RESP_ERR__ALL_BROKERS_DOWN = -187,
    RD_KAFKA_RESP_ERR__INVALID_ARG = -186,
    RD_KAFKA_RESP_ERR__TIMED_OUT = -185,
    RD_KAFKA_RESP_ERR__QUEUE_FULL = -184,
    RD_KAFKA_RESP_ERR__ISR_INSUFF = -183,
    RD_KAFKA_RESP_ERR__NODE_UPDATE = -182,
    RD_KAFKA_RESP_ERR__SSL = -181,
    RD_KAFKA_RESP_ERR__WAIT_COORD = -180,
    RD_KAFKA_RESP_ERR__UNKNOWN_GROUP = -179,
    RD_KAFKA_RESP_ERR__IN_PROGRESS = -178,
    RD_KAFKA_RESP_ERR__PREV_IN_PROGRESS = -177,
    RD_KAFKA_RESP_ERR__EXISTING_SUBSCRIPTION = -176,
    RD_KAFKA_RESP_ERR__ASSIGN_PARTITIONS = -175,
    RD_KAFKA_RESP_ERR__REVOKE_PARTITIONS = -174,
    RD_KAFKA_RESP_ERR__CONFLICT = -173,
    RD_KAFKA_RESP_ERR__STATE = -172,
    RD_KAFKA_RESP_ERR__UNKNOWN_PROTOCOL = -171,
    RD_KAFKA_RESP_ERR__NOT_IMPLEMENTED = -170,
    RD_KAFKA_RESP_ERR__AUTHENTICATION = -169,
    RD_KAFKA_RESP_ERR__NO_OFFSET = -168,
    RD_KAFKA_RESP_ERR__OUTDATED = -167,
    RD_KAFKA_RESP_ERR__TIMED_OUT_QUEUE = -166,
    RD_KAFKA_RESP_ERR__UNSUPPORTED_FEATURE = -165,
    RD_KAFKA_RESP_ERR__WAIT_CACHE = -164,
    RD_KAFKA_RESP_ERR__INTR = -163,
    RD_KAFKA_RESP_ERR__KEY_SERIALIZATION = -162,
    RD_KAFKA_RESP_ERR__VALUE_SERIALIZATION = -161,
    RD_KAFKA_RESP_ERR__KEY_DESERIALIZATION = -160,
    RD_KAFKA_RESP_ERR__VALUE_DESERIALIZATION = -159,
    RD_KAFKA_RESP_ERR__PARTIAL = -158,
    RD_KAFKA_RESP_ERR__END = -100,
    RD_KAFKA_RESP_ERR_UNKNOWN = -1,
    RD_KAFKA_RESP_ERR_NO_ERROR = 0,
    RD_KAFKA_RESP_ERR_OFFSET_OUT_OF_RANGE = 1,
    RD_KAFKA_RESP_ERR_INVALID_MSG = 2,
    RD_KAFKA_RESP_ERR_UNKNOWN_TOPIC_OR_PART = 3,
    RD_KAFKA_RESP_ERR_INVALID_MSG_SIZE = 4,
    RD_KAFKA_RESP_ERR_LEADER_NOT_AVAILABLE = 5,
    RD_KAFKA_RESP_ERR_NOT_LEADER_FOR_PARTITION = 6,
    RD_KAFKA_RESP_ERR_REQUEST_TIMED_OUT = 7,
    RD_KAFKA_RESP_ERR_BROKER_NOT_AVAILABLE = 8,
    RD_KAFKA_RESP_ERR_REPLICA_NOT_AVAILABLE = 9,
    RD_KAFKA_RESP_ERR_MSG_SIZE_TOO_LARGE = 10,
    RD_KAFKA_RESP_ERR_STALE_CTRL_EPOCH = 11,
    RD_KAFKA_RESP_ERR_OFFSET_METADATA_TOO_LARGE = 12,
    RD_KAFKA_RESP_ERR_NETWORK_EXCEPTION = 13,
    RD_KAFKA_RESP_ERR_COORDINATOR_LOAD_IN_PROGRESS = 14,
    RD_KAFKA_RESP_ERR_COORDINATOR_NOT_AVAILABLE = 15,
    RD_KAFKA_RESP_ERR_NOT_COORDINATOR = 16,
    RD_KAFKA_RESP_ERR_INVALID_TOPIC_EXCEPTION = 17,
    RD_KAFKA_RESP_ERR_RECORD_LIST_TOO_LARGE = 18,
    RD_KAFKA_RESP_ERR_NOT_ENOUGH_REPLICAS = 19,
    RD_KAFKA_RESP_ERR_NOT_ENOUGH_REPLICAS_AFTER_APPEND = 20,
    RD_KAFKA_RESP_ERR_INVALID_REQUIRED_ACKS = 21,
    RD_KAFKA_RESP_ERR_ILLEGAL_GENERATION = 22,
    RD_KAFKA_RESP_ERR_INCONSISTENT_GROUP_PROTOCOL = 23,
    RD_KAFKA_RESP_ERR_INVALID_GROUP_ID = 24,
    RD_KAFKA_RESP_ERR_UNKNOWN_MEMBER_ID = 25,
    RD_KAFKA_RESP_ERR_INVALID_SESSION_TIMEOUT = 26,
    RD_KAFKA_RESP_ERR_REBALANCE_IN_PROGRESS = 27,
    RD_KAFKA_RESP_ERR_INVALID_COMMIT_OFFSET_SIZE = 28,
    RD_KAFKA_RESP_ERR_TOPIC_AUTHORIZATION_FAILED = 29,
    RD_KAFKA_RESP_ERR_GROUP_AUTHORIZATION_FAILED = 30,
    RD_KAFKA_RESP_ERR_CLUSTER_AUTHORIZATION_FAILED = 31,
    RD_KAFKA_RESP_ERR_INVALID_TIMESTAMP = 32,
    RD_KAFKA_RESP_ERR_UNSUPPORTED_SASL_MECHANISM = 33,
    RD_KAFKA_RESP_ERR_ILLEGAL_SASL_STATE = 34,
    RD_KAFKA_RESP_ERR_UNSUPPORTED_VERSION = 35,
    RD_KAFKA_RESP_ERR_TOPIC_ALREADY_EXISTS = 36,
    RD_KAFKA_RESP_ERR_INVALID_PARTITIONS = 37,
    RD_KAFKA_RESP_ERR_INVALID_REPLICATION_FACTOR = 38,
    RD_KAFKA_RESP_ERR_INVALID_REPLICA_ASSIGNMENT = 39,
    RD_KAFKA_RESP_ERR_INVALID_CONFIG = 40,
    RD_KAFKA_RESP_ERR_NOT_CONTROLLER = 41,
    RD_KAFKA_RESP_ERR_INVALID_REQUEST = 42,
    RD_KAFKA_RESP_ERR_UNSUPPORTED_FOR_MESSAGE_FORMAT = 43,
    RD_KAFKA_RESP_ERR_POLICY_VIOLATION = 44,
    RD_KAFKA_RESP_ERR_OUT_OF_ORDER_SEQUENCE_NUMBER = 45,
    RD_KAFKA_RESP_ERR_DUPLICATE_SEQUENCE_NUMBER = 46,
    RD_KAFKA_RESP_ERR_INVALID_PRODUCER_EPOCH = 47,
    RD_KAFKA_RESP_ERR_INVALID_TXN_STATE = 48,
    RD_KAFKA_RESP_ERR_INVALID_PRODUCER_ID_MAPPING = 49,
    RD_KAFKA_RESP_ERR_INVALID_TRANSACTION_TIMEOUT = 50,
    RD_KAFKA_RESP_ERR_CONCURRENT_TRANSACTIONS = 51,
    RD_KAFKA_RESP_ERR_TRANSACTION_COORDINATOR_FENCED = 52,
    RD_KAFKA_RESP_ERR_TRANSACTIONAL_ID_AUTHORIZATION_FAILED = 53,
    RD_KAFKA_RESP_ERR_SECURITY_DISABLED = 54,
    RD_KAFKA_RESP_ERR_OPERATION_NOT_ATTEMPTED = 55,
    RD_KAFKA_RESP_ERR_KAFKA_STORAGE_ERROR = 56,
    RD_KAFKA_RESP_ERR_LOG_DIR_NOT_FOUND = 57,
    RD_KAFKA_RESP_ERR_SASL_AUTHENTICATION_FAILED = 58,
    RD_KAFKA_RESP_ERR_UNKNOWN_PRODUCER_ID = 59,
    RD_KAFKA_RESP_ERR_REASSIGNMENT_IN_PROGRESS = 60,
    RD_KAFKA_RESP_ERR_DELEGATION_TOKEN_AUTH_DISABLED = 61,
    RD_KAFKA_RESP_ERR_DELEGATION_TOKEN_NOT_FOUND = 62,
    RD_KAFKA_RESP_ERR_DELEGATION_TOKEN_OWNER_MISMATCH = 63,
    RD_KAFKA_RESP_ERR_DELEGATION_TOKEN_REQUEST_NOT_ALLOWED = 64,
    RD_KAFKA_RESP_ERR_DELEGATION_TOKEN_AUTHORIZATION_FAILED = 65,
    RD_KAFKA_RESP_ERR_DELEGATION_TOKEN_EXPIRED = 66,
    RD_KAFKA_RESP_ERR_INVALID_PRINCIPAL_TYPE = 67,
    RD_KAFKA_RESP_ERR_NON_EMPTY_GROUP = 68,
    RD_KAFKA_RESP_ERR_GROUP_ID_NOT_FOUND = 69,
    RD_KAFKA_RESP_ERR_FETCH_SESSION_ID_NOT_FOUND = 70,
    RD_KAFKA_RESP_ERR_INVALID_FETCH_SESSION_EPOCH = 71,
    RD_KAFKA_RESP_ERR_LISTENER_NOT_FOUND = 72,
    RD_KAFKA_RESP_ERR_TOPIC_DELETION_DISABLED = 73,
    RD_KAFKA_RESP_ERR_FENCED_LEADER_EPOCH = 74,
    RD_KAFKA_RESP_ERR_UNKNOWN_LEADER_EPOCH = 75,
    RD_KAFKA_RESP_ERR_UNSUPPORTED_COMPRESSION_TYPE = 76,
    RD_KAFKA_RESP_ERR_STALE_BROKER_EPOCH = 77,
    RD_KAFKA_RESP_ERR_OFFSET_NOT_AVAILABLE = 78,
    RD_KAFKA_RESP_ERR_MEMBER_ID_REQUIRED = 79,
    RD_KAFKA_RESP_ERR_PREFERRED_LEADER_NOT_AVAILABLE = 80,
    RD_KAFKA_RESP_ERR_GROUP_MAX_SIZE_REACHED = 81,
    RD_KAFKA_RESP_ERR_FENCED_INSTANCE_ID = 82,
    RD_KAFKA_RESP_ERR_ELIGIBLE_LEADERS_AVAILABLE = 83,
    RD_KAFKA_RESP_ERR_ELECTION_NOT_NEEDED = 84,
    RD_KAFKA_RESP_ERR_NO_REASSIGNMENT_IN_PROGRESS = 85,
    RD_KAFKA_RESP_ERR_GROUP_SUBSCRIBED_TO_TOPIC = 86,
    RD_KAFKA_RESP_ERR_INVALID_RECORD = 87,
    RD_KAFKA_RESP_ERR_UNSTABLE_OFFSET_COMMIT = 88,
    RD_KAFKA_RESP_ERR_THROTTLING_QUOTA_EXCEEDED = 89,
    RD_KAFKA_RESP_ERR_PRODUCER_FENCED = 90,
    RD_KAFKA_RESP_ERR_RESOURCE_NOT_FOUND = 91,
    RD_KAFKA_RESP_ERR_DUPLICATE_RESOURCE = 92,
    RD_KAFKA_RESP_ERR_UNACCEPTABLE_CREDENTIAL = 93,
    RD_KAFKA_RESP_ERR_INCONSISTENT_VOTER_SET = 94,
    RD_KAFKA_RESP_ERR_INVALID_UPDATE_VERSION = 95,
    RD_KAFKA_RESP_ERR_FEATURE_UPDATE_FAILED = 96,
    RD_KAFKA_RESP_ERR_PRINCIPAL_DESERIALIZATION_FAILURE = 97,
    RD_KAFKA_RESP_ERR_UNKNOWN_TOPIC_ID = 100,
    RD_KAFKA_RESP_ERR_INCONSISTENT_TOPIC_ID = 101,
    RD_KAFKA_RESP_ERR_INCONSISTENT_CLUSTER_ID = 102,
    RD_KAFKA_RESP_END_ALL
} rd_kafka_resp_err_t;

typedef struct rd_kafka_message_s {
    rd_kafka_resp_err_t err;
    rd_kafka_topic_t *rkt;
    int32_t partition;
    void *payload;
    size_t len;
    void *key;
    size_t key_len;
    int64_t offset;
    void *_private;
} rd_kafka_message_t;

typedef struct rd_kafka_topic_partition_s {
    char *topic;
    int32_t partition;
    int64_t offset;
    void *metadata;
    size_t metadata_size;
    void *opaque;
    rd_kafka_resp_err_t err;
} rd_kafka_topic_partition_t;

typedef struct rd_kafka_topic_partition_list_s {
    int cnt;
    int size;
    rd_kafka_topic_partition_t *elems;
} rd_kafka_topic_partition_list_t;

RD_EXPORT int rd_kafka_version(void);
RD_EXPORT const char *rd_kafka_version_str(void);
RD_EXPORT const char *rd_kafka_get_debug_contexts(void);
RD_EXPORT const char *rd_kafka_err2str(rd_kafka_resp_err_t err);
RD_EXPORT rd_kafka_resp_err_t rd_kafka_last_error(void);
RD_EXPORT const char *rd_kafka_message_errstr(const rd_kafka_message_t *rkmessage);

RD_EXPORT rd_kafka_conf_t *rd_kafka_conf_new(void);
RD_EXPORT void rd_kafka_conf_destroy(rd_kafka_conf_t *conf);
RD_EXPORT rd_kafka_conf_res_t rd_kafka_conf_set(rd_kafka_conf_t *conf,
                                                  const char *name,
                                                  const char *value,
                                                  char *errstr,
                                                  size_t errstr_size);
RD_EXPORT void rd_kafka_conf_set_opaque(rd_kafka_conf_t *conf, void *opaque);
RD_EXPORT void rd_kafka_conf_set_dr_msg_cb(rd_kafka_conf_t *conf,
                                            void (*dr_msg_cb)(rd_kafka_t *rk,
                                                              const rd_kafka_message_t *rkmessage,
                                                              void *opaque));

RD_EXPORT rd_kafka_t *rd_kafka_new(rd_kafka_type_t type,
                                    rd_kafka_conf_t *conf,
                                    char *errstr,
                                    size_t errstr_size);
RD_EXPORT void rd_kafka_destroy(rd_kafka_t *rk);
RD_EXPORT const char *rd_kafka_name(const rd_kafka_t *rk);
RD_EXPORT rd_kafka_type_t rd_kafka_type(const rd_kafka_t *rk);
RD_EXPORT int rd_kafka_poll(rd_kafka_t *rk, int timeout_ms);
RD_EXPORT void rd_kafka_pause_partitions(rd_kafka_t *rk,
                                          rd_kafka_topic_partition_list_t *partitions);
RD_EXPORT void rd_kafka_resume_partitions(rd_kafka_t *rk,
                                           rd_kafka_topic_partition_list_t *partitions);

RD_EXPORT rd_kafka_topic_t *rd_kafka_topic_new(rd_kafka_t *rk,
                                                const char *topic,
                                                rd_kafka_topic_conf_t *conf);
RD_EXPORT void rd_kafka_topic_destroy(rd_kafka_topic_t *rkt);
RD_EXPORT const char *rd_kafka_topic_name(const rd_kafka_topic_t *rkt);
RD_EXPORT rd_kafka_t *rd_kafka_topic_opaque(rd_kafka_topic_t *rkt);

RD_EXPORT int rd_kafka_produce(rd_kafka_topic_t *rkt,
                                int32_t partition,
                                int msgflags,
                                void *payload,
                                size_t len,
                                const void *key,
                                size_t key_len,
                                void *msg_opaque);
RD_EXPORT int rd_kafka_producev(rd_kafka_t *rk, ...);

#define RD_KAFKA_PARTITION_UA -1
#define RD_KAFKA_MSG_F_FREE 0x1
#define RD_KAFKA_MSG_F_COPY 0x2
#define RD_KAFKA_MSG_F_BLOCK 0x4
#define RD_KAFKA_MSG_F_PARTITION 0x8

#define RD_KAFKA_V_TOPIC(topic) RD_KAFKA_VTYPE_TOPIC, (topic)
#define RD_KAFKA_VTOPIC(topic) RD_KAFKA_V_TOPIC(topic)
#define RD_KAFKA_V_PARTITION(partition) RD_KAFKA_VTYPE_PARTITION, (int)(partition)
#define RD_KAFKA_VP(partition) RD_KAFKA_V_PARTITION(partition)
#define RD_KAFKA_V_VALUE(payload, len) RD_KAFKA_VTYPE_VALUE, (void *)(payload), (size_t)(len)
#define RD_KAFKA_V_KEY(key, len) RD_KAFKA_VTYPE_KEY, (const void *)(key), (size_t)(len)
#define RD_KAFKA_V_OPAQUE(opaque) RD_KAFKA_VTYPE_OPAQUE, (opaque)
#define RD_KAFKA_V_MSGFLAGS(flags) RD_KAFKA_VTYPE_MSGFLAGS, (flags)
#define RD_KAFKA_V_TIMESTAMP(timestamp) RD_KAFKA_VTYPE_TIMESTAMP, (int64_t)(timestamp)
#define RD_KAFKA_V_END RD_KAFKA_VTYPE_END

typedef enum {
    RD_KAFKA_VTYPE_END = 0,
    RD_KAFKA_VTYPE_TOPIC = 1,
    RD_KAFKA_VTYPE_RKT = 2,
    RD_KAFKA_VTYPE_PARTITION = 3,
    RD_KAFKA_VTYPE_VALUE = 4,
    RD_KAFKA_VTYPE_KEY = 5,
    RD_KAFKA_VTYPE_OPAQUE = 6,
    RD_KAFKA_VTYPE_MSGFLAGS = 7,
    RD_KAFKA_VTYPE_TIMESTAMP = 8,
} rd_kafka_vtype_t;

RD_EXPORT rd_kafka_resp_err_t rd_kafka_flush(rd_kafka_t *rk, int timeout_ms);

RD_EXPORT rd_kafka_topic_partition_list_t *rd_kafka_topic_partition_list_new(int size);
RD_EXPORT void rd_kafka_topic_partition_list_destroy(rd_kafka_topic_partition_list_t *rkparlist);
RD_EXPORT void rd_kafka_topic_partition_list_add(rd_kafka_topic_partition_list_t *rktparlist,
                                                  const char *topic,
                                                  int32_t partition);
RD_EXPORT rd_kafka_topic_partition_t *rd_kafka_topic_partition_list_add_range(
    rd_kafka_topic_partition_list_t *rktparlist,
    const char *topic,
    int32_t start,
    int32_t stop);
RD_EXPORT void rd_kafka_topic_partition_list_del(rd_kafka_topic_partition_list_t *rktparlist,
                                                  const char *topic,
                                                  int32_t partition);
RD_EXPORT void rd_kafka_topic_partition_list_del_by_idx(
    rd_kafka_topic_partition_list_t *rktparlist,
    int idx);
RD_EXPORT int rd_kafka_topic_partition_list_set_offset(
    rd_kafka_topic_partition_list_t *rktparlist,
    const char *topic,
    int32_t partition,
    int64_t offset);
RD_EXPORT rd_kafka_topic_partition_t *rd_kafka_topic_partition_list_find(
    rd_kafka_topic_partition_list_t *rktparlist,
    const char *topic,
    int32_t partition);

RD_EXPORT rd_kafka_resp_err_t rd_kafka_subscribe(rd_kafka_t *rk,
                                                  rd_kafka_topic_partition_list_t *topics);
RD_EXPORT rd_kafka_resp_err_t rd_kafka_unsubscribe(rd_kafka_t *rk);
RD_EXPORT rd_kafka_topic_partition_list_t *rd_kafka_subscription(rd_kafka_t *rk);
RD_EXPORT rd_kafka_message_t *rd_kafka_consumer_poll(rd_kafka_t *rk, int timeout_ms);
RD_EXPORT rd_kafka_resp_err_t rd_kafka_consumer_close(rd_kafka_t *rk);
RD_EXPORT void rd_kafka_message_destroy(rd_kafka_message_t *rkmessage);
RD_EXPORT rd_kafka_resp_err_t rd_kafka_offset_store(rd_kafka_topic_t *rkt,
                                                     int32_t partition,
                                                     int64_t offset);
RD_EXPORT rd_kafka_resp_err_t rd_kafka_commit(rd_kafka_t *rk,
                                               const rd_kafka_topic_partition_list_t *offsets,
                                               int async);

#ifdef __cplusplus
}
#endif

#endif /* _RDKAFKA_H_ */
