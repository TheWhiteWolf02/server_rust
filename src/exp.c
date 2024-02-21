#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "mmc_cmds.h"
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <stddef.h>
#include "3rdparty/hmac_sha/hmac_sha2.h"
#include <sys/socket.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define PORT 8888
#define NBLOCKS 65535
#define BLOCKSIZE 256
#define SK_PATH "./privkey.pem"
#define CERT_PATH "./cert.pem"
#define ZEROBLOCK "000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"
#define MAXLATENCIES 65535
#define MILLION_US 1000000

/* RPMB file paths */
const char *RPMB_DEV_PATH = "/dev/mmcblk1rpmb";
const char *STD_KEY_FILENAME = "/MK2KEY.sk";

/* RPMB consts */
const int KEY_LEN = 33;
const char *key = "YRi55\\GAxgEZD9viP>j8=IUe;oIjYPY\n";

/* Evaluation filepaths */
const char *LATENCY_FILEPATH_MC = "/home/usbarmory/mmc-utils/data/ssl_get_mc.csv";
const char *LATENCY_FILEPATH_READ = "/home/usbarmory/mmc-utils/data/ssl_r_no_cache.csv";
const char *LATENCY_FILEPATH_WRITE = "/home/usbarmory/mmc-utils/data/ssl_w_no_cache.csv";
const char *LATENCY_FILEPATH_ZERO = "/home/usbarmory/mmc-utils/data/zero_all.csv";

/* operation switch */
const int OP_READ = 0;
const int OP_WRITE = 1;
const int OP_MC = 2;
const int OP_INIT = 3;
const int OP_ZERO = 4;

/* flags */
int eval = 0;

/* Error strings */
const char *ERROR_O_DEV = "Unable to open device file. (Check that you have the due permissions)";
const char *ERROR_O_FILE = "Unable to open file.";
const char *ERROR_INVLD_BLK_CNT = "Invalid block count.";

/* STDOUT Tags */
const char *INFO_TAG = "[INFO] ";
const char *ERROR_TAG = "[ERROR] ";
const char *RESULT_TAG = "[RESULT] ";

/* netcode datastructures */
struct sockaddr_in serv_addr;

const char *o_input = "a bloc\ntwo blocz\nsree bolcz\n4 flocks\n5 bolbz\nzex solx\nseben eben\neit night\nneun neun neun\nden blogs\na bloc\ntwo blocz\nsree bolcz\n4 flocks\n5 bolbz\nzex solx\nseben eben\neit night\nneun neun neu\nanoder 40 chars. God i'm tired. This is absolutely nothing though.\n";
extern int write_block(char *input);
extern char *read_block();

struct rpmb_frame
{
    u_int8_t stuff[196];
    u_int8_t key_mac[32];
    u_int8_t data[256];
    u_int8_t nonce[16];
    u_int32_t write_counter;
    u_int16_t addr;
    u_int16_t block_count;
    u_int16_t result;
    u_int16_t req_resp;
};

void free_all(char **args, int args_s)
{
    for (int i = 0; i < args_s; i++)
    {
        free(args[i]);
    }
    free(args);
}

enum rpmb_op_type
{
    MMC_RPMB_WRITE_KEY = 0x01,
    MMC_RPMB_READ_CNT = 0x02,
    MMC_RPMB_WRITE = 0x03,
    MMC_RPMB_READ = 0x04,

    /* For internal usage only, do not use it directly */
    MMC_RPMB_READ_RESP = 0x05
};

int write_block(char *input)
{
    char **args = (char **)malloc(sizeof(char *) * 5);
    args[0] = (char *)malloc(strlen("write-block") * sizeof(char) + 1);
    strcpy(args[0], (char *)"write-block");
    args[1] = (char *)malloc(strlen(RPMB_DEV_PATH) * sizeof(char));
    strcpy(args[1], RPMB_DEV_PATH);
    char *block_addr = (char *)"0";
    args[2] = (char *)malloc(strlen(block_addr) * sizeof(char) + 1);
    strcpy(args[2], block_addr);
    args[3] = (char *)malloc(256);
    memset(args[3], ' ', 256); // ascii value of special character
    memcpy(args[3], input, strlen(input) * sizeof(char));
    args[4] = (char *)malloc(strlen(key) * sizeof(char));
    strcpy(args[4], key);
    int nargs = 5;
    int res = do_rpmb_write_block_if(nargs, args);
    free_all(args, 5);
    return res;
}

