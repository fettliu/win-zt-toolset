// ZeroTier Port Forwarder for Windows
// Usage: zt-forward.exe tcp:<local_port>:<target_ip>:<target_port> [tcp:...]

#define _WIN32_WINNT 0x0600
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ZeroTierSockets.h"

#define DEFAULT_DATA_DIR "./zt-data"
#define MAX_RULES 10
#define BUFFER_SIZE 4096

typedef struct {
    int local_port;
    char target_ip[64];
    int target_port;
    int protocol;  // 0 = TCP, 1 = UDP
} forward_rule_t;

static forward_rule_t rules[MAX_RULES];
static int rule_count = 0;

// Thread data structure for TCP
typedef struct {
    SOCKET client_fd;
    int rule_index;
} thread_data_t;

// Forward declarations
DWORD WINAPI tcp_forward_thread(LPVOID param);
DWORD WINAPI udp_forward_thread(LPVOID param);

static void print_usage(const char* program_name) {
    printf("Usage: %s [-d <data_dir>] <protocol>:<local_port>:<target_ip>:<target_port> [...]\n", program_name);
    printf("\nOptions:\n");
    printf("  -d, --data <path>     Data directory (default: ./zt-data)\n");
    printf("\nProtocols:\n");
    printf("  tcp - TCP port forwarding\n");
    printf("  udp - UDP port forwarding\n");
    printf("\nExamples:\n");
    printf("  %s tcp:2222:172.22.1.17:22\n", program_name);
    printf("  %s -d ./zt-data tcp:8080:172.22.1.18:80 udp:5353:172.22.1.19:53\n", program_name);
}

static int parse_rule(const char* rule_str, forward_rule_t* rule) {
    // Format: protocol:local_port:target_ip:target_port
    if (strncmp(rule_str, "tcp:", 4) == 0) {
        rule->protocol = 0;  // TCP
    } else if (strncmp(rule_str, "udp:", 4) == 0) {
        rule->protocol = 1;  // UDP
    } else {
        return -1;
    }
    
    char temp[256];
    strncpy(temp, rule_str + 4, sizeof(temp) - 1);
    temp[sizeof(temp) - 1] = '\0';
    
    char* token = strtok(temp, ":");
    if (!token) return -1;
    rule->local_port = atoi(token);
    
    token = strtok(NULL, ":");
    if (!token) return -1;
    strncpy(rule->target_ip, token, sizeof(rule->target_ip) - 1);
    
    token = strtok(NULL, ":");
    if (!token) return -1;
    rule->target_port = atoi(token);
    
    return 0;
}

DWORD WINAPI tcp_forward_thread(LPVOID param) {
    thread_data_t* data = (thread_data_t*)param;
    SOCKET client_fd = data->client_fd;
    forward_rule_t* rule = &rules[data->rule_index];
    
    free(data);
    
    printf("[INFO] New TCP connection from client\n");
    printf("[INFO] -> [ZeroTier] %s:%d\n", rule->target_ip, rule->target_port);
    
    // Connect to target via ZeroTier using simplified API
    int dest_fd = zts_tcp_client(rule->target_ip, rule->target_port);
    if (dest_fd < 0) {
        fprintf(stderr, "[ERROR] Failed to connect to %s:%d (error: %d)\n", 
                rule->target_ip, rule->target_port, dest_fd);
        closesocket(client_fd);
        return 0;
    }
    
    printf("[INFO] TCP connection established, forwarding data...\n");
    
    // Bidirectional forwarding
    char buffer[BUFFER_SIZE];
    fd_set readfds;
    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    
    while (1) {
        FD_ZERO(&readfds);
        
        // Note: On Windows, we cannot use select() with ZeroTier sockets
        // Using simple polling approach instead
        int bytes = recv(client_fd, buffer, sizeof(buffer), 0);
        if (bytes <= 0) {
            break;
        }
        
        if (zts_send(dest_fd, buffer, bytes, 0) <= 0) {
            break;
        }
        
        bytes = zts_recv(dest_fd, buffer, sizeof(buffer), 0);
        if (bytes <= 0) {
            break;
        }
        
        if (send(client_fd, buffer, bytes, 0) <= 0) {
            break;
        }
    }
    
    printf("[INFO] TCP connection closed\n");
    zts_close(dest_fd);
    closesocket(client_fd);
    
    return 0;
}

