// defines for message 'assign_channel'
#define ASSIGN_CHANNEL_CHANNEL(message)          ((uint8_t)message[1])
#define ASSIGN_CHANNEL_CHANNEL_TYPE(message)     ((uint8_t)message[2])
#define ASSIGN_CHANNEL_NETWORK_NUMBER(message)   ((uint8_t)message[3])
#define MESSAGE_IS_ASSIGN_CHANNEL(message) ((message[0]==0x42))
#define MESSAGE_LEN_ASSIGN_CHANNEL (4)

// defines for message 'unassign_channel'
#define UNASSIGN_CHANNEL_CHANNEL(message)        ((uint8_t)message[1])
#define MESSAGE_IS_UNASSIGN_CHANNEL(message) ((message[0]==0x41))
#define MESSAGE_LEN_UNASSIGN_CHANNEL (2)

// defines for message 'open_channel'
#define OPEN_CHANNEL_CHANNEL(message)            ((uint8_t)message[1])
#define MESSAGE_IS_OPEN_CHANNEL(message) ((message[0]==0x4B))
#define MESSAGE_LEN_OPEN_CHANNEL (2)

// defines for message 'channel_id'
#define CHANNEL_ID_CHANNEL(message)              ((uint8_t)message[1])
#define CHANNEL_ID_DEVICE_NUMBER(message)        ((uint16_t)(message[2]+(message[3]<<8)))
#define CHANNEL_ID_DEVICE_TYPE_ID(message)       ((uint8_t)message[4])
#define CHANNEL_ID_TRANSMISSION_TYPE(message)    ((uint8_t)message[5])
#define MESSAGE_IS_CHANNEL_ID(message) ((message[0]==0x51))
#define MESSAGE_LEN_CHANNEL_ID (6)

// defines for message 'burst_message'
#define BURST_MESSAGE_CHAN_SEQ(message)          ((uint8_t)message[1])
#define BURST_MESSAGE_DATA0(message)             ((uint8_t)message[2])
#define BURST_MESSAGE_DATA1(message)             ((uint8_t)message[3])
#define BURST_MESSAGE_DATA2(message)             ((uint8_t)message[4])
#define BURST_MESSAGE_DATA3(message)             ((uint8_t)message[5])
#define BURST_MESSAGE_DATA4(message)             ((uint8_t)message[6])
#define BURST_MESSAGE_DATA5(message)             ((uint8_t)message[7])
#define BURST_MESSAGE_DATA6(message)             ((uint8_t)message[8])
#define BURST_MESSAGE_DATA7(message)             ((uint8_t)message[9])
#define MESSAGE_IS_BURST_MESSAGE(message) ((message[0]==0x50))
#define MESSAGE_LEN_BURST_MESSAGE (10)

// defines for message 'burst_message7'
#define BURST_MESSAGE7_CHAN_SEQ(message)         ((uint8_t)message[1])
#define BURST_MESSAGE7_DATA0(message)            ((uint8_t)message[2])
#define BURST_MESSAGE7_DATA1(message)            ((uint8_t)message[3])
#define BURST_MESSAGE7_DATA2(message)            ((uint8_t)message[4])
#define BURST_MESSAGE7_DATA3(message)            ((uint8_t)message[5])
#define BURST_MESSAGE7_DATA4(message)            ((uint8_t)message[6])
#define BURST_MESSAGE7_DATA5(message)            ((uint8_t)message[7])
#define BURST_MESSAGE7_DATA6(message)            ((uint8_t)message[8])
#define MESSAGE_IS_BURST_MESSAGE7(message) ((message[0]==0x50))
#define MESSAGE_LEN_BURST_MESSAGE7 (9)

// defines for message 'burst_message6'
#define BURST_MESSAGE6_CHAN_SEQ(message)         ((uint8_t)message[1])
#define BURST_MESSAGE6_DATA0(message)            ((uint8_t)message[2])
#define BURST_MESSAGE6_DATA1(message)            ((uint8_t)message[3])
#define BURST_MESSAGE6_DATA2(message)            ((uint8_t)message[4])
#define BURST_MESSAGE6_DATA3(message)            ((uint8_t)message[5])
#define BURST_MESSAGE6_DATA4(message)            ((uint8_t)message[6])
#define BURST_MESSAGE6_DATA5(message)            ((uint8_t)message[7])
#define MESSAGE_IS_BURST_MESSAGE6(message) ((message[0]==0x50))
#define MESSAGE_LEN_BURST_MESSAGE6 (8)

// defines for message 'burst_message5'
#define BURST_MESSAGE5_CHAN_SEQ(message)         ((uint8_t)message[1])
#define BURST_MESSAGE5_DATA0(message)            ((uint8_t)message[2])
#define BURST_MESSAGE5_DATA1(message)            ((uint8_t)message[3])
#define BURST_MESSAGE5_DATA2(message)            ((uint8_t)message[4])
#define BURST_MESSAGE5_DATA3(message)            ((uint8_t)message[5])
#define BURST_MESSAGE5_DATA4(message)            ((uint8_t)message[6])
#define MESSAGE_IS_BURST_MESSAGE5(message) ((message[0]==0x50))
#define MESSAGE_LEN_BURST_MESSAGE5 (7)

// defines for message 'burst_message4'
#define BURST_MESSAGE4_CHAN_SEQ(message)         ((uint8_t)message[1])
#define BURST_MESSAGE4_DATA0(message)            ((uint8_t)message[2])
#define BURST_MESSAGE4_DATA1(message)            ((uint8_t)message[3])
#define BURST_MESSAGE4_DATA2(message)            ((uint8_t)message[4])
#define BURST_MESSAGE4_DATA3(message)            ((uint8_t)message[5])
#define MESSAGE_IS_BURST_MESSAGE4(message) ((message[0]==0x50))
#define MESSAGE_LEN_BURST_MESSAGE4 (6)

// defines for message 'burst_message3'
#define BURST_MESSAGE3_CHAN_SEQ(message)         ((uint8_t)message[1])
#define BURST_MESSAGE3_DATA0(message)            ((uint8_t)message[2])
#define BURST_MESSAGE3_DATA1(message)            ((uint8_t)message[3])
#define BURST_MESSAGE3_DATA2(message)            ((uint8_t)message[4])
#define MESSAGE_IS_BURST_MESSAGE3(message) ((message[0]==0x50))
#define MESSAGE_LEN_BURST_MESSAGE3 (5)

// defines for message 'burst_message2'
#define BURST_MESSAGE2_CHAN_SEQ(message)         ((uint8_t)message[1])
#define BURST_MESSAGE2_DATA0(message)            ((uint8_t)message[2])
#define BURST_MESSAGE2_DATA1(message)            ((uint8_t)message[3])
#define MESSAGE_IS_BURST_MESSAGE2(message) ((message[0]==0x50))
#define MESSAGE_LEN_BURST_MESSAGE2 (4)

// defines for message 'burst_message1'
#define BURST_MESSAGE1_CHAN_SEQ(message)         ((uint8_t)message[1])
#define BURST_MESSAGE1_DATA0(message)            ((uint8_t)message[2])
#define MESSAGE_IS_BURST_MESSAGE1(message) ((message[0]==0x50))
#define MESSAGE_LEN_BURST_MESSAGE1 (3)

// defines for message 'burst_message0'
#define BURST_MESSAGE0_CHAN_SEQ(message)         ((uint8_t)message[1])
#define MESSAGE_IS_BURST_MESSAGE0(message) ((message[0]==0x50))
#define MESSAGE_LEN_BURST_MESSAGE0 (2)

// defines for message 'channel_period'
#define CHANNEL_PERIOD_CHANNEL(message)          ((uint8_t)message[1])
#define CHANNEL_PERIOD_PERIOD(message)           ((uint16_t)(message[2]+(message[3]<<8)))
#define MESSAGE_IS_CHANNEL_PERIOD(message) ((message[0]==0x43))
#define MESSAGE_LEN_CHANNEL_PERIOD (4)

// defines for message 'search_timeout'
#define SEARCH_TIMEOUT_CHANNEL(message)          ((uint8_t)message[1])
#define SEARCH_TIMEOUT_SEARCH_TIMEOUT(message)   ((uint8_t)message[2])
#define MESSAGE_IS_SEARCH_TIMEOUT(message) ((message[0]==0x44))
#define MESSAGE_LEN_SEARCH_TIMEOUT (3)

// defines for message 'channel_frequency'
#define CHANNEL_FREQUENCY_CHANNEL(message)       ((uint8_t)message[1])
#define CHANNEL_FREQUENCY_RF_FREQUENCY(message)  ((uint8_t)message[2])
#define MESSAGE_IS_CHANNEL_FREQUENCY(message) ((message[0]==0x45))
#define MESSAGE_LEN_CHANNEL_FREQUENCY (3)

// defines for message 'set_network'
#define SET_NETWORK_CHANNEL(message)             ((uint8_t)message[1])
#define SET_NETWORK_KEY0(message)                ((uint8_t)message[2])
#define SET_NETWORK_KEY1(message)                ((uint8_t)message[3])
#define SET_NETWORK_KEY2(message)                ((uint8_t)message[4])
#define SET_NETWORK_KEY3(message)                ((uint8_t)message[5])
#define SET_NETWORK_KEY4(message)                ((uint8_t)message[6])
#define SET_NETWORK_KEY5(message)                ((uint8_t)message[7])
#define SET_NETWORK_KEY6(message)                ((uint8_t)message[8])
#define SET_NETWORK_KEY7(message)                ((uint8_t)message[9])
#define MESSAGE_IS_SET_NETWORK(message) ((message[0]==0x46))
#define MESSAGE_LEN_SET_NETWORK (10)

// defines for message 'transmit_power'
#define TRANSMIT_POWER_TX_POWER(message)         ((uint8_t)message[2])
#define MESSAGE_IS_TRANSMIT_POWER(message) ((message[0]==0x47) && (message[1]==0x00))
#define MESSAGE_LEN_TRANSMIT_POWER (3)

// defines for message 'reset_system'
#define MESSAGE_IS_RESET_SYSTEM(message) ((message[0]==0x4A))
#define MESSAGE_LEN_RESET_SYSTEM (2)

// defines for message 'request_message'
#define REQUEST_MESSAGE_CHANNEL(message)         ((uint8_t)message[1])
#define REQUEST_MESSAGE_MESSAGE_REQUESTED(message) ((uint8_t)message[2])
#define MESSAGE_IS_REQUEST_MESSAGE(message) ((message[0]==0x4D))
#define MESSAGE_LEN_REQUEST_MESSAGE (3)

// defines for message 'close_channel'
#define CLOSE_CHANNEL_CHANNEL(message)           ((uint8_t)message[1])
#define MESSAGE_IS_CLOSE_CHANNEL(message) ((message[0]==0x4C))
#define MESSAGE_LEN_CLOSE_CHANNEL (2)

// defines for message 'response_no_error'
#define RESPONSE_NO_ERROR_CHANNEL(message)       ((uint8_t)message[1])
#define RESPONSE_NO_ERROR_MESSAGE_ID(message)    ((uint8_t)message[2])
#define MESSAGE_IS_RESPONSE_NO_ERROR(message) ((message[0]==0x40) && (message[3]==0x00))
#define MESSAGE_LEN_RESPONSE_NO_ERROR (4)

// defines for message 'response_assign_channel_ok'
#define RESPONSE_ASSIGN_CHANNEL_OK_CHANNEL(message) ((uint8_t)message[1])
#define MESSAGE_IS_RESPONSE_ASSIGN_CHANNEL_OK(message) ((message[0]==0x40) && (message[2]==0x42) && (message[3]==0x00))
#define MESSAGE_LEN_RESPONSE_ASSIGN_CHANNEL_OK (4)

// defines for message 'response_channel_unassign_ok'
#define RESPONSE_CHANNEL_UNASSIGN_OK_CHANNEL(message) ((uint8_t)message[1])
#define MESSAGE_IS_RESPONSE_CHANNEL_UNASSIGN_OK(message) ((message[0]==0x40) && (message[2]==0x41) && (message[3]==0x00))
#define MESSAGE_LEN_RESPONSE_CHANNEL_UNASSIGN_OK (4)

// defines for message 'response_open_channel_ok'
#define RESPONSE_OPEN_CHANNEL_OK_CHANNEL(message) ((uint8_t)message[1])
#define MESSAGE_IS_RESPONSE_OPEN_CHANNEL_OK(message) ((message[0]==0x40) && (message[2]==0x4B) && (message[3]==0x00))
#define MESSAGE_LEN_RESPONSE_OPEN_CHANNEL_OK (4)

// defines for message 'response_channel_id_ok'
#define RESPONSE_CHANNEL_ID_OK_CHANNEL(message)  ((uint8_t)message[1])
#define MESSAGE_IS_RESPONSE_CHANNEL_ID_OK(message) ((message[0]==0x40) && (message[2]==0x51) && (message[3]==0x00))
#define MESSAGE_LEN_RESPONSE_CHANNEL_ID_OK (4)

// defines for message 'response_channel_period_ok'
#define RESPONSE_CHANNEL_PERIOD_OK_CHANNEL(message) ((uint8_t)message[1])
#define MESSAGE_IS_RESPONSE_CHANNEL_PERIOD_OK(message) ((message[0]==0x40) && (message[2]==0x43) && (message[3]==0x00))
#define MESSAGE_LEN_RESPONSE_CHANNEL_PERIOD_OK (4)

