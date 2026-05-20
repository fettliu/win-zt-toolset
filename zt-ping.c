#include "ZeroTierSockets.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#define sleep(x) Sleep((x) * 1000)
#else
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/time.h>
#endif

// Get error message
#ifdef _WIN32
#define GET_LAST_ERROR() WSAGetLastError()
#else
#define GET_LAST_ERROR() errno
#endif

// Default Network ID (can be overridden by environment variable ZT_NETWORK_ID)
#ifndef DEFAULT_NETWORK_ID
#define DEFAULT_NETWORK_ID 0x6ab565387a03bfc3ULL
#endif

// Global Variables
static volatile int got_ip = 0;
static char ip_str[ZTS_IP_MAX_STR_LEN] = {0};

// ICMP Header Structure
typedef struct {
    uint8_t type;        // Type (8 = Echo Request, 0 = Echo Reply)
    uint8_t code;        // Code
    uint16_t checksum;   // Checksum
    uint16_t id;         // Identifier
    uint16_t sequence;   // Sequence Number
} icmp_header_t;

// Get current time in microseconds (high precision)
static long long get_time_us() {
#ifdef _WIN32
    static LARGE_INTEGER frequency = {0};
    LARGE_INTEGER counter;
    
    if (frequency.QuadPart == 0) {
        QueryPerformanceFrequency(&frequency);
    }
    
    QueryPerformanceCounter(&counter);
    return (long long)(counter.QuadPart * 1000000LL / frequency.QuadPart);
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (long long)(tv.tv_sec * 1000000LL + tv.tv_usec);
#endif
}

// Get current time in milliseconds (for compatibility)
static long long get_time_ms() {
    return get_time_us() / 1000;
}

// Calculate Internet Checksum (RFC 1071)
uint16_t calculate_checksum(uint16_t* data, int length) {
    uint32_t sum = 0;
    
    // Sum all 16-bit words
    while (length > 1) {
        sum += *data++;
        length -= 2;
    }
    
    // Add left-over byte, if any
    if (length > 0) {
        sum += *(uint8_t*)data;
    }
    
    // Fold 32-bit sum to 16 bits
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    
    // One's complement
    return (uint16_t)(~sum);
}

// Event Callback
void on_zerotier_event(void* msg_ptr) {
    zts_event_msg_t* msg = (zts_event_msg_t*)msg_ptr;
    
    switch(msg->event_code) {
        case ZTS_EVENT_NODE_UP:
            // Silent - node initialized
            break;
        case ZTS_EVENT_NODE_ONLINE:
            // Silent - node online
            break;
        case ZTS_EVENT_ADDR_ADDED_IP4:
            if (msg->addr != NULL) {
                struct zts_sockaddr_in* in = (struct zts_sockaddr_in*)&(msg->addr->addr);
                zts_inet_ntop(ZTS_AF_INET, &(in->sin_addr), ip_str, ZTS_IP_MAX_STR_LEN);
                got_ip = 1;
                printf("[OK] Got IPv4 address: %s\n", ip_str);
            }
            break;
        case ZTS_EVENT_NETWORK_READY_IP4:
            // Silent - network ready (will be printed by main thread)
            break;
        case ZTS_EVENT_NETWORK_ACCESS_DENIED:
            printf("[ERROR] Network access denied (authorize node at my.zerotier.com)\n");
            break;
        default:
            break;
    }
}

