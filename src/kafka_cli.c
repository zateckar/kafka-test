/*
 * Kafka CLI Tool - Producer and Consumer with mTLS Authentication
 * 
 * This application provides a command-line interface for testing Kafka brokers
 * with mutual TLS (mTLS) authentication.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <rdkafka.h>

#ifdef _WIN32
#include <windows.h>
#include <conio.h>
#include <direct.h>
#define getcwd _getcwd
#define mkdir(path, mode) _mkdir(path)
#else
#include <unistd.h>
#include <termios.h>
#include <dirent.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#endif

#define VERSION "1.0.0"
#define DEFAULT_INI_FILE "kafka_cli.ini"
#define MAX_LINE_LENGTH 1024
#define MAX_KEY_LENGTH 256
#define MAX_VALUE_LENGTH 768
#define MAX_INI_FILES 20
#define MAX_FILENAME_LENGTH 256
#define LOGS_DIR "logs"

/* Configuration structure */
typedef struct {
    /* Broker settings */
    char brokers[MAX_VALUE_LENGTH];
    char topic[MAX_VALUE_LENGTH];
    
    /* mTLS settings */
    char security_protocol[MAX_VALUE_LENGTH];
    char ssl_ca_location[MAX_VALUE_LENGTH];
    char ssl_certificate_location[MAX_VALUE_LENGTH];
    char ssl_key_location[MAX_VALUE_LENGTH];
    char ssl_key_password[MAX_VALUE_LENGTH];
    int ssl_skip_certificate_verify;
    
    /* Producer settings */
    int producer_batch_size;
    int producer_linger_ms;
    int producer_ack;
    
    /* Consumer settings */
    char consumer_group_id[MAX_VALUE_LENGTH];
    char consumer_auto_offset_reset[MAX_VALUE_LENGTH];
    int consumer_session_timeout_ms;
    char consumer_enable_auto_commit[MAX_VALUE_LENGTH];
    
    /* General settings */
    int verbose;
    int message_count;
} Config;

/* Global variables for signal handling */
static volatile int run = 1;
static rd_kafka_t *global_kafka_handle = NULL;

/* Console colors for Windows */
#ifdef _WIN32
static HANDLE hConsole = NULL;
static WORD savedAttributes = 0;
#endif

/* Global log file */
static FILE *log_file = NULL;

/* Function prototypes */
static void print_usage(const char *program);
static void print_version(void);
static void log_message(int verbose, const char *level, const char *format, ...);
static int parse_ini_file(const char *filename, Config *config);
static char* trim_whitespace(char *str);
static rd_kafka_t* create_producer(const Config *config);
static rd_kafka_t* create_consumer(const Config *config);
static int produce_messages(rd_kafka_t *rk, const Config *config);
static int consume_messages(rd_kafka_t *rk, const Config *config);
static void stop_consumer(int sig);
static void dr_msg_cb(rd_kafka_t *rk, const rd_kafka_message_t *rkmessage, void *opaque);
static void print_config(const Config *config);
static void init_log_file(const char *topic, const char *mode);
static void close_log_file(void);
static void sanitize_filename(char *dst, const char *src, size_t size);

/* TUI Function prototypes */
static void init_console(void);
static void restore_console(void);
static void clear_screen(void);
static void set_color(int color);
static void reset_color(void);
static void move_cursor(int x, int y);
static void draw_box(int x, int y, int width, int height);
static int get_console_width(void);
static int show_main_menu(void);
static int show_ini_selector(char ini_files[MAX_INI_FILES][MAX_FILENAME_LENGTH], int file_count);
static int find_ini_files(char ini_files[MAX_INI_FILES][MAX_FILENAME_LENGTH]);
static int run_tui(char *selected_ini_file, int *selected_mode);

/*
 * Console color codes
 */
#define COLOR_DEFAULT   7
#define COLOR_GREEN     10
#define COLOR_CYAN      11
#define COLOR_RED       12
#define COLOR_YELLOW    14
#define COLOR_WHITE     15
#define COLOR_GRAY      8

/*
 * Initialize console for TUI
 */
static void init_console(void) {
#ifdef _WIN32
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
    GetConsoleScreenBufferInfo(hConsole, &consoleInfo);
    savedAttributes = consoleInfo.wAttributes;
    
    /* Enable ANSI escape codes on newer Windows versions */
    DWORD mode;
    GetConsoleMode(hConsole, &mode);
    SetConsoleMode(hConsole, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#endif
}

/*
 * Restore console to original state
 */
static void restore_console(void) {
#ifdef _WIN32
    if (hConsole != NULL) {
        SetConsoleTextAttribute(hConsole, savedAttributes);
    }
#else
    printf("\033[0m");
#endif
}

/*
 * Clear screen
 */
static void clear_screen(void) {
#ifdef _WIN32
    system("cls");
#else
    printf("\033[2J\033[H");
#endif
}

/*
 * Set console color
 */
static void set_color(int color) {
#ifdef _WIN32
    if (hConsole != NULL) {
        SetConsoleTextAttribute(hConsole, color);
    }
#else
    const char *colors[] = {
        "\033[0m",      /* 0 - reset */
        "\033[31m",     /* 1 - red */
        "\033[32m",     /* 2 - green */
        "\033[33m",     /* 3 - yellow */
        "\033[34m",     /* 4 - blue */
        "\033[35m",     /* 5 - magenta */
        "\033[36m",     /* 6 - cyan */
        "\033[37m",     /* 7 - white */
    };
    if (color >= 0 && color < 8) {
        printf("%s", colors[color]);
    }
#endif
}

/*
 * Reset color to default
 */
static void reset_color(void) {
    set_color(COLOR_DEFAULT);
}

/*
 * Move cursor to position
 */
static void move_cursor(int x, int y) {
#ifdef _WIN32
    COORD coord;
    coord.X = (SHORT)x;
    coord.Y = (SHORT)y;
    SetConsoleCursorPosition(hConsole, coord);
#else
    printf("\033[%d;%dH", y, x);
#endif
}

/*
 * Get console width
 */
static int get_console_width(void) {
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hConsole, &csbi);
    return csbi.srWindow.Right - csbi.srWindow.Left + 1;
#else
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    return w.ws_col;
#endif
}