// defines for message 'response_channel_frequency_ok'
#define RESPONSE_CHANNEL_FREQUENCY_OK_CHANNEL(message) ((uint8_t)message[1])
#define MESSAGE_IS_RESPONSE_CHANNEL_FREQUENCY_OK(message) ((message[0]==0x40) && (message[2]==0x45) && (message[3]==0x00))
#define MESSAGE_LEN_RESPONSE_CHANNEL_FREQUENCY_OK (4)

// defines for message 'response_set_network_ok'
#define RESPONSE_SET_NETWORK_OK_CHANNEL(message) ((uint8_t)message[1])
#define MESSAGE_IS_RESPONSE_SET_NETWORK_OK(message) ((message[0]==0x40) && (message[2]==0x46) && (message[3]==0x00))
#define MESSAGE_LEN_RESPONSE_SET_NETWORK_OK (4)

// defines for message 'response_transmit_power_ok'
#define RESPONSE_TRANSMIT_POWER_OK_CHANNEL(message) ((uint8_t)message[1])
#define MESSAGE_IS_RESPONSE_TRANSMIT_POWER_OK(message) ((message[0]==0x40) && (message[2]==0x47) && (message[3]==0x00))
#define MESSAGE_LEN_RESPONSE_TRANSMIT_POWER_OK (4)

// defines for message 'response_close_channel_ok'
#define RESPONSE_CLOSE_CHANNEL_OK_CHANNEL(message) ((uint8_t)message[1])
#define MESSAGE_IS_RESPONSE_CLOSE_CHANNEL_OK(message) ((message[0]==0x40) && (message[2]==0x4C) && (message[3]==0x00))
#define MESSAGE_LEN_RESPONSE_CLOSE_CHANNEL_OK (4)

// defines for message 'response_search_timeout_ok'
#define RESPONSE_SEARCH_TIMEOUT_OK_CHANNEL(message) ((uint8_t)message[1])
#define MESSAGE_IS_RESPONSE_SEARCH_TIMEOUT_OK(message) ((message[0]==0x40) && (message[2]==0x44) && (message[3]==0x00))
#define MESSAGE_LEN_RESPONSE_SEARCH_TIMEOUT_OK (4)

// defines for message 'event_rx_search_timeout'
#define EVENT_RX_SEARCH_TIMEOUT_CHANNEL(message) ((uint8_t)message[1])
#define EVENT_RX_SEARCH_TIMEOUT_MESSAGE_ID(message) ((uint8_t)message[2])
#define MESSAGE_IS_EVENT_RX_SEARCH_TIMEOUT(message) ((message[0]==0x40) && (message[3]==0x01))
#define MESSAGE_LEN_EVENT_RX_SEARCH_TIMEOUT (4)

// defines for message 'event_rx_fail'
#define EVENT_RX_FAIL_CHANNEL(message)           ((uint8_t)message[1])
#define EVENT_RX_FAIL_MESSAGE_ID(message)        ((uint8_t)message[2])
#define MESSAGE_IS_EVENT_RX_FAIL(message) ((message[0]==0x40) && (message[3]==0x02))
#define MESSAGE_LEN_EVENT_RX_FAIL (4)

// defines for message 'event_tx'
#define EVENT_TX_CHANNEL(message)                ((uint8_t)message[1])
#define EVENT_TX_MESSAGE_ID(message)             ((uint8_t)message[2])
#define MESSAGE_IS_EVENT_TX(message) ((message[0]==0x40) && (message[3]==0x03))
#define MESSAGE_LEN_EVENT_TX (4)

// defines for message 'event_transfer_rx_failed'
#define EVENT_TRANSFER_RX_FAILED_CHANNEL(message) ((uint8_t)message[1])
#define EVENT_TRANSFER_RX_FAILED_MESSAGE_ID(message) ((uint8_t)message[2])
#define MESSAGE_IS_EVENT_TRANSFER_RX_FAILED(message) ((message[0]==0x40) && (message[3]==0x04))
#define MESSAGE_LEN_EVENT_TRANSFER_RX_FAILED (4)

// defines for message 'event_transfer_tx_completed'
#define EVENT_TRANSFER_TX_COMPLETED_CHANNEL(message) ((uint8_t)message[1])
#define EVENT_TRANSFER_TX_COMPLETED_MESSAGE_ID(message) ((uint8_t)message[2])
#define MESSAGE_IS_EVENT_TRANSFER_TX_COMPLETED(message) ((message[0]==0x40) && (message[3]==0x05))
#define MESSAGE_LEN_EVENT_TRANSFER_TX_COMPLETED (4)

// defines for message 'event_transfer_tx_failed'
#define EVENT_TRANSFER_TX_FAILED_CHANNEL(message) ((uint8_t)message[1])
#define EVENT_TRANSFER_TX_FAILED_MESSAGE_ID(message) ((uint8_t)message[2])
#define MESSAGE_IS_EVENT_TRANSFER_TX_FAILED(message) ((message[0]==0x40) && (message[3]==0x06))
#define MESSAGE_LEN_EVENT_TRANSFER_TX_FAILED (4)

// defines for message 'event_channel_closed'
#define EVENT_CHANNEL_CLOSED_CHANNEL(message)    ((uint8_t)message[1])
#define EVENT_CHANNEL_CLOSED_MESSAGE_ID(message) ((uint8_t)message[2])
#define MESSAGE_IS_EVENT_CHANNEL_CLOSED(message) ((message[0]==0x40) && (message[3]==0x07))
#define MESSAGE_LEN_EVENT_CHANNEL_CLOSED (4)

// defines for message 'event_rx_fail_go_to_search'
#define EVENT_RX_FAIL_GO_TO_SEARCH_CHANNEL(message) ((uint8_t)message[1])
#define EVENT_RX_FAIL_GO_TO_SEARCH_MESSAGE_ID(message) ((uint8_t)message[2])
#define MESSAGE_IS_EVENT_RX_FAIL_GO_TO_SEARCH(message) ((message[0]==0x40) && (message[3]==0x08))
#define MESSAGE_LEN_EVENT_RX_FAIL_GO_TO_SEARCH (4)

// defines for message 'event_channel_collision'
#define EVENT_CHANNEL_COLLISION_CHANNEL(message) ((uint8_t)message[1])
#define EVENT_CHANNEL_COLLISION_MESSAGE_ID(message) ((uint8_t)message[2])
#define MESSAGE_IS_EVENT_CHANNEL_COLLISION(message) ((message[0]==0x40) && (message[3]==0x09))
#define MESSAGE_LEN_EVENT_CHANNEL_COLLISION (4)

// defines for message 'event_transfer_tx_start'
#define EVENT_TRANSFER_TX_START_CHANNEL(message) ((uint8_t)message[1])
#define EVENT_TRANSFER_TX_START_MESSAGE_ID(message) ((uint8_t)message[2])
#define MESSAGE_IS_EVENT_TRANSFER_TX_START(message) ((message[0]==0x40) && (message[3]==0x0A))
#define MESSAGE_LEN_EVENT_TRANSFER_TX_START (4)

// defines for message 'event_rx_acknowledged'
#define EVENT_RX_ACKNOWLEDGED_CHANNEL(message)   ((uint8_t)message[1])
#define EVENT_RX_ACKNOWLEDGED_MESSAGE_ID(message) ((uint8_t)message[2])
#define MESSAGE_IS_EVENT_RX_ACKNOWLEDGED(message) ((message[0]==0x40) && (message[3]==0x0B))
#define MESSAGE_LEN_EVENT_RX_ACKNOWLEDGED (4)

// defines for message 'event_rx_burst_packet'
#define EVENT_RX_BURST_PACKET_CHANNEL(message)   ((uint8_t)message[1])
#define EVENT_RX_BURST_PACKET_MESSAGE_ID(message) ((uint8_t)message[2])
#define MESSAGE_IS_EVENT_RX_BURST_PACKET(message) ((message[0]==0x40) && (message[3]==0x0C))
#define MESSAGE_LEN_EVENT_RX_BURST_PACKET (4)

// defines for message 'channel_in_wrong_state'
#define CHANNEL_IN_WRONG_STATE_CHANNEL(message)  ((uint8_t)message[1])
#define CHANNEL_IN_WRONG_STATE_MESSAGE_ID(message) ((uint8_t)message[2])
#define MESSAGE_IS_CHANNEL_IN_WRONG_STATE(message) ((message[0]==0x40) && (message[3]==0x15))
#define MESSAGE_LEN_CHANNEL_IN_WRONG_STATE (4)

// defines for message 'channel_not_opened'
#define CHANNEL_NOT_OPENED_CHANNEL(message)      ((uint8_t)message[1])
#define CHANNEL_NOT_OPENED_MESSAGE_ID(message)   ((uint8_t)message[2])
#define MESSAGE_IS_CHANNEL_NOT_OPENED(message) ((message[0]==0x40) && (message[3]==0x16))
#define MESSAGE_LEN_CHANNEL_NOT_OPENED (4)

// defines for message 'channel_id_not_set'
#define CHANNEL_ID_NOT_SET_CHANNEL(message)      ((uint8_t)message[1])
#define CHANNEL_ID_NOT_SET_MESSAGE_ID(message)   ((uint8_t)message[2])
#define MESSAGE_IS_CHANNEL_ID_NOT_SET(message) ((message[0]==0x40) && (message[3]==0x18))
#define MESSAGE_LEN_CHANNEL_ID_NOT_SET (4)

// defines for message 'transfer_in_progress'
#define TRANSFER_IN_PROGRESS_CHANNEL(message)    ((uint8_t)message[1])
#define TRANSFER_IN_PROGRESS_MESSAGE_ID(message) ((uint8_t)message[2])
#define MESSAGE_IS_TRANSFER_IN_PROGRESS(message) ((message[0]==0x40) && (message[3]==0x1F))
#define MESSAGE_LEN_TRANSFER_IN_PROGRESS (4)

// defines for message 'channel_status'
#define CHANNEL_STATUS_CHANNEL(message)          ((uint8_t)message[1])
#define CHANNEL_STATUS_STATUS(message)           ((uint8_t)message[2])
#define MESSAGE_IS_CHANNEL_STATUS(message) ((message[0]==0x52))
#define MESSAGE_LEN_CHANNEL_STATUS (3)

// defines for message 'cw_init'
#define MESSAGE_IS_CW_INIT(message) ((message[0]==0x53))
#define MESSAGE_LEN_CW_INIT (2)

// defines for message 'cw_test'
#define CW_TEST_POWER(message)                   ((uint8_t)message[2])
#define CW_TEST_FREQ(message)                    ((uint8_t)message[3])
#define MESSAGE_IS_CW_TEST(message) ((message[0]==0x48))
#define MESSAGE_LEN_CW_TEST (4)

// defines for message 'capabilities'
#define CAPABILITIES_MAX_CHANNELS(message)       ((uint8_t)message[1])
#define CAPABILITIES_MAX_NETWORKS(message)       ((uint8_t)message[2])
#define CAPABILITIES_STANDARD_OPTIONS(message)   ((uint8_t)message[3])
#define CAPABILITIES_ADVANCED_OPTIONS(message)   ((uint8_t)message[4])
#define MESSAGE_IS_CAPABILITIES(message) ((message[0]==0x54))
#define MESSAGE_LEN_CAPABILITIES (5)

// defines for message 'capabilities_extended'
#define CAPABILITIES_EXTENDED_MAX_CHANNELS(message) ((uint8_t)message[1])
#define CAPABILITIES_EXTENDED_MAX_NETWORKS(message) ((uint8_t)message[2])
#define CAPABILITIES_EXTENDED_STANDARD_OPTIONS(message) ((uint8_t)message[3])
#define CAPABILITIES_EXTENDED_ADVANCED_OPTIONS(message) ((uint8_t)message[4])
#define CAPABILITIES_EXTENDED_ADVANCED_OPTIONS_2(message) ((uint8_t)message[5])
#define CAPABILITIES_EXTENDED_MAX_DATA_CHANNELS(message) ((uint8_t)message[6])
#define MESSAGE_IS_CAPABILITIES_EXTENDED(message) ((message[0]==0x54))
#define MESSAGE_LEN_CAPABILITIES_EXTENDED (7)

// defines for message 'ant_version'
#define ANT_VERSION_DATA0(message)               ((uint8_t)message[1])
#define ANT_VERSION_DATA1(message)               ((uint8_t)message[2])
#define ANT_VERSION_DATA2(message)               ((uint8_t)message[3])
#define ANT_VERSION_DATA3(message)               ((uint8_t)message[4])
#define ANT_VERSION_DATA4(message)               ((uint8_t)message[5])
#define ANT_VERSION_DATA5(message)               ((uint8_t)message[6])
#define ANT_VERSION_DATA6(message)               ((uint8_t)message[7])
#define ANT_VERSION_DATA7(message)               ((uint8_t)message[8])
#define ANT_VERSION_DATA8(message)               ((uint8_t)message[9])
#define MESSAGE_IS_ANT_VERSION(message) ((message[0]==0x3E))
#define MESSAGE_LEN_ANT_VERSION (10)