DWORD WINAPI udp_forward_thread(LPVOID param) {
    thread_data_t* data = (thread_data_t*)param;
    SOCKET client_fd = data->client_fd;
    forward_rule_t* rule = &rules[data->rule_index];
    
    free(data);
    
    printf("[INFO] UDP forwarding started for %s:%d\n", rule->target_ip, rule->target_port);
    
    // Create UDP socket for ZeroTier
    int dest_fd = zts_socket(ZTS_AF_INET, ZTS_SOCK_DGRAM, 0);
    if (dest_fd < 0) {
        fprintf(stderr, "[ERROR] Failed to create UDP socket (error: %d)\n", dest_fd);
        closesocket(client_fd);
        return 0;
    }
    
    // Set up destination address
    struct zts_sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = ZTS_AF_INET;
    dest_addr.sin_port = htons(rule->target_port);
    zts_inet_pton(ZTS_AF_INET, rule->target_ip, &dest_addr.sin_addr);
    
    char buffer[BUFFER_SIZE];
    struct sockaddr_in client_addr;
    int addr_len;  // Use int for Windows recvfrom
    
    printf("[INFO] UDP forwarding ready\n");
    
    // UDP forwarding loop
    while (1) {
        // Receive from local client
        addr_len = sizeof(client_addr);
        int bytes = recvfrom(client_fd, buffer, sizeof(buffer), 0, 
                            (struct sockaddr*)&client_addr, &addr_len);
        if (bytes <= 0) {
            break;
        }
        
        // Send to ZeroTier target
        ssize_t sent = zts_bsd_sendto(dest_fd, buffer, bytes, 0,
                                     (struct zts_sockaddr*)&dest_addr, sizeof(dest_addr));
        if (sent <= 0) {
            fprintf(stderr, "[ERROR] Failed to send UDP packet via ZeroTier\n");
            continue;
        }
        
        // Receive response from ZeroTier
        struct zts_sockaddr_in response_addr;
        zts_socklen_t resp_addr_len = sizeof(response_addr);
        bytes = zts_bsd_recvfrom(dest_fd, buffer, sizeof(buffer), 0,
                                (struct zts_sockaddr*)&response_addr, &resp_addr_len);
        if (bytes > 0) {
            // Send response back to local client
            sendto(client_fd, buffer, bytes, 0,
                  (struct sockaddr*)&client_addr, addr_len);
        }
    }
    
    printf("[INFO] UDP forwarding stopped\n");
    zts_close(dest_fd);
    closesocket(client_fd);
    
    return 0;
}

