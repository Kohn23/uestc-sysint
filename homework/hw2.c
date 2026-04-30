/*
* 功能：模仿 tree (windows) 命令，
* 以缩进树形结构打印指定目录下所有文件和子目录
*/

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>

/**
 * 判断当前目录的 readdir 指针后面是否还有有效条目（即非 "." 和 ".." 的项）
 * @param dir 已打开的 DIR 指针
 * @return 1 表示当前是最后一个有效条目（后面没有了），0 表示后面还有
 */
int is_last_entry(DIR *dir) {
    long pos = telldir(dir);          // 保存当前读取位置
    struct dirent *next;
    int last = 1;
    while ((next = readdir(dir)) != NULL) {
        if (strcmp(next->d_name, ".") != 0 && strcmp(next->d_name, "..") != 0) {
            last = 0;                 // 发现有后续有效条目
            break;
        }
    }
    seekdir(dir, pos);                // 恢复原来的读取位置
    return last;
}

/**
 * 递归列出目录内容
 * @param path        目录路径
 * @param level       当前深度（根目录深度为1，每次递归加1）
 * @param parent_last 父目录传递下来的掩码，记录哪些层祖先已经是父目录的最后一个子项
 */
void list_dir(const char *path, int level, int parent_last) {
    DIR *dir = opendir(path);
    if (!dir) {
        perror("opendir");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // 忽略当前目录项和父目录项，避免无限循环
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        // 打印缩进：根据 parent_last 位掩码判断是否打印竖线
        // 从第 1 层循环到当前层 level，每一层独立决定输出 "   |" 还是 "    "
        for (int i = 1; i <= level; i++) {
            if ((parent_last & (1 << i)) == 0)   // 该层不是最后一个子项，需要竖线
                printf("   |");
            else                                 // 是最后一个子项，只输出空格
                printf("    ");
        }

        // 根据文件类型进行不同输出和递归
        if (entry->d_type == DT_DIR) {
            // 目录：前面加连字符，并递归进入
            printf("- %s/\n", entry->d_name);

            char subpath[1024];
            snprintf(subpath, sizeof(subpath), "%s/%s", path, entry->d_name);

            // 计算传给子目录的掩码：先继承父掩码，如果当前条目是最后一个，则置位本层
            int child_last = parent_last;
            if (is_last_entry(dir))
                child_last |= (1 << level);

            list_dir(subpath, level + 1, child_last);
        } else if (entry->d_type == DT_REG) {
            // 普通文件：只打印名称，前面两个空格
            printf("  %s\n", entry->d_name);
        } else {
            // 其他类型（如链接、设备等），也按文件方式输出
            printf("  %s\n", entry->d_name);
        }
    }
    closedir(dir);
}

/**
 * 主函数：解析命令行参数，启动目录树打印
 * @param argc 参数个数
 * @param argv 参数数组
 * @return 0 表示正常结束
 */
int main(int argc, char *argv[]) {
    // 如果提供了命令行参数，则使用第一个参数作为起始目录；否则使用当前目录 "."
    const char *dir = (argc > 1) ? argv[1] : ".";
    // 打印根目录标记，格式同 tree 命令
    printf(".:%s/\n", dir);
    // 从深度1开始递归遍历，初始掩码为0（根目录没有祖先限制）
    list_dir(dir, 1, 0);
    return 0;
}
