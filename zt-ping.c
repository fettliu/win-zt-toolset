/**
 * ZeroTier ICMP Ping Tool
 * 
 * Uses libzt Raw Socket API to send ICMP Echo Request directly
 * No virtual network adapter or admin privileges required
 * 
 * Usage: zt-ping.exe [-d <data_dir>] <target_ip>
 * 
 * Examples:
 *   zt-ping.exe 172.22.1.2
 *   zt-ping.exe -d ./zt-data 172.22.1.2
 */

#include "ZeroTierSockets.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#define sleep(x) Sleep((x) * 1000)
#else
#include <unistd.h>
#include <arpa/inet.h>
#endif

// Default Network ID (can be overridden by environment variable ZT_NETWORK_ID)
#ifndef DEFAULT_NETWORK_ID
#define DEFAULT_NETWORK_ID 0x6ab565387a03bfc3ULL
#endif

// ICMP Header Structure
typedef struct {
    uint8_t type;        // Type (8 = Echo Request, 0 = Echo Reply)
    uint8_t code;        // Code
    uint16_t checksum;   // Checksum
    uint16_t id;         // Identifier
    uint16_t sequence;   // Sequence Number
} icmp_header_t;

// Calculate Internet Checksum
uint16_t calculate_checksum(uint16_t* data, int length) {
    uint32_t sum = 0;
    
    while (length > 1) {
        sum += *data++;
        length -= 2;
    }
    
    if (length > 0) {
        sum += *(uint8_t*)data;
    }
    
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    
    return (uint16_t)(~sum);
}