int main(int argc, char* argv[]) {
    // Set console output code page to UTF-8 for correct character display
    SetConsoleOutputCP(CP_UTF8);

    const char* data_dir = "./zt-data";
    int rule_start_index = 1;
    
    // Parse command line options
    for (int i = 1; i < argc; i++) {
        if ((strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--data") == 0) && i + 1 < argc) {
            data_dir = argv[++i];
            rule_start_index = i + 1;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strncmp(argv[i], "-", 1) != 0) {
            break;
        }
    }
    
    if (argc <= rule_start_index) {
        print_usage(argv[0]);
        return 1;
    }
    
    printf("[INFO] ========================================\n");
    printf("[INFO]   ZeroTier Port Forwarder\n");
    printf("[INFO] ========================================\n");
    printf("[INFO] Data directory: %s\n", data_dir);
    
    // Parse rules
    for (int i = rule_start_index; i < argc && rule_count < MAX_RULES; i++) {
        if (parse_rule(argv[i], &rules[rule_count]) == 0) {
            const char* proto_str = rules[rule_count].protocol == 0 ? "TCP" : "UDP";
            printf("[INFO] Rule %d: %s 0.0.0.0:%d -> %s:%d\n", 
                   rule_count + 1, 
                   proto_str,
                   rules[rule_count].local_port,
                   rules[rule_count].target_ip,
                   rules[rule_count].target_port);
            rule_count++;
        } else {
            fprintf(stderr, "[ERROR] Invalid rule format: %s\n", argv[i]);
            return 1;
        }
    }
    
    // Initialize Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        fprintf(stderr, "[ERROR] Failed to initialize Winsock\n");
        return 1;
    }
    
    // Initialize ZeroTier
    printf("[INFO] Initializing ZeroTier...\n");
    int err = zts_init_from_storage(data_dir);
    if (err != ZTS_ERR_OK) {
        fprintf(stderr, "[ERROR] Failed to initialize ZeroTier (error: %d)\n", err);
        WSACleanup();
        return 1;
    }
    
    zts_init_allow_port_mapping(0);
    
    printf("[INFO] Starting ZeroTier node...\n");
    if (zts_node_start() != ZTS_ERR_OK) {
        fprintf(stderr, "[ERROR] Failed to start ZeroTier node\n");
        WSACleanup();
        return 1;
    }
    
    // Wait for node to come online
    printf("[INFO] Waiting for node to come online...\n");
    int wait_count = 0;
    while (!zts_node_is_online() && wait_count < 30) {
        Sleep(500);
        wait_count++;
        printf(".");
        fflush(stdout);
    }
    printf("\n");
    
    if (!zts_node_is_online()) {
        fprintf(stderr, "[ERROR] Node online timeout\n");
        zts_node_stop();
        WSACleanup();
        return 1;
    }
    
    printf("[INFO] ZeroTier node is online. ID: %llx\n\n", zts_node_get_id());
    
    // Create listening sockets
    SOCKET listeners[MAX_RULES];
    for (int i = 0; i < rule_count; i++) {
        listeners[i] = socket(AF_INET, rules[i].protocol == 0 ? SOCK_STREAM : SOCK_DGRAM, 0);
        if (listeners[i] == INVALID_SOCKET) {
            fprintf(stderr, "[ERROR] Failed to create socket for rule %d\n", i + 1);
            WSACleanup();
            return 1;
        }
        
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(rules[i].local_port);
        
        if (bind(listeners[i], (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
            fprintf(stderr, "[ERROR] Failed to bind port %d\n", rules[i].local_port);
            closesocket(listeners[i]);
            WSACleanup();
            return 1;
        }
        
        if (rules[i].protocol == 0) {
            // TCP: start listening
            if (listen(listeners[i], 10) == SOCKET_ERROR) {
                fprintf(stderr, "[ERROR] Failed to listen on port %d\n", rules[i].local_port);
                closesocket(listeners[i]);
                WSACleanup();
                return 1;
            }
            printf("[INFO] TCP listening on 0.0.0.0:%d...\n", rules[i].local_port);
        } else {
            printf("[INFO] UDP listening on 0.0.0.0:%d...\n", rules[i].local_port);
        }
    }
    
    // Accept connections and start forwarding
    printf("[INFO] Ready to accept connections\n\n");
    
    while (1) {
        fd_set readfds;
        FD_ZERO(&readfds);
        
        SOCKET max_fd = 0;
        for (int i = 0; i < rule_count; i++) {
            FD_SET(listeners[i], &readfds);
            if (listeners[i] > max_fd) {
                max_fd = listeners[i];
            }
        }
        
        struct timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        
        int ret = select((int)max_fd + 1, &readfds, NULL, NULL, &timeout);
        if (ret <= 0) {
            continue;
        }
        
        for (int i = 0; i < rule_count; i++) {
            if (FD_ISSET(listeners[i], &readfds)) {
                if (rules[i].protocol == 0) {
                    // TCP: accept connection and start thread
                    SOCKET client_fd = accept(listeners[i], NULL, NULL);
                    if (client_fd != INVALID_SOCKET) {
                        thread_data_t* data = (thread_data_t*)malloc(sizeof(thread_data_t));
                        data->client_fd = client_fd;
                        data->rule_index = i;
                        
                        HANDLE hThread = CreateThread(NULL, 0, tcp_forward_thread, data, 0, NULL);
                        if (hThread) {
                            CloseHandle(hThread);
                        }
                    }
                } else {
                    // UDP: start forwarding thread directly
                    thread_data_t* data = (thread_data_t*)malloc(sizeof(thread_data_t));
                    data->client_fd = listeners[i];
                    data->rule_index = i;
                    
                    HANDLE hThread = CreateThread(NULL, 0, udp_forward_thread, data, 0, NULL);
                    if (hThread) {
                        CloseHandle(hThread);
                    }
                    
                    // Remove from select monitoring after starting UDP thread
                    FD_CLR(listeners[i], &readfds);
                }
            }
        }
    }
    
    // Cleanup (unreachable in this simplified version)
    for (int i = 0; i < rule_count; i++) {
        closesocket(listeners[i]);
    }
    zts_node_stop();
    WSACleanup();
    
    return 0;
}