/*
 * Draw a box with ASCII borders
 */
static void draw_box(int x, int y, int width, int height) {
    int i;
    
    /* Top border */
    move_cursor(x, y);
    printf("+");
    for (i = 0; i < width - 2; i++) printf("-");
    printf("+");
    
    /* Side borders */
    for (i = 1; i < height - 1; i++) {
        move_cursor(x, y + i);
        printf("|");
        move_cursor(x + width - 1, y + i);
        printf("|");
    }
    
    /* Bottom border */
    move_cursor(x, y + height - 1);
    printf("+");
    for (i = 0; i < width - 2; i++) printf("-");
    printf("+");
}

/*
 * Find all INI files in current directory
 */
static int find_ini_files(char ini_files[MAX_INI_FILES][MAX_FILENAME_LENGTH]) {
    int count = 0;
    
#ifdef _WIN32
    WIN32_FIND_DATA findData;
    HANDLE hFind;
    
    hFind = FindFirstFile("*.ini", &findData);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                strncpy(ini_files[count], findData.cFileName, MAX_FILENAME_LENGTH - 1);
                ini_files[count][MAX_FILENAME_LENGTH - 1] = '\0';
                count++;
                if (count >= MAX_INI_FILES) break;
            }
        } while (FindNextFile(hFind, &findData) && count < MAX_INI_FILES);
        FindClose(hFind);
    }
#else
    DIR *dir;
    struct dirent *entry;
    
    dir = opendir(".");
    if (dir != NULL) {
        while ((entry = readdir(dir)) != NULL && count < MAX_INI_FILES) {
            if (strstr(entry->d_name, ".ini") != NULL) {
                strncpy(ini_files[count], entry->d_name, MAX_FILENAME_LENGTH - 1);
                ini_files[count][MAX_FILENAME_LENGTH - 1] = '\0';
                count++;
            }
        }
        closedir(dir);
    }
#endif
    
    return count;
}

/*
 * Show INI file selector menu
 * Returns index of selected file, or -1 on cancel
 */
static int show_ini_selector(char ini_files[MAX_INI_FILES][MAX_FILENAME_LENGTH], int file_count) {
    int selected = 0;
    int key;
    int i;
    int start_y = 8;
    int width = 60;
    
    while (1) {
        clear_screen();
        
        /* Draw header box */
        int console_width = get_console_width();
        int box_x = (console_width - width) / 2;
        draw_box(box_x, 2, width, 5);
        
        set_color(COLOR_CYAN);
        move_cursor(box_x + 14, 4);
        printf("[INI] SELECT CONFIGURATION FILE");
        reset_color();
        
        /* Draw file list box */
        int list_height = file_count + 4;
        if (list_height > 15) list_height = 15;
        draw_box(box_x, start_y - 1, width, list_height);
        
        /* Show files */
        for (i = 0; i < file_count; i++) {
            move_cursor(box_x + 3, start_y + i);
            if (i == selected) {
                set_color(COLOR_YELLOW);
                printf(" > %d. %s", i + 1, ini_files[i]);
                reset_color();
            } else {
                set_color(COLOR_GRAY);
                printf("   %d. ", i + 1);
                set_color(COLOR_WHITE);
                printf("%s", ini_files[i]);
                reset_color();
            }
        }
        
        /* Instructions */
        set_color(COLOR_GRAY);
        move_cursor(box_x, start_y + list_height + 1);
        printf("  Up/Down Navigate  ENTER Select  ESC Cancel");
        reset_color();
        
        /* Get key */
#ifdef _WIN32
        key = _getch();
        if (key == 224) { /* Arrow key prefix */
            key = _getch();
            if (key == 72) { /* Up */
                selected--;
                if (selected < 0) selected = file_count - 1;
            } else if (key == 80) { /* Down */
                selected++;
                if (selected >= file_count) selected = 0;
            }
        } else if (key == 13) { /* Enter */
            return selected;
        } else if (key == 27) { /* Escape */
            return -1;
        } else if (key >= '1' && key <= '9') {
            int num = key - '1';
            if (num < file_count) return num;
        }
#else
        key = getchar();
        if (key == 27) { /* Escape sequence */
            if (getchar() == '[') {
                key = getchar();
                if (key == 'A') { /* Up */
                    selected--;
                    if (selected < 0) selected = file_count - 1;
                } else if (key == 'B') { /* Down */
                    selected++;
                    if (selected >= file_count) selected = 0;
                }
            } else {
                return -1;
            }
        } else if (key == '\n') {
            return selected;
        } else if (key >= '1' && key <= '9') {
            int num = key - '1';
            if (num < file_count) return num;
        }
#endif
    }
}

/*
 * Show main menu (Produce/Consume)
 * Returns: 1 = Produce, 2 = Consume, 0 = Exit
 */
