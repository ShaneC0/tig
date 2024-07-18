#include "plumbing.h"

void create_object_path(char *hash, char *path) {
   sprintf(path, "tig/objects/%c%c/%s", hash[0], hash[1], hash + 2); 
}

void create_commit(char *message, char *commit_hash) {
    char tree_hash[41];
    char name[64];
    char timestamp[64];
    char message_content[1024];
    char *message_template = "parent\t%s\ntree\t%s\ncommitter\t%s\ntimestamp\t%s\nmessage\t%s\n";
    char head_ref[512];
    char head_hash[41];
    // Build file tree
    write_file_tree(tree_hash, "./test");
    if(remove("tig/snapshot.temp") != 0) {
        printf("ERROR -- Error removing snapshot.temp file!");
    }
    // Grab name from config file
    parse_file_from_prefix("tig/.tigconfig", "name ", name) ;
    generate_timestamp(timestamp, sizeof(timestamp));
    // Read head commit hash
    read_ref("tig/HEAD", head_ref, sizeof(head_ref));
    read_ref(head_ref, head_hash, sizeof(head_hash));
    // Write commit object (including head commit hash)
    sprintf(message_content, message_template, head_hash, tree_hash, name, timestamp, message);
    write_object("commit", message_content, commit_hash);
    // Update head to new commit hash
    write_ref(head_ref, commit_hash); 
}

void create_branch(char *name) {
    // Really just need to make a ref file that points to the head hash
    char head_ref[64];
    char head_hash[41];
    char ref_path[64] = "tig/refs/";
    // Get head ref
    read_ref("tig/HEAD", head_ref, sizeof(head_ref));
    //Get head hash
    read_ref(head_ref, head_hash, sizeof(head_hash));
    // Write head hash to new file
    strcat(ref_path, name);
    write_ref(ref_path, head_hash);
}

void switch_branch(char *name) {
    printf("WARNING -- Switching branch. Uncommitted changes will be lost\n");
    struct stat statbuf;
    char ref_path[64] = "tig/refs/";
    strcat(ref_path, name);
    if(stat(ref_path, &statbuf) != 0) {
        printf("ERROR -- Specified branch name does not exist %s\n", name);
    }
    write_ref("tig/HEAD", ref_path);
    char commit_hash[41];
    char commit_path[128];
    char tree_hash[41];
    read_ref(ref_path, commit_hash, sizeof(commit_hash));
    create_object_path(commit_hash, commit_path);
    parse_file_from_prefix(commit_path, "tree ", tree_hash);
    write_work_directory(tree_hash, "temp");
    // TODO: replace working dir with temp dir
}

void enumerate_commits(char *commit_hash) {
    char commit_path[128];
    create_object_path(commit_hash, commit_path);
    char *commit_content = read_to_buffer(commit_path);
    printf("%.6s\n================================================\n%s\n", commit_hash, commit_content);
    free(commit_content);
    char parent_hash[41];
    parse_file_from_prefix(commit_path, "parent ", parent_hash);
    if(strncmp(parent_hash, "root", 4) == 0) return;
    enumerate_commits(parent_hash);
}

void print_commit_history(char *branch_name) {
    char ref_path[128];
    char commit_hash[41];
    sprintf(ref_path, "tig/refs/%s", branch_name);
    read_ref(ref_path, commit_hash, sizeof(commit_hash));
    enumerate_commits(commit_hash);
}

void print_branches() {
    struct dirent *files;    
    DIR *ref_dir = opendir_safe("tig/refs");
    printf("Branch\tCommit\n===============\n");
    while((files = readdir(ref_dir)) != NULL) {
        if(strcmp(files->d_name, ".") == 0 || strcmp(files->d_name, "..") == 0) continue;
        char commit_hash[41];
        char filepath[128];
        sprintf(filepath, "tig/refs/%s", files->d_name);
        read_ref(filepath, commit_hash, sizeof(commit_hash));
        printf("%s\t%.6s\n", files->d_name, commit_hash);
    }
}

void initialize_repository() {
    char hash[41];
    mkdir_safe("tig", 0);
    mkdir_safe("tig/objects", 0);
    mkdir_safe("tig/refs", 0);
    write_ref("tig/refs/master", "root\n");
    write_config();
    write_ref("tig/HEAD", "tig/refs/master\n");
    create_commit("init", hash);
    printf("INFO -- Repository initialized with commit hash %s\n", hash);
}