// defines for message 'ant_version_long'
#define ANT_VERSION_LONG_DATA0(message)          ((uint8_t)message[1])
#define ANT_VERSION_LONG_DATA1(message)          ((uint8_t)message[2])
#define ANT_VERSION_LONG_DATA2(message)          ((uint8_t)message[3])
#define ANT_VERSION_LONG_DATA3(message)          ((uint8_t)message[4])
#define ANT_VERSION_LONG_DATA4(message)          ((uint8_t)message[5])
#define ANT_VERSION_LONG_DATA5(message)          ((uint8_t)message[6])
#define ANT_VERSION_LONG_DATA6(message)          ((uint8_t)message[7])
#define ANT_VERSION_LONG_DATA7(message)          ((uint8_t)message[8])
#define ANT_VERSION_LONG_DATA8(message)          ((uint8_t)message[9])
#define ANT_VERSION_LONG_DATA9(message)          ((uint8_t)message[10])
#define ANT_VERSION_LONG_DATA10(message)         ((uint8_t)message[11])
#define ANT_VERSION_LONG_DATA11(message)         ((uint8_t)message[12])
#define ANT_VERSION_LONG_DATA12(message)         ((uint8_t)message[13])
#define MESSAGE_IS_ANT_VERSION_LONG(message) ((message[0]==0x3E))
#define MESSAGE_LEN_ANT_VERSION_LONG (14)

// defines for message 'transfer_seq_number_error'
#define TRANSFER_SEQ_NUMBER_ERROR_CHANNEL(message) ((uint8_t)message[1])
#define TRANSFER_SEQ_NUMBER_ERROR_MESSAGE_ID(message) ((uint8_t)message[2])
#define MESSAGE_IS_TRANSFER_SEQ_NUMBER_ERROR(message) ((message[0]==0x40) && (message[3]==0x20))
#define MESSAGE_LEN_TRANSFER_SEQ_NUMBER_ERROR (4)

// defines for message 'invalid_message'
#define INVALID_MESSAGE_CHANNEL(message)         ((uint8_t)message[1])
#define INVALID_MESSAGE_MESSAGE_ID(message)      ((uint8_t)message[2])
#define MESSAGE_IS_INVALID_MESSAGE(message) ((message[0]==0x40) && (message[3]==0x28))
#define MESSAGE_LEN_INVALID_MESSAGE (4)

// defines for message 'invalid_network_number'
#define INVALID_NETWORK_NUMBER_CHANNEL(message)  ((uint8_t)message[1])
#define INVALID_NETWORK_NUMBER_MESSAGE_ID(message) ((uint8_t)message[2])
#define MESSAGE_IS_INVALID_NETWORK_NUMBER(message) ((message[0]==0x40) && (message[3]==0x29))
#define MESSAGE_LEN_INVALID_NETWORK_NUMBER (4)

// defines for message 'channel_response'
#define CHANNEL_RESPONSE_CHANNEL(message)        ((uint8_t)message[1])
#define CHANNEL_RESPONSE_MESSAGE_ID(message)     ((uint8_t)message[2])
#define CHANNEL_RESPONSE_MESSAGE_CODE(message)   ((uint8_t)message[3])
#define MESSAGE_IS_CHANNEL_RESPONSE(message) ((message[0]==0x40))
#define MESSAGE_LEN_CHANNEL_RESPONSE (4)

// defines for message 'extended_broadcast_data'
#define EXTENDED_BROADCAST_DATA_CHANNEL(message) ((uint8_t)message[1])
#define EXTENDED_BROADCAST_DATA_DEVICE_NUMBER(message) ((uint16_t)(message[2]+(message[3]<<8)))
#define EXTENDED_BROADCAST_DATA_DEVICE_TYPE(message) ((uint8_t)message[4])
#define EXTENDED_BROADCAST_DATA_TRANSMISSION_TYPE(message) ((uint8_t)message[5])
#define EXTENDED_BROADCAST_DATA_DATA0(message)   ((uint8_t)message[6])
#define EXTENDED_BROADCAST_DATA_DATA1(message)   ((uint8_t)message[7])
#define EXTENDED_BROADCAST_DATA_DATA2(message)   ((uint8_t)message[8])
#define EXTENDED_BROADCAST_DATA_DATA3(message)   ((uint8_t)message[9])
#define EXTENDED_BROADCAST_DATA_DATA4(message)   ((uint8_t)message[10])
#define EXTENDED_BROADCAST_DATA_DATA5(message)   ((uint8_t)message[11])
#define EXTENDED_BROADCAST_DATA_DATA6(message)   ((uint8_t)message[12])
#define EXTENDED_BROADCAST_DATA_DATA7(message)   ((uint8_t)message[13])
#define MESSAGE_IS_EXTENDED_BROADCAST_DATA(message) ((message[0]==0x5D))
#define MESSAGE_LEN_EXTENDED_BROADCAST_DATA (14)

// defines for message 'extended_ack_data'
#define EXTENDED_ACK_DATA_CHANNEL(message)       ((uint8_t)message[1])
#define EXTENDED_ACK_DATA_DEVICE_NUMBER(message) ((uint16_t)(message[2]+(message[3]<<8)))
#define EXTENDED_ACK_DATA_DEVICE_TYPE(message)   ((uint8_t)message[4])
#define EXTENDED_ACK_DATA_TRANSMISSION_TYPE(message) ((uint8_t)message[5])
#define EXTENDED_ACK_DATA_DATA0(message)         ((uint8_t)message[6])
#define EXTENDED_ACK_DATA_DATA1(message)         ((uint8_t)message[7])
#define EXTENDED_ACK_DATA_DATA2(message)         ((uint8_t)message[8])
#define EXTENDED_ACK_DATA_DATA3(message)         ((uint8_t)message[9])
#define EXTENDED_ACK_DATA_DATA4(message)         ((uint8_t)message[10])
#define EXTENDED_ACK_DATA_DATA5(message)         ((uint8_t)message[11])
#define EXTENDED_ACK_DATA_DATA6(message)         ((uint8_t)message[12])
#define EXTENDED_ACK_DATA_DATA7(message)         ((uint8_t)message[13])
#define MESSAGE_IS_EXTENDED_ACK_DATA(message) ((message[0]==0x5E))
#define MESSAGE_LEN_EXTENDED_ACK_DATA (14)

// defines for message 'extended_burst_data'
#define EXTENDED_BURST_DATA_CHANNEL(message)     ((uint8_t)message[1])
#define EXTENDED_BURST_DATA_DEVICE_NUMBER(message) ((uint16_t)(message[2]+(message[3]<<8)))
#define EXTENDED_BURST_DATA_DEVICE_TYPE(message) ((uint8_t)message[4])
#define EXTENDED_BURST_DATA_TRANSMISSION_TYPE(message) ((uint8_t)message[5])
#define EXTENDED_BURST_DATA_DATA0(message)       ((uint8_t)message[6])
#define EXTENDED_BURST_DATA_DATA1(message)       ((uint8_t)message[7])
#define EXTENDED_BURST_DATA_DATA2(message)       ((uint8_t)message[8])
#define EXTENDED_BURST_DATA_DATA3(message)       ((uint8_t)message[9])
#define EXTENDED_BURST_DATA_DATA4(message)       ((uint8_t)message[10])
#define EXTENDED_BURST_DATA_DATA5(message)       ((uint8_t)message[11])
#define EXTENDED_BURST_DATA_DATA6(message)       ((uint8_t)message[12])
#define EXTENDED_BURST_DATA_DATA7(message)       ((uint8_t)message[13])
#define MESSAGE_IS_EXTENDED_BURST_DATA(message) ((message[0]==0x5F))
#define MESSAGE_LEN_EXTENDED_BURST_DATA (14)

// defines for message 'startup_message'
#define STARTUP_MESSAGE_START_MESSAGE(message)   ((uint8_t)message[1])
#define MESSAGE_IS_STARTUP_MESSAGE(message) ((message[0]==0x6F))
#define MESSAGE_LEN_STARTUP_MESSAGE (2)

// defines for message 'calibration_request'
#define CALIBRATION_REQUEST_CHANNEL(message)     ((uint8_t)message[1])
#define MESSAGE_IS_CALIBRATION_REQUEST(message) ((message[2]==0x01) && (message[3]==0xAA))
#define MESSAGE_LEN_CALIBRATION_REQUEST (10)

// defines for message 'srm_zero_response'
#define SRM_ZERO_RESPONSE_CHANNEL(message)       ((uint8_t)message[1])
#define SRM_ZERO_RESPONSE_OFFSET(message)        ((uint16_t)(message[9]+(message[8]<<8)))
#define MESSAGE_IS_SRM_ZERO_RESPONSE(message) ((message[2]==0x01) && (message[3]==0x10) && (message[4]==0x01))
#define MESSAGE_LEN_SRM_ZERO_RESPONSE (10)

// defines for message 'calibration_pass'
#define CALIBRATION_PASS_CHANNEL(message)        ((uint8_t)message[1])
#define CALIBRATION_PASS_AUTOZERO_STATUS(message) ((uint8_t)message[4])
#define CALIBRATION_PASS_CALIBRATION_DATA(message) ((uint16_t)(message[8]+(message[9]<<8)))
#define MESSAGE_IS_CALIBRATION_PASS(message) ((message[2]==0x01) && (message[3]==0xAC))
#define MESSAGE_LEN_CALIBRATION_PASS (10)

// defines for message 'calibration_fail'
#define CALIBRATION_FAIL_CHANNEL(message)        ((uint8_t)message[1])
#define CALIBRATION_FAIL_AUTOZERO_STATUS(message) ((uint8_t)message[4])
#define CALIBRATION_FAIL_CALIBRATION_DATA(message) ((uint16_t)(message[8]+(message[9]<<8)))
#define MESSAGE_IS_CALIBRATION_FAIL(message) ((message[2]==0x01) && (message[3]==0xAF))
#define MESSAGE_LEN_CALIBRATION_FAIL (10)

// defines for message 'torque_support'
#define TORQUE_SUPPORT_CHANNEL(message)          ((uint8_t)message[1])
#define TORQUE_SUPPORT_SENSOR_CONFIGURATION(message) ((uint8_t)message[4])
#define TORQUE_SUPPORT_RAW_TORQUE(message)       ((int16_t)(message[5]+(message[6]<<8)))
#define TORQUE_SUPPORT_OFFSET_TORQUE(message)    ((int16_t)(message[7]+(message[8]<<8)))
#define MESSAGE_IS_TORQUE_SUPPORT(message) ((message[2]==0x01) && (message[3]==0x12))
#define MESSAGE_LEN_TORQUE_SUPPORT (10)

// defines for message 'standard_power'
#define STANDARD_POWER_CHANNEL(message)          ((uint8_t)message[1])
#define STANDARD_POWER_EVENT_COUNT_SUM(message)  ((uint8_t)message[3])
#define STANDARD_POWER_INSTANT_CADENCE(message)  ((uint8_t)message[5])
#define STANDARD_POWER_SUM_POWER_SUM(message)    ((uint16_t)(message[6]+(message[7]<<8)))
#define STANDARD_POWER_INSTANT_POWER(message)    ((uint16_t)(message[8]+(message[9]<<8)))
#define MESSAGE_IS_STANDARD_POWER(message) ((message[0]==0x4E) && (message[2]==0x10))
#define MESSAGE_LEN_STANDARD_POWER (10)

// defines for message 'wheel_torque'
#define WHEEL_TORQUE_CHANNEL(message)            ((uint8_t)message[1])
#define WHEEL_TORQUE_EVENT_COUNT_SUM(message)    ((uint8_t)message[3])
#define WHEEL_TORQUE_WHEEL_REV(message)          ((uint8_t)message[4])
#define WHEEL_TORQUE_INSTANT_CADENCE(message)    ((uint8_t)message[5])
#define WHEEL_TORQUE_WHEEL_PERIOD_SUM(message)   ((uint16_t)(message[6]+(message[7]<<8)))
#define WHEEL_TORQUE_TORQUE_SUM(message)         ((uint16_t)(message[8]+(message[9]<<8)))
#define MESSAGE_IS_WHEEL_TORQUE(message) ((message[0]==0x4E) && (message[2]==0x11))
#define MESSAGE_LEN_WHEEL_TORQUE (10)

// defines for message 'crank_torque'
#define CRANK_TORQUE_CHANNEL(message)            ((uint8_t)message[1])
#define CRANK_TORQUE_EVENT_COUNT_SUM(message)    ((uint8_t)message[3])
#define CRANK_TORQUE_CRANK_REV(message)          ((uint8_t)message[4])
#define CRANK_TORQUE_INSTANT_CADENCE(message)    ((uint8_t)message[5])
#define CRANK_TORQUE_CRANK_PERIOD_SUM(message)   ((uint16_t)(message[6]+(message[7]<<8)))
#define CRANK_TORQUE_TORQUE_SUM(message)         ((uint16_t)(message[8]+(message[9]<<8)))
#define MESSAGE_IS_CRANK_TORQUE(message) ((message[0]==0x4E) && (message[2]==0x12))
#define MESSAGE_LEN_CRANK_TORQUE (10)

// defines for message 'crank_SRM'
#define CRANK_SRM_CHANNEL(message)               ((uint8_t)message[1])
#define CRANK_SRM_EVENT_COUNT_SUM(message)       ((uint8_t)message[3])
#define CRANK_SRM_SLOPE(message)                 ((uint16_t)(message[5]+(message[4]<<8)))
#define CRANK_SRM_CRANK_PERIOD_SUM(message)      ((uint16_t)(message[7]+(message[6]<<8)))
#define CRANK_SRM_TORQUE_SUM(message)            ((uint16_t)(message[9]+(message[8]<<8)))
#define MESSAGE_IS_CRANK_SRM(message) ((message[0]==0x4E) && (message[2]==0x20))
#define MESSAGE_LEN_CRANK_SRM (10)