static int show_main_menu(void) {
    int selected = 0;
    int key;
    const char *options[] = {
        "[>] PRODUCE MESSAGES",
        "[<] CONSUME MESSAGES",
        "[X] EXIT"
    };
    int num_options = 3;
    int width = 50;
    int i;
    
    while (1) {
        clear_screen();
        
        /* Get console width for centering */
        int console_width = get_console_width();
        int box_x = (console_width - width) / 2;
        
        /* Draw main box */
        draw_box(box_x, 1, width, 12);
        
        /* Title */
        set_color(COLOR_CYAN);
        move_cursor(box_x + 11, 3);
        printf("* KAFKA CLI TOOL v%s *", VERSION);
        reset_color();
        
        /* Subtitle */
        set_color(COLOR_GRAY);
        move_cursor(box_x + 11, 5);
        printf("mTLS-secured Kafka Client");
        reset_color();
        
        /* Separator line */
        move_cursor(box_x + 2, 6);
        for (i = 0; i < width - 4; i++) printf("-");
        
        /* Menu options */
        for (i = 0; i < num_options; i++) {
            move_cursor(box_x + 10, 8 + i * 2);
            if (i == selected) {
                set_color(COLOR_YELLOW);
                printf(" > %s <", options[i]);
                reset_color();
            } else {
                set_color(COLOR_WHITE);
                printf("   %s", options[i]);
                reset_color();
            }
        }
        
        /* Instructions */
        set_color(COLOR_GRAY);
        move_cursor(box_x + 4, 15);
        printf("Up/Down Navigate  ENTER Select  1/2/3 Quick Select");
        reset_color();
        
        /* Get key press */
#ifdef _WIN32
        key = _getch();
        if (key == 224) { /* Arrow key prefix on Windows */
            key = _getch();
            if (key == 72) { /* Up arrow */
                selected--;
                if (selected < 0) selected = num_options - 1;
            } else if (key == 80) { /* Down arrow */
                selected++;
                if (selected >= num_options) selected = 0;
            }
        } else if (key == 13) { /* Enter */
            return selected + 1;
        } else if (key >= '1' && key <= '3') {
            return key - '0';
        }
#else
        key = getchar();
        if (key == 27) { /* Escape sequence */
            getchar(); /* skip [ */
            key = getchar();
            if (key == 'A') { /* Up */
                selected--;
                if (selected < 0) selected = num_options - 1;
            } else if (key == 'B') { /* Down */
                selected++;
                if (selected >= num_options) selected = 0;
            }
        } else if (key == '\n') {
            return selected + 1;
        } else if (key >= '1' && key <= '3') {
            return key - '0';
        }
#endif
    }
}

/*
 * Run TUI and get user selections
 * Returns 1 on success, 0 on exit
 */
static int run_tui(char *selected_ini_file, int *selected_mode) {
    char ini_files[MAX_INI_FILES][MAX_FILENAME_LENGTH];
    int file_count;
    int ini_index;
    
    init_console();
    
    /* Show main menu */
    *selected_mode = show_main_menu();
    
    if (*selected_mode == 3) { /* Exit */
        clear_screen();
        set_color(COLOR_CYAN);
        printf("\n  Goodbye!\n\n");
        reset_color();
        restore_console();
        return 0;
    }
    
    /* Find INI files */
    file_count = find_ini_files(ini_files);
    
    if (file_count == 0) {
        /* No INI files found, use default */
        strcpy(selected_ini_file, DEFAULT_INI_FILE);
        clear_screen();
        set_color(COLOR_YELLOW);
        printf("\n  ! No .ini files found. Using default: %s\n\n", DEFAULT_INI_FILE);
        reset_color();
        printf("  Press any key to continue...");
#ifdef _WIN32
        _getch();
#else
        getchar();
#endif
    } else if (file_count == 1) {
        /* Only one INI file, use it */
        strcpy(selected_ini_file, ini_files[0]);
    } else {
        /* Multiple INI files, let user select */
        ini_index = show_ini_selector(ini_files, file_count);
        if (ini_index < 0) {
            /* User cancelled */
            clear_screen();
            restore_console();
            return 0;
        }
        strcpy(selected_ini_file, ini_files[ini_index]);
    }
    
    restore_console();
    return 1;
}

/*
 * Sanitize topic name for use in filename
 * Replaces special characters with underscores
 */
static void sanitize_filename(char *dst, const char *src, size_t size) {
    size_t i, j;
    for (i = 0, j = 0; src[i] != '\0' && j < size - 1; i++) {
        char c = src[i];
        /* Allow alphanumeric, hyphen, underscore */
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || 
            (c >= '0' && c <= '9') || c == '-' || c == '_') {
            dst[j++] = c;
        } else {
            dst[j++] = '_';
        }
    }
    dst[j] = '\0';
}

/*
 * Initialize log file
 * Creates logs directory and opens file with naming: topic-name_mode_datetime.log
 */
static void init_log_file(const char *topic, const char *mode) {
    time_t now;
    struct tm *timeinfo;
    char datetime[64];
    char sanitized_topic[256];
    char filename[MAX_FILENAME_LENGTH];
    char filepath[MAX_FILENAME_LENGTH * 2];
    
    /* Create logs directory if it doesn't exist */
#ifdef _WIN32
    CreateDirectory(LOGS_DIR, NULL);
#else
    mkdir(LOGS_DIR, 0755);
#endif
    
    /* Get current datetime */
    time(&now);
    timeinfo = localtime(&now);
    strftime(datetime, sizeof(datetime), "%Y%m%d_%H%M%S", timeinfo);
    
    /* Sanitize topic name for filename */
    sanitize_filename(sanitized_topic, topic, sizeof(sanitized_topic));
    
    /* Create filename: topic-name_mode_datetime.log */
    snprintf(filename, sizeof(filename), "%s_%s_%s.log", sanitized_topic, mode, datetime);
    
    /* Full path */
#ifdef _WIN32
    snprintf(filepath, sizeof(filepath), "%s\\%s", LOGS_DIR, filename);
#else
    snprintf(filepath, sizeof(filepath), "%s/%s", LOGS_DIR, filename);
#endif
    
    /* Open log file */
    log_file = fopen(filepath, "w");
    if (log_file) {
        fprintf(log_file, "Kafka CLI Tool Log\n");
        fprintf(log_file, "==================\n");
        fprintf(log_file, "Topic: %s\n", topic);
        fprintf(log_file, "Mode: %s\n", mode);
        fprintf(log_file, "Started: %s\n", datetime);
        fprintf(log_file, "==================\n\n");
        fflush(log_file);
    }
}

/*
 * Close log file
 */
static void close_log_file(void) {
    if (log_file) {
        time_t now;
        struct tm *timeinfo;
        char datetime[64];
        
        time(&now);
        timeinfo = localtime(&now);
        strftime(datetime, sizeof(datetime), "%Y-%m-%d %H:%M:%S", timeinfo);
        
        fprintf(log_file, "\n==================\n");
        fprintf(log_file, "Finished: %s\n", datetime);
        fprintf(log_file, "==================\n");
        
        fclose(log_file);
        log_file = NULL;
    }
}

