#include "mymqtt.h"

static const char *TAG = "mymqtt";

int sock; // Socket TCP

/**
 * @brief Cria o socket TCP
 *
 * @param host_ip IP do servidor
 * @param port Porta de aplicação
 */
void tcp_socket_init(char *host_ip, int port)
{
    int addr_family = 0;
    int ip_protocol = 0;
    int err;

#if defined(CONFIG_EXAMPLE_IPV4)
    struct sockaddr_in dest_addr;
    dest_addr.sin_addr.s_addr = inet_addr(host_ip);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(port);
    addr_family = AF_INET;
    ip_protocol = IPPROTO_IP;
#elif defined(CONFIG_EXAMPLE_IPV6)
    struct sockaddr_in6 dest_addr = {0};
    inet6_aton(host_ip, &dest_addr.sin6_addr);
    dest_addr.sin6_family = AF_INET6;
    dest_addr.sin6_port = htons(port);
    dest_addr.sin6_scope_id = esp_netif_get_netif_impl_index(EXAMPLE_INTERFACE);
    addr_family = AF_INET6;
    ip_protocol = IPPROTO_IPV6;
#elif defined(CONFIG_EXAMPLE_SOCKET_IP_INPUT_STDIN)
    struct sockaddr_storage dest_addr = {0};
    ESP_ERROR_CHECK(get_addr_from_stdin(port, SOCK_STREAM, &ip_protocol, &addr_family, &dest_addr));
#endif

    sock = socket(addr_family, SOCK_STREAM, ip_protocol);
    if (sock < 0)
    {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
    }
    ESP_LOGI(TAG, "Socket created, connecting to %s:%d", host_ip, port);

    err = connect(sock, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr_in6));
    if (err != 0)
    {
        ESP_LOGE(TAG, "Socket unable to connect: errno %d", errno);
    }
    ESP_LOGI(TAG, "Successfully connected");
}

/**
 * @brief Abre a conexão MQTT com o broker
 *
 * @param host_ip IP do servidor
 * @param port Porta de aplicação MQTT
 * @return int (-1: falha | 0 ou mais: bytes enviados com sucesso)
 */
int mymqtt_connect(char *host_ip, int port)
{
    char msgc[100];    // Buffer de envio
    char rx_buffer[8]; // Buffer de recebimento
    int ret, len;

    tcp_socket_init(host_ip, port); // abre conexão TCP

    msgc[0] = 0x10; // CONTROL HEADER		1 = Command Type (CONNECT) | 0 = Control Flags

    msgc[1] = 0x11; // REMAINING LENGTH		0x11 = 17 bytes

    msgc[2] = 0x00;  // VARIABLE HEADER		Protocol name length MSB
    msgc[3] = 0x04;  // VARIABLE HEADER		Protocol name length LSB = (0004 = 4 bytes)
    msgc[4] = 'M';   // VARIABLE HEADER		Protocol name byte 1
    msgc[5] = 'Q';   // VARIABLE HEADER		Protocol name byte 2
    msgc[6] = 'T';   // VARIABLE HEADER		Protocol name byte 3
    msgc[7] = 'T';   // VARIABLE HEADER		Protocol name byte 4
    msgc[8] = 0x04;  // VARIABLE HEADER		Protocol version (0x04 = version 3.1.1)
    msgc[9] = 0x02;  // VARIABLE HEADER		Connect flags
    msgc[10] = 0x00; // VARIABLE HEADER		Keep alive duration MSB
    msgc[11] = 0x3C; // VARIABLE HEADER		Keep alive duration LSB (003C = 60s)

    msgc[12] = 0x00; // PAYLOAD				Client ID Length MSB
    msgc[13] = 0x05; // PAYLOAD				Client ID Length LSB
    msgc[14] = 'P';  // PAYLOAD				Client ID
    msgc[15] = 'Q';  // PAYLOAD				Client ID
    msgc[16] = 'R';  // PAYLOAD				Client ID
    msgc[17] = 'S';  // PAYLOAD				Client ID
    msgc[18] = 'T';  // PAYLOAD				Client ID

    /* Envio do pacote CONNECT */
    ESP_LOGI(TAG, "Connecting");
    ret = send(sock, msgc, 19, 0);
    if (ret < 0)
    {
        ESP_LOGE(TAG, "send failed: errno %d", errno);
        return -1;
    }

    /* Recepção do CONNACK */
    len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
    if (len < 0)
    {
        ESP_LOGE(TAG, "recv failed: errno %d", errno);
        return -1;
    }
    else
    {
        rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string
        ESP_LOGI(TAG, "Received %d bytes", len);
        ESP_LOG_BUFFER_HEX(TAG, rx_buffer, len);
        return ret;
    }
}

