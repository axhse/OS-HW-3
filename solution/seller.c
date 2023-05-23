#include "shared.h"


int main(int argc, const char *argv[]) {
    char product_name_sizes[MAX_PRODUCT_LIST_SIZE];
    char product_names[MAX_PRODUCT_LIST_SIZE][MAX_PRODUCT_NAME_SIZE];
    int product_count = read_product_list(argc, argv, product_name_sizes, product_names);
    if (product_count == -1) {
        return 0;
    }
    double pause_in_seconds = get_pause_in_seconds(argc, argv);
    in_addr_t ip_address = get_ip_address(argc, argv, "--server-ip");
    int port;
    int seller_index;
    if (find_flag_index(argc, argv, "--is-second") == argc) {
        seller_index = 1;
        port = get_port(argc, argv, "--server-port", DEFAULT_SELLER_1_PORT);
    } else {
        seller_index = 2;
        port = get_port(argc, argv, "--server-port", DEFAULT_SELLER_2_PORT);
    }


    printf(FORMAT_LOG);
    printf("Seller %d is working\n", seller_index);
    printf("Avaliable goods (%d):\n", product_count);
    for (int index = 0; index < product_count; ++index) {
        print_string(product_names[index], product_name_sizes[index]);
        printf("\n");
    }
    printf("\n");
    printf(FORMAT_INFO);


    int byte_count;
    char int_buffer[4];
    while (1) {
        int server_socket = create_socket();
        if (connect_socket(server_socket, build_socket_address(ip_address, port)) == -1) {
            printf(FORMAT_LOG);
            printf("\nServer is not avaliable\n");
            break;
        }
        byte_count = recv(server_socket, int_buffer, 4, 0);
        if (byte_count != 4) {
            printf(FORMAT_LOG);
            printf("\nServer is not avaliable\n");
            break;
        }
        int product_name_size = bytes_to_int(int_buffer);
        if (product_name_size > MAX_PRODUCT_NAME_SIZE) {
            log_error("Product name is too long");
            break;
        }
        char product_name[product_name_size];
        byte_count = recv(server_socket, product_name, product_name_size, 0);
        if (byte_count != product_name_size) {
            log_error("Product name is not received");
            break;
        }
        printf("Product requested    ");
        print_string(product_name, product_name_size);
        printf("\n");

        int has_product = 0;
        for (int product_index = 0; product_index < product_count; ++product_index) {
            if (product_name_sizes[product_index] != product_name_size) {
                continue;
            }
            int is_equal = 1;
            for (int index = 0; index < product_name_size; ++index) {
                if (product_name[index] != product_names[product_index][index]) {
                    is_equal = 0;
                    break;
                }
            }
            if (is_equal) {
                has_product = 1;
                break;
            }
        }
        sleep(pause_in_seconds);
        int response_code = RESPONSE_CODE_NOT_FOUND;
        if (has_product) {
            sleep(3 * pause_in_seconds);
            response_code = RESPONSE_CODE_SUCCESS;
            printf("Product delivered    ");
        } else {
            printf("Product not found    ");
        }
        print_string(product_name, product_name_size);
        printf("\n");
        int_to_bytes(response_code, int_buffer);
        byte_count = send(server_socket, int_buffer, 4, 0);
        if (byte_count != 4) {
            log_error("Response code is not sent");
            break;
        }
    }
    return 0;
}
