// ZeroTier Network Join Tool for Windows
// Usage: zt-join.exe [-d <data_dir>] -n <network_id>

#define _WIN32_WINNT 0x0600
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ZeroTierSockets.h"

#define DEFAULT_DATA_DIR "./zt-data"
#define TIMEOUT_SECONDS 30

static void print_usage(const char* program_name) {
    printf("Usage: %s [-d <data_dir>] -n <network_id>\n", program_name);
    printf("\nOptions:\n");
    printf("  -d, --data <path>       Configuration directory (default: %s)\n", DEFAULT_DATA_DIR);
    printf("  -n, --networkid <id>    ZeroTier network ID (16 hex digits)\n");
    printf("  -h, --help              Show this help message\n");
    printf("\nExamples:\n");
    printf("  %s -n abcdef1234567890\n", program_name);
    printf("  %s -d my-config -n abcdef1234567890\n", program_name);
    printf("  %s --data ./zt-data --networkid abcdef1234567890\n", program_name);
}

int main(int argc, char* argv[]) {
    const char* data_dir = DEFAULT_DATA_DIR;
    const char* network_id_str = NULL;
    unsigned long long network_id = 0;
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--data") == 0) {
            if (i + 1 < argc) {
                data_dir = argv[++i];
            } else {
                fprintf(stderr, "[ERROR] %s requires an argument\n", argv[i]);
                return 1;
            }
        } else if (strcmp(argv[i], "-n") == 0 || strcmp(argv[i], "--networkid") == 0) {
            if (i + 1 < argc) {
                network_id_str = argv[++i];
            } else {
                fprintf(stderr, "[ERROR] %s requires an argument\n", argv[i]);
                return 1;
            }
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else {
            fprintf(stderr, "[ERROR] Unknown option: %s\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }
    
    // If no network ID provided, use interactive mode
    if (!network_id_str) {
        printf("========================================\n");
        printf("  ZeroTier Network Join Tool\n");
        printf("========================================\n\n");
        
        char input[256];
        
        printf("Configuration directory [%s]: ", DEFAULT_DATA_DIR);
        fgets(input, sizeof(input), stdin);
        if (strlen(input) > 1) {
            input[strcspn(input, "\r\n")] = 0;
            if (strlen(input) > 0) {
                data_dir = strdup(input);
            }
        }
        
        printf("Network ID (16 hex digits): ");
        fgets(input, sizeof(input), stdin);
        input[strcspn(input, "\r\n")] = 0;
        network_id_str = input;
    }
    
    // Validate network ID
    if (!network_id_str) {
        fprintf(stderr, "[ERROR] Network ID is required\n");
        return 1;
    }
    
    // Create a mutable copy for validation
    char network_id_buf[64];
    strncpy(network_id_buf, network_id_str, sizeof(network_id_buf) - 1);
    network_id_buf[sizeof(network_id_buf) - 1] = '\0';
    
    // Remove trailing whitespace and newline characters
    size_t len = strlen(network_id_buf);
    while (len > 0 && (network_id_buf[len-1] == '\n' || 
                       network_id_buf[len-1] == '\r' || 
                       network_id_buf[len-1] == ' ')) {
        network_id_buf[--len] = '\0';
    }
    
    if (len != 16) {
        fprintf(stderr, "[ERROR] Network ID must be exactly 16 hexadecimal characters (got %zu chars)\n", len);
        fprintf(stderr, "[INFO] You entered: '%s'\n", network_id_buf);
        return 1;
    }
    
    // Use the cleaned string for further processing
    network_id_str = network_id_buf;
    
    // Convert hex string to unsigned long long
    char* endptr;
    network_id = strtoull(network_id_str, &endptr, 16);
    if (*endptr != '\0') {
        fprintf(stderr, "[ERROR] Invalid network ID format\n");
        return 1;
    }
    
    printf("[INFO] ========================================\n");
    printf("[INFO]   ZeroTier Network Join Tool\n");
    printf("[INFO] ========================================\n");
    printf("[INFO] Data directory: %s\n", data_dir);
    printf("[INFO] Network ID: %s\n", network_id_str);
    
    // Initialize Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        fprintf(stderr, "[ERROR] WSAStartup failed\n");
        return 1;
    }
    
    // Initialize node from storage
    printf("[INFO] Initializing node from storage...\n");
    int err = zts_init_from_storage(data_dir);
    if (err != ZTS_ERR_OK) {
        fprintf(stderr, "[ERROR] Failed to initialize node (error: %d)\n", err);
        WSACleanup();
        return 1;
    }
    
    // Disable port mapping
    printf("[INFO] Disabling port mapping...\n");
    zts_init_allow_port_mapping(0);
    
    // Start node
    printf("[INFO] Starting ZeroTier node...\n");
    if (zts_node_start() != ZTS_ERR_OK) {
        fprintf(stderr, "[ERROR] Failed to start ZeroTier node\n");
        WSACleanup();
        return 1;
    }
    
    // Wait for node to come online
    printf("[INFO] Waiting for node to come online...\n");
    while (!zts_node_is_online()) {
        Sleep(50);
    }
    printf("[INFO] Node is online. ID: %llx\n", (long long int)zts_node_get_id());
    
    // Join network
    printf("[INFO] Joining network %s...\n", network_id_str);
    if (zts_net_join(network_id) != ZTS_ERR_OK) {
        fprintf(stderr, "[ERROR] Failed to join network\n");
        zts_node_stop();
        WSACleanup();
        return 1;
    }
    
    // Wait for network ready
    printf("[INFO] Waiting for network to be ready...\n");
    while (!zts_net_transport_is_ready(network_id)) {
        Sleep(50);
    }
    
    // Get assigned IP address
    int family = ZTS_AF_INET;
    printf("[INFO] Waiting for IP address assignment (timeout: %ds)...\n", TIMEOUT_SECONDS);
    
    int timeout = TIMEOUT_SECONDS * 20; // Check every 50ms
    while (timeout-- > 0 && !zts_addr_is_assigned(network_id, family)) {
        Sleep(50);
    }
    
    if (zts_addr_is_assigned(network_id, family)) {
        char ip_str[ZTS_IP_MAX_STR_LEN] = {0};
        zts_addr_get_str(network_id, family, ip_str, ZTS_IP_MAX_STR_LEN);
        printf("[SUCCESS] Successfully joined network!\n");
        printf("[INFO] Your IP: %s\n", ip_str);
    } else {
        fprintf(stderr, "[ERROR] Timeout waiting for IP address\n");
        fprintf(stderr, "[INFO] Please check:\n");
        fprintf(stderr, "  1. Network ID is correct\n");
        fprintf(stderr, "  2. Node is authorized on my.zerotier.com\n");
        fprintf(stderr, "  3. Firewall allows UDP port 9993\n");
    }
    
    // Cleanup
    printf("\n[INFO] Press Enter to exit...\n");
    getchar();
    
    zts_net_leave(network_id);
    zts_node_stop();
    WSACleanup();
    
    return zts_addr_is_assigned(network_id, family) ? 0 : 1;
}
