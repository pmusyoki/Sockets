#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>
#include <pthread.h>

int is_white_space(char c) {
    return (c == ' ' || c == '\t' || c == '\n');
}

int get_first_position(char const *str) {
    int i = 0;
    while (is_white_space(str[i])) {
        i += 1;
    }
    return (i);
}

int get_str_len(char const *str) {
    int len = 0;
    while (str[len] != '\0') {
        len += 1;
    }
    return (len);
}

int get_last_position(char const *str) {
    int i = get_str_len(str) - 1;
    while (is_white_space(str[i])) {
        i -= 1;
    }
    return (i);
}

int get_trim_len(char const *str) {
    return (get_last_position(str) - get_first_position(str));
}

char *strtrim(char const *str) {
    char *trim = NULL;
    int i, len, start, end;
    if (str != NULL) {
        i = 0;
        len = get_trim_len(str) + 1;
        trim = (char *)malloc(len);
        start = get_first_position(str);
        while (i < len) {
            trim[i] = str[start];
            i += 1;
            start += 1;
        }
        trim[i] = '\0';
    }
    return (trim);
}

void DieWithError(char *message) {
    printf("\033[1;31mTERMINATING\033[0m: Error: [%s]\n", message);
    exit(EXIT_FAILURE);
}

void StatusSuccess(char *message) {
    printf("\033[1;32mSUCCESS\033[0m: Message: [%s]\n", message);
}