/*
 * Print usage information
 */
static void print_usage(const char *program) {
    printf("Usage: %s [options] <command>\n\n", program);
    printf("Commands:\n");
    printf("  produce    Run as producer\n");
    printf("  consume    Run as consumer\n");
    printf("\nOptions:\n");
    printf("  -c <file>  Configuration file (default: %s)\n", DEFAULT_INI_FILE);
    printf("  -m <num>   Number of messages to produce/consume (default: from config)\n");
    printf("  -v         Enable verbose logging\n");
    printf("  -V         Show version\n");
    printf("  -h         Show this help\n");
    printf("\nTUI Mode:\n");
    printf("  Run without arguments to launch interactive menu\n");
    printf("\nExamples:\n");
    printf("  %s                    # Launch TUI menu\n", program);
    printf("  %s -c config.ini produce\n", program);
    printf("  %s -v -m 100 consume\n", program);
}

/*
 * Print version information
 */
static void print_version(void) {
    printf("Kafka CLI Tool v%s\n", VERSION);
    printf("Built with librdkafka %s\n", rd_kafka_version_str());
}

/*
 * Logging function with timestamp and verbosity control
 * Also writes to log file if initialized
 */
static void log_message(int verbose, const char *level, const char *format, ...) {
    va_list args;
    time_t now;
    struct tm *timeinfo;
    char timestamp[64];
    
    if (!verbose && strcmp(level, "DEBUG") == 0) {
        return;
    }
    
    time(&now);
    timeinfo = localtime(&now);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", timeinfo);
    
    /* Print to console */
    printf("[%s] [%s] ", timestamp, level);
    
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    
    printf("\n");
    
    /* Write to log file if open */
    if (log_file) {
        fprintf(log_file, "[%s] [%s] ", timestamp, level);
        va_start(args, format);
        vfprintf(log_file, format, args);
        va_end(args);
        fprintf(log_file, "\n");
        fflush(log_file);
    }
}

/*
 * Trim whitespace from string
 */
static char* trim_whitespace(char *str) {
    char *end;
    
    while (isspace((unsigned char)*str)) str++;
    
    if (*str == 0) return str;
    
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    
    end[1] = '\0';
    return str;
}

/*
 * Parse INI configuration file
 */
static int parse_ini_file(const char *filename, Config *config) {
    FILE *file;
    char line[MAX_LINE_LENGTH];
    char *key, *value;
    char *delimiter;
    
    /* Set defaults */
    strcpy(config->brokers, "localhost:9092");
    strcpy(config->topic, "test-topic");
    strcpy(config->security_protocol, "SSL");
    strcpy(config->ssl_ca_location, "");
    strcpy(config->ssl_certificate_location, "");
    strcpy(config->ssl_key_location, "");
    strcpy(config->ssl_key_password, "");
    config->ssl_skip_certificate_verify = 0;
    config->producer_batch_size = 16384;
    config->producer_linger_ms = 5;
    config->producer_ack = 1;
    strcpy(config->consumer_group_id, "kafka-cli-consumer");
    strcpy(config->consumer_auto_offset_reset, "earliest");
    config->consumer_session_timeout_ms = 45000;
    strcpy(config->consumer_enable_auto_commit, "true");
    config->verbose = 0;
    config->message_count = 10;
    
    file = fopen(filename, "r");
    if (!file) {
        log_message(1, "WARNING", "Cannot open config file '%s', using defaults", filename);
        return 0;
    }
    
    log_message(1, "INFO", "Loading configuration from: %s", filename);
    
    while (fgets(line, sizeof(line), file)) {
        /* Remove comments */
        char *comment = strchr(line, ';');
        if (comment) *comment = '\0';
        comment = strchr(line, '#');
        if (comment) *comment = '\0';
        
        /* Trim whitespace */
        char *trimmed = trim_whitespace(line);
        if (strlen(trimmed) == 0) continue;
        
        /* Skip section headers */
        if (trimmed[0] == '[') continue;
        
        /* Find key-value delimiter */
        delimiter = strchr(trimmed, '=');
        if (!delimiter) continue;
        
        *delimiter = '\0';
        key = trim_whitespace(trimmed);
        value = trim_whitespace(delimiter + 1);
        
        /* Parse configuration values */
        if (strcmp(key, "brokers") == 0) {
            strncpy(config->brokers, value, MAX_VALUE_LENGTH - 1);
        } else if (strcmp(key, "topic") == 0) {
            strncpy(config->topic, value, MAX_VALUE_LENGTH - 1);
        } else if (strcmp(key, "security_protocol") == 0) {
            strncpy(config->security_protocol, value, MAX_VALUE_LENGTH - 1);
        } else if (strcmp(key, "ssl_ca_location") == 0) {
            strncpy(config->ssl_ca_location, value, MAX_VALUE_LENGTH - 1);
        } else if (strcmp(key, "ssl_certificate_location") == 0) {
            strncpy(config->ssl_certificate_location, value, MAX_VALUE_LENGTH - 1);
        } else if (strcmp(key, "ssl_key_location") == 0) {
            strncpy(config->ssl_key_location, value, MAX_VALUE_LENGTH - 1);
        } else if (strcmp(key, "ssl_key_password") == 0) {
            strncpy(config->ssl_key_password, value, MAX_VALUE_LENGTH - 1);
        } else if (strcmp(key, "ssl_skip_certificate_verify") == 0) {
            config->ssl_skip_certificate_verify = atoi(value);
        } else if (strcmp(key, "producer_batch_size") == 0) {
            config->producer_batch_size = atoi(value);
        } else if (strcmp(key, "producer_linger_ms") == 0) {
            config->producer_linger_ms = atoi(value);
        } else if (strcmp(key, "producer_ack") == 0) {
            config->producer_ack = atoi(value);
        } else if (strcmp(key, "consumer_group_id") == 0) {
            strncpy(config->consumer_group_id, value, MAX_VALUE_LENGTH - 1);
        } else if (strcmp(key, "consumer_auto_offset_reset") == 0) {
            strncpy(config->consumer_auto_offset_reset, value, MAX_VALUE_LENGTH - 1);
        } else if (strcmp(key, "consumer_session_timeout_ms") == 0) {
            config->consumer_session_timeout_ms = atoi(value);
        } else if (strcmp(key, "consumer_enable_auto_commit") == 0) {
            strncpy(config->consumer_enable_auto_commit, value, MAX_VALUE_LENGTH - 1);
        } else if (strcmp(key, "verbose") == 0) {
            config->verbose = atoi(value);
        } else if (strcmp(key, "message_count") == 0) {
            config->message_count = atoi(value);
        }
    }
    
    fclose(file);
    return 0;
}