// defines for message 'manufacturer'
#define MANUFACTURER_CHANNEL(message)            ((uint8_t)message[1])
#define MANUFACTURER_HW_REV(message)             ((uint8_t)message[5])
#define MANUFACTURER_MANUFACTURER_ID(message)    ((uint16_t)(message[6]+(message[7]<<8)))
#define MANUFACTURER_MODEL_NUMBER_ID(message)    ((uint16_t)(message[8]+(message[9]<<8)))
#define MESSAGE_IS_MANUFACTURER(message) ((message[0]==0x4E) && (message[2]==0x50))
#define MESSAGE_LEN_MANUFACTURER (10)

// defines for message 'product'
#define PRODUCT_CHANNEL(message)                 ((uint8_t)message[1])
#define PRODUCT_SW_REV(message)                  ((uint8_t)message[5])
#define PRODUCT_SERIAL_NUMBER_QPOD(message)      ((uint16_t)(message[6]+(message[7]<<8)))
#define PRODUCT_SERIAL_NUMBER_SPIDER(message)    ((uint16_t)(message[8]+(message[9]<<8)))
#define MESSAGE_IS_PRODUCT(message) ((message[0]==0x4E) && (message[2]==0x51))
#define MESSAGE_LEN_PRODUCT (10)

// defines for message 'battery_voltage'
#define BATTERY_VOLTAGE_CHANNEL(message)         ((uint8_t)message[1])
#define BATTERY_VOLTAGE_OPERATING_TIME_LSB(message) ((uint8_t)message[5])
#define BATTERY_VOLTAGE_OPERATING_TIME_1SB(message) ((uint8_t)message[6])
#define BATTERY_VOLTAGE_OPERATING_TIME_MSB(message) ((uint8_t)message[7])
#define BATTERY_VOLTAGE_VOLTAGE_LSB(message)     ((uint8_t)message[8])
#define BATTERY_VOLTAGE_DESCRIPTIVE_BITS(message) ((uint8_t)message[9])
#define MESSAGE_IS_BATTERY_VOLTAGE(message) ((message[0]==0x4E) && (message[2]==0x52))
#define MESSAGE_LEN_BATTERY_VOLTAGE (10)

// defines for message 'heart_rate'
#define HEART_RATE_CHANNEL(message)              ((uint8_t)message[1])
#define HEART_RATE_MEASUREMENT_TIME_SUM(message) ((uint16_t)(message[6]+(message[7]<<8)))
#define HEART_RATE_BEATS_SUM(message)            ((uint8_t)message[8])
#define HEART_RATE_INSTANT_HEART_RATE(message)   ((uint8_t)message[9])
#define MESSAGE_IS_HEART_RATE(message) ((message[0]==0x4E))
#define MESSAGE_LEN_HEART_RATE (10)

// defines for message 'speed'
#define SPEED_CHANNEL(message)                   ((uint8_t)message[1])
#define SPEED_MEASUREMENT_TIME_SUM(message)      ((uint16_t)(message[6]+(message[7]<<8)))
#define SPEED_WHEEL_REVS_SUM(message)            ((uint16_t)(message[8]+(message[9]<<8)))
#define MESSAGE_IS_SPEED(message) ((message[0]==0x4E))
#define MESSAGE_LEN_SPEED (10)

// defines for message 'cadence'
#define CADENCE_CHANNEL(message)                 ((uint8_t)message[1])
#define CADENCE_MEASUREMENT_TIME_SUM(message)    ((uint16_t)(message[6]+(message[7]<<8)))
#define CADENCE_CRANK_REVS_SUM(message)          ((uint16_t)(message[8]+(message[9]<<8)))
#define MESSAGE_IS_CADENCE(message) ((message[0]==0x4E))
#define MESSAGE_LEN_CADENCE (10)

// defines for message 'speed_cadence'
#define SPEED_CADENCE_CHANNEL(message)           ((uint8_t)message[1])
#define SPEED_CADENCE_CADENCE_MEASUREMENT_TIME_SUM(message) ((uint16_t)(message[2]+(message[3]<<8)))
#define SPEED_CADENCE_CRANK_REVS_SUM(message)    ((uint16_t)(message[4]+(message[5]<<8)))
#define SPEED_CADENCE_SPEED_MEASUREMENT_TIME_SUM(message) ((uint16_t)(message[6]+(message[7]<<8)))
#define SPEED_CADENCE_WHEEL_REVS_SUM(message)    ((uint16_t)(message[8]+(message[9]<<8)))
#define MESSAGE_IS_SPEED_CADENCE(message) ((message[0]==0x4E))
#define MESSAGE_LEN_SPEED_CADENCE (10)

#define STANDARD_POWER_EVENT_COUNT(message) ((uint8_t)(STANDARD_POWER_EVENT_COUNT_SUM(message)-STANDARD_POWER_EVENT_COUNT_SUM(last_message)))
#define STANDARD_POWER_SUM_POWER(message) ((uint16_t)(STANDARD_POWER_SUM_POWER_SUM(message)-STANDARD_POWER_SUM_POWER_SUM(last_message)))

#define WHEEL_TORQUE_EVENT_COUNT(message) ((uint8_t)(WHEEL_TORQUE_EVENT_COUNT_SUM(message)-WHEEL_TORQUE_EVENT_COUNT_SUM(last_message)))
#define WHEEL_TORQUE_WHEEL_PERIOD(message) ((uint16_t)(WHEEL_TORQUE_WHEEL_PERIOD_SUM(message)-WHEEL_TORQUE_WHEEL_PERIOD_SUM(last_message)))
#define WHEEL_TORQUE_TORQUE(message) ((uint16_t)(WHEEL_TORQUE_TORQUE_SUM(message)-WHEEL_TORQUE_TORQUE_SUM(last_message)))
#define WHEEL_TORQUE_NM_TORQUE(message) ((float)(WHEEL_TORQUE_TORQUE(message)/(32.0*WHEEL_TORQUE_EVENT_COUNT(message))))
#define WHEEL_TORQUE_WHEEL_RPM(message) ((float)(2048.0*60*WHEEL_TORQUE_EVENT_COUNT(message)/WHEEL_TORQUE_WHEEL_PERIOD(message)))
#define WHEEL_TORQUE_POWER(message) ((float)(3.14159*WHEEL_TORQUE_NM_TORQUE(message)*WHEEL_TORQUE_WHEEL_RPM(message)/30))
#define WHEEL_TORQUE_TIMEDIFF(message) ((float)(WHEEL_TORQUE_WHEEL_PERIOD(message)/2048.0))

#define CRANK_TORQUE_EVENT_COUNT(message) ((uint8_t)(CRANK_TORQUE_EVENT_COUNT_SUM(message)-CRANK_TORQUE_EVENT_COUNT_SUM(last_message)))
#define CRANK_TORQUE_CRANK_PERIOD(message) ((uint16_t)(CRANK_TORQUE_CRANK_PERIOD_SUM(message)-CRANK_TORQUE_CRANK_PERIOD_SUM(last_message)))
#define CRANK_TORQUE_TORQUE(message) ((uint16_t)(CRANK_TORQUE_TORQUE_SUM(message)-CRANK_TORQUE_TORQUE_SUM(last_message)))
#define CRANK_TORQUE_NM_TORQUE(message) ((float)(CRANK_TORQUE_TORQUE(message)/(32.0*CRANK_TORQUE_EVENT_COUNT(message))))
#define CRANK_TORQUE_CADENCE(message) ((float)(2048.0*60*CRANK_TORQUE_EVENT_COUNT(message)/CRANK_TORQUE_CRANK_PERIOD(message)))
#define CRANK_TORQUE_POWER(message) ((float)(3.14159*CRANK_TORQUE_NM_TORQUE(message)*CRANK_TORQUE_CADENCE(message)/30))
#define CRANK_TORQUE_TIMEDIFF(message) ((float)(CRANK_TORQUE_CRANK_PERIOD(message)/2048.0))

#define CRANK_SRM_EVENT_COUNT(message) ((uint8_t)(CRANK_SRM_EVENT_COUNT_SUM(message)-CRANK_SRM_EVENT_COUNT_SUM(last_message)))
#define CRANK_SRM_CRANK_PERIOD(message) ((uint16_t)(CRANK_SRM_CRANK_PERIOD_SUM(message)-CRANK_SRM_CRANK_PERIOD_SUM(last_message)))
#define CRANK_SRM_TORQUE(message) ((uint16_t)(CRANK_SRM_TORQUE_SUM(message)-CRANK_SRM_TORQUE_SUM(last_message)))
#define CRANK_SRM_ELAPSED_TIME(message) ((float)((CRANK_SRM_CRANK_PERIOD(message)/2000.0)))
#define CRANK_SRM_TORQUE_FREQUENCY(message) ((float)(CRANK_SRM_TORQUE(message)/CRANK_SRM_ELAPSED_TIME(message)-CRANK_SRM_OFFSET(message)))
#define CRANK_SRM_NM_TORQUE(message) ((float)(10.0*CRANK_SRM_TORQUE_FREQUENCY(message)/CRANK_SRM_SLOPE(message)))
#define CRANK_SRM_CADENCE(message) ((float)(2000.0*60*CRANK_SRM_EVENT_COUNT(message)/CRANK_SRM_CRANK_PERIOD(message)))
#define CRANK_SRM_POWER(message) ((float)(3.14159*CRANK_SRM_NM_TORQUE(message)*CRANK_SRM_CADENCE(message)/30))
#define CRANK_SRM_OFFSET(message) (get_srm_offset(self->device_id))

#define PRODUCT_SERIAL_NUMBER(message) ((uint32_t)((PRODUCT_SERIAL_NUMBER_QPOD(message))|(PRODUCT_SERIAL_NUMBER_SPIDER(message)<<16)))

#define BATTERY_VOLTAGE_VOLTAGE(message) ((float)((BATTERY_VOLTAGE_DESCRIPTIVE_BITS(message)&0x0F)+BATTERY_VOLTAGE_VOLTAGE_LSB(message)/256.0))

#define HEART_RATE_MEASUREMENT_TIME(message) ((uint16_t)(HEART_RATE_MEASUREMENT_TIME_SUM(message)-HEART_RATE_MEASUREMENT_TIME_SUM(last_message)))
#define HEART_RATE_BEATS(message) ((uint8_t)(HEART_RATE_BEATS_SUM(message)-HEART_RATE_BEATS_SUM(last_message)))
#define HEART_RATE_TIMEDIFF(message) ((float)(HEART_RATE_MEASUREMENT_TIME(message)/1024.0))

#define SPEED_MEASUREMENT_TIME(message) ((uint16_t)(SPEED_MEASUREMENT_TIME_SUM(message)-SPEED_MEASUREMENT_TIME_SUM(last_message)))
#define SPEED_WHEEL_REVS(message) ((uint16_t)(SPEED_WHEEL_REVS_SUM(message)-SPEED_WHEEL_REVS_SUM(last_message)))
#define SPEED_RPM(message) ((float)(1024*60*(SPEED_WHEEL_REVS(message))/SPEED_MEASUREMENT_TIME(message)))
#define SPEED_TIMEDIFF(message) ((float)(SPEED_MEASUREMENT_TIME(message)/1024.0))

#define CADENCE_MEASUREMENT_TIME(message) ((uint16_t)(CADENCE_MEASUREMENT_TIME_SUM(message)-CADENCE_MEASUREMENT_TIME_SUM(last_message)))
#define CADENCE_CRANK_REVS(message) ((uint16_t)(CADENCE_CRANK_REVS_SUM(message)-CADENCE_CRANK_REVS_SUM(last_message)))
#define CADENCE_RPM(message) ((float)(1024*60*(CADENCE_CRANK_REVS(message))/CADENCE_MEASUREMENT_TIME(message)))
#define CADENCE_TIMEDIFF(message) ((float)(CADENCE_MEASUREMENT_TIME(message)/1024.0))

#define SPEED_CADENCE_CADENCE_MEASUREMENT_TIME(message) ((uint16_t)(SPEED_CADENCE_CADENCE_MEASUREMENT_TIME_SUM(message)-SPEED_CADENCE_CADENCE_MEASUREMENT_TIME_SUM(last_message)))
#define SPEED_CADENCE_CRANK_REVS(message) ((uint16_t)(SPEED_CADENCE_CRANK_REVS_SUM(message)-SPEED_CADENCE_CRANK_REVS_SUM(last_message)))
#define SPEED_CADENCE_SPEED_MEASUREMENT_TIME(message) ((uint16_t)(SPEED_CADENCE_SPEED_MEASUREMENT_TIME_SUM(message)-SPEED_CADENCE_SPEED_MEASUREMENT_TIME_SUM(last_message)))
#define SPEED_CADENCE_WHEEL_REVS(message) ((uint16_t)(SPEED_CADENCE_WHEEL_REVS_SUM(message)-SPEED_CADENCE_WHEEL_REVS_SUM(last_message)))
#define SPEED_CADENCE_WHEEL_RPM(message) ((float)(1024*60.0*(SPEED_CADENCE_WHEEL_REVS(message))/SPEED_CADENCE_SPEED_MEASUREMENT_TIME(message)))
#define SPEED_CADENCE_CRANK_RPM(message) ((float)(1024*60.0*(SPEED_CADENCE_CRANK_REVS(message))/SPEED_CADENCE_CADENCE_MEASUREMENT_TIME(message)))
#define SPEED_CADENCE_TIMEDIFF(message) ((float)(SPEED_CADENCE_SPEED_MEASUREMENT_TIME(message)/1024.0))


