#include "shared.h"


int main(int argc, const char *argv[]) {
    char product_name_sizes[MAX_ORDER_SIZE];
    char product_names[MAX_ORDER_SIZE][MAX_PRODUCT_NAME_SIZE];
    int order_size = read_product_list(argc, argv, product_name_sizes, product_names);
    if (order_size == -1) {
        return 0;
    }
    double pause_in_seconds = get_pause_in_seconds(argc, argv);
    int customer_id = get_id(argc, argv);
    in_addr_t ip_address = get_ip_address(argc, argv, "--server-ip");
    int port = get_port(argc, argv, "--server-port", DEFAULT_CUSTOMER_PORT);


    printf(FORMAT_LOG);
    printf("Customer is working\n\n");
    printf(FORMAT_INFO);
    int byte_count;
    char int_buffer[4];
    for (int product_index = 0; product_index < order_size; ++product_index) {
        int server_socket = create_socket();
        if (connect_socket(server_socket, build_socket_address(ip_address, port)) == -1) {
            printf(FORMAT_LOG);
            printf("\nServer is not avaliable\n");
            return 0;
        }
        int_to_bytes(customer_id, int_buffer);
        byte_count = send(server_socket, int_buffer, 4, 0);
        if (byte_count != 4) {
            log_error("Customer id is not sent");
            return 0;
        }
        int_to_bytes(product_name_sizes[product_index], int_buffer);
        byte_count = send(server_socket, int_buffer, 4, 0);
        if (byte_count != 4) {
            log_error("Product size is not sent");
            return 0;
        }
        byte_count = send(server_socket, product_names[product_index], product_name_sizes[product_index], 0);
        if (byte_count != product_name_sizes[product_index]) {
            log_error("Product name is not sent");
            return 0;
        }
        printf("Product requested       ");
        print_string(product_names[product_index], product_name_sizes[product_index]);
        printf("\n");
        byte_count = recv(server_socket, int_buffer, 4, 0);
        if (byte_count != 4) {
            log_error("Response code is not received");
            return 0;
        }
        int response_code = bytes_to_int(int_buffer);
        if (response_code == RESPONSE_CODE_SUCCESS) {
            printf("Product received        ");
        } else {
            printf("Product not received    ");
        }
        print_string(product_names[product_index], product_name_sizes[product_index]);
        printf("\n");
        sleep(2 * pause_in_seconds);
    }
    printf(FORMAT_LOG);
    printf("\nProduct list is fully checked\n");
    printf("Customer is stopped\n");
    return 0;
}