/*
 * Print configuration (with sensitive data masked)
 */
static void print_config(const Config *config) {
    log_message(1, "CONFIG", "=== Configuration ===");
    log_message(1, "CONFIG", "Brokers: %s", config->brokers);
    log_message(1, "CONFIG", "Topic: %s", config->topic);
    log_message(1, "CONFIG", "Security Protocol: %s", config->security_protocol);
    log_message(1, "CONFIG", "CA Location: %s", 
                strlen(config->ssl_ca_location) > 0 ? config->ssl_ca_location : "(not set)");
    log_message(1, "CONFIG", "Certificate Location: %s",
                strlen(config->ssl_certificate_location) > 0 ? config->ssl_certificate_location : "(not set)");
    log_message(1, "CONFIG", "Key Location: %s",
                strlen(config->ssl_key_location) > 0 ? config->ssl_key_location : "(not set)");
    log_message(1, "CONFIG", "Key Password: %s",
                strlen(config->ssl_key_password) > 0 ? "***" : "(not set)");
    log_message(1, "CONFIG", "Message Count: %d", config->message_count);
    log_message(1, "CONFIG", "Verbose: %d", config->verbose);
    log_message(1, "CONFIG", "Auto Commit: %s", config->consumer_enable_auto_commit);
    log_message(1, "CONFIG", "Skip Certificate Verify: %s", 
                config->ssl_skip_certificate_verify ? "true" : "false");
    log_message(1, "CONFIG", "=====================");
}

/*
 * Delivery report callback for producer
 */
static void dr_msg_cb(rd_kafka_t *rk, const rd_kafka_message_t *rkmessage, void *opaque) {
    Config *config = (Config *)opaque;
    
    if (rkmessage->err) {
        log_message(config->verbose, "ERROR", "Message delivery failed: %s",
                    rd_kafka_err2str(rkmessage->err));
    } else {
        log_message(config->verbose, "DEBUG", "Message delivered to partition %d at offset %lld",
                    (int)rkmessage->partition, (long long)rkmessage->offset);
    }
}

/*
 * Create Kafka producer with mTLS configuration
 */
static rd_kafka_t* create_producer(const Config *config) {
    rd_kafka_t *rk;
    rd_kafka_conf_t *conf;
    char errstr[512];
    
    conf = rd_kafka_conf_new();
    
    /* Enable librdkafka debug logging if verbose mode */
    if (config->verbose) {
        if (rd_kafka_conf_set(conf, "debug", "all",
                              errstr, sizeof(errstr)) != RD_KAFKA_CONF_OK) {
            log_message(1, "WARNING", "Failed to set debug: %s", errstr);
        }
        if (rd_kafka_conf_set(conf, "log_level", "7",
                              errstr, sizeof(errstr)) != RD_KAFKA_CONF_OK) {
            log_message(1, "WARNING", "Failed to set log_level: %s", errstr);
        }
    }
    
    /* Set bootstrap servers */
    if (rd_kafka_conf_set(conf, "bootstrap.servers", config->brokers,
                          errstr, sizeof(errstr)) != RD_KAFKA_CONF_OK) {
        log_message(1, "ERROR", "Failed to set bootstrap.servers: %s", errstr);
        rd_kafka_conf_destroy(conf);
        return NULL;
    }
    
    /* Configure mTLS authentication */
    if (strcmp(config->security_protocol, "SSL") == 0) {
        log_message(1, "INFO", "Configuring mTLS authentication...");
        
        if (rd_kafka_conf_set(conf, "security.protocol", "SSL",
                              errstr, sizeof(errstr)) != RD_KAFKA_CONF_OK) {
            log_message(1, "ERROR", "Failed to set security.protocol: %s", errstr);
            rd_kafka_conf_destroy(conf);
            return NULL;
        }
        
        /* CA Certificate */
        if (strlen(config->ssl_ca_location) > 0) {
            if (rd_kafka_conf_set(conf, "ssl.ca.location", config->ssl_ca_location,
                                  errstr, sizeof(errstr)) != RD_KAFKA_CONF_OK) {
                log_message(1, "ERROR", "Failed to set ssl.ca.location: %s", errstr);
                rd_kafka_conf_destroy(conf);
                return NULL;
            }
            log_message(1, "INFO", "CA certificate configured: %s", config->ssl_ca_location);
        }
        
        /* Client Certificate */
        if (strlen(config->ssl_certificate_location) > 0) {
            if (rd_kafka_conf_set(conf, "ssl.certificate.location", config->ssl_certificate_location,
                                  errstr, sizeof(errstr)) != RD_KAFKA_CONF_OK) {
                log_message(1, "ERROR", "Failed to set ssl.certificate.location: %s", errstr);
                rd_kafka_conf_destroy(conf);
                return NULL;
            }
            log_message(1, "INFO", "Client certificate configured: %s", config->ssl_certificate_location);
        }
        
        /* Client Key */
        if (strlen(config->ssl_key_location) > 0) {
            if (rd_kafka_conf_set(conf, "ssl.key.location", config->ssl_key_location,
                                  errstr, sizeof(errstr)) != RD_KAFKA_CONF_OK) {
                log_message(1, "ERROR", "Failed to set ssl.key.location: %s", errstr);
                rd_kafka_conf_destroy(conf);
                return NULL;
            }
            log_message(1, "INFO", "Client key configured: %s", config->ssl_key_location);
        }
        
        /* Key Password (if provided) */
        if (strlen(config->ssl_key_password) > 0) {
            if (rd_kafka_conf_set(conf, "ssl.key.password", config->ssl_key_password,
                                  errstr, sizeof(errstr)) != RD_KAFKA_CONF_OK) {
                log_message(1, "ERROR", "Failed to set ssl.key.password: %s", errstr);
                rd_kafka_conf_destroy(conf);
                return NULL;
            }
            log_message(1, "INFO", "Key password configured");
        }
        
        /* Skip certificate verification (for testing only) */
        if (config->ssl_skip_certificate_verify) {
            if (rd_kafka_conf_set(conf, "enable.ssl.certificate.verification", "false",
                                  errstr, sizeof(errstr)) != RD_KAFKA_CONF_OK) {
                log_message(1, "ERROR", "Failed to disable SSL certificate verification: %s", errstr);
                rd_kafka_conf_destroy(conf);
                return NULL;
            }
            log_message(1, "WARNING", "SSL certificate verification is DISABLED - use only for testing!");
        }
    }
    
    /* Producer-specific settings */
    char batch_size_str[32], linger_str[32], acks_str[32];
    snprintf(batch_size_str, sizeof(batch_size_str), "%d", config->producer_batch_size);
    snprintf(linger_str, sizeof(linger_str), "%d", config->producer_linger_ms);
    snprintf(acks_str, sizeof(acks_str), "%d", config->producer_ack);
    
    rd_kafka_conf_set(conf, "batch.size", batch_size_str, NULL, 0);
    rd_kafka_conf_set(conf, "linger.ms", linger_str, NULL, 0);
    rd_kafka_conf_set(conf, "acks", acks_str, NULL, 0);
    
    /* Set delivery report callback */
    rd_kafka_conf_set_dr_msg_cb(conf, dr_msg_cb);
    
    /* Pass config to callback for verbose logging */
    rd_kafka_conf_set_opaque(conf, (void *)config);
    
    /* Create producer */
    rk = rd_kafka_new(RD_KAFKA_PRODUCER, conf, errstr, sizeof(errstr));
    if (!rk) {
        log_message(1, "ERROR", "Failed to create producer: %s", errstr);
        rd_kafka_conf_destroy(conf);
        return NULL;
    }
    
    log_message(1, "INFO", "Producer created successfully");
    return rk;
}

