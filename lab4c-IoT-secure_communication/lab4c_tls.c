#ifdef DUMMY
    #define MRAA_GPIO_IN 0
    typedef int mraa_aio_context;
    typedef int mraa_gpio_context;
    mraa_aio_context mraa_aio_init(int p){ 
        p++;
        return 1;  // return fake device handler
    }
    int mraa_aio_read(mraa_aio_context c) {
        c++;
        return 650;  // return fake temperature value
    }
    void mraa_aio_close(mraa_aio_context c) {c++;} 
    mraa_gpio_context mraa_gpio_init(int p){ 
        p++;
        return 1;
    }
    void mraa_deinit() {}
    void mraa_gpio_dir(mraa_gpio_context c, int d) {
        c++;
        d++;
    }
    int mraa_gpio_read(mraa_gpio_context c) { 
        c++;
        return 0;  // return fake button input
    }
    void mraa_gpio_close(mraa_gpio_context c) {c++;}
#else
    #include <mraa.h>
    #include <mraa/aio.h>
#endif


#include <stdbool.h>
#include <stdlib.h>
#include <getopt.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <poll.h>
#include <math.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>

#define FILENAME_LEN 100
#define HOSTNAME_LEN 200
#define ID_LEN 10
#define BUFSIZE 1000
#define CMDSIZE 2000
#define B 4275 // thermistor value
#define R0 100000.0 // nominal base value
enum temp_scale{CELCI, FAHREN};
enum temp_scale scale = FAHREN;
int period = 1, port, socket_fd;
char logfile[FILENAME_LEN], host[HOSTNAME_LEN], id[ID_LEN];
char my_err_msg[BUFSIZE];
char cur_cmd[CMDSIZE] = "";
int cur_cmd_len = 0;
bool should_log = false;
mraa_aio_context thermo;
time_t last_measure = 0;
bool should_stop = false;
FILE *fd;
SSL *ssl_client;
struct option opts[] =
	{
		{"period", 1, NULL, 'p'},
		{"scale", 1, NULL, 's'},
		{"log", 1, NULL, 'l'},
		{"id", 1, NULL, 'i'},
		{"host", 1, NULL, 'h'},
		{0, 0, 0, 0}			
	};

void get_args(int argc, char **argv);
void init_sensors();
void print_current_time(bool just_print);
void get_current_time(char *str);
void report_temp(char *time);
void just_log(char *msg);
void print_and_log(char *msg);
void read_command();
void myexit(int status);
void proc_cur_cmd();
int client_connect(char * host_name, unsigned int port);
SSL_CTX *ssl_init();
void attach_ssl_to_socket(SSL_CTX * context);
void ssl_clean_client();
void report_error_and_exit();

int main(int argc, char **argv) {
    get_args(argc, argv);
    socket_fd = client_connect(host, (unsigned int) port);
    SSL_CTX *context = ssl_init();
    attach_ssl_to_socket(context);
    init_sensors();
    char str[BUFSIZE];
    sprintf(str, "ID=%s\n", id);
    if (SSL_write(ssl_client, str, strlen(str)) <= 0) {
        strcpy(my_err_msg, "SSL_write() failed\n");
        report_error_and_exit();
    };
    // fprintf(stderr, "%s", str);
    struct pollfd pollfds[1];
    pollfds[0].fd = socket_fd;
    pollfds[0].events = POLLIN + POLLHUP + POLLERR;
    while (true) {
        if (poll(pollfds, 1, 0) == -1) {
            strcpy(my_err_msg, "poll() failed\n");
            report_error_and_exit();
        }
        // report temperature
        if ((time(NULL) - last_measure) >= period && !should_stop) {
            last_measure = time(NULL);
            char time[BUFSIZE];
            get_current_time(time);
            report_temp(time);
        }
        // read stdin
        if (pollfds[0].revents & POLLIN) {
            read_command();
        };
    }
    mraa_aio_close(thermo);
    if (should_log) {
        fclose(fd);
    }
    ssl_clean_client();
}

