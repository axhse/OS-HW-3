#include "shared.h"


int main(int argc, const char *argv[]) {
    double pause_in_seconds = get_pause_in_seconds(argc, argv);
    int observer_id = get_id(argc, argv);
    in_addr_t ip_address = get_ip_address(argc, argv, "--server-ip");
    int port = get_port(argc, argv, "--server-port", DEFAULT_OBSERVER_PORT);


    printf(FORMAT_LOG);
    printf("Observer is working\n\n");
    printf(FORMAT_INFO);
    int byte_count;
    char int_buffer[4];
    while (1) {
        int server_socket = create_socket();
        if (connect_socket(server_socket, build_socket_address(ip_address, port)) == -1) {
            printf(FORMAT_LOG);
            printf("Server is not avaliable\n");
            break;
        }
        int_to_bytes(observer_id, int_buffer);
        byte_count = send(server_socket, int_buffer, 4, 0);
        if (byte_count != 4) {
            log_error("Observer id is not sent");
            break;
        }
        printf("Status requested\n");
        byte_count = recv(server_socket, int_buffer, 4, 0);
        if (byte_count != 4) {
            log_error("Customer request count is not received");
            break;
        }
        int customer_request_count = bytes_to_int(int_buffer);
        byte_count = recv(server_socket, int_buffer, 4, 0);
        if (byte_count != 4) {
            log_error("Observer request count is not received");
            break;
        }
        int observer_request_count = bytes_to_int(int_buffer);
        byte_count = recv(server_socket, int_buffer, 4, 0);
        if (byte_count != 4) {
            log_error("Delivered product count is not received");
            break;
        }
        int delivered_product_count = bytes_to_int(int_buffer);
        printf("Customer request count     %d\n", customer_request_count);
        printf("Observer request count     %d\n", observer_request_count);
        printf("Delivered product count    %d\n", delivered_product_count);
        printf("\n");
        sleep(3 * pause_in_seconds);
    }
    return 0;
}
