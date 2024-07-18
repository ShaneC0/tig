# tig

## Resources:
- [The Git Parable by Tom Preston-Werner](https://tom.preston-werner.com/2009/05/19/the-git-parable.html)
- [Pro Git Book: Git Internals - Plumbing and Porcelain](https://git-scm.com/book/en/v2/Git-Internals-Plumbing-and-Porcelain)

## Usage: `tig [options]`
### Options:
- `-i, --init`  
  Initialize a new repository
- `-c, --commit <message>`  
  Create a new commit with the given message
- `-b, --create-branch <name>`  
  Create a new branch with the given name
- `-s, --switch-branch <name>`  
  Switch to the branch with the given name
- `-x, --commit-history <name>`  
  Show the commit history for the given branch
- `-l, --list-branch`  
  Show the list of branches with latest commit hashes
- `-h, --help`  
  Display this help message and exit

## How does it work?

### Filesystem Structure
The filesystem is organized as follows:
- `objects/`  
  Stores all objects (commits, trees, blobs).
- `refs/`  
  Contains references to commit objects, such as branches and tags.
- `HEAD`  
  Points to the current branch.
- `.tigconfig`  
  Configuration file for `tig`.

### Plumbing vs. Porcelain
- **Plumbing**: These are the low-level commands that provide the core functionality.
- **Porcelain**: These are the high-level commands that are more user-friendly and abstract the complexity of the plumbing commands.

### Initialization
The `tig --init` command sets up a new repository by creating the necessary directory structure and initial files:
- Creates the `objects/` and `refs/` directories.
- Sets up the `HEAD` file to point to the default branch (e.g., `master` or `main`).
- Optionally initializes the `.tigconfig` file with default settings.

### Creating Commits
The `tig --commit <message>` command creates a new commit:
- Stages the current state of the working directory.
- Writes the commit object to the `objects/` directory.
- Updates the current branch reference in `refs/`.

### Creating Branches
The `tig --create-branch <name>` command creates a new branch:
- Adds a new reference in the `refs/` directory for the branch.
- Points the new branch to the current commit.

### Switching Branches
The `tig --switch-branch <name>` command switches to a different branch:
- Updates the `HEAD` file to point to the specified branch.
- Resets the working directory to the state of the commit pointed to by the branch.

### Viewing Commit History
The `tig --commit-history <name>` command shows the commit history for the specified branch:
- Traverses the commit graph from the branch reference.
- Displays the commit messages in a readable format.

### Listing Branches
The `tig --list-branch` command shows the list of branches with their latest commit hashes:
- Iterates through the references in the `refs/` directory.
- Displays the branch names and the corresponding commit hashes.

### Merging -- WIP
Merging requires some interesting plumbing algorithms to implement, namely Least Common Ancestor and Diff.
- Get the tree hash of current branch and specified branch
- Find the least common ancestor commit.
- Calc file diff from LCM to current branch.
- Calc file diff from LCM to specified branch.
- If both branches modify the same file:
    - Calc linewise diff from current to specified on that file
    - If both branches modify the same line
        - MERGE CONFLICT
    - Apply linewise diff
- Apply filewise diff

### Least Common Ancestor
The LCM algorithm works by performing a Breadth-First Search on both commit trees and returns the first
commit which is common to both branches.

### Diff
The Diff algorithm takes two sets of input (lines or files) and returns the longest subsequence that can be produced from both.
