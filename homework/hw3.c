/**
 * 功能：实现 ls -l 基本功能（文件类型、权限、大小、文件名）
 * 
 * 支持选项：-a（显示隐藏文件） -l（长格式输出）
 * 未指定路径时默认列出当前目录 "."
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>   // lstat, struct stat
#include <unistd.h>     // getopt, getpwuid, getgrgid
#include <dirent.h>     // opendir, readdir, DIR
#include <pwd.h>        // struct passwd
#include <grp.h>        // struct group
#include <string.h>     // strncpy, strerror
#include <errno.h>      // errno

#define MAX_FILE_COUNT 1024      // 最多支持的命令行路径数量
#define MAX_PATH_LEN   4096      // 路径字符串最大长度

#define OPT_A (1 << 0)           // -a 选项标志位
#define OPT_L (1 << 1)           // -l 选项标志位

// 检查 flag 是否包含某选项
#define HAS_OPT(flag, opt) (((flag)&(opt))!=0)

void parse_args(int argc, char* argv[], char* files[], int* file_count, int* flag);
void mode_to_str(mode_t mode, char *str);
const char* get_user_name(uid_t uid);
const char* get_group_name(gid_t gid);
void print_entry_long_format(const char* entry_name, struct stat* st);
void print_entry(const char* dir_path, const char* entry_name, int flag);
void list_dir(const char* dir_path, int flag);

/**
 * 解析命令行参数，提取选项和路径列表
 * @param argc       main的argc
 * @param argv       main的argv
 * @param files      输出参数：存储路径字符串指针的数组
 * @param file_count 输出参数：有效路径数量
 * @param flag       输出参数：选项掩码（OPT_A / OPT_L）
 */
void parse_args(int argc, char* argv[], char* files[], int* file_count, int* flag) {
    *file_count = 0;
    *flag = 0;

    int opt = 0;
    // getopt 自动处理 "-al" 合并写法，第三个参数 "al" 表示接受 -a 和 -l
    while ((opt = getopt(argc, argv, "al")) != -1) {
        switch (opt) {
            case 'a':
                *flag |= OPT_A;
                break;
            case 'l':
                *flag |= OPT_L;
                break;
            default:    // 遇到非法选项（如 -x）
                fprintf(stderr, "valid options: -a|-l");
                exit(EXIT_FAILURE);
        }
    }

    // 剩余的非选项参数都是要列出的路径
    for (int i = optind; i < argc && *file_count < MAX_FILE_COUNT; i++) {
        files[(*file_count)++] = argv[i];
    }
}

/**
 * 将 mode_t 权限位转换为 ls 风格的字符串（10字符 + '\0'）
 * @param mode  st_mode 值
 * @param str   输出缓冲区（至少11字节）
 */
void mode_to_str(mode_t mode, char *str) {
    // ----- 文件类型（第1字符）-----
    if (S_ISREG(mode))      str[0] = '-';   // 普通文件
    else if (S_ISDIR(mode)) str[0] = 'd';   // 目录
    else if (S_ISLNK(mode)) str[0] = 'l';   // 符号链接
    else if (S_ISCHR(mode)) str[0] = 'c';   // 字符设备
    else if (S_ISBLK(mode)) str[0] = 'b';   // 块设备
    else if (S_ISFIFO(mode))str[0] = 'p';   // 管道
    else if (S_ISSOCK(mode))str[0] = 's';   // socket
    else                    str[0] = '?';   // 未知

    // ----- 所有者权限（第2-4字符）-----
    str[1] = (mode & S_IRUSR) ? 'r' : '-';   // 读
    str[2] = (mode & S_IWUSR) ? 'w' : '-';   // 写
    str[3] = (mode & S_IXUSR) ? 'x' : '-';   // 执行

    // ----- 组用户权限（第5-7字符）-----
    str[4] = (mode & S_IRGRP) ? 'r' : '-';
    str[5] = (mode & S_IWGRP) ? 'w' : '-';
    str[6] = (mode & S_IXGRP) ? 'x' : '-';

    // ----- 其他用户权限（第8-10字符）-----
    str[7] = (mode & S_IROTH) ? 'r' : '-';
    str[8] = (mode & S_IWOTH) ? 'w' : '-';
    str[9] = (mode & S_IXOTH) ? 'x' : '-';

    str[10] = '\0';  // 字符串终止符
}

