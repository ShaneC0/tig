#include <unistd.h>
#include <openssl/evp.h>
#include "ioutil.h"

void create_object_path(char *hash, char *path) {
   sprintf(path, "tig/objects/%c%c/%s", hash[0], hash[1], hash + 2); 
}

void write_config() {
    FILE *config = open_safe("tig/.tigconfig", "w");
    char name[64];
    char cwd[1024];
    getcwd(cwd, sizeof(cwd));
    prompt_input("PROMPT -- Enter name: ", name, sizeof(name));
    fprintf(config, "name %s\ncwd %s", name, cwd);
    close_safe(config);
}

void write_ref(char *path, char *value) {
    FILE *ref = open_safe(path, "w");
    fprintf(ref, "%s\n", value);
    close_safe(ref);
}

void read_ref(char *path, char *value, size_t value_sz) {
    FILE *f = open_safe(path, "r");
    if(fgets(value, value_sz, f) != NULL) {
        value[strcspn(value, "\n")] = '\0';
    }
}

void sha1(char *input, char *output) {
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int length = 0;
    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
    if (mdctx == NULL) {
        printf("ERROR -- Error initializing EVP_MD_CTX\n");
        exit(1);
    }
    if (EVP_DigestInit_ex(mdctx, EVP_sha1(), NULL) != 1) {
        printf("ERROR -- Error initializing digest\n");
        exit(1);
    }
    if (EVP_DigestUpdate(mdctx, input, strlen(input)) != 1) {
        printf("ERROR -- Error updating digest\n");
        exit(1);
    }
    if (EVP_DigestFinal_ex(mdctx, hash, &length) != 1) {
        printf("ERROR -- Error finalizing digest\n");
        exit(1);
    }
    EVP_MD_CTX_free(mdctx);
    for (unsigned int i = 0; i < length; ++i) {
        sprintf(output + (i * 2), "%02x", hash[i]);
    }
    output[length * 2] = '\0';
}


void write_object(char *type, char *file_content, char *hash_to_create) {
    // Length of type id, length of all file content, 8 for file size digits
    size_t hash_str_size = strlen(type) + strlen(file_content) + 8;
    char hash_str[hash_str_size];
    char dirpath[128];
    char filepath[256];
    sprintf(hash_str, "%s %zu %s", type, strlen(file_content), file_content);
    sha1(hash_str, hash_to_create);
    sprintf(dirpath, "tig/objects/%c%c", hash_to_create[0], hash_to_create[1]);
    mkdir_safe(dirpath, 1);
    sprintf(filepath, "%s/%s", dirpath, hash_to_create + 2);
    if(access(filepath, F_OK) != -1) {
        printf("INFO -- Object file already exists %s\n", filepath);
        return;
    }
    FILE *object_file = open_safe(filepath, "w");
    fputs(file_content, object_file);
    close_safe(object_file);
}

void write_file_tree(char *hash_to_create, char *basepath) {
    struct dirent *files;
    FILE *temp_file = open_safe("tig/snapshot.temp", "w");
    DIR *dir = opendir_safe(basepath);
    while((files = readdir(dir)) != NULL) {
        if(strcmp(files->d_name, ".") == 0 || strcmp(files->d_name, "..") == 0) continue;         
        struct stat statbuf;
        char path[256];
        sprintf(path, "%s/%s", basepath, files->d_name);
        stat(path, &statbuf);
        if(S_ISDIR(statbuf.st_mode)) {
            char tree_hash[41];
            write_file_tree(tree_hash, path);
            fprintf(temp_file, "tree %s %s\n", tree_hash, files->d_name);
        } else if(S_ISREG(statbuf.st_mode)) {
            char blob_hash[41];
            char *file_content = read_to_buffer(path); 
            write_object("blob", file_content, blob_hash);
            free(file_content);
            fprintf(temp_file, "blob %s %s\n", blob_hash, files->d_name);
        } else {
            printf("ERROR -- Unsupported filetype");
            exit(1);
        }
    }
    close_safe(temp_file);
    char *tree_content = read_to_buffer("tig/snapshot.temp");
    write_object("tree", tree_content, hash_to_create);
    free(tree_content);
    closedir(dir);
}

void write_work_directory(char *tree_hash, char *basepath) {
    char dirpath[64];
    char filepath[128];
    mkdir_safe(basepath, 1);
    sprintf(dirpath, "tig/objects/%c%c", tree_hash[0], tree_hash[1]);
    if(access(dirpath, F_OK) == -1) {
        printf("ERROR -- Can't find tree directory!");
    }
    sprintf(filepath, "%s/%s", dirpath, tree_hash + 2);
    FILE *tree_file = open_safe(filepath, "r");
    char type[16];
    char hash[41];
    char name[32];
    char path[128];
    while(fscanf(tree_file, "%s %s %s", type, hash, name) != EOF) {
        sprintf(path, "%s/%s", basepath, name);
        if(strncmp(type, "blob", 4) == 0) {
            char blob_path[128];
            create_object_path(hash, blob_path);
            if(access(path, F_OK) == -1) {
                //file doesn't exist
                //so copy blob to working dir
                FILE *new_file = open_safe(path, "w");
                char *blob_content = read_to_buffer(blob_path);
                fputs(blob_content, new_file);
                free(blob_content);
                close_safe(new_file);
            } else {
                //file exists
                //compute hash
                char existing_file_hash[41];
                char *file_content = read_to_buffer(path);
                size_t hash_str_size = strlen(type) + strlen(file_content) + 8;
                char hash_str[hash_str_size];
                sprintf(hash_str, "%s %zu %s", type, strlen(file_content), file_content);
                sha1(hash_str, existing_file_hash);
                free(file_content);
                //compare with existing hash
                if(strncmp(hash, existing_file_hash, 40) != 0) {
                    //overwrite existing file
                    FILE *existing_file = open_safe(path, "w");
                    char *blob_content = read_to_buffer(blob_path);
                    fputs(blob_content, existing_file);
                    free(blob_content); 
                    close_safe(existing_file);
                }
            }
        } else if (strncmp(type, "tree", 4) == 0) {
            write_work_directory(hash, path);
        }
    }
    close_safe(tree_file);
}
