#include <getopt.h>
#include <stdio.h>
#include "porcelain.h"

void print_help() {
    printf("Usage: tig [options]\n");
    printf("Options:\n");
    printf("  -i, --init                     Initialize a new repository\n");
    printf("  -c, --commit <message>         Create a new commit with the given message\n");
    printf("  -b, --create-branch <name>     Create a new branch with the given name\n");
    printf("  -s, --switch-branch <name>     Switch to the branch with the given name\n");
    printf("  -x, --commit-history <name>    Show the commit history for the given branch\n");
    printf("  -l, --list-branch              Show the list of branches with latest commit hashes\n");
    printf("  -m, --merge <name>             TODO\n");
    printf("  -r, --rebase <name>            TODO\n");
    printf("  -h, --help                     Display this help message and exit\n");
}

int main(int argc, char **argv) {
    int init_flag = 0;
    int commit_flag = 0;
    int create_branch_flag = 0;
    int switch_branch_flag = 0;
    int commit_history_flag = 0;
    int list_branch_flag = 0;
    int merge_flag = 0;
    int rebase_flag = 0;
    int diff_flag = 0;
    int help_flag = 0;

    char *commit_msg = NULL;
    char *branch_name = NULL;
    char *file_path = NULL;
    int c;

    struct option long_options[] = {
        {"init",           no_argument,       0,  'i' },
        {"commit",         required_argument, 0,  'c' },
        {"create-branch",  required_argument, 0,  'b' },
        {"switch-branch",  required_argument, 0,  's' },
        {"commit-history", required_argument, 0,  'x' },
        {"list-branch",    no_argument,       0,  'l' },
        {"merge",          required_argument, 0,  'm'},
        {"rebase",         required_argument, 0,  'r'},
        {"diff",           required_argument, 0,  'd'},
        {"help",           no_argument,       0,  'h'},
        {0,                0,                 0,  0   }
    };

    while((c = getopt_long(argc, argv, "ic:b:s:x:lhm:r:d:", long_options, NULL)) != -1) {
        switch(c) {
            case 'i':
                init_flag = 1;
                break;
            case 'c':
                commit_flag = 1;
                commit_msg = optarg;
                break;
            case 'b':
                create_branch_flag = 1;
                branch_name = optarg;
                break;
            case 's':
                switch_branch_flag = 1;
                branch_name = optarg;
                break;
            case 'x':
                commit_history_flag = 1;
                branch_name = optarg;
                break;
            case 'l':
                list_branch_flag = 1; 
                break;
            case 'r':
                rebase_flag = 1;
                branch_name = optarg;
                break;
            case 'm':
                merge_flag = 1;
                branch_name = optarg;
                break;
            case 'd':
                diff_flag = 1;
                file_path = optarg;
                break;
            case 'h':
                help_flag = 1;
                break;
            default:
                print_help();
                abort();
        }
    }

    if(init_flag) {
        initialize_repository();
    } else if(commit_flag) {
        char commit_hash[41];
        create_commit(commit_msg, commit_hash);
    } else if(create_branch_flag) {
        create_branch(branch_name);
    } else if(switch_branch_flag) {
        switch_branch(branch_name);
    } else if(commit_history_flag) {
        print_commit_history(branch_name);
    } else if(list_branch_flag) {
        print_branches();
    } else if(help_flag) {
        print_help();
    } else if(diff_flag) {
        print_diff(file_path);
    }

    return 0;
}

