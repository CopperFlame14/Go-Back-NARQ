#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>

#define SERVER_IP "127.0.0.1"
#define PORT 12345
#define BUF_SIZE 1024
#define WINDOW_SIZE 4
#define MAX_SEQ 10

int main() {
    int sockfd;
    struct sockaddr_in servaddr;
    char buffer[BUF_SIZE];
    socklen_t len = sizeof(servaddr);

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set recv timeout
    struct timeval tv;
    tv.tv_sec = 2;
    tv.tv_usec = 0;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof(tv)) < 0) {
        perror("Error setting socket timeout");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);
    servaddr.sin_addr.s_addr = inet_addr(SERVER_IP);

    char *messages[MAX_SEQ] = {
        "Msg0", "Msg1", "Msg2", "Msg3", "Msg4",
        "Msg5", "Msg6", "Msg7", "Msg8", "Msg9"
    };

    int base = 0, next_seq_num = 0;
    int window_size = WINDOW_SIZE;
    int total_msgs = MAX_SEQ;

    while (base < total_msgs) {
        // Send packets in window
        while (next_seq_num < base + window_size && next_seq_num < total_msgs) {
            snprintf(buffer, BUF_SIZE, "%d|%s", next_seq_num, messages[next_seq_num]);
            sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *)&servaddr, len);
            printf("Sent packet seq %d: %s\n", next_seq_num, messages[next_seq_num]);
            next_seq_num++;
        }

        // Wait for ACK
        int n = recvfrom(sockfd, buffer, BUF_SIZE, 0, NULL, NULL);
        if (n < 0) {
            perror("Timeout or recvfrom error. Resending window...");
            // Timeout: resend all packets from base
            next_seq_num = base;
            continue;
        }

        buffer[n] = '\0';
        int ack_num;
        if (sscanf(buffer, "ACK%d", &ack_num) == 1) {
            printf("Received %s\n", buffer);
            if (ack_num >= base) {
                base = ack_num + 1;
            }
        } else {
            printf("Received invalid ACK: %s\n", buffer);
        }
    }

    printf("All packets sent and acknowledged.\n");

    close(sockfd);
    return 0;
}
