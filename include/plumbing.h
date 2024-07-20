#include <unistd.h>
#include <openssl/evp.h>
#include "ioutil.h"

#define MAX(a, b) ((a) > (b) ? (a) : (b))

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
        if(strcmp(files->d_name, ".") == 0 || strcmp(files->d_name, "..") == 0 || strcmp(files->d_name, ".git") == 0 || strcmp(files->d_name, "tig") == 0) continue;         
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
    char filepath[128];
    mkdir_safe(basepath, 1);
    create_object_path(tree_hash, filepath);
    if(access(filepath, F_OK) == -1) {
        printf("ERROR -- Error reading tree file %s\n", tree_hash);
        exit(1);
    }
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

void free_lcs(char **lcs, int index) {
    for(int i = 0; i < index; i++) {
        free(lcs[index]);
    }
    free(lcs);
}

void backtrack_diff(int *diff[], char **X, char **Y, int i, int j, char **lcs, int *index) {
    if(i == 0 || j == 0) return;
    if(strcmp(X[i-1], Y[j-1]) == 0) {
        backtrack_diff(diff, X, Y, i-1, j-1, lcs, index);
        lcs[*index] = strdup(X[i-1]);
        (*index)++;
    } else if(diff[i][j-1] > diff[i-1][j]) {
        backtrack_diff(diff, X, Y, i, j-1, lcs, index);
    } else {
        backtrack_diff(diff, X, Y, i-1, j, lcs, index);
    }
}

char **longest_common_subsequence(char **X, char **Y, int Xn, int Yn, int *index) {
    int i, j;
    int **diff = malloc((Xn + 1) * sizeof(int *));
    for (i = 0; i <= Xn; i++) {
        diff[i] = malloc((Yn + 1) * sizeof(int));
    }
    for(i = 0; i <= Xn; i++) {
        for(j = 0; j <= Yn; j++) {
            if(i == 0 || j == 0) {
                diff[i][j] = 0;
            } else if(strcmp(X[i-1], Y[j-1]) == 0) {
                diff[i][j] = diff[i-1][j-1] + 1;
            } else {
                diff[i][j] = MAX(diff[i][j-1], diff[i-1][j]);
            }
        }
    }
    char **lcs = malloc((diff[Xn][Yn] + 1) * sizeof(char *));
    *index = 0;
    backtrack_diff(diff, X, Y, Xn, Yn, lcs, index);
    lcs[*index] = NULL;
    for(i = 0; i <=Xn; i++) {
        free(diff[i]);
    }
    free(diff);
    return lcs;
}

void apply_file_diff(char *path, char *patch_hash) {
    char patch_path[256];
    int patch_n;
    int f_n;
    create_object_path(patch_hash, patch_path);
    char **patch = read_to_lines(patch_path, &patch_n); 
    char **f = read_to_lines(path, &f_n);
    char **new_lines = malloc((patch_n + f_n) * sizeof(char *));
    int newline_ct = 0;
    int i = 0, j = 0;
    while(j < patch_n) {
        if(patch[j][0] == ' ') {
            new_lines[newline_ct++] = strdup(patch[j] + 1);
            i++;
        } else if(patch[j][0] == '+') {
            new_lines[newline_ct++] = strdup(patch[j] + 1);
        } else if(patch[j][0] == '-') {
            i++;
        }
        j++;
    }
    while(i < f_n) {
        new_lines[newline_ct++] = strdup(f[i++]);
    }
    FILE *file = open_safe(path, "w");
    for(int k = 0; k < newline_ct; k++) {
        fprintf(file, "%s\n", new_lines[k]);
        free(new_lines[k]);
    }
    close_safe(file);
    free(new_lines);
    free(patch);
    free(f);
}

void file_diff(char *path1, char *path2, char *patch_hash, int apply) {
    int Xn, Yn;
    char **X = read_to_lines(path1, &Xn);
    char **Y = read_to_lines(path2, &Yn);
    int lcs_n;
    char **lcs = longest_common_subsequence(X, Y, Xn, Yn, &lcs_n);
    int i = 0, j = 0, k = 0;
    size_t patch_sz = 1024;
    char *patch = malloc(patch_sz); //----------------------------------
    char line[512];
    patch[0] = '\0';
    while (i < Xn || j < Yn) {
        if (i < Xn && (k >= lcs_n || strcmp(X[i], lcs[k]) != 0)) {
            snprintf(line, sizeof(line), "-%s\n", X[i]);
            i++;
        } else if (j < Yn && (k >= lcs_n || strcmp(Y[j], lcs[k]) != 0)) {
            snprintf(line, sizeof(line), "+%s\n", Y[j]);
            j++;
        } else {
            snprintf(line, sizeof(line), " %s\n", X[i]);
            i++;
            j++;
            k++;
        }
        if (strlen(patch) + strlen(line) >= patch_sz) {
            patch_sz *= 2;
            patch = realloc(patch, patch_sz);
            if (!patch) {
                printf("ERROR -- Error allocating memory for patch!");
            }
        }
        strncat(patch, line, patch_sz - strlen(patch) - 1);
    }
    free_lcs(lcs, lcs_n);
    free_lines(X, Xn);
    free_lines(Y, Yn);
    write_object("patch", patch, patch_hash);
    free(patch);
    if(apply) apply_file_diff(path1, patch_hash);
}

bool file_is_modified(char *path1, char *path2) {

}

void dir_diff(char *dir1, char *dir2, char *patch_hash) {
    DIR *d1 = opendir_safe(dir1);
    DIR *d2 = opendir_safe(dir2);
    struct dirent *entry1;
    struct dirent *entry2;

    while((entry1 = readdir(d1)) != NULL) {
        if(strcmp(entry1->d_name, ".") == 0 || strcmp(entry1->d_name, "..") == 0) continue;
        char path1[256];
        char path2[256];
        sprintf(path1, "%s/%s", dir1, entry1->d_name);
        sprintf(path2, "%s/%s", dir2, entry2->d_name);
        struct stat stat1;
        struct stat stat2;
    }
    while((entry2 = readdir(d2)) != NULL) {
        if(strcmp(entry2->d_name, ".") == 0 || strcmp(entry2->d_name, "..") == 0) continue;
    }
    closedir(d1);
    closedir(d2);
}

    
    // TODO -- Write function to show additions and deletions from Xn -> Yn with line numbers
    //int i = 0, j = 0, k = 0;
    //while (i < Xn || j < Yn) {
        //if (i < Xn && (k >= lcs_n || strcmp(X[i], lcs[k]) != 0)) {
            //printf("--D-- %d: %s\n", i + 1, X[i]);
            //i++;
        //} else if (j < Yn && (k >= lcs_n || strcmp(Y[j], lcs[k]) != 0)) {
            //printf("++A++ %d: %s\n", j + 1, Y[j]);
            //j++;
        //} else {
            //printf("      %d: %s\n", i + 1, X[i]);
            //i++;
            //j++;
            //k++;
        //}
    //}