// defines for message 'assign_channel'
#define SEND_MESSAGE_ASSIGN_CHANNEL(channel,channel_type,network_number) do {\
	TRANSMIT(0xa4); /* sync */                                 \
	TRANSMIT(3); /* length */                                  \
	TRANSMIT(0x42); /* d66 */                                  \
	TRANSMIT(((uint8_t)channel));                              \
	TRANSMIT(((uint8_t)channel_type));                         \
	TRANSMIT(((uint8_t)network_number));                       \
	SEND_CHECK();                                              \
} while (0);


// defines for message 'unassign_channel'
#define SEND_MESSAGE_UNASSIGN_CHANNEL(channel) do {\
	TRANSMIT(0xa4); /* sync */                                 \
	TRANSMIT(1); /* length */                                  \
	TRANSMIT(0x41); /* d65 */                                  \
	TRANSMIT(((uint8_t)channel));                              \
	SEND_CHECK();                                              \
} while (0);


// defines for message 'open_channel'
#define SEND_MESSAGE_OPEN_CHANNEL(channel) do {\
	TRANSMIT(0xa4); /* sync */                                 \
	TRANSMIT(1); /* length */                                  \
	TRANSMIT(0x4b); /* d75 */                                  \
	TRANSMIT(((uint8_t)channel));                              \
	SEND_CHECK();                                              \
} while (0);


// defines for message 'channel_id'
#define SEND_MESSAGE_CHANNEL_ID(channel,device_number,device_type_id,transmission_type) do {\
	TRANSMIT(0xa4); /* sync */                                 \
	TRANSMIT(5); /* length */                                  \
	TRANSMIT(0x51); /* d81 */                                  \
	TRANSMIT(((uint8_t)channel));                              \
	TRANSMIT((((uint16_t)device_number)&0xff));                \
	TRANSMIT((((uint16_t)device_number)>>8));                  \
	TRANSMIT(((uint8_t)device_type_id));                       \
	TRANSMIT(((uint8_t)transmission_type));                    \
	SEND_CHECK();                                              \
} while (0);


// defines for message 'burst_message'
#define SEND_MESSAGE_BURST_MESSAGE(chan_seq,data0,data1,data2,data3,data4,data5,data6,data7) do {\
	TRANSMIT(0xa4); /* sync */                                 \
	TRANSMIT(9); /* length */                                  \
	TRANSMIT(0x50); /* d80 */                                  \
	TRANSMIT(((uint8_t)chan_seq));                             \
	TRANSMIT(((uint8_t)data0));                                \
	TRANSMIT(((uint8_t)data1));                                \
	TRANSMIT(((uint8_t)data2));                                \
	TRANSMIT(((uint8_t)data3));                                \
	TRANSMIT(((uint8_t)data4));                                \
	TRANSMIT(((uint8_t)data5));                                \
	TRANSMIT(((uint8_t)data6));                                \
	TRANSMIT(((uint8_t)data7));                                \
	SEND_CHECK();                                              \
} while (0);


// defines for message 'burst_message7'
#define SEND_MESSAGE_BURST_MESSAGE7(chan_seq,data0,data1,data2,data3,data4,data5,data6) do {\
	TRANSMIT(0xa4); /* sync */                                 \
	TRANSMIT(8); /* length */                                  \
	TRANSMIT(0x50); /* d80 */                                  \
	TRANSMIT(((uint8_t)chan_seq));                             \
	TRANSMIT(((uint8_t)data0));                                \
	TRANSMIT(((uint8_t)data1));                                \
	TRANSMIT(((uint8_t)data2));                                \
	TRANSMIT(((uint8_t)data3));                                \
	TRANSMIT(((uint8_t)data4));                                \
	TRANSMIT(((uint8_t)data5));                                \
	TRANSMIT(((uint8_t)data6));                                \
	SEND_CHECK();                                              \
} while (0);


// defines for message 'burst_message6'
#define SEND_MESSAGE_BURST_MESSAGE6(chan_seq,data0,data1,data2,data3,data4,data5) do {\
	TRANSMIT(0xa4); /* sync */                                 \
	TRANSMIT(7); /* length */                                  \
	TRANSMIT(0x50); /* d80 */                                  \
	TRANSMIT(((uint8_t)chan_seq));                             \
	TRANSMIT(((uint8_t)data0));                                \
	TRANSMIT(((uint8_t)data1));                                \
	TRANSMIT(((uint8_t)data2));                                \
	TRANSMIT(((uint8_t)data3));                                \
	TRANSMIT(((uint8_t)data4));                                \
	TRANSMIT(((uint8_t)data5));                                \
	SEND_CHECK();                                              \
} while (0);


// defines for message 'burst_message5'
#define SEND_MESSAGE_BURST_MESSAGE5(chan_seq,data0,data1,data2,data3,data4) do {\
	TRANSMIT(0xa4); /* sync */                                 \
	TRANSMIT(6); /* length */                                  \
	TRANSMIT(0x50); /* d80 */                                  \
	TRANSMIT(((uint8_t)chan_seq));                             \
	TRANSMIT(((uint8_t)data0));                                \
	TRANSMIT(((uint8_t)data1));                                \
	TRANSMIT(((uint8_t)data2));                                \
	TRANSMIT(((uint8_t)data3));                                \
	TRANSMIT(((uint8_t)data4));                                \
	SEND_CHECK();                                              \
} while (0);


// defines for message 'burst_message4'
#define SEND_MESSAGE_BURST_MESSAGE4(chan_seq,data0,data1,data2,data3) do {\
	TRANSMIT(0xa4); /* sync */                                 \
	TRANSMIT(5); /* length */                                  \
	TRANSMIT(0x50); /* d80 */                                  \
	TRANSMIT(((uint8_t)chan_seq));                             \
	TRANSMIT(((uint8_t)data0));                                \
	TRANSMIT(((uint8_t)data1));                                \
	TRANSMIT(((uint8_t)data2));                                \
	TRANSMIT(((uint8_t)data3));                                \
	SEND_CHECK();                                              \
} while (0);


// defines for message 'burst_message3'
#define SEND_MESSAGE_BURST_MESSAGE3(chan_seq,data0,data1,data2) do {\
	TRANSMIT(0xa4); /* sync */                                 \
	TRANSMIT(4); /* length */                                  \
	TRANSMIT(0x50); /* d80 */                                  \
	TRANSMIT(((uint8_t)chan_seq));                             \
	TRANSMIT(((uint8_t)data0));                                \
	TRANSMIT(((uint8_t)data1));                                \
	TRANSMIT(((uint8_t)data2));                                \
	SEND_CHECK();                                              \
} while (0);


// defines for message 'burst_message2'
#define SEND_MESSAGE_BURST_MESSAGE2(chan_seq,data0,data1) do {\
	TRANSMIT(0xa4); /* sync */                                 \
	TRANSMIT(3); /* length */                                  \
	TRANSMIT(0x50); /* d80 */                                  \
	TRANSMIT(((uint8_t)chan_seq));                             \
	TRANSMIT(((uint8_t)data0));                                \
	TRANSMIT(((uint8_t)data1));                                \
	SEND_CHECK();                                              \
} while (0);


// defines for message 'burst_message1'
#define SEND_MESSAGE_BURST_MESSAGE1(chan_seq,data0) do {\
	TRANSMIT(0xa4); /* sync */                                 \
	TRANSMIT(2); /* length */                                  \
	TRANSMIT(0x50); /* d80 */                                  \
	TRANSMIT(((uint8_t)chan_seq));                             \
	TRANSMIT(((uint8_t)data0));                                \
	SEND_CHECK();                                              \
} while (0);


// defines for message 'burst_message0'
#define SEND_MESSAGE_BURST_MESSAGE0(chan_seq) do {\
	TRANSMIT(0xa4); /* sync */                                 \
	TRANSMIT(1); /* length */                                  \
	TRANSMIT(0x50); /* d80 */                                  \
	TRANSMIT(((uint8_t)chan_seq));                             \
	SEND_CHECK();                                              \
} while (0);


// defines for message 'channel_period'
#define SEND_MESSAGE_CHANNEL_PERIOD(channel,period) do {\
	TRANSMIT(0xa4); /* sync */                                 \
	TRANSMIT(3); /* length */                                  \
	TRANSMIT(0x43); /* d67 */                                  \
	TRANSMIT(((uint8_t)channel));                              \
	TRANSMIT((((uint16_t)period)&0xff));                       \
	TRANSMIT((((uint16_t)period)>>8));                         \
	SEND_CHECK();                                              \
} while (0);


// defines for message 'search_timeout'
#define SEND_MESSAGE_SEARCH_TIMEOUT(channel,search_timeout) do {\
	TRANSMIT(0xa4); /* sync */                                 \
	TRANSMIT(2); /* length */                                  \
	TRANSMIT(0x44); /* d68 */                                  \
	TRANSMIT(((uint8_t)channel));                              \
	TRANSMIT(((uint8_t)search_timeout));                       \
	SEND_CHECK();                                              \
} while (0);


// defines for message 'channel_frequency'
#define SEND_MESSAGE_CHANNEL_FREQUENCY(channel,rf_frequency) do {\
	TRANSMIT(0xa4); /* sync */                                 \
	TRANSMIT(2); /* length */                                  \
	TRANSMIT(0x45); /* d69 */                                  \
	TRANSMIT(((uint8_t)channel));                              \
	TRANSMIT(((uint8_t)rf_frequency));                         \
	SEND_CHECK();                                              \
} while (0);


// defines for message 'set_network'
#define SEND_MESSAGE_SET_NETWORK(channel,key0,key1,key2,key3,key4,key5,key6,key7) do {\
	TRANSMIT(0xa4); /* sync */                                 \
	TRANSMIT(9); /* length */                                  \
	TRANSMIT(0x46); /* d70 */                                  \
	TRANSMIT(((uint8_t)channel));                              \
	TRANSMIT(((uint8_t)key0));                                 \
	TRANSMIT(((uint8_t)key1));                                 \
	TRANSMIT(((uint8_t)key2));                                 \
	TRANSMIT(((uint8_t)key3));                                 \
	TRANSMIT(((uint8_t)key4));                                 \
	TRANSMIT(((uint8_t)key5));                                 \
	TRANSMIT(((uint8_t)key6));                                 \
	TRANSMIT(((uint8_t)key7));                                 \
	SEND_CHECK();                                              \
} while (0);


// defines for message 'transmit_power'
#define SEND_MESSAGE_TRANSMIT_POWER(tx_power) do {\
	TRANSMIT(0xa4); /* sync */                                 \
	TRANSMIT(2); /* length */                                  \
	TRANSMIT(0x47); /* d71 */                                  \
	TRANSMIT(0x00); /* d0 */                                   \
	TRANSMIT(((uint8_t)tx_power));                             \
	SEND_CHECK();                                              \
} while (0);


// defines for message 'reset_system'
#define SEND_MESSAGE_RESET_SYSTEM() do {\
	TRANSMIT(0xa4); /* sync */                                 \
	TRANSMIT(1); /* length */                                  \
	TRANSMIT(0x4a); /* d74 */                                  \
	TRANSMIT(0); /* unknown/don't care */                      \
	SEND_CHECK();                                              \
} while (0);


// defines for message 'request_message'
#define SEND_MESSAGE_REQUEST_MESSAGE(channel,message_requested) do {\
	TRANSMIT(0xa4); /* sync */                                 \
	TRANSMIT(2); /* length */                                  \
	TRANSMIT(0x4d); /* d77 */                                  \
	TRANSMIT(((uint8_t)channel));                              \
	TRANSMIT(((uint8_t)message_requested));                    \
	SEND_CHECK();                                              \
} while (0);


// defines for message 'close_channel'
#define SEND_MESSAGE_CLOSE_CHANNEL(channel) do {\
	TRANSMIT(0xa4); /* sync */                                 \
	TRANSMIT(1); /* length */                                  \
	TRANSMIT(0x4c); /* d76 */                                  \
	TRANSMIT(((uint8_t)channel));                              \
	SEND_CHECK();                                              \
} while (0);


// defines for message 'response_no_error'
#define SEND_MESSAGE_RESPONSE_NO_ERROR(channel,message_id) do {\
	TRANSMIT(0xa4); /* sync */                                 \
	TRANSMIT(3); /* length */                                  \
	TRANSMIT(0x40); /* d64 */                                  \
	TRANSMIT(((uint8_t)channel));                              \
	TRANSMIT(((uint8_t)message_id));                           \
	TRANSMIT(0x00); /* d0 */                                   \
	SEND_CHECK();                                              \
} while (0);


// defines for message 'response_assign_channel_ok'
#define SEND_MESSAGE_RESPONSE_ASSIGN_CHANNEL_OK(channel) do {\
	TRANSMIT(0xa4); /* sync */                                 \
	TRANSMIT(3); /* length */                                  \
	TRANSMIT(0x40); /* d64 */                                  \
	TRANSMIT(((uint8_t)channel));                              \
	TRANSMIT(0x42); /* d66 */                                  \
	TRANSMIT(0x00); /* d0 */                                   \
	SEND_CHECK();                                              \
} while (0);