SSL_CTX *ssl_init() {
    SSL_CTX * newContext = NULL;
    SSL_library_init();
    //Initialize the error message 
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();
    if ((newContext = SSL_CTX_new(TLSv1_client_method())) == NULL) {
        fprintf(stderr, "SSL_CTX_new() failed\n");
        exit(2);
    };
    return newContext;
}

void attach_ssl_to_socket(SSL_CTX * context) {
    ssl_client = SSL_new(context);
    if (ssl_client == NULL) {
        fprintf(stderr, "SSL_new() failed\n");
        SSL_free(ssl_client); 
        exit(2);
    }
    if (SSL_set_fd(ssl_client, socket_fd) == 0) {
        fprintf(stderr, "SSL_set_fd() failed\n");
        SSL_free(ssl_client); 
        exit(2);
    };
    int ret = SSL_connect(ssl_client);
    if (ret == 2) { // no need to shutdown
        fprintf(stderr, "SSL_connect() failed\n");
        SSL_free(ssl_client); 
        exit(2);
    }
    else if (ret < 0) { // need to shutdown
        fprintf(stderr, "SSL_connect() failed\n");
        ssl_clean_client();
        exit(2);
    }
}

void ssl_clean_client() { 
    SSL_shutdown(ssl_client);
    SSL_free(ssl_client);
}

int client_connect(char * host_name, unsigned int port) {
    struct sockaddr_in serv_addr; //encode the ip address and the port for the remote server
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        strcpy(my_err_msg, "socket() failed\n");
        report_error_and_exit();
    }
    // AF_INET: IPv4, SOCK_STREAM: TCP connection
    struct hostent *server = gethostbyname(host_name);
    if (server == NULL) {
        strcpy(my_err_msg, "gethostbyname() failed\n");
        report_error_and_exit();
    }
    // convert host_name to IP addr
    memset(&serv_addr, 0, sizeof(struct sockaddr_in));
    serv_addr.sin_family = AF_INET; //address is Ipv4
    memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);
    //copy ip address from server to serv_addr 
    serv_addr.sin_port = htons(port); //setup the port
    if ((connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr))) < 0) {
        strcpy(my_err_msg, "connet() failed\n");
        report_error_and_exit();
    }; //initiate the connection to server
    return sockfd;
}

void read_command() {
    char buf[BUFSIZ];
    int ret;
    if ((ret = SSL_read(ssl_client, buf, sizeof(buf))) > 0) {
        char ch;
	    int i;
        for (i = 0; buf[i] != '\0'; i++) {
            ch = buf[i];
            // if it's space character at the start of the line
            if (cur_cmd[0] == '\0' && (ch == ' ' || ch == '\t')) {
                continue;
            }
            // if newline not encountered yet
            if (ch != '\n') {
                if (cur_cmd_len > CMDSIZE - 2) {
                    fprintf(stderr, "ERROR: command too long");
                    myexit(1);
                }
                strncat(cur_cmd, &ch, 1);
                cur_cmd_len++;
            }
            // if newline encountered
            else {
                proc_cur_cmd();
                cur_cmd[0] = '\0';
                cur_cmd_len = 0;
            }
        }
    }
}

void proc_cur_cmd() {
    if (should_log) {
        just_log(cur_cmd);
        fputs("\n", fd);
    }
    // fprintf(stderr, "CMD: %s\n", cur_cmd);
    if (strcmp(cur_cmd, "OFF") == 0) {
        char time[BUFSIZE], msg[BUFSIZE*2];
        get_current_time(time);
        sprintf(msg, "%s SHUTDOWN\n", time);
        print_and_log(msg);
        myexit(0);
    }

    if (strcmp(cur_cmd, "STOP") == 0) {
        should_stop = true;
    }
    else if (strcmp(cur_cmd, "START") == 0) {
        should_stop = false;
    }
    else if (strcmp(cur_cmd, "SCALE=F") == 0) {
        scale = FAHREN;
    }
    else if (strcmp(cur_cmd, "SCALE=C") == 0) {
        scale = CELCI;
    }
    else if (strlen(cur_cmd) >= 3 && (strncmp(cur_cmd, "LOG", 3)) == 0) {
        // just_log(cur_cmd);
    }
    else if (strlen(cur_cmd) > 7 && (strncmp(cur_cmd, "PERIOD=", 7)) == 0) {
        period = atoi(cur_cmd + 7);
    }
}

