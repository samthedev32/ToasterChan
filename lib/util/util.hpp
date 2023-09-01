#pragma once

#include <mpg123.h>
#include <out123.h>

#include <dirent.h>
#include <string.h>

#include <fstream>
#include <string>
#include <vector>

// Get Token from File
std::string getToken(std::string path) {
    std::ifstream f(path);

    if (!f.is_open()) {
        perror("getToken()");
        fprintf(stderr, "Failed to open file\n");
        exit(1);
    }

    std::string token((std::istreambuf_iterator<char>(f)),
                      (std::istreambuf_iterator<char>()));

    return token;
}

void scanForFiles(std::vector<std::string> &files) {
    files.clear();

    // List all Music Files
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir("audio/")) != NULL) {
        /* print all the files and directories within directory */
        printf("Audio Files:\n");
        while ((ent = readdir(dir)) != NULL) {
            if (ent->d_type == DT_REG) {
                const char *ext = strrchr(ent->d_name, '.');
                if (!strcmp(ext, ".mp3")) {
                    files.push_back(ent->d_name);
                    printf("%s\n", ent->d_name);
                }
            }
        }
        printf("----\n");
        closedir(dir);
    } else {
        // could not open directory
    }
}

bool loadMusic(std::vector<uint8_t> &data, const char *path) {
    data.clear();

    mpg123_init();

    int err = 0;
    unsigned char *buffer;
    size_t buffer_size, done;
    int channels, encoding;
    long rate;

    /* Note it is important to force the frequency to 48000 for
     * Discord compatibility */
    mpg123_handle *mh = mpg123_new(NULL, &err);
    mpg123_param(mh, MPG123_FORCE_RATE, 48000, 48000.0);

    /* Decode entire file into a vector. You could do this on
     * the fly, but if you do that you may get timing issues if
     * your CPU is busy at the time and you are streaming to a
     * lot of channels/guilds.
     */
    buffer_size = mpg123_outblock(mh);
    buffer = new unsigned char[buffer_size];

    /* Note: In a real world bot, this should have some error
     * logging */
    mpg123_open(mh, path);
    mpg123_getformat(mh, &rate, &channels, &encoding);

    unsigned int counter = 0;
    for (int totalBytes = 0;
         mpg123_read(mh, buffer, buffer_size, &done) == MPG123_OK;) {
        for (auto i = 0; i < buffer_size; i++) {
            data.push_back(buffer[i]);
        }
        counter += buffer_size;
        totalBytes += done;
    }

    delete buffer;
    mpg123_close(mh);
    mpg123_delete(mh);

    return true;
}

int min(int a, int b) {
    if (a < b)
        return a;
    return b;
}

// Download Music
int ytdlp(std::string search, std::string &id) {
    FILE *fp;
    int return_code;

    // Run the command and capture its output
    fp = popen(("../../bin/ytdlp --no-warnings -x --audio-format mp3 "
                "--output 'audio/%(id)s' ytsearch:'" +
                search + "'")
                   .c_str(),
               "r");

    if (fp == NULL) {
        perror("popen");
        return -1;
    }

    // Read the captured output
    id.resize(64);
    if (fgets(id.data(), 64, fp) == NULL)
        perror("yt-dlp");

    return pclose(fp);
}

// system() with va_args
int run(const char *text, ...) {
    char *command;

    va_list args;
    va_start(args, text);
    vasprintf(&command, text, args);
    va_end(args);

    return system(command);
}