// defines for message 'response_channel_unassign_ok'
#define SEND_MESSAGE_RESPONSE_CHANNEL_UNASSIGN_OK(channel) do {\
	TRANSMIT(0xa4); /* sync */                                 \
	TRANSMIT(3); /* length */                                  \
	TRANSMIT(0x40); /* d64 */                                  \
	TRANSMIT(((uint8_t)channel));                              \
	TRANSMIT(0x41); /* d65 */                                  \
	TRANSMIT(0x00); /* d0 */                                   \
	SEND_CHECK();                                              \
} while (0);


// defines for message 'response_open_channel_ok'
#define SEND_MESSAGE_RESPONSE_OPEN_CHANNEL_OK(channel) do {\
	TRANSMIT(0xa4); /* sync */                                 \
	TRANSMIT(3); /* length */                                  \
	TRANSMIT(0x40); /* d64 */                                  \
	TRANSMIT(((uint8_t)channel));                              \
	TRANSMIT(0x4b); /* d75 */                                  \
	TRANSMIT(0x00); /* d0 */                                   \
	SEND_CHECK();                                              \
} while (0);


// defines for message 'response_channel_id_ok'
#define SEND_MESSAGE_RESPONSE_CHANNEL_ID_OK(channel) do {\
	TRANSMIT(0xa4); /* sync */                                 \
	TRANSMIT(3); /* length */                                  \
	TRANSMIT(0x40); /* d64 */                                  \
	TRANSMIT(((uint8_t)channel));                              \
	TRANSMIT(0x51); /* d81 */                                  \
	TRANSMIT(0x00); /* d0 */                                   \
	SEND_CHECK();                                              \
} while (0);


// defines for message 'response_channel_period_ok'
#define SEND_MESSAGE_RESPONSE_CHANNEL_PERIOD_OK(channel) do {\
	TRANSMIT(0xa4); /* sync */                                 \
	TRANSMIT(3); /* length */                                  \
	TRANSMIT(0x40); /* d64 */                                  \
	TRANSMIT(((uint8_t)channel));                              \
	TRANSMIT(0x43); /* d67 */                                  \
	TRANSMIT(0x00); /* d0 */                                   \
	SEND_CHECK();                                              \
} while (0);


// defines for message 'response_channel_frequency_ok'
#define SEND_MESSAGE_RESPONSE_CHANNEL_FREQUENCY_OK(channel) do {\
	TRANSMIT(0xa4); /* sync */                                 \
	TRANSMIT(3); /* length */                                  \
	TRANSMIT(0x40); /* d64 */                                  \
	TRANSMIT(((uint8_t)channel));                              \
	TRANSMIT(0x45); /* d69 */                                  \
	TRANSMIT(0x00); /* d0 */                                   \
	SEND_CHECK();                                              \
} while (0);


// defines for message 'response_set_network_ok'
#define SEND_MESSAGE_RESPONSE_SET_NETWORK_OK(channel) do {\
	TRANSMIT(0xa4); /* sync */                                 \
	TRANSMIT(3); /* length */                                  \
	TRANSMIT(0x40); /* d64 */                                  \
	TRANSMIT(((uint8_t)channel));                              \
	TRANSMIT(0x46); /* d70 */                                  \
	TRANSMIT(0x00); /* d0 */                                   \
	SEND_CHECK();                                              \
} while (0);


// defines for message 'response_transmit_power_ok'
#define SEND_MESSAGE_RESPONSE_TRANSMIT_POWER_OK(channel) do {\
	TRANSMIT(0xa4); /* sync */                                 \
	TRANSMIT(3); /* length */                                  \
	TRANSMIT(0x40); /* d64 */                                  \
	TRANSMIT(((uint8_t)channel));                              \
	TRANSMIT(0x47); /* d71 */                                  \
	TRANSMIT(0x00); /* d0 */                                   \
	SEND_CHECK();                                              \
} while (0);


// defines for message 'response_close_channel_ok'
#define SEND_MESSAGE_RESPONSE_CLOSE_CHANNEL_OK(channel) do {\
	TRANSMIT(0xa4); /* sync */                                 \
	TRANSMIT(3); /* length */                                  \
	TRANSMIT(0x40); /* d64 */                                  \
	TRANSMIT(((uint8_t)channel));                              \
	TRANSMIT(0x4c); /* d76 */                                  \
	TRANSMIT(0x00); /* d0 */                                   \
	SEND_CHECK();                                              \
} while (0);


// defines for message 'response_search_timeout_ok'
#define SEND_MESSAGE_RESPONSE_SEARCH_TIMEOUT_OK(channel) do {\
	TRANSMIT(0xa4); /* sync */                                 \
	TRANSMIT(3); /* length */                                  \
	TRANSMIT(0x40); /* d64 */                                  \
	TRANSMIT(((uint8_t)channel));                              \
	TRANSMIT(0x44); /* d68 */                                  \
	TRANSMIT(0x00); /* d0 */                                   \
	SEND_CHECK();                                              \
} while (0);


// defines for message 'event_rx_search_timeout'
#define SEND_MESSAGE_EVENT_RX_SEARCH_TIMEOUT(channel,message_id) do {\
	TRANSMIT(0xa4); /* sync */                                 \
	TRANSMIT(3); /* length */                                  \
	TRANSMIT(0x40); /* d64 */                                  \
	TRANSMIT(((uint8_t)channel));                              \
	TRANSMIT(((uint8_t)message_id));                           \
	TRANSMIT(0x01); /* d1 */                                   \
	SEND_CHECK();                                              \
} while (0);


// defines for message 'event_rx_fail'
#define SEND_MESSAGE_EVENT_RX_FAIL(channel,message_id) do {\
	TRANSMIT(0xa4); /* sync */                                 \
	TRANSMIT(3); /* length */                                  \
	TRANSMIT(0x40); /* d64 */                                  \
	TRANSMIT(((uint8_t)channel));                              \
	TRANSMIT(((uint8_t)message_id));                           \
	TRANSMIT(0x02); /* d2 */                                   \
	SEND_CHECK();                                              \
} while (0);


// defines for message 'event_tx'
#define SEND_MESSAGE_EVENT_TX(channel,message_id) do {\
	TRANSMIT(0xa4); /* sync */                                 \
	TRANSMIT(3); /* length */                                  \
	TRANSMIT(0x40); /* d64 */                                  \
	TRANSMIT(((uint8_t)channel));                              \
	TRANSMIT(((uint8_t)message_id));                           \
	TRANSMIT(0x03); /* d3 */                                   \
	SEND_CHECK();                                              \
} while (0);


// defines for message 'event_transfer_rx_failed'
#define SEND_MESSAGE_EVENT_TRANSFER_RX_FAILED(channel,message_id) do {\
	TRANSMIT(0xa4); /* sync */                                 \
	TRANSMIT(3); /* length */                                  \
	TRANSMIT(0x40); /* d64 */                                  \
	TRANSMIT(((uint8_t)channel));                              \
	TRANSMIT(((uint8_t)message_id));                           \
	TRANSMIT(0x04); /* d4 */                                   \
	SEND_CHECK();                                              \
} while (0);


// defines for message 'event_transfer_tx_completed'
#define SEND_MESSAGE_EVENT_TRANSFER_TX_COMPLETED(channel,message_id) do {\
	TRANSMIT(0xa4); /* sync */                                 \
	TRANSMIT(3); /* length */                                  \
	TRANSMIT(0x40); /* d64 */                                  \
	TRANSMIT(((uint8_t)channel));                              \
	TRANSMIT(((uint8_t)message_id));                           \
	TRANSMIT(0x05); /* d5 */                                   \
	SEND_CHECK();                                              \
} while (0);


// defines for message 'event_transfer_tx_failed'
#define SEND_MESSAGE_EVENT_TRANSFER_TX_FAILED(channel,message_id) do {\
	TRANSMIT(0xa4); /* sync */                                 \
	TRANSMIT(3); /* length */                                  \
	TRANSMIT(0x40); /* d64 */                                  \
	TRANSMIT(((uint8_t)channel));                              \
	TRANSMIT(((uint8_t)message_id));                           \
	TRANSMIT(0x06); /* d6 */                                   \
	SEND_CHECK();                                              \
} while (0);


// defines for message 'event_channel_closed'
#define SEND_MESSAGE_EVENT_CHANNEL_CLOSED(channel,message_id) do {\
	TRANSMIT(0xa4); /* sync */                                 \
	TRANSMIT(3); /* length */                                  \
	TRANSMIT(0x40); /* d64 */                                  \
	TRANSMIT(((uint8_t)channel));                              \
	TRANSMIT(((uint8_t)message_id));                           \
	TRANSMIT(0x07); /* d7 */                                   \
	SEND_CHECK();                                              \
} while (0);


// defines for message 'event_rx_fail_go_to_search'
#define SEND_MESSAGE_EVENT_RX_FAIL_GO_TO_SEARCH(channel,message_id) do {\
	TRANSMIT(0xa4); /* sync */                                 \
	TRANSMIT(3); /* length */                                  \
	TRANSMIT(0x40); /* d64 */                                  \
	TRANSMIT(((uint8_t)channel));                              \
	TRANSMIT(((uint8_t)message_id));                           \
	TRANSMIT(0x08); /* d8 */                                   \
	SEND_CHECK();                                              \
} while (0);


// defines for message 'event_channel_collision'
#define SEND_MESSAGE_EVENT_CHANNEL_COLLISION(channel,message_id) do {\
	TRANSMIT(0xa4); /* sync */                                 \
	TRANSMIT(3); /* length */                                  \
	TRANSMIT(0x40); /* d64 */                                  \
	TRANSMIT(((uint8_t)channel));                              \
	TRANSMIT(((uint8_t)message_id));                           \
	TRANSMIT(0x09); /* d9 */                                   \
	SEND_CHECK();                                              \
} while (0);


// defines for message 'event_transfer_tx_start'
#define SEND_MESSAGE_EVENT_TRANSFER_TX_START(channel,message_id) do {\
	TRANSMIT(0xa4); /* sync */                                 \
	TRANSMIT(3); /* length */                                  \
	TRANSMIT(0x40); /* d64 */                                  \
	TRANSMIT(((uint8_t)channel));                              \
	TRANSMIT(((uint8_t)message_id));                           \
	TRANSMIT(0x0a); /* d10 */                                  \
	SEND_CHECK();                                              \
} while (0);


// defines for message 'event_rx_acknowledged'
#define SEND_MESSAGE_EVENT_RX_ACKNOWLEDGED(channel,message_id) do {\
	TRANSMIT(0xa4); /* sync */                                 \
	TRANSMIT(3); /* length */                                  \
	TRANSMIT(0x40); /* d64 */                                  \
	TRANSMIT(((uint8_t)channel));                              \
	TRANSMIT(((uint8_t)message_id));                           \
	TRANSMIT(0x0b); /* d11 */                                  \
	SEND_CHECK();                                              \
} while (0);


// defines for message 'event_rx_burst_packet'
#define SEND_MESSAGE_EVENT_RX_BURST_PACKET(channel,message_id) do {\
	TRANSMIT(0xa4); /* sync */                                 \
	TRANSMIT(3); /* length */                                  \
	TRANSMIT(0x40); /* d64 */                                  \
	TRANSMIT(((uint8_t)channel));                              \
	TRANSMIT(((uint8_t)message_id));                           \
	TRANSMIT(0x0c); /* d12 */                                  \
	SEND_CHECK();                                              \
} while (0);


// defines for message 'channel_in_wrong_state'
#define SEND_MESSAGE_CHANNEL_IN_WRONG_STATE(channel,message_id) do {\
	TRANSMIT(0xa4); /* sync */                                 \
	TRANSMIT(3); /* length */                                  \
	TRANSMIT(0x40); /* d64 */                                  \
	TRANSMIT(((uint8_t)channel));                              \
	TRANSMIT(((uint8_t)message_id));                           \
	TRANSMIT(0x15); /* d21 */                                  \
	SEND_CHECK();                                              \
} while (0);


// defines for message 'channel_not_opened'
#define SEND_MESSAGE_CHANNEL_NOT_OPENED(channel,message_id) do {\
	TRANSMIT(0xa4); /* sync */                                 \
	TRANSMIT(3); /* length */                                  \
	TRANSMIT(0x40); /* d64 */                                  \
	TRANSMIT(((uint8_t)channel));                              \
	TRANSMIT(((uint8_t)message_id));                           \
	TRANSMIT(0x16); /* d22 */                                  \
	SEND_CHECK();                                              \
} while (0);


// defines for message 'channel_id_not_set'
#define SEND_MESSAGE_CHANNEL_ID_NOT_SET(channel,message_id) do {\
	TRANSMIT(0xa4); /* sync */                                 \
	TRANSMIT(3); /* length */                                  \
	TRANSMIT(0x40); /* d64 */                                  \
	TRANSMIT(((uint8_t)channel));                              \
	TRANSMIT(((uint8_t)message_id));                           \
	TRANSMIT(0x18); /* d24 */                                  \
	SEND_CHECK();                                              \
} while (0);


// defines for message 'transfer_in_progress'
#define SEND_MESSAGE_TRANSFER_IN_PROGRESS(channel,message_id) do {\
	TRANSMIT(0xa4); /* sync */                                 \
	TRANSMIT(3); /* length */                                  \
	TRANSMIT(0x40); /* d64 */                                  \
	TRANSMIT(((uint8_t)channel));                              \
	TRANSMIT(((uint8_t)message_id));                           \
	TRANSMIT(0x1f); /* d31 */                                  \
	SEND_CHECK();                                              \
} while (0);