/*
 * Create Kafka consumer with mTLS configuration
 */
static rd_kafka_t* create_consumer(const Config *config) {
    rd_kafka_t *rk;
    rd_kafka_conf_t *conf;
    char errstr[512];
    char timeout_str[32];
    
    conf = rd_kafka_conf_new();
    
    /* Enable librdkafka debug logging if verbose mode */
    if (config->verbose) {
        if (rd_kafka_conf_set(conf, "debug", "all",
                              errstr, sizeof(errstr)) != RD_KAFKA_CONF_OK) {
            log_message(1, "WARNING", "Failed to set debug: %s", errstr);
        }
        if (rd_kafka_conf_set(conf, "log_level", "7",
                              errstr, sizeof(errstr)) != RD_KAFKA_CONF_OK) {
            log_message(1, "WARNING", "Failed to set log_level: %s", errstr);
        }
    }
    
    /* Set bootstrap servers */
    if (rd_kafka_conf_set(conf, "bootstrap.servers", config->brokers,
                          errstr, sizeof(errstr)) != RD_KAFKA_CONF_OK) {
        log_message(1, "ERROR", "Failed to set bootstrap.servers: %s", errstr);
        rd_kafka_conf_destroy(conf);
        return NULL;
    }
    
    /* Configure mTLS authentication */
    if (strcmp(config->security_protocol, "SSL") == 0) {
        log_message(1, "INFO", "Configuring mTLS authentication...");
        
        if (rd_kafka_conf_set(conf, "security.protocol", "SSL",
                              errstr, sizeof(errstr)) != RD_KAFKA_CONF_OK) {
            log_message(1, "ERROR", "Failed to set security.protocol: %s", errstr);
            rd_kafka_conf_destroy(conf);
            return NULL;
        }
        
        /* CA Certificate */
        if (strlen(config->ssl_ca_location) > 0) {
            if (rd_kafka_conf_set(conf, "ssl.ca.location", config->ssl_ca_location,
                                  errstr, sizeof(errstr)) != RD_KAFKA_CONF_OK) {
                log_message(1, "ERROR", "Failed to set ssl.ca.location: %s", errstr);
                rd_kafka_conf_destroy(conf);
                return NULL;
            }
            log_message(1, "INFO", "CA certificate configured: %s", config->ssl_ca_location);
        }
        
        /* Client Certificate */
        if (strlen(config->ssl_certificate_location) > 0) {
            if (rd_kafka_conf_set(conf, "ssl.certificate.location", config->ssl_certificate_location,
                                  errstr, sizeof(errstr)) != RD_KAFKA_CONF_OK) {
                log_message(1, "ERROR", "Failed to set ssl.certificate.location: %s", errstr);
                rd_kafka_conf_destroy(conf);
                return NULL;
            }
            log_message(1, "INFO", "Client certificate configured: %s", config->ssl_certificate_location);
        }
        
        /* Client Key */
        if (strlen(config->ssl_key_location) > 0) {
            if (rd_kafka_conf_set(conf, "ssl.key.location", config->ssl_key_location,
                                  errstr, sizeof(errstr)) != RD_KAFKA_CONF_OK) {
                log_message(1, "ERROR", "Failed to set ssl.key.location: %s", errstr);
                rd_kafka_conf_destroy(conf);
                return NULL;
            }
            log_message(1, "INFO", "Client key configured: %s", config->ssl_key_location);
        }
        
        /* Key Password (if provided) */
        if (strlen(config->ssl_key_password) > 0) {
            if (rd_kafka_conf_set(conf, "ssl.key.password", config->ssl_key_password,
                                  errstr, sizeof(errstr)) != RD_KAFKA_CONF_OK) {
                log_message(1, "ERROR", "Failed to set ssl.key.password: %s", errstr);
                rd_kafka_conf_destroy(conf);
                return NULL;
            }
            log_message(1, "INFO", "Key password configured");
        }
        
        /* Skip certificate verification (for testing only) */
        if (config->ssl_skip_certificate_verify) {
            if (rd_kafka_conf_set(conf, "enable.ssl.certificate.verification", "false",
                                  errstr, sizeof(errstr)) != RD_KAFKA_CONF_OK) {
                log_message(1, "ERROR", "Failed to disable SSL certificate verification: %s", errstr);
                rd_kafka_conf_destroy(conf);
                return NULL;
            }
            log_message(1, "WARNING", "SSL certificate verification is DISABLED - use only for testing!");
        }
    }
    
    /* Consumer-specific settings */
    if (rd_kafka_conf_set(conf, "group.id", config->consumer_group_id,
                          errstr, sizeof(errstr)) != RD_KAFKA_CONF_OK) {
        log_message(1, "ERROR", "Failed to set group.id: %s", errstr);
        rd_kafka_conf_destroy(conf);
        return NULL;
    }
    
    if (rd_kafka_conf_set(conf, "auto.offset.reset", config->consumer_auto_offset_reset,
                          errstr, sizeof(errstr)) != RD_KAFKA_CONF_OK) {
        log_message(1, "ERROR", "Failed to set auto.offset.reset: %s", errstr);
        rd_kafka_conf_destroy(conf);
        return NULL;
    }
    
    snprintf(timeout_str, sizeof(timeout_str), "%d", config->consumer_session_timeout_ms);
    if (rd_kafka_conf_set(conf, "session.timeout.ms", timeout_str,
                          errstr, sizeof(errstr)) != RD_KAFKA_CONF_OK) {
        log_message(1, "ERROR", "Failed to set session.timeout.ms: %s", errstr);
        rd_kafka_conf_destroy(conf);
        return NULL;
    }
    
    /* Enable/disable auto commit */
    if (rd_kafka_conf_set(conf, "enable.auto.commit", config->consumer_enable_auto_commit,
                          errstr, sizeof(errstr)) != RD_KAFKA_CONF_OK) {
        log_message(1, "ERROR", "Failed to set enable.auto.commit: %s", errstr);
        rd_kafka_conf_destroy(conf);
        return NULL;
    }
    log_message(1, "INFO", "Auto commit enabled: %s", config->consumer_enable_auto_commit);
    
    /* Create consumer */
    rk = rd_kafka_new(RD_KAFKA_CONSUMER, conf, errstr, sizeof(errstr));
    if (!rk) {
        log_message(1, "ERROR", "Failed to create consumer: %s", errstr);
        rd_kafka_conf_destroy(conf);
        return NULL;
    }
    
    log_message(1, "INFO", "Consumer created successfully (Group ID: %s)", config->consumer_group_id);
    return rk;
}