/**
 * 根据 uid 获取用户名
 * @param uid 用户ID
 * @return 用户名字符串，若查找失败返回 "?"
 */
const char* get_user_name(uid_t uid) {
    struct passwd *pw = getpwuid(uid);
    return pw ? pw->pw_name : "?";
}

/**
 * 根据 gid 获取组名
 * @param gid 组ID
 * @return 组名字符串，若查找失败返回 "?"
 */
const char* get_group_name(gid_t gid) {
    struct group *gr = getgrgid(gid);
    return gr ? gr->gr_name : "?";
}

/**
 * 长格式输出单个文件信息（权限、拥有者、组、大小、文件名）
 * @param entry_name  文件名（不包含路径）
 * @param st          已获取的 struct stat
 */
void print_entry_long_format(const char* entry_name, struct stat* st) {
    char mode[11];
    mode_to_str(st->st_mode, mode);

    const char* user = get_user_name(st->st_uid);
    const char* group = get_group_name(st->st_gid);

    // 格式: 权限 用户名 组名 大小 文件名\n
    printf("%s %s %s %ld %s\n", mode, user, group, st->st_size, entry_name);
}

/**
 * 根据选项输出一个文件/目录项（可能是简洁名称或长格式）
 * @param dir_path   项所在的目录路径（为 NULL 表示 entry_name 是绝对/相对路径）
 * @param entry_name 文件名或完整路径
 * @param flag       选项掩码
 */
void print_entry(const char* dir_path, const char* entry_name, int flag) {
    char path[MAX_PATH_LEN];
    // 构造完整路径用于 lstat
    if (dir_path) {
        snprintf(path, sizeof(path), "%s/%s", dir_path, entry_name);
    } else {
        // 当 dir_path 为 NULL 时，entry_name 本身就是路径
        strncpy(path, entry_name, sizeof(path) - 1);
        path[sizeof(path) - 1] = '\0';   // 确保终止
    }

    struct stat st;
    if (lstat(path, &st) == -1) {
        perror(entry_name);   // 输出错误信息
        return;
    }

    if (HAS_OPT(flag, OPT_L)) {
        print_entry_long_format(entry_name, &st);
    } else {
        printf("%s \n", entry_name);     // 简洁模式，加空格和换行便于阅读
    }
}

/**
 * 列出目录下的所有条目（根据 -a 选项决定是否包含隐藏文件）
 * @param dir_path 目录路径
 * @param flag     选项掩码
 */
void list_dir(const char* dir_path, int flag) {
    DIR *dir = opendir(dir_path);
    if (!dir) {
        fprintf(stderr, "myls: cannot access '%s': %s\n", dir_path, strerror(errno));
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // 如果未开启 -a 且文件名以 '.' 开头，则跳过
        if (!HAS_OPT(flag, OPT_A) && entry->d_name[0] == '.') {
            continue;
        }
        print_entry(dir_path, entry->d_name, flag);
    }

    closedir(dir);
}

int main(int argc, char *argv[]) {
    int flag = 0;
    int file_count = 0;
    char* files[MAX_FILE_COUNT];

    parse_args(argc, argv, files, &file_count, &flag);

    // 若无指定路径，默认列出当前目录
    if (file_count == 0) {
        files[0] = ".";
        file_count = 1;
    }

    // 逐个处理每个路径
    for (int i = 0; i < file_count; ++i) {
        struct stat st;
        if (lstat(files[i], &st) == -1) {
            perror(files[i]);
            continue;
        }

        if (S_ISDIR(st.st_mode)) {
            // 目录：遍历其内容
            list_dir(files[i], flag);
        } else {
            // 普通文件或其他类型文件：直接输出
            print_entry(NULL, files[i], flag);
        }
    }

    return 0;
}