// Event Callback
void on_zerotier_event(void* msg_ptr) {
    zts_event_msg_t* msg = (zts_event_msg_t*)msg_ptr;
    
    switch(msg->event_code) {
        case ZTS_EVENT_NODE_UP:
            printf("[OK] Node initialized\n");
            break;
        case ZTS_EVENT_NODE_ONLINE:
            printf("[OK] Node online\n");
            break;
        case ZTS_EVENT_NETWORK_READY_IP4:
            if (msg->network) {
                printf("[OK] IPv4 network ready (network: %llx)\n", msg->network->net_id);
            }
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
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if ((strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--data") == 0) && i + 1 < argc) {
            storage_path = argv[++i];
        } else if (target_ip == NULL) {
            target_ip = argv[i];
        } else {
            fprintf(stderr, "[ERROR] Unknown argument: %s\n", argv[i]);
            fprintf(stderr, "Usage: %s [-d <data_dir>] <target_ip>\n", argv[0]);
            return 1;
        }
    }
    
    if (!target_ip) {
        printf("Usage: %s [-d <data_dir>] <target_ip>\n", argv[0]);
        printf("\nOptions:\n");
        printf("  -d, --data <path>     Data directory (default: ./zt-data)\n");
        printf("\nExamples:\n");
        printf("  %s 172.22.1.2\n", argv[0]);
        printf("  %s -d ./zt-data 172.22.1.2\n", argv[0]);
        return 1;
    }
    
    // Use default network ID from macro
    uint64_t network_id = DEFAULT_NETWORK_ID;
    char network_id_str[17]; // 16 hex chars + null terminator
    sprintf(network_id_str, "%016llx", (unsigned long long)network_id);
    
    printf("========================================\n");
    printf("  ZeroTier ICMP Ping Tool\n");
    printf("========================================\n\n");
    
    printf("Storage Path: %s\n", storage_path);
    printf("Network ID:   %s\n", network_id_str);
    printf("Target IP:    %s\n\n", target_ip);
    
    // Initialize ZeroTier
    printf("[INIT] Starting ZeroTier...\n");
    zts_init_from_storage(storage_path);
    
    // Set up event handler
    zts_init_set_event_handler(on_zerotier_event);
    
    if (zts_node_start() != ZTS_ERR_OK) {
        printf("[ERROR] Unable to start node\n");
        return 1;
    }
    
    // Wait for node to come online
    printf("[WAIT] Waiting for node to come online...\n");
    int wait_count = 0;
    while (!zts_node_is_online() && wait_count < 30) {
        sleep(1);
        wait_count++;
        printf(".");
        fflush(stdout);
    }
    printf("\n");
    
    if (!zts_node_is_online()) {
        printf("[ERROR] Node online timeout\n");
        return 1;
    }
    
    printf("[OK] Node ID: %llx\n\n", zts_node_get_id());
    
    // Join Network
    printf("[JOIN] Joining network %s...\n", network_id_str);
    if (zts_net_join(network_id) != ZTS_ERR_OK) {
        printf("[ERROR] Unable to join network\n");
        return 1;
    }
    
    // Wait for network to be ready
    printf("[WAIT] Waiting for network to be ready...\n");
    wait_count = 0;
    while (!zts_net_transport_is_ready(network_id) && wait_count < 60) {
        sleep(1);
        wait_count++;
        printf(".");
        fflush(stdout);
    }
    printf("\n");
    
    if (!zts_net_transport_is_ready(network_id)) {
        printf("[ERROR] Network connection timeout\n");
        printf("[HINT] Ensure node is authorized at https://my.zerotier.com/network/%016llx\n", network_id);
        return 1;
    }
    
    printf("[OK] Network ready\n\n");
    
    // Create Raw Socket
    printf("[PING] Starting to send ICMP Echo Request...\n\n");
    
    int sock = zts_socket(ZTS_AF_INET, ZTS_SOCK_RAW, ZTS_IPPROTO_ICMP);
    if (sock < 0) {
        printf("[ERROR] Unable to create Raw Socket (error code: %d)\n", sock);
        return 1;
    }
    
    printf("[OK] Raw Socket created (fd: %d)\n\n", sock);
    
    // Set Destination Address
    struct zts_sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = ZTS_AF_INET;
    dest_addr.sin_port = 0;  // ICMP does not use ports
    zts_inet_pton(ZTS_AF_INET, target_ip, &dest_addr.sin_addr);
    
    // Send 4 ping packets
    int packets_sent = 0;
    int packets_received = 0;
    
    for (int i = 0; i < 4; i++) {
        // Build ICMP Echo Request
        char packet[64];
        memset(packet, 0, sizeof(packet));
        
        icmp_header_t* icmp = (icmp_header_t*)packet;
        icmp->type = 8;  // Echo Request
        icmp->code = 0;
        icmp->id = (uint16_t)zts_node_get_id() & 0xFFFF;
        icmp->sequence = i;
        icmp->checksum = 0;
        
        // Fill Data
        memset(packet + sizeof(icmp_header_t), 'A', sizeof(packet) - sizeof(icmp_header_t));
        
        // Calculate Checksum
        icmp->checksum = calculate_checksum((uint16_t*)packet, sizeof(packet));
        
        // Send
        printf("Sending ICMP Echo Request to %s (seq: %d)... ", target_ip, i);
        fflush(stdout);
        
        int send_time = GetTickCount();
        ssize_t sent = zts_bsd_sendto(sock, packet, sizeof(packet), 0, 
                                      (struct zts_sockaddr*)&dest_addr, sizeof(dest_addr));
        
        if (sent < 0) {
            printf("[ERROR] Send failed (error code: %zd)\n", sent);
            continue;
        }
        
        packets_sent++;
        
        // Receive Response (timeout 3 seconds)
        struct zts_timeval timeout;
        timeout.tv_sec = 3;
        timeout.tv_usec = 0;
        zts_bsd_setsockopt(sock, ZTS_SOL_SOCKET, ZTS_SO_RCVTIMEO, &timeout, sizeof(timeout));
        
        char recv_buf[128];
        struct zts_sockaddr_in src_addr;
        zts_socklen_t addr_len = sizeof(src_addr);
        
        ssize_t received = zts_bsd_recvfrom(sock, recv_buf, sizeof(recv_buf), 0,
                                            (struct zts_sockaddr*)&src_addr, &addr_len);
        
        if (received > 0) {
            int rtt = GetTickCount() - send_time;
            
            // Validate ICMP Response
            icmp_header_t* resp_icmp = (icmp_header_t*)recv_buf;
            if (resp_icmp->type == 0 && resp_icmp->id == icmp->id && resp_icmp->sequence == i) {
                printf("[OK] Reply from %s: seq=%d ttl=64 time=%d ms\n", target_ip, i, rtt);
                packets_received++;
            } else {
                printf("[WARN] Received unknown packet\n");
            }
        } else {
            printf("[TIMEOUT] Request timed out for seq=%d\n", i);
        }
        
        sleep(1);  // Interval 1 second
    }
    
    // Close socket
    zts_close(sock);
    
    // Statistics
    printf("\n========================================\n");
    printf("  Ping Statistics\n");
    printf("========================================\n");
    printf("%d packets transmitted, %d received, %.0f%% packet loss\n", 
           packets_sent, packets_received,
           packets_sent > 0 ? ((float)(packets_sent - packets_received) / packets_sent * 100) : 0);
    printf("========================================\n\n");
    
    if (packets_received > 0) {
        printf("[SUCCESS] Ping test successful! Traffic routed through ZeroTier tunnel\n");
    } else {
        printf("[FAILED] Ping test failed\n");
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