// defines for message 'channel_status'
#define SEND_MESSAGE_CHANNEL_STATUS(channel,status) do {\
	TRANSMIT(0xa4); /* sync */                                 \
	TRANSMIT(2); /* length */                                  \
	TRANSMIT(0x52); /* d82 */                                  \
	TRANSMIT(((uint8_t)channel));                              \
	TRANSMIT(((uint8_t)status));                               \
	SEND_CHECK();                                              \
} while (0);


// defines for message 'cw_init'
#define SEND_MESSAGE_CW_INIT() do {\
	TRANSMIT(0xa4); /* sync */                                 \
	TRANSMIT(1); /* length */                                  \
	TRANSMIT(0x53); /* d83 */                                  \
	TRANSMIT(0); /* unknown/don't care */                      \
	SEND_CHECK();                                              \
} while (0);


// defines for message 'cw_test'
#define SEND_MESSAGE_CW_TEST(power,freq) do {\
	TRANSMIT(0xa4); /* sync */                                 \
	TRANSMIT(3); /* length */                                  \
	TRANSMIT(0x48); /* d72 */                                  \
	TRANSMIT(0); /* unknown/don't care */                      \
	TRANSMIT(((uint8_t)power));                                \
	TRANSMIT(((uint8_t)freq));                                 \
	SEND_CHECK();                                              \
} while (0);


// defines for message 'capabilities'
#define SEND_MESSAGE_CAPABILITIES(max_channels,max_networks,standard_options,advanced_options) do {\
	TRANSMIT(0xa4); /* sync */                                 \
	TRANSMIT(4); /* length */                                  \
	TRANSMIT(0x54); /* d84 */                                  \
	TRANSMIT(((uint8_t)max_channels));                         \
	TRANSMIT(((uint8_t)max_networks));                         \
	TRANSMIT(((uint8_t)standard_options));                     \
	TRANSMIT(((uint8_t)advanced_options));                     \
	SEND_CHECK();                                              \
} while (0);


// defines for message 'capabilities_extended'
#define SEND_MESSAGE_CAPABILITIES_EXTENDED(max_channels,max_networks,standard_options,advanced_options,advanced_options_2,max_data_channels) do {\
	TRANSMIT(0xa4); /* sync */                                 \
	TRANSMIT(6); /* length */                                  \
	TRANSMIT(0x54); /* d84 */                                  \
	TRANSMIT(((uint8_t)max_channels));                         \
	TRANSMIT(((uint8_t)max_networks));                         \
	TRANSMIT(((uint8_t)standard_options));                     \
	TRANSMIT(((uint8_t)advanced_options));                     \
	TRANSMIT(((uint8_t)advanced_options_2));                   \
	TRANSMIT(((uint8_t)max_data_channels));                    \
	SEND_CHECK();                                              \
} while (0);


// defines for message 'ant_version'
#define SEND_MESSAGE_ANT_VERSION(data0,data1,data2,data3,data4,data5,data6,data7,data8) do {\
	TRANSMIT(0xa4); /* sync */                                 \
	TRANSMIT(9); /* length */                                  \
	TRANSMIT(0x3e); /* d62 */                                  \
	TRANSMIT(((uint8_t)data0));                                \
	TRANSMIT(((uint8_t)data1));                                \
	TRANSMIT(((uint8_t)data2));                                \
	TRANSMIT(((uint8_t)data3));                                \
	TRANSMIT(((uint8_t)data4));                                \
	TRANSMIT(((uint8_t)data5));                                \
	TRANSMIT(((uint8_t)data6));                                \
	TRANSMIT(((uint8_t)data7));                                \
	TRANSMIT(((uint8_t)data8));                                \
	SEND_CHECK();                                              \
} while (0);


// defines for message 'ant_version_long'
#define SEND_MESSAGE_ANT_VERSION_LONG(data0,data1,data2,data3,data4,data5,data6,data7,data8,data9,data10,data11,data12) do {\
	TRANSMIT(0xa4); /* sync */                                 \
	TRANSMIT(13); /* length */                                 \
	TRANSMIT(0x3e); /* d62 */                                  \
	TRANSMIT(((uint8_t)data0));                                \
	TRANSMIT(((uint8_t)data1));                                \
	TRANSMIT(((uint8_t)data2));                                \
	TRANSMIT(((uint8_t)data3));                                \
	TRANSMIT(((uint8_t)data4));                                \
	TRANSMIT(((uint8_t)data5));                                \
	TRANSMIT(((uint8_t)data6));                                \
	TRANSMIT(((uint8_t)data7));                                \
	TRANSMIT(((uint8_t)data8));                                \
	TRANSMIT(((uint8_t)data9));                                \
	TRANSMIT(((uint8_t)data10));                               \
	TRANSMIT(((uint8_t)data11));                               \
	TRANSMIT(((uint8_t)data12));                               \
	SEND_CHECK();                                              \
} while (0);


// defines for message 'transfer_seq_number_error'
#define SEND_MESSAGE_TRANSFER_SEQ_NUMBER_ERROR(channel,message_id) do {\
	TRANSMIT(0xa4); /* sync */                                 \
	TRANSMIT(3); /* length */                                  \
	TRANSMIT(0x40); /* d64 */                                  \
	TRANSMIT(((uint8_t)channel));                              \
	TRANSMIT(((uint8_t)message_id));                           \
	TRANSMIT(0x20); /* d32 */                                  \
	SEND_CHECK();                                              \
} while (0);


// defines for message 'invalid_message'
#define SEND_MESSAGE_INVALID_MESSAGE(channel,message_id) do {\
	TRANSMIT(0xa4); /* sync */                                 \
	TRANSMIT(3); /* length */                                  \
	TRANSMIT(0x40); /* d64 */                                  \
	TRANSMIT(((uint8_t)channel));                              \
	TRANSMIT(((uint8_t)message_id));                           \
	TRANSMIT(0x28); /* d40 */                                  \
	SEND_CHECK();                                              \
} while (0);


// defines for message 'invalid_network_number'
#define SEND_MESSAGE_INVALID_NETWORK_NUMBER(channel,message_id) do {\
	TRANSMIT(0xa4); /* sync */                                 \
	TRANSMIT(3); /* length */                                  \
	TRANSMIT(0x40); /* d64 */                                  \
	TRANSMIT(((uint8_t)channel));                              \
	TRANSMIT(((uint8_t)message_id));                           \
	TRANSMIT(0x29); /* d41 */                                  \
	SEND_CHECK();                                              \
} while (0);


// defines for message 'channel_response'
#define SEND_MESSAGE_CHANNEL_RESPONSE(channel,message_id,message_code) do {\
	TRANSMIT(0xa4); /* sync */                                 \
	TRANSMIT(3); /* length */                                  \
	TRANSMIT(0x40); /* d64 */                                  \
	TRANSMIT(((uint8_t)channel));                              \
	TRANSMIT(((uint8_t)message_id));                           \
	TRANSMIT(((uint8_t)message_code));                         \
	SEND_CHECK();                                              \
} while (0);


// defines for message 'extended_broadcast_data'
#define SEND_MESSAGE_EXTENDED_BROADCAST_DATA(channel,device_number,device_type,transmission_type,data0,data1,data2,data3,data4,data5,data6,data7) do {\
	TRANSMIT(0xa4); /* sync */                                 \
	TRANSMIT(13); /* length */                                 \
	TRANSMIT(0x5d); /* d93 */                                  \
	TRANSMIT(((uint8_t)channel));                              \
	TRANSMIT((((uint16_t)device_number)&0xff));                \
	TRANSMIT((((uint16_t)device_number)>>8));                  \
	TRANSMIT(((uint8_t)device_type));                          \
	TRANSMIT(((uint8_t)transmission_type));                    \
	TRANSMIT(((uint8_t)data0));                                \
	TRANSMIT(((uint8_t)data1));                                \
	TRANSMIT(((uint8_t)data2));                                \
	TRANSMIT(((uint8_t)data3));                                \
	TRANSMIT(((uint8_t)data4));                                \
	TRANSMIT(((uint8_t)data5));                                \
	TRANSMIT(((uint8_t)data6));                                \
	TRANSMIT(((uint8_t)data7));                                \
	SEND_CHECK();                                              \
} while (0);


// defines for message 'extended_ack_data'
#define SEND_MESSAGE_EXTENDED_ACK_DATA(channel,device_number,device_type,transmission_type,data0,data1,data2,data3,data4,data5,data6,data7) do {\
	TRANSMIT(0xa4); /* sync */                                 \
	TRANSMIT(13); /* length */                                 \
	TRANSMIT(0x5e); /* d94 */                                  \
	TRANSMIT(((uint8_t)channel));                              \
	TRANSMIT((((uint16_t)device_number)&0xff));                \
	TRANSMIT((((uint16_t)device_number)>>8));                  \
	TRANSMIT(((uint8_t)device_type));                          \
	TRANSMIT(((uint8_t)transmission_type));                    \
	TRANSMIT(((uint8_t)data0));                                \
	TRANSMIT(((uint8_t)data1));                                \
	TRANSMIT(((uint8_t)data2));                                \
	TRANSMIT(((uint8_t)data3));                                \
	TRANSMIT(((uint8_t)data4));                                \
	TRANSMIT(((uint8_t)data5));                                \
	TRANSMIT(((uint8_t)data6));                                \
	TRANSMIT(((uint8_t)data7));                                \
	SEND_CHECK();                                              \
} while (0);


// defines for message 'extended_burst_data'
#define SEND_MESSAGE_EXTENDED_BURST_DATA(channel,device_number,device_type,transmission_type,data0,data1,data2,data3,data4,data5,data6,data7) do {\
	TRANSMIT(0xa4); /* sync */                                 \
	TRANSMIT(13); /* length */                                 \
	TRANSMIT(0x5f); /* d95 */                                  \
	TRANSMIT(((uint8_t)channel));                              \
	TRANSMIT((((uint16_t)device_number)&0xff));                \
	TRANSMIT((((uint16_t)device_number)>>8));                  \
	TRANSMIT(((uint8_t)device_type));                          \
	TRANSMIT(((uint8_t)transmission_type));                    \
	TRANSMIT(((uint8_t)data0));                                \
	TRANSMIT(((uint8_t)data1));                                \
	TRANSMIT(((uint8_t)data2));                                \
	TRANSMIT(((uint8_t)data3));                                \
	TRANSMIT(((uint8_t)data4));                                \
	TRANSMIT(((uint8_t)data5));                                \
	TRANSMIT(((uint8_t)data6));                                \
	TRANSMIT(((uint8_t)data7));                                \
	SEND_CHECK();                                              \
} while (0);


// defines for message 'startup_message'
#define SEND_MESSAGE_STARTUP_MESSAGE(start_message) do {\
	TRANSMIT(0xa4); /* sync */                                 \
	TRANSMIT(1); /* length */                                  \
	TRANSMIT(0x6f); /* d111 */                                 \
	TRANSMIT(((uint8_t)start_message));                        \
	SEND_CHECK();                                              \
} while (0);


// defines for message 'calibration_request'
#define SEND_MESSAGE_CALIBRATION_REQUEST(channel) do {\
	TRANSMIT(0xa4); /* sync */                                 \
	TRANSMIT(9); /* length */                                  \
	TRANSMIT(0); /* unknown/don't care */                      \
	TRANSMIT(((uint8_t)channel));                              \
	TRANSMIT(0x01); /* d1 */                                   \
	TRANSMIT(0xaa); /* d170 */                                 \
	TRANSMIT(0); /* unknown/don't care */                      \
	TRANSMIT(0); /* unknown/don't care */                      \
	TRANSMIT(0); /* unknown/don't care */                      \
	TRANSMIT(0); /* unknown/don't care */                      \
	TRANSMIT(0); /* unknown/don't care */                      \
	TRANSMIT(0); /* unknown/don't care */                      \
	SEND_CHECK();                                              \
} while (0);


// defines for message 'srm_zero_response'
#define SEND_MESSAGE_SRM_ZERO_RESPONSE(channel,offset) do {\
	TRANSMIT(0xa4); /* sync */                                 \
	TRANSMIT(9); /* length */                                  \
	TRANSMIT(0); /* unknown/don't care */                      \
	TRANSMIT(((uint8_t)channel));                              \
	TRANSMIT(0x01); /* d1 */                                   \
	TRANSMIT(0x10); /* d16 */                                  \
	TRANSMIT(0x01); /* d1 */                                   \
	TRANSMIT(0); /* unknown/don't care */                      \
	TRANSMIT(0); /* unknown/don't care */                      \
	TRANSMIT(0); /* unknown/don't care */                      \
	TRANSMIT((((uint16_t)offset)>>8));                         \
	TRANSMIT((((uint16_t)offset)&0xff));                       \
	SEND_CHECK();                                              \
} while (0);


// defines for message 'calibration_pass'
#define SEND_MESSAGE_CALIBRATION_PASS(channel,autozero_status,calibration_data) do {\
	TRANSMIT(0xa4); /* sync */                                 \
	TRANSMIT(9); /* length */                                  \
	TRANSMIT(0); /* unknown/don't care */                      \
	TRANSMIT(((uint8_t)channel));                              \
	TRANSMIT(0x01); /* d1 */                                   \
	TRANSMIT(0xac); /* d172 */                                 \
	TRANSMIT(((uint8_t)autozero_status));                      \
	TRANSMIT(0); /* unknown/don't care */                      \
	TRANSMIT(0); /* unknown/don't care */                      \
	TRANSMIT(0); /* unknown/don't care */                      \
	TRANSMIT((((uint16_t)calibration_data)&0xff));             \
	TRANSMIT((((uint16_t)calibration_data)>>8));               \
	SEND_CHECK();                                              \
} while (0);