int main(int argc, char* argv[]) {
    // Set console output code page to UTF-8
    #ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    #endif
    
    const char* storage_path = "./zt-data";
    const char* target_ip = NULL;
    int ping_count = -1;  // -1 means infinite ping
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if ((strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--data") == 0) && i + 1 < argc) {
            storage_path = argv[++i];
        } else if (strcmp(argv[i], "-c") == 0 && i + 1 < argc) {
            ping_count = atoi(argv[++i]);
            if (ping_count <= 0) {
                printf("Error: Invalid count value\n");
                return 1;
            }
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            printf("Usage: %s [-d <data_dir>] [-c <count>] <target_ip>\n", argv[0]);
            printf("\nOptions:\n");
            printf("  -d, --data <path>     Data directory (default: ./zt-data)\n");
            printf("  -c <count>            Number of packets to send (default: infinite)\n");
            printf("\nExamples:\n");
            printf("  %s 172.22.1.2              # Infinite ping\n", argv[0]);
            printf("  %s -c 4 172.22.1.2         # Send 4 packets\n", argv[0]);
            printf("  %s -d ./zt-data 172.22.1.2\n", argv[0]);
            return 0;
        } else {
            target_ip = argv[i];
        }
    }
    
    if (!target_ip) {
        printf("Usage: %s [-d <data_dir>] [-c <count>] <target_ip>\n", argv[0]);
        printf("\nOptions:\n");
        printf("  -d, --data <path>     Data directory (default: ./zt-data)\n");
        printf("  -c <count>            Number of packets to send (default: infinite)\n");
        printf("\nExamples:\n");
        printf("  %s 172.22.1.2              # Infinite ping\n", argv[0]);
        printf("  %s -c 4 172.22.1.2         # Send 4 packets\n", argv[0]);
        printf("  %s -d ./zt-data 172.22.1.2\n", argv[0]);
        return 1;
    }
    
    // Use default network ID from macro
    uint64_t network_id = DEFAULT_NETWORK_ID;
    char network_id_str[17]; // 16 hex chars + null terminator
    sprintf(network_id_str, "%016llx", (unsigned long long)network_id);
    
    // Initialize ZeroTier (minimal output)
    zts_init_from_storage(storage_path);
    zts_init_set_event_handler(on_zerotier_event);
    
    if (zts_node_start() != ZTS_ERR_OK) {
        printf("[ERROR] Unable to start node\n");
        return 1;
    }
    
    // Wait for node to come online (silent wait)
    int wait_count = 0;
    while (!zts_node_is_online() && wait_count < 30) {
        sleep(1);
        wait_count++;
    }
    
    if (!zts_node_is_online()) {
        printf("[ERROR] Node online timeout\n");
        return 1;
    }
    
    // Join Network (silent join)
    if (zts_net_join(network_id) != ZTS_ERR_OK) {
        printf("[ERROR] Unable to join network\n");
        return 1;
    }
    
    // Wait for network to be ready (silent wait)
    wait_count = 0;
    while (!zts_net_transport_is_ready(network_id) && wait_count < 60) {
        sleep(1);
        wait_count++;
    }
    
    if (!zts_net_transport_is_ready(network_id)) {
        printf("[ERROR] Network connection timeout\n");
        printf("[HINT] Ensure node is authorized at https://my.zerotier.com/network/%016llx\n", network_id);
        return 1;
    }
    
    // Wait for IP address (silent wait)
    wait_count = 0;
    while (!got_ip && wait_count < 30) {
        sleep(1);
        wait_count++;
    }
    
    if (!got_ip) {
        printf("[ERROR] Failed to obtain IP address\n");
        printf("[HINT] Ensure node is authorized at https://my.zerotier.com/network/%016llx\n", network_id);
        return 1;
    }
    
    printf("[OK] Node ID: %llx\n", zts_node_get_id());
    printf("[OK] IPv4 network ready (network: %s)\n\n", network_id_str);
    
    // Create Raw Socket with retry mechanism
    int sock = -1;
    int retry_count = 0;
    const int max_retries = 3;
    
    while (sock < 0 && retry_count < max_retries) {
        sock = zts_socket(ZTS_AF_INET, ZTS_SOCK_RAW, ZTS_IPPROTO_ICMP);
        if (sock < 0) {
            retry_count++;
            if (retry_count < max_retries) {
                sleep(2);
            }
        }
    }
    
    if (sock < 0) {
        printf("[ERROR] Unable to create Raw Socket after %d attempts (error code: %d)\n", 
               max_retries, sock);
        printf("[HINT] Possible causes:\n");
        printf("  1. Windows requires Administrator privileges for RAW sockets\n");
        printf("  2. ZeroTier node is not fully initialized\n");
        printf("  3. Network interface is not ready\n");
        printf("\n[SOLUTION] Try running as Administrator:\n");
        printf("  Right-click -> Run as Administrator\n");
        return 1;
    }
    
    // Set Destination Address
    struct zts_sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = ZTS_AF_INET;
    dest_addr.sin_port = 0;  // ICMP does not use ports
    zts_inet_pton(ZTS_AF_INET, target_ip, &dest_addr.sin_addr);
    
    // Send ping packets
    int packets_sent = 0;
    int packets_received = 0;
    int sequence = 0;
    
    // RTT statistics (in microseconds for precision)
    long long rtt_min_us = LLONG_MAX;
    long long rtt_max_us = 0;
    long long rtt_total_us = 0;
    
    printf("\n");
    
    while (ping_count < 0 || packets_sent < ping_count) {
        // Build ICMP Echo Request
        char packet[64];
        memset(packet, 0, sizeof(packet));
        
        icmp_header_t* icmp = (icmp_header_t*)packet;
        icmp->type = 8;  // Echo Request
        icmp->code = 0;
        icmp->id = htons((uint16_t)zts_node_get_id() & 0xFFFF);
        icmp->sequence = htons(sequence);
        icmp->checksum = 0;
        
        // Fill Data
        memset(packet + sizeof(icmp_header_t), 'A', sizeof(packet) - sizeof(icmp_header_t));
        
        // Calculate Checksum (on entire packet with checksum=0)
        icmp->checksum = calculate_checksum((uint16_t*)packet, sizeof(packet));
        
        // Send
        long long send_time = get_time_us();
        ssize_t sent = zts_bsd_sendto(sock, packet, sizeof(packet), 0, 
                                      (struct zts_sockaddr*)&dest_addr, sizeof(dest_addr));
        
        if (sent < 0) {
            printf("[ERROR] Send failed (error code: %zd)\n", sent);
            sleep(1);
            continue;
        }
        
        packets_sent++;
        
        // Receive Response (timeout 5 seconds for ZeroTier latency)
        struct zts_timeval timeout;
        timeout.tv_sec = 5;
        timeout.tv_usec = 0;
        zts_bsd_setsockopt(sock, ZTS_SOL_SOCKET, ZTS_SO_RCVTIMEO, &timeout, sizeof(timeout));
        
        char recv_buf[128];
        struct zts_sockaddr_in src_addr;
        zts_socklen_t addr_len = sizeof(src_addr);
        
        ssize_t received = zts_bsd_recvfrom(sock, recv_buf, sizeof(recv_buf), 0,
                                            (struct zts_sockaddr*)&src_addr, &addr_len);
        
        if (received > 0) {
            long long rtt_us = get_time_us() - send_time;
            double rtt_ms = rtt_us / 1000.0;
            
            // lwIP returns full IP packet (IP header + ICMP payload)
            // Check if we have IP header (usually 20 bytes)
            int icmp_offset = 0;
            if (received > 20) {
                // Check if first byte looks like IPv4 header (version=4, IHL=5)
                unsigned char first_byte = (unsigned char)recv_buf[0];
                if ((first_byte >> 4) == 4) {
                    // This is an IP packet, skip IP header
                    unsigned char ihl = (first_byte & 0x0F) * 4;  // IP header length in bytes
                    icmp_offset = ihl;
                }
            }
            
            // Parse ICMP header
            if (received > icmp_offset + sizeof(icmp_header_t)) {
                icmp_header_t* resp_icmp = (icmp_header_t*)(recv_buf + icmp_offset);
                
                // Validate ICMP Echo Reply
                // Note: resp_icmp fields are in network byte order, need to convert
                uint16_t resp_id = ntohs(resp_icmp->id);
                uint16_t resp_seq = ntohs(resp_icmp->sequence);
                
                if (resp_icmp->type == 0 &&  // Echo Reply
                    resp_id == (uint16_t)(zts_node_get_id() & 0xFFFF) &&
                    resp_seq == sequence) {
                    
                    // Update RTT stats (in microseconds)
                    if (rtt_us < rtt_min_us) rtt_min_us = rtt_us;
                    if (rtt_us > rtt_max_us) rtt_max_us = rtt_us;
                    rtt_total_us += rtt_us;
                    
                    packets_received++;
                    printf("64 bytes from %s: icmp_seq=%d ttl=64 time=%.3f ms\n", target_ip, sequence, rtt_ms);
                } else {
                    // Received packet doesn't match current request, might be out of order or duplicate
                    printf("[INFO] Received mismatched packet (type=%d, id=%u, seq=%u), expected seq=%d\n", 
                           resp_icmp->type, resp_id, resp_seq, sequence);
                }
            }
        } else {
            printf("Request timeout for icmp_seq %d\n", sequence);
        }
        
        sequence++;
        sleep(1);  // Interval 1 second
    }
    
    // Close socket
    zts_close(sock);
    
    // Statistics (Linux ping format)
    printf("\n--- %s ping statistics ---\n", target_ip);
    printf("%d packets transmitted, %d received, %.0f%% packet loss\n", 
           packets_sent, packets_received,
           packets_sent > 0 ? ((float)(packets_sent - packets_received) / packets_sent * 100) : 0);
    
    if (packets_received > 0) {
        int rtt_avg_us = (int)(rtt_total_us / packets_received);
        printf("rtt min/avg/max = %.3f/%.3f/%.3f ms\n", rtt_min_us / 1000.0, rtt_avg_us / 1000.0, rtt_max_us / 1000.0);
    } else {
        printf("[HINT] Possible reasons:\n");
        printf("  - Node not authorized at my.zerotier.com\n");
        printf("  - Target device offline or not joined to network\n");
        printf("  - Target device firewall blocking ICMP\n");
    }
    
    // Cleanup
    zts_net_leave(network_id);
    zts_node_stop();
    
    return packets_received > 0 ? 0 : 1;
}
