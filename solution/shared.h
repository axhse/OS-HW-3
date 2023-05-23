#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define DEFAULT_CUSTOMER_PORT 9500
#define DEFAULT_SELLER_1_PORT 9501
#define DEFAULT_SELLER_2_PORT 9502
#define DEFAULT_OBSERVER_PORT 9510

#define MAX_PRODUCT_NAME_SIZE 100
#define MAX_PRODUCT_LIST_SIZE 100
#define MAX_ORDER_SIZE 100

#define RESPONSE_CODE_SUCCESS 200
#define RESPONSE_CODE_NOT_FOUND 404

#define FORMAT_ERROR "\033[91m"
#define FORMAT_WARNING "\033[93m"
#define FORMAT_INFO "\033[94m"
#define FORMAT_LOG "\033[96m"
#define FORMAT_NONE "\033[0m"


int print_string(const char *message, int size) {
    for (int index = 0; index < size; ++index) {
        printf("%c", message[index]);
    }
}

int log_error(const char *message) {
    printf(FORMAT_ERROR);
    printf("%s\n", message);
    printf(FORMAT_NONE);
}


int find_flag_index(int argc, const char *argv[], const char *flag_name) {
    for (int index = 1; index < argc; ++index) {
        if (strcmp(argv[index], flag_name) == 0) {
            return index;
        }
    }
    return argc;
}

int get_ip_address(int argc, const char *argv[], const char *flag_name) {
    int ip_address = htonl(INADDR_ANY);
    int param_index = find_flag_index(argc, argv, flag_name) + 1;
    if (param_index < argc) {
        ip_address = inet_addr(argv[param_index]);
        if (ip_address == -1) {
            ip_address = htonl(INADDR_ANY);
            printf(FORMAT_WARNING);
            printf("Invalid ip value %s. Default value INADDR_ANY is used\n", argv[param_index]);
            printf(FORMAT_NONE);
        }
    }
    return ip_address;
}

int get_port(int argc, const char *argv[], const char *flag_name, int default_value) {
    int port = default_value;
    int param_index = find_flag_index(argc, argv, flag_name) + 1;
    if (param_index < argc) {
        port = atoi(argv[param_index]);
        if (port < 1 || 65535 < port) {
            port = default_value;
            printf(FORMAT_WARNING);
            printf("Invalid port value %s. Default port %d is used\n", argv[param_index], default_value);
            printf(FORMAT_NONE);
        }
    }
    return port;
}

double get_pause_in_seconds(int argc, const char *argv[]) {
    double pause_in_seconds = 1;
    int param_index = find_flag_index(argc, argv, "--tempo") + 1;
    if (param_index < argc) {
        int tempo_param = atoi(argv[param_index]);
        if (1 <= tempo_param && tempo_param <= 10000) {
            pause_in_seconds = 0.001 * tempo_param;
        } else {
            printf(FORMAT_WARNING);
            printf("Invalid tempo value %s. Default tempo 1000 is used\n", argv[param_index]);
            printf(FORMAT_NONE);
        }
    }
    return pause_in_seconds;
}

int get_id(int argc, const char *argv[]) {
    int id = 0;
    int param_index = find_flag_index(argc, argv, "--id") + 1;
    if (param_index < argc) {
        id = atoi(argv[param_index]);
    }
    return id;
}


int read_product_list(int argc, const char *argv[],
                      char product_name_sizes[],
                      char product_names[][MAX_PRODUCT_NAME_SIZE]) {
    int param_index = find_flag_index(argc, argv, "--products") + 1;
    if (argc <= param_index) {
        printf(FORMAT_ERROR);
        printf("Product list file is not specified\n");
        printf(FORMAT_NONE);
        return -1;
    }
    const char *product_list_file_path = argv[param_index];
    FILE *product_list_file = fopen(product_list_file_path, "r");
    if (product_list_file == NULL) {
        printf(FORMAT_ERROR);
        printf("Product list file is not accessible\n");
        printf(FORMAT_NONE);
        return -1;
    }
    int product_count = 0;
    int current_name_size = 0;
    int is_in_product_name = 0;
    while (1) {
        char current_symbol = fgetc(product_list_file);
        if (current_symbol == '\r') {
            continue;
        }
        if (current_symbol == '\n') {
            is_in_product_name = 0;
            continue;
        }
        if (current_symbol == EOF) {
            break;
        }
        if (!is_in_product_name) {
            if (product_count == MAX_PRODUCT_LIST_SIZE) {
                printf(FORMAT_ERROR);
                printf("Product list has too many values\n");
                printf(FORMAT_NONE);
                fclose(product_list_file);
                return -1;
            }
            is_in_product_name = 1;
            ++product_count;
            current_name_size = 0;
        }
        if (current_name_size == MAX_PRODUCT_NAME_SIZE) {
            printf(FORMAT_ERROR);
            printf("Product name is too long\n");
            printf(FORMAT_NONE);
            fclose(product_list_file);
            return -1;
        }
        product_names[product_count - 1][current_name_size] = current_symbol;
        ++current_name_size;
        product_name_sizes[product_count - 1] = current_name_size;
    }
    fclose(product_list_file);
    return product_count;
}


struct sockaddr_in build_socket_address(in_addr_t ip_address, int port) {
    struct sockaddr_in socket_address;
    socket_address.sin_family = AF_INET;
    socket_address.sin_port = htons(port);
    socket_address.sin_addr.s_addr = ip_address;
    return socket_address;
}

int create_socket() {
    return socket(AF_INET, SOCK_STREAM, 0);
}

int connect_socket(int socket, struct sockaddr_in socket_address) {
    return connect(socket, (struct sockaddr*)&socket_address, sizeof(socket_address));
}

int bytes_to_int(char *bytes) {
    int value = 0;
    memcpy((char*)&value, bytes, 4);
    return value;
}

void int_to_bytes(int value, char *bytes) {
    memcpy(bytes, &value, 4);
}