/*
 * Produce messages to Kafka
 */
static int produce_messages(rd_kafka_t *rk, const Config *config) {
    rd_kafka_resp_err_t err;
    char message[1024];
    int i;
    
    log_message(1, "INFO", "Starting to produce %d messages to topic '%s'...",
                config->message_count, config->topic);
    
    for (i = 0; i < config->message_count; i++) {
        snprintf(message, sizeof(message), 
                 "Test message %d from Kafka CLI at %ld", i + 1, (long)time(NULL));
        
        /* Produce message */
        err = rd_kafka_producev(
            rk,
            RD_KAFKA_V_TOPIC(config->topic),
            RD_KAFKA_V_VALUE(message, strlen(message)),
            RD_KAFKA_V_MSGFLAGS(RD_KAFKA_MSG_F_COPY),
            RD_KAFKA_V_KEY(NULL, 0),
            RD_KAFKA_V_END
        );
        
        if (err) {
            log_message(1, "ERROR", "Failed to produce message %d: %s",
                        i + 1, rd_kafka_err2str(err));
        } else {
            log_message(config->verbose, "INFO", "Produced message %d/%d: %s",
                        i + 1, config->message_count, message);
        }
        
        /* Poll for delivery reports */
        rd_kafka_poll(rk, 0);
        
        /* Small delay between messages */
#ifdef _WIN32
        Sleep(10);
#else
        usleep(10000);
#endif
    }
    
    /* Wait for all messages to be delivered */
    log_message(1, "INFO", "Flushing messages...");
    rd_kafka_flush(rk, 10000);
    
    log_message(1, "INFO", "Produced %d messages successfully", config->message_count);
    return 0;
}

/*
 * Signal handler to stop consumer
 */
static void stop_consumer(int sig) {
    log_message(1, "INFO", "Received signal %d, shutting down...", sig);
    run = 0;
    if (global_kafka_handle) {
        rd_kafka_consumer_close(global_kafka_handle);
    }
}

/*
 * Consume messages from Kafka
 */