/**
 * @brief Encerra conexão com broker MQTT e fecha conexão TCP
 *
 */
void mymqtt_disconnect(void)
{
    char msgc[2]; // Buffer de envio

    msgc[0] = 0xE0; // CONTROL HEADER		E = Command Type (DISCONNECT) | 0 = Control Flags
    msgc[1] = 0x00; // NO CONTENT

    /* Envio do pacote DISCONNECT */
    ESP_LOGI(TAG, "Disconnecting");
    send(sock, msgc, 2, 0);

    /* Encerramento do socket TCP */
    ESP_LOGI(TAG, "Shutting down socket");
    shutdown(sock, 0);
    close(sock);
}

/**
 * @brief Faz a publicação MQTT
 *
 * @param topic Nome do tópico
 * @param payload Ponteiro para o conteúdo
 * @param size Tamanho do conteúdo
 * @return int (-1: falha | 0 ou mais: bytes enviados com sucesso)
 */
int mymqtt_publish(char *topic, void *payload, uint32_t size)
{
    char msgc[100];      // Buffer de envio
    uint32_t msglen = 4; // Tamanho total da mensagem (iniciando com 4 bytes)
    int ret;

    msgc[0] = 0x30;                          // CONTROL HEADER		3 = Command Type (PUBLISH) | 0 = Control Flags
    msgc[1] = 0x0B;                          // REMAINING LENGTH	0x0B = 11 bytes
    msgc[2] = (uint8_t)(strlen(topic) >> 8); // VARIABLE HEADER		Topic name length MSB
    msgc[3] = (uint8_t)(strlen(topic));      // VARIABLE HEADER		Topic name length LSB

    /* PAYLOAD: Copia o nome do tópico para a mensagem */
    strncpy(&msgc[msglen], topic, strlen(topic));
    msglen = msglen + strlen(topic);

    /* PAYLOAD: Copia o conteúdo para a mensagem */
    memcpy(&msgc[msglen], payload, size);
    msglen = msglen + size;

    /* Faz a publicação */
    ESP_LOGI(TAG, "Publishing");
    ESP_LOG_BUFFER_HEX(TAG, msgc, msglen);
    ret = send(sock, msgc, msglen, 0);
    if (ret < 0)
    {
        ESP_LOGE(TAG, "send failed: errno %d", errno);
        return -1;
    }
    else
    {
        return ret;
    }
}

int mymqtt_subscribe()
{
    char msgc[100]; // Buffer de envio
    char rx_buffer[8]; // Buffer de recebimento
    int ret, len;

    msgc[0] = 0X82; // CONTROL HEADER		8 = Command Type (SUBSCRIBE) | 2 = Control Flags
    msgc[1] = 0X08; // REMAINING LENGTH		0x08 = 8 bytes
    msgc[2] = 0X00; // VARIABLE HEADER		Packet ID MSB
    msgc[3] = 0X01; // VARIABLE HEADER		Packet ID LSB (0001 = 1)
    msgc[4] = 0X00; // PAYLOAD				Length MSB
    msgc[5] = 0X03; // PAYLOAD				Length LSB
    msgc[6] = 'M';  // PAYLOAD				Topic name byte 1
    msgc[7] = 'P';  // PAYLOAD				Topic name byte 2
    msgc[8] = 'B';  // PAYLOAD				Topic name byte 3
    msgc[9] = 0x00; // PAYLOAD				Requested QoS

    /* Faz a inscrição */
    ESP_LOGI(TAG, "Subscribing");
    ESP_LOG_BUFFER_HEX(TAG, msgc, 10);
    ret = send(sock, msgc, 10, 0);
    if (ret < 0)
    {
        ESP_LOGE(TAG, "send failed: errno %d", errno);
        return -1;
    }

    /* Recepção do SUBACK */
    len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
    if (len < 0)
    {
        ESP_LOGE(TAG, "recv failed: errno %d", errno);
        return -1;
    }
    else
    {
        rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string
        ESP_LOGI(TAG, "Received %d bytes", len);
        ESP_LOG_BUFFER_HEX(TAG, rx_buffer, len);
        return ret;
    }
}

int mymqtt_listen(char * rx_buffer, int buffer_size)
{
    int len;

    /* Recepção de dados */
    len = recv(sock, rx_buffer, buffer_size, 0);
    if (len < 0)
    {
        ESP_LOGE(TAG, "recv failed: errno %d", errno);
        return -1;
    }
    else
    {
        rx_buffer[len] = 0; // Insere um NULL no fim do pacote
        return len;
    }
}