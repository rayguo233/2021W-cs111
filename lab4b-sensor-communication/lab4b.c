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

#define FILENAME_LEN 100
#define BUFSIZE 100
#define CMDSIZE 2000
#define B 4275 // thermistor value
#define R0 100000.0 // nominal base value
enum temp_scale{CELCI, FAHREN};
enum temp_scale scale = FAHREN;
int period = 1;
char logfile[FILENAME_LEN];
char cur_cmd[CMDSIZE] = "";
int cur_cmd_len = 0;
bool should_log = false;
mraa_gpio_context button;
mraa_aio_context thermo;
time_t last_measure = 0;
bool should_stop = false;
FILE *fd;
struct option opts[] =
	{
		{"period", 1, NULL, 'p'},
		{"scale", 1, NULL, 's'},
		{"log", 1, NULL, 'l'},
		{0, 0, 0, 0}			
	};

void get_args(int argc, char **argv);
void init_sensors();
void print_current_time(bool just_print);
void report_temp();
void just_log(char *msg);
void print_and_log(char *msg);
void read_command();
void myexit(int status);
void proc_cur_cmd();

int main(int argc, char **argv) {
    get_args(argc, argv);
    init_sensors();
    struct pollfd pollStdin = { 0, POLLIN, 0 }; 
    int poll_ret;
    while (true) {
        // report temperature
        if ((time(NULL) - last_measure) >= period && !should_stop) {
            last_measure = time(NULL);
            print_current_time(false);
            report_temp();
        }
        // read stdin
        if ((poll_ret = poll(&pollStdin, 1, 1000)) >= 1) {
            read_command();
        };
        // check button press
        if (mraa_gpio_read(button) == 1) {
            print_current_time(false);
            char msg[] = "SHUTDOWN\n";
            print_and_log(msg);
            myexit(0);
        }
    }
    mraa_aio_close(thermo);
    mraa_gpio_close(button);
    if (should_log) {
        fclose(fd);
    }
}

void read_command() {
    char buf[BUFSIZ];
    int ret;
    if ((ret = read(0, buf, sizeof(buf))) > 0) {
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

    if (strcmp(cur_cmd, "OFF") == 0) {
        print_current_time(false);
        char msg[] = "SHUTDOWN\n";
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
    printf(msg);
    just_log(msg);
}

void just_log(char *msg) {
    if (should_log) {
        fputs(msg, fd);
        fflush(fd);
    }
}

void report_temp() {
    int raw;
    raw = mraa_aio_read(thermo);
    double R = 1023.0/((double) raw) - 1.0;
    R = R0 * R;
    double C = 1.0/(log(R/R0)/B + 1/298.15) - 273.15;
    char msg[BUFSIZ];
    if (scale == CELCI) {
        sprintf(msg, "%.1f\n", C);
        print_and_log(msg);
    }
    else {
        sprintf(msg, "%.1f\n", (C * 9)/5 + 32);
        print_and_log(msg);
    }
}

void init_sensors() {
    button = mraa_gpio_init(60);
    if (button == 0) {
        fprintf(stderr, "Failed to initialize GPIO %d\n", 60);
        mraa_deinit();
        exit(1);
    }
	mraa_gpio_dir(button, MRAA_GPIO_IN);
	thermo = mraa_aio_init(1);
    if (thermo == 0) {
        fprintf(stderr, "Failed to initialize AIO\n");
        mraa_deinit();
        exit(1);
    }
}

void print_current_time(bool just_print) {
    struct timespec ts;
    struct tm * tm;
    char msg[BUFSIZ];
    if (clock_gettime(CLOCK_REALTIME, &ts) == -1) {
        fprintf(stderr, "ERROR: clock_gettime() - %s\n", strerror(errno));
        myexit(1);
    }
    tm = localtime(&(ts.tv_sec));
    sprintf(msg, "%02d:%02d:%02d ", tm->tm_hour, tm->tm_min, tm->tm_sec);
    if (!just_print)
        print_and_log(msg);
    else
        printf(msg);
}

void get_args(int argc, char **argv) {
    int i;
    while((i = getopt_long(argc, argv, "", opts, NULL)) != -1) {
        switch (i)
        {
        case 'p':
            period = atoi(optarg);
            break;
        case 's':
            if (strcmp(optarg, "C") == 0) {
                scale = CELCI;
            }
            else if (strcmp(optarg, "F") != 0) {
                fprintf(stderr, "ERROR: incorrect option for 'scale'\n");
                myexit(1);
            }
            break;
        case 'l':
            should_log = true;
            strcpy(logfile, optarg);
            fd = fopen(logfile, "a");
            if (fd == NULL) {
                fprintf(stderr, "ERROR: fopen() - %s\n", strerror(errno));
                myexit(1);
            }
            break;
        default:
            fprintf(stderr, "ERROR: unexpected argument\n");
            myexit(1);
            break;
        }
    }
}

void myexit(int status) {
    mraa_aio_close(thermo);
    mraa_gpio_close(button);
    if (should_log) {
        fclose(fd);
    }
    exit(status);
}
