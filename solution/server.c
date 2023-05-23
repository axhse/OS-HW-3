#include <sys/shm.h>
#include "shared.h"


int bind_socket(int socket, struct sockaddr_in socket_address) {
    return bind(socket, (struct sockaddr*)&socket_address, sizeof(socket_address));
}


int main(int argc, char const* argv[]) {
    in_addr_t customer_ip_address = get_ip_address(argc, argv, "--customer-ip");
    in_addr_t seller_1_ip_address = get_ip_address(argc, argv, "--seller1-ip");
    in_addr_t seller_2_ip_address = get_ip_address(argc, argv, "--seller2-ip");
    in_addr_t observer_ip_address = get_ip_address(argc, argv, "--observer-ip");
    int customer_port = get_port(argc, argv, "--customer-port", DEFAULT_CUSTOMER_PORT);
    int seller1_port = get_port(argc, argv, "--seller1-port", DEFAULT_SELLER_1_PORT);
    int seller2_port = get_port(argc, argv, "--seller2-port", DEFAULT_SELLER_2_PORT);
    int observer_port = get_port(argc, argv, "--observer-port", DEFAULT_OBSERVER_PORT);

    int customer_socket = create_socket();
    int seller1_socket = create_socket();
    int seller2_socket = create_socket();
    int observer_socket = create_socket();
    bind_socket(customer_socket, build_socket_address(customer_ip_address, customer_port));
    bind_socket(seller1_socket, build_socket_address(seller_1_ip_address, seller1_port));
    bind_socket(seller2_socket, build_socket_address(seller_2_ip_address, seller2_port));
    bind_socket(observer_socket, build_socket_address(observer_ip_address, observer_port));
    listen(customer_socket, 1);
    listen(seller1_socket, 1);
    listen(seller2_socket, 1);
    listen(observer_socket, 1);
    printf(FORMAT_LOG);
    printf("Server is started\n\n");
    
    int *customer_request_count = (int*)shmat(shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0644), 0, 0);
    int *observer_request_count = (int*)shmat(shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0644), 0, 0);
    int *delivered_product_count = (int*)shmat(shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0644), 0, 0);

    pid_t child_pid = -2;
    child_pid = fork();
    if (child_pid == -1) {
        log_error("Forking is failed");
        return 0;
    }
    int byte_count;
    char int_buffer[4];
    if (child_pid == 0) {
        // We are inside the child thread.
        int accepted_customer_socket = -1;
        int accepted_seller1_socket = -1;
        int accepted_seller2_socket = -1;
        while (1) {
            printf(FORMAT_INFO);
            if (accepted_customer_socket != -1) {
                close(accepted_customer_socket);
                accepted_customer_socket = -1;
            }
            if (accepted_seller1_socket != -1) {
                close(accepted_seller1_socket);
                accepted_seller1_socket = -1;
            }
            if (accepted_seller2_socket != -1) {
                close(accepted_seller2_socket);
                accepted_seller2_socket = -1;
            }
            accepted_customer_socket = accept(customer_socket, NULL, NULL);
            byte_count = recv(accepted_customer_socket, int_buffer, 4, 0);
            if (byte_count != 4) {
                log_error("Customer id is not received");
                continue;
            }
            int customer_id = bytes_to_int(int_buffer);
            printf("\nConnected with customer %d\n", customer_id);
            byte_count = recv(accepted_customer_socket, int_buffer, 4, 0);
            if (byte_count != 4) {
                log_error("Product name size is not received");
                continue;
            }
            int product_name_size = bytes_to_int(int_buffer);
            if (product_name_size > MAX_PRODUCT_NAME_SIZE) {
                log_error("Product name is too long");
                continue;
            }
            char product_name[product_name_size];
            byte_count = recv(accepted_customer_socket, product_name, product_name_size, 0);
            if (byte_count != product_name_size) {
                log_error("Product name is not received");
                continue;
            }
            printf("Product requested    ");
            print_string(product_name, product_name_size);
            printf("\n");
            printf(FORMAT_LOG);
            printf("Connecting to Seller 1\n");
            accepted_seller1_socket = accept(seller1_socket, NULL, NULL);
            printf("Connected to Seller 1\n");
            printf(FORMAT_INFO);
            int_to_bytes(product_name_size, int_buffer);
            byte_count = send(accepted_seller1_socket, int_buffer, 4, 0);
            if (byte_count != 4) {
                log_error("Product name size is not sent to Seller1");
                continue;
            }
            byte_count = send(accepted_seller1_socket, product_name, product_name_size, 0);
            if (byte_count != product_name_size) {
                log_error("Product name is not sent to Seller1");
                continue;
            }
            byte_count = recv(accepted_seller1_socket, int_buffer, 4, 0);
            if (byte_count != 4) {
                log_error("Response code is not received from Seller1");
                continue;
            }
            int response_code = bytes_to_int(int_buffer);
            if (response_code == RESPONSE_CODE_SUCCESS) {
                ++(*delivered_product_count);
            } else {
                printf(FORMAT_LOG);
                printf("Connecting to Seller 2\n");
                accepted_seller2_socket = accept(seller2_socket, NULL, NULL);
                printf("Connected to Seller 2\n");
                printf(FORMAT_INFO);
                int_to_bytes(product_name_size, int_buffer);
                byte_count = send(accepted_seller2_socket, int_buffer, 4, 0);
                if (byte_count != 4) {
                    log_error("Product name size is not sent to Seller2");
                    continue;
                }
                byte_count = send(accepted_seller2_socket, product_name, product_name_size, 0);
                if (byte_count != product_name_size) {
                    log_error("Product name is not sent to Seller2");
                    continue;
                }
                byte_count = recv(accepted_seller2_socket, int_buffer, 4, 0);
                if (byte_count != 4) {
                    log_error("Response code is not received from Seller2");
                    continue;
                }
                response_code = bytes_to_int(int_buffer);
                if (response_code == RESPONSE_CODE_SUCCESS) {
                    ++(*delivered_product_count);
                }
            }
            int_to_bytes(response_code, int_buffer);
            byte_count = send(accepted_customer_socket, int_buffer, 4, 0);
            if (byte_count != 4) {
                log_error("Response code is not sent");
                continue;
            }
            ++(*customer_request_count);
            printf("Order request handled\n");
        }
    } else {
        int accepted_observer_socket = -1;
        while (1) {
            printf(FORMAT_INFO);
            if (accepted_observer_socket != -1) {
                close(accepted_observer_socket);
                accepted_observer_socket = -1;
            }
            accepted_observer_socket = accept(observer_socket, NULL, NULL);
            byte_count = recv(accepted_observer_socket, int_buffer, 4, 0);
            if (byte_count != 4) {
                log_error("Observer id is not received");
                continue;
            }
            int observer_id = bytes_to_int(int_buffer);
            printf("\nConnected with observer %d\n", observer_id);
            printf("Status requested\n");
            int_to_bytes(*customer_request_count, int_buffer);
            byte_count = send(accepted_observer_socket, int_buffer, 4, 0);
            if (byte_count != 4) {
                log_error("Customer request count is not sent");
                continue;
            }
            int_to_bytes(*observer_request_count, int_buffer);
            byte_count = send(accepted_observer_socket, int_buffer, 4, 0);
            if (byte_count != 4) {
                log_error("Observer request count is not sent");
                continue;
            }
            int_to_bytes(*delivered_product_count, int_buffer);
            byte_count = send(accepted_observer_socket, int_buffer, 4, 0);
            if (byte_count != 4) {
                log_error("Delivered product count is not sent");
                continue;
            }
            ++(*observer_request_count);
            printf("Status request handled\n");
        }
    }
    return 0;
}