void print_and_log(char *msg) {
    if (SSL_write(ssl_client, msg, strlen(msg)) <= 0) {
        strcpy(my_err_msg, "SSL_write() failed\n");
        report_error_and_exit();
    };
    // fprintf(stderr, "%s", msg);
    just_log(msg);
}

void just_log(char *msg) {
    if (should_log) {
        fputs(msg, fd);
        fflush(fd);
    }
}

void report_temp(char *time) {
    int raw;
    raw = mraa_aio_read(thermo);
    double R = 1023.0/((double) raw) - 1.0;
    R = R0 * R;
    double C = 1.0/(log(R/R0)/B + 1/298.15) - 273.15;
    char msg[BUFSIZ*2];
    if (scale == CELCI) {
        sprintf(msg, "%s %.1f\n", time, C);
        print_and_log(msg);
    }
    else {
        sprintf(msg, "%s %.1f\n", time, (C * 9)/5 + 32);
        print_and_log(msg);
    }
}

void init_sensors() {
	thermo = mraa_aio_init(1);
    if (thermo == 0) {
        fprintf(stderr, "Failed to initialize AIO\n");
        mraa_deinit();
        ssl_clean_client();
        exit(2);
    }
}

void get_current_time(char *str) {
    struct timespec ts;
    struct tm * tm;
    if (clock_gettime(CLOCK_REALTIME, &ts) == -1) {
        strcpy(my_err_msg, "clock_gettime() failed\n");
        report_error_and_exit();
    }
    tm = localtime(&(ts.tv_sec));
    sprintf(str, "%02d:%02d:%02d", tm->tm_hour, tm->tm_min, tm->tm_sec);
}

void get_args(int argc, char **argv) {
    int i, required_arg = 0;
    while((i = getopt_long(argc, argv, "", opts, NULL)) != -1) {
        switch (i)
        {
        case 'p': // period
            period = atoi(optarg);
            break;
        case 's': // scale
            if (strcmp(optarg, "C") == 0) {
                scale = CELCI;
            }
            else if (strcmp(optarg, "F") != 0) {
                fprintf(stderr, "ERROR: incorrect option for 'scale'\n");
                exit(1);
            }
            break;
        case 'l': // log
            required_arg++;
            should_log = true;
            strcpy(logfile, optarg);
            fd = fopen(logfile, "a");
            if (fd == NULL) {
                fprintf(stderr, "ERROR: fopen() - %s\n", strerror(errno));
                exit(2);
            }
            break;
        case 'i': // id
            required_arg++;
            strcpy(id, optarg);
            break;
        case 'h': //host
            required_arg++;
            strcpy(host, optarg);
            break;
        default:
            fprintf(stderr, "ERROR: unexpected argument\n");
            exit(1);
            break;
        }
    }
    if (required_arg != 3) {
        fprintf(stderr, "ERROR: 3 parameters (id, host, log) are required\n");
        exit(1);
    }
    if (optind < argc) {
        port = atoi(argv[optind]);
    }
    else {
        fprintf(stderr, "ERROR: port number is required\n");
        exit(1);
    }
}

void report_error_and_exit() {
    fprintf(stderr, "%s\n", my_err_msg);
    myexit(2);
}

void myexit(int status) {
    mraa_aio_close(thermo);
    if (should_log) {
        fclose(fd);
    }
    ssl_clean_client();
    exit(status);
}