char *read_block()
{
    char **args = (char **)malloc(sizeof(char *) * 6);
    args[0] = (char *)malloc(strlen("read-block") * sizeof(char) + 1);
    strcpy(args[0], (char *)"read-block");
    args[1] = (char *)malloc(strlen(RPMB_DEV_PATH) * sizeof(char));
    strcpy(args[1], RPMB_DEV_PATH);
    char *block_addr = (char *)"0";
    args[2] = (char *)malloc(strlen(block_addr) * sizeof(char) + 1);
    strcpy(args[2], block_addr);
    char *block_count = (char *)"1";
    args[3] = (char *)malloc(strlen(block_count) * sizeof(char) + 1);
    strcpy(args[3], block_count);
    args[4] = (char *)malloc(strlen(key) * sizeof(char));
    strcpy(args[4], key);
    unsigned char **out_mac = (unsigned char **)malloc(sizeof(unsigned char *));
    int nargs = 6;
    char *out = (char *)malloc(atoi(block_count) * 256 * sizeof(char));
    out = do_rpmb_read_block_if(nargs, args, out);
    free_all(args, 5);
    return out;
    // original
    /*
    printf("%s%s%s%s%s\n", INFO_TAG, "Reading Block ", block_addr, " block count: ", block_count);
    char **args = (char **)malloc(sizeof(char *) * 6);
    args[0] = (char *)malloc(strlen("read-block") * sizeof(char) + 1);
    strcpy(args[0], (char *)"read-block");
    args[1] = (char *)malloc(strlen(RPMB_DEV_PATH) * sizeof(char));
    strcpy(args[1], RPMB_DEV_PATH);
    args[2] = (char *)malloc(strlen(block_addr) * sizeof(char) + 1);
    strcpy(args[2], block_addr);
    args[3] = (char *)malloc(strlen(block_count) * sizeof(char) + 1);
    strcpy(args[3], block_count);
    args[4] = (char *)malloc(strlen(key) * sizeof(char));
    strcpy(args[4], key);
    int nargs = 6;
    */
}

char *get_field(char *req_buffer, int *index, int max_field_size)
{
    char sep = '|';
    if (req_buffer[*index] == sep)
    {
        *index = *index + 1;
    }
    int count = 0;
    char *field = (char *)calloc(max_field_size, sizeof(char));
    while (req_buffer[*index] != sep)
    {
        field[count] = req_buffer[*index];
        *index = *index + 1;
        count++;
    }
    return field;
}

unsigned long average(unsigned long *partials, int size)
{
    unsigned long sum = 0l;
    for (int i = 0; i < size; i++)
    {
        sum += partials[i];
    }
    unsigned long sizel = (unsigned long)size;
    return sum / sizel;
}