static int consume_messages(rd_kafka_t *rk, const Config *config) {
    rd_kafka_topic_partition_list_t *topics;
    rd_kafka_resp_err_t err;
    rd_kafka_message_t *rkmessage;
    int msg_count = 0;
    
    global_kafka_handle = rk;
    
    /* Subscribe to topic */
    topics = rd_kafka_topic_partition_list_new(1);
    rd_kafka_topic_partition_list_add(topics, config->topic, RD_KAFKA_PARTITION_UA);
    
    err = rd_kafka_subscribe(rk, topics);
    if (err) {
        log_message(1, "ERROR", "Failed to subscribe to topic: %s", rd_kafka_err2str(err));
        rd_kafka_topic_partition_list_destroy(topics);
        return 1;
    }
    
    rd_kafka_topic_partition_list_destroy(topics);
    
    log_message(1, "INFO", "Subscribed to topic '%s'", config->topic);
    log_message(1, "INFO", "Waiting for messages... (Press Ctrl+C to stop)");
    
    /* Consume messages */
    while (run && (config->message_count == 0 || msg_count < config->message_count)) {
        rkmessage = rd_kafka_consumer_poll(rk, 1000);
        
        if (!rkmessage) {
            /* Timeout - no message */
            continue;
        }
        
        if (rkmessage->err) {
            if (rkmessage->err == RD_KAFKA_RESP_ERR__PARTITION_EOF) {
                log_message(config->verbose, "DEBUG", "Reached end of partition");
            } else {
                log_message(1, "ERROR", "Consumer error: %s",
                            rd_kafka_message_errstr(rkmessage));
            }
        } else {
            /* Valid message received */
            msg_count++;
            log_message(1, "INFO", "Received message %d:", msg_count);
            log_message(1, "INFO", "  Topic: %s", rd_kafka_topic_name(rkmessage->rkt));
            log_message(1, "INFO", "  Partition: %d", (int)rkmessage->partition);
            log_message(1, "INFO", "  Offset: %lld", (long long)rkmessage->offset);
            log_message(1, "INFO", "  Key: %.*s",
                        (int)rkmessage->key_len, (char *)rkmessage->key);
            log_message(1, "INFO", "  Value: %.*s",
                        (int)rkmessage->len, (char *)rkmessage->payload);
            
            /* Store offset */
            rd_kafka_offset_store(rkmessage->rkt, rkmessage->partition, rkmessage->offset);
        }
        
        rd_kafka_message_destroy(rkmessage);
    }
    
    log_message(1, "INFO", "Consumed %d messages", msg_count);
    return 0;
}

/*
 * Wait for user to press a key before exiting
 */
static void wait_for_key_press(void) {
    printf("\n");
    printf("========================================\n");
    printf("  Press any key to exit...\n");
    printf("========================================\n");
#ifdef _WIN32
    _getch();
#else
    getchar();
#endif
}

/*
 * Main function
 */
int main(int argc, char **argv) {
    Config config;
    rd_kafka_t *rk = NULL;
    const char *config_file = NULL;
    const char *command = NULL;
    int i;
    int is_producer = 0;
    int is_consumer = 0;
    char tui_ini_file[MAX_FILENAME_LENGTH];
    int tui_mode = 0;
    int use_tui = 0;
    
    /* Check if we should use TUI (no arguments provided) */
    if (argc == 1) {
        use_tui = 1;
    }
    
    /* Initialize config defaults first */
    memset(&config, 0, sizeof(config));
    
    /* Run TUI if no arguments */
    if (use_tui) {
        if (!run_tui(tui_ini_file, &tui_mode)) {
            return 0; /* User exited from TUI */
        }
        config_file = tui_ini_file;
        if (tui_mode == 1) {
            is_producer = 1;
            command = "produce";
        } else if (tui_mode == 2) {
            is_consumer = 1;
            command = "consume";
        }
    } else {
        /* Parse command line arguments */
        for (i = 1; i < argc; i++) {
            if (strcmp(argv[i], "-h") == 0) {
                print_usage(argv[0]);
                return 0;
            } else if (strcmp(argv[i], "-V") == 0) {
                print_version();
                return 0;
            } else if (strcmp(argv[i], "-v") == 0) {
                config.verbose = 1;
            } else if (strcmp(argv[i], "-c") == 0 && i + 1 < argc) {
                config_file = argv[++i];
            } else if (strcmp(argv[i], "-m") == 0 && i + 1 < argc) {
                config.message_count = atoi(argv[++i]);
            } else if (strcmp(argv[i], "produce") == 0) {
                is_producer = 1;
                command = "produce";
            } else if (strcmp(argv[i], "consume") == 0) {
                is_consumer = 1;
                command = "consume";
            }
        }
    }
    
    if (!command) {
        print_usage(argv[0]);
        return 1;
    }
    
    /* Use default config file if not specified */
    if (!config_file) {
        config_file = DEFAULT_INI_FILE;
    }
    
    /* Print version info */
    print_version();
    
    /* Load configuration */
    if (parse_ini_file(config_file, &config) != 0) {
        return 1;
    }
    
    /* Override verbose from command line */
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-v") == 0) {
            config.verbose = 1;
        }
    }
    
    /* Initialize log file */
    init_log_file(config.topic, command);
    
    print_config(&config);
    
    /* Validate mTLS configuration */
    if (strcmp(config.security_protocol, "SSL") == 0) {
        if (strlen(config.ssl_ca_location) == 0) {
            log_message(1, "ERROR", "mTLS is enabled but ssl_ca_location is not set");
            close_log_file();
            wait_for_key_press();
            return 1;
        }
        if (strlen(config.ssl_certificate_location) == 0) {
            log_message(1, "ERROR", "mTLS is enabled but ssl_certificate_location is not set");
            close_log_file();
            wait_for_key_press();
            return 1;
        }
        if (strlen(config.ssl_key_location) == 0) {
            log_message(1, "ERROR", "mTLS is enabled but ssl_key_location is not set");
            close_log_file();
            wait_for_key_press();
            return 1;
        }
    }
    
    /* Setup signal handler for consumer */
    if (is_consumer) {
        signal(SIGINT, stop_consumer);
        signal(SIGTERM, stop_consumer);
    }
    
    /* Create Kafka client */
    if (is_producer) {
        rk = create_producer(&config);
        if (!rk) {
            close_log_file();
            wait_for_key_press();
            return 1;
        }
        
        /* Produce messages */
        produce_messages(rk, &config);
    } else if (is_consumer) {
        rk = create_consumer(&config);
        if (!rk) {
            close_log_file();
            wait_for_key_press();
            return 1;
        }
        
        /* Consume messages */
        consume_messages(rk, &config);
        
        /* Close consumer */
        log_message(1, "INFO", "Closing consumer...");
        rd_kafka_consumer_close(rk);
    }
    
    /* Destroy Kafka handle */
    if (rk) {
        rd_kafka_destroy(rk);
    }
    
    log_message(1, "INFO", "Application finished");
    
    /* Close log file */
    close_log_file();
    
    /* Wait for user to press a key before closing */
    wait_for_key_press();
    
    return 0;
}