// defines for message 'calibration_fail'
#define SEND_MESSAGE_CALIBRATION_FAIL(channel,autozero_status,calibration_data) do {\
	TRANSMIT(0xa4); /* sync */                                 \
	TRANSMIT(9); /* length */                                  \
	TRANSMIT(0); /* unknown/don't care */                      \
	TRANSMIT(((uint8_t)channel));                              \
	TRANSMIT(0x01); /* d1 */                                   \
	TRANSMIT(0xaf); /* d175 */                                 \
	TRANSMIT(((uint8_t)autozero_status));                      \
	TRANSMIT(0); /* unknown/don't care */                      \
	TRANSMIT(0); /* unknown/don't care */                      \
	TRANSMIT(0); /* unknown/don't care */                      \
	TRANSMIT((((uint16_t)calibration_data)&0xff));             \
	TRANSMIT((((uint16_t)calibration_data)>>8));               \
	SEND_CHECK();                                              \
} while (0);


// defines for message 'torque_support'
#define SEND_MESSAGE_TORQUE_SUPPORT(channel,sensor_configuration,raw_torque,offset_torque) do {\
	TRANSMIT(0xa4); /* sync */                                 \
	TRANSMIT(9); /* length */                                  \
	TRANSMIT(0); /* unknown/don't care */                      \
	TRANSMIT(((uint8_t)channel));                              \
	TRANSMIT(0x01); /* d1 */                                   \
	TRANSMIT(0x12); /* d18 */                                  \
	TRANSMIT(((uint8_t)sensor_configuration));                 \
	TRANSMIT((((int16_t)raw_torque)&0xff));                    \
	TRANSMIT((((int16_t)raw_torque)>>8));                      \
	TRANSMIT((((int16_t)offset_torque)&0xff));                 \
	TRANSMIT((((int16_t)offset_torque)>>8));                   \
	TRANSMIT(0); /* unknown/don't care */                      \
	SEND_CHECK();                                              \
} while (0);


// defines for message 'standard_power'
#define SEND_MESSAGE_STANDARD_POWER(channel,event_count_sum,instant_cadence,sum_power_sum,instant_power) do {\
	TRANSMIT(0xa4); /* sync */                                 \
	TRANSMIT(9); /* length */                                  \
	TRANSMIT(0x4e); /* d78 */                                  \
	TRANSMIT(((uint8_t)channel));                              \
	TRANSMIT(0x10); /* d16 */                                  \
	TRANSMIT(((uint8_t)event_count_sum));                      \
	TRANSMIT(0); /* unknown/don't care */                      \
	TRANSMIT(((uint8_t)instant_cadence));                      \
	TRANSMIT((((uint16_t)sum_power_sum)&0xff));                \
	TRANSMIT((((uint16_t)sum_power_sum)>>8));                  \
	TRANSMIT((((uint16_t)instant_power)&0xff));                \
	TRANSMIT((((uint16_t)instant_power)>>8));                  \
	SEND_CHECK();                                              \
} while (0);


// defines for message 'wheel_torque'
#define SEND_MESSAGE_WHEEL_TORQUE(channel,event_count_sum,wheel_rev,instant_cadence,wheel_period_sum,torque_sum) do {\
	TRANSMIT(0xa4); /* sync */                                 \
	TRANSMIT(9); /* length */                                  \
	TRANSMIT(0x4e); /* d78 */                                  \
	TRANSMIT(((uint8_t)channel));                              \
	TRANSMIT(0x11); /* d17 */                                  \
	TRANSMIT(((uint8_t)event_count_sum));                      \
	TRANSMIT(((uint8_t)wheel_rev));                            \
	TRANSMIT(((uint8_t)instant_cadence));                      \
	TRANSMIT((((uint16_t)wheel_period_sum)&0xff));             \
	TRANSMIT((((uint16_t)wheel_period_sum)>>8));               \
	TRANSMIT((((uint16_t)torque_sum)&0xff));                   \
	TRANSMIT((((uint16_t)torque_sum)>>8));                     \
	SEND_CHECK();                                              \
} while (0);


// defines for message 'crank_torque'
#define SEND_MESSAGE_CRANK_TORQUE(channel,event_count_sum,crank_rev,instant_cadence,crank_period_sum,torque_sum) do {\
	TRANSMIT(0xa4); /* sync */                                 \
	TRANSMIT(9); /* length */                                  \
	TRANSMIT(0x4e); /* d78 */                                  \
	TRANSMIT(((uint8_t)channel));                              \
	TRANSMIT(0x12); /* d18 */                                  \
	TRANSMIT(((uint8_t)event_count_sum));                      \
	TRANSMIT(((uint8_t)crank_rev));                            \
	TRANSMIT(((uint8_t)instant_cadence));                      \
	TRANSMIT((((uint16_t)crank_period_sum)&0xff));             \
	TRANSMIT((((uint16_t)crank_period_sum)>>8));               \
	TRANSMIT((((uint16_t)torque_sum)&0xff));                   \
	TRANSMIT((((uint16_t)torque_sum)>>8));                     \
	SEND_CHECK();                                              \
} while (0);


// defines for message 'crank_SRM'
#define SEND_MESSAGE_CRANK_SRM(channel,event_count_sum,slope,crank_period_sum,torque_sum) do {\
	TRANSMIT(0xa4); /* sync */                                 \
	TRANSMIT(9); /* length */                                  \
	TRANSMIT(0x4e); /* d78 */                                  \
	TRANSMIT(((uint8_t)channel));                              \
	TRANSMIT(0x20); /* d32 */                                  \
	TRANSMIT(((uint8_t)event_count_sum));                      \
	TRANSMIT((((uint16_t)slope)>>8));                          \
	TRANSMIT((((uint16_t)slope)&0xff));                        \
	TRANSMIT((((uint16_t)crank_period_sum)>>8));               \
	TRANSMIT((((uint16_t)crank_period_sum)&0xff));             \
	TRANSMIT((((uint16_t)torque_sum)>>8));                     \
	TRANSMIT((((uint16_t)torque_sum)&0xff));                   \
	SEND_CHECK();                                              \
} while (0);


// defines for message 'manufacturer'
#define SEND_MESSAGE_MANUFACTURER(channel,hw_rev,manufacturer_id,model_number_id) do {\
	TRANSMIT(0xa4); /* sync */                                 \
	TRANSMIT(9); /* length */                                  \
	TRANSMIT(0x4e); /* d78 */                                  \
	TRANSMIT(((uint8_t)channel));                              \
	TRANSMIT(0x50); /* d80 */                                  \
	TRANSMIT(0); /* unknown/don't care */                      \
	TRANSMIT(0); /* unknown/don't care */                      \
	TRANSMIT(((uint8_t)hw_rev));                               \
	TRANSMIT((((uint16_t)manufacturer_id)&0xff));              \
	TRANSMIT((((uint16_t)manufacturer_id)>>8));                \
	TRANSMIT((((uint16_t)model_number_id)&0xff));              \
	TRANSMIT((((uint16_t)model_number_id)>>8));                \
	SEND_CHECK();                                              \
} while (0);


// defines for message 'product'
#define SEND_MESSAGE_PRODUCT(channel,sw_rev,serial_number_qpod,serial_number_spider) do {\
	TRANSMIT(0xa4); /* sync */                                 \
	TRANSMIT(9); /* length */                                  \
	TRANSMIT(0x4e); /* d78 */                                  \
	TRANSMIT(((uint8_t)channel));                              \
	TRANSMIT(0x51); /* d81 */                                  \
	TRANSMIT(0); /* unknown/don't care */                      \
	TRANSMIT(0); /* unknown/don't care */                      \
	TRANSMIT(((uint8_t)sw_rev));                               \
	TRANSMIT((((uint16_t)serial_number_qpod)&0xff));           \
	TRANSMIT((((uint16_t)serial_number_qpod)>>8));             \
	TRANSMIT((((uint16_t)serial_number_spider)&0xff));         \
	TRANSMIT((((uint16_t)serial_number_spider)>>8));           \
	SEND_CHECK();                                              \
} while (0);


// defines for message 'battery_voltage'
#define SEND_MESSAGE_BATTERY_VOLTAGE(channel,operating_time_lsb,operating_time_1sb,operating_time_msb,voltage_lsb,descriptive_bits) do {\
	TRANSMIT(0xa4); /* sync */                                 \
	TRANSMIT(9); /* length */                                  \
	TRANSMIT(0x4e); /* d78 */                                  \
	TRANSMIT(((uint8_t)channel));                              \
	TRANSMIT(0x52); /* d82 */                                  \
	TRANSMIT(0); /* unknown/don't care */                      \
	TRANSMIT(0); /* unknown/don't care */                      \
	TRANSMIT(((uint8_t)operating_time_lsb));                   \
	TRANSMIT(((uint8_t)operating_time_1sb));                   \
	TRANSMIT(((uint8_t)operating_time_msb));                   \
	TRANSMIT(((uint8_t)voltage_lsb));                          \
	TRANSMIT(((uint8_t)descriptive_bits));                     \
	SEND_CHECK();                                              \
} while (0);


// defines for message 'heart_rate'
#define SEND_MESSAGE_HEART_RATE(channel,measurement_time_sum,beats_sum,instant_heart_rate) do {\
	TRANSMIT(0xa4); /* sync */                                 \
	TRANSMIT(9); /* length */                                  \
	TRANSMIT(0x4e); /* d78 */                                  \
	TRANSMIT(((uint8_t)channel));                              \
	TRANSMIT(0); /* unknown/don't care */                      \
	TRANSMIT(0); /* unknown/don't care */                      \
	TRANSMIT(0); /* unknown/don't care */                      \
	TRANSMIT(0); /* unknown/don't care */                      \
	TRANSMIT((((uint16_t)measurement_time_sum)&0xff));         \
	TRANSMIT((((uint16_t)measurement_time_sum)>>8));           \
	TRANSMIT(((uint8_t)beats_sum));                            \
	TRANSMIT(((uint8_t)instant_heart_rate));                   \
	SEND_CHECK();                                              \
} while (0);


// defines for message 'speed'
#define SEND_MESSAGE_SPEED(channel,measurement_time_sum,wheel_revs_sum) do {\
	TRANSMIT(0xa4); /* sync */                                 \
	TRANSMIT(9); /* length */                                  \
	TRANSMIT(0x4e); /* d78 */                                  \
	TRANSMIT(((uint8_t)channel));                              \
	TRANSMIT(0); /* unknown/don't care */                      \
	TRANSMIT(0); /* unknown/don't care */                      \
	TRANSMIT(0); /* unknown/don't care */                      \
	TRANSMIT(0); /* unknown/don't care */                      \
	TRANSMIT((((uint16_t)measurement_time_sum)&0xff));         \
	TRANSMIT((((uint16_t)measurement_time_sum)>>8));           \
	TRANSMIT((((uint16_t)wheel_revs_sum)&0xff));               \
	TRANSMIT((((uint16_t)wheel_revs_sum)>>8));                 \
	SEND_CHECK();                                              \
} while (0);


// defines for message 'cadence'
#define SEND_MESSAGE_CADENCE(channel,measurement_time_sum,crank_revs_sum) do {\
	TRANSMIT(0xa4); /* sync */                                 \
	TRANSMIT(9); /* length */                                  \
	TRANSMIT(0x4e); /* d78 */                                  \
	TRANSMIT(((uint8_t)channel));                              \
	TRANSMIT(0); /* unknown/don't care */                      \
	TRANSMIT(0); /* unknown/don't care */                      \
	TRANSMIT(0); /* unknown/don't care */                      \
	TRANSMIT(0); /* unknown/don't care */                      \
	TRANSMIT((((uint16_t)measurement_time_sum)&0xff));         \
	TRANSMIT((((uint16_t)measurement_time_sum)>>8));           \
	TRANSMIT((((uint16_t)crank_revs_sum)&0xff));               \
	TRANSMIT((((uint16_t)crank_revs_sum)>>8));                 \
	SEND_CHECK();                                              \
} while (0);


// defines for message 'speed_cadence'
#define SEND_MESSAGE_SPEED_CADENCE(channel,cadence_measurement_time_sum,crank_revs_sum,speed_measurement_time_sum,wheel_revs_sum) do {\
	TRANSMIT(0xa4); /* sync */                                 \
	TRANSMIT(9); /* length */                                  \
	TRANSMIT(0x4e); /* d78 */                                  \
	TRANSMIT(((uint8_t)channel));                              \
	TRANSMIT((((uint16_t)cadence_measurement_time_sum)&0xff)); \
	TRANSMIT((((uint16_t)cadence_measurement_time_sum)>>8));   \
	TRANSMIT((((uint16_t)crank_revs_sum)&0xff));               \
	TRANSMIT((((uint16_t)crank_revs_sum)>>8));                 \
	TRANSMIT((((uint16_t)speed_measurement_time_sum)&0xff));   \
	TRANSMIT((((uint16_t)speed_measurement_time_sum)>>8));     \
	TRANSMIT((((uint16_t)wheel_revs_sum)&0xff));               \
	TRANSMIT((((uint16_t)wheel_revs_sum)>>8));                 \
	SEND_CHECK();                                              \
} while (0);