/*
int read_mc_value()
{
    unsigned int counter;
    int fd;
    fd = open(RPMB_DEV_PATH, O_RDWR);
    if (fd < 0)
    {
        printf("%s%s\n", ERROR_TAG, ERROR_O_DEV);
        exit(1);
    }
    int ret = rpmb_read_counter(fd, &counter);
    if (ret != 0)
    {
        printf("%s%s%d\n", ERROR_TAG, "Unable to read the monotonic counter. Error code: ", ret);
    }
    return counter;
}

void listen_r()
{
    struct timeval timestamp;
    unsigned long start;
    unsigned long *latencies;
    if (eval)
    {
        latencies = (unsigned long *)malloc(MAXLATENCIES * sizeof(unsigned long));
    }
    int req_count = 0;
    InitializeSSL();
    SSL *ssl;
    SSL_CTX *sslctx;
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(PORT);
    sslctx = SSL_CTX_new(TLS_server_method());
    if (SSL_CTX_use_certificate_file(sslctx, CERT_PATH, SSL_FILETYPE_PEM) <= 0)
    {
        printf("%s\n", "[ERROR] Invalid certificate");
        exit(1);
    }
    if (SSL_CTX_use_PrivateKey_file(sslctx, SK_PATH, SSL_FILETYPE_PEM) <= 0)
    {
        printf("%s\n", "[ERROR] Invalid private key");
        exit(1);
    }

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("%s\n", "[ERROR] Unable to bind to socket");
        exit(1);
    }
    printf("%s\n", "[SERVER] Listening");
    listen(sockfd, 5);
    socklen_t size = sizeof(serv_addr);
    int newsockfd;
    newsockfd = accept(sockfd, (struct sockaddr *)&serv_addr, &size);
    int rbufsize = 2048;
    char *req_buffer = (char *)malloc(rbufsize * sizeof(char));
    ssl = SSL_new(sslctx);
    SSL_set_fd(ssl, newsockfd);
    SSL_accept(ssl);
    while (1)
    {
        if (eval)
        {
            gettimeofday(&timestamp, NULL);
            start = timestamp.tv_sec * MILLION_US + timestamp.tv_usec;
        }
        memset(req_buffer, '\0', rbufsize);
        int err;
        do
        {
            err = SSL_read(ssl, req_buffer, rbufsize);
            if (strlen(req_buffer) == 0)
            {
                printf("%s\n", "no data to read- sleeping");
                sleep(10);
            }
            req_count++;
            int32_t ssl_error = SSL_get_error(ssl, err);
            switch (ssl_error)
            {
            case SSL_ERROR_NONE:
                break;
            case SSL_ERROR_WANT_READ:
                printf("SSL_ERROR_WANT_READ\n");
                break;
            case SSL_ERROR_WANT_WRITE:
                printf("SSL_ERROR_WANT_WRITE\n");
                break;
            case SSL_ERROR_ZERO_RETURN:
                printf("SSL_ERROR_ZERO_RETURN\n");
                break;
            case SSL_ERROR_WANT_CONNECT:
                printf("SSL_ERROR_WANT_CONNECT\n");
                break;
            case SSL_ERROR_WANT_ACCEPT:
                printf("SSL_ERROR_WANT_ACCEPT\n");
                break;
            case SSL_ERROR_WANT_ASYNC:
                printf("SSL_ERROR_WANT_ASYNC\n");
                break;
            case SSL_ERROR_SYSCALL:
                printf("SSL_ERROR_SYSCALL\n");
                break;
            default:
                printf("SOME OTHER ERROR\n");
                break;
            }
            if (err < 0)
            {
                ERR_print_errors_fp(stderr);
            }
            printf("%s%s\n", "[SERVER] request: ", req_buffer);
        } while (err < 0);
        if (newsockfd < 0)
        {
            printf("[ERROR] Connection not accepted");
            exit(1);
        }

        int index = 0;
        char *str_op = get_field(req_buffer, &index, 3);
        int op = atoi(str_op);
        char *block_addr;
        int mc = 0;
        char *filepath = (char *)LATENCY_FILEPATH_READ;
        if (op == OP_INIT)
        {
            printf("op init\n");
            printf("%s%d\n", "nblocks ", NBLOCKS);
            for (int i = 0; i < NBLOCKS; i++)
            {
                printf("%s%d\n", "reading block ", i);
                fflush(stdout);
                char *iblock_addr = (char *)calloc(16, sizeof(char));
                sprintf(iblock_addr, "%d", i);
                char *block = read_block(iblock_addr, "1");
                printf("%s%d\n", "sending block: ", i);
                SSL_write(ssl, block, BLOCKSIZE);
                printf("%s%d\n", "sent block: ", i);
                free(block);
            }
        }
        if (op == OP_READ)
        {
            filepath = (char *)LATENCY_FILEPATH_READ;
            block_addr = get_field(req_buffer, &index, 7);
            char *block_count = get_field(req_buffer, &index, 7);
            char *out = read_block(block_addr, block_count);
            int outlen = strlen(out);
            if (SSL_write(ssl, out, outlen) < 0)
            {
                printf("%s%d%s\n", "Failed to send out's ", outlen, " bytes");
            }
            free(block_count);
            free(out);
            free(block_addr);
        }
        else if (op == OP_WRITE)
        {
            filepath = (char *)LATENCY_FILEPATH_WRITE;
            block_addr = get_field(req_buffer, &index, 7);
            char *input = get_field(req_buffer, &index, 2048);
            int result = write_block(block_addr, input);
            free(input);
            free(block_addr);
        }
        else if (op == OP_MC)
        {
            filepath = (char *)LATENCY_FILEPATH_MC;
            mc = read_mc_value();
        }
        else if (op == OP_ZERO)
        {
            printf("%s\n", "operation zero");
            fflush(stdout);
            filepath = (char *)LATENCY_FILEPATH_ZERO;
            for (int i = 0; i < NBLOCKS; i++)
            {
                char *iblock_addr = (char *)calloc(16, sizeof(char));
                sprintf(iblock_addr, "%d", i);
                printf("%s%s\n", "iblock_addres ", iblock_addr);
                int result = write_block(iblock_addr, ZEROBLOCK);
            }
        }
        if (eval)
        {
            gettimeofday(&timestamp, NULL);
            unsigned long end = timestamp.tv_sec * 1000000 + timestamp.tv_usec;
            unsigned long latency = end - start;
            latencies[req_count] = latency;
            unsigned long avg = average(latencies, req_count);
            FILE *fd;
            fd = fopen(filepath, "a+");
            char *line = (char *)malloc(32);
            sprintf(line, "%lu\n", latency);
            fputs(line, fd);
            printf("%s%lu%s%d%s%lu\n", "Average latency: ", avg, " in request nr: ", req_count, " cur latency: ", latency);
            fclose(fd);
        }
    }
}

void init(char *file)
{
    char **args;
    args = (char **)malloc(sizeof(char *) * 16);
    for (int i = 0; i < sizeof(args); i++)
    {
        args[i] = (char *)malloc(sizeof(char) * 256);
    }
    strcpy(args[0], "args");
    strcpy(args[1], RPMB_DEV_PATH);
    if (strcmp(file, "-") == 0)
    {
        char cwd[256];
        char *res = getcwd(cwd, sizeof(cwd));
        if (res == NULL)
        {
            perror("Unable to get filepath");
        }
        strcat(cwd, STD_KEY_FILENAME);
        strcpy(args[2], cwd);
    }
    else
    {
        strcpy(args[2], file);
    }
    int nargs = 3;
    int ret = do_rpmb_write_key(nargs, args);
    if (ret == 0)
    {
        printf("%skey from %s successfully programmed on RPMB device in %s\n", INFO_TAG, args[2], args[1]);
    }
    else
    {
        printf("%skey NOT programmed successfully. Return value: %d", INFO_TAG, ret);
    }
    for (int i = 0; i < sizeof(args); i++)
    {
        free(args[i]);
    }
    free(args);
}

int main(int argc, char *argv[])
{
    if (argc > 1)
    {
        if (strcmp(argv[1], "--progkey") == 0)
        {
            printf("%s%s%s\n", INFO_TAG, "Programming key with file: ", argv[2]);
            init(argv[2]);
        }
        else if (strcmp(argv[1], "--read-count") == 0)
        {
            printf("%s%s%d\n", RESULT_TAG, "Current monotonic counter value: ", read_mc_value());
        }
        else if (strcmp(argv[1], "--write-block") == 0)
        {
            char *filepath = (char *)LATENCY_FILEPATH_WRITE;
            unsigned long *latencies = (unsigned long *)malloc(MAXLATENCIES * sizeof(unsigned long));
            for (int i = 1; i < 5000; i++)
            {
                struct timeval timestamp;
                gettimeofday(&timestamp, NULL);
                unsigned long start = timestamp.tv_sec * MILLION_US + timestamp.tv_usec;
                char **args = (char **)malloc(sizeof(char *) * 5);
                args[0] = (char *)malloc(strlen("write-block") * sizeof(char) + 1);
                strcpy(args[0], (char *)"write-block");
                args[1] = (char *)malloc(strlen(RPMB_DEV_PATH) * sizeof(char));
                strcpy(args[1], RPMB_DEV_PATH);
                char *block_addr = (char *)malloc(sizeof(char) * 32);
                sprintf(block_addr, "%s%X", (char *)"0x", i);
                args[2] = (char *)malloc(strlen(block_addr) * sizeof(char) + 1);
                strcpy(args[2], block_addr);
                args[3] = (char *)malloc(strlen(o_input) * sizeof(char));
                strcpy(args[3], o_input);
                args[4] = (char *)malloc(strlen(key) * sizeof(char));
                strcpy(args[4], key);
                int nargs = 5;
                write_block(block_addr, (char *)o_input);
                free_all(args, 5);

                gettimeofday(&timestamp, NULL);
                unsigned long end = timestamp.tv_sec * MILLION_US + timestamp.tv_usec;
                unsigned long latency = end - start;
                latencies[i] = latency;
                unsigned long avg = average(latencies, i);
                FILE *fd;
                fd = fopen(filepath, "a+");
                char *line = (char *)malloc(32);
                sprintf(line, "%lu\n", latency);
                fputs(line, fd);
                printf("%s%lu%s%d%s%lu\n", "Average latency: ", avg, " in request nr: ", i, " cur latency: ", latency);
                fclose(fd);
            }
        }
        else if (strcmp(argv[1], "--read-block") == 0)
        {
            char *filepath = (char *)LATENCY_FILEPATH_READ;
            unsigned long *latencies = (unsigned long *)malloc(4096 * sizeof(unsigned long));
            for (int i = 1; i < 5000; i++)
            {
                struct timeval timestamp;
                gettimeofday(&timestamp, NULL);
                unsigned long start = timestamp.tv_sec * 1000000 + timestamp.tv_usec;
                char **args = (char **)malloc(sizeof(char *) * 6);
                args[0] = (char *)malloc(strlen("read-block") * sizeof(char) + 1);
                strcpy(args[0], (char *)"read-block");
                args[1] = (char *)malloc(strlen(RPMB_DEV_PATH) * sizeof(char));
                strcpy(args[1], RPMB_DEV_PATH);
                char *block_addr = (char *)malloc(sizeof(char) * 32);
                sprintf(block_addr, "%s%X", (char *)"0x", i);
                args[2] = (char *)malloc(strlen(block_addr) * sizeof(char) + 1);
                strcpy(args[2], block_addr);
                char *block_count = (char *)"1";
                args[3] = (char *)malloc(strlen(block_count) * sizeof(char) + 1);
                strcpy(args[3], block_count);
                args[4] = (char *)malloc(strlen(key) * sizeof(char));
                strcpy(args[4], key);
                int nargs = 6;
                read_block(block_addr, block_count);
                // printf("%s%s%s\n", RESULT_TAG, "Blocks: \n", do_rpmb_read_block_if(nargs, args));
                free_all(args, 5);
                printf("%s\n", "free");
                gettimeofday(&timestamp, NULL);
                unsigned long end = timestamp.tv_sec * 1000000 + timestamp.tv_usec;
                printf("%s\n", "end");
                unsigned long latency = end - start;
                latencies[i] = latency;
                unsigned long avg = average(latencies, i);
                printf("%s\n", "average");
                FILE *fd;

                fd = fopen(filepath, "a+");
                char *line = (char *)malloc(32);
                sprintf(line, "%lu\n", latency);
                fputs(line, fd);
                printf("%s%lu%s%d%s%lu\n", "Average latency: ", avg, " in request nr: ", i, " cur latency: ", latency);
                fclose(fd);
            }
        }
        else
        {
            printf("%s\n", "usage: mmc [--progkey | --read-count | --write-block | --read-block]\n--write-block block_addr input\n--read-block block_addr block_count\n");
        }
    }
    else if (argc == 1)
    {
        listen_r();
    }
    return 0;
}
*